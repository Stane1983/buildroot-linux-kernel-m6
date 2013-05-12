/*****************************************************************
**
**  Copyright (C) 2009 Amlogic,Inc.
**  All rights reserved
**        Filename : avlfrontend.c
**
**  comment:
**        Driver for AVL6211 demodulator
**  author :
**	    Shijie.Rong@amlogic
**  version :
**	    v1.0	 12/3/30
*****************************************************************/

/*
    Driver for AVL6211 demodulator
*/

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#ifdef ARC_700
#include <asm/arch/am_regs.h>
#else
#include <mach/am_regs.h>
#endif
#include <linux/i2c.h>
#include <asm/gpio.h>
#include "avlfrontend.h"
#include "LockSignal_Api.h"
#include <linux/kthread.h>



#if 1
#define pr_dbg	printk
//#define pr_dbg(fmt, args...) printk( KERN_DEBUG"DVB: " fmt, ## args)
#else
#define pr_dbg(fmt, args...)
#endif

#define pr_error(fmt, args...) printk( KERN_ERR"DVB: " fmt, ## args)

MODULE_PARM_DESC(frontend0_reset, "\n\t\t Reset GPIO of frontend0");
static int frontend0_reset = -1;
module_param(frontend0_reset, int, S_IRUGO);

MODULE_PARM_DESC(frontend0_i2c, "\n\t\t IIc adapter id of frontend0");
static int frontend0_i2c = -1;
module_param(frontend0_i2c, int, S_IRUGO);

MODULE_PARM_DESC(frontend0_tuner_addr, "\n\t\t Tuner IIC address of frontend0");
static int frontend0_tuner_addr = -1;
module_param(frontend0_tuner_addr, int, S_IRUGO);

MODULE_PARM_DESC(frontend0_demod_addr, "\n\t\t Demod IIC address of frontend0");
static int frontend0_demod_addr = -1;
module_param(frontend0_demod_addr, int, S_IRUGO);

static struct aml_fe avl6211_fe[FE_DEV_COUNT];

MODULE_PARM_DESC(frontend_reset, "\n\t\t Reset GPIO of frontend");
static int frontend_reset = -1;
module_param(frontend_reset, int, S_IRUGO);

MODULE_PARM_DESC(frontend_i2c, "\n\t\t IIc adapter id of frontend");
static int frontend_i2c = -1;
module_param(frontend_i2c, int, S_IRUGO);

MODULE_PARM_DESC(frontend_tuner_addr, "\n\t\t Tuner IIC address of frontend");
static int frontend_tuner_addr = -1;
module_param(frontend_tuner_addr, int, S_IRUGO);

MODULE_PARM_DESC(frontend_demod_addr, "\n\t\t Demod IIC address of frontend");
static int frontend_demod_addr = -1;
module_param(frontend_demod_addr, int, S_IRUGO);

#define M_TUNERMAXLPF_100KHZ	440
#define bs_start_freq			950				//The start RF frequency, 950MHz
#define bs_stop_freq			2150			//The stop RF frequency, 2150MHz
#define Blindscan_Mode   AVL_DVBSx_BS_Slow_Mode	//The Blind scan mode.	AVL_DVBSx_BS_Fast_Mode = 0,AVL_DVBSx_BS_Slow_Mode = 1


static int frontend_LNB = -1;
static int frontend_PWR = -1;
static int frontend_ANT = -1;

static struct aml_fe avl6211_fe[FE_DEV_COUNT];

extern struct AVL_Tuner *avl6211pTuner;
extern struct AVL_DVBSx_Chip * pAVLChip_all;
AVL_semaphore blindscanSem;
static int blindstart=0;
struct dvb_frontend *fe_use = NULL;

static int AVL6211_Reset(void)
{
	gpio_out(frontend_reset, 0);
	msleep(300);
	gpio_out(frontend_reset, 1);
	return 0;
}

static int AVL6211_Lnb_Power_Ctrl(int lnb)
{
	AVL_DVBSx_ErrorCode r = AVL_DVBSx_EC_OK;

	if(1 == lnb)
		r=AVL_DVBSx_IDiseqc_SetLNB1Out(1, pAVLChip_all);	//set LNB1_PIN60 1: Hight 
	else
		r=AVL_DVBSx_IDiseqc_SetLNB1Out(0, pAVLChip_all);	//set LNB1_PIN60 1: Low 	
	
	return r;
}

static int AVL6211_Tuner_Power_Ctrl(int tunerpwr)
{
	if(1==tunerpwr)	AVL_DVBSx_IBase_SetGPIOVal(0, pAVLChip_all);
	else				AVL_DVBSx_IBase_SetGPIOVal(0, pAVLChip_all);
	return 0;
}

static int AVL6211_Ant_Overload_Ctrl(void)
{
	return 0;//gpio_get_value(frontend_ANT);
}

static int	AVL6211_Diseqc_Reset_Overload(struct dvb_frontend* fe)
{
		return 0;
}

