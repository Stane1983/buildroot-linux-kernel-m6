#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>

#include <linux/spi/spi.h>
#include <mach/spi.h>
#include <mach/pinmux.h>

#define AML_SPI_DBG 0

#ifdef AML_SPI_DBG
#define spi_dbg     printk("enter %s line:%d \n", __func__, __LINE__)
#else
#define spi_dbg
#endif


#define SPICC_FIFO_SIZE 16

#define SPICC_DMA  0
#define SPICC_PIO  1

struct spicc_conreg {
  unsigned int enable       :1;
  unsigned int mode         :1;
  unsigned int xch          :1;
  unsigned int smc          :1;
  unsigned int clk_pol      :1;
  unsigned int clk_pha      :1;
  unsigned int ss_ctl       :1;
  unsigned int ss_pol       :1;
  unsigned int drctl        :2;
  unsigned int rsv1         :2;
  unsigned int chip_select  :2;
  unsigned int rsv2         :2;
  unsigned int data_rate_div :3;
  unsigned int bits_per_word :6;
  unsigned int burst_len    :7;
};

struct spicc_intreg {
  unsigned int tx_empty_en  :1;
  unsigned int tx_half_en   :1;
  unsigned int tx_full_en   :1;
  unsigned int rx_ready_en  :1;
  unsigned int rx_half_en   :1;
  unsigned int rx_full_en   :1;
  unsigned int rx_of_en     :1;
  unsigned int xfer_com_en  :1;
  unsigned int rsv1         :24;
};

struct spicc_dmareg {
  unsigned int en           :1;
  unsigned int tx_fifo_th   :5;
  unsigned int rx_fifo_th   :5;
  unsigned int num_rd_burst :4;
  unsigned int num_wr_burst :4;
  unsigned int urgent       :1;
  unsigned int thread_id    :6;
  unsigned int burst_num    :6;
};

struct spicc_statreg {
  unsigned int tx_empty     :1;
  unsigned int tx_half      :1;
  unsigned int tx_full      :1;
  unsigned int rx_ready     :1;
  unsigned int rx_half      :1;
  unsigned int rx_full      :1;
  unsigned int rx_of        :1;
  unsigned int xfer_com     :1;
  unsigned int rsv1         :24;
};

struct aml_spi_reg_master {
	volatile unsigned int rxdata;
	volatile unsigned int txdata;
	volatile struct spicc_conreg conreg;
	volatile struct spicc_intreg intreg;
	volatile struct spicc_dmareg dmareg;
	volatile struct spicc_statreg statreg;
	volatile unsigned int periodreg;
	volatile unsigned int testreg;
	volatile unsigned int draddr;
	volatile unsigned int dwaddr;
};

struct amlogic_spi {
	spinlock_t		    lock;
	struct list_head	msg_queue;
	//struct completion   done;
	struct workqueue_struct *workqueue;
	struct work_struct work;	
	int			        irq;
	
		
	struct spi_master	*master;
	struct spi_device	spi_dev;
	struct spi_device   *tgl_spi;
	
	
	struct  aml_spi_reg_master __iomem*  master_regs;
	

	unsigned        master_no;

	unsigned        io_pad_type;
	unsigned char   wp_pin_enable;
	unsigned char   hold_pin_enable;
	unsigned        num_cs;
	unsigned        cur_mode;
	unsigned        cur_bpw;
	unsigned        cur_speed;

};


