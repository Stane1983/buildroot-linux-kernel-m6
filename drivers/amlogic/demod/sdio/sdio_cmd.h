#ifndef _H_SD_PROTOCOL
#define _H_SD_PROTOCOL

#include <linux/slab.h>
#include <linux/cardreader/cardreader.h>
#include <linux/cardreader/card_block.h>
#include <linux/cardreader/sdio_hw.h>

#include <mach/am_regs.h>
#include <mach/irqs.h>
#include <mach/card_io.h>





#define SDIO_PORT_MAX   5
#define SDIO_PORT_A    0
#define SDIO_PORT_B    1
#define SDIO_PORT_C    2
#define SDIO_PORT_B1   5
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


#define SDIO_WRITE_CMD_LEN	6
#define SDIO_WRITE_CMD_DATA_LEN	6

#define SDIO_READ_CMD_FLAG	(0<<12)
#define SDIO_WRITE_CMD_FLAG (1<<12)
#define SDIO_DDR_EN 		(1<<13)
#define SDIO_SRAM_EN 		(1<<14)
typedef struct SDIO_WR_CMD {
        unsigned int  addr;
        unsigned short  len_flag;
} __attribute__ ((packed)) SDIO_WR_CMD;







///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////



//Never change any sequence of following data variables
#pragma pack(1)

//MSB->LSB, structure for Operation Conditions Register
typedef struct _SD_REG_OCR {

        unsigned Reserved0:6;
        unsigned Card_Capacity_Status:1;	//Card_High_capacity
        unsigned Card_Busy:1;	//Card power up status bit (busy)

        unsigned VDD_28_29:1;	/* VDD voltage 2.8 ~ 2.9 */
        unsigned VDD_29_30:1;	/* VDD voltage 2.9 ~ 3.0 */
        unsigned VDD_30_31:1;	/* VDD voltage 3.0 ~ 3.1 */
        unsigned VDD_31_32:1;	/* VDD voltage 3.1 ~ 3.2 */
        unsigned VDD_32_33:1;	/* VDD voltage 3.2 ~ 3.3 */
        unsigned VDD_33_34:1;	/* VDD voltage 3.3 ~ 3.4 */
        unsigned VDD_34_35:1;	/* VDD voltage 3.4 ~ 3.5 */
        unsigned VDD_35_36:1;	/* VDD voltage 3.5 ~ 3.6 */

        unsigned VDD_20_21:1;	/* VDD voltage 2.0 ~ 2.1 */
        unsigned VDD_21_22:1;	/* VDD voltage 2.1 ~ 2.2 */
        unsigned VDD_22_23:1;	/* VDD voltage 2.2 ~ 2.3 */
        unsigned VDD_23_24:1;	/* VDD voltage 2.3 ~ 2.4 */
        unsigned VDD_24_25:1;	/* VDD voltage 2.4 ~ 2.5 */
        unsigned VDD_25_26:1;	/* VDD voltage 2.5 ~ 2.6 */
        unsigned VDD_26_27:1;	/* VDD voltage 2.6 ~ 2.7 */
        unsigned VDD_27_28:1;	/* VDD voltage 2.7 ~ 2.8 */

        unsigned Reserved4:1;	/* MMC VDD voltage 1.45 ~ 1.50 */
        unsigned Reserved3:1;	/* MMC VDD voltage 1.50 ~ 1.55 */
        unsigned Reserved2:1;	/* MMC VDD voltage 1.55 ~ 1.60 */
        unsigned Reserved1:1;	/* MMC VDD voltage 1.60 ~ 1.65 */
        unsigned VDD_16_17:1;	/* VDD voltage 1.6 ~ 1.7 */
        unsigned VDD_17_18:1;	/* VDD voltage 1.7 ~ 1.8 */
        unsigned VDD_18_19:1;	/* VDD voltage 1.8 ~ 1.9 */
        unsigned VDD_19_20:1;	/* VDD voltage 1.9 ~ 2.0 */
} SD_REG_OCR_t;


typedef struct _SDIO_REG_OCR {

        unsigned VDD_28_29:1;	/* VDD voltage 2.8 ~ 2.9 */
        unsigned VDD_29_30:1;	/* VDD voltage 2.9 ~ 3.0 */
        unsigned VDD_30_31:1;	/* VDD voltage 3.0 ~ 3.1 */
        unsigned VDD_31_32:1;	/* VDD voltage 3.1 ~ 3.2 */
        unsigned VDD_32_33:1;	/* VDD voltage 3.2 ~ 3.3 */
        unsigned VDD_33_34:1;	/* VDD voltage 3.3 ~ 3.4 */
        unsigned VDD_34_35:1;	/* VDD voltage 3.4 ~ 3.5 */
        unsigned VDD_35_36:1;	/* VDD voltage 3.5 ~ 3.6 */

        unsigned VDD_20_21:1;	/* VDD voltage 2.0 ~ 2.1 */
        unsigned VDD_21_22:1;	/* VDD voltage 2.1 ~ 2.2 */
        unsigned VDD_22_23:1;	/* VDD voltage 2.2 ~ 2.3 */
        unsigned VDD_23_24:1;	/* VDD voltage 2.3 ~ 2.4 */
        unsigned VDD_24_25:1;	/* VDD voltage 2.4 ~ 2.5 */
        unsigned VDD_25_26:1;	/* VDD voltage 2.5 ~ 2.6 */
        unsigned VDD_26_27:1;	/* VDD voltage 2.6 ~ 2.7 */
        unsigned VDD_27_28:1;	/* VDD voltage 2.7 ~ 2.8 */

        unsigned Reserved:8;	/* reserved */
} SDIO_REG_OCR_t;

//MSB->LSB, structure for Card_Identification Register
typedef struct _SD_REG_CID {

        unsigned char MID;	//Manufacturer ID
        char OID[2];		//OEM/Application ID
        char PNM[5];		//Product Name
        unsigned char PRV;	//Product Revision
        unsigned long PSN;	//Serial Number
        unsigned MDT_high:4;	//Manufacture Date Code
        unsigned Reserved:4;
        unsigned MDT_low:8;	// MDT = (MDT_high << 8) | MDT_low
        unsigned NotUsed:1;
        unsigned CRC:7;	//CRC7 checksum
} SD_REG_CID_t;

