/*
 * Amlogic WATCH DOG
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
 */
 
#include <linux/fs.h>
#include <linux/gfp.h>
#include <linux/io.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/watchdog.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <mach/watchdog.h>

#define WATCHDOG_NAME "aml-wdt"
#define DEFUALT_TIME_OUT 40

static struct platform_device *aml_wdt_dev;
static unsigned long driver_open;

#define	WDT_STATE_STOP	0
#define	WDT_STATE_START	1

struct aml_wdt {
	unsigned long timeout;
	unsigned int state;
	unsigned int wake;
	aml_watchdog_t* watchdog;
	spinlock_t lock;
};

static const struct watchdog_info aml_wdt_info = {
	.identity = "Amlogic Watchdog",
	.options = WDIOF_SETTIMEOUT | WDIOF_KEEPALIVEPING,
};

static void aml_wdt_start(struct aml_wdt *wdt)
{
	unsigned long flags;

	spin_lock_irqsave(&wdt->lock, flags);
	if (wdt->watchdog)
		aml_enable_watchdog(wdt->watchdog);
	spin_unlock_irqrestore(&wdt->lock, flags);
}

static void aml_wdt_stop(struct aml_wdt *wdt)
{
	unsigned long flags;

	spin_lock_irqsave(&wdt->lock, flags);
	if (wdt->watchdog)
		aml_disable_watchdog(wdt->watchdog);
	spin_unlock_irqrestore(&wdt->lock, flags);
}

static void aml_wdt_set_timeout(struct aml_wdt *wdt, unsigned long seconds)
{
	unsigned long timeout = seconds * 1000;
	unsigned long flags;
	unsigned int state;

	spin_lock_irqsave(&wdt->lock, flags);
	state = wdt->state;
	if (wdt->watchdog)
		aml_disable_watchdog(wdt->watchdog);
	aml_set_watchdog_timeout_ms(wdt->watchdog, timeout);

	if (state == WDT_STATE_START)
		aml_enable_watchdog(wdt->watchdog);

	wdt->timeout = timeout;
	spin_unlock_irqrestore(&wdt->lock, flags);
}

static void aml_wdt_get_timeout(struct aml_wdt *wdt, unsigned long *seconds)
{
	*seconds = wdt->timeout / 1000;
}

static void aml_wdt_keepalive(struct aml_wdt *wdt)
{
	unsigned long flags;

	spin_lock_irqsave(&wdt->lock, flags);
	if (wdt->watchdog) {
		pr_info("name is %s\n", wdt->watchdog->name);
		aml_reset_watchdog(wdt->watchdog);
	}
	spin_unlock_irqrestore(&wdt->lock, flags);
}

static int aml_wdt_open(struct inode *inode, struct file *file)
{
	struct aml_wdt *wdt = platform_get_drvdata(aml_wdt_dev);

	if (test_and_set_bit(0, &driver_open))
		return -EBUSY;

	file->private_data = wdt;
	aml_wdt_set_timeout(wdt, DEFUALT_TIME_OUT);
	aml_wdt_start(wdt);

	return nonseekable_open(inode, file);
}

static int aml_wdt_release(struct inode *inode, struct file *file)
{
	struct aml_wdt *wdt = file->private_data;

	aml_wdt_stop(wdt);
	clear_bit(0, &driver_open);

	return 0;
}

static long aml_wdt_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct aml_wdt *wdt = file->private_data;
	void __user *argp = (void __user *)arg;
	unsigned long __user *p = argp;
	unsigned long seconds = 0;
	unsigned int options;
	long ret = -EINVAL;

	switch (cmd) {
	case WDIOC_GETSUPPORT:
		if (copy_to_user(argp, &aml_wdt_info, sizeof(aml_wdt_info)))
			return -EFAULT;
		else
			return 0;

	case WDIOC_GETSTATUS:
	case WDIOC_GETBOOTSTATUS:
		return put_user(0, p);

	case WDIOC_KEEPALIVE:
		aml_wdt_keepalive(wdt);
		return 0;

	case WDIOC_SETTIMEOUT:
		if (get_user(seconds, p))
			return -EFAULT;

		aml_wdt_set_timeout(wdt, seconds);

		/* fallthrough */
	case WDIOC_GETTIMEOUT:
		aml_wdt_get_timeout(wdt, &seconds);
		return put_user(seconds, p);

	case WDIOC_SETOPTIONS:
		if (copy_from_user(&options, argp, sizeof(options)))
			return -EFAULT;

		if (options & WDIOS_DISABLECARD) {
			aml_wdt_stop(wdt);
			ret = 0;
		}

		if (options & WDIOS_ENABLECARD) {
			aml_wdt_start(wdt);
			ret = 0;
		}

		return ret;

	default:
		break;
	}

	return -ENOTTY;
}

