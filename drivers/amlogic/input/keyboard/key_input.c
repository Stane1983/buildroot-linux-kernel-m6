/*
 * linux/drivers/input/key_input/key_input.c
 *
 * ADC Keypad Driver
 *
 * Copyright (C) 2010 Amlogic Corporation
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * author :   Elvis Yu
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/types.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/errno.h>
#include <asm/irq.h>
#include <asm/io.h>

#include <mach/am_regs.h>
#include <mach/pinmux.h>
#include <linux/major.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/types.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/errno.h>
#include <asm/irq.h>
#include <asm/io.h>

#include <mach/am_regs.h>
#include <mach/pinmux.h>
#include <linux/major.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/input/key_input.h>

#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6
#include <plat/io.h>
#endif

#ifdef CONFIG_AML1212
#include <amlogic/aml1212.h>
#endif

//#define AML_KEYINPUT_DBG
#define AML_KEYINPUT_INTR     0
#define AML_KEYINPUT_POLLING   2

#define CALL_FLAG (0x1234ca11)

static void keyinput_tasklet(unsigned long data);
DECLARE_TASKLET_DISABLED(ki_tasklet, keyinput_tasklet, 0);

struct key_input {
    struct input_dev *input;
    struct timer_list timer;
    int* key_state_list_0;
    int* key_state_list_1;
    int* key_hold_time_list;
    int major;
    char name[20];
    struct class *class;
    struct device *dev;
    struct key_input_platform_data *pdata;
    unsigned status;
    unsigned pending;
    unsigned suspend;
};

static struct key_input *KeyInput = NULL;

#ifdef CONFIG_AML1212
static int    power_key_status;
static struct delayed_work power_key_work;
#endif

#ifdef CONFIG_AML_RTC
static int resume_jeff_num = 0;
#endif

void key_input_polling(unsigned long data)
{
    int i;
    struct key_input *ki_data=(struct key_input*)data;
    if(ki_data->pdata->scan_func(ki_data->key_state_list_0) >= 0)
    {
        for(i = 0; i < ki_data->pdata->key_num; i++)
        {
            if(ki_data->key_state_list_0[i])
            {
                if((ki_data->key_hold_time_list[i] += ki_data->pdata->scan_period) > ki_data->pdata->fuzz_time)
                {
#ifdef AML_KEYINPUT_DBG                    
                    print_dbg("key %d pressed.\n", ki_data->pdata->key_code_list[i]);
#endif                    
                    input_report_key(ki_data->input, ki_data->pdata->key_code_list[i], 1);
                    input_sync(ki_data->input);
                    ki_data->key_state_list_1[i] = 1;
                    ki_data->key_hold_time_list[i] = 0;
                }
            }
            else
            {
                if(ki_data->key_state_list_1[i])
                {
#ifdef AML_KEYINPUT_DBG                         
                    print_dbg("key %d released.\n", ki_data->pdata->key_code_list[i]);
#endif                    
                    input_report_key(ki_data->input, ki_data->pdata->key_code_list[i], 0);
                    input_sync(ki_data->input);
                    ki_data->key_state_list_1[i] = 0;
                    ki_data->key_hold_time_list[i] = 0;
                }
            }
        }
    }
    mod_timer(&ki_data->timer,jiffies+msecs_to_jiffies(ki_data->pdata->fuzz_time));
}

static int
key_input_open(struct inode *inode, struct file *file)
{
	file->private_data = KeyInput;
    return 0;
}

static int
key_input_release(struct inode *inode, struct file *file)
{
    file->private_data=NULL;
    return 0;
}

static const struct file_operations key_input_fops = {
    .owner      = THIS_MODULE,
    .open       = key_input_open,
    .release    = key_input_release,
};

static int register_key_input_dev(struct key_input  *ki_data)
{
    int ret=0;
    strcpy(ki_data->name,"am_key_input");
    ret=register_chrdev(0, ki_data->name, &key_input_fops);
    if (ret<=0) {
        printk(KERN_ERR "Meson KeyInput register_chrdev() error=%d\n", ret);
        return ret;
    }
    ki_data->major=ret;
    printk(KERN_INFO "Meson KeyInput major=%d\n",ret);
    ki_data->class=class_create(THIS_MODULE,ki_data->name);
    ki_data->dev=device_create(ki_data->class, NULL,
    		MKDEV(ki_data->major,0), NULL, ki_data->name);
    return ret;
}

#ifdef CONFIG_AML1212
static void long_press_power_key(struct work_struct *work)
{
    printk("%s in, power key status:%d\n", __func__, power_key_status);
    if (power_key_status) {
        printk("power key long pressed 7 s\n");
        aml_pmu_poweroff();
    }
}
#endif

static void keyinput_tasklet(unsigned long data)
{
    if (KeyInput->status) {
        input_report_key(KeyInput->input, KeyInput->pdata->key_code_list[0], 0);
        input_sync(KeyInput->input);
        printk(KERN_INFO "=== key %d up ===\n", KeyInput->pdata->key_code_list[0]);
    #ifdef CONFIG_AML1212
        if (KeyInput->pdata->key_code_list[0] == KEY_POWER) {
            power_key_status = 0;
            printk(KERN_INFO "cancel power key delay work\n");
            cancel_delayed_work_sync(&power_key_work);
        }
    #endif
    }
    else {
        input_report_key(KeyInput->input, KeyInput->pdata->key_code_list[0], 1);
        input_sync(KeyInput->input);
        printk(KERN_INFO "=== key %d down ===\n", KeyInput->pdata->key_code_list[0]);
    #ifdef CONFIG_AML1212
        if (KeyInput->pdata->key_code_list[0] == KEY_POWER) {
            power_key_status = 1;
            printk("send power key delay work\n");
            schedule_delayed_work(&power_key_work, msecs_to_jiffies(70 * 100));         // delay 7 second to check power key status

        }
    #endif
    }
}

#ifdef CONFIG_AML_RTC
extern int aml_rtc_alarm_status(void);
#endif /* CONFIG_AML_RTC */

