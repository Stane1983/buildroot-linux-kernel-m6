/*
 * AMLOGIC Smart card driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the named License,
 * or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
 */

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/string.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/fcntl.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#ifdef CONFIG_ARCH_ARC700
#include <asm/arch/am_regs.h>
#else
#include "linux/clk.h"
#include <mach/am_regs.h>
#endif
#include <mach/devio_aml.h>
#include <mach/gpio_data.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <asm/gpio.h>
#include <linux/amsmc.h>
#include <linux/platform_device.h>

#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6
#include <mach/mod_gate.h>
#endif

#include "smc_reg.h"

#define DRIVER_NAME "amsmc"
#define MODULE_NAME "amsmc"
#define DEVICE_NAME "amsmc"
#define CLASS_NAME  "amsmc-class"

#if 0
#define pr_dbg(fmt, args...) printk("Smartcard: " fmt, ## args)
#else
#define pr_dbg(fmt, args...)
#endif

#define pr_error(fmt, args...) printk(KERN_ERR "Smartcard: " fmt, ## args)

MODULE_PARM_DESC(smc0_irq, "\n\t\t Irq number of smartcard0");
static int smc0_irq = -1;
module_param(smc0_irq, int, S_IRUGO);

MODULE_PARM_DESC(smc0_reset, "\n\t\t Reset GPIO pin of smartcard0");
static int smc0_reset = -1;
module_param(smc0_reset, int, S_IRUGO);

#define NO_HOT_RESET
//#define DISABLE_RECV_INT

#define RECV_BUF_SIZE     512
#define SEND_BUF_SIZE     512

#define RESET_ENABLE      (smc->reset_level)
#define RESET_DISABLE     (!smc->reset_level)

typedef struct {
	int                id;
	struct device     *dev;
	struct platform_device *pdev;
	int                init;
	int                used;
	int                cardin;
	int                active;
	struct mutex       lock;
	spinlock_t         slock;
	wait_queue_head_t  rd_wq;
	wait_queue_head_t  wr_wq;
	int                recv_start;
	int                recv_count;
	int                send_start;
	int                send_count;
	char               recv_buf[RECV_BUF_SIZE];
	char               send_buf[SEND_BUF_SIZE];
	struct am_smc_param param;
	struct am_smc_atr   atr;
	u32                reset_pin;
	int 			(*reset)(void *, int);
	u32                irq_num;
	int                reset_level;
} smc_dev_t;

#define SMC_DEV_NAME     "smc"
#define SMC_CLASS_NAME   "smc-class"
#define SMC_DEV_COUNT    1

#define SMC_READ_REG(a)           READ_MPEG_REG(SMARTCARD_##a)
#define SMC_WRITE_REG(a,b)        WRITE_MPEG_REG(SMARTCARD_##a,b)

static struct mutex smc_lock;
static int          smc_major;
static smc_dev_t    smc_dev[SMC_DEV_COUNT];
static int ENA_GPIO_PULL = 1;

static ssize_t show_gpio_pull(struct class *class, struct class_attribute *attr,	char *buf)
{
	if(ENA_GPIO_PULL > 0)
		return sprintf(buf, "%s\n","enable GPIO pull low");
	else
		return sprintf(buf, "%s\n","disable GPIO pull low");
}

static ssize_t set_gpio_pull(struct class *class, struct class_attribute *attr,	const char *buf, size_t count)
{
    unsigned int dbg;
    ssize_t r;

    r = sscanf(buf, "%d", &dbg);
    if (r != 1)
    return -EINVAL;

    ENA_GPIO_PULL = dbg;
    printk("Smartcard the ENA_GPIO_PULL is:%d.\n", ENA_GPIO_PULL);
    return count;
}

static struct class_attribute smc_class_attrs[] = {
	__ATTR(smc_gpio_pull,  S_IRUGO | S_IWUSR, show_gpio_pull,    set_gpio_pull),
    __ATTR_NULL
};

static struct class smc_class = {
    .name = SMC_CLASS_NAME,
    .class_attrs = smc_class_attrs,
};

static unsigned long get_clk(char *name)
{
	struct clk *clk=NULL;
	clk = clk_get_sys(name, NULL);
	if(clk)
		return clk_get_rate(clk);
	return 0;
}

static unsigned long get_module_clk(int sel)
{
#ifdef CONFIG_ARCH_ARC700
	extern unsigned long get_mpeg_clk(void);
	return get_mpeg_clk();
#else

	unsigned long clk=0;

#ifdef CONFIG_ARCH_MESON6/*M6*/
	/*sel = [0:clk81, 1:ddr-pll, 2:fclk-div5, 3:XTAL]*/
	switch(sel)
	{
		case 0: clk = get_clk("clk81"); break;
		case 1: clk = get_clk("pll_ddr"); break;
		case 2: clk = get_clk("fixed")/5; break;
		case 3: clk = get_clk("xtal"); break;
	}
#else /*M6TV*/
	/*sel = [0:fclk-div2, 1:fclk-div3, 2:fclk-div5, 3:XTAL]*/
	switch(sel)
	{
		case 0: clk = 1000000000; break;
		//case 0: clk = get_clk("fixed")/2; break;
		case 1: clk = get_clk("fixed")/3; break;
		case 2: clk = get_clk("fixed")/5; break;
		case 3: clk = get_clk("xtal"); break;
	}
#endif /*M6*/

	if(!clk)
		printk("fail: unknown clk source");

	return clk;

#endif
}

