#include <linux/init.h>
#include <linux/types.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/major.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/ctype.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <mach/am_regs.h>
#include <linux/device.h>
#include <linux/cdev.h>

#include <asm/fiq.h>
#include <asm/uaccess.h>
#include "aml_demod.h"
#include "demod_func.h"

#include <linux/slab.h>
//#include "sdio/sdio_init.h"
#define DRIVER_NAME "aml_demod"
#define MODULE_NAME "aml_demod"
#define DEVICE_NAME "aml_demod"

const char aml_demod_dev_id[] = "aml_demod";

/*#ifndef CONFIG_AM_DEMOD_DVBAPI
static struct aml_demod_i2c demod_i2c;
static struct aml_demod_sta demod_sta;
#else 
  extern struct aml_demod_i2c demod_i2c;
  extern struct aml_demod_sta demod_sta;
#endif*/
static struct aml_demod_i2c demod_i2c;
static struct aml_demod_sta demod_sta;

int sdio_read_ddr(unsigned long sdio_addr, unsigned long byte_count, unsigned char *data_buf)
{
}
int sdio_write_ddr(unsigned long sdio_addr, unsigned long byte_count, unsigned char *data_buf)
{
}


static DECLARE_WAIT_QUEUE_HEAD(lock_wq);

static ssize_t aml_demod_info(struct class *cla, 
			      struct class_attribute *attr, 
			      char *buf)
{
    return 0;
}

static struct class_attribute aml_demod_class_attrs[] = {
    __ATTR(info,       
	   S_IRUGO | S_IWUSR, 
	   aml_demod_info,    
	   NULL),
    __ATTR_NULL
};

static struct class aml_demod_class = {
    .name = "aml_demod",
    .class_attrs = aml_demod_class_attrs,
};

static irqreturn_t aml_demod_isr(int irq, void *dev_id)
{
#if 0
    if (demod_sta.dvb_mode == 0) {
	//dvbc_isr(&demod_sta);
	if(dvbc_isr_islock()){
		printk("sync4\n");
		if(waitqueue_active(&lock_wq))
			wake_up_interruptible(&lock_wq);
	}
    }
    else {
	dvbt_isr(&demod_sta);
    }
#endif
    return IRQ_HANDLED;
}

static int aml_demod_open(struct inode *inode, struct file *file)
{
    printk("Amlogic Demod DVB-T/C Open\n");    
    return 0;
}

static int aml_demod_release(struct inode *inode, struct file *file)

{
    printk("Amlogic Demod DVB-T/C Release\n");    
    return 0;
}
#if 0
static int amdemod_islock(void)
{
	struct aml_demod_sts demod_sts;
	if(demod_sta.dvb_mode == 0) {
		dvbc_status(&demod_sta, &demod_i2c, &demod_sts);
		return demod_sts.ch_sts&0x1;
	} else if(demod_sta.dvb_mode == 1) {
		dvbt_status(&demod_sta, &demod_i2c, &demod_sts);
		return demod_sts.ch_sts>>12&0x1;
	}
	return 0;
}
#endif
static int sdio_ret =0;
static int sdio_total_cnt = 0;
static int sdio_error_cnt = 0;
static int sdio_work = 1;


extern long *mem_buf;
void mem_read(struct aml_demod_mem* arg)
{
	int data;
	int addr;
	addr = arg->addr;
	data = arg->dat;
	memcpy(mem_buf[addr],data,1);
	printk("[addr %x] data is %x\n",addr,data);
}