#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6
static irqreturn_t am_key_interrupt(int irq, void *dev)
{
    int newval = (aml_read_reg32(P_AO_RTC_ADDR1)>>2)&1;
    if (KeyInput->status != newval) {
        KeyInput->status = newval;
        tasklet_schedule(&ki_tasklet);
    }
    aml_write_reg32(P_AO_RTC_ADDR1, (aml_read_reg32(P_AO_RTC_ADDR1) | (0x0000f000)));
    return IRQ_HANDLED;
}

#else // !CONFIG_ARCH_MESON6
static irqreturn_t am_key_interrupt(int irq, void *dev)
{
    int alarm = 0;
    /*
     * RTC interrupt is shared between RTC alarm and power key button.
     * We should only send key input event if power key button is pressed.
     * Read RTC alarm GPO status bit to differentiate between the two cases.
     */
#ifdef CONFIG_AML_RTC
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6
	if(((aml_read_reg32(P_AO_RTC_ADDR1)>>2)&3) == 0)
	{
		alarm = 1;
	}
#else
    alarm = (READ_AOBUS_REG(AO_RTC_ADDR1)>>3)&1;    
#endif 
    if (READ_AOBUS_REG(AO_RTI_STATUS_REG2)==0xabcd1234){
        alarm = 1;
    }
#endif /* CONFIG_AML_RTC */

#ifdef CONFIG_MESON_SUSPEND
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6
    KeyInput->status = (aml_read_reg32(P_AO_RTC_ADDR1)>>2)&1;
    if (READ_AOBUS_REG(AO_RTI_STATUS_REG2)== 0x1234abcd) {
        WRITE_AOBUS_REG(AO_RTI_STATUS_REG2,0);
        if (alarm == 0) {
            if (((aml_read_reg32(P_AO_RTC_ADDR1)>>2)&1) == 1) {
                KeyInput->status = 0;
                printk(KERN_INFO "Force key down: %x\n",aml_read_reg32(P_AO_RTC_ADDR1));
                aml_write_reg32(P_AO_RTC_ADDR1, (aml_read_reg32(P_AO_RTC_ADDR1) | (0x0000c000)));
                keyinput_tasklet(0);
                return IRQ_HANDLED;
            }
        }
    }
    aml_write_reg32(P_AO_RTC_ADDR1, (aml_read_reg32(P_AO_RTC_ADDR1) | (0x0000c000)));
#else
    KeyInput->status = (READ_AOBUS_REG(AO_RTC_ADDR1)>>2)&1;
    if (READ_AOBUS_REG(AO_RTI_STATUS_REG2)== 0x1234abcd) {
        WRITE_AOBUS_REG(AO_RTI_STATUS_REG2,0);
        if (alarm == 0) {
            if (((READ_AOBUS_REG(AO_RTC_ADDR1)>>2)&1) == 1) {
                KeyInput->status = 0;
                printk(KERN_INFO "Force key down: %x\n",READ_AOBUS_REG(AO_RTC_ADDR1));
                WRITE_AOBUS_REG(AO_RTC_ADDR1, (READ_AOBUS_REG(AO_RTC_ADDR1) | (0x0000c000)));
                keyinput_tasklet(0);
                return IRQ_HANDLED;
            }
        }
    }
    WRITE_AOBUS_REG(AO_RTC_ADDR1, (READ_AOBUS_REG(AO_RTC_ADDR1) | (0x0000c000)));
#endif    
    if (!alarm)
        tasklet_schedule(&ki_tasklet);
#else    
	if (!alarm) {
        if (KeyInput->suspend) { // when suspend
            if (READ_AOBUS_REG(AO_RTI_STATUS_REG2)!=0x12345678) {
                WRITE_AOBUS_REG(AO_RTI_STATUS_REG2, 0x12345678);
                KeyInput->status = 0;
                tasklet_schedule(&ki_tasklet);
            }
        }
        else {
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6
 			KeyInput->status = (aml_read_reg32(P_AO_RTC_ADDR1)>>2)&1;
#else        	
            KeyInput->status = (READ_AOBUS_REG(AO_RTC_ADDR1)>>2)&1;
#endif            
            tasklet_schedule(&ki_tasklet);
        }
    }
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6
	aml_write_reg32(P_AO_RTC_ADDR1, (aml_read_reg32(P_AO_RTC_ADDR1) | (0x0000c000)));
#else    
    WRITE_AOBUS_REG(AO_RTC_ADDR1, (READ_AOBUS_REG(AO_RTC_ADDR1) | (0x0000c000)));
#endif    
#endif
    return IRQ_HANDLED;
}
#endif //!CONFIG_ARCH_MESON6