static int inline smc_write_end(smc_dev_t *smc)
{
	int ret;
	unsigned long flags;
	
	spin_lock_irqsave(&smc->slock, flags);
	ret = (!smc->cardin || !smc->send_count);
	spin_unlock_irqrestore(&smc->slock, flags);
	
	return ret;
}


static int inline smc_can_read(smc_dev_t *smc)
{
	int ret;
	unsigned long flags;
	
	spin_lock_irqsave(&smc->slock, flags);
	ret = (!smc->cardin || smc->recv_count);
	spin_unlock_irqrestore(&smc->slock, flags);
	
	return ret;
}

static int inline smc_can_write(smc_dev_t *smc)
{
	int ret;
	unsigned long flags;
	
	spin_lock_irqsave(&smc->slock, flags);
	ret = (!smc->cardin || (smc->send_count!=SEND_BUF_SIZE));
	spin_unlock_irqrestore(&smc->slock, flags);
	
	return ret;
}

static int smc_hw_set_param(smc_dev_t *smc)
{
	unsigned long v=0;
	SMCCARD_HW_Reg0_t *reg0;
	SMC_ANSWER_TO_RST_t *reg1;
	SMCCARD_HW_Reg2_t *reg2;
	SMC_INTERRUPT_Reg_t *reg_int;
	SMCCARD_HW_Reg5_t *reg5;
	SMCCARD_HW_Reg6_t *reg6;

	v = SMC_READ_REG(REG0);
	reg0 = (SMCCARD_HW_Reg0_t*)&v;
	reg0->etu_divider = ETU_DIVIDER_CLOCK_HZ*smc->param.f/(smc->param.d*smc->param.freq)-1;
	SMC_WRITE_REG(REG0, v);
	
	v = SMC_READ_REG(REG6);
	reg6 = (SMCCARD_HW_Reg6_t*)&v;
	reg6->N_parameter = smc->param.n;
	reg6->cwi_value = smc->param.cwi;
	reg6->bgt = smc->param.bgt;
	reg6->bwi = smc->param.bwi;
	SMC_WRITE_REG(REG6, v);
	
	return 0;
}

static int smc_hw_setup(smc_dev_t *smc)
{
	unsigned long v=0;
	SMCCARD_HW_Reg0_t *reg0;
	SMC_ANSWER_TO_RST_t *reg1;
	SMCCARD_HW_Reg2_t *reg2;
	SMC_INTERRUPT_Reg_t *reg_int;
	SMCCARD_HW_Reg5_t *reg5;
	SMCCARD_HW_Reg6_t *reg6;

	unsigned sys_clk_rate = get_module_clk(CLK_SRC_DEFAULT);

	unsigned long freq_cpu = sys_clk_rate/1000;

	printk("SMC CLK SOURCE - %luKHz\n", freq_cpu);
	
	v = SMC_READ_REG(REG0);
	reg0 = (SMCCARD_HW_Reg0_t*)&v;
	reg0->enable = 1;
	reg0->clk_en = 0;
	reg0->clk_oen = 0;
	reg0->card_detect = 0;
	reg0->start_atr = 0;
	reg0->start_atr_en = 0;
	reg0->rst_level = RESET_DISABLE;
	reg0->io_level = 0;
	reg0->recv_fifo_threshold = FIFO_THRESHOLD_DEFAULT;
	reg0->etu_divider = ETU_DIVIDER_CLOCK_HZ*smc->param.f/(smc->param.d*smc->param.freq)-1;
	SMC_WRITE_REG(REG0, v);
	
	v = SMC_READ_REG(REG1);
	reg1 = (SMC_ANSWER_TO_RST_t*)&v;
	reg1->atr_final_tcnt = ATR_FINAL_TCNT_DEFAULT;
	reg1->atr_holdoff_tcnt = ATR_HOLDOFF_TCNT_DEFAULT;
	reg1->atr_clk_mux = ATR_CLK_MUX_DEFAULT;
	reg1->atr_holdoff_en = ATR_HOLDOFF_EN;		
	SMC_WRITE_REG(REG1, v);
	
	v = SMC_READ_REG(REG2);
	reg2 = (SMCCARD_HW_Reg2_t*)&v;
	reg2->recv_invert = smc->param.recv_invert;
	reg2->recv_lsb_msb = smc->param.recv_lsb_msb;
	reg2->xmit_invert = smc->param.xmit_invert;
	reg2->xmit_lsb_msb = smc->param.xmit_lsb_msb;	
	reg2->xmit_retries = smc->param.xmit_retries;
	reg2->xmit_repeat_dis = smc->param.xmit_repeat_dis;
	reg2->recv_no_parity = smc->param.recv_no_parity;
	reg2->clk_tcnt = freq_cpu/smc->param.freq - 1;
	reg2->det_filter_sel = DET_FILTER_SEL_DEFAULT;
	reg2->io_filter_sel = IO_FILTER_SEL_DEFAULT; 
	reg2->clk_sel = CLK_SRC_DEFAULT;
	//reg2->pulse_irq = 0;
	SMC_WRITE_REG(REG2, v);
	
	v = SMC_READ_REG(INTR);	
	reg_int = (SMC_INTERRUPT_Reg_t*)&v;
	reg_int->recv_fifo_bytes_threshold_int_mask = 0;
	reg_int->send_fifo_last_byte_int_mask = 1;
	reg_int->cwt_expeired_int_mask = 1;
	reg_int->bwt_expeired_int_mask = 1;
	reg_int->write_full_fifo_int_mask = 1;
	reg_int->send_and_recv_confilt_int_mask = 1;
	reg_int->recv_error_int_mask = 1;
	reg_int->send_error_int_mask = 1;
	reg_int->rst_expired_int_mask = 1;
	reg_int->card_detect_int_mask = 0;
	SMC_WRITE_REG(INTR,v|0x03FF);
	
	v = SMC_READ_REG(REG5);
	reg5 = (SMCCARD_HW_Reg5_t*)&v;
	reg5->cwt_detect_en = 1;
	reg5->bwt_base_time_gnt = BWT_BASE_DEFAULT;
	SMC_WRITE_REG(REG5, v);

						
	v = SMC_READ_REG(REG6);
	reg6 = (SMCCARD_HW_Reg6_t*)&v;
	reg6->N_parameter = smc->param.n;
	reg6->cwi_value = smc->param.cwi;
	reg6->bgt = smc->param.bgt;
	reg6->bwi = smc->param.bwi;
	SMC_WRITE_REG(REG6, v);
	
	return 0;
}

