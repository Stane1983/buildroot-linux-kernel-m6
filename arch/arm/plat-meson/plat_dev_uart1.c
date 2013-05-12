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
#include <linux/platform_device.h>

#include <mach/irqs.h>
#include <mach/map.h>
#include <linux/uart-aml.h>

/*add your common resource here*/
static struct resource meson_uart_resource[] = {
    [0] = {/*a*/
        .start =    0,
        .end   =    0,
        .flags =    IORESOURCE_IO,
    },
};


struct platform_device meson_uart_device = {
    .name         = "am_uart",  
    .id       = -1, 
    .num_resources    = ARRAY_SIZE(meson_uart_resource),  
    .resource     = meson_uart_resource,   
    .dev = {        
                .platform_data = NULL,
           },
};
/*create more than one uart device node. this is the second uart platform device style*/ 
void __init meson_uart_add_device(void *pd,int pd_size,int dev_index)
{
	void *npd=NULL;
       platform_device  *cur_dev=NULL;
	if (pd && pd_size !=0)
	{
        	npd = kmemdup(pd, pd_size , GFP_KERNEL);
	        if (!npd)
		printk(KERN_ERR "%s: no memory for new platform data [%s]\n", __func__);
	}
       cur_dev=  kmemdup(meson_uart_device, sizeof(struct platform_device) , GFP_KERNEL);
       if(!cur_dev)
       {
            printk(KERN_ERR "%s: no memory for new platform device [%s]\n", __func__);
            return ;
       }
       cur_dev->id=dev_index;
       cur_dev->dev.platform_data=npd;
       platform_add_devices(cur_dev,1);
}