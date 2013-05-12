#include <linux/init.h>
#include <linux/module.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/sysfs.h>
#include <asm/uaccess.h>
#include <mach/am_regs.h>
#include <mach/power_gate.h>
#include <mach/gpio.h>
#include <mach/clock.h>
#include <linux/delay.h>

MODULE_LICENSE("GPL");

#define INFO_LENGTH	4096
//#define GPIO_TEST

extern struct clk *clk_get_sys(const char *dev_id, const char *con_id);

#ifdef GPIO_TEST
extern unsigned long  get_gpio_val(gpio_bank_t bank, int bit);

typedef struct device_power {
	char* name;
	unsigned gpio_bank;
	unsigned gpio_bit;
	int inverse_flag;
} device_power_t;

#define DEF_DEVICE_POWER(name_param, gpio_bank_param, gpio_bit_param, inverse_flag_param) \
{ \
	.name = (name_param), \
	.gpio_bank = (gpio_bank_param), \
	.gpio_bit = (gpio_bit_param), \
	.inverse_flag = (inverse_flag_param), \
} 

static device_power_t devices_power[] = {
	DEF_DEVICE_POWER("VCCX2_EN", GPIOA_bank_bit0_27(26), GPIOA_bit_bit0_27(26), 1),
	DEF_DEVICE_POWER("VCCX3_EN", GPIOC_bank_bit0_15(2), GPIOC_bit_bit0_15(2), 0),
	DEF_DEVICE_POWER("LCD_POWER_EN", GPIOA_bank_bit0_27(27), GPIOA_bit_bit0_27(27), 1),
	DEF_DEVICE_POWER("BL_EN", GPIOD_bank_bit0_9(1), GPIOD_bit_bit0_9(1), 0),
	DEF_DEVICE_POWER("CARD_EN", GPIOCARD_bank_bit0_8(8), GPIOCARD_bit_bit0_8(8), 1),
	DEF_DEVICE_POWER("USB_PWR_CTRL", GPIOD_bank_bit0_9(9), GPIOD_bit_bit0_9(9), 0),
	DEF_DEVICE_POWER("SPK", GPIOC_bank_bit0_15(4), GPIOC_bit_bit0_15(4), 0),
	DEF_DEVICE_POWER("CAP_TP_EN", GPIOC_bank_bit0_15(3), GPIOC_bit_bit0_15(3), 0),
	DEF_DEVICE_POWER("WIFI_PWREN", GPIOC_bank_bit0_15(8), GPIOC_bit_bit0_15(8), 0),
	DEF_DEVICE_POWER("WL_RST_N", GPIOC_bank_bit0_15(7), GPIOC_bit_bit0_15(7), 0),
	DEF_DEVICE_POWER("BT_RST_N", GPIOC_bank_bit0_15(6), GPIOC_bit_bit0_15(6), 0),
	DEF_DEVICE_POWER("GPS_RSTN", GPIOC_bank_bit0_15(5), GPIOC_bit_bit0_15(5), 0),
};	

static int get_dev_power_state(char* info_buf)
{
	int dev_num = sizeof(devices_power)/sizeof(device_power_t);
	int power_state = 0;
	int i;
	sprintf(&info_buf[strlen(info_buf)], "devices power state are:\n\n");
	for (i = 0; i < dev_num; i++) {
		power_state = get_gpio_val(devices_power[i].gpio_bank, 
						devices_power[i].gpio_bit);
		if(devices_power[i].inverse_flag)
			power_state = !power_state;
		sprintf(&info_buf[strlen(info_buf)], 
			"%s state is %s \n", devices_power[i].name, power_state?"on":"off");
	}
	sprintf(&info_buf[strlen(info_buf)], "\n");
	return 0;
}
#endif

static char* clock_devices[] = {
	"clk_sys_pll",
	"clk_misc_pll",
	"clk_ddr_pll",
	"clk81",
	"a9_clk",
	NULL,
};

static int get_frequece(char* info_buf)
{
	struct clk* l_clk = NULL;
	int i = 0;
	sprintf(&info_buf[strlen(info_buf)], "clock frequence are:\n\n");
	while (clock_devices[i] != NULL) {
		l_clk = clk_get_sys(clock_devices[i], NULL);
		if (l_clk) 
			sprintf(&info_buf[strlen(info_buf)], 
				"%s clock is: %ld M\n", clock_devices[i], l_clk->rate / 1000000);
		i++;
	}
	sprintf(&info_buf[strlen(info_buf)], "\n");
	return 0;
}

#define GET_FREQUENCE(MOD, BUF) sprintf(&BUF[strlen(BUF)], \
		 "%s clock is %dM \n", #MOD, get_##MOD##_clk()/1000000)

/*static int get_frequece2(char* info_buf)
{
	sprintf(&info_buf[strlen(info_buf)], "clock frequence are:\n\n");
	GET_FREQUENCE(system, info_buf);
	GET_FREQUENCE(mpeg, info_buf);
	GET_FREQUENCE(misc_pll, info_buf);
	GET_FREQUENCE(ddr_pll, info_buf);
	//GET_FREQUENCE(a9, info_buf);
	sprintf(&info_buf[strlen(info_buf)], "\n");
	return 0;
}*/

#define TOSTRING(_ARG) #_ARG

