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
    .name         = "meson_uart",
    .id       = -1,
    .num_resources    = 0,
    .resource     = NULL,
    .dev = {
        .platform_data = &aml_uart_plat,
    },
};
#endif
static struct platform_device aml_uart_device = {
    .name         = "meson_uart",
    .id       = 0,
    .num_resources    = 0,
    .resource     = NULL,
    .dev = {
        .platform_data = NULL,
    },
};


struct platform_device * early_platform_devices[]={
	&aml_uart_device,
};


static __init void meson_init_machine(void)
{
	mesonplat_register_device("meson_uart","AO",NULL);
    //~ mesonplat_register_device("meson_uart","1",NULL);
}
static __init void meson_init_early(void)
{///boot seq 1
/*	
	early_platform_add_devices(early_platform_devices,
				   ARRAY_SIZE(early_platform_devices));
				   */
	mesonplat_register_device_early("meson_uart","AO",NULL);
	parse_early_param();
	/* Let earlyprintk output early console messages */
	early_platform_driver_probe("earlyprintk", 4, 0);
	
}


MACHINE_START(M3_REF, "Meson 3 reference board")
    .boot_params    = BOOT_PARAMS_OFFSET,
    .map_io         = meson_map_io,///2
    .init_early		= meson_init_early,///3
    .init_irq       = meson_init_irq,///0
    .timer          = &meson_sys_timer,
    .init_machine   = meson_init_machine,
    .fixup          = meson_fixup,///1
    .video_start    = RESERVED_MEM_START,
    .video_end      = RESERVED_MEM_END,
MACHINE_END
