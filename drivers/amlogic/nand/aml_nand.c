// linux/drivers/amlogic/nand/aml_nand.c


#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/bitops.h>
#include <linux/crc32.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/mtd/partitions.h>

#include <mach/nand.h>
/*
 * CONFIG_SYS_NAND_RESET_CNT is used as a timeout mechanism when resetting
 * a flash.  NAND flash is initialized prior to interrupts so standard timers
 * can't be used.  CONFIG_SYS_NAND_RESET_CNT should be set to a value
 * which is greater than (max NAND reset time / NAND status read time).
 * A conservative default of 200000 (500 us / 25 ns) is used as a default.
 */
#ifndef CONFIG_SYS_NAND_RESET_CNT
#define CONFIG_SYS_NAND_RESET_CNT 200000
#endif


#define NAND_DEBUG

#ifdef NAND_DEBUG

#define aml_nand_debug(a...) {printk("%s()[%s,%d]",__func__,__FILE__,__LINE__); printk(a);}
#define aml_nand_debug2(a...)  //printk(a)
#else
#define aml_nand_debug(a...) 
#define aml_nand_debug2(a...) 
#endif
static char *aml_nand_bch_string[]={
	"NAND_SOFT_MODE",
	"NAND_BCH9_MODE",
	"NAND_BCH8_MODE",
	"NAND_BCH12_MODE",
	"NAND_BCH16_MODE",
};

static char *aml_nand_plane_string[]={
	"NAND_SINGLE_PLANE_MODE",
	"NAND_TWO_PLANE_MODE",
};

static char *aml_nand_internal_string[]={
	"NAND_NONE_INTERLEAVING_MODE",
	"NAND_INTERLEAVING_MODE",
};

static struct nand_ecclayout aml_nand_oob_64 = {
	.eccbytes = 60,
	.eccpos = {
		   4, 5, 6, 7, 8, 9, 10, 11,
		   12, 13, 14, 15, 16, 17, 18, 19,
		   20, 21, 22, 23, 24, 25, 26, 27,
		   28, 29, 30, 31, 32, 33, 34, 35,
		   36, 37, 38, 39, 40, 41, 42, 43,
		   44, 45, 46, 47, 48, 49, 50, 51,
		   52, 53, 54, 55, 56, 57, 58, 59,
		   60, 61, 62, 63},
	.oobfree = {
		{.offset = 0,
		 .length = 4}}
};

static struct nand_ecclayout aml_nand_uboot_oob = {
	.eccbytes = 84,
	.oobfree = {
		{.offset = 0,
		 .length = 6}}
};

static struct nand_ecclayout aml_nand_oob_64_2info = {
	.eccbytes = 56,
	.oobfree = {
		{.offset = 0,
		 .length = 8}}
};

static struct nand_ecclayout aml_nand_oob_128 = {
	.eccbytes = 120,
	.oobfree = {
		{.offset = 0,
		 .length = 8}}
};

static struct nand_ecclayout aml_nand_oob_218 = {
	.eccbytes = 200,
	.oobfree = {
		{.offset = 0,
		 .length = 8}}
};

static struct nand_ecclayout aml_nand_oob_224 = {
	.eccbytes = 208,
	.oobfree = {
		{.offset = 0,
		 .length = 8}}
};

static struct nand_ecclayout aml_nand_oob_256 = {
	.eccbytes = 240,
	.oobfree = {
		{.offset = 0,
		 .length = 16}}
};

static struct nand_ecclayout aml_nand_oob_376 = {
	.eccbytes = 352,
	.oobfree = {
		{.offset = 0,
		 .length = 16}}
};

static struct nand_ecclayout aml_nand_oob_436 = {
	.eccbytes = 352,
	.oobfree = {
		{.offset = 0,
		 .length = 16}}
};

static struct nand_ecclayout aml_nand_oob_448 = {
	.eccbytes = 416,
	.oobfree = {
		{.offset = 0,
		 .length = 16}}
};

static struct nand_ecclayout aml_nand_oob_640 = {
	.eccbytes = 608,
	.oobfree = {
		{.offset = 0,
		 .length = 16}}
};

static struct nand_ecclayout aml_nand_oob_744 = {
	.eccbytes = 700,
	.oobfree = {
		{.offset = 0,
		 .length = 32}}
};

static struct nand_ecclayout aml_nand_oob_1280 = {
	.eccbytes = 1200,
	.oobfree = {
		{.offset = 0,
		 .length = 32}}
};
#if  0
#define aml_nand_debug(a...) {printk("%s()[%s,%d]",__func__,__FILE__,__LINE__); printk(a);}
#else
#define aml_nand_debug(a...) 
#endif
static unsigned default_environment_size = (ENV_SIZE - sizeof(struct aml_nand_bbt_info));
static uint8_t nand_boot_flag = 0;
static uint8_t nand_erarly_suspend_flag = 0;
static uint8_t nand_mode_time[6] = {9, 7, 6, 5, 5, 4};
extern ssize_t show_nand_version_info(struct class *class,
			struct class_attribute *attr,	char *buf);
static int aml_nand_update_env(struct mtd_info *mtd);
//static void aml_nand_cmdfunc(struct mtd_info *mtd, unsigned command, int column, int page_addr);

struct aml_nand_flash_dev aml_nand_flash_ids[] = {

	{"A revision NAND 2GiB H27UAG8T2A",	{NAND_MFR_HYNIX, 0xd5, 0x94, 0x25, 0x44, 0x41}, 4096, 2048, 0x80000, 224, 1, 20, 15, 0, (NAND_TIMING_MODE5 | NAND_ECC_BCH16_MODE | NAND_TWO_PLANE_MODE)},
	{"A revision NAND 4GiB H27UBG8T2A",	{NAND_MFR_HYNIX, 0xd7, 0x94, 0x9a, 0x74, 0x42}, 8192, 4096, 0x200000, 448, 1, 20, 15, 0, (NAND_TIMING_MODE5 | NAND_ECC_BCH16_MODE | NAND_TWO_PLANE_MODE)},
	{"B revision NAND 2GiB H27UAG8T2B",	{NAND_MFR_HYNIX, 0xd5, 0x94, 0x9a, 0x74, 0x42}, 8192, 2048, 0x200000, 448, 1, 20, 15, 0, (NAND_TIMING_MODE5 | NAND_ECC_BCH16_MODE | NAND_TWO_PLANE_MODE)},
#ifdef NEW_NAND_SUPPORT
	{"B revision 26nm NAND 4GiB H27UBG8T2B",	{NAND_MFR_HYNIX, 0xd7, 0x94, 0xda, 0x74, 0xc3}, 8192, 4096, 0x200000, 640, 1, 16, 15, 0, (NAND_TIMING_MODE5 | NAND_ECC_BCH16_MODE)},	//need readretry, disable two plane mode
   	{"B revision 26nm NAND 8GiB H27UCG8T2M",	{NAND_MFR_HYNIX, 0xde, 0x94, 0xd2, 0x04, 0x43}, 8192, 8192, 0x200000, 448, 1, 16, 15, 0, (NAND_TIMING_MODE5 | NAND_ECC_BCH16_MODE )},	//need readretry, disable two plane mode
	{"C revision 20nm NAND 4GiB H27UBG8T2C", {NAND_MFR_HYNIX, 0xd7, 0x94, 0x91, 0x60, 0x44}, 8192, 4096, 0x200000, 640, 1, 16, 15, 0, (NAND_TIMING_MODE5 | NAND_ECC_BCH16_MODE)},	//need readretry, disable two plane mode
	{"A revision 20nm NAND 8GiB H27UCG8T2A", {NAND_MFR_HYNIX, 0xde, 0x94, 0xda, 0x74, 0xc4}, 8192, 8192, 0x200000, 640, 1, 16, 15, 0, (NAND_TIMING_MODE5 | NAND_ECC_BCH16_MODE )},	//need readretry, disable two plane mode
	{"B revision 20nm NAND 8GiB H27UCG8T2B",	{NAND_MFR_HYNIX, 0xde, 0x94, 0xeb, 0x74, 0x44}, 16384, 8192, 0x400000, 1280, 1, 16, 15, 0, (NAND_TIMING_MODE5 | NAND_ECC_BCH16_MODE)},	//need readretry, disable two plane mode

#endif
#ifdef NEW_NAND_SUPPORT
	{"B revision NAND 8GiB MT29F64G08CBABA", {NAND_MFR_MICRON, 0x64, 0x44, 0x4B, 0xA9}, 8192, 8192, 0x200000, 744, 1, 16, 15, 5, (NAND_TIMING_MODE5 | NAND_ECC_BCH16_MODE )}, 
	{"D revision NAND 4GiB MT29F32G08CBADA", {NAND_MFR_MICRON, 0x44, 0x44, 0x4B, 0xA9}, 8192, 4096, 0x200000, 744, 1, 16, 15, 5, (NAND_TIMING_MODE5 | NAND_ECC_BCH16_MODE )}, 
#endif
	{"A revision NAND 4GiB MT29F32G-A", {NAND_MFR_MICRON, 0xd7, 0x94, 0x3e, 0x84}, 4096, 4096, 0x80000, 218, 1, 16, 15, 0, (NAND_TIMING_MODE5 | NAND_ECC_BCH12_MODE | NAND_TWO_PLANE_MODE)},
	{"A revision NAND 16GiB MT29F128G-A", {NAND_MFR_MICRON, 0xd9, 0xd5, 0x3e, 0x88}, 4096, 16384, 0x80000, 218, 1, 16, 15, 0, (NAND_TIMING_MODE5 | NAND_ECC_BCH12_MODE | NAND_TWO_PLANE_MODE)},
	{"B revision NAND 4GiB MT29F32G-B", {NAND_MFR_MICRON, 0x68, 0x04, 0x46, 0x89}, 4096, 4096, 0x100000, 224, 1, 20, 15, 4, (NAND_TIMING_MODE5 | NAND_ECC_BCH16_MODE | NAND_TWO_PLANE_MODE)},
	{"B revision NAND 8GiB MT29F64G-B", {NAND_MFR_MICRON, 0x88, 0x05, 0xc6, 0x89}, 4096, 8192, 0x100000, 224, 1, 20, 15, 4, (NAND_TIMING_MODE5 | NAND_ECC_BCH16_MODE | NAND_TWO_PLANE_MODE)},
	{"C revision NAND 4GiB MT29F32G-C", {NAND_MFR_MICRON, 0x68, 0x04, 0x4a, 0xa9}, 4096, 4096, 0x100000, 224, 1, 16, 15, 5, (NAND_TIMING_MODE5 | NAND_ECC_BCH16_MODE | NAND_TWO_PLANE_MODE)},
	{"C revision NAND 8GiB MT29F64G-C", {NAND_MFR_MICRON, 0x88, 0x04, 0x4b, 0xa9}, 8192, 8192, 0x200000, 448, 1, 16, 15, 5, (NAND_TIMING_MODE5 | NAND_ECC_BCH30_1K_MODE | NAND_TWO_PLANE_MODE)},
	{"C revision NAND 32GiB MT29F256G-C", {NAND_MFR_MICRON, 0xa8, 0x05, 0xcb, 0xa9}, 8192, 32768, 0x200000, 448, 2, 16, 15, 5, (NAND_TIMING_MODE5 | NAND_ECC_BCH16_MODE | NAND_TWO_PLANE_MODE | NAND_INTERLEAVING_MODE)},

	{"1 Generation NAND 4GiB JS29F32G08AA-1", {NAND_MFR_INTEL, 0x68, 0x04, 0x46, 0xA9}, 4096, 4096, 0x100000, 218, 1, 20, 15, 0, (NAND_TIMING_MODE5 | NAND_ECC_BCH12_MODE | NAND_TWO_PLANE_MODE)},
	{"1 Generation NAND 8GiB JS29F64G08AA-1", {NAND_MFR_INTEL, 0x88, 0x24, 0x4b, 0xA9}, 8192, 8192, 0x200000, 448, 1, 20, 15, 0, (NAND_TIMING_MODE5 | NAND_ECC_BCH16_MODE | NAND_TWO_PLANE_MODE)},

	{"E serials NAND 2GiB TC58NVG4D2ETA00", {NAND_MFR_TOSHIBA, 0xD5, 0x94, 0x32, 0x76, 0x54}, 8192, 2048, 0x100000, 376, 1, 20, 25, 0, (NAND_TIMING_MODE5 | NAND_ECC_BCH12_MODE | NAND_TWO_PLANE_MODE)},
	{"E serials NAND 4GiB TC58NVG5D2ETA00", {NAND_MFR_TOSHIBA, 0xD7, 0x94, 0x32, 0x76, 0x54}, 8192, 4096, 0x100000, 376, 1, 20, 25, 0, (NAND_TIMING_MODE5 | NAND_ECC_BCH12_MODE | NAND_TWO_PLANE_MODE)},
	{"F serials NAND 2GiB TC58NVG4D2FTA00", {NAND_MFR_TOSHIBA, 0xD5, 0x94, 0x32, 0x76, 0x55}, 8192, 2076, 0x100000, 448, 1, 20, 25, 0, (NAND_TIMING_MODE5 | NAND_ECC_BCH16_MODE | NAND_TWO_PLANE_MODE)},
	{"F serials NAND 4GiB TC58NVG5D2FTA00", {NAND_MFR_TOSHIBA, 0xD7, 0x94, 0x32, 0x76, 0x55}, 8192, 4096, 0x100000, 448, 1, 20, 25, 0, (NAND_TIMING_MODE5 | NAND_ECC_BCH16_MODE | NAND_TWO_PLANE_MODE)},
	{"F serials NAND 8GiB TC58NVG6D2FTA00", {NAND_MFR_TOSHIBA, 0xDE, 0x94, 0x32, 0x76, 0x55}, 8192, 8192, 0x100000, 448, 1, 20, 25, 0, (NAND_TIMING_MODE5 | NAND_ECC_BCH16_MODE | NAND_TWO_PLANE_MODE)},
	{"F serials NAND 8GiB TH58NVG7D2FTA20", {NAND_MFR_TOSHIBA, 0xDE, 0x95, 0x32, 0x7a, 0x55}, 8192, 8200, 0x100000, 448, 2, 20, 25, 0, (NAND_TIMING_MODE5 | NAND_ECC_BCH16_MODE | NAND_TWO_PLANE_MODE | NAND_INTERLEAVING_MODE)},
#ifdef NEW_NAND_SUPPORT
	{"F serials NAND 4GiB TC58NVG5D2HTA00", {NAND_MFR_TOSHIBA, 0xD7, 0x94, 0x32, 0x76, 0x56}, 8192, 4096, 0x100000, 640, 1, 20, 25, 0, (NAND_TIMING_MODE5 | NAND_ECC_BCH16_MODE )},	//need readretry, disable two plane mode
	{"F serials NAND 8GiB TC58NVG6D2GTA00", {NAND_MFR_TOSHIBA, 0xDE, 0x94, 0x82, 0x76, 0x56}, 8192, 8192, 0x200000, 640, 1, 20, 25, 0, (NAND_TIMING_MODE5 | NAND_ECC_BCH16_MODE )},	//need readretry, disable two plane mode
	{"F serials NAND 8GiB TC58TEG6DCJTA00",  {NAND_MFR_TOSHIBA, 0xDE, 0x84, 0x93, 0x72, 0x57}, 16384, 8192, 0x400000, 1280, 1, 20, 25, 0, (NAND_TIMING_MODE5 | NAND_ECC_BCH16_MODE )},  //need readretry, disable two plane mode
	{"A serials NAND 4GiB TC58TEG5DCJTA00 ", {NAND_MFR_TOSHIBA, 0xD7, 0x84, 0x93, 0x72, 0x57}, 16384, 4096, 0x400000, 1280, 1, 20, 25, 0, (NAND_TIMING_MODE5 | NAND_ECC_BCH16_MODE )},

#endif
#ifdef NEW_NAND_SUPPORT
	{"A serials NAND 8GiB SDTNQGAMA-008G ", {NAND_MFR_SANDISK, 0xDE, 0x94, 0x93, 0x76, 0x57}, 16384, 8192, 0x400000, 1280, 1, 20, 25, 0, (NAND_TIMING_MODE5 | NAND_ECC_BCH16_MODE )},	
	{"A serials NAND 4GiB SDTNQGAMA-004G ", {NAND_MFR_SANDISK, 0xD7, 0x84, 0x93, 0x72, 0x57}, 16384, 4096, 0x400000, 1280, 1, 20, 25, 0, (NAND_TIMING_MODE5 | NAND_ECC_BCH16_MODE )},
	{"A serials NAND 8GiB SDTNPMAHEM-008G ", {NAND_MFR_SANDISK, 0xDE, 0xA4, 0x82, 0x76, 0x56}, 8192, 8192, 0x200000, 640, 1, 20, 25, 0, (NAND_TIMING_MODE5 | NAND_ECC_BCH16_MODE )},
#endif

	{"M Generation NAND 2GiB K9GAG08U0M", {NAND_MFR_SAMSUNG, 0xD5, 0x14, 0xb6, 0x74}, 4096, 2048, 0x80000, 128, 1, 20, 15, 0, (NAND_TIMING_MODE5 | NAND_ECC_BCH8_MODE)},
	{"5 Generation NAND 2GiB K9GAG08X0D", {NAND_MFR_SAMSUNG, 0xD5, 0x94, 0x29, 0x34, 0x41}, 4096, 2048, 0x80000, 218, 1, 20, 15, 0, (NAND_TIMING_MODE5 | NAND_ECC_BCH12_MODE | NAND_TWO_PLANE_MODE)},
	{"6 Generation NAND 2GiB K9GAG08U0E", {NAND_MFR_SAMSUNG, 0xD5, 0x84, 0x72, 0x50, 0x42}, 8192, 2048, 0x100000, 436, 1, 25, 15, 0, (NAND_TIMING_MODE5 | NAND_ECC_BCH12_MODE)},
	{"7 Generation NAND 2GiB K9GAG08U0F", {NAND_MFR_SAMSUNG, 0xD5, 0x94, 0x76, 0x54, 0x43}, 8192, 2048, 0x100000, 512, 1, 25, 15, 0, (NAND_TIMING_MODE5 | NAND_ECC_BCH16_MODE | NAND_TWO_PLANE_MODE)},
	{"6 Generation NAND 4GiB K9LBG08U0E", {NAND_MFR_SAMSUNG, 0xD7, 0xC5, 0x72, 0x54, 0x42}, 8192, 4096, 0x100000, 436, 1, 20, 15, 0, (NAND_TIMING_MODE5 | NAND_ECC_BCH12_MODE | NAND_TWO_PLANE_MODE)},
	{"6 Generation NAND 8GiB K9HCG08U0E", {NAND_MFR_SAMSUNG, 0xDE, 0xC5, 0x72, 0x54, 0x42}, 8192, 8192, 0x100000, 436, 1, 20, 15, 0, (NAND_TIMING_MODE5 | NAND_ECC_BCH12_MODE | NAND_TWO_PLANE_MODE)},
	{"2 Generation NAND 4GiB K9GBG08U0A", {NAND_MFR_SAMSUNG, 0xD7, 0x94, 0x7a, 0x54, 0x43}, 8192, 4152, 0x100000, 640, 1, 20, 15, 0, (NAND_TIMING_MODE5 | NAND_ECC_BCH16_MODE | NAND_TWO_PLANE_MODE)},
	{"2 Generation NAND 8GiB K9LCG08U0A", {NAND_MFR_SAMSUNG, 0xDE, 0xD5, 0x7a, 0x58, 0x43}, 8192, 8304, 0x100000, 640, 2, 20, 15, 0, (NAND_TIMING_MODE5 | NAND_ECC_BCH16_MODE | NAND_TWO_PLANE_MODE | NAND_INTERLEAVING_MODE)},
#ifdef NEW_NAND_SUPPORT
	{"2 Generation NAND 4GiB K9GBG08U0B", {NAND_MFR_SAMSUNG, 0xD7, 0x94, 0x7e, 0x64, 0x44}, 8192, 4096, 0x100000, 640, 1, 20, 15, 0, (NAND_TIMING_MODE5 | NAND_ECC_BCH16_MODE )},
	{"2 Generation NAND 8GiB K9LCG08U0B", {NAND_MFR_SAMSUNG, 0xDE, 0xD5, 0x7e, 0x68, 0x44}, 8192, 8192, 0x100000, 640, 1, 20, 15, 0, (NAND_TIMING_MODE5 | NAND_ECC_BCH16_MODE )},
#endif
	{NULL,}
};

uint8_t aml_nand_get_onfi_features(struct aml_nand_chip *aml_chip,  uint8_t *buf, int addr)
{
	struct nand_chip *chip = &aml_chip->chip;
	struct mtd_info *mtd = &aml_chip->mtd;
	int i, j;

	for (i=0; i<aml_chip->chip_num; i++) {

		if (aml_chip->valid_chip[i]) {

			aml_chip->aml_nand_select_chip(aml_chip, i);
			//aml_chip->aml_nand_command(aml_chip, NAND_CMD_GET_FEATURES, addr, -1, i);
			aml_chip->aml_nand_command(aml_chip, NAND_CMD_GET_FEATURES, -1, -1, i);
			chip->cmd_ctrl(mtd, addr, NAND_CTRL_CHANGE | NAND_NCE | NAND_ALE);
			
			NFC_SEND_CMD_IDLE(aml_chip->chip_selected, 5);

			for (j=0; j<4; j++)
				buf[j] = chip->read_byte(mtd);
		}
	}

	return 0;
}

void aml_nand_set_onfi_features(struct aml_nand_chip *aml_chip,  uint8_t *buf, int addr)
{
	int i, j;
	struct nand_chip *chip = &aml_chip->chip;
	struct mtd_info *mtd = &aml_chip->mtd;
	for (i=0; i<aml_chip->chip_num; i++) {

		if (aml_chip->valid_chip[i]) {

			aml_chip->aml_nand_select_chip(aml_chip, i);
			//aml_chip->aml_nand_command(aml_chip, NAND_CMD_SET_FEATURES, addr, -1, i);
			aml_chip->aml_nand_command(aml_chip, NAND_CMD_SET_FEATURES, -1, -1, i);
			chip->cmd_ctrl(mtd, addr, NAND_CTRL_CHANGE | NAND_NCE | NAND_ALE);
			NFC_SEND_CMD_IDLE(aml_chip->chip_selected, 5);
			for (j=0; j<4; j++)
				aml_chip->aml_nand_write_byte(aml_chip, buf[j]);
			aml_chip->aml_nand_wait_devready(aml_chip, i);
		}
	}
}

static void aml_platform_get_user_byte(struct aml_nand_chip *aml_chip, unsigned char *oob_buf, int byte_num)
{
	int read_times = 0;
	unsigned int len = PER_INFO_BYTE/sizeof(unsigned int);

	while (byte_num > 0) {
		*oob_buf++ = (aml_chip->user_info_buf[read_times*len] & 0xff);
		byte_num--;
		if (aml_chip->user_byte_mode == 2) {
			*oob_buf++ = ((aml_chip->user_info_buf[read_times*len] >> 8) & 0xff);
			byte_num--;
		}
		read_times++;
	}
}

static void aml_platform_set_user_byte(struct aml_nand_chip *aml_chip, unsigned char *oob_buf, int byte_num)
{
	int write_times = 0;
	unsigned int len = PER_INFO_BYTE/sizeof(unsigned int);

	while (byte_num > 0) {
		aml_chip->user_info_buf[write_times*len] = *oob_buf++;
		byte_num--;
		if (aml_chip->user_byte_mode == 2) {
			aml_chip->user_info_buf[write_times*len] |= (*oob_buf++ << 8);
			byte_num--;
		}
		write_times++;
	}
}

#ifdef NEW_NAND_SUPPORT
/*****************************HYNIX******************************************/
uint8_t aml_nand_get_reg_value_hynix(struct aml_nand_chip *aml_chip,  uint8_t *buf, uint8_t *addr, int chipnr, int cnt)
{
	struct nand_chip *chip = &aml_chip->chip;
	struct mtd_info *mtd = &aml_chip->mtd;
	int j;	
	
	if((aml_chip->new_nand_info.type == 0) ||(aml_chip->new_nand_info.type > 10))
		return 0;

	aml_chip->aml_nand_wait_devready(aml_chip, chipnr);
	aml_chip->aml_nand_command(aml_chip, NAND_CMD_HYNIX_GET_VALUE, -1, -1, chipnr);

	for (j=0; j<cnt; j++){
	        chip->cmd_ctrl(mtd, addr[j], NAND_CTRL_CHANGE | NAND_NCE | NAND_ALE);
	        NFC_SEND_CMD_IDLE(aml_chip->chip_selected, 10);
		buf[j] = chip->read_byte(mtd);
		NFC_SEND_CMD_IDLE(aml_chip->chip_selected, 10);
        }
        
        aml_chip->aml_nand_wait_devready(aml_chip, chipnr);

	return 0;
}

uint8_t aml_nand_set_reg_value_hynix(struct aml_nand_chip *aml_chip,  uint8_t *buf, uint8_t *addr, int chipnr, int cnt)
{
	struct nand_chip *chip = &aml_chip->chip;
	struct mtd_info *mtd = &aml_chip->mtd;
	int j;

	if((aml_chip->new_nand_info.type == 0) ||(aml_chip->new_nand_info.type > 10))
		return 0;

	//printk("Enter %s\n", __func__);

	aml_chip->aml_nand_wait_devready(aml_chip, chipnr);
	aml_chip->aml_nand_command(aml_chip, NAND_CMD_HYNIX_SET_VALUE_START, -1, -1, chipnr);
	
	for (j=0; j<cnt; j++){
	        chip->cmd_ctrl(mtd, addr[j], NAND_CTRL_CHANGE | NAND_NCE | NAND_ALE);
		NFC_SEND_CMD_IDLE(aml_chip->chip_selected, 15);
		aml_chip->aml_nand_write_byte(aml_chip, buf[j]);		
		NFC_SEND_CMD_IDLE(aml_chip->chip_selected, 0);
		
	//	printk("%s, REG(0x%x): 	value:0x%x for chip[%d]\n", __func__, addr[j], buf[j], chipnr);
	}

	aml_chip->aml_nand_command(aml_chip, NAND_CMD_HYNIX_SET_VALUE_END, -1, -1, chipnr);

	aml_chip->aml_nand_wait_devready(aml_chip, chipnr);

	return 0;
}

uint8_t aml_nand_get_reg_value_formOTP_hynix(struct aml_nand_chip *aml_chip, int chipnr)
{
	struct nand_chip *chip = &aml_chip->chip;
	struct mtd_info *mtd = &aml_chip->mtd;
	int i, j, k, reg_cnt_otp, total_reg_cnt, check_flag = 0;
	unsigned char *one_copy_buf, *tmp_buf;

	if((aml_chip->new_nand_info.type < 3) ||(aml_chip->new_nand_info.type > 10))
		return 0;


	one_copy_buf = (unsigned char *)kmalloc(256, GFP_KERNEL);
	if(one_copy_buf == NULL){
		printk("%s %d no mem!!!!!\n", __func__, __LINE__);
		BUG();
		return 0;
	}	

	aml_chip->aml_nand_wait_devready(aml_chip, chipnr);
	aml_chip->aml_nand_command(aml_chip, NAND_CMD_RESET, -1, -1, chipnr);
	aml_chip->aml_nand_wait_devready(aml_chip, chipnr);

	aml_chip->aml_nand_command(aml_chip, NAND_CMD_HYNIX_SET_VALUE_START, -1, -1, chipnr);	//send cmd 0x36 
	NFC_SEND_CMD_IDLE(aml_chip->chip_selected, 0);
	
	if(aml_chip->new_nand_info.type == HYNIX_20NM_8GB){
		chip->cmd_ctrl(mtd, 0xff, NAND_CTRL_CHANGE | NAND_NCE | NAND_ALE);			//send 0xff add
		NFC_SEND_CMD_IDLE(aml_chip->chip_selected, 0);
		aml_chip->aml_nand_write_byte(aml_chip,0x40 );								//write 0x40 into 0xff add
		NFC_SEND_CMD_IDLE(aml_chip->chip_selected, 0);
		chip->cmd_ctrl(mtd, 0xcc, NAND_CTRL_CHANGE | NAND_NCE | NAND_ALE);			//send 0xcc add
	}
	else if(aml_chip->new_nand_info.type == HYNIX_20NM_4GB){
		chip->cmd_ctrl(mtd, 0xae, NAND_CTRL_CHANGE | NAND_NCE | NAND_ALE);			//send 0xff add
		NFC_SEND_CMD_IDLE(aml_chip->chip_selected, 0);
		aml_chip->aml_nand_write_byte(aml_chip,0x00 );								//write 0x40 into 0xff add
		NFC_SEND_CMD_IDLE(aml_chip->chip_selected, 0);
		chip->cmd_ctrl(mtd, 0xb0, NAND_CTRL_CHANGE | NAND_NCE | NAND_ALE);			//send 0xcc add
	}
	
	NFC_SEND_CMD_IDLE(aml_chip->chip_selected, 0);
	aml_chip->aml_nand_write_byte(aml_chip,0x4d );								//write 0x4d into 0xcc add
	aml_chip->aml_nand_command(aml_chip, 0x16, -1, -1, chipnr);					//send cmd 0x16 
	aml_chip->aml_nand_command(aml_chip, 0x17, -1, -1, chipnr);					//send cmd 0x17
	aml_chip->aml_nand_command(aml_chip, 0x04, -1, -1, chipnr);					//send cmd 0x04 
	aml_chip->aml_nand_command(aml_chip, 0x19, -1, -1, chipnr);					//send cmd 0x19 
	aml_chip->aml_nand_command(aml_chip, NAND_CMD_READ0, 0, 0x200, chipnr);
#if 0	
	chip->cmd_ctrl(mtd, NAND_CMD_READ0, NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
	udelay(1);
	chip->cmd_ctrl(mtd, 00, NAND_CTRL_CHANGE | NAND_NCE | NAND_ALE);
	chip->cmd_ctrl(mtd, 00, NAND_CTRL_CHANGE | NAND_NCE | NAND_ALE);
	chip->cmd_ctrl(mtd, 00, NAND_CTRL_CHANGE | NAND_NCE | NAND_ALE);
	chip->cmd_ctrl(mtd, 02, NAND_CTRL_CHANGE | NAND_NCE | NAND_ALE);
	chip->cmd_ctrl(mtd, 00, NAND_CTRL_CHANGE | NAND_NCE | NAND_ALE);
	chip->cmd_ctrl(mtd, NAND_CMD_READSTART, NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);

	chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);	
	ndelay(200);
#endif	

#if 1
	aml_chip->aml_nand_wait_devready(aml_chip, chipnr);
	if (aml_chip->ops_mode & AML_CHIP_NONE_RB) 
		chip->cmd_ctrl(mtd, NAND_CMD_READ0 & 0xff, NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
#else
	udelay(500);
#endif
	
	total_reg_cnt = chip->read_byte(mtd);
	reg_cnt_otp = chip->read_byte(mtd);
#if 0
	for(j=0; j<8; j++){
		printk("#############%dst COPY#############\n", j);
		for(i=0;i<128;i++){
			//one_copy_buf[j] = chip->read_byte(mtd);
			printk("%2x\t", chip->read_byte(mtd));
			if (i &&  ((i+1)%8 == 0))
				printk("\n");
			if (i &&  ((i+1)%64 == 0))
				printk("\n");
		}
	}


	aml_chip->aml_nand_command(aml_chip, NAND_CMD_RESET, -1, -1, chipnr);

	aml_chip->aml_nand_wait_devready(aml_chip, chipnr);
	
	aml_chip->aml_nand_command(aml_chip, 0x38, -1, -1, chipnr);				//end read otp mode

	aml_chip->aml_nand_wait_devready(aml_chip, chipnr);

	
	BUG();
#endif	
	
	for(i=0; i<8; i++){
		check_flag = 0;
		memset(one_copy_buf, 0, 128);
		for(j=0;j<128;j++){
			one_copy_buf[j] = chip->read_byte(mtd);
			ndelay(50);
		}
#if 0
		printk("\n#############%dst COPY#############\n", i);
		for(j=0;j<128;j++){
			printk("%2x\t", one_copy_buf[j]);
			if (j &&  ((j+1)%8 == 0))
				printk("\n");
			if (j &&  ((j+1)%64 == 0))
				printk("\n");
		}
#endif
		for(j=0;j<64;j+=8){
			for(k=0;k<7;k++){
				if(((one_copy_buf[k+j] < 0x80) && (one_copy_buf[k+j+64] < 0x80)) ||
				   ((one_copy_buf[k+j] > 0x80) && (one_copy_buf[k+j+64] > 0x80))  ||
					((unsigned char)(one_copy_buf[k+j]^one_copy_buf[k+j+64]) != 0xFF)){
					check_flag = 1;
					break;
				}
				if(check_flag){
					break;
				}
			}
			if(check_flag){
				break;
			}		
		}
		if(check_flag == 0){
			break;
		}				
	}
	
		
	aml_chip->aml_nand_command(aml_chip, NAND_CMD_RESET, -1, -1, chipnr);

	aml_chip->aml_nand_wait_devready(aml_chip, chipnr);
	
	aml_chip->aml_nand_command(aml_chip, 0x38, -1, -1, chipnr);				//end read otp mode

	aml_chip->aml_nand_wait_devready(aml_chip, chipnr);
	
	if(check_flag){
		printk("%s %d 20 nm flashdefault vaule abnormal not safe !!!!!, chip[%d]\n", __func__, __LINE__, chipnr);
		BUG();		
	}
	else{	
		tmp_buf = one_copy_buf;
		
		memcpy(&aml_chip->new_nand_info.read_rety_info.reg_default_value[chipnr][0], tmp_buf, aml_chip->new_nand_info.read_rety_info.reg_cnt);

		tmp_buf += aml_chip->new_nand_info.read_rety_info.reg_cnt;
		
		
		for(j=0;j<aml_chip->new_nand_info.read_rety_info.retry_cnt;j++){
			for(k=0;k<aml_chip->new_nand_info.read_rety_info.reg_cnt;k++){
				aml_chip->new_nand_info.read_rety_info.reg_offset_value[chipnr][j][k] = (char)tmp_buf[0];
				tmp_buf++;

			}
		
		}
	}	

	return 0;
}
void aml_nand_set_readretry_default_value_hynix(struct mtd_info *mtd)
{
	unsigned char hynix_reg_read_value_tmp[READ_RETRY_REG_NUM];	
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	struct nand_chip *chip = mtd->priv;
	int i;

	if((aml_chip->new_nand_info.type == 0) ||(aml_chip->new_nand_info.type > 10))
		return;	

	memset(&hynix_reg_read_value_tmp[0], 0, READ_RETRY_REG_NUM);

	chip->select_chip(mtd, 0);
	for (i=0; i<aml_chip->chip_num; i++) {

		if (aml_chip->valid_chip[i]) {

			aml_nand_set_reg_value_hynix(aml_chip, &aml_chip->new_nand_info.read_rety_info.reg_default_value[i][0], &aml_chip->new_nand_info.read_rety_info.reg_addr[0], i, aml_chip->new_nand_info.read_rety_info.reg_cnt);
		}
	}
}

void aml_nand_enter_enslc_mode_hynix(struct mtd_info *mtd)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	unsigned char hynix_reg_program_value_tmp[ENHANCE_SLC_REG_NUM];	
	struct nand_chip *chip = mtd->priv;
	int i, j;

	if((aml_chip->new_nand_info.type == 0) ||(aml_chip->new_nand_info.type > 10))
		return;

	//printk("Enter %s\n", __func__);

	memset(&hynix_reg_program_value_tmp[0], 0, ENHANCE_SLC_REG_NUM);

	chip->select_chip(mtd, 0);

	for (i=0; i<aml_chip->chip_num; i++) {

		if (aml_chip->valid_chip[i]) {
			for(j=0;j<aml_chip->new_nand_info.slc_program_info.reg_cnt;j++)
				hynix_reg_program_value_tmp[j] = aml_chip->new_nand_info.slc_program_info.reg_default_value[i][j]  + aml_chip->new_nand_info.slc_program_info.reg_offset_value[j];
		
			aml_nand_set_reg_value_hynix(aml_chip, &hynix_reg_program_value_tmp[0], &aml_chip->new_nand_info.slc_program_info.reg_addr[0], i, aml_chip->new_nand_info.slc_program_info.reg_cnt);
			memset(&hynix_reg_program_value_tmp[0], 0, aml_chip->new_nand_info.slc_program_info.reg_cnt);
		}
	}
	
}

