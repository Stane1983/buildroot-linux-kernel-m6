/*
 * linux/drivers/amlogic/i2c/aml_i2c.c
 */

#include <asm/errno.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <plat/io.h>
#include <linux/i2c-aml.h>
#include <linux/i2c-algo-bit.h>
#include <linux/hardirq.h>

#include <mach/am_regs.h>

#include "aml_i2c.h"

static int no_stop_flag = 0;
static int i2c_silence = 0;

#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON3
static  struct mutex aml_i2c_xfer_lock;	//add by sz.wu.zhu 20111017
#endif

static void aml_i2c_set_clk(struct aml_i2c *i2c, unsigned int speed)
{
	unsigned int i2c_clock_set;
	unsigned int sys_clk_rate;
	struct clk *sys_clk;
	struct aml_i2c_reg_ctrl* ctrl;

	sys_clk = clk_get_sys("clk81", NULL);
	sys_clk_rate = clk_get_rate(sys_clk);
	//sys_clk_rate = get_mpeg_clk();

	i2c_clock_set = sys_clk_rate / speed;
	i2c_clock_set >>= 2;

	ctrl = (struct aml_i2c_reg_ctrl*)&(i2c->master_regs->i2c_ctrl);
	ctrl->clk_delay = i2c_clock_set & AML_I2C_CTRL_CLK_DELAY_MASK;
}

static void aml_i2c_set_platform_data(struct aml_i2c *i2c,
										struct aml_i2c_platform *plat)
{
	i2c->master_i2c_speed = plat->master_i2c_speed;
	i2c->wait_count = plat->wait_count;
	i2c->wait_ack_interval = plat->wait_ack_interval;
	i2c->wait_read_interval = plat->wait_read_interval;
	i2c->wait_xfer_interval = plat->wait_xfer_interval;

      if(plat->master_pinmux.pinmux){
        	i2c->master_pinmux=plat->master_pinmux;
      }
      else if(i2c->master_no == MASTER_A){
		i2c->master_pinmux=plat->master_a_pinmux;
	}
	else {
		i2c->master_pinmux=plat->master_b_pinmux;
	}
}

static void aml_i2c_pinmux_master(struct aml_i2c *i2c)
{
	pinmux_set(&i2c->master_pinmux);
}

/*set to gpio for -EIO & -ETIMEOUT?*/
static void aml_i2c_clr_pinmux(struct aml_i2c *i2c)
{
    pinmux_clr(&i2c->master_pinmux);
}


static void aml_i2c_dbg(struct aml_i2c *i2c)
{
	int i;
	struct aml_i2c_reg_ctrl* ctrl;
	unsigned int sys_clk_rate;
	struct clk *sys_clk;

	if(i2c->i2c_debug == 0)
		return ;

      printk("addr [%x]  \t token tag : ",
                i2c->master_regs->i2c_slave_addr>>1);
      	for(i=0; i<AML_I2C_MAX_TOKENS; i++)
		printk("%d,", i2c->token_tag[i]);

	sys_clk = clk_get_sys("clk81", NULL);
	sys_clk_rate = clk_get_rate(sys_clk);
	ctrl = ((struct aml_i2c_reg_ctrl*)&(i2c->master_regs->i2c_ctrl));
      printk("clk_delay %x,  clk is %dK \n", ctrl->clk_delay,
            sys_clk_rate/4/ctrl->clk_delay/1000);
      printk("w0 %x, w1 %x, r0 %x, r1 %x, cur_token %d, rd cnt %d, status %d,"
            "error %d, ack_ignore %d,start %d\n",
            i2c->master_regs->i2c_token_wdata_0,
            i2c->master_regs->i2c_token_wdata_1,
            i2c->master_regs->i2c_token_rdata_0,
            i2c->master_regs->i2c_token_rdata_1,
            ctrl->cur_token, ctrl->rd_data_cnt, ctrl->status, ctrl->error,
            ctrl->ack_ignore, ctrl->start);

      if(ctrl->manual_en)
            printk("[aml_i2c_dbg] manual_en, why?\n");
}

static void aml_i2c_clear_token_list(struct aml_i2c *i2c)
{
	i2c->master_regs->i2c_token_list_0 = 0;
	i2c->master_regs->i2c_token_list_1 = 0;
	memset(i2c->token_tag, TOKEN_END, AML_I2C_MAX_TOKENS);
}

