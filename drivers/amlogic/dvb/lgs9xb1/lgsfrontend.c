/*****************************************************************
**
**  Copyright (C) 2009 Amlogic,Inc.
**  All rights reserved
**        Filename : itefrontend.c
**
**  comment:
**        Driver for LGS-9X-B1 demodulator
**  author :
**	      Shanwu.Hu@amlogic.com
**  version :
**	      v1.0	 12/11/28
*****************************************************************/

/*
    Driver for LGS-9X-B1 demodulator
*/

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#ifdef ARC_700
#include <asm/arch/am_regs.h>
#else
#include <mach/am_regs.h>
#endif
#include <linux/i2c.h>
#include <asm/gpio.h>
#include "mxl5007/MxL5007_API.h"
#include "LGS_USER.h"
#include "LGS_9X.h"
#include "lgsfrontend.h"

#if 0
#define pr_dbg(fmt, args...) printk( "DVB: " fmt, ## args)
//#define pr_dbg(fmt, args...) printk( KERN_DEBUG"DVB: " fmt, ## args)
#else
#define pr_dbg(fmt, args...)
#endif

#define pr_error(fmt, args...) printk( KERN_ERR"DVB: " fmt, ## args)

int lgs9x_get_fe_config(struct  lgs9x_fe_config *cfg);

MODULE_PARM_DESC(frontend_i2c, "\n\t\t IIc adapter id of frontend");
static int frontend_i2c = -1;
module_param(frontend_i2c, int, S_IRUGO);
MODULE_PARM_DESC(frontend_reset, "\n\t\t Reset GPIO of frontend");
static int frontend_reset = -1;
module_param(frontend_reset, int, S_IRUGO);

MODULE_PARM_DESC(frontend_power, "\n\t\t TUNER_POWER of frontend");
static int frontend_power = -1;
module_param(frontend_power, int, S_IRUGO);

MxL5007_TunerConfigS   myTuner = {
	.Mode = MxL_MODE_DVBT,
	.IF_Diff_Out_Level = -8,
	.Xtal_Freq = MxL_XTAL_24_MHZ,
	.IF_Freq = MxL_IF_5_MHZ,
	.IF_Spectrum = MxL_NORMAL_IF,
	.ClkOut_Setting = MxL_CLKOUT_DISABLE,
	.ClkOut_Amp = MxL_CLKOUT_AMP_0,
};

static UINT8 locked1 = 0, locked2 = 0;
static struct mutex lgs_lock;
static struct aml_fe lgs9x_fe[FE_DEV_COUNT];
DemodInitPara myDemod = {
	.workMode = DEMOD_WORK_MODE_ANT1_DTMB,
	.tsOutputType = TS_Output_Serial,	
	.IF = 5,
	.dtmbIFSelect = 1,
	.SampleClock = 0,
};

static int lgs9x_init(struct dvb_frontend *fe)
{
	MxL_ERR_MSG Status = MxL_OK;
	struct lgs9x_fe_config cfg;

	gpio_out(frontend_reset, 0);
	msleep(300);
	gpio_out(frontend_reset, 1); //enable tuner power
	msleep(500);

	gpio_out(frontend_power, 1);

	LGS_DemodRegisterWait(lgs_wait);
	LGS_DemodRegisterRegisterAccess(lgs_read, lgs_write, lgs_read_multibyte, lgs_write_multibyte);

	lgs9x_get_fe_config(&cfg); 
	myTuner.I2C_adap = (UINT32)cfg.i2c_adapter;

	LGS9X_I2CEchoOn();
	Status = MxL_Tuner_Init(&myTuner);
	LGS9X_I2CEchoOff();
	if (MxL_OK != Status) {
		pr_error("Error: MxL_Tuner_Init fail!\n");
		return -1;
	}

	if(LGS_NO_ERROR != LGS9X_Init (&myDemod)){
		pr_error("Error: LGS_Demod_Init fail!\n");
		return -1;
	}

	printk("=========================demod init\r\n");

	return 0;
}

static void lgs9x_release(struct dvb_frontend *fe)
{
	struct lgs9x_state *state = fe->demodulator_priv;

	pr_dbg("lgs9x_release\n");

	kfree(state);
}

