/*
 *  amlGsensor.c - Linux kernel modules for 3-Axis Orientation/Motion
 *  Detection Sensor 
 *
 *  Copyright (C) 2011-2012 Amlogic Ltd.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#include "aml_Gsensor.h"


#define DBG_LEV 1

#define AML_GSENSOR_DRV_NAME	"amlGsensor"
#define DEF_MAX_DALAY 200

#define ABSMIN				-512
#define ABSMAX				512

#define LOW_G_INTERRUPT				REL_Z
#define HIGH_G_INTERRUPT 			REL_HWHEEL
#define SLOP_INTERRUPT 				REL_DIAL
#define DOUBLE_TAP_INTERRUPT 			REL_WHEEL
#define SINGLE_TAP_INTERRUPT 			REL_MISC
#define ORIENT_INTERRUPT 			ABS_PRESSURE
#define FLAT_INTERRUPT 				ABS_DISTANCE

struct amlGsensor_ops *gp_amlGsensor_ops = NULL ;

struct amlGsensor_workdata {
	struct i2c_client *amlGsensor_client;
	atomic_t delay;
	atomic_t enable;
	atomic_t selftest_result;
	unsigned char mode;

	struct input_dev *input;
	struct amlGsensor_acc value;
	struct mutex value_mutex;
	struct mutex enable_mutex;
	struct mutex mode_mutex;
	struct delayed_work work;
	struct work_struct irq_work;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
	int IRQ;
};

#ifdef CONFIG_HAS_EARLYSUSPEND
static void amlGsensor_early_suspend(struct early_suspend *h);
static void amlGsensor_late_resume(struct early_suspend *h);
#endif


struct amlGsensor_workdata *gp_amlGsensor_workdata = NULL;


static int amlGsensor_set_range(struct i2c_client *client, unsigned char Range)
{
    return gp_amlGsensor_ops->set_range(client,Range);
}

static int amlGsensor_get_range(struct i2c_client *client, unsigned char *Range)
{
    return gp_amlGsensor_ops->get_range(client,Range);
}



static int amlGsensor_set_workmode(struct i2c_client *client, unsigned char Mode)
{
    return gp_amlGsensor_ops->set_workmode(client,Mode);
}

static int amlGsensor_get_workmode(struct i2c_client *client, unsigned char *Mode)
{
    return gp_amlGsensor_ops->get_workmode(client,Mode);
}

static int amlGsensor_start_work(struct i2c_client *client)
{
    return gp_amlGsensor_ops->start_work(client);
}

static int amlGsensor_stop_work(struct i2c_client *client)
{
    return gp_amlGsensor_ops->stop_work(client);
}

static int amlGsensor_read_accel_x(struct i2c_client *client, short *a_x)
{
    return gp_amlGsensor_ops->read_accel_x(client,a_x);
}
static int amlGsensor_read_accel_y(struct i2c_client *client, short *a_y)
{
    return gp_amlGsensor_ops->read_accel_y(client,a_y);
}

static int amlGsensor_read_accel_z(struct i2c_client *client, short *a_z)
{
    return gp_amlGsensor_ops->read_accel_z(client,a_z);
}


static int amlGsensor_read_accel_xyz(struct i2c_client *client,struct amlGsensor_acc *acc)
{
    return gp_amlGsensor_ops->read_accel_xyz(client,acc);
}

static void amlGsensor_work_func(struct work_struct *work)
{
    struct amlGsensor_workdata *amlGsensor = container_of((struct delayed_work *)work,
    		struct amlGsensor_workdata, work);
    static struct amlGsensor_acc acc;
    unsigned long delay = msecs_to_jiffies(atomic_read(&amlGsensor->delay));

    amlGsensor_read_accel_xyz(amlGsensor->amlGsensor_client, &acc);
    input_report_abs(amlGsensor->input, ABS_X, acc.x);
    input_report_abs(amlGsensor->input, ABS_Y, acc.y);
    input_report_abs(amlGsensor->input, ABS_Z, acc.z);
    input_sync(amlGsensor->input);

    if(DBG_LEV>1)
        printk("acc.x=%d,acc.y=%d,acc.z=%d \n",amlGsensor->value.x,amlGsensor->value.y,amlGsensor->value.z);  

    schedule_delayed_work(&amlGsensor->work, delay);
}

static void amlGsensor_set_enable(struct device *dev, int enable)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct amlGsensor_workdata *amlGsensor = i2c_get_clientdata(client);
    int pre_enable = atomic_read(&amlGsensor->enable);

    if(DBG_LEV)
        printk("amlGsensor_set_enable %d \n",enable);

    mutex_lock(&amlGsensor->enable_mutex);
    if (enable) {
        if (pre_enable == 0) {
            amlGsensor_start_work(amlGsensor->amlGsensor_client);
            schedule_delayed_work(&amlGsensor->work, msecs_to_jiffies(atomic_read(&amlGsensor->delay)));
            atomic_set(&amlGsensor->enable, 1);
        }
    } else {
        if (pre_enable == 1) {
            amlGsensor_stop_work(amlGsensor->amlGsensor_client);
            cancel_delayed_work_sync(&amlGsensor->work);
            atomic_set(&amlGsensor->enable, 0);
        }
    }
    mutex_unlock(&amlGsensor->enable_mutex);

}

static ssize_t amlGsensor_range_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
    unsigned char data;
    struct i2c_client *client = to_i2c_client(dev);
    struct amlGsensor_workdata *amlGsensor = i2c_get_clientdata(client);

    if (amlGsensor_get_range(amlGsensor->amlGsensor_client, &data) < 0)
        return sprintf(buf, "Read error\n");

    return sprintf(buf, "%d\n", data);
}

static ssize_t amlGsensor_range_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
    unsigned long data;
    int error;
    struct i2c_client *client = to_i2c_client(dev);
    struct amlGsensor_workdata *amlGsensor = i2c_get_clientdata(client);

    error = strict_strtoul(buf, 10, &data);
    if (error)
        return error;
    if (amlGsensor_set_range(amlGsensor->amlGsensor_client, (unsigned char) data) < 0)
        return -EINVAL;

    return count;
}

static ssize_t amlGsensor_workmode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
    unsigned char data;
    struct i2c_client *client = to_i2c_client(dev);
    struct amlGsensor_workdata *amlGsensor = i2c_get_clientdata(client);

    if (amlGsensor_get_workmode(amlGsensor->amlGsensor_client, &data) < 0)
        return sprintf(buf, "Read error\n");

    return sprintf(buf, "%d\n", data);
}

static ssize_t amlGsensor_workmode_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
    unsigned long data;
    int error;
    struct i2c_client *client = to_i2c_client(dev);
    struct amlGsensor_workdata *amlGsensor = i2c_get_clientdata(client);

    error = strict_strtoul(buf, 10, &data);
    if (error)
        return error;
    if (amlGsensor_set_workmode(amlGsensor->amlGsensor_client, (unsigned char) data) < 0)
        return -EINVAL;

    return count;
}

static ssize_t amlGsensor_delay_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct amlGsensor_workdata *amlGsensor = i2c_get_clientdata(client);

    return sprintf(buf, "%d\n", atomic_read(&amlGsensor->delay));
}

static ssize_t amlGsensor_delay_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
    unsigned long data;
    int error;
    struct i2c_client *client = to_i2c_client(dev);
    struct amlGsensor_workdata *amlGsensor = i2c_get_clientdata(client);

    error = strict_strtoul(buf, 10, &data);
    if (error)
    	return error;
    if (data > DEF_MAX_DALAY)
    	data = DEF_MAX_DALAY;
    atomic_set(&amlGsensor->delay, (unsigned int) data);

    return count;
}


static ssize_t amlGsensor_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct amlGsensor_workdata *amlGsensor = i2c_get_clientdata(client);

    return sprintf(buf, "%d\n", atomic_read(&amlGsensor->enable));
}

static ssize_t amlGsensor_enable_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long data;
	int error;

	error = strict_strtoul(buf, 10, &data);
	if (error)
		return error;
	if ((data == 0) || (data == 1))
		amlGsensor_set_enable(dev, data);

	return count;
}

static ssize_t amlGsensor_register_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	size_t count=0;
	if(gp_amlGsensor_ops->all_registers_show)
		count = gp_amlGsensor_ops->all_registers_show(gp_amlGsensor_workdata->amlGsensor_client,buf);	
    
    return count;
}

static ssize_t amlGsensor_register_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	if(gp_amlGsensor_ops->set_register)
    	gp_amlGsensor_ops->set_register(gp_amlGsensor_workdata->amlGsensor_client,buf);
    return count;
}


static DEVICE_ATTR(range, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
		amlGsensor_range_show, amlGsensor_range_store);
static DEVICE_ATTR(mode, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
		amlGsensor_workmode_show, amlGsensor_workmode_store);
static DEVICE_ATTR(delay, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
		amlGsensor_delay_show, amlGsensor_delay_store);
static DEVICE_ATTR(enable, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
		amlGsensor_enable_show, amlGsensor_enable_store);
static DEVICE_ATTR(reg, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
		amlGsensor_register_show, amlGsensor_register_store);

static struct attribute *amlGsensor_attributes[] = {
    &dev_attr_range.attr,
    &dev_attr_mode.attr,
    &dev_attr_delay.attr,
    &dev_attr_enable.attr,
    &dev_attr_reg.attr,
    NULL
};

static struct attribute_group amlGsensor_attribute_group = {
    .attrs = amlGsensor_attributes
};


static int amlGsensor_init_client(struct i2c_client *client)
{
    gp_amlGsensor_ops->init(client);

    return 0;
}


static int amlGsensor_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
    int err = 0;
    unsigned char tempvalue;
    struct amlGsensor_workdata *data;
    struct input_dev *dev;

    printk("Gsensor Device amlGsensor_probe!\n");

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        printk(KERN_INFO "i2c_check_functionality error\n");
        goto exit;
    }
    data = kzalloc(sizeof(struct amlGsensor_workdata), GFP_KERNEL);
    if (!data) {
        err = -ENOMEM;
        goto exit;
    }

    gp_amlGsensor_ops = get_amlGsensor_ops();
    if(NULL == gp_amlGsensor_ops){
        printk("can't get amlGsensor_ops!!!!\n");
        return -1;
    }
    
    /* read chip id */
    tempvalue = i2c_smbus_read_byte_data(client, gp_amlGsensor_ops->chip_id_addr);

    if (tempvalue == gp_amlGsensor_ops->chip_id) {
        printk(KERN_INFO "sensor Device detected!\n"
            "amlGsensor registered I2C driver!\n");
    } else{
        printk(KERN_INFO "sensor Sensortec Device not found"
            "i2c error %d \n", tempvalue);
        err = -ENODEV;
        goto kfree_exit;
    }
    i2c_set_clientdata(client, data);
    data->amlGsensor_client = client;
    mutex_init(&data->value_mutex);
    mutex_init(&data->mode_mutex);
    mutex_init(&data->enable_mutex);

    amlGsensor_init_client(client);

    gp_amlGsensor_workdata = data ;

    INIT_DELAYED_WORK(&data->work, amlGsensor_work_func);
    atomic_set(&data->delay, DEF_MAX_DALAY);
    atomic_set(&data->enable, 0);

    dev = input_allocate_device();
    if (!dev)
        return -ENOMEM;
    dev->name = AML_GSENSOR_DRV_NAME;
    dev->id.bustype = BUS_I2C;

    input_set_capability(dev, EV_REL, LOW_G_INTERRUPT);
    input_set_capability(dev, EV_REL, HIGH_G_INTERRUPT);
    input_set_capability(dev, EV_REL, SLOP_INTERRUPT);
    input_set_capability(dev, EV_REL, DOUBLE_TAP_INTERRUPT);
    input_set_capability(dev, EV_REL, SINGLE_TAP_INTERRUPT);
    input_set_capability(dev, EV_ABS, ORIENT_INTERRUPT);
    input_set_capability(dev, EV_ABS, FLAT_INTERRUPT);
    input_set_abs_params(dev, ABS_X, ABSMIN, ABSMAX, 0, 0);
    input_set_abs_params(dev, ABS_Y, ABSMIN, ABSMAX, 0, 0);
    input_set_abs_params(dev, ABS_Z, ABSMIN, ABSMAX, 0, 0);

    input_set_drvdata(dev, data);

    err = input_register_device(dev);
    if (err < 0) {
        input_free_device(dev);
        goto kfree_exit;
    }

    data->input = dev;

    err = sysfs_create_group(&data->input->dev.kobj,
        &amlGsensor_attribute_group);
    if (err < 0)
        goto error_sysfs;