static void enable_smc_clk(){
    unsigned int _value = READ_CBUS_REG(0x2030);
    pr_dbg("enable 111 value: %lx \n", _value);
    _value |= 0x100000;
    pr_dbg("enable 222 value: %lx \n", _value);
    WRITE_CBUS_REG(0x2030, _value);
    _value = READ_CBUS_REG(0x2030);
    pr_dbg("enable 333 value: %lx \n", _value);
}

static void disable_smc_clk(){
    unsigned int _value = READ_CBUS_REG(0x2030);
    pr_dbg("disable 111 value: %lx \n", _value);
    _value &= 0xFFEFFFFF;
    pr_dbg("disable 222 value: %lx \n", _value);
    WRITE_CBUS_REG(0x2030, _value);
    _value = READ_CBUS_REG(0x2030);
    pr_dbg("disable 333 value: %lx \n", _value);
    return;
}

static int smc_hw_active(smc_dev_t *smc)
{
    if(ENA_GPIO_PULL > 0){
        enable_smc_clk();
        udelay(200);
    }
	if(!smc->active) {
		if(smc->reset) {
			smc->reset(NULL, 0);
		} else {
			if(smc->reset_pin != -1) {
				gpio_request(smc->reset_pin, "smc:RESET");
				gpio_out(smc->reset_pin, 0);
			}
		}
	
		udelay(200);
		smc_hw_setup(smc);

		smc->active = 1;
	}
	
	return 0;
}

static int smc_hw_deactive(smc_dev_t *smc)
{
	if(smc->active) {
		unsigned long sc_reg0 = SMC_READ_REG(REG0);
		SMCCARD_HW_Reg0_t *sc_reg0_reg = (void *)&sc_reg0;
		sc_reg0_reg->rst_level = RESET_DISABLE;
		sc_reg0_reg->enable= 1;
		sc_reg0_reg->start_atr = 0;	
		sc_reg0_reg->start_atr_en = 0;	
		sc_reg0_reg->clk_en=0;
		SMC_WRITE_REG(REG0,sc_reg0);
	
		udelay(200);

		if(smc->reset) {
			smc->reset(NULL, 1);
		} else {
			if(smc->reset_pin != -1) {
				gpio_request(smc->reset_pin, "smc:RESET");
				gpio_out(smc->reset_pin, 1);
			}
		}
            if(ENA_GPIO_PULL > 0){
                 disable_smc_clk();
                 udelay(20);
                 gpio_out(PAD_GPIOA_13, 0);
                 udelay(1000);
            }
		smc->active = 0;
	}
	
	return 0;
}