static int	AVL6211_Diseqc_Send_Master_Cmd(struct dvb_frontend* fe, struct dvb_diseqc_master_cmd* cmd)
{
	AVL_DVBSx_ErrorCode r = AVL_DVBSx_EC_OK;
	AVL_uchar ucData[8];
	int j=100;
	struct AVL_DVBSx_Diseqc_TxStatus TxStatus;
	int i;
	pr_dbg("msg_len is %d,\n data is",cmd->msg_len);
	for(i=0;i<cmd->msg_len;i++){
		ucData[i]=cmd->msg[i];
		printk("%x ",cmd->msg[i]);
	}
	
	r=AVL_DVBSx_IDiseqc_SendModulationData(ucData, cmd->msg_len, pAVLChip_all);
	if(r != AVL_DVBSx_EC_OK)
	{
		pr_dbg("AVL_DVBSx_IDiseqc_SendModulationData failed !\n");
	}
	else
	{
		do
		{
			j--;
			AVL_DVBSx_IBSP_Delay(1);
			r= AVL_DVBSx_IDiseqc_GetTxStatus(&TxStatus, pAVLChip_all);
		}while((TxStatus.m_TxDone != 1)&&j);
		if(r ==AVL_DVBSx_EC_OK )
		{

		}
		else
		{
			pr_dbg("AVL_DVBSx_IDiseqc_SendModulationData Err. !\n");
		}		
	}
	return r;
}

static int	AVL6211_Diseqc_Recv_Slave_Reply(struct dvb_frontend* fe, struct dvb_diseqc_slave_reply* reply)
{
		return 0;
}

static int	AVL6211_Diseqc_Send_Burst(struct dvb_frontend* fe, fe_sec_mini_cmd_t minicmd)
{
	AVL_DVBSx_ErrorCode r = AVL_DVBSx_EC_OK;
 	struct AVL_DVBSx_Diseqc_TxStatus sTxStatus;
	AVL_uchar ucTone = 0;
	int i=100;
	#define TONE_COUNT				8
	if(minicmd == SEC_MINI_A)
		ucTone = 1;
	else if(minicmd == SEC_MINI_B)
		ucTone = 0;
	else ;

  	r = AVL_DVBSx_IDiseqc_SendTone(ucTone, TONE_COUNT, pAVLChip_all);
	if(AVL_DVBSx_EC_OK != r)
	{
		pr_dbg("\rSend tone %d --- Fail!\n",ucTone);
	}
	else
	{
	    do
	    {
	    	i--;
			AVL_DVBSx_IBSP_Delay(1);
		    r =AVL_DVBSx_IDiseqc_GetTxStatus(&sTxStatus, pAVLChip_all);   //Get current status of the Diseqc transmitter data FIFO.
	    }
	    while((1 != sTxStatus.m_TxDone)&&i);			//Wait until operation finished.
	    if(AVL_DVBSx_EC_OK != r)
	    {
		    pr_dbg("\rOutput tone %d --- Fail!\n",ucTone);
	    }
	}
	return (r);

}

static int	AVL6211_Set_Tone(struct dvb_frontend* fe, fe_sec_tone_mode_t tone)
{
	AVL_DVBSx_ErrorCode r = AVL_DVBSx_EC_OK;
	AVL_uchar  uc22kOn = 0;
	if(SEC_TONE_ON == tone)
		uc22kOn = 1;
	else if(SEC_TONE_OFF == tone)
		uc22kOn = 0;
	else ;
	if(uc22kOn)
	{
		r=AVL_DVBSx_IDiseqc_StartContinuous(pAVLChip_all);
	}else{
		r=AVL_DVBSx_IDiseqc_StopContinuous(pAVLChip_all);
	}
	if(r!=AVL_DVBSx_EC_OK)
	{
		pr_dbg("[AVL6211_22K_Control] Err:0x%x\n",r);
	}	
	
	return r;
	
}

static int	AVL6211_Set_Voltage(struct dvb_frontend* fe, fe_sec_voltage_t voltage)
{
	AVL_DVBSx_ErrorCode r=AVL_DVBSx_EC_OK;	
	AVL_uchar nValue = 1;
	if(voltage == SEC_VOLTAGE_OFF){
		AVL6211_Lnb_Power_Ctrl(0);//lnb power off
		return 0;
	}

	if(voltage ==  SEC_VOLTAGE_13)
		nValue = 0;
	else if(voltage ==SEC_VOLTAGE_18)
		nValue = 1;
	else;	
	
	AVL6211_Lnb_Power_Ctrl(1);//lnb power on

	if(1==nValue)
		r=AVL_DVBSx_IDiseqc_SetLNBOut(1, pAVLChip_all);	//set LNB0_PIN59 1: Hight 
	else
		r=AVL_DVBSx_IDiseqc_SetLNBOut(0, pAVLChip_all);	//set LNB0_PIN59 1: Low 

	return r;
}

static int	AVL6211_Enable_High_Lnb_Voltage(struct dvb_frontend* fe, long arg)
{
	return 0;
}

