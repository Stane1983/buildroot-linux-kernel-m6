/*
 *
 * arch/arm/plat-meson/include/plat/uart.h
 *
 *  Copyright (C) 2010 AMLOGIC, INC.
 *
 * License terms: GNU General Public License (GPL) version 2
 * Basic platform init and mapping functions.
 */
#ifndef  _PLAT_UART_DEV_H
#define _PLAT_UART_DEV_H

extern struct platform_device meson_device_uart;
extern void __init meson_uart_set_platdata(void *pd,int pd_size);
#endif 