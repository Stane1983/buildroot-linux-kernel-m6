/*
 * TVIN Tuner Bridge Driver
 *
 * Author: Bobby Yang <bo.yang@amlogic.com>
 *
 *
 * Copyright (C) 2010 Amlogic Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


/* Standard Linux Headers */
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include <linux/sysctl.h>



/* Amlogic Headers */
#include <linux/tvin/tvin.h>

/* Driver Headers */
#include "tvin_tuner_driver.h"
#include "tvin_tuner.h"
#include "tvin_demod.h"


#define TUNER_DEVICE_NAME   "tuner"
#define TUNER_CLASS_NAME    "tuner"

typedef struct tuner_device_s {
    struct class            *clsp;
    dev_t                   devno;
    struct cdev             cdev;
    //struct i2c_adapter      *adap;
    //struct i2c_client       *tuner;
    //struct i2c_client       *demod;

    /* reserved for futuer abstract */
    struct tvin_tuner_ops_s *tops;
} tuner_device_t;


static struct tuner_device_s *devp;


static ssize_t tuner_name_show(struct class *class,
    struct class_attribute *attr, char *buf)
{
    int len = 0;
    len += sprintf(buf+len, "%s\n", tvin_tuenr_get_name());
    return len;
}

static CLASS_ATTR(tuner_name, 0644, tuner_name_show, NULL);

static int tuner_open(struct inode *inode, struct file *file)
{
    struct tuner_device_s *devp_o;
//    pr_info( "%s . \n", __FUNCTION__ );
    /* Get the per-device structure that contains this cdev */
    devp_o = container_of(inode->i_cdev, tuner_device_t, cdev);
    file->private_data = devp_o;

    return 0;
}

static int tuner_release(struct inode *inode, struct file *file)
{

    file->private_data = NULL;

    //    pr_info( "%s . \n", __FUNCTION__ );
    return 0;
}

static int tuner_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    struct tuner_device_s *devp_c;
    void __user *argp = (void __user *)arg;

	if (_IOC_TYPE(cmd) != TVIN_IOC_MAGIC) {
		return -EINVAL;
	}

    devp_c = container_of(inode->i_cdev, tuner_device_t, cdev);
//    pr_info("%s:  \n",__FUNCTION__);
    switch (cmd)
    {
        case TVIN_IOC_G_TUNER_STD:
            {
                tuner_std_id std_id = 0;
                tvin_tuner_get_std(&std_id);
                //pr_info("TVIN_IOC_G_TUNER_STD: %lx, \n", std_id);
    		    if (copy_to_user((void __user *)arg, &std_id, sizeof(tuner_std_id)))
    		    {
                    pr_err("%s: TVIN_IOC_G_TUNER_STD para is error. \n " ,__FUNCTION__);
                    ret = -EFAULT;
    		    }
            }
            break;
        case TVIN_IOC_S_TUNER_STD:
        {
            tuner_std_id std_id = 0;
            if (copy_from_user(&std_id, argp, sizeof(tuner_std_id)))
            {
                pr_err("%s: TVIN_IOC_S_TUNER_STD para is error. \n ",__FUNCTION__);
                ret = -EFAULT;
                break;
            }
            pr_info("TVIN_IOC_S_TUNER_STD: 0x%08x%08x\n", (unsigned int)(std_id >> 32), (unsigned int)std_id);
            tvin_tuner_set_std(std_id);
            tvin_demod_set_std(std_id);
            tvin_set_tuner();
            tvin_set_demod(0);

        }
            break;
        case TVIN_IOC_G_TUNER_FREQ:
        {
            struct tuner_freq_s tuner_freq = {0, 0};
            tvin_tuner_get_freq(&tuner_freq);
            //pr_info(" TVIN_IOC_G_TUNER_FREQ: %d, \n", tuner_freq.freq);
		    if (copy_to_user((void __user *)arg, &tuner_freq, sizeof(struct tuner_freq_s)))
		    {
                pr_err("%s: TVIN_IOC_G_TUNER_FREQ para is error. \n " ,__FUNCTION__);
                ret = -EFAULT;
		    }

        }
            break;
        case TVIN_IOC_S_TUNER_FREQ:
        {
            struct tuner_freq_s tuner_freq = {0, 0};
            if (copy_from_user(&tuner_freq, argp, sizeof(struct tuner_freq_s)))
            {
                pr_err("%s: TVIN_IOC_S_TUNER_FREQ para is error. \n " ,__FUNCTION__);
                ret = -EFAULT;
                break;
            }
            pr_info("TVIN_IOC_S_TUNER_FREQ: freq = %d, reserved = %d\n", tuner_freq.freq, tuner_freq.reserved);
            tvin_tuner_set_freq(tuner_freq);
            tvin_set_tuner();
            tvin_set_demod(tuner_freq.reserved);
        }
            break;
        case TVIN_IOC_G_TUNER_PARM:
        {
            struct tuner_parm_s tuner_parm = {0, 0, 0, 0, 0};
            tvin_get_tuner(&tuner_parm);
            tvin_demod_get_afc(&tuner_parm);
		    if (copy_to_user((void __user *)arg, &tuner_parm, sizeof(struct tuner_parm_s)))
		    {
                pr_err("%s: TVIN_IOC_G_TUNER_PARM para is error. \n " ,__FUNCTION__);
                ret = -EFAULT;
		    }
        }
            break;
#ifdef CONFIG_TVIN_TUNER_SI2176
        case TVIN_IOC_G_TUNER_STATUS:
        {
            struct si2176_tuner_status_s si2176_tuner_status;
            memset(&si2176_tuner_status,0,sizeof(struct si2176_tuner_status_s));
            si2176_get_tuner_status(&si2176_tuner_status);
            if(copy_to_user((void __user *)arg,&si2176_tuner_status,sizeof(struct si2176_tuner_status_s)))
            {
                pr_err("%s: TVIN_IOC_G_TUNER_STATUS para is error. \n " ,__FUNCTION__);
                ret = -EFAULT;
            }
        }
            break;
#endif
        default:
            ret = -ENOIOCTLCMD;
            break;
    }

    return ret;
}