//MSB->LSB, structure for Card-Specific Data Register
typedef struct _SD_REG_CSD {

        unsigned Reserved1:2;
        unsigned MMC_SPEC_VERS:4;	//MMC Spec_vers
        unsigned CSD_STRUCTURE:2;	//CSD structure

        unsigned TAAC:8;	//data read access-time-1
        unsigned NSAC:8;	//data read access-time-2 in CLK cycles(NSAC*100)
        unsigned TRAN_SPEED:8;	//max. data transfer rate
        unsigned CCC_high:8;	//card command classes

        unsigned READ_BL_LEN:4;	//max. read data block length
        unsigned CCC_low:4;	// CCC = (CCC_high << 4) | CCC_low

        unsigned C_SIZE_high:2;	//device size
        unsigned Reserved2:2;
        unsigned DSR_IMP:1;	//DSR implemented
        unsigned READ_BLK_MISALIGN:1;	//read block misalignment
        unsigned WRITE_BLK_MISALIGN:1;	//write block misalignment
        unsigned READ_BL_PARTIAL:1;	//partial blocks for read allowed

        unsigned C_SIZE_mid:8;

        unsigned VDD_R_CURR_MAX:3;	//max. read current @VDD max
        unsigned VDD_R_CURR_MIN:3;	//max. read current @VDD min
        unsigned C_SIZE_low:2;	// C_SIZE = (C_SIZE_high << 10) | (C_SIZE_mid << 2) | C_SIZE_low

        unsigned C_SIZE_MULT_high:2;
        unsigned VDD_W_CURR_MAX:3;	//max. write current @VDD max
        unsigned VDD_W_CURR_MIN:3;	//max. write current @VDD min

        unsigned SECTOR_SIZE_high:6;	//erase sector size
        unsigned ERASE_BLK_EN:1;	//erase single block enable
        unsigned C_SIZE_MULT_low:1;	// C_SIZE_MULT = (C_SIZE_MULT_high << 1) | C_SIZE_MULT_low

        unsigned WP_GRP_SIZE:7;	//write protect group size
        unsigned SECTOR_SIZE_low:1;	// SECTOR_SIZE = (SECTOR_SIZE_high << 1) | SECTOR_SIZE_low

        unsigned WRITE_BL_LEN_high:2;	//max. write data block length
        unsigned R2W_FACTOR:3;	//write speed factor
        unsigned Reserved3:2;

        unsigned WP_GRP_ENABLE:1;	//write protect group enable
        unsigned Reserved4:5;
        unsigned WRITE_BL_PARTIAL:1;	//partial blocks for write allowed
        unsigned WRITE_BL_LEN_low:2;	//WRITE_BL_LEN = (WRITE_BL_LEN_high << 2) | WRITE_BL_LEN_low

        unsigned Reserved5:2;
        unsigned FILE_FORMAT:2;	//File format
        unsigned TMP_WRITE_PROTECT:1;	//temporary write protection
        unsigned PERM_WRITE_PROTECT:1;	//permanent write protection
        unsigned COPY:1;	//copy flag (OTP)
        unsigned FILE_FORMAT_GRP:1;	//File format group

        unsigned NotUsed:1;
        unsigned CRC:7;	//CRC checksum
} SD_REG_CSD_t;


typedef struct _SDHC_REG_CSD {

        unsigned Reserved1:6;
        unsigned CSD_STRUCTURE:2;	//CSD structure

        unsigned TAAC:8;	//data read access-time-1
        unsigned NSAC:8;	//data read access-time-2 in CLK cycles(NSAC*100)
        unsigned TRAN_SPEED:8;	//max. data transfer rate

        unsigned CCC_high:8;	//card command classes
        unsigned READ_BL_LEN:4;	//max. read data block length
        unsigned CCC_low:4;	// CCC = (CCC_high << 4) | CCC_low

        unsigned Reserved2:4;
        unsigned DSR_IMP:1;	//DSR implemented
        unsigned READ_BLK_MISALIGN:1;	//read block misalignment
        unsigned WRITE_BLK_MISALIGN:1;	//write block misalignment
        unsigned READ_BL_PARTIAL:1;	//partial blocks for read allowed

        unsigned C_SIZE_high:6;	//device size
        unsigned Reserved3:2;

        unsigned C_SIZE_mid:8;
        unsigned C_SIZE_low:8;

        unsigned SECTOR_SIZE_high:6;	//erase sector size
        unsigned ERASE_BLK_EN:1;	//erase single block enable
        unsigned Reserved4:1;

        unsigned WP_GRP_SIZE:7;	//write protect group size
        unsigned SECTOR_SIZE_low:1;	// SECTOR_SIZE = (SECTOR_SIZE_high << 1) | SECTOR_SIZE_low

        unsigned WRITE_BL_LEN_high:2;	//max. write data block length
        unsigned R2W_FACTOR:3;	//write speed factor
        unsigned Reserved5:2;
        unsigned WP_GRP_ENABLE:1;	//write protect group enable

        unsigned Reserved6:5;
        unsigned WRITE_BL_PARTIAL:1;	//partial blocks for write allowed
        unsigned WRITE_BL_LEN_low:2;	//WRITE_BL_LEN = (WRITE_BL_LEN_high << 2) | WRITE_BL_LEN_low
        unsigned Reserved7:2;

        unsigned FILE_FORMAT:2;	//File format
        unsigned TMP_WRITE_PROTECT:1;	//temporary write protection
        unsigned PERM_WRITE_PROTECT:1;	//permanent write protection
        unsigned COPY:1;	//copy flag (OTP)
        unsigned FILE_FORMAT_GRP:1;	//File format group

        unsigned NotUsed:1;
        unsigned CRC:7;	//CRC checksum
} SDHC_REG_CSD_t;