static int aml_demod_ioctl(struct file *file,
                        unsigned int cmd, ulong arg)
{
    // printk("AML_DEMOD_SET_SYS     0x%x\n", AML_DEMOD_SET_SYS    );
    // printk("AML_DEMOD_GET_SYS     0x%x\n", AML_DEMOD_GET_SYS    );
    // printk("AML_DEMOD_TEST        0x%x\n", AML_DEMOD_TEST       );
    // printk("AML_DEMOD_DVBC_SET_CH 0x%x\n", AML_DEMOD_DVBC_SET_CH);
    // printk("AML_DEMOD_DVBC_GET_CH 0x%x\n", AML_DEMOD_DVBC_GET_CH);
    // printk("AML_DEMOD_DVBC_TEST   0x%x\n", AML_DEMOD_DVBC_TEST  );
    // printk("AML_DEMOD_DVBT_SET_CH 0x%x\n", AML_DEMOD_DVBT_SET_CH);
    // printk("AML_DEMOD_DVBT_GET_CH 0x%x\n", AML_DEMOD_DVBT_GET_CH);
    // printk("AML_DEMOD_DVBT_TEST   0x%x\n", AML_DEMOD_DVBT_TEST  );
    // printk("AML_DEMOD_SET_REG     0x%x\n", AML_DEMOD_SET_REG    );
    // printk("AML_DEMOD_GET_REG     0x%x\n", AML_DEMOD_GET_REG    );
	int i=0;
	int step;
	struct aml_tuner_sys *tuner;
	
  struct fpga_m1_sdio *arg_t;

    switch (cmd) {
	
	case AML_DEMOD_GET_RSSI :
		printk("Ioctl Demod GET_RSSI. It is blank \n"); 
		//tuner_get_ch_power(&demod_i2c);
		break;

	case AML_DEMOD_SET_TUNER : 
	     printk("Ioctl Demod Set Tuner.\n");    	
		demod_set_tuner(&demod_sta, &demod_i2c, (struct aml_tuner_sys *)arg);
		break;
		
    	case AML_DEMOD_SET_SYS : 
		printk("Ioctl Demod Set System\n");    	
		demod_set_sys(&demod_sta, &demod_i2c, (struct aml_demod_sys *)arg);
		break;
	
    	case AML_DEMOD_GET_SYS :
		printk("Ioctl Demod Get System\n"); 
	
	//	demod_get_sys(&demod_i2c, (struct aml_demod_sys *)arg);
		break;

    	case AML_DEMOD_TEST :
		printk("Ioctl Demod Test. It is blank now\n");    	
		//demod_msr_clk(13);
		//demod_msr_clk(14);
		//demod_calc_clk(&demod_sta);
		break;

    	case AML_DEMOD_TURN_ON :
		printk("Ioctl Demod Turn ON.It is blank now\n");
		//demod_turn_on(&demod_sta, (struct aml_demod_sys *)arg);
		break;

	case AML_DEMOD_TURN_OFF :
		printk("Ioctl Demod Turn OFF.It is blank now\n");
		//demod_turn_off(&demod_sta, (struct aml_demod_sys *)arg);
		break;
	
	case AML_DEMOD_DVBC_SET_CH :
		printk("Ioctl DVB-C Set Channel. \n");   
		dvbc_set_ch(&demod_sta, &demod_i2c, (struct aml_demod_dvbc *)arg);
		break;

    	case AML_DEMOD_DVBC_GET_CH :
	//	printk("Ioctl DVB-C Get Channel. It is blank\n");
		dvbc_status(&demod_sta, &demod_i2c, (struct aml_demod_sts *)arg);
		break;
	case AML_DEMOD_DVBC_TEST :
		printk("Ioctl DVB-C Test. It is blank\n");    	
		//dvbc_get_test_out(0xb, 1000, (u32 *)arg);
		break;
	case AML_DEMOD_DVBT_SET_CH :
		printk("Ioctl DVB-T Set Channel\n");    	
		dvbt_set_ch(&demod_sta, &demod_i2c, (struct aml_demod_dvbt *)arg);
		break;

    	case AML_DEMOD_DVBT_GET_CH :
		printk("Ioctl DVB-T Get Channel\n");
	//	dvbt_status(&demod_sta, &demod_i2c, (struct aml_demod_sts *)arg);
		break;

    	case AML_DEMOD_DVBT_TEST :
		printk("Ioctl DVB-T Test. It is blank\n"); 
		//dvbt_get_test_out(0x1e, 1000, (u32 *)arg);
		break;

	case AML_DEMOD_DTMB_SET_CH:
	//	dtmb_set_ch(&demod_sta, &demod_i2c, (struct aml_demod_dtmb *)arg);
		break;

	case AML_DEMOD_DTMB_GET_CH:
		break;

	case AML_DEMOD_DTMB_TEST:
		break;

	case AML_DEMOD_ATSC_SET_CH:
		atsc_set_ch(&demod_sta, &demod_i2c, (struct aml_demod_atsc *)arg);
		break;

	case AML_DEMOD_ATSC_GET_CH:
		check_atsc_fsm_status();
		break;

	case AML_DEMOD_ATSC_TEST:
		break;
		
	case AML_DEMOD_SET_REG :
	//	printk("Ioctl Set Register\n"); 
   		demod_set_reg((struct aml_demod_reg *)arg);
		break;

	case AML_DEMOD_GET_REG :
	//	printk("Ioctl Get Register\n");    	
   		demod_get_reg((struct aml_demod_reg *)arg);
		break;
            
 	case AML_DEMOD_SET_REGS : 
		break;

 	case AML_DEMOD_GET_REGS :
		break;

	case AML_DEMOD_RESET_MEM : 
		memset(mem_buf,0,64*1024*1024-1);
		printk("set mem ok\n");
		break;

 	case AML_DEMOD_READ_MEM :
		step=arg;
		printk("[%x]0x%x------------------\n",i,mem_buf[step]);
		for(i=step;i<step+1024;i++){
		printk("0x%x,",mem_buf[i]);
		}
		break;
	case AML_DEMOD_SET_MEM :
		/*step=(struct aml_demod_mem)arg;
		printk("[%x]0x%x------------------\n",i,mem_buf[step]);
		for(i=step;i<1024-1;i++){
		printk("0x%x,",mem_buf[i]);
		}*/
		mem_read((struct aml_demod_mem *)arg);
		break;


    default:
		printk("Enter Default ! 0x%X\n", cmd);
		printk("AML_DEMOD_GET_REGS=0x%08X\n", AML_DEMOD_GET_REGS);
		printk("AML_DEMOD_SET_REGS=0x%08X\n", AML_DEMOD_SET_REGS);
	return -EINVAL;
    }

    return 0;
}