static void AVL6211_DumpSetting(struct AVL_DVBSx_BlindScanAPI_Setting *pBSsetting)
{
	printk(KERN_INFO "AVL6211_DumpSetting+++\n");

	printk(KERN_INFO "m_uiScan_Min_Symbolrate_MHz %d\n", pBSsetting->m_uiScan_Min_Symbolrate_MHz);
	printk(KERN_INFO "m_uiScan_Max_Symbolrate_MHz %d\n", pBSsetting->m_uiScan_Max_Symbolrate_MHz);
	printk(KERN_INFO "m_uiScan_Start_Freq_MHz %d\n", pBSsetting->m_uiScan_Start_Freq_MHz);
	printk(KERN_INFO "m_uiScan_Stop_Freq_MHz %d\n", pBSsetting->m_uiScan_Stop_Freq_MHz);
	printk(KERN_INFO "m_uiScan_Next_Freq_100KHz %d\n", pBSsetting->m_uiScan_Next_Freq_100KHz);
	printk(KERN_INFO "m_uiScan_Progress_Per %d\n", pBSsetting->m_uiScan_Progress_Per);
	printk(KERN_INFO "m_uiScan_Bind_No %d\n", pBSsetting->m_uiScan_Bind_No);
	printk(KERN_INFO "m_uiTuner_MaxLPF_100kHz %d\n", pBSsetting->m_uiTuner_MaxLPF_100kHz);
	printk(KERN_INFO "m_uiScan_Center_Freq_Step_100KHz %d\n", pBSsetting->m_uiScan_Center_Freq_Step_100KHz);
	printk(KERN_INFO "BS_Mode %d\n", pBSsetting->BS_Mode);
	printk(KERN_INFO "m_uiScaning %d\n", pBSsetting->m_uiScaning);
	printk(KERN_INFO "m_uiChannelCount %d\n", pBSsetting->m_uiChannelCount);
	printk(KERN_INFO "m_eSpectrumMode %d\n", pBSsetting->m_eSpectrumMode);

	printk(KERN_INFO "AVL6211_DumpSetting---\n");
	
	return;
}

static int dvbs2_blindscan_task(struct dvbsx_blindscanpara *pbspara)//(struct dvb_frontend* fe, struct dvbsx_blindscanpara *pbspara)

{
		struct dvbsx_blindscanevent bsevent;
		AVL_DVBSx_ErrorCode r = AVL_DVBSx_EC_OK;
		AVL_uint16	index = 0;
		struct AVL_DVBSx_Channel * pChannel;
	//	AVL_uchar HandIndex = 0;

		//struct AVL_DVBSx_Chip * pAVLChip = &g_stAvlDVBSxChip[HandIndex];
		//struct AVL_Tuner * pTuner = &g_stTuner[HandIndex];
		struct AVL_DVBSx_BlindScanAPI_Setting BSsetting;
		enum AVL_DVBSx_BlindScanAPI_Status BS_Status;
		struct AVL_DVBSx_BlindScanAPI_Setting * pBSsetting = &BSsetting;
		BS_Status = AVL_DVBSx_BS_Status_Init;
	
		//This function do all the initialization work.It should be called only once at the beginning.It needn't be recalled when we want to lock a new channel.
	/*	r = Initialize(pAVLChip,pTuner);
		if(AVL_DVBSx_EC_OK != r)
		{
			printf("Initialization failed !\n");
			return (r);
		}
		printf("Initialization success !\n");*/

		memset(pBSsetting, 0, sizeof(struct AVL_DVBSx_BlindScanAPI_Setting));
	
		pBSsetting->m_uiScan_Start_Freq_MHz=pbspara->minfrequency/1000;
		pBSsetting->m_uiScan_Stop_Freq_MHz=pbspara->maxfrequency/1000;
		pBSsetting->m_uiScan_Max_Symbolrate_MHz=pbspara->maxSymbolRate/(1000 * 1000);
		pBSsetting->m_uiScan_Min_Symbolrate_MHz=pbspara->minSymbolRate/(1000 * 1000);
		
		while(BS_Status != AVL_DVBSx_BS_Status_Exit)
			{
				if(!blindstart){
					BS_Status = AVL_DVBSx_BS_Status_Cancel;
					printf("AVL_DVBSx_BS_Status_Cancel\n");
				}
				printk(KERN_INFO "BS_Status %d blindstart %d\n", BS_Status, blindstart);
				switch(BS_Status)
				{
				case AVL_DVBSx_BS_Status_Init:			{
															printk(KERN_INFO "AVL_DVBSx_BS_Status_Init\n");
														AVL_DVBSx_IBlindScanAPI_Initialize(pBSsetting);//this function set the parameters blind scan process needed.	
		
														AVL_DVBSx_IBlindScanAPI_SetFreqRange(pBSsetting, bs_start_freq, bs_stop_freq); //Default scan rang is from 950 to 2150. User may call this function to change scan frequency rang.
														AVL_DVBSx_IBlindScanAPI_SetScanMode(pBSsetting, Blindscan_Mode);
		
														AVL_DVBSx_IBlindScanAPI_SetSpectrumMode(pBSsetting, AVL_DVBSx_Spectrum_Invert); //Default set is AVL_DVBSx_Spectrum_Normal, it must be set correctly according Board HW configuration
														AVL_DVBSx_IBlindScanAPI_SetMaxLPF(pBSsetting, M_TUNERMAXLPF_100KHZ); //Set Tuner max LPF value, this value will difference according tuner type
		
														BS_Status = AVL_DVBSx_BS_Status_Start;

														AVL6211_DumpSetting(pBSsetting);
														break;
													}
		
				case AVL_DVBSx_BS_Status_Start: 	{	
															
														r = AVL_DVBSx_IBlindScanAPI_Start(pAVLChip_all, avl6211pTuner, pBSsetting);
														printk(KERN_INFO "AVL_DVBSx_BS_Status_Start %d\n", r);
														if(AVL_DVBSx_EC_OK != r)
														{
															BS_Status = AVL_DVBSx_BS_Status_Exit;
														}
														else
														{	
															
															bsevent.status = BLINDSCAN_UPDATESTARTFREQ;
															bsevent.u.m_uistartfreq_khz = avl6211pTuner->m_uiFrequency_100kHz * 100;
															fe_use->ops.blindscan_ops.info.blindscan_callback(fe_use, &bsevent);
															BS_Status = AVL_DVBSx_BS_Status_Wait;
														}
														break;
													}
		
				case AVL_DVBSx_BS_Status_Wait:		{
														r = AVL_DVBSx_IBlindScanAPI_GetCurrentScanStatus(pAVLChip_all, pBSsetting);
														printk(KERN_INFO "AVL_DVBSx_BS_Status_Wait %d %d\n", r, pBSsetting->bsInfo.m_uiChannelCount);
														if(AVL_DVBSx_EC_GeneralFail == r)
														{
															BS_Status = AVL_DVBSx_BS_Status_Exit;
														}
														if(AVL_DVBSx_EC_OK == r)
														{
															BS_Status = AVL_DVBSx_BS_Status_Adjust;
														}
														if(AVL_DVBSx_EC_Running == r)
														{
															AVL_DVBSx_IBSP_Delay(100);
														}
														break;
													}
		
				case AVL_DVBSx_BS_Status_Adjust:		{
														r = AVL_DVBSx_IBlindScanAPI_Adjust(pAVLChip_all, pBSsetting);
														printk(KERN_INFO "AVL_DVBSx_BS_Status_Adjust %d\n", r);
														if(AVL_DVBSx_EC_OK != r)
														{
															BS_Status = AVL_DVBSx_BS_Status_Exit;
														}
														BS_Status = AVL_DVBSx_BS_Status_User_Process;
														break;
													}
		
				case AVL_DVBSx_BS_Status_User_Process:	{
														printk(KERN_INFO "AVL_DVBSx_BS_Status_User_Process\n");
														//------------Custom code start-------------------
														//customer can add the callback function here such as adding TP information to TP list or lock the TP for parsing PSI
														//Add custom code here; Following code is an example
		
														/*----- example 1: print Blindscan progress ----*/
														printf(" %2d%% \n", AVL_DVBSx_IBlindscanAPI_GetProgress(pBSsetting)); //display progress Percent of blindscan process
		
														/*----- example 2: print TP information if found valid TP ----*/
														while(index < pBSsetting->m_uiChannelCount) //display new TP info found in current stage
														{
															pChannel = &pBSsetting->channels[index++];
															printf("	  Ch%2d: RF: %4d SR: %5d ",index, (pChannel->m_uiFrequency_kHz/1000),(pChannel->m_uiSymbolRate_Hz/1000));

															bsevent.status = BLINDSCAN_UPDATERESULTFREQ;
															bsevent.u.parameters.frequency = pChannel->m_uiFrequency_kHz;
															bsevent.u.parameters.u.qpsk.symbol_rate = pChannel->m_uiSymbolRate_Hz;
															
															fe_use->ops.blindscan_ops.info.blindscan_callback(fe_use, &bsevent);
														}	
														
														bsevent.status = BLINDSCAN_UPDATEPROCESS;
														bsevent.u.m_uiprogress = AVL_DVBSx_IBlindscanAPI_GetProgress(pBSsetting);
														fe_use->ops.blindscan_ops.info.blindscan_callback(fe_use, &bsevent);
														//------------Custom code end -------------------
		
														if ( (AVL_DVBSx_IBlindscanAPI_GetProgress(pBSsetting) < 100))
															BS_Status = AVL_DVBSx_BS_Status_Start;
														else											
															BS_Status = AVL_DVBSx_BS_Status_WaitExit;
														break;
													}
				case AVL_DVBSx_BS_Status_WaitExit:
													{
														msleep(50);
														break;
													}
		
				case AVL_DVBSx_BS_Status_Cancel:		{ 
														r = AVL_DVBSx_IBlindScanAPI_Exit(pAVLChip_all,pBSsetting);
														printk(KERN_INFO "AVL_DVBSx_BS_Status_Cancel %d\n", r);
														BS_Status = AVL_DVBSx_BS_Status_Exit;
														blindstart=2;
														break;
													}
		
				default:							{
														BS_Status = AVL_DVBSx_BS_Status_Cancel;
														break;
													}
					}
				}
			return 0;


}


