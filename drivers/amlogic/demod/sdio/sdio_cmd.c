#include "sdio_init.h"

static char * sd_error_string[]={
        "SD_MMC_NO_ERROR",
        "SD_MMC_ERROR_OUT_OF_RANGE",        //Bit 31
        "SD_MMC_ERROR_ADDRESS",             //Bit 30
        "SD_MMC_ERROR_BLOCK_LEN",           //Bit 29
        "SD_MMC_ERROR_ERASE_SEQ",           //Bit 28
        "SD_MMC_ERROR_ERASE_PARAM",         //Bit 27
        "SD_MMC_ERROR_WP_VIOLATION",        //Bit 26
        "SD_ERROR_CARD_IS_LOCKED",          //Bit 25
        "SD_ERROR_LOCK_UNLOCK_FAILED",      //Bit 24
        "SD_MMC_ERROR_COM_CRC",             //Bit 23
        "SD_MMC_ERROR_ILLEGAL_COMMAND",     //Bit 22
        "SD_ERROR_CARD_ECC_FAILED",         //Bit 21
        "SD_ERROR_CC",                      //Bit 20
        "SD_MMC_ERROR_GENERAL",             //Bit 19
        "SD_ERROR_Reserved1",               //Bit 18
        "SD_ERROR_Reserved2",               //Bit 17
        "SD_MMC_ERROR_CID_CSD_OVERWRITE",   //Bit 16
        "SD_ERROR_AKE_SEQ",                 //Bit 03
        "SD_MMC_ERROR_STATE_MISMATCH",
        "SD_MMC_ERROR_HEADER_MISMATCH",
        "SD_MMC_ERROR_DATA_CRC",
        "SD_MMC_ERROR_TIMEOUT",
        "SD_MMC_ERROR_DRIVER_FAILURE",
        "SD_MMC_ERROR_WRITE_PROTECTED",
        "SD_MMC_ERROR_NO_MEMORY",
        "SD_ERROR_SWITCH_FUNCTION_COMUNICATION",
        "SD_ERROR_NO_FUNCTION_SWITCH",
        "SD_MMC_ERROR_NO_CARD_INS",
        "SD_MMC_ERROR_READ_DATA_FAILED",
        "SD_SDIO_ERROR_NO_FUNCTION"
};

void sd_clear_response(unsigned char * res_buf);
static unsigned sdio_read_crc_close = 0;
#define FPGA_SDIO_WIDTH  					SD_BUS_WIDE
#define FPGA_SDIO_BLOCK_SIZE				SDIO_BLOCK_SIZE
#define FPGA_SDIO_CLOCK					((FPGA_CLK_DIVIDE)?(1000/(50/FPGA_CLK_DIVIDE)):1000/50)

extern int i_GPIO_timer;
extern unsigned sdio_timeout_int_times ;
extern unsigned sdio_timeout_int_num;
extern struct completion sdio_int_complete;
unsigned char *sd_mmc_phy_buf = NULL;
unsigned char *sd_mmc_buf = NULL;

int sd_buffer_alloc(void)
{
        struct sk_buff *skb ;
        skb =  dev_alloc_skb(512);
        sd_mmc_buf =skb->data;
        return SD_MMC_NO_ERROR;
}

//Return the string buf address of specific errcode
char * sd_error_to_string(int errcode)
{
        return sd_error_string[errcode];
}
//Get response length according to response type
int sd_get_response_length(SD_Response_Type_t res_type)
{
        int num_res;

        switch (res_type) {
        case RESPONSE_R1:
        case RESPONSE_R1B:
        case RESPONSE_R3:
        case RESPONSE_R4:
        case RESPONSE_R5:
        case RESPONSE_R6:
        case RESPONSE_R7:
                num_res = RESPONSE_R1_R3_R4_R5_R6_R7_LENGTH;
                break;
        case RESPONSE_R2_CID:
        case RESPONSE_R2_CSD:
                num_res = RESPONSE_R2_CID_CSD_LENGTH;
                break;
        case RESPONSE_NONE:
                num_res = RESPONSE_NONE_LENGTH;
                break;
        default:
                num_res = RESPONSE_NONE_LENGTH;
                break;
        }

        return num_res;
}

//Clear response data buffer
void sd_clear_response(unsigned char * res_buf)
{
        int i;

        if (res_buf == NULL)
                return;

        for (i = 0; i < MAX_RESPONSE_BYTES; i++)
                res_buf[i]=0;
}