static int smc_hw_get(smc_dev_t *smc, int cnt, int timeout)
{
	unsigned long sc_status;
	int times = timeout*100;
	SMC_STATUS_Reg_t *sc_status_reg = (SMC_STATUS_Reg_t*)&sc_status;
	
	while((times > 0) && (cnt > 0)) {
		sc_status = SMC_READ_REG(STATUS);

		//pr_dbg("read atr status %08x\n", sc_status);
						
		if(sc_status_reg->rst_expired_status)
		{
			//pr_error("atr timeout\n");
		}
		
		if(sc_status_reg->cwt_expeired_status)
		{
			//pr_error("cwt timeout when get atr, but maybe it is natural!\n");
		}

		if(sc_status_reg->recv_fifo_empty_status)
		{
			udelay(10);
			times--;
		}
		else
		{
			while(sc_status_reg->recv_fifo_bytes_number > 0)
			{
				smc->atr.atr[smc->atr.atr_len++] = (SMC_READ_REG(FIFO))&0xff;
				sc_status_reg->recv_fifo_bytes_number--;
				cnt--;
				if(cnt==0){
					pr_dbg("read atr bytes ok\n");
					return 0;
				}
			}
		}
	}
	
	pr_error("read atr failed\n");
	return -1;
}

static int smc_hw_read_atr(smc_dev_t *smc)
{
	char *ptr = smc->atr.atr;
	int his_len, t, tnext = 0, only_t0 = 1, loop_cnt=0;
	int i;
	
	pr_dbg("read atr\n");
	
	smc->atr.atr_len = 0;
	if(smc_hw_get(smc, 2, 2000)<0){
		goto end;
	}

	if(ptr[0] == 0){
		smc->atr.atr[0] = smc->atr.atr[1];
		smc->atr.atr_len = 1;
		if(smc_hw_get(smc, 1, 2000)<0){
			goto end;
		}
	}
	
	ptr++;
	his_len = ptr[0]&0x0F;
	
	do {
		tnext = 0;
		loop_cnt++;
		if(ptr[0]&0x10) {
			if(smc_hw_get(smc, 1, 1000)<0)
				goto end;
		}
		if(ptr[0]&0x20) {
			if(smc_hw_get(smc, 1, 1000)<0)
				goto end;
		}
		if(ptr[0]&0x40) {
			if(smc_hw_get(smc, 1, 1000)<0)
				goto end;
		}
		if(ptr[0]&0x80) {
			if(smc_hw_get(smc, 1, 1000)<0)
				goto end;
			ptr = &smc->atr.atr[smc->atr.atr_len-1];
			t = ptr[0]&0x0F;
			if(t) {
				only_t0 = 0;
			}
			if(ptr[0]&0xF0) {
				tnext = 1;
			}
		}
	} while(tnext && loop_cnt<4);
	
	if(!only_t0) his_len++;
	smc_hw_get(smc, his_len, 1000);

	printk("get atr len:%d data: ", smc->atr.atr_len);
	for(i=0; i<smc->atr.atr_len; i++){
		printk("%02x ", smc->atr.atr[i]);
	}
	printk("\n");

	return 0;

end:
	pr_error("read atr failed\n");
	return -EIO;
}

static int smc_hw_reset(smc_dev_t *smc)
{
	unsigned long flags;
	int ret;
	unsigned long sc_reg0 = SMC_READ_REG(REG0);
	SMCCARD_HW_Reg0_t *sc_reg0_reg = (void *)&sc_reg0;
	unsigned long sc_int;
	SMC_INTERRUPT_Reg_t *sc_int_reg = (void *)&sc_int;
	
	spin_lock_irqsave(&smc->slock, flags);
	if(smc->cardin) {
		ret = 0;
	} else {
		ret = -ENODEV;
	}
	spin_unlock_irqrestore(&smc->slock, flags);
	
	if(ret>=0) {
		/*Reset*/
#ifdef NO_HOT_RESET
		smc->active = 0;
#endif
		if(smc->active) {
			sc_reg0_reg->rst_level = RESET_DISABLE; 
			sc_reg0_reg->clk_en = 1;
			sc_reg0_reg->etu_divider = ETU_DIVIDER_CLOCK_HZ*smc->param.f/(smc->param.d*smc->param.freq)-1;
			
			pr_dbg("hot reset\n");
			
			SMC_WRITE_REG(REG0, sc_reg0);
			
			udelay(800/smc->param.freq); // >= 400/f ;

			/* disable receive interrupt*/
			sc_int = SMC_READ_REG(INTR);
			sc_int_reg->recv_fifo_bytes_threshold_int_mask = 0;
			SMC_WRITE_REG(INTR, sc_int|0x3FF);
		
			sc_reg0_reg->rst_level = RESET_ENABLE;
			sc_reg0_reg->start_atr = 1;
			SMC_WRITE_REG(REG0, sc_reg0);
		} else {
			pr_dbg("cold reset\n");
			
			smc_hw_deactive(smc);
			
			udelay(200);

			smc_hw_active(smc);

			sc_reg0_reg->clk_en =1 ;
			sc_reg0_reg->enable = 0;
			sc_reg0_reg->rst_level = RESET_DISABLE;
			SMC_WRITE_REG(REG0, sc_reg0);

			udelay(2000); // >= 400/f ;

			/* disable receive interrupt*/
			sc_int = SMC_READ_REG(INTR);
			sc_int_reg->recv_fifo_bytes_threshold_int_mask = 0;
			SMC_WRITE_REG(INTR, sc_int|0x3FF);
		
			sc_reg0_reg->rst_level = RESET_ENABLE;
			sc_reg0_reg->start_atr_en = 1;
			sc_reg0_reg->enable = 1;
			SMC_WRITE_REG(REG0, sc_reg0);
		}

		/*reset recv&send buf*/
		smc->send_start = 0;
		smc->send_count = 0;
		smc->recv_start = 0;
		smc->recv_count = 0;
		
		/*Read ATR*/
		smc->atr.atr_len = 0;
		smc->recv_count = 0;
		smc->send_count = 0;

		ret = smc_hw_read_atr(smc);
		
		/*Disable ATR*/
		sc_reg0 = SMC_READ_REG(REG0);
		sc_reg0_reg->start_atr_en = 0;
		sc_reg0_reg->start_atr = 0;
		SMC_WRITE_REG(REG0,sc_reg0);
		
#ifndef DISABLE_RECV_INT
		sc_int_reg->recv_fifo_bytes_threshold_int_mask = 1;
#endif
		SMC_WRITE_REG(INTR, sc_int|0x3FF);
	}
	
	return ret;
}