static struct task_struct *dvbs2_task;
static int AVL6211_Blindscan_Scan(struct dvb_frontend* fe, struct dvbsx_blindscanpara *pbspara)
{
	printk(KERN_INFO "AVL6211_Blindscan_Scan printk\n");

	AVL_DVBSx_IBSP_WaitSemaphore(&blindscanSem);
	fe_use = fe;
	blindstart=1;
	AVL_DVBSx_IBSP_ReleaseSemaphore(&blindscanSem);
	  dvbs2_task = kthread_create(dvbs2_blindscan_task, pbspara, "dvbs2_task");
      if(!dvbs2_task){
      	printk("Unable to start dvbs2 thread.\n");
     	dvbs2_task = NULL;
      return -1;
      }
	  wake_up_process(dvbs2_task);
	  return 0;
}

static int AVL6211_Blindscan_Cancel(struct dvb_frontend* fe)
{
		blindstart=0;
		printk(KERN_INFO "AVL6211_Blindscan_Cancel\n");
		while(2!=blindstart){
				pr_dbg("wait for scan exit\n");
				msleep(100);
		}
		/*call do_exit() directly*/
		dvbs2_task = NULL;
		fe_use = NULL;
		return 0;
}


static int AVL6211_I2c_Gate_Ctrl(struct dvb_frontend *fe, int enable)
{

	return 0;
}

static int initflag=-1;