//working  in Normal program mode
void aml_nand_exit_enslc_mode_hynix(struct mtd_info *mtd)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	struct nand_chip *chip = mtd->priv;
	int i;

	if((aml_chip->new_nand_info.type == 0) ||(aml_chip->new_nand_info.type > 10))
		return;

	//printk("Enter %s\n", __func__);
	
	chip->select_chip(mtd, 0);
	for (i=0; i<aml_chip->chip_num; i++) {

		if (aml_chip->valid_chip[i]) {

			aml_nand_set_reg_value_hynix(aml_chip, &aml_chip->new_nand_info.slc_program_info.reg_default_value[i][0], &aml_chip->new_nand_info.slc_program_info.reg_addr[0], i, aml_chip->new_nand_info.slc_program_info.reg_cnt);
		}
	}

}

//when ecc fail,set nand retry reg  
void aml_nand_read_retry_handle_hynix(struct mtd_info *mtd, int chipnr)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	u8 hynix_reg_read_value[READ_RETRY_REG_NUM];
	int i, cur_cnt;
	int retry_zone,retry_offset;
	if((aml_chip->new_nand_info.type == 0) ||(aml_chip->new_nand_info.type > 10))
		return;
	//cur_cnt = aml_chip->new_nand_info.read_rety_info.cur_cnt[chipnr];
		if(aml_chip->new_nand_info.read_rety_info.cur_cnt[chipnr] < aml_chip->new_nand_info.read_rety_info.retry_cnt)
		cur_cnt = aml_chip->new_nand_info.read_rety_info.cur_cnt[chipnr] ;
	else{
		retry_zone = aml_chip->new_nand_info.read_rety_info.cur_cnt[chipnr] /aml_chip->new_nand_info.read_rety_info.retry_cnt;
		retry_offset = aml_chip->new_nand_info.read_rety_info.cur_cnt[chipnr] % aml_chip->new_nand_info.read_rety_info.retry_cnt;
		cur_cnt = (retry_zone + retry_offset) % aml_chip->new_nand_info.read_rety_info.retry_cnt;
	}
	
	memset(&hynix_reg_read_value[0], 0, READ_RETRY_REG_NUM);
	
	for(i=0;i<aml_chip->new_nand_info.read_rety_info.reg_cnt;i++){
		if((aml_chip->new_nand_info.type == HYNIX_26NM_8GB) || (aml_chip->new_nand_info.type == HYNIX_26NM_4GB)){
			if(aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][cur_cnt][i] == READ_RETRY_ZERO)
				hynix_reg_read_value[i] = 0;
			else
				hynix_reg_read_value[i] = aml_chip->new_nand_info.read_rety_info.reg_default_value[chipnr][i]  + aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][cur_cnt][i];
		}
		else  if((aml_chip->new_nand_info.type == HYNIX_20NM_8GB) || (aml_chip->new_nand_info.type == HYNIX_20NM_4GB)){
			//hynix_reg_read_value[i] = aml_chip->new_nand_info.read_rety_info.reg_default_value[chipnr][i]  + aml_chip->new_nand_info.read_rety_info.reg_offset_value[chipnr][cur_cnt][i];
			hynix_reg_read_value[i] = aml_chip->new_nand_info.read_rety_info.reg_offset_value[chipnr][cur_cnt][i];
		}
	}
	
	aml_nand_set_reg_value_hynix(aml_chip, &hynix_reg_read_value[0], &aml_chip->new_nand_info.read_rety_info.reg_addr[0], chipnr, aml_chip->new_nand_info.read_rety_info.reg_cnt);

	aml_chip->new_nand_info.read_rety_info.cur_cnt[chipnr]++;
}


void aml_nand_read_retry_exit_hynix(struct mtd_info *mtd, int chipnr)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	struct nand_chip *chip = mtd->priv;
	int i;

	if((aml_chip->new_nand_info.type == 0) ||(aml_chip->new_nand_info.type > 10))
		return;
	aml_nand_debug("hyinx retry cnt :%d\n",aml_chip->new_nand_info.read_rety_info.cur_cnt[chipnr]);
	chip->select_chip(mtd, chipnr);
	for (i=0; i<aml_chip->chip_num; i++) {

		if (aml_chip->valid_chip[i]) {

			aml_nand_set_reg_value_hynix(aml_chip, &aml_chip->new_nand_info.read_rety_info.reg_default_value[i][0], &aml_chip->new_nand_info.read_rety_info.reg_addr[0], i, aml_chip->new_nand_info.read_rety_info.reg_cnt);

			
		}
	}
	//aml_chip->new_nand_info.read_rety_info.cur_cnt[chipnr] =0;
	memset(&aml_chip->new_nand_info.read_rety_info.cur_cnt[0], 0, MAX_CHIP_NUM);
}

void aml_nand_get_slc_default_value_hynix(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	int i;
	
	chip->select_chip(mtd, 0);
	for(i=0; i<aml_chip->chip_num; i++){
		if(aml_chip->valid_chip[i]){
			aml_nand_get_reg_value_hynix(aml_chip, &aml_chip->new_nand_info.slc_program_info.reg_default_value[i][0], &aml_chip->new_nand_info.slc_program_info.reg_addr[0], i, aml_chip->new_nand_info.slc_program_info.reg_cnt);
		}		
	}
	//chip->select_chip(mtd, -1);
}

void aml_nand_get_read_default_value_hynix(struct mtd_info *mtd)
{
	//struct mtd_oob_ops aml_oob_ops;
	struct mtd_oob_ops  *aml_oob_ops;
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	struct nand_chip *chip = mtd->priv;
	size_t addr;
	unsigned char *data_buf;
	char oob_buf[4];
	unsigned char page_list[RETRY_NAND_COPY_NUM] = {0x07, 0x0B, 0x0F, 0x13};
	int error = 0, err, i, j, k, nand_type, total_blk, phys_erase_shift = fls(mtd->erasesize) - 1;

	aml_oob_ops = kzalloc(sizeof(struct mtd_oob_ops), GFP_KERNEL);
	if (aml_oob_ops == NULL){
		printk("%s %d no mem for aml_oob_ops\n", __func__, __LINE__);
		err = -ENOMEM;
		goto exit;
	}
	data_buf = kzalloc(mtd->writesize, GFP_KERNEL);
	if (data_buf == NULL){
		printk("%s %d no mem for databuf and mtd->writesize:%d \n", __func__, __LINE__, mtd->writesize);
		err = -ENOMEM;
		goto exit;
	}

	if (nand_boot_flag){
		addr = (1024 * mtd->writesize / aml_chip->plane_num);
	}		
	else {
		addr = 0;
	}

	total_blk = 0;	
	aml_chip->new_nand_info.read_rety_info.default_flag = 0;
	while(total_blk < RETRY_NAND_BLK_NUM){
		nand_type = aml_chip->new_nand_info.type;
		aml_chip->new_nand_info.type = 0;
		error = mtd->block_isbad(mtd, addr);
		aml_chip->new_nand_info.type = nand_type;
		if (error) {
			printk("%s %d detect bad blk at blk:%d\n", __func__, __LINE__, addr>> phys_erase_shift);
			addr += mtd->erasesize;
			total_blk++;
			continue;
		}
		
		aml_oob_ops->mode = MTD_OOB_AUTO;
		aml_oob_ops->len = mtd->writesize;
		aml_oob_ops->ooblen = 4;
		aml_oob_ops->ooboffs = mtd->ecclayout->oobfree[0].offset;
		aml_oob_ops->datbuf = data_buf;
		aml_oob_ops->oobbuf = oob_buf;

		memset(oob_buf, 0, 4);
		//memset((unsigned char *)aml_oob_ops.datbuf, 0x0, mtd->writesize);
		//memset((unsigned char *)aml_oob_ops.oobbuf, 0x0, aml_oob_ops.ooblen);
		memset((unsigned char *)aml_oob_ops->datbuf, 0x0, mtd->writesize);
		memset((unsigned char *)aml_oob_ops->oobbuf, 0x0, aml_oob_ops->ooblen);

		for(i=0;i<RETRY_NAND_COPY_NUM;i++){
			memset(oob_buf, 0, 4);
			memset((unsigned char *)aml_oob_ops->datbuf, 0x0, mtd->writesize);
			memset((unsigned char *)aml_oob_ops->oobbuf, 0x0, aml_oob_ops->ooblen);		
			nand_type = aml_chip->new_nand_info.type;
			aml_chip->new_nand_info.type = 0;
			error = mtd->read_oob(mtd, (addr +  page_list[i]*mtd->writesize), aml_oob_ops);
			aml_chip->new_nand_info.type = nand_type;
			if ((error != 0) && (error != -EUCLEAN)) {
				printk("%s %d read oob failed at blk:%d, page:%d\n", __func__, __LINE__, addr>> phys_erase_shift, (addr +  page_list[i]*mtd->writesize)/mtd->writesize);
				continue;
			}			
			if (!memcmp(oob_buf, RETRY_NAND_MAGIC, 4)){ 
				memcpy(&aml_chip->new_nand_info.read_rety_info.reg_default_value[0][0], (unsigned char *)aml_oob_ops->datbuf, MAX_CHIP_NUM*READ_RETRY_REG_NUM);
				//memcpy(&aml_chip->new_nand_info.slc_program_info.reg_default_value[0][0], (unsigned char *)aml_oob_ops.datbuf, MAX_CHIP_NUM*ENHANCE_SLC_REG_NUM);
				printk("%s %d get default reg value at blk:%d, page:%d\n", __func__, __LINE__, addr>> phys_erase_shift, (addr +  page_list[i]*mtd->writesize)/mtd->writesize);
				
				if((aml_chip->new_nand_info.type == HYNIX_20NM_8GB) || (aml_chip->new_nand_info.type == HYNIX_20NM_4GB)){
					memcpy(&aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][0][0], (unsigned char *)(aml_oob_ops->datbuf+MAX_CHIP_NUM*READ_RETRY_REG_NUM), 		
															MAX_CHIP_NUM*READ_RETRY_CNT*READ_RETRY_REG_NUM);

					
				}
				aml_chip->new_nand_info.read_rety_info.default_flag = 1;
				goto READ_OK;
			}			
		}
		
		addr += mtd->erasesize;
		total_blk++;
	}	
	aml_chip->new_nand_info.read_rety_info.default_flag = 0;

	printk("######%s %d read default read retry reg value failed and need read from chip write back to nand using SLC\n", __func__, __LINE__);
	chip->select_chip(mtd, 0);
	for(i=0; i<aml_chip->chip_num; i++){
		if(aml_chip->valid_chip[i]){
			if((aml_chip->new_nand_info.type == HYNIX_26NM_8GB) || (aml_chip->new_nand_info.type == HYNIX_26NM_4GB)){
				aml_nand_get_reg_value_hynix(aml_chip, &aml_chip->new_nand_info.read_rety_info.reg_default_value[i][0], 
					&aml_chip->new_nand_info.read_rety_info.reg_addr[0], i, aml_chip->new_nand_info.read_rety_info.reg_cnt);
			}
			else  if((aml_chip->new_nand_info.type == HYNIX_20NM_8GB) || (aml_chip->new_nand_info.type == HYNIX_20NM_4GB)){
				aml_nand_get_reg_value_formOTP_hynix(aml_chip, i);
			}
		}		
	}
	//chip->select_chip(mtd, -1);

READ_OK:	

	kfree(data_buf);
	kfree(aml_oob_ops);
	return ;
	
	
exit:
	
	if (aml_oob_ops) {
		
		kfree(aml_oob_ops);
		aml_oob_ops = NULL;
	}
	if (data_buf) {
		kfree(data_buf);
		data_buf = NULL;
	}
return;
}
/*******************************************TOSHIBA*********************************************/
void aml_nand_set_reg_value_toshiba(struct aml_nand_chip *aml_chip,  uint8_t *buf, uint8_t *addr, int chipnr, int cnt)
{
	struct nand_chip *chip = &aml_chip->chip;
	struct mtd_info *mtd = &aml_chip->mtd;
	int j;

	if(aml_chip->new_nand_info.type != TOSHIBA_24NM)
		return;


	aml_chip->aml_nand_wait_devready(aml_chip, chipnr);

	aml_chip->aml_nand_select_chip(aml_chip, chipnr);

	if(aml_chip->new_nand_info.read_rety_info.cur_cnt[chipnr] ==0){

		aml_chip->aml_nand_command(aml_chip, NAND_CMD_TOSHIBA_PRE_CON1, -1, -1, chipnr);
		
		NFC_SEND_CMD_IDLE(aml_chip->chip_selected, 2);
		
		aml_chip->aml_nand_command(aml_chip, NAND_CMD_TOSHIBA_PRE_CON2, -1, -1, chipnr);
		
		NFC_SEND_CMD_IDLE(aml_chip->chip_selected, 2);
	}
	
	for (j=0; j<cnt; j++){
		aml_chip->aml_nand_command(aml_chip, NAND_CMD_TOSHIBA_SET_VALUE, -1, -1, chipnr);
		NFC_SEND_CMD_IDLE(aml_chip->chip_selected, 2);
	        chip->cmd_ctrl(mtd, addr[j], NAND_CTRL_CHANGE | NAND_NCE | NAND_ALE);
		NFC_SEND_CMD_IDLE(aml_chip->chip_selected, 2);
		aml_chip->aml_nand_write_byte(aml_chip, buf[j]);	
		NFC_SEND_CMD_IDLE(aml_chip->chip_selected, 2);
	}

	aml_chip->aml_nand_command(aml_chip, NAND_CMD_TOSHIBA_BEF_COMMAND1, -1, -1, chipnr);
	NFC_SEND_CMD_IDLE(aml_chip->chip_selected, 2);
	aml_chip->aml_nand_command(aml_chip, NAND_CMD_TOSHIBA_BEF_COMMAND2, -1, -1, chipnr);
	NFC_SEND_CMD_IDLE(aml_chip->chip_selected, 2);
	aml_chip->aml_nand_wait_devready(aml_chip, chipnr);

	return;
}

//when ecc fail,set nand retry reg  
void aml_nand_read_retry_handle_toshiba(struct mtd_info *mtd, int chipnr)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	int cur_cnt;
	
	if(aml_chip->new_nand_info.type != TOSHIBA_24NM)
		return;
	
	if(aml_chip->toggle_mode) {
		NFC_EXIT_SYNC_ADJ() ; 
		NFC_EXIT_TOSHIBA_TOGGLE_MODE();
	}
	cur_cnt = aml_chip->new_nand_info.read_rety_info.cur_cnt[chipnr];

	aml_nand_set_reg_value_toshiba(aml_chip, &aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][cur_cnt][0], &aml_chip->new_nand_info.read_rety_info.reg_addr[0], chipnr, aml_chip->new_nand_info.read_rety_info.reg_cnt);

	cur_cnt++;
	aml_chip->new_nand_info.read_rety_info.cur_cnt[chipnr] = (cur_cnt > (aml_chip->new_nand_info.read_rety_info.retry_cnt-1)) ? 0 : cur_cnt;

	if(aml_chip->toggle_mode) {
		
		NFC_SYNC_ADJ(); 
		NFC_ENABLE_TOSHIBA_TOGGLE_MODE();
	}
}

void aml_nand_read_retry_exit_toshiba(struct mtd_info *mtd, int chipnr)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);

	if(aml_chip->new_nand_info.type != TOSHIBA_24NM)
		return;
		
	aml_nand_debug("toshiba retry cnt :%d\n",aml_chip->new_nand_info.read_rety_info.cur_cnt[chipnr]);	
	aml_chip->aml_nand_wait_devready(aml_chip, chipnr);
	aml_chip->aml_nand_command(aml_chip, NAND_CMD_RESET, -1, -1, chipnr);
	aml_chip->aml_nand_wait_devready(aml_chip, chipnr);	

	memset(&aml_chip->new_nand_info.read_rety_info.cur_cnt[0], 0, MAX_CHIP_NUM);
}


/*******************************************SUMSUNG*********************************************/
uint8_t aml_nand_set_reg_value_samsung(struct aml_nand_chip *aml_chip,  uint8_t *buf, uint8_t *addr, int chipnr, int cnt)
{
	struct nand_chip *chip = &aml_chip->chip;
	struct mtd_info *mtd = &aml_chip->mtd;
	int j;

	if(aml_chip->new_nand_info.type != SUMSUNG_2XNM)
		return 0;

	aml_chip->aml_nand_wait_devready(aml_chip, chipnr);

	aml_chip->aml_nand_select_chip(aml_chip, chipnr);

	for (j=0; j<cnt; j++){
		aml_chip->aml_nand_command(aml_chip, NAND_CMD_SAMSUNG_SET_VALUE, -1, -1, chipnr);
		
	       chip->cmd_ctrl(mtd,0, NAND_CTRL_CHANGE | NAND_NCE | NAND_ALE);
		
	        chip->cmd_ctrl(mtd,addr[j], NAND_CTRL_CHANGE | NAND_NCE | NAND_ALE);
		
		aml_chip->aml_nand_write_byte(aml_chip, buf[j]);	
		
		NFC_SEND_CMD_IDLE(aml_chip->chip_selected, 20);
	}

	aml_chip->aml_nand_wait_devready(aml_chip, chipnr);

	return 0;
}


void aml_nand_read_retry_handle_samsung(struct mtd_info *mtd, int chipnr)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	int cur_cnt;

	if(aml_chip->new_nand_info.type != SUMSUNG_2XNM)
		return;
	
	cur_cnt = aml_chip->new_nand_info.read_rety_info.cur_cnt[chipnr];

	aml_nand_set_reg_value_samsung(aml_chip, &aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][cur_cnt][0], &aml_chip->new_nand_info.read_rety_info.reg_addr[0], chipnr, aml_chip->new_nand_info.read_rety_info.reg_cnt);

	cur_cnt++;
	aml_chip->new_nand_info.read_rety_info.cur_cnt[chipnr] = (cur_cnt > (aml_chip->new_nand_info.read_rety_info.retry_cnt-1)) ? 0 : cur_cnt;
}

void aml_nand_read_retry_exit_samsung(struct mtd_info *mtd, int chipnr)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);

	if(aml_chip->new_nand_info.type != SUMSUNG_2XNM)
		return;
	aml_nand_debug("samsung retry cnt :%d\n",aml_chip->new_nand_info.read_rety_info.cur_cnt[chipnr]);	
	aml_nand_set_reg_value_samsung(aml_chip, &aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][0][0], &aml_chip->new_nand_info.read_rety_info.reg_addr[0], chipnr, aml_chip->new_nand_info.read_rety_info.reg_cnt);
	memset(&aml_chip->new_nand_info.read_rety_info.cur_cnt[0], 0, MAX_CHIP_NUM);
}





/***********************************MICRON************************************/

uint8_t aml_nand_set_reg_value_micron(struct aml_nand_chip *aml_chip,  uint8_t *buf, uint8_t *addr, int chipnr, int cnt)
{
	struct nand_chip *chip = &aml_chip->chip;
	struct mtd_info *mtd = &aml_chip->mtd;
	int i;

	if(aml_chip->new_nand_info.type != MICRON_20NM)
		return 0;

	aml_chip->aml_nand_wait_devready(aml_chip, chipnr);

	aml_chip->aml_nand_select_chip(aml_chip, chipnr);
	
		aml_chip->aml_nand_command(aml_chip, NAND_CMD_MICRON_SET_VALUE, -1, -1, chipnr);
	        chip->cmd_ctrl(mtd,addr[0], NAND_CTRL_CHANGE | NAND_NCE | NAND_ALE);
		NFC_SEND_CMD_IDLE(aml_chip->chip_selected, 10);
		aml_chip->aml_nand_write_byte(aml_chip, buf[0]);	
		for(i =0; i < 3; i++){
			NFC_SEND_CMD(aml_chip->chip_selected | IDLE | 1);	
			aml_chip->aml_nand_write_byte(aml_chip, 0x0);	//for P2 P3 P4
		}

	aml_chip->aml_nand_wait_devready(aml_chip, chipnr);

	return 0;
}

void aml_nand_read_retry_handle_micron(struct mtd_info *mtd, int chipnr)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	int cur_cnt;

	if(aml_chip->new_nand_info.type != MICRON_20NM)
		return;	
	
	cur_cnt = aml_chip->new_nand_info.read_rety_info.cur_cnt[chipnr];

	aml_nand_set_reg_value_micron(aml_chip, &aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][cur_cnt][0], 
		&aml_chip->new_nand_info.read_rety_info.reg_addr[0], chipnr, aml_chip->new_nand_info.read_rety_info.reg_cnt);

	cur_cnt++;
	aml_chip->new_nand_info.read_rety_info.cur_cnt[chipnr] = 
		(cur_cnt > (aml_chip->new_nand_info.read_rety_info.retry_cnt-1)) ? 0 : cur_cnt;

	return ;
}

void aml_nand_read_retry_exit_micron(struct mtd_info *mtd, int chipnr)
{
	
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	uint8_t default_val = 0;
	
	if(aml_chip->new_nand_info.type != MICRON_20NM)
		return;
	//printk("Enter %s\n", __func__);
	
	aml_nand_debug("micron retry cnt :%d\n",aml_chip->new_nand_info.read_rety_info.cur_cnt[chipnr]);
	aml_nand_set_reg_value_micron(aml_chip, &default_val,
		&aml_chip->new_nand_info.read_rety_info.reg_addr[0], chipnr, aml_chip->new_nand_info.read_rety_info.reg_cnt);
	memset(&aml_chip->new_nand_info.read_rety_info.cur_cnt[0], 0, MAX_CHIP_NUM);

	return ;
}

/***********************************SANDISK************************************/

uint8_t aml_nand_dynamic_read_init_start(struct aml_nand_chip *aml_chip, int chipnr)
{
	struct nand_chip *chip = &aml_chip->chip;
	struct mtd_info *mtd = &aml_chip->mtd;
	uint8_t *buf;
	int i=0;


	buf = &aml_chip->new_nand_info.dynamic_read_info.reg_addr_init[0];

	/*
	aml_chip->aml_nand_command(aml_chip, NAND_CMD_RESET, -1, -1, chipnr);
	aml_chip->aml_nand_wait_devready(aml_chip, chipnr);
	udelay(1);*/

	aml_chip->aml_nand_command(aml_chip, NAND_CMD_SANDISK_INIT_ONE, -1, -1, chipnr);	//send cmd 0x3B 
	
	aml_chip->aml_nand_command(aml_chip, NAND_CMD_SANDISK_INIT_TWO, -1, -1, chipnr);	//send cmd 0xB9
	
	for(i=0; i<DYNAMIC_REG_INIT_NUM; i++){
		aml_chip->aml_nand_command(aml_chip, NAND_CMD_SANDISK_LOAD_VALUE_ONE, -1, -1, chipnr);	//send cmd 0x53 
		NFC_SEND_CMD_IDLE(aml_chip->chip_selected, 10);
		chip->cmd_ctrl(mtd, buf[i], NAND_CTRL_CHANGE | NAND_NCE | NAND_ALE);			//send 0x04 add
		NFC_SEND_CMD_IDLE(aml_chip->chip_selected, 10);
		aml_chip->aml_nand_write_byte(aml_chip,0x0 );								//write 0x00 into  add
		NFC_SEND_CMD_IDLE(aml_chip->chip_selected, 10);
	}
	return 0;

}

void aml_nand_dynamic_read_init(struct mtd_info *mtd)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	//struct nand_chip *chip = mtd->priv;
	int i;

	if((aml_chip->new_nand_info.type < SANDISK_19NM))
		return;	
	
	aml_chip->new_nand_info.dynamic_read_info.dynamic_read_flag = 1; //DRF

	for(i=0; i<aml_chip->chip_num; i++){
		if(aml_chip->valid_chip[i]){
			if((aml_chip->new_nand_info.type == SANDISK_19NM)||(aml_chip->new_nand_info.type == SANDISK_24NM) ){
				aml_nand_dynamic_read_init_start(aml_chip, i);
			}
		}		
	}
}

uint8_t aml_nand_dynamic_read_load_register_value(struct aml_nand_chip *aml_chip,  uint8_t *buf, uint8_t *addr, int chipnr, int cnt)
{
	
	struct nand_chip *chip = &aml_chip->chip;
	struct mtd_info *mtd = &aml_chip->mtd;
	int j;
	
	if(aml_chip->new_nand_info.type < SANDISK_19NM)
		return 0;
	
	
	aml_chip->aml_nand_wait_devready(aml_chip, chipnr);
	
	aml_chip->aml_nand_select_chip(aml_chip, chipnr);
	
	aml_chip->aml_nand_command(aml_chip, NAND_CMD_SANDISK_INIT_ONE, -1, -1, chipnr);//3B
	aml_chip->aml_nand_command(aml_chip, NAND_CMD_SANDISK_INIT_TWO, -1, -1, chipnr);//B9
	
	for (j=0; j<cnt; j++){
		aml_chip->aml_nand_command(aml_chip, NAND_CMD_SANDISK_LOAD_VALUE_ONE, -1, -1, chipnr); //53
		NFC_SEND_CMD_IDLE(aml_chip->chip_selected, 10);
		chip->cmd_ctrl(mtd, addr[j], NAND_CTRL_CHANGE | NAND_NCE | NAND_ALE); //addr
		NFC_SEND_CMD_IDLE(aml_chip->chip_selected, 10);
		aml_chip->aml_nand_write_byte(aml_chip, buf[j]);			
		NFC_SEND_CMD_IDLE(aml_chip->chip_selected, 10);
	}	
		aml_chip->aml_nand_wait_devready(aml_chip, chipnr);	
		return 0;	
}

void aml_nand_dynamic_read_handle(struct mtd_info *mtd, int page, int chipnr)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	struct nand_chip *chip = &aml_chip->chip;
	u8 dynamic_reg_read_value[DYNAMIC_REG_NUM];	
	u8 dynamic_reg_addr_value[DYNAMIC_REG_NUM];
	int cur_case_num_lower_page, cur_case_num_upper_page , i;
	int pages_per_blk;
	if(aml_chip->new_nand_info.type !=SANDISK_19NM)
		return;
	
	//printk("Enter %s\n", __func__);

	//aml_chip->new_nand_info.dynamic_read_info.dynamic_read_init(mtd);
	cur_case_num_upper_page = aml_chip->new_nand_info.dynamic_read_info.cur_case_num_upper_page[chipnr]; //N
	cur_case_num_lower_page = aml_chip->new_nand_info.dynamic_read_info.cur_case_num_lower_page[chipnr]; //N

	
	pages_per_blk = (1 << (chip->phys_erase_shift - chip->page_shift));
	if(((page !=0) && (page % 2 ) == 0) || (page == (pages_per_blk -1))){
		//cur_case_num_upper_page = aml_chip->new_nand_info.dynamic_read_info.cur_case_num_upper_page[chipnr]; //N

		memset(&dynamic_reg_read_value[0], 0, DYNAMIC_REG_NUM);
		
		memset(&dynamic_reg_addr_value[0], 0, DYNAMIC_REG_NUM);
		
		for(i=0;i<DYNAMIC_REG_NUM;i++){

			dynamic_reg_read_value[i] =  aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[cur_case_num_upper_page][i];
			dynamic_reg_addr_value[i] =  aml_chip->new_nand_info.dynamic_read_info.reg_addr_upper_page[i];

		}
			//dynamic_reg_read_value[0] |= aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[0][cur_case_num_lower_page -1][0];
		aml_nand_dynamic_read_load_register_value(aml_chip, &dynamic_reg_read_value[0], &dynamic_reg_addr_value [0], chipnr, DYNAMIC_REG_NUM);
		
		cur_case_num_upper_page++;
		aml_chip->new_nand_info.dynamic_read_info.cur_case_num_upper_page[chipnr] = 
			(cur_case_num_upper_page > DYNAMIC_READ_CNT_UPPER) ? 0 : cur_case_num_upper_page;
		
		//enable dynamic read
		aml_chip->aml_nand_command(aml_chip, NAND_CMD_SANDISK_DYNAMIC_ENABLE, -1, -1, chipnr);//B6
		udelay(1);
	}else{

		//cur_case_num_lower_page = aml_chip->new_nand_info.dynamic_read_info.cur_case_num_lower_page[chipnr]; //N

		memset(&dynamic_reg_read_value[0], 0, DYNAMIC_REG_NUM);
		
		memset(&dynamic_reg_addr_value[0], 0, DYNAMIC_REG_NUM);
		
		for(i=0;i<DYNAMIC_REG_NUM;i++){

			dynamic_reg_read_value[i] =  aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[cur_case_num_lower_page][i];		
			dynamic_reg_addr_value[i] =  aml_chip->new_nand_info.dynamic_read_info.reg_addr_lower_page[i];

		}

			//dynamic_reg_read_value[0] |= aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[0][cur_case_num_upper_page -1][0];
		aml_nand_dynamic_read_load_register_value(aml_chip, &dynamic_reg_read_value[0], &dynamic_reg_addr_value [0], chipnr, DYNAMIC_REG_NUM);

		cur_case_num_lower_page++;
		aml_chip->new_nand_info.dynamic_read_info.cur_case_num_lower_page[chipnr] = 
			(cur_case_num_lower_page > DYNAMIC_READ_CNT_LOWER) ? 0 : cur_case_num_lower_page;
		
		//enable dynamic read
		aml_chip->aml_nand_command(aml_chip, NAND_CMD_SANDISK_DYNAMIC_ENABLE, -1, -1, chipnr);//B6
	}
	
}
void aml_nand_read_retry_handle_sandisk(struct mtd_info *mtd, int chipnr)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	int cur_cnt;
	cur_cnt = aml_chip->new_nand_info.read_rety_info.cur_cnt[chipnr];

	aml_nand_dynamic_read_load_register_value(aml_chip, (uint8_t *)&aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][cur_cnt][0], 
		&aml_chip->new_nand_info.read_rety_info.reg_addr[0], chipnr, aml_chip->new_nand_info.read_rety_info.reg_cnt);
	cur_cnt++;
	aml_chip->new_nand_info.read_rety_info.cur_cnt[chipnr] = 
		(cur_cnt > (aml_chip->new_nand_info.read_rety_info.retry_cnt-1)) ? 0 : cur_cnt;
	return ;
}
void aml_nand_dynamic_read_exit(struct mtd_info *mtd, int chipnr)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);

	if(aml_chip->new_nand_info.type < SANDISK_19NM)
		return;
	
	//printk("Enter %s\n", __func__);
	aml_chip->aml_nand_wait_devready(aml_chip, chipnr);
	aml_chip->aml_nand_command(aml_chip, NAND_CMD_SANDISK_DYNAMIC_DISABLE, -1, -1, chipnr);//D6
	aml_chip->aml_nand_wait_devready(aml_chip, chipnr);	
	
	aml_nand_dynamic_read_init(mtd);  // set all register to 0x00
	
	aml_chip->aml_nand_wait_devready(aml_chip, chipnr);
	aml_chip->aml_nand_command(aml_chip, NAND_CMD_RESET, -1, -1, chipnr);
	aml_chip->aml_nand_wait_devready(aml_chip, chipnr);	

	memset(&aml_chip->new_nand_info.read_rety_info.cur_cnt[0], 0, MAX_CHIP_NUM);
	return ;
}

void aml_nand_enter_slc_mode_sandisk(struct mtd_info *mtd)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	
	//printk("Enter %s\n", __func__);
	aml_chip->new_nand_info.dynamic_read_info.slc_flag = 1;
	return;
}

void aml_nand_exit_slc_mode_sandisk(struct mtd_info *mtd)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	
	//printk("Enter %s\n", __func__);
	aml_chip->new_nand_info.dynamic_read_info.slc_flag = 0;
	return;
}



uint8_t aml_nand_set_featureReg_value_toshiba(struct aml_nand_chip *aml_chip,  uint8_t *buf, uint8_t addr, int chipnr, int cnt)
{
	struct nand_chip *chip = &aml_chip->chip;
	struct mtd_info *mtd = &aml_chip->mtd;
	int j;

	//if(aml_chip->new_nand_info.type != SANDISK_19NM)
	//	return 0;

	//printk("Enter %s\n", __func__);
	
	chip->select_chip(mtd, chipnr);

	aml_chip->aml_nand_wait_devready(aml_chip, chipnr);

	
	chip->cmd_ctrl(mtd, NAND_CMD_SANDISK_SET_VALUE, NAND_CTRL_CHANGE | NAND_NCE | NAND_CLE);
													
	NFC_SEND_CMD_IDLE(aml_chip->chip_selected, 10);

	chip->cmd_ctrl(mtd, addr, NAND_CTRL_CHANGE | NAND_NCE | NAND_ALE);
	
	for (j=0; j<cnt; j++){
		NFC_SEND_CMD_IDLE(aml_chip->chip_selected, 10);

		aml_chip->aml_nand_write_byte(aml_chip, buf[j]);			
	}

	aml_chip->aml_nand_wait_devready(aml_chip, chipnr);

	return 0;
}
void aml_nand_read_set_flash_feature_toshiba(struct mtd_info *mtd, int chipnr)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	uint8_t reg10h[4] ={4,0,0,0};
	uint8_t reg80h[4] ={0,0,0,0};


	aml_nand_set_featureReg_value_toshiba(aml_chip,reg10h,NAND_CMD_SANDISK_SET_OUTPUT_DRV,chipnr,4);

	aml_nand_set_featureReg_value_toshiba(aml_chip,reg80h,NAND_CMD_SANDISK_SET_VENDOR_SPC,chipnr,4);

	printk("set flash toggle mode end\n");	
	
}



void aml_nand_set_toggle_mode_toshiba(struct mtd_info *mtd, int chipnr)
{
	//switch SDR to Toggle mode
	//first send cmd to switch nand mode from SDR to toggle mode
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);

	aml_nand_read_set_flash_feature_toshiba(mtd,chipnr);
	
	printk("##NAND CFG =0x%x before enable toggle mode \n",READ_CBUS_REG(NAND_CFG));
	//second set nand controller pinmux and enable toggle mode
	NFC_SYNC_ADJ();
	NFC_ENABLE_TOSHIBA_TOGGLE_MODE();
	aml_chip->toggle_mode = 1;
	printk("NAND CFG =0x%x after enable toggle mode \n",READ_CBUS_REG(NAND_CFG));

}



void aml_nand_debug_toggle_flash(struct mtd_info *mtd, int chipnr)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	//struct aml_nand_platform *plat = aml_chip->platform;
	
     if(aml_chip->mfr_type == NAND_MFR_TOSHIBA){

		aml_nand_set_toggle_mode_toshiba(mtd,0);
	}
	 /*
	else  if(aml_chip->mfr_type == NAND_MFR_MICRON) {

			aml_nand_set_toggle_mode_micron(mtd,0);
	}*/		

}
#endif

/****************************************************************************/
static void aml_platform_hw_init(struct aml_nand_chip *aml_chip)
{
	struct clk *sys_clk;
	int sys_clk_rate, sys_time, start_cycle, end_cycle, bus_cycle, time_mode, adjust, Tcycle, T_REA = DEFAULT_T_REA, T_RHOH = DEFAULT_T_RHOH, i;

#ifdef CONFIG_ARCH_MESON
	struct mtd_info *mtd = &aml_chip->mtd;
	struct nand_chip *chip = &aml_chip->chip;
	if (aml_chip->chip_num > 1) {
		chip->select_chip(mtd, -1);
		CLEAR_CBUS_REG_MASK(PREG_HGPIO_EN_N, (1 << 5));
		CLEAR_CBUS_REG_MASK(PREG_HGPIO_O, (1 << 5));
		SET_CBUS_REG_MASK(PREG_HGPIO_EN_N, (1 << 16));
		if (!(READ_CBUS_REG(PREG_HGPIO_I) & (1 << 16))) {
			SET_CBUS_REG_MASK(PREG_HGPIO_O, (1 << 5));
			if ((READ_CBUS_REG(PREG_HGPIO_I) & (1 << 16))) {
				aml_chip->chip_enable[1] = (aml_chip->chip_enable[1] & aml_chip->chip_enable[2]);
				aml_chip->rb_enable[1] = aml_chip->rb_enable[2];
				aml_chip->chip_num = 2;
				aml_nand_debug("ce1 and ce2 connected\n");
			}
		}
	}
#endif

	sys_clk = clk_get_sys(NAND_SYS_CLK_NAME, NULL);
	sys_clk_rate = clk_get_rate(sys_clk);
	sys_time = (10000 / (sys_clk_rate / 1000000));

	start_cycle = (((NAND_CYCLE_DELAY + T_REA * 10) * 10) / sys_time);
	start_cycle = (start_cycle + 9) / 10;
	time_mode = -1;
	for (i=5; i>=0; i--) {
		bus_cycle = nand_mode_time[i];
		Tcycle = bus_cycle * sys_time;
		end_cycle = (((NAND_CYCLE_DELAY + Tcycle / 2 + T_RHOH * 10) * 10) / sys_time);
		end_cycle = end_cycle / 10;
		if ((((start_cycle >= 3) && (start_cycle <= ( bus_cycle + 1)))
			|| ((end_cycle >= 3) && (end_cycle <= (bus_cycle + 1))))
			&& (start_cycle <= end_cycle)) {
			time_mode = i;
			break;
		}
	}

	if (time_mode < 0) {
		time_mode = 0;
		for (bus_cycle = 19; bus_cycle > nand_mode_time[time_mode]; bus_cycle--) {
			Tcycle = bus_cycle * sys_time;
			end_cycle = (((NAND_CYCLE_DELAY + Tcycle / 2 + T_RHOH * 10) * 10) / sys_time);
			end_cycle = end_cycle / 10;
			if ((((start_cycle >= 3) && (start_cycle <= ( bus_cycle + 1)))
				|| ((end_cycle >= 3) && (end_cycle <= (bus_cycle + 1))))
				&& (start_cycle <= end_cycle)) {
				break;
			}			
		}
		if (bus_cycle <= nand_mode_time[time_mode])
			return;
	}

	if (nand_mode_time[time_mode] < start_cycle)
		adjust = start_cycle - nand_mode_time[time_mode];
	else if(nand_mode_time[time_mode] > end_cycle) 
		adjust = ((((~(nand_mode_time[time_mode] - end_cycle) + 1)) & 0xf) | 0x8);
	else
		adjust = 0;

	NFC_SET_CFG(0);
	NFC_SET_TIMING(time_mode, (bus_cycle - 1), adjust);
	NFC_SEND_CMD(1<<31);
	dev_info(aml_chip->device, "time_mode=%d, bus_cycle=%d, adjust=%d, start_cycle=%d, end_cycle=%d,system=%d.%dns\n",
		time_mode, bus_cycle, adjust, start_cycle, end_cycle, sys_time/10, sys_time%10);
}