static void aml_i2c_set_token_list(struct aml_i2c *i2c)
{
	int i;
	unsigned int token_reg=0;

	for(i=0; i<AML_I2C_MAX_TOKENS; i++)
		token_reg |= i2c->token_tag[i]<<(i*4);

	i2c->master_regs->i2c_token_list_0=token_reg;
}

/*poll status*/
static int aml_i2c_wait_ack(struct aml_i2c *i2c)
{
	int i;
	struct aml_i2c_reg_ctrl* ctrl;

	for(i=0; i<i2c->wait_count; i++) {
		udelay(i2c->wait_ack_interval);
		ctrl = (struct aml_i2c_reg_ctrl*)&(i2c->master_regs->i2c_ctrl);
		if(ctrl->status == IDLE){
			if(ctrl->error)
			{
			      i2c->cur_token = ctrl->cur_token;
				/*set to gpio, more easier to fail again*/
				//aml_i2c_clr_pinmux(i2c);
				no_stop_flag = 1; /*controler will auto send stop in this case,
				                    The CLK line will be polled if software
				                    send stop again */
				return -EIO;
			}
			else
				return 0;
		}

		if(!in_atomic())
			cond_resched();
	}

	/*
	  * dangerous -ETIMEOUT, set to gpio here,
	  * set pinxmux again in next i2c_transfer in xfer_prepare
	  */
	aml_i2c_clr_pinmux(i2c);
	return -ETIMEDOUT;
}

static void aml_i2c_get_read_data(struct aml_i2c *i2c, unsigned char *buf,
														size_t len)
{
	int i;
	unsigned long rdata0 = i2c->master_regs->i2c_token_rdata_0;
	unsigned long rdata1 = i2c->master_regs->i2c_token_rdata_1;

	for(i=0; i< min_t(size_t, len, AML_I2C_MAX_TOKENS>>1); i++)
		*buf++ = (rdata0 >> (i*8)) & 0xff;

	for(; i< min_t(size_t, len, AML_I2C_MAX_TOKENS); i++)
		*buf++ = (rdata1 >> ((i - (AML_I2C_MAX_TOKENS>>1))*8)) & 0xff;
}

static void aml_i2c_fill_data(struct aml_i2c *i2c, unsigned char *buf,
							size_t len)
{
	int i;
	unsigned int wdata0 = 0;
	unsigned int wdata1 = 0;

	for(i=0; i< min_t(size_t, len, AML_I2C_MAX_TOKENS>>1); i++)
		wdata0 |= (*buf++) << (i*8);

	for(; i< min_t(size_t, len, AML_I2C_MAX_TOKENS); i++)
		wdata1 |= (*buf++) << ((i - (AML_I2C_MAX_TOKENS>>1))*8);

	i2c->master_regs->i2c_token_wdata_0 = wdata0;
	i2c->master_regs->i2c_token_wdata_1 = wdata1;
}

static void aml_i2c_xfer_prepare(struct aml_i2c *i2c, unsigned int speed)
{
    if(i2c_silence) return;
    no_stop_flag = 0;
	aml_i2c_pinmux_master(i2c);
	aml_i2c_set_clk(i2c, speed);
}

static void aml_i2c_start_token_xfer(struct aml_i2c *i2c)
{
	//((struct aml_i2c_reg_ctrl*)&(i2c->master_regs->i2c_ctrl))->start = 0;	/*clear*/
	//((struct aml_i2c_reg_ctrl*)&(i2c->master_regs->i2c_ctrl))->start = 1;	/*set*/
	i2c->master_regs->i2c_ctrl &= ~1;	/*clear*/
	i2c->master_regs->i2c_ctrl |= 1;	/*set*/

	udelay(i2c->wait_xfer_interval);
}

/*our controller should send write data with slave addr in a token list,
	so we can't do normal address, just set addr into addr reg*/
static int aml_i2c_do_address(struct aml_i2c *i2c, unsigned int addr)
{
    if(i2c_silence) return 0;
	i2c->cur_slave_addr = addr&0x7f;
	i2c->master_regs->i2c_slave_addr = i2c->cur_slave_addr<<1;

	return 0;
}

static void aml_i2c_stop(struct aml_i2c *i2c)
{
	if (no_stop_flag) {
		aml_i2c_clear_token_list(i2c);
		aml_i2c_clr_pinmux(i2c);
		return;
	}
    if(i2c_silence) return 0;
	aml_i2c_clear_token_list(i2c);
	i2c->token_tag[0]=TOKEN_STOP;
	aml_i2c_set_token_list(i2c);
	aml_i2c_start_token_xfer(i2c);
	aml_i2c_clr_pinmux(i2c);
}

