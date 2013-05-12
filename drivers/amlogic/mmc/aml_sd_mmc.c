/*******************************************************************
 * 
 * Copyright C 2005 by Amlogic, Inc. All Rights Reserved.
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/clk.h>
#include <linux/mmc/host.h>
#include <linux/mmc/core.h>
#include <linux/io.h>
#include <linux/semaphore.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/sd.h>
#include <linux/mmc/card.h>
#include <linux/genhd.h>
#include <mach/hardware.h>
#include <plat/regops.h>
//#include <plat/mmc.h>
//#include <plat/cpu.h>
#include <linux/slab.h>
#include <mach/mmc.h>
#include <asm/cacheflush.h>
#include <asm/dma-mapping.h>
#include <asm/outercache.h>
#include <mach/power_gate.h>

//#define dbg_print(a...) printk(a);
#define dbg_print(a...) 
//#define DBG_LINE_INFO()  printk(KERN_INFO "[%s] : %s\n",__func__,__FILE__);
#define DBG_LINE_INFO()  
#define dev_err(a,s) printk(KERN_INFO s);


#define AML_MMC_DISABLED_TIMEOUT	100
#define AML_MMC_SLEEP_TIMEOUT		1000
#define AML_MMC_OFF_TIMEOUT 8000


#define MMC_TIMEOUT_MS		20
#define DRIVER_NAME		"aml_sd_mmc"


/**
 * enable IRQ, after having disabled it.
 * @host: The device state.
 * @more: True if more IRQs are expected from transfer.
 *
 * Enable the main IRQ if needed after it has been disabled.
 *
 * The IRQ can be one of the following states:
 *	- disabled during IDLE
 *	- disabled whilst processing data
 *	- enabled during transfer
 *	- enabled whilst awaiting SDIO interrupt detection
 */
#if 0
static void aml_sd_enable_irq(struct aml_sd_host *host, bool more)
{
	unsigned long flags;
	bool enable = false;

	local_irq_save(flags);

	host->irq_enabled = more;
	host->irq_disabled = false;

	enable = more | host->sdio_irqen;

	if (host->irq_state != enable) {
		host->irq_state = enable;

		if (enable)
			enable_irq(host->irq);
		else
			disable_irq(host->irq);
	}

	local_irq_restore(flags);
}

static void aml_sd_disable_irq(struct aml_sd_host *host, bool transfer)
{
	unsigned long flags;
	DBG_LINE_INFO();
	local_irq_save(flags);

	host->irq_disabled = transfer;

	if (transfer && host->irq_state) {
		host->irq_state = false;
		disable_irq(host->irq);
	}

	local_irq_restore(flags);
}

/**
 * test whether the SDIO IRQ is being signalled
 * @host: The host to check.
 *
 * Test to see if the SDIO interrupt is being signalled in case the
 * controller has failed to re-detect a card interrupt. Read GPE8 and
 * see if it is low and if so, signal a SDIO interrupt.
 *
 * This is currently called if a request is finished (we assume that the
 * bus is now idle) and when the SDIO IRQ is enabled in case the IRQ is
 * already being indicated.
*/
static void aml_sd_check_sdio_irq(struct aml_sd_host *host)
{
//	DBG_LINE_INFO();
	if (host->sdio_irqen) {
	//	if (gpio_get_value(S3C2410_GPE(8)) == 0) {
	//		printk(KERN_DEBUG "%s: signalling irq\n", __func__);
			mmc_signal_sdio_irq(host->mmc);
	//	}
	}
}
#endif

#ifdef CONFIG_PM

static int aml_mmc_suspend(struct platform_device *pdev)
{
	struct mmc_host *mmc = platform_get_drvdata(pdev);
	printk(KERN_INFO 	"***Entered %s:%s\n", __FILE__,__func__);
//	CLK_GATE_OFF(SDIO);
	return mmc_suspend_host(mmc);
}

static int aml_mmc_resume(struct platform_device *pdev)
{
	struct mmc_host *mmc = platform_get_drvdata(pdev);
	//clock gate on
	printk(KERN_INFO 	"***Entered %s:%s\n", __FILE__,__func__);
//	CLK_GATE_ON(SDIO);
	return mmc_resume_host(mmc);
}

#else

#define aml_mmc_suspend	NULL
#define aml_mmc_resume		NULL

#endif


#if 0
#ifdef CONFIG_PM
/*
 * Restore the MMC host context, if it was lost as result of a
 * power state change.
 */
static int aml_sd_context_restore(struct aml_sd_host *host)
{
	struct mmc_ios *ios = &host->mmc->ios;
	struct aml_mmc_platform_data *pdata = host->pdata;

	return 0;
}

/*
 * Save the MMC host context (store the number of power state changes so far).
 */
static void aml_sd_context_save(struct aml_sd_host *host)
{
#if 0
	struct aml_mmc_platform_data *pdata = host->pdata;
	int context_loss;
	if (pdata->get_context_loss_count) {
		context_loss = pdata->get_context_loss_count(host->dev);
		if (context_loss < 0)
			return;
		host->context_loss = context_loss;
	}
#endif	
}