//Check R1 response and return the result
//Check R1 response and return the result
int sd_check_response_r1(unsigned char cmd, SD_Response_R1_t * r1)
{
#ifdef SD_MMC_HW_CONTROL
        return SD_MMC_NO_ERROR;
#else
        if (r1->command != cmd)
                return SD_MMC_ERROR_HEADER_MISMATCH;
        if (r1->card_status.OUT_OF_RANGE)
                return SD_MMC_ERROR_OUT_OF_RANGE;
        else if (r1->card_status.ADDRESS_ERROR)
                return SD_MMC_ERROR_ADDRESS;
        else if (r1->card_status.BLOCK_LEN_ERROR)
                return SD_MMC_ERROR_BLOCK_LEN;
        else if (r1->card_status.ERASE_SEQ_ERROR)
                return SD_MMC_ERROR_ERASE_SEQ;
        else if (r1->card_status.ERASE_PARAM)
                return SD_MMC_ERROR_ERASE_PARAM;
        else if (r1->card_status.WP_VIOLATION)
                return SD_MMC_ERROR_WP_VIOLATION;
        else if (r1->card_status.CARD_IS_LOCKED)
                return SD_ERROR_CARD_IS_LOCKED;
        else if (r1->card_status.LOCK_UNLOCK_FAILED)
                return SD_ERROR_LOCK_UNLOCK_FAILED;
        else if (r1->card_status.COM_CRC_ERROR)
                return SD_MMC_ERROR_COM_CRC;
        else if (r1->card_status.ILLEGAL_COMMAND)
                return SD_MMC_ERROR_ILLEGAL_COMMAND;
        else if (r1->card_status.CARD_ECC_FAILED)
                return SD_ERROR_CARD_ECC_FAILED;
        else if (r1->card_status.CC_ERROR)
                return SD_ERROR_CC;
        else if (r1->card_status.ERROR)
                return SD_MMC_ERROR_GENERAL;
        else if (r1->card_status.CID_CSD_OVERWRITE)
                return SD_MMC_ERROR_CID_CSD_OVERWRITE;
        else if (r1->card_status.AKE_SEQ_ERROR)
                return SD_ERROR_AKE_SEQ;
#endif

        return SD_MMC_NO_ERROR;
}

//Check R3 response and return the result
int sd_check_response_r3(unsigned char cmd, SD_Response_R3_t * r3)
{
#ifdef SD_MMC_SW_CONTROL
        if ((r3->reserved1 != 0x3F))
                return SD_MMC_ERROR_HEADER_MISMATCH;

#endif

        return SD_MMC_NO_ERROR;
}

int sd_check_response_r4(unsigned char cmd, SDIO_Response_R4_t * r4)
{
#ifdef SD_MMC_SW_CONTROL
        if ((r4->reserved1 != 0x3F))// || (r3->reserved2 != 0x7F))
                return SD_MMC_ERROR_HEADER_MISMATCH;

#endif
        return SD_MMC_NO_ERROR;
}

int sd_check_response_r5(unsigned char cmd, SDIO_RW_CMD_Response_R5_t * r5)
{
#ifdef SD_MMC_SW_CONTROL
        if ((r5->command == IO_RW_DIRECT) || (r5->command == IO_RW_EXTENDED))
                return SD_MMC_NO_ERROR;
        return SD_MMC_ERROR_HEADER_MISMATCH;

#endif

        return SD_MMC_NO_ERROR;
}

//Check R6 response and return the result
int sd_check_response_r6(unsigned char cmd, SD_Response_R6_t * r6)
{
        return SD_MMC_NO_ERROR;
}