static void aml_platform_adjust_timing(struct aml_nand_chip *aml_chip)
{
	struct aml_nand_platform *plat = aml_chip->platform;
	struct clk *sys_clk;
	int sys_clk_rate, sys_time, start_cycle, end_cycle, bus_cycle, time_mode, time_mode_select, adjust, Tcycle, i;

	time_mode_select = ((plat->platform_nand_data.chip.options & NAND_TIMING_OPTIONS_MASK) >> 8);
	if ((time_mode_select > 5) || (time_mode_select < 0))
		time_mode_select = 5;
	if (!aml_chip->T_REA)
		aml_chip->T_REA = 20;
	if (!aml_chip->T_RHOH)
		aml_chip->T_RHOH = 15;

	if (READ_CBUS_REG(HHI_MPEG_CLK_CNTL)&(1<<8)) {
		sys_clk = clk_get_sys(NAND_SYS_CLK_NAME, NULL);
		sys_clk_rate = clk_get_rate(sys_clk);
		time_mode = -1;
	}
	else {
		time_mode = 0;
		sys_clk_rate = 27000000;
	}

	sys_time = (10000 / (sys_clk_rate / 1000000));
	start_cycle = (((NAND_CYCLE_DELAY + aml_chip->T_REA * 10) * 10) / sys_time);
	start_cycle = (start_cycle + 9) / 10;

	if (time_mode == 0) {

		for (bus_cycle = 5; bus_cycle <= 19; bus_cycle++) {
			Tcycle = bus_cycle * sys_time;
			end_cycle = (((NAND_CYCLE_DELAY + Tcycle / 2 + aml_chip->T_RHOH * 10) * 10) / sys_time);
			end_cycle = end_cycle / 10;
			if (((start_cycle - bus_cycle) > 7) || ((bus_cycle - end_cycle) > 8)) 
				continue;

			if ((((start_cycle >= 3) && (start_cycle <= ( bus_cycle + 1)))
				|| ((end_cycle >= 3) && (end_cycle <= (bus_cycle + 1))))
				&& (start_cycle <= end_cycle)) {
				break;
			}			
		}
		BUG_ON(bus_cycle > 19);
	}
	else{

		for (i=time_mode_select; i>=0; i--) {
			bus_cycle = nand_mode_time[i];
			Tcycle = bus_cycle * sys_time;
			end_cycle = (((NAND_CYCLE_DELAY + Tcycle / 2 + aml_chip->T_RHOH * 10) * 10) / sys_time);
			end_cycle = end_cycle / 10;
			if ((((start_cycle >= 3) && (start_cycle <= ( bus_cycle + 1)))
				|| ((end_cycle >= 3) && (end_cycle <= (bus_cycle + 1))))
				&& (start_cycle <= end_cycle)) {
				time_mode = i;
				break;
			}
		}
		if (time_mode < 0)
			return;
	}

	if (nand_mode_time[time_mode] < start_cycle)
		adjust = start_cycle - nand_mode_time[time_mode];
	else if(nand_mode_time[time_mode] > end_cycle) 
		adjust = ((((~(nand_mode_time[time_mode] - end_cycle) + 1)) & 0xf) | 0x8);
	else
		adjust = 0;

	NFC_SET_TIMING(time_mode, (bus_cycle - 1), adjust);
	dev_info(aml_chip->device, "time_mode=%d, bus_cycle=%d, adjust=%d, start_cycle=%d, end_cycle=%d,system=%d.%dns\n",
		time_mode, bus_cycle, adjust, start_cycle, end_cycle, sys_time/10, sys_time%10);
}

static int aml_nand_add_partition(struct aml_nand_chip *aml_chip)
{
	uint64_t adjust_offset = 0, mini_part_blk_num, start_blk = 0,key_block = 0;
	struct mtd_info *mtd = &aml_chip->mtd;
	struct aml_nand_platform *plat = aml_chip->platform;
	struct platform_nand_chip *chip = &plat->platform_nand_data.chip;
#ifdef CONFIG_MTD_PARTITIONS
	struct mtd_partition *temp_parts = NULL;
	struct mtd_partition *parts;
	struct erase_info aml_env_erase_info;

	int nr, i, error = 0, part_save_in_env = 1, file_system_part = 0, phys_erase_shift;
	u8 part_num = 0;
	uint64_t erase_length;
	size_t offset;
	uint64_t mini_part_size = ((mtd->erasesize > NAND_MINI_PART_SIZE) ? mtd->erasesize : NAND_MINI_PART_SIZE);
#ifdef CONFIG_AML_NAND_KEY
	//mini_part_size = ((mtd->erasesize > (NAND_MINI_PART_SIZE + NAND_MINIKEY_PART_SIZE)) ? mtd->erasesize : (NAND_MINI_PART_SIZE + NAND_MINIKEY_PART_SIZE));
#endif

	if (chip->set_parts)
		chip->set_parts(mtd->size, chip);
	phys_erase_shift = fls(mtd->erasesize) - 1;
	parts = plat->platform_nand_data.chip.partitions;
	nr = plat->platform_nand_data.chip.nr_partitions;
	if (!strncmp((char*)plat->name, NAND_BOOT_NAME, strlen((const char*)NAND_BOOT_NAME))) {
		if (nr == 0) {
			parts = kzalloc(sizeof(struct mtd_partition), GFP_KERNEL);
			if (!parts)
				return -ENOMEM;
		}	
		parts->name = NAND_BOOT_NAME;
		parts->offset = 0;
		parts->size = (mtd->writesize * 1024);
		nr = 1;
		nand_boot_flag = 1;
	}
	else {
		if (nand_boot_flag)
			adjust_offset = (1024 * mtd->writesize / aml_chip->plane_num);
#ifdef NEW_NAND_SUPPORT
		if((aml_chip->new_nand_info.type) && (aml_chip->new_nand_info.type < 10))
			adjust_offset += RETRY_NAND_BLK_NUM* mtd->erasesize;		
#endif
		part_num++;
		start_blk = adjust_offset / mtd->erasesize;
		if ((NAND_MINI_PART_SIZE / mtd->erasesize) < NAND_MINI_PART_BLOCKNUM)
			mini_part_blk_num = NAND_MINI_PART_BLOCKNUM;
		else
			mini_part_blk_num = (NAND_MINI_PART_SIZE >> phys_erase_shift);
#ifdef CONFIG_AML_NAND_KEY
		//if ((NAND_MINIKEY_PART_SIZE / mtd->erasesize) < NAND_MINIKEY_PART_BLOCKNUM)
		//	mini_part_blk_num += NAND_MINIKEY_PART_BLOCKNUM; //for nand key
		//else
		//	mini_part_blk_num += (NAND_MINIKEY_PART_SIZE >> phys_erase_shift);
		key_block = aml_chip->aml_nandkey_info->end_block - aml_chip->aml_nandkey_info->start_block + 1;
#endif
        
        start_blk = 0;
		do {
			offset = adjust_offset + start_blk * mtd->erasesize;
			error = mtd->block_isbad(mtd, offset);
			if (error) {
				adjust_offset += mtd->erasesize;
				continue;
			}
			start_blk++;
		} while (start_blk < mini_part_blk_num);
		adjust_offset += mini_part_blk_num * mtd->erasesize;

		if (nr == 0) {
			part_save_in_env = 0;
			if (nand_boot_flag)
				nr = NAND_MINI_PART_NUM + 1;
			else
				nr = 2;
			parts = kzalloc((nr * sizeof(struct mtd_partition)), GFP_KERNEL);
			if (!parts)
				return -ENOMEM;
			mini_part_size = ((mtd->erasesize > NAND_MINI_PART_SIZE) ? mtd->erasesize : NAND_MINI_PART_SIZE);
		}

		for (i=0; i<nr; i++) {

			temp_parts = parts + i;
			if ((temp_parts->size >= mtd->erasesize) || (i == (nr - 1)))
				mini_part_size = temp_parts->size;
			temp_parts->offset = adjust_offset;
			if((aml_chip->update_env_flag)&&((mini_part_size > NAND_SYS_PART_SIZE))&&(file_system_part == 0)){
				
				memset(&aml_env_erase_info, 0, sizeof(struct erase_info));
				aml_env_erase_info.mtd = mtd;
				aml_env_erase_info.addr = temp_parts->offset;
				aml_env_erase_info.len = mtd->erasesize;
				//erase_length = mtd->size - temp_parts->offset;
				//erase_length = mtd->size - temp_parts->offset -(key_block+1) * mtd->erasesize;
                erase_length = mtd->size - temp_parts->offset - key_block * mtd->erasesize;
                
				for (;aml_env_erase_info.addr < temp_parts->offset + erase_length; aml_env_erase_info.addr += mtd->erasesize){
						
						error = mtd->block_isbad(mtd, aml_env_erase_info.addr);
						if (error){
							printk("Skipping bad block at 0x%08llx \n",aml_env_erase_info.addr);
							continue;
						}
						#if 0
						error = mtd->erase(mtd, &aml_env_erase_info);
						if (error) {
							printk("nand erase failed %d\n", error);
							mtd->block_markbad(mtd, temp_parts->offset);
							continue;
						}
						#endif
					}
				file_system_part = 1;
				aml_chip->update_env_flag = 0;
			}
			if (i < (nr -1)) {
				start_blk = 0;
				do {
					offset = adjust_offset + start_blk * mtd->erasesize;
					error = mtd->block_isbad(mtd, offset);
					if (error == FACTORY_BAD_BLOCK_ERROR) {
						adjust_offset += mtd->erasesize;
						continue;
					}
					start_blk++;
				} while (start_blk < (mini_part_size >> phys_erase_shift));
			}

			if ((i == (nr - 1)) && (part_save_in_env == 0))
				temp_parts->size = NAND_SYS_PART_SIZE;
			else if (mini_part_size != MTDPART_SIZ_FULL)
				temp_parts->size = mini_part_size + (adjust_offset - temp_parts->offset);
			adjust_offset += mini_part_size;

			if (temp_parts->name == NULL) {
				temp_parts->name = kzalloc(MAX_MTD_PART_NAME_LEN, GFP_KERNEL);
				if (!temp_parts->name)
					return -ENOMEM;
				sprintf(temp_parts->name, "mtd%d", part_num++);
			}
		}
#ifdef CONFIG_AML_NAND_KEY
		temp_parts = parts + (nr-1);
		//key_block = aml_chip->aml_nandkey_info->end_block - aml_chip->aml_nandkey_info->start_block + 1;

		if(temp_parts->size == MTDPART_SIZ_FULL){
			//printk("mtd->size:%llx,temp_parts->offset:%llx,key_block*mtd->erasesize:%x,%s:%d\n",mtd->size,temp_parts->offset,key_block*mtd->erasesize,__func__,__LINE__);
			temp_parts->size = mtd->size - temp_parts->offset - key_block*mtd->erasesize;
			//printk("temp_parts->size:%llx,temp_parts->offset:%llx,key_block*mtd->erasesize:%x,%s:%d\n",temp_parts->size,temp_parts->offset,key_block*mtd->erasesize,__func__,__LINE__);
		}
		else{
			temp_parts->size -= key_block*mtd->erasesize;
			//printk("temp_parts->size:%llx,temp_parts->offset:%llx,key_block*mtd->erasesize:%x,%s:%d\n",temp_parts->size,temp_parts->offset,key_block*mtd->erasesize,__func__,__LINE__);
		}
#endif
	}

	return add_mtd_partitions(mtd, parts, nr);
#else
	return add_mtd_device(mtd);
#endif
}

static void aml_nand_select_chip(struct mtd_info *mtd, int chipnr)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	if (((nand_erarly_suspend_flag == 1) && (!(READ_CBUS_REG(HHI_MPEG_CLK_CNTL)&(1<<8)))) 
		|| ((READ_CBUS_REG(HHI_MPEG_CLK_CNTL)&(1<<8)) && (nand_erarly_suspend_flag == 2))) {

		aml_chip->aml_nand_adjust_timing(aml_chip);
		if (nand_erarly_suspend_flag == 1)
			nand_erarly_suspend_flag = 2;
		else if (nand_erarly_suspend_flag == 2)
			nand_erarly_suspend_flag = 0;
	}
	switch (chipnr) {
		case -1:
			nand_release_chip();
			break;
		case 0:
			nand_get_chip();
			aml_chip->aml_nand_select_chip(aml_chip, chipnr);
			break;
		case 1:
		case 2:
		case 3:
			aml_chip->aml_nand_select_chip(aml_chip, chipnr);
			break;

		default:		
			BUG();
}
		return;
}

static void aml_platform_select_chip(struct aml_nand_chip *aml_chip, int chipnr)
{
	int i;
	switch (chipnr) {
		case 0:
		case 1:
		case 2:
		case 3:
			aml_chip->chip_selected = aml_chip->chip_enable[chipnr];
			aml_chip->rb_received = aml_chip->rb_enable[chipnr];

			for (i=1; i<aml_chip->chip_num; i++) {

				if (aml_chip->valid_chip[i]) {

					if (!((aml_chip->chip_enable[i] >> 10) & 1))
						SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_6, (1 << 4));
					if (!((aml_chip->chip_enable[i] >> 10) & 2))
						SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_6, (1 << 3));
					if (!((aml_chip->chip_enable[i] >> 10) & 4))
						SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_6, (1 << 14));
					if (!((aml_chip->chip_enable[i] >> 10) & 8))
						SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_6, (1 << 13));

					if (!((aml_chip->rb_enable[i] >> 10) & 1))
						SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_6, (1 << 2));
					if (!((aml_chip->rb_enable[i] >> 10) & 2))
						SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_6, (1 << 1));
					if (!((aml_chip->rb_enable[i] >> 10) & 4))
						SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_6, (1 << 12));
					if (!((aml_chip->rb_enable[i] >> 10) & 8))
						SET_CBUS_REG_MASK(PERIPHS_PIN_MUX_6, (1 << 11));
				}
			}

			NFC_SEND_CMD_IDLE(aml_chip->chip_selected, 0);

			break;

		default:
			BUG();
			aml_chip->chip_selected = CE_NOT_SEL;
			break;
	}
	return;
}

static void aml_platform_cmd_ctrl(struct aml_nand_chip *aml_chip, int cmd,  unsigned int ctrl)
{
	if (cmd == NAND_CMD_NONE)
	return;
#ifdef CONFIG_CLK81_DFS
    down(&aml_chip->nand_sem);
#endif

	if (ctrl & NAND_CLE)
		cmd=NFC_CMD_CLE(aml_chip->chip_selected, cmd);
	else
		cmd=NFC_CMD_ALE(aml_chip->chip_selected, cmd);

    NFC_SEND_CMD(cmd);
#ifdef CONFIG_CLK81_DFS
    up(&aml_chip->nand_sem);
#endif    
}

static int aml_platform_wait_devready(struct aml_nand_chip *aml_chip, int chipnr)
{
	struct nand_chip *chip = &aml_chip->chip;
	struct mtd_info *mtd = &aml_chip->mtd;
	unsigned time_out_cnt = 0;
	int status;

	/* wait until command is processed or timeout occures */
	aml_chip->aml_nand_select_chip(aml_chip, chipnr);
#if 1
	if (aml_chip->ops_mode & AML_CHIP_NONE_RB) {
	
		NFC_SEND_CMD(aml_chip->chip_selected | IDLE | 0);
		NFC_SEND_CMD(aml_chip->chip_selected | IDLE | 0);
		while(NFC_CMDFIFO_SIZE()>0);
		
		aml_chip->aml_nand_command(aml_chip, NAND_CMD_STATUS, -1, -1, chipnr);    		
		udelay(2);    		
		NFC_SEND_CMD(aml_chip->chip_selected | IDLE | 0);
		NFC_SEND_CMD(aml_chip->chip_selected | IDLE | 0);
		while(NFC_CMDFIFO_SIZE()>0);
		do{
			
			status = (int)chip->read_byte(mtd);    		
			if (status & NAND_STATUS_READY)    			
				break;    		
			udelay(1);    	
		}while(time_out_cnt++ <= 0x1000);   //10ms max	    	    

		if (time_out_cnt > 0x1000)		    
			return 0;   		
	}
	else{
		do{		
			if (chip->dev_ready(mtd))
				break;
		} while (time_out_cnt++ <= AML_NAND_BUSY_TIMEOUT);		

			if (time_out_cnt > AML_NAND_BUSY_TIMEOUT)
		return 0;
	}
#else
	do {
		if (aml_chip->ops_mode & AML_CHIP_NONE_RB) {
			//udelay(chip->chip_delay);
			aml_chip->aml_nand_command(aml_chip, NAND_CMD_STATUS, -1, -1, chipnr);
			udelay(2);
			status = (int)chip->read_byte(mtd);
			if (status & NAND_STATUS_READY)
				break;
			udelay(20);
		}
		else {
			if (chip->dev_ready(mtd))
				break;
		}

	} while (time_out_cnt++ <= AML_NAND_BUSY_TIMEOUT);

	if (time_out_cnt > AML_NAND_BUSY_TIMEOUT)
		return 0;
#endif
	return 1;
}

static int aml_nand_dev_ready(struct mtd_info *mtd)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	return NFC_GET_RB_STATUS(aml_chip->rb_received);
		}

static int aml_nand_verify_buf(struct mtd_info *mtd, const uint8_t *buf, int len)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	struct nand_chip *chip = mtd->priv;

	chip->read_buf(mtd, aml_chip->aml_nand_data_buf, len);
	if (memcmp(buf, aml_chip->aml_nand_data_buf, len))
		return -EFAULT;

	return 0;
	}

static void aml_nand_cmd_ctrl(struct mtd_info *mtd, int cmd,  unsigned int ctrl)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);

	aml_chip->aml_nand_cmd_ctrl(aml_chip, cmd, ctrl);
}

static int aml_nand_wait(struct mtd_info *mtd, struct nand_chip *chip)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	int status[MAX_CHIP_NUM], state = chip->state, i = 0, time_cnt = 0, chip_nr = 1;
	struct aml_nand_platform *plat = aml_chip->platform; 

	/* Apply this short delay always to ensure that we do wait tWB in
	 * any case on any machine. */
	ndelay(100);

	if (state == FL_ERASING)
		chip_nr = aml_chip->chip_num;
	for (i=0; i<chip_nr; i++) {
		if (aml_chip->valid_chip[i]) {
			//active ce for operation chip and send cmd
			aml_chip->aml_nand_select_chip(aml_chip, i);

			NFC_SEND_CMD(aml_chip->chip_selected | IDLE | 0);
			NFC_SEND_CMD(aml_chip->chip_selected | IDLE | 0);
			while(NFC_CMDFIFO_SIZE()>0);
			if ((state == FL_ERASING) && (chip->options & NAND_IS_AND))
				aml_chip->aml_nand_command(aml_chip, NAND_CMD_STATUS_MULTI, -1, -1, i);
			else
				aml_chip->aml_nand_command(aml_chip, NAND_CMD_STATUS, -1, -1, i);
			NFC_SEND_CMD(aml_chip->chip_selected | IDLE | 0);
			NFC_SEND_CMD(aml_chip->chip_selected | IDLE | 0);
			while(NFC_CMDFIFO_SIZE()>0);

			time_cnt = 0;
			while (time_cnt++ < 0x40000) {
				if (chip->dev_ready) {
					if (chip->dev_ready(mtd))
						break;
					udelay(2);
				} else {
					//if(time_cnt == 1)
			                    udelay(2);
					if (chip->read_byte(mtd) & NAND_STATUS_READY) {
					    break;
					}
				}					
			}
			status[i] = (int)chip->read_byte(mtd);

			status[0] |= status[i];
		}
	}
     if (!strncmp((char*)plat->name, NAND_BOOT_NAME, strlen((const char*)NAND_BOOT_NAME)))
         status[0] = 0xe0;
         
	return status[0];
}