static struct file_operations tuner_fops = {
    .owner   = THIS_MODULE,
    .open    = tuner_open,
    .release = tuner_release,
    .ioctl   = tuner_ioctl,
};


static int tuner_device_init(struct tuner_device_s *devp)
{
    int ret;
    struct device *devp_;

    devp = kmalloc(sizeof(struct tuner_device_s), GFP_KERNEL);
    if (!devp)
    {
        printk(KERN_ERR "failed to allocate memory\n");
        return -ENOMEM;
    }

    ret = alloc_chrdev_region(&devp->devno, 0, 1, TUNER_DEVICE_NAME);
	if (ret < 0) {
		printk(KERN_ERR "failed to allocate major number\n");
        kfree(devp);
		return -1;
	}
    devp->clsp = class_create(THIS_MODULE, TUNER_CLASS_NAME);
    if (IS_ERR(devp->clsp))
    {
        unregister_chrdev_region(devp->devno, 1);
        kfree(devp);
        ret = (int)PTR_ERR(devp->clsp);
        return -1;
    }
    cdev_init(&devp->cdev, &tuner_fops);
    devp->cdev.owner = THIS_MODULE;
    ret = cdev_add(&devp->cdev, devp->devno, 1);
    if (ret) {
        printk(KERN_ERR "failed to add device\n");
        class_destroy(devp->clsp);
        unregister_chrdev_region(devp->devno, 1);
        kfree(devp);
        return ret;
    }
    devp_ = device_create(devp->clsp, NULL, devp->devno , NULL, "tuner%d", 0);
    if (IS_ERR(devp_)) {
        pr_err("failed to create device node\n");
        class_destroy(devp->clsp);
        unregister_chrdev_region(devp->devno, 1);
        kfree(devp);

        /* @todo do with error */
        return PTR_ERR(devp);;
    }
    class_create_file(devp->clsp, &class_attr_tuner_name);
    return 0;
}

static void tuner_device_release(struct tuner_device_s *devp)
{
    cdev_del(&devp->cdev);
    device_destroy(devp->clsp, devp->devno );
    cdev_del(&devp->cdev);
    class_destroy(devp->clsp);
    unregister_chrdev_region(devp->devno, 1);
    kfree(devp);
}


static int tuner_probe(struct platform_device *pdev)
{
    int ret;
    pr_info("%s: \n", __FUNCTION__);
    ret = tuner_device_init(devp);
    if (ret)
    {
        pr_info(" device init failed\n");
        return ret;
    }

    printk("tuner_probe ok.\n");
    return 0;
}

static int tuner_remove(struct platform_device *pdev)
{
    tuner_device_release(devp);
    printk(KERN_ERR "driver removed ok.\n");

    return 0;
}

static struct platform_driver tuner_driver = {
    .probe      = tuner_probe,
    .remove     = tuner_remove,
    .driver     = {
        .name   = TUNER_DEVICE_NAME,
    }
};


static int __init tuner_init(void)
{
    int ret = 0;
    pr_info( "%s . \n", __FUNCTION__ );
    ret = platform_driver_register(&tuner_driver);
    if (ret != 0) {
        printk(KERN_ERR "failed to register module, error %d\n", ret);
        return -ENODEV;
    }
    return ret;
}

static void __exit tuner_exit(void)
{
    pr_info( "%s . \n", __FUNCTION__ );

    platform_driver_unregister(&tuner_driver);
}

module_init(tuner_init);
module_exit(tuner_exit);

MODULE_DESCRIPTION("Amlogic Analog tuner Driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Bo Yang <bo.yang@amlogic.com>");