int sd_check_response_r7(unsigned char cmd, SD_Response_R7_t * r7)
{
        return SD_MMC_NO_ERROR;
}
//Check response and return the result
int sd_check_response(unsigned char cmd, SD_Response_Type_t res_type, unsigned char * res_buf)
{
        int ret = SD_MMC_NO_ERROR;

        switch (res_type) {
        case RESPONSE_R1:
        case RESPONSE_R1B:
                ret = sd_check_response_r1(cmd, (SD_Response_R1_t *)res_buf);
                break;

        case RESPONSE_R3:
                ret = sd_check_response_r3(cmd, (SD_Response_R3_t *)res_buf);
                break;

        case RESPONSE_R4:
                ret = sd_check_response_r4(cmd, (SDIO_Response_R4_t *)res_buf);
                break;
        case RESPONSE_R5:
                ret = sd_check_response_r5(cmd, (SDIO_RW_CMD_Response_R5_t *)res_buf);
                break;
        case RESPONSE_R6:
                ret = sd_check_response_r6(cmd, (SD_Response_R6_t *)res_buf);
                break;

        case RESPONSE_NONE:
                break;

        default:
                break;
        }

        return ret;
}
extern void Dump( void* data, unsigned int length );
int sd_send_cmd_hw(unsigned char cmd, unsigned long arg, SD_Response_Type_t res_type, unsigned char * res_buf, unsigned char *data_buf, unsigned long data_cnt, int retry_flag)
{
        int ret = SD_MMC_NO_ERROR, num_res;
        unsigned char *buffer = NULL;
        unsigned int cmd_ext, cmd_send;
        int loop=0;

        MSHW_IRQ_Config_Reg_t *irq_config_reg;
        SDIO_Status_IRQ_Reg_t *status_irq_reg;
        SDHW_CMD_Send_Reg_t *cmd_send_reg;
        SDHW_Extension_Reg_t *cmd_ext_reg;
        unsigned int irq_config, status_irq, timeout;
        dma_addr_t data_dma_to_device_addr=0;
        dma_addr_t data_dma_from_device_addr=0;

        //Dump(data_buf,data_cnt);
        cmd_send = 0;
        cmd_send_reg = (void *)&cmd_send;
        if (cmd == SD_SWITCH_FUNCTION)
                cmd_send_reg->cmd_data  = 0x40 | (cmd-40);          //for distinguish ACMD6 and CMD6,Maybe more good way but now I cant find
        else
                cmd_send_reg->cmd_data = 0x40 | cmd;
        cmd_send_reg->use_int_window = 1;

        cmd_ext = 0;
        cmd_ext_reg = (void *)&cmd_ext;

        sd_clear_response(res_buf);

        switch (res_type) {
        case RESPONSE_R1:
        case RESPONSE_R1B:
        case RESPONSE_R3:
        case RESPONSE_R4:
        case RESPONSE_R6:
        case RESPONSE_R5:
        case RESPONSE_R7:
                cmd_send_reg->cmd_res_bits = 45;		// RESPONSE have 7(cmd)+32(respnse)+7(crc)-1 data
                break;
        case RESPONSE_R2_CID:
        case RESPONSE_R2_CSD:
                cmd_send_reg->cmd_res_bits = 133;		// RESPONSE have 7(cmd)+120(respnse)+7(crc)-1 data
                cmd_send_reg->res_crc7_from_8 = 1;
                break;
        case RESPONSE_NONE:
                cmd_send_reg->cmd_res_bits = 0;			// NO_RESPONSE
                break;
        default:
                cmd_send_reg->cmd_res_bits = 0;			// NO_RESPONSE
                break;
        }

        //cmd with adtc
        switch (cmd) {
        case SD_MMC_READ_SINGLE_BLOCK:
        case SD_MMC_READ_MULTIPLE_BLOCK:
                cmd_send_reg->res_with_data = 1;
                cmd_send_reg->repeat_package_times = data_cnt/FPGA_SDIO_BLOCK_SIZE - 1;
                if (FPGA_SDIO_WIDTH == SD_BUS_WIDE)
                        cmd_ext_reg->data_rw_number = FPGA_SDIO_BLOCK_SIZE * 8 + (16 - 1) * 4;
                else
                        cmd_ext_reg->data_rw_number = FPGA_SDIO_BLOCK_SIZE * 8 + 16 - 1;
                buffer = sd_mmc_phy_buf;
                break;

        case SD_SWITCH_FUNCTION:
                cmd_send_reg->res_with_data = 1;
                cmd_send_reg->repeat_package_times = 0;
                if (FPGA_SDIO_WIDTH == SD_BUS_WIDE)
                        cmd_ext_reg->data_rw_number = data_cnt * 8 + (16 - 1) * 4;
                else
                        cmd_ext_reg->data_rw_number = data_cnt * 8 + 16 - 1;
                buffer = sd_mmc_phy_buf;
                break;

        case SD_MMC_WRITE_BLOCK:
        case SD_MMC_WRITE_MULTIPLE_BLOCK:
                cmd_send_reg->cmd_send_data = 1;
                cmd_send_reg->repeat_package_times = data_cnt/FPGA_SDIO_BLOCK_SIZE - 1;
                if (FPGA_SDIO_WIDTH == SD_BUS_WIDE)
                        cmd_ext_reg->data_rw_number = FPGA_SDIO_BLOCK_SIZE * 8 + (16 - 1) * 4;
                else
                        cmd_ext_reg->data_rw_number = FPGA_SDIO_BLOCK_SIZE * 8 + 16 - 1;
                //buffer = sd_write_buf;
                //memcpy(buffer, data_buf, data_cnt);
                buffer = data_buf;
                //inv_dcache_range((unsigned long)buffer, ((unsigned long)buffer + data_cnt));
                break;

        case IO_RW_EXTENDED:
                if (arg & (1<<27)) {
                        cmd_send_reg->repeat_package_times = data_cnt/FPGA_SDIO_BLOCK_SIZE - 1;
                        if (FPGA_SDIO_WIDTH == SD_BUS_WIDE)
                                cmd_ext_reg->data_rw_number = FPGA_SDIO_BLOCK_SIZE * 8 + (16 - 1) * 4;
                        else
                                cmd_ext_reg->data_rw_number = FPGA_SDIO_BLOCK_SIZE * 8 + (16 - 1);
                }
                else {
                        cmd_send_reg->repeat_package_times = 0;
                        if (FPGA_SDIO_WIDTH == SD_BUS_WIDE)
                                cmd_ext_reg->data_rw_number = data_cnt * 8 + (16 - 1) * 4;
                        else
                                cmd_ext_reg->data_rw_number = data_cnt * 8 + (16 - 1);
                }

                if (arg & (1<<31)) {
                        cmd_send_reg->cmd_send_data = 1;
                        //buffer = data_buf;
                        // printk("data_buf %x data_cnt%d \n",data_cnt);
                        data_dma_to_device_addr=dma_map_single(NULL, (void *)data_buf, data_cnt, DMA_TO_DEVICE);
                        if (unlikely(dma_mapping_error(NULL,data_dma_to_device_addr))) {
                                PRINT("dma_map_single dma_mapping_error!\n");
                                return SD_SDIO_ERROR_NO_FUNCTION;
                        }
                        buffer = (unsigned char*)data_dma_to_device_addr;

                        //inv_dcache_range((unsigned long)buffer,((unsigned long)buffer)+data_cnt-1);
                }
                else {
                        cmd_send_reg->res_with_data = 1;
                        // buffer = sd_mmc_phy_buf;
                        data_dma_from_device_addr = dma_map_single(NULL, (void *)sd_mmc_buf, data_cnt, DMA_FROM_DEVICE );
                        if (unlikely(dma_mapping_error(NULL,data_dma_from_device_addr))) {
                                PRINT("dma_map_single dma_mapping_error!\n");
                                return SD_SDIO_ERROR_NO_FUNCTION;
                        }
                        buffer = (unsigned char*)data_dma_from_device_addr;
                }
                break;

        case SD_READ_DAT_UNTIL_STOP:
        case SD_SEND_NUM_WR_BLOCKS:

        case SD_MMC_PROGRAM_CSD:
        case SD_MMC_SEND_WRITE_PROT:
        case MMC_LOCK_UNLOCK:
        case SD_SEND_SCR:
        case SD_GEN_CMD:
                cmd_send_reg->res_with_data = 1;
                if (FPGA_SDIO_WIDTH == SD_BUS_WIDE)
                        cmd_ext_reg->data_rw_number = data_cnt * 8 + (16 - 1) * 4;
                else
                        cmd_ext_reg->data_rw_number = data_cnt * 8 + 16 - 1;
                buffer = data_buf;
                break;

        default:
                break;

        }

        //cmd with R1b
        switch (cmd) {
        case SD_MMC_STOP_TRANSMISSION:
        case SD_MMC_SET_WRITE_PROT:
        case SD_MMC_CLR_WRITE_PROT:
        case SD_MMC_ERASE:
        case MMC_LOCK_UNLOCK:
                cmd_send_reg->check_dat0_busy = 1;
                break;
        default:
                break;

        }

        //cmd with R3
        switch (cmd) {
        case MMC_SEND_OP_COND:
        case SD_APP_OP_COND:
        case IO_SEND_OP_COND:
                cmd_send_reg->res_without_crc7 = 1;
                break;
        default:
                break;

        }

#define SD_MMC_READ_BUSY_COUNT		20000//20
#define SD_MMC_WRITE_BUSY_COUNT		500000//500000
#define SD_MMC_RETRY_COUNT			2

        if (cmd_send_reg->cmd_send_data) {
                if (cmd == SD_MMC_WRITE_MULTIPLE_BLOCK)
                        timeout = SD_MMC_WRITE_BUSY_COUNT * (data_cnt/512);
                else if (cmd == IO_RW_EXTENDED)
                        timeout = SD_MMC_WRITE_BUSY_COUNT * (cmd_send_reg->repeat_package_times + 1);
                else
                        timeout = SD_MMC_WRITE_BUSY_COUNT;
        }
        else {
                if (cmd == SD_MMC_READ_MULTIPLE_BLOCK)
                        timeout = SD_MMC_READ_BUSY_COUNT * (data_cnt/512);
                else if (cmd == IO_RW_EXTENDED)
                        timeout = SD_MMC_READ_BUSY_COUNT * (cmd_send_reg->repeat_package_times + 1);
                else
                        timeout = SD_MMC_READ_BUSY_COUNT;
        }

        if (cmd == SD_MMC_STOP_TRANSMISSION)
                timeout = 2000000;

        irq_config = READ_CBUS_REG(SDIO_IRQ_CONFIG);
        irq_config_reg = (void *)&irq_config;

        irq_config_reg->soft_reset = 1;
        WRITE_CBUS_REG(SDIO_IRQ_CONFIG, irq_config);

        status_irq = 0;
        status_irq_reg = (void *)&status_irq;
        status_irq_reg->if_int = 1;
        status_irq_reg->cmd_int = 1;
        status_irq_reg->timing_out_int = 1;
        if (timeout > (FPGA_SDIO_CLOCK*0x1FFF)/1000) {
                status_irq_reg->timing_out_count = 0x1FFF;
                sdio_timeout_int_times = (timeout*1000)/(FPGA_SDIO_CLOCK*0x1FFF);
        }
        else {
                status_irq_reg->timing_out_count = (timeout/FPGA_SDIO_CLOCK)*1000;
                sdio_timeout_int_times = 1;
        }
        WRITE_CBUS_REG(SDIO_STATUS_IRQ, status_irq);

        WRITE_CBUS_REG(CMD_ARGUMENT, arg);
        WRITE_CBUS_REG(SDIO_EXTENSION, cmd_ext);
        if (buffer != NULL) {
                WRITE_CBUS_REG(SDIO_M_ADDR, (unsigned long)buffer);
        }
#ifdef SDIO_IRQ
        init_completion(&sdio_int_complete);
        sdio_open_host_interrupt(SDIO_CMD_INT);
        sdio_open_host_interrupt(SDIO_TIMEOUT_INT);

#endif//SDIO_IRQ
        WRITE_CBUS_REG(CMD_SEND, cmd_send);
#ifdef SDIO_IRQ
        timeout =500;/*5s*/
        timeout = wait_for_completion_timeout(&sdio_int_complete,timeout);
#endif//SDIO_IRQ
        while (1) {
                status_irq = READ_CBUS_REG(SDIO_STATUS_IRQ);
                if ((status_irq&(1<<4)) == 0) break;
                udelay(10);
                loop++;
                if (loop>100000) {
                        sdio_timeout_int_times =0;
                }

                if (sdio_timeout_int_times == 0)
                        break;
        }

        if (sdio_timeout_int_times == 0) {
                ret =  SD_MMC_ERROR_TIMEOUT;
                goto error;
        }

        status_irq = READ_CBUS_REG(SDIO_STATUS_IRQ);
        if (cmd_send_reg->cmd_res_bits && !cmd_send_reg->res_without_crc7 && !status_irq_reg->res_crc7_ok && !sdio_read_crc_close) {
                ret =  SD_MMC_ERROR_COM_CRC;
                goto error;
        }

        num_res = sd_get_response_length(res_type);

        if (num_res > 0) {
                unsigned long multi_config = 0;
                SDIO_Multi_Config_Reg_t *multi_config_reg = (void *)&multi_config;
                multi_config= READ_CBUS_REG(SDIO_MULT_CONFIG );
                multi_config_reg->write_read_out_index = 1;
                WRITE_CBUS_REG(SDIO_MULT_CONFIG, multi_config);
                num_res--;		// Minus CRC byte
        }
        while (num_res) {
                unsigned long data_temp = READ_CBUS_REG(CMD_ARGUMENT);

                res_buf[--num_res] = data_temp & 0xFF;
                if (num_res <= 0)
                        break;
                res_buf[--num_res] = (data_temp >> 8) & 0xFF;
                if (num_res <= 0)
                        break;
                res_buf[--num_res] = (data_temp >> 16) & 0xFF;
                if (num_res <= 0)
                        break;
                res_buf[--num_res] = (data_temp >> 24) & 0xFF;
        }

        ret = sd_check_response(cmd, res_type, res_buf);
        if (ret)
                goto error;

        //cmd with adtc
        switch (cmd) {
        case SD_READ_DAT_UNTIL_STOP:
        case SD_MMC_READ_SINGLE_BLOCK:
        case SD_MMC_READ_MULTIPLE_BLOCK:
        case SD_SWITCH_FUNCTION:
                if (!status_irq_reg->data_read_crc16_ok)
                        ret =  SD_MMC_ERROR_DATA_CRC;
                goto error;
                break;
        case SD_MMC_WRITE_BLOCK:
        case SD_MMC_WRITE_MULTIPLE_BLOCK:
        case SD_MMC_PROGRAM_CSD:
                if (!status_irq_reg->data_write_crc16_ok)
                        ret =  SD_MMC_ERROR_DATA_CRC;
                goto error;
                break;
        case SD_SEND_NUM_WR_BLOCKS:
        case SD_MMC_SEND_WRITE_PROT:
        case MMC_LOCK_UNLOCK:
        case SD_SEND_SCR:
        case SD_GEN_CMD:
                if (!status_irq_reg->data_read_crc16_ok)
                        ret =  SD_MMC_ERROR_DATA_CRC;
                goto error;
                break;
        case IO_RW_EXTENDED:
                if (arg & (1<<31)) {
                        if (!status_irq_reg->data_write_crc16_ok)
                                ret =  SD_MMC_ERROR_DATA_CRC;
                        goto error;
                }
                else {
                        if (!sdio_read_crc_close) {
                                if (!status_irq_reg->data_read_crc16_ok)
                                        ret =  SD_MMC_ERROR_DATA_CRC;
                                goto error;
                        }
                }
                break;
        default:
                break;

        }
        /*error need dma_unmap_single also*/

error:
        if (data_dma_from_device_addr) {
                dma_unmap_single(NULL, data_dma_from_device_addr, data_cnt, DMA_FROM_DEVICE);
        }
        if (data_dma_to_device_addr) {
                dma_unmap_single(NULL, data_dma_to_device_addr, data_cnt, DMA_TO_DEVICE);
        }

        if (cmd_send_reg->res_with_data && buffer && (data_buf != sd_mmc_buf)) {
                memcpy(data_buf, sd_mmc_buf, data_cnt);
        }

       // return SD_MMC_NO_ERROR;
		
        return ret;
}
#define SDIO_DDR_MAX_LEN	512
int sdio_read_ddr(unsigned long sdio_addr, unsigned long byte_count, unsigned char *data_buf)
{

        printk("sdio_read_ddr %x,%x,%x \n",sdio_addr,byte_count,data_buf);
        int error = 0,ret;
        unsigned char response[MAX_RESPONSE_BYTES];
        unsigned long read_byte_count, data_offset = 0;
        unsigned long sdio_extend_rw = 0;
        SDIO_IO_RW_EXTENDED_ARG *sdio_io_extend_rw = (void *)&sdio_extend_rw;
        SDIO_WR_CMD cmd;

        byte_count = ALIGN(byte_count,4);

        HAL_BEGIN_LOCK();
        while (byte_count) {
read_dataretry:
                if (byte_count > SDIO_DDR_MAX_LEN)
                        read_byte_count = SDIO_DDR_MAX_LEN;
                else
                        read_byte_count = byte_count;
                /*
                (3) command53 for config
                (command argument = 32'h1400_0006:    RW_flag=0, FunctionNum=3'h1, Block_mode=0, Op_code=1,  Register_Address=17'h0,  ByteCount=9'd6)
                ---> wait response R5 from slave
                ---> config data transfer (6 bytes for configure, byte3~byte0 is base address of desired transfer(in fact, only 28-bit is used),
                {byte5[1:0], byte4[7:0]} is 10-bit ddr transfer length(in bytes),  byte5[4] is ddr read/write flag of desired ddr transfer, byte5[5] is ddr transfer enable bit.)
                */

                sdio_io_extend_rw->R_W_Flag = SDIO_Write_Data;
                sdio_io_extend_rw->Block_Mode = SDIO_Byte_MODE;
                sdio_io_extend_rw->OP_Code = 1;
                sdio_io_extend_rw->Function_No = 1;
                sdio_io_extend_rw->Byte_Block_Count = SDIO_WRITE_CMD_LEN;
                sdio_io_extend_rw->Register_Address = 0;
                cmd.addr= sdio_addr+data_offset;
                cmd.len_flag= read_byte_count|SDIO_DDR_EN|SDIO_READ_CMD_FLAG;
                memcpy(sd_mmc_buf,&cmd,6);
                //  Dump(sd_mmc_buf,6);
                //  printk("sdio_read_ddr sdio_addr = %x read_byte_count = %x\n",sdio_addr,read_byte_count);
                ret = sd_send_cmd_hw(IO_RW_EXTENDED, sdio_extend_rw, RESPONSE_R5, response, sd_mmc_buf, SDIO_WRITE_CMD_DATA_LEN, 0);
                if (ret != SD_MMC_NO_ERROR) {
                        //Dump(response,MAX_RESPONSE_BYTES);
                        printk("sd_send_cmd_hw IO_RW_EXTENDED error!!!sdio_addr = %x read_byte_count = %x\n",sdio_addr,read_byte_count);
                        //printk("#%s error occured in sdio_read_ddr()\n", sd_error_to_string(ret));
                        //goto read_dataretry;
        							  return -1;

                }

                /*
                	(command argument is different from step(3):
                	RW_flag=0, FunctionNum=3'h1, Block_mode=0, Op_code=0,  Register_Address=17'h7,
                	ByteCount is ddr transfer length, max is 256byte)
                */
                sdio_io_extend_rw->R_W_Flag = SDIO_Read_Data;
                sdio_io_extend_rw->Block_Mode = SDIO_Byte_MODE;
                sdio_io_extend_rw->OP_Code = 0;
                sdio_io_extend_rw->Function_No = 1;
                sdio_io_extend_rw->Byte_Block_Count = read_byte_count;
                sdio_io_extend_rw->Register_Address = 7;
                memset(sd_mmc_buf,0,SDIO_DDR_MAX_LEN);
                ret = sd_send_cmd_hw(IO_RW_EXTENDED, sdio_extend_rw, RESPONSE_R5, response,data_buf+data_offset, read_byte_count, 0);
                if (ret != SD_MMC_NO_ERROR) {
                        //printk("sd_send_cmd_hw IO_RW_EXTENDED2 error!!!sdio_addr = %x data_offset = %x\n",sdio_addr,data_offset);
                        printk("#%s error occured in sdio_read_ddr()\n", sd_error_to_string(ret));
                        //break;
                        //Dump(response,16);
                        //goto read_dataretry;
        								HAL_END_LOCK()	;
        								return -1;
                }
                data_offset += read_byte_count;
                byte_count -= read_byte_count;
        }
        HAL_END_LOCK()	;
        // printk("dump read data  \n");
        //Dump(data_buf,byte_count);
        return 0;
}