/*typedef struct _MMC_REG_EXT_CSD
{
    unsigned char Reserved1[7];
    unsigned char S_CMD_SET;
    unsigned char Reserved2[300];
    unsigned char PWR_CL_26_360;
    unsigned char PWR_CL_52_360;
    unsigned char PWR_CL_26_195;
    unsigned char PWR_CL_52_195;
    unsigned char Reserved3[3];
    unsigned char CARD_TYPE;
    unsigned char Reserved4;
    unsigned char CSD_STRUCTURE;
    unsigned char Reserved5;
    unsigned char EXT_CSD_REV;
    unsigned char CMD_SET;
    unsigned char Reserved6;
    unsigned char CMD_SET_REV;
    unsigned char Reserved7;
    unsigned char POWER_CLASS;
    unsigned char Reserved8;
    unsigned char HS_TIMING;
    unsigned char Reserved9;
    unsigned char BUS_WIDTH;
    unsigned char Reserved10[183];
            } MMC_REG_EXT_CSD_t;*/// reserved for future use

//MSB->LSB, structure for SD CARD Configuration Register
typedef struct _SD_REG_SCR {

        unsigned SD_SPEC:4;	//SD Card¡ªSpec. Version
        unsigned SCR_STRUCTURE:4;	//SCR Structure

        unsigned SD_BUS_WIDTHS:4;	//DAT Bus widths supported
        unsigned SD_SECURITY:3;	//SD Security Support
        unsigned DATA_STAT_AFTER_ERASE:1;	//data_status_after erases

        unsigned Reserved1:16;	//for alignment
        unsigned long Reserved2;
} SD_REG_SCR_t;


typedef struct _SDIO_REG_CCCR {

        unsigned char CCCR_SDIO_SPEC;	//cccr and sdio spec verion low four bits(cccr) high four bits(sdio)
        unsigned char SD_SPEC;	//SD Card¡ªSpec. Version low four bits
        unsigned char IO_ENABLE;	//
        unsigned char IO_READY;	//
        unsigned char INT_ENABLE;	//
        unsigned char INT_PENDING;
        unsigned char INT_ABORT;
        unsigned char BUS_Interface_Control;
        unsigned char Card_Capability;
        unsigned short Common_CIS_Pointer1;
        unsigned char Common_CIS_Pointer2;
        unsigned char BUS_Suspend;
        unsigned char Function_Select;
        unsigned char Exec_Flags;
        unsigned short FN0_Block_Size;
        unsigned char Power_Control;
        unsigned char High_Speed;
        unsigned char RFU[220];	//
        unsigned char Reserved[16];
} SDIO_REG_CCCR_t;


typedef struct _SD_REG_DSR {

} SD_REG_DSR_t;

//MSB->LSB, structrue for SD Card Status
typedef struct _SD_Card_Status {

        unsigned LOCK_UNLOCK_FAILED:1;	//Set when a sequence or password error has been detected in lock/ unlock card command or if there was an attempt to access a locked card
        unsigned CARD_IS_LOCKED:1;	//When set, signals that the card is locked by the host
        unsigned WP_VIOLATION:1;	//Attempt to program a write-protected block.
        unsigned ERASE_PARAM:1;	//An invalid selection of write-blocks for erase occurred.
        unsigned ERASE_SEQ_ERROR:1;	//An error in the sequence of erase commands occurred.
        unsigned BLOCK_LEN_ERROR:1;	//The transferred block length is not allowed for this card, or the number of transferred bytes does not match the block length.
        unsigned ADDRESS_ERROR:1;	//A misaligned address that did not match the block length was used in the command.
        unsigned OUT_OF_RANGE:1;	//The command¡¯s argument was out of the allowed range for this card.

        unsigned CID_CSD_OVERWRITE:1;	//Can be either one of the following errors:
        unsigned Reserved1:1;
        unsigned Reserved2:1;
        unsigned ERROR:1;	//A general or an unknown error occurred during the operation.
        unsigned CC_ERROR:1;	//Internal card controller error
        unsigned CARD_ECC_FAILED:1;	//Card internal ECC was applied but failed to correct the data.
        unsigned ILLEGAL_COMMAND:1;	//Command not legal for the card state
        unsigned COM_CRC_ERROR:1;	//The CRC check of the previous command failed.

        unsigned READY_FOR_DATA:1;	//Corresponds to buffer empty signalling on the bus.
        unsigned CURRENT_STATE:4;	//The state of the card when receiving the command.
        unsigned ERASE_RESET:1;	//An erase sequence was cleared beforem executing because an out of erase sequence command was received.
        unsigned CARD_ECC_DISABLED:1;	//The command has been executed without using the internal ECC.
        unsigned WP_ERASE_SKIP:1;	//Only partial address space was erased due to existing write protected blocks.

        unsigned Reserved3:2;
        unsigned Reserved4:1;
        unsigned AKE_SEQ_ERROR:1;	//Error in the sequence of authentication process.
        unsigned Reserved5:1;

        unsigned APP_CMD:1;	//The card will expect ACMD, or indication that the command has been interpreted as ACMD.
        unsigned NotUsed:2;
} SD_Card_Status_t;

//MSB->LSB, structure for SD SD_Status
typedef struct _SD_SD_Status {

        unsigned Reserved1:5;
        unsigned SECURED_MODE:1;	//Card is in Secured Mode of operation (refer to the SD Security Specifications document).
        unsigned DAT_BUS_WIDTH:2;	//Shows the currently defined data bus width that was defined by the SET_sd_info.bus_width command.

        unsigned Reserved2:8;

        unsigned SIZE_OF_PROTECTED_AREA:2;	//Shows the size of the protected area. The actual area = (SIZE_OF_PROTECTED_AREA) * MULT * BLOCK_LEN.
        unsigned SD_CARD_TYPE:6;	//In the future, the 8 LSBs will be used to define different variations of an SD Card (each bit will define different SD types).

        unsigned Reserved3:8;	//just for bit structure alignment
        unsigned char Reserved4[16];
        unsigned char Reserved5[39];
} SD_SD_Status_t;


typedef struct _SD_Switch_Function__Status {

        unsigned short Max_Current_Consumption;
        unsigned short Function_Group[6];
        unsigned Function_Group_Status6:4;
        unsigned Function_Group_Status5:4;
        unsigned Function_Group_Status4:4;
        unsigned Function_Group_Status3:4;
        unsigned Function_Group_Status2:4;
        unsigned Function_Group_Status1:4;
        unsigned char Data_Struction_Verion;
        unsigned short Function_Status_In_Group[6];
        unsigned char Reserved[34];
} SD_Switch_Function_Status_t;

