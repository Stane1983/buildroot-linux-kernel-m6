/*
 * arch/arm/mach-meson2/board-m2-skt.c
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
#ifdef CONFIG_CACHE_L2X0
#include <asm/hardware/cache-l2x0.h>
#endif

#include <plat/platform.h>
#include <plat/plat_dev.h>
#include <plat/platform_data.h>
#include <plat/lm.h>
#include <plat/regops.h>

#include <mach/am_regs.h>
#include <mach/clock.h>
#include <mach/map.h>
#include <mach/gpio.h>
#include <mach/pinmux.h>
#ifdef CONFIG_SUSPEND
#include <mach/pm.h>
#endif


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

static struct platform_device __initdata *platform_devs[] = {
    /* @todo add platform devices */
};

static struct i2c_board_info __initdata m2_i2c_devs[] = {
    /* @todo add i2c devices */
};

static int __init m2_i2c_init(void)
{
    i2c_register_board_info(0, m2_i2c_devs, ARRAY_SIZE(m2_i2c_devs));
    return 0;
}

static void __init device_pinmux_init(void )
{
    /* @todo init pinmux */
}

static void __init device_clk_setting(void)
{
    /* @todo set clock */
}

static void disable_unused_model(void)
{
    CLK_GATE_OFF(VIDEO_IN);
    CLK_GATE_OFF(BT656_IN);
    CLK_GATE_OFF(ETHERNET);
    CLK_GATE_OFF(SATA);
    CLK_GATE_OFF(WIFI);
    video_dac_disable();
    //audio_internal_dac_disable();
}

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
    disable_unused_model();
}

/******************************************************************************
 * IO Mapping
 *****************************************************************************/
static __initdata struct map_desc m2_io_desc[] = {
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
    }, {
        .virtual    = PAGE_ALIGN(__phys_to_virt(RESERVED_MEM_START)),
        .pfn        = __phys_to_pfn(RESERVED_MEM_START),
        .length     = RESERVED_MEM_END - RESERVED_MEM_START + 1,
        .type       = MT_DEVICE,
    }
};

static  void __init m2_map_io(void)
{
    iotable_init(m2_io_desc, ARRAY_SIZE(m2_io_desc));
}

static __init void m2_irq_init(void)
{
    meson_init_irq();
}

static __init void m2_fixup(struct machine_desc *mach, struct tag *tag, char **cmdline, struct meminfo *m)
{
    struct membank *pbank;
    m->nr_banks = 0;
    pbank = &m->bank[m->nr_banks];
    pbank->start = PAGE_ALIGN(PHYS_MEM_START);
    pbank->size  = SZ_64M & PAGE_MASK;
    pbank->node  = PHYS_TO_NID(PHYS_MEM_START);
    m->nr_banks++;
    pbank = &m->bank[m->nr_banks];
    pbank->start = PAGE_ALIGN(RESERVED_MEM_END + 1);
    pbank->size  = (PHYS_MEM_END - RESERVED_MEM_END) & PAGE_MASK;
    pbank->node  = PHYS_TO_NID(RESERVED_MEM_END + 1);
    m->nr_banks++;

}

MACHINE_START(MESON2_SKT, "Amlogic Meson2 Socket Development Platform")
    .boot_params    = BOOT_PARAMS_OFFSET,
    .map_io         = m2_map_io,
    .init_irq       = m2_irq_init,
    .timer          = &m2_sys_timer,
    .init_machine   = m2_init_machine,
    .fixup          = m2_fixup,
    .video_start    = RESERVED_MEM_START,
    .video_end      = RESERVED_MEM_END,
MACHINE_END