static int AVL6211_Init(struct dvb_frontend *fe)
{
	AVL_DVBSx_ErrorCode r = AVL_DVBSx_EC_OK;
	pr_dbg("frontend_reset is %d\n",frontend_reset);
	//init sema
	AVL_DVBSx_IBSP_InitSemaphore( &blindscanSem );
	//reset
	AVL6211_Reset();
	msleep(100);
	//init
	r=AVL6211_LockSignal_Init();
	//LBNON
//	AVL6211_Lnb_Power_Ctrl(1);
	//tunerpower
	AVL6211_Tuner_Power_Ctrl(0);
//	r=AVL_DVBSx_IDiseqc_StopContinuous(pAVLChip_all);

	r=AVL_DVBSx_IBase_SetGPIODir(0, pAVLChip_all);
	if(AVL_DVBSx_EC_OK != r)
	{
		return r;
	}
	initflag =0;
	pr_dbg("0x%x(ptuner),0x%x(pavchip)=========================demod init\r\n",avl6211pTuner->m_uiSlaveAddress,pAVLChip_all->m_SlaveAddr);
	msleep(200);
	return 0;
}

static int AVL6211_Sleep(struct dvb_frontend *fe)
{
	struct avl6211_state *state = fe->demodulator_priv;

//	GX_Set_Sleep(state, 1);

	return 0;
}

static int AVL6211_Read_Status(struct dvb_frontend *fe, fe_status_t * status)
{
	unsigned char s=0;
//printk("Get status blindstart:%d.\n",blindstart);
	if(1==blindstart)
	     s=1;
         else
	   s=AVL6211_GETLockStatus();

	if(s==1)
	{
		*status = FE_HAS_LOCK|FE_HAS_SIGNAL|FE_HAS_CARRIER|FE_HAS_VITERBI|FE_HAS_SYNC;
	}
	else
	{
		*status = FE_TIMEDOUT;
	}
	
	return  0;
}

static int AVL6211_Read_Ber(struct dvb_frontend *fe, u32 * ber)
{
	if(1==blindstart)
		return 0;
	*ber=AVL6211_GETBer();
	return 0;
}

static int AVL6211_Read_Signal_Strength(struct dvb_frontend *fe, u16 *strength)
{
	if(1==blindstart)
		return ;
	*strength=AVL_Get_Level_Percent(pAVLChip_all);
//	*strength=AVL6211_GETSignalLevel();
	return 0;
}

static int AVL6211_Read_Snr(struct dvb_frontend *fe, u16 * snr)
{
	if(1==blindstart)
		return 0;
	*snr=AVL_Get_Quality_Percent(pAVLChip_all);
	//*snr=AVL6211_GETSnr();
	return 0;
}

static int AVL6211_Read_Ucblocks(struct dvb_frontend *fe, u32 * ucblocks)
{
	ucblocks=NULL;
	return 0;
}