//structure for response
typedef struct _SD_Response_R1 {

        unsigned char command;	//command index = bit 6:0
        SD_Card_Status_t card_status;	//card status
        unsigned end_bit:1;	//end bit = bit 0
        unsigned crc7:7;	//CRC7 = bit 7:1
} SD_Response_R1_t;


typedef struct _SD_Response_R2_CID {

        unsigned char reserved;	//should be 0x3F
        SD_REG_CID_t cid;	//response CID
        unsigned end_bit:1;	//end bit = bit 0
        unsigned crc7:7;	//CRC7 = bit 7:1
} SD_Response_R2_CID_t;


typedef struct _SD_Response_R2_CSD {

        unsigned char reserved;	//should be 0x3F
        SD_REG_CSD_t csd;	//response CSD
        unsigned end_bit:1;	//end bit = bit 0
        unsigned crc7:7;	//CRC7 = bit 7:1
} SD_Response_R2_CSD_t;


typedef struct _SDHC_Response_R2_CSD {

        unsigned char reserved;	//should be 0x3F
        SDHC_REG_CSD_t csd;	//response CSD
        unsigned end_bit:1;	//end bit = bit 0
        unsigned crc7:7;	//CRC7 = bit 7:1
} SDHC_Response_R2_CSD_t;


typedef struct _SD_Response_R3 {

        unsigned char reserved1;	//should be 0x3F
        SD_REG_OCR_t ocr;	//OCR register
        unsigned end_bit:1;	//end bit = bit 0
        unsigned reserved2:7;	//should be 0x7F
} SD_Response_R3_t;


typedef struct _SDIO_Response_R4 {

        unsigned char reserved1;	//should be 0x3F
        unsigned Stuff_bits:3;
        unsigned Memory_Present:1;
        unsigned IO_Function_No:3;
        unsigned Card_Ready:1;
        SDIO_REG_OCR_t ocr;	//OCR register
        unsigned end_bit:1;	//end bit = bit 0
        unsigned reserved2:7;	//should be 0x7F
} SDIO_Response_R4_t;


typedef struct _SDIO_RW_CMD_Response_R5 {

        unsigned command:6;	//command index = bit 6:0
        unsigned direct_bit:1;
        unsigned start_bit:1;

        unsigned short stuff;	//not used

        unsigned Out_Of_Range:1;	//status of sdio card
        unsigned Function_Number:1;
        unsigned RFU:1;
        unsigned Error:1;
        unsigned IO_Current_State:2;
        unsigned Illegal_CMD:1;
        unsigned CMD_CRC_Error:1;

        unsigned char read_or_write_data;	//read back data

        unsigned end_bit:1;	//end bit = bit 0
        unsigned crc7:7;	//CRC7 = bit 7:1
} SDIO_RW_CMD_Response_R5_t;


typedef struct _SD_Response_R6 {

        unsigned char command;	//command index = bit 6:0
        unsigned char rca_high;	//New published RCA [31:16] of the card
        unsigned char rca_low;
        unsigned short part_card_status;	//[15:0] card status bits: 23,22,19,12:0
        unsigned end_bit:1;	//end bit = bit 0
        unsigned crc7:7;	//CRC7 = bit 7:1
} SD_Response_R6_t;


typedef struct _SD_Response_R7 {

        unsigned char command;	//command index = bit 6:0

        unsigned reserved1:4;
        unsigned cmd_version:4;	//0:voltage check

        unsigned char reserved;

        unsigned voltage_accept:4;	//0001b:2.7v-3.6v  0010b:1.65v-1.95v
        unsigned reserved2:4;

        unsigned char check_pattern;

        unsigned end_bit:1;	//end bit = bit 0
        unsigned crc7:7;	//CRC7 = bit 7:1
} SD_Response_R7_t;


typedef struct _SDIO_IO_RW_CMD_ARG {

        unsigned char write_data_bytes;	//write data bytes count

        unsigned stuff1:1;	//not used
        unsigned Register_Address:17;	//byte address of select fuction to read
        unsigned stuff2:1;	//not used
        unsigned RAW_Flag:1;	//read after write flag
        unsigned Function_No:3;	//function number of wish to read or write
        unsigned R_W_Flag:1;	//the direction of I/O operation
} SDIO_IO_RW_CMD_ARG_t;


typedef struct _SDIO_IO_RW_EXTENDED_ARG {

        unsigned Byte_Block_Count:9;	//bytes or block count
        unsigned Register_Address:17;	//start address of I/O register
        unsigned OP_Code:1;	//define the read write operation
        unsigned Block_Mode:1;	//Block or byte mode of read and write
        unsigned Function_No:3;	//function number of wish to read or write
        unsigned R_W_Flag:1;	//the direction of I/O operation
} SDIO_IO_RW_EXTENDED_ARG;

#pragma pack()


typedef enum _SD_Card_State {

        STATE_UNKNOWN = -1,
        STATE_INACTIVE = 0,
        STATE_IDLE,
        STATE_READY,
        STATE_IDENTIFICATION,
        STATE_STAND_BY,
        STATE_TRANSFER,
        STATE_SENDING_DATA,
        STATE_RECEIVE_DATA,
        STATE_PROGRAMMING,
        STATE_DISCONNECT
} SD_Card_State_t;


typedef enum _SD_Response_Type {

        RESPONSE_NONE = -1,
        RESPONSE_R1 = 0,
        RESPONSE_R1B,
        RESPONSE_R2_CID,
        RESPONSE_R2_CSD,
        RESPONSE_R3,
        RESPONSE_R4,	//SD, responses are not supported.
        RESPONSE_R5,		//SD, responses are not supported.
        RESPONSE_R6,
        RESPONSE_R7
} SD_Response_Type_t;

