/*
 *
 * arch/arm/mach-meson/meson.c
 *
 *  Copyright (C) 2010 AMLOGIC, INC.
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
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/mtd/partitions.h>
#include <linux/device.h>
#include <linux/spi/flash.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <asm/mach-types.h>
#include <asm/setup.h>
#include <asm/memory.h>
#include <plat/platform.h>
#include <plat/plat_dev.h>
#include <plat/platform_data.h>
#include <plat/lm.h>
#include <plat/regops.h>
#include <mach/am_regs.h>
#include "board-m3-ref.h"
#include <mach/map.h>
#include <mach/i2c_aml.h>
#include <mach/usbclock.h>
#include "common-data.h"
#if 0

static  pinmux_item_t   uart_pins[]={
		{
			.reg=PINMUX_REG(AO),
			.clrmask=3<<16,
			.setmask=3<<11
		},
		PINMUX_END_ITEM
	};
static  pinmux_set_t   aml_uart_ao={
	.chip_select=NULL,
	.pinmux=&uart_pins[0]
};
static struct aml_uart_platform  __initdata aml_uart_plat = {
    
    .uart_line[0]       =  UART_AO,
    .uart_line[1]       =   UART_A,
    .uart_line[2]       =   UART_B,
    .uart_line[3]       =   UART_C,

    
    .pinmux_uart[0] = (void*)&aml_uart_ao,
    .pinmux_uart[1] = NULL,
    .pinmux_uart[2] = NULL,
    .pinmux_uart[3] = NULL,
};

static struct platform_device aml_uart_device = {
    .name         = "mesonuart",
    .id       = -1,
    .num_resources    = 0,
    .resource     = NULL,
    .dev = {
        .platform_data = &aml_uart_plat,
    },
};
#endif






static __init void meson_init_machine(void)
{
    
    
///platform_device_register(&aml_uart_device);
///   platform_add_devices(platform_devs, ARRAY_SIZE(platform_devs));

}


MACHINE_START(M3_SKT, "Meson 3 socket board")
    .boot_params    = BOOT_PARAMS_OFFSET,
    .map_io         = meson_map_io,
    .init_irq       = meson_init_irq,
    .timer          = &meson_sys_timer,
    .init_machine   = meson_init_machine,
    .fixup          = meson_fixup,
    .video_start    = RESERVED_MEM_START,
    .video_end      = RESERVED_MEM_END,
MACHINE_END