#else

static int aml_sd_context_restore(struct aml_sd_host *host)
{
	return 0;
}

static void aml_sd_context_save(struct aml_sd_host *host)
{
}

#endif

#endif

/*
 * Configure the response type and send the cmd.
 */
//
static int aml_mmc_check_response(struct mmc_command *cmd)
{
//	DBG_LINE_INFO();

	int ret = (int)SD_NO_ERROR;
	SD_Response_R1_t * r1 = (SD_Response_R1_t*)cmd->resp;
	
	if(cmd->opcode == SD_SEND_IF_COND ||
		cmd->opcode == SD_SEND_RELATIVE_ADDR)
		return ret;
		
	switch (mmc_resp_type(cmd)) {
	case MMC_RSP_R1:
	case MMC_RSP_R1B:
		if (r1->card_status.OUT_OF_RANGE)
			return SD_ERROR_OUT_OF_RANGE;
		else if (r1->card_status.ADDRESS_ERROR)
			return SD_ERROR_ADDRESS;
		else if (r1->card_status.BLOCK_LEN_ERROR)
			return SD_ERROR_BLOCK_LEN;
		else if (r1->card_status.ERASE_SEQ_ERROR)
			return SD_ERROR_ERASE_SEQ;
		else if (r1->card_status.ERASE_PARAM)
			return SD_ERROR_ERASE_PARAM;
		else if (r1->card_status.WP_VIOLATION)
			return SD_ERROR_WP_VIOLATION;
		else if (r1->card_status.CARD_IS_LOCKED)
			return SD_ERROR_CARD_IS_LOCKED;
		else if (r1->card_status.LOCK_UNLOCK_FAILED)
			return SD_ERROR_LOCK_UNLOCK_FAILED;
		else if (r1->card_status.COM_CRC_ERROR)
			return SD_ERROR_COM_CRC;
		else if (r1->card_status.ILLEGAL_COMMAND)
			return SD_ERROR_ILLEGAL_COMMAND;
		else if (r1->card_status.CARD_ECC_FAILED)
			return SD_ERROR_CARD_ECC_FAILED;
		else if (r1->card_status.CC_ERROR)
			return SD_ERROR_CC;
		else if (r1->card_status.ERROR)
			return SD_ERROR_GENERAL;
		else if (r1->card_status.CID_CSD_OVERWRITE)
				return SD_ERROR_CID_CSD_OVERWRITE;
		else if (r1->card_status.AKE_SEQ_ERROR)
			if(cmd->opcode != SD_SEND_IF_COND)
				return SD_ERROR_AKE_SEQ;
		break;
	default:
		break;
	}
	return ret;
}

static int mmc_wait_busy(unsigned timeout)
{
    unsigned r;
 //   unsigned timebase=gettimer(0);
  	int timercount = timeout;
  	while(timercount > 0)  
     {
        r = aml_read_reg32(P_SDIO_STATUS_IRQ);
        if(((r >> sdio_cmd_busy_bit) & 0x1) == 0)
        {
            return 0;
        }
        timercount --;
				udelay(8);
    }
    return -1;
}

static void mmc_soft_reset(void)
{
   aml_write_reg32(P_SDIO_IRQ_CONFIG,aml_read_reg32(P_SDIO_IRQ_CONFIG)|(1<<soft_reset_bit));
   aml_write_reg32(P_SDIO_STATUS_IRQ,((1<<8) | (1<<9)));
   udelay(10);
}