int sdio_write_ddr(unsigned long sdio_addr, unsigned long byte_count, unsigned char *data_buf)
{
        int ret;
        unsigned char response[MAX_RESPONSE_BYTES];
        unsigned long write_byte_count, data_offset = 0;
        unsigned long sdio_extend_rw = 0;
        SDIO_IO_RW_EXTENDED_ARG *sdio_io_extend_rw = (void *)&sdio_extend_rw;
        SDIO_WR_CMD cmd;

        // printk("sdio_write_ddr %x,%x,%x \n",sdio_addr,byte_count,data_buf);
        byte_count = ALIGN(byte_count,4);

        HAL_BEGIN_LOCK();
        while (byte_count) {
write_dataretry:
                if (byte_count > SDIO_DDR_MAX_LEN) {
                        write_byte_count = SDIO_DDR_MAX_LEN;
                }
                else        {
                        write_byte_count = byte_count;
                }
                /*
                command53, IO_RW_EXTENDED,
                {2'b01, 6'd53,  RW_flag(1bit),  Function_Num[2:0],  Block_mode(1bit),  OP_code(1bit),
                Register_Address[16:0],  ByteCount[8:0],   crc[6:0],  1'b1}
                */
                /*---> command53 (RW_flag=1（1表示写）, FunctionNum=3'h1, Block_mode=0, Op_code=1,  Register_Address=17'h0,  ByteCount=9'd6)  */

                sdio_io_extend_rw->R_W_Flag = SDIO_Write_Data;
                sdio_io_extend_rw->Block_Mode = SDIO_Byte_MODE;
                sdio_io_extend_rw->OP_Code = 1;
                sdio_io_extend_rw->Function_No = 1;
                sdio_io_extend_rw->Byte_Block_Count = SDIO_WRITE_CMD_LEN;
                sdio_io_extend_rw->Register_Address = 0;
                cmd.addr= sdio_addr+data_offset;
                cmd.len_flag= write_byte_count|SDIO_DDR_EN|SDIO_WRITE_CMD_FLAG;
                memcpy(sd_mmc_buf,&cmd,6);
                //Dump(sd_mmc_buf,6);
                // printk("sdio_write_ddr %x,%x,%x \n",sdio_addr,byte_count,data_buf);
                ret = sd_send_cmd_hw(IO_RW_EXTENDED, sdio_extend_rw, RESPONSE_R5, response, sd_mmc_buf, SDIO_WRITE_CMD_DATA_LEN, 0);
                if (ret != SD_MMC_NO_ERROR)  {
                        Dump(response,MAX_RESPONSE_BYTES);
                        printk("sd_send_cmd_hw IO_RW_EXTENDED error!!!\n");
                        printk("#%s error occured in sdio_write_ddr()\n", sd_error_to_string(ret));
                        goto write_dataretry;
                }
                sdio_io_extend_rw->R_W_Flag = SDIO_Write_Data;
                sdio_io_extend_rw->Block_Mode = SDIO_Byte_MODE;
                sdio_io_extend_rw->OP_Code = 0;
                sdio_io_extend_rw->Function_No = 1;
                sdio_io_extend_rw->Byte_Block_Count = write_byte_count;
                sdio_io_extend_rw->Register_Address = 6;
                memcpy(sd_mmc_buf,data_buf+data_offset,write_byte_count);
                //printk("sdio_write_ddr %x,%x,%x \n",sdio_addr,byte_count,data_buf);
                //Dump(data_buf,32);
                //  Dump(sd_mmc_buf,32);
                ret = sd_send_cmd_hw(IO_RW_EXTENDED, sdio_extend_rw, RESPONSE_R5, response,sd_mmc_buf, write_byte_count, 0);
                if (ret != SD_MMC_NO_ERROR) {
                        printk("sd_send_cmd_hw IO_RW_EXTENDED error!!!\n");
                        printk("#%s error occured in sdio_write_ddr()\n", sd_error_to_string(ret));
                        goto write_dataretry;
                }
                data_offset += write_byte_count;
                byte_count -= write_byte_count;
        }
        HAL_END_LOCK()	;
        return SD_MMC_NO_ERROR;
}

