/*
 * arch/arm/mach-meson2/include/mach/gpio.h
 *
 * Copyright (C) 2010 Amlogic Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#ifndef __MACH_MESON2_GPIO_H
#define __MACH_MESON2_GPIO_H

#include <linux/types.h>
#include <mach/pinmux.h>


int32_t gpio_set_status(uint32_t pin,bool gpio_in);
bool gpio_get_status(uint32_t pin);

int32_t gpio_get_val(uint32_t pin);

int32_t gpio_out(uint32_t pin,bool high);

int32_t gpio_in_get(uint32_t pin);

#endif /* __MACH_MESON2_GPIO_H */

