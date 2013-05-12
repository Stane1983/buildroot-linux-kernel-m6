/*
 * arch/arm/mach-meson6tv/board-m6tv-skt-tvin.c
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

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>

#include <mach/pinmux.h>
#include <mach/irqs.h>
#include <linux/tvin/tvin.h>

#include "board-m6tv-skt.h"


static struct resource vdin0_resources[] = {
	[0] = {
		.start = VDIN0_ADDR_START,
		.end   = VDIN0_ADDR_END,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = INT_VDIN0_VSYNC,
		.end   = INT_VDIN0_VSYNC,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device m6tvskt_vdin0_device = {
	.name		= "vdin",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(vdin0_resources),
	.resource	= vdin0_resources,
	.dev = {
		.platform_data = NULL,
	},
};


static struct resource vdin1_resources[] = {
	[0] = {
		.start = VDIN1_ADDR_START,
		.end   = VDIN1_ADDR_END,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = INT_VDIN1_VSYNC,
		.end   = INT_VDIN1_VSYNC,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device m6tvskt_vdin1_device = {
	.name       = "vdin",
	.id         = 1,
	.num_resources = ARRAY_SIZE(vdin1_resources),
	.resource      = vdin1_resources,
	.dev = {
		.platform_data = NULL,
	},
};

static struct platform_device m6tvskt_hdmirx_device = {
	.name	= "hdmirx",
	.id	= -1,
};

/*
 * add pin mux info for tvafe input
 */