int sdio_read_sram(unsigned char *data_buf,unsigned long sdio_addr, unsigned long byte_count)
{

        int error = 0,ret;
        unsigned char response[MAX_RESPONSE_BYTES];
        unsigned long read_byte_count, data_offset = 0;
        unsigned long sdio_extend_rw = 0;
        SDIO_IO_RW_EXTENDED_ARG *sdio_io_extend_rw = (void *)&sdio_extend_rw;
        SDIO_WR_CMD cmd;

        byte_count = ALIGN(byte_count,4);
        HAL_BEGIN_LOCK();
        //printk("sdio_read_sram %x,%x,%x \n",sdio_addr,byte_count,data_buf);
        while (byte_count) {
read_dataretry:
                if (byte_count > SDIO_DDR_MAX_LEN)
                        read_byte_count = SDIO_DDR_MAX_LEN;
                else
                        read_byte_count = byte_count;
                /*
                (3) command53 for config
                (command argument = 32'h1400_0006:    RW_flag=0, FunctionNum=3'h1, Block_mode=0, Op_code=1,  Register_Address=17'h0,  ByteCount=9'd6)
                ---> wait response R5 from slave
                ---> config data transfer (6 bytes for configure, byte3~byte0 is base address of desired transfer(in fact, only 28-bit is used),
                {byte5[1:0], byte4[7:0]} is 10-bit ddr transfer length(in bytes),  byte5[4] is ddr read/write flag of desired ddr transfer, byte5[5] is ddr transfer enable bit.)
                */

                sdio_io_extend_rw->R_W_Flag = SDIO_Write_Data;
                sdio_io_extend_rw->Block_Mode = SDIO_Byte_MODE;
                sdio_io_extend_rw->OP_Code = 1;
                sdio_io_extend_rw->Function_No = 1;
                sdio_io_extend_rw->Byte_Block_Count = SDIO_WRITE_CMD_LEN;
                sdio_io_extend_rw->Register_Address = 0;
                cmd.addr= sdio_addr+data_offset;
                cmd.len_flag= read_byte_count|SDIO_SRAM_EN|SDIO_READ_CMD_FLAG;
                memcpy(sd_mmc_buf,&cmd,6);
                //Dump(sd_mmc_buf,6);
                ret = sd_send_cmd_hw(IO_RW_EXTENDED, sdio_extend_rw, RESPONSE_R5, response, sd_mmc_buf, SDIO_WRITE_CMD_DATA_LEN, 0);
                if (ret != SD_MMC_NO_ERROR) {
                        Dump(response,MAX_RESPONSE_BYTES);
                        printk("sd_send_cmd_hw IO_RW_EXTENDED error!!!\n");
                        printk("#%s error occured in sdio_read_sram()\n", sd_error_to_string(ret));
                        //goto read_dataretry;
                        break;

                }

                /*
                	(command argument is different from step(3):
                	RW_flag=0, FunctionNum=3'h1, Block_mode=0, Op_code=0,  Register_Address=17'h7,
                	ByteCount is ddr transfer length, max is 256byte)
                */
                sdio_io_extend_rw->R_W_Flag = SDIO_Read_Data;
                sdio_io_extend_rw->Block_Mode = SDIO_Byte_MODE;
                sdio_io_extend_rw->OP_Code = 0;
                sdio_io_extend_rw->Function_No = 1;
                sdio_io_extend_rw->Byte_Block_Count = read_byte_count;
                sdio_io_extend_rw->Register_Address = 7;
                ret = sd_send_cmd_hw(IO_RW_EXTENDED, sdio_extend_rw, RESPONSE_R5, response,data_buf+data_offset, read_byte_count, 0);
                if (ret != SD_MMC_NO_ERROR) {
                        printk("sd_send_cmd_hw IO_RW_EXTENDED error!!!\n");
                        printk("#%s error occured in sdio_read_sram()\n", sd_error_to_string(ret));
                        Dump(response,16);
                        //goto read_dataretry;
        				HAL_END_LOCK()	;
        				return -1;
                }
                data_offset += read_byte_count;
                byte_count -= read_byte_count;
        }
        HAL_END_LOCK()	;
        // printk("dump read data  \n");
        //Dump(data_buf,byte_count);
        return SD_MMC_NO_ERROR;
}