static int smc_hw_get_status(smc_dev_t *smc, int *sret)
{
	unsigned long flags;
	unsigned int  reg_val;
	SMCCARD_HW_Reg0_t *reg = (SMCCARD_HW_Reg0_t*)&reg_val;

	spin_lock_irqsave(&smc->slock, flags);
	
	reg_val = SMC_READ_REG(REG0);
	
	smc->cardin = reg->card_detect;

	//pr_dbg("get_status: smc reg0 %08x, card detect: %d\n", reg_val, smc->cardin);
	*sret = smc->cardin;
	
	spin_unlock_irqrestore(&smc->slock, flags);

	return 0;
}

static int smc_hw_start_send(smc_dev_t *smc)
{
	unsigned long flags;
	unsigned int sc_status;
	SMC_STATUS_Reg_t *sc_status_reg = (SMC_STATUS_Reg_t*)&sc_status;
	u8 byte;
	int cnt = 0;

	while(1) {			
		spin_lock_irqsave(&smc->slock, flags);
		
		sc_status = SMC_READ_REG(STATUS);
		if (!smc->send_count || sc_status_reg->send_fifo_full_status) {
			spin_unlock_irqrestore(&smc->slock, flags);
			break;
		}

		pr_dbg("s i f [%d], [%d]\n", smc->send_count, cnt);
		byte = smc->send_buf[smc->send_start++];
		SMC_WRITE_REG(FIFO, byte);
		smc->send_start %= SEND_BUF_SIZE;
		smc->send_count--;
		cnt++;

		spin_unlock_irqrestore(&smc->slock, flags);
	}

	pr_dbg("send %d bytes to hw\n", cnt);
	
	return 0;
}

static irqreturn_t smc_irq_handler(int irq, void *data)
{
	smc_dev_t *smc = (smc_dev_t*)data;
	unsigned int sc_status;
	unsigned int sc_reg0;
	unsigned int sc_int;
	SMC_STATUS_Reg_t *sc_status_reg = (SMC_STATUS_Reg_t*)&sc_status;
	SMC_INTERRUPT_Reg_t *sc_int_reg = (SMC_INTERRUPT_Reg_t*)&sc_int;
	SMCCARD_HW_Reg0_t *sc_reg0_reg = (void *)&sc_reg0;
	unsigned long flags;

	spin_lock_irqsave(&smc->slock, flags);		

	sc_int = SMC_READ_REG(INTR);
	pr_dbg("smc intr:0x%x\n", sc_int);

	if(sc_int_reg->recv_fifo_bytes_threshold_int) {
		
		if(smc->recv_count==RECV_BUF_SIZE) {
			pr_error("receive buffer overflow\n");
		} else {
			int pos = smc->recv_start+smc->recv_count;
			
			pos %= RECV_BUF_SIZE;
			smc->recv_buf[pos] = SMC_READ_REG(FIFO);
			smc->recv_count++;
			
			pr_dbg("irq: recv 1 byte [0x%x]\n", smc->recv_buf[pos]);
		}

		sc_int_reg->recv_fifo_bytes_threshold_int = 0;
		
		wake_up_interruptible(&smc->rd_wq);
	}
	
	if(sc_int_reg->send_fifo_last_byte_int) {
		int cnt = 0;
		
		while(1) {
			u8 byte;
			
			sc_status = SMC_READ_REG(STATUS);
			if (!smc->send_count || sc_status_reg->send_fifo_full_status) {
				break;
			}
			
			byte = smc->send_buf[smc->send_start++];
			SMC_WRITE_REG(FIFO, byte);
			smc->send_start %= SEND_BUF_SIZE;
			smc->send_count--;
			cnt++;
		}
		
		pr_dbg("irq: send %d bytes to hw\n", cnt);

		if(!smc->send_count) {
			sc_int_reg->send_fifo_last_byte_int_mask = 0;
			sc_int_reg->recv_fifo_bytes_threshold_int_mask = 1;
		}

		sc_int_reg->send_fifo_last_byte_int = 0;
		
		wake_up_interruptible(&smc->wr_wq);
	}
	
	sc_reg0 = SMC_READ_REG(REG0);
	smc->cardin = sc_reg0_reg->card_detect;

	SMC_WRITE_REG(INTR, sc_int|0x3FF);

	spin_unlock_irqrestore(&smc->slock, flags);
	
	return IRQ_HANDLED;
}