static int
aml_sd_send_command(struct aml_sd_host *host, struct mmc_command *cmd)
{
//	DBG_LINE_INFO();
	struct mmc_host* mmc = host->mmc;
	int ret = ERROR_NONE, num_res;
	unsigned int cmd_send = 0;
	SDHW_CMD_Send_Reg_t *cmd_send_reg = (void *)&cmd_send;
	unsigned int cmd_ext = 0;
	SDHW_Extension_Reg_t *cmd_ext_reg = (void *)&cmd_ext;
	unsigned int status_irq = 0;
	SDIO_Status_IRQ_Reg_t *status_irq_reg = (void *)&status_irq;
	struct mmc_data* data = cmd->data;
	bool bread_command = 0;
	unsigned int timeout;
	unsigned int data_temp;
	unsigned int loop_num;
	unsigned char respone[18];

	dbg_print("[cmd: %d ]\n",cmd->opcode);

  cmd_send_reg->cmd_data = 0x40 | cmd->opcode;
	cmd_send_reg->use_int_window = 1;
	
	/* check read/write address overflow */
	switch (cmd->opcode) {
	case MMC_READ_SINGLE_BLOCK:
  case MMC_READ_MULTIPLE_BLOCK:
	case MMC_WRITE_BLOCK:
	case MMC_WRITE_MULTIPLE_BLOCK:
		{
			unsigned capacity = 0;
			struct mmc_card* card = mmc->card;	
			if(card){
				capacity =	card->csd.capacity << (card->csd.read_blkbits - 9);	
				ret = (capacity * data->blksz <= cmd->arg);
				dbg_print("mmc->ocr:%d capacity:%x data->blksz:%d  cmd->arg:%x  ret:%d\n",
																		mmc->ocr,capacity,data->blksz,cmd->arg,ret);
				if(ret){
					printk(KERN_INFO "!!! please check capacity. capacity:%x  cmd->arg:%x\n",capacity,cmd->arg);
					return -1;
				}
			}
		}
		break;
	}
	
	switch (mmc_resp_type(cmd)) {
	case MMC_RSP_R1:
	case MMC_RSP_R1B:
	case MMC_RSP_R3:
		cmd_send_reg->cmd_res_bits = 45;		// RESPONSE	have 7(cmd)+32(respnse)+7(crc)-1 data
		break;
	case MMC_RSP_R2:
		cmd_send_reg->cmd_res_bits = 133;		// RESPONSE	have 7(cmd)+120(respnse)+7(crc)-1 data
		cmd_send_reg->res_crc7_from_8 = 1;
		break;
	default:
		cmd_send_reg->cmd_res_bits = 0;			// NO_RESPONSE
		break;
	}

	switch (cmd->opcode) {
	case MMC_READ_SINGLE_BLOCK:
	case MMC_READ_MULTIPLE_BLOCK:
	case MMC_SEND_EXT_CSD:					//same as: SD_CMD_SEND_IF_COND
	case MMC_SWITCH:					//same as: MMC_CMD_SWITCH
		if(!data)
			break;
#if 0
		{
				int j;
	//			char*	pv = sg_virt(data->sg);
	//			for(j = 0; j < data->sg->length; j++){
	//				pv[j] = 3;
	//			}
		}
#endif
		
		cmd_send_reg->res_with_data = 1;
		cmd_send_reg->repeat_package_times = data->blocks - 1;
		 	if(host->bus_width == SD_BUS_WIDE)
				cmd_ext_reg->data_rw_number = data->blksz * 8 + (16 - 1) * 4;
		else
				cmd_ext_reg->data_rw_number = data->blksz * 8 + 16 - 1;
		bread_command = 1;
		dma_sync_sg_for_device(host->dev,data->sg,data->sg_len,DMA_FROM_DEVICE);

		break;
	case MMC_WRITE_BLOCK:
	case MMC_WRITE_MULTIPLE_BLOCK:
	   if(!data)
	   	break;
		  cmd_send_reg->cmd_send_data = 1;
		  cmd_send_reg->repeat_package_times = data->blocks - 1;
		 	if(host->bus_width == SD_BUS_WIDE)
				cmd_ext_reg->data_rw_number = data->blksz * 8 + (16 - 1) * 4;
			else
				cmd_ext_reg->data_rw_number = data->blksz * 8 + 16 - 1;

			dma_sync_sg_for_device(host->dev,data->sg,data->sg_len,DMA_TO_DEVICE);
			
//			dcache_clean_range(buffer,data->blocks<<9);
			
 		break;
		case SD_APP_SEND_SCR:
			if(!data)
				break;
				bread_command = 1;
	  	cmd_send_reg->res_with_data = 1;
		 	if(host->bus_width == SD_BUS_WIDE)
				cmd_ext_reg->data_rw_number = data->blksz * 8 + (16 - 1) * 4;
			else
				cmd_ext_reg->data_rw_number = data->blksz * 8 + 16 - 1;
 			dma_sync_sg_for_device(host->dev,data->sg,data->sg_len,DMA_FROM_DEVICE);
		break;
	default:
		
		break;
	}
	
	//cmd with R1b
	switch (cmd->opcode) {
	case MMC_STOP_TRANSMISSION:
		cmd_send_reg->check_dat0_busy =	1;
		break;
	default:
		break;
	}

	//cmd with R3
	switch (cmd->opcode) {
	case MMC_SEND_OP_COND:
	case SD_APP_OP_COND:
		cmd_send_reg->res_without_crc7 = 1;
//		cmd_ext = 0x8300;
		break;
	default:
		break;
	}
	
	timeout = TIMEOUT_SHORT;
	if(data && data->timeout_ns > 10000)
		timeout = data->timeout_ns /1000;

	if(data && data->sg){
		int i;
		struct scatterlist *sg;
		int nsgs = 0;
		unsigned address = cmd->arg;
		unsigned status_bk = status_irq;
		unsigned cmd_ext_bk = cmd_ext;
		unsigned cmd_send_bk = cmd_send;
		unsigned buffer;
		for_each_sg(data->sg, sg, data->sg_len, i){
			cmd_send = cmd_send_bk;
			cmd_ext = cmd_ext_bk;
			status_irq = status_bk;
			
			nsgs++;
			cmd_send_reg->repeat_package_times = (sg->length-1)/data->blksz;
			buffer = sg_phys(sg);
			
			status_irq_reg->if_int = 1;
			status_irq_reg->cmd_int = 1;
			dbg_print("P_SDIO_STATUS_IRQ=%x P_SDIO_M_ADDR=%x P_CMD_SEND=%x\n",status_irq,buffer,cmd_send);
			dbg_print("P_CMD_ARGUMENT =%x      P_SDIO_EXTENSION=%x\n",cmd->arg,cmd_ext);
		
			dbg_print("address:%x[%x]  length:%x blocks:%d \n",address,address * data->blksz,sg->length, cmd_send_reg->repeat_package_times);
			aml_write_reg32(P_SDIO_STATUS_IRQ,status_irq|(0x1fff<<19));
			aml_write_reg32(P_SDIO_EXTENSION,cmd_ext);
			aml_write_reg32(P_SDIO_M_ADDR,(unsigned int)buffer);
			aml_write_reg32(P_CMD_ARGUMENT,address);
			aml_write_reg32(P_CMD_SEND,cmd_send);
			
			if(mmc_wait_busy(timeout) !=0){
				printk(KERN_INFO "[mmc cmd %d] time out!\n",cmd->opcode);
				return SD_ERROR_TIMEOUT;
			}
			address += cmd_send_reg->repeat_package_times+1;
			//send command stop
			if(data->sg_len > 1 && nsgs < data->sg_len){
					aml_write_reg32(P_SDIO_STATUS_IRQ,status_irq|(0x1fff<<19));
					aml_write_reg32(P_SDIO_EXTENSION,0);
					aml_write_reg32(P_SDIO_M_ADDR,0);
					aml_write_reg32(P_CMD_ARGUMENT,0);
					aml_write_reg32(P_CMD_SEND,0x282d4c);
					if(mmc_wait_busy(TIMEOUT_SHORT) !=0){
						printk(KERN_INFO"[mmc cmd %d] time out!\n",MMC_STOP_TRANSMISSION);
						return SD_ERROR_TIMEOUT;
					}//wait
			}//if
		}//for each sg
	}//if
	else{
		status_irq_reg->if_int = 1;
		status_irq_reg->cmd_int = 1;
		dbg_print("P_SDIO_STATUS_IRQ=%x P_SDIO_M_ADDR=%x P_CMD_SEND=%x\n",status_irq,0,cmd_send);
		dbg_print("P_CMD_ARGUMENT =%x      P_SDIO_EXTENSION=%x\n",cmd->arg,cmd_ext);
	
		aml_write_reg32(P_SDIO_STATUS_IRQ,status_irq|(0x1fff<<19));
		   
		aml_write_reg32(P_SDIO_EXTENSION,cmd_ext);
		aml_write_reg32(P_SDIO_M_ADDR,0);
		aml_write_reg32(P_CMD_ARGUMENT,cmd->arg);
		aml_write_reg32(P_CMD_SEND,cmd_send);
		
		if(mmc_wait_busy(timeout) !=0){
			printk(KERN_INFO "[mmc cmd %d] time out!\n",cmd->opcode);
			return SD_ERROR_TIMEOUT;
		}
	}
	
	
	status_irq = aml_read_reg32(P_SDIO_STATUS_IRQ);
	if(cmd_send_reg->cmd_res_bits && !cmd_send_reg->res_without_crc7 && !status_irq_reg->res_crc7_ok)
		return SD_ERROR_COM_CRC;

	switch (mmc_resp_type(cmd)) {
	case MMC_RSP_R1:
	case MMC_RSP_R1B:
	case MMC_RSP_R3:
		num_res = RESPONSE_R1_R3_R6_R7_LENGTH;
		break;
	case MMC_RSP_R2:
		num_res = RESPONSE_R2_CID_CSD_LENGTH;
		break;
	default:
		num_res = RESPONSE_R4_R5_NONE_LENGTH;
		break;
	 }

	if (num_res > 0) {
		unsigned int multi_config = 0;
		SDIO_Multi_Config_Reg_t *multi_config_reg = (void *)&multi_config;
		multi_config_reg->write_read_out_index = 1;
		aml_write_reg32(P_SDIO_MULT_CONFIG,multi_config);
		num_res--;							// Minus CRC byte
	}

	loop_num = (num_res + 3 - 1) /4;
	while (num_res > 0) {
		data_temp = aml_read_reg32(P_CMD_ARGUMENT);
		if(num_res <= 1)
			break;
		respone[--num_res - 1] = data_temp & 0xFF;
		if(num_res <= 1)
			break;
		respone[--num_res - 1] = (data_temp >> 8) & 0xFF;
		if(num_res <= 1)
			break;
		respone[--num_res - 1] = (data_temp >> 16) & 0xFF;
		if(num_res <= 1)
			break;
		respone[--num_res - 1] = (data_temp >> 24) & 0xFF;
	}
	
	while (loop_num--) {
		((uint *)cmd->resp)[loop_num] = __be32_to_cpu(((uint *)respone)[loop_num]);
		dbg_print("[mmc cmd:%d] response[%d]:%x\n",cmd->opcode,loop_num,((uint *)cmd->resp)[loop_num]);
	}
	
	//check_response
	ret = aml_mmc_check_response(cmd);
	if(ret)
	{
			printk(KERN_INFO "[mmc cmd:%d] check response failed!\n",cmd->opcode);
			return ret;
	}
	
	switch (cmd->opcode) {
	case MMC_READ_SINGLE_BLOCK:
	case MMC_READ_MULTIPLE_BLOCK:
	case MMC_SEND_EXT_CSD:							//same as SD_CMD_SEND_IF_COND
	case MMC_SWITCH:							//same as: MMC_CMD_SWITCH
		if(!data)
			break;
		if(!status_irq_reg->data_read_crc16_ok)
			return SD_ERROR_DATA_CRC;
		break;
	case MMC_WRITE_BLOCK:
	case MMC_WRITE_MULTIPLE_BLOCK:
		if(!status_irq_reg->data_write_crc16_ok)
			return SD_ERROR_DATA_CRC;
		break;
	case SD_APP_SEND_SCR:
		if(!status_irq_reg->data_read_crc16_ok)
			return SD_ERROR_DATA_CRC;
		break;
	case SD_IO_SEND_OP_COND:
		break;
	case SD_APP_OP_COND:
		break;
	default:
		break;
	}

#if 0
	if(mmc_resp_type(cmd) == MMC_RSP_R2){
		if(cmd->resp[0]&1)
			host->locked = 1;
		else
			host->locked = 0;
		dbg_print("sdcard:locked:%d\n",host->locked);
	}
#endif

	if(data && data->sg && bread_command == 1){
		dma_sync_sg_for_cpu(host->dev,data->sg,data->sg_len,DMA_FROM_DEVICE);
		
#if 0 //dump value
		{
		int i=0;
		struct scatterlist *sg;
		printk("[mmc cmd:%d] argument:%x\n",cmd->opcode,cmd->arg);
		for_each_sg(data->sg, sg, data->sg_len, i) {
				char*	pv = sg_virt(sg);
				int j;
				printk("virt_addr:%X  phys_addr:%X  :%d   length:%x\n",pv,sg_phys(sg),i, sg->length);
				for(j = 0; j < 32 && j < sg->length; j++){
					if((j % 16) == 0)
						printk("\n");
					printk("%.2x ",pv[j]);
				}
				printk("\n");
				
				if( i > 2)
					break;
			}//for each
		}
#endif
	}//if
	
	if(MMC_SEND_STATUS == cmd->opcode)
		mdelay(100);
		
//	printk("[cmd : %d ] run ok!\n",cmd->opcode);
	return SD_NO_ERROR;	
}


