#include <mach/card_io.h>
#include <linux/cardreader/card_block.h>
#include <linux/cardreader/cardreader.h>
#include <plat/regops.h>
#include <mach/pinmux.h>

static unsigned sd_backup_input_val = 0;
static unsigned sd_backup_output_val = 0;
static unsigned SD_BAKUP_INPUT_REG = (unsigned)&sd_backup_input_val;
static unsigned SD_BAKUP_OUTPUT_REG = (unsigned)&sd_backup_output_val;

unsigned SD_CMD_OUTPUT_EN_REG;
unsigned SD_CMD_OUTPUT_EN_MASK;
unsigned SD_CMD_INPUT_REG;
unsigned SD_CMD_INPUT_MASK;
unsigned SD_CMD_OUTPUT_REG;
unsigned SD_CMD_OUTPUT_MASK;

unsigned SD_CLK_OUTPUT_EN_REG;
unsigned SD_CLK_OUTPUT_EN_MASK;
unsigned SD_CLK_OUTPUT_REG;
unsigned SD_CLK_OUTPUT_MASK;

unsigned SD_DAT_OUTPUT_EN_REG;
unsigned SD_DAT0_OUTPUT_EN_MASK;
unsigned SD_DAT0_3_OUTPUT_EN_MASK;
unsigned SD_DAT_INPUT_REG;
unsigned SD_DAT_OUTPUT_REG;
unsigned SD_DAT0_INPUT_MASK;
unsigned SD_DAT0_OUTPUT_MASK;
unsigned SD_DAT0_3_INPUT_MASK;
unsigned SD_DAT0_3_OUTPUT_MASK;
unsigned SD_DAT_INPUT_OFFSET;
unsigned SD_DAT_OUTPUT_OFFSET;

unsigned SD_INS_OUTPUT_EN_REG;
unsigned SD_INS_OUTPUT_EN_MASK;
unsigned SD_INS_INPUT_REG;
unsigned SD_INS_INPUT_MASK;

unsigned SD_WP_OUTPUT_EN_REG;
unsigned SD_WP_OUTPUT_EN_MASK;
unsigned SD_WP_INPUT_REG;
unsigned SD_WP_INPUT_MASK;

unsigned SD_PWR_OUTPUT_EN_REG;
unsigned SD_PWR_OUTPUT_EN_MASK;
unsigned SD_PWR_OUTPUT_REG;
unsigned SD_PWR_OUTPUT_MASK;
unsigned SD_PWR_EN_LEVEL;

unsigned SD_WORK_MODE;