static int aml_i2c_read(struct aml_i2c *i2c, unsigned char *buf,
							size_t len)
{
	int i;
	int ret;
	size_t rd_len;
	int tagnum=0;

	if(!buf || !len) return -EINVAL; 
	aml_i2c_clear_token_list(i2c);

	if(! (i2c->msg_flags & I2C_M_NOSTART)){
		i2c->token_tag[tagnum++]=TOKEN_START;
		i2c->token_tag[tagnum++]=TOKEN_SLAVE_ADDR_READ;

		aml_i2c_set_token_list(i2c);
		aml_i2c_dbg(i2c);
		aml_i2c_start_token_xfer(i2c);

		udelay(i2c->wait_ack_interval);

		ret = aml_i2c_wait_ack(i2c);
		if(ret<0)
			return ret;
		aml_i2c_clear_token_list(i2c);
	}

	while(len){
		tagnum = 0;
		rd_len = min_t(size_t, len, AML_I2C_MAX_TOKENS);
		if(rd_len == 1)
			i2c->token_tag[tagnum++]=TOKEN_DATA_LAST;
		else{
			for(i=0; i<rd_len-1; i++)
				i2c->token_tag[tagnum++]=TOKEN_DATA;
			if(len > rd_len)
				i2c->token_tag[tagnum++]=TOKEN_DATA;
			else
				i2c->token_tag[tagnum++]=TOKEN_DATA_LAST;
		}
		aml_i2c_set_token_list(i2c);
		aml_i2c_dbg(i2c);
		aml_i2c_start_token_xfer(i2c);

		udelay(i2c->wait_ack_interval);

		ret = aml_i2c_wait_ack(i2c);
		if(ret<0)
			return ret;

		aml_i2c_get_read_data(i2c, buf, rd_len);
		len -= rd_len;
		buf += rd_len;

		aml_i2c_dbg(i2c);
		udelay(i2c->wait_read_interval);
		aml_i2c_clear_token_list(i2c);
	}
	return 0;
}

static int aml_i2c_write(struct aml_i2c *i2c, unsigned char *buf,
							size_t len)
{
        int i;
        int ret;
        size_t wr_len;
	int tagnum=0;
    if(i2c_silence) return 0;
	if(!buf || !len) return -EINVAL; 
	aml_i2c_clear_token_list(i2c);
	if(! (i2c->msg_flags & I2C_M_NOSTART)){
		i2c->token_tag[tagnum++]=TOKEN_START;
		i2c->token_tag[tagnum++]=TOKEN_SLAVE_ADDR_WRITE;
	}
	while(len){
		wr_len = min_t(size_t, len, AML_I2C_MAX_TOKENS-tagnum);
		for(i=0; i<wr_len; i++)
			i2c->token_tag[tagnum++]=TOKEN_DATA;

		aml_i2c_set_token_list(i2c);

		aml_i2c_fill_data(i2c, buf, wr_len);

		aml_i2c_dbg(i2c);
		aml_i2c_start_token_xfer(i2c);

		len -= wr_len;
		buf += wr_len;
		tagnum = 0;

		ret = aml_i2c_wait_ack(i2c);
		if(ret<0)
			return ret;

		aml_i2c_clear_token_list(i2c);
    	}
	return 0;
}

static struct aml_i2c_ops aml_i2c_m1_ops = {
	.xfer_prepare 	= aml_i2c_xfer_prepare,
	.read 		= aml_i2c_read,
	.write 		= aml_i2c_write,
	.do_address	= aml_i2c_do_address,
	.stop		= aml_i2c_stop,
};

