/*
 * arch/arm/mach-meson2/gpio.c
 *
 * Copyright (C) 2010 Amlogic Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <linux/types.h>

#include <mach/pinmux.h>
#include <mach/gpio.h>


int32_t gpio_out(uint32_t pin,bool high)
{
    /**
     * @todo to be implemented later.
     */
	return -1;
}
EXPORT_SYMBOL(gpio_out);

int32_t gpio_in_get(uint32_t pin)
{
    /**
     * @todo to be implemented later.
     */
    return 0;
}
EXPORT_SYMBOL(gpio_in_get);