extern int using_sdxc_controller;
void sd_io_init(struct memory_card *card)
{
	struct aml_card_info *aml_card_info = card->card_plat_info;
	SD_WORK_MODE = aml_card_info->work_mode;

	if ((aml_card_info->io_pad_type == SDXC_CARD_0_5) ||
		(aml_card_info->io_pad_type == SDXC_BOOT_0_11) ||
		(aml_card_info->io_pad_type == SDXC_GPIOX_0_9))
		using_sdxc_controller = 1;

	switch (aml_card_info->io_pad_type) {

		case SDHC_CARD_0_5:		//SDHC-B
			SD_CMD_OUTPUT_EN_REG = CARD_GPIO_ENABLE;
			SD_CMD_OUTPUT_EN_MASK = PREG_IO_28_MASK;
			SD_CMD_OUTPUT_REG = CARD_GPIO_OUTPUT;
			SD_CMD_OUTPUT_MASK = PREG_IO_28_MASK;
			SD_CMD_INPUT_REG = CARD_GPIO_INPUT;
			SD_CMD_INPUT_MASK = PREG_IO_28_MASK;

			SD_CLK_OUTPUT_EN_REG = CARD_GPIO_ENABLE;
			SD_CLK_OUTPUT_EN_MASK = PREG_IO_27_MASK;
			SD_CLK_OUTPUT_REG = CARD_GPIO_OUTPUT;
			SD_CLK_OUTPUT_MASK = PREG_IO_27_MASK;


			SD_DAT_OUTPUT_EN_REG = CARD_GPIO_ENABLE;
			SD_DAT0_OUTPUT_EN_MASK = PREG_IO_23_MASK;
			SD_DAT0_3_OUTPUT_EN_MASK = PREG_IO_23_26_MASK;

			SD_DAT_OUTPUT_REG = CARD_GPIO_OUTPUT;
			SD_DAT0_OUTPUT_MASK = PREG_IO_23_MASK;
			SD_DAT0_3_OUTPUT_MASK = PREG_IO_23_26_MASK;
			SD_DAT_OUTPUT_OFFSET = 23;

			SD_DAT_INPUT_REG = CARD_GPIO_INPUT;
			SD_DAT0_INPUT_MASK = PREG_IO_23_MASK;
			SD_DAT0_3_INPUT_MASK = PREG_IO_23_26_MASK;
			SD_DAT_INPUT_OFFSET = 23;
			break;

		case SDHC_BOOT_0_11:		//SDHC-C
			SD_CMD_OUTPUT_EN_REG = BOOT_GPIO_ENABLE;
			SD_CMD_OUTPUT_EN_MASK = PREG_IO_10_MASK;
			SD_CMD_OUTPUT_REG = BOOT_GPIO_OUTPUT;
			SD_CMD_OUTPUT_MASK = PREG_IO_10_MASK;
			SD_CMD_INPUT_REG = BOOT_GPIO_INPUT;
			SD_CMD_INPUT_MASK = PREG_IO_10_MASK;

			SD_CLK_OUTPUT_EN_REG = BOOT_GPIO_ENABLE;
			SD_CLK_OUTPUT_EN_MASK = PREG_IO_11_MASK;
			SD_CLK_OUTPUT_REG = BOOT_GPIO_OUTPUT;
			SD_CLK_OUTPUT_MASK = PREG_IO_11_MASK;


			SD_DAT_OUTPUT_EN_REG = BOOT_GPIO_ENABLE;
			SD_DAT0_OUTPUT_EN_MASK = PREG_IO_0_MASK;
			SD_DAT0_3_OUTPUT_EN_MASK = PREG_IO_0_3_MASK;

			SD_DAT_OUTPUT_REG = BOOT_GPIO_OUTPUT;
			SD_DAT0_OUTPUT_MASK = PREG_IO_0_MASK;
			SD_DAT0_3_OUTPUT_MASK = PREG_IO_0_3_MASK;
			SD_DAT_OUTPUT_OFFSET = 0;

			SD_DAT_INPUT_REG = BOOT_GPIO_INPUT;
			SD_DAT0_INPUT_MASK = PREG_IO_0_MASK;
			SD_DAT0_3_INPUT_MASK = PREG_IO_0_3_MASK;
			SD_DAT_INPUT_OFFSET = 0;
			break;

		case SDHC_GPIOX_0_9:		//SDHC-A
			SD_CMD_OUTPUT_EN_REG = EGPIO_GPIOXL_ENABLE;
			SD_CMD_OUTPUT_EN_MASK = PREG_IO_9_MASK;
			SD_CMD_OUTPUT_REG = EGPIO_GPIOXL_OUTPUT;
			SD_CMD_OUTPUT_MASK = PREG_IO_9_MASK;
			SD_CMD_INPUT_REG = EGPIO_GPIOXL_INPUT;
			SD_CMD_INPUT_MASK = PREG_IO_9_MASK;

			SD_CLK_OUTPUT_EN_REG = EGPIO_GPIOXL_ENABLE;
			SD_CLK_OUTPUT_EN_MASK = PREG_IO_8_MASK;
			SD_CLK_OUTPUT_REG = EGPIO_GPIOXL_OUTPUT;
			SD_CLK_OUTPUT_MASK = PREG_IO_8_MASK;


			SD_DAT_OUTPUT_EN_REG = EGPIO_GPIOXL_ENABLE;
			SD_DAT0_OUTPUT_EN_MASK = PREG_IO_0_MASK;
			SD_DAT0_3_OUTPUT_EN_MASK = PREG_IO_0_3_MASK;

			SD_DAT_OUTPUT_REG = EGPIO_GPIOXL_OUTPUT;
			SD_DAT0_OUTPUT_MASK = PREG_IO_0_MASK;
			SD_DAT0_3_OUTPUT_MASK = PREG_IO_0_3_MASK;
			SD_DAT_OUTPUT_OFFSET = 0;

			SD_DAT_INPUT_REG = EGPIO_GPIOXL_INPUT;
			SD_DAT0_INPUT_MASK = PREG_IO_0_MASK;
			SD_DAT0_3_INPUT_MASK = PREG_IO_0_3_MASK;
			SD_DAT_INPUT_OFFSET = 0;
			break;

		case SDXC_CARD_0_5:		//SDXC-B
			SD_CMD_OUTPUT_EN_REG = CARD_GPIO_ENABLE;
			SD_CMD_OUTPUT_EN_MASK = PREG_IO_28_MASK;
			SD_CMD_OUTPUT_REG = CARD_GPIO_OUTPUT;
			SD_CMD_OUTPUT_MASK = PREG_IO_28_MASK;
			SD_CMD_INPUT_REG = CARD_GPIO_INPUT;
			SD_CMD_INPUT_MASK = PREG_IO_28_MASK;

			SD_CLK_OUTPUT_EN_REG = CARD_GPIO_ENABLE;
			SD_CLK_OUTPUT_EN_MASK = PREG_IO_27_MASK;
			SD_CLK_OUTPUT_REG = CARD_GPIO_OUTPUT;
			SD_CLK_OUTPUT_MASK = PREG_IO_27_MASK;


			SD_DAT_OUTPUT_EN_REG = CARD_GPIO_ENABLE;
			SD_DAT0_OUTPUT_EN_MASK = PREG_IO_23_MASK;
			SD_DAT0_3_OUTPUT_EN_MASK = PREG_IO_23_26_MASK;

			SD_DAT_OUTPUT_REG = CARD_GPIO_OUTPUT;
			SD_DAT0_OUTPUT_MASK = PREG_IO_23_MASK;
			SD_DAT0_3_OUTPUT_MASK = PREG_IO_23_26_MASK;
			SD_DAT_OUTPUT_OFFSET = 23;

			SD_DAT_INPUT_REG = CARD_GPIO_INPUT;
			SD_DAT0_INPUT_MASK = PREG_IO_23_MASK;
			SD_DAT0_3_INPUT_MASK = PREG_IO_23_26_MASK;
			SD_DAT_INPUT_OFFSET = 23;
			break;

		case SDXC_BOOT_0_11:		//SDXC-C
			SD_CMD_OUTPUT_EN_REG = BOOT_GPIO_ENABLE;
			SD_CMD_OUTPUT_EN_MASK = PREG_IO_10_MASK;
			SD_CMD_OUTPUT_REG = BOOT_GPIO_OUTPUT;
			SD_CMD_OUTPUT_MASK = PREG_IO_10_MASK;
			SD_CMD_INPUT_REG = BOOT_GPIO_INPUT;
			SD_CMD_INPUT_MASK = PREG_IO_10_MASK;

			SD_CLK_OUTPUT_EN_REG = BOOT_GPIO_ENABLE;
			SD_CLK_OUTPUT_EN_MASK = PREG_IO_11_MASK;
			SD_CLK_OUTPUT_REG = BOOT_GPIO_OUTPUT;
			SD_CLK_OUTPUT_MASK = PREG_IO_11_MASK;


			SD_DAT_OUTPUT_EN_REG = BOOT_GPIO_ENABLE;
			SD_DAT0_OUTPUT_EN_MASK = PREG_IO_0_MASK;
			SD_DAT0_3_OUTPUT_EN_MASK = PREG_IO_0_3_MASK;

			SD_DAT_OUTPUT_REG = BOOT_GPIO_OUTPUT;
			SD_DAT0_OUTPUT_MASK = PREG_IO_0_MASK;
			SD_DAT0_3_OUTPUT_MASK = PREG_IO_0_3_MASK;
			SD_DAT_OUTPUT_OFFSET = 0;

			SD_DAT_INPUT_REG = BOOT_GPIO_INPUT;
			SD_DAT0_INPUT_MASK = PREG_IO_0_MASK;
			SD_DAT0_3_INPUT_MASK = PREG_IO_0_3_MASK;
			SD_DAT_INPUT_OFFSET = 0;
			break;

		case SDXC_GPIOX_0_9:		//SDXC-A
			SD_CMD_OUTPUT_EN_REG = EGPIO_GPIOXL_ENABLE;
			SD_CMD_OUTPUT_EN_MASK = PREG_IO_9_MASK;
			SD_CMD_OUTPUT_REG = EGPIO_GPIOXL_OUTPUT;
			SD_CMD_OUTPUT_MASK = PREG_IO_9_MASK;
			SD_CMD_INPUT_REG = EGPIO_GPIOXL_INPUT;
			SD_CMD_INPUT_MASK = PREG_IO_9_MASK;

			SD_CLK_OUTPUT_EN_REG = EGPIO_GPIOXL_ENABLE;
			SD_CLK_OUTPUT_EN_MASK = PREG_IO_8_MASK;
			SD_CLK_OUTPUT_REG = EGPIO_GPIOXL_OUTPUT;
			SD_CLK_OUTPUT_MASK = PREG_IO_8_MASK;


			SD_DAT_OUTPUT_EN_REG = EGPIO_GPIOXL_ENABLE;
			SD_DAT0_OUTPUT_EN_MASK = PREG_IO_0_MASK;
			SD_DAT0_3_OUTPUT_EN_MASK = PREG_IO_0_3_MASK;

			SD_DAT_OUTPUT_REG = EGPIO_GPIOXL_OUTPUT;
			SD_DAT0_OUTPUT_MASK = PREG_IO_0_MASK;
			SD_DAT0_3_OUTPUT_MASK = PREG_IO_0_3_MASK;
			SD_DAT_OUTPUT_OFFSET = 0;

			SD_DAT_INPUT_REG = EGPIO_GPIOXL_INPUT;
			SD_DAT0_INPUT_MASK = PREG_IO_0_MASK;
			SD_DAT0_3_INPUT_MASK = PREG_IO_0_3_MASK;
			SD_DAT_INPUT_OFFSET = 0;
			break;
        default:
			printk("Warning couldn`t find any valid hw io pad!!!\n");
            break;
	}

	if (aml_card_info->card_ins_en_reg) {
		SD_INS_OUTPUT_EN_REG = aml_card_info->card_ins_en_reg;
		SD_INS_OUTPUT_EN_MASK = aml_card_info->card_ins_en_mask;
		SD_INS_INPUT_REG = aml_card_info->card_ins_input_reg;
		SD_INS_INPUT_MASK = aml_card_info->card_ins_input_mask;
	}
	else {
		SD_INS_OUTPUT_EN_REG = SD_BAKUP_OUTPUT_REG;
		SD_INS_OUTPUT_EN_MASK = 1;
		SD_INS_INPUT_REG = SD_BAKUP_INPUT_REG;
		SD_INS_INPUT_MASK =
		SD_WP_INPUT_MASK = 1;
	}

	if (aml_card_info->card_power_en_reg) {
		SD_PWR_OUTPUT_EN_REG = aml_card_info->card_power_en_reg;
		SD_PWR_OUTPUT_EN_MASK = aml_card_info->card_power_en_mask;
		SD_PWR_OUTPUT_REG = aml_card_info->card_power_output_reg;
		SD_PWR_OUTPUT_MASK = aml_card_info->card_power_output_mask;
		SD_PWR_EN_LEVEL = aml_card_info->card_power_en_lev;
	}
	else {
		SD_PWR_OUTPUT_EN_REG = SD_BAKUP_OUTPUT_REG;
		SD_PWR_OUTPUT_EN_MASK = 1;
		SD_PWR_OUTPUT_REG = SD_BAKUP_OUTPUT_REG;
		SD_PWR_OUTPUT_MASK = 1;
		SD_PWR_EN_LEVEL = 0;
	}

	if (aml_card_info->card_wp_en_reg) {
		SD_WP_OUTPUT_EN_REG = aml_card_info->card_wp_en_reg;
		SD_WP_OUTPUT_EN_MASK = aml_card_info->card_wp_en_mask;
		SD_WP_INPUT_REG = aml_card_info->card_wp_input_reg;
		SD_WP_INPUT_MASK = aml_card_info->card_wp_input_mask;
	}
	else {
		SD_WP_OUTPUT_EN_REG = SD_BAKUP_OUTPUT_REG;
		SD_WP_OUTPUT_EN_MASK = 1;
		SD_WP_INPUT_REG = SD_BAKUP_INPUT_REG;
		SD_WP_INPUT_MASK = 1;
	}

	return;
}