static int __devinit key_input_probe(struct platform_device *pdev)
{
    struct key_input *ki_data = NULL;
    struct input_dev *input_dev = NULL;
    int i, ret = 0;
    struct key_input_platform_data *pdata = pdev->dev.platform_data;

    if (!pdata) {
        dev_err(&pdev->dev, "platform data is required!\n");
        ret = -EINVAL;
        goto CATCH_ERR;
    }
    
    ki_data = kzalloc(sizeof(struct key_input), GFP_KERNEL);
    input_dev = input_allocate_device();
    ki_data->key_state_list_0 = kzalloc((sizeof(int)*pdata->key_num), GFP_KERNEL);
    ki_data->key_state_list_1 = kzalloc((sizeof(int)*pdata->key_num), GFP_KERNEL);
    ki_data->key_hold_time_list = kzalloc((sizeof(int)*pdata->key_num), GFP_KERNEL);
    if (!ki_data || !input_dev || !ki_data->key_state_list_0 || !ki_data->key_state_list_1) {
        ret = -ENOMEM;
        goto CATCH_ERR;
    }
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6
    ki_data->status = 1;
#endif
    KeyInput = ki_data;

    platform_set_drvdata(pdev, ki_data);
    ki_data->input = input_dev;
    ki_data->pdata = pdata;

    if(ki_data->pdata->init_func != NULL)
    {
        if(ki_data->pdata->init_func() < 0)
        {
            printk(KERN_ERR "ki_data->pdata->init_func() failed.\n");
            ret = -EINVAL;
            goto CATCH_ERR;
        }
    }
    if(ki_data->pdata->fuzz_time <= 0)
    {
        ki_data->pdata->fuzz_time = 100;
    }

    if (ki_data->pdata->config == AML_KEYINPUT_POLLING) {
        setup_timer(&ki_data->timer, key_input_polling, (unsigned long)ki_data);
        mod_timer(&ki_data->timer, jiffies+msecs_to_jiffies(ki_data->pdata->fuzz_time));
    }

    /* setup input device */
    set_bit(EV_KEY, input_dev->evbit);
    set_bit(EV_REP, input_dev->evbit);

    for(i = 0; i < pdata->key_num; i++)
    {
        set_bit(pdata->key_code_list[i], input_dev->keybit);
        printk(KERN_INFO "Key %d registed.\n", pdata->key_code_list[i]);
    }
    
    input_dev->name = "key_input";
    input_dev->phys = "key_input/input0";
    input_dev->dev.parent = &pdev->dev;

    input_dev->id.bustype = BUS_ISA;
    input_dev->id.vendor = 0x0001;
    input_dev->id.product = 0x0001;
    input_dev->id.version = 0x0100;

    input_dev->rep[REP_DELAY]=0xffffffff;
    input_dev->rep[REP_PERIOD]=0xffffffff;

    input_dev->keycodesize = sizeof(unsigned short);
    input_dev->keycodemax = 0x1ff;

    ret = input_register_device(ki_data->input);
    if (ret < 0) {
        printk(KERN_ERR "Unable to register key input device.\n");
        ret = -EINVAL;
        goto CATCH_ERR;
    }

    if (ki_data->pdata->config == AML_KEYINPUT_INTR) {
        printk(KERN_INFO "Meson KeyInput register RTC interrupt");
        tasklet_enable(&ki_tasklet);
        ki_tasklet.data = (unsigned long)KeyInput;
        ret = request_irq(INT_RTC, (irq_handler_t) am_key_interrupt, IRQF_SHARED, "power key", (void*)am_key_interrupt);
		if (ret) {
            printk(KERN_ERR "Meson KeyInput request_irq failed, errno=%d\n", -ret);
            goto CATCH_ERR;
        }
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6
		aml_write_reg32(P_AO_RTC_ADDR0, (aml_read_reg32(P_AO_RTC_ADDR0) | (0x0000f000)));
#else        
        WRITE_AOBUS_REG(AO_RTC_ADDR0, (READ_AOBUS_REG(AO_RTC_ADDR0) | (0x0000c000)));
#endif        
        //enable_irq(INT_RTC);
    }
    register_key_input_dev(KeyInput);
#ifdef CONFIG_AML1212
    INIT_DELAYED_WORK(&power_key_work, long_press_power_key);
#endif
    return 0;

CATCH_ERR:
    if(input_dev)
    {
        input_free_device(input_dev);
    }
    if(ki_data->key_state_list_0)
    {
        kfree(ki_data->key_state_list_0);
    }
    if(ki_data->key_state_list_1)
    {
        kfree(ki_data->key_state_list_1);
    }
    if(ki_data->key_hold_time_list)
    {
        kfree(ki_data->key_hold_time_list);
    }
    if(ki_data)
    {
        kfree(ki_data);
    }
    return ret;
}

