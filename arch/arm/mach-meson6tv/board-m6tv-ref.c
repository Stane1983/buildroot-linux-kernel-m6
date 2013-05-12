/*
 * arch/arm/mach-meson6tv/board-m6tv-ref.c
 *
 * Copyright (C) 2011-2012 Amlogic, Inc.
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
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/dma-mapping.h>
#include <linux/device.h>
#include <linux/reboot.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/setup.h>
#include <asm/memory.h>
#include <asm/mach/map.h>
#include <plat/platform.h>
#include <plat/plat_dev.h>
#include <plat/platform_data.h>
#include <linux/io.h>
#include <plat/io.h>
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
#include <linux/io.h>
#include <plat/io.h>

#include <mach/map.h>
#include <mach/i2c_aml.h>
#include <mach/nand.h>
#include <mach/usbclock.h>
#include <mach/usbsetting.h>
#include <mach/pm.h>
#include <mach/gpio_data.h>
#include <mach/pinmux.h>

#include <linux/uart-aml.h>
#include <linux/i2c-aml.h>

#include "board-m6tv-ref.h"

#ifdef CONFIG_MMC_AML
#include <mach/mmc.h>
#endif

#ifdef CONFIG_CARDREADER
#include <mach/card_io.h>
#endif // CONFIG_CARDREADER

#include <mach/gpio.h>
#include <mach/gpio_data.h>


#ifdef CONFIG_EFUSE
#include <linux/efuse.h>
#endif
#ifdef CONFIG_SND_AML_M6TV_AUDIO_CODEC
#include <sound/aml_m6tv_audio.h>
#endif
#ifdef CONFIG_SND_SOC_DUMMY_CODEC
#include <sound/dummy_codec.h>
#endif

#ifdef CONFIG_AM_WIFI
#include <plat/wifi_power.h>
#endif

#ifdef CONFIG_AM_ETHERNET
#include <mach/am_regs.h>
#include <mach/am_eth_reg.h>
#endif

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

static struct resource meson_codec_resource[] = {
    [0] = {
        .start = CODEC_ADDR_START,
        .end   = CODEC_ADDR_END,
        .flags = IORESOURCE_MEM,
    },
    [1] = {
        .start = STREAMBUF_ADDR_START,
        .end   = STREAMBUF_ADDR_END,
        .flags = IORESOURCE_MEM,
    },
};

#ifdef CONFIG_POST_PROCESS_MANAGER
static struct resource ppmgr_resources[] = {
    [0] = {
        .start = PPMGR_ADDR_START,
        .end   = PPMGR_ADDR_END,
        .flags = IORESOURCE_MEM,
    },
};

static struct platform_device ppmgr_device = {
    .name   = "ppmgr",
    .id     = 0,
    .num_resources = ARRAY_SIZE(ppmgr_resources),
    .resource      = ppmgr_resources,
};
#endif

#ifdef CONFIG_V4L_AMLOGIC_VIDEO2
static struct resource amlvideo2_resources[] = {
    [0] = {
        .start = AMLVIDEO2_ADDR_START,
        .end   = AMLVIDEO2_ADDR_END,
        .flags = IORESOURCE_MEM,
    },
};

static struct platform_device amlvideo2_device = {
    .name   = "amlvideo2",
    .id     = 0,
    .num_resources = ARRAY_SIZE(amlvideo2_resources),
    .resource      = amlvideo2_resources,
};
#endif

#if defined(CONFIG_AM_DEINTERLACE) || defined (CONFIG_DEINTERLACE)
static struct resource deinterlace_resources[] = {
    [0] = {
        .start =  DI_ADDR_START,
        .end   = DI_ADDR_END,
        .flags = IORESOURCE_MEM,
    },
};

static struct platform_device deinterlace_device = {
    .name       = "deinterlace",
    .id         = 0,
    .num_resources = ARRAY_SIZE(deinterlace_resources),
    .resource      = deinterlace_resources,
};
#endif

#ifdef CONFIG_FREE_SCALE
static struct resource freescale_resources[] = {
    [0] = {
        .start = FREESCALE_ADDR_START,
        .end   = FREESCALE_ADDR_END,
        .flags = IORESOURCE_MEM,
    },
};

static struct platform_device freescale_device = {
    .name       = "freescale",
    .id         = 0,
    .num_resources  = ARRAY_SIZE(freescale_resources),
    .resource       = freescale_resources,
};
#endif
static  int __init setup_devices_resource(void)
{
    setup_fb_resource(meson_fb_resource, ARRAY_SIZE(meson_fb_resource));
#ifdef CONFIG_AM_STREAMING
    setup_codec_resource(meson_codec_resource, ARRAY_SIZE(meson_codec_resource));
#endif
    return 0;
}

/***********************************************************
*Remote Section
************************************************************/
#ifdef CONFIG_AM_REMOTE
#include <plat/remote.h>

static pinmux_item_t aml_remote_pins[] = {
    {
        .reg = PINMUX_REG(AO),
        .clrmask = 0,
        .setmask = 1 << 0,
    },
    PINMUX_END_ITEM
};

static struct aml_remote_platdata aml_remote_pdata __initdata = {
    .pinmux_items  = aml_remote_pins,
    .ao_baseaddr = P_AO_IR_DEC_LDR_ACTIVE,
};

static void __init setup_remote_device(void)
{
    meson_remote_set_platdata(&aml_remote_pdata);
}
#endif
/***********************************************************************
 * I2C Section
 **********************************************************************/
#if defined(CONFIG_I2C_AML) || defined(CONFIG_I2C_HW_AML)
static bool pinmux_dummy_share(bool select)
{
    return select;
}

static pinmux_item_t aml_i2c_a_pinmux_item[] = {
    {
        .reg = 5,
        //.clrmask = (3<<24)|(3<<30),
        .setmask = 3<<30
    },
    PINMUX_END_ITEM
};

static struct aml_i2c_platform aml_i2c_plat_a = {
    .wait_count         = 50000,
    .wait_ack_interval   = 5,
    .wait_read_interval  = 5,
    .wait_xfer_interval   = 5,
    .master_no      = AML_I2C_MASTER_A,
    .use_pio        = 0,
    .master_i2c_speed   = AML_I2C_SPPED_300K,

    .master_pinmux  = {
        .chip_select    = pinmux_dummy_share,
        .pinmux     = &aml_i2c_a_pinmux_item[0]
    }
};

static pinmux_item_t aml_i2c_b_pinmux_item[]= {
    {
        .reg = 5,
        //.clrmask = (3<<28)|(3<<26),
        .setmask = 3<<26
    },
    PINMUX_END_ITEM
};

static struct aml_i2c_platform aml_i2c_plat_b = {
    .wait_count     = 50000,
    .wait_ack_interval = 5,
    .wait_read_interval = 5,
    .wait_xfer_interval = 5,
    .master_no      = AML_I2C_MASTER_B,
    .use_pio        = 0,
    .master_i2c_speed   = AML_I2C_SPPED_300K,

    .master_pinmux  = {
        .chip_select    = pinmux_dummy_share,
        .pinmux     = &aml_i2c_b_pinmux_item[0]
    }
};

static pinmux_item_t aml_i2c_ao_pinmux_item[] = {
    {
        .reg = AO,
        .clrmask  = 3<<1,
        .setmask = 3<<5
    },
    PINMUX_END_ITEM
};

static struct aml_i2c_platform aml_i2c_plat_ao = {
    .wait_count     = 50000,
    .wait_ack_interval  = 5,
    .wait_read_interval = 5,
    .wait_xfer_interval = 5,
    .master_no      = AML_I2C_MASTER_AO,
    .use_pio        = 0,
    .master_i2c_speed   = AML_I2C_SPPED_100K,

    .master_pinmux  = {
        .pinmux     = &aml_i2c_ao_pinmux_item[0]
    }
};

static struct resource aml_i2c_resource_a[] = {
    [0] = {
        .start = MESON_I2C_MASTER_A_START,
        .end   = MESON_I2C_MASTER_A_END,
        .flags = IORESOURCE_MEM,
    }
};

static struct resource aml_i2c_resource_b[] = {
    [0] = {
        .start = MESON_I2C_MASTER_B_START,
        .end   = MESON_I2C_MASTER_B_END,
        .flags = IORESOURCE_MEM,
    }
};

static struct resource aml_i2c_resource_ao[] = {
    [0]= {
        .start =    MESON_I2C_MASTER_AO_START,
        .end   =    MESON_I2C_MASTER_AO_END,
        .flags =    IORESOURCE_MEM,
    }
};

static struct platform_device aml_i2c_device_a = {
    .name     = "aml-i2c",
    .id       = 0,
    .num_resources    = ARRAY_SIZE(aml_i2c_resource_a),
    .resource     = aml_i2c_resource_a,
    .dev = {
        .platform_data = &aml_i2c_plat_a,
    },
};

static struct platform_device aml_i2c_device_b = {
    .name     = "aml-i2c",
    .id       = 1,
    .num_resources    = ARRAY_SIZE(aml_i2c_resource_b),
    .resource     = aml_i2c_resource_b,
    .dev = {
        .platform_data = &aml_i2c_plat_b,
    },
};

static struct platform_device aml_i2c_device_ao = {
    .name     = "aml-i2c",
    .id       = 2,
    .num_resources    = ARRAY_SIZE(aml_i2c_resource_ao),
    .resource     = aml_i2c_resource_ao,
    .dev = {
        .platform_data = &aml_i2c_plat_ao,
    },
};

static struct i2c_board_info __initdata aml_i2c_bus_info_a[] = {
#ifdef CONFIG_SND_SOC_STA339X
    {
        I2C_BOARD_INFO("sta33x",  0x38>>1),
        .platform_data = (void *)&sta339x_pdata,
    },
#endif
#ifdef CONFIG_SND_SOC_STA380
    {
        I2C_BOARD_INFO("sta380BW",  0x38>>1),
        .platform_data = NULL,
    },
#endif
#ifdef CONFIG_SND_SOC_TAS5711
    {
	I2C_BOARD_INFO("tas5711",  0x36>>1),
	.platform_data = NULL,
    },
#endif
#ifdef CONFIG_SND_SOC_RT5631
    {
        I2C_BOARD_INFO("rt5631", 0x1A),
        .platform_data = (void *)NULL,
    },
#endif
#ifdef CONFIG_EEPROM_AT24
    {
        I2C_BOARD_INFO("at24",  0x50),
        .platform_data = &at24_pdata,
    },
    {
        I2C_BOARD_INFO("at24",  0x51),
    },
#endif

};

static struct i2c_board_info __initdata aml_i2c_bus_info_ao[] = {

};


static struct i2c_board_info __initdata aml_i2c_bus_info_b[] = {

};
static int __init aml_i2c_init(void)
{
    i2c_register_board_info(0, aml_i2c_bus_info_a,
                            ARRAY_SIZE(aml_i2c_bus_info_a));
    /*i2c_register_board_info(1, aml_i2c_bus_info_b,
    ARRAY_SIZE(aml_i2c_bus_info_b));
    i2c_register_board_info(2, aml_i2c_bus_info_ao,
    ARRAY_SIZE(aml_i2c_bus_info_ao));*/
/*
#ifdef CONFIG_AM_SI2176
      //reset for si2176            
      WRITE_MPEG_REG_BITS(0x2018,0,8,1);
      printk("si2176 tuner reset.\n");
      WRITE_MPEG_REG_BITS(0x2019,0,8,1);
      udelay(400);
      WRITE_MPEG_REG_BITS(0x2019,1,8,1);
#endif
*/

    return 0;
}
#endif

