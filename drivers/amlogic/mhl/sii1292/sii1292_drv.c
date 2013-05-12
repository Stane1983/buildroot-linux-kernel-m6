/*
 * Silicon Image SiI1292 Device Driver
 *
 * Author: Bobby Yang <bo.yang@amlogic.com>
 *
 *
 * Copyright (C) 2012 Amlogic Inc.
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

#include "api/si_api1292.h"
#include "main/si_cp1292.h"
#include "hal/si_i2c.h"
#include "sii1292_drv.h"

#define  SII1292_DRV_NAME	"sii1292"

static struct class *sii_clsp;

struct i2c_adapter *adapter;

#define SII1292_WK_TMR_INT	(HZ/20)		// 50ms
struct timer_list worktimer;

static struct workqueue_struct *workqueue = NULL;
static struct work_struct mainwork;



#define REG_VND_IDL		0X00
#define REG_VND_IDH		0X01
#define REG_DEV_IDL		0X02
#define REG_DEV_IDH		0X03
#define REG_DEV_REV		0X04

static bool work_enable = false;
module_param(work_enable, bool, 0644);

static uint8_t registers_d0[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
	0x09, 0x0a, 0x0c, 0x0c, 0x4b, 0x4c, 0x4d, 0x4e, 0x51,
	0x70, 0x73, 0x7a, 0x84
};

static ssize_t sii_i2c_show(struct class *cls,
			struct class_attribute *attr,
			char *buf)
{
	int i = 0;
	uint8_t slave_address = 0xD0;

#ifdef CONFIG_MHL_SII1292_CI2CA_PULLUP
	slave_address += 0x4;
#endif
	slave_address >>= 1;

	for (i = 0; i < sizeof(registers_d0); i++)
	{
		sii_i2c_read_reg(slave_address, registers_d0[i]);
	}
	return 0;
}


static ssize_t sii_i2c_store(struct class *cls,
			 struct class_attribute *attr,
			 const char *buffer, size_t count)
{
	int n = 0;
	char *buf_orig, *ps, *token;
	char *parm[4];
	unsigned int addr = 0, reg = 0, val = 0;

	buf_orig = kstrdup(buffer, GFP_KERNEL);

	ps = buf_orig;
	while (1) {
		token = strsep(&ps, " \n");
		if (token == NULL)
			break;
		if (*token == '\0')
			continue;
		parm[n++] = token;
	}
	pr_info("%s n:%d\n", __func__, n);

	if ((parm[0][0] == 'r')) {
		if (n != 3) {
			pr_info("read: invalid parameter\n");
			kfree(buf_orig);
			return count;
		}
		addr = simple_strtoul(parm[1], NULL, 16);
		reg  = simple_strtoul(parm[2], NULL, 16);
		val = sii_i2c_read_reg((uint8_t)addr, (uint8_t)reg);
		pr_info("sii_i2c_read_reg   slave 0x%x, 0x%x = 0x%x\n", addr, reg, val);
	}
	else if ((parm[0][0] == 'w')) {
		if (n != 4) {
			pr_info("write: invalid parameter\n");
			kfree(buf_orig);
			return count;
		}
		addr = simple_strtoul(parm[1], NULL, 16);
		reg  = simple_strtoul(parm[2], NULL, 16);
		val  = simple_strtoul(parm[3], NULL, 16);

		sii_i2c_write_reg((uint8_t)addr, (uint8_t)reg, (uint8_t)val);
		pr_info("sii_i2c_write_reg   slave 0x%x, 0x%x = 0x%x\n", addr, reg, val);
	}

	kfree(buf_orig);
	return count;
}

static CLASS_ATTR(i2c, S_IWUSR | S_IRUGO, sii_i2c_show, sii_i2c_store);


static ssize_t reg_store(struct class *cls,
			 struct class_attribute *attr,
			 const char *buffer, size_t count)
{
	int n = 0;
	char *buf_orig, *ps, *token;
	char *parm[4];
	unsigned int addr = 0, reg = 0, val = 0;

	buf_orig = kstrdup(buffer, GFP_KERNEL);

	ps = buf_orig;
	while (1) {
		token = strsep(&ps, " \n");
		if (token == NULL)
			break;
		if (*token == '\0')
			continue;
		parm[n++] = token;
	}
	pr_info("%s n:%d\n", __func__, n);

	if ((parm[0][0] == 'r')) {
		if (n != 3) {
			pr_info("read: invalid parameter\n");
			kfree(buf_orig);
			return count;
		}
		addr = simple_strtoul(parm[1], NULL, 16);
		reg  = simple_strtoul(parm[2], NULL, 16);
		//val = sii_read_reg((uint8_t)addr, (uint8_t)reg);
		//pr_info("sii_read_reg   slave 0x%x, 0x%x = 0x%x\n", addr, reg, val);

		val = HalI2cReadByte( addr, reg);
		pr_info("HalI2cReadByte slave 0x%x, 0x%x = 0x%x\n", addr, reg, val);
	}
	else if ((parm[0][0] == 'w')) {
		if (n != 4) {
			pr_info("write: invalid parameter\n");
			kfree(buf_orig);
			return count;
		}
		addr = simple_strtoul(parm[1], NULL, 16);
		reg  = simple_strtoul(parm[2], NULL, 16);
		val  = simple_strtoul(parm[3], NULL, 16);

		//sii_write_reg((uint8_t)addr, (uint8_t)reg, (uint8_t)val);
		//pr_info("sii_write_reg   slave 0x%x, 0x%x = 0x%x\n", addr, reg, val);


		HalI2cWriteByte( addr, reg, val );
		pr_info("HalI2cWriteByte slave 0x%x, 0x%x = 0x%x\n", addr, reg, val);
	}

	kfree(buf_orig);
	return count;
}

static CLASS_ATTR(reg, S_IWUSR | S_IRUGO, NULL, reg_store);


void work_timer_func(unsigned long arg)
{
	if (work_enable)
		queue_work(workqueue, &mainwork);
	/* add timer */
	worktimer.expires = jiffies + SII1292_WK_TMR_INT;
	add_timer(&worktimer);
}

