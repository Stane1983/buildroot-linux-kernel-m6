/*
 * (C) Copyright 2011, Amlogic, Inc. http://www.amlogic.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR /PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <linux/platform_device.h>

#include <mach/irqs.h>
#include <mach/map.h>
#include <mach/pinmux.h>
#include <plat/eth.h>

static pinmux_set_t  meson_eth_pinmux = {
	.chip_select = NULL,
	.pinmux = NULL,
};

struct platform_device meson_device_eth = {
	.name   = "meson-eth",
	.id     = -1,
	.dev    = {
		.platform_data = NULL,
	}
};

static void meson_eth_pinmux_setup(void)
{
	printk(KERN_INFO "meson_eth_pinmux_setup()\n");

	pinmux_set(&meson_eth_pinmux);
}

static void meson_eth_pinmux_cleanup(void)
{
	printk(KERN_INFO "meson_eth_pinmux_cleanup()\n");

	pinmux_clr(&meson_eth_pinmux);
}

void __init meson_eth_set_platdata(struct aml_eth_platdata *pd)
{
	struct aml_eth_platdata *npd;

	if (!pd) {
		/* too early to use dev_name(), may not be registered */
		printk(KERN_ERR "%s: no platform data supplied\n", meson_device_eth.name);
		return;
	}

	npd = kmemdup(pd, sizeof(struct aml_eth_platdata), GFP_KERNEL);
	if (!npd) {
		printk(KERN_ERR "%s: cannot clone platform data\n", meson_device_eth.name);
		return;
	}

	if (npd->pinmux_setup == NULL)
		npd->pinmux_setup = meson_eth_pinmux_setup;
	if (npd->pinmux_cleanup == NULL)
		npd->pinmux_cleanup = meson_eth_pinmux_cleanup;

	meson_eth_pinmux.pinmux = npd->pinmux_items;
	meson_device_eth.dev.platform_data = npd;
}