static void smc_dev_deinit(smc_dev_t *smc)
{
	struct devio_aml_platform_data *pd_smc;

	if(smc->irq_num!=-1) {
		free_irq(smc->irq_num, &smc);
	}
	
	if(smc->dev) {
		device_destroy(&smc_class, MKDEV(smc_major, smc->id));
	}
	
	mutex_destroy(&smc->lock);

	pd_smc =  (struct devio_aml_platform_data*)smc->pdev->dev.platform_data;
	if(pd_smc && pd_smc->io_cleanup)
		pd_smc->io_cleanup(NULL);
	
	smc->init = 0;

}

static int smc_dev_init(smc_dev_t *smc, int id)
{
	struct resource *res;
	char buf[32];
	struct devio_aml_platform_data *pd_smc;

	smc->id = id;
	
	smc->reset_pin = smc0_reset;
	if(smc->reset_pin==-1) {
		snprintf(buf, sizeof(buf), "smc%d_reset", id);
		res = platform_get_resource_byname(smc->pdev, IORESOURCE_MEM, buf);
		if (!res) {
			pr_error("cannot get resource \"%s\"\n", buf);
		} else {
			smc->reset_pin = res->start;
		}
	}

	smc->reset_level = 1;
	if(1) {
		snprintf(buf, sizeof(buf), "smc%d_reset_level", id);
		res = platform_get_resource_byname(smc->pdev, IORESOURCE_MEM, buf);
		if (!res) {
			pr_error("cannot get resource \"%s\"\n", buf);
		} else {
			smc->reset_level = res->start;
		}
	}
	
	smc->irq_num = smc0_irq;
	if(smc->irq_num==-1) {
		snprintf(buf, sizeof(buf), "smc%d_irq", id);
		res = platform_get_resource_byname(smc->pdev, IORESOURCE_IRQ, buf);
	if (!res) {
		pr_error("cannot get resource \"%s\"\n", buf);
		return -1;
	}
	smc->irq_num = res->start;
	}
	
	init_waitqueue_head(&smc->rd_wq);
	init_waitqueue_head(&smc->wr_wq);
	spin_lock_init(&smc->slock);
	mutex_init(&smc->lock);

	pd_smc =  (struct devio_aml_platform_data*)smc->pdev->dev.platform_data;
	if(pd_smc) {
		if(pd_smc->io_setup)
			pd_smc->io_setup(NULL);

		smc->reset = pd_smc->io_reset;
	}
	
	smc->irq_num=request_irq(smc->irq_num,(irq_handler_t)smc_irq_handler,IRQF_SHARED,"smc",smc);
	if(smc->irq_num<0) {
		pr_error("request irq error!\n");
		smc_dev_deinit(smc);
		return -1;
	}
	
	snprintf(buf, sizeof(buf), "smc%d", smc->id);
	smc->dev=device_create(&smc_class, NULL, MKDEV(smc_major, smc->id), smc, buf);
	if(!smc->dev) {
		pr_error("create device error!\n");
		smc_dev_deinit(smc);
		return -1;
	}

	smc->param.f = F_DEFAULT;
	smc->param.d = D_DEFAULT;
	smc->param.n = N_DEFAULT;
	smc->param.bwi = BWI_DEFAULT;
	smc->param.cwi = CWI_DEFAULT;
	smc->param.bgt = BGT_DEFAULT;
	smc->param.freq = FREQ_DEFAULT;
	smc->param.recv_invert = 0;
	smc->param.recv_lsb_msb = 0;
	smc->param.recv_no_parity = 1;
	smc->param.xmit_invert = 0;
	smc->param.xmit_lsb_msb = 0;
	smc->param.xmit_retries = 1;
	smc->param.xmit_repeat_dis = 1;
	smc->init = 1;

	smc_hw_setup(smc);
	
	return 0;
}

static int smc_open(struct inode *inode, struct file *filp)
{
	int id = iminor(inode);
	smc_dev_t *smc = &smc_dev[id];
	
	mutex_lock(&smc->lock);
	
	if(smc->used) {
		mutex_unlock(&smc->lock);
		pr_error("smartcard %d already openned!", id);
		return -EBUSY;
	}
	
	smc->used = 1;

#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6
	switch_mod_gate_by_name("smart_card", 1);
#endif

	mutex_unlock(&smc->lock);
	
	filp->private_data = smc;
	return 0;
}

static int smc_close(struct inode *inode, struct file *filp)
{
	smc_dev_t *smc = (smc_dev_t*)filp->private_data;
	
	mutex_lock(&smc->lock);
	smc_hw_deactive(smc);
	smc->used = 0;

#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6
	switch_mod_gate_by_name("smart_card", 0);
#endif

	mutex_unlock(&smc->lock);
	
	return 0;
}