/* Error codes */
typedef enum _SD_Error_Status_t {

        SD_MMC_NO_ERROR = 0,
        SD_MMC_ERROR_OUT_OF_RANGE,	//Bit 31
        SD_MMC_ERROR_ADDRESS,	//Bit 30
        SD_MMC_ERROR_BLOCK_LEN,	//Bit 29
        SD_MMC_ERROR_ERASE_SEQ,	//Bit 28
        SD_MMC_ERROR_ERASE_PARAM,	//Bit 27
        SD_MMC_ERROR_WP_VIOLATION,	//Bit 26
        SD_ERROR_CARD_IS_LOCKED,	//Bit 25
        SD_ERROR_LOCK_UNLOCK_FAILED,	//Bit 24
        SD_MMC_ERROR_COM_CRC,	//Bit 23
        SD_MMC_ERROR_ILLEGAL_COMMAND,	//Bit 22
        SD_ERROR_CARD_ECC_FAILED,	//Bit 21
        SD_ERROR_CC,		//Bit 20
        SD_MMC_ERROR_GENERAL,	//Bit 19
        SD_ERROR_Reserved1,	//Bit 18
        SD_ERROR_Reserved2,	//Bit 17
        SD_MMC_ERROR_CID_CSD_OVERWRITE,	//Bit 16
        SD_ERROR_AKE_SEQ,	//Bit 03
        SD_MMC_ERROR_STATE_MISMATCH,
        SD_MMC_ERROR_HEADER_MISMATCH,
        SD_MMC_ERROR_DATA_CRC,
        SD_MMC_ERROR_TIMEOUT,
        SD_MMC_ERROR_DRIVER_FAILURE,
        SD_MMC_ERROR_WRITE_PROTECTED,
        SD_MMC_ERROR_NO_MEMORY,
        SD_ERROR_SWITCH_FUNCTION_COMUNICATION,
        SD_ERROR_NO_FUNCTION_SWITCH,
        SD_MMC_ERROR_NO_CARD_INS,
        SD_MMC_ERROR_READ_DATA_FAILED,
        SD_SDIO_ERROR_NO_FUNCTION
} SD_Error_Status_t;


typedef enum _SD_Operation_Mode {

        CARD_INDENTIFICATION_MODE = 0,	//fod = 100 ~ 400 Khz, OHz stops the clock. The given minimum frequency range is for cases where a continuous clock is required.
        DATA_TRANSFER_MODE = 1	//fpp = 0 ~ 25 Mhz
} SD_Operation_Mode_t;


typedef enum _SD_Bus_Width {

        SD_BUS_SINGLE = 1,	//only DAT0
        SD_BUS_WIDE = 4		//use DAT0-4
} SD_Bus_Width_t;


typedef enum SD_Card_Type {

        CARD_TYPE_NONE = 0,
        CARD_TYPE_SD,
        CARD_TYPE_SDHC,
        CARD_TYPE_MMC,
        CARD_TYPE_SDIO
} SD_Card_Type_t;


typedef enum SDIO_Card_Type {

        CARD_TYPE_NONE_SDIO =0,
        CARD_TYPE_SDIO_STD_UART,
        CARD_TYPE_SDIO_BT_TYPEA,
        CARD_TYPE_SDIO_BT_TYPEB,
        CARD_TYPE_SDIO_GPS,
        CARD_TYPE_SDIO_CAMERA,
        CARD_TYPE_SDIO_PHS,
        CARD_TYPE_SDIO_WLAN,
        CARD_TYPE_SDIO_OTHER_IF
} SDIO_Card_Type_t;


typedef enum SD_SPEC_VERSION {

        SPEC_VERSION_10_101,
        SPEC_VERSION_110,
        SPEC_VERSION_20
} SD_SPEC_VERSION_t;


typedef enum MMC_SPEC_VERSION {

        SPEC_VERSION_10_12,
        SPEC_VERSION_14,
        SPEC_VERSION_20_22,
        SPEC_VERSION_30_33,
        SPEC_VERSION_40_41
} MMC_SPEC_VERSION_t;


typedef enum SD_SPEED_CLASS {

        NORMAL_SPEED,
        HIGH_SPEED
} SD_SPEED_CLASS_t;


typedef struct SD_MMC_Card_Info {

        SD_Card_Type_t card_type;
        SDIO_Card_Type_t sdio_card_type;
        SD_Operation_Mode_t operation_mode;
        SD_Bus_Width_t bus_width;
        SD_SPEC_VERSION_t spec_version;
        MMC_SPEC_VERSION_t mmc_spec_version;
        SD_SPEED_CLASS_t speed_class;
        SD_REG_CID_t raw_cid;

        unsigned short card_rca;
        unsigned sdio_function_no;
        unsigned sdio_clk_unit;
        unsigned long blk_len;
        unsigned long blk_nums;
        unsigned long clks_nac;
        int write_protected_flag;
        int inited_flag;
        int removed_flag;
        int init_retry;
        int single_blk_failed;
        int sdio_init_flag;

        void (*sd_mmc_power) (int power_on);
        int (*sd_mmc_get_ins) (void);
        int (*sd_get_wp) (void);
        void (*sd_mmc_io_release) (void);
} SD_MMC_Card_Info_t;

//SDIO_REG_DEFINE
#define CCCR_SDIO_SPEC_REG               0x00
#define SD_SPEC_REG                      0x01
#define IO_ENABLE_REG                    0x02
#define IO_READY_REG                     0x03
#define INT_ENABLE_REG                   0x04
#define INT_PENDING_REG				  	 0x05
#define IO_ABORT_REG					 0x06
#define BUS_Interface_Control_REG		 0x07
#define Card_Capability_REG			  	 0x08
#define Common_CIS_Pointer1_REG		  	 0x09
#define Common_CIS_Pointer2_REG		  	 0x0a
#define Common_CIS_Pointer3_REG		  	 0x0b
#define BUS_Suspend_REG				  	 0x0c
#define Function_Select_REG			  	 0x0d
#define Exec_Flags_REG					 0x0e
#define Ready_Flags_REG 				 0x0f
#define FN0_Block_Size_Low_REG			 0x10
#define FN0_Block_Size_High_REG		  	 0x11
#define Power_Control_REG				 0x12
#define High_Speed_REG					 0x13
#define FN1_Block_Size_Low_REG			 0x110
#define FN1_Block_Size_High_REG			 0x111