static int AVL6211_Set_Frontend(struct dvb_frontend *fe, struct dvb_frontend_parameters *p)
{
	if(initflag!=0)
	{
		pr_dbg("[%s] avl6211 init fail\n",__FUNCTION__);
		return 0;	
	}
//	printk("[AVL6211_Set_Frontend],blindstart is %d\n",blindstart);

	if(1==blindstart)
		return 0;
	AVL_DVBSx_IBSP_WaitSemaphore(&blindscanSem);
	if((850000>p->frequency)||(p->frequency>2300000))
	{
			p->frequency =945000;
			pr_dbg("freq is out of range,force to set 945000khz\n");
	}
	struct avl6211_state *state = fe->demodulator_priv;
	struct AVL_DVBSx_Channel Channel;
	AVL_DVBSx_ErrorCode r = AVL_DVBSx_EC_OK;
	avl6211pTuner->m_uiFrequency_100kHz=p->frequency/100;
//	avl6211pTuner->m_uiFrequency_100kHz=15000;
//	printk("avl6211pTuner m_uiFrequency_100kHz is %d",avl6211pTuner->m_uiFrequency_100kHz);
	
	/* r = CPU_Halt(pAVLChip_all);
	if(AVL_DVBSx_EC_OK != r)
	{
		printf("CPU halt failed !\n");
		return (r);
	}*/

	//Change the value defined by macro and go back here when we want to lock a new channel.
//	avl6211pTuner->m_uiFrequency_100kHz = tuner_freq*10;      
	avl6211pTuner->m_uiSymbolRate_Hz = p->u.qam.symbol_rate;//p->u.qam.symbol_rate;//30000000; //symbol rate of the channel to be locked.
	//This function should be called before locking the tuner to adjust the tuner LPF based on channel symbol rate.
	AVL_Set_LPF(avl6211pTuner, avl6211pTuner->m_uiSymbolRate_Hz);

	r=avl6211pTuner->m_pLockFunc(avl6211pTuner);
	if (AVL_DVBSx_EC_OK != r)
	{
		state->freq=p->frequency;
		state->mode=p->u.qam.modulation ;
		state->symbol_rate=p->u.qam.symbol_rate;
		AVL_DVBSx_IBSP_ReleaseSemaphore(&blindscanSem);
 		pr_dbg("Tuner test failed !\n");
		return (r);
	}
	pr_dbg("Tuner test ok !\n");
	msleep(50);
	#if 0
	Channel.m_uiSymbolRate_Hz = p->u.qam.symbol_rate;      //Change the value defined by macro when we want to lock a new channel.
	Channel.m_Flags = (CI_FLAG_MANUAL_LOCK_MODE) << CI_FLAG_MANUAL_LOCK_MODE_BIT;		//Manual lock Flag
									
	Channel.m_Flags |= (CI_FLAG_IQ_NO_SWAPPED) << CI_FLAG_IQ_BIT;   		//Auto IQ swap
	Channel.m_Flags |= (CI_FLAG_IQ_AUTO_BIT_AUTO) << CI_FLAG_IQ_AUTO_BIT;			//Auto IQ swap Flag
													//Support QPSK and 8PSK  dvbs2
	{
	#define Coderate				RX_DVBS2_2_3
	#define Modulation				AVL_DVBSx_MM_QPSK
	
		if (Coderate > 16 || Coderate < 6 || Modulation > 3)
		{			
			printf("Configure error !\n");
			return AVL_DVBSx_EC_GeneralFail;
		}
		Channel.m_Flags |= (CI_FLAG_DVBS2) << CI_FLAG_DVBS2_BIT;											//Disable automatic standard detection
		Channel.m_Flags |= (enum AVL_DVBSx_FecRate)(Coderate) << CI_FLAG_CODERATE_BIT;						//Manual config FEC code rate
		Channel.m_Flags |= ((enum AVL_DVBSx_ModulationMode)(Modulation)) << CI_FLAG_MODULATION_BIT;			//Manual config Modulation
	}
	//This function should be called after tuner locked to lock the channel.
	#else
	Channel.m_uiSymbolRate_Hz = p->u.qam.symbol_rate;
	Channel.m_Flags = (CI_FLAG_IQ_NO_SWAPPED) << CI_FLAG_IQ_BIT;	//Normal IQ
	Channel.m_Flags |= (CI_FLAG_IQ_AUTO_BIT_AUTO) << CI_FLAG_IQ_AUTO_BIT;	//Enable automatic IQ swap detection
	Channel.m_Flags |= (CI_FLAG_DVBS2_UNDEF) << CI_FLAG_DVBS2_BIT;			//Enable automatic standard detection
	#endif
	r = AVL_DVBSx_IRx_LockChannel(&Channel, pAVLChip_all);  
	if (AVL_DVBSx_EC_OK != r)
	{
		state->freq=p->frequency;
		state->mode=p->u.qam.modulation ;
		state->symbol_rate=p->u.qam.symbol_rate;	
		AVL_DVBSx_IBSP_ReleaseSemaphore(&blindscanSem);
		pr_dbg("Lock channel failed !\n");
		return (r);
	}
	AVL_DVBSx_IBSP_ReleaseSemaphore(&blindscanSem);
	if (AVL_DVBSx_EC_OK != r)
	{
		printf("Lock channel failed !\n");
		return (r);
	}
	int waittime=150;//3s 
	//Channel lock time increase while symbol rate decrease.Give the max waiting time for different symbolrates.
	if(p->u.qam.symbol_rate<5000000)
	{
		waittime = 150;//250;       //The max waiting time is 5000ms,considering the IQ swapped status the time should be doubled.
	}
	else if(p->u.qam.symbol_rate<10000000)
	{
        waittime = 30;        //The max waiting time is 600ms,considering the IQ swapped status the time should be doubled.
	}
	else
	{
        waittime = 15;        //The max waiting time is 300ms,considering the IQ swapped status the time should be doubled.
	} 
	int lockstatus = 0;
	while(waittime)
	{
		msleep(20);
		lockstatus=AVL6211_GETLockStatus();
		if(1==lockstatus){
			pr_dbg("lock success !\n");
			break;
		}
		
		waittime--;
	}
	if(!AVL6211_GETLockStatus())
		pr_dbg("lock timeout !\n");
	
	r=AVL_DVBSx_IRx_ResetErrorStat(pAVLChip_all);
	if (AVL_DVBSx_EC_OK != r)
	{
		state->freq=p->frequency;
		state->mode=p->u.qam.modulation ;
		state->symbol_rate=p->u.qam.symbol_rate;
		
		printf("Reset error status failed !\n");
		return (r);
	}
	
//	demod_connect(state, p->frequency,p->u.qam.modulation,p->u.qam.symbol_rate);
	state->freq=p->frequency;
	state->mode=p->u.qam.modulation ;
	state->symbol_rate=p->u.qam.symbol_rate; //these data will be writed to eeprom

	pr_dbg("avl6211=>frequency=%d,symbol_rate=%d\r\n",p->frequency,p->u.qam.symbol_rate);
	return  0;
}

static int AVL6211_Get_Frontend(struct dvb_frontend *fe, struct dvb_frontend_parameters *p)
{//these content will be writed into eeprom .

	struct avl6211_state *state = fe->demodulator_priv;
	
	p->frequency=state->freq;
	p->u.qam.modulation=state->mode;
	p->u.qam.symbol_rate=state->symbol_rate;
	
	return 0;
}

static void AVL6211_Release(struct dvb_frontend *fe)
{
	struct avl6211_state *state = fe->demodulator_priv;
	kfree(state);
}

static ssize_t avl_frontend_show_short_circuit(struct class* class, struct class_attribute* attr, char* buf)
{
	int ant_overload_status = AVL6211_Ant_Overload_Ctrl();
	
	return sprintf(buf, "%d\n", ant_overload_status);
}

static struct class_attribute avl_frontend_class_attrs[] = {
	__ATTR(short_circuit,  S_IRUGO | S_IWUSR, avl_frontend_show_short_circuit, NULL),
	__ATTR_NULL
};

static struct class avl_frontend_class = {
	.name = "avl_frontend",
	.class_attrs = avl_frontend_class_attrs,
};