static inline bool aml_host_usedma(struct aml_sd_host *host)
{
//	DBG_LINE_INFO();

	return host->dodma;
}

#if 0
static irqreturn_t aml_sd_irq(int irq, void *dev_id)
{

	DBG_LINE_INFO();
	return IRQ_HANDLED;
}
#endif 
#if 0
/*
 * ISR for the CardDetect Pin
*/
static irqreturn_t aml_sd_irq_cd(int irq, void *dev_id)
{
	struct aml_sd_host *host = (struct aml_sd_host *)dev_id;
	DBG_LINE_INFO();
	mmc_detect_change(host->mmc, msecs_to_jiffies(500));
	return IRQ_HANDLED;
}
#endif
#if 0
/* Protect the card while the cover is open */
static void aml_sd_protect_card(struct aml_sd_host *host)
{
	DBG_LINE_INFO();

}

/*
 * ISR for handling card insertion and removal
 */
static irqreturn_t aml_sd_cd_handler(int irq, void *dev_id)
{
	DBG_LINE_INFO();
/*
	struct aml_sd_host *host = (struct aml_sd_host *)dev_id;

	if (host->suspended)
		return IRQ_HANDLED;
	schedule_work(&host->mmc_carddetect_work);
*/
	return IRQ_HANDLED;
}
#endif