static struct tvafe_pin_mux_s tvafe_pin_mux = { {
	TVAFE_ADC_PIN_A_PGA_0,  //CVBS0_Y = 0,
	TVAFE_ADC_PIN_SOG_7,    //CVBS0_SOG,
	TVAFE_ADC_PIN_A_PGA_2,  //CVBS1_Y,
	TVAFE_ADC_PIN_SOG_0,    //CVBS1_SOG,share with ypbpr
	TVAFE_ADC_PIN_A_PGA_1,  //CVBS2_Y,
	TVAFE_ADC_PIN_SOG_7,    //CVBS2_SOG,
	TVAFE_ADC_PIN_A_3,      //CVBS3_Y,
	TVAFE_ADC_PIN_SOG_7,    //CVBS3_SOG,
	TVAFE_ADC_PIN_NULL,     //CVBS4_Y,
	TVAFE_ADC_PIN_NULL,     //CVBS4_SOG,
	TVAFE_ADC_PIN_NULL,     //CVBS5_Y,
	TVAFE_ADC_PIN_NULL,     //CVBS5_SOG,
	TVAFE_ADC_PIN_NULL,     //CVBS6_Y,
	TVAFE_ADC_PIN_NULL,     //CVBS6_SOG,
	TVAFE_ADC_PIN_NULL,     //CVBS7_Y,
	TVAFE_ADC_PIN_NULL,     //CVBS7_SOG,
	TVAFE_ADC_PIN_A_PGA_2,  //S_VIDEO0_Y,
	TVAFE_ADC_PIN_B_4,      //S_VIDEO0_C,
	TVAFE_ADC_PIN_SOG_6,    //S_VIDEO0_SOG,
	TVAFE_ADC_PIN_A_PGA_3,  //S_VIDEO1_Y,
	TVAFE_ADC_PIN_C_4,      //S_VIDEO1_C,
	TVAFE_ADC_PIN_SOG_7,    //S_VIDEO1_SOG,
	TVAFE_ADC_PIN_NULL,     //S_VIDEO2_Y,
	TVAFE_ADC_PIN_NULL,     //S_VIDEO2_C,
	TVAFE_ADC_PIN_NULL,     //S_VIDEO2_SOG,
	TVAFE_ADC_PIN_NULL,     //S_VIDEO3_Y,
	TVAFE_ADC_PIN_NULL,     //S_VIDEO3_C,
	TVAFE_ADC_PIN_NULL,     //S_VIDEO3_SOG,
	TVAFE_ADC_PIN_NULL,     //S_VIDEO4_Y,
	TVAFE_ADC_PIN_NULL,     //S_VIDEO4_C,
	TVAFE_ADC_PIN_NULL,     //S_VIDEO4_SOG,
	TVAFE_ADC_PIN_NULL,     //S_VIDEO5_Y,
	TVAFE_ADC_PIN_NULL,     //S_VIDEO5_C,
	TVAFE_ADC_PIN_NULL,     //S_VIDEO5_SOG,
	TVAFE_ADC_PIN_NULL,     //S_VIDEO6_Y,
	TVAFE_ADC_PIN_NULL,     //S_VIDEO6_C,
	TVAFE_ADC_PIN_NULL,     //S_VIDEO6_SOG,
	TVAFE_ADC_PIN_NULL,     //S_VIDEO7_Y,
	TVAFE_ADC_PIN_NULL,     //S_VIDEO7_C,
	TVAFE_ADC_PIN_NULL,     //S_VIDEO7_SOG,
	TVAFE_ADC_PIN_A_2,      //VGA0_G,
	TVAFE_ADC_PIN_B_2,      //VGA0_B,
	TVAFE_ADC_PIN_C_2,      //VGA0_R,
	TVAFE_ADC_PIN_SOG_2,    //VGA0_SOG
	TVAFE_ADC_PIN_NULL,     //VGA1_G,
	TVAFE_ADC_PIN_NULL,     //VGA1_B,
	TVAFE_ADC_PIN_NULL,     //VGA1_R,
	TVAFE_ADC_PIN_NULL,     //VGA1_SOG
	TVAFE_ADC_PIN_NULL,     //VGA2_G,
	TVAFE_ADC_PIN_NULL,     //VGA2_B,
	TVAFE_ADC_PIN_NULL,     //VGA2_R,
	TVAFE_ADC_PIN_NULL,     //VGA2_SOG
	TVAFE_ADC_PIN_NULL,     //VGA3_G,
	TVAFE_ADC_PIN_NULL,     //VGA3_B,
	TVAFE_ADC_PIN_NULL,     //VGA3_R,
	TVAFE_ADC_PIN_NULL,     //VGA3_SOG
	TVAFE_ADC_PIN_NULL,     //VGA4_G,
	TVAFE_ADC_PIN_NULL,     //VGA4_B,
	TVAFE_ADC_PIN_NULL,     //VGA4_R,
	TVAFE_ADC_PIN_NULL,     //VGA4_SOG
	TVAFE_ADC_PIN_NULL,     //VGA5_G,
	TVAFE_ADC_PIN_NULL,     //VGA5_B,
	TVAFE_ADC_PIN_NULL,     //VGA5_R,
	TVAFE_ADC_PIN_NULL,     //VGA5_SOG
	TVAFE_ADC_PIN_NULL,     //VGA6_G,
	TVAFE_ADC_PIN_NULL,     //VGA6_B,
	TVAFE_ADC_PIN_NULL,     //VGA6_R,
	TVAFE_ADC_PIN_NULL,     //VGA6_SOG
	TVAFE_ADC_PIN_NULL,     //VGA7_G,
	TVAFE_ADC_PIN_NULL,     //VGA7_B,
	TVAFE_ADC_PIN_NULL,     //VGA7_R,
	TVAFE_ADC_PIN_NULL,     //VGA7_SOG
	TVAFE_ADC_PIN_A_0,      //COMP0_Y,
	TVAFE_ADC_PIN_B_0,      //COMP0_PB,
	TVAFE_ADC_PIN_C_0,      //COMP0_PR,
	TVAFE_ADC_PIN_SOG_0,    //COMP0_SOG
	TVAFE_ADC_PIN_A_1,      //COMP1_Y,
	TVAFE_ADC_PIN_B_1,      //COMP1_PB,
	TVAFE_ADC_PIN_C_1,      //COMP1_PR,
	TVAFE_ADC_PIN_SOG_1,    //COMP1_SOG
	TVAFE_ADC_PIN_A_2,      //COMP2_Y,
	TVAFE_ADC_PIN_B_2,      //COMP2_PB,
	TVAFE_ADC_PIN_C_2,      //COMP2_PR,
	TVAFE_ADC_PIN_SOG_2,    //COMP2_SOG
	TVAFE_ADC_PIN_NULL,     //COMP3_Y,
	TVAFE_ADC_PIN_NULL,     //COMP3_PB,
	TVAFE_ADC_PIN_NULL,     //COMP3_PR,
	TVAFE_ADC_PIN_NULL,     //COMP3_SOG
	TVAFE_ADC_PIN_NULL,     //COMP4_Y,
	TVAFE_ADC_PIN_NULL,     //COMP4_PB,
	TVAFE_ADC_PIN_NULL,     //COMP4_PR,
	TVAFE_ADC_PIN_NULL,     //COMP4_SOG
	TVAFE_ADC_PIN_NULL,     //COMP5_Y,
	TVAFE_ADC_PIN_NULL,     //COMP5_PB,
	TVAFE_ADC_PIN_NULL,     //COMP5_PR,
	TVAFE_ADC_PIN_NULL,     //COMP5_SOG
	TVAFE_ADC_PIN_NULL,     //COMP6_Y,
	TVAFE_ADC_PIN_NULL,     //COMP6_PB,
	TVAFE_ADC_PIN_NULL,     //COMP6_PR,
	TVAFE_ADC_PIN_NULL,     //COMP6_SOG
	TVAFE_ADC_PIN_NULL,     //COMP7_Y,
	TVAFE_ADC_PIN_NULL,     //COMP7_PB,
	TVAFE_ADC_PIN_NULL,     //COMP7_PR,
	TVAFE_ADC_PIN_NULL,     //COMP7_SOG
	TVAFE_ADC_PIN_NULL,     //SCART0_G,
	TVAFE_ADC_PIN_NULL,     //SCART0_B,
	TVAFE_ADC_PIN_NULL,     //SCART0_R,
	TVAFE_ADC_PIN_NULL,     //SCART0_CVBS,
	TVAFE_ADC_PIN_NULL,     //SCART1_G,
	TVAFE_ADC_PIN_NULL,     //SCART1_B,
	TVAFE_ADC_PIN_NULL,     //SCART1_R,
	TVAFE_ADC_PIN_NULL,     //SCART1_CVBS,
	TVAFE_ADC_PIN_NULL,     //SCART2_G,
	TVAFE_ADC_PIN_NULL,     //SCART2_B,
	TVAFE_ADC_PIN_NULL,     //SCART2_R,
	TVAFE_ADC_PIN_NULL,     //SCART2_CVBS,
	TVAFE_ADC_PIN_NULL,     //SCART3_G,
	TVAFE_ADC_PIN_NULL,     //SCART3_B,
	TVAFE_ADC_PIN_NULL,     //SCART3_R,
	TVAFE_ADC_PIN_NULL,     //SCART3_CVBS,
	TVAFE_ADC_PIN_NULL,     //SCART4_G,
	TVAFE_ADC_PIN_NULL,     //SCART4_B,
	TVAFE_ADC_PIN_NULL,     //SCART4_R,
	TVAFE_ADC_PIN_NULL,     //SCART4_CVBS,
	TVAFE_ADC_PIN_NULL,     //SCART5_G,
	TVAFE_ADC_PIN_NULL,     //SCART5_B,
	TVAFE_ADC_PIN_NULL,     //SCART5_R,
	TVAFE_ADC_PIN_NULL,     //SCART5_CVBS,
	TVAFE_ADC_PIN_NULL,     //SCART6_G,
	TVAFE_ADC_PIN_NULL,     //SCART6_B,
	TVAFE_ADC_PIN_NULL,     //SCART6_R,
	TVAFE_ADC_PIN_NULL,     //SCART6_CVBS,
	TVAFE_ADC_PIN_NULL,     //SCART7_G,
	TVAFE_ADC_PIN_NULL,     //SCART7_B,
	TVAFE_ADC_PIN_NULL,     //SCART7_R,
	TVAFE_ADC_PIN_NULL,     //SCART7_CVBS,
	// TVAFE_ADC_PIN_NULL,  //TVAFE_SRC_SIG_MAX_NUM,
}};