const static struct file_operations aml_demod_fops = {
    .owner    = THIS_MODULE,
    .open     = aml_demod_open,
    .release  = aml_demod_release,
    .unlocked_ioctl    = aml_demod_ioctl,
};

static struct device *aml_demod_dev;
static dev_t aml_demod_devno;
static struct cdev*  aml_demod_cdevp;

#ifdef CONFIG_AM_DEMOD_DVBAPI
int aml_demod_init(void)
#else
static int __init aml_demod_init(void)
#endif
{
    int r = 0;

    printk("Amlogic Demod DVB-T/C DebugIF Init\n");    
	
    init_waitqueue_head(&lock_wq);

    /* hook demod isr */
    r = request_irq(INT_DEMOD, &aml_demod_isr,
                    IRQF_SHARED, "aml_demod",
                    (void *)aml_demod_dev_id);

    if (r) {
        printk("aml_demod irq register error.\n");
        r = -ENOENT;
        goto err0;
    }

    /* sysfs node creation */
    r = class_register(&aml_demod_class);
    if (r) {
        printk("create aml_demod class fail\r\n");
        goto err1;
    }

	r = alloc_chrdev_region(&aml_demod_devno, 0, 1, DEVICE_NAME);
	if(r < 0){
		printk(KERN_ERR"aml_demod: faild to alloc major number\n");
		r = - ENODEV;
		goto err2;
	}

	aml_demod_cdevp = kmalloc(sizeof(struct cdev), GFP_KERNEL);
	if(!aml_demod_cdevp){
		printk(KERN_ERR"aml_demod: failed to allocate memory\n");
		r = -ENOMEM;
		goto err3;
	}
	// connect the file operation with cdev
	cdev_init(aml_demod_cdevp, &aml_demod_fops);
	aml_demod_cdevp->owner = THIS_MODULE;
	// connect the major/minor number to cdev
	r = cdev_add(aml_demod_cdevp, aml_demod_devno, 1);
	if(r){
		printk(KERN_ERR "aml_demod:failed to add cdev\n");
		goto err4;
	} 

    aml_demod_dev = device_create(&aml_demod_class, NULL,
				  MKDEV(MAJOR(aml_demod_devno),0), NULL,
				  DEVICE_NAME);

    if (IS_ERR(aml_demod_dev)) {
        printk("Can't create aml_demod device\n");
        goto err5;
    }
	 printk("Amlogic Demod DVB-T/C DebugIF Init ok----------------\n");    
#if defined(CONFIG_AM_AMDEMOD_FPGA_VER) && !defined(CONFIG_AM_DEMOD_DVBAPI)
    printk("sdio_init \n");
    sdio_init();
#endif	  

    return (0);
	
  err5:
    cdev_del(aml_demod_cdevp);
  err4:
    kfree(aml_demod_cdevp);

  err3:
    unregister_chrdev_region(aml_demod_devno, 1);

  err2:
    free_irq(INT_DEMOD, (void *)aml_demod_dev_id);

  err1:
    class_unregister(&aml_demod_class);
    
  err0:
    return r;
}

#ifdef CONFIG_AM_DEMOD_DVBAPI
void aml_demod_exit(void)
#else
static void __exit aml_demod_exit(void)
#endif
{
    printk("Amlogic Demod DVB-T/C DebugIF Exit\n");    

    unregister_chrdev_region(aml_demod_devno, 1);
    device_destroy(&aml_demod_class, MKDEV(MAJOR(aml_demod_devno),0));
    cdev_del(aml_demod_cdevp);
    kfree(aml_demod_cdevp);

    free_irq(INT_DEMOD, (void *)aml_demod_dev_id);

    class_unregister(&aml_demod_class);
}

#ifndef CONFIG_AM_DEMOD_DVBAPI
module_init(aml_demod_init);
module_exit(aml_demod_exit);

MODULE_LICENSE("GPL");
//MODULE_AUTHOR(DRV_AUTHOR);
//MODULE_DESCRIPTION(DRV_DESC);
#endif