static int lgs9x_read_status(struct dvb_frontend *fe, fe_status_t * status)
{
	UINT16 ms = 0;
	UINT8 ret = 0;

	mutex_lock(&lgs_lock);
	ret = LGS9X_CheckLocked(myDemod.workMode,&locked1,&locked2,ms);
	printk("DVB: Demod lock status is %d\n",!locked1);
	mutex_unlock(&lgs_lock);

	if(LGS_NO_ERROR != ret)
		return -1;

	if(!locked1==1) {
		*status = FE_HAS_LOCK|FE_HAS_SIGNAL|FE_HAS_CARRIER|FE_HAS_VITERBI|FE_HAS_SYNC;
	} else {
		*status = FE_TIMEDOUT;
	}

	return  0;
}

static int lgs9x_read_ber(struct dvb_frontend *fe, u32 * ber)
{
	UINT8 ret = 0;
	UINT16 ms = 0;

	mutex_lock(&lgs_lock);
	ret = LGS9X_GetBER(myDemod.workMode,ms,(UINT8 *)ber);
	mutex_unlock(&lgs_lock);

	if(LGS_NO_ERROR != ret)
		return -1;

	return 0;
}

static int lgs9x_read_signal_strength(struct dvb_frontend *fe, u16 *strength)
{
	UINT8 ret = 0;

	mutex_lock(&lgs_lock);
	ret = LGS9X_GetSignalStrength(myDemod.workMode,(UINT32 *)strength);
	mutex_unlock(&lgs_lock);

	if(LGS_NO_ERROR != ret)
		return -1;

	return 0;
}

static int lgs9x_read_snr(struct dvb_frontend *fe, u16 *snr)
{
	return 0;
}

static int lgs9x_read_ucblocks(struct dvb_frontend *fe, u32 * ucblocks)
{
	ucblocks=NULL;
	return 0;
}

static int lgs9x_set_frontend(struct dvb_frontend *fe, struct dvb_frontend_parameters *p)
{
	MxL_ERR_MSG Status = MxL_OK;
	UINT32 bandwidth=8;
	struct lgs9x_state *state = fe->demodulator_priv;

	bandwidth=p->u.ofdm.bandwidth;
	if(bandwidth==0)
		bandwidth=8;
	else if(bandwidth==1)
		bandwidth=7;
	else if(bandwidth==2)
		bandwidth=6;
	else
		bandwidth=8;	

	state->freq = p->frequency;

	pr_dbg("state->freq ==== %d \n", state->freq);

	if(state->freq>0&&state->freq!=-1) {

		mutex_lock(&lgs_lock);
		LGS9X_I2CEchoOn();
		Status = MxL_Tuner_RFTune(&myTuner, state->freq, bandwidth);
		if (Status!=0) {
			pr_error("Error: MxL_Tuner_RFTune fail!\n");
			LGS9X_I2CEchoOff();
			mutex_unlock(&lgs_lock);
			return -1;
		}
		LGS9X_I2CEchoOff();
		mutex_unlock(&lgs_lock);
	} else {
		pr_error("\n--Invalidate Fre!!!!!!!!!!!!!--\n");
	}

	return  0;
}

static int lgs9x_get_frontend(struct dvb_frontend *fe, struct dvb_frontend_parameters *p)
{
	struct lgs9x_state *state = fe->demodulator_priv;

	p->frequency = state->freq;

	return 0;
}

static struct dvb_frontend_ops lgs9x_ops = {

	.info = {
	 .name = "AMLOGIC DTMB",
	.type = FE_OFDM,
	.frequency_min = 51000000,
	.frequency_max = 858000000,
	.frequency_stepsize = 166667,
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

	.init = lgs9x_init,
	.release = lgs9x_release,

	.read_status = lgs9x_read_status,
	.read_ber = lgs9x_read_ber,
	.read_signal_strength =lgs9x_read_signal_strength,
	.read_snr = lgs9x_read_snr,
	.read_ucblocks = lgs9x_read_ucblocks,

	.set_frontend = lgs9x_set_frontend,
	.get_frontend = lgs9x_get_frontend,

};

struct dvb_frontend *lgs9x_attach(const struct lgs9x_fe_config *config)
{
	struct lgs9x_state *state = NULL;

	state = kmalloc(sizeof(struct lgs9x_state), GFP_KERNEL);
	if (state == NULL)
		return NULL;

	state->config = *config;

	memcpy(&state->fe.ops, &lgs9x_ops, sizeof(struct dvb_frontend_ops));
	state->fe.demodulator_priv = state;

