/*
 * MMC definitions
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __AML_MMC_H__
#define __AML_MMC_H__

#include <linux/types.h>
#include <linux/device.h>
#include <linux/mmc/host.h>

enum aml_mmc_waitfor {
	COMPLETION_NONE,
	COMPLETION_FINALIZE,
	COMPLETION_CMDSENT,
	COMPLETION_RSPFIN,
	COMPLETION_XFERFINISH,
	COMPLETION_XFERFINISH_RSPFIN,
};
struct aml_sd_host;
struct aml_mmc_platform_data {
	unsigned int	no_wprotect : 1;
	unsigned int	no_detect : 1;
	unsigned int	wprotect_invert : 1;
	unsigned int	detect_invert : 1;   /* set => detect active high. */
	unsigned int	use_dma : 1;

	unsigned int	gpio_detect;
	unsigned int	gpio_wprotect;
	unsigned long	ocr_avail;
	
	int   sdio_port; 
	unsigned int	max_width;
	unsigned int	host_caps;

	unsigned int f_min;
	unsigned int f_max;
	unsigned int clock;
	
	unsigned int max_blk_count;
	unsigned int max_blk_size;
	unsigned int max_req_size;
	unsigned int max_seg_size;
			
	int		(*sdio_init)(struct aml_sd_host * host);
	int		(*sdio_detect)(struct aml_sd_host * host);
	void	(*sdio_pwr_prepare)(unsigned port);
	void	(*sdio_pwr_on)(unsigned port);
	void	(*sdio_pwr_off)(unsigned port);

};

struct aml_sd_host {
	/* back-link to device */
	struct device *dev;
	struct aml_mmc_platform_data * pdata;

	struct mmc_host		*mmc;
	struct resource		*mem;
	struct clk		*clk;
	void __iomem		*base;
	int			irq;
	int			irq_cd;
	int			dma;

	unsigned long		clk_rate;

	int		  dodma;
	int			dmatogo;

	bool			irq_disabled;
	bool			irq_enabled;
	bool			irq_state;
	int			  sdio_irqen;

	struct  mmc_request	*mrq;
	int			cmd_is_stop;

	spinlock_t		        complete_lock;
	enum aml_mmc_waitfor	complete_what;

	int			dma_complete;

	u32			pio_sgptr;
	u32			pio_bytes;
	u32			pio_count;
	u32			*pio_ptr;
#define XFER_NONE 0
#define XFER_READ 1
#define XFER_WRITE 2
	u32			pio_active;

	int			bus_width;
	int     sdio_port;
	int     locked;
	char 		dbgmsg_cmd[301];
	char 		dbgmsg_dat[301];
	char		*status;

	unsigned int		ccnt, dcnt;
	
	struct tasklet_struct	pio_tasklet;

#ifdef CONFIG_DEBUG_FS
	struct dentry		*debug_root;
	struct dentry		*debug_state;
	struct dentry		*debug_regs;
#endif

#ifdef CONFIG_CPU_FREQ
	struct notifier_block	freq_transition;
#endif
	
};

#ifdef CONFIG_MMC_AML

typedef struct SDHW_CMD_Argument_Reg
{
	unsigned int arg;
} SDHW_CMD_Argument_Reg_t;

typedef struct SDHW_CMD_Send_Reg
{
	unsigned cmd_data: 8;							  // Bit 7:0
	unsigned cmd_res_bits: 8;						// Bit 15:8
	unsigned res_without_crc7: 1;				// Bit 16
	unsigned res_with_data: 1;					// Bit 17
	unsigned res_crc7_from_8: 1;				// Bit 18
	unsigned check_dat0_busy: 1;				// Bit 19
	unsigned cmd_send_data: 1;					// Bit 20
	unsigned use_int_window: 1;					// Bit 21
	unsigned reserved: 2;						    // Bit 22:23
	unsigned repeat_package_times: 8;		// Bit 31:24
} SDHW_CMD_Send_Reg_t;

typedef struct SDIO_Config_Reg
{
	unsigned cmd_clk_divide: 10;					// Bit 9:0
	unsigned cmd_disable_crc: 1;					// Bit 10
	unsigned cmd_out_at_posedge: 1;				// Bit 11
	unsigned cmd_argument_bits: 6;				// Bit 17:12
	unsigned res_latch_at_negedge: 1;			// Bit 18
	unsigned data_latch_at_negedge: 1;		// Bit 19
	unsigned bus_width: 1;							  // Bit 20
	unsigned m_endian: 2;							    // Bit 22:21
	unsigned write_Nwr: 6;							  // Bit 28:23
	unsigned write_crc_ok_status: 3;			// Bit 31:29
} SDIO_Config_Reg_t;

