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
#include <plat/i2c.h>
#include <plat/pinmux.h>

struct platform_device  meson_device_i2c2 = {
	.name		  = "mesoni2c",
	.id		  =2, /*i2c device id*/
	.num_resources	  = 0,
	.resource	  = NULL,
};
/*when we have more than one i2c device,we need add one file like this.*/
void __init meson_i2c2_set_platdata(void *pd,int pd_size)
{
     void *npd=NULL;
     npd = meson_set_platdata(pd,pd_size,&meson_device_i2c2);
     if (!npd)
     printk(KERN_ERR "%s: no memory for new platform data\n", __func__);
     //set default pinmux set&clr.
     plat_set_default_pinmux((plat_data_public_t*)npd);
}