static void sii1292_mainwork(struct work_struct *data)
{
	/* Check to see if the chip wants service.  */
	CpCbusHandler();

#ifdef CONFIG_MHL_SII1292_CEC
	if ( g_data.cecEnabled )
	{
		/* Process any pending CEC messages received.   */
		SI_CecHandler( g_data.portSelect, false );
	}
#endif

	SI_DeviceEventMonitor();
}

static int sii1292_probe(struct platform_device *pdev)
{
	int ret = 0;
	pr_info("%s pdev->name:%s\n", __func__, pdev->name);

	/* get i2c adapter */
	adapter = i2c_get_adapter(0);
	pr_info("%s adapter->name:%s\n", __func__, adapter->name);

	sii_clsp = class_create(THIS_MODULE, "sii1292");

	ret = class_create_file(sii_clsp, &class_attr_i2c);
	ret = class_create_file(sii_clsp, &class_attr_reg);

	/*
	 * SiI1292 Init
	 */
	BoardInit();
	CpCbusInitialize();

	init_timer(&worktimer);
	worktimer.data = 0;
	worktimer.function = work_timer_func;
	worktimer.expires = jiffies + SII1292_WK_TMR_INT;
	add_timer(&worktimer);

	/* init workqueue */
	workqueue = create_singlethread_workqueue("sii1292");
	INIT_WORK(&mainwork, sii1292_mainwork);


	return 0;
}

static int sii1292_remove(struct platform_device *pdev)
{
	class_remove_file(sii_clsp, &class_attr_i2c);
	class_remove_file(sii_clsp, &class_attr_reg);
	class_destroy(sii_clsp);
	return 0;
}


static struct platform_driver sii1292_driver = {
	.driver = {
		.name	= "sii1292",
		.owner	= THIS_MODULE,
	},
	.probe	= sii1292_probe,
	.remove	= sii1292_remove,
};


static int __init sii1292_init(void)
{
	pr_info("%s\n", __func__);
	return platform_driver_register(&sii1292_driver);
}
module_init(sii1292_init);

static void __exit sii1292_exit(void)
{
	platform_driver_unregister(&sii1292_driver);
}
module_exit(sii1292_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Amlogic Inc.");