static void aml_nand_base_command(struct aml_nand_chip *aml_chip, unsigned command, int column, int page_addr, int chipnr)
{
	struct nand_chip *chip = &aml_chip->chip;
	struct mtd_info *mtd = &aml_chip->mtd;
	unsigned command_temp, pages_per_blk_shift, plane_page_addr = 0, plane_blk_addr = 0;
	//unsigned time_out_cnt = 0;
	uint32_t rst_sts_cnt = CONFIG_SYS_NAND_RESET_CNT;
	pages_per_blk_shift = (chip->phys_erase_shift - chip->page_shift);

	if (page_addr != -1) {
		page_addr /= aml_chip->plane_num;
		plane_page_addr = (page_addr & ((1 << pages_per_blk_shift) - 1));
		plane_blk_addr = (page_addr >> pages_per_blk_shift);
		plane_blk_addr = (plane_blk_addr << 1);
	}

	if (aml_chip->plane_num == 2) {

		switch (command) {

			case NAND_CMD_READ0:
				if ((aml_chip->mfr_type == NAND_MFR_MICRON) || (aml_chip->mfr_type == NAND_MFR_INTEL)) {
					command_temp = command;
				}
				else {
					command_temp = NAND_CMD_TWOPLANE_PREVIOS_READ;
					column = -1;
				}
				plane_page_addr |= (plane_blk_addr << pages_per_blk_shift);
				break;

			case NAND_CMD_TWOPLANE_READ1:
				command_temp = NAND_CMD_READ0;
				if ((aml_chip->mfr_type == NAND_MFR_MICRON) || (aml_chip->mfr_type == NAND_MFR_INTEL))
					//plane_page_addr |= ((plane_blk_addr + 1) << 8);
					return;
				else
					plane_page_addr |= (plane_blk_addr << pages_per_blk_shift);
				break;

			case NAND_CMD_TWOPLANE_READ2:
				if ((aml_chip->mfr_type == NAND_MFR_MICRON) || (aml_chip->mfr_type == NAND_MFR_INTEL)) {
					command_temp = NAND_CMD_PLANE2_READ_START;
				}
				else {
					command_temp = NAND_CMD_READ0;
				}
				plane_page_addr |= ((plane_blk_addr + 1) << pages_per_blk_shift);
				break;

			case NAND_CMD_SEQIN:
				command_temp = command;
				plane_page_addr |= (plane_blk_addr << pages_per_blk_shift);
				break;

			case NAND_CMD_TWOPLANE_WRITE2:
				if ((aml_chip->mfr_type == NAND_MFR_HYNIX) || (aml_chip->mfr_type == NAND_MFR_SAMSUNG))
					command_temp = command;
				else
					command_temp = NAND_CMD_TWOPLANE_WRITE2_MICRO;
				plane_page_addr |= ((plane_blk_addr + 1) << pages_per_blk_shift);
				break;

			case NAND_CMD_ERASE1:
				command_temp = command;
				plane_page_addr |= (plane_blk_addr << pages_per_blk_shift);
				break;

			case NAND_CMD_MULTI_CHIP_STATUS:
				command_temp = command;
				plane_page_addr |= (plane_blk_addr << pages_per_blk_shift);
				break;

			default:
				command_temp = command;
				break;

		}
		chip->cmd_ctrl(mtd, command_temp & 0xff, NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
		//if ((command_temp == NAND_CMD_SEQIN) || (command_temp == NAND_CMD_TWOPLANE_WRITE2) || (command_temp == NAND_CMD_READ0))
			//printk(" NAND plane_page_addr: %x plane_blk_addr %x command: %x \n", plane_page_addr, plane_blk_addr, command);

		if (column != -1 || page_addr != -1) {
			int ctrl = NAND_CTRL_CHANGE | NAND_NCE | NAND_ALE;

			/* Serially input address */
			if (column != -1) {
				/* Adjust columns for 16 bit buswidth */
				if (chip->options & NAND_BUSWIDTH_16)
					column >>= 1;
				chip->cmd_ctrl(mtd, column, ctrl);
				ctrl &= ~NAND_CTRL_CHANGE;
				chip->cmd_ctrl(mtd, column >> 8, ctrl);
			}
			if (page_addr != -1) {

				chip->cmd_ctrl(mtd, plane_page_addr, ctrl);
				chip->cmd_ctrl(mtd, plane_page_addr >> 8, NAND_NCE | NAND_ALE);
				/* One more address cycle for devices > 128MiB */
				if (chip->chipsize > (128 << 20))
					chip->cmd_ctrl(mtd, plane_page_addr >> 16, NAND_NCE | NAND_ALE);
			}
		}

		switch (command) {

			case NAND_CMD_READ0:
				plane_page_addr = page_addr % (1 << pages_per_blk_shift);
				
				if ((aml_chip->mfr_type == NAND_MFR_MICRON) || (aml_chip->mfr_type == NAND_MFR_INTEL)) {
					plane_page_addr |= ((plane_blk_addr + 1) << pages_per_blk_shift);
					command_temp = command;
					chip->cmd_ctrl(mtd, command_temp & 0xff, NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
				}
				else {
					command_temp = NAND_CMD_TWOPLANE_PREVIOS_READ;
					column = -1;
					plane_page_addr |= ((plane_blk_addr + 1) << pages_per_blk_shift);
					chip->cmd_ctrl(mtd, command_temp & 0xff, NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
				}

				break;

			case NAND_CMD_TWOPLANE_READ1:
				if ((aml_chip->mfr_type == NAND_MFR_MICRON) || (aml_chip->mfr_type == NAND_MFR_INTEL)) {
					page_addr = -1;
					column = -1;
				}
				else {
					command_temp = NAND_CMD_RNDOUT;
					page_addr = -1;
					chip->cmd_ctrl(mtd, command_temp & 0xff, NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
				}
				break;

			case NAND_CMD_TWOPLANE_READ2:
				if ((aml_chip->mfr_type == NAND_MFR_MICRON) || (aml_chip->mfr_type == NAND_MFR_INTEL)) {
					page_addr = -1;
					column = -1;
				}
				else {
					command_temp = NAND_CMD_RNDOUT;
					page_addr = -1;
					chip->cmd_ctrl(mtd, command_temp & 0xff, NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
				}
				break;

			case NAND_CMD_ERASE1:
				if ((aml_chip->mfr_type == NAND_MFR_MICRON) || (aml_chip->mfr_type == NAND_MFR_INTEL)) {
					command_temp = NAND_CMD_ERASE1_END;
					chip->cmd_ctrl(mtd, command_temp & 0xff, NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
					aml_chip->aml_nand_wait_devready(aml_chip, chipnr);
				}

				command_temp = command;
				chip->cmd_ctrl(mtd, command_temp & 0xff, NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
				plane_page_addr = page_addr % (1 << pages_per_blk_shift);
				plane_page_addr |= ((plane_blk_addr + 1) << pages_per_blk_shift);
				break;

			default:
				column = -1;
				page_addr = -1;
				break;
		}

		if (column != -1 || page_addr != -1) {
			int ctrl = NAND_CTRL_CHANGE | NAND_NCE | NAND_ALE;

			/* Serially input address */
			if (column != -1) {
				/* Adjust columns for 16 bit buswidth */
				if (chip->options & NAND_BUSWIDTH_16)
					column >>= 1;
				chip->cmd_ctrl(mtd, column, ctrl);
				ctrl &= ~NAND_CTRL_CHANGE;
				chip->cmd_ctrl(mtd, column >> 8, ctrl);
			}
			if (page_addr != -1) {
				//plane_page_addr |= (1 << (pages_per_blk_shift + 1));
				//BUG_ON((plane_page_addr & 0x7FF) == 0);

				chip->cmd_ctrl(mtd, plane_page_addr, ctrl);
				chip->cmd_ctrl(mtd, plane_page_addr >> 8, NAND_NCE | NAND_ALE);
				/* One more address cycle for devices > 128MiB */
				if (chip->chipsize > (128 << 20))
					chip->cmd_ctrl(mtd, plane_page_addr >> 16, NAND_NCE | NAND_ALE);
			}
		}

		if ((command == NAND_CMD_RNDOUT) || (command == NAND_CMD_TWOPLANE_READ2))
			chip->cmd_ctrl(mtd, NAND_CMD_RNDOUTSTART, NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
		else if ((command == NAND_CMD_TWOPLANE_READ1)) {
			chip->cmd_ctrl(mtd, NAND_CMD_RNDOUTSTART, NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
		}
		else if (command == NAND_CMD_READ0) {
			chip->cmd_ctrl(mtd, NAND_CMD_READSTART, NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
		}

		chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);
	}
	else {
		if(( aml_chip->new_nand_info.dynamic_read_info.slc_flag  == 1 ) &&( aml_chip->new_nand_info.type == SANDISK_19NM ) &&( (command == NAND_CMD_ERASE1)
			|| (command == NAND_CMD_READ0)|| (command == NAND_CMD_SEQIN) )){
			//printk("aml_nand_base_command: sent NAND_CMD_SANDISK_SLC command\n");
			chip->cmd_ctrl(mtd, NAND_CMD_SANDISK_SLC & 0xff, NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
			udelay(2);
		}
		chip->cmd_ctrl(mtd, command & 0xff, NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE); // 

		if (column != -1 || page_addr != -1) {
			int ctrl = NAND_CTRL_CHANGE | NAND_NCE | NAND_ALE;

			/* Serially input address */
			if (column != -1) {
				/* Adjust columns for 16 bit buswidth */
				if (chip->options & NAND_BUSWIDTH_16)
					column >>= 1;
				chip->cmd_ctrl(mtd, column, ctrl);
				ctrl &= ~NAND_CTRL_CHANGE;
				chip->cmd_ctrl(mtd, column >> 8, ctrl);
			}
			if (page_addr != -1) {

				chip->cmd_ctrl(mtd, page_addr, ctrl);
				chip->cmd_ctrl(mtd, page_addr >> 8, NAND_NCE | NAND_ALE);
				/* One more address cycle for devices > 128MiB */
				if (chip->chipsize > (128 << 20))
					chip->cmd_ctrl(mtd, page_addr >> 16, NAND_NCE | NAND_ALE);
			}
		}
		if (command == NAND_CMD_RNDOUT)
			chip->cmd_ctrl(mtd, NAND_CMD_RNDOUTSTART, NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
		else if (command == NAND_CMD_READ0)
			chip->cmd_ctrl(mtd, NAND_CMD_READSTART, NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);

		chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);
	}

	/*
	 * program and erase have their own busy handlers
	 * status, sequential in, and deplete1 need no delay
	 */
	switch (command) {

	case NAND_CMD_CACHEDPROG:
	case NAND_CMD_PAGEPROG:
	case NAND_CMD_ERASE1:
	case NAND_CMD_ERASE2:
	case NAND_CMD_SEQIN:
	case NAND_CMD_RNDIN:
	case NAND_CMD_STATUS:
	case NAND_CMD_DEPLETE1:
		return;

		/*
		 * read error status commands require only a short delay
		 */
	case NAND_CMD_STATUS_ERROR:
	case NAND_CMD_STATUS_ERROR0:
	case NAND_CMD_STATUS_ERROR1:
	case NAND_CMD_STATUS_ERROR2:
	case NAND_CMD_STATUS_ERROR3:
		udelay(chip->chip_delay);
		return;

	case NAND_CMD_RESET:
		if (!aml_chip->aml_nand_wait_devready(aml_chip, chipnr))
			aml_nand_debug ("couldn't found selected chip: %d ready\n", chipnr);
 		if (chip->dev_ready)
			break;
		udelay(chip->chip_delay);
		chip->cmd_ctrl(mtd, NAND_CMD_STATUS, NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
		chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);

	//while( (!(chip->read_byte(mtd) & NAND_STATUS_READY)) ;
	//	return;
		while (!(chip->read_byte(mtd) & NAND_STATUS_READY) &&
			(rst_sts_cnt--));
		return;

	default:
		/*
		 * If we don't have access to the busy pin, we apply the given
		 * command delay
		 */
		break;
	}

	/* Apply this short delay always to ensure that we do wait tWB in
	 * any case on any machine. */
	ndelay(100);
}

static void aml_nand_command(struct mtd_info *mtd, unsigned command, int column, int page_addr)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	struct nand_chip *chip = &aml_chip->chip;
	int i = 0, valid_page_num = 1, internal_chip;

	if (page_addr != -1) {
		valid_page_num = (mtd->writesize >> chip->page_shift);
		valid_page_num /= aml_chip->plane_num;

		aml_chip->page_addr = page_addr / valid_page_num;
		if (unlikely(aml_chip->page_addr >= aml_chip->internal_page_nums)) {
			internal_chip = aml_chip->page_addr / aml_chip->internal_page_nums; 
			aml_chip->page_addr -= aml_chip->internal_page_nums;
			aml_chip->page_addr |= (1 << aml_chip->internal_chip_shift) * internal_chip;
		}
	}

	/* Emulate NAND_CMD_READOOB */
	if (command == NAND_CMD_READOOB) {
		command = NAND_CMD_READ0;
		aml_chip->aml_nand_wait_devready(aml_chip, 0);
		aml_chip->aml_nand_command(aml_chip, command, column, aml_chip->page_addr, 0);
		return;
	}
	if (command == NAND_CMD_PAGEPROG)
		return;

	if (command == NAND_CMD_SEQIN) {
		aml_chip->aml_nand_wait_devready(aml_chip, 0);
		aml_chip->aml_nand_command(aml_chip, command, column, aml_chip->page_addr, 0);
		return;
	}

	for (i=0; i<aml_chip->chip_num; i++) {
		if (aml_chip->valid_chip[i]) {
			//active ce for operation chip and send cmd
			aml_chip->aml_nand_wait_devready(aml_chip, i);
			aml_chip->aml_nand_command(aml_chip, command, column, aml_chip->page_addr, i);
		}
	}

	return;
}

static void aml_nand_erase_cmd(struct mtd_info *mtd, int page)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	struct nand_chip *chip = mtd->priv;
	unsigned pages_per_blk_shift = (chip->phys_erase_shift - chip->page_shift);
	unsigned vt_page_num, i = 0, j = 0, internal_chipnr = 1, page_addr, valid_page_num;

	vt_page_num = (mtd->writesize / (1 << chip->page_shift));
	vt_page_num *= (1 << pages_per_blk_shift);
	if (page % vt_page_num)
		return;

	/* Send commands to erase a block */
	valid_page_num = (mtd->writesize >> chip->page_shift);
	valid_page_num /= aml_chip->plane_num;

	aml_chip->page_addr = page / valid_page_num;
	if (unlikely(aml_chip->page_addr >= aml_chip->internal_page_nums)) {
		internal_chipnr = aml_chip->page_addr / aml_chip->internal_page_nums;
		aml_chip->page_addr -= aml_chip->internal_page_nums;
		aml_chip->page_addr |= (1 << aml_chip->internal_chip_shift) * internal_chipnr;
	}

	if (unlikely(aml_chip->ops_mode & AML_INTERLEAVING_MODE))
		internal_chipnr = aml_chip->internal_chipnr;
	else
		internal_chipnr = 1;

	for (i=0; i<aml_chip->chip_num; i++) {
		if (aml_chip->valid_chip[i]) {

			aml_chip->aml_nand_select_chip(aml_chip, i);
			page_addr = aml_chip->page_addr;
			for (j=0; j<internal_chipnr; j++) {
				if (j > 0) {
					page_addr = aml_chip->page_addr;
					page_addr |= (1 << aml_chip->internal_chip_shift) * j;
				}

				aml_chip->aml_nand_command(aml_chip, NAND_CMD_ERASE1, -1, page_addr, i);
				aml_chip->aml_nand_command(aml_chip, NAND_CMD_ERASE2, -1, -1, i);
			}
		}
	}

	return ;
}

#ifdef CONFIG_ARCH_MESON
static int aml_platform_dma_waiting(struct aml_nand_chip *aml_chip)
{
	unsigned time_out_cnt = 0;

	NFC_SEND_CMD_IDLE(aml_chip->chip_selected, 0);
	NFC_SEND_CMD_IDLE(aml_chip->chip_selected, 0);
	do {
		if (NFC_CMDFIFO_SIZE() <= 0)
			break;
	}while (time_out_cnt++ <= AML_DMA_BUSY_TIMEOUT);

	if (time_out_cnt < AML_DMA_BUSY_TIMEOUT)
		return 0;

	return -EBUSY;
}

static int aml_platform_hwecc_correct(struct aml_nand_chip *aml_chip, unsigned char *buf, unsigned size, unsigned char *oob_buf)
{
	struct nand_chip *chip = &aml_chip->chip;
	struct mtd_info *mtd = &aml_chip->mtd;
	unsigned ecc_step_num;
	int error = 0;

	if (size % chip->ecc.size) {
		printk ("error parameter size for ecc correct %x\n", size);
		return -EINVAL;
	}

	 for (ecc_step_num = 0; ecc_step_num < (size / chip->ecc.size); ecc_step_num++) {
	 	//check if there have uncorrectable sector
	 	if (NAND_ECC_FAIL(aml_chip->user_info_buf[ecc_step_num]) 
	 		|| (NAND_ECC_CNT(aml_chip->user_info_buf[ecc_step_num]) == 0x1f)) {
	 		printk ("nand communication have uncorrectable ecc error %d %d %d\n", ecc_step_num, NAND_ECC_CNT(aml_chip->user_info_buf[ecc_step_num-1]), NAND_ECC_CNT(aml_chip->user_info_buf[ecc_step_num+1]));
	 		error = -EIO;
	 	}
	 	else {
			mtd->ecc_stats.corrected += NAND_ECC_CNT(aml_chip->user_info_buf[ecc_step_num]);
		}
	}

	return error;
}

static int aml_platform_dma_write(struct aml_nand_chip *aml_chip, unsigned char *buf, int len, unsigned bch_mode)
{
	int ret = 0;
	memcpy(aml_chip->aml_nand_data_buf, buf, len);
	wmb();

	NFC_SEND_CMD_ADL(aml_chip->data_dma_addr);
	NFC_SEND_CMD_ADH(aml_chip->data_dma_addr);
	NFC_SEND_CMD_AIL(aml_chip->nand_info_dma_addr);
	NFC_SEND_CMD_AIH((aml_chip->nand_info_dma_addr));

	NFC_SEND_CMD_M2N(len, bch_mode);	

	ret = aml_platform_dma_waiting(aml_chip);

	return ret;		 
}

static int aml_platform_dma_read(struct aml_nand_chip *aml_chip, unsigned char *buf, int len, unsigned bch_mode)
{
	volatile unsigned int * info_buf;
	struct nand_chip *chip = &aml_chip->chip;
	unsigned dma_unit_size;
	int ret = 0;

	if (chip->ecc.size >= 512)
		dma_unit_size = chip->ecc.size;
	else
		dma_unit_size = 512;
	info_buf = aml_chip->user_info_buf + (((len + dma_unit_size - 1) / dma_unit_size) - 1);
	memset((unsigned char *)aml_chip->user_info_buf, 0, ((len + dma_unit_size - 1) / dma_unit_size)*sizeof(int));
	wmb();

	NFC_SEND_CMD_ADL(aml_chip->data_dma_addr);
	NFC_SEND_CMD_ADH(aml_chip->data_dma_addr);
	NFC_SEND_CMD_AIL(aml_chip->nand_info_dma_addr);
	NFC_SEND_CMD_AIH((aml_chip->nand_info_dma_addr));

	NFC_SEND_CMD_N2M(len, bch_mode);

	ret = aml_platform_dma_waiting(aml_chip);
	if (ret)
		return ret;
	while(NAND_INFO_DONE(*info_buf) == 0);
	rmb();

	if (buf != aml_chip->aml_nand_data_buf)
		memcpy(buf, aml_chip->aml_nand_data_buf, len);
	wmb();
	return 0;
}
#else
static int aml_platform_hwecc_correct(struct aml_nand_chip *aml_chip, unsigned char *buf, unsigned size, unsigned char *oob_buf)
{
	return 0;
}

static int aml_platform_dma_write(struct aml_nand_chip *aml_chip, unsigned char *buf, int len, unsigned bch_mode)
{
	return 0;		 
}

static int aml_platform_dma_read(struct aml_nand_chip *aml_chip, unsigned char *buf, int len, unsigned bch_mode)
{
	return 0;
}
#endif

static void aml_nand_dma_read_buf(struct mtd_info *mtd, uint8_t *buf, int len)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	
	aml_chip->aml_nand_dma_read(aml_chip, buf, len, 0);
}

static void aml_nand_dma_write_buf(struct mtd_info *mtd, const uint8_t *buf, int len)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);

	aml_chip->aml_nand_dma_write(aml_chip, (unsigned char *)buf, len, 0);
}

static int aml_nand_read_page_raw(struct mtd_info *mtd, struct nand_chip *chip, uint8_t *buf, int page)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	unsigned nand_page_size = aml_chip->page_size;
	unsigned nand_oob_size = aml_chip->oob_size;
	uint8_t *oob_buf = chip->oob_poi;
	int i, error = 0, j = 0, page_addr, internal_chipnr = 1;

	if (aml_chip->ops_mode & AML_INTERLEAVING_MODE)
		internal_chipnr = aml_chip->internal_chipnr;

	for (i=0; i<aml_chip->chip_num; i++) {
		if (aml_chip->valid_chip[i]) {

			page_addr = aml_chip->page_addr;
			for (j=0; j<internal_chipnr; j++) {

				if (j > 0) {
					page_addr = aml_chip->page_addr;
					page_addr |= (1 << aml_chip->internal_chip_shift) * j;
					aml_chip->aml_nand_select_chip(aml_chip, i);
					aml_chip->aml_nand_command(aml_chip, NAND_CMD_READ0, 0, page_addr, i);
				}

				if (!aml_chip->aml_nand_wait_devready(aml_chip, i)) {
					printk ("couldn`t found selected chip: %d ready\n", i);
					error = -EBUSY;
					goto exit;
				}
				if (aml_chip->ops_mode & AML_CHIP_NONE_RB) 
					chip->cmd_ctrl(mtd, NAND_CMD_READ0 & 0xff, NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);

				if (aml_chip->plane_num == 2) {

					aml_chip->aml_nand_command(aml_chip, NAND_CMD_TWOPLANE_READ1, 0x00, page_addr, i);
					aml_chip->aml_nand_dma_read(aml_chip, aml_chip->aml_nand_data_buf, nand_page_size + nand_oob_size, 0);
					memcpy(buf, aml_chip->aml_nand_data_buf, (nand_page_size + nand_oob_size));
					memcpy(oob_buf, aml_chip->aml_nand_data_buf + nand_page_size, nand_oob_size);

					oob_buf += nand_oob_size;
					buf += (nand_page_size + nand_oob_size);

					aml_chip->aml_nand_command(aml_chip, NAND_CMD_TWOPLANE_READ2, 0x00, page_addr, i);
					aml_chip->aml_nand_dma_read(aml_chip, aml_chip->aml_nand_data_buf, nand_page_size + nand_oob_size, 0);
					memcpy(buf, aml_chip->aml_nand_data_buf, (nand_page_size + nand_oob_size));
					memcpy(oob_buf, aml_chip->aml_nand_data_buf + nand_page_size, nand_oob_size);

					oob_buf += nand_oob_size;
					buf += (nand_page_size + nand_oob_size);
				}
				else if (aml_chip->plane_num == 1) {

					aml_chip->aml_nand_dma_read(aml_chip, aml_chip->aml_nand_data_buf, nand_page_size + nand_oob_size, 0);
					memcpy(buf, aml_chip->aml_nand_data_buf, (nand_page_size + nand_oob_size));
					memcpy(oob_buf, aml_chip->aml_nand_data_buf + nand_page_size, nand_oob_size);

					oob_buf += nand_oob_size;
					buf += nand_page_size;
				}
				else {
					error = -ENODEV;
					aml_nand_debug ("plane_num mistake\n");
					goto exit;
				}
			}
		}
	}

exit:
	return error;
}

static void aml_nand_write_page_raw(struct mtd_info *mtd, struct nand_chip *chip, const uint8_t *buf)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	unsigned nand_page_size = aml_chip->page_size;
	unsigned nand_oob_size = aml_chip->oob_size;
	uint8_t *oob_buf = chip->oob_poi;
	int i, error = 0, j = 0, page_addr, internal_chipnr = 1;

	if (aml_chip->ops_mode & AML_INTERLEAVING_MODE)
		internal_chipnr = aml_chip->internal_chipnr;

	for (i=0; i<aml_chip->chip_num; i++) {
		if (aml_chip->valid_chip[i]) {

			aml_chip->aml_nand_select_chip(aml_chip, i);

			page_addr = aml_chip->page_addr;
			for (j=0; j<internal_chipnr; j++) {

				if (j > 0) {
					page_addr = aml_chip->page_addr;
					page_addr |= (1 << aml_chip->internal_chip_shift) * j;
					aml_chip->aml_nand_command(aml_chip, NAND_CMD_SEQIN, 0, page_addr, i);
				}
	
				if (aml_chip->plane_num == 2) {
	
					memcpy(aml_chip->aml_nand_data_buf, buf, nand_page_size);
					memcpy(aml_chip->aml_nand_data_buf + nand_page_size, oob_buf, nand_oob_size);
					aml_chip->aml_nand_dma_write(aml_chip, aml_chip->aml_nand_data_buf, nand_page_size + nand_oob_size, 0);
					aml_chip->aml_nand_command(aml_chip, NAND_CMD_DUMMY_PROGRAM, -1, -1, i);
	
					oob_buf += nand_oob_size;
					buf += nand_page_size;
	
					if (!aml_chip->aml_nand_wait_devready(aml_chip, i)) {
						aml_nand_debug ("couldn`t found selected chip: %d ready\n", i);
						error = -EBUSY;
						goto exit;
					}
	
					memcpy(aml_chip->aml_nand_data_buf, buf, nand_page_size);
					memcpy(aml_chip->aml_nand_data_buf + nand_page_size, oob_buf, nand_oob_size);
					aml_chip->aml_nand_command(aml_chip, NAND_CMD_TWOPLANE_WRITE2, 0x00, page_addr, i);
					aml_chip->aml_nand_dma_write(aml_chip, aml_chip->aml_nand_data_buf, nand_page_size + nand_oob_size, 0);
					aml_chip->aml_nand_command(aml_chip, NAND_CMD_PAGEPROG, -1, -1, i);
	
					oob_buf += nand_oob_size;
					buf += nand_page_size;
				}
				else if (aml_chip->plane_num == 1) {
	
					memcpy(aml_chip->aml_nand_data_buf, buf, nand_page_size);
					memcpy(aml_chip->aml_nand_data_buf + nand_page_size, oob_buf, nand_oob_size);
					aml_chip->aml_nand_dma_write(aml_chip, aml_chip->aml_nand_data_buf, nand_page_size + nand_oob_size, 0);
					if (chip->cmdfunc == aml_nand_command)
						aml_chip->aml_nand_command(aml_chip, NAND_CMD_PAGEPROG, -1, -1, i);
	
					oob_buf += nand_oob_size;
					buf += nand_page_size;
				}
				else {
					error = -ENODEV;
					aml_nand_debug ("plane_num mistake\n");
					goto exit;
				}
			}
		}
	}

exit:
	return ;
}

static int aml_nand_read_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip, uint8_t *buf, int page)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	uint8_t *oob_buf = chip->oob_poi;
	unsigned nand_page_size = (1 << chip->page_shift);
	unsigned pages_per_blk_shift = (chip->phys_erase_shift - chip->page_shift);
	int user_byte_num = (((nand_page_size + chip->ecc.size - 1) / chip->ecc.size) * aml_chip->user_byte_mode);
	int error = 0, i = 0, stat = 0, j = 0, page_addr, internal_chipnr = 1;
	//uint8_t *buf_tmp = buf;
	int readretry_failed_cnt = 0, pages_per_blk =  (1 << (chip->phys_erase_shift - chip->page_shift));
	int page_temp ;
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON3
	int ran_mode = aml_chip->ran_mode;
#endif
	int retry_cnt =aml_chip->new_nand_info.read_rety_info.retry_cnt;
	if ((aml_chip->new_nand_info.type == HYNIX_20NM_8GB) || (aml_chip->new_nand_info.type == HYNIX_20NM_4GB))
		retry_cnt = aml_chip->new_nand_info.read_rety_info.retry_cnt *aml_chip->new_nand_info.read_rety_info.retry_cnt;
	
	if (aml_chip->ops_mode & AML_INTERLEAVING_MODE)
		internal_chipnr = aml_chip->internal_chipnr;
	if (nand_page_size > chip->ecc.steps * chip->ecc.size) {
		nand_page_size = chip->ecc.steps * chip->ecc.size;
		user_byte_num = chip->ecc.steps;
	}

	for (i=0; i<aml_chip->chip_num; i++) {
		if (aml_chip->valid_chip[i]) {
			readretry_failed_cnt = 0;
read_retry:

			page_addr = aml_chip->page_addr;
			for (j=0; j<internal_chipnr; j++) {

				if (j > 0) {
					page_addr = aml_chip->page_addr;
					page_addr |= (1 << aml_chip->internal_chip_shift) * j;
					aml_chip->aml_nand_select_chip(aml_chip, i);
					aml_chip->aml_nand_command(aml_chip, NAND_CMD_READ0, 0, page_addr, i);
				}

				if (!aml_chip->aml_nand_wait_devready(aml_chip, i)) {
					printk ("read couldn`t found selected chip: %d ready\n", i);
					mdelay(50);
					if (!aml_chip->aml_nand_wait_devready(aml_chip, i)) 
					{
						printk ("read couldn`t found selected chip: %d ready\n", i);
						mdelay(100);
					error = -EBUSY;
					goto exit;
				}
				}

				if (aml_chip->ops_mode & AML_CHIP_NONE_RB) 
					chip->cmd_ctrl(mtd, NAND_CMD_READ0 & 0xff, NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
				if (aml_chip->plane_num == 2) {

					aml_chip->aml_nand_command(aml_chip, NAND_CMD_TWOPLANE_READ1, 0x00, page_addr, i);
dma_retry_plane0:					
					error = aml_chip->aml_nand_dma_read(aml_chip, buf, nand_page_size, aml_chip->bch_mode);
					if (error){
						mdelay(50);
						printk("aml nand read data ecc plane0 failed at page %d chip %d\n", page_addr, i);
						error = aml_chip->aml_nand_dma_read(aml_chip, buf, nand_page_size, aml_chip->bch_mode);
						if(error){
							mdelay(100);
							goto exit;
						}   
					}

					aml_chip->aml_nand_get_user_byte(aml_chip, oob_buf, user_byte_num);
					stat = aml_chip->aml_nand_hwecc_correct(aml_chip, buf, nand_page_size, oob_buf);
					if (stat < 0) {
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON3
						if(aml_chip->ran_mode && (aml_chip->zero_cnt <  aml_chip->ecc_max)){
							memset(buf, 0xff, nand_page_size);
						    	memset(oob_buf, 0xff, user_byte_num);	
						    	goto plane0_ff;
						 }
						 
					 	if(ran_mode && aml_chip->ran_mode){
							//printk("%s dma retry here at page:%d  blk %d chip %d\n", __func__, page_addr, (page_addr >> pages_per_blk_shift), i);
							aml_chip->ran_mode = 0;
							ndelay(300);
							aml_chip->aml_nand_command(aml_chip, NAND_CMD_RNDOUT, 0, -1, i);
                            				ndelay(500);					
							goto dma_retry_plane0;			    
						 } 							 
#endif						
						memset(buf, 0xff, nand_page_size);
						memset(oob_buf, 0x22, user_byte_num);	

				        	mtd->ecc_stats.failed++;  
					    	printk("aml nand read data ecc plane0 failed at page %d chip %d \n", page_addr, i);
					}
					else{
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON3
						if(aml_chip->ecc_cnt_cur > aml_chip->ecc_cnt_limit){
							printk("%s line:%d uncorrected ecc_cnt_cur:%d, and limit:%d and at page:%d, blk:%d chip[%d]\n",\
											__func__, __LINE__, aml_chip->ecc_cnt_cur, aml_chip->ecc_cnt_limit, page_addr, (page_addr >> pages_per_blk_shift), i);

							mtd->ecc_stats.corrected++;
						}
#endif						
						mtd->ecc_stats.corrected += stat;
					}
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON3
plane0_ff:					
					aml_chip->ran_mode = ran_mode;
#endif
					oob_buf += user_byte_num;
					buf += nand_page_size;

					aml_chip->aml_nand_command(aml_chip, NAND_CMD_TWOPLANE_READ2, 0x00, page_addr, i);
dma_retry_plane1:					
					error = aml_chip->aml_nand_dma_read(aml_chip, buf, nand_page_size, aml_chip->bch_mode);
					if (error){
						printk("aml nand read data dma plane1 failed at page %d chip %d\n", page_addr, i);
						mdelay(50);
						error = aml_chip->aml_nand_dma_read(aml_chip, buf, nand_page_size, aml_chip->bch_mode);
						if(error){
							mdelay(100);
							goto exit;
						}
					}
					aml_chip->aml_nand_get_user_byte(aml_chip, oob_buf, user_byte_num);
					stat = aml_chip->aml_nand_hwecc_correct(aml_chip, buf, nand_page_size, oob_buf);
					if (stat < 0) {
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON3
						if(aml_chip->ran_mode && (aml_chip->zero_cnt <  aml_chip->ecc_max)){
						    	memset(buf, 0xff, nand_page_size);
						    	memset(oob_buf, 0xff, user_byte_num);
						  	oob_buf += user_byte_num;
							buf += nand_page_size;
							continue;
						 }
						 
					 	if(ran_mode && aml_chip->ran_mode){
							//printk("%s dma retry here at page:%d  blk %d chip %d\n", __func__, page_addr, (page_addr >> pages_per_blk_shift), i);
							aml_chip->ran_mode = 0;
							ndelay(300);
							aml_chip->aml_nand_command(aml_chip, NAND_CMD_RNDOUT, 0, -1, i);
                            				ndelay(500);					
							goto dma_retry_plane1;			    
						 } 							 
#endif						
						memset(buf, 0xff, nand_page_size);
						memset(oob_buf, 0x22, user_byte_num);

				        	mtd->ecc_stats.failed++;  
					    	printk("aml nand read data ecc plane1 failed at page %d chip %d \n", page_addr, i);
					}
					else{
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON3
						if(aml_chip->ecc_cnt_cur > aml_chip->ecc_cnt_limit) {
							printk("%s line:%d uncorrected ecc_cnt_cur:%d, and limit:%d and at page:%d, blk:%d chip[%d]\n",
											__func__, __LINE__, aml_chip->ecc_cnt_cur, aml_chip->ecc_cnt_limit, page_addr, (page_addr >> pages_per_blk_shift), i);

							mtd->ecc_stats.corrected++;
						}
#endif				
						mtd->ecc_stats.corrected += stat;
					}
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON3
					aml_chip->ran_mode = ran_mode;
#endif					
					oob_buf += user_byte_num;
					buf += nand_page_size;

				}
				else if (aml_chip->plane_num == 1) {
//dma_retry_3:		
					error = aml_chip->aml_nand_dma_read(aml_chip, buf, nand_page_size, aml_chip->bch_mode);
					if (error){
						printk("aml nand read data dma plane failed at page %d chip %d\n", page_addr, i);
						mdelay(50);
						error = aml_chip->aml_nand_dma_read(aml_chip, buf, nand_page_size, aml_chip->bch_mode);
						if(error){
							mdelay(100);
						goto exit;
					}
					}
	
					aml_chip->aml_nand_get_user_byte(aml_chip, oob_buf, user_byte_num);
					stat = aml_chip->aml_nand_hwecc_correct(aml_chip, buf, nand_page_size, oob_buf);
					if (stat < 0) {
						//mtd->ecc_stats.failed++;
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON3
						if(aml_chip->ran_mode && (aml_chip->zero_cnt <  aml_chip->ecc_max)){
						    	memset(buf, 0xff, nand_page_size);
						    	memset(oob_buf, 0xff, user_byte_num);
							oob_buf += user_byte_num;
							buf += nand_page_size;						    
						    	continue;
						 }

						 aml_chip->ran_mode = ran_mode;	
#endif						 
#ifdef NEW_NAND_SUPPORT						 

						if(aml_chip->new_nand_info.type == SANDISK_19NM){
							page_temp = page_addr - pages_per_blk* (page_addr >> pages_per_blk_shift);
							if(((page_temp % 2 == 0) && (page_temp !=0) )||(page_temp == (pages_per_blk -1))) {
								if(readretry_failed_cnt++ < DYNAMIC_CNT_UPPER){
							aml_nand_debug("aml nand read data ecc failed at page:%d  blk %d chip %d, readretry_failed_cnt:%d\n", 
															page_addr, (page_addr >> pages_per_blk_shift), i, readretry_failed_cnt);
									aml_chip->new_nand_info.dynamic_read_info.dynamic_read_handle(mtd, page_temp, i);
							aml_chip->aml_nand_command(aml_chip, NAND_CMD_READ0, 0, page_addr, i);
							goto read_retry;
						}	
							}
							else{
								if(readretry_failed_cnt++ < DYNAMIC_CNT_LOWER){
										aml_nand_debug("aml nand read data ecc failed at page:%d  blk %d chip %d, readretry_failed_cnt:%d\n", 
																		page_addr, (page_addr >> pages_per_blk_shift), i, readretry_failed_cnt);
										aml_chip->new_nand_info.dynamic_read_info.dynamic_read_handle(mtd, page_temp, i);
										aml_chip->aml_nand_command(aml_chip, NAND_CMD_READ0, 0, page_addr, i);
										goto read_retry;
									}	
								}
						}
						else if(aml_chip->new_nand_info.type){
								if(readretry_failed_cnt++ < retry_cnt){
								aml_nand_debug("aml nand read data ecc failed at page:%d  blk %d chip %d, readretry_failed_cnt:%d\n", 
																page_addr, (page_addr >> pages_per_blk_shift), i, readretry_failed_cnt);
								aml_chip->new_nand_info.read_rety_info.read_retry_handle(mtd, i);
								aml_chip->aml_nand_command(aml_chip, NAND_CMD_READ0, 0, page_addr, i);
								goto read_retry;
								}						
						}
#endif
						memset(buf, 0xff, nand_page_size);
						memset(oob_buf, 0x22, user_byte_num);
						printk("########%s %d read ecc failed here at at page:%d, blk:%d chip[%d]\n", __func__, __LINE__, page_addr, (page_addr >> pages_per_blk_shift), i);
						mtd->ecc_stats.failed++;
					}
					else{
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6
						aml_chip->ran_mode = ran_mode;
#endif					
#ifdef NEW_NAND_SUPPORT						

						if(aml_chip->new_nand_info.type == SANDISK_19NM){
							page_temp = page_addr - pages_per_blk* (page_addr >> pages_per_blk_shift);
							if(((page_temp % 2 == 0) && (page_temp !=0) )||(page_temp == (pages_per_blk -1))) {
								if(readretry_failed_cnt > DYNAMIC_CNT_UPPER -2){
								printk("%s line:%d uncorrected ecc_cnt_cur:%d, and limit:%d and at page:%d, blk:%d chip[%d], readretry_failed_cnt:%d\n",
												__func__, __LINE__, aml_chip->ecc_cnt_cur, aml_chip->ecc_cnt_limit, page_addr, (page_addr >> pages_per_blk_shift), i, readretry_failed_cnt);

								mtd->ecc_stats.corrected++;
						}
							}
							else{
								
								if(readretry_failed_cnt > DYNAMIC_CNT_LOWER -2){
									printk("%s line:%d uncorrected ecc_cnt_cur:%d, and limit:%d and at page:%d, blk:%d chip[%d], readretry_failed_cnt:%d\n",
													__func__, __LINE__, aml_chip->ecc_cnt_cur, aml_chip->ecc_cnt_limit, page_addr, (page_addr >> pages_per_blk_shift), i, readretry_failed_cnt);

									mtd->ecc_stats.corrected++;
								}	
								}
						}
						else if(aml_chip->new_nand_info.type){
								if(readretry_failed_cnt > (aml_chip->new_nand_info.read_rety_info.retry_cnt-2)){
									printk("%s line:%d uncorrected ecc_cnt_cur:%d, and limit:%d and at page:%d, blk:%d chip[%d], readretry_failed_cnt:%d\n",
													__func__, __LINE__, aml_chip->ecc_cnt_cur, aml_chip->ecc_cnt_limit, page_addr, (page_addr >> pages_per_blk_shift), i, readretry_failed_cnt);

									mtd->ecc_stats.corrected++;
								}
				
							}
#endif					
						mtd->ecc_stats.corrected += stat;
					}
#ifdef NEW_NAND_SUPPORT
					if( readretry_failed_cnt && (aml_chip->new_nand_info.read_rety_info.read_retry_exit)) {
						if(aml_chip->new_nand_info.type == SANDISK_19NM) {
							aml_chip->new_nand_info.dynamic_read_info.dynamic_read_exit(mtd, i);
					}
						else if(aml_chip->new_nand_info.type)
							aml_chip->new_nand_info.read_rety_info.read_retry_exit(mtd, i);
					}
#endif
					oob_buf += user_byte_num;
					buf += nand_page_size;
				}
				else {
					error = -ENODEV;
					mdelay(100);
					goto exit;
				}
			}
		}
	}

exit:
	return 0;  //do not return error when failed
}

static void aml_nand_write_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip, const uint8_t *buf)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	uint8_t *oob_buf = chip->oob_poi;
	unsigned nand_page_size = (1 << chip->page_shift);
	int user_byte_num = (((nand_page_size + chip->ecc.size - 1) / chip->ecc.size) * aml_chip->user_byte_mode);
	int error = 0, i = 0, j = 0, page_addr, internal_chipnr = 1;

	if (aml_chip->ops_mode & AML_INTERLEAVING_MODE)
		internal_chipnr = aml_chip->internal_chipnr;

	memset(oob_buf + mtd->oobavail, 0xa5, user_byte_num * (mtd->writesize / nand_page_size));
	for (i=0; i<aml_chip->chip_num; i++) {

		if (aml_chip->valid_chip[i]) {

			aml_chip->aml_nand_select_chip(aml_chip, i);

			page_addr = aml_chip->page_addr;
			if (i > 0) {
				if (!aml_chip->aml_nand_wait_devready(aml_chip, i)) {
					printk ("chip: %d: busy\n", i);
					error = -EBUSY;
					goto exit;
				}
				page_addr = aml_chip->page_addr;
				aml_chip->aml_nand_command(aml_chip, NAND_CMD_SEQIN, 0, page_addr, i);
			}

			for (j=0; j<internal_chipnr; j++) {

				if (j > 0) {
					page_addr = aml_chip->page_addr;
					page_addr |= (1 << aml_chip->internal_chip_shift) * j;
					aml_chip->aml_nand_command(aml_chip, NAND_CMD_SEQIN, 0, page_addr, i);
				}

				if (aml_chip->plane_num == 2) {
	
					aml_chip->aml_nand_set_user_byte(aml_chip, oob_buf, user_byte_num);
					error = aml_chip->aml_nand_dma_write(aml_chip, (unsigned char *)buf, nand_page_size, aml_chip->bch_mode);
					if (error)
						goto exit;
					aml_chip->aml_nand_command(aml_chip, NAND_CMD_DUMMY_PROGRAM, -1, -1, i);

					oob_buf += user_byte_num;
					buf += nand_page_size;
	
					if (!aml_chip->aml_nand_wait_devready(aml_chip, i)) {
						printk ("write couldn`t found selected chip: %d ready\n", i);
						error = -EBUSY;
						goto exit;
					}

					aml_chip->aml_nand_command(aml_chip, NAND_CMD_TWOPLANE_WRITE2, 0x00, page_addr, i);
					aml_chip->aml_nand_set_user_byte(aml_chip, oob_buf, user_byte_num);
					error = aml_chip->aml_nand_dma_write(aml_chip, (unsigned char *)buf, nand_page_size, aml_chip->bch_mode);
					if (error)
						goto exit;
					if (aml_chip->cached_prog_status)
						aml_chip->aml_nand_command(aml_chip, NAND_CMD_CACHEDPROG, -1, -1, i);
					else
						aml_chip->aml_nand_command(aml_chip, NAND_CMD_PAGEPROG, -1, -1, i);

					oob_buf += user_byte_num;
					buf += nand_page_size;
				}
				else if (aml_chip->plane_num == 1) {

					aml_chip->aml_nand_set_user_byte(aml_chip, oob_buf, user_byte_num);
					error = aml_chip->aml_nand_dma_write(aml_chip, (unsigned char *)buf, nand_page_size, aml_chip->bch_mode);
					if (error)
						goto exit;
					if (chip->cmdfunc == aml_nand_command) {
						if (aml_chip->cached_prog_status)
							aml_chip->aml_nand_command(aml_chip, NAND_CMD_CACHEDPROG, -1, -1, i);
						else
							aml_chip->aml_nand_command(aml_chip, NAND_CMD_PAGEPROG, -1, -1, i);
					}

					oob_buf += user_byte_num;
					buf += nand_page_size;
				}
				else {
					error = -ENODEV;
					goto exit;
				}
			}
		}
	}

exit:
	return;
}

static int aml_nand_write_page(struct mtd_info *mtd, struct nand_chip *chip, const uint8_t *buf, int page, int cached, int raw)
{
	int status;
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);

	chip->cmdfunc(mtd, NAND_CMD_SEQIN, 0x00, page);

	/*if ((cached) && (chip->options & NAND_CACHEPRG))
		aml_chip->cached_prog_status = 1;
	else
		aml_chip->cached_prog_status = 0;*/
	if (unlikely(raw))
		chip->ecc.write_page_raw(mtd, chip, buf);
	else
		chip->ecc.write_page(mtd, chip, buf);

	if (!cached || !(chip->options & NAND_CACHEPRG)) {

		//chip->cmdfunc(mtd, NAND_CMD_PAGEPROG, -1, -1);
		status = chip->waitfunc(mtd, chip);
		/*
		 * See if operation failed and additional status checks are
		 * available
		 */
		if ((status & NAND_STATUS_FAIL) && (chip->errstat))
			status = chip->errstat(mtd, chip, FL_WRITING, status, page);

		if (status & NAND_STATUS_FAIL) {
			printk("aml nand write failed at %d \n", page);
			return -EIO;
		}
	} else {
		//chip->cmdfunc(mtd, NAND_CMD_CACHEDPROG, -1, -1);
		status = chip->waitfunc(mtd, chip);
	}

	aml_chip->cached_prog_status = 0;

#ifdef CONFIG_MTD_NAND_VERIFY_WRITE
	chip->cmdfunc(mtd, NAND_CMD_READ0, 0x00, page);
	status = chip->ecc.read_page(mtd, chip, chip->buffers->databuf, page);
	if (status == -EUCLEAN)
		status = 0;
	chip->pagebuf = page;

	if (memcmp(buf, chip->buffers->databuf, mtd->writesize)) {
		printk("nand verify failed at %d \n", page);
		return -EFAULT;
	}
#endif

	return 0;
}