static int key_input_remove(struct platform_device *pdev)
{
    struct key_input *ki_data = platform_get_drvdata(pdev);

    if (ki_data->pdata->config == AML_KEYINPUT_INTR) {
        tasklet_disable(&ki_tasklet);
        tasklet_kill(&ki_tasklet);
        disable_irq(INT_RTC);
        free_irq(INT_RTC, am_key_interrupt);
    }

    input_unregister_device(ki_data->input);
    input_free_device(ki_data->input);
    unregister_chrdev(ki_data->major,ki_data->name);

    if (ki_data->class)
    {
        if (ki_data->dev)
            device_destroy(ki_data->class,MKDEV(ki_data->major,0));
        class_destroy(ki_data->class);
    }

    kfree(ki_data->key_state_list_0);
    kfree(ki_data->key_state_list_1);
    kfree(ki_data->key_hold_time_list);
    kfree(ki_data);
    KeyInput = NULL ;
    return 0;
}

#ifdef CONFIG_PM
static int key_input_suspend(struct platform_device *dev, pm_message_t state)
{
    KeyInput->suspend = 1;
#ifdef CONFIG_AML_RTC
    resume_jeff_num = 0;
#endif
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6
    free_irq(INT_RTC, (void*)am_key_interrupt);
#endif
    return 0;
}