/*General i2c master transfer*/
static int aml_i2c_xfer(struct i2c_adapter *i2c_adap, struct i2c_msg *msgs,
							int num)
{
	struct aml_i2c *i2c = i2c_get_adapdata(i2c_adap);
	struct i2c_msg * p=NULL;
	unsigned int i;
	unsigned int ret=0;
///	struct aml_i2c_reg_ctrl* ctrl;
///	struct aml_i2c_reg_master __iomem* regs = i2c->master_regs;

	BUG_ON(!i2c);
	/*should not use spin_lock, cond_resched in wait ack*/
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON3
	if((AML_I2C_MASTER_A ==i2c->master_no) || (AML_I2C_MASTER_B ==i2c->master_no)){
		mutex_lock(&aml_i2c_xfer_lock);
	}else{
		mutex_lock(&i2c->lock);
	}
#else
	mutex_lock(&i2c->lock);
#endif

	i2c->ops->xfer_prepare(i2c, i2c->master_i2c_speed);
	/*make sure change speed before start*/
    mb();

	for (i = 0; !ret && i < num; i++) {
		p = &msgs[i];
		i2c->msg_flags = p->flags;
		ret = i2c->ops->do_address(i2c, p->addr);
		if (ret || !p->len)
			continue;
		if (p->flags & I2C_M_RD)
			ret = i2c->ops->read(i2c, p->buf, p->len);
		else
			ret = i2c->ops->write(i2c, p->buf, p->len);
	}

	i2c->ops->stop(i2c);

	AML_I2C_PRINT_DATA(i2c->adap.name);

	/* Return the number of messages processed, or the error code*/
	if (ret == 0){
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON3
		if((AML_I2C_MASTER_A ==i2c->master_no) || (AML_I2C_MASTER_B ==i2c->master_no)){
			mutex_unlock(&aml_i2c_xfer_lock);
		}else{
			mutex_unlock(&i2c->lock);
		}
#else
		mutex_unlock(&i2c->lock);
#endif
		return num;
       }
	else {
		dev_err(&i2c_adap->dev, "[aml_i2c_xfer] error ret = %d (%s) token %d, "
                   "master_no(%d) %dK addr 0x%x\n",
                   ret, ret == -EIO ? "-EIO" : "-ETIMEOUT", i2c->cur_token,
			i2c->master_no, i2c->master_i2c_speed/1000,
			i2c->cur_slave_addr);
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON3
		if((AML_I2C_MASTER_A ==i2c->master_no) || (AML_I2C_MASTER_B ==i2c->master_no)){
			mutex_unlock(&aml_i2c_xfer_lock);
		}else{
			mutex_unlock(&i2c->lock);
		}
#else
		mutex_unlock(&i2c->lock);
#endif
		return -EAGAIN;
	}
}

/*General i2c master transfer , speed set by i2c->master_i2c_speed2*/
static int aml_i2c_xfer_s2(struct i2c_adapter *i2c_adap, struct i2c_msg *msgs,
							int num)
{
	struct aml_i2c *i2c = i2c_get_adapdata(i2c_adap);
	struct i2c_msg * p=NULL;
	unsigned int i;
	unsigned int ret=0;

	BUG_ON(!i2c);
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON3
	if((AML_I2C_MASTER_A ==i2c->master_no) || (AML_I2C_MASTER_B ==i2c->master_no)){
		mutex_lock(&aml_i2c_xfer_lock);
	}else{
		mutex_lock(&i2c->lock);
	}
#else
	mutex_lock(&i2c->lock);
#endif
	BUG_ON(!i2c->master_i2c_speed2);
	i2c->ops->xfer_prepare(i2c, i2c->master_i2c_speed2);
    mb();

	for (i = 0; !ret && i < num; i++) {
		p = &msgs[i];
		i2c->msg_flags = p->flags;
		ret = i2c->ops->do_address(i2c, p->addr);
		if (ret || !p->len)
			continue;
		if (p->flags & I2C_M_RD)
			ret = i2c->ops->read(i2c, p->buf, p->len);
		else
			ret = i2c->ops->write(i2c, p->buf, p->len);
	}

	i2c->ops->stop(i2c);

	AML_I2C_PRINT_DATA(i2c->adap2.name);

	/* Return the number of messages processed, or the error code*/
	if (ret == 0){
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON3
		if((AML_I2C_MASTER_A ==i2c->master_no) || (AML_I2C_MASTER_B ==i2c->master_no)){
			mutex_unlock(&aml_i2c_xfer_lock);
		}else{
			mutex_unlock(&i2c->lock);
		}
#else
		mutex_unlock(&i2c->lock);
#endif
		return num;
      }
	else {
		dev_err(&i2c_adap->dev, "[aml_i2c_xfer_s2] error ret = %d (%s) token %d\t"
                   "master_no(%d)  %dK addr 0x%x\n",
                   ret, ret == -EIO ? "-EIO" : "-ETIMEOUT", i2c->cur_token,
			i2c->master_no, i2c->master_i2c_speed2/1000,
			i2c->cur_slave_addr);
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON3
		if((AML_I2C_MASTER_A ==i2c->master_no) || (AML_I2C_MASTER_B ==i2c->master_no)){
			mutex_unlock(&aml_i2c_xfer_lock);
		}else{
			mutex_unlock(&i2c->lock);
		}
#else
		mutex_unlock(&i2c->lock);
#endif
		return -EAGAIN;
	}
}
static u32 aml_i2c_func(struct i2c_adapter *i2c_adap)
{
	return I2C_FUNC_I2C |I2C_FUNC_SMBUS_EMUL;
}