static int aml_nand_read_oob(struct mtd_info *mtd, struct nand_chip *chip, int page, int readlen)
{
	int32_t error = 0, i, stat = 0, j = 0, page_addr, user_byte_num, internal_chipnr = 1; 
	unsigned dma_once_size;
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	unsigned char *nand_buffer = aml_chip->aml_nand_data_buf;
	unsigned char *oob_buffer = chip->oob_poi;
	unsigned pages_per_blk_shift = (chip->phys_erase_shift - chip->page_shift);
	unsigned nand_page_size = (1 << chip->page_shift);
	//unsigned nand_read_size = ((readlen / (aml_chip->user_byte_mode * aml_chip->plane_num)) * chip->ecc.size);
	unsigned nand_read_size = ((readlen / aml_chip->user_byte_mode) * chip->ecc.size);
	unsigned read_chip_num = (((nand_read_size + (aml_chip->plane_num * nand_page_size) - 1) / (aml_chip->plane_num * nand_page_size)));
	int readretry_failed_cnt = 0, pages_per_blk =  (1 << (chip->phys_erase_shift - chip->page_shift));
	int page_temp;
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON3
	int ran_mode = aml_chip->ran_mode;
#endif	
	int retry_cnt =aml_chip->new_nand_info.read_rety_info.retry_cnt;
	if ((aml_chip->new_nand_info.type == HYNIX_20NM_8GB) || (aml_chip->new_nand_info.type == HYNIX_20NM_4GB))
		retry_cnt = aml_chip->new_nand_info.read_rety_info.retry_cnt *aml_chip->new_nand_info.read_rety_info.retry_cnt;
	
	if (nand_read_size >= nand_page_size)
		user_byte_num = (((nand_page_size + chip->ecc.size - 1) / chip->ecc.size) * aml_chip->user_byte_mode);
	else
		user_byte_num = (((nand_read_size + chip->ecc.size - 1) / chip->ecc.size) * aml_chip->user_byte_mode);
	page_addr = page;
	if (aml_chip->ops_mode & AML_INTERLEAVING_MODE) {
		internal_chipnr = aml_chip->internal_chipnr;
		if (read_chip_num < internal_chipnr) {
			internal_chipnr = (read_chip_num + aml_chip->internal_chipnr - 1) / aml_chip->internal_chipnr;
			read_chip_num = 1;
		}
		else {
			read_chip_num = (read_chip_num + aml_chip->internal_chipnr - 1) / aml_chip->internal_chipnr;
		}
	}
	

		if (chip->cmdfunc == aml_nand_command)
			chip->cmdfunc(mtd, NAND_CMD_READOOB, 0, page_addr);
		else {
			aml_chip->aml_nand_select_chip(aml_chip, 0);
			chip->cmdfunc(mtd, NAND_CMD_READ0, 0, page_addr);
		}

	for (i=0; i<read_chip_num; i++) {
		if (aml_chip->valid_chip[i]) {
			readretry_failed_cnt = 0;


			page_addr = aml_chip->page_addr;
			if (i > 0) {
				aml_chip->aml_nand_select_chip(aml_chip, i);
				aml_chip->aml_nand_command(aml_chip, NAND_CMD_READ0, 0, page_addr, i);
			}
read_retry:

			page_addr = aml_chip->page_addr;
			for (j=0; j<internal_chipnr; j++) {

				if (j > 0) {
					page_addr = aml_chip->page_addr;
					page_addr |= (1 << aml_chip->internal_chip_shift) * j;
					aml_chip->aml_nand_select_chip(aml_chip, i);
					aml_chip->aml_nand_command(aml_chip, NAND_CMD_READ0, 0, page_addr, i);
				}

				if (!aml_chip->aml_nand_wait_devready(aml_chip, i)) {
					mdelay(50);
					if (!aml_chip->aml_nand_wait_devready(aml_chip, i)) {
						printk ("read oob couldn`t found selected chip: %d ready\n", i);
					error = -EBUSY;
						mdelay(100);
					goto exit;
				}
				}

				if (aml_chip->ops_mode & AML_CHIP_NONE_RB) 
					chip->cmd_ctrl(mtd, NAND_CMD_READ0 & 0xff, NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
					
				if (aml_chip->plane_num == 2) {

					dma_once_size = min(nand_read_size, nand_page_size);
					aml_chip->aml_nand_command(aml_chip, NAND_CMD_TWOPLANE_READ1, 0x00, page_addr, i);
dma_retry_plane0:	
					error = aml_chip->aml_nand_dma_read(aml_chip, nand_buffer, dma_once_size, aml_chip->bch_mode);
					if (error)
					{
						printk("read oob dma failed at page %d\n", page_addr);
						mdelay(50);
						error = aml_chip->aml_nand_dma_read(aml_chip, nand_buffer, dma_once_size, aml_chip->bch_mode);
						if(error)
						{
							printk("read oob dma failed again at page %d\n", page_addr);
							mdelay(100);
							return 0;//error;
						}
					}

					aml_chip->aml_nand_get_user_byte(aml_chip, oob_buffer, user_byte_num);
					stat = aml_chip->aml_nand_hwecc_correct(aml_chip, nand_buffer, dma_once_size, oob_buffer);
					if (stat < 0) {
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON3
						if(aml_chip->ran_mode && (aml_chip->zero_cnt <  aml_chip->ecc_max)){
						    memset(oob_buffer, 0xff, user_byte_num);
						    goto plane0_ff;
						 }
						 
					 	if(ran_mode && aml_chip->ran_mode){
							//printk("%s dma retry here at page:%d  blk %d chip %d\n", __func__, page_addr, (page_addr >> pages_per_blk_shift), i);
							aml_chip->ran_mode = 0;
							ndelay(300);
							aml_chip->aml_nand_command(aml_chip, NAND_CMD_RNDOUT, 0, -1, i);
                            				ndelay(500);					
							goto dma_retry_plane0;			    
						 } 							 
#endif						 
						memset(oob_buffer, 0x22, user_byte_num);

				        	mtd->ecc_stats.failed++;  
					    	printk("aml nand read oob plane0 failed at page %d chip %d \n", page_addr, i);

					}
					else{
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON3					
						if(aml_chip->ecc_cnt_cur > aml_chip->ecc_cnt_limit){
							printk("%s line:%d uncorrected ecc_cnt_cur:%d, and limit:%d and at page:%d, blk:%d chip[%d]\n",
											__func__, __LINE__, aml_chip->ecc_cnt_cur, aml_chip->ecc_cnt_limit, page_addr, (page_addr >> pages_per_blk_shift), i);

							mtd->ecc_stats.corrected++;
						}
#endif						
						mtd->ecc_stats.corrected += stat;
					}
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON3
plane0_ff:
					aml_chip->ran_mode = ran_mode;	
#endif				
					oob_buffer += user_byte_num;
					nand_read_size -= dma_once_size;

					if (nand_read_size > 0) {

						dma_once_size = min(nand_read_size, nand_page_size);
						aml_chip->aml_nand_command(aml_chip, NAND_CMD_TWOPLANE_READ2, 0x00, page_addr, i);
dma_retry_plane1:	
						error = aml_chip->aml_nand_dma_read(aml_chip, nand_buffer, dma_once_size, aml_chip->bch_mode);
						if (error){
							printk("read oob dma failed at page %d\n", page_addr);
							mdelay(50);
							error = aml_chip->aml_nand_dma_read(aml_chip, nand_buffer, dma_once_size, aml_chip->bch_mode);
							if(error){
								printk("read oob dma failed again at page %d\n", page_addr);
								mdelay(100);
								return 0;//error;
							}
						}

						aml_chip->aml_nand_get_user_byte(aml_chip, oob_buffer, user_byte_num);
						stat = aml_chip->aml_nand_hwecc_correct(aml_chip, nand_buffer, dma_once_size, oob_buffer);
						if (stat < 0) {
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON3
							if(aml_chip->ran_mode && (aml_chip->zero_cnt <  aml_chip->ecc_max)){
								memset(oob_buffer, 0xff, user_byte_num);
								oob_buffer += user_byte_num;
								nand_read_size -= dma_once_size;
								continue;
							}
								
							if(ran_mode && aml_chip->ran_mode){
								//printk("%s dma retry here at page:%d  blk %d chip %d\n", __func__, page_addr, (page_addr >> pages_per_blk_shift), i);
								aml_chip->ran_mode = 0;
								ndelay(300);
								aml_chip->aml_nand_command(aml_chip, NAND_CMD_RNDOUT, 0, -1, i);
								ndelay(500); 				   
								goto dma_retry_plane1;			   
							}			
#endif							 
							memset(oob_buffer, 0x22, user_byte_num);																
							mtd->ecc_stats.failed++;  
							printk("aml nand read oob plane1 failed at page %d chip %d \n", page_addr, i);
						}
						else{
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON3
							if(aml_chip->ecc_cnt_cur > aml_chip->ecc_cnt_limit){
								printk("%s line:%d uncorrected ecc_cnt_cur:%d, and limit:%d and at page:%d, blk:%d chip[%d]\n",
												__func__, __LINE__, aml_chip->ecc_cnt_cur, aml_chip->ecc_cnt_limit, page_addr, (page_addr >> pages_per_blk_shift), i);
						
								mtd->ecc_stats.corrected++;
							}
#endif							
							mtd->ecc_stats.corrected += stat;
						}
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON3
						aml_chip->ran_mode = ran_mode;
#endif						
						oob_buffer += user_byte_num;
						nand_read_size -= dma_once_size;
					}
				}
				else if (aml_chip->plane_num == 1) {
					dma_once_size = min(nand_read_size, nand_page_size);
//dma_retry:					
					error = aml_chip->aml_nand_dma_read(aml_chip, nand_buffer, dma_once_size, aml_chip->bch_mode);
					if (error){
						printk("read oob dma failed at page %d\n", page_addr);
						mdelay(50);
						error = aml_chip->aml_nand_dma_read(aml_chip, nand_buffer, dma_once_size, aml_chip->bch_mode);
						if(error){
							printk("read oob dma failed again at page %d\n", page_addr);
							mdelay(100);
							return 0;//error;
						}
					}

					aml_chip->aml_nand_get_user_byte(aml_chip, oob_buffer, user_byte_num);
					stat = aml_chip->aml_nand_hwecc_correct(aml_chip, nand_buffer, dma_once_size, oob_buffer);
					if (stat < 0) {
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON3
						if(aml_chip->ran_mode && (aml_chip->zero_cnt <  aml_chip->ecc_max)){
						    	memset(oob_buffer, 0xff, user_byte_num);
							//oob_buffer += user_byte_num;
							//nand_read_size -= dma_once_size;						    	
						  //	continue;
						  goto all_ff;
						 }

						 aml_chip->ran_mode = ran_mode;							 
#endif						 
#ifdef NEW_NAND_SUPPORT						 

						if(aml_chip->new_nand_info.type == SANDISK_19NM){
							page_temp = page_addr - pages_per_blk* (page_addr >> pages_per_blk_shift);
							if(((page_temp % 2 == 0) && (page_temp !=0) )||(page_temp == (pages_per_blk -1))) {
								if(readretry_failed_cnt++ < DYNAMIC_CNT_UPPER){
							aml_nand_debug("aml nand read data ecc failed at page:%d  blk %d chip %d, readretry_failed_cnt:%d\n", 
															page_addr, (page_addr >> pages_per_blk_shift), i, readretry_failed_cnt);
									aml_chip->new_nand_info.dynamic_read_info.dynamic_read_handle(mtd, page_temp, i);
							aml_chip->aml_nand_command(aml_chip, NAND_CMD_READ0, 0, page_addr, i);
							goto read_retry;
						}	
							}
							else{
								if(readretry_failed_cnt++ < DYNAMIC_CNT_LOWER){
									aml_nand_debug("aml nand read data ecc failed at page:%d  blk %d chip %d, readretry_failed_cnt:%d\n", 
																	page_addr, (page_addr >> pages_per_blk_shift), i, readretry_failed_cnt);
									aml_chip->new_nand_info.dynamic_read_info.dynamic_read_handle(mtd, page_temp, i);
									aml_chip->aml_nand_command(aml_chip, NAND_CMD_READ0, 0, page_addr, i);
									goto read_retry;
									}	
								}
						}
						else if(aml_chip->new_nand_info.type){
								if(readretry_failed_cnt++ < retry_cnt){
									aml_nand_debug("aml nand read data ecc failed at page:%d  blk %d chip %d, readretry_failed_cnt:%d\n", 
																	page_addr, (page_addr >> pages_per_blk_shift), i, readretry_failed_cnt);
									aml_chip->new_nand_info.read_rety_info.read_retry_handle(mtd, i);
									aml_chip->aml_nand_command(aml_chip, NAND_CMD_READ0, 0, page_addr, i);
									goto read_retry;
								}						
						}
#endif
						printk("########%s %d read oob failed here at at page:%d, blk:%d chip[%d]\n", __func__, __LINE__, page_addr, (page_addr >> pages_per_blk_shift), i);
						
						memset(oob_buffer, 0x22, user_byte_num);
						mtd->ecc_stats.failed++;
					}
					else{
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON3
						aml_chip->ran_mode = ran_mode;
#endif						
#ifdef NEW_NAND_SUPPORT						

						if(aml_chip->new_nand_info.type == SANDISK_19NM){
							page_temp = page_addr - pages_per_blk* (page_addr >> pages_per_blk_shift);
							if(((page_temp % 2 == 0) && (page_temp !=0) )||(page_temp == (pages_per_blk -1))) {
								if(readretry_failed_cnt > DYNAMIC_CNT_UPPER -2){
									printk("%s line:%d uncorrected ecc_cnt_cur:%d, and limit:%d and at page:%d, blk:%d chip[%d], readretry_failed_cnt:%d\n",
													__func__, __LINE__, aml_chip->ecc_cnt_cur, aml_chip->ecc_cnt_limit, page_addr, (page_addr >> pages_per_blk_shift), i, readretry_failed_cnt);

									mtd->ecc_stats.corrected++;
						}
							}
							else{
									if(readretry_failed_cnt > DYNAMIC_CNT_LOWER -2){
										printk("%s line:%d uncorrected ecc_cnt_cur:%d, and limit:%d and at page:%d, blk:%d chip[%d], readretry_failed_cnt:%d\n",
														__func__, __LINE__, aml_chip->ecc_cnt_cur, aml_chip->ecc_cnt_limit, page_addr, (page_addr >> pages_per_blk_shift), i, readretry_failed_cnt);

										mtd->ecc_stats.corrected++;
									}	
								}
						}
						else if(aml_chip->new_nand_info.type){
								if(readretry_failed_cnt > (aml_chip->new_nand_info.read_rety_info.retry_cnt-2)){
									printk("%s line:%d uncorrected ecc_cnt_cur:%d, and limit:%d and at page:%d, blk:%d chip[%d], readretry_failed_cnt:%d\n",
													__func__, __LINE__, aml_chip->ecc_cnt_cur, aml_chip->ecc_cnt_limit, page_addr, (page_addr >> pages_per_blk_shift), i, readretry_failed_cnt);

									mtd->ecc_stats.corrected++;
								}
				
							}
#endif					
						mtd->ecc_stats.corrected += stat;
					}
#ifdef NEW_NAND_SUPPORT
					if(aml_chip->new_nand_info.type &&(aml_chip->new_nand_info.type < SANDISK_19NM) && (readretry_failed_cnt) && (aml_chip->new_nand_info.read_rety_info.read_retry_exit)){
						aml_chip->new_nand_info.read_rety_info.read_retry_exit(mtd, i);
					}
					if( (aml_chip->new_nand_info.type == SANDISK_19NM) && (readretry_failed_cnt) && (aml_chip->new_nand_info.dynamic_read_info.dynamic_read_exit)){
							aml_chip->new_nand_info.dynamic_read_info.dynamic_read_exit(mtd, i);
						}
#endif

all_ff:
					oob_buffer += user_byte_num;
					nand_read_size -= dma_once_size;
				}
				else {
					error = -ENODEV;
					mdelay(100);
					goto exit;
				}
			}
		}
	}

exit:
	return readlen;
}

static int aml_nand_write_oob(struct mtd_info *mtd, struct nand_chip *chip, int page)
{
	printk("our host controller`s structure couldn`t support oob write\n");
	BUG();
	return 0;
}

static int aml_nand_block_bad(struct mtd_info *mtd, loff_t ofs, int getchip)
{ 
	struct nand_chip * chip = mtd->priv;	
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	struct aml_nand_platform *plat = aml_chip->platform;
	//struct mtd_oob_ops aml_oob_ops;
	struct mtd_oob_ops *aml_oob_ops;
	int32_t ret = 0, read_cnt, page, mtd_erase_shift, blk_addr, pages_per_blk;
	int err;
	loff_t addr;

	if ((!strncmp((char*)plat->name, NAND_BOOT_NAME, strlen((const char*)NAND_BOOT_NAME)))/* && ((chip->ecc.read_page == aml_nand_read_page_hwecc) || (!getchip))*/)
		return 0;

	mtd_erase_shift = fls(mtd->erasesize) - 1;
	blk_addr = (int)(ofs >> mtd_erase_shift);
	if (aml_chip->block_status != NULL) {
		if (aml_chip->block_status[blk_addr] == NAND_BLOCK_BAD) {
			printk(" NAND bbt detect Bad block at %llx \n", (uint64_t)ofs);
			return EFAULT;
		}
		else if (aml_chip->block_status[blk_addr] == NAND_FACTORY_BAD) {
			printk(" NAND bbt detect factory Bad block at %llx \n", (uint64_t)ofs);
			return FACTORY_BAD_BLOCK_ERROR;  //159
		}
		else if (aml_chip->block_status[blk_addr] == NAND_BLOCK_GOOD) {
			return 0;
		}
	}
	
	aml_oob_ops = kzalloc(sizeof(struct mtd_oob_ops), GFP_KERNEL);
	if (aml_oob_ops == NULL)
		return -ENOMEM; 
	chip->pagebuf = -1;
	pages_per_blk = (1 << (chip->phys_erase_shift - chip->page_shift));
	if (getchip) {

		aml_oob_ops->mode = MTD_OOB_AUTO;
		aml_oob_ops->len = mtd->writesize;
		aml_oob_ops->ooblen = mtd->oobavail;
		aml_oob_ops->ooboffs = chip->ecc.layout->oobfree[0].offset;
		aml_oob_ops->datbuf = chip->buffers->databuf;
		aml_oob_ops->oobbuf = chip->oob_poi;

		for (read_cnt=0; read_cnt<2; read_cnt++) {

			addr = ofs + (pages_per_blk - 1) * read_cnt * mtd->writesize;
			ret = mtd->read_oob(mtd, addr, aml_oob_ops);
			if (ret == -EUCLEAN)
				ret = 0;

			if (ret < 0) {
				printk(" NAND detect Bad block at %llx \n", (uint64_t)addr);
				err = EFAULT;
				goto exit;
			}
			if (aml_oob_ops->oobbuf[chip->badblockpos] == 0xFF)
				continue;
			if (aml_oob_ops->oobbuf[chip->badblockpos] == 0) {
				memset(aml_chip->aml_nand_data_buf, 0, aml_oob_ops->ooblen);
				if (!memcmp(aml_chip->aml_nand_data_buf, aml_oob_ops->oobbuf, aml_oob_ops->ooblen)) {
					printk(" NAND detect Bad block at %llx \n", (uint64_t)addr);
					err = EFAULT;
					goto exit;
				}
			}
		}
	}
	else {

		for (read_cnt=0; read_cnt<2; read_cnt++) {

			addr = ofs + (pages_per_blk - 1) * read_cnt * mtd->writesize;
			page = (int)(addr >> chip->page_shift);
			ret = chip->ecc.read_oob(mtd, chip, page, mtd->oobavail);
			if (ret == -EUCLEAN)
				ret = 0;
			if (ret < 0){
				err = EFAULT;
				goto exit;
			}
			if (chip->oob_poi[chip->badblockpos] == 0xFF){
				if(aml_oob_ops){
					kfree(aml_oob_ops);
					aml_oob_ops = NULL;
				}
				return 0;
			}
			if (chip->oob_poi[chip->badblockpos] == 0) {
				memset(aml_chip->aml_nand_data_buf, 0, (mtd->writesize + mtd->oobsize));
				if (!memcmp(aml_chip->aml_nand_data_buf + mtd->writesize, chip->oob_poi, mtd->oobavail)) {
					printk(" NAND detect Bad block at %llx \n", (uint64_t)addr);
					err = EFAULT;
					goto exit;
				}	
			}
		}
	}
	
	if(aml_oob_ops){
		kfree(aml_oob_ops);
		aml_oob_ops = NULL;
	}
	return 0;
exit: 
	if(aml_oob_ops){
		kfree(aml_oob_ops);
		aml_oob_ops = NULL;
	}
	return err;
}

static int aml_nand_block_markbad(struct mtd_info *mtd, loff_t ofs)
{ 
	struct nand_chip * chip = mtd->priv;	
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	//struct mtd_oob_ops aml_oob_ops;
	struct mtd_oob_ops  *aml_oob_ops;
	int blk_addr, mtd_erase_shift, j, ret;

	aml_oob_ops = kzalloc(sizeof(struct mtd_oob_ops), GFP_KERNEL);
	if (aml_oob_ops == NULL)
		return -ENOMEM; 
		
	mtd_erase_shift = fls(mtd->erasesize) - 1;
	blk_addr = (int)(ofs >> mtd_erase_shift);
	if (aml_chip->block_status != NULL) {
		if ((aml_chip->block_status[blk_addr] == NAND_BLOCK_BAD)||(aml_chip->block_status[blk_addr] == NAND_FACTORY_BAD)) {
			if(aml_oob_ops){
				kfree(aml_oob_ops);
				aml_oob_ops = NULL;
			}
			return 0;
		}
		else if (aml_chip->block_status[blk_addr] == NAND_BLOCK_GOOD) {
			aml_chip->block_status[blk_addr] = NAND_BLOCK_BAD;
			for (j=0; j<MAX_BAD_BLK_NUM; j++) {
				if (aml_chip->aml_nandenv_info->nand_bbt_info.nand_bbt[j] == 0) {
					aml_chip->aml_nandenv_info->nand_bbt_info.nand_bbt[j] = blk_addr;
					if (aml_nand_update_env(mtd))
						printk("update env bbt failed %d \n", blk_addr);
					break;
				}
			}
		}
	}

	aml_oob_ops->mode = MTD_OOB_AUTO;
	aml_oob_ops->len = mtd->writesize;
	aml_oob_ops->ooblen = mtd->oobavail;
	aml_oob_ops->ooboffs = chip->ecc.layout->oobfree[0].offset;
	aml_oob_ops->datbuf = chip->buffers->databuf;
	aml_oob_ops->oobbuf = chip->oob_poi;
	chip->pagebuf = -1;

	memset((unsigned char *)aml_oob_ops->datbuf, 0x0, mtd->writesize);
	memset((unsigned char *)aml_oob_ops->oobbuf, 0x0, aml_oob_ops->ooblen);

	ret = mtd->write_oob(mtd, ofs, aml_oob_ops);

	if(aml_oob_ops){
		kfree(aml_oob_ops);
		aml_oob_ops = NULL;
	}
	return ret;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void aml_platform_nand_suspend(struct early_suspend *nand_early_suspend)
{
	struct aml_nand_chip *aml_chip = (struct aml_nand_chip *)nand_early_suspend->param;
	struct nand_chip *chip = &aml_chip->chip;
	spinlock_t *lock = &chip->controller->lock;

	if (nand_erarly_suspend_flag == 1)
		return;
	spin_lock(lock);
	nand_erarly_suspend_flag = 1;
	spin_unlock(lock);

	printk("aml_m1_nand_early suspend entered\n");
	return;
}

static void aml_platform_nand_resume(struct early_suspend *nand_early_suspend)
{
	struct aml_nand_chip *aml_chip = (struct aml_nand_chip *)nand_early_suspend->param;

    if (((READ_CBUS_REG(HHI_MPEG_CLK_CNTL)&(1<<8))) && (nand_erarly_suspend_flag == 2)) {
	    aml_chip->aml_nand_adjust_timing(aml_chip);
	    nand_erarly_suspend_flag = 0;
	}

	printk("aml_m1_nand_early resume entered\n");
	return;
}
#endif

static int aml_nand_suspend(struct mtd_info *mtd)
{
	printk("aml_nand suspend entered\n");
	return 0;
}

static void aml_nand_resume(struct mtd_info *mtd)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);

    if (((READ_CBUS_REG(HHI_MPEG_CLK_CNTL)&(1<<8))) && (nand_erarly_suspend_flag == 2)) {
	    aml_chip->aml_nand_adjust_timing(aml_chip);
	    nand_erarly_suspend_flag = 0;
	}

	printk("aml_m1_nand resume entered\n");
	return;
}

static struct aml_nand_flash_dev *aml_nand_get_flash_type(struct mtd_info *mtd,
						  struct nand_chip *chip,
						  int busw, int *maf_id)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	struct aml_nand_platform *plat = aml_chip->platform;
	struct aml_nand_flash_dev *type = NULL;
	int i, maf_idx;
	u8 dev_id[MAX_ID_LEN];
#ifdef NEW_NAND_SUPPORT
	u8 dev_id_hynix_26nm_8g[MAX_ID_LEN] = {NAND_MFR_HYNIX, 0xde, 0x94, 0xd2, 0x04, 0x43};		
	u8 dev_id_hynix_26nm_4g[MAX_ID_LEN] = {NAND_MFR_HYNIX, 0xd7, 0x94, 0xda, 0x74, 0xc3};	
	u8 dev_id_hynix_20nm_4g[MAX_ID_LEN] = {NAND_MFR_HYNIX, 0xd7, 0x94, 0x91, 0x60, 0x44};			
	u8 dev_id_hynix_20nm_8g[MAX_ID_LEN] = {NAND_MFR_HYNIX, 0xde, 0x94, 0xda, 0x74, 0xc4};	
	u8 dev_id_hynix_20nm_lga_8g[MAX_ID_LEN] = {NAND_MFR_HYNIX, 0xde, 0x94, 0xeb, 0x74, 0x44};	
	u8 dev_id_toshiba_24nm_4g[MAX_ID_LEN] = {NAND_MFR_TOSHIBA, 0xD7, 0x94, 0x32, 0x76, 0x56};	
	u8 dev_id_toshiba_24nm_8g[MAX_ID_LEN] = {NAND_MFR_TOSHIBA, 0xDE, 0x94, 0x82, 0x76, 0x56};
	u8 dev_id_toshiba_19nm_8g[MAX_ID_LEN] = {NAND_MFR_TOSHIBA, 0xDE, 0x84, 0x93, 0x72, 0x57};
	u8 dev_id_toshiba_19nm_4g[MAX_ID_LEN] = {NAND_MFR_TOSHIBA, 0xD7, 0x84, 0x93, 0x72, 0x57};
	u8 dev_id_samsung_2xnm_8g[MAX_ID_LEN] = {NAND_MFR_SAMSUNG, 0xDE, 0xD5, 0x7E, 0x68, 0x44};	
	u8 dev_id_samsung_2xnm_4g[MAX_ID_LEN] = {NAND_MFR_SAMSUNG, 0xD7, 0x94, 0x7e, 0x64, 0x44};		
	u8 dev_id_sandisk_19nm_8g[MAX_ID_LEN] = {NAND_MFR_SANDISK, 0xDE, 0x94, 0x93, 0x76, 0x57};	
	u8 dev_id_sandisk_19nm_4g[MAX_ID_LEN] =  {NAND_MFR_SANDISK, 0xD7, 0x84, 0x93, 0x72, 0x57};
	u8 dev_id_sandisk_24nm_8g[MAX_ID_LEN] =  {NAND_MFR_SANDISK, 0xDE, 0xA4, 0x82, 0x76, 0x56};	
	u8 dev_id_micron_20nm_8g[MAX_ID_LEN] = {NAND_MFR_MICRON, 0x64, 0x44, 0x4B, 0xA9};
	u8 dev_id_micron_20nm_4g[MAX_ID_LEN] = {NAND_MFR_MICRON, 0x44, 0x44, 0x4B, 0xA9};	
#endif
	//int tmp_id, tmp_manf;

	/* Send the command for reading device ID */
	chip->cmdfunc(mtd, NAND_CMD_READID, 0x00, -1);

	/* Read manufacturer and device IDs */
	for (i=0; i<MAX_ID_LEN; i++) {
		dev_id[i] = chip->read_byte(mtd);
	}
	*maf_id = dev_id[0];
	printk("NAND device id: %x %x %x %x %x %x \n", dev_id[0], dev_id[1], dev_id[2], dev_id[3], dev_id[4], dev_id[5]);

	/* Lookup the flash id */
	for (i = 0; aml_nand_flash_ids[i].name != NULL; i++) {
		if(!strncmp((char*)aml_nand_flash_ids[i].id, (char*)dev_id, strlen((const char*)aml_nand_flash_ids[i].id))){
			type = &aml_nand_flash_ids[i];
			break;
		}
	}

	if (!type) {
		if (plat->nand_flash_dev) {
			if(!strncmp((char*)plat->nand_flash_dev->id, (char*)dev_id, strlen((const char*)plat->nand_flash_dev->id))){
				type = plat->nand_flash_dev;
			}
		}

		if (!type)
			return ERR_PTR(-ENODEV);
	}
