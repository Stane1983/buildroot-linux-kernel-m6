/*
 *
 * arch/arm/plat-meson/include/mach/i2c.h
 *
 *  Copyright (C) 2010 AMLOGIC, INC.
 *
 * License terms: GNU General Public License (GPL) version 2
 * Basic platform init and mapping functions.
 */
#ifndef _PLAT_DEV_I2C_H
 #define _PLAT_DEV_I2C_H

#define INVALID_DEV_INDEX     (-1)


extern struct platform_device  meson_device_i2c1;
extern struct platform_device  meson_device_i2c2;
extern void __init meson_i2c1_set_platdata(void *pd,int pd_size);
extern void __init meson_i2c2_set_platdata(void *pd,int pd_size);
#endif