static const struct i2c_algorithm aml_i2c_algorithm = {
    .master_xfer = aml_i2c_xfer,
    .functionality = aml_i2c_func,
};

static const struct i2c_algorithm aml_i2c_algorithm_s2 = {
    .master_xfer = aml_i2c_xfer_s2,
    .functionality = aml_i2c_func,
};

/***************i2c class****************/

static ssize_t show_i2c_debug(struct class *class,
                    struct class_attribute *attr,	char *buf)
{
    struct aml_i2c *i2c = container_of(class, struct aml_i2c, cls);
    return sprintf(buf, "i2c debug is 0x%x\n", i2c->i2c_debug);
}

static ssize_t store_i2c_debug(struct class *class,
                    struct class_attribute *attr,	const char *buf, size_t count)
{
    unsigned int dbg;
    ssize_t r;
    struct aml_i2c *i2c = container_of(class, struct aml_i2c, cls);

    r = sscanf(buf, "%d", &dbg);
    if (r != 1)
        return -EINVAL;

    i2c->i2c_debug = dbg;
    return count;
}

static ssize_t show_i2c_info(struct class *class,
                    struct class_attribute *attr,	char *buf)
{
    struct aml_i2c *i2c = container_of(class, struct aml_i2c, cls);
    struct aml_i2c_reg_ctrl* ctrl;
    struct aml_i2c_reg_master __iomem* regs = i2c->master_regs;

    printk( "i2c master_no(%d) current slave addr is 0x%x\n",
        i2c->master_no, i2c->cur_slave_addr);
    printk( "wait ack timeout is 0x%x\n",
        i2c->wait_count * i2c->wait_ack_interval);
    printk( "master regs base is 0x%x \n", (unsigned int)regs);

    ctrl = ((struct aml_i2c_reg_ctrl*)&(i2c->master_regs->i2c_ctrl));
    printk( "i2c_ctrl:  0x%x\n", i2c->master_regs->i2c_ctrl);
    printk( "ctrl.rdsda  0x%x\n", ctrl->rdsda);
    printk( "ctrl.rdscl  0x%x\n", ctrl->rdscl);
    printk( "ctrl.wrsda  0x%x\n", ctrl->wrsda);
    printk( "ctrl.wrscl  0x%x\n", ctrl->wrscl);
    printk( "ctrl.manual_en  0x%x\n", ctrl->manual_en);
    printk( "ctrl.clk_delay  0x%x\n", ctrl->clk_delay);
    printk( "ctrl.rd_data_cnt  0x%x\n", ctrl->rd_data_cnt);
    printk( "ctrl.cur_token  0x%x\n", ctrl->cur_token);
    printk( "ctrl.error  0x%x\n", ctrl->error);
    printk( "ctrl.status  0x%x\n", ctrl->status);
    printk( "ctrl.ack_ignore  0x%x\n", ctrl->ack_ignore);
    printk( "ctrl.start  0x%x\n", ctrl->start);

    printk( "i2c_slave_addr:  0x%x\n", regs->i2c_slave_addr);
    printk( "i2c_token_list_0:  0x%x\n", regs->i2c_token_list_0);
    printk( "i2c_token_list_1:  0x%x\n", regs->i2c_token_list_1);
    printk( "i2c_token_wdata_0:  0x%x\n", regs->i2c_token_wdata_0);
    printk( "i2c_token_wdata_1:  0x%x\n", regs->i2c_token_wdata_1);
    printk( "i2c_token_rdata_0:  0x%x\n", regs->i2c_token_rdata_0);
    printk( "i2c_token_rdata_1:  0x%x\n", regs->i2c_token_rdata_1);