#define SDIO_Read_Data				  0
#define SDIO_Write_Data				  1
#define SDIO_DONT_Read_After_Write	  0
#define SDIO_Read_After_Write		  1
#define SDIO_Block_MODE			  	  1
#define SDIO_Byte_MODE		  	  	  0

#define SDIO_Wide_bus_Bit			  0x02
#define SDIO_Single_bus_Bit			  0x01
#define SDIO_Support_High_Speed		  0x01
#define SDIO_Enable_High_Speed		  0x02
#define SDIO_Support_Multi_Block	  0x02
#define SDIO_INT_EN_MASK			  0x01
#define SDIO_E4MI_EN_MASK			  0x20
#define SDIO_RES_bit				  0x08

#define SDIO_BLOCK_SIZE				  128

//SD/MMC Card bus commands          CMD     type    argument                response

//Broadcast Commands (bc), no response
//Broadcast Commands with Response (bcr)
//Addressed (point-to-point) Commands (ac)¡ªno data transfer on DAT
//Addressed (point-to-point) Data Transfer Commands (adtc)¡ªdata transfer on DAT.

/* Class 0 and 1, Basic Commands */
#define SD_MMC_GO_IDLE_STATE            0	//---   [31:0] don¡¯t care       --------
#define MMC_SEND_OP_COND                1	//bcr   [31:0] OCR w/out busy   R3
#define SD_MMC_ALL_SEND_CID             2	//bcr   [31:0] don¡¯t care       R2
#define SD_MMC_SEND_RELATIVE_ADDR       3	//bcr   [31:0] don¡¯t care       R6 for SD and R1 for MMC
//  Reserved                4       -----   ----------              --------
#define IO_SEND_OP_COND                 5	//bcr   [23:0] OCR w/out busy  R4
#define SD_SET_BUS_WIDTHS               6	//ac    [31:2]stuff,[1:0]B/W    R1  ,Application Specific Commands Used
#define MMC_SWITCH_FUNTION              6	//MMC_c mmc switch power clock bus_width cmd
#define SD_SWITCH_FUNCTION				46	//bcr   [31:23]mode,[22:8]default bit,[7:0]function
#define SD_MMC_SELECT_DESELECT_CARD     7	//ac    [31:16] RCA             R1
#define SD_SEND_IF_COND                 8	//      [11:8]supply voltage    R7
#define MMC_SEND_EXT_CSD                8	//ac    [31:0]stuff             R1
#define SD_MMC_SEND_CSD                 9	//ac    [31:16] RCA             R2
#define SD_MMC_SEND_CID                 10	//ac    [31:16] RCA             R2
#define SD_READ_DAT_UNTIL_STOP          11	//adtc  [31:0] data address     R1
#define SD_MMC_STOP_TRANSMISSION        12	//ac    [31:0] don¡¯t care       R1b
#define SD_MMC_SEND_STATUS              13	//ac    [31:16] RCA             R1
//  Reserved                14      -----   ----------              --------
#define SD_MMC_GO_INACTIVE_STATE        15	//ac    [31:16] RCA             --------

/* Class 2, Block Read Commands */
#define SD_MMC_SET_BLOCKLEN             16	//ac    [31:0] block length     R1
#define SD_MMC_READ_SINGLE_BLOCK        17	//adtc  [31:0] data address     R1
#define SD_MMC_READ_MULTIPLE_BLOCK      18	//adtc  [31:0] data address     R1
//  Reserved                19      -----   ----------              --------
//  Reserved                20      -----   ----------              --------
//  Reserved                21      -----   ----------              --------
#define SD_SEND_NUM_WR_BLOCKS           22	//adtc  [31:0] stuff bits       R1  ,Application Specific Commands Used
#define SD_SET_WR_BLK_ERASE_COUNT       23	//ac    [31:23]stuff,[22:0]B/N  R1  ,Application Specific Commands Used

/* Class 4, Block Write Commands */
#define SD_MMC_WRITE_BLOCK              24	//adtc  [31:0] data address     R1
#define SD_MMC_WRITE_MULTIPLE_BLOCK     25	//adtc  [31:0] data address     R1
//  Reserved                26      -----   ----------              --------
#define SD_MMC_PROGRAM_CSD              27	//adtc  [31:0] don¡¯t care*      R1

/* Class 6, Write Protection */
#define SD_MMC_SET_WRITE_PROT           28	//ac    [31:0] data address     R1b
#define SD_MMC_CLR_WRITE_PROT           29	//ac    [31:0] data address     R1b
#define SD_MMC_SEND_WRITE_PROT          30	//adtc  [31:0] WP data address  R1
//  Reserved                31      -----   ----------              --------

/* Class 5, Erase Commands */
#define SD_ERASE_WR_BLK_START           32	//ac    [31:0] data address     R1
#define MMC_TAG_SECTOR_START            32	//ac    [31:0] data address     R1
#define SD_ERASE_WR_BLK_END             33	//ac    [31:0] data address     R1
#define MMC_TAG_SECTOR_END              33	//ac    [31:0] data address     R1
#define MMC_UNTAG_SECTOR                34	//ac    [31:0] data address     R1
#define MMC_TAG_ERASE_GROUP_START       35	//ac    [31:0] data address     R1
#define MMC_TAG_ERASE_GROUP_END         36	//ac    [31:0] data address     R1
#define MMC_UNTAG_ERASE_GROUP           37	//ac    [31:0] data address     R1
#define SD_MMC_ERASE                    38	//ac    [31:0] don¡¯t care       R1b
//  Reserved                39      -----   ----------              --------
//  Reserved                40      -----   ----------              --------
#define SD_APP_OP_COND                  41	//bcr   [31:0]OCR without busy  R3  ,Application Specific Commands Used

/* Class 7, Lock Card Commands */
#define SD_SET_CLR_CARD_DETECT          42	//ac    [31:1]stuff,[0]set_cd   R1
#define MMC_LOCK_UNLOCK                 42	//adtc  [31:0] stuff bits       R1b
//  SDA Optional Commands           43      -----   ----------              --------
//  SDA Optional Commands           44      -----   ----------              --------
//  SDA Optional Commands           45      -----   ----------              --------
//  SDA Optional Commands           46      -----   ----------              --------
//  SDA Optional Commands           47      -----   ----------              --------
//  SDA Optional Commands           48      -----   ----------              --------
//  SDA Optional Commands           49      -----   ----------              --------
//  SDA Optional Commands           50      -----   ----------              --------
#define SD_SEND_SCR                     51	//adtc  [31:0] staff bits       R1  ,Application Specific Commands Used
#define IO_RW_DIRECT                    52	//R5
#define IO_RW_EXTENDED                  53	//R5-----   ----------              --------
//  SDA Optional Commands           54      -----   ----------              --------