#ifdef CONFIG_SCREEN_ON_EARLY
int power_key_pressed;
EXPORT_SYMBOL(power_key_pressed);
#endif

static int key_input_resume(struct platform_device *dev)
{
    int ret;
    KeyInput->suspend = 0;
#ifdef CONFIG_AML_RTC
    resume_jeff_num = jiffies;
#endif 
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6
    /* 0x1234abcd : woke by power button. set by uboot
     * 0x12345678 : woke by alarm. set in pm.c
     */
    if (READ_AOBUS_REG(AO_RTI_STATUS_REG2) == 0x1234abcd) {
        // power button, not alarm
        input_report_key(KeyInput->input, KeyInput->pdata->key_code_list[0], 0);
        input_sync(KeyInput->input);
        input_report_key(KeyInput->input, KeyInput->pdata->key_code_list[0], 1);
        input_sync(KeyInput->input);
		//aml_write_reg32(P_AO_RTC_ADDR0, (aml_read_reg32(P_AO_RTC_ADDR0) | (0x0000f000)));
		#ifdef CONFIG_SCREEN_ON_EARLY
		power_key_pressed = 1;
		#endif		
    }
    KeyInput->status = 1;
#else
    if (READ_AOBUS_REG(AO_RTI_STATUS_REG2) == 0x12345678) {
        if ((READ_AOBUS_REG(AO_RTC_ADDR1)>>2)&1) { // released
            KeyInput->status = 1;
            tasklet_schedule(&ki_tasklet);
        }
    }
#endif
    if(READ_AOBUS_REG(AO_RTI_STATUS_REG2) != CALL_FLAG)
        WRITE_AOBUS_REG(AO_RTI_STATUS_REG2, 0);

#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6
    ret = request_irq(INT_RTC, (irq_handler_t) am_key_interrupt,
            IRQF_SHARED, "power key", (void*)am_key_interrupt);
#endif
    return 0;
}
#else
#define key_input_suspend NULL
#define key_input_resume  NULL
#endif // CONFIG_PM

static struct platform_driver key_input_driver = {
    .probe      = key_input_probe,
    .remove     = key_input_remove,
    .suspend    = key_input_suspend,
    .resume     = key_input_resume,
    .driver     = {
        .name   = "meson-keyinput",
    },
};

static int __devinit key_input_init(void)
{
    printk(KERN_INFO "Meson KeyInput init\n");
    return platform_driver_register(&key_input_driver);
}

static void __exit key_input_exit(void)
{
    printk(KERN_INFO "Meson KeyInput exit\n");
    platform_driver_unregister(&key_input_driver);
}

module_init(key_input_init);
module_exit(key_input_exit);

MODULE_AUTHOR("Amlogic, Inc.");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Meson KeyInput Device Driver");