    printk( "master pinmux\n");
    printk( "pinmux_reg:  0x%02x\n", i2c->master_pinmux.pinmux->reg);
    printk( "clrmask:  0x%08x\n", i2c->master_pinmux.pinmux->clrmask);
    printk( "setmask:  0x%08x\n", i2c->master_pinmux.pinmux->setmask);
    return 0;
}

static ssize_t store_register(struct class *class,
                    struct class_attribute *attr,	const char *buf, size_t count)
{
    unsigned int reg, val, ret;
    int n=1,i;
    if(buf[0] == 'w'){
        ret = sscanf(buf, "w %x %x", &reg, &val);
        //printk("sscanf w reg = %x, val = %x\n",reg, val);
        printk("write cbus reg 0x%x value %x\n", reg, val);
        aml_write_reg32(CBUS_REG_ADDR(reg), val);
    }else{
        ret =  sscanf(buf, "%x %d", &reg,&n);
        printk("read %d cbus register from reg: %x \n",n,reg);
        for(i=0;i<n;i++)
        {
            val = aml_read_reg32(CBUS_REG_ADDR(reg+i));
            printk("reg 0x%x : 0x%x\n", reg+i, val);
        }
    }

    if (ret != 1 || ret !=2)
        return -EINVAL;

    return 0;
}

static unsigned int clock81_reading(void)
{
	int val;

	val = aml_read_reg32(P_HHI_OTHER_PLL_CNTL);
	printk( "1070=%x\n", val);
	val = aml_read_reg32(P_HHI_MPEG_CLK_CNTL);
	printk( "105d=%x\n", val);
	return 148;
}

static ssize_t rw_special_reg(struct class *class,
                    struct class_attribute *attr,	const char *buf, size_t count)
{
    unsigned int id, val, ret;

    if(buf[0] == 'w'){
    ret = sscanf(buf, "w %x", &id);
    switch(id)
    {
        case 0:
            break;
        default:
            printk( "'echo h > customize' for help\n");
            break;
    }
    //printk("sscanf w reg = %x, val = %x\n",reg, val);
    //printk("write cbus reg 0x%x value %x\n", reg, val);
    //WRITE_CBUS_REG(reg, val);
    }
    else if(buf[0] == 'r'){
        ret =  sscanf(buf, "r %x", &id);
        switch(id)
        {
            case 0:
                val = clock81_reading();
                printk("Reading Value=%04d Mhz\n", val);
                break;
            default:
                printk( "'echo h > customize' for help\n");
                break;
        }
        //printk("sscanf r reg = %x\n", reg);
        //val = READ_CBUS_REG(reg);
        //printk("read cbus reg 0x%x value %x\n", reg, val);
    }else if(buf[0] == 'h'){
        printk( "Customize sys fs help\n");
        printk( "**********************************************************\n");
        printk( "This interface for customer special register value getting\n");
        printk( "echo w id > customize: for write the value to customer specified register\n");
        printk( "echo r id > customize: for read the value from customer specified register\n");
        printk( "reading ID: 0--for clock81 reading\n");
        printk( "writting ID: reserved currently \n");
        printk( "**********************************************************\n");
    }
    else
        printk( "'echo h > customize' for help\n");

    if (ret != 1 || ret !=2)
        return -EINVAL;

    return 0;
}

static ssize_t show_i2c_silence(struct class *class,
        struct class_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", i2c_silence);
}

static ssize_t store_i2c_silence(struct class *class,
        struct class_attribute *attr, const char *buf, size_t count)
{
    unsigned int dbg;
    ssize_t r;

    r = sscanf(buf, "%d", &dbg);
    if (r != 1)
    return -EINVAL;

    i2c_silence = dbg;
    return count;
}

static ssize_t test_slave_device(struct class *class,
                    struct class_attribute *attr,	const char *buf, size_t count)
{
 
    struct i2c_adapter *i2c_adap;
    struct aml_i2c *i2c;
    unsigned int bus_num=0, slave_addr=0, speed=0, wnum=0, rnum=0;
    u8 wbuf[4]={0}, rbuf[4]={0};
    int wret=1, rret=1, i;
    bool restart = 0;
    
