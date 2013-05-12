/*
 * arch/arm/mach-meson2/include/mach/pinmux.h
 *
 * Copyright (C) 2010 Amlogic Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#ifndef __MACH_MESON2_PINMUX_H
#define __MACH_MESON2_PINMUX_H

#include <linux/types.h>

#define PINMUX_REG(n)	n
#define PINMUX_END_VOID 0xFFFFFFFF
#define PINMUX_END_ITEM {.reg = PINMUX_END_VOID}


typedef struct pinmux_item {
    uint32_t reg;
    uint32_t clrmask;
    uint32_t setmask;
} pinmux_item_t;

typedef struct pinmux_set {
    bool (* chip_select)(bool);
    pinmux_item_t * pinmux;
} pinmux_set_t;

/*
 * @param str formate is "pad=sig pad=sig "
 *
 * @return NULL is fail
 * 		errno NOTAVAILABLE ,
 * 			  SOMEPIN IS LOCKED
 */
pinmux_set_t* pinmux_cacl(char * str);


/*
 * pinmux set function
 *
 * @return 0, success ,
 * 		   SOMEPIN IS LOCKED, some pin is locked to the specail feature.
 *                            you can not change it
 * 		   NOTAVAILABLE, not available .
 */
int32_t pinmux_set(pinmux_set_t *);
int32_t pinmux_clr(pinmux_set_t *);

#endif /* __MACH_MESON2_PINMUX_H */