#if defined(CONFIG_I2C_SW_AML)
//#include "gpio_data.c"
static struct aml_sw_i2c_platform aml_sw_i2c_plat_1 = {
    .sw_pins = {
        .scl_reg_out        = P_PREG_PAD_GPIO4_O,
        .scl_reg_in = P_PREG_PAD_GPIO4_I,
        .scl_bit        = 26,
        .scl_oe     = P_PREG_PAD_GPIO4_EN_N,
        .sda_reg_out        = P_PREG_PAD_GPIO4_O,
        .sda_reg_in = P_PREG_PAD_GPIO4_I,
        .sda_bit        = 25,
        .sda_oe     = P_PREG_PAD_GPIO4_EN_N,
    },
    .udelay     = 2,
     .timeout        = 100,
  };

static struct platform_device aml_sw_i2c_device_1 = {
    .name     = "aml-sw-i2c",
    .id       = -1,
    .dev = {
        .platform_data = &aml_sw_i2c_plat_1,
    },
};
#endif

/***********************************************************************
 * UART Section
 **********************************************************************/
static pinmux_item_t uart_pins[] = {
    {
        .reg = PINMUX_REG(AO),
        .setmask = 3 << 11
    },
    PINMUX_END_ITEM
};

static pinmux_set_t aml_uart_ao = {
    .chip_select = NULL,
    .pinmux = &uart_pins[0]
};

static struct aml_uart_platform  aml_uart_plat = {
    .uart_line[0]   = UART_AO,
    .uart_line[1]   = UART_A,
    .uart_line[2]   = UART_B,
    .uart_line[3]   = UART_C,
    .uart_line[4]   = UART_D,

    .pinmux_uart[0] = (void*)&aml_uart_ao,
    .pinmux_uart[1] = NULL,
    .pinmux_uart[2] = NULL,
    .pinmux_uart[3] = NULL,
    .pinmux_uart[4] = NULL
};

static struct platform_device aml_uart_device = {
    .name   = "mesonuart",
    .id     = -1,
    .num_resources  = 0,
    .resource   = NULL,
    .dev = {
        .platform_data = &aml_uart_plat,
    },
};
#ifdef CONFIG_AM_ETHERNET
#include <plat/eth.h>
//#define ETH_MODE_RGMII
#define ETH_MODE_RMII_INTERNAL
//#define ETH_MODE_RMII_EXTERNAL
static void aml_eth_reset(void)
{
    unsigned int val = 0;

    printk(KERN_INFO "****** aml_eth_reset() ******\n");
#ifdef ETH_MODE_RGMII
    val = 0x211;
#else
    val = 0x241;
#endif
    /* setup ethernet mode */
    aml_set_reg32_mask(P_PREG_ETHERNET_ADDR0, val);

    /* setup ethernet interrupt */
    aml_set_reg32_mask(P_SYS_CPU_0_IRQ_IN0_INTR_MASK, 1 << 8);
    aml_set_reg32_mask(P_SYS_CPU_0_IRQ_IN1_INTR_STAT, 1 << 8);

    /* hardware reset ethernet phy */
    gpio_out(PAD_GPIOY_15, 0);
    msleep(20);
    gpio_out(PAD_GPIOY_15, 1);
}

static void aml_eth_clock_enable(void)
{
    unsigned int val = 0;

    printk(KERN_INFO "****** aml_eth_clock_enable() ******\n");
#ifdef ETH_MODE_RGMII
    val = 0x309;
#elif defined(ETH_MODE_RMII_EXTERNAL)
    val = 0x130;
#else
    val = 0x702;
#endif
    /* setup ethernet clk */
    aml_set_reg32_mask(P_HHI_ETH_CLK_CNTL, val);
}

static void aml_eth_clock_disable(void)
{
    printk(KERN_INFO "****** aml_eth_clock_disable() ******\n");
    /* disable ethernet clk */
    aml_clr_reg32_mask(P_HHI_ETH_CLK_CNTL, 1 << 8);
}

static pinmux_item_t aml_eth_pins[] = {
    /* RMII pin-mux */
    {
        .reg = PINMUX_REG(6),
        .clrmask = 0,
#ifdef ETH_MODE_RMII_EXTERNAL
        .setmask = 0x8007ffe0,
#else
        .setmask = 0x4007ffe0,
#endif
    },
    PINMUX_END_ITEM
};

static pinmux_set_t aml_eth_pinmux = {
    .chip_select = NULL,
    .pinmux = aml_eth_pins,
};

static void aml_eth_pinmux_setup(void)
{
    printk(KERN_INFO "****** aml_eth_pinmux_setup() ******\n");

    pinmux_set(&aml_eth_pinmux);
}

static void aml_eth_pinmux_cleanup(void)
{
    printk(KERN_INFO "****** aml_eth_pinmux_cleanup() ******\n");

    pinmux_clr(&aml_eth_pinmux);
}

static void aml_eth_init(void)
{
    aml_eth_pinmux_setup();
    aml_eth_clock_enable();
    aml_eth_reset();
}

static struct aml_eth_platdata aml_eth_pdata __initdata = {
    .pinmux_items   = aml_eth_pins,
    .pinmux_setup   = aml_eth_pinmux_setup,
    .pinmux_cleanup = aml_eth_pinmux_cleanup,
    .clock_enable   = aml_eth_clock_enable,
    .clock_disable  = aml_eth_clock_disable,
    .reset      = aml_eth_reset,
};

static void __init setup_eth_device(void)
{
    meson_eth_set_platdata(&aml_eth_pdata);
    aml_eth_init();
}
#endif

/***********************************************************************
 * Nand Section
 **********************************************************************/

#ifdef CONFIG_AM_NAND
static struct mtd_partition normal_partition_info[] = {
    {
        .name = "logo",
        .offset = 8*1024*1024,
        .size = 8*1024*1024,
    },
    {
        .name = "recovery",
        .offset = 16*1024*1024,
        .size = 8*1024*1024,
    },
    {
        .name = "boot",
        .offset = 24*1024*1024,
        .size = 8*1024*1024,
    },
    {
        .name = "system",
        .offset = 32*1024*1024,
        .size = 224*1024*1024,
    },
    {
        .name = "param",
        .offset = 256*1024*1024,
        .size = 2*1024*1024,
    },
    {
        .name = "dtv",
        .offset = 258*1024*1024,
        .size = 32*1024*1024,
    },
    {
        .name = "atv",
        .offset = 290*1024*1024,
        .size = 64*1024*1024,
    },
    {
        .name = "cache",
        .offset = 354*1024*1024,
        .size = 16*1024*1024,
    },
    {
        .name = "userdata",
        .offset=MTDPART_OFS_APPEND,
        .size=MTDPART_SIZ_FULL,
    },
};


static struct aml_nand_platform aml_nand_mid_platform[] = {
    {
        .name = NAND_NORMAL_NAME,
        .chip_enable_pad = (AML_NAND_CE0/* | (AML_NAND_CE1 << 4) | (AML_NAND_CE2 << 8) | (AML_NAND_CE3 << 12)*/),
        .ready_busy_pad = (AML_NAND_CE0 /*| (AML_NAND_CE0 << 4) | (AML_NAND_CE1 << 8) | (AML_NAND_CE1 << 12)*/),
        .platform_nand_data = {
            .chip =  {
                .nr_chips = 1,
                .nr_partitions = ARRAY_SIZE(normal_partition_info),
                .partitions = normal_partition_info,
                .options = (NAND_TIMING_MODE4 | NAND_ECC_BCH8_MODE | NAND_TWO_PLANE_MODE),
            },
        },
        .T_REA = 20,
         .T_RHOH = 15,
      }
  };

static struct aml_nand_device aml_nand_mid_device = {
    .aml_nand_platform = aml_nand_mid_platform,
    .dev_num = ARRAY_SIZE(aml_nand_mid_platform),
};

static struct resource aml_nand_resources[] = {
    {
        .start = 0xc1108600,
        .end = 0xc1108624,
        .flags = IORESOURCE_MEM,
    },
};

static struct platform_device aml_nand_device = {
    .name = "aml_nand",
    .id = 0,
    .num_resources = ARRAY_SIZE(aml_nand_resources),
    .resource = aml_nand_resources,
    .dev = {
        .platform_data = &aml_nand_mid_device,
    },
};
#endif

#if defined(CONFIG_AMLOGIC_SPI_NOR)
static struct mtd_partition spi_partition_info[] = {
    {
        .name = "bootloader",
        .offset = 0,
        .size = 0x200000,
    },

    {
        .name = "ubootenv",
        .offset = 0x3e000,
        .size = 0x2000,
    },

};

static struct flash_platform_data amlogic_spi_platform = {
    .parts = spi_partition_info,
    .nr_parts = ARRAY_SIZE(spi_partition_info),
};

static struct resource amlogic_spi_nor_resources[] = {
    {
        .start = 0xcc000000,
        .end = 0xcfffffff,
        .flags = IORESOURCE_MEM,
    },
};

static struct platform_device amlogic_spi_nor_device = {
    .name = "AMLOGIC_SPI_NOR",
    .id = -1,
    .num_resources = ARRAY_SIZE(amlogic_spi_nor_resources),
    .resource = amlogic_spi_nor_resources,
    .dev = {
        .platform_data = &amlogic_spi_platform,
    },
};
#endif




/***********************************************************************
 * Card Reader Section
 **********************************************************************/
/* WIFI ON Flag */
static int WIFI_ON;
/* BT ON Flag */
static int BT_ON;
/* WL_BT_REG_ON control function */
static void reg_on_control(int is_on)
{
    CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_1,(1<<11)); //WIFI_RST GPIO mode
    CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_0,(1<<18)); //WIFI_EN GPIO mode
    CLEAR_CBUS_REG_MASK(PREG_PAD_GPIO2_EN_N, (1<<8));//GPIOC_8 ==WIFI_EN

    if(is_on) {
        SET_CBUS_REG_MASK(PREG_PAD_GPIO2_O, (1<<8));
    } else {
        /* only pull donw reg_on pin when wifi and bt off */
        if((!WIFI_ON) && (!BT_ON)) {
            CLEAR_CBUS_REG_MASK(PREG_PAD_GPIO2_O, (1<<8));
            printk("WIFI BT Power down\n");
        }
    }
}
#ifdef CONFIG_CARDREADER
static struct resource meson_card_resource[] = {
    [0] = {
        .start = 0x1200230,   //physical address
        .end   = 0x120024c,
        .flags = 0x200,
    }
};
#if 0
static void extern_wifi_power(int is_power)
{
    WIFI_ON = is_power;
    reg_on_control(is_power);
    /*
    CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_1,(1<<11));
    CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_0,(1<<18));
    CLEAR_CBUS_REG_MASK(PREG_PAD_GPIO2_EN_N, (1<<8));
    if(is_power)
    SET_CBUS_REG_MASK(PREG_PAD_GPIO2_O, (1<<8));
    else
    CLEAR_CBUS_REG_MASK(PREG_PAD_GPIO2_O, (1<<8));
    */
}

EXPORT_SYMBOL(extern_wifi_power);
#endif
static void sdio_extern_init(void)
{
#if defined(CONFIG_BCM4329_HW_OOB) || defined(CONFIG_BCM4329_OOB_INTR_ONLY)/* Jone add */
    gpio_set_status(PAD_GPIOX_11,gpio_status_in);
    gpio_irq_set(PAD_GPIOX_11,GPIO_IRQ(4,GPIO_IRQ_RISING));

    extern_wifi_power(1);
#endif
}
static struct aml_card_info meson_card_info[] = {
    [0] = {
        .name       = "sd_card",
        .work_mode  = CARD_HW_MODE,
        .io_pad_type        = SDHC_CARD_0_5,
        .card_ins_en_reg    = CARD_GPIO_ENABLE,
        .card_ins_en_mask   = PREG_IO_29_MASK,
        .card_ins_input_reg = CARD_GPIO_INPUT,
        .card_ins_input_mask    = PREG_IO_29_MASK,
        .card_power_en_reg  = CARD_GPIO_ENABLE,
        .card_power_en_mask = PREG_IO_31_MASK,
        .card_power_output_reg  = CARD_GPIO_OUTPUT,
        .card_power_output_mask = PREG_IO_31_MASK,
        .card_power_en_lev  = 0,
        .card_wp_en_reg     = 0,
        .card_wp_en_mask    = 0,
        .card_wp_input_reg  = 0,
        .card_wp_input_mask = 0,
        .card_extern_init   = 0,
    },
    /*[1] = {
    .name       = "sdio_card",
    .work_mode  = CARD_HW_MODE,
    .io_pad_type        = SDHC_GPIOX_0_9,
    .card_ins_en_reg    = 0,
    .card_ins_en_mask   = 0,
    .card_ins_input_reg = 0,
    .card_ins_input_mask    = 0,
    .card_power_en_reg  = EGPIO_GPIOC_ENABLE,
    .card_power_en_mask = PREG_IO_7_MASK,
    .card_power_output_reg  = EGPIO_GPIOC_OUTPUT,
    .card_power_output_mask = PREG_IO_7_MASK,
    .card_power_en_lev  = 1,
    .card_wp_en_reg     = 0,
    .card_wp_en_mask    = 0,
    .card_wp_input_reg  = 0,
    .card_wp_input_mask = 0,
    .card_extern_init   = sdio_extern_init,
    },*/
};

