/*
 * arch/arm/mach-meson/board-8726m-dvbc.c
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
#include <linux/mtd/partitions.h>
#include <linux/device.h>
#include <linux/spi/flash.h>
#include <linux/delay.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/setup.h>
#include <asm/memory.h>
#include <asm/mach/map.h>

#include <plat/platform.h>
#include <plat/plat_dev.h>
#include <plat/lm.h>

#include <mach/power_gate.h>
#include <mach/memory.h>
#include <mach/hardware.h>
#include <mach/clock.h>
#include <mach/pinmux.h>
#include <mach/clk_set.h>
#include <mach/gpio.h>

#ifdef CONFIG_AM_UART
#include <linux/uart-aml.h>
#endif

#ifdef CONFIG_AM_ETHERNET
#include <mach/am_regs.h>
#include <mach/am_eth_reg.h>
#include <mach/am_eth_pinmux.h>
#endif

#include "board-8726m-refc03.h"


#ifdef CONFIG_AM_UART
static pinmux_item_t uart_pins[] = {
	{
		.reg = PINMUX_REG(2),
		.clrmask = 0,
		.setmask = (1 << 11) | (1 << 15),
	},
	PINMUX_END_ITEM
};

static pinmux_set_t aml_uart_a = {
	.chip_select = NULL,
	.pinmux = &uart_pins[0],
};

static struct aml_uart_platform aml_uart_plat = {
	.uart_line[0] = UART_A,
	.uart_line[1] = UART_B,

	.pinmux_uart[0] = (void *)&aml_uart_a,
	.pinmux_uart[1] = NULL,
};

static struct platform_device aml_uart_device = {
	.name = "mesonuart",
	.id = -1,
	.num_resources = 0,
	.resource = NULL,
	.dev = {
		.platform_data = &aml_uart_plat,
	},
};
#endif

#ifdef CONFIG_AM_ETHERNET
static void aml_eth_reset(void)
{
#define DELAY_TIME 500
	int i;

	//eth_set_pinmux(ETH_BANK2_GPIOD15_D23, ETH_CLK_OUT_GPIOD24_REG5_1, 0);
	CLEAR_CBUS_REG_MASK(PREG_ETHERNET_ADDR0, 1);
	SET_CBUS_REG_MASK(PREG_ETHERNET_ADDR0, (1 << 1));
	SET_CBUS_REG_MASK(PREG_ETHERNET_ADDR0, 1);
	for (i = 0; i < DELAY_TIME; i++)
		udelay(100);

	/*reset*/
	///GPIOC19/NA   nRst;
	set_gpio_mode(PREG_GGPIO,12,GPIO_OUTPUT_MODE);
	set_gpio_val(PREG_GGPIO,12,0);
	udelay(100);    //waiting reset end;
	set_gpio_val(PREG_GGPIO,12,1);
	udelay(10);     //waiting reset end;
}

static void aml_eth_clock_enable(void)
{
	printk(KERN_INFO "****** aml_eth_clock_enable() ******\n");
	eth_clk_set(ETH_CLKSRC_APLL_CLK, 400*CLK_1M, 50*CLK_1M);
}

static void aml_eth_clock_disable(void)
{
	printk(KERN_INFO "****** aml_eth_clock_disable() ******\n");
	eth_clk_set(ETH_CLKSRC_APLL_CLK, 0, 0);
}

static pinmux_item_t aml_eth_pins[] = {
	/* RMII pin-mux */
	{
		.reg = PINMUX_REG(ETH_BANK2_REG1),
		.clrmask = 0,
		.setmask = ETH_BANK2_REG1_VAL,
	},
	/* RMII CLK50 in-out */
	{
		.reg = PINMUX_REG(5),
		.clrmask = 0,
		.setmask = 1 << 1,
	},
	PINMUX_END_ITEM
};

static struct aml_eth_platdata aml_eth_pdata __initdata = {
	.pinmux_items  = aml_eth_pins,
	.clock_enable  = aml_eth_clock_enable,
	.clock_disable = aml_eth_clock_disable,
	.reset         = aml_eth_reset,
};

static void __init setup_eth_device(void)
{
	meson_eth_set_platdata(&aml_eth_pdata);
}
#endif

#ifdef CONFIG_AM_REMOTE
static pinmux_item_t aml_remote_pins[] = {
	{
		.reg = PINMUX_REG(5),
		.clrmask = 0,
		.setmask = 1 << 31,
	},
	PINMUX_END_ITEM
};

static struct aml_remote_platdata aml_remote_pdata __initdata = {
	.pinmux_items  = aml_remote_pins,
	.ao_baseaddr = P_IR_DEC_LDR_ACTIVE,
};

static void __init setup_remote_device(void)
{
	meson_remote_set_platdata(&aml_remote_pdata);
}
#endif

#ifdef CONFIG_FB_AM
static struct resource meson_fb_resource[] = {
	[0] = {
		.start = OSD1_ADDR_START,
		.end   = OSD1_ADDR_END,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = OSD2_ADDR_START,
		.end   = OSD2_ADDR_END,
		.flags = IORESOURCE_MEM,
	},
};
#endif

#ifdef CONFIG_AM_STREAMING
static struct resource meson_codec_resource[] = {
	[0] = {
		.start = CODEC_ADDR_START,
		.end   = CODEC_ADDR_END,
		.flags = IORESOURCE_MEM,
    },
	[1] = {
		.start = STREAMBUF_ADDR_START,
		.end = STREAMBUF_ADDR_END,
		.flags = IORESOURCE_MEM,
	},
};
#endif

static int __init setup_devices_resource(void)
{
#ifdef CONFIG_FB_AM
	setup_fb_resource(meson_fb_resource, ARRAY_SIZE(meson_fb_resource));
#endif

#ifdef CONFIG_AM_STREAMING
	setup_codec_resource(meson_codec_resource,ARRAY_SIZE(meson_codec_resource));
#endif

	return 0;
}

static void __init device_clk_setting(void)
{
	/*Demod CLK for eth and sata*/
	//demod_apll_setting(0, 1200*CLK_1M);

	/*eth clk*/
	//eth_clk_set(ETH_CLKSRC_SYS_D3,900*CLK_1M/3,50*CLK_1M);
	//eth_clk_set(ETH_CLKSRC_APLL_CLK, 400*CLK_1M, 50*CLK_1M);
}

static void disable_unused_model(void)
{
}

static struct platform_device __initdata *platform_devs[] = {
#ifdef CONFIG_AM_UART
	&aml_uart_device,
#endif
#ifdef CONFIG_AM_ETHERNET
	&meson_device_eth,
#endif
#ifdef CONFIG_AM_REMOTE
	&meson_device_remote,
#endif
#ifdef CONFIG_AM_TV_OUTPUT
	&meson_device_vout,
#endif
#ifdef CONFIG_FB_AM
	&meson_device_fb,
#endif
#ifdef CONFIG_AM_STREAMING
	&meson_device_codec,
#endif

};

static __init void m1_init_machine(void)
{
	device_clk_setting();

#ifdef CONFIG_AM_ETHERNET
	setup_eth_device();
#endif

#ifdef CONFIG_AM_REMOTE
	setup_remote_device();
#endif

	setup_devices_resource();

	platform_add_devices(platform_devs, ARRAY_SIZE(platform_devs));

	disable_unused_model();
}

/***********************************************************************
 * IO Mapping
 **********************************************************************/
static __initdata struct map_desc meson_io_desc[] = {
    {
        .virtual    = IO_CBUS_BASE,
        .pfn        = __phys_to_pfn(IO_CBUS_PHY_BASE),
        .length     = SZ_2M,
        .type       = MT_DEVICE,
    } , {
        .virtual    = IO_AXI_BUS_BASE,
        .pfn        = __phys_to_pfn(IO_AXI_BUS_PHY_BASE),
        .length     = SZ_1M,
        .type       = MT_DEVICE,
    } , {
        .virtual    = IO_PL310_BASE,
        .pfn        = __phys_to_pfn(IO_PL310_PHY_BASE),
        .length     = SZ_4K,
        .type       = MT_DEVICE,
    } , {
        .virtual    = IO_AHB_BUS_BASE,
        .pfn        = __phys_to_pfn(IO_AHB_BUS_PHY_BASE),
        .length     = SZ_16M,
        .type       = MT_DEVICE,
    } , {
        .virtual    = IO_APB_BUS_BASE,
        .pfn        = __phys_to_pfn(IO_APB_BUS_PHY_BASE),
        .length     = SZ_512K,
        .type       = MT_DEVICE,
    }
};

/*VIDEO MEMORY MAPING*/
static __initdata struct map_desc meson_video_mem_desc[] = {
	{
		.virtual	= PAGE_ALIGN(__phys_to_virt(RESERVED_MEM_START)),
		.pfn		= __phys_to_pfn(RESERVED_MEM_START),
		.length		= RESERVED_MEM_END-RESERVED_MEM_START+1,
		.type		= MT_DEVICE,
	},
};

static __init void m1_map_io(void)
{
    iotable_init(meson_io_desc, ARRAY_SIZE(meson_io_desc));
	iotable_init(meson_video_mem_desc, ARRAY_SIZE(meson_video_mem_desc));
}

static __init void m1_irq_init(void)
{
	meson_init_irq();
}

static __init void m1_fixup(struct machine_desc *mach, struct tag *tag, char **cmdline, struct meminfo *m)
{
	struct membank *pbank;
	m->nr_banks = 0;
	pbank=&m->bank[m->nr_banks];
	pbank->start = PAGE_ALIGN(PHYS_MEM_START);
	pbank->size  = SZ_64M & PAGE_MASK;
	m->nr_banks++;
	pbank=&m->bank[m->nr_banks];
	pbank->start = PAGE_ALIGN(RESERVED_MEM_END+1);
	pbank->size  = (PHYS_MEM_END-RESERVED_MEM_END) & PAGE_MASK;
	m->nr_banks++;
}

MACHINE_START(MESON, "AMLOGIC MESON-M1 8726M DVBC")
	.boot_params	= BOOT_PARAMS_OFFSET,
	.map_io			= m1_map_io,
	.init_irq		= m1_irq_init,
	.timer			= &meson_sys_timer,
	.init_machine	= m1_init_machine,
	.fixup			= m1_fixup,
	.video_start	= RESERVED_MEM_START,
	.video_end		= RESERVED_MEM_END,
MACHINE_END