static struct resource tvafe_resources[] = {
	[0] = {
		.start = TVAFE_ADDR_START,
		.end   = TVAFE_ADDR_END,
		.flags = IORESOURCE_MEM,
	},
};

static struct platform_device m6tvskt_tvafe_device = {
	.name	= "tvafe",
	.id	= -1,
	.dev	= {
		.platform_data  = &tvafe_pin_mux,
	},
	.num_resources = ARRAY_SIZE(tvafe_resources),
	.resource      = tvafe_resources,
};

static struct platform_device __initdata * m6tvskt_tvin_devices[] = {
	&m6tvskt_vdin0_device,
	&m6tvskt_vdin1_device,
	&m6tvskt_hdmirx_device,
	&m6tvskt_tvafe_device,
};
static pinmux_item_t tvafe_vga_pins[] = {
        {
                .reg = PINMUX_REG(9),
                .clrmask = 0xff<<3,
                .setmask = 0xff<<3,
        },
        PINMUX_END_ITEM
 };
static pinmux_set_t tvafe_vga_set = {
        .chip_select = NULL,
        .pinmux      = &tvafe_vga_pins[0],
};

int __init m6tvskt_tvin_init(void)
{
	int ret;
	ret = platform_add_devices(m6tvskt_tvin_devices,
				   ARRAY_SIZE(m6tvskt_tvin_devices));
        if(pinmux_set(&tvafe_vga_set))
                pr_err("%s: set vga pinmux error.\n",__func__);
	return ret;
}