    if (buf[0] == 'h') {
      printk("i2c slave test help\n");
      printk("You can test the i2c slave device even without its driver through this sysfs node\n");
      printk("echo bus_num slave_addr speed write_num read_num [wdata1 wdata2 wdata3 wdata4] >test_slave\n");
      printk("write (0x12 0x34) 2 bytes to the slave(addr=0x50) on i2c_bus 0, speed=50K\n");
      printk("  echo 0 0x50 50000 2 0 0x12 0x34 >test_slave\n");
      printk("read 2 bytes from the slave(addr=0x67) on i2c_bus 1, speed=300K\n");
      printk("  echo 1 0x67 300000 0 2 >test_slave\n");
      printk("write (0x12 0x34) 2 bytes to the slave(addr=0x50) on i2c_bus 0, then read 2 bytes, speed=50K\n");
      printk("  echo 0 0x50 50000 3 2 0x12 0x34 >test_slave \n");
      return count;
    }
    
    i = sscanf(buf, "%d%x%d%d%d%x%x%x%x", &bus_num, &slave_addr, &speed, &wnum, &rnum, 
      &wbuf[0], &wbuf[1], &wbuf[2], &wbuf[3]);
    restart = !!(rnum & 0x80);
    rnum &= 0x7f;
    printk("bus_num=%d, slave_addr=%x, speed=%d, wnum=%d, rnum=%d\n",
      bus_num, slave_addr, speed, wnum, rnum);
    if ((i<(wnum+5)) || (!slave_addr) || (!speed) ||(wnum>4) || (rnum>4) || (!(wnum | rnum))) {
      printk("invalid data\n");
      return -EINVAL;
    }

    i2c_adap = i2c_get_adapter(bus_num);
    if (!i2c_adap) {
      printk("invalid i2c adapter\n");      
      return -EINVAL;
    }
	  i2c= i2c_get_adapdata(i2c_adap);
    if (!i2c) {
      printk("invalid i2c master\n");      
      return -EINVAL;
    }
      
  	aml_i2c_pinmux_master(i2c);
  	aml_i2c_set_clk(i2c, speed);
   	i2c->cur_slave_addr = slave_addr&0x7f;
   	i2c->master_regs->i2c_slave_addr = i2c->cur_slave_addr<<1;
    i2c->msg_flags = 0;
    wret = aml_i2c_write(i2c, &wbuf[0], wnum);
    /* if restart=0, a stop and a start condition will be do between the writing and reading;
     * else only the restart condition will be do.
    */
    if ((!restart) && wnum) {
      aml_i2c_stop(i2c);
      udelay(10);
  	  aml_i2c_pinmux_master(i2c);
  	}
    i2c->msg_flags = 0; // restart
    rret = aml_i2c_read(i2c, &rbuf[0], rnum);
    aml_i2c_stop(i2c);

    if (wnum) {
      printk("write %d data to slave (", wnum);
      for (i=0; i<wnum; i++)
        printk("0x%x, ", wbuf[i]);
      printk(") %s!\n", (wret==0) ? "success" : "failed");
    }
    if (rnum) {
      printk("read %d data from slave (", rnum);
      for (i=0; i<rnum; i++)
        printk("0x%x, ", rbuf[i]);
      printk(") %s!\n", (rret==0) ? "success" : "failed");        
    }
     
    return count;
}

static struct class_attribute i2c_class_attrs[] = {
    __ATTR(silence,  S_IRUGO | S_IWUSR, show_i2c_silence,    store_i2c_silence),
    __ATTR(debug,  S_IRUGO | S_IWUSR, show_i2c_debug,    store_i2c_debug),
    __ATTR(info,       S_IRUGO | S_IWUSR, show_i2c_info,    NULL),
    __ATTR(cbus_reg,  S_IRUGO | S_IWUSR, NULL,    store_register),
    __ATTR(customize,  S_IRUGO | S_IWUSR, NULL,    rw_special_reg),
    __ATTR(test_slave,  S_IRUGO | S_IWUSR, NULL,    test_slave_device),
    __ATTR_NULL
};