	return &state->fe;
}

static int lgs9x_fe_init(struct aml_dvb *advb, struct platform_device *pdev, struct aml_fe *fe, int id)
{
	struct dvb_frontend_ops *ops;
	int ret;
	struct resource *res;
	struct lgs9x_fe_config *cfg;
	char buf[32];

	cfg = kzalloc(sizeof(struct lgs9x_fe_config), GFP_KERNEL);
	if (!cfg)
		return -ENOMEM;

	if(frontend_i2c==-1) {
		snprintf(buf, sizeof(buf), "frontend%d_i2c", id);
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM, buf);
		if (!res) {
			pr_error("cannot get resource \"%s\"\n", buf);
			ret = -EINVAL;
			goto err_resource;
		}
		frontend_i2c = res->start;
	}

	if(frontend_reset==-1) {
		snprintf(buf, sizeof(buf), "frontend%d_reset", id);
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM, buf);
		if (!res) {
			pr_error("cannot get resource \"%s\"\n", buf);
			ret = -EINVAL;
			goto err_resource;
		}
		frontend_reset = res->start;
	}

	if(frontend_power==-1) {
		snprintf(buf, sizeof(buf), "frontend%d_TUNER_POWER", id);
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM, buf);
		if (!res) {
			pr_error("cannot get resource \"%s\"\n", buf);
			ret = -EINVAL;
			goto err_resource;
		}
		frontend_power = res->start;
	}

	cfg->i2c_id = frontend_i2c;

	fe->fe = lgs9x_attach(cfg); 
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

	mutex_init(&lgs_lock);

	return 0;

err_resource:
	kfree(cfg);
	return ret;
}

static void lgs9x_fe_release(struct aml_dvb *advb, struct aml_fe *fe)
{
	if(fe && fe->fe) {
		pr_dbg("release lgs9x frontend %d\n", fe->id);
		mutex_destroy(&lgs_lock);
		dvb_unregister_frontend(fe->fe);
		dvb_frontend_detach(fe->fe);
		if(fe->cfg){
			kfree(fe->cfg);
			fe->cfg = NULL;
		}
		fe->id = -1;
	}
}

int lgs9x_get_fe_config(struct  lgs9x_fe_config *cfg)
{
	struct i2c_adapter *i2c_handle;

	cfg->i2c_id = frontend_i2c;
	i2c_handle = i2c_get_adapter(cfg->i2c_id);
	if (!i2c_handle) {
		pr_error("cannot get i2c adaptor\n");
		return 0;
	}
	cfg->i2c_adapter =i2c_handle;
	return 1;
}

static int lgs9x_fe_probe(struct platform_device *pdev)
{
	struct aml_dvb *dvb = aml_get_dvb_device();

	if(lgs9x_fe_init(dvb, pdev, &lgs9x_fe[0], 0)<0)
		return -ENXIO;

	platform_set_drvdata(pdev, &lgs9x_fe[0]);

	return 0;
}

static int lgs9x_fe_remove(struct platform_device *pdev)
{
	struct aml_fe *drv_data = platform_get_drvdata(pdev);
	struct aml_dvb *dvb = aml_get_dvb_device();

	platform_set_drvdata(pdev, NULL);

	lgs9x_fe_release(dvb, drv_data);

	return 0;
}

static int lgs9x_fe_resume(struct platform_device *pdev)
{
	gpio_out(frontend_reset, 0);
	msleep(300);
	gpio_out(frontend_reset, 1); //enable tuner power
	msleep(500);

	gpio_out(frontend_power, 1);

	if(LGS_NO_ERROR != LGS9X_Init (&myDemod))
		return -1;

	return 0;
}

static int lgs9x_fe_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

static struct platform_driver aml_fe_driver = {
	.probe		= lgs9x_fe_probe,
	.remove		= lgs9x_fe_remove,	
	.resume		= lgs9x_fe_resume,
	.suspend	= lgs9x_fe_suspend,
	.driver		= {
		.name	= "lgs9x",
		.owner	= THIS_MODULE,
	}
};

static int __init lgsfrontend_init(void)
{
	return platform_driver_register(&aml_fe_driver);
}

static void __exit lgsfrontend_exit(void)
{
	platform_driver_unregister(&aml_fe_driver);
}

module_init(lgsfrontend_init);
module_exit(lgsfrontend_exit);

MODULE_DESCRIPTION("LGS9X DTMB demodulator driver");
MODULE_AUTHOR("HSW");
MODULE_LICENSE("GPL");


