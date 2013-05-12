/*
 * Silicon Image SiI1292 Device Driver
 *
 * Author: Amlogic, Inc.
 *
 * Copyright (C) 2012 Amlogic Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#include <linux/i2c.h>

#include "../hal/si_i2c.h"



int sii_i2c_read_reg(uint8_t address, uint8_t reg)
{

	uint8_t val;

	struct i2c_msg msgs[] = {
		{
			.addr = address,
			.flags = 0,
			.len = 1,
			.buf = &reg,
		},
		{
			.addr = address,
			.flags = I2C_M_RD,
			.len = 1,
			.buf = &val,
		}
	};

	if (i2c_transfer(adapter, msgs, 2) != 2) {
		pr_err("%s i2c transfer failed\n", __func__);
		return -EIO;
	}

	pr_info("%s [0x%02x]0x%02x = 0x%02x\n", __func__, address, reg, val);
	return val;
}


void sii_i2c_write_reg(uint8_t address, uint8_t reg, uint8_t val)
{
	uint8_t buf[2];

	struct i2c_msg msgs[] = {
		{
			.addr = address,
			.flags = 0,
			.len = 2,
			.buf = buf,
		}
	};

	buf[0] = reg;
	buf[1] = val;

	if (i2c_transfer(adapter, msgs, 1) != 1) {
		pr_err("%s i2c transfer failed\n", __func__);
	}

	pr_info("%s [0x%02x]0x%02x = 0x%02x\n", __func__, address, reg, val);
}


uint8_t HalI2cReadByte( uint8_t device_id, uint8_t reg )
{
	uint8_t val = 0;
	struct i2c_msg msgs[2];

#ifdef CONFIG_MHL_SII1292_CI2CA_PULLUP
	device_id += 0x4;
#endif

	msgs[0].addr = device_id >> 1;
	msgs[0].flags = 0;
	msgs[0].len = 1;
	msgs[0].buf = &reg;

	msgs[1].addr = device_id >> 1;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = 1;
	msgs[1].buf = &val;

	if (i2c_transfer(adapter, msgs, 2) != 2) {
		pr_err("%s i2c transfer failed\n", __func__);
		return -EIO;
	}

	return val;
}

void HalI2cWriteByte( uint8_t device_id, uint8_t offset, uint8_t value )
{
	struct i2c_msg msgs[1];
	uint8_t buf[2];

#ifdef CONFIG_MHL_SII1292_CI2CA_PULLUP
	device_id += 0x4;
#endif

	buf[0] = offset;
	buf[1] = value;

	msgs[0].addr = device_id >> 1;
	msgs[0].flags = 0;
	msgs[0].len = 2;
	msgs[0].buf = buf;

	if (i2c_transfer(adapter, msgs, 1) != 1) {
		pr_err("%s i2c transfer failed\n", __func__);
	}
}

bool_t HalI2cReadBlock( uint8_t device_id, uint8_t addr, uint8_t *p_data, uint16_t nbytes )
{
	return true;
}

bool_t HalI2cWriteBlock( uint8_t device_id, uint8_t addr, uint8_t *p_data, uint16_t nbytes )
{
	return true;
}