#ifdef NEW_NAND_SUPPORT
	memset(&aml_chip->new_nand_info, 0, sizeof(struct new_tech_nand_t));
	if(!strncmp((char*)type->id, (char*)dev_id_hynix_26nm_8g, strlen((const char*)aml_nand_flash_ids[i].id))){
		aml_chip->new_nand_info.type = HYNIX_26NM_8GB;
		aml_chip->ran_mode = 1;
		printk("aml_chip->hynix_new_nand_type =: %d \n", aml_chip->new_nand_info.type);

		//read retry
		aml_chip->new_nand_info.read_rety_info.reg_cnt = 4;
		aml_chip->new_nand_info.read_rety_info.retry_cnt = 6;

		aml_chip->new_nand_info.read_rety_info.reg_addr[0] = 0xAC;
		aml_chip->new_nand_info.read_rety_info.reg_addr[1] = 0xAD;
		aml_chip->new_nand_info.read_rety_info.reg_addr[2] = 0xAE;
		aml_chip->new_nand_info.read_rety_info.reg_addr[3] = 0xAF;

		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][0][0] = 0;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][0][1] = 0x06;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][0][2] = 0x0A;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][0][3] = 0x06;

		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][1][0] = READ_RETRY_ZERO;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][1][1] = -0x03;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][1][2] = -0x07;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][1][3] = -0x08;		

		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][2][0] = READ_RETRY_ZERO;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][2][1] = -0x06;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][2][2] = -0x0D;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][2][3] = -0x0F;	

		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][3][0] = READ_RETRY_ZERO;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][3][1] = -0x0B;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][3][2] = -0x14;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][3][3] = -0x17;	

		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][4][0] = READ_RETRY_ZERO;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][4][1] = READ_RETRY_ZERO;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][4][2] = -0x1A;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][4][3] = -0x1E;	

		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][5][0] = READ_RETRY_ZERO;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][5][1] = READ_RETRY_ZERO;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][5][2] = -0x20;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][5][3] = -0x25;	

		aml_chip->new_nand_info.slc_program_info.reg_cnt = 5;

		aml_chip->new_nand_info.slc_program_info.reg_addr[0] = 0xA4;   //not same
		aml_chip->new_nand_info.slc_program_info.reg_addr[1] = 0xA5;
		aml_chip->new_nand_info.slc_program_info.reg_addr[2] = 0xB0;
		aml_chip->new_nand_info.slc_program_info.reg_addr[3] = 0xB1;
		aml_chip->new_nand_info.slc_program_info.reg_addr[4] = 0xC9;

		aml_chip->new_nand_info.slc_program_info.reg_offset_value[0] = 0x25;  //not same
		aml_chip->new_nand_info.slc_program_info.reg_offset_value[1] = 0x25;
		aml_chip->new_nand_info.slc_program_info.reg_offset_value[2] = 0x25;
		aml_chip->new_nand_info.slc_program_info.reg_offset_value[3] = 0x25;
		aml_chip->new_nand_info.slc_program_info.reg_offset_value[4] = 0x01;	

		aml_chip->new_nand_info.read_rety_info.get_default_value = aml_nand_get_read_default_value_hynix;
		aml_chip->new_nand_info.read_rety_info.read_retry_handle = aml_nand_read_retry_handle_hynix;
		aml_chip->new_nand_info.read_rety_info.set_default_value= aml_nand_set_readretry_default_value_hynix;
		
		aml_chip->new_nand_info.slc_program_info.enter_enslc_mode = aml_nand_enter_enslc_mode_hynix;
		aml_chip->new_nand_info.slc_program_info.exit_enslc_mode = aml_nand_exit_enslc_mode_hynix;
		aml_chip->new_nand_info.slc_program_info.get_default_value = aml_nand_get_slc_default_value_hynix;

	
	}
	else  if(!strncmp((char*)type->id, (char*)dev_id_hynix_26nm_4g, strlen((const char*)aml_nand_flash_ids[i].id))){
		aml_chip->new_nand_info.type = HYNIX_26NM_4GB;
		aml_chip->ran_mode = 1;
		printk("aml_chip->hynix_new_nand_type =: %d \n", aml_chip->new_nand_info.type);

		//read retry
		aml_chip->new_nand_info.read_rety_info.reg_cnt = 4;
		aml_chip->new_nand_info.read_rety_info.retry_cnt = 6;

		aml_chip->new_nand_info.read_rety_info.reg_addr[0] = 0xA7;	//not same
		aml_chip->new_nand_info.read_rety_info.reg_addr[1] = 0xAD;
		aml_chip->new_nand_info.read_rety_info.reg_addr[2] = 0xAE;
		aml_chip->new_nand_info.read_rety_info.reg_addr[3] = 0xAF;

		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][0][0] = 0;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][0][1] = 0x06;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][0][2] = 0x0A;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][0][3] = 0x06;

		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][1][0] = READ_RETRY_ZERO;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][1][1] = -0x03;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][1][2] = -0x07;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][1][3] = -0x08;		

		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][2][0] = READ_RETRY_ZERO;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][2][1] = -0x06;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][2][2] = -0x0D;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][2][3] = -0x0F;	

		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][3][0] = READ_RETRY_ZERO;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][3][1] = -0x09;   //not same
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][3][2] = -0x14;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][3][3] = -0x17;	

		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][4][0] = READ_RETRY_ZERO;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][4][1] = READ_RETRY_ZERO;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][4][2] = -0x1A;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][4][3] = -0x1E;	

		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][5][0] = READ_RETRY_ZERO;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][5][1] = READ_RETRY_ZERO;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][5][2] = -0x20;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][5][3] = -0x25;	

		aml_chip->new_nand_info.slc_program_info.reg_cnt = 5;

		aml_chip->new_nand_info.slc_program_info.reg_addr[0] = 0xA0;		//not same
		aml_chip->new_nand_info.slc_program_info.reg_addr[1] = 0xA1;
		aml_chip->new_nand_info.slc_program_info.reg_addr[2] = 0xB0;
		aml_chip->new_nand_info.slc_program_info.reg_addr[3] = 0xB1;
		aml_chip->new_nand_info.slc_program_info.reg_addr[4] = 0xC9;

		aml_chip->new_nand_info.slc_program_info.reg_offset_value[0] = 0x26;		//not same
		aml_chip->new_nand_info.slc_program_info.reg_offset_value[1] = 0x26;
		aml_chip->new_nand_info.slc_program_info.reg_offset_value[2] = 0x26;
		aml_chip->new_nand_info.slc_program_info.reg_offset_value[3] = 0x26;
		aml_chip->new_nand_info.slc_program_info.reg_offset_value[4] = 0x01;	

		aml_chip->new_nand_info.read_rety_info.get_default_value = aml_nand_get_read_default_value_hynix;
		aml_chip->new_nand_info.read_rety_info.read_retry_handle = aml_nand_read_retry_handle_hynix;
		aml_chip->new_nand_info.read_rety_info.set_default_value= aml_nand_set_readretry_default_value_hynix;
		
		aml_chip->new_nand_info.slc_program_info.enter_enslc_mode = aml_nand_enter_enslc_mode_hynix;
		aml_chip->new_nand_info.slc_program_info.exit_enslc_mode = aml_nand_exit_enslc_mode_hynix;	
		aml_chip->new_nand_info.slc_program_info.get_default_value = aml_nand_get_slc_default_value_hynix;
	
	}
	else  if(!strncmp((char*)type->id, (char*)dev_id_hynix_20nm_8g, strlen((const char*)aml_nand_flash_ids[i].id))){
		aml_chip->new_nand_info.type = HYNIX_20NM_8GB;
		aml_chip->ran_mode = 1;
		printk("aml_chip->hynix_new_nand_type =: %d \n", aml_chip->new_nand_info.type);

		//read retry
		aml_chip->new_nand_info.read_rety_info.reg_cnt = 8;
		aml_chip->new_nand_info.read_rety_info.retry_cnt = 7;

		aml_chip->new_nand_info.read_rety_info.reg_addr[0] = 0xCC;	//not same
		aml_chip->new_nand_info.read_rety_info.reg_addr[1] = 0xBF;
		aml_chip->new_nand_info.read_rety_info.reg_addr[2] = 0xAA;
		aml_chip->new_nand_info.read_rety_info.reg_addr[3] = 0xAB;
		aml_chip->new_nand_info.read_rety_info.reg_addr[4] = 0xCD;	//not same
		aml_chip->new_nand_info.read_rety_info.reg_addr[5] = 0xAD;
		aml_chip->new_nand_info.read_rety_info.reg_addr[6] = 0xAE;
		aml_chip->new_nand_info.read_rety_info.reg_addr[7] = 0xAF;
	/*	
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][0] = -0x14;		
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][1] = 0x0a;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][2] = 0x04;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][3] = -0x03;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][4] = -0x14;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][5] = 0x0A;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][6] = 0x04;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][7] = -0x03;

		aml_chip->new_nand_info.read_rety_info.reg_offset_value[1][0] = -0x0f;		//2//2nd
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[1][1] = 0x11;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[1][2] = 0x0a;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[1][3] = -0x04;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[1][4] = -0x0f;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[1][5] = 0x11;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[1][6] = 0x0a;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[1][7] = -0x04;	

		aml_chip->new_nand_info.read_rety_info.reg_offset_value[2][0] = -0x1a;		//3//3rd
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[2][1] = 0x02;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[2][2] = -0x05;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[2][3] = -0x0d;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[2][4] = -0x1a;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[2][5] = 0x02;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[2][6] = -0x05;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[2][7] = -0x0d;

		aml_chip->new_nand_info.read_rety_info.reg_offset_value[3][0] = 0x23;		//3//4th
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[3][1] = 0x00;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[3][2] = -0x05;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[3][3] = -0x0e;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[3][4] = 0x23;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[3][5] = 0x00;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[3][6] = -0x05;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[3][7] = -0x0e;

		aml_chip->new_nand_info.read_rety_info.reg_offset_value[4][0] = 0x1e;		//3//5th
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[4][1] = -0x02;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[4][2] = -0x06;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[4][3] = -0x13;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[4][4] = 0x1e;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[4][5] = -0x02;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[4][6] = -0x06;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[4][7] = -0x13;

		aml_chip->new_nand_info.read_rety_info.reg_offset_value[5][0] = 0x19;		//6th
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[5][1] = -0x03;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[5][2] = -0x08;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[5][3] = -0x16;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[5][4] = 0x19;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[5][5] = -0x03;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[5][6] = -0x08;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[5][7] = -0x16;	

		aml_chip->new_nand_info.read_rety_info.reg_offset_value[6][0] = 0x14;		//7th
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[6][1] = -0x21;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[6][2] = -0x1f;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[6][3] = -0x27;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[6][4] = 0x14;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[6][5] = -0x21;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[6][6] = -0x1f;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[6][7] = -0x27;
	*/

		aml_chip->new_nand_info.slc_program_info.reg_cnt = 4;

		aml_chip->new_nand_info.slc_program_info.reg_addr[0] = 0xB0;		//not same
		aml_chip->new_nand_info.slc_program_info.reg_addr[1] = 0xB1;
		aml_chip->new_nand_info.slc_program_info.reg_addr[2] = 0xA0;
		aml_chip->new_nand_info.slc_program_info.reg_addr[3] = 0xA1;

		aml_chip->new_nand_info.slc_program_info.reg_offset_value[0] = 0x0A;		//not same
		aml_chip->new_nand_info.slc_program_info.reg_offset_value[1] = 0x0A;
		aml_chip->new_nand_info.slc_program_info.reg_offset_value[2] = 0x0A;
		aml_chip->new_nand_info.slc_program_info.reg_offset_value[3] = 0x0A;


		aml_chip->new_nand_info.read_rety_info.get_default_value = aml_nand_get_read_default_value_hynix;
		aml_chip->new_nand_info.read_rety_info.read_retry_handle = aml_nand_read_retry_handle_hynix;
		aml_chip->new_nand_info.read_rety_info.set_default_value= aml_nand_set_readretry_default_value_hynix;
		aml_chip->new_nand_info.read_rety_info.read_retry_exit = aml_nand_read_retry_exit_hynix;
		
		aml_chip->new_nand_info.slc_program_info.enter_enslc_mode = aml_nand_enter_enslc_mode_hynix;
		aml_chip->new_nand_info.slc_program_info.exit_enslc_mode = aml_nand_exit_enslc_mode_hynix;	
		aml_chip->new_nand_info.slc_program_info.get_default_value = aml_nand_get_slc_default_value_hynix;

	}
	else  if(!strncmp((char*)type->id, (char*)dev_id_hynix_20nm_4g, strlen((const char*)aml_nand_flash_ids[i].id))
		||(!strncmp((char*)type->id, (char*)dev_id_hynix_20nm_lga_8g, strlen((const char*)aml_nand_flash_ids[i].id)))){
		aml_chip->new_nand_info.type = HYNIX_20NM_4GB;
		aml_chip->ran_mode = 1;
		printk("aml_chip->hynix_new_nand_type =: %d \n", aml_chip->new_nand_info.type);

		//read retry
		aml_chip->new_nand_info.read_rety_info.reg_cnt = 8;
		aml_chip->new_nand_info.read_rety_info.retry_cnt = 7;

		aml_chip->new_nand_info.read_rety_info.reg_addr[0] = 0xB0;	//not same
		aml_chip->new_nand_info.read_rety_info.reg_addr[1] = 0xB1;
		aml_chip->new_nand_info.read_rety_info.reg_addr[2] = 0xB2;
		aml_chip->new_nand_info.read_rety_info.reg_addr[3] = 0xB3;
		aml_chip->new_nand_info.read_rety_info.reg_addr[4] = 0xB4;	
		aml_chip->new_nand_info.read_rety_info.reg_addr[5] = 0xB5;
		aml_chip->new_nand_info.read_rety_info.reg_addr[6] = 0xB6;
		aml_chip->new_nand_info.read_rety_info.reg_addr[7] = 0xB7;
/*	
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][0] = -0x14;		
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][1] = 0x0a;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][2] = 0x04;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][3] = -0x03;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][4] = -0x14;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][5] = 0x0A;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][6] = 0x04;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][7] = -0x03;

	aml_chip->new_nand_info.read_rety_info.reg_offset_value[1][0] = -0x0f;		//2//2nd
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[1][1] = 0x11;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[1][2] = 0x0a;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[1][3] = -0x04;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[1][4] = -0x0f;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[1][5] = 0x11;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[1][6] = 0x0a;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[1][7] = -0x04;	

	aml_chip->new_nand_info.read_rety_info.reg_offset_value[2][0] = -0x1a;		//3//3rd
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[2][1] = 0x02;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[2][2] = -0x05;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[2][3] = -0x0d;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[2][4] = -0x1a;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[2][5] = 0x02;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[2][6] = -0x05;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[2][7] = -0x0d;

	aml_chip->new_nand_info.read_rety_info.reg_offset_value[3][0] = 0x23;		//3//4th
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[3][1] = 0x00;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[3][2] = -0x05;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[3][3] = -0x0e;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[3][4] = 0x23;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[3][5] = 0x00;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[3][6] = -0x05;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[3][7] = -0x0e;

	aml_chip->new_nand_info.read_rety_info.reg_offset_value[4][0] = 0x1e;		//3//5th
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[4][1] = -0x02;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[4][2] = -0x06;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[4][3] = -0x13;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[4][4] = 0x1e;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[4][5] = -0x02;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[4][6] = -0x06;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[4][7] = -0x13;

	aml_chip->new_nand_info.read_rety_info.reg_offset_value[5][0] = 0x19;		//6th
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[5][1] = -0x03;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[5][2] = -0x08;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[5][3] = -0x16;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[5][4] = 0x19;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[5][5] = -0x03;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[5][6] = -0x08;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[5][7] = -0x16;	

	aml_chip->new_nand_info.read_rety_info.reg_offset_value[6][0] = 0x14;		//7th
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[6][1] = -0x21;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[6][2] = -0x1f;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[6][3] = -0x27;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[6][4] = 0x14;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[6][5] = -0x21;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[6][6] = -0x1f;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[6][7] = -0x27;
*/

		aml_chip->new_nand_info.slc_program_info.reg_cnt = 4;

		aml_chip->new_nand_info.slc_program_info.reg_addr[0] = 0xA0;		//not same
		aml_chip->new_nand_info.slc_program_info.reg_addr[1] = 0xA1;
		aml_chip->new_nand_info.slc_program_info.reg_addr[2] = 0xA7;
		aml_chip->new_nand_info.slc_program_info.reg_addr[3] = 0xA8;

		aml_chip->new_nand_info.slc_program_info.reg_offset_value[0] = 0x0A;		//not same
		aml_chip->new_nand_info.slc_program_info.reg_offset_value[1] = 0x0A;
		aml_chip->new_nand_info.slc_program_info.reg_offset_value[2] = 0x0A;
		aml_chip->new_nand_info.slc_program_info.reg_offset_value[3] = 0x0A;


		aml_chip->new_nand_info.read_rety_info.get_default_value = aml_nand_get_read_default_value_hynix;
		aml_chip->new_nand_info.read_rety_info.read_retry_handle = aml_nand_read_retry_handle_hynix;
		aml_chip->new_nand_info.read_rety_info.set_default_value= aml_nand_set_readretry_default_value_hynix;
		aml_chip->new_nand_info.read_rety_info.read_retry_exit = aml_nand_read_retry_exit_hynix;
		
		aml_chip->new_nand_info.slc_program_info.enter_enslc_mode = aml_nand_enter_enslc_mode_hynix;
		aml_chip->new_nand_info.slc_program_info.exit_enslc_mode = aml_nand_exit_enslc_mode_hynix;	
		aml_chip->new_nand_info.slc_program_info.get_default_value = aml_nand_get_slc_default_value_hynix;

	}
	else  if((!strncmp((char*)type->id, (char*)dev_id_toshiba_24nm_4g, strlen((const char*)aml_nand_flash_ids[i].id)))
	            ||(!strncmp((char*)type->id, (char*)dev_id_toshiba_24nm_8g, strlen((const char*)aml_nand_flash_ids[i].id)))
	            || (!strncmp((char*)type->id, (char*)dev_id_toshiba_19nm_8g, strlen((const char*)aml_nand_flash_ids[i].id)))
	            || (!strncmp((char*)type->id, (char*)dev_id_toshiba_19nm_4g, strlen((const char*)aml_nand_flash_ids[i].id)))){
		aml_chip->new_nand_info.type =  TOSHIBA_24NM;
		aml_chip->ran_mode = 1;

		aml_chip->new_nand_info.read_rety_info.reg_addr[0] = 0x04;
		aml_chip->new_nand_info.read_rety_info.reg_addr[1] = 0x05;
		aml_chip->new_nand_info.read_rety_info.reg_addr[2] = 0x06;
		aml_chip->new_nand_info.read_rety_info.reg_addr[3] = 0x07;		

		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][0][0] = 0;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][0][1] = 0;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][0][2] = 0;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][0][3] = 0;

		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][1][0] = 0x04;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][1][1] = 0x04;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][1][2] = 0x04;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][1][3] = 0x04;		

		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][2][0] = 0x7c;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][2][1] = 0x7c;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][2][2] = 0x7c;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][2][3] = 0x7c;	

		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][3][0] = 0x78;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][3][1] = 0x78;   
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][3][2] = 0x78;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][3][3] = 0x78;	

		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][4][0] = 0x74;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][4][1] = 0x74;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][4][2] = 0x74;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][4][3] = 0x74;	

		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][5][0] = 0x08;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][5][1] = 0x08;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][5][2] = 0x08;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][5][3] = 0x08;	

		aml_chip->new_nand_info.read_rety_info.reg_cnt = 4;
		aml_chip->new_nand_info.read_rety_info.retry_cnt = 6;
		
		aml_chip->new_nand_info.read_rety_info.read_retry_handle = aml_nand_read_retry_handle_toshiba;
		aml_chip->new_nand_info.read_rety_info.read_retry_exit = aml_nand_read_retry_exit_toshiba;	
		
	}
	else  if((!strncmp((char*)type->id, (char*)dev_id_samsung_2xnm_4g, strlen((const char*)aml_nand_flash_ids[i].id)))||
		(!strncmp((char*)type->id, (char*)dev_id_samsung_2xnm_8g, strlen((const char*)aml_nand_flash_ids[i].id)))){
	aml_chip->new_nand_info.type =  SUMSUNG_2XNM;
	aml_chip->ran_mode = 1;

	aml_chip->new_nand_info.read_rety_info.reg_addr[0] = 0xA7;
	aml_chip->new_nand_info.read_rety_info.reg_addr[1] = 0xA4;
	aml_chip->new_nand_info.read_rety_info.reg_addr[2] = 0xA5;
	aml_chip->new_nand_info.read_rety_info.reg_addr[3] = 0xA6;		

	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][0][0] = 0;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][0][1] = 0;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][0][2] = 0;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][0][3] = 0;

	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][1][0] = 0x05;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][1][1] = 0x0A;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][1][2] = 0x00;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][1][3] = 0x00;		

	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][2][0] = 0x28;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][2][1] = 0x00;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][2][2] = 0xEc;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][2][3] = 0xD8;	

	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][3][0] = 0xED;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][3][1] = 0xF5;   
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][3][2] = 0xED;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][3][3] = 0xE6;	

	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][4][0] = 0x0A;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][4][1] = 0x0F;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][4][2] = 0x05;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][4][3] = 0x00;	

	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][5][0] = 0x0F;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][5][1] = 0x0A;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][5][2] = 0xFB;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][5][3] = 0xEC;	

	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][6][0] = 0XE8;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][6][1] = 0XEF;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][6][2] = 0XE8;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][6][3] = 0XDC;

	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][7][0] = 0xF1;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][7][1] = 0xFB;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][7][2] = 0xFE;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][7][3] = 0xF0;		

	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][8][0] = 0x0A;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][8][1] = 0x00;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][8][2] = 0xFB;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][8][3] = 0xEC;	

	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][9][0] = 0xD0;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][9][1] = 0xE2;   
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][9][2] = 0xD0;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][9][3] = 0xC2;	

	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][10][0] = 0x14;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][10][1] = 0x0F;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][10][2] = 0xFB;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][10][3] = 0xEC;	

	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][11][0] = 0xE8;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][11][1] = 0xFB;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][11][2] = 0xE8;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][11][3] = 0xDC;	

	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][12][0] = 0X1E;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][12][1] = 0X14;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][12][2] = 0XFB;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][12][3] = 0XEC;

	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][13][0] = 0xFB;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][13][1] = 0xFF;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][13][2] = 0xFB;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][13][3] = 0xF8;		

	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][14][0] = 0x07;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][14][1] = 0x0C;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][14][2] = 0x02;
	aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][14][3] = 0x00;	
	

	aml_chip->new_nand_info.read_rety_info.reg_cnt = 4;
	aml_chip->new_nand_info.read_rety_info.retry_cnt = 15;
	
	aml_chip->new_nand_info.read_rety_info.read_retry_handle = aml_nand_read_retry_handle_samsung;
	aml_chip->new_nand_info.read_rety_info.read_retry_exit = aml_nand_read_retry_exit_samsung;
}
else  if(!strncmp((char*)type->id, (char*)dev_id_sandisk_24nm_8g, strlen((const char*)aml_nand_flash_ids[i].id))) {
		aml_chip->new_nand_info.type =  SANDISK_24NM;	
		aml_chip->ran_mode = 1;	
		aml_chip->new_nand_info.dynamic_read_info.dynamic_read_flag = 0; //DRF
		aml_chip->new_nand_info.dynamic_read_info.reg_addr_init[0] = 0x04;
		aml_chip->new_nand_info.dynamic_read_info.reg_addr_init[1] = 0x05;
		aml_chip->new_nand_info.dynamic_read_info.reg_addr_init[2] = 0x06;
		aml_chip->new_nand_info.dynamic_read_info.reg_addr_init[3] = 0x07;
		aml_chip->new_nand_info.dynamic_read_info.reg_addr_init[4] = 0x08;
		aml_chip->new_nand_info.dynamic_read_info.reg_addr_init[5] = 0x09;
		aml_chip->new_nand_info.dynamic_read_info.reg_addr_init[6] = 0x0a;
		aml_chip->new_nand_info.dynamic_read_info.reg_addr_init[7] = 0x0b;
		aml_chip->new_nand_info.dynamic_read_info.reg_addr_init[8] = 0x0c;
		aml_chip->new_nand_info.read_rety_info.reg_addr[0] = 0x04;		
		aml_chip->new_nand_info.read_rety_info.reg_addr[1] = 0x05;
		aml_chip->new_nand_info.read_rety_info.reg_addr[2] = 0x07;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][0][0] = 0;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][0][1] = 0xf0;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][0][2] = 0;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][1][0] = 0;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][1][1] = 0xe0;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][1][2] = 0;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][2][0] = 0xff;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][2][1] = 0xf0;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][2][2] = 0xf0;	
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][3][0] = 0xee;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][3][1] = 0xe0;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][3][2] = 0xe0;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][4][0] = 0xde;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][4][1] = 0xd0;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][4][2] = 0xd0;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][5][0] = 0xcd;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][5][1] = 0xc0;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][5][2] = 0xc0;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][6][0] = 0x01;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][6][1] = 0x00;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][6][2] = 0x00;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][7][0] = 0x02;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][7][1] = 0x00;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][7][2] = 0x00;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][8][0] = 0x03;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][8][1] = 0x00;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][8][2] = 0x00;		
		aml_chip->new_nand_info.read_rety_info.reg_cnt = 3;
		aml_chip->new_nand_info.read_rety_info.retry_cnt = 9;
		aml_chip->new_nand_info.dynamic_read_info.dynamic_read_init = aml_nand_dynamic_read_init;
		aml_chip->new_nand_info.read_rety_info.read_retry_handle = aml_nand_read_retry_handle_sandisk;
		aml_chip->new_nand_info.read_rety_info.read_retry_exit = aml_nand_dynamic_read_exit;
	}
	else  if((!strncmp((char*)type->id, (char*)dev_id_sandisk_19nm_8g, strlen((const char*)aml_nand_flash_ids[i].id))) ||
		(!strncmp((char*)type->id, (char*)dev_id_sandisk_19nm_4g, strlen((const char*)aml_nand_flash_ids[i].id)))){
		aml_chip->new_nand_info.type =  SANDISK_19NM;
		aml_chip->ran_mode = 1;		
		aml_chip->new_nand_info.dynamic_read_info.dynamic_read_flag = 0; //DRF
		
		aml_chip->new_nand_info.dynamic_read_info.slc_flag = 0; //slc_mode
		
		memset(&aml_chip->new_nand_info.dynamic_read_info.cur_case_num_lower_page[0], 0, MAX_CHIP_NUM);
		memset(&aml_chip->new_nand_info.dynamic_read_info.cur_case_num_upper_page[0], 0, MAX_CHIP_NUM);
		
		aml_chip->new_nand_info.dynamic_read_info.reg_addr_init[0] = 0x04;
		aml_chip->new_nand_info.dynamic_read_info.reg_addr_init[1] = 0x05;
		aml_chip->new_nand_info.dynamic_read_info.reg_addr_init[2] = 0x06;
		aml_chip->new_nand_info.dynamic_read_info.reg_addr_init[3] = 0x07;
		aml_chip->new_nand_info.dynamic_read_info.reg_addr_init[4] = 0x08;
		aml_chip->new_nand_info.dynamic_read_info.reg_addr_init[5] = 0x09;
		aml_chip->new_nand_info.dynamic_read_info.reg_addr_init[6] = 0x0a;
		aml_chip->new_nand_info.dynamic_read_info.reg_addr_init[7] = 0x0b;
		aml_chip->new_nand_info.dynamic_read_info.reg_addr_init[8] = 0x0c;

		aml_chip->new_nand_info.dynamic_read_info.reg_addr_lower_page[0] = 0x04;		
		aml_chip->new_nand_info.dynamic_read_info.reg_addr_lower_page[1] = 0x05;
		aml_chip->new_nand_info.dynamic_read_info.reg_addr_lower_page[2] = 0x07;

		aml_chip->new_nand_info.dynamic_read_info.reg_addr_upper_page[0] = 0x04;	
		aml_chip->new_nand_info.dynamic_read_info.reg_addr_upper_page[1] = 0x05;
		aml_chip->new_nand_info.dynamic_read_info.reg_addr_upper_page[2] = 0x07;

//lower_page
		//aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[0][0][0] = 0x0;	
		//aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[0][0][1] = 0x0;
		
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[0][0] = 0xF0;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[0][1] = 0x0;	
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[0][2] = 0xF0;
		
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[1][0] = 0xE0;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[1][1] = 0x0;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[1][2] = 0xE0;

		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[2][0] = 0xD0;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[2][1] = 0x0;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[2][2] = 0xD0;

		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[3][0] = 0x10;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[3][1] = 0x0;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[3][2] = 0x10;

		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[4][0] = 0x20;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[4][1] = 0x0;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[4][2] = 0x20;

		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[5][0] = 0x30;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[5][1] = 0x0;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[5][2] = 0x30;

		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[6][0] = 0xC0;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[6][1] = 0x0;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[6][2] = 0xD0;

		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[7][0] = 0x0;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[7][1] = 0x0;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[7][2] = 0x10;

		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[8][0] = 0x0;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[8][1] = 0x0;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[8][2] = 0x20;

		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[9][0] = 0x10;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[9][1] = 0x0;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[9][2] = 0x20;

		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[10][0] = 0xB0;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[10][1] = 0x0;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[10][2] = 0xD0;

		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[11][0] = 0xA0;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[11][1] = 0x0;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[11][2] = 0xD0;

		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[12][0] = 0x90;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[12][1] = 0x0;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[12][2] = 0xD0;

		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[13][0] = 0xB0;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[13][1] = 0x0;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[13][2] = 0xC0;

		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[14][0] = 0xA0;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[14][1] = 0x0;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[14][2] = 0xC0;

		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[15][0] = 0x90;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[15][1] = 0x0;	
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_lower_page[15][2] = 0xC0;
//upper_page

		//aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[0][0][0] = 0x0;
		//aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[0][0][1] = 0x0;

		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[0][0] = 0x0;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[0][1] = 0xF0;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[0][2] = 0x0;

		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[1][0] = 0xF;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[1][1] = 0xE0;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[1][2] = 0x0;

		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[2][0] = 0xF;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[2][1] = 0xD0;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[2][2] = 0x0;

		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[3][0] = 0xE;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[3][1] = 0xE0;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[3][2] = 0x0;

		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[4][0] = 0xE;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[4][1] = 0xD0;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[4][2] = 0x0;

		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[5][0] = 0xD;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[5][1] = 0xF0;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[5][2] = 0x0;

		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[6][0] = 0xD;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[6][1] = 0xE0;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[6][2] = 0x0;

		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[7][0] = 0xD;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[7][1] = 0xD0;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[7][2] = 0x0;

		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[8][0] = 0x1;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[8][1] = 0x10;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[8][2] = 0x0;

		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[9][0] = 0x2;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[9][1] = 0x20;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[9][2] = 0x0;

		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[10][0] = 0x2;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[10][1] = 0x10;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[10][2] = 0x0;

		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[11][0] = 0x3;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[11][1] = 0x20;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[11][2] = 0x0;

		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[12][0] = 0xF;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[12][1] = 0x00;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[12][2] = 0x0;

		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[13][0] = 0xE;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[13][1] = 0xF0;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[13][2] = 0x0;

		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[14][0] = 0xD;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[14][1] = 0xC0;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[14][2] = 0x0;

		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[15][0] = 0xF;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[15][1] = 0xF0;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[15][2] = 0x0;

		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[16][0] = 0x1;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[16][1] = 0x00;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[16][2] = 0x0;

		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[17][0] = 0x20;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[17][1] = 0x00;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[17][2] = 0x0;

		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[18][0] = 0xD;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[18][1] = 0xB0;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[18][2] = 0x0;

		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[19][0] = 0xC;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[19][1] = 0xA0;
		aml_chip->new_nand_info.dynamic_read_info.reg_offset_value_upper_page[19][2] = 0x0;

		aml_chip->new_nand_info.dynamic_read_info.dynamic_read_init = aml_nand_dynamic_read_init;
		aml_chip->new_nand_info.dynamic_read_info.dynamic_read_handle = aml_nand_dynamic_read_handle;
		aml_chip->new_nand_info.dynamic_read_info.dynamic_read_exit = aml_nand_dynamic_read_exit;
		aml_chip->new_nand_info.dynamic_read_info.enter_slc_mode = aml_nand_enter_slc_mode_sandisk;
		aml_chip->new_nand_info.dynamic_read_info.exit_slc_mode= aml_nand_exit_slc_mode_sandisk;
	}
	else  if((!strncmp((char*)type->id, (char*)dev_id_micron_20nm_8g, strlen((const char*)aml_nand_flash_ids[i].id)))
		||(!strncmp((char*)type->id, (char*)dev_id_micron_20nm_4g, strlen((const char*)aml_nand_flash_ids[i].id)))){
		aml_chip->new_nand_info.type =  MICRON_20NM;
		aml_chip->ran_mode = 1;	
		
		aml_chip->new_nand_info.read_rety_info.reg_addr[0] = 0x89;

		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][0][0] = 0x1;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][1][0] = 0x2;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][2][0] = 0x3;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][3][0] = 0x4;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][4][0] = 0x5;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][5][0] = 0x6;
		aml_chip->new_nand_info.read_rety_info.reg_offset_value[0][6][0] = 0x7;


		aml_chip->new_nand_info.read_rety_info.reg_cnt = 1;
		aml_chip->new_nand_info.read_rety_info.retry_cnt = 7;

		aml_chip->new_nand_info.read_rety_info.read_retry_handle = aml_nand_read_retry_handle_micron;
		aml_chip->new_nand_info.read_rety_info.read_retry_exit = aml_nand_read_retry_exit_micron;	

}
#endif
	if (!mtd->name)
		mtd->name = type->name;

	chip->chipsize = type->chipsize;
	chip->chipsize = chip->chipsize << 20;

	/* Newer devices have all the information in additional id bytes */
	if (!type->pagesize) {
		int extid;
		/* The 3rd id byte holds MLC / multichip data */
		chip->cellinfo = chip->read_byte(mtd);
		/* The 4th id byte is the important one */
		extid = chip->read_byte(mtd);
		/* Calc pagesize */
		mtd->writesize = 1024 << (extid & 0x3);
		extid >>= 2;
		/* Calc oobsize */
		mtd->oobsize = (8 << (extid & 0x01)) * (mtd->writesize >> 9);
		extid >>= 2;
		/* Calc blocksize. Blocksize is multiples of 64KiB */
		mtd->erasesize = (64 * 1024) << (extid & 0x03);
		extid >>= 2;
		/* Get buswidth information */
		busw = (extid & 0x01) ? NAND_BUSWIDTH_16 : 0;

	} else {
		/*
		 * Old devices have chip data hardcoded in the device id table
		 */
		mtd->erasesize = type->erasesize;
		mtd->writesize = type->pagesize;
		mtd->oobsize = type->oobsize;
		busw = type->options & NAND_BUSW_OPTIONS_MASK;
	}

	/* Try to identify manufacturer */
	for (maf_idx = 0; nand_manuf_ids[maf_idx].id != 0x0; maf_idx++) {
		if (nand_manuf_ids[maf_idx].id == *maf_id)
			break;
	}

	/*
	 * Check, if buswidth is correct. Hardware drivers should set
	 * chip correct !
	 */
	if (busw != (chip->options & NAND_BUSWIDTH_16)) {
		printk(KERN_INFO "NAND device: Manufacturer ID:"
		       " 0x%02x, Chip ID: 0x%02x (%s %s)\n", *maf_id,
		       dev_id[0], nand_manuf_ids[maf_idx].name, mtd->name);
		printk(KERN_WARNING "NAND bus width %d instead %d bit\n",
		       (chip->options & NAND_BUSWIDTH_16) ? 16 : 8,
		       busw ? 16 : 8);
		return ERR_PTR(-EINVAL);
	}

	/* Calculate the address shift from the page size */
	chip->page_shift = ffs(mtd->writesize) - 1;
	/* Convert chipsize to number of pages per chip -1. */
	chip->pagemask = (chip->chipsize >> chip->page_shift) - 1;

	chip->bbt_erase_shift = chip->phys_erase_shift = ffs(mtd->erasesize) - 1;
	chip->chip_shift = ffs(chip->chipsize) - 1;

	/* Set the bad block position */
	chip->badblockpos = AML_BADBLK_POS;

	/* Get chip options, preserve non chip based options */
	//chip->options &= ~NAND_CHIPOPTIONS_MSK;
	//chip->options |= type->options & NAND_CHIPOPTIONS_MSK;

	/*
	 * Set chip as a default. Board drivers can override it, if necessary
	 */
	chip->options |= NAND_NO_AUTOINCR;

	/* Check if chip is a not a samsung device. Do not clear the
	 * options for chips which are not having an extended id.
	 */
	//if (*maf_id != NAND_MFR_SAMSUNG && !type->pagesize)
		//chip->options &= ~NAND_SAMSUNG_LP_OPTIONS;

	printk(KERN_INFO "NAND device: Manufacturer ID:"
	       " 0x%02x, Chip ID: 0x%02x (%s %s)\n", *maf_id, dev_id[0],
	       nand_manuf_ids[maf_idx].name, type->name);

	return type;
}

static int aml_nand_scan_ident(struct mtd_info *mtd, int maxchips)
{
	int i, busw, nand_maf_id, valid_chip_num = 1;
	struct nand_chip *chip = mtd->priv;
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	struct aml_nand_flash_dev *aml_type;
	u8 dev_id[MAX_ID_LEN], onfi_features[4];
	unsigned temp_chip_shift;

	/* Get buswidth to select the correct functions */
	busw = chip->options & NAND_BUSWIDTH_16;

	/* Select the device */
	chip->select_chip(mtd, 0);

	//reset chip for some nand need reset after power up
	chip->cmdfunc(mtd, NAND_CMD_RESET, -1, -1);
	aml_chip->aml_nand_wait_devready(aml_chip, 0);

	/* Read the flash type */
	aml_type = aml_nand_get_flash_type(mtd, chip, busw, &nand_maf_id);

	if (IS_ERR(aml_type)) {
		printk(KERN_WARNING "No NAND device found!!!\n");
		chip->select_chip(mtd, -1);
		return PTR_ERR(aml_type);
	}

	chip->cmdfunc(mtd, NAND_CMD_READID, 0x20, -1);
	for (i=0; i<MAX_ID_LEN; i++) {
		dev_id[i] = chip->read_byte(mtd);
	}
	if(!memcmp((char*)dev_id, "ONFI", 4))
		aml_chip->onfi_mode = aml_type->onfi_mode;

	aml_chip->T_REA = aml_type->T_REA;
	aml_chip->T_RHOH = aml_type->T_RHOH;
	aml_chip->mfr_type = aml_type->id[0];

	/* Check for a chip array */
	for (i = 1; i < maxchips; i++) {
		aml_chip->aml_nand_select_chip(aml_chip, i);
		chip->cmdfunc(mtd, NAND_CMD_RESET, -1, -1);
		aml_chip->aml_nand_wait_devready(aml_chip, i);

		/* Send the command for reading device ID */
		chip->cmdfunc(mtd, NAND_CMD_READID, 0x00, -1);
		/* Read manufacturer and device IDs */

		if (nand_maf_id != chip->read_byte(mtd) || aml_type->id[1] != chip->read_byte(mtd))
		//if (nand_maf_id != dev_id[0] || aml_type->id[1] != dev_id[1])
			aml_chip->valid_chip[i] = 0;
		else
			valid_chip_num ++;
	}
	if (i > 1) {
		printk(KERN_INFO "%d NAND chips detected\n", valid_chip_num);
		/*if ((aml_chip->valid_chip[1] == 0) && (aml_chip->valid_chip[2] == 1)) {
			printk("ce1 and ce2 connected\n");
			aml_chip->chip_enable[2] = (aml_chip->chip_enable[1] & aml_chip->chip_enable[2]);
		}*/
	}
	if (aml_chip->onfi_mode) {
		aml_nand_set_onfi_features(aml_chip, (uint8_t *)(&aml_chip->onfi_mode), ONFI_TIMING_ADDR);
		aml_nand_get_onfi_features(aml_chip, onfi_features, ONFI_TIMING_ADDR);
		if (onfi_features[0] != aml_chip->onfi_mode) {
			aml_chip->T_REA = DEFAULT_T_REA;
			aml_chip->T_RHOH = DEFAULT_T_RHOH;
			printk("onfi timing mode set failed: %x\n", onfi_features[0]);		
		}
	}

	/* Store the number of chips and calc total size for mtd */
	chip->numchips = 1;
	if ((chip->chipsize >> 32) & 0xffffffff)
		chip->chip_shift = fls((unsigned)(chip->chipsize >> 32))-1 + fls(valid_chip_num)-1 + 32;
	else 
		chip->chip_shift = fls((unsigned)chip->chipsize)-1 + fls(valid_chip_num)-1;

	chip->pagemask = ((chip->chipsize * valid_chip_num) >> chip->page_shift) - 1;
	chip->options &= ~NAND_CACHEPRG;
	aml_chip->internal_chipnr = aml_type->internal_chipnr;
	aml_chip->internal_page_nums = (chip->chipsize >> chip->page_shift);
	aml_chip->internal_page_nums /= aml_chip->internal_chipnr;
	aml_chip->internal_chip_shift = fls((unsigned)aml_chip->internal_page_nums) - 1;
	temp_chip_shift = ffs((unsigned)aml_chip->internal_page_nums) - 1;
	if (aml_chip->internal_chip_shift != temp_chip_shift) {
		aml_chip->internal_chip_shift += 1;
		chip->chip_shift += 1;
		chip->pagemask = ((1 << (chip->chip_shift + 1)) >> chip->page_shift) - 1;
	}

	aml_chip->options = aml_type->options;
	aml_chip->page_size = aml_type->pagesize;
	aml_chip->block_size = aml_type->erasesize;
	aml_chip->oob_size = aml_type->oobsize;
	mtd->erasesize = valid_chip_num * aml_type->erasesize;
	mtd->writesize = valid_chip_num * aml_type->pagesize;
	mtd->oobsize = valid_chip_num * aml_type->oobsize;
	mtd->size = valid_chip_num * chip->chipsize;

	chip->cmdfunc = aml_nand_command;
	chip->waitfunc = aml_nand_wait;
	chip->erase_cmd = aml_nand_erase_cmd;
	chip->write_page = aml_nand_write_page;

	return 0;
}

int aml_nand_scan(struct mtd_info *mtd, int maxchips)
{
	int ret;

	ret = aml_nand_scan_ident(mtd, maxchips);
	if (!ret)
		ret = nand_scan_tail(mtd);
	return ret;
}

static int aml_platform_options_confirm(struct aml_nand_chip *aml_chip)
{
	struct mtd_info *mtd = &aml_chip->mtd;
	struct nand_chip *chip = &aml_chip->chip;
	struct aml_nand_platform *plat = aml_chip->platform;
	unsigned options_selected = 0, options_support = 0, ecc_bytes, options_define;
	int error = 0;

	options_selected = (plat->platform_nand_data.chip.options & NAND_ECC_OPTIONS_MASK);
	options_define = (aml_chip->options & NAND_ECC_OPTIONS_MASK);

ecc_unit_change:
	ecc_bytes = aml_chip->oob_size / (aml_chip->page_size / chip->ecc.size);
	if (chip->ecc.size == NAND_ECC_UNIT_1KSIZE) {
		if (ecc_bytes >= (NAND_BCH24_1K_ECC_SIZE + 2))
			options_support = NAND_ECC_BCH24_1K_MODE;
		else {
			aml_nand_debug("oob size is not enough for 1K UNIT ECC mode: %d try 512 UNIT ECC\n", aml_chip->oob_size);
			chip->ecc.size = NAND_ECC_UNIT_SIZE;
			goto ecc_unit_change;
		}	
	}
	else {
		if (ecc_bytes >= (NAND_BCH16_ECC_SIZE + 2))
			options_support = NAND_ECC_BCH16_MODE;
		else if (ecc_bytes >= (NAND_BCH12_ECC_SIZE + 2))
			options_support = NAND_ECC_BCH12_MODE;
		else if (ecc_bytes >= (NAND_BCH8_ECC_SIZE + 2))
			options_support = NAND_ECC_BCH8_MODE;
		else {
			options_support = NAND_ECC_SOFT_MODE;
			aml_nand_debug("page size: %d oob size %d is not enough for HW ECC\n", aml_chip->page_size, aml_chip->oob_size);
		}
	}
	if (options_define != options_support) {
		options_define = options_support;
		aml_nand_debug("define oob size: %d could support bch mode: %s\n", aml_chip->oob_size, aml_nand_bch_string[options_support]);
	}

	if ((options_selected > options_define) && (strncmp((char*)plat->name, NAND_BOOT_NAME, strlen((const char*)NAND_BOOT_NAME)))) {
		aml_nand_debug("oob size is not enough for selected bch mode: %s force bch to mode: %s\n", aml_nand_bch_string[options_selected], aml_nand_bch_string[options_define]);
		options_selected = options_define;
	}

	switch (options_selected) {

		case NAND_ECC_BCH9_MODE:
			chip->ecc.size = NAND_ECC_UNIT_SIZE;				//our hardware ecc unit is 512bytes
			chip->ecc.bytes = NAND_BCH9_ECC_SIZE;
			aml_chip->bch_mode = NAND_ECC_BCH9;
			aml_chip->user_byte_mode = 1;
			break;

		case NAND_ECC_BCH8_MODE:
			chip->ecc.size = NAND_ECC_UNIT_SIZE;
			chip->ecc.bytes = NAND_BCH8_ECC_SIZE;
			aml_chip->bch_mode = NAND_ECC_BCH8;
			aml_chip->user_byte_mode = 2;
			break;

		case NAND_ECC_BCH12_MODE:
			chip->ecc.size = NAND_ECC_UNIT_SIZE;
			chip->ecc.bytes = NAND_BCH12_ECC_SIZE;
			aml_chip->bch_mode = NAND_ECC_BCH12;
			aml_chip->user_byte_mode = 2;
			break;

		case NAND_ECC_BCH16_MODE:
			chip->ecc.size = NAND_ECC_UNIT_SIZE;
			chip->ecc.bytes = NAND_BCH16_ECC_SIZE;
			aml_chip->bch_mode = NAND_ECC_BCH16;
			aml_chip->user_byte_mode = 2;
			break;

		case NAND_ECC_BCH24_1K_MODE:
			chip->ecc.size = NAND_ECC_UNIT_1KSIZE;
			chip->ecc.bytes = NAND_BCH24_1K_ECC_SIZE;
			aml_chip->bch_mode = NAND_ECC_BCH24_1K;
			aml_chip->user_byte_mode = 2;
			break;

		default :
			if ((plat->platform_nand_data.chip.options & NAND_ECC_OPTIONS_MASK) != NAND_ECC_SOFT_MODE) {
				aml_nand_debug("soft ecc or none ecc just support in linux self nand base please selected it at platform options\n");
				error = -ENXIO;
			}
			break;
	}

	options_selected = (plat->platform_nand_data.chip.options & NAND_PLANE_OPTIONS_MASK);
	options_define = (aml_chip->options & NAND_PLANE_OPTIONS_MASK);
	if (options_selected > options_define) {
		aml_nand_debug("multi plane error for selected plane mode: %s force plane to : %s\n", aml_nand_plane_string[options_selected >> 4], aml_nand_plane_string[options_define >> 4]);
		options_selected = options_define;
	}

	switch (options_selected) {

		case NAND_TWO_PLANE_MODE:
			aml_chip->plane_num = 2;
			mtd->erasesize *= 2;
			mtd->writesize *= 2;
			mtd->oobsize *= 2;
			break;

		default:
			aml_chip->plane_num = 1;
			break;
	}

	options_selected = (plat->platform_nand_data.chip.options & NAND_INTERLEAVING_OPTIONS_MASK);
	options_define = (aml_chip->options & NAND_INTERLEAVING_OPTIONS_MASK);
	if (options_selected > options_define) {
		aml_nand_debug("internal mode error for selected internal mode: %s force internal mode to : %s\n", aml_nand_internal_string[options_selected >> 16], aml_nand_internal_string[options_define >> 16]);
		options_selected = options_define;
	}

	switch (options_selected) {

		case NAND_INTERLEAVING_MODE:
			aml_chip->ops_mode |= AML_INTERLEAVING_MODE;
			mtd->erasesize *= aml_chip->internal_chipnr;
			mtd->writesize *= aml_chip->internal_chipnr;
			mtd->oobsize *= aml_chip->internal_chipnr;
			break;

		default:		
			break;
	}

	return error;
}

static uint8_t aml_platform_read_byte(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);

	NFC_SEND_CMD(aml_chip->chip_selected | DRD | 0);
	NFC_SEND_CMD(aml_chip->chip_selected | IDLE | 5);
	while(NFC_CMDFIFO_SIZE()>0);
	return readb(chip->IO_ADDR_R);
}

static void aml_platform_write_byte(struct aml_nand_chip *aml_chip, uint8_t data)
{
	NFC_SEND_CMD(aml_chip->chip_selected | IDLE | 5);
	NFC_SEND_CMD(aml_chip->chip_selected | DWR | data);
	NFC_SEND_CMD(aml_chip->chip_selected | IDLE | 5);
	while(NFC_CMDFIFO_SIZE()>0);
	return;
}