int aml_sd_get_cd(struct mmc_host *mmc)
{
	struct aml_sd_host *host = mmc_priv(mmc);
	struct aml_mmc_platform_data * platform_data = host->pdata;
	int ret = 1;
	DBG_LINE_INFO();
	if(platform_data->sdio_detect)
		ret = platform_data->sdio_detect(host);
	dbg_print("sdio detect: %s.\n",ret ? "no inserted":"inserted");
	//need return 0: inserted. 1: no inserted
	return (ret?0:1);
}
/*
 * Configure block length for MMC/SD cards and initiate the transfer.
 */
static int
aml_sd_setup_data(struct aml_sd_host *host, struct mmc_data *data)
{
	unsigned int status_irq = 0;
	SDIO_Status_IRQ_Reg_t *status_irq_reg = (void *)&status_irq;
	unsigned int irq_config = 0;
	MSHW_IRQ_Config_Reg_t *irq_config_reg = (void *)&irq_config;   
	
	DBG_LINE_INFO();
	
	//u32 dcon, imsk, stoptries = 3;
	/* write DCON register */

	if (!data) {
		aml_write_reg32(P_SDIO_CONFIG,0);
		return 0;
	}
	
	if ((data->blksz & 3) != 0) {
		/* We cannot deal with unaligned blocks with more than
		 * one block being transfered. */
		if (data->blocks > 1) {
			pr_warning("%s: can't do non-word sized block transfers (blksz %d)\n", __func__, data->blksz);
			return -EINVAL;
		}
	}
	
	status_irq = aml_read_reg32(P_SDIO_STATUS_IRQ);

	while(status_irq_reg->cmd_busy && status_irq_reg->cmd_int){
		printk(KERN_INFO "%s() transfer still in progress.\n",__func__);
		
		//reset
		irq_config_reg->soft_reset = 1;
		aml_write_reg32(P_SDIO_IRQ_CONFIG,irq_config);
		udelay(1000);

		status_irq = aml_read_reg32(P_SDIO_STATUS_IRQ);
	}

	return 0;
}

