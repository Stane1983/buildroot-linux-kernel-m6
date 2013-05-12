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
#include <asm/mach/arch.h>
#include <asm/setup.h>
#include <asm/memory.h>
#include <asm/mach/map.h>
#include <plat/platform_data.h>
#include <plat/platform.h>
#include <plat/plat_dev.h>
#include <plat/lm.h>
#include <mach/am_regs.h>
#include <mach/i2c_aml.h>
#include <mach/usbclock.h>
#include "board-m3ref.h"
#include <mach/map.h>
#ifdef CONFIG_CACHE_L2X0
#include <asm/hardware/cache-l2x0.h>
#endif
//#ifdef CONFIG_AM_UART
#include <linux/uart-aml.h>
//#endif

#include <mach/pinmux.h>
#include <mach/gpio.h>

#ifdef CONFIG_USB_DWC_OTG_HCD
#define USB_CONTROLLER_ENABLE
#endif
#include "board-m3skt-pinmux.h"
#if 0
static  pinmux_item_t  (*cur_board_pinmux)[MAX_DEVICE_NUMBER][MAX_PIN_ITEM_NUM];
static struct aml_uart_platform  __initdata aml_uart_plat = {
    .uart_line[0]       =  UART_AO,
    .uart_line[1]       =   UART_A,
    .uart_line[2]       =   UART_B,
    .uart_line[3]       =   UART_C,
};
#endif
    
//#endif


static  struct aml_i2c_platform_a  __initdata aml_i2c_data_a={
       .udelay = 8,
       .timeout =10 ,
       .resource ={
            .start =    MESON_I2C_1_START,
            .end   =    MESON_I2C_1_END,
            .flags =    IORESOURCE_MEM,
       }
   } ;
static struct aml_i2c_platform_b   __initdata aml_i2c_data_b={
       .freq= 3,
       .wait=7,
       .resource ={
            .start =    MESON_I2C_2_START,
            .end   =    MESON_I2C_2_END,
            .flags =    IORESOURCE_MEM,
       }
   };

#ifdef USB_CONTROLLER_ENABLE
static void set_usb_a_vbus_power(char is_power_on)
{
#if 0
#define USB_A_POW_GPIO	PREG_EGPIO
#define USB_A_POW_GPIO_BIT	3
#define USB_A_POW_GPIO_BIT_ON	1
#define USB_A_POW_GPIO_BIT_OFF	0
	if(is_power_on){
		printk(KERN_INFO "set usb port power on (board gpio %d)!\n",USB_A_POW_GPIO_BIT);
		set_gpio_mode(USB_A_POW_GPIO,USB_A_POW_GPIO_BIT,GPIO_OUTPUT_MODE);
		set_gpio_val(USB_A_POW_GPIO,USB_A_POW_GPIO_BIT,USB_A_POW_GPIO_BIT_ON);
	}
	else	{
		printk(KERN_INFO "set usb port power off (board gpio %d)!\n",USB_A_POW_GPIO_BIT);		
		set_gpio_mode(USB_A_POW_GPIO,USB_A_POW_GPIO_BIT,GPIO_OUTPUT_MODE);
		set_gpio_val(USB_A_POW_GPIO,USB_A_POW_GPIO_BIT,USB_A_POW_GPIO_BIT_OFF);		
	}
#endif	
}
//usb_a is OTG port
static struct lm_device usb_ld_a = {
	.type = LM_DEVICE_TYPE_USB,
	.id = 0,
	.irq = INT_USB_A,
	.resource.start = IO_USB_A_BASE,
	.resource.end = SZ_256K,
	.dma_mask_room = DMA_BIT_MASK(32),
	.clock.sel = USB_PHY_CLK_SEL_XTAL_DIV_2,
	.clock.div = 0,
	.clock.src = 24000000,
	.param.usb.port_type = USB_PORT_TYPE_OTG,
	.param.usb.port_speed = USB_PORT_SPEED_DEFAULT,
	.param.usb.dma_config = USB_DMA_BURST_DEFAULT,
	.param.usb.set_vbus_power = set_usb_a_vbus_power,
};
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
        .start =  CODEC_ADDR_START,
        .end   = CODEC_ADDR_END,
        .flags = IORESOURCE_MEM,
    },
    [1] = {
        .start = STREAMBUF_ADDR_START,
	 .end = STREAMBUF_ADDR_END,
	 .flags = IORESOURCE_MEM,
    },
};
//usb_b is HOST only port
static struct lm_device usb_ld_b = {
	.type = LM_DEVICE_TYPE_USB,
	.id = 1,
	.irq = INT_USB_B,
	.resource.start = IO_USB_B_BASE,
	.resource.end = -1,
	.resource.end = SZ_256K,
	.dma_mask_room = DMA_BIT_MASK(32),
	.clock.sel = USB_PHY_CLK_SEL_XTAL_DIV_2,
	.clock.div = 0,
	.clock.src = 24000000,
	.param.usb.port_type = USB_PORT_TYPE_HOST,
	.param.usb.port_speed = USB_PORT_SPEED_DEFAULT,
	.param.usb.dma_config = USB_DMA_BURST_DEFAULT,
	.param.usb.set_vbus_power = 0,
};
#endif



















