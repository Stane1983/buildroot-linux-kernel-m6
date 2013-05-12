/*
 *
 * arch/arm/mach-meson3/usbclock.c
 *
 *  Copyright (C) 2011 AMLOGIC, INC.
 *	by Victor Wan 2011.2.14 @Santa Clara, CA
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
#include <mach/reg_addr.h>
#include <mach/usbclock.h>

/*
 * M3 chip USB clock setting
 */
 
/*
 * Clock source name index must sync with chip's spec
 * M1/M2/M3/M6 are different!
 */ 
static const char * clock_src_name[] = {
    "XTAL input",
    "XTAL input divided by 2",
    "SYS PLL",
    "MISC PLL"
    "DDR PLL",
    "AUD PLL",
    "VID PLL",
    "VID PLL / 2"
};

static const uint32_t p_usb_reg = P_USB_ADDR0;
static int usb_ctl_init_table[2] = {0,0};

/* test table is empty */
#define no_clock_inited	(!(usb_ctl_init_table[0] || usb_ctl_init_table[1]))

int set_usb_phy_clk(struct lm_device * plmdev,int is_enable)
{

	int port_idx;
	usb_peri_reg_t config;
	int clk_sel,clk_div,clk_src;
	int delay = 200; //usec
	
	if(!plmdev)
		return -1;

	port_idx = plmdev->param.usb.port_idx;
	if(port_idx < 0 || port_idx > 1)
		return -1;

	if(set_usb_phy_id_mode(plmdev,plmdev->param.usb.phy_id_mode) < 0){
		printk(KERN_ERR"set_usb_phy_id_mode error!\n");
		return -1;
	}
	
	clk_sel = plmdev->clock.sel;
	clk_div = plmdev->clock.div;
	clk_src = plmdev->clock.src;
	config.d32 = aml_read_reg32(p_usb_reg);
		
	if(is_enable){
		if(usb_ctl_init_table[port_idx]){
			printk(KERN_ERR"USB %c clock inited already!\n",port_idx?'B':'A');
			return -1;
		}
		
		if(no_clock_inited){
			/* set globle clk sel/div/en */
			printk(KERN_NOTICE"USB use clock source: %s\n",clock_src_name[clk_sel]);
			config.b.clk_sel = clk_sel;
			config.b.clk_div = clk_div;
			config.b.clk_en = 1;
			aml_write_reg32(p_usb_reg,config.d32);
		}
		
		if(port_idx == USB_PORT_IDX_A){
			config.d32 = aml_read_reg32(p_usb_reg);
			/* ahb reset */
			config.b.a_reset = 1;
			udelay(delay);
			aml_write_reg32(p_usb_reg,config.d32);
			udelay(delay);
			config.b.a_reset = 0;
			udelay(delay);
			aml_write_reg32(p_usb_reg,config.d32);			
			
			/* pll reset */
			config.b.a_pll_dect_rst = 1;
			udelay(delay);
			aml_write_reg32(p_usb_reg,config.d32);
			udelay(delay);
			config.b.a_pll_dect_rst = 0;
			udelay(delay);
			aml_write_reg32(p_usb_reg,config.d32);	
			
			/* phy reset */
			config.b.a_prst_n = 1;
			udelay(delay);
			aml_write_reg32(p_usb_reg,config.d32);
			udelay(delay);
			config.b.a_prst_n = 0;
			udelay(delay);
			aml_write_reg32(p_usb_reg,config.d32);	
			
			/* por */
			config.b.a_por = 1;
			udelay(delay);
			aml_write_reg32(p_usb_reg,config.d32);
			udelay(delay);
			config.b.a_por = 0;
			udelay(delay);
			aml_write_reg32(p_usb_reg,config.d32);	
					
		}else{/* port_idx == USB_PORT_IDX_B */
			config.d32 = aml_read_reg32(p_usb_reg);
			/* ahb reset */
			config.b.b_reset = 1;
			udelay(delay);
			aml_write_reg32(p_usb_reg,config.d32);
			udelay(delay);
			config.b.b_reset = 0;
			udelay(delay);
			aml_write_reg32(p_usb_reg,config.d32);			
			
			/* pll reset */
			config.b.b_pll_dect_rst = 1;
			udelay(delay);
			aml_write_reg32(p_usb_reg,config.d32);
			udelay(delay);
			config.b.b_pll_dect_rst = 0;
			udelay(delay);
			aml_write_reg32(p_usb_reg,config.d32);	
			
			/* phy reset */
			config.b.b_prst_n = 1;
			udelay(delay);
			aml_write_reg32(p_usb_reg,config.d32);
			udelay(delay);
			config.b.b_prst_n = 0;
			udelay(delay);
			aml_write_reg32(p_usb_reg,config.d32);	
			
			/* por */
			config.b.b_por = 1;
			udelay(delay);
			aml_write_reg32(p_usb_reg,config.d32);
			udelay(delay);
			config.b.b_por = 0;
			udelay(delay);
			aml_write_reg32(p_usb_reg,config.d32);				
		}
		
		usb_ctl_init_table[port_idx] = 1;	
		
	}else{/* clock disable */
		if(!usb_ctl_init_table[port_idx]){
			printk(KERN_NOTICE"USB %c clock not inited\n",port_idx?'A':'B');
			return -1;
		}
		
		config.d32 = aml_read_reg32(p_usb_reg);
		if(port_idx == USB_PORT_IDX_A){
			config.b.a_por = 1;
			aml_write_reg32(p_usb_reg,config.d32);
		}else{
			config.b.b_por = 1;
			aml_write_reg32(p_usb_reg,config.d32);			
		}		
		usb_ctl_init_table[port_idx] = 0;

		if(no_clock_inited){
			/* clear globle clk sel/div/en */
			config.d32 = aml_read_reg32(p_usb_reg);
			printk(KERN_NOTICE"USB remove clock source: %s\n",clock_src_name[config.b.clk_sel]);
			config.b.clk_en = 0;
			aml_write_reg32(p_usb_reg,config.d32);
		}	
	}

	return 0;
}

EXPORT_SYMBOL(set_usb_phy_clk);

int set_usb_phy_id_mode(struct lm_device * plmdev,unsigned int phy_id_mode)
{
	int port_idx;
	usb_peri_misc_reg_t misc;
	uint32_t usb_misc_reg;
	
	if(!plmdev)
		return -1;

	port_idx = plmdev->param.usb.port_idx;
	if(port_idx < 0 || port_idx > 1)
		return -1;

	if(port_idx == USB_PORT_IDX_A){
		usb_misc_reg = P_USB_ADDR3;	/* PHY A*/
	}else{
		usb_misc_reg = P_USB_ADDR4;	/* PHY B*/		
	}

	misc.d32 = aml_read_reg32(usb_misc_reg);
	
	switch(phy_id_mode){
	case USB_PHY_ID_MODE_SW_HOST:
		misc.b.utmi0_iddig_overrid  = 1;
		misc.b.utmi0_iddig_val = 0;
		break;
	case USB_PHY_ID_MODE_SW_SLAVE:
		misc.b.utmi0_iddig_overrid  = 1;
		misc.b.utmi0_iddig_val = 1;
		break;
	case USB_PHY_ID_MODE_HW:
	default:
		misc.b.utmi0_iddig_overrid  = 0;
		break;
	}

	aml_write_reg32(usb_misc_reg, misc.d32);

	return 0;
}
EXPORT_SYMBOL(set_usb_phy_id_mode);