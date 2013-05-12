/*
 *
 * arch/arm/mach-meson6tv/usbsetting.c
 *
 * make default settings
 *
 *  Copyright (C) 2011,2012 AMLOGIC, INC.
 *
 *	by Victor Wan 2012.11.16 @Shanghai, China
 *	by Victor Wan 2012.02.14 @Santa Clara, CA
 *
 * License terms: GNU General Public License (GPL) version 2
 * Platform machine definition.
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/delay.h>
#include <plat/platform.h>
#include <plat/lm.h>
#include <plat/regops.h>
#include <mach/hardware.h>
#include <mach/memory.h>
#include <mach/clock.h>
#include <mach/am_regs.h>
#include <mach/usbclock.h>

struct lm_device * alloc_usb_lm_device(int port_index)
{
	struct lm_device * ld;

	if(port_index != USB_PORT_IDX_A && 
		 port_index != USB_PORT_IDX_B &&
		 port_index != USB_PORT_IDX_C &&
		 port_index != USB_PORT_IDX_D){
		printk(KERN_ERR "wrong port index (%d) in alloc_usb_im_device\n",port_index);
		return 0;
	}

	ld = kzalloc(sizeof(struct lm_device),GFP_KERNEL);
	if(!ld){
		printk(KERN_ERR "out of memory to alloc usb lm device\n");
		return 0;
	}

	ld->type = LM_DEVICE_TYPE_USB;
	ld->dma_mask_room = DMA_BIT_MASK(32);

	/* Default clock setting */
	ld->clock.sel = USB_PHY_CLK_SEL_XTAL;
	ld->clock.div = 1;
	ld->clock.src = 24000000;
	ld->param.usb.port_idx = port_index;
	ld->param.usb.port_speed = USB_PORT_SPEED_DEFAULT;
	ld->param.usb.dma_config = USB_DMA_BURST_DEFAULT;
	ld->param.usb.port_type = USB_PORT_TYPE_HOST;
	ld->resource.end = SZ_256K;

	if(port_index == USB_PORT_IDX_A){
		ld->id = 0; //lm0
		ld->irq = INT_USB_A;
		ld->resource.start = IO_USB_A_BASE;
		//ld->param.usb.port_type = USB_PORT_TYPE_OTG;
		ld->param.usb.phy_tune_reg = P_USB_ADDR0;
	}else if(port_index == USB_PORT_IDX_B){
		ld->id = 1; //lm1
		ld->irq = INT_USB_B;
		ld->resource.start = IO_USB_B_BASE;
		ld->param.usb.phy_tune_reg = P_USB_ADDR8;
	}else if(port_index == USB_PORT_IDX_C){
		ld->id = 2; //lm2
		ld->irq = INT_USB_C;
		ld->resource.start = IO_USB_C_BASE;
		ld->param.usb.phy_tune_reg = P_USB_ADDR16;
	}else if(port_index == USB_PORT_IDX_D){
		ld->id = 3; //lm3
		ld->irq = INT_USB_D;
		ld->resource.start = IO_USB_C_BASE;
		ld->param.usb.phy_tune_reg = P_USB_ADDR24;
	}else{
		printk(KERN_ERR "error usb port index: %d\n",port_index);
		return 0;
	}

	return ld;
}

EXPORT_SYMBOL(alloc_usb_lm_device);