typedef struct SDIO_Status_IRQ_Reg
{
	unsigned status: 4;								    // Bit 3:0
	unsigned cmd_busy: 1;							    // Bit 4
	unsigned res_crc7_ok: 1;						  // Bit 5
	unsigned data_read_crc16_ok: 1;				// Bit 6
	unsigned data_write_crc16_ok: 1;			// Bit 7
	unsigned if_int: 1;								    // Bit 8
	unsigned cmd_int: 1;							    // Bit 9
	unsigned soft_int: 1;							    // Bit 10
	unsigned set_soft_int: 1;						  // Bit 11
	unsigned status_info: 4;						  // Bit 15:12
	unsigned timing_out_int: 1;						// Bit 16
	unsigned amrisc_timing_out_int_en:1;	// Bit 17
	unsigned arc_timing_out_int_en: 1;		// Bit 18
	unsigned timing_out_count: 13;				// Bit 31:19
} SDIO_Status_IRQ_Reg_t;

typedef struct SDHW_IRQ_Config_Reg
{
	unsigned amrisc_if_int_en: 1;					// Bit 0
	unsigned amrisc_cmd_int_en: 1;				// Bit 1
	unsigned amrisc_soft_int_en: 1;				// Bit 2
	unsigned arc_if_int_en: 1;						// Bit 3
	unsigned arc_cmd_int_en: 1;						// Bit 4
	unsigned arc_soft_int_en: 1;					// Bit 5
	unsigned if_int_config: 2;						// Bit 7:6
	unsigned data: 6;								      // Bit 13:8
	unsigned force_enable: 1;						  // Bit 14
	unsigned soft_reset: 1;							  // Bit 15
	unsigned force_output_en: 6;					// Bit 21:16
	unsigned diable_mem_halt: 2;					// Bit 23:22
	unsigned force_data_read: 6;					// Bit 29:24
	unsigned force_halt: 1;							  // Bit 30
	unsigned halt_hole:1;								  // Bit 31
} SDHW_IRQ_Config_Reg_t;

typedef struct MSHW_IRQ_Config_Reg
{
	unsigned amrisc_if_int_en: 1;					// Bit 0
	unsigned amrisc_cmd_int_en: 1;				// Bit 1
	unsigned amrisc_soft_int_en: 1;				// Bit 2
	unsigned arc_if_int_en: 1;						// Bit 3
	unsigned arc_cmd_int_en: 1;						// Bit 4
	unsigned arc_soft_int_en: 1;					// Bit 5
	unsigned if_int_config: 2;						// Bit 7:6
	unsigned data0: 1;								// Bit 8
	unsigned data1: 1;								// Bit 9
	unsigned data2: 1;								// Bit 10
	unsigned data3: 1;								// Bit 11
	unsigned bs: 1;									  // Bit 12
	unsigned sclk: 1;								  // Bit 13
	unsigned force_enable: 1;					// Bit 14
	unsigned soft_reset: 1;						// Bit 15
	unsigned force_output_en: 6;			// Bit 21:16
	unsigned diable_mem_halt: 2;			// Bit 23:22
	unsigned force_data_read: 6;			// Bit 29:24
	unsigned force_halt: 1;						// Bit 30
	unsigned halt_hole:1;							// Bit 31
} MSHW_IRQ_Config_Reg_t;

typedef struct SDIO_Multi_Config_Reg
{
	unsigned port_sel: 2;							// Bit 1:0
	unsigned ms_enable: 1;						// Bit 2
	unsigned ms_sclk_always: 1;				// Bit 3
	unsigned stream_enable: 1;				// Bit 4
	unsigned stream_8_bits_mode: 1;		// Bit 5
	unsigned data_catch_level: 2;			// Bit 7:6
	unsigned write_read_out_index: 1;	// Bit 8
	unsigned data_catch_out_en: 1;		// Bit 9
	unsigned reserved: 2;							// Bit 11:10
	unsigned res_read_index: 4;				// Bit 15:12
	unsigned data_catch_finish_point: 12;	// Bit 27:16
	unsigned reserved1: 4;							  // Bit 31:28
} SDIO_Multi_Config_Reg_t;

typedef struct SDIO_M_Addr_Reg
{
	unsigned int m_addr;
} SDIO_M_Addr_Reg_t;

typedef struct SDHW_Extension_Reg
{
	unsigned cmd_arg_ext: 16;						// Bit 15:0
	unsigned data_rw_number: 14;				// Bit 29:16
	unsigned data_rw_without_crc16: 1;	// Bit 30
	unsigned crc_status_4line: 1;				// Bit 31
} SDHW_Extension_Reg_t;                             	