static int smc_read(struct file *filp,char __user *buff, size_t size, loff_t *ppos)
{
	smc_dev_t *smc = (smc_dev_t*)filp->private_data;
	unsigned long flags;
	int pos = 0, ret;
	unsigned long sc_int;
	SMC_INTERRUPT_Reg_t *sc_int_reg = (void *)&sc_int;
	
	ret = mutex_lock_interruptible(&smc->lock);
	if(ret) return ret;

#ifdef DISABLE_RECV_INT
	pr_dbg("wait write end\n");
	if(!(filp->f_flags&O_NONBLOCK)) {
		ret = wait_event_interruptible(smc->wr_wq, smc_write_end(smc));
	}
	
	if(ret==0) {
		pr_dbg("wait read buffer\n");
		
		sc_int = SMC_READ_REG(INTR);
		sc_int_reg->recv_fifo_bytes_threshold_int_mask = 1;
		sc_int_reg->send_fifo_last_byte_int_mask = 0;
		SMC_WRITE_REG(INTR, sc_int|0x3FF);
		
		if(!(filp->f_flags&O_NONBLOCK)) {
			ret = wait_event_interruptible(smc->rd_wq, smc_can_read(smc));
		}
	}
#endif
	
	if(ret==0) {
		
		spin_lock_irqsave(&smc->slock, flags);
		
		if(!smc->cardin) {
			ret = -ENODEV;
		} else if (!smc->recv_count) {
			ret = -EAGAIN;
		} else {
			ret = smc->recv_count;
			if(ret>size) ret = size;
			
			pos = smc->recv_start;
			smc->recv_start += ret;
			smc->recv_count -= ret;
			smc->recv_start %= RECV_BUF_SIZE;
		}
		spin_unlock_irqrestore(&smc->slock, flags);
	}
	
	if(ret>0) {
		int cnt = RECV_BUF_SIZE-pos;
		pr_dbg("read %d bytes\n", ret);
		
		if(cnt>=ret) {
			copy_to_user(buff, smc->recv_buf+pos, ret);
		} else {
			int cnt1 = ret-cnt;
			copy_to_user(buff, smc->recv_buf+pos, cnt);
			copy_to_user(buff+cnt, smc->recv_buf, cnt1);
		}
	}
	
	mutex_unlock(&smc->lock);
	
	return ret;
}

static int smc_write(struct file *filp, const char __user *buff, size_t size, loff_t *offp)
{
	smc_dev_t *smc = (smc_dev_t*)filp->private_data;
	unsigned long flags;
	int pos = 0, ret;
	unsigned long sc_int;
	SMC_INTERRUPT_Reg_t *sc_int_reg = (void *)&sc_int;

	ret = mutex_lock_interruptible(&smc->lock);
	if(ret) return ret;
	
	pr_dbg("wait write buffer\n");
	
	if(!(filp->f_flags&O_NONBLOCK)) {
		ret = wait_event_interruptible(smc->wr_wq, smc_can_write(smc));
	}
	
	if(ret==0) {
		spin_lock_irqsave(&smc->slock, flags);
		
		if(!smc->cardin) {
			ret = -ENODEV;
		} else if (smc->send_count==SEND_BUF_SIZE) {
			ret = -EAGAIN;
		} else {
			ret = SEND_BUF_SIZE-smc->send_count;
			if(ret>size) ret = size;
			
			pos = smc->send_start+smc->send_count;
			pos %= SEND_BUF_SIZE;
			smc->send_count += ret;
		}
		
		spin_unlock_irqrestore(&smc->slock, flags);
	}
	
	if(ret>0) {
		int cnt = SEND_BUF_SIZE-pos;
		
		if(cnt>=ret) {
			copy_from_user(smc->send_buf+pos, buff, ret);
		} else {
			int cnt1 = ret-cnt;
			copy_from_user(smc->send_buf+pos, buff, cnt);
			copy_from_user(smc->send_buf, buff+cnt, cnt1);
		}
		
		sc_int = SMC_READ_REG(INTR);
#ifdef DISABLE_RECV_INT
		sc_int_reg->recv_fifo_bytes_threshold_int_mask = 0;
#endif
		sc_int_reg->send_fifo_last_byte_int_mask = 1;
		SMC_WRITE_REG(INTR, sc_int|0x3FF);
		
		pr_dbg("write %d bytes\n", ret);

		smc_hw_start_send(smc);
	}

	
	mutex_unlock(&smc->lock);
	
	return ret;
}

