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

 
#include <linux/gfp.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <plat/platform_data.h>
#include <plat/pinmux.h>
#include <linux/uart-aml.h>

struct platform_device  meson_device_uart = {
	.name		= "mesonuart",
	.id		= -1,
	.num_resources	= 0,
	.resource	= NULL,
};

/**
 *  We use pd_size in order to adapt to differnt I2C platform_data size.
 */
void __init meson_uart_set_platdata(void * pd, int pd_size)
{
	void * npd = NULL;
	struct aml_uart_platform * p = NULL;
	pinmux_set_t * x = NULL;

	p = (struct aml_uart_platform *)pd;
	x = (pinmux_set_t *)(p->pinmux_uart[0]);

	//printk(KERN_INFO " A==%p %p %p\n", p->pinmux_uart[0], x ,x->pinmux);

	npd = meson_set_platdata(pd, pd_size, &meson_device_uart);
	if (!npd)
		printk(KERN_ERR "%s() No memory for new platform data\n", __func__);

	// Set default pinmux set&clr.
	p = (struct aml_uart_platform *)npd;
	x = (pinmux_set_t *)(p->pinmux_uart[0]);

	//printk(KERN_INFO " A==%p %p %p\n",p->pinmux_uart[0],x,x->pinmux);

	plat_set_default_pinmux((plat_data_public_t*)npd);
}