static struct aml_card_platform meson_card_platform = {
    .card_num   = ARRAY_SIZE(meson_card_info),
    .card_info  = meson_card_info,
};

static struct platform_device meson_card_device = {
    .name   = "AMLOGIC_CARD",
    .id     = -1,
    .num_resources  = ARRAY_SIZE(meson_card_resource),
    .resource   = meson_card_resource,
    .dev = {
        .platform_data = &meson_card_platform,
    },
};

/**
 *  Some Meson6 socket board has card detect issue.
 *  Force card detect success for socket board.
 */
static int meson_mmc_detect(void)
{
    return 0;
}
#endif // CONFIG_CARDREADER

/***********************************************************************
 * MMC SD Card  Section
 **********************************************************************/
#ifdef CONFIG_MMC_AML
struct platform_device;
struct mmc_host;
struct mmc_card;
struct mmc_ios;

//return 1: no inserted  0: inserted
static int aml_sdio_detect(struct aml_sd_host * host)
{
    aml_set_reg32_mask(P_PREG_PAD_GPIO5_EN_N,1<<29);//CARD_6 input mode
    if((aml_read_reg32(P_PREG_PAD_GPIO5_I)&(1<<29)) == 0)
        return 0;
    else { //for socket card box
        return 0;
    }
    return 1; //no insert.
}

static void  cpu_sdio_pwr_prepare(unsigned port)
{
    switch(port) {
    case MESON_SDIO_PORT_A:
        aml_clr_reg32_mask(P_PREG_PAD_GPIO4_EN_N,0x30f);
        aml_clr_reg32_mask(P_PREG_PAD_GPIO4_O   ,0x30f);
        aml_clr_reg32_mask(P_PERIPHS_PIN_MUX_8,0x3f);
        break;
    case MESON_SDIO_PORT_B:
        aml_clr_reg32_mask(P_PREG_PAD_GPIO5_EN_N,0x3f<<23);
        aml_clr_reg32_mask(P_PREG_PAD_GPIO5_O   ,0x3f<<23);
        aml_clr_reg32_mask(P_PERIPHS_PIN_MUX_2,0x3f<<10);
        break;
    case MESON_SDIO_PORT_C:
        aml_clr_reg32_mask(P_PREG_PAD_GPIO3_EN_N,0xc0f);
        aml_clr_reg32_mask(P_PREG_PAD_GPIO3_O   ,0xc0f);
        aml_clr_reg32_mask(P_PERIPHS_PIN_MUX_6,(0x3f<<24));
        break;
    case MESON_SDIO_PORT_XC_A:
        break;
    case MESON_SDIO_PORT_XC_B:
        break;
    case MESON_SDIO_PORT_XC_C:
        break;
    }
}

static int cpu_sdio_init(unsigned port)
{
    switch(port) {
    case MESON_SDIO_PORT_A:
        aml_set_reg32_mask(P_PERIPHS_PIN_MUX_8,0x3d<<0);
        aml_set_reg32_mask(P_PERIPHS_PIN_MUX_8,0x1<<1);
        break;
    case MESON_SDIO_PORT_B:
        aml_set_reg32_mask(P_PERIPHS_PIN_MUX_2,0x3d<<10);
        aml_set_reg32_mask(P_PERIPHS_PIN_MUX_2,0x1<<11);
        break;
    case MESON_SDIO_PORT_C://SDIOC GPIOB_2~GPIOB_7
        aml_clr_reg32_mask(P_PERIPHS_PIN_MUX_2,(0x1f<<22));
        aml_set_reg32_mask(P_PERIPHS_PIN_MUX_6,(0x1f<<25));
        aml_set_reg32_mask(P_PERIPHS_PIN_MUX_6,(0x1<<24));
        break;
    case MESON_SDIO_PORT_XC_A:
#if 0
        //sdxc controller can't work
        aml_clr_reg32_mask(P_PERIPHS_PIN_MUX_8,(0x3f<<0));
        aml_clr_reg32_mask(P_PERIPHS_PIN_MUX_3,(0x0f<<27));
        aml_clr_reg32_mask(P_PERIPHS_PIN_MUX_7,((0x3f<<18)|(0x7<<25)));
        //aml_set_reg32_mask(P_PERIPHS_PIN_MUX_5,(0x1f<<10));//data 8 bit
        aml_set_reg32_mask(P_PERIPHS_PIN_MUX_5,(0x1b<<10));//data 4 bit
#endif
        break;
    case MESON_SDIO_PORT_XC_B:
        //sdxc controller can't work
        //aml_set_reg32_mask(P_PERIPHS_PIN_MUX_2,(0xf<<4));
        break;
    case MESON_SDIO_PORT_XC_C:
#if 0
        //sdxc controller can't work
        aml_clr_reg32_mask(P_PERIPHS_PIN_MUX_6,(0x3f<<24));
        aml_clr_reg32_mask(P_PERIPHS_PIN_MUX_2,((0x13<<22)|(0x3<<16)));
        aml_set_reg32_mask(P_PERIPHS_PIN_MUX_4,(0x1f<<26));
        printk(KERN_INFO "inand sdio xc-c init\n");
#endif
        break;
    default:
        return -1;
    }
    return 0;
}

static void aml_sdio_pwr_prepare(unsigned port)
{
    /// @todo NOT FINISH
    ///do nothing here
    cpu_sdio_pwr_prepare(port);
}

static void aml_sdio_pwr_on(unsigned port)
{
    if((aml_read_reg32(P_PREG_PAD_GPIO5_O) & (1<<31)) != 0) {
        aml_clr_reg32_mask(P_PREG_PAD_GPIO5_O,(1<<31));
        aml_clr_reg32_mask(P_PREG_PAD_GPIO5_EN_N,(1<<31));
        udelay(1000);
    }
    /// @todo NOT FINISH
}
static void aml_sdio_pwr_off(unsigned port)
{
    if((aml_read_reg32(P_PREG_PAD_GPIO5_O) & (1<<31)) == 0) {
        aml_set_reg32_mask(P_PREG_PAD_GPIO5_O,(1<<31));
        aml_clr_reg32_mask(P_PREG_PAD_GPIO5_EN_N,(1<<31));//GPIOD13
        udelay(1000);
    }
    /// @todo NOT FINISH
}
static int aml_sdio_init(struct aml_sd_host * host)
{
    //set pinumx ..
    aml_set_reg32_mask(P_PREG_PAD_GPIO5_EN_N,1<<29);//CARD_6
    cpu_sdio_init(host->sdio_port);
    host->clk = clk_get_sys("clk81",NULL);
    if(!IS_ERR(host->clk))
        host->clk_rate = clk_get_rate(host->clk);
    else
        host->clk_rate = 0;
    return 0;
}

static struct resource aml_mmc_resource[] = {
    [0] = {
        .start = 0x1200230,   //physical address
        .end   = 0x1200248,
        .flags = IORESOURCE_MEM, //0x200
    },
};

static u64 aml_mmc_device_dmamask = 0xffffffffUL;
static struct aml_mmc_platform_data aml_mmc_def_platdata = {
    .no_wprotect = 1,
    .no_detect = 0,
    .wprotect_invert = 0,
    .detect_invert = 0,
    .use_dma = 0,
    .gpio_detect=1,
    .gpio_wprotect=0,
    .ocr_avail = MMC_VDD_33_34,

    .sdio_port = MESON_SDIO_PORT_B,
    .max_width  = 4,
    .host_caps  = (MMC_CAP_4_BIT_DATA |
    MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED | MMC_CAP_NEEDS_POLL),

    .f_min = 200000,
    .f_max = 40000000,
    .clock = 300000,

    .sdio_init = aml_sdio_init,
    .sdio_detect = aml_sdio_detect,
    .sdio_pwr_prepare = aml_sdio_pwr_prepare,
    .sdio_pwr_on = aml_sdio_pwr_on,
    .sdio_pwr_off = aml_sdio_pwr_off,
};

static struct platform_device aml_mmc_device = {
    .name   = "aml_sd_mmc",
    .id     = 0,
    .num_resources  = ARRAY_SIZE(aml_mmc_resource),
    .resource   = aml_mmc_resource,
    .dev    = {
        .dma_mask   =   &aml_mmc_device_dmamask,
        .coherent_dma_mask  = 0xffffffffUL,
        .platform_data      = &aml_mmc_def_platdata,
    },
};
#endif //CONFIG_MMC_AML
/***********************************************************************
 * IO Mapping
 **********************************************************************/