static struct dvb_frontend_ops avl6211_ops;

struct dvb_frontend *avl6211_attach(const struct avl6211_fe_config *config)
{
	struct avl6211_state *state = NULL;

	/* allocate memory for the internal state */
	
	state = kmalloc(sizeof(struct avl6211_state), GFP_KERNEL);
	if (state == NULL)
		return NULL;

	/* setup the state */
	state->config = *config;
	
	/* create dvb_frontend */
	memcpy(&state->fe.ops, &avl6211_ops, sizeof(struct dvb_frontend_ops));
	state->fe.demodulator_priv = state;
	
	return &state->fe;
}

static struct dvb_frontend_ops avl6211_ops = {


		.info = {
		 .name = "AMLOGIC DVB-S2",
		.type = FE_QPSK,
		.frequency_min = 850000,
		.frequency_max = 2300000,
		.frequency_stepsize = 0,
		.frequency_tolerance = 0,
		.caps =
			FE_CAN_FEC_1_2 | FE_CAN_FEC_2_3 | FE_CAN_FEC_3_4 |
			FE_CAN_FEC_5_6 | FE_CAN_FEC_7_8 | FE_CAN_FEC_AUTO |
			FE_CAN_QPSK | FE_CAN_QAM_16 |
			FE_CAN_QAM_64 | FE_CAN_QAM_AUTO |
			FE_CAN_TRANSMISSION_MODE_AUTO |
			FE_CAN_GUARD_INTERVAL_AUTO |
			FE_CAN_HIERARCHY_AUTO |
			FE_CAN_RECOVER |
			FE_CAN_MUTE_TS
	},

	.release = AVL6211_Release,

	.init = AVL6211_Init,
	.sleep = AVL6211_Sleep,
	.i2c_gate_ctrl = AVL6211_I2c_Gate_Ctrl,

	.set_frontend = AVL6211_Set_Frontend,
	.get_frontend = AVL6211_Get_Frontend,	
	.read_status = AVL6211_Read_Status,
	.read_ber = AVL6211_Read_Ber,
	.read_signal_strength =AVL6211_Read_Signal_Strength,
	.read_snr = AVL6211_Read_Snr,
	.read_ucblocks = AVL6211_Read_Ucblocks,


	.diseqc_reset_overload = AVL6211_Diseqc_Reset_Overload,
	.diseqc_send_master_cmd = AVL6211_Diseqc_Send_Master_Cmd,
	.diseqc_recv_slave_reply = AVL6211_Diseqc_Recv_Slave_Reply,
	.diseqc_send_burst = AVL6211_Diseqc_Send_Burst,
	.set_tone = AVL6211_Set_Tone,
	.set_voltage = AVL6211_Set_Voltage,
	.enable_high_lnb_voltage = AVL6211_Enable_High_Lnb_Voltage,
	.blindscan_ops = {
		.blindscan_scan	=	AVL6211_Blindscan_Scan,
		.blindscan_cancel	=	AVL6211_Blindscan_Cancel
	},

};

static void avl6211_fe_release(struct aml_dvb *advb, struct aml_fe *fe)
{
	if(fe && fe->fe) {
		pr_dbg("release avl6211 frontend %d\n", fe->id);
		dvb_unregister_frontend(fe->fe);
		dvb_frontend_detach(fe->fe);
		if(fe->cfg){
			kfree(fe->cfg);
			fe->cfg = NULL;
		}
		fe->id = -1;
	}
}

static int avl6211_fe_init(struct aml_dvb *advb, struct platform_device *pdev, struct aml_fe *fe, int id)
{
	struct dvb_frontend_ops *ops;
	int ret;
	struct resource *res;
	struct avl6211_fe_config *cfg;
	char buf[32];
	
	pr_dbg("init avl6211 frontend %d\n", id);


	cfg = kzalloc(sizeof(struct avl6211_fe_config), GFP_KERNEL);
	if (!cfg)
		return -ENOMEM;
	
	cfg->reset_pin = frontend_reset;
	if(cfg->reset_pin==-1) {
		snprintf(buf, sizeof(buf), "frontend%d_reset_pin", id);
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM, buf);
		if (!res) {
			pr_error("cannot get resource \"%s\"\n", buf);
			ret = -EINVAL;
			goto err_resource;
		}
		cfg->reset_pin = res->start;		
	}
	
	cfg->i2c_id = frontend_i2c;
	if(cfg->i2c_id==-1) {
		snprintf(buf, sizeof(buf), "frontend%d_i2c", id);
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM, buf);
		if (!res) {
			pr_error("cannot get resource \"%s\"\n", buf);
			ret = -EINVAL;
			goto err_resource;
		}
		cfg->i2c_id = res->start;
	}
	
	cfg->tuner_addr = frontend_tuner_addr;
	
	if(cfg->tuner_addr==-1) {
		snprintf(buf, sizeof(buf), "frontend%d_tuner_addr", id);
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM, buf);
		if (!res) {
			pr_error("cannot get resource \"%s\"\n", buf);
			ret = -EINVAL;
			goto err_resource;
		}
		cfg->tuner_addr = res->start>>1;
	}
	
	cfg->demod_addr = frontend_demod_addr;
	if(cfg->demod_addr==-1) {
		snprintf(buf, sizeof(buf), "frontend%d_demod_addr", id);
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM, buf);
		if (!res) {
			pr_error("cannot get resource \"%s\"\n", buf);
			ret = -EINVAL;
			goto err_resource;
		}
		cfg->demod_addr = res->start>>1;
	}

	
		snprintf(buf, sizeof(buf), "frontend%d_LNBON/OFF", id);
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM, buf);
		if (!res) {
			pr_error("cannot get resource \"%s\"\n", buf);
		} else {		
			frontend_LNB = res->start;
		}

		snprintf(buf, sizeof(buf), "frontend%d_POWERON/OFF", id);
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM, buf);
		if (!res) {
			pr_error("cannot get resource \"%s\"\n", buf);
		} else {
			frontend_PWR = res->start;

		}

		snprintf(buf, sizeof(buf), "frontend%d_ANTOVERLOAD", id);
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM, buf);
		if (!res) {
			pr_error("cannot get resource \"%s\"\n", buf);
		} else {
			frontend_ANT = res->start;
		}

	frontend_reset = cfg->reset_pin;
	frontend_i2c = cfg->i2c_id;
	frontend_tuner_addr = cfg->tuner_addr;
	frontend_demod_addr = cfg->demod_addr;	
	fe->fe = avl6211_attach(cfg);
	if (!fe->fe) {
		ret = -ENOMEM;
		goto err_resource;
	}

	if ((ret=dvb_register_frontend(&advb->dvb_adapter, fe->fe))) {
		pr_error("frontend registration failed!");
		ops = &fe->fe->ops;
		if (ops->release != NULL)
			ops->release(fe->fe);
		fe->fe = NULL;
		goto err_resource;
	}
	
	fe->id = id;
	fe->cfg = cfg;
	
	return 0;

