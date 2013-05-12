/*
 * meson_cs_dcdc_regulator.c
 *
 * Support for Meson current source DCDC voltage regulator
 *
 * Copyright (C) 2012 Elvis Yu <elvis.yu@amlogic.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any kind,
 * whether express or implied; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <plat/io.h>
#include <linux/regulator/meson_cs_dcdc_regulator.h>

static struct meson_cs_pdata_t* g_vcck_voltage = NULL;

static int set_voltage(int from, int to)
{
	if(to<0 || to>MESON_CS_MAX_STEPS)
	{
		printk(KERN_ERR "%s: to(%d) out of range!\n", __FUNCTION__, to);
		return -EINVAL;
	}
	if(from<0 || from>MESON_CS_MAX_STEPS)
	{
		if(g_vcck_voltage && (g_vcck_voltage->set_voltage))
			g_vcck_voltage->set_voltage(to);
		else
			aml_set_reg32_bits(P_VGHL_PWM_REG0, to, 0, 4);
		udelay(200);

	}
	else if(to < from)
	{
		// going to higher voltage
		// lower index is higher voltage.
		if (from - to > 3) {
			if(g_vcck_voltage && (g_vcck_voltage->set_voltage))
				g_vcck_voltage->set_voltage(to+3);
			else
				aml_set_reg32_bits(P_VGHL_PWM_REG0, to + 3, 0, 4);
			udelay(100);
		}
		if (from - to > 1) {
			if(g_vcck_voltage && (g_vcck_voltage->set_voltage))
				g_vcck_voltage->set_voltage(to+1);
			else
				aml_set_reg32_bits(P_VGHL_PWM_REG0, to + 1, 0, 4);
			udelay(100);
		}
		if(g_vcck_voltage && (g_vcck_voltage->set_voltage))
			g_vcck_voltage->set_voltage(to);
		else
			aml_set_reg32_bits(P_VGHL_PWM_REG0, to, 0, 4);
		udelay(100);
	}
	else if(to > from)
	{
		// going to lower voltage
		if (to - from > 3) {
			if(g_vcck_voltage && (g_vcck_voltage->set_voltage))
				g_vcck_voltage->set_voltage(to-3);
			else
				aml_set_reg32_bits(P_VGHL_PWM_REG0, to - 3, 0, 4);
			udelay(100);
		}
		if (to - from > 1) {
			if(g_vcck_voltage && (g_vcck_voltage->set_voltage))
				g_vcck_voltage->set_voltage(to-1);
			else
				aml_set_reg32_bits(P_VGHL_PWM_REG0, to - 1, 0, 4);
			udelay(100);
		}
		if(g_vcck_voltage && (g_vcck_voltage->set_voltage))
			g_vcck_voltage->set_voltage(to);
		else
			aml_set_reg32_bits(P_VGHL_PWM_REG0, to, 0, 4);
		// don't need to delay when going down since freq will
		// already be lowered
	}
	return 0;
}


static int meson_cs_dcdc_get_voltage(struct regulator_dev *dev)
{
	struct meson_cs_regulator_dev *meson_cs_regulator = rdev_get_drvdata(dev);
	u32 reg_val;
	int data;
	mutex_lock(&meson_cs_regulator->io_lock);

	if(g_vcck_voltage && (g_vcck_voltage->get_voltage)) {
		reg_val = g_vcck_voltage->get_voltage();
		if(reg_val < 0) {
			data = -1;
			goto out;
		}
	}
	else {
		reg_val = aml_read_reg32(P_VGHL_PWM_REG0);

		if ((reg_val>>12&3) != 1) {
			dev_err(&dev->dev, "Error getting voltage\n");
			data = -1;
			goto out;
		}

	}
	/* Convert the data from table & step to microvolts */
	data = meson_cs_regulator->voltage_step_table[reg_val & 0xf];

out:
	mutex_unlock(&meson_cs_regulator->io_lock);
	return data;
}