/*
#define IO_CBUS_BASE        0xf1100000  ///2M
#define IO_AXI_BUS_BASE     0xf1300000  ///1M
#define IO_PL310_BASE       0xf2200000  ///4k
#define IO_PERIPH_BASE      0xf2300000  ///4k
#define IO_APB_BUS_BASE     0xf3000000  ///8k
#define IO_DOS_BUS_BASE     0xf3010000  ///64k
#define IO_AOBUS_BASE       0xf3100000  ///1M
#define IO_USB_A_BASE       0xf3240000  ///256k
#define IO_USB_B_BASE       0xf32C0000  ///256k
#define IO_WIFI_BASE        0xf3300000  ///1M
#define IO_SATA_BASE        0xf3400000  ///64k
#define IO_ETH_BASE     0xf3410000  ///64k

#define IO_SPIMEM_BASE      0xf4000000  ///64M
#define IO_A9_APB_BASE      0xf8000000  ///256k
#define IO_DEMOD_APB_BASE   0xf8044000  ///112k
#define IO_MALI_APB_BASE    0xf8060000  ///128k
#define IO_APB2_BUS_BASE    0xf8000000
#define IO_AHB_BASE     0xf9000000  ///128k
#define IO_BOOTROM_BASE     0xf9040000  ///64k
#define IO_SECBUS_BASE      0xfa000000
#define IO_EFUSE_BASE       0xfa000000  ///4k
*/
static __initdata struct map_desc meson_io_desc[] = {
    {
        .virtual    = IO_CBUS_BASE,
        .pfn        = __phys_to_pfn(IO_CBUS_PHY_BASE),
        .length     = SZ_2M,
        .type       = MT_DEVICE,
    }, {
        .virtual    = IO_AXI_BUS_BASE,
        .pfn        = __phys_to_pfn(IO_AXI_BUS_PHY_BASE),
        .length     = SZ_1M,
        .type       = MT_DEVICE,
    }, {
        .virtual    = IO_PL310_BASE,
        .pfn        = __phys_to_pfn(IO_PL310_PHY_BASE),
        .length     = SZ_4K,
        .type       = MT_DEVICE,
    }, {
        .virtual    = IO_PERIPH_BASE,
        .pfn        = __phys_to_pfn(IO_PERIPH_PHY_BASE),
        .length     = SZ_1M,
        .type       = MT_DEVICE,
    }, {
        .virtual    = IO_APB_BUS_BASE,
        .pfn        = __phys_to_pfn(IO_APB_BUS_PHY_BASE),
        .length     = SZ_1M,
        .type       = MT_DEVICE,
    }, /*{
    .virtual    = IO_DOS_BUS_BASE,
    .pfn        = __phys_to_pfn(IO_DOS_BUS_PHY_BASE),
    .length     = SZ_64K,
    .type       = MT_DEVICE,
    }, */{
        .virtual    = IO_AOBUS_BASE,
        .pfn        = __phys_to_pfn(IO_AOBUS_PHY_BASE),
        .length     = SZ_1M,
        .type       = MT_DEVICE,
    }, {
        .virtual    = IO_AHB_BUS_BASE,
        .pfn        = __phys_to_pfn(IO_AHB_BUS_PHY_BASE),
        .length     = SZ_8M,
        .type       = MT_DEVICE,
    }, {
        .virtual    = IO_SPIMEM_BASE,
        .pfn        = __phys_to_pfn(IO_SPIMEM_PHY_BASE),
        .length     = SZ_64M,
        .type       = MT_ROM,
    }, {
        .virtual    = IO_APB2_BUS_BASE,
        .pfn        = __phys_to_pfn(IO_APB2_BUS_PHY_BASE),
        .length     = SZ_512K,
        .type       = MT_DEVICE,
    }, {
        .virtual    = IO_AHB_BASE,
        .pfn        = __phys_to_pfn(IO_AHB_PHY_BASE),
        .length     = SZ_128K,
        .type       = MT_DEVICE,
    }, {
        .virtual    = IO_BOOTROM_BASE,
        .pfn        = __phys_to_pfn(IO_BOOTROM_PHY_BASE),
        .length     = SZ_64K,
        .type       = MT_DEVICE,
    }, {
        .virtual    = IO_SECBUS_BASE,
        .pfn        = __phys_to_pfn(IO_SECBUS_PHY_BASE),
        .length     = SZ_4K,
        .type       = MT_DEVICE,
    }, {
        .virtual    = IO_SECURE_BASE,
        .pfn        = __phys_to_pfn(IO_SECURE_PHY_BASE),
        .length     = SZ_16K,
        .type       = MT_DEVICE,
    }, {
        .virtual    = PAGE_ALIGN(__phys_to_virt(RESERVED_MEM_START)),
        .pfn        = __phys_to_pfn(RESERVED_MEM_START),
        .length     = RESERVED_MEM_END - RESERVED_MEM_START + 1,
        .type       = MT_MEMORY_NONCACHED,
    },
#ifdef CONFIG_MESON_SUSPEND
    {
        .virtual    = PAGE_ALIGN(__phys_to_virt(0x9ff00000)),
        .pfn        = __phys_to_pfn(0x9ff00000),
        .length     = SZ_1M,
        .type       = MT_MEMORY_NONCACHED,
    },
#endif

};

static void __init meson_map_io(void)
{
    iotable_init(meson_io_desc, ARRAY_SIZE(meson_io_desc));
}

static void __init meson_fixup(struct machine_desc *mach, struct tag *tag, char **cmdline, struct meminfo *m)
{
    struct membank *pbank;
    mach->video_start    = RESERVED_MEM_START;
    mach->video_end  = RESERVED_MEM_END;

    m->nr_banks = 0;
    pbank = &m->bank[m->nr_banks];
    pbank->start = PAGE_ALIGN(PHYS_MEM_START);
    pbank->size  = SZ_64M & PAGE_MASK;
    m->nr_banks++;
    pbank = &m->bank[m->nr_banks];
    pbank->start = PAGE_ALIGN(RESERVED_MEM_END + 1);
#ifdef CONFIG_MESON_SUSPEND
    pbank->size  = (PHYS_MEM_END-RESERVED_MEM_END-SZ_1M) & PAGE_MASK;
#else
    pbank->size  = (PHYS_MEM_END-RESERVED_MEM_END) & PAGE_MASK;
#endif
    m->nr_banks++;
}

/***********************************************************************
 *USB Setting section
 **********************************************************************/
static void set_usb_a_vbus_power(char is_power_on)
{

}
static  int __init setup_usb_devices(void)
{
    struct lm_device * usb_ld_a, *usb_ld_b, *usb_ld_c, *usb_ld_d;
    usb_ld_a = alloc_usb_lm_device(USB_PORT_IDX_A);
    usb_ld_b = alloc_usb_lm_device (USB_PORT_IDX_B);
    usb_ld_c = alloc_usb_lm_device(USB_PORT_IDX_C);
    usb_ld_d = alloc_usb_lm_device (USB_PORT_IDX_D);

    lm_device_register(usb_ld_a);
    lm_device_register(usb_ld_b);
    lm_device_register(usb_ld_c);
//  lm_device_register(usb_ld_d);
    return 0;
}

/***********************************************************************
 *WiFi power section
 **********************************************************************/
/* built-in usb wifi power ctrl, usb dongle must register NULL to power_ctrl! 1:power on  0:power off */
#ifdef CONFIG_AM_WIFI
#ifdef CONFIG_AM_WIFI_USB
static int usb_wifi_power(int is_power)
{
#if 0
    //printk(KERN_INFO "usb_wifi_power %s\n", is_power ? "On" : "Off");
    printk(KERN_INFO "usb_wifi_power %s\n", is_power ? "On" : "Off");
    CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_1,(1<<11));
    CLEAR_CBUS_REG_MASK(PERIPHS_PIN_MUX_0,(1<<18));
    CLEAR_CBUS_REG_MASK(PREG_PAD_GPIO2_EN_N, (1<<5));
    if (0)//is_power
        SET_CBUS_REG_MASK(PREG_PAD_GPIO2_O, (1<<5));
    else
        CLEAR_CBUS_REG_MASK(PREG_PAD_GPIO2_O, (1<<5));
    return 0;
#endif
}

static struct wifi_power_platform_data wifi_plat_data = {
    .usb_set_power = usb_wifi_power,
};
#elif defined(CONFIG_AM_WIFI_SD_MMC)&&defined(CONFIG_CARDREADER)
wifi_plat_data = {

};
#endif

static struct platform_device wifi_power_device = {
    .name   = "wifi_power",
    .id     = -1,
    .dev = {
        .platform_data = &wifi_plat_data,
    },
};
#endif

/***********************************************************************/
#ifdef CONFIG_EFUSE
static bool efuse_data_verify(unsigned char *usid)
{
    int len;

    len = strlen(usid);
    if((len > 0)&&(len<58) )
        return true;
    else
        return false;
}

static struct efuse_platform_data aml_efuse_plat = {
    .pos = 454,
    .count = 58,
    .data_verify = efuse_data_verify,
};

static struct platform_device aml_efuse_device = {
    .name   = "efuse",
    .id = -1,
    .dev = {
        .platform_data = &aml_efuse_plat,
    },
};

// BSP EFUSE layout setting
static efuseinfo_item_t aml_efuse_setting[] = {
    // usid layout can be defined by customer
    {
        .title = "usid",
        .id = EFUSE_USID_ID,
        .offset = 454,      // customer can modify the offset which must >= 454
        .enc_len = 58,     // customer can modify the encode length by self must <=58
        .data_len = 58, // customer can modify the data length by self must <=58
        .bch_en = 0,        // customer can modify do bch or not
        .bch_reverse = 0,
    },
    // customer also can add new EFUSE item to expand, but must be correct and bo conflict
};

static int aml_efuse_getinfoex_byID(unsigned param, efuseinfo_item_t *info)
{
    unsigned num = sizeof(aml_efuse_setting)/sizeof(efuseinfo_item_t);
    int i=0;
    int ret = -1;
    for(i=0; i<num; i++) {
        if(aml_efuse_setting[i].id == param) {
            strcpy(info->title, aml_efuse_setting[i].title);
            info->offset = aml_efuse_setting[i].offset;
            info->id = aml_efuse_setting[i].id;
            info->data_len = aml_efuse_setting[i].data_len;
            info->enc_len = aml_efuse_setting[i].enc_len;
            info->bch_en = aml_efuse_setting[i].bch_en;
            info->bch_reverse = aml_efuse_setting[i].bch_reverse;
            ret = 0;
            break;
        }
    }
    return ret;
}

static int aml_efuse_getinfoex_byPos(unsigned param, efuseinfo_item_t *info)
{
    unsigned num = sizeof(aml_efuse_setting)/sizeof(efuseinfo_item_t);
    int i=0;
    int ret = -1;
    for(i=0; i<num; i++) {
        if(aml_efuse_setting[i].offset == param) {
            strcpy(info->title, aml_efuse_setting[i].title);
            info->offset = aml_efuse_setting[i].offset;
            info->id = aml_efuse_setting[i].id;
            info->data_len = aml_efuse_setting[i].data_len;
            info->enc_len = aml_efuse_setting[i].enc_len;
            info->bch_en = aml_efuse_setting[i].bch_en;
            info->bch_reverse = aml_efuse_setting[i].bch_reverse;
            ret = 0;
            break;
        }
    }
    return ret;
}

extern pfn efuse_getinfoex;
extern pfn efuse_getinfoex_byPos;

static __init void setup_aml_efuse(void)
{
    efuse_getinfoex = aml_efuse_getinfoex_byID;
    efuse_getinfoex_byPos = aml_efuse_getinfoex_byPos;
}
#endif

#if defined(CONFIG_AML_RTC)
static struct platform_device aml_rtc_device = {
    .name        = "aml_rtc",
    .id           = -1,
};
#endif

#if defined(CONFIG_SUSPEND)
typedef struct {
    char name[32];
    unsigned bank;
    unsigned bit;
    gpio_mode_t mode;
    unsigned value;
    unsigned enable;
    unsigned keep_last;
} gpio_data_t;

#define MAX_GPIO 2

static gpio_data_t gpio_data[MAX_GPIO] = {
    // ----------------------------------- bl ----------------------------------
    {"GPIOX2 -- BL_EN",         GPIOX_bank_bit0_31(2),     GPIOX_bit_bit0_31(2),  GPIO_OUTPUT_MODE, 1, 1, 1},
    // ----------------------------------- panel ----------------------------------
    {"GPIOZ5 -- PANEL_PWR",     GPIOZ_bank_bit0_19(5),     GPIOZ_bit_bit0_19(5),  GPIO_OUTPUT_MODE, 1, 1, 1},
    // ----------------------------------- i2c ----------------------------------
    //{"GPIOZ6 -- iic",           GPIOZ_bank_bit0_19(6),     GPIOZ_bit_bit0_19(6),  GPIO_OUTPUT_MODE, 1, 1, 1},
    //{"GPIOZ7 -- iic",           GPIOZ_bank_bit0_19(7),     GPIOZ_bit_bit0_19(7),  GPIO_OUTPUT_MODE, 1, 1, 1},
};

static void save_gpio(int port)
{
    gpio_data[port].mode = get_gpio_mode(gpio_data[port].bank, gpio_data[port].bit);
    if (gpio_data[port].mode==GPIO_OUTPUT_MODE)
    {
        if (gpio_data[port].enable){
            printk("%d---change %s output %d to input\n", port, gpio_data[port].name, gpio_data[port].value);
            gpio_data[port].value = get_gpio_val(gpio_data[port].bank, gpio_data[port].bit);
            set_gpio_mode(gpio_data[port].bank, gpio_data[port].bit, GPIO_INPUT_MODE);
        } else{
            printk("%d---no change %s output %d\n", port, gpio_data[port].name, gpio_data[port].value);
        }
    } else {
        printk("%d---%s input %d\n", port, gpio_data[port].name, gpio_data[port].mode);
    }
}

