/*
 * EP94M3 3-Port MHL/HDMI Dual Mode Switcher
 *
 * Author: Bobby Yang <bo.yang@amlogic.com>
 *
 *
 * Copyright (C) 2013 Amlogic Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */



#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>

#include "CEC_Convert.h"
#include "MHL_If.h"
#include "EP94M3_I2C.h"

static struct class *ep94m3_clsp;

struct i2c_adapter *adapter;

#define EP94M3_WORKTIMER_INT			(HZ/10)	// 100ms
struct timer_list worktimer;

static struct workqueue_struct *workqueue = NULL;
static struct work_struct taskwork;

static bool work_enable = false;
module_param(work_enable, bool, 0644);

int work_mode = 0;	// 0 HDMI, 1 MHL
module_param(work_mode, int, 0644);

//static int work_port = 0;	// 0/1/2 or reserved: 3
//module_param(work_port, int, 0644);


//
// Global Data
//
unsigned int StopTimer = 0;


static ssize_t ep94m3_reg_rd_show(struct class *class,
	struct class_attribute *attr, char *buf)
{
	int len = 0;
	unsigned char Reg;
	unsigned char Value[16];

	Reg = EP94M0_General_Control_0;
	EP94M3_I2C_Reg_Rd(Reg, Value, 1);
	len += sprintf(buf+len, "0x%hhx = 0x%02hhx\n", Reg, Value[0]);

	Reg = EP94M0_General_Control_1;
	EP94M3_I2C_Reg_Rd(Reg, Value, 1);
	len += sprintf(buf+len, "0x%hhx = 0x%02hhx\n", Reg, Value[0]);

	Reg = EP94M0_RX_PHY_Control;
	EP94M3_I2C_Reg_Rd(Reg, Value, 3);
	len += sprintf(buf+len, "0x%hhx = 0x%02hhx, 0x%02hhx, 0x%02hhx\n", Reg, Value[0], Value[1], Value[2]);

	Reg = EP94M0_TX_PHY_Control;
	EP94M3_I2C_Reg_Rd(Reg, Value, 2);
	len += sprintf(buf+len, "0x%hhx = 0x%02hhx, 0x%02hhx\n", Reg, Value[0], Value[1]);

	Reg = EP94M0_Serial_Data_Stream;
	EP94M3_I2C_Reg_Rd(Reg, Value, 6);
	len += sprintf(buf+len, "0x%hhx = 0x%02hhx, 0x%02hhx, 0x%02hhx, 0x%02hhx, 0x%02hhx, 0x%02hhx\n", Reg,
		Value[0], Value[1], Value[2], Value[3], Value[4], Value[5]);

	Reg = EP94M0_CBUS_MSC_Dec_Status;
	EP94M3_I2C_Reg_Rd(Reg, Value, 4);
	len += sprintf(buf+len, "0x%hhx = 0x%02hhx, 0x%02hhx, 0x%02hhx, 0x%02hhx\n", Reg,
		Value[0], Value[1], Value[2], Value[3]);

	Reg = EP94M0_CBUS_Vendor_ID;
	EP94M3_I2C_Reg_Rd(Reg, Value, 1);
	len += sprintf(buf+len, "0x%hhx = 0x%02hhx\n", Reg, Value[0]);

	return len;
}

static ssize_t ep94m3_reg_rd_store(struct class *cls,
	struct class_attribute *attr, const char *buffer, size_t count)
{
	unsigned char Reg;
	unsigned int Size;
	unsigned char Value[16];
	int i = 0;

	sscanf(buffer, "0x%hhx %u", &Reg, &Size);
	pr_info("%s Reg=0x%x Size=%u\n", __func__, Reg, Size);

	EP94M3_I2C_Reg_Rd(Reg, Value, Size);

	for (i = 0; i < Size; i++) {
		pr_info("%s Value[%2d]=%hhx\n", __func__, i, Value[i]);
	}

	return count;
}


static ssize_t ep94m3_reg_wr_store(struct class *cls,
	struct class_attribute *attr, const char *buffer, size_t count)
{
	unsigned char Reg;
	unsigned char Value;

	sscanf(buffer, "0x%hhx %hhx", &Reg, &Value);
	pr_info("%s Reg=0x%x Value=%hx\n", __func__, (unsigned int)Reg, Value);

	EP94M3_I2C_Reg_Wr(Reg, &Value, 1);

	return count;
}



static CLASS_ATTR(reg_rd, S_IWUSR | S_IRUGO, ep94m3_reg_rd_show, ep94m3_reg_rd_store);
static CLASS_ATTR(reg_wr, S_IWUSR | S_IRUGO, ep94m3_reg_rd_show, ep94m3_reg_wr_store);

static void ep94m3_worktimer_func(unsigned long arg)
{

	if (work_enable) {
		/* queue main task work */
		queue_work(workqueue, &taskwork);
	}

	/* add timer */
	worktimer.expires = jiffies + EP94M3_WORKTIMER_INT;
	add_timer(&worktimer);
}

/* 100ms */
static void ep94m3_taskwork_func(struct work_struct *data)
{
	// MHL Task
	RCP_Conversion_Task();
}

static int ep94m3_probe(struct platform_device *pdev)
{
	int ret = 0;
	pr_info("%s pdev->name:%s\n", __func__, pdev->name);

	/* get i2c adapter */
	adapter = i2c_get_adapter(0);
	pr_info("%s adapter->name:%s\n", __func__, adapter->name);

	ep94m3_clsp = class_create(THIS_MODULE, "ep94m3");
	ret = class_create_file(ep94m3_clsp, &class_attr_reg_rd);
	ret = class_create_file(ep94m3_clsp, &class_attr_reg_wr);


	/* init EP94M3 */
	Rx_HTPLUG(0, Disable_All);
	RCP_Conversion_Init();


	/* init work timer */
	init_timer(&worktimer);
	worktimer.data = 0;
	worktimer.function = ep94m3_worktimer_func;
	worktimer.expires = jiffies + EP94M3_WORKTIMER_INT;
	add_timer(&worktimer);

	/* init workqueue */
	workqueue = create_singlethread_workqueue("ep94m3");
	INIT_WORK(&taskwork, ep94m3_taskwork_func);

	return 0;
}

static int ep94m3_remove(struct platform_device *pdev)
{
	class_remove_file(ep94m3_clsp, &class_attr_reg_rd);
	class_remove_file(ep94m3_clsp, &class_attr_reg_wr);
	class_destroy(ep94m3_clsp);
	return 0;
}


static struct platform_driver ep94m3_driver = {
	.driver = {
		.name	= "ep94m3",
		.owner	= THIS_MODULE,
	},
	.probe	= ep94m3_probe,
	.remove	= ep94m3_remove,
};


static int __init ep94m3_init(void)
{
	pr_info("%s\n", __func__);
	return platform_driver_register(&ep94m3_driver);
}
module_init(ep94m3_init);

static void __exit ep94m3_exit(void)
{
	platform_driver_unregister(&ep94m3_driver);
}
module_exit(ep94m3_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Amlogic Inc.");