void aml_mmc_goto_idle(void)
{
	unsigned cmd_send;
	mmc_soft_reset();
	aml_write_reg32(P_CMD_ARGUMENT,0x0);
  cmd_send =  ((0 << cmd_response_bits_bit) | // NO_RESPONSE
  		        (0x40 << cmd_command_bit));
 	aml_write_reg32(P_CMD_SEND,cmd_send);
  if (mmc_wait_busy(TIMEOUT_SHORT) == 0)
		printk(KERN_INFO "mmc goto idle ok!\n");
	else 
		printk(KERN_INFO "mmc goto idle failed!\n");
	mdelay(10);
}
/*
 * Request function. for read/write operation
 */
static void aml_sd_send_request(struct mmc_host *mmc)
{
	struct aml_sd_host *host = mmc_priv(mmc);
	struct mmc_request *mrq = host->mrq;
	struct mmc_command *cmd = host->cmd_is_stop ? mrq->stop : mrq->cmd;
	int res = 0;
//	DBG_LINE_INFO();

	host->ccnt++;

#if 1 
	//for SDIO detect.
	if(cmd->opcode == 5 || cmd->opcode == 52)
	{
		cmd->error = -1;
		host->complete_what = COMPLETION_FINALIZE;
		mmc_request_done(mmc, mrq);
		return;
	}
#endif
	if (cmd->data) {
		res = aml_sd_setup_data(host, cmd->data);

		host->dcnt++;

		if (res) {
			cmd->error = res;
			cmd->data->error = res;
			mmc_request_done(mmc, mrq);
			return;
		}

		if (res) {
			cmd->error = res;
			cmd->data->error = res;

			mmc_request_done(mmc, mrq);
			return;
		}
	}

	/* Send command */
	res = aml_sd_send_command(host, cmd);
	cmd->error = res;	
	if (cmd->data && cmd->error)
		cmd->data->error = cmd->error;
	if(mrq->data){
		if (mrq->data->error == 0)
			mrq->data->bytes_xfered =(mrq->data->blocks * mrq->data->blksz); 
		else 
			mrq->data->bytes_xfered = 0;
	}		
	
	if(res != 0){
			//goto idle
			aml_mmc_goto_idle();
	}
	
	if (cmd->data && cmd->data->stop && (!host->cmd_is_stop)) {
		host->cmd_is_stop = 1;
		cmd = mrq->stop;
		if(res != 0){
				cmd->error = res;
				if (cmd->data && cmd->error)
					cmd->data->error = cmd->error;
		}
		else{
			res = aml_sd_send_command(host, cmd);
			cmd->error = res;	
			if (cmd->data && cmd->error)
				cmd->data->error = cmd->error;
				
			if(res != 0){
				//goto idle
				aml_mmc_goto_idle();
			}
		}
	}
		
	host->complete_what = COMPLETION_FINALIZE;
	mmc_request_done(mmc, mrq);

}

static void aml_sd_request(struct mmc_host *mmc, struct mmc_request *mrq)
{
	struct aml_sd_host *host = mmc_priv(mmc);
	DBG_LINE_INFO();

	host->status = "mmc request";
	host->cmd_is_stop = 0;
	host->mrq = mrq;

	if (aml_sd_get_cd(mmc) == 0) {
	//	dbg(host, dbg_err, "%s: no medium present\n", __func__);
		printk(KERN_INFO "[mmc] no medium inserted!\n");
		host->mrq->cmd->error = -ENOMEDIUM;
		mmc_request_done(mmc, mrq);
	} 
	else	
		aml_sd_send_request(mmc);		
}

