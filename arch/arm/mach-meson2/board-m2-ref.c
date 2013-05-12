/*
 * arch/arm/mach-meson2/board-m2-ref.c
 *
 * Copyright (C) 2010 Amlogic, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <linux/init.h>
#include <linux/mm.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/setup.h>
#include <asm/memory.h>
#include <asm/delay.h>
#ifdef CONFIG_CACHE_L2X0
#include <asm/hardware/cache-l2x0.h>
#endif
#include <linux/uart-aml.h>

#include <plat/platform.h>
#include <plat/plat_dev.h>
#include <plat/platform_data.h>
#include <plat/lm.h>
#include <plat/regops.h>

#include <mach/am_regs.h>
#include <mach/clock.h>
#include <mach/map.h>
#include <mach/gpio.h>
#include <mach/irqs.h>

#ifdef CONFIG_SUSPEND
#include <mach/pm.h>
#endif

#ifdef CONFIG_AM_NAND
#include <mach/nand.h>
#include <linux/mtd/partitions.h>
#endif

#ifdef CONFIG_I2C_AML
#include <linux/i2c.h>
#include <linux/i2c-aml.h>
//#include <mach/i2c-aml.h>
#endif

#ifdef CONFIG_CARDREADER
#include <mach/card_io.h>
#endif


#include "board-m2-ref.h"


#ifdef CONFIG_AM_UART
static pinmux_item_t m2_uart_pins[] = {
	{
		.reg = PINMUX_REG(2),
		.clrmask = 0,
		.setmask = (1<<29)|(1<<30),
	},
	PINMUX_END_ITEM,
};

static pinmux_set_t m2_uart_b = {
	.chip_select	= NULL,
	.pinmux		= &m2_uart_pins[0],
};

static struct aml_uart_platform m2_uart_plat = {
	.uart_line[0]	= UART_B,
	.uart_line[1]	= UART_A,

	.pinmux_uart[0] = (void*)&m2_uart_b,
	.pinmux_uart[1] = NULL,
};

static struct platform_device m2_uart_device = {
	.name		= "mesonuart",
	.id 		= -1,
	.num_resources	= 0,
	.resource	= NULL,
	.dev = {
		.platform_data = &m2_uart_plat,
	},
};
#endif /* CONFIG_AM_UART */

#ifdef CONFIG_I2C_AML

static bool pinmux_dummy_share(bool select)
{
	return select;
}

static pinmux_item_t aml_i2c_a_pinmux_item[] = {
	{
	.reg = PINMUX_REG(1),
	.setmask =  (1<<4)|(1<<7),
	},
	PINMUX_END_ITEM
};

static struct aml_i2c_platform aml_i2c_plat_a = {
	.wait_count		= 1000000,
	.wait_ack_interval	= 25,
	.wait_read_interval	= 25,
	.wait_xfer_interval	= 25,
	.master_no		= AML_I2C_MASTER_A,
	.use_pio		= 0,
	.master_i2c_speed	= AML_I2C_SPPED_50K,

	.master_pinmux	= {
		.chip_select	= pinmux_dummy_share,
		.pinmux		= &aml_i2c_a_pinmux_item[0]
	}
};

static pinmux_item_t aml_i2c_b_pinmux_item[]={
	{
	.reg = PINMUX_REG(1),
	.setmask = (1<<14)|(1<<15),
	},
	PINMUX_END_ITEM
};

static struct aml_i2c_platform aml_i2c_plat_b = {
	.wait_count		= 1000000,
	.wait_ack_interval	= 25,
	.wait_read_interval	= 25,
	.wait_xfer_interval	= 25,
	.master_no		= AML_I2C_MASTER_B,
	.use_pio		= 0,
	.master_i2c_speed	= AML_I2C_SPPED_50K,

	.master_pinmux	= {
		.chip_select	= pinmux_dummy_share,
		.pinmux		= &aml_i2c_b_pinmux_item[0]
	}
};


static struct resource aml_i2c_resource_a[] = {
	[0] = {
	.start	= MESON_I2C_MASTER_A_START,
	.end	= MESON_I2C_MASTER_A_END,
	.flags	= IORESOURCE_MEM,
	}
};

static struct resource aml_i2c_resource_b[] = {
	[0] = {
	.start	= MESON_I2C_MASTER_B_START,
	.end	= MESON_I2C_MASTER_B_END,
	.flags	= IORESOURCE_MEM,
	}
};