static struct platform_device  *platform_devs[] = {
///    &meson_device_uart,
    &meson_device_fb,
};


static  int __init setup_devices_resource(void)
{
     setup_fb_resource(meson_fb_resource, ARRAY_SIZE(meson_fb_resource));
     setup_codec_resource(meson_codec_resource,ARRAY_SIZE(meson_codec_resource));
     return 0;
}

static  int __init setup_i2c_devices(void)
{
    //just a sample 
    meson_i2c1_set_platdata(&aml_i2c_data_a,sizeof(struct aml_i2c_platform_a ));
    meson_i2c2_set_platdata(&aml_i2c_data_b,sizeof(struct aml_i2c_platform_b ));    
    return 0;
}

static  int __init setup_uart_devices(void)
{
#if 0    
    static pinmux_set_t temp;
//#if defined(CONFIG_AM_UART)
    aml_uart_plat.public.pinmux_cfg.setup=NULL; //NULL repsent use defaut pinmux_set.
    aml_uart_plat.public.pinmux_cfg.clear=NULL;
    aml_uart_plat.public.clk_src=clk_get_sys("clk81", NULL);
    temp.pinmux=cur_board_pinmux[DEVICE_PIN_ITEM_UART];
    aml_uart_plat.public.pinmux_set.pinmux=cur_board_pinmux[DEVICE_PIN_ITEM_UART];
    aml_uart_plat.pinmux_uart[0]=&temp;
    meson_uart_set_platdata(&aml_uart_plat,sizeof(struct aml_uart_platform));
#endif    
    return 0;
//#endif    
}

static void __init device_pinmux_init(void)
{
#if 0 ///@todo Jerry Yu, Compile break , enable it later	
    clearall_pinmux();
    /*other deivce power on*/
    uart_set_pinmux(UART_PORT_AO, UART_AO_GPIO_AO0_AO1_STD);
    /*pinmux of eth*/
    eth_pinmux_init();
    set_audio_pinmux(AUDIO_IN_JTAG); // for MIC input
    set_audio_pinmux(AUDIO_OUT_TEST_N); //External AUDIO DAC
    set_audio_pinmux(SPDIF_OUT_GPIOA); //SPDIF GPIOA_6
#endif    
}

static void __init  device_clk_setting(void)
{
#if 0 ///@todo Jerry Yu, Compile break , enable it later	
    /*Configurate the ethernet clock*/
    eth_clk_set(ETH_CLKSRC_MISC_CLK, get_misc_pll_clk(), (50 * CLK_1M), 0);
#endif    
}