/* Routine to configure clock values. Exposed API to core */
static void aml_sd_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct aml_sd_host *host = mmc_priv(mmc);
	struct aml_mmc_platform_data * platform_data = host->pdata;

	unsigned int sdio_config;
	unsigned bus_width= ios->bus_width==MMC_BUS_WIDTH_4?1:0;//MMC
	unsigned clk_div=31;
	
	DBG_LINE_INFO();
		
	switch (ios->power_mode) {
	case MMC_POWER_ON:
		if(platform_data->sdio_pwr_on)
			platform_data->sdio_pwr_on(host->sdio_port);
		mdelay(50);
		break;
	case MMC_POWER_UP:
		return;
		break;

	case MMC_POWER_OFF:
	default:
		if(platform_data->sdio_pwr_prepare)
			platform_data->sdio_pwr_prepare(host->sdio_port);
		if(platform_data->sdio_pwr_off)
			platform_data->sdio_pwr_off(host->sdio_port);
		return;
		break;
	}
	
	if(ios->clock == 0)
		ios->clock = 300000;
	if(ios->clock<mmc->f_min)
	    ios->clock=mmc->f_min;
	if(ios->clock>mmc->f_max)
	    ios->clock=mmc->f_max;

	if(ios->clock)	
		clk_div=(host->clk_rate / (2*ios->clock));

	sdio_config = ((2 << sdio_write_CRC_ok_status_bit) |
                      (2 << sdio_write_Nwr_bit) |
                      (3 << m_endian_bit) |
                      (bus_width<<bus_width_bit)|
                      (39 << cmd_argument_bits_bit) |
                      (0 << cmd_out_at_posedge_bit) |
                      (0 << cmd_disable_CRC_bit) |
                      (0 << response_latch_at_negedge_bit) |
                      (clk_div << cmd_clk_divide_bit));
                      
  if(aml_read_reg32(P_SDIO_CONFIG) != sdio_config){

		if(platform_data->sdio_init)
			platform_data->sdio_init(host);		
		host->bus_width = bus_width?SD_BUS_WIDE:1;
	
		mmc_soft_reset();
 		mdelay(10);
		dbg_print("sdio_config=%x",sdio_config);
		dbg_print("host->clk_rate:%d\n",(int)host->clk_rate);
		dbg_print("sdio clk_div:%d\n",clk_div);
		dbg_print("clock:%d\n",ios->clock);
		dbg_print("bus_width:%x\n",bus_width);
		dbg_print("sdio_port:%d\n",host->sdio_port);

		dbg_print("sdio addtress:%x  : %x\n",P_SDIO_CONFIG,P_SDIO_MULT_CONFIG);
		aml_write_reg32(P_SDIO_CONFIG,sdio_config);
		aml_write_reg32(P_SDIO_MULT_CONFIG,(host->sdio_port & 0x3));	//Switch to	SDIO_A/B/C
	}
}

static int aml_sd_get_ro(struct mmc_host *mmc)
{
	struct aml_sd_host *host = mmc_priv(mmc);
	DBG_LINE_INFO();
	return host->locked;
}

static const struct mmc_host_ops aml_sd_ops = {
//	.enable = aml_sd_enable_fclk,
//	.disable = aml_sd_disable_fclk,
	.request = aml_sd_request,
	.set_ios = aml_sd_set_ios,
	.get_cd = aml_sd_get_cd,
	.get_ro = aml_sd_get_ro,
	/* NYET -- enable_sdio_irq */
//	.enable_sdio_irq = aml_sd_enable_sdio_irq,
};


static struct aml_mmc_platform_data aml_mmc_def_pdata = {
	/* This is currently here to avoid a number of if (host->pdata)
	 * checks. Any zero fields to ensure reasonable defaults are picked. */
	 .no_wprotect = 1,
	 .no_detect = 1,
};