#ifdef CONFIG_HAS_EARLYSUSPEND
    data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
    data->early_suspend.suspend = amlGsensor_early_suspend;
    data->early_suspend.resume = amlGsensor_late_resume;
    register_early_suspend(&data->early_suspend);
#endif

    mutex_init(&data->value_mutex);
    mutex_init(&data->mode_mutex);
    mutex_init(&data->enable_mutex);

    return 0;

error_sysfs:
    input_unregister_device(data->input);

kfree_exit:
    kfree(data);
exit:
    return err;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void amlGsensor_early_suspend(struct early_suspend *h)
{
	struct amlGsensor_workdata *data =
		container_of(h, struct amlGsensor_workdata, early_suspend);

	mutex_lock(&data->enable_mutex);
	if (atomic_read(&data->enable) == 1) {
		amlGsensor_stop_work(data->amlGsensor_client);
		cancel_delayed_work_sync(&data->work);
	}
	mutex_unlock(&data->enable_mutex);
}


static void amlGsensor_late_resume(struct early_suspend *h)
{
	struct amlGsensor_workdata *data =
		container_of(h, struct amlGsensor_workdata, early_suspend);

	mutex_lock(&data->enable_mutex);
	if (atomic_read(&data->enable) == 1) {
		amlGsensor_start_work(data->amlGsensor_client);
		schedule_delayed_work(&data->work,
				msecs_to_jiffies(atomic_read(&data->delay)));
	}
	mutex_unlock(&data->enable_mutex);
}
#endif