//do nothing
static bool card_pinmux_dummy(bool select)
{
	return true;
}

static pinmux_item_t SDHC_CARD_0_5_pins[] = {
    {
        .reg = PINMUX_REG(2),
        .setmask = 0x3f << 10,
    },
    PINMUX_END_ITEM
};
static pinmux_set_t SDHC_CARD_0_5_set = {
    .chip_select = card_pinmux_dummy,
    .pinmux = &SDHC_CARD_0_5_pins[0]
};


static pinmux_item_t SDHC_BOOT_0_11_pins[] = {
    {
        .reg = PINMUX_REG(6),
        .setmask = 0x3F<<24,
    },
    PINMUX_END_ITEM
};
static pinmux_set_t SDHC_BOOT_0_11_set = {
    .chip_select = card_pinmux_dummy,
    .pinmux = &SDHC_BOOT_0_11_pins[0]
};


static pinmux_item_t SDHC_GPIOX_0_9_pins[] = {
    {
        .reg = PINMUX_REG(8),
        .setmask = 0x3f,
    },
    PINMUX_END_ITEM
};
static pinmux_set_t SDHC_GPIOX_0_9_set = {
    .chip_select = card_pinmux_dummy,
    .pinmux = &SDHC_GPIOX_0_9_pins[0]
};