static int aml_nand_read_env (struct mtd_info *mtd, size_t offset, u_char * buf)
{
	struct env_oobinfo_t *env_oobinfo;
	int error = 0, err, start_blk, total_blk;
	size_t addr = 0;
	size_t amount_loaded = 0;
	size_t len;
	//struct mtd_oob_ops aml_oob_ops;
	struct mtd_oob_ops  * aml_oob_ops = NULL; 
	unsigned char *data_buf;
	unsigned char env_oob_buf[sizeof(struct env_oobinfo_t)];

	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	if (!aml_chip->aml_nandenv_info->env_valid)
		return 1;

	addr = (1024 * mtd->writesize / aml_chip->plane_num);
#ifdef NEW_NAND_SUPPORT
	if((aml_chip->new_nand_info.type) && (aml_chip->new_nand_info.type < 10))
		addr += RETRY_NAND_BLK_NUM* mtd->erasesize;
#endif
	start_blk = addr / mtd->erasesize;
	total_blk = mtd->size / mtd->erasesize;
	addr = aml_chip->aml_nandenv_info->env_valid_node->phy_blk_addr;
	addr *= mtd->erasesize;
	addr += aml_chip->aml_nandenv_info->env_valid_node->phy_page_addr * mtd->writesize;

	data_buf = kzalloc(mtd->writesize, GFP_KERNEL);
	if (data_buf == NULL){
		printk("%s %d no mem for data_buf\n", __func__, __LINE__);
		err = -ENOMEM;
		goto exit;
	}
	
	aml_oob_ops = kzalloc(sizeof(struct mtd_oob_ops), GFP_KERNEL);
	if (aml_oob_ops == NULL){
		printk("%s %d no mem for aml_oob_ops\n", __func__, __LINE__);
		err = -ENOMEM;
		goto exit;
	}

	env_oobinfo = (struct env_oobinfo_t *)env_oob_buf;
	while (amount_loaded < CONFIG_ENV_SIZE ) {

		aml_oob_ops->mode = MTD_OOB_AUTO;
		aml_oob_ops->len = mtd->writesize;
		aml_oob_ops->ooblen = sizeof(struct env_oobinfo_t);
		aml_oob_ops->ooboffs = mtd->ecclayout->oobfree[0].offset;
		aml_oob_ops->datbuf = data_buf;
		aml_oob_ops->oobbuf = env_oob_buf;

		memset((unsigned char *)aml_oob_ops->datbuf, 0x0, mtd->writesize);
		memset((unsigned char *)aml_oob_ops->oobbuf, 0x0, aml_oob_ops->ooblen);

		error = mtd->read_oob(mtd, addr, aml_oob_ops);
		if ((error != 0) && (error != -EUCLEAN)) {
			printk("blk check good but read failed: %llx, %d\n", (uint64_t)addr, error);
			//return 1; 
			err = 1;
			goto exit;
		}

		if (memcmp(env_oobinfo->name, ENV_NAND_MAGIC, 4)) 
			printk("invalid nand env magic: %llx\n", (uint64_t)addr);

		addr += mtd->writesize;
		len = min(mtd->writesize, CONFIG_ENV_SIZE - amount_loaded);
		memcpy(buf + amount_loaded, data_buf, len);
		amount_loaded += mtd->writesize;
	}
	if (amount_loaded < CONFIG_ENV_SIZE){
		//return 1;
		err = 1;
		goto exit;
		}
	
	kfree(data_buf);
	kfree(aml_oob_ops);
	return 0;

exit:
	if (aml_oob_ops) {
		kfree(aml_oob_ops);
		aml_oob_ops = NULL;
	}
	if (data_buf) {
		kfree(data_buf);
		data_buf = NULL;
	}
	return err;
}

static int aml_nand_write_env(struct mtd_info *mtd, loff_t offset, u_char *buf)
{
	struct env_oobinfo_t *env_oobinfo;
	int error = 0, err;
	loff_t addr = 0;
	size_t amount_saved = 0;
	size_t len;
	//struct mtd_oob_ops aml_oob_ops;
	
	struct mtd_oob_ops  * aml_oob_ops = NULL;
	unsigned char *data_buf;
	unsigned char env_oob_buf[sizeof(struct env_oobinfo_t)];

	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);

	aml_oob_ops = kzalloc(sizeof(struct mtd_oob_ops), GFP_KERNEL);
	if (aml_oob_ops == NULL){
		printk("%s %d no mem for aml_oob_ops\n", __func__, __LINE__);
		err = -ENOMEM;
		goto exit;
	}
	data_buf = kzalloc(mtd->writesize, GFP_KERNEL);
	if (data_buf == NULL){
		printk("%s %d no mem for data_buf\n", __func__, __LINE__);
		err = -ENOMEM;
		goto exit;
	}
	addr = offset;
	env_oobinfo = (struct env_oobinfo_t *)env_oob_buf;
	memcpy(env_oobinfo->name, ENV_NAND_MAGIC, 4);
	env_oobinfo->ec = aml_chip->aml_nandenv_info->env_valid_node->ec;
	env_oobinfo->timestamp = aml_chip->aml_nandenv_info->env_valid_node->timestamp;
	env_oobinfo->status_page = 1;

	while (amount_saved < CONFIG_ENV_SIZE ) {

		aml_oob_ops->mode = MTD_OOB_AUTO;
		aml_oob_ops->len = mtd->writesize;
		aml_oob_ops->ooblen = sizeof(struct env_oobinfo_t);
		aml_oob_ops->ooboffs = mtd->ecclayout->oobfree[0].offset;
		aml_oob_ops->datbuf = data_buf;
		aml_oob_ops->oobbuf = env_oob_buf;

		memset((unsigned char *)aml_oob_ops->datbuf, 0x0, mtd->writesize);
		len = min(mtd->writesize, CONFIG_ENV_SIZE - amount_saved);
		memcpy((unsigned char *)aml_oob_ops->datbuf, buf + amount_saved, len);

		error = mtd->write_oob(mtd, addr, aml_oob_ops);
		if (error) {
			printk("blk check good but write failed: %llx, %d\n", (uint64_t)addr, error);
					//return 1;
			err = 1;
			goto exit;
		}

		addr += mtd->writesize;;
		amount_saved += mtd->writesize;
	}
	if (amount_saved < CONFIG_ENV_SIZE){
				//return 1;
		err = 1;
		goto exit;
		}

	kfree(data_buf);
	kfree(aml_oob_ops);
	return 0;

exit:
	if (aml_oob_ops) {
		kfree(aml_oob_ops);
		aml_oob_ops = NULL;
	}
	if (data_buf) {
		kfree(data_buf);
		data_buf = NULL;
	}
	return err;	
}

static int aml_nand_save_env(struct mtd_info *mtd, u_char *buf)
{
	struct aml_nand_bbt_info *nand_bbt_info;
	struct env_free_node_t *env_free_node = NULL, *env_tmp_node;
	int error = 0, pages_per_blk, i = 1;
	loff_t addr = 0;
	//struct erase_info aml_env_erase_info;
	struct erase_info  *aml_env_erase_info; 
	env_t *env_ptr = (env_t *)buf;

	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	if (!aml_chip->aml_nandenv_info->env_init) 
		return 1;

	aml_env_erase_info = kzalloc(sizeof(struct erase_info), GFP_KERNEL);
	if (aml_env_erase_info == NULL){
		printk("%s %d no mem for aml_env_erase_info\n", __func__, __LINE__);
		error = -ENOMEM;
		goto exit;
	}

	pages_per_blk = mtd->erasesize / mtd->writesize;
	if ((mtd->writesize < CONFIG_ENV_SIZE) && (aml_chip->aml_nandenv_info->env_valid == 1))
		i = (CONFIG_ENV_SIZE + mtd->writesize - 1) / mtd->writesize;

	if (aml_chip->aml_nandenv_info->env_valid) {
		aml_chip->aml_nandenv_info->env_valid_node->phy_page_addr += i;
		if ((aml_chip->aml_nandenv_info->env_valid_node->phy_page_addr + i) > pages_per_blk) {

			env_free_node = kzalloc(sizeof(struct env_free_node_t), GFP_KERNEL);
			if (env_free_node == NULL){
				printk("%s %d no mem for env_free_node\n", __func__, __LINE__);
				error = -ENOMEM;
				goto exit;
			}


			env_free_node->phy_blk_addr = aml_chip->aml_nandenv_info->env_valid_node->phy_blk_addr;
			env_free_node->ec = aml_chip->aml_nandenv_info->env_valid_node->ec;
			env_tmp_node = aml_chip->aml_nandenv_info->env_free_node;
			while (env_tmp_node->next != NULL) {
				env_tmp_node = env_tmp_node->next;
			}
			env_tmp_node->next = env_free_node;

			env_tmp_node = aml_chip->aml_nandenv_info->env_free_node;
			aml_chip->aml_nandenv_info->env_valid_node->phy_blk_addr = env_tmp_node->phy_blk_addr;
			aml_chip->aml_nandenv_info->env_valid_node->phy_page_addr = 0;
			aml_chip->aml_nandenv_info->env_valid_node->ec = env_tmp_node->ec;
			aml_chip->aml_nandenv_info->env_valid_node->timestamp += 1;
			aml_chip->aml_nandenv_info->env_free_node = env_tmp_node->next;
			kfree(env_tmp_node);
		}
	}
	else {

		env_tmp_node = aml_chip->aml_nandenv_info->env_free_node;
		aml_chip->aml_nandenv_info->env_valid_node->phy_blk_addr = env_tmp_node->phy_blk_addr;
		aml_chip->aml_nandenv_info->env_valid_node->phy_page_addr = 0;
		aml_chip->aml_nandenv_info->env_valid_node->ec = env_tmp_node->ec;
		aml_chip->aml_nandenv_info->env_valid_node->timestamp += 1;
		aml_chip->aml_nandenv_info->env_free_node = env_tmp_node->next;
		kfree(env_tmp_node);
	}

	addr = aml_chip->aml_nandenv_info->env_valid_node->phy_blk_addr;
	addr *= mtd->erasesize;
	addr += aml_chip->aml_nandenv_info->env_valid_node->phy_page_addr * mtd->writesize;
	if (aml_chip->aml_nandenv_info->env_valid_node->phy_page_addr == 0) {

		memset(aml_env_erase_info, 0, sizeof(struct erase_info));
		aml_env_erase_info->mtd = mtd;
		aml_env_erase_info->addr = addr;
		aml_env_erase_info->len = mtd->erasesize;

		error = mtd->erase(mtd, aml_env_erase_info);
		if (error) {
			printk("env free blk erase failed %d\n", error);
			mtd->block_markbad(mtd, addr);
			//return error;
			goto exit;
		}
		aml_chip->aml_nandenv_info->env_valid_node->ec++;
	}

	nand_bbt_info = &aml_chip->aml_nandenv_info->nand_bbt_info;
	if ((!memcmp(nand_bbt_info->bbt_head_magic, BBT_HEAD_MAGIC, 4)) && (!memcmp(nand_bbt_info->bbt_tail_magic, BBT_TAIL_MAGIC, 4))) {
		memcpy(env_ptr->data + default_environment_size, aml_chip->aml_nandenv_info->nand_bbt_info.bbt_head_magic, sizeof(struct aml_nand_bbt_info));
		env_ptr->crc = (crc32((0 ^ 0xffffffffL), env_ptr->data, ENV_SIZE) ^ 0xffffffffL);
	}

	if (aml_nand_write_env(mtd, addr, (u_char *) env_ptr)) {
		printk("update nand env FAILED!\n");
		error = 1;
		goto exit;
	}
	
	kfree(aml_env_erase_info);
	return error;
exit:
	if (aml_env_erase_info) {
		kfree(aml_env_erase_info);
		aml_env_erase_info = NULL;
	}
	if (env_free_node) {
		kfree(env_free_node);
		env_free_node = NULL;
	}
	return error;
}

static int aml_nand_env_init(struct mtd_info *mtd)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	struct nand_chip *chip = &aml_chip->chip;
	struct env_oobinfo_t *env_oobinfo;
	struct env_free_node_t *env_free_node, *env_tmp_node, *env_prev_node;
	int error = 0, err, start_blk, total_blk, env_blk, i, j, pages_per_blk, bad_blk_cnt = 0, max_env_blk, phys_erase_shift;
	loff_t offset;
	unsigned char *data_buf = NULL;
	//struct mtd_oob_ops aml_oob_ops;
	
	struct mtd_oob_ops  *aml_oob_ops; 
	unsigned char env_oob_buf[sizeof(struct env_oobinfo_t)];

	aml_oob_ops = kzalloc(sizeof(struct mtd_oob_ops), GFP_KERNEL);
	if (aml_oob_ops == NULL){
		printk("%s %d no mem for aml_oob_ops \n", __func__, __LINE__);
		err = -ENOMEM;
		goto exit;
	}
	data_buf = kzalloc(mtd->writesize, GFP_KERNEL);
	if (data_buf == NULL){
		printk("%s %d no mem for data_buf \n", __func__, __LINE__);
		err = -ENOMEM;
		goto exit;
	}
	aml_chip->aml_nandenv_info = kzalloc(sizeof(struct aml_nandenv_info_t), GFP_KERNEL);
	if (aml_chip->aml_nandenv_info == NULL){
		printk("%s %d no mem for aml_chip->aml_nandenv_info \n", __func__, __LINE__);
		err = -ENOMEM;
		goto exit;
	}
	aml_chip->aml_nandenv_info->mtd = mtd;
	aml_chip->aml_nandenv_info->env_valid_node = kzalloc(sizeof(struct env_valid_node_t), GFP_KERNEL);
	if (aml_chip->aml_nandenv_info->env_valid_node == NULL){
		printk("%s %d no mem for aml_chip->aml_nandenv_info->env_valid_node \n", __func__, __LINE__);
		err = -ENOMEM;
		goto exit;
	}
	aml_chip->aml_nandenv_info->env_valid_node->phy_blk_addr = -1;

	phys_erase_shift = fls(mtd->erasesize) - 1;
	max_env_blk = (NAND_MINI_PART_SIZE >> phys_erase_shift);
	if (max_env_blk < 2)
		max_env_blk = 2;
	if (nand_boot_flag)
		offset = (1024 * mtd->writesize / aml_chip->plane_num);
	else {
		default_environment_size = 0;
		offset = 0;
	}
#ifdef NEW_NAND_SUPPORT
	if((aml_chip->new_nand_info.type) && (aml_chip->new_nand_info.type < 10))
		offset += RETRY_NAND_BLK_NUM* mtd->erasesize;
#endif
	start_blk = (int)(offset >> phys_erase_shift);
	total_blk = (int)(mtd->size >> phys_erase_shift);
	pages_per_blk = (1 << (chip->phys_erase_shift - chip->page_shift));
	env_oobinfo = (struct env_oobinfo_t *)env_oob_buf;
	if ((default_environment_size + sizeof(struct aml_nand_bbt_info)) > ENV_SIZE)
		total_blk = start_blk + max_env_blk;

	env_blk = 0;
	do {

		offset = mtd->erasesize;
		offset *= start_blk;
		error = mtd->block_isbad(mtd, offset);
		if (error) {
			for (j=0; j<MAX_BAD_BLK_NUM; j++) {
				if (aml_chip->aml_nandenv_info->nand_bbt_info.nand_bbt[j] == 0) {
					aml_chip->aml_nandenv_info->nand_bbt_info.nand_bbt[j] = start_blk;
					bad_blk_cnt++;
					break;
				}
			}
			//aml_chip->aml_nandenv_info->nand_bbt_info.nand_bbt[bad_blk_cnt++] = start_blk;
			continue;
		}

		aml_oob_ops->mode = MTD_OOB_AUTO;
		aml_oob_ops->len = mtd->writesize;
		aml_oob_ops->ooblen = sizeof(struct env_oobinfo_t);
		aml_oob_ops->ooboffs = mtd->ecclayout->oobfree[0].offset;
		aml_oob_ops->datbuf = data_buf;
		aml_oob_ops->oobbuf = env_oob_buf;

		memset((unsigned char *)aml_oob_ops->datbuf, 0x0, mtd->writesize);
		memset((unsigned char *)aml_oob_ops->oobbuf, 0x0, aml_oob_ops->ooblen);

		error = mtd->read_oob(mtd, offset, aml_oob_ops);
		if ((error != 0) && (error != -EUCLEAN)) {
			printk("blk check good but read failed: %llx, %d\n", (uint64_t)offset, error);
			continue;
		}

		aml_chip->aml_nandenv_info->env_init = 1;
		if (!memcmp(env_oobinfo->name, ENV_NAND_MAGIC, 4)) {
			aml_chip->aml_nandenv_info->env_valid = 1;
			if (aml_chip->aml_nandenv_info->env_valid_node->phy_blk_addr >= 0) {
				env_free_node = kzalloc(sizeof(struct env_free_node_t), GFP_KERNEL);
				if (env_free_node == NULL){
					printk("%s %d no mem for env_free_node\n", __func__, __LINE__);
					err = -ENOMEM;
					goto exit;
				}
				env_free_node->dirty_flag = 1;
				if (env_oobinfo->timestamp > aml_chip->aml_nandenv_info->env_valid_node->timestamp) {

					env_free_node->phy_blk_addr = aml_chip->aml_nandenv_info->env_valid_node->phy_blk_addr;
					env_free_node->ec = aml_chip->aml_nandenv_info->env_valid_node->ec;
					aml_chip->aml_nandenv_info->env_valid_node->phy_blk_addr = start_blk;
					aml_chip->aml_nandenv_info->env_valid_node->phy_page_addr = 0;
					aml_chip->aml_nandenv_info->env_valid_node->ec = env_oobinfo->ec;
					aml_chip->aml_nandenv_info->env_valid_node->timestamp = env_oobinfo->timestamp;	
				}
				else {
					env_free_node->phy_blk_addr = start_blk;
					env_free_node->ec = env_oobinfo->ec;
				}
				if (aml_chip->aml_nandenv_info->env_free_node == NULL)
					aml_chip->aml_nandenv_info->env_free_node = env_free_node;
				else {
					env_tmp_node = aml_chip->aml_nandenv_info->env_free_node;
					while (env_tmp_node->next != NULL) {
						env_tmp_node = env_tmp_node->next;
					}
					env_tmp_node->next = env_free_node;
				}
			}
			else {

				aml_chip->aml_nandenv_info->env_valid_node->phy_blk_addr = start_blk;
				aml_chip->aml_nandenv_info->env_valid_node->phy_page_addr = 0;
				aml_chip->aml_nandenv_info->env_valid_node->ec = env_oobinfo->ec;
				aml_chip->aml_nandenv_info->env_valid_node->timestamp = env_oobinfo->timestamp;	
			}
		}
		else if (env_blk < max_env_blk) {
			env_free_node = kzalloc(sizeof(struct env_free_node_t), GFP_KERNEL);
			if (env_free_node == NULL){
					printk("%s %d no mem for env_free_node\n", __func__, __LINE__);
					err = -ENOMEM;
					goto exit;
			}
			env_free_node->phy_blk_addr = start_blk;
			env_free_node->ec = env_oobinfo->ec;
			if (aml_chip->aml_nandenv_info->env_free_node == NULL)
				aml_chip->aml_nandenv_info->env_free_node = env_free_node;
			else {
				env_tmp_node = aml_chip->aml_nandenv_info->env_free_node;
				env_prev_node = env_tmp_node;
				while (env_tmp_node != NULL) {
					if (env_tmp_node->dirty_flag == 1)
						break;
					env_prev_node = env_tmp_node;
					env_tmp_node = env_tmp_node->next;
				}
				if (env_prev_node == env_tmp_node) {
					env_free_node->next = env_tmp_node;
					aml_chip->aml_nandenv_info->env_free_node = env_free_node;
				}
				else {
					env_prev_node->next = env_free_node;
					env_free_node->next = env_tmp_node;
				}
			}
		}
		env_blk++;
		if ((env_blk >= max_env_blk) && (aml_chip->aml_nandenv_info->env_valid == 1))
			break;

	} while ((++start_blk) < ENV_NAND_SCAN_BLK); //total_blk  ENV_NAND_SCAN_BLK
	if (start_blk >= ENV_NAND_SCAN_BLK) {  //total_blk  ENV_NAND_SCAN_BLK
		memcpy(aml_chip->aml_nandenv_info->nand_bbt_info.bbt_head_magic, BBT_HEAD_MAGIC, 4);
		memcpy(aml_chip->aml_nandenv_info->nand_bbt_info.bbt_tail_magic, BBT_TAIL_MAGIC, 4);
	}

	if (aml_chip->aml_nandenv_info->env_valid == 1) {

		aml_oob_ops->mode = MTD_OOB_AUTO;
		aml_oob_ops->len = mtd->writesize;
		aml_oob_ops->ooblen = sizeof(struct env_oobinfo_t);
		aml_oob_ops->ooboffs = mtd->ecclayout->oobfree[0].offset;
		aml_oob_ops->datbuf = data_buf;
		aml_oob_ops->oobbuf = env_oob_buf;

		for (i=0; i<pages_per_blk; i++) {

			memset((unsigned char *)aml_oob_ops->datbuf, 0x0, mtd->writesize);
			memset((unsigned char *)aml_oob_ops->oobbuf, 0x0, aml_oob_ops->ooblen);

			offset = aml_chip->aml_nandenv_info->env_valid_node->phy_blk_addr;
			offset *= mtd->erasesize;
			offset += i * mtd->writesize;
			error = mtd->read_oob(mtd, offset, aml_oob_ops);
			if ((error != 0) && (error != -EUCLEAN)) {
				printk("blk check good but read failed: %llx, %d\n", (uint64_t)offset, error);
				continue;
			}

			if (!memcmp(env_oobinfo->name, ENV_NAND_MAGIC, 4))
				aml_chip->aml_nandenv_info->env_valid_node->phy_page_addr = i;
			else
				break;
		}
	}
	if ((mtd->writesize < CONFIG_ENV_SIZE) && (aml_chip->aml_nandenv_info->env_valid == 1)) {
		i = (CONFIG_ENV_SIZE + mtd->writesize - 1) / mtd->writesize;
		aml_chip->aml_nandenv_info->env_valid_node->phy_page_addr -= (i - 1);
	}

	offset = aml_chip->aml_nandenv_info->env_valid_node->phy_blk_addr;
	offset *= mtd->erasesize;
	offset += aml_chip->aml_nandenv_info->env_valid_node->phy_page_addr * mtd->writesize;
	printk("aml nand env valid addr: %llx \n", (uint64_t)offset);
	printk(KERN_DEBUG "CONFIG_ENV_SIZE=0x%x; ENV_SIZE=0x%x; bbt=0x%x; default_environment_size=0x%x\n",
		CONFIG_ENV_SIZE, ENV_SIZE, sizeof(struct aml_nand_bbt_info), default_environment_size);
	kfree(data_buf);
	kfree(aml_oob_ops);
	return 0;
exit:
	if (data_buf) {
		kfree(data_buf);
		data_buf = NULL;
	}
	if (aml_oob_ops) {
		kfree(aml_oob_ops);
		aml_oob_ops = NULL;
	}
	if (aml_chip->aml_nandenv_info) {
		kfree(aml_chip->aml_nandenv_info);
		aml_chip->aml_nandenv_info = NULL;
	}
	if (aml_chip->aml_nandenv_info->env_valid_node ) {
		kfree(aml_chip->aml_nandenv_info->env_valid_node );
		aml_chip->aml_nandenv_info->env_valid_node = NULL;
	}
	if (env_free_node ) {
		kfree(env_free_node );
		env_free_node = NULL;
	}
	return err;

}

static int aml_nand_update_env(struct mtd_info *mtd)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	env_t *env_ptr;
	loff_t offset;
	int error = 0;

	env_ptr = kzalloc(sizeof(env_t), GFP_KERNEL);
	if (env_ptr == NULL)
		return -ENOMEM;

	if (aml_chip->aml_nandenv_info->env_valid == 1) {

		offset = aml_chip->aml_nandenv_info->env_valid_node->phy_blk_addr;
		offset *= mtd->erasesize;
		offset += aml_chip->aml_nandenv_info->env_valid_node->phy_page_addr * mtd->writesize;

		error = aml_nand_read_env (mtd, offset, (u_char *)env_ptr);
		if (error) {
			printk("nand env read failed: %llx, %d\n", (uint64_t)offset, error);
			return error;
		}

		error = aml_nand_save_env(mtd, (u_char *)env_ptr);
		if (error) {
			printk("update env bbt failed %d \n", error);
			return error;
		}
	}

	return error;
}

static int aml_nand_env_check(struct mtd_info *mtd)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	struct aml_nand_platform *plat = aml_chip->platform;
	struct platform_nand_chip *chip = &plat->platform_nand_data.chip;
	struct aml_nand_bbt_info *nand_bbt_info;
	struct aml_nand_part_info *aml_nand_part;
	struct mtd_partition *parts;
	env_t *env_ptr;
	int error = 0, start_blk, total_blk, i, j, nr, phys_erase_shift;
	loff_t offset;
	aml_chip->update_env_flag=0;
	error = aml_nand_env_init(mtd);
	if (error)
		return error;

	env_ptr = kzalloc(sizeof(env_t), GFP_KERNEL);
	if (env_ptr == NULL)
		return -ENOMEM;

	if (aml_chip->aml_nandenv_info->env_valid == 1) {

		offset = aml_chip->aml_nandenv_info->env_valid_node->phy_blk_addr;
		offset *= mtd->erasesize;
		offset += aml_chip->aml_nandenv_info->env_valid_node->phy_page_addr * mtd->writesize;

		error = aml_nand_read_env (mtd, offset, (u_char *)env_ptr);
		if (error) {
			printk("nand env read failed: %llx, %d\n", (uint64_t)offset, error);
			goto exit;
		}

		phys_erase_shift = fls(mtd->erasesize) - 1;
		offset = (1024 * mtd->writesize / aml_chip->plane_num);
#ifdef NEW_NAND_SUPPORT	
		if((aml_chip->new_nand_info.type) && (aml_chip->new_nand_info.type < 10))
			offset += RETRY_NAND_BLK_NUM* mtd->erasesize;
#endif			
		start_blk = (int)(offset >> phys_erase_shift);
		total_blk = (int)(mtd->size >> phys_erase_shift);
		nand_bbt_info = (struct aml_nand_bbt_info *)(env_ptr->data + default_environment_size);
		if ((!memcmp(nand_bbt_info->bbt_head_magic, BBT_HEAD_MAGIC, 4)) && (!memcmp(nand_bbt_info->bbt_tail_magic, BBT_TAIL_MAGIC, 4))) {
			for (i=start_blk; i<total_blk; i++) {
				aml_chip->block_status[i] = NAND_BLOCK_GOOD;
				for (j=0; j<MAX_BAD_BLK_NUM; j++) {
					if ((nand_bbt_info->nand_bbt[j] &0x7fff)== i) {    
					
						if((nand_bbt_info->nand_bbt[j] &0x8000)) {
						    aml_chip->block_status[i] = NAND_FACTORY_BAD;	
							//printk("aml_nand_env_check init the block_status factory bbt blk=%d,aml_chip->block_status[%d] =%d\n",i,i,aml_chip->block_status[i]);
						}
						else{
						    aml_chip->block_status[i] = NAND_BLOCK_BAD;
							//printk("aml_nand_env_check init the block_status bbt blk=%d\n",i);
						}
						break;
					}
				}
			}

			if (chip->set_parts)
				chip->set_parts(mtd->size, chip);
			aml_nand_part = nand_bbt_info->aml_nand_part;
			if (plat->platform_nand_data.chip.nr_partitions == 0) {
				parts = kzalloc((MAX_MTD_PART_NUM * sizeof(struct mtd_partition)), GFP_KERNEL);
				if (!parts) {
					error = -ENOMEM;
					goto exit;
				}
				plat->platform_nand_data.chip.partitions = parts;
				nr = 0;
				while(memcmp(aml_nand_part->mtd_part_magic, MTD_PART_MAGIC, 4) == 0) {
					parts->name = kzalloc(MAX_MTD_PART_NAME_LEN, GFP_KERNEL);
					if (!parts->name) {
						error = -ENOMEM;
						goto exit;
					}		
					strncpy(parts->name, aml_nand_part->mtd_part_name, MAX_MTD_PART_NAME_LEN);
					parts->offset = aml_nand_part->offset;
					parts->size = aml_nand_part->size;
					parts->mask_flags = aml_nand_part->mask_flags;
					nr++;
					parts++;
					aml_nand_part++;
				}
				plat->platform_nand_data.chip.nr_partitions = nr;
			}
			else {
				parts = plat->platform_nand_data.chip.partitions;
				nr = 0;
				if (strlen(parts->name) >= MAX_MTD_PART_NAME_LEN)
					parts->name[MAX_MTD_PART_NAME_LEN - 1] = '\0';
				while(memcmp(aml_nand_part->mtd_part_magic, MTD_PART_MAGIC, 4) == 0) {
					nr++;
					if (nr > plat->platform_nand_data.chip.nr_partitions) {
						aml_chip->update_env_flag  = 1;
						memset((unsigned char *)aml_nand_part, 0, sizeof(struct aml_nand_part_info));
						aml_nand_part++;
						continue;
					}

					if (strcmp(parts->name, aml_nand_part->mtd_part_name)) {
						printk("mtd parttion %d name %s changed to %s \n", nr, parts->name, aml_nand_part->mtd_part_name);
						aml_chip->update_env_flag = 1;
						strncpy(aml_nand_part->mtd_part_name, parts->name, MAX_MTD_PART_NAME_LEN);
					}
					if (parts->offset != aml_nand_part->offset) {
						printk("mtd parttion %d offset %llx changed to %llx \n", nr, aml_nand_part->offset, parts->offset);
						aml_chip->update_env_flag  = 1;
						aml_nand_part->offset = parts->offset;
					}
					if (parts->size != aml_nand_part->size) {
						printk("mtd parttion %d size %llx changed to %llx \n", nr, aml_nand_part->size, parts->size);
						aml_chip->update_env_flag  = 1;
						aml_nand_part->size = parts->size;
					}
					if (parts->mask_flags != aml_nand_part->mask_flags) {
						printk("mtd parttion %d mask_flags %x changed to %x \n", nr, aml_nand_part->mask_flags, parts->mask_flags);
						aml_chip->update_env_flag  = 1;
						aml_nand_part->mask_flags = parts->mask_flags;
					}

					parts++;
					aml_nand_part++;
				}
				if (nr < plat->platform_nand_data.chip.nr_partitions) {
					aml_chip->update_env_flag  = 1;
					for (i=nr; i<plat->platform_nand_data.chip.nr_partitions; i++) {
						parts = plat->platform_nand_data.chip.partitions + i;
						aml_nand_part = nand_bbt_info->aml_nand_part + i;
						memcpy(aml_nand_part->mtd_part_magic, MTD_PART_MAGIC, 4);
						strncpy(aml_nand_part->mtd_part_name, parts->name, MAX_MTD_PART_NAME_LEN);
						aml_nand_part->offset = parts->offset;
						aml_nand_part->size = parts->size;
						aml_nand_part->mask_flags = parts->mask_flags;
					}
				}
			}

			memcpy((unsigned char *)aml_chip->aml_nandenv_info->nand_bbt_info.bbt_head_magic, (unsigned char *)nand_bbt_info, sizeof(struct aml_nand_bbt_info));
		}
	}

	if (aml_chip->update_env_flag ) {
		error = aml_nand_save_env(mtd, (u_char *)env_ptr);
		if (error) {
			printk("nand env save failed: %d\n", error);
			goto exit;
		}
	}

exit:
	kfree(env_ptr);
	return 0;
}

static int aml_nand_scan_bbt(struct mtd_info *mtd)
{
	return 0;
}

#ifdef CONFIG_AML_NAND_ENV
static struct mtd_info *nand_env_mtd = NULL;
#define NAND_ENV_DEVICE_NAME	"nand_env"

static int nand_env_open(struct inode * inode, struct file * filp)
{
	return 0;
}
/*
 * This funcion reads the u-boot envionment variables. 
 * The f_pos points directly to the env location.
 */
static ssize_t nand_env_read(struct file *file, char __user *buf,
			size_t count, loff_t *ppos)
{
	env_t *env_ptr = NULL;
	ssize_t read_size;
	int error = 0,ret=0;
	if(*ppos == CONFIG_ENV_SIZE)
	{
		return 0;
	}

	if(*ppos >= CONFIG_ENV_SIZE)
	{
		printk(KERN_ERR "nand env: data access violation!\n");
		return -EFAULT;
	}

	env_ptr = kzalloc(sizeof(env_t), GFP_KERNEL);
	if (env_ptr == NULL)
	{
		return -ENOMEM;
	}
	
	error = aml_nand_read_env (nand_env_mtd, 0, (u_char *)env_ptr);

	if (error) 
	{
		printk("nand_env_read: nand env read failed: %llx, %d\n", (uint64_t)*ppos, error);
		kfree(env_ptr);
		return -EFAULT;
	}

	if((*ppos + count) > CONFIG_ENV_SIZE)
	{
		read_size = CONFIG_ENV_SIZE - *ppos;
	}
	else
	{
		read_size = count;
	}

	ret=copy_to_user(buf, (env_ptr + *ppos), read_size);

	*ppos += read_size;
	
	kfree(env_ptr);
	return read_size;
}

static ssize_t nand_env_write(struct file *file, const char __user *buf,
			 size_t count, loff_t *ppos)
{
	u_char *env_ptr = NULL;
	ssize_t write_size;
	int error = 0,ret;
	if(*ppos == CONFIG_ENV_SIZE)
	{
		return 0;
	}

	if(*ppos >= CONFIG_ENV_SIZE)
	{
		printk(KERN_ERR "nand env: data access violation!\n");
		return -EFAULT;
	}

	env_ptr = kzalloc(sizeof(env_t), GFP_KERNEL);
	if (env_ptr == NULL)
	{
		return -ENOMEM;
	}

	error = aml_nand_read_env (nand_env_mtd, 0, (u_char *)env_ptr);

	if (error) 
	{
		printk("nand_env_read: nand env read failed: %llx, %d\n", (uint64_t)*ppos, error);
		kfree(env_ptr);
		return -EFAULT;
	}

	if((*ppos + count) > CONFIG_ENV_SIZE)
	{
		write_size = CONFIG_ENV_SIZE - *ppos;
	}
	else
	{
		write_size = count;
	}

	ret=copy_from_user((env_ptr + *ppos), buf, write_size);

	error = aml_nand_save_env(nand_env_mtd, env_ptr);

	if (error) 
	{
		printk("nand_env_read: nand env read failed: %llx, %d\n", (uint64_t)*ppos, error);
		kfree(env_ptr);
		return -EFAULT;
	}

	*ppos += write_size;
	
	kfree(env_ptr);
	return write_size;
}

static int nand_env_close(struct inode *inode, struct file *file)
{
	return 0;
}

//static int nand_env_ioctl(struct inode *inode, struct file *file,
//		     u_int cmd, u_long arg)
//{
//	return 0;
//}

static int nand_env_cls_suspend(struct device *dev, pm_message_t state)
{
		return 0;
}

static int nand_env_cls_resume(struct device *dev)
{
	return 0;
}


static struct class nand_env_class = {
    
	.name = "nand_env",
	.owner = THIS_MODULE,
	.suspend = nand_env_cls_suspend,
	.resume = nand_env_cls_resume,
};

static struct file_operations nand_env_fops = {
    .owner	= THIS_MODULE,
    .open	= nand_env_open,
    .read	= nand_env_read,
    .write	= nand_env_write,
    .release	= nand_env_close,
//    .ioctl	= nand_env_ioctl,
};

#endif

static ssize_t show_nand_info(struct class *class, 
			struct class_attribute *attr,	char *buf)
{
    struct aml_nand_chip *aml_chip = container_of(class, struct aml_nand_chip, cls);

    printk("mfr_type:\t\t0x%x\n", aml_chip->mfr_type);
    printk("onfi_mode:\t\t0x%x\n", aml_chip->onfi_mode);
    printk("T_REA:\t\t0x%x\n", aml_chip->T_REA);
    printk("T_RHOH:\t\t0x%x\n", aml_chip->T_RHOH);
    printk("options:\t\t0x%x\n", aml_chip->options);
    printk("page_size:\t\t0x%x\n", aml_chip->page_size);
    printk("block_size:\t\t0x%x\n", aml_chip->block_size);
    printk("oob_size:\t\t0x%x\n", aml_chip->oob_size);
    printk("virtual_page_size:\t\t0x%x\n", aml_chip->virtual_page_size);
    printk("virtual_block_size:\t\t0x%x\n", aml_chip->virtual_block_size);
    printk("plane_num:\t\t0x%x\n", aml_chip->plane_num);
    printk("chip_num:\t\t0x%x\n", aml_chip->chip_num);
    printk("internal_chipnr:\t\t0x%x\n", aml_chip->internal_chipnr);
    printk("internal_page_nums:\t\t0x%x\n", aml_chip->internal_page_nums);
    printk("internal_chip_shift:\t\t0x%x\n", aml_chip->internal_chip_shift);
    printk("bch_mode:\t\t0x%x\n", aml_chip->bch_mode);
    printk("user_byte_mode:\t\t0x%x\n", aml_chip->user_byte_mode);
    printk("ops_mode:\t\t0x%x\n", aml_chip->ops_mode);
    printk("cached_prog_status:\t\t0x%x\n", aml_chip->cached_prog_status);
    printk("max_bch_mode:\t\t0x%x\n", aml_chip->max_bch_mode);