static int __init aml_sd_probe(struct platform_device *pdev)
{
	struct mmc_host *mmc;
	struct aml_sd_host *host = NULL;
	struct aml_mmc_platform_data* platform_data = (struct aml_mmc_platform_data*)pdev->dev.platform_data;
	int ret = 0;
	DBG_LINE_INFO();
	
	mmc = mmc_alloc_host(sizeof(struct aml_sd_host), &pdev->dev);
	if (!mmc) {
		ret = -ENOMEM;
		goto probe_out;
	}
	
	host = mmc_priv(mmc);
	host->mmc 	= mmc;
	host->dev	= (struct device *)pdev;
	host->sdio_port = platform_data->sdio_port;	
	host->pdata = (struct aml_mmc_platform_data*)pdev->dev.platform_data;
	if (!host->pdata) {
		pdev->dev.platform_data = &aml_mmc_def_pdata;
		host->pdata = &aml_mmc_def_pdata;
	}

	spin_lock_init(&host->complete_lock);
	
	if(platform_data->sdio_init)
		platform_data->sdio_init(host);

	host->complete_what 	= COMPLETION_NONE;
	host->pio_active 	= XFER_NONE;

#ifdef CONFIG_MMC_AML_PIODMA
	host->dodma		= host->pdata->dma;
#endif

	host->mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!host->mem) {
		dev_err(&pdev->dev,
			"failed to get io memory region resouce.\n");
		ret = -ENOENT;
		goto probe_free_gpio;
	}

	host->mem = request_mem_region(host->mem->start,
				       resource_size(host->mem), pdev->name);

	if (!host->mem) {
		dev_err(&pdev->dev, "failed to request io memory region.\n");
		ret = -ENOENT;
		goto probe_free_gpio;
	}

	host->base = ioremap(host->mem->start, resource_size(host->mem));
	if (!host->base) {
		dev_err(&pdev->dev, "failed to ioremap() io memory region.\n");
		ret = -EINVAL;
		goto probe_free_mem_region;
	}

	host->irq_cd = -1;

	if (host->clk_rate == 0) {
		dev_err(&pdev->dev, "failed to find clock source.\n");
		host->clk = NULL;
		goto probe_free_dma;
	}
	dbg_print("host clock rate: %d\n",host->clk_rate);	

	mmc->ops 	= &aml_sd_ops;
	if(platform_data->ocr_avail)
		mmc->ocr_avail	= platform_data->ocr_avail;
	else
		mmc->ocr_avail = MMC_VDD_33_34;
		
	if(platform_data->host_caps)
		mmc->caps	= platform_data->host_caps;
	else
		mmc->caps	= MMC_CAP_4_BIT_DATA | MMC_CAP_SD_HIGHSPEED | MMC_CAP_NEEDS_POLL;
	if(platform_data->f_min)
		mmc->f_min = platform_data->f_min;
	else
		mmc->f_min = 200000;
	if(platform_data->f_max)
		mmc->f_max = platform_data->f_max;
	else
		mmc->f_max = 50000000;
	if(platform_data->clock)
		mmc->ios.clock = platform_data->clock;
	else
		mmc->ios.clock = 300000;
	mmc->ios.bus_width = MMC_BUS_WIDTH_1;
	mmc->ocr = mmc->ocr_avail;
	host->locked = 0;

	if(platform_data->max_blk_count)
		mmc->max_blk_count	= platform_data->max_blk_count;
	else
		mmc->max_blk_count	= 4095;
	if(platform_data->max_blk_size)
		mmc->max_blk_size	= platform_data->max_blk_size;
	else
		mmc->max_blk_size	= 4095;
	if(platform_data->max_req_size)
		mmc->max_req_size	= platform_data->max_req_size;
	else
		mmc->max_req_size	= 4095 * 512;
	if(platform_data->max_req_size)
		mmc->max_seg_size	= platform_data->max_req_size;
	else
		mmc->max_seg_size	= mmc->max_req_size;
  
	ret = mmc_add_host(mmc);
	if (ret) {
		dev_err(&pdev->dev, "[mmc probe] failed to add mmc host.\n");
		goto free_cpufreq;
	}
	
	platform_set_drvdata(pdev, mmc);
	
	return 0;

free_cpufreq:


probe_free_dma:
	
probe_free_gpio:
//probe_free_gpio_cd:
	
//probe_iounmap:
	iounmap(host->base);

probe_free_mem_region:
	release_mem_region(host->mem->start, resource_size(host->mem));


 mmc_free_host(mmc);

probe_out:

	return ret;
}

void aml_sd_shutdown(struct platform_device *pdev)
{
	struct mmc_host	*mmc = platform_get_drvdata(pdev);
	struct aml_sd_host *host = mmc_priv(mmc);
	DBG_LINE_INFO();

	if (host->irq_cd >= 0)
		free_irq(host->irq_cd, host);

	mmc_remove_host(mmc);
	clk_disable(host->clk);
}

int aml_sd_remove(struct platform_device *pdev)
{
	struct mmc_host		*mmc  = platform_get_drvdata(pdev);
	struct aml_sd_host	*host = mmc_priv(mmc);
	struct aml_mmc_platform_data *pd = host->pdata;
	DBG_LINE_INFO();

	aml_sd_shutdown(pdev);

	clk_put(host->clk);

//	tasklet_disable(&host->pio_tasklet);

//	free_irq(host->irq, host);

	if (!pd->no_wprotect)
		gpio_free(pd->gpio_wprotect);

	if (!pd->no_detect)
		gpio_free(pd->gpio_detect);

	iounmap(host->base);

	release_mem_region(host->mem->start, resource_size(host->mem));

	mmc_free_host(mmc);
	return 0;
}

static struct platform_driver aml_sd_driver = {
	.probe = aml_sd_probe,
	.remove		= aml_sd_remove,
	.suspend	= aml_mmc_suspend,
	.resume		= aml_mmc_resume,
	.driver		= {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
	},
};

static int __init aml_mmc_init(void)
{
	/* Register the MMC driver */
	return platform_driver_register(&aml_sd_driver);
}

static void __exit aml_mmc_cleanup(void)
{
	/* Unregister MMC driver */
	platform_driver_unregister(&aml_sd_driver);
}

module_init(aml_mmc_init);
module_exit(aml_mmc_cleanup);

MODULE_DESCRIPTION("Amlogic Multimedia Card driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" DRIVER_NAME);
MODULE_AUTHOR("Amlogic Inc");