static pinmux_item_t SDXC_CARD_0_5_pins[] = {
    {
        .reg = PINMUX_REG(2),
        .setmask = 0xf << 4,
    },
    PINMUX_END_ITEM
};
static pinmux_set_t SDXC_CARD_0_5_set = {
    .chip_select = card_pinmux_dummy,
    .pinmux = &SDXC_CARD_0_5_pins[0]
};


static pinmux_item_t SDXC_BOOT_0_11_pins[] = {
    {
        .reg = PINMUX_REG(4),
        .setmask = 0x1f << 26,
    },
    PINMUX_END_ITEM
};
static pinmux_set_t SDXC_BOOT_0_11_set = {
    .chip_select = card_pinmux_dummy,
    .pinmux = &SDXC_BOOT_0_11_pins[0]
};


static pinmux_item_t SDXC_GPIOX_0_9_pins[] = {
    {
        .reg = PINMUX_REG(5),
        .setmask = 0x1f << 10,
    },
    PINMUX_END_ITEM
};
static pinmux_set_t SDXC_GPIOX_0_9_set = {
    .chip_select = card_pinmux_dummy,
    .pinmux = &SDXC_GPIOX_0_9_pins[0]
};


void sd_sdio_enable(SDIO_Pad_Type_t io_pad_type)
{
	switch (io_pad_type) {

		case SDHC_CARD_0_5 :	//SDHC-B
			pinmux_set(&SDHC_CARD_0_5_set);
			SET_CBUS_REG_MASK(SDIO_MULT_CONFIG, (1));
			break;

		case SDHC_BOOT_0_11 :	//SDHC-C
			pinmux_set(&SDHC_BOOT_0_11_set);
			SET_CBUS_REG_MASK(SDIO_MULT_CONFIG, (2));
			break;

		case SDHC_GPIOX_0_9 :	//SDHC-A
			pinmux_set(&SDHC_GPIOX_0_9_set);
			SET_CBUS_REG_MASK(SDIO_MULT_CONFIG, (0));
			break;

		case SDXC_CARD_0_5 :	//SDXC-B
			pinmux_set(&SDXC_CARD_0_5_set);
			break;

		case SDXC_BOOT_0_11 :	//SDXC-C
			pinmux_set(&SDXC_BOOT_0_11_set);
			break;

		case SDXC_GPIOX_0_9 :	//SDXC-A
			pinmux_set(&SDXC_GPIOX_0_9_set);
			break;

		default :
			printk("invalid hw io pad!!!\n");
			break;
	}

	return;
}