static void restore_gpio(int port)
{
    if ((gpio_data[port].mode==GPIO_OUTPUT_MODE)&&(gpio_data[port].enable))
    {
        set_gpio_val(gpio_data[port].bank, gpio_data[port].bit, gpio_data[port].value);
        set_gpio_mode(gpio_data[port].bank, gpio_data[port].bit, GPIO_OUTPUT_MODE);
        printk("%d---%s output %d\n", port, gpio_data[port].name, gpio_data[port].value);
    } else {
        printk("%d---%s output/input:%d, enable:%d\n", port, gpio_data[port].name, gpio_data[port].mode, gpio_data[port].value);
    }
}

typedef struct {
    char name[32];
    unsigned reg;
    unsigned bits;
    unsigned enable;
} pinmux_data_t;


#define MAX_PINMUX 10

pinmux_data_t pinmux_data[MAX_PINMUX] = {
    {"PERIPHS_PIN_MUX_0",         0, 0xffffffff,               1},
    {"PERIPHS_PIN_MUX_1",         1, 0xffffffff,               1},
    {"PERIPHS_PIN_MUX_2",         2, 0xffffffff,               1},
    {"PERIPHS_PIN_MUX_3",         3, 0xffffffff,               1},
    {"PERIPHS_PIN_MUX_4",         4, 0xffffffff,               1},
    {"PERIPHS_PIN_MUX_5",         5, 0xffffffff,               1},
    {"PERIPHS_PIN_MUX_6",         6, 0xffffffff,               1},
    {"PERIPHS_PIN_MUX_7",         7, 0xffffffff,               1},
    {"PERIPHS_PIN_MUX_8",         8, 0xffffffff,               1},
    {"PERIPHS_PIN_MUX_9",         9, 0xffffffff,               1},
};
#define MAX_INPUT_MODE 9
pinmux_data_t gpio_inputmode_data[MAX_INPUT_MODE] = {
    {"LCDGPIOA",         P_PREG_PAD_GPIO0_EN_N, 0x3fffffff,                 1},
    {"LCDGPIOB",         P_PREG_PAD_GPIO1_EN_N, 0x00ffffff,                 1},
    {"GPIOX0_12",        P_PREG_PAD_GPIO4_EN_N, 0x00001fff,                 1},
    {"BOOT0_17",         P_PREG_PAD_GPIO3_EN_N, 0x0003ffff,                 1},
    {"GPIOZ0_19",        P_PREG_PAD_GPIO6_EN_N, 0x000fffff,                 1},
    {"GPIOY0_27",        P_PREG_PAD_GPIO2_EN_N, 0x0fffffff,                 1},
    {"GPIOW0_19",        P_PREG_PAD_GPIO5_EN_N, 0x000fffff,                 1},
    {"CRAD0_8",          P_PREG_PAD_GPIO5_EN_N, 0xff000000,                 1},
    {"GPIOP6",           P_PREG_PAD_GPIO1_EN_N, 0x40000000,                 1},
};

#define MAX_RESUME_OUTPUT_MODE 1
pinmux_data_t gpio_outputmode_data[MAX_RESUME_OUTPUT_MODE] = {
    {"GPIOZ6_7",        P_PREG_PAD_GPIO6_EN_N, 0x000fff3f,                 1},
};
static unsigned pinmux_backup[10];

#define MAX_PADPULL 7

pinmux_data_t pad_pull[MAX_PADPULL] = {
    {"PAD_PULL_UP_REG0",         P_PAD_PULL_UP_REG0, 0xffffffff,               1},
    {"PAD_PULL_UP_REG1",         P_PAD_PULL_UP_REG1, 0x00000000,               1},
    {"PAD_PULL_UP_REG2",         P_PAD_PULL_UP_REG2, 0xffffffff,               1},
    {"PAD_PULL_UP_REG3",         P_PAD_PULL_UP_REG3, 0xffffffff,               1},
    {"PAD_PULL_UP_REG4",         P_PAD_PULL_UP_REG4, 0xffffffff,               1},
    {"PAD_PULL_UP_REG5",         P_PAD_PULL_UP_REG5, 0xffffffff,               1},
    {"PAD_PULL_UP_REG6",         P_PAD_PULL_UP_REG6, 0xffffffff,               1},
};

static unsigned pad_pull_backup[MAX_PADPULL];

int  clear_mio_mux(unsigned mux_index, unsigned mux_mask)
{
    unsigned mux_reg[] = {PERIPHS_PIN_MUX_0, PERIPHS_PIN_MUX_1, PERIPHS_PIN_MUX_2,PERIPHS_PIN_MUX_3,
        PERIPHS_PIN_MUX_4,PERIPHS_PIN_MUX_5,PERIPHS_PIN_MUX_6,PERIPHS_PIN_MUX_7,PERIPHS_PIN_MUX_8,
        PERIPHS_PIN_MUX_9,PERIPHS_PIN_MUX_10,PERIPHS_PIN_MUX_11,PERIPHS_PIN_MUX_12};
    if (mux_index < 13) {
        CLEAR_CBUS_REG_MASK(mux_reg[mux_index], mux_mask);
        return 0;
    }
    return -1;
}

static void save_pinmux(void)
{
    int i;
    for (i=0;i<10;i++){
        pinmux_backup[i] = READ_CBUS_REG(PERIPHS_PIN_MUX_0+i);
        printk("--PERIPHS_PIN_MUX_%d = %x\n", i,pinmux_backup[i]);
    }
    for (i=0;i<MAX_PADPULL;i++){
        pad_pull_backup[i] = aml_read_reg32(pad_pull[i].reg);
        printk("--PAD_PULL_UP_REG%d = %x\n", i,pad_pull_backup[i]);
    }
    for (i=0;i<MAX_PINMUX;i++){
        if (pinmux_data[i].enable){
            printk("%s %x\n", pinmux_data[i].name, pinmux_data[i].bits);
            clear_mio_mux(pinmux_data[i].reg, pinmux_data[i].bits);
        }
    }
    for (i=0;i<MAX_PADPULL;i++){
        if (pad_pull[i].enable){
            printk("%s %x\n", pad_pull[i].name, pad_pull[i].bits);
            aml_write_reg32(pad_pull[i].reg, aml_read_reg32(pad_pull[i].reg) | pad_pull[i].bits);
        }
    }
    for (i=0;i<MAX_INPUT_MODE;i++){
        if (gpio_inputmode_data[i].enable){
            printk("%s %x\n", gpio_inputmode_data[i].name, gpio_inputmode_data[i].bits);
            aml_write_reg32(gpio_inputmode_data[i].reg, aml_read_reg32(gpio_inputmode_data[i].reg) | gpio_inputmode_data[i].bits);
        }
    }
}

static void restore_pinmux(void)
{
    int i;
    /*for (i=0;i<MAX_RESUME_OUTPUT_MODE;i++){
        if (gpio_outputmode_data[i].enable){
            printk("%s %x\n", gpio_outputmode_data[i].name, gpio_outputmode_data[i].bits);
            aml_write_reg32(gpio_outputmode_data[i].reg, aml_read_reg32(gpio_outputmode_data[i].reg) & gpio_outputmode_data[i].bits);
        }
    }
    set_gpio_val(GPIOZ_bank_bit0_19(6), GPIOZ_bit_bit0_19(6), 1);
    set_gpio_mode(GPIOZ_bank_bit0_19(6), GPIOZ_bit_bit0_19(6), GPIO_OUTPUT_MODE);
    set_gpio_val(GPIOZ_bank_bit0_19(7), GPIOZ_bit_bit0_19(7), 1);
    set_gpio_mode(GPIOZ_bank_bit0_19(7), GPIOZ_bit_bit0_19(7), GPIO_OUTPUT_MODE);*/
    aml_clr_reg32_mask(P_PREG_PAD_GPIO5_EN_N,(1<<19));
    aml_set_reg32_mask(P_PREG_PAD_GPIO5_O,(1<<19));
    for (i=0;i<10;i++){
    	 printk("++PERIPHS_PIN_MUX_%d = %x\n", i,pinmux_backup[i]);
         WRITE_CBUS_REG(PERIPHS_PIN_MUX_0+i, pinmux_backup[i]);
     }
     for (i=0;i<MAX_PADPULL;i++){
    	 printk("++PAD_PULL_UP_REG%d = %x\n", i,pad_pull_backup[i]);
         aml_write_reg32(pad_pull[i].reg, pad_pull_backup[i]);
     }
}

static void m6tvref_set_vccx2(int power_on)
{
    int i = 0;
    if (power_on) {
        printk(KERN_INFO "%s() Power ON\n", __FUNCTION__);
        aml_clr_reg32_mask(P_PREG_PAD_GPIO0_EN_N,(1<<26));
        aml_clr_reg32_mask(P_PREG_PAD_GPIO0_O,(1<<26));
    } else {
        printk(KERN_INFO "%s() Power OFF\n", __FUNCTION__);
        aml_clr_reg32_mask(P_PREG_PAD_GPIO0_EN_N,(1<<26));
        aml_set_reg32_mask(P_PREG_PAD_GPIO0_O,(1<<26));
    }
}

static void m6tvref_set_pinmux(int power_on)
{
    int i = 0;
    if (power_on) {
        restore_pinmux();
        for (i=0;i<MAX_GPIO;i++)
            restore_gpio(i);
        printk(KERN_INFO "%s() Power ON\n", __FUNCTION__);
    } else {
        save_pinmux();
        for (i=0;i<MAX_GPIO;i++)
            save_gpio(i);
        printk(KERN_INFO "%s() Power OFF\n", __FUNCTION__);
    }
}

static struct meson_pm_config aml_pm_pdata = {
    .pctl_reg_base = (void *)IO_APB_BUS_BASE,
    .mmc_reg_base = (void *)APB_REG_ADDR(0x1000),
    .hiu_reg_base = (void *)CBUS_REG_ADDR(0x1000),
    .power_key = (1<<8),
    .ddr_clk = 0x00110820,
    .sleepcount = 128,
    .set_vccx2 = m6tvref_set_vccx2,
    .core_voltage_adjust = 7,  //5,8
    .set_pinmux = m6tvref_set_pinmux,
};

static struct platform_device aml_pm_device = {
    .name       = "pm-meson",
    .dev = {
        .platform_data  = &aml_pm_pdata,
    },
    .id     = -1,
 };
#endif /* CONFIG_SUSPEND */

/***********************************************************************
 * Meson CS DCDC section
 **********************************************************************/
#ifdef CONFIG_MESON_CS_DCDC_REGULATOR
#include <linux/regulator/meson_cs_dcdc_regulator.h>
#include <linux/regulator/machine.h>
static struct regulator_consumer_supply vcck_data[] = {
    {
        .supply = "vcck-armcore",
    },
};

static struct regulator_init_data vcck_init_data = {
    .constraints = { /* VCCK default 1.2V */
        .name = "vcck",
        .min_uV =  1045000,
        .max_uV =  1370000,
        .valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
    },
    .num_consumer_supplies = ARRAY_SIZE(vcck_data),
     .consumer_supplies = vcck_data,
  };

/*
static unsigned int vcck_pwm_table[MESON_CS_MAX_STEPS] = {
	0x060020, 0x060020, 0x060020, 0x060020, 
	0x100016, 0x100016, 0x100016, 0x100016, 
	0x18000e, 0x18000e, 0x18000e, 0x18000e, 
	0x200006, 0x200006, 0x200006, 0x200006, 
};*/
static unsigned int vcck_pwm_table[MESON_CS_MAX_STEPS] = {
	0x00000020, 0x00000020, 0x0004001c, 0x0004001c,
	0x0006001a, 0x0006001a, 0x0006001a, 0x0006001a,
	0x00100010, 0x00100010, 0x00100010, 0x00100010,
    0x0016000a, 0x0016000a, 0x001e0002, 0x001e0002,
};

static int get_voltage() {
//    printk("***vcck: get_voltage");
    int i;
    unsigned int reg = aml_read_reg32(P_PWM_PWM_B);
    for(i=0; i<MESON_CS_MAX_STEPS; i++) {
        if(reg == vcck_pwm_table[i])
	     break;
    }
    if(i >= MESON_CS_MAX_STEPS)
        return -1;
    else 
        return i;
}