static int aml_i2c_probe(struct platform_device *pdev)
{
    int ret;

    struct aml_i2c_platform *plat = (struct aml_i2c_platform *)(pdev->dev.platform_data);
    struct resource *res;
    struct aml_i2c *i2c = kzalloc(sizeof(struct aml_i2c), GFP_KERNEL);

    printk("%s : %s\n", __FILE__, __FUNCTION__);

    i2c->ops = &aml_i2c_m1_ops;

    /*master a or master b*/
    i2c->master_no = plat->master_no;
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    i2c->master_regs = (struct aml_i2c_reg_master __iomem*)(res->start);
    printk("master_no = %d, resource = %p, maseter_regs=%p\n", i2c->master_no, res, i2c->master_regs);

    BUG_ON(!i2c->master_regs);
    BUG_ON(!plat);
    aml_i2c_set_platform_data(i2c, plat);

    /*lock init*/
    mutex_init(&i2c->lock);

    /*setup adapter*/
    i2c->adap.nr = pdev->id==-1? 0: pdev->id;
    i2c->adap.class = I2C_CLASS_HWMON;
    i2c->adap.algo = &aml_i2c_algorithm;
    i2c->adap.retries = 2;
    i2c->adap.timeout = 5;
    //memset(i2c->adap.name, 0 , 48);
    sprintf(i2c->adap.name, ADAPTER_NAME"%d", i2c->adap.nr);
    i2c_set_adapdata(&i2c->adap, i2c);
    ret = i2c_add_numbered_adapter(&i2c->adap);
    if (ret < 0)
    {
            dev_err(&pdev->dev, "Adapter %s registration failed\n",
                i2c->adap.name);
            kzfree(i2c);
            return -1;
    }
    dev_info(&pdev->dev, "add adapter %s(%p)\n", i2c->adap.name, &i2c->adap);

    /*need 2 different speed in 1 adapter, add a virtual one*/
    if(plat->master_i2c_speed2){
        i2c->master_i2c_speed2 = plat->master_i2c_speed2;
        /*setup adapter 2*/
        i2c->adap2.nr = i2c->adap.nr+1;
        i2c->adap2.class = I2C_CLASS_HWMON;
        i2c->adap2.algo = &aml_i2c_algorithm_s2;
        i2c->adap2.retries = 2;
        i2c->adap2.timeout = 5;
        //memset(i2c->adap.name, 0 , 48);
        sprintf(i2c->adap2.name, ADAPTER_NAME"%d", i2c->adap2.nr);
        i2c_set_adapdata(&i2c->adap2, i2c);
        ret = i2c_add_numbered_adapter(&i2c->adap2);
        if (ret < 0)
        {
            dev_err(&pdev->dev, "Adapter %s registration failed\n",
            i2c->adap2.name);
            i2c_del_adapter(&i2c->adap);
            kzfree(i2c);
            return -1;
        }
        dev_info(&pdev->dev, "add adapter %s\n", i2c->adap2.name);
    }
    dev_info(&pdev->dev, "aml i2c bus driver.\n");

    /*setup class*/
    i2c->cls.name = kzalloc(NAME_LEN, GFP_KERNEL);
    if(i2c->adap.nr)
        sprintf((char*)i2c->cls.name, "i2c%d", i2c->adap.nr);
    else
        sprintf((char*)i2c->cls.name, "i2c");
    i2c->cls.class_attrs = i2c_class_attrs;
    ret = class_register(&i2c->cls);
    if(ret)
        printk(" class register i2c_class fail!\n");

    return 0;
}



static int aml_i2c_remove(struct platform_device *pdev)
{
    struct aml_i2c *i2c = platform_get_drvdata(pdev);
    i2c_del_adapter(&i2c->adap);
    if(i2c->adap2.nr)
        i2c_del_adapter(&i2c->adap2);
    kzfree(i2c);
    i2c= NULL;
    return 0;
}

static struct platform_driver aml_i2c_driver = {
    .probe = aml_i2c_probe,
    .remove = aml_i2c_remove,
    .driver = {
        .name = "aml-i2c",
        .owner = THIS_MODULE,
    },
};

static int __init aml_i2c_init(void)
{
    int ret;
    printk("%s : %s\n", __FILE__, __FUNCTION__);

#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON3
    mutex_init(&aml_i2c_xfer_lock);
    ret = platform_driver_register(&aml_i2c_driver);
    if(ret){
        mutex_destroy(&aml_i2c_xfer_lock);
    }
#else
    ret = platform_driver_register(&aml_i2c_driver);
#endif
    return ret;
}

static void __exit aml_i2c_exit(void)
{
    printk("%s : %s\n", __FILE__, __FUNCTION__);
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON3
    mutex_destroy(&aml_i2c_xfer_lock);
#endif
    platform_driver_unregister(&aml_i2c_driver);
}

arch_initcall(aml_i2c_init);
module_exit(aml_i2c_exit);

MODULE_AUTHOR("AMLOGIC");
MODULE_DESCRIPTION("I2C driver for amlogic");
MODULE_LICENSE("GPL");