static ssize_t aml_wdt_write(struct file *file, const char __user *data,
		size_t len, loff_t *ppos)
{
	struct aml_wdt *wdt = file->private_data;
	
	if (len)
		aml_wdt_keepalive(wdt);
		
	return len;
}

static const struct file_operations aml_wdt_fops = {
	.owner = THIS_MODULE,
	.llseek = no_llseek,
	.open = aml_wdt_open,
	.release = aml_wdt_release,
	.unlocked_ioctl = aml_wdt_ioctl,
	.write = aml_wdt_write,
};

static struct miscdevice aml_wdt_miscdev = {
	.minor = WATCHDOG_MINOR,
	.name = "watchdog",
	.fops = &aml_wdt_fops,
};

static int __devinit aml_wdt_probe(struct platform_device *pdev)
{
	struct aml_wdt *wdt;
	int ret = 0;

	wdt = kzalloc(sizeof(*wdt), GFP_KERNEL);
	if (!wdt) {
		pr_err("cannot allocate WDT structure\n");
		return -ENOMEM;
	}
	wdt->watchdog = aml_create_watchdog("aml_wdt");
	if (!wdt->watchdog) {
		pr_err("cannot create aml watchdog\n");
		kfree(wdt);
		return -ENOMEM;
	}
	printk("aml_wdt name is %s \n", wdt->watchdog->name);

	spin_lock_init(&wdt->lock);

	/* disable watchdog and reboot on timeout */
	aml_disable_watchdog(wdt->watchdog);
	aml_reset_watchdog(wdt->watchdog);

	platform_set_drvdata(pdev, wdt);

	ret = misc_register(&aml_wdt_miscdev);
	if (ret) {
		pr_err("cannot register miscdev on minor %d "
				"(err=%d)\n", WATCHDOG_MINOR, ret);
		return ret;
	}

	return 0;
}

static int __devexit aml_wdt_remove(struct platform_device *pdev)
{
	struct aml_wdt *wdt = platform_get_drvdata(pdev);

	misc_deregister(&aml_wdt_miscdev);
	aml_wdt_stop(wdt);
	platform_set_drvdata(pdev, NULL);
	aml_destroy_watchdog(wdt->watchdog);
	kfree(wdt);

	return 0;
}

static void aml_wdt_shutdown(struct platform_device *pdev)
{
	struct aml_wdt *wdt = platform_get_drvdata(pdev);
	aml_wdt_stop(wdt);
}

#ifdef CONFIG_PM
static int aml_wdt_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct aml_wdt *wdt = platform_get_drvdata(pdev);

	wdt->wake = (wdt->state == WDT_STATE_START) ? 1 : 0;
	aml_wdt_stop(wdt);

	return 0;
}

static int aml_wdt_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct aml_wdt *wdt = platform_get_drvdata(pdev);

	if (wdt->wake)
		aml_wdt_start(wdt);

	return 0;
}

static const struct dev_pm_ops aml_wdt_pm_ops = {
	.suspend = aml_wdt_suspend,
	.resume = aml_wdt_resume,
};

#  define AML_WDT_PM_OPS	(&aml_wdt_pm_ops)
#else
#  define AML_WDT_PM_OPS	NULL
#endif

static struct platform_driver aml_wdt_driver = {
	.probe = aml_wdt_probe,
	.remove = __devexit_p(aml_wdt_remove),
	.shutdown = aml_wdt_shutdown,
	.driver = {
		.name = WATCHDOG_NAME,
		.owner = THIS_MODULE,
		.pm = AML_WDT_PM_OPS,
	},
};

static int __init aml_wdt_init(void)
{
	int err;

	printk(KERN_INFO
	     "WDT driver for Amlogic platform initialising.\n");
	     
	err = platform_driver_register(&aml_wdt_driver);
	if (err)
		return err;
	
	aml_wdt_dev = platform_device_register_simple(WATCHDOG_NAME,
							-1, NULL, 0);
	if (IS_ERR(aml_wdt_dev)) {
		err = PTR_ERR(aml_wdt_dev);
		goto unreg_platform_driver;
	}
	
	return 0;

unreg_platform_driver:
	platform_driver_unregister(&aml_wdt_driver);
	return err;
}

static void __exit aml_wdt_exit(void)
{
	platform_driver_unregister(&aml_wdt_driver);
}

module_init(aml_wdt_init);
module_exit(aml_wdt_exit);

MODULE_DESCRIPTION("Amlogic WATCH DOG");
MODULE_LICENSE("GPL");
MODULE_ALIAS_MISCDEV(WATCHDOG_MINOR);