//LSB -> MSB, structrue for SD Card Status
typedef struct _SD_Card_Status
{
	unsigned Reserved3: 2;
	unsigned Reserved4: 1;
	unsigned AKE_SEQ_ERROR: 1;                  //Error in the sequence of authentication process.
	unsigned Reserved5: 1;
	unsigned APP_CMD: 1;                        //The card will expect ACMD, or indication that the command has been interpreted as ACMD.
	unsigned NotUsed: 2;
	
	unsigned READY_FOR_DATA: 1;                 //Corresponds to buffer empty signalling on the bus.
	unsigned CURRENT_STATE: 4;                  //The state of the card when receiving the command. 
	unsigned ERASE_RESET: 1;                    //An erase sequence was cleared beforem executing because an out of erase sequence command was received.
	unsigned CARD_ECC_DISABLED: 1;              //The command has been executed without using the internal ECC.
	unsigned WP_ERASE_SKIP: 1;                  //Only partial address space was erased due to existing write protected blocks.
	
	unsigned CID_CSD_OVERWRITE: 1;              //Can be either one of the following errors:
	unsigned Reserved1: 1;
	unsigned Reserved2: 1;
	unsigned ERROR: 1;                          //A general or an unknown error occurred during the operation.
	unsigned CC_ERROR: 1;                       //Internal card controller error
	unsigned CARD_ECC_FAILED: 1;                //Card internal ECC was applied but failed to correct the data.
	unsigned ILLEGAL_COMMAND: 1;                //Command not legal for the card state
	unsigned COM_CRC_ERROR: 1;                  //The CRC check of the previous command failed.
	
  unsigned LOCK_UNLOCK_FAILED: 1;             //Set when a sequence or password error has been detected in lock/ unlock card command or if there was an attempt to access a locked card
	unsigned CARD_IS_LOCKED: 1;                 //When set, signals that the card is locked by the host
	unsigned WP_VIOLATION: 1;                   //Attempt to program a write-protected block.
	unsigned ERASE_PARAM: 1;                    //An invalid selection of write-blocks for erase occurred.
	unsigned ERASE_SEQ_ERROR: 1;                //An error in the sequence of erase commands occurred.
	unsigned BLOCK_LEN_ERROR: 1;                //The transferred block length is not allowed for this card, or the number of transferred bytes does not match the block length.
	unsigned ADDRESS_ERROR: 1;                  //A misaligned address that did not match the block length was used in the command.
	unsigned OUT_OF_RANGE: 1;                   //The command??s argument was out of the allowed range for this card.
	
} SD_Card_Status_t;

//structure for response
typedef struct _SD_Response_R1
{
	SD_Card_Status_t card_status;           //card status
} SD_Response_R1_t;

typedef enum _SD_Error_Status_t { 
	SD_NO_ERROR                 = 0,
	SD_ERROR_OUT_OF_RANGE,                  //Bit 31
	SD_ERROR_ADDRESS,                       //Bit 30 
	SD_ERROR_BLOCK_LEN,                     //Bit 29
	SD_ERROR_ERASE_SEQ,                     //Bit 28
	SD_ERROR_ERASE_PARAM,                   //Bit 27
	SD_ERROR_WP_VIOLATION,                  //Bit 26
	SD_ERROR_CARD_IS_LOCKED,                //Bit 25
	SD_ERROR_LOCK_UNLOCK_FAILED,            //Bit 24
	SD_ERROR_COM_CRC,                       //Bit 23
	SD_ERROR_ILLEGAL_COMMAND,               //Bit 22
	SD_ERROR_CARD_ECC_FAILED,               //Bit 21
	SD_ERROR_CC,                            //Bit 20
	SD_ERROR_GENERAL,                       //Bit 19
	SD_ERROR_Reserved1,                     //Bit 18
	SD_ERROR_Reserved2,                     //Bit 17
	SD_ERROR_CID_CSD_OVERWRITE,             //Bit 16
	SD_ERROR_AKE_SEQ,                       //Bit 03
	SD_ERROR_STATE_MISMATCH,
	SD_ERROR_HEADER_MISMATCH,
	SD_ERROR_DATA_CRC,
	SD_ERROR_TIMEOUT,  
	SD_ERROR_DRIVER_FAILURE,
	SD_ERROR_WRITE_PROTECTED,
	SD_ERROR_NO_MEMORY,
	SD_ERROR_SWITCH_FUNCTION_COMUNICATION,
	SD_ERROR_NO_FUNCTION_SWITCH,
	SD_ERROR_NO_CARD_INS
} SD_Error_Status_t;

typedef enum _SD_Bus_Width
{
	SD_BUS_SINGLE                   = 1,        //only DAT0
	SD_BUS_WIDE                     = 4         //use DAT0-4
} SD_Bus_Width_t;