static int set_voltage(unsigned int level) {
    //printk("Kave test***vcck: set_voltage %d\n",level);
    aml_write_reg32(P_PWM_PWM_B, vcck_pwm_table[level]);	
    //printk("set_voltage 0x%08x\n",aml_read_reg32(P_PWM_PWM_B));
	/*printk("pinmux1=0x%x,pinmux2=0x%x,pinmux7=0x%x,pinmux9=0x%x\n",aml_read_reg32(P_PERIPHS_PIN_MUX_1),
		aml_read_reg32(P_PERIPHS_PIN_MUX_2),
		aml_read_reg32(P_PERIPHS_PIN_MUX_7),
		aml_read_reg32(P_PERIPHS_PIN_MUX_9));
	*/
}

#define PWM_PRE_DIV 0 //pwm_freq = 24M / (pre_div + 1) / PWM_MAX
static void vcck_pwm_init(void) {
    printk("vcck: vcck_pwm_init\n");
    //enable pwm clk & pwm output
    aml_write_reg32(P_PWM_MISC_REG_AB, (aml_read_reg32(P_PWM_MISC_REG_AB) & ~(0x7f << 16)) | ((1 << 23) | (PWM_PRE_DIV << 16) | (1 << 1)));
    aml_write_reg32(P_PWM_PWM_B, vcck_pwm_table[0]);
    //enable pwm_B pinmux 
    aml_write_reg32(P_PERIPHS_PIN_MUX_2, aml_read_reg32(P_PERIPHS_PIN_MUX_2) | (1 << 1));
}
early_initcall(vcck_pwm_init);

/*
PWM	        Voltage
0x00000020	1.366
0x0002001e	1.347
0x0004001c	1.328
0x0006001a	1.31
0x00080018	1.291
0x000a0016	1.272
0x000c0014	1.253
0x000e0012	1.235
0x00100010	1.216
0x0012000e	1.197
0x0014000c	1.178
0x0016000a	1.159
0x00180008	1.141
0x001a0006	1.122
0x001c0004	1.103
0x001e0002	1.084
0xffff0000	1.056

default	1.275

*/
static struct meson_cs_pdata_t vcck_pdata = {
	.meson_cs_init_data = &vcck_init_data,
	.voltage_step_table = {
		1370000, 1348000, 1327000, 1301000,
		1283000, 1261000, 1240000, 1218000,
		1193000, 1175000, 1153000, 1132000,
		1110000, 1088000, 1067000, 1045000,
	},
	.default_uV = 1370000,
	.set_voltage = set_voltage,
	.get_voltage = get_voltage,
};

static struct platform_device meson_cs_dcdc_regulator_device = {
    .name = "meson-cs-regulator",
    .dev = {
        .platform_data = &vcck_pdata,
    }
};
#endif
/***********************************************************************
 * Meson CPUFREQ section
 **********************************************************************/
#ifdef CONFIG_CPU_FREQ
#include <linux/cpufreq.h>
#include <plat/cpufreq.h>

#ifdef CONFIG_MESON_CS_DCDC_REGULATOR
#include <mach/voltage.h>
static struct regulator *vcck;
static struct meson_cpufreq_config cpufreq_info;

static unsigned int vcck_cur_max_freq()
{
    return meson_vcck_cur_max_freq(vcck, meson_vcck_opp_table, meson_vcck_opp_table_size);
}

static int vcck_scale(unsigned int frequency)
{
    return meson_vcck_scale(vcck, meson_vcck_opp_table, meson_vcck_opp_table_size, frequency);
}

static int vcck_regulator_init(void)
{
    vcck = regulator_get(NULL, vcck_data[0].supply);
    if (WARN(IS_ERR(vcck), "Unable to obtain voltage regulator for vcck;"
             " voltage scaling unsupported\n")) {
        return PTR_ERR(vcck);
    }

    return 0;
}

static struct meson_cpufreq_config cpufreq_info = {
    .freq_table = NULL,
    .init = vcck_regulator_init,
    .cur_volt_max_freq = vcck_cur_max_freq,
    .voltage_scale = vcck_scale,
};
#endif //CONFIG_MESON_CS_DCDC_REGULATOR


static struct platform_device meson_cpufreq_device = {
    .name   = "cpufreq-meson",
    .dev = {
#ifdef CONFIG_MESON_CS_DCDC_REGULATOR
        .platform_data = &cpufreq_info,
#else
        .platform_data = NULL,
#endif
    },
    .id = -1,
 };
#endif //CONFIG_CPU_FREQ

#ifdef CONFIG_SARADC_AM
#include <linux/saradc.h>
static struct platform_device saradc_device = {
    .name = "saradc",
    .id = 0,
    .dev = {
        .platform_data = NULL,
    },
};
#endif

#if defined(CONFIG_ADC_KEYPADS_AM)||defined(CONFIG_ADC_KEYPADS_AM_MODULE)
#include <linux/input.h>
#include <linux/adc_keypad.h>

static struct adc_key adc_kp_key[] = {
    {KEY_FIND,      "vol+",     CHAN_0, 0,  60},
    {KEY_MENU,      "menu",     CHAN_0, 253,    60},
    {KEY_ENTER,     "enter",    CHAN_0, 506,    60},
    {KEY_BACK,      "back",     CHAN_0, 763,    60},
    {KEY_CUT,       "ch-",      CHAN_1, 0,  60},
    {KEY_MENU,      "vol-",     CHAN_1, 763,    60},

};

static struct adc_kp_platform_data adc_kp_pdata = {
    .key = &adc_kp_key[0],
    .key_num = ARRAY_SIZE(adc_kp_key),
};

static struct platform_device adc_kp_device = {
    .name = "m1-adckp",
    .id = 0,
    .num_resources = 0,
    .resource = NULL,
    .dev = {
        .platform_data = &adc_kp_pdata,
    }
};
#endif

/***********************************************************************
 * Power Key Section
 **********************************************************************/


#if defined(CONFIG_KEY_INPUT_CUSTOM_AM) || defined(CONFIG_KEY_INPUT_CUSTOM_AM_MODULE)
#include <linux/input.h>
#include <linux/input/key_input.h>

#define GPIO_PENIRQ ((GPIOAO_bank_bit0_11(11)<<16)|GPIOAO_bank_bit0_11(11))

static int _key_code_list[] = {
    KEY_POWER,
    KEY_UP,
    KEY_UP,
};

static inline int key_input_init_func(void)
{
    //Set GPIO input mode
    gpio_direction_input(GPIO_PENIRQ);
    //Set GPIO int edge mode: falling
    gpio_enable_edge_int(gpio_to_idx(GPIO_PENIRQ), 1, 0);
    return 0;
}
static inline int key_scan(void *key_state_list)
{
    int ret = 0;
    int mode = 0;

    mode = gpio_get_edge_mode(0);
    if (mode) { //Key press down
        //Set GPIO int edge mode: rising, that means it's time to wait for key up.
        gpio_enable_edge_int(gpio_to_idx(GPIO_PENIRQ), 0, 0);
        ret = 0;
    } else { //Key press up
        //Set GPIO int edge mode: falling, that means it's time to wait for key down.
        gpio_enable_edge_int(gpio_to_idx(GPIO_PENIRQ), 1, 0);
        ret = 1;
    }

    return ret;
}

static struct key_input_platform_data key_input_pdata = {
    .scan_period = 20,
    .fuzz_time = 60,
    .key_code_list = &_key_code_list[0],
    .key_num = ARRAY_SIZE(_key_code_list),
    .scan_func = key_scan,
    .init_func = key_input_init_func,
    .config = 0,
};

static struct platform_device input_device_key = {
    .name = "meson-keyinput",
    .id = 0,
    .num_resources = 0,
    .resource = NULL,
    .dev = {
        .platform_data = &key_input_pdata,
    }
};
#endif



/***********************************************************************
 * Audio section
 **********************************************************************/
static struct resource aml_m6_audio_resource[] = {
    [0] =   {
        .start      = 0,
        .end        = 0,
        .flags      = IORESOURCE_MEM,
    },
};

static struct platform_device aml_audio = {
    .name       = "aml-audio",
    .id         = 0,
};

static struct platform_device aml_audio_dai = {
    .name       = "aml-dai",
    .id         = 0,
};

#if defined(CONFIG_SND_SOC_DUMMY_CODEC)
static pinmux_item_t dummy_codec_pinmux[] = {
    /* I2S_MCLK I2S_BCLK I2S_LRCLK I2S_DOUT */
    {
        .reg = PINMUX_REG(9),
        .setmask = (1 << 7) | (1 << 5) | (1 << 9) | (1 << 4),
        .clrmask = (7 << 19) | (7 << 1) | (3 << 10) | (1 << 6),
    },
    {
        .reg = PINMUX_REG(8),
        .clrmask = (0x7f << 24),
    },
    /* spdif out from GPIOC_9 */
    {
        .reg = PINMUX_REG(3),
        .setmask = (1<<24),
    },
    /* mask spdif out from GPIOE_8 */
    {
        .reg = PINMUX_REG(9),
        .clrmask = (1<<0),
    },
    PINMUX_END_ITEM
};

static pinmux_set_t dummy_codec_pinmux_set = {
    .chip_select = NULL,
    .pinmux = &dummy_codec_pinmux[0],
};

static void dummy_codec_device_init(void)
{
    /* audio pinmux */
    pinmux_set(&dummy_codec_pinmux_set);
}

static void dummy_codec_device_deinit(void)
{
    pinmux_clr(&dummy_codec_pinmux_set);
}


static struct dummy_codec_platform_data dummy_codec_pdata = {
    .device_init    = dummy_codec_device_init,
    .device_uninit  = dummy_codec_device_deinit,
};

static struct platform_device aml_dummy_codec_audio = {
    .name       = "aml_dummy_codec_audio",
    .id         = 0,
    .resource   = aml_m6_audio_resource,
    .num_resources  = ARRAY_SIZE(aml_m6_audio_resource),
    .dev = {
        .platform_data = &dummy_codec_pdata,
    },
};

static struct platform_device aml_dummy_codec = {
    .name       = "dummy_codec",
    .id     = 0,
};
#endif

#if defined(CONFIG_SND_AML_M6TV_AUDIO_CODEC)
static pinmux_item_t m6tv_audio_codec_pinmux[] = {
    /* I2S_MCLK I2S_BCLK I2S_LRCLK I2S_DOUT */
    {
        .reg = PINMUX_REG(8),
        .setmask = (1 << 26) | (1 << 25) | (1 << 23) | (1 << 21),
        .clrmask = (1 << 12) | (1 << 18) | (1 << 17) | (1 << 22)|(1 << 24),
    },
    /* mute gpio pinmux clear */
    {
        .reg = PINMUX_REG(1),
        .clrmask = (1 << 0),
    },
    {
        .reg = PINMUX_REG(3),
        .clrmask = (1 << 22),
    },
    /* spdif out from GPIOC_9 */
    {
        .reg = PINMUX_REG(3),
        .setmask = (1<<24),
    },
    /* mask spdif out from GPIOE_8 */
    {
        .reg = PINMUX_REG(9),
        .clrmask = (1<<0),
    },
    /* amplifier reset pinmux clear */
    {
    	.reg = PINMUX_REG(3),
		.clrmask = (24<<0),	
    },
    PINMUX_END_ITEM
};

static pinmux_set_t m6tv_audio_codec_pinmux_set = {
    .chip_select = NULL,
    .pinmux = &m6tv_audio_codec_pinmux[0],
};

