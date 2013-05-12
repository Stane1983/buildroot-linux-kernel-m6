/*
 * Author: AMLOGIC, Inc.
 * Copyright (C) 2010 Amlogic Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _REMOTE_PLAT_
#define _REMOTE_PLAT_
#include <mach/pinmux.h>

struct aml_remote_platdata {
	pinmux_item_t *pinmux_items;
	unsigned int ao_baseaddr;
	void (*pinmux_setup)(void);
	void (*pinmux_cleanup)(void);
};

extern struct platform_device meson_device_remote;
extern void meson_remote_set_platdata(struct aml_remote_platdata *pd);
#endif /* _REMOTE_PLAT_ */