// for CMD_SEND
#define     repeat_package_times_bit  24
#define     use_int_window_bit        21
#define     use_int_window_bit        21
#define     cmd_send_data_bit         20
#define     check_busy_on_dat0_bit    19
#define     response_crc7_from_8_bit  18
#define     response_have_data_bit 17
#define     response_do_not_have_crc7_bit 16
#define     cmd_response_bits_bit 8
#define     cmd_command_bit       0
// for SDIO_CONFIG
#define     sdio_write_CRC_ok_status_bit 29
#define     sdio_write_Nwr_bit        23
#define     m_endian_bit              21
#define     bus_width_bit             20   // 0 1-bit, 1-4bits
#define     data_latch_at_negedge_bit 19
#define     response_latch_at_negedge_bit 18
#define     cmd_argument_bits_bit 12
#define     cmd_out_at_posedge_bit 11
#define     cmd_disable_CRC_bit   10
#define     cmd_clk_divide_bit    0

// SDIO_STATUS_IRQ
#define     sdio_timing_out_count_bit   19
#define     arc_timing_out_int_en_bit   18
#define     amrisc_timing_out_int_en_bit 17
#define     sdio_timing_out_int_bit      16
#define     sdio_status_info_bit        12
#define     sdio_set_soft_int_bit       11
#define     sdio_soft_int_bit           10
#define     sdio_cmd_int_bit             9
#define     sdio_if_int_bit              8
#define     sdio_data_write_crc16_ok_bit 7
#define     sdio_data_read_crc16_ok_bit  6
#define     sdio_cmd_crc7_ok_bit  5
#define     sdio_cmd_busy_bit     4
#define     sdio_status_bit       0

// SDIO_IRQ_CONFIG
#define     force_halt_bit           30
#define     sdio_force_read_back_bit 24
#define     disable_mem_halt_bit     22
#define     sdio_force_output_en_bit 16
#define     soft_reset_bit           15
#define     sdio_force_enable_bit    14
#define     sdio_force_read_data_bit  8
#define     sdio_if_int_config_bit 6
#define     arc_soft_int_en_bit    5
#define     arc_cmd_int_en_bit     4
#define     arc_if_int_en_bit      3
#define     amrisc_soft_int_en_bit 2
#define     amrisc_cmd_int_en_bit  1
#define     amrisc_if_int_en_bit   0


// for SDIO_MULT_CONFIG
#define     data_catch_finish_point_bit 16
#define     response_read_index_bit     12
#define     data_catch_readout_en_bit    9
#define     write_read_out_index_bit     8
#define     data_catch_level_bit   6
#define     stream_8_bits_mode_bit 5
#define     stream_enable_bit      4
#define     ms_sclk_always_bit     3
#define     ms_enable_bit_bit      2
#define     SDIO_port_sel_bit      0


#define ERROR_NONE                  0
#define ERROR_GO_IDLE1              1
#define ERROR_GO_IDLE2              2
#define ERROR_APP55_1               3
#define ERROR_ACMD41                4
#define ERROR_APP55_2               5
#define ERROR_VOLTAGE_VALIDATION    6
#define ERROR_SEND_CID1             7
#define ERROR_SEND_RELATIVE_ADDR    8
#define ERROR_SEND_CID2             9
#define ERROR_SELECT_CARD           10
#define ERROR_APP55_RETRY3          11
#define ERROR_SEND_SCR              12
#define ERROR_READ_BLOCK            13
#define ERROR_STOP_TRANSMISSION     14
#define ERROR_MAGIC_WORDS           15
#define ERROR_CMD1                  16
#define ERROR_MMC_SWITCH_BUS        17
#define ERROR_COM_CRC               18
#define ERROR_DATA_CRC              19


#define CARD_TYPE_SD		0
#define CARD_TYPE_SDHC		1
#define CARD_TYPE_MMC		2
#define CARD_TYPE_EMMC		3

/* 250ms of timeout */
#define CLK_DIV                     250  /* (200/((31+1)*2)=0.390625MHz),this define is for SD_CLK in Card Identification Stage */

#define VOLTAGE_VALIDATION_RETRY    0x8000

#define TIMEOUT_SHORT       (250*1000)  /* 250ms */
#define TIMEOUT_DATA        (300*1000)  /* 300ms (TIMEOUT_SHORT+ (READ_SIZE*8)/10000000) */
#ifndef CONFIG_SDIO_BUFFER_SIZE
#define CONFIG_SDIO_BUFFER_SIZE 64*1024
#endif
#define NO_DELAY_DATA 0

//Misc definitions
#define MAX_RESPONSE_BYTES              18

#define RESPONSE_R1_R3_R6_R7_LENGTH     6
#define RESPONSE_R2_CID_CSD_LENGTH      17
#define RESPONSE_R4_R5_NONE_LENGTH      0

#define OCR_HCS           0x40000000

#define M3_SDIO_PORT_A 0
#define M3_SDIO_PORT_B 1
#define M3_SDIO_PORT_C 2
#define M3_SDIO_PORT_XC_A 3
#define M3_SDIO_PORT_XC_B 4
#define M3_SDIO_PORT_XC_C 5

#endif

#endif
