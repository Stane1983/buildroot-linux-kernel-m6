/*****************************************************************
**
**  Copyright (C) 2009 Amlogic,Inc.
**  All rights reserved
**        Filename : mxlfrontend.c
**
**  comment:
**        Driver for MXL101 demodulator
**  author :
**	    Shijie.Rong@amlogic
**  version :
**	    v1.0	 12/3/13
*****************************************************************/

/*
    Driver for MXL101 demodulator
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
#include <linux/gpio.h>
#include "demod_MxL101SF.h"
#include "../aml_fe.h"

#if 1
#define pr_dbg(args...) printk("MXL: " args)
#else
#define pr_dbg(args...)
#endif

#define pr_error(args...) printk("MXL: " args)

static int mxl101_read_status(struct dvb_frontend *fe, fe_status_t * status)
{
	struct aml_fe *afe = fe->demodulator_priv;
	struct aml_fe_dev *dev = afe->dtv_demod;
	unsigned char s=0;
	//msleep(1000);
	s=MxL101SF_GetLock(dev);
	/*ber=MxL101SF_GetBER();
	snr=MxL101SF_GetSNR();
	strength=MxL101SF_GetSigStrength();
	printk("ber is %d,snr is%d,strength is%d\n",ber,snr,strength);
	s=1;*/
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

static int mxl101_read_ber(struct dvb_frontend *fe, u32 * ber)
{
	struct aml_fe *afe = fe->demodulator_priv;
	struct aml_fe_dev *dev = afe->dtv_demod;

	*ber=MxL101SF_GetBER(dev);
	return 0;
}

static int mxl101_read_signal_strength(struct dvb_frontend *fe, u16 *strength)
{
	struct aml_fe *afe = fe->demodulator_priv;
	struct aml_fe_dev *dev = afe->dtv_demod;

	*strength=MxL101SF_GetSigStrength(dev);
	return 0;
}

static int mxl101_read_snr(struct dvb_frontend *fe, u16 * snr)
{
	struct aml_fe *afe = fe->demodulator_priv;
	struct aml_fe_dev *dev = afe->dtv_demod;

	*snr=MxL101SF_GetSNR(dev) ;		
	return 0;
}

static int mxl101_read_ucblocks(struct dvb_frontend *fe, u32 * ucblocks)
{
	*ucblocks=0;
	return 0;
}

static int mxl101_set_frontend(struct dvb_frontend *fe, struct dvb_frontend_parameters *p)
{
	struct aml_fe *afe = fe->demodulator_priv;
	struct aml_fe_dev *dev = afe->dtv_demod;

	UINT8 bandwidth=8;
	bandwidth=p->u.ofdm.bandwidth;
	if(bandwidth==0)
		bandwidth=8;
	else if(bandwidth==1)
		bandwidth=7;
	else if(bandwidth==2)
		bandwidth=6;
	else
		bandwidth=8;	
	MxL101SF_Tune(p->frequency,bandwidth, dev);
//	demod_connect(state, p->frequency,p->u.qam.modulation,p->u.qam.symbol_rate);
	afe->params = *p;
//	Mxl101SF_Debug();
	pr_dbg("mxl101=>frequency=%d,symbol_rate=%d\r\n",p->frequency,p->u.qam.symbol_rate);
	return  0;
}

static int mxl101_get_frontend(struct dvb_frontend *fe, struct dvb_frontend_parameters *p)
{//these content will be writed into eeprom .

	struct aml_fe *afe = fe->demodulator_priv;
	
	*p = afe->params;
	return 0;
}

static int mxl101_fe_get_ops(struct aml_fe_dev *dev, int mode, void *ops)
{
	struct dvb_frontend_ops *fe_ops = (struct dvb_frontend_ops*)ops;

	fe_ops->info.frequency_min = 51000000;
	fe_ops->info.frequency_max = 858000000;
	fe_ops->info.frequency_stepsize = 166667;
	fe_ops->info.frequency_tolerance = 0;
	fe_ops->info.caps = FE_CAN_FEC_1_2 | FE_CAN_FEC_2_3 | FE_CAN_FEC_3_4 |
			FE_CAN_FEC_5_6 | FE_CAN_FEC_7_8 | FE_CAN_FEC_AUTO |
			FE_CAN_QPSK | FE_CAN_QAM_16 |
			FE_CAN_QAM_64 | FE_CAN_QAM_AUTO |
			FE_CAN_TRANSMISSION_MODE_AUTO |
			FE_CAN_GUARD_INTERVAL_AUTO |
			FE_CAN_HIERARCHY_AUTO |
			FE_CAN_RECOVER |
			FE_CAN_MUTE_TS;

	fe_ops->set_frontend = mxl101_set_frontend;
	fe_ops->get_frontend = mxl101_get_frontend;	
	fe_ops->read_status = mxl101_read_status;
	fe_ops->read_ber = mxl101_read_ber;
	fe_ops->read_signal_strength = mxl101_read_signal_strength;
	fe_ops->read_snr = mxl101_read_snr;
	fe_ops->read_ucblocks = mxl101_read_ucblocks;
	
	return 0;
}

static int mxl101_fe_enter_mode(struct aml_fe *fe, int mode)
{
	struct aml_fe_dev *dev = fe->dtv_demod;

	pr_dbg("=========================demod init\r\n");
	gpio_direction_output(dev->reset_gpio, dev->reset_value);
	msleep(300);
	gpio_direction_output(dev->reset_gpio, !dev->reset_value); //enable tuner power
	msleep(200);
	MxL101SF_Init(dev);

	return 0;
}

static int mxl101_fe_resume(struct aml_fe_dev *dev)
{
	printk("mxl101_fe_resume\n");
	gpio_direction_output(dev->reset_gpio, dev->reset_value);
	msleep(300);
	gpio_direction_output(dev->reset_gpio, !dev->reset_value); //enable tuner power
	msleep(200);
	MxL101SF_Init(dev);
	return 0;

}

static int mxl101_fe_suspend(struct aml_fe_dev *dev)
{
	return 0;
}

static struct aml_fe_drv mxl101_dtv_demod_drv = {
.id         = AM_DTV_DEMOD_MXL101,
.name       = "Mxl101",
.capability = AM_FE_OFDM,
.get_ops    = mxl101_fe_get_ops,
.enter_mode = mxl101_fe_enter_mode,
.suspend    = mxl101_fe_suspend,
.resume     = mxl101_fe_resume
};

static int __init mxlfrontend_init(void)
{
	pr_dbg("register mxl101 demod driver\n");
	return aml_register_fe_drv(AM_DEV_DTV_DEMOD, &mxl101_dtv_demod_drv);
}


static void __exit mxlfrontend_exit(void)
{
	pr_dbg("unregister mxl101 demod driver\n");
	aml_unregister_fe_drv(AM_DEV_DTV_DEMOD, &mxl101_dtv_demod_drv);
}

fs_initcall(mxlfrontend_init);
module_exit(mxlfrontend_exit);


MODULE_DESCRIPTION("mxl101 DVB-T Demodulator driver");
MODULE_AUTHOR("RSJ");
MODULE_LICENSE("GPL");