static struct platform_device aml_i2c_device_a = {
	.name		= "aml-i2c",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(aml_i2c_resource_a),
	.resource	= aml_i2c_resource_a,
	.dev = {
		.platform_data = &aml_i2c_plat_a,
	},
};

static struct platform_device aml_i2c_device_b = {
	.name		= "aml-i2c",
	.id		= 1,
	.num_resources	= ARRAY_SIZE(aml_i2c_resource_b),
	.resource	= aml_i2c_resource_b,
	.dev = {
		.platform_data = &aml_i2c_plat_b,
	},
};

#endif /* CONFIG_I2C_AML */


#ifdef CONFIG_AM_NAND
static struct mtd_partition normal_partition_info[] =
{
	{
		.name   = "logo",
		.offset = 8*1024*1024,
		.size   = 8*1024*1024,
	},
	{
		.name   = "recovery",
		.offset = 16*1024*1024,
		.size   = 8*1024*1024,
	},
	{
		.name   = "boot",
		.offset = 24*1024*1024,
		.size   = 8*1024*1024,
	},
	{
		.name   = "system",
		.offset = 32*1024*1024,
		.size   = 224*1024*1024,
	},
	{
		.name   = "param",
		.offset = 256*1024*1024,
		.size   = 2*1024*1024,
	},
	{
		.name   = "dtv",
		.offset = 258*1024*1024,
		.size   = 32*1024*1024,
	},
	{
		.name   = "atv",
		.offset = 290*1024*1024,
		.size   = 64*1024*1024,
	},
	{
		.name   = "cache",
		.offset = 354*1024*1024,
		.size   = 16*1024*1024,
	},
	{
		.name   = "userdata",
		.offset = MTDPART_OFS_APPEND,
		.size   = MTDPART_SIZ_FULL,
	},
};

static struct aml_nand_platform aml_nand_tv_platform[] = {
	{
	.name = NAND_NORMAL_NAME,
	.chip_enable_pad = AML_NAND_CE0,
	.ready_busy_pad = AML_NAND_CE0,
	.platform_nand_data = {
		.chip = {
			.nr_chips = 1,
			.nr_partitions = ARRAY_SIZE(normal_partition_info),
			.partitions = normal_partition_info,
			.options = ( NAND_TIMING_MODE4 |
				     NAND_ECC_BCH8_MODE |
				     NAND_TWO_PLANE_MODE ),
		},
	},
	.T_REA = 20,
	.T_RHOH = 15,
	}
};

struct aml_nand_device aml_nand_tv_device = {
	.aml_nand_platform = aml_nand_tv_platform,
	.dev_num = ARRAY_SIZE(aml_nand_tv_platform),
};

static struct resource aml_nand_resources[] = {
	{
	.start = 0xc1108600,
	.end   = 0xc1108624,
	.flags = IORESOURCE_MEM,
	},
};

static struct platform_device aml_nand_device = {
	.name = "aml_m2_nand",
	.id = 0,
	.num_resources = ARRAY_SIZE(aml_nand_resources),
	.resource = aml_nand_resources,
	.dev = {
		.platform_data = &aml_nand_tv_device,
	},
};
#endif /* CONFIG_AM_NAND */

#if defined(CONFIG_CARDREADER)

static struct resource amlogic_card_resource[]  = {
	[0] = {
	.start = 0x1200230,   //physical address
	.end   = 0x120024c,
	.flags = 0x200,
	}
};

static struct aml_card_info  amlogic_card_info[] = {
	[0] = {
	.name			= "sd_card",
	.work_mode		= CARD_HW_MODE,
	.io_pad_type		= SDIO_GPIOX_31_36,
	.card_ins_en_reg	= EGPIO_GPIOA_ENABLE,
	.card_ins_en_mask	= PREG_IO_29_MASK,
	.card_ins_input_reg	= EGPIO_GPIOA_INPUT,
	.card_ins_input_mask	= PREG_IO_29_MASK,
	.card_power_en_reg	= EGPIO_GPIOA_ENABLE,
	.card_power_en_mask	= PREG_IO_8_MASK,
	.card_power_output_reg	= EGPIO_GPIOA_OUTPUT,
	.card_power_output_mask	= PREG_IO_8_MASK,
	.card_power_en_lev	= 0,
	.card_wp_en_reg		= EGPIO_GPIOA_ENABLE,
	.card_wp_en_mask	= PREG_IO_28_MASK,
	.card_wp_input_reg	= EGPIO_GPIOA_INPUT,
	.card_wp_input_mask	= PREG_IO_28_MASK,
	.card_extern_init	= 0,
	},
};

