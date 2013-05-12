/*
 *
 * arch/arm/mach-meson6tv/usbclock.c
 *
 *  Copyright (C) 2011,2012 AMLOGIC, INC.
 *
 *	by Victor Wan 2012.11.16 @Shanghai, China
 *	by Victor Wan 2012.2.14 @Santa Clara, CA
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

/*
 * M chip USB clock setting
 */
 
/*
 * Clock source name index must sync with chip's spec
 * M1/M2/M3/M6/M6TV are different!
 * This is only for M6TV
 */
static const char * clock_src_name[] = {
    "XTAL input",
    "XTAL input divided by 2",
    "FCLK / 2",
    "FCLK / 5"
};

int set_usb_phy_clk(struct lm_device * plmdev,int is_enable)
{
	int port_idx;
	usb_peri_reg_t * peri;
	usb_config_data_t config;
	usb_ctrl_data_t control;
	int clk_sel,clk_div,clk_src;
	int time_dly = 500; //usec
	
	if(!plmdev)
		return -1;

	port_idx = plmdev->param.usb.port_idx;
	if(port_idx < 0 || port_idx > 3)
		return -1;

	peri = (usb_peri_reg_t *)plmdev->param.usb.phy_tune_reg;
	
	printk(KERN_NOTICE"USB (%d) peri reg base: %x\n",port_idx,(uint32_t)peri);
	if(is_enable){

		clk_sel = plmdev->clock.sel;
		clk_div = plmdev->clock.div;
		clk_src = plmdev->clock.src;

		config.d32 = peri->config;
		config.b.clk_sel = clk_sel;	
		config.b.clk_div = clk_div; 
	  config.b.clk_en = 1;
		peri->config = config.d32;

		printk(KERN_NOTICE"USB (%d) use clock source: %s\n",port_idx,clock_src_name[clk_sel]);
		
		control.d32 = peri->ctrl;
		control.b.fsel = 2;	/* PHY default is 24M (5), change to 12M (2) */
		control.b.por = 1;
		peri->ctrl = control.d32;
		udelay(time_dly);
		control.b.por = 0;
		peri->ctrl = control.d32;
		udelay(time_dly);

		/* read back clock detected flag*/
		control.d32 = peri->ctrl;
		if(!control.b.clk_detected){
			printk(KERN_ERR"USB (%d) PHY Clock not detected!\n",port_idx);
			return -1;
		}
	}else{

		config.d32 = peri->config;
		config.b.clk_en = 0;
		peri->config = config.d32;
		control.d32 = peri->ctrl;
		control.b.por = 1;
		peri->ctrl = control.d32;
	}
	dmb();
	
	return 0;
}

EXPORT_SYMBOL(set_usb_phy_clk);

