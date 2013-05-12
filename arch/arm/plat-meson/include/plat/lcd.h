/*
 *
 * arch/arm/plat-meson/include/mach/lcd.h
 *
 *  Copyright (C) 2010 AMLOGIC, INC.
 *
 * License terms: GNU General Public License (GPL) version 2
 * Basic platform init and mapping functions.
 */
#ifndef _PLAT_DEV_LCD_H
 #define _PLAT_DEV_LCD_H

#define INVALID_DEV_INDEX     (-1)


extern struct platform_device  meson_device_lcd;
extern void __init meson_lcd_set_platdata(void *pd,int pd_size);
#endif