static struct aml_card_platform amlogic_card_platform = {
	.card_num = ARRAY_SIZE(amlogic_card_info),
	.card_info = amlogic_card_info,
};

static struct platform_device amlogic_card_device = {
	.name = "AMLOGIC_CARD",
	.id = -1,
	.num_resources = ARRAY_SIZE(amlogic_card_resource),
	.resource = amlogic_card_resource,
	.dev = {
		.platform_data = &amlogic_card_platform,
	},
};

#endif

#if defined(CONFIG_SUSPEND)

static void set_vccx2(int power_on)
{
	/* @todo m2-ref set_vccx2 of struct meson_pm_config */
}

static struct meson_pm_config aml_pm_pdata = {
	.pctl_reg_base	= (void __iomem *)IO_APB_BUS_BASE,
	.mmc_reg_base	= (void __iomem *)APB_REG_ADDR(0x1000),
	.hiu_reg_base	= (void __iomem *)CBUS_REG_ADDR(0x1000),
	.power_key	= (1<<15),
	.ddr_clk	= 0x00120234, // 312m, 0x00110220, //384m
	.sleepcount	= 128,
	.set_vccx2	= set_vccx2,
	.core_voltage_adjust = 5,
};

static struct platform_device aml_pm_device = {
	.name = "pm-meson",
	.dev = {
		.platform_data = &aml_pm_pdata,
	},
	.id = -1,
};

#endif /* CONFIG_SUSPEND */

#if defined(CONFIG_FB_AM)

static struct resource fb_device_resources[] = {
	[0] = {
	.start = OSD1_ADDR_START,
	.end   = OSD1_ADDR_END,
	.flags = IORESOURCE_MEM,
	},
#if defined(CONFIG_FB_OSD2_ENABLE)
	[1] = {
	.start = OSD2_ADDR_START,
	.end   = OSD2_ADDR_END,
	.flags = IORESOURCE_MEM,
	},
#endif
};

static struct platform_device m2_fb_device = {
	.name           = "mesonfb",
	.id             = 0,
	.num_resources  = ARRAY_SIZE(fb_device_resources),
	.resource       = fb_device_resources,
};

#endif /* CONFIG_FB_AM */


static struct platform_device __initdata *platform_devs[] = {
	/* @todo add platform devices */
#ifdef CONFIG_AM_UART
	&m2_uart_device,
#endif

#ifdef CONFIG_I2C_AML
	&aml_i2c_device_a,
	&aml_i2c_device_b,
#endif

#ifdef CONFIG_AM_NAND
	&aml_nand_device,
#endif

#if defined(CONFIG_CARDREADER)
	&amlogic_card_device,
#endif

#if defined(CONFIG_SUSPEND)
	&aml_pm_device,
#endif

#if defined(CONFIG_FB_AM)
	&m2_fb_device,
#endif
};

static struct i2c_board_info __initdata m2_i2c_devs[] = {
	/* @todo add i2c devices */
#ifdef CONFIG_TVIN_TUNER_SI2176
	{
	/*Silicon labs si2176 tuner i2c address if 0xC0*/
	I2C_BOARD_INFO("si2176_tuner_i2c",  0xC0 >> 1),
	},
#endif
};

static int __init m2_i2c_init(void)
{
	i2c_register_board_info(0, m2_i2c_devs, ARRAY_SIZE(m2_i2c_devs));
	return 0;
}

static void __init meson_cache_init(void)
{
#ifdef CONFIG_CACHE_L2X0
	/*
	* Early BRESP, I/D prefetch enabled
	* Non-secure enabled
	* 128kb (16KB/way),
	* 8-way associativity,
	* evmon/parity/share disabled
	* Full Line of Zero enabled
	* Bits:  .111 .... .100 0010 0000 .... .... ...1
	*/
	l2x0_init((void __iomem *)IO_PL310_BASE, 0x7c420001, 0xff800fff);
#endif
}

static void __init device_clk_setting(void)
{
	/* @todo set clock */
}

static void __init device_pinmux_init(void )
{
	/* @todo init pinmux */
}