#define PRINT_GATE_STATE(_MOD, INFO_BUF) \
do { \
	sprintf(&INFO_BUF[strlen(INFO_BUF)], \
		"%s state is %s \n",  #_MOD, IS_CLK_GATE_ON(_MOD)?"on":"off"); \
	printk("%s state is %s \n",  #_MOD, IS_CLK_GATE_ON(_MOD)?"on":"off"); \
} while(0)

static ssize_t power_info_show(struct class *cla, struct class_attribute *attr, char *buf)
{
	char* info_buf = kzalloc(sizeof(char)*INFO_LENGTH, GFP_KERNEL);
	//int ret = 0;
	if (!info_buf) 
		return - ENOMEM;
		
	//get_frequece(info_buf);
	
	//get_frequece2(info_buf);

#ifdef GPIO_TEST	
	get_dev_power_state(info_buf);
#endif
	
	printk("%s \n", info_buf);
	
	//copy_to_user(buf, info_buf, strlen(info_buf));
	
	kfree(info_buf);
	info_buf = NULL;
	return 0;//ret = strlen(info_buf);
}

static ssize_t gate_info_show(struct class *cla, struct class_attribute *attr, char *buf)
{
	char* info_buf = kzalloc(sizeof(char)*INFO_LENGTH, GFP_KERNEL);
	int ret = 0;
	if (!info_buf) 
		return - ENOMEM;
		
	PRINT_GATE_STATE(DDR, info_buf);
	PRINT_GATE_STATE(DOS, info_buf);
	PRINT_GATE_STATE(MIPI_APB_CLK, info_buf);
	PRINT_GATE_STATE(MIPI_SYS_CLK, info_buf);
	PRINT_GATE_STATE(AHB_BRIDGE, info_buf);
	PRINT_GATE_STATE(ISA, info_buf);
	PRINT_GATE_STATE(APB_CBUS, info_buf);
	PRINT_GATE_STATE(_1200XXX, info_buf);
	PRINT_GATE_STATE(SPICC, info_buf);
	PRINT_GATE_STATE(I2C, info_buf);
	PRINT_GATE_STATE(SAR_ADC, info_buf);
	PRINT_GATE_STATE(SMART_CARD_MPEG_DOMAIN, info_buf);
	PRINT_GATE_STATE(RANDOM_NUM_GEN, info_buf);
	PRINT_GATE_STATE(UART0, info_buf);
	PRINT_GATE_STATE(SDHC, info_buf);
	PRINT_GATE_STATE(STREAM, info_buf);
	PRINT_GATE_STATE(ASYNC_FIFO, info_buf);
	PRINT_GATE_STATE(SDIO, info_buf);
	PRINT_GATE_STATE(AUD_BUF, info_buf);
	PRINT_GATE_STATE(HIU_PARSER, info_buf);

	PRINT_GATE_STATE(BT656_IN, info_buf);
	PRINT_GATE_STATE(ASSIST_MISC, info_buf);
	PRINT_GATE_STATE(VENC_I_TOP, info_buf);
	PRINT_GATE_STATE(VENC_P_TOP, info_buf);
	PRINT_GATE_STATE(VENC_T_TOP, info_buf);
	PRINT_GATE_STATE(VENC_DAC, info_buf);
	PRINT_GATE_STATE(VI_CORE, info_buf);
	PRINT_GATE_STATE(SPI2, info_buf);

	PRINT_GATE_STATE(SPI1, info_buf);
	PRINT_GATE_STATE(AUD_IN, info_buf);
	PRINT_GATE_STATE(ETHERNET, info_buf);
	PRINT_GATE_STATE(DEMUX, info_buf);
	PRINT_GATE_STATE(AIU_AI_TOP_GLUE, info_buf);
	PRINT_GATE_STATE(AIU_IEC958, info_buf);
	PRINT_GATE_STATE(AIU_I2S_OUT, info_buf);
	PRINT_GATE_STATE(AIU_AMCLK_MEASURE, info_buf);
	PRINT_GATE_STATE(AIU_AIFIFO2, info_buf);
	PRINT_GATE_STATE(AIU_AUD_MIXER, info_buf);
	PRINT_GATE_STATE(AIU_MIXER_REG, info_buf);
	PRINT_GATE_STATE(AIU_ADC, info_buf);
	PRINT_GATE_STATE(BLK_MOV, info_buf);
	PRINT_GATE_STATE(UART1, info_buf);
	PRINT_GATE_STATE(LED_PWM, info_buf);
	PRINT_GATE_STATE(VGHL_PWM, info_buf);
	PRINT_GATE_STATE(GE2D, info_buf);
	PRINT_GATE_STATE(USB0, info_buf);
	PRINT_GATE_STATE(USB1, info_buf);
	PRINT_GATE_STATE(RESET, info_buf);
	PRINT_GATE_STATE(NAND, info_buf);
	PRINT_GATE_STATE(HIU_PARSER_TOP, info_buf);
	PRINT_GATE_STATE(MIPI_PHY, info_buf);
	PRINT_GATE_STATE(VIDEO_IN, info_buf);
	PRINT_GATE_STATE(AHB_ARB0, info_buf);
	PRINT_GATE_STATE(EFUSE, info_buf);
	PRINT_GATE_STATE(ROM_CLK, info_buf);
	PRINT_GATE_STATE(AHB_DATA_BUS, info_buf);
	PRINT_GATE_STATE(AHB_CONTROL_BUS, info_buf);
	PRINT_GATE_STATE(HDMI_INTR_SYNC, info_buf);
	PRINT_GATE_STATE(HDMI_PCLK, info_buf);
	PRINT_GATE_STATE(MISC_USB1_TO_DDR, info_buf);
	PRINT_GATE_STATE(MISC_USB0_TO_DDR, info_buf);

	PRINT_GATE_STATE(MMC_PCLK, info_buf);
	PRINT_GATE_STATE(MISC_DVIN, info_buf);
	PRINT_GATE_STATE(MISC_RDMA, info_buf);
	PRINT_GATE_STATE(UART2, info_buf);
	PRINT_GATE_STATE(VENCI_INT, info_buf);
	PRINT_GATE_STATE(VIU2, info_buf);
	PRINT_GATE_STATE(VENCP_INT, info_buf);
	PRINT_GATE_STATE(VENCT_INT, info_buf);
	PRINT_GATE_STATE(VENCL_INT, info_buf);
	PRINT_GATE_STATE(VENC_L_TOP, info_buf);
	PRINT_GATE_STATE(UART3, info_buf);
	PRINT_GATE_STATE(VCLK2_VENCI, info_buf);
	PRINT_GATE_STATE(VCLK2_VENCI1, info_buf);
	PRINT_GATE_STATE(VCLK2_VENCP, info_buf);
	PRINT_GATE_STATE(VCLK2_VENCP1, info_buf);
	PRINT_GATE_STATE(VCLK2_VENCT, info_buf);
	PRINT_GATE_STATE(VCLK2_VENCT1, info_buf);
	PRINT_GATE_STATE(VCLK2_OTHER, info_buf);
	PRINT_GATE_STATE(VCLK2_ENCI, info_buf);
	PRINT_GATE_STATE(VCLK2_ENCP, info_buf);
	PRINT_GATE_STATE(DAC_CLK, info_buf);
	PRINT_GATE_STATE(AIU_AOCLK, info_buf);
	PRINT_GATE_STATE(AIU_AMCLK, info_buf);
	PRINT_GATE_STATE(AIU_ICE958_AMCLK, info_buf);
	PRINT_GATE_STATE(VCLK1_HDMI, info_buf);
	PRINT_GATE_STATE(AIU_AUDIN_SCLK, info_buf);
	PRINT_GATE_STATE(ENC480P, info_buf);
	PRINT_GATE_STATE(VCLK2_ENCT, info_buf);
	PRINT_GATE_STATE(VCLK2_ENCL, info_buf);
	PRINT_GATE_STATE(MMC_CLK, info_buf);
	PRINT_GATE_STATE(VCLK2_VENCL, info_buf);
	PRINT_GATE_STATE(VCLK2_OTHER1, info_buf);
		
	PRINT_GATE_STATE(MEDIA_CPU, info_buf);
	PRINT_GATE_STATE(AHB_SRAM, info_buf);
	PRINT_GATE_STATE(AHB_BUS, info_buf);
	PRINT_GATE_STATE(AO_REGS, info_buf);
	
	return ret = 0; //strlen(info_buf);
}


typedef struct gate_dev {
	const char* name;
	int dev_index;
	unsigned clock_gate_reg_adr;
	unsigned clock_gate_reg_mask;
	unsigned char can_be_off;
} gate_dev_t;

#define DEFINE_CLK_DEV(_MOD)  \
{ \
	.name = GCLK_NAME_##_MOD, \
	.dev_index = GCLK_IDX_##_MOD,	\
	.clock_gate_reg_adr = GCLK_REG_##_MOD,	\
	.clock_gate_reg_mask = GCLK_MASK_##_MOD, \
	.can_be_off = 0, \
}

#define DEFINE_CLK_DEV_CAN_BE_OFF(_MOD)  \
{ \
	.name = GCLK_NAME_##_MOD, \
	.dev_index = GCLK_IDX_##_MOD,	\
	.clock_gate_reg_adr = GCLK_REG_##_MOD,	\
	.clock_gate_reg_mask = GCLK_MASK_##_MOD, \
	.can_be_off = 1, \
}

static gate_dev_t gate_devices[GCLK_IDX_MAX] = {
	DEFINE_CLK_DEV(DDR),
	DEFINE_CLK_DEV(DOS),
	DEFINE_CLK_DEV_CAN_BE_OFF(MIPI_APB_CLK),
	DEFINE_CLK_DEV_CAN_BE_OFF(MIPI_SYS_CLK),
	DEFINE_CLK_DEV(AHB_BRIDGE),
	DEFINE_CLK_DEV(ISA),
	DEFINE_CLK_DEV(APB_CBUS),
	DEFINE_CLK_DEV(_1200XXX),
	DEFINE_CLK_DEV(SPICC),
	DEFINE_CLK_DEV(I2C),
	//
	DEFINE_CLK_DEV(SAR_ADC),
	DEFINE_CLK_DEV(SMART_CARD_MPEG_DOMAIN),
	DEFINE_CLK_DEV_CAN_BE_OFF(RANDOM_NUM_GEN),
	DEFINE_CLK_DEV_CAN_BE_OFF(UART0),
	DEFINE_CLK_DEV_CAN_BE_OFF(SDHC),
	DEFINE_CLK_DEV_CAN_BE_OFF(STREAM),
	DEFINE_CLK_DEV_CAN_BE_OFF(ASYNC_FIFO),
	DEFINE_CLK_DEV(SDIO),
	DEFINE_CLK_DEV_CAN_BE_OFF(AUD_BUF),
	DEFINE_CLK_DEV(HIU_PARSER),
	DEFINE_CLK_DEV(RESERVED0),
	//
	DEFINE_CLK_DEV_CAN_BE_OFF(BT656_IN),
	DEFINE_CLK_DEV(ASSIST_MISC),
	DEFINE_CLK_DEV_CAN_BE_OFF(VENC_I_TOP),
	DEFINE_CLK_DEV_CAN_BE_OFF(VENC_P_TOP),
	DEFINE_CLK_DEV_CAN_BE_OFF(VENC_T_TOP),
	DEFINE_CLK_DEV_CAN_BE_OFF(VENC_DAC),
	DEFINE_CLK_DEV(VI_CORE),
	DEFINE_CLK_DEV(RESERVED1),
	DEFINE_CLK_DEV_CAN_BE_OFF(SPI2),
	/***************** 0x1050 = 0x30BA0FFF ************************/
	DEFINE_CLK_DEV_CAN_BE_OFF(SPI1),
	
	DEFINE_CLK_DEV_CAN_BE_OFF(AUD_IN),
	DEFINE_CLK_DEV_CAN_BE_OFF(ETHERNET),
	DEFINE_CLK_DEV_CAN_BE_OFF(DEMUX),
	DEFINE_CLK_DEV(RESERVED2),
	DEFINE_CLK_DEV_CAN_BE_OFF(AIU_AI_TOP_GLUE),
	DEFINE_CLK_DEV_CAN_BE_OFF(AIU_IEC958),
	DEFINE_CLK_DEV_CAN_BE_OFF(AIU_I2S_OUT),
	DEFINE_CLK_DEV_CAN_BE_OFF(AIU_AMCLK_MEASURE),
	DEFINE_CLK_DEV_CAN_BE_OFF(AIU_AIFIFO2),
	DEFINE_CLK_DEV_CAN_BE_OFF(AIU_AUD_MIXER),
	DEFINE_CLK_DEV_CAN_BE_OFF(AIU_MIXER_REG),
	DEFINE_CLK_DEV_CAN_BE_OFF(AIU_ADC),
	DEFINE_CLK_DEV_CAN_BE_OFF(BLK_MOV),
	
	DEFINE_CLK_DEV(RESERVED3),
	DEFINE_CLK_DEV(UART1),
	DEFINE_CLK_DEV_CAN_BE_OFF(LED_PWM),
	DEFINE_CLK_DEV_CAN_BE_OFF(VGHL_PWM),
	DEFINE_CLK_DEV(RESERVED4),
	DEFINE_CLK_DEV(GE2D),
	DEFINE_CLK_DEV_CAN_BE_OFF(USB0),
	DEFINE_CLK_DEV_CAN_BE_OFF(USB1),
	
	DEFINE_CLK_DEV(RESET),
	DEFINE_CLK_DEV(NAND),
	DEFINE_CLK_DEV_CAN_BE_OFF(HIU_PARSER_TOP),
	DEFINE_CLK_DEV_CAN_BE_OFF(MIPI_PHY),
	
	DEFINE_CLK_DEV_CAN_BE_OFF(VIDEO_IN),
	DEFINE_CLK_DEV(AHB_ARB0),
	DEFINE_CLK_DEV_CAN_BE_OFF(EFUSE),
	DEFINE_CLK_DEV_CAN_BE_OFF(ROM_CLK),
	
	/********************0x1051 = 21998020****************/
	DEFINE_CLK_DEV(RESERVED5),
	DEFINE_CLK_DEV(AHB_DATA_BUS),
	DEFINE_CLK_DEV(AHB_CONTROL_BUS),
	DEFINE_CLK_DEV_CAN_BE_OFF(HDMI_INTR_SYNC),
	DEFINE_CLK_DEV_CAN_BE_OFF(HDMI_PCLK),
	DEFINE_CLK_DEV(RESERVED6),
	DEFINE_CLK_DEV(RESERVED7),
	DEFINE_CLK_DEV(RESERVED8),
	DEFINE_CLK_DEV_CAN_BE_OFF(MISC_USB1_TO_DDR),
	DEFINE_CLK_DEV_CAN_BE_OFF(MISC_USB0_TO_DDR),
	DEFINE_CLK_DEV_CAN_BE_OFF(MMC_PCLK),
	DEFINE_CLK_DEV_CAN_BE_OFF(MISC_DVIN),
	
	DEFINE_CLK_DEV(MISC_RDMA),
	DEFINE_CLK_DEV(RESERVED9),
	DEFINE_CLK_DEV(UART2),
	DEFINE_CLK_DEV(VENCI_INT),
	DEFINE_CLK_DEV(VIU2),
	
	DEFINE_CLK_DEV_CAN_BE_OFF(VENCP_INT),
	
	DEFINE_CLK_DEV(VENCT_INT),
	
	DEFINE_CLK_DEV_CAN_BE_OFF(VENCL_INT),
	DEFINE_CLK_DEV_CAN_BE_OFF(VENC_L_TOP),
	DEFINE_CLK_DEV_CAN_BE_OFF(UART3),
	/**********************************************************/
	//
	DEFINE_CLK_DEV_CAN_BE_OFF(VCLK2_VENCI),
	DEFINE_CLK_DEV_CAN_BE_OFF(VCLK2_VENCI1),
	DEFINE_CLK_DEV_CAN_BE_OFF(VCLK2_VENCP),
	DEFINE_CLK_DEV_CAN_BE_OFF(VCLK2_VENCP1),
	//
	DEFINE_CLK_DEV(VCLK2_VENCT),
	DEFINE_CLK_DEV(VCLK2_VENCT1),
	DEFINE_CLK_DEV_CAN_BE_OFF(VCLK2_OTHER),
	DEFINE_CLK_DEV(VCLK2_ENCI),
	DEFINE_CLK_DEV(VCLK2_ENCP),
	DEFINE_CLK_DEV_CAN_BE_OFF(DAC_CLK),
	DEFINE_CLK_DEV_CAN_BE_OFF(AIU_AOCLK),
	DEFINE_CLK_DEV_CAN_BE_OFF(AIU_AMCLK),
	
	DEFINE_CLK_DEV_CAN_BE_OFF(AIU_ICE958_AMCLK),
	DEFINE_CLK_DEV_CAN_BE_OFF(VCLK1_HDMI),
	DEFINE_CLK_DEV_CAN_BE_OFF(AIU_AUDIN_SCLK),
	DEFINE_CLK_DEV_CAN_BE_OFF(ENC480P),
	DEFINE_CLK_DEV(VCLK2_ENCT),
	DEFINE_CLK_DEV_CAN_BE_OFF(VCLK2_ENCL),
	
	DEFINE_CLK_DEV(MMC_CLK),
	DEFINE_CLK_DEV_CAN_BE_OFF(VCLK2_VENCL),
	DEFINE_CLK_DEV_CAN_BE_OFF(VCLK2_OTHER1),
	
    DEFINE_CLK_DEV_CAN_BE_OFF(MEDIA_CPU),
	DEFINE_CLK_DEV(AHB_SRAM),
	DEFINE_CLK_DEV(AHB_BUS),
	DEFINE_CLK_DEV(AO_REGS),
};

#ifdef GPIO_TEST
static ssize_t power_off_store(struct class *cla, 
		struct class_attribute *attr, char *buf, size_t count)
{
	int dev_num = sizeof(devices_power)/sizeof(device_power_t);
	int i = 0;
	
	while(i < dev_num) {
		if(!strncmp(devices_power[i].name, buf, \
				(strlen(devices_power[i].name) > strlen(buf)) ? strlen(buf) \
					: strlen(devices_power[i].name))) {
			break;
		} else
			i++;
	}

	if (i < dev_num ) {
		set_gpio_val(devices_power[i].gpio_bank, devices_power[i].gpio_bit, 
				devices_power[i].inverse_flag?1:0);
		printk("power of device %s \n", devices_power[i].name);
	}
	return strlen(buf);
}	
#endif

/***********************************************************************************************/

#define Wr WRITE_CBUS_REG 
#define Wr_reg_bits WRITE_CBUS_REG_BITS

void gclk_enable(void);
void clk_off_vpu(void);
void clk_off_hdmi(void);
void clk_off_mdec(void);
void clk_off_usb(void);
void clk_off_aiu(void);
void clk_off_ge2d(void);
void clk_off_vpu_viu1(void);
void clk_off_vpu_viu2(void);
void clk_off_vpu_vdi1(void);
void clk_off_vpu_vdi6(void);
void clk_off_vpu_vdin0_com(void);
void clk_off_vpu_vdin1_com(void);
void clk_off_vpu_di_mad_top(void);
void clk_off_vpu_vdin_meas(void);
void clk_off_vpu_enci(void);
void clk_off_vpu_encp(void);
void clk_off_vpu_enct(void);
void clk_off_vpu_encl(void);
void clk_off_vpu_vdac(void);
void clk_off_vpu_lcd_an_clk_ph23(void);

// -----------------------------------------------------------------------------
//                     Function: clk_off_vpu
// Turn off VPU clocks when not used, for saving dynamic power consumption.
// -----------------------------------------------------------------------------
void clk_off_vpu (void)
{
    //stimulus_print("[TEST.C] Clock off VPU\n");
    clk_off_vpu_viu1();
    //clk_off_vpu_viu2();
    clk_off_vpu_vdi1();
    clk_off_vpu_vdi6();
    clk_off_vpu_vdin_meas();
    clk_off_vpu_enci();
    clk_off_vpu_encp();
    //clk_off_vpu_enct();
    clk_off_vpu_encl();
    clk_off_vpu_vdac();
    clk_off_vpu_lcd_an_clk_ph23();
} /* clk_off_vpu */

// -----------------------------------------------------------------------------
//                     Function: clk_off_hdmi
// Turn off HDMI clocks when not used, for saving dynamic power consumption.
// -----------------------------------------------------------------------------
void clk_off_hdmi (void)
{
    //stimulus_print("[TEST.C] Clock off HDMI\n");
    Wr_reg_bits(HHI_GCLK_MPEG2,         0,  3,  1); // Turn off HCLK
    Wr_reg_bits(HHI_GCLK_MPEG2,         0,  4,  1); // Turn off PCLK
    Wr_reg_bits(HHI_HDMI_CLK_CNTL,      0,  8,  1); // Turn off hdmi_sys_clk
    Wr_reg_bits(HHI_HDMI_CLK_CNTL,      0xc,16, 4); // Turn off hdmi_tx_pixel_clk
    Wr_reg_bits(AIU_HDMI_CLK_DATA_CTRL, 0,  4,  2); // Set hdmi_audio_data_sel=0
    Wr_reg_bits(AIU_HDMI_CLK_DATA_CTRL, 0,  0,  2); // Set hdmi_audio_clk_sel=0
} /* clk_off_hdmi */

// -----------------------------------------------------------------------------
//                     Function: clk_off_mdec
// Turn off MDEC clocks when not used, for saving dynamic power consumption.
// -----------------------------------------------------------------------------
void clk_off_mdec (void)
{
    //stimulus_print("[TEST.C] Clock off MDEC\n");
    Wr_reg_bits(HHI_GCLK_MPEG0,         0,  1,  3);
    Wr_reg_bits(HHI_GCLK_MPEG0,         0, 31,  1);
    Wr_reg_bits(HHI_GCLK_MPEG1,         0,  0,  1);
    Wr_reg_bits(HHI_GCLK_MPEG1,         0, 26,  2);
} /* clk_off_mdec */

// -----------------------------------------------------------------------------
//                     Function: clk_off_usb
// Turn off USB clocks when not used, for saving dynamic power consumption.
// -----------------------------------------------------------------------------
void clk_off_usb (void)
{
   // stimulus_print("[TEST.C] Clock off USB\n");
    Wr_reg_bits(HHI_GCLK_MPEG1,         0, 21,  2);
} /* clk_off_usb */

// -----------------------------------------------------------------------------
//                     Function: clk_off_aiu
// Turn off AIU clocks when not used, for saving dynamic power consumption.
// -----------------------------------------------------------------------------
void clk_off_aiu (void)
{
    //stimulus_print("[TEST.C] Clock off AIU\n");
    Wr_reg_bits(HHI_GCLK_MPEG2,     0, 10, 1); // Turn off PCLK
    Wr_reg_bits(HHI_GCLK_MPEG1,     0,  6, 8); // Turn off clk_mpeg
    Wr_reg_bits(HHI_AUD_CLK_CNTL,   0,  8, 1); // Turn off u_clk_rst_tst.cts_amclk
    Wr_reg_bits(HHI_AUD_CLK_CNTL,   0, 23, 1); // Turn off u_clk_rst_tst.cts_audac_clkpi
} /* clk_off_aiu */

// -----------------------------------------------------------------------------
//                     Function: clk_off_ge2d
// Turn off GE2D clocks when not used, for saving dynamic power consumption.
// -----------------------------------------------------------------------------
void clk_off_ge2d (void)
{
    //stimulus_print("[TEST.C] Clock off GE2D\n");
    Wr_reg_bits(HHI_GCLK_MPEG1, 0,  20, 1); // Turn off clk
} /* clk_off_ge2d */

// -----------------------------------------------------------------------------
//                     Function: clk_off_vpu_vdin0_com
// Turn off VDIN0_COM_PROC clocks when not used, for saving dynamic power consumption.
// -----------------------------------------------------------------------------
void clk_off_vpu_vdin0_com (void)
{
    //stimulus_print("[TEST.C] Clock off VDIN0_COM_PROC\n");
   // Wr(VDIN0_COM_GCLK_CTRL, 0x5555);    // Turn off clk
} /* clk_off_vpu_vdin0_com */

// -----------------------------------------------------------------------------
//                     Function: clk_off_vpu_vdin1_com
// Turn off VDIN1_COM_PROC clocks when not used, for saving dynamic power consumption.
// -----------------------------------------------------------------------------
void clk_off_vpu_vdin1_com (void)
{
    //stimulus_print("[TEST.C] Clock off VDIN1_COM_PROC\n");
    //Wr(VDIN1_COM_GCLK_CTRL, 0x5555);    // Turn off clk
} /* clk_off_vpu_vdin1_com */

// -----------------------------------------------------------------------------
//                     Function: clk_off_vpu_viu1
// Turn off VIU1 clocks when not used, for saving dynamic power consumption.
// -----------------------------------------------------------------------------
void clk_off_vpu_viu1 (void)
{
    //stimulus_print("[TEST.C] Clock off VPU VIU1\n");
    Wr_reg_bits(HHI_GCLK_MPEG0, 0,  28, 1); // Turn off clk_mpeg
} /* clk_off_vpu_viu1 */

// -----------------------------------------------------------------------------
//                     Function: clk_off_vpu_viu2
// Turn off VIU2 clocks when not used, for saving dynamic power consumption.
// -----------------------------------------------------------------------------
void clk_off_vpu_viu2 (void)
{
    //stimulus_print("[TEST.C] Clock off VPU VIU2\n");
    Wr_reg_bits(HHI_GCLK_MPEG2, 0,  17, 1); // Turn off clk_mpeg
} /* clk_off_vpu_viu2 */

// -----------------------------------------------------------------------------
//                     Function: clk_off_vpu_vdi1
// Turn off VPU's VDI1 clocks when not used, for saving dynamic power consumption.
// -----------------------------------------------------------------------------
void clk_off_vpu_vdi1 (void)
{
    //stimulus_print("[TEST.C] Clock off VPU VDI1\n");
    Wr_reg_bits(HHI_GCLK_MPEG1, 0,  28, 1); // Turn off vid1_clk
} /* clk_off_vpu_vdi1 */

// -----------------------------------------------------------------------------
//                     Function: clk_off_vpu_vdi6
// Turn off VPU's VDI6 clocks when not used, for saving dynamic power consumption.
// -----------------------------------------------------------------------------
void clk_off_vpu_vdi6 (void)
{
    //stimulus_print("[TEST.C] Clock off VPU VDI6\n");
    Wr_reg_bits(VPU_VIU_VENC_MUX_CTRL,  0,  8,  4); // Set cntl_viu_vdin_sel_data=0
    Wr_reg_bits(VPU_VIU_VENC_MUX_CTRL,  0,  4,  4); // Set cntl_viu_vdin_sel_clk=0
} /* clk_off_vpu_vdi6 */
// -----------------------------------------------------------------------------
//                     Function: clk_off_vpu_di_mad_top
// Turn off DI_MAD_TOP clocks when not used, for saving dynamic power consumption.
// -----------------------------------------------------------------------------
void clk_off_vpu_di_mad_top (void)
{
    //stimulus_print("[TEST.C] Clock off DI_MAD_TOP\n");
    Wr_reg_bits(DI_CLKG_CTRL,   1,  1,  1); // Turn off clk
} /* clk_off_vpu_di_mad_top */


// -----------------------------------------------------------------------------
//                     Function: clk_off_vpu_vdin_meas
// Turn off VPU's VDIN_MEAS clocks when not used, for saving dynamic power consumption.
// -----------------------------------------------------------------------------
void clk_off_vpu_vdin_meas (void)
{
    //stimulus_print("[TEST.C] Clock off VPU VDIN_MEAS\n");
    Wr_reg_bits(HHI_VDIN_MEAS_CLK_CNTL, 0,  8,  1); // Turn off vdin_meas_clk
} /* clk_off_vpu_vdin_meas */

// -----------------------------------------------------------------------------
//                     Function: clk_off_vpu_enci
// Turn off ENCI clocks when not used, for saving dynamic power consumption.
// -----------------------------------------------------------------------------
void clk_off_vpu_enci (void)
{
    //stimulus_print("[TEST.C] Clock off VPU ENCI\n");
    Wr_reg_bits(HHI_GCLK_MPEG2,     0,      16, 1); // Turn off u_venci_int.clk
    Wr_reg_bits(HHI_GCLK_OTHER,     0,      8,  1); // Turn off u_venci_int.vclk
    Wr_reg_bits(HHI_GCLK_MPEG0,     0,      24, 1); // Turn off u_venc_i_top.clk81
    Wr_reg_bits(HHI_GCLK_OTHER,     0,      1,  2); // Turn off u_venc_i_top.clk_venc_i
    Wr_reg_bits(HHI_VID_CLK_DIV,    0xc,    28, 4); // Turn off u_clk_rst_tst.cts_enci_clk
} /* clk_off_vpu_enci */

// -----------------------------------------------------------------------------
//                     Function: clk_off_vpu_encp
// Turn off ENCP clocks when not used, for saving dynamic power consumption.
// -----------------------------------------------------------------------------
void clk_off_vpu_encp (void)
{
    //stimulus_print("[TEST.C] Clock off VPU ENCP\n");
    Wr_reg_bits(HHI_GCLK_MPEG2,     0,      18, 1); // Turn off u_vencp_int.clk
    Wr_reg_bits(HHI_GCLK_OTHER,     0,      9,  1); // Turn off u_vencp_int.vclk
    Wr_reg_bits(HHI_GCLK_MPEG0,     0,      25, 1); // Turn off u_venc_p_top.clk81
    Wr_reg_bits(HHI_GCLK_OTHER,     0,      3,  2); // Turn off u_venc_p_top.clk_venc_p
    Wr_reg_bits(HHI_VID_CLK_DIV,    0xc,    24, 4); // Turn off u_clk_rst_tst.cts_encp_clk
} /* clk_off_vpu_encp */

// -----------------------------------------------------------------------------
//                     Function: clk_off_vpu_enct
// Turn off ENCT clocks when not used, for saving dynamic power consumption.
// -----------------------------------------------------------------------------
void clk_off_vpu_enct (void)
{
    //stimulus_print("[TEST.C] Clock off VPU ENCT\n");
    Wr_reg_bits(HHI_GCLK_MPEG2,     0,  19, 1); // Turn off u_venct_int.clk
    Wr_reg_bits(HHI_GCLK_OTHER,     0,  22, 1); // Turn off u_venct_int.vclk
    Wr_reg_bits(HHI_GCLK_MPEG0,     0,  26, 1); // Turn off u_venc_t_top.clk81
    Wr_reg_bits(HHI_GCLK_OTHER,     0,  5,  2); // Turn off u_venc_t_top.clk_venc_t
    Wr_reg_bits(HHI_VID_CLK_DIV,    0xc,    20, 4); // Turn off u_clk_rst_tst.cts_enct_clk
} /* clk_off_vpu_enct */

// -----------------------------------------------------------------------------
//                     Function: clk_off_vpu_encl
// Turn off ENCL clocks when not used, for saving dynamic power consumption.
// -----------------------------------------------------------------------------
void clk_off_vpu_encl (void)
{
    //stimulus_print("[TEST.C] Clock off VPU ENCL\n");
    Wr_reg_bits(HHI_GCLK_MPEG2,         0,  20, 1); // Turn off u_vencl_int.clk
    Wr_reg_bits(HHI_GCLK_OTHER,         0,  23, 1); // Turn off u_vencl_int.vclk
    Wr_reg_bits(HHI_GCLK_MPEG2,         0,  21, 1); // Turn off u_venc_l_top.clk81
    //Wr_reg_bits(HHI_GCLK_OTHER,         0,  24, 2); // Turn off u_venc_l_top.clk_venc_l
    Wr_reg_bits(HHI_GCLK_OTHER,         0,  25, 1); // Turn off u_venc_l_top.clk_venc_l. Note: Don't turn off bit 24, as it clashes with u_mmc_top.enable_mmc_clk_all
    Wr_reg_bits(HHI_VIID_DIVIDER_CNTL,  0,  11, 1); // Turn off u_lvds_phy_top.lvds_phy_clk
    Wr_reg_bits(HHI_VIID_CLK_DIV,       0xc,12, 4); // Turn off u_clk_rst_tst.cts_encl_clk
} /* clk_off_vpu_encl */

// -----------------------------------------------------------------------------
//                     Function: clk_off_vpu_vdac
// Turn off VDAC clocks when not used, for saving dynamic power consumption.
// -----------------------------------------------------------------------------
void clk_off_vpu_vdac (void)
{
    //stimulus_print("[TEST.C] Clock off VPU VDAC\n");
    Wr_reg_bits(HHI_GCLK_MPEG0,         0,  27, 1); // Turn off u_venc_dac_process.clk81
    Wr_reg_bits(HHI_VIID_CLK_DIV,       0xc,    28, 4); // Turn off u_clk_rst_tst.cts_vdac_clk[0]
    Wr_reg_bits(HHI_VIID_CLK_DIV,       0xc,    24, 4); // Turn off u_clk_rst_tst.cts_vdac_clk[1]
} /* clk_off_vpu_vdac */

// -----------------------------------------------------------------------------
//                     Function: clk_off_vpu_lcd_an_clk_ph23
// Turn off lcd_an_clk_ph2 and  lcd_an_clk_ph3 when not used, for saving dynamic power consumption.
// -----------------------------------------------------------------------------
void clk_off_vpu_lcd_an_clk_ph23 (void)
{
    //stimulus_print("[TEST.C] Clock off lcd_an_clk_ph2/3\n");
    Wr_reg_bits(HHI_VID_CLK_CNTL,   0,  14, 1); // Turn off u_clk_rst_tst.lcd_an_clk_ph2/3
} /* clk_off_vpu_lcd_an_clk_ph23 */

/*******************************************************************************************************/
typedef void (*clock_off_func_t)(void);

struct gate_manager {
	char* name;
	clock_off_func_t off_func;
} gate_managers [] = {
	{"vpu",			clk_off_vpu,},
	{"hdmi",		clk_off_hdmi,},
	{"mdec",		clk_off_mdec,},
	{"usb",			clk_off_usb,},
	{"aiu",			clk_off_aiu,},
	//{"ge2d",		clk_off_ge2d,},
	//{"vpu_vpu1",		clk_off_vpu_viu1,}
	//{"vpu_vpu2",		clk_off_vpu_viu2,}
	//{"vpu_vdi1",		clk_off_vpu_vdi1,}
	//{"vpu_vdi6",		clk_off_vpu_vdi6,}
	//{"vpu_vdin0_com",	clk_off_vpu_vdin0_com,}
	//{"vpu_vdin1_com",	clk_off_vpu_vdin1_com,}
	//{"vpu_di_mad_top",	clk_off_vpu_di_mad_top,}
	//{"vpu_vdin_meas",	clk_off_vpu_vdin_meas,}
	//{"vpu_enci",		clk_off_vpu_enci,}
	//{"vpu_encp",		clk_off_vpu_encp,}
	//{"vpu_enct",		clk_off_vpu_enct,}
	//{"vpu_encl",		clk_off_vpu_encl,}
	//{"vpu_vdac",		clk_off_vpu_vdac,}
	//{"vpu_lcd_an_clk_ph23",	clk_off_vpu_lcd_an_clk_ph23,}
};

static ssize_t gate_off_store(struct class *cla, 
		struct class_attribute *attr, char *buf, size_t count)
{
	int i = GCLK_IDX_MAX - 1;
	printk("the buf is %s \n", buf);
	
	if(!strncmp(buf, "all", 3)) {
		while(i >= 0) {
			if (gate_devices[i].can_be_off) {
				printk("power off gate %s \n", gate_devices[i].name);
				CLEAR_CBUS_REG_MASK(gate_devices[i].clock_gate_reg_adr, 
						gate_devices[i].clock_gate_reg_mask);
				msleep(100);
			}
			i--;
		}
	} else {
		i = 0;
		while(i < GCLK_IDX_MAX) {
			if(!strncmp(gate_devices[i].name, buf, \
					(strlen(gate_devices[i].name) > strlen(buf)) ? strlen(buf) \
						: strlen(gate_devices[i].name))) {
				break;
			} else
				i++;
		}
		if (i < GCLK_IDX_MAX ) {
			CLEAR_CBUS_REG_MASK(gate_devices[i].clock_gate_reg_adr, 
							gate_devices[i].clock_gate_reg_mask);
			printk("power off gate %s \n", gate_devices[i].name);
		}
	}
	return strlen(buf);
}	

static ssize_t gate_on_store(struct class *cla, 
		struct class_attribute *attr, char *buf, size_t count)
{
	int i = 0;
	printk("the buf is %s \n", buf);
	
	if(!strncmp(buf, "all", 3)) {
		while(i < GCLK_IDX_MAX) {
			SET_CBUS_REG_MASK(gate_devices[i].clock_gate_reg_adr, 
					gate_devices[i].clock_gate_reg_mask);
			msleep(100);
		}
		i++;
	} else {
		while(i < GCLK_IDX_MAX) {
			if(!strncmp(gate_devices[i].name, buf, \
					(strlen(gate_devices[i].name) > strlen(buf)) ? strlen(buf) \
						: strlen(gate_devices[i].name))) {
				break;
			} else
				i++;
		}
		if (i < GCLK_IDX_MAX ) {
			SET_CBUS_REG_MASK(gate_devices[i].clock_gate_reg_adr, 
							gate_devices[i].clock_gate_reg_mask);
			printk("power on gate %s \n", gate_devices[i].name);
		}
	}
	return strlen(buf);
}	

static ssize_t module_off_store(struct class *cla, 
		struct class_attribute *attr, char *buf, size_t count)
{
	int i = 0;
	int total_num = sizeof(gate_managers)/sizeof(struct gate_manager);
	if (!strncmp(buf, "all", 3)) {
		while(i < total_num) {
			if (gate_managers[i].off_func) {
				gate_managers[i].off_func();
				printk("power off module %s \n", gate_managers[i].name);
			}
			i++;
		}
	} else {
		while(i < total_num) {
			if (!strncmp(gate_managers[i].name, buf, \
				(strlen(gate_managers[i].name) > strlen(buf)) ? strlen(buf) \
						: strlen(gate_managers[i].name))) {
				if (gate_managers[i].off_func) {
					gate_managers[i].off_func();
					printk("power off module %s \n", gate_managers[i].name);
					break;
				}
			}
			i++;
		}
	}
	return 0;	
}

static struct class_attribute aml_power_debug_attrs[] = {
	__ATTR_RO(power_info),
#ifdef GPIO_TEST
	__ATTR(power_off, S_IRWXU, NULL, power_off_store),
#endif
	__ATTR_RO(gate_info),
	__ATTR(gate_off, S_IRWXU, NULL, gate_off_store),
	__ATTR(gate_on, S_IRWXU, NULL, gate_on_store),
	__ATTR(module_off, S_IRWXU, NULL, module_off_store),
	__ATTR_NULL,
};

struct class aml_power_debug_class = {
	.name = "aml_power_debug",
	.class_attrs = aml_power_debug_attrs,
};

static int  __init aml_power_debug_init(void)
{
	int ret;
	ret = class_register(&aml_power_debug_class);
	return ret;
}

static void __exit aml_power_debug_exit(void)
{
       return class_unregister(&aml_power_debug_class);
}

module_init(aml_power_debug_init);
module_exit(aml_power_debug_exit);