static int meson_cs_dcdc_set_voltage(struct regulator_dev *dev,
				int minuV, int maxuV,
				unsigned *selector)
{
	struct meson_cs_regulator_dev *meson_cs_regulator = rdev_get_drvdata(dev);
	int cur_idx, last_idx;
	
	if (minuV < meson_cs_regulator->voltage_step_table[MESON_CS_MAX_STEPS-1] || minuV > meson_cs_regulator->voltage_step_table[0])
		return -EINVAL;
	if (maxuV < meson_cs_regulator->voltage_step_table[MESON_CS_MAX_STEPS-1] || maxuV > meson_cs_regulator->voltage_step_table[0])
		return -EINVAL;

	for(last_idx=0; last_idx<MESON_CS_MAX_STEPS; last_idx++)
	{
		if(meson_cs_regulator->cur_uV >= meson_cs_regulator->voltage_step_table[last_idx])
		{
			break;
		}
	}

	for(cur_idx=0; cur_idx<MESON_CS_MAX_STEPS; cur_idx++)
	{
		if(minuV >= meson_cs_regulator->voltage_step_table[cur_idx])
		{
			break;
		}
	}

	*selector = cur_idx;
	
	if(meson_cs_regulator->voltage_step_table[cur_idx] != minuV)
	{
		printk("set voltage to %d; selector=%d\n", meson_cs_regulator->voltage_step_table[cur_idx], cur_idx);
	}
	mutex_lock(&meson_cs_regulator->io_lock);

	set_voltage(last_idx, cur_idx);

	meson_cs_regulator->cur_uV = meson_cs_regulator->voltage_step_table[cur_idx];
	mutex_unlock(&meson_cs_regulator->io_lock);
	return 0;
}

static int meson_cs_dcdc_list_voltage(struct regulator_dev *dev, unsigned selector)
{
	struct meson_cs_regulator_dev *meson_cs_regulator = rdev_get_drvdata(dev);
	return meson_cs_regulator->voltage_step_table[selector];
}

static struct regulator_ops meson_cs_ops = {
	.get_voltage	= meson_cs_dcdc_get_voltage,
	.set_voltage	= meson_cs_dcdc_set_voltage,
	.list_voltage	= meson_cs_dcdc_list_voltage,
};






static void update_voltage_constraints(struct meson_cs_regulator_dev *data)
{
	int ret;

	if (data->min_uV && data->max_uV
	    && data->min_uV <= data->max_uV) {
		ret = regulator_set_voltage(data->regulator,
					    data->min_uV, data->max_uV);
		if (ret != 0) {
			printk(KERN_ERR "regulator_set_voltage() failed: %d\n",
			       ret);
			return;
		}
	}
}

static ssize_t show_min_uV(struct device *dev,
			   struct device_attribute *attr, char *buf)
{
	struct meson_cs_regulator_dev *data = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", data->min_uV);
}

static ssize_t set_min_uV(struct device *dev, struct device_attribute *attr,
			  const char *buf, size_t count)
{
	struct meson_cs_regulator_dev *data = dev_get_drvdata(dev);
	long val;

	if (strict_strtol(buf, 10, &val) != 0)
		return count;

	data->min_uV = val;
	update_voltage_constraints(data);

	return count;
}

static ssize_t show_max_uV(struct device *dev,
			   struct device_attribute *attr, char *buf)
{
	struct meson_cs_regulator_dev *data = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", data->max_uV);
}

static ssize_t set_max_uV(struct device *dev, struct device_attribute *attr,
			  const char *buf, size_t count)
{
	struct meson_cs_regulator_dev *data = dev_get_drvdata(dev);
	long val;

	if (strict_strtol(buf, 10, &val) != 0)
		return count;

	data->max_uV = val;
	update_voltage_constraints(data);

	return count;
}

static DEVICE_ATTR(min_microvolts, 0644, show_min_uV, set_min_uV);
static DEVICE_ATTR(max_microvolts, 0644, show_max_uV, set_max_uV);


static struct device_attribute *attributes_virtual[] = {
	&dev_attr_min_microvolts,
	&dev_attr_max_microvolts,
};