void sd_gpio_enable(SDIO_Pad_Type_t io_pad_type)
{
	switch (io_pad_type) {

		case SDHC_CARD_0_5 :	//SDHC-B
			pinmux_clr(&SDHC_CARD_0_5_set);
			CLEAR_CBUS_REG_MASK(SDIO_MULT_CONFIG, (1));
			break;

		case SDHC_BOOT_0_11 :	//SDHC-C
			pinmux_clr(&SDHC_BOOT_0_11_set);
			CLEAR_CBUS_REG_MASK(SDIO_MULT_CONFIG, (2));
			break;

		case SDHC_GPIOX_0_9 :	//SDHC-A
			pinmux_clr(&SDHC_GPIOX_0_9_set);
			CLEAR_CBUS_REG_MASK(SDIO_MULT_CONFIG, (0));
			break;

		case SDXC_CARD_0_5 :	//SDXC-B
			pinmux_clr(&SDXC_CARD_0_5_set);
			break;

		case SDXC_BOOT_0_11 :	//SDXC-C
			pinmux_clr(&SDXC_BOOT_0_11_set);
			break;

		case SDXC_GPIOX_0_9 :	//SDXC-A
			pinmux_clr(&SDXC_GPIOX_0_9_set);
			break;

		default :
			printk("invalid hw io pad!!!\n");
			break;
	}

	return;
}