/* Class 8, Application Specific Commands */
#define SD_APP_CMD                      55	//ac    [31:16]RCA,[15:0]stuff  R1
#define SD_GEN_CMD                      56	//adtc  [31:1]stuff,[0]RD/WR    R1
//  Reserved                57      -----   ----------              --------
//  Reserved                58      -----   ----------              --------
//  Reserved                59      -----   ----------              --------
//  Rsserved for Manufacturer       60      -----   ----------              --------
//  Rsserved for Manufacturer       61      -----   ----------              --------
//  Rsserved for Manufacturer       62      -----   ----------              --------
//  Rsserved for Manufacturer       63      -----   ----------              --------

//All timing values definition for NAND MMC and SD-based Products
#define SD_MMC_TIME_NCR_MIN             2           /* min. of Number of cycles
between command and response */
#define SD_MMC_TIME_NCR_MAX             (128*10)    /* max. of Number of cycles
between command and response */
#define SD_MMC_TIME_NID                 5           /* Number of cycles
between card identification or
card operation conditions command
and the corresponding response */
#define SD_MMC_TIME_NAC_MIN             2           /* min. of Number of cycles
between command and
the start of a related data block */
#define SD_MMC_TIME_NRC_MIN             8           /* min. of Number of cycles
between the last reponse and
a new command */
#define SD_MMC_TIME_NCC_MIN             8           /* min. of Number of cycles
between two commands, if no reponse
will be send after the first command
(e.g. broadcast) */
#define SD_MMC_TIME_NWR_MIN             2           /* min. of Number of cycles
        between a write command and
        the start of a related data block */
#define SD_MMC_TIME_NRC_NCC             16          /* actual NRC/NCC time used in source code */

#define SD_MMC_TIME_NWR                 8          /* actual Nwr time used in source code */

#define SD_MMC_Z_CMD_TO_RES             2          	/* number of Z cycles
        allowing time for direction switching on the bus) */
#define SD_MMC_TIME_NAC_DEFAULT         25500

//Misc definitions
#define MAX_RESPONSE_BYTES              20

#define RESPONSE_R1_R3_R4_R5_R6_R7_LENGTH     6
#define RESPONSE_R2_CID_CSD_LENGTH            17
#define RESPONSE_NONE_LENGTH                  0

#define SD_MMC_IDENTIFY_TIMEOUT			1500	// ms, SD=1000, MMC=500
#define MAX_CHECK_INSERT_RETRY          3

#define SD_IDENTIFICATION_TIMEOUT       (250*TIMER_1MS)	// ms
#define SD_PROGRAMMING_TIMEOUT          (1500*TIMER_1MS)	// ms

#define SDIO_FUNCTION_TIMEOUT			1000

#define SD_MMC_INIT_RETRY				3

#define SD_MMC_IDENTIFY_CLK							300	//K HZ
#define SD_MMC_TRANSFER_SLOWER_CLK					12	//M HZ
#define SD_MMC_TRANSFER_CLK							18	//M HZ
#define SD_MMC_TRANSFER_HIGHSPEED_CLK				25	//M HZ


/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
#define SD_CMD_OUTPUT_EN_REG 		EGPIO_GPIOA_ENABLE
#define SD_CMD_OUTPUT_EN_MASK		PREG_IO_18_MASK
#define SD_CMD_INPUT_REG			EGPIO_GPIOA_INPUT
#define SD_CMD_INPUT_MASK			PREG_IO_18_MASK
#define SD_CMD_OUTPUT_REG			EGPIO_GPIOA_OUTPUT
#define SD_CMD_OUTPUT_MASK			PREG_IO_18_MASK

#define SD_CLK_OUTPUT_EN_REG		EGPIO_GPIOA_ENABLE
#define SD_CLK_OUTPUT_EN_MASK		PREG_IO_17_MASK
#define SD_CLK_OUTPUT_REG			EGPIO_GPIOA_OUTPUT
#define SD_CLK_OUTPUT_MASK			PREG_IO_17_MASK

#define SD_DAT_OUTPUT_EN_REG		EGPIO_GPIOA_ENABLE
#define SD_DAT0_OUTPUT_EN_MASK		PREG_IO_13_MASK
#define SD_DAT0_3_OUTPUT_EN_MASK	PREG_IO_13_16_MASK
#define SD_DAT_INPUT_REG			EGPIO_GPIOA_INPUT
#define SD_DAT_OUTPUT_REG			EGPIO_GPIOA_OUTPUT
#define SD_DAT0_INPUT_MASK			PREG_IO_13_MASK
#define SD_DAT0_OUTPUT_MASK			PREG_IO_13_MASK
#define SD_DAT0_3_INPUT_MASK		PREG_IO_13_16_MASK
#define SD_DAT0_3_OUTPUT_MASK		PREG_IO_13_16_MASK
#define SD_DAT_INPUT_OFFSET			13
#define SD_DAT_OUTPUT_OFFSET		13

#define SD_INS_OUTPUT_EN_REG		EGPIO_GPIOA_ENABLE
#define SD_INS_OUTPUT_EN_MASK		PREG_IO_3_MASK
#define SD_INS_INPUT_REG			EGPIO_GPIOA_INPUT
#define SD_INS_INPUT_MASK			PREG_IO_3_MASK

#define SD_WP_OUTPUT_EN_REG			EGPIO_GPIOA_ENABLE
#define SD_WP_OUTPUT_EN_MASK		PREG_IO_11_MASK
#define SD_WP_INPUT_REG				EGPIO_GPIOA_INPUT
#define SD_WP_INPUT_MASK			PREG_IO_11_MASK