static __devinit
int meson_cs_probe(struct platform_device *pdev)
{
	//struct meson_cs_pdata_t *meson_cs_pdata = platform_get_drvdata(pdev);
	struct meson_cs_pdata_t *meson_cs_pdata  = pdev->dev.platform_data;
	struct meson_cs_regulator_dev *meson_cs_regulator;
	int error = 0, i, cur_idx;

       g_vcck_voltage = meson_cs_pdata;
	meson_cs_regulator = kzalloc(sizeof(struct meson_cs_regulator_dev), GFP_KERNEL);
	if (!meson_cs_regulator)
		return -ENOMEM;

	meson_cs_regulator->rdev = NULL;
	meson_cs_regulator->regulator = NULL;
	meson_cs_regulator->voltage_step_table = meson_cs_pdata->voltage_step_table;

	meson_cs_regulator->desc.name = "meson_cs_desc";
	meson_cs_regulator->desc.id = 0;
	meson_cs_regulator->desc.n_voltages = MESON_CS_MAX_STEPS;
	meson_cs_regulator->desc.ops = &meson_cs_ops;
	meson_cs_regulator->desc.type = REGULATOR_VOLTAGE;
	meson_cs_regulator->desc.owner = THIS_MODULE;

	mutex_init(&meson_cs_regulator->io_lock);

	aml_set_reg32_bits(P_VGHL_PWM_REG0, 1, 12, 2);		//Enable
	
	meson_cs_regulator->rdev = regulator_register(&meson_cs_regulator->desc, &pdev->dev,
				  meson_cs_pdata->meson_cs_init_data, meson_cs_regulator);


	if (IS_ERR(meson_cs_regulator->rdev)) {
			dev_err(&pdev->dev,
				"failed to register %s regulator\n",
				pdev->name);
			error = PTR_ERR(meson_cs_regulator->rdev);
			goto fail;
	}

	meson_cs_regulator->regulator = regulator_get(NULL,
		meson_cs_pdata->meson_cs_init_data->consumer_supplies->supply);

	if (IS_ERR(meson_cs_regulator->regulator)) {
			dev_err(&pdev->dev,
				"failed to get %s regulator\n",
				pdev->name);
			error = PTR_ERR(meson_cs_regulator->rdev);
			goto fail;
	}

	for (i = 0; i < ARRAY_SIZE(attributes_virtual); i++) {
		error = device_create_file(&pdev->dev, attributes_virtual[i]);
		if (error != 0)
			goto fail;
	}

	platform_set_drvdata(pdev, meson_cs_regulator);		//Elvis

	for(cur_idx=0; cur_idx<MESON_CS_MAX_STEPS; cur_idx++)
	{
		if(meson_cs_pdata->default_uV >= meson_cs_regulator->voltage_step_table[cur_idx])
		{
			break;
		}
	}

	if(set_voltage(0, cur_idx))
	{
		goto fail;
	}

	meson_cs_regulator->cur_uV = meson_cs_regulator->voltage_step_table[cur_idx];

	
	return 0;

fail:

	if(meson_cs_regulator->rdev)
		regulator_unregister(meson_cs_regulator->rdev);
	
	if(meson_cs_regulator->regulator)
		regulator_put(meson_cs_regulator->regulator);
	
	kfree(meson_cs_regulator);
	return error;
}

static int __devexit meson_cs_remove(struct platform_device *pdev)
{
	struct meson_cs_regulator_dev *meson_cs_regulator = platform_get_drvdata(pdev);
	if(meson_cs_regulator->rdev)
		regulator_unregister(meson_cs_regulator->rdev);
	
	if(meson_cs_regulator->regulator)
		regulator_put(meson_cs_regulator->regulator);
	
	kfree(meson_cs_regulator);

	return 0;
}



static struct platform_driver meson_cs_driver = {
	.driver = {
		.name = "meson-cs-regulator",
		.owner = THIS_MODULE,
	},
	.probe = meson_cs_probe,
	.remove = __devexit_p(meson_cs_remove),
};


static int __init meson_cs_init(void)
{
	return platform_driver_register(&meson_cs_driver);
}

static void __exit meson_cs_cleanup(void)
{
	platform_driver_unregister(&meson_cs_driver);
}

subsys_initcall(meson_cs_init);
module_exit(meson_cs_cleanup);

MODULE_AUTHOR("Elvis Yu <elvis.yu@amlogic.com>");
MODULE_DESCRIPTION("Amlogic Meson current source voltage regulator driver");
MODULE_LICENSE("GPL v2");