static void disable_unused_model(void)
{
	//CLK_GATE_OFF(VIDEO_IN);
	//CLK_GATE_OFF(BT656_IN);
	//CLK_GATE_OFF(ETHERNET);
	//CLK_GATE_OFF(SATA);
	//CLK_GATE_OFF(WIFI);
	//video_dac_disable();
	//audio_internal_dac_disable();
}

#ifdef CONFIG_TVIN_TUNER_SI2176
static void __init si2176_tuner_init(void)
{
	WRITE_CBUS_REG_BITS(PREG_FGPIO_O,9,1,0);
	udelay(400);
	WRITE_CBUS_REG_BITS(PREG_FGPIO_O,9,1,1);
}
#endif

static __init void m2_init_machine(void)
{
	meson_cache_init();
	device_clk_setting();
	device_pinmux_init();

	/* add platform devices */
	platform_add_devices(platform_devs, ARRAY_SIZE(platform_devs));

	/* add i2c devices */
#if defined(CONFIG_I2C_AML) || defined(CONFIG_I2C_HW_AML)
	m2_i2c_init();
#endif

	/* custom devices init, pinmux & gpio setting */
#ifdef CONFIG_TVIN_TUNER_SI2176
	si2176_tuner_init();
#endif

	disable_unused_model();
}

/******************************************************************************
 * IO Mapping
 *****************************************************************************/
static __initdata struct map_desc m2_io_desc[] = {
	{
		.virtual	= IO_CBUS_BASE,
		.pfn		= __phys_to_pfn(IO_CBUS_PHY_BASE),
		.length 	= SZ_2M,
		.type		= MT_DEVICE,
	}, {
		.virtual	= IO_AXI_BUS_BASE,
		.pfn		= __phys_to_pfn(IO_AXI_BUS_PHY_BASE),
		.length 	= SZ_1M,
		.type		= MT_DEVICE,
	}, {
		.virtual	= IO_PL310_BASE,
		.pfn		= __phys_to_pfn(IO_PL310_PHY_BASE),
		.length 	= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= IO_AHB_BUS_BASE,
		.pfn		= __phys_to_pfn(IO_AHB_BUS_PHY_BASE),
		.length 	= SZ_16M,
		.type		= MT_DEVICE,
	}, {
		.virtual	= IO_APB_BUS_BASE,
		.pfn		= __phys_to_pfn(IO_APB_BUS_PHY_BASE),
		.length 	= SZ_512K,
		.type		= MT_DEVICE,
	}
};

static __initdata struct map_desc m2_video_mem_desc[] = {
	{
		.virtual	= PAGE_ALIGN(__phys_to_virt(RESERVED_MEM_START)),
		.pfn		= __phys_to_pfn(RESERVED_MEM_START),
		.length 	= RESERVED_MEM_END - RESERVED_MEM_START + 1,
		.type		= MT_DEVICE,
	}
};

static __init void m2_map_io(void)
{
	iotable_init(m2_io_desc, ARRAY_SIZE(m2_io_desc));
	iotable_init(m2_video_mem_desc, ARRAY_SIZE(m2_video_mem_desc));
}

#if 0
static __init void m2_init_irq(void)
{
	meson_init_irq();
}
#endif

static __init void m2_fixup(struct machine_desc *mach, struct tag *tag, char **cmdline, struct meminfo *m)
{
	struct membank *pbank;
	m->nr_banks = 0;
	pbank = &m->bank[m->nr_banks];
	pbank->start = PAGE_ALIGN(PHYS_MEM_START);
	pbank->size  = SZ_64M & PAGE_MASK;
	m->nr_banks++;
	pbank = &m->bank[m->nr_banks];
	pbank->start = PAGE_ALIGN(RESERVED_MEM_END + 1);
	pbank->size  = (PHYS_MEM_END - RESERVED_MEM_END) & PAGE_MASK;
	m->nr_banks++;
}

MACHINE_START(MESON2_REF, "Amlogic Meson2 Reference Development Platform")
	.boot_params	= BOOT_PARAMS_OFFSET,
	.map_io 	= m2_map_io,
	.init_irq	= m2_init_irq,
	.timer		= &m2_sys_timer,
	.init_machine	= m2_init_machine,
	.fixup		= m2_fixup,
	.video_start	= RESERVED_MEM_START,
	.video_end	= RESERVED_MEM_END,
MACHINE_END

