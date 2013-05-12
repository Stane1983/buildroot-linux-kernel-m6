/*
 * EP94M3 3-Port MHL/HDMI Dual Mode Switcher
 *
 * Author: Amlogic, Inc.
 *
 * Copyright (C) 2013 Amlogic Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#include <linux/i2c.h>

#include "EP94M3_I2C.h"

//#define EP94M3_SLAVE_ADDR			0x88
#define EP94M3_SLAVE_ADDR			0x98

extern struct i2c_adapter *adapter;


unsigned char EP94M3_I2C_Reg_Rd(unsigned char RegAddr, unsigned char *Data, unsigned int Size)
{
	struct i2c_msg msgs[] = {
		{
			.addr = EP94M3_SLAVE_ADDR >> 1,
			.flags = 0,
			.len = 1,
			.buf = &RegAddr,
		},
		{
			.addr = EP94M3_SLAVE_ADDR >> 1,
			.flags = I2C_M_RD,
			.len = Size,
			.buf = Data,
		}
	};

	if (i2c_transfer(adapter, msgs, 2) != 2) {
		pr_err("%s i2c transfer failed\n", __func__);
		return -EIO;
	}

	return 0;
}



unsigned char EP94M3_I2C_Reg_Wr(unsigned char RegAddr, unsigned char *Data, unsigned int Size)
{

	unsigned char Buf[Size + 1];
	struct i2c_msg msgs[] = {
		{
			.addr = EP94M3_SLAVE_ADDR >> 1,
			.flags = 0,
			.len = Size + 1,
			.buf = Buf,
		}
	};

	Buf[0] = RegAddr;
	memcpy(&Buf[1], Data, Size);

	if (i2c_transfer(adapter, msgs, 1) != 1) {
		pr_err("%s i2c transfer failed\n", __func__);
		return -EIO;
	}
	return 0;
}

