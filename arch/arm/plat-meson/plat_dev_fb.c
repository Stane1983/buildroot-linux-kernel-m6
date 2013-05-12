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
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <plat/fb.h>
#include <mach/map.h>




struct platform_device meson_device_fb = {
	.name		  = "mesonfb",
	.id		  = 0,
	.num_resources	  = 0,
	.resource	  = NULL,
};
uint32_t   setup_fb_resource(struct resource * res,char res_num)
{
     int  ret=-1;
     
     meson_device_fb.num_resources=res_num;
     meson_device_fb.resource=res;
     return ret;
}