static void disable_unused_model(void)
{
#if 0 ///@todo Jerry Yu, Compile break , enable it later	
    CLK_GATE_OFF(VIDEO_IN);
    CLK_GATE_OFF(BT656_IN);
#endif    
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
static void __init  backup_board_pinmux(void)
{//devices_pins in __initdata section ,it will be released .
#if 0
        cur_board_pinmux=kmemdup(devices_pins, sizeof(devices_pins), GFP_KERNEL);
        printk(KERN_INFO " cur_board_pinmux=%p",cur_board_pinmux[0]);
        printk(KERN_INFO " cur_board_pinmux=%p",&(cur_board_pinmux[0]));
	printk(KERN_INFO " cur_board_pinmux=%x",cur_board_pinmux[0][0]);
	printk(KERN_INFO " cur_board_pinmux=%x",cur_board_pinmux[0][0]->reg);
#endif    
}

static  pinmux_item_t  __initdata uart_pins[]={
		{
			.reg=PINMUX_REG(AO),
			.clrmask=3<<16,
			.setmask=3<<11
		},
		PINMUX_END_ITEM
	};
static  pinmux_set_t __initdata  aml_uart_ao={
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

static __init void meson_init_machine(void)
{
    backup_board_pinmux();
    meson_cache_init();
    setup_devices_resource();
    ///setup_i2c_devices();
    ///setup_uart_devices();
    device_clk_setting();
    device_pinmux_init();
    platform_device_register(&aml_uart_device);
   /// platform_add_devices(platform_devs, ARRAY_SIZE(platform_devs));

#ifdef USB_CONTROLLER_ENABLE
    lm_device_register(&usb_ld_a);
    lm_device_register(&usb_ld_b);
#endif

#if defined(CONFIG_TOUCHSCREEN_ADS7846)
    ads7846_init_gpio();
    spi_register_board_info(spi_board_info_list, ARRAY_SIZE(spi_board_info_list));
#endif
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
        .virtual    = IO_PERIPH_BASE,
        .pfn        = __phys_to_pfn(IO_PERIPH_PHY_BASE),
        .length     = SZ_4K,
        .type       = MT_DEVICE,
    } , {
        .virtual    = IO_APB_BUS_BASE,
        .pfn        = __phys_to_pfn(IO_APB_BUS_PHY_BASE),
        .length     = SZ_512K,
        .type       = MT_DEVICE,
    } , {
        .virtual    = IO_AOBUS_BASE,
        .pfn        = __phys_to_pfn(IO_AOBUS_PHY_BASE),
        .length     = SZ_1M,
        .type       = MT_DEVICE,
    } , {
        .virtual    = IO_AHB_BUS_BASE,
        .pfn        = __phys_to_pfn(IO_AHB_BUS_PHY_BASE),
        .length     = SZ_16M,
        .type       = MT_DEVICE,
    } , {
        .virtual    = IO_APB2_BUS_BASE,
        .pfn        = __phys_to_pfn(IO_APB2_BUS_PHY_BASE),
        .length     = SZ_512K,
        .type       = MT_DEVICE,
    }, {
        .virtual    = PAGE_ALIGN(__phys_to_virt(RESERVED_MEM_START)),
        .pfn        = __phys_to_pfn(RESERVED_MEM_START),
        .length     = RESERVED_MEM_END - RESERVED_MEM_START + 1,
        .type       = MT_DEVICE,
    },
#ifdef CONFIG_MESON_SUSPEND
	{
        .virtual    = PAGE_ALIGN(0xdff00000),
        .pfn        = __phys_to_pfn(0x1ff00000),
        .length     = SZ_1M,
        .type       = MT_MEMORY,
	},
#endif
};

static  void __init meson_map_io(void)
{
    iotable_init(meson_io_desc, ARRAY_SIZE(meson_io_desc));
}

static __init void m3_irq_init(void)
{
    meson_init_irq();
}

static __init void m3_fixup(struct machine_desc *mach, struct tag *tag, char **cmdline, struct meminfo *m)
{
/**
 * @todo Jerry Yu , Compile break, Enable it later	
 */
    struct membank *pbank;
    m->nr_banks = 0;
    pbank = &m->bank[m->nr_banks];
    pbank->start = PAGE_ALIGN(PHYS_MEM_START);
    pbank->size  = SZ_64M & PAGE_MASK;
    ///pbank->node  = PHYS_TO_NID(PHYS_MEM_START);
    m->nr_banks++;
    pbank = &m->bank[m->nr_banks];
    pbank->start = PAGE_ALIGN(RESERVED_MEM_END + 1);
    pbank->size  = (PHYS_MEM_END - RESERVED_MEM_END) & PAGE_MASK;
///    pbank->node  = PHYS_TO_NID(RESERVED_MEM_END + 1);
    m->nr_banks++;
   
}

MACHINE_START(M3_SKT, "Meson 3 socket board")
    .boot_params    = BOOT_PARAMS_OFFSET,
    .map_io         = meson_map_io,
    .init_irq       = m3_irq_init,
    .timer          = &meson_sys_timer,
    .init_machine   = meson_init_machine,
    .fixup          = m3_fixup,
    .video_start    = RESERVED_MEM_START,
    .video_end      = RESERVED_MEM_END,
MACHINE_END
