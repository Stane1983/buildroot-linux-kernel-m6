#ifndef __SDIO_INIT_H__
#define __SDIO_INIT_H__
#define FPGA_CLK_DIVIDE		50  //12
#define FPGA_SDIO_PORT   SDIO_PORT_A
//#define SDIO_IRQ
#define HAL_BEGIN_LOCK()
#define HAL_END_LOCK()
#define PRINT		printk

#include <linux/slab.h>
#include <linux/cardreader/cardreader.h>
#include <linux/cardreader/card_block.h>
#include <linux/cardreader/sdio_hw.h>
#include <mach/am_regs.h>
#include <mach/irqs.h>
#include <mach/card_io.h>
#include <mach/io.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/mii.h>
#include <linux/skbuff.h>
#include "amlreg.h"
#include "sdio_cmd.h"
#include "sdio_misc.h"

int sdio_read_ddr(unsigned long sdio_addr, unsigned long byte_count, unsigned char *data_buf);
int sdio_write_ddr(unsigned long sdio_addr, unsigned long byte_count, unsigned char *data_buf);
int sdio_read_sram(unsigned char *data_buf,unsigned long sdio_addr, unsigned long byte_count);
int sdio_write_sram(unsigned char *data_buf,unsigned long sdio_addr, unsigned long byte_count);
unsigned long  sdio_Read_SRAM32(unsigned long sram_addr );
void   sdio_Write_SRAM32(unsigned long sram_addr, unsigned long sramdata);
int     sdio_init(void);
int     sdio_exit(void);
#endif //__SDIO_INIT_H__