err_resource:
	kfree(cfg);
	return ret;
}

int avl6211_get_fe_config(struct avl6211_fe_config *cfg)
{
	struct i2c_adapter *i2c_handle;
	cfg->i2c_id = frontend_i2c;
	cfg->demod_addr = frontend_demod_addr;
	cfg->tuner_addr = frontend_tuner_addr;
	cfg->reset_pin = frontend_reset;
//	printk("\n frontend_i2c is %d,,frontend_demod_addr is %x,frontend_tuner_addr is %x,frontend_reset is %d",frontend_i2c,frontend_demod_addr,frontend_tuner_addr,frontend_reset);
	i2c_handle = i2c_get_adapter(cfg->i2c_id);
	if (!i2c_handle) {
		printk("cannot get i2c adaptor\n");
		return 0;
	}
	cfg->i2c_adapter =i2c_handle;
	return 1;
	
//	printk("\n frontend0_i2c is %d, frontend_i2c is %d,",frontend0_i2c,frontend_i2c);
	
}

static int avl6211_fe_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct aml_dvb *dvb = aml_get_dvb_device();
	
	if(avl6211_fe_init(dvb, pdev, &avl6211_fe[0], 0)<0)
		return -ENXIO;

	platform_set_drvdata(pdev, &avl6211_fe[0]);

	if((ret = class_register(&avl_frontend_class))<0) {
		pr_error("register class error\n");

		struct aml_fe *drv_data = platform_get_drvdata(pdev);
		
		platform_set_drvdata(pdev, NULL);
	
		avl6211_fe_release(dvb, drv_data);

		return ret;
	}
	
	return ret;
}

static int avl6211_fe_remove(struct platform_device *pdev)
{
	struct aml_fe *drv_data = platform_get_drvdata(pdev);
	struct aml_dvb *dvb = aml_get_dvb_device();

	class_unregister(&avl_frontend_class);

	platform_set_drvdata(pdev, NULL);
	
	avl6211_fe_release(dvb, drv_data);
	
	return 0;
}

static int avl6211_fe_resume(struct platform_device *pdev)
{
	pr_dbg("avl6211_fe_resume \n");
	AVL_DVBSx_ErrorCode r = AVL_DVBSx_EC_OK;
	//init sema
	AVL_DVBSx_IBSP_InitSemaphore( &blindscanSem );
	//reset
	AVL6211_Reset();
	msleep(100);
	//init
	r=AVL6211_LockSignal_Init();
	//LBNON
//	AVL6211_Lnb_Power_Ctrl(1);
	//tunerpower
	AVL6211_Tuner_Power_Ctrl(0);
//	r=AVL_DVBSx_IDiseqc_StopContinuous(pAVLChip_all);

	r=AVL_DVBSx_IBase_SetGPIODir(0, pAVLChip_all);
	if(AVL_DVBSx_EC_OK != r)
	{
		return r;
	}
	initflag =0;
	pr_dbg("0x%x(ptuner),0x%x(pavchip)=========================demod init\r\n",avl6211pTuner->m_uiSlaveAddress,pAVLChip_all->m_SlaveAddr);
	msleep(200);
	return 0;

}

static int avl6211_fe_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}


static struct platform_driver aml_fe_driver = {
	.probe		= avl6211_fe_probe,
	.remove		= avl6211_fe_remove,
	.resume		= avl6211_fe_resume,
	.suspend	= avl6211_fe_suspend,
	.driver		= {
		.name	= "avl6211",
		.owner	= THIS_MODULE,
	}
};

static int __init avlfrontend_init(void)
{
	return platform_driver_register(&aml_fe_driver);
}


static void __exit avlfrontend_exit(void)
{
	platform_driver_unregister(&aml_fe_driver);
}

module_init(avlfrontend_init);
module_exit(avlfrontend_exit);


MODULE_DESCRIPTION("avl6211 DVB-S2 Demodulator driver");
MODULE_AUTHOR("RSJ");
MODULE_LICENSE("GPL");


