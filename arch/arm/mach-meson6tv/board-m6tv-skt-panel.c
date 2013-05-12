/*
 * arch/arm/mach-meson6tv/board-m6tv-skt-panel.c
 *
 * Copyright (C) 2012 Amlogic, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <plat/platform.h>
#include <plat/plat_dev.h>
#include <plat/lm.h>
#include <mach/clock.h>
#include <mach/map.h>
#include <mach/gpio.h>
#include <mach/gpio_data.h>
#include <linux/delay.h>
#include <plat/regops.h>
//#include <mach/reg_addr.h>

#include <linux/panel/lcd.h>
#include <linux/aml_bl.h>
#include <mach/lcd_aml.h>

#include "board-m6tv-skt.h"

#ifdef CONFIG_AW_AXP
extern int axp_gpio_set_io(int gpio, int io_state);
extern int axp_gpio_get_io(int gpio, int *io_state);
extern int axp_gpio_set_value(int gpio, int value);
extern int axp_gpio_get_value(int gpio, int *value);
#endif

#define H_ACTIVE		1920
#define V_ACTIVE		1080
#define H_PERIOD		2200
#define V_PERIOD		1125
#define VIDEO_ON_PIXEL		148
#define VIDEO_ON_LINE		41

static void lcd_sync_duration(Lcd_Config_t *pConf)
{
	unsigned m, n, od, div, xd;
	unsigned pre_div;
	unsigned sync_duration;

	m = ((pConf->lcd_timing.pll_ctrl) >> 0) & 0x1ff;
	n = ((pConf->lcd_timing.pll_ctrl) >> 9) & 0x1f;
	od = ((pConf->lcd_timing.pll_ctrl) >> 16) & 0x3;
	div = ((pConf->lcd_timing.div_ctrl) >> 4) & 0x7;
	xd = ((pConf->lcd_timing.clk_ctrl) >> 0) & 0xf;

	od = (od == 0) ? 1:((od == 1) ? 2:4);
	switch(pConf->lcd_basic.lcd_type)
	{
		case LCD_DIGITAL_TTL:
			pre_div = 1;
			break;
		case LCD_DIGITAL_LVDS:
			pre_div = 7;
			break;
		default:
			pre_div = 1;
			break;
	}

	sync_duration = m*24*100/(n*od*(div+1)*xd*pre_div);
	sync_duration = ((sync_duration * 100000 / H_PERIOD) * 10) / V_PERIOD;
	sync_duration = (sync_duration + 5) / 10;

	pConf->lcd_timing.sync_duration_num = 60;
	pConf->lcd_timing.sync_duration_den = 1;
}

// Define LVDS physical PREM SWING VCM skt
static Lvds_Phy_Control_t lcd_lvds_phy_control = {
	.lvds_prem_ctl	= 0xf,
	.lvds_swing_ctl	= 0x3,
	.lvds_vcm_ctl	= 0x2,
	.lvds_ref_ctl	= 0xf,
};

//Define LVDS data mapping, pn swap.
static Lvds_Config_t lcd_lvds_config = {
	.lvds_repack	= 1,   //data mapping  //0:JEDIA mode, 1:VESA mode
	.pn_swap	= 0,       //0:normal, 1:swap
	.dual_port	= 1,
	.port_reverse	= 1,
};

static Lcd_Config_t m6tvskt_lcd_config = {
	// skter to LCD Spec
	.lcd_basic = {
		.h_active = H_ACTIVE,
		.v_active = V_ACTIVE,
		.h_period = H_PERIOD,
		.v_period = V_PERIOD,
		.screen_ratio_width   = 16,
		.screen_ratio_height  = 9,
		.screen_actual_width  = 127, //this is value for 160 dpi please set real value according to spec.
		.screen_actual_height = 203, //
		.lcd_type = LCD_DIGITAL_LVDS,   //LCD_DIGITAL_TTL  //LCD_DIGITAL_LVDS  //LCD_DIGITAL_MINILVDS
		.lcd_bits = 8,  //8  //6
	},

	.lcd_timing = {
		.pll_ctrl = 0x40050c82,//0x400514d0, //
		.div_ctrl = 0x00010803,
		.clk_ctrl = 0x1111,  //[19:16]ss_ctrl, [12]pll_sel, [8]div_sel, [4]vclk_sel, [3:0]xd
		//.sync_duration_num = 501,
		//.sync_duration_den = 10,

		.video_on_pixel = VIDEO_ON_PIXEL,
		.video_on_line  = VIDEO_ON_LINE,

		.sth1_hs_addr = 44,
		.sth1_he_addr = 2156,
		.sth1_vs_addr = 0,
		.sth1_ve_addr = V_PERIOD - 1,
		.stv1_hs_addr = 2100,
		.stv1_he_addr = 2164,
		.stv1_vs_addr = 3,
		.stv1_ve_addr = 5,

		.pol_cntl_addr = (0x0 << LCD_CPH1_POL) |(0x1 << LCD_HS_POL) | (0x1 << LCD_VS_POL),
		.inv_cnt_addr = (0<<LCD_INV_EN) | (0<<LCD_INV_CNT),
		.tcon_misc_sel_addr = (1<<LCD_STV1_SEL) | (1<<LCD_STV2_SEL),
		.dual_port_cntl_addr = (1<<LCD_TTL_SEL) | (1<<LCD_ANALOG_SEL_CPH3) | (1<<LCD_ANALOG_3PHI_CLK_SEL) | (0<<LCD_RGB_SWP) | (0<<LCD_BIT_SWP),
	},

	.lvds_mlvds_config = {
		.lvds_config = &lcd_lvds_config,
		.lvds_phy_control = &lcd_lvds_phy_control,
	},
};

static struct aml_lcd_platform m6tvskt_lcd_data = {
	.lcd_conf = &m6tvskt_lcd_config,
};

static struct platform_device m6tvskt_lcd_device = {
	.name = "mesonlcd",
	.id = 0,
	.num_resources = 0,
	.resource      = NULL,
	.dev = {
		.platform_data = &m6tvskt_lcd_data,
	},
};

#define BL_CTL_GPIO	0
#define BL_CTL_PWM	1
#define BL_CTL		BL_CTL_GPIO

#define PWM_MAX		60000 //PWM_MAX <= 65535
#define	PWM_PRE_DIV	0 //pwm_freq = 24M / (pre_div + 1) / PWM_MAX

#define BL_MAX_LEVEL	255
#define BL_MIN_LEVEL	0

static unsigned int m6tvskt_bl_level = 0;

static unsigned m6tvskt_get_bl_level(void)
{
	return m6tvskt_bl_level;
}

static void m6tvskt_set_bl_level(unsigned int level)
{
	m6tvskt_bl_level = level;

	if (level > BL_MAX_LEVEL)
		level = BL_MAX_LEVEL;

	if (level < BL_MIN_LEVEL)
		level = BL_MIN_LEVEL;

#if (BL_CTL==BL_CTL_GPIO)
	level = level * 15 / BL_MAX_LEVEL;
	level = 15 - level;
	aml_set_reg32_bits(P_LED_PWM_REG0, level, 0, 4);
#elif (BL_CTL==BL_CTL_PWM)
	level = level * PWM_MAX / BL_MAX_LEVEL ;
	aml_set_reg32_bits(P_PWM_PWM_D, (PWM_MAX - level), 0, 16);
	aml_set_reg32_bits(P_PWM_PWM_D, level, 16, 16); //pwm high
#endif
}


static struct aml_bl_platform_data m6tvskt_bl_data = {
	.power_on_bl		= NULL,
	.power_off_bl		= NULL,
	.get_bl_level		= m6tvskt_get_bl_level,
	.set_bl_level		= m6tvskt_set_bl_level,
	.max_brightness		= 255,
	.dft_brightness 	= 200,
};

static struct platform_device m6tvskt_bl_device = {
	.name = "aml-bl",
	.id = -1,
	.num_resources = 0,
	.resource = NULL,
	.dev = {
		.platform_data = &m6tvskt_bl_data,
	},
};

static struct platform_device __initdata * m6tvskt_panel_devices[] = {
	&m6tvskt_lcd_device,
	//&meson_device_vout,
	&m6tvskt_bl_device,
};

int __init m6tvskt_lcd_init(void)
{
	int ret;
	lcd_sync_duration(&m6tvskt_lcd_config);
	ret = platform_add_devices(m6tvskt_panel_devices,
				   ARRAY_SIZE(m6tvskt_panel_devices));
	return ret;
}