	return 0;
}


static ssize_t show_bbt_table(struct class *class, 
			struct class_attribute *attr,	char *buf)
{
    struct aml_nand_chip *aml_chip = container_of(class, struct aml_nand_chip, cls);
	struct mtd_info *mtd = &aml_chip->mtd;
    int start_blk, total_blk, i, phys_erase_shift;
    loff_t offset;

    phys_erase_shift = fls(mtd->erasesize) - 1;
	offset = (1024 * mtd->writesize / aml_chip->plane_num);
	start_blk = (int)(offset >> phys_erase_shift);
	total_blk = (int)(mtd->size >> phys_erase_shift);

    for (i = start_blk; i < total_blk; i++) {
		if(NAND_BLOCK_BAD == aml_chip->block_status[i])
            printk("block %d is a bad block\n", i);
	}

	return 0;
}

static ssize_t nand_page_dump(struct class *class, 
			struct class_attribute *attr,	const char *buf, size_t count)
{
    struct aml_nand_chip *aml_chip = container_of(class, struct aml_nand_chip, cls);
    struct mtd_info *mtd = &aml_chip->mtd;

    struct mtd_oob_ops ops;
    loff_t off;
    loff_t addr;
    u_char *datbuf, *oobbuf, *p;
    int ret, i;
    printk(KERN_DEBUG "enter %s\n", __FUNCTION__);
    ret = sscanf(buf, "%llx", &off);

	datbuf = kmalloc(mtd->writesize + mtd->oobsize, GFP_KERNEL);
	oobbuf = kmalloc(mtd->oobsize, GFP_KERNEL);
	if (!datbuf || !oobbuf) {
		printk("No memory for page buffer\n");
		return 1;
	}
	addr = ~((loff_t)mtd->writesize - 1);
	addr &= off;

	memset(&ops, 0, sizeof(ops));
	ops.datbuf = datbuf;
	ops.oobbuf = NULL; /* must exist, but oob data will be appended to ops.datbuf */
	ops.len = mtd->writesize;
	ops.ooblen = mtd->oobsize;
	ops.mode = MTD_OOB_RAW;
	i = mtd->read_oob(mtd, addr, &ops);
	if (i < 0) {
		printk("Error (%d) reading page %09llx\n", i, off);
		kfree(datbuf);
		kfree(oobbuf);
		return 1;
	}
	printk("Page %09llx dump,page size %d:\n", off,mtd->writesize);
	i = (mtd->writesize + mtd->oobsize) >> 4;
	p = datbuf;

	while (i--) {
		printk("\t%02x %02x %02x %02x %02x %02x %02x %02x"
		       "  %02x %02x %02x %02x %02x %02x %02x %02x\n",
		       p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
		       p[8], p[9], p[10], p[11], p[12], p[13], p[14],
		       p[15]);
		p += 16;
	}
	/*printf("OOB oob size %d:\n",nand->oobsize);
	i = nand->oobsize >> 3;
	while (i--) {
		printf("\t%02x %02x %02x %02x %02x %02x %02x %02x\n",
		       p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
		p += 8;
		}*/
	kfree(datbuf);
	kfree(oobbuf);
    printk(KERN_DEBUG "exit %s\n", __FUNCTION__);
    return count;
}

static ssize_t nand_page_read(struct class *class, 
			struct class_attribute *attr,	const char *buf, size_t count)
{
    struct aml_nand_chip *aml_chip = container_of(class, struct aml_nand_chip, cls);
	struct mtd_info *mtd = &aml_chip->mtd;
    loff_t off;
    loff_t addr;
    u_char *datbuf, *oobbuf, *p;
    size_t ret;
    int i;
    printk(KERN_DEBUG "enter %s\n", __FUNCTION__);
    ret = sscanf(buf, "%llx", &off);

    datbuf = kmalloc(mtd->writesize + mtd->oobsize, GFP_KERNEL);
	oobbuf = kmalloc(mtd->oobsize, GFP_KERNEL);
	if (!datbuf || !oobbuf) {
		printk("No memory for page buffer\n");
		return 1;
			}
	addr = ~((loff_t)mtd->writesize - 1);
	addr &= off;

    mtd->read(mtd, addr, mtd->writesize, &ret, datbuf);
    if (ret < 0) {
		printk("Error (%d) reading page %09llx\n", i, off);
		kfree(datbuf);
		kfree(oobbuf);
		return 1;
		}

    printk("Page %09llx read,page size %d:\n", off,mtd->writesize);
	i = (mtd->writesize ) >> 4;
	p = datbuf;

    while (i--) {
		printk("\t%02x %02x %02x %02x %02x %02x %02x %02x"
		       "  %02x %02x %02x %02x %02x %02x %02x %02x\n",
		       p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
		       p[8], p[9], p[10], p[11], p[12], p[13], p[14],
		       p[15]);
		p += 16;
			}

    kfree(datbuf);
	kfree(oobbuf);
    printk(KERN_DEBUG "exit %s\n", __FUNCTION__);
    return count;
	}

static struct class_attribute nand_class_attrs[] = {
    __ATTR(info,       S_IRUGO | S_IWUSR, show_nand_info,    NULL),
    __ATTR(bbt_table,       S_IRUGO | S_IWUSR, show_bbt_table,    NULL),    
    __ATTR(page_dump,  S_IRUGO | S_IWUSR, NULL,    nand_page_dump),
    __ATTR(page_read,  S_IRUGO | S_IWUSR, NULL,    nand_page_read),
    __ATTR(version,       S_IRUGO | S_IWUSR, show_nand_version_info,    NULL),

    __ATTR_NULL
};

int aml_nand_init(struct aml_nand_chip *aml_chip)
{
	struct aml_nand_platform *plat = aml_chip->platform;
	struct nand_chip *chip = &aml_chip->chip;
	struct mtd_info *mtd = &aml_chip->mtd;
	int err = 0, i = 0, phys_erase_shift;
	int oobmul  ;
	unsigned por_cfg, valid_chip_num = 0;
#ifdef CONFIG_AML_NAND_ENV
	int ret;
	struct device *devp;
	static dev_t nand_env_devno;
#endif
#ifdef CONFIG_HAS_EARLYSUSPEND
    aml_chip->nand_early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
    if (!aml_chip->nand_early_suspend.suspend)
    	aml_chip->nand_early_suspend.suspend = aml_platform_nand_suspend;
    if (!aml_chip->nand_early_suspend.resume)
    	aml_chip->nand_early_suspend.resume = aml_platform_nand_resume;
    aml_chip->nand_early_suspend.param = aml_chip;
    register_early_suspend(&aml_chip->nand_early_suspend);
#endif

	switch (plat->platform_nand_data.chip.options & NAND_ECC_OPTIONS_MASK) {

		case NAND_ECC_SOFT_MODE:
			chip->write_buf = aml_nand_dma_write_buf;
			chip->read_buf = aml_nand_dma_read_buf;
			chip->ecc.read_page_raw = aml_nand_read_page_raw;
			chip->ecc.write_page_raw = aml_nand_write_page_raw;
			chip->ecc.mode = NAND_ECC_SOFT;
			aml_chip->user_byte_mode = 1;
			aml_chip->bch_mode = 0;
			break;

		case NAND_ECC_BCH9_MODE:
			chip->write_buf = aml_nand_dma_write_buf;
			chip->read_buf = aml_nand_dma_read_buf;
			chip->block_bad = aml_nand_block_bad;
			chip->block_markbad = aml_nand_block_markbad;
			chip->ecc.mode = NAND_ECC_HW;
			chip->ecc.size = NAND_ECC_UNIT_SIZE;
			chip->ecc.bytes = NAND_BCH9_ECC_SIZE;
			chip->ecc.read_page_raw = aml_nand_read_page_raw;
			chip->ecc.write_page_raw = aml_nand_write_page_raw;
			chip->ecc.read_page = aml_nand_read_page_hwecc;
			chip->ecc.write_page = aml_nand_write_page_hwecc;
			chip->ecc.read_oob  = aml_nand_read_oob;
			chip->ecc.write_oob = aml_nand_write_oob;
			aml_chip->bch_mode = NAND_ECC_BCH9;
			aml_chip->user_byte_mode = 1;
			break;

		case NAND_ECC_BCH8_MODE:
			chip->write_buf = aml_nand_dma_write_buf;
			chip->read_buf = aml_nand_dma_read_buf;
			chip->block_bad = aml_nand_block_bad;
			chip->block_markbad = aml_nand_block_markbad;
			chip->ecc.mode = NAND_ECC_HW;
			chip->ecc.size = NAND_ECC_UNIT_SIZE;
			chip->ecc.bytes = NAND_BCH8_ECC_SIZE;
			chip->ecc.read_page_raw = aml_nand_read_page_raw;
			chip->ecc.write_page_raw = aml_nand_write_page_raw;
			chip->ecc.read_page = aml_nand_read_page_hwecc;
			chip->ecc.write_page = aml_nand_write_page_hwecc;
			chip->ecc.read_oob  = aml_nand_read_oob;
			chip->ecc.write_oob = aml_nand_write_oob;
			aml_chip->bch_mode = NAND_ECC_BCH8;
			aml_chip->user_byte_mode = 2;
			break;

		case NAND_ECC_BCH12_MODE:
			chip->write_buf = aml_nand_dma_write_buf;
			chip->read_buf = aml_nand_dma_read_buf;
			chip->block_bad = aml_nand_block_bad;
			chip->block_markbad = aml_nand_block_markbad;
			chip->ecc.mode = NAND_ECC_HW;
			chip->ecc.size = NAND_ECC_UNIT_SIZE;
			chip->ecc.bytes = NAND_BCH12_ECC_SIZE;
			chip->ecc.read_page_raw = aml_nand_read_page_raw;
			chip->ecc.write_page_raw = aml_nand_write_page_raw;
			chip->ecc.read_page = aml_nand_read_page_hwecc;
			chip->ecc.write_page = aml_nand_write_page_hwecc;
			chip->ecc.read_oob  = aml_nand_read_oob;
			chip->ecc.write_oob = aml_nand_write_oob;
			aml_chip->bch_mode = NAND_ECC_BCH12;
			aml_chip->user_byte_mode = 2;
			break;

		case NAND_ECC_BCH16_MODE:
			chip->write_buf = aml_nand_dma_write_buf;
			chip->read_buf = aml_nand_dma_read_buf;
			chip->block_bad = aml_nand_block_bad;
			chip->block_markbad = aml_nand_block_markbad;
			chip->ecc.mode = NAND_ECC_HW;
			chip->ecc.size = NAND_ECC_UNIT_SIZE;
			chip->ecc.bytes = NAND_BCH16_ECC_SIZE;
			chip->ecc.read_page_raw = aml_nand_read_page_raw;
			chip->ecc.write_page_raw = aml_nand_write_page_raw;
			chip->ecc.read_page = aml_nand_read_page_hwecc;
			chip->ecc.write_page = aml_nand_write_page_hwecc;
			chip->ecc.read_oob  = aml_nand_read_oob;
			chip->ecc.write_oob = aml_nand_write_oob;
			aml_chip->bch_mode = NAND_ECC_BCH16;
			aml_chip->user_byte_mode = 2;
			break;

		case NAND_ECC_BCH8_1K_MODE:
			chip->write_buf = aml_nand_dma_write_buf;
			chip->read_buf = aml_nand_dma_read_buf;
			chip->block_bad = aml_nand_block_bad;
			chip->block_markbad = aml_nand_block_markbad;
			chip->ecc.mode = NAND_ECC_HW;
			chip->ecc.size = NAND_ECC_UNIT_1KSIZE;
			chip->ecc.bytes = NAND_BCH8_1K_ECC_SIZE;
			chip->ecc.read_page_raw = aml_nand_read_page_raw;
			chip->ecc.write_page_raw = aml_nand_write_page_raw;
			chip->ecc.read_page = aml_nand_read_page_hwecc;
			chip->ecc.write_page = aml_nand_write_page_hwecc;
			chip->ecc.read_oob  = aml_nand_read_oob;
			chip->ecc.write_oob = aml_nand_write_oob;
			aml_chip->bch_mode = NAND_ECC_BCH8_1K;
			aml_chip->user_byte_mode = 2;
			break;

		case NAND_ECC_BCH16_1K_MODE:
			chip->write_buf = aml_nand_dma_write_buf;
			chip->read_buf = aml_nand_dma_read_buf;
			chip->block_bad = aml_nand_block_bad;
			chip->block_markbad = aml_nand_block_markbad;
			chip->ecc.mode = NAND_ECC_HW;
			chip->ecc.size = NAND_ECC_UNIT_1KSIZE;
			chip->ecc.bytes = NAND_BCH16_1K_ECC_SIZE;
			chip->ecc.read_page_raw = aml_nand_read_page_raw;
			chip->ecc.write_page_raw = aml_nand_write_page_raw;
			chip->ecc.read_page = aml_nand_read_page_hwecc;
			chip->ecc.write_page = aml_nand_write_page_hwecc;
			chip->ecc.read_oob  = aml_nand_read_oob;
			chip->ecc.write_oob = aml_nand_write_oob;
			aml_chip->bch_mode = NAND_ECC_BCH16_1K;
			aml_chip->user_byte_mode = 2;
			break;

		case NAND_ECC_BCH24_1K_MODE:
			chip->write_buf = aml_nand_dma_write_buf;
			chip->read_buf = aml_nand_dma_read_buf;
			chip->block_bad = aml_nand_block_bad;
			chip->block_markbad = aml_nand_block_markbad;
			chip->ecc.mode = NAND_ECC_HW;
			chip->ecc.size = NAND_ECC_UNIT_1KSIZE;
			chip->ecc.bytes = NAND_BCH24_1K_ECC_SIZE;
			chip->ecc.read_page_raw = aml_nand_read_page_raw;
			chip->ecc.write_page_raw = aml_nand_write_page_raw;
			chip->ecc.read_page = aml_nand_read_page_hwecc;
			chip->ecc.write_page = aml_nand_write_page_hwecc;
			chip->ecc.read_oob  = aml_nand_read_oob;
			chip->ecc.write_oob = aml_nand_write_oob;
			aml_chip->bch_mode = NAND_ECC_BCH24_1K;
			aml_chip->user_byte_mode = 2;
			break;

		case NAND_ECC_BCH30_1K_MODE:
			chip->write_buf = aml_nand_dma_write_buf;
			chip->read_buf = aml_nand_dma_read_buf;
			chip->block_bad = aml_nand_block_bad;
			chip->block_markbad = aml_nand_block_markbad;
			chip->ecc.mode = NAND_ECC_HW;
			chip->ecc.size = NAND_ECC_UNIT_1KSIZE;
			chip->ecc.bytes = NAND_BCH30_1K_ECC_SIZE;
			chip->ecc.read_page_raw = aml_nand_read_page_raw;
			chip->ecc.write_page_raw = aml_nand_write_page_raw;
			chip->ecc.read_page = aml_nand_read_page_hwecc;
			chip->ecc.write_page = aml_nand_write_page_hwecc;
			chip->ecc.read_oob  = aml_nand_read_oob;
			chip->ecc.write_oob = aml_nand_write_oob;
			aml_chip->bch_mode = NAND_ECC_BCH30_1K;
			aml_chip->user_byte_mode = 2;
			break;

		case NAND_ECC_BCH40_1K_MODE:
			chip->write_buf = aml_nand_dma_write_buf;
			chip->read_buf = aml_nand_dma_read_buf;
			chip->block_bad = aml_nand_block_bad;
			chip->block_markbad = aml_nand_block_markbad;
			chip->ecc.mode = NAND_ECC_HW;
			chip->ecc.size = NAND_ECC_UNIT_1KSIZE;
			chip->ecc.bytes = NAND_BCH40_1K_ECC_SIZE;
			chip->ecc.read_page_raw = aml_nand_read_page_raw;
			chip->ecc.write_page_raw = aml_nand_write_page_raw;
			chip->ecc.read_page = aml_nand_read_page_hwecc;
			chip->ecc.write_page = aml_nand_write_page_hwecc;
			chip->ecc.read_oob  = aml_nand_read_oob;
			chip->ecc.write_oob = aml_nand_write_oob;
			aml_chip->bch_mode = NAND_ECC_BCH40_1K;
			aml_chip->user_byte_mode = 2;
			break;

		case NAND_ECC_BCH60_1K_MODE:
			chip->write_buf = aml_nand_dma_write_buf;
			chip->read_buf = aml_nand_dma_read_buf;
			chip->block_bad = aml_nand_block_bad;
			chip->block_markbad = aml_nand_block_markbad;
			chip->ecc.mode = NAND_ECC_HW;
			chip->ecc.size = NAND_ECC_UNIT_1KSIZE;
			chip->ecc.bytes = NAND_BCH60_1K_ECC_SIZE;
			chip->ecc.read_page_raw = aml_nand_read_page_raw;
			chip->ecc.write_page_raw = aml_nand_write_page_raw;
			chip->ecc.read_page = aml_nand_read_page_hwecc;
			chip->ecc.write_page = aml_nand_write_page_hwecc;
			chip->ecc.read_oob  = aml_nand_read_oob;
			chip->ecc.write_oob = aml_nand_write_oob;
			aml_chip->bch_mode = NAND_ECC_BCH60_1K;
			aml_chip->user_byte_mode = 2;
			break;

		case NAND_ECC_SHORT_MODE:
			chip->write_buf = aml_nand_dma_write_buf;
			chip->read_buf = aml_nand_dma_read_buf;
			chip->block_bad = aml_nand_block_bad;
			chip->block_markbad = aml_nand_block_markbad;
			chip->ecc.mode = NAND_ECC_HW;
			chip->ecc.size = NAND_ECC_UNIT_SHORT;
			chip->ecc.bytes = NAND_BCH60_1K_ECC_SIZE;
			chip->ecc.read_page_raw = aml_nand_read_page_raw;
			chip->ecc.write_page_raw = aml_nand_write_page_raw;
			chip->ecc.read_page = aml_nand_read_page_hwecc;
			chip->ecc.write_page = aml_nand_write_page_hwecc;
			chip->ecc.read_oob  = aml_nand_read_oob;
			chip->ecc.write_oob = aml_nand_write_oob;
			aml_chip->bch_mode = NAND_ECC_BCH_SHORT;
			aml_chip->user_byte_mode = 2;
			break;

		default :
			printk(KERN_WARNING "haven`t found any ecc mode just selected NAND_ECC_NONE\n");
			chip->write_buf = aml_nand_dma_write_buf;
			chip->read_buf = aml_nand_dma_read_buf;
			chip->ecc.read_page_raw = aml_nand_read_page_raw;
			chip->ecc.write_page_raw = aml_nand_write_page_raw;
			chip->ecc.mode = NAND_ECC_NONE;
			aml_chip->user_byte_mode = 1;
			aml_chip->bch_mode = 0;
			break;
	}

	if (!aml_chip->aml_nand_hw_init)
		aml_chip->aml_nand_hw_init = aml_platform_hw_init;
	if (!aml_chip->aml_nand_adjust_timing)
		aml_chip->aml_nand_adjust_timing = aml_platform_adjust_timing;
	if (!aml_chip->aml_nand_options_confirm)
		aml_chip->aml_nand_options_confirm = aml_platform_options_confirm;
	if (!aml_chip->aml_nand_cmd_ctrl)
		aml_chip->aml_nand_cmd_ctrl = aml_platform_cmd_ctrl;
	if (!aml_chip->aml_nand_select_chip)
		aml_chip->aml_nand_select_chip = aml_platform_select_chip;
	if (!aml_chip->aml_nand_write_byte)
		aml_chip->aml_nand_write_byte = aml_platform_write_byte;
	if (!aml_chip->aml_nand_wait_devready)
		aml_chip->aml_nand_wait_devready = aml_platform_wait_devready;
	if (!aml_chip->aml_nand_get_user_byte)
		aml_chip->aml_nand_get_user_byte = aml_platform_get_user_byte;
	if (!aml_chip->aml_nand_set_user_byte)
		aml_chip->aml_nand_set_user_byte = aml_platform_set_user_byte;
	if (!aml_chip->aml_nand_command)
		aml_chip->aml_nand_command = aml_nand_base_command;
	if (!aml_chip->aml_nand_dma_read)
		aml_chip->aml_nand_dma_read = aml_platform_dma_read;
	if (!aml_chip->aml_nand_dma_write)
		aml_chip->aml_nand_dma_write = aml_platform_dma_write;
	if (!aml_chip->aml_nand_hwecc_correct)
		aml_chip->aml_nand_hwecc_correct = aml_platform_hwecc_correct;

	if (!chip->IO_ADDR_R)
		chip->IO_ADDR_R = (void __iomem *) CBUS_REG_ADDR(NAND_BUF);
	if (!chip->IO_ADDR_W)
		chip->IO_ADDR_W = (void __iomem *) CBUS_REG_ADDR(NAND_BUF);

	chip->options |=  NAND_SKIP_BBTSCAN;
	chip->options |= NAND_NO_SUBPAGE_WRITE;
	if (chip->ecc.mode != NAND_ECC_SOFT) {
		if (aml_chip->user_byte_mode == 2)
			chip->ecc.layout = &aml_nand_oob_64_2info;
		else
			chip->ecc.layout = &aml_nand_oob_64;
	}

	chip->select_chip = aml_nand_select_chip;
	chip->cmd_ctrl = aml_nand_cmd_ctrl;
	//chip->dev_ready = aml_nand_dev_ready;
	chip->verify_buf = aml_nand_verify_buf;
	chip->read_byte = aml_platform_read_byte;

	aml_chip->chip_num = plat->platform_nand_data.chip.nr_chips;
	if (aml_chip->chip_num > MAX_CHIP_NUM) {
		dev_err(aml_chip->device, "couldn`t support for so many chips\n");
		err = -ENXIO;
		goto exit_error;
	}
	for (i=0; i<aml_chip->chip_num; i++) {
		aml_chip->valid_chip[i] = 1;
		aml_chip->chip_enable[i] = (((plat->chip_enable_pad >> i*4) & 0xf) << 10);
		aml_chip->rb_enable[i] = (((plat->ready_busy_pad >> i*4) & 0xf) << 10);
	}
	//use NO RB mode to detect nand chip num
	aml_chip->ops_mode |= AML_CHIP_NONE_RB;
	chip->chip_delay = 100;

	aml_chip->aml_nand_hw_init(aml_chip);
	aml_chip->toggle_mode =0;

	if (nand_scan(mtd, aml_chip->chip_num) == -ENODEV) {
		chip->options = 0;
		chip->options |=  NAND_SKIP_BBTSCAN;
		chip->options |= NAND_NO_SUBPAGE_WRITE;
		if (aml_nand_scan(mtd, aml_chip->chip_num)) {
			err = -ENXIO;
			goto exit_error;
		}
	}
	else {
		for (i=1; i<aml_chip->chip_num; i++) {
			aml_chip->valid_chip[i] = 0;
		}
		aml_chip->options = NAND_DEFAULT_OPTIONS;
		aml_chip->page_size = mtd->writesize;
		aml_chip->block_size = mtd->erasesize;
		aml_chip->oob_size = mtd->oobsize;
		aml_chip->plane_num = 1;
		aml_chip->internal_chipnr = 1;
		chip->ecc.read_page_raw = aml_nand_read_page_raw;
		chip->ecc.write_page_raw = aml_nand_write_page_raw;
	}
	
	valid_chip_num = 0;
	for (i=0; i<aml_chip->chip_num; i++) {
		if (aml_chip->valid_chip[i]) {
		    valid_chip_num++;
		}
    	}
    	
    	//due to hardware limit, for chip num over 2, must use NO RB mode.
    	if(valid_chip_num > 2){
		printk("dect valid_chip_num:%d over 2, using NO RB mode\n", valid_chip_num);
    	}
    	else{
		
		if(aml_chip->rbpin_detect){
			por_cfg = READ_CBUS_REG(ASSIST_POR_CONFIG);		
			printk("%s auto detect RB pin here and por_cfg:%x\n", __func__, por_cfg);
			if(por_cfg&4){
				if(por_cfg&1){				
					printk("%s detect without RB pin here\n", __func__);
					aml_chip->rb_enable[0] = 0;
				}
				else{
					printk("%s detect with RB pin here\n", __func__);
				}
			}
			else{
				printk("%s power config ERROR and force using NO RB mode here\n", __func__);			
				aml_chip->rb_enable[0] = 0;
			}
		}	
		
		if (!aml_chip->rb_enable[0]) {
			aml_chip->ops_mode |= AML_CHIP_NONE_RB;
			chip->dev_ready = NULL;
			chip->chip_delay = 100;
			printk("#####%s, none RB and chip->chip_delay:%d\n", __func__, chip->chip_delay);
		}	
		else{
			chip->chip_delay = 20;
			chip->dev_ready = aml_nand_dev_ready;
			aml_chip->ops_mode &= ~AML_CHIP_NONE_RB;			
			printk("#####%s, with RB pins and chip->chip_delay:%d\n", __func__, chip->chip_delay);
		}		
	}	
	
	chip->scan_bbt = aml_nand_scan_bbt;

	mtd->suspend = aml_nand_suspend;
	mtd->resume = aml_nand_resume;

	if (aml_chip->aml_nand_adjust_timing)
		aml_chip->aml_nand_adjust_timing(aml_chip);

	if (chip->ecc.mode != NAND_ECC_SOFT) {
		if (aml_chip->aml_nand_options_confirm(aml_chip)) {
			err = -ENXIO;
			goto exit_error;
		}
	}
	mtd->writebufsize = mtd->writesize;
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON3
	switch(aml_chip->bch_mode){
		case NAND_ECC_BCH8:
		case NAND_ECC_BCH8_1K:
			aml_chip->ecc_cnt_limit = 6;
			aml_chip->ecc_max = 8;
			break;		
		case NAND_ECC_BCH9:
			aml_chip->ecc_cnt_limit = 6;
			aml_chip->ecc_max = 9;
			break;		
		case NAND_ECC_BCH12:
			aml_chip->ecc_cnt_limit = 6;
			aml_chip->ecc_max = 12;
			break;
		case NAND_ECC_BCH16:
		case NAND_ECC_BCH16_1K:
			aml_chip->ecc_cnt_limit = 13;
			aml_chip->ecc_max = 16;
			break;	
		case NAND_ECC_BCH24_1K:
			aml_chip->ecc_cnt_limit = 20;
			aml_chip->ecc_max = 24;
			break;		
		case NAND_ECC_BCH30_1K:
			aml_chip->ecc_cnt_limit = 25;
			aml_chip->ecc_max = 30;
			break;		
		case NAND_ECC_BCH40_1K:
			aml_chip->ecc_cnt_limit = 35;
			aml_chip->ecc_max = 40;
			break;		
		case NAND_ECC_BCH60_1K:
			aml_chip->ecc_cnt_limit = 50;
			aml_chip->ecc_max = 60;
			break;		

		default:
			aml_chip->ecc_cnt_limit = 9;
			aml_chip->ecc_max = 16;
			break;
	}
#endif
	if (plat->platform_nand_data.chip.ecclayout) {
		chip->ecc.layout = plat->platform_nand_data.chip.ecclayout;
	}
	else {
		oobmul =mtd->oobsize /aml_chip->oob_size ;
		if (!strncmp((char*)plat->name, NAND_BOOT_NAME, strlen((const char*)NAND_BOOT_NAME))) {
			chip->ecc.layout = &aml_nand_uboot_oob;
		}
		else if (chip->ecc.mode != NAND_ECC_SOFT) {
		/*	switch (mtd->oobsize) {*/
			switch (aml_chip->oob_size) {
				case 64:
					chip->ecc.layout = &aml_nand_oob_64_2info;
					break;
				case 128:
					chip->ecc.layout = &aml_nand_oob_128;
					break;
				case 218:
					chip->ecc.layout = &aml_nand_oob_218;
					break;
				case 224:
					chip->ecc.layout = &aml_nand_oob_224;
					break;
				case 256:
					chip->ecc.layout = &aml_nand_oob_256;
					break;
				case 376:
					chip->ecc.layout = &aml_nand_oob_376;
					break;
				case 436:
					chip->ecc.layout = &aml_nand_oob_436;
					break;				
				case 448:
					chip->ecc.layout = &aml_nand_oob_448;
					break;
				case 640:
					chip->ecc.layout = &aml_nand_oob_640;
					break;	
				case 744:
					chip->ecc.layout = &aml_nand_oob_744;
					break;			
				case 1280:
					chip->ecc.layout = &aml_nand_oob_1280;
					break;		
				default:
					printk("havn`t found any oob layout use nand base oob layout " "oobsize %d\n", mtd->oobsize);
					chip->ecc.layout = kzalloc(sizeof(struct nand_ecclayout), GFP_KERNEL);
					if (!chip->ecc.layout)
						chip->ecc.layout = &aml_nand_oob_64_2info;
					else
						chip->ecc.layout->oobfree[0].length = ((mtd->writesize / chip->ecc.size) * aml_chip->user_byte_mode);
					break;
			}
			chip->ecc.layout->eccbytes *= oobmul;
			chip->ecc.layout->oobfree[0].length *=oobmul;
		printk(" oob layout use nand base oob layout oobsize = %d,oobmul =%d,mtd->oobsize =%d,aml_chip->oob_size =%d\n", chip->ecc.layout->oobfree[0].length,oobmul,mtd->oobsize,aml_chip->oob_size);
		}
	}

	/*
	 * The number of bytes available for a client to place data into
	 * the out of band area
	 */
	chip->ecc.layout->oobavail = 0;
	for (i = 0; chip->ecc.layout->oobfree[i].length && i < ARRAY_SIZE(chip->ecc.layout->oobfree); i++)
		chip->ecc.layout->oobavail += chip->ecc.layout->oobfree[i].length;
	mtd->oobavail = chip->ecc.layout->oobavail;
	mtd->ecclayout = chip->ecc.layout;

	aml_chip->virtual_page_size = mtd->writesize;
	aml_chip->virtual_block_size = mtd->erasesize;

	aml_chip->aml_nand_data_buf = dma_alloc_coherent(aml_chip->device, (mtd->writesize + mtd->oobsize), &aml_chip->data_dma_addr, GFP_KERNEL);
	if (aml_chip->aml_nand_data_buf == NULL) {
		printk("no memory for flash data buf\n");
		err = -ENOMEM;
		goto exit_error;
	}

	aml_chip->user_info_buf = dma_alloc_coherent(aml_chip->device, (mtd->writesize / chip->ecc.size)*sizeof(int), &(aml_chip->nand_info_dma_addr), GFP_KERNEL);
	if (aml_chip->user_info_buf == NULL) {
		printk("no memory for flash info buf\n");
		err = -ENOMEM;
		goto exit_error;
	}

	if (chip->buffers)
		kfree(chip->buffers);
	if (mtd->oobsize >= NAND_MAX_OOBSIZE)
		chip->buffers = kzalloc((mtd->writesize + 3*mtd->oobsize), GFP_KERNEL);
	else
		chip->buffers = kzalloc((mtd->writesize + 3*NAND_MAX_OOBSIZE), GFP_KERNEL);
	if (chip->buffers == NULL) {
		printk("no memory for flash data buf\n");
		err = -ENOMEM;
		goto exit_error;
	}
	chip->oob_poi = chip->buffers->databuf + mtd->writesize;
	chip->options |= NAND_OWN_BUFFERS;
#ifdef NEW_NAND_SUPPORT
	if((aml_chip->new_nand_info.type) && (aml_chip->new_nand_info.type < 10)){
		aml_chip->new_nand_info.slc_program_info.get_default_value(mtd);
	}
#endif
	if (strncmp((char*)plat->name, NAND_BOOT_NAME, strlen((const char*)NAND_BOOT_NAME))) {
#ifdef NEW_NAND_SUPPORT
		if((aml_chip->new_nand_info.type) && (aml_chip->new_nand_info.type < 10)){
		
			aml_chip->new_nand_info.read_rety_info.get_default_value(mtd);
		}
		if((aml_chip->new_nand_info.type) 
		&& ((aml_chip->new_nand_info.type == SANDISK_19NM)||(aml_chip->new_nand_info.type == SANDISK_24NM))){
			aml_chip->new_nand_info.dynamic_read_info.dynamic_read_init(mtd);
		}//for sandisk
#endif
		phys_erase_shift = fls(mtd->erasesize) - 1;
		aml_chip->block_status = kzalloc((mtd->size >> phys_erase_shift), GFP_KERNEL);
		if (aml_chip->block_status == NULL) {
			printk("no memory for flash block status\n");
			err = -ENOMEM;
			goto exit_error;		
		}
		memset(aml_chip->block_status, 0xff, (mtd->size >> phys_erase_shift));

		err = aml_nand_env_check(mtd);
		if (err)
			printk("invalid nand env\n");

#ifdef CONFIG_AML_NAND_ENV
			pr_info("nand env: nand_env_probe. \n");

			nand_env_mtd = mtd;
			
			ret = alloc_chrdev_region(&nand_env_devno, 0, 1, NAND_ENV_DEVICE_NAME);
			
			if (ret < 0) {
				pr_err("nand_env: failed to allocate chrdev. \n");
				return 0;
			}
			/* connect the file operations with cdev */
			cdev_init(&aml_chip->nand_env_cdev, &nand_env_fops);
			aml_chip->nand_env_cdev.owner = THIS_MODULE;

			/* connect the major/minor number to the cdev */
			ret = cdev_add(&aml_chip->nand_env_cdev, nand_env_devno, 1);
			if (ret) {
				pr_err("nand env: failed to add device. \n");
				/* @todo do with error */
				return ret;
			}

			ret = class_register(&nand_env_class);
			if (ret < 0) {
				printk(KERN_NOTICE "class_register(&nand_env_class) failed!\n");
			}

			devp = device_create(&nand_env_class, NULL, nand_env_devno, NULL, "nand_env");
			if (IS_ERR(devp)) {
			 	printk(KERN_ERR "nand_env: failed to create device node\n");
			 	ret = PTR_ERR(devp);
	 		}
#endif

#ifdef CONFIG_AML_NAND_KEY
		int aml_key_init(struct aml_nand_chip *aml_chip);
        aml_key_init(aml_chip);
#endif
          /*setup class*/
    	aml_chip->cls.name = kzalloc(strlen((const char*)NAND_MULTI_NAME)+1, GFP_KERNEL);
        strcpy((char *)(aml_chip->cls.name), (const char*)NAND_MULTI_NAME);
        //sprintf(aml_chip->cls.name, NAND_MULTI_NAME);
        aml_chip->cls.class_attrs = nand_class_attrs;
       	err = class_register(&aml_chip->cls);
    	if(err)
    		printk(" class register nand_class fail!\n");
	}

	if (aml_nand_add_partition(aml_chip) != 0) {
		err = -ENXIO;
		goto exit_error;
	}

	dev_dbg(aml_chip->device, "initialized ok\n");
	return 0;

exit_error:
	unregister_early_suspend(&aml_chip->nand_early_suspend);
	if (aml_chip->user_info_buf) {
		dma_free_coherent(aml_chip->device, (mtd->writesize / chip->ecc.size)*sizeof(int), aml_chip->user_info_buf, (dma_addr_t)aml_chip->nand_info_dma_addr);
		aml_chip->user_info_buf = NULL;
	}
	if (chip->buffers) {
		kfree(chip->buffers);
		chip->buffers = NULL;
	}
	if (aml_chip->aml_nand_data_buf) {
		dma_free_coherent(aml_chip->device, (mtd->writesize + mtd->oobsize), aml_chip->aml_nand_data_buf, (dma_addr_t)aml_chip->data_dma_addr);
		aml_chip->aml_nand_data_buf = NULL;
	}
	if (aml_chip->block_status) {
		kfree(aml_chip->block_status);
		aml_chip->block_status = NULL;
	}

	return err;
}

#define DRV_NAME	"aml-nand"
#define DRV_VERSION	"1.1"
#define DRV_AUTHOR	"xiaojun_yoyo"
#define DRV_DESC	"Amlogic nand flash host controll driver for M1"

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRV_AUTHOR);
MODULE_DESCRIPTION(DRV_DESC);
MODULE_ALIAS("platform:" DRV_NAME);