int sdio_write_sram(unsigned char *data_buf,unsigned long sdio_addr, unsigned long byte_count)
{
        int ret;
        unsigned char response[MAX_RESPONSE_BYTES];
        unsigned long write_byte_count, data_offset = 0;
        unsigned long sdio_extend_rw = 0;
        SDIO_IO_RW_EXTENDED_ARG *sdio_io_extend_rw = (void *)&sdio_extend_rw;
        SDIO_WR_CMD cmd;
        byte_count = ALIGN(byte_count,4);
//        printk("sdio_write_sram %x,%x,%x \n",sdio_addr,byte_count,data_buf);
        HAL_BEGIN_LOCK();
        while (byte_count) {
write_dataretry:
                if (byte_count > SDIO_DDR_MAX_LEN)
                        write_byte_count = SDIO_DDR_MAX_LEN;
                else
                        write_byte_count = byte_count;
                /*
                command53, IO_RW_EXTENDED,
                {2'b01, 6'd53,  RW_flag(1bit),  Function_Num[2:0],  Block_mode(1bit),  OP_code(1bit),
                Register_Address[16:0],  ByteCount[8:0],   crc[6:0],  1'b1}
                */
                /*---> command53 (RW_flag=1（1表示写）, FunctionNum=3'h1, Block_mode=0, Op_code=1,  Register_Address=17'h0,  ByteCount=9'd6)  */

                sdio_io_extend_rw->R_W_Flag = SDIO_Write_Data;
                sdio_io_extend_rw->Block_Mode = SDIO_Byte_MODE;
                sdio_io_extend_rw->OP_Code = 1;
                sdio_io_extend_rw->Function_No = 1;
                sdio_io_extend_rw->Byte_Block_Count = SDIO_WRITE_CMD_LEN;
                sdio_io_extend_rw->Register_Address = 0;
                cmd.addr= sdio_addr+data_offset;
                cmd.len_flag= write_byte_count|SDIO_SRAM_EN|SDIO_WRITE_CMD_FLAG;
                memcpy(sd_mmc_buf,&cmd,6);
                // Dump(sd_mmc_buf,6);
                ret = sd_send_cmd_hw(IO_RW_EXTENDED, sdio_extend_rw, RESPONSE_R5, response, sd_mmc_buf, SDIO_WRITE_CMD_DATA_LEN, 0);
                if (ret != SD_MMC_NO_ERROR) {
                        Dump(response,MAX_RESPONSE_BYTES);
                        printk("sd_send_cmd_hw IO_RW_EXTENDED error!!!\n");
                        printk("#%s error occured in sdio_write_sram()\n", sd_error_to_string(ret));
                        //goto write_dataretry;
                        break;
                }
                sdio_io_extend_rw->R_W_Flag = SDIO_Write_Data;
                sdio_io_extend_rw->Block_Mode = SDIO_Byte_MODE;
                sdio_io_extend_rw->OP_Code = 0;
                sdio_io_extend_rw->Function_No = 1;
                sdio_io_extend_rw->Byte_Block_Count = write_byte_count;
                sdio_io_extend_rw->Register_Address = 6;
                memcpy(sd_mmc_buf,data_buf+data_offset,write_byte_count);
                //Dump(sd_mmc_buf,6);
                ret = sd_send_cmd_hw(IO_RW_EXTENDED, sdio_extend_rw, RESPONSE_R5, response,sd_mmc_buf, write_byte_count, 0);
                if (ret != SD_MMC_NO_ERROR) {
                        printk("sd_send_cmd_hw IO_RW_EXTENDED error!!!\n");
                        printk("#%s error occured in sdio_write_sram()\n", sd_error_to_string(ret));
                        //goto write_dataretry;
                        break;
                        
                }
                data_offset += write_byte_count;
                byte_count -= write_byte_count;
        }
        HAL_END_LOCK()	;
        return SD_MMC_NO_ERROR;
}
int sdio_data_transfer_abort(int function_no)
{
        printk("---------------------------------------\n");
        printk("---------sdio_data_transfer_abort---------------\n");
        printk("---------------------------------------\n");
        return SD_MMC_NO_ERROR;
}
unsigned long  sdio_Read_SRAM32(unsigned long sram_addr )
{
        unsigned long sramdata;
        sdio_read_sram((unsigned char *)&sramdata,sram_addr, 4);
        return sramdata;
}
void   sdio_Write_SRAM32(unsigned long sram_addr, unsigned long sramdata)
{
//        printk("sdio_Write_SRAM32 %x,%x \n",sram_addr,sramdata);
        sdio_write_sram(&sramdata,sram_addr,4);
}