static int smc_ioctl(struct file *file, unsigned int cmd, ulong arg)
{
	smc_dev_t *smc = (smc_dev_t*)file->private_data;
	int ret = 0;
	
	switch(cmd) {
		case AMSMC_IOC_RESET:
		{
			ret = mutex_lock_interruptible(&smc->lock);
			if(ret) return ret;
			
			ret = smc_hw_reset(smc);
			if(ret>=0) {
				copy_to_user((void*)arg, &smc->atr, sizeof(struct am_smc_atr));
			}
			
			mutex_unlock(&smc->lock);
		}
		break;
		case AMSMC_IOC_GET_STATUS:
		{
			int status;
			ret = smc_hw_get_status(smc, &status);
			if(ret>=0) {
				copy_to_user((void*)arg, &status, sizeof(int));
			}
		}
		break;
		case AMSMC_IOC_ACTIVE:
		{
			ret = mutex_lock_interruptible(&smc->lock);
			if(ret) return ret;
			
			ret = smc_hw_active(smc);
			
			mutex_unlock(&smc->lock);
		}
		break;
		case AMSMC_IOC_DEACTIVE:
		{
			ret = mutex_lock_interruptible(&smc->lock);
			if(ret) return ret;
			
			ret = smc_hw_deactive(smc);
			
			mutex_unlock(&smc->lock);
		}
		break;
		case AMSMC_IOC_GET_PARAM:
		{
			ret = mutex_lock_interruptible(&smc->lock);
			if(ret) return ret;
			
			copy_to_user((void*)arg, &smc->param, sizeof(struct am_smc_param));
			
			mutex_unlock(&smc->lock);
		}
		break;
		case AMSMC_IOC_SET_PARAM:
		{
			ret = mutex_lock_interruptible(&smc->lock);
			if(ret) return ret;
			
			copy_from_user(&smc->param, (void*)arg, sizeof(struct am_smc_param));
			ret = smc_hw_set_param(smc);
			
			mutex_unlock(&smc->lock);
		}
		break;
		default:
			ret = -EINVAL;
		break;
	}
	
	return ret;
}

static unsigned int smc_poll(struct file *filp, struct poll_table_struct *wait)
{
	smc_dev_t *smc = (smc_dev_t*)filp->private_data;
	unsigned int ret = 0;
	unsigned long flags;
	
	poll_wait(filp, &smc->rd_wq, wait);
	poll_wait(filp, &smc->wr_wq, wait);
	
	spin_lock_irqsave(&smc->slock, flags);

	if(smc->recv_count) ret |= POLLIN|POLLRDNORM;
	if(smc->send_count!=SEND_BUF_SIZE) ret |= POLLOUT|POLLWRNORM;
	if(!smc->cardin) ret |= POLLERR;
	
	spin_unlock_irqrestore(&smc->slock, flags);
	
	return ret;
}

static struct file_operations smc_fops =
{
	.owner = THIS_MODULE,
	.open  = smc_open,
	.write = smc_write,
	.read  = smc_read,
	.release = smc_close,
	.unlocked_ioctl = smc_ioctl,
	.poll  = smc_poll
};

static int smc_probe(struct platform_device *pdev)
{
	smc_dev_t *smc = NULL;
	int i, ret;
	
	mutex_lock(&smc_lock);
	
	for (i=0; i<SMC_DEV_COUNT; i++) {
		if (!smc_dev[i].init) {
			smc = &smc_dev[i];
			break;
		}
	}
	
	if(smc) {
		smc->init = 1;
		smc->pdev = pdev;
		dev_set_drvdata(&pdev->dev, smc);
	
		if ((ret=smc_dev_init(smc, i))<0) {
			smc = NULL;
		}
	}
	
	mutex_unlock(&smc_lock);
	
	return smc ? 0 : -1;
}

static int smc_remove(struct platform_device *pdev)
{
	smc_dev_t *smc = (smc_dev_t*)dev_get_drvdata(&pdev->dev);
	
	mutex_lock(&smc_lock);
	
	smc_dev_deinit(smc);
	
	mutex_unlock(&smc_lock);
	
	return 0;
}

static struct platform_driver smc_driver = {
	.probe		= smc_probe,
	.remove		= smc_remove,	
	.driver		= {
		.name	= "amlogic-smc",
		.owner	= THIS_MODULE,
	}
};

static int __init smc_mod_init(void)
{
	int ret = -1;
	
	mutex_init(&smc_lock);
	
	smc_major = register_chrdev(0, SMC_DEV_NAME, &smc_fops);
	if(smc_major<=0) {
		mutex_destroy(&smc_lock);
		pr_error("register chrdev error\n");
		goto error_register_chrdev;
	}
	
	if(class_register(&smc_class)<0) {
		pr_error("register class error\n");
		goto error_class_register;
	}
	
	if(platform_driver_register(&smc_driver)<0) {
		pr_error("register platform driver error\n");
		goto error_platform_drv_register;
	}
	
	return 0;
error_platform_drv_register:
	class_unregister(&smc_class);
error_class_register:
	unregister_chrdev(smc_major, SMC_DEV_NAME);
error_register_chrdev:
	mutex_destroy(&smc_lock);
	return ret;
}

static void __exit smc_mod_exit(void)
{
	platform_driver_unregister(&smc_driver);
	class_unregister(&smc_class);
	unregister_chrdev(smc_major, SMC_DEV_NAME);
	mutex_destroy(&smc_lock);
}

module_init(smc_mod_init);

module_exit(smc_mod_exit);

MODULE_AUTHOR("AMLOGIC");

MODULE_DESCRIPTION("AMLOGIC smart card driver");

MODULE_LICENSE("GPL");

