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



#ifndef __EP94M3_I2C_H
#define __EP94M3_I2C_H

#include <linux/i2c.h>

unsigned char EP94M3_I2C_Reg_Rd(unsigned char RegAddr, unsigned char *Data, unsigned int Size);
unsigned char EP94M3_I2C_Reg_Wr(unsigned char RegAddr, unsigned char *Data, unsigned int Size);


#endif /* __EP94M3_I2C_H */