static void m6tv_audio_codec_device_init(void)
{
    /*  audio MUTE pin goio setting,by default  unmute< pull high >  */
    WRITE_CBUS_REG_BITS(0x201b,0,19,1);
    WRITE_CBUS_REG_BITS(0x201c,1,19,1);
    pinmux_set(&m6tv_audio_codec_pinmux_set);
#ifdef CONFIG_SND_SOC_STA380
	/* amplifier reset */
	WRITE_CBUS_REG_BITS(0x2008,0,18,1);
    WRITE_CBUS_REG_BITS(0x2009,0,18,1);
	msleep(5);
	WRITE_CBUS_REG_BITS(0x2009,1,18,1);
#endif
#ifdef CONFIG_SND_SOC_TAS5711
	/* amplifier reset */
	WRITE_CBUS_REG_BITS(0x2008,0,18,1);
    WRITE_CBUS_REG_BITS(0x2009,0,18,1);
	msleep(1);
	WRITE_CBUS_REG_BITS(0x2009,1,18,1);
#endif
}

static void m6tv_audio_codec_device_deinit(void)
{
    pinmux_clr(&m6tv_audio_codec_pinmux_set);
}


static struct m6tv_audio_codec_platform_data m6tv_audio_codec_pdata = {
    .device_init    = m6tv_audio_codec_device_init,
    .device_uninit  = m6tv_audio_codec_device_deinit,
};

static struct platform_device aml_m6tv_audio = {
    .name       = "aml_m6tv_audio",
    .id         = 0,
    .resource   = aml_m6_audio_resource,
    .num_resources  = ARRAY_SIZE(aml_m6_audio_resource),
    .dev = {
        .platform_data = &m6tv_audio_codec_pdata,
    },
};
#ifdef CONFIG_SND_AML_M6TV_SYNOPSYS9629_CODEC
static struct platform_device aml_syno9629_codec = {
    .name       = "syno9629",
    .id     = 0,
};
#endif
#endif

#if defined(CONFIG_AM_DVB)
static struct resource amlogic_dvb_fe_resource[]  = {
#if (defined CONFIG_AM_MXL101)
    [0] = {
        .start = 2,                                 //DTV demod: M6=0, MXL101=2
        .end   = 2,
        .flags = IORESOURCE_MEM,
        .name  = "dtv_demod0"
    },
    [1] = {
        .start = 0,                                 //i2c adapter id
        .end   = 0,
        .flags = IORESOURCE_MEM,
        .name  = "dtv_demod0_i2c_adap_id"
    },
    [2] = {
        .start = 0x60,                              //i2c address
        .end   = 0x60,
        .flags = IORESOURCE_MEM,
        .name  = "dtv_demod0_i2c_addr"
    },
    [3] = {
        .start = 0,                                 //reset value
        .end   = 0,
        .flags = IORESOURCE_MEM,
        .name  = "dtv_demod0_reset_value"
    },
    [4] = {
        .start = 0,                                 //reset pin
        .end   = 0,
        .flags = IORESOURCE_MEM,
        .name  = "dtv_demod0_reset_gpio"
    },
    [5] = {
        .start = 0,
        .end   = 0,
        .flags = IORESOURCE_MEM,
        .name  = "fe0_dtv_demod"
    },
    [6] = {
        .start = 2,
        .end   = 2,
        .flags = IORESOURCE_MEM,
        .name  = "fe0_ts"
    },
    [7] = {
        .start = 0,
        .end   = 0,
        .flags = IORESOURCE_MEM,
        .name  = "fe0_dev"
    },
#elif (defined CONFIG_AM_M6_DEMOD)
    [0] = {
        .start = 0,                                 //DTV demod: M6=0, MXL101=2
        .end   = 0,
        .flags = IORESOURCE_MEM,
        .name  = "dtv_demod0"
    },
    [1] = {
        .start = 0,                                 //i2c adapter id
        .end   = 0,
        .flags = IORESOURCE_MEM,
        .name  = "dtv_demod0_i2c_adap_id"
    },
    [2] = {
        .start = 0x60,                              //i2c address
        .end   = 0x60,
        .flags = IORESOURCE_MEM,
        .name  = "dtv_demod0_i2c_addr"
    },
    [3] = {
        .start = 0,                                 //reset value
        .end   = 0,
        .flags = IORESOURCE_MEM,
        .name  = "dtv_demod0_reset_value"
    },
    [4] = {
        .start = 0,                                 //reset pin
        .end   = 0,
        .flags = IORESOURCE_MEM,
        .name  = "dtv_demod0_reset_gpio"
    },
    [5] = {
        .start = 0,
        .end   = 0,
        .flags = IORESOURCE_MEM,
        .name  = "fe0_dtv_demod"
    },
    [6] = {
        .start = 2,
        .end   = 2,
        .flags = IORESOURCE_MEM,
        .name  = "fe0_ts"
    },
    [7] = {
        .start = 0,
        .end   = 0,
        .flags = IORESOURCE_MEM,
        .name  = "fe0_dev"
        
    },
    [8] = {
		.start = DEMODBUF_ADDR_START,			  //frontend  64m
		.end   = DEMODBUF_ADDR_END,
		.flags = IORESOURCE_MEM,
		.name  = "fe0_mem"
	},
#endif

#if (defined CONFIG_AM_SI2176)
    [10] = {
        .start = 1,                                 //DTV demod: M1=0, MXL101=2
        .end   = 1,
        .flags = IORESOURCE_MEM,
        .name  = "tuner0"
    },
    [11] = {
        .start = 0,                                 //i2c adapter id
        .end   = 0,
        .flags = IORESOURCE_MEM,
        .name  = "tuner0_i2c_adap_id"
    },
    [12] = {
        .start = 0x60,                              //i2c address
        .end   = 0x60,
        .flags = IORESOURCE_MEM,
        .name  = "tuner0_i2c_addr"
    },
    [13] = {
        .start = 0,                                 //reset value
        .end   = 0,
        .flags = IORESOURCE_MEM,
        .name  = "tuner0_reset_value"
    },
    [14] = {
        .start = PAD_GPIOX_8,                                 //reset pin
        .end   = PAD_GPIOX_8,
        .flags = IORESOURCE_MEM,
        .name  = "tuner0_reset_gpio"
    },
    [15] = {
        .start = 0,
        .end   = 0,
        .flags = IORESOURCE_MEM,
        .name  = "fe0_tuner"
    },
    [16] = {
        .start = 1,                                 //DTV demod: M1=0, MXL101=2
        .end   = 1,
        .flags = IORESOURCE_MEM,
        .name  = "atv_demod0"
    },
    [17] = {
        .start = 0,
        .end   = 0,
        .flags = IORESOURCE_MEM,
        .name  = "fe0_atv_demod"
    },
#elif (defined CONFIG_AM_SI2196)
    [10] = {
        .start = 2,                                 //DTV demod: M1=0, MXL101=2
        .end   = 2,
        .flags = IORESOURCE_MEM,
        .name  = "tuner0"
    },
    [11] = {
        .start = 0,                                 //i2c adapter id
        .end   = 0,
        .flags = IORESOURCE_MEM,
        .name  = "tuner0_i2c_adap_id"
    },
    [12] = {
        .start = 0x60,                              //i2c address
        .end   = 0x60,
        .flags = IORESOURCE_MEM,
        .name  = "tuner0_i2c_addr"
    },
    [13] = {
        .start = 0,                                 //reset value
        .end   = 0,
        .flags = IORESOURCE_MEM,
        .name  = "tuner0_reset_value"
    },
    [14] = {
        .start = GPIOX_bank_bit0_31(8),                                 //reset pin
        .end   = GPIOX_bank_bit0_31(8),
        .flags = IORESOURCE_MEM,
        .name  = "tuner0_reset_gpio"
    },
    [15] = {
        .start = 0,
        .end   = 0,
        .flags = IORESOURCE_MEM,
        .name  = "fe0_tuner"
    },
    [16] = {
        .start = 2,                                 //DTV demod: M1=0, MXL101=2
        .end   = 2,
        .flags = IORESOURCE_MEM,
        .name  = "atv_demod0"
    },
    [17] = {
        .start = 0,
        .end   = 0,
        .flags = IORESOURCE_MEM,
        .name  = "fe0_atv_demod"
    },
    
#elif (defined CONFIG_AM_CTC703)  
    [10] = {
        .start = 5,                                 //DTV demod: M1=0, MXL101=2
        .end   = 5,
        .flags = IORESOURCE_MEM,
        .name  = "tuner0"
    },
    [11] = {
        .start = 0,                                 //i2c adapter id
        .end   = 0,
        .flags = IORESOURCE_MEM,
        .name  = "tuner0_i2c_adap_id"
    },
    [12] = {
        .start = 0x61,                             //i2c address
        .end   = 0x61,
        .flags = IORESOURCE_MEM,
        .name  = "tuner0_i2c_addr"
    },
    [13] = {
        .start = 0,                                 //reset value
        .end   = 0,
        .flags = IORESOURCE_MEM,
        .name  = "tuner0_reset_value"
    },
    [14] = {
        .start = PAD_GPIOX_8,                                 //reset pin
        .end   =PAD_GPIOX_8,
        .flags = IORESOURCE_MEM,
        .name  = "tuner0_reset_gpio"
    },
    [15] = {
        .start = 0,
        .end   = 0,
        .flags = IORESOURCE_MEM,
        .name  = "fe0_tuner"
    },
    [16] = {
        .start = 5,                                 //ATV demod: M1=0, MXL101=2
        .end   = 5,
        .flags = IORESOURCE_MEM,
        .name  = "atv_demod0"
    },
    [17] = {
        .start = 0,
        .end   = 0,
        .flags = IORESOURCE_MEM,
        .name  = "fe0_atv_demod"
    }
 #endif

};

static struct platform_device amlogic_dvb_fe_device = {
    .name             = "amlogic-dvb-fe",
    .id               = -1,
    .num_resources    = ARRAY_SIZE(amlogic_dvb_fe_resource),
    .resource         = amlogic_dvb_fe_resource,
};
static struct resource amlfe_resource[]  = {

    [0] = {
        .start = 0,                    //frontend  i2c adapter id
        .end   = 0,
        .flags = IORESOURCE_MEM,
        .name  = "frontend0_i2c"
    },
    [1] = {
        .start = 0xC0,                     //frontend  tuner address
        .end   = 0xC0,
        .flags = IORESOURCE_MEM,
        .name  = "frontend0_tuner_addr"
    },
    [2] = {
        .start = 4,           //frontend   mode 0-dvbc 1-dvbt 2-isdbt 3-dtmb,4-atsc
        .end   = 4,
        .flags = IORESOURCE_MEM,
        .name  = "frontend0_mode"
    },
    [3] = {
        .start = 7,           //frontend  tuner 0-NULL, 1-DCT7070, 2-Maxliner, 3-FJ2207, 4-TD1316
        .end   = 7,
        .flags = IORESOURCE_MEM,
        .name  = "frontend0_tuner"
    },
};

static  struct platform_device amlfe_device = {
    .name       = "amlfe",
    .id     = -1,
    .num_resources  = ARRAY_SIZE(amlfe_resource),
    .resource   = amlfe_resource,
};



static struct resource amlogic_dvb_resource[]  = {
    [0] = {
        .start = INT_DEMUX,           //demux 0 irq
        .end   = INT_DEMUX,
        .flags = IORESOURCE_IRQ,
        .name  = "demux0_irq"
    },
    [1] = {
        .start = INT_DEMUX_1,            //demux 1 irq
        .end   = INT_DEMUX_1,
        .flags = IORESOURCE_IRQ,
        .name  = "demux1_irq"
    },
    [2] = {
        .start = INT_DEMUX_2,            //demux 2 irq
        .end   = INT_DEMUX_2,
        .flags = IORESOURCE_IRQ,
        .name  = "demux2_irq"
    },
    [3] = {
        .start = INT_ASYNC_FIFO_FLUSH,           //dvr 0 irq
        .end   = INT_ASYNC_FIFO_FLUSH,
        .flags = IORESOURCE_IRQ,
        .name  = "dvr0_irq"
    },
    [4] = {
        .start = INT_ASYNC_FIFO2_FLUSH,      //dvr 1 irq
        .end   = INT_ASYNC_FIFO2_FLUSH,
        .flags = IORESOURCE_IRQ,
        .name  = "dvr1_irq"
    },
};