#define SD_PWR_OUTPUT_EN_REG		JTAG_GPIO_ENABLE
#define SD_PWR_OUTPUT_EN_MASK		PREG_IO_16_MASK
#define SD_PWR_OUTPUT_REG			JTAG_GPIO_OUTPUT
#define SD_PWR_OUTPUT_MASK			PREG_IO_20_MASK
#define SD_PWR_EN_LEVEL				0

#define sd_gpio_enable()			{CLEAR_CBUS_REG_MASK(CARD_PIN_MUX_0, (0x3F<<23));CLEAR_CBUS_REG_MASK(SDIO_MULT_CONFIG, (0));}

#define sd_set_cmd_input()				{(*(volatile unsigned int *)SD_CMD_OUTPUT_EN_REG) |= SD_CMD_OUTPUT_EN_MASK; for(i_GPIO_timer=0;i_GPIO_timer<15;i_GPIO_timer++);}
#define sd_set_cmd_output()			{(*(volatile unsigned int *)SD_CMD_OUTPUT_EN_REG) &= (~SD_CMD_OUTPUT_EN_MASK);}
#define sd_set_cmd_value(data)			{if(data){(*(volatile unsigned int *)SD_CMD_OUTPUT_REG) |= SD_CMD_OUTPUT_MASK;}else{(*(volatile unsigned int *)SD_CMD_OUTPUT_REG) &= (~SD_CMD_OUTPUT_MASK);}}
#define sd_get_cmd_value()				((*(volatile unsigned int *)SD_CMD_INPUT_REG & SD_CMD_INPUT_MASK)?1:0)

#define sd_set_clk_output()    			{(*(volatile unsigned int *)SD_CLK_OUTPUT_EN_REG) &= (~SD_CLK_OUTPUT_EN_MASK);}
#define sd_set_clk_high()				{(*(volatile unsigned int *)SD_CLK_OUTPUT_REG) |= SD_CLK_OUTPUT_MASK;}
#define sd_set_clk_low()				{(*(volatile unsigned int *)SD_CLK_OUTPUT_REG) &= (~SD_CLK_OUTPUT_MASK);}

#define sd_set_dat0_input()				{(*(volatile unsigned int *)SD_DAT_OUTPUT_EN_REG) |= SD_DAT0_OUTPUT_EN_MASK; for(i_GPIO_timer=0;i_GPIO_timer<15;i_GPIO_timer++);}
#define sd_set_dat0_output()			{(*(volatile unsigned int *)SD_DAT_OUTPUT_EN_REG) &= (~SD_DAT0_OUTPUT_EN_MASK);}
#define sd_set_dat0_value(data)			{if(data){*(volatile unsigned int *)SD_DAT_OUTPUT_REG |= SD_DAT0_OUTPUT_MASK;}else{*(volatile unsigned int *)SD_DAT_OUTPUT_REG &= (~SD_DAT0_OUTPUT_MASK);}}
#define sd_get_dat0_value()				((*(volatile unsigned int *)SD_DAT_INPUT_REG & SD_DAT0_INPUT_MASK)?1:0)

#define sd_set_dat0_3_input()			{(*(volatile unsigned int *)SD_DAT_OUTPUT_EN_REG) |= (SD_DAT0_3_OUTPUT_EN_MASK); for(i_GPIO_timer=0;i_GPIO_timer<15;i_GPIO_timer++);}
#define sd_set_dat0_3_output()			{(*(volatile unsigned int *)SD_DAT_OUTPUT_EN_REG) &= (~SD_DAT0_3_OUTPUT_EN_MASK);}

//#define sd_set_dat0_3_value(data)		{(*(volatile unsigned int *)SD_DAT_OUTPUT_REG) = ((*(volatile unsigned int *)SD_DAT_OUTPUT_REG) & ((~SD_DAT0_3_OUTPUT_MASK) | (data << SD_DAT_OUTPUT_OFFSET))); }


#define sd_set_dat0_3_value(data)		{(*(volatile unsigned int *)SD_DAT_OUTPUT_REG) =( ((*(volatile unsigned int *)SD_DAT_OUTPUT_REG) & (~SD_DAT0_3_OUTPUT_MASK)) | (data << SD_DAT_OUTPUT_OFFSET));}
//#define sd_set_dat0_3_value(data)		{(*(volatile unsigned int *)SD_DAT_OUTPUT_REG) =( ((*(volatile unsigned int *)SD_DAT_OUTPUT_REG) & (~SD_DAT0_3_OUTPUT_MASK)) |(( (~data )&0xf)<< SD_DAT_OUTPUT_OFFSET));}


#define sd_get_dat0_3_value()			((*(volatile unsigned int *)SD_DAT_INPUT_REG & SD_DAT0_3_INPUT_MASK) >> SD_DAT_OUTPUT_OFFSET)

#define	sd_set_ins_input()				{(*(volatile unsigned int *)SD_INS_OUTPUT_EN_REG) |= SD_INS_OUTPUT_EN_MASK; for(i_GPIO_timer=0;i_GPIO_timer<15;i_GPIO_timer++);}
#define sd_get_ins_value()				((*(volatile unsigned int *)SD_INS_INPUT_REG & SD_INS_INPUT_MASK)?1:0)


/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

 int sdio_write_data_block_hw(int function_no, int buf_or_fifo,
                                     unsigned long sdio_addr,
                                     unsigned long block_count,
                                     unsigned char *data_buf);
int sdio_close_target_interrupt(int function_no);
int sdio_open_target_interrupt(int function_no);
int sd_buffer_alloc(void);
int sd_send_cmd_hw(unsigned char cmd, unsigned long arg, SD_Response_Type_t res_type, unsigned char * res_buf, unsigned char *data_buf, unsigned long data_cnt, int retry_flag);
 extern int i_GPIO_timer;
int sd_send_cmd_sw(unsigned char cmd, unsigned long arg, SD_Response_Type_t res_type, unsigned char * res_buf);
int sdio_write_data_byte_sw(unsigned long sdio_addr, unsigned long byte_count, unsigned char *data_buf);
int sdio_read_data_byte_sw( unsigned long sdio_addr, unsigned long byte_count, unsigned char *data_buf);

#endif				//_H_SD_PROTOCOL