static int __devexit amlGsensor_remove(struct i2c_client *client)
{
	struct amlGsensor_workdata *data = i2c_get_clientdata(client);

	amlGsensor_set_enable(&client->dev, 0);
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&data->early_suspend);
#endif
       sysfs_remove_group(&data->input->dev.kobj, &amlGsensor_attribute_group);
	input_unregister_device(data->input);
	kfree(data);

	return 0;
}
#ifdef CONFIG_PM

static int amlGsensor_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct amlGsensor_workdata *data = i2c_get_clientdata(client);

	mutex_lock(&data->enable_mutex);
	if (atomic_read(&data->enable) == 1) {
		amlGsensor_stop_work(data->amlGsensor_client);
		cancel_delayed_work_sync(&data->work);
	}
	mutex_unlock(&data->enable_mutex);

	return 0;
}

static int amlGsensor_resume(struct i2c_client *client)
{
	struct amlGsensor_workdata *data = i2c_get_clientdata(client);

	mutex_lock(&data->enable_mutex);
	if (atomic_read(&data->enable) == 1) {
		amlGsensor_start_work(data->amlGsensor_client);
		schedule_delayed_work(&data->work,
				msecs_to_jiffies(atomic_read(&data->delay)));
	}
	mutex_unlock(&data->enable_mutex);

	return 0;
}

#else

#define amlGsensor_suspend		NULL
#define amlGsensor_resume		NULL

#endif /* CONFIG_PM */

static const struct i2c_device_id amlGsensor_id[] = {
	{ AML_GSENSOR_DRV_NAME, 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, amlGsensor_id);

static struct i2c_driver amlGsensor_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= AML_GSENSOR_DRV_NAME,
	},
	.suspend	= amlGsensor_suspend,
	.resume		= amlGsensor_resume,
	.id_table	= amlGsensor_id,
	.probe		= amlGsensor_probe,
	.remove		= __devexit_p(amlGsensor_remove),

};

static int __init amlGsensor_init(void)
{
	return i2c_add_driver(&amlGsensor_driver);
}

static void __exit amlGsensor_exit(void)
{
	i2c_del_driver(&amlGsensor_driver);
}

MODULE_AUTHOR("alex.deng <alex.deng@amlogic.com>");
MODULE_DESCRIPTION("aml accelerometer sensor driver");
MODULE_LICENSE("GPL");

module_init(amlGsensor_init);
module_exit(amlGsensor_exit);