static struct platform_device amlogic_dvb_device = {
    .name       = "amlogic-dvb",
    .id     = -1,
    .num_resources  = ARRAY_SIZE(amlogic_dvb_resource),
    .resource   = amlogic_dvb_resource,
};

static int m6tvref_dvb_init(void)
{
    {
//#define FEC_B
        static pinmux_item_t fec_pins[] = {
            {/*fec mode*/
                .reg = PINMUX_REG(0),
                .clrmask = 1 << 6
            },
#if FEC_B   /*FEC_B*/
            {
                .reg = PINMUX_REG(6),
                .setmask = 0x1f << 19
            },
            {
                .reg = PINMUX_REG(3),
                .clrmask = 1 << 5
            },
#else       /*FEC_A*/
            {
                .reg = PINMUX_REG(3),
                .setmask = 0x3f
            },
            {
                .reg = PINMUX_REG(6),
                .clrmask = 0x1f << 19
            },
#endif
            PINMUX_END_ITEM
        };
        static pinmux_set_t fec_pinmux_set = {
            .chip_select = NULL,
            .pinmux = &fec_pins[0]
        };
        pinmux_set(&fec_pinmux_set);
    }

    {
        static pinmux_item_t i2c_pins[] = {
            {/*sw i2c*/
                .reg = PINMUX_REG(5),
                .clrmask = 0xf << 28
            },
            PINMUX_END_ITEM
        };

        static pinmux_set_t i2c_pinmux_set = {
            .chip_select = NULL,
            .pinmux = &i2c_pins[0]
        };
        pinmux_set(&i2c_pinmux_set);
    }
    return 0;
}

#ifdef CONFIG_AM_DIB7090P
static struct resource dib7090p_resource[] = {

    [0] = {
        .start = 0,                    //frontend  i2c adapter id
        .end   = 0,
        .flags = IORESOURCE_MEM,
        .name  = "frontend0_i2c"
    },
    [1] = {
        .start = 0x10,                     //frontend 0 demod address
        .end   = 0x10,
        .flags = IORESOURCE_MEM,
        .name  = "frontend0_demod_addr"
    },
};

static struct platform_device dib7090p_device = {
    .name         = "DiB7090P",
    .id       = -1,
    .num_resources    = ARRAY_SIZE(dib7090p_resource),
    .resource     = dib7090p_resource,
};
#endif
#endif

#ifdef CONFIG_AM_SMARTCARD
static int m6tvref_smc_init(void)
{
    static pinmux_item_t smc_pins[] = {
        {/*disable i2s_in*/
            .reg = PINMUX_REG(8),
            .clrmask = 0x7f << 24
        },
        {/*disable uart_b*/
            .reg = PINMUX_REG(4),
            .clrmask = 0xf << 6
        },
        {/*enable 7816*/
            .reg = PINMUX_REG(4),
            .setmask = 0xf << 18
        },
        {/*disable pcm*/
            .reg = PINMUX_REG(4),
            .clrmask = 0xf << 22
        },
        PINMUX_END_ITEM
    };

    static pinmux_set_t smc_pinmux_set = {
        .chip_select = NULL,
        .pinmux = &smc_pins[0]
    };
    pinmux_set(&smc_pinmux_set);
    return 0;
}

static struct resource smartcard_resource[] = {
    [0] = {
        .start = PAD_GPIOX_18,
        .end   = PAD_GPIOX_18,
        .flags = IORESOURCE_MEM,
        .name  = "smc0_reset"
    },
    [1] = {
        .start = INT_SMART_CARD,
        .end   = INT_SMART_CARD,
        .flags = IORESOURCE_IRQ,
        .name  = "smc0_irq"
    },
};

static struct platform_device smartcard_device = {
    .name       = "amlogic-smc",
    .id     = -1,
    .num_resources  = ARRAY_SIZE(smartcard_resource),
    .resource   = smartcard_resource,
};

#endif //def CONFIG_AM_SMARTCARD

#ifdef CONFIG_D2D3_PROCESS
static struct resource d2d3_resource[] = {
    [0] = {
        .start = D2D3_ADDR_START,
        .end = D2D3_ADDR_END,
        .flags = IORESOURCE_MEM,
        .name = "d2d3_mem"
    },
    [1] = {
        .start = INT_D2D3,
        .end  = INT_D2D3,
        .flags = IORESOURCE_IRQ,
        .name = "d2d3_irq"
    },
};

static struct platform_device d2d3_device = {
    .name = "d2d3",
    .id = -1,
    .num_resources = ARRAY_SIZE(d2d3_resource),
    .resource = d2d3_resource,
};
#endif//de CONFIG_D2D3_PROCESS
#ifdef CONFIG_AML_LOCAL_DIMMING
static struct platform_device ldim_device = {
    .name = "aml_ldim",
    .id = 0,
    .num_resources = 0,
    .resource      = NULL,
};
#endif

/***********************************************************************
 * Device Register Section
 **********************************************************************/
static struct platform_device  *platform_devs[] = {
#if defined(CONFIG_I2C_AML) || defined(CONFIG_I2C_HW_AML)
    &aml_i2c_device_a,
    &aml_i2c_device_b,
    &aml_i2c_device_ao,
#endif
#if defined(CONFIG_I2C_SW_AML)
    &aml_sw_i2c_device_1,
#endif
    &aml_uart_device,
#ifdef CONFIG_AM_ETHERNET
    &meson_device_eth,
#endif
    &meson_device_fb,
    &meson_device_vout,
#ifdef CONFIG_AM_STREAMING
    &meson_device_codec,
#endif
#if defined(CONFIG_AM_NAND)
    &aml_nand_device,
#endif
#ifdef CONFIG_SARADC_AM
    &saradc_device,
#endif
#if defined(CONFIG_ADC_KEYPADS_AM)||defined(CONFIG_ADC_KEYPADS_AM_MODULE)
    &adc_kp_device,
#endif
#if defined(CONFIG_KEY_INPUT_CUSTOM_AM) || defined(CONFIG_KEY_INPUT_CUSTOM_AM_MODULE)
    &input_device_key,
#endif
#if defined(CONFIG_CARDREADER)
    &meson_card_device,
#endif // CONFIG_CARDREADER
#if defined(CONFIG_MMC_AML)
    &aml_mmc_device,
#endif
#if defined(CONFIG_SUSPEND)
    &aml_pm_device,
#endif
#ifdef CONFIG_EFUSE
    &aml_efuse_device,
#endif
#if defined(CONFIG_AML_RTC)
    &aml_rtc_device,
#endif
    &aml_audio,
    &aml_audio_dai,
#if defined(CONFIG_SND_SOC_DUMMY_CODEC)
    &aml_dummy_codec_audio,
    &aml_dummy_codec,
#endif
#ifdef CONFIG_SND_AML_M6TV_AUDIO_CODEC
#if defined(CONFIG_SND_AML_M6TV_SYNOPSYS9629_CODEC)
    &aml_syno9629_codec,
#endif    
    &aml_m6tv_audio,
#endif    
#ifdef CONFIG_AM_WIFI
    &wifi_power_device,
#endif
#ifdef CONFIG_AM_REMOTE
    &meson_device_remote,
#endif
#if defined(CONFIG_AMLOGIC_SPI_NOR)
    &amlogic_spi_nor_device,
#endif
#ifdef CONFIG_POST_PROCESS_MANAGER
    &ppmgr_device,
#endif
#if defined(CONFIG_AM_DEINTERLACE) || defined (CONFIG_DEINTERLACE)
    &deinterlace_device,
#endif
#ifdef CONFIG_FREE_SCALE
    &freescale_device,
#endif
#ifdef CONFIG_AM_DVB
    &amlogic_dvb_fe_device,
    &amlogic_dvb_device,
    &amlfe_device,
#endif
#ifdef CONFIG_AM_DIB7090P
    &dib7090p_device,
#endif

#ifdef CONFIG_AM_SMARTCARD
    &smartcard_device,
#endif

#ifdef CONFIG_MESON_CS_DCDC_REGULATOR
    &meson_cs_dcdc_regulator_device,
#endif
#ifdef CONFIG_CPU_FREQ
    &meson_cpufreq_device,
#endif
#ifdef CONFIG_D2D3_PROCESS
    &d2d3_device,
#endif
#ifdef CONFIG_AML_LOCAL_DIMMING
    &ldim_device,
#endif
};

static __init void meson_init_machine(void)
{
//  meson_cache_init();

#ifdef CONFIG_AM_ETHERNET
    setup_eth_device();
#endif

#ifdef CONFIG_AM_REMOTE
    setup_remote_device();
#endif

#ifdef CONFIG_EFUSE
    setup_aml_efuse();
#endif

    setup_usb_devices();
    setup_devices_resource();
    platform_add_devices(platform_devs, ARRAY_SIZE(platform_devs));

#ifdef CONFIG_AML_TV_LCD
    m6tvref_lcd_init();
#endif

    m6tvref_tvin_init();

#ifdef CONFIG_AM_WIFI_USB
    if(wifi_plat_data.usb_set_power)
        wifi_plat_data.usb_set_power(0);//power off built-in usb wifi
#endif

#if defined(CONFIG_SUSPEND)
    {
        //todo: remove it after verified. need set it in uboot environment variable.
        extern  int console_suspend_enabled;
        console_suspend_enabled = 0;
    }
#endif

#if defined(CONFIG_I2C_AML) || defined(CONFIG_I2C_HW_AML)
    aml_i2c_init();

#if 0
    {
        static pinmux_item_t i2c_pins[] = {
            {/*sw i2c*/
                .reg = PINMUX_REG(5),
                .clrmask = 0xf << 24
            },
            PINMUX_END_ITEM
        };

        static pinmux_set_t i2c_pinmux_set = {
            .chip_select = NULL,
            .pinmux = &i2c_pins[0]
        };
        pinmux_set(&i2c_pinmux_set);
    }
#endif
#endif

//  CLEAR_CBUS_REG_MASK(PREG_PAD_GPIO2_EN_N, (1<<21));
//  SET_CBUS_REG_MASK(PREG_PAD_GPIO2_O, (1<<21));

#ifdef CONFIG_AM_DVB

    {
#define FEC_B
        static pinmux_item_t fec_pins[] = {
            {/*fec mode*/
                .reg = PINMUX_REG(0),
                .clrmask = 1 << 6
            },
#ifdef FEC_B    /*FEC_B*/
            {
                .reg = PINMUX_REG(3),
                .setmask = 0x1f << 7
            },
            {
                .reg = PINMUX_REG(0),
                .clrmask = 0xf << 0
            },
            {
                .reg = PINMUX_REG(5),
                .clrmask = 0xff << 16
            },
#else       /*FEC_A*/
            {
                .reg = PINMUX_REG(3),
                .setmask = 0x3f
            },
            {
                .reg = PINMUX_REG(6),
                .clrmask = 0x1f << 19
            },
#endif
            PINMUX_END_ITEM
        };
        static pinmux_set_t fec_pinmux_set = {
            .chip_select = NULL,
            .pinmux = &fec_pins[0]
        };
        pinmux_set(&fec_pinmux_set);
    }
#endif

#ifdef CONFIG_AM_SMARTCARD
    m6tvref_smc_init();
#endif

}

static __init void meson_init_early(void)
{
    ///boot seq 1

}

MACHINE_START(MESON6TV_REF, "Amlogic Meson6TV reference board")
	.boot_params    = BOOT_PARAMS_OFFSET,
	.map_io     = meson_map_io,///2
	.init_early = meson_init_early,///3
	.init_irq   = meson_init_irq,///0
	.timer      = &meson_sys_timer,
	.init_machine   = meson_init_machine,
	.fixup      = meson_fixup,///1
MACHINE_END
