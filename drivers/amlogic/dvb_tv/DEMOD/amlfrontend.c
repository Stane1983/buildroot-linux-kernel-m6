/*****************************************************************
**
**  Copyright (C) 2009 Amlogic,Inc.
**  All rights reserved
**        Filename : amlfrontend.c
**
**  comment:
**        Driver for m6_demod demodulator
**  author :
**	    Shijie.Rong@amlogic
**  version :
**	    v1.0	 12/3/13
*****************************************************************/

/*
    Driver for m6_demod demodulator
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
#include "../aml_fe.h"

#include "aml_demod.h"
#include "demod_func.h"
#include "../aml_dvb.h"
#include "amlfrontend.h"


#if 1
#define pr_dbg(args...) printk("M6_DEMOD: " args)
#else
#define pr_dbg(args...)
#endif

#define pr_error(args...) printk("M6_DEMOD: " args)

static int last_lock=-1;

static struct aml_demod_sta demod_status;


MODULE_PARM_DESC(frontend_mode, "\n\t\t Frontend mode 0-DVBC, 1-DVBT");
static int frontend_mode = -1;
module_param(frontend_mode, int, S_IRUGO);

MODULE_PARM_DESC(frontend_i2c, "\n\t\t IIc adapter id of frontend");
static int frontend_i2c = -1;
module_param(frontend_i2c, int, S_IRUGO);

MODULE_PARM_DESC(frontend_tuner, "\n\t\t Frontend tuner type 0-NULL, 1-DCT7070, 2-Maxliner, 3-FJ2207, 4-TD1316");
static int frontend_tuner = -1;
module_param(frontend_tuner, int, S_IRUGO);

MODULE_PARM_DESC(frontend_tuner_addr, "\n\t\t Tuner IIC address of frontend");
static int frontend_tuner_addr = -1;
module_param(frontend_tuner_addr, int, S_IRUGO);


static irqreturn_t amdemod_isr(int irq, void *data)
{
/*	struct aml_fe_dev *state = data;

	#define dvb_isr_islock()	(((frontend_mode==0)&&dvbc_isr_islock()) \
								||((frontend_mode==1)&&dvbt_isr_islock()))
	#define dvb_isr_monitor()       do { if(frontend_mode==1) dvbt_isr_monitor(); }while(0)
	#define dvb_isr_cancel()	do { if(frontend_mode==1) dvbt_isr_cancel(); \
		else if(frontend_mode==0) dvbc_isr_cancel();}while(0)

	
	dvb_isr_islock();
	{
		if(waitqueue_active(&state->lock_wq))
			wake_up_interruptible(&state->lock_wq);
	}

	dvb_isr_monitor();
	
	dvb_isr_cancel();*/
	
	return IRQ_HANDLED;
}

static int install_isr(struct aml_fe_dev *state)
{
	int r = 0;

	/* hook demod isr */
	pr_dbg("amdemod irq register[IRQ(%d)].\n", INT_DEMOD);
	r = request_irq(INT_DEMOD, &amdemod_isr,
				IRQF_SHARED, "amldemod",
				(void *)state);
	if (r) {
		pr_error("amdemod irq register error.\n");
	}
	return r;
}

static void uninstall_isr(struct aml_fe_dev *state)
{
	pr_dbg("amdemod irq unregister[IRQ(%d)].\n", INT_DEMOD);

	free_irq(INT_DEMOD, (void*)state);
}


static int amdemod_qam(fe_modulation_t qam)
{
	switch(qam)
	{
		case QAM_16:  return 0;
		case QAM_32:  return 1;
		case QAM_64:  return 2;
		case QAM_128:return 3;
		case QAM_256:return 4;
		default:          return 2;
	}
	return 2;
}


static int amdemod_stat_islock(struct aml_fe_dev *dev, int mode)
{
	struct aml_demod_sts demod_sts;

	if(mode==0){
		/*DVBC*/
		//dvbc_status(state->sta, state->i2c, &demod_sts);
		demod_sts.ch_sts = apb_read_reg(QAM_BASE+0x18);
		return (demod_sts.ch_sts&0x1);
	} else if (mode==1){
		/*DVBT*/
	return (apb_read_reg(DVBT_BASE+0x0)>>12)&0x1;//dvbt_get_status_ops()->get_status(&demod_sts, &demod_sta);
	}else if (mode==2){
		/*ISDBT*/
	//	return dvbt_get_status_ops()->get_status(demod_sts, demod_sta);
	}else if (mode==3){
		/*ATSC*/
			return (atsc_read_reg(0x0980)==0x79);
	}
	return 0;
}
#define amdemod_dvbc_stat_islock(dev)  amdemod_stat_islock((dev), 0)
#define amdemod_dvbt_stat_islock(dev)  amdemod_stat_islock((dev), 1)
#define amdemod_isdbt_stat_islock(dev)  amdemod_stat_islock((dev), 2)
#define amdemod_atsc_stat_islock(dev)  amdemod_stat_islock((dev), 3)


static void m6_demod_dvbc_release(struct dvb_frontend *fe)
{
	struct aml_fe_dev *state = fe->demodulator_priv;

	uninstall_isr(state);
	
	kfree(state);
}


static int m6_demod_dvbc_read_status(struct dvb_frontend *fe, fe_status_t * status)
{
//	struct aml_fe_dev *dev = afe->dtv_demod;
	struct aml_demod_sts demod_sts;
	struct aml_demod_sta demod_sta;
	struct aml_demod_i2c demod_i2c;
	int ilock;
	demod_sts.ch_sts = apb_read_reg(QAM_BASE+0x18);
//	dvbc_status(&demod_sta, &demod_i2c, &demod_sts);
	if(demod_sts.ch_sts&0x1)
	{
		ilock=1;
		*status = FE_HAS_LOCK|FE_HAS_SIGNAL|FE_HAS_CARRIER|FE_HAS_VITERBI|FE_HAS_SYNC;
	}
	else
	{
		ilock=0;
		*status = FE_TIMEDOUT;
	}
	if(last_lock != ilock){
		pr_error("%s.\n", ilock? "!!  >> LOCK << !!" : "!! >> UNLOCK << !!");
		last_lock = ilock;
	}
	
	return  0;
}

static int m6_demod_dvbc_read_ber(struct dvb_frontend *fe, u32 * ber)
{
	//struct aml_fe_dev *dev = afe->dtv_demod;
	struct aml_demod_sts demod_sts;
	struct aml_demod_i2c demod_i2c;
	struct aml_demod_sta demod_sta;


	dvbc_status(&demod_sta, &demod_i2c, &demod_sts);
	*ber = demod_sts.ch_ber;
	return 0;
}

static int m6_demod_dvbc_read_signal_strength(struct dvb_frontend *fe, u16 *strength)
{
	struct aml_fe *afe = fe->demodulator_priv;
	struct aml_fe_dev *dev = afe->dtv_demod;

	*strength=tuner_get_ch_power(dev);
	return 0;
}

static int m6_demod_dvbc_read_snr(struct dvb_frontend *fe, u16 * snr)
{
//	struct aml_fe_dev *dev = afe->dtv_demod;

	struct aml_demod_sts demod_sts;
	struct aml_demod_i2c demod_i2c;
	struct aml_demod_sta demod_sta;


	dvbc_status(&demod_sta, &demod_i2c, &demod_sts);
	*snr = demod_sts.ch_snr;	
	return 0;
}

static int m6_demod_dvbc_read_ucblocks(struct dvb_frontend *fe, u32 * ucblocks)
{
	*ucblocks=0;
	return 0;
}

extern int aml_fe_analog_set_frontend(struct dvb_frontend* fe, struct dvb_frontend_parameters* params);
static int m6_demod_dvbc_set_frontend(struct dvb_frontend *fe, struct dvb_frontend_parameters *p)
{
//	struct amlfe_state *state = fe->demodulator_priv;
	
	struct aml_demod_dvbc param;//mode 0:16, 1:32, 2:64, 3:128, 4:256
//	struct aml_demod_sta demod_sta;
	struct aml_demod_sts demod_sts;
	struct aml_demod_i2c demod_i2c;
	struct aml_fe *afe = fe->demodulator_priv;
	struct aml_fe_dev *dev = afe->dtv_demod;
	int error,count,times;
	demod_i2c.tuner=dev->drv->id;
	demod_i2c.addr=dev->i2c_addr;
	times = 2;
	
	memset(&param, 0, sizeof(param));
	param.ch_freq = p->frequency/1000;
	param.mode = amdemod_qam(p->u.qam.modulation);
	param.symb_rate = p->u.qam.symbol_rate/1000;
	
	last_lock = -1;
	pr_dbg("[m6_demod_dvbc_set_frontend]PARA demod_i2c.tuner is %d||||demod_i2c.addr is %d||||param.ch_freq is %d||||param.symb_rate is %d,param.mode is %d\n",
		demod_i2c.tuner,demod_i2c.addr,param.ch_freq,param.symb_rate,param.mode);
retry:
//	demod_i2c.tuner = 6;
//	param.mode = 2;
	aml_dmx_before_retune(AM_TS_SRC_TS2, fe);
	aml_fe_analog_set_frontend(fe,p);
	dvbc_set_ch(&demod_status, &demod_i2c, &param);
#if 0
	{
		int ret;
		ret = wait_event_interruptible_timeout(dev->lock_wq, amdemod_dvbc_stat_islock(dev), 4*HZ);
		if(!ret)	pr_error("amlfe wait lock timeout.\n");
	}
#else if
		for(count=0;count<100;count++){
			if(amdemod_dvbc_stat_islock(dev)){
				printk("first lock success\n");
				break;
			}
			msleep(20);
		}	
		if(param.mode==3){
			for(count=0;count<100;count++){
			if(amdemod_dvbc_stat_islock(dev)){
				printk("128qam second lock success\n");
				break;
			}
			msleep(20);
		}	
		}
#endif
//rsj_debug
	
    dvbc_status(&demod_status,&demod_i2c, &demod_sts);
//

	times--;
	if(amdemod_dvbc_stat_islock(dev) && times){
		int lock;

		aml_dmx_start_error_check(AM_TS_SRC_TS2, fe);
		msleep(20);
		error = aml_dmx_stop_error_check(AM_TS_SRC_TS2, fe);
		lock  = amdemod_dvbc_stat_islock(dev);
		if((error > 200) || !lock){
			pr_error("amlfe too many error, error count:%d lock statuc:%d, retry\n", error, lock);
			goto retry;
		}
	}

	aml_dmx_after_retune(AM_TS_SRC_TS2, fe);

	afe->params = *p;
	pr_dbg("AML DEMOD => frequency=%d,symbol_rate=%d\r\n",p->frequency,p->u.qam.symbol_rate);
	return  0;

}

static int m6_demod_dvbc_get_frontend(struct dvb_frontend *fe, struct dvb_frontend_parameters *p)
{//these content will be writed into eeprom .

	struct aml_fe *afe = fe->demodulator_priv;
	
	*p = afe->params;
	return 0;
}


int M6_Demod_Dvbc_Init(struct aml_fe_dev *dev)
{
//	struct amlfe_state *state; //= fe->demodulator_priv;
	struct aml_demod_sys sys;
	struct aml_demod_i2c i2c;
//	struct aml_demod_sta demod_sta;
	pr_dbg("AML Demod DVB-C init\r\n");
	memset(&sys, 0, sizeof(sys));
	memset(&i2c, 0, sizeof(i2c));
//	memset(&demod_sta, 0, sizeof(demod_sta));
	i2c.tuner = dev->drv->id;
	i2c.addr = dev->i2c_addr;
	// 0 -DVBC, 1-DVBT, ISDBT, 2-ATSC
	demod_status.dvb_mode = 0;
	sys.adc_clk=35000;
	sys.demod_clk=70000;
//	sys.adc_clk=28571;
//	sys.demod_clk=66666;

	demod_status.ch_if=6000;
	demod_set_sys(&demod_status, &i2c, &sys);

//	state->sys = sys;

}


static void m6_demod_dvbt_release(struct dvb_frontend *fe)
{
	struct aml_fe_dev *state = fe->demodulator_priv;

	uninstall_isr(state);
	
	kfree(state);
}


static int m6_demod_dvbt_read_status(struct dvb_frontend *fe, fe_status_t * status)
{
	struct aml_fe *afe = fe->demodulator_priv;
//	struct aml_fe_dev *dev = afe->dtv_demod;
	struct aml_demod_i2c demod_i2c;
	struct aml_demod_sta demod_sta;
	int ilock;
	unsigned char s=0;
	s = dvbt_get_status_ops()->get_status(&demod_sta, &demod_i2c);
	if(s==1)
	{
		ilock=1;
		*status = FE_HAS_LOCK|FE_HAS_SIGNAL|FE_HAS_CARRIER|FE_HAS_VITERBI|FE_HAS_SYNC;
	}
	else
	{
		ilock=0;
		*status = FE_TIMEDOUT;
	}
	if(last_lock != ilock){
		pr_error("%s.\n", ilock? "!!  >> LOCK << !!" : "!! >> UNLOCK << !!");
		last_lock = ilock;
	}
	
	return  0;
}

static int m6_demod_dvbt_read_ber(struct dvb_frontend *fe, u32 * ber)
{
	struct aml_fe *afe = fe->demodulator_priv;
	struct aml_demod_i2c demod_i2c;
	struct aml_demod_sta demod_sta;


	*ber = dvbt_get_status_ops()->get_ber(&demod_sta, &demod_i2c)&0xffff;
	return 0;
}

static int m6_demod_dvbt_read_signal_strength(struct dvb_frontend *fe, u16 *strength)
{
	struct aml_fe *afe = fe->demodulator_priv;
	struct aml_fe_dev *dev = afe->dtv_demod;

	*strength=tuner_get_ch_power(dev);
	return 0;
}

static int m6_demod_dvbt_read_snr(struct dvb_frontend *fe, u16 * snr)
{
//	struct aml_fe *afe = fe->demodulator_priv;
//	struct aml_demod_sts demod_sts;
	struct aml_demod_i2c demod_i2c;
	struct aml_demod_sta demod_sta;


	*snr = dvbt_get_status_ops()->get_snr(&demod_sta, &demod_i2c);
	return 0;
}

static int m6_demod_dvbt_read_ucblocks(struct dvb_frontend *fe, u32 * ucblocks)
{
	*ucblocks=0;
	return 0;
}



static int m6_demod_dvbt_set_frontend(struct dvb_frontend *fe, struct dvb_frontend_parameters *p)
{	
	struct aml_demod_sta demod_sta;
	struct aml_demod_sts demod_sts;
	struct aml_demod_i2c demod_i2c;
	struct aml_fe *afe = fe->demodulator_priv;
	struct aml_fe_dev *dev = afe->dtv_demod;
	
	demod_i2c.tuner=dev->drv->id;
	demod_i2c.addr=dev->i2c_addr;
	int error,times,count;
	times = 2;

	struct aml_demod_dvbt param;
	
    //////////////////////////////////////
    // bw == 0 : 8M 
    //       1 : 7M
    //       2 : 6M
    //       3 : 5M
    // agc_mode == 0: single AGC
    //             1: dual AGC
    //////////////////////////////////////
    	memset(&param, 0, sizeof(param));
	param.ch_freq = p->frequency/1000;
	param.bw = p->u.ofdm.bandwidth;
	param.agc_mode = 1;
	/*ISDBT or DVBT : 0 is QAM, 1 is DVBT, 2 is ISDBT, 3 is DTMB, 4 is ATSC */
	param.dat0 = 1;
	last_lock = -1;

retry:
	aml_dmx_before_retune(AM_TS_SRC_TS2, fe);
	aml_fe_analog_set_frontend(fe,p);
	dvbt_set_ch(&demod_sta, &demod_i2c, &param);
	
		for(count=0;count<10;count++){
			if(amdemod_dvbt_stat_islock(dev)){
				printk("first lock success\n");
				break;
			}
				
			msleep(300);
		}	
//rsj_debug
	
//

	times--;
	if(amdemod_dvbt_stat_islock(dev) && times){
		int lock;

		aml_dmx_start_error_check(AM_TS_SRC_TS2, fe);
		msleep(20);
		error = aml_dmx_stop_error_check(AM_TS_SRC_TS2, fe);
		lock  = amdemod_dvbt_stat_islock(dev);
		if((error > 200) || !lock){
			pr_error("amlfe too many error, error count:%d lock statuc:%d, retry\n", error, lock);
			goto retry;
		}
	}

	aml_dmx_after_retune(AM_TS_SRC_TS2, fe);

	afe->params = *p;
//	pr_dbg("AML DEMOD => frequency=%d,symbol_rate=%d\r\n",p->frequency,p->u.qam.symbol_rate);
	return  0;

}

static int m6_demod_dvbt_get_frontend(struct dvb_frontend *fe, struct dvb_frontend_parameters *p)
{//these content will be writed into eeprom .

	struct aml_fe *afe = fe->demodulator_priv;
	
	*p = afe->params;
	return 0;
}



int M6_Demod_Dvbt_Init(struct aml_fe_dev *dev)
{
//	struct amlfe_state *state; //= fe->demodulator_priv;
	struct aml_demod_sys sys;
	struct aml_demod_i2c i2c;
//	struct aml_demod_sta demod_sta;
	
	pr_dbg("AML Demod DVB-T init\r\n");

	memset(&sys, 0, sizeof(sys));
	memset(&i2c, 0, sizeof(i2c));
	memset(&demod_status, 0, sizeof(demod_status));
	i2c.tuner = dev->drv->id;
	i2c.addr = dev->i2c_addr;
	// 0 -DVBC, 1-DVBT, ISDBT, 2-ATSC
	demod_status.dvb_mode = 1;	
	sys.adc_clk=28571;
	sys.demod_clk=66666;
	demod_set_sys(&demod_status, &i2c, &sys);

//	state->sys = sys;

}



static void m6_demod_atsc_release(struct dvb_frontend *fe)
{
	struct aml_fe_dev *state = fe->demodulator_priv;

	uninstall_isr(state);
	
	kfree(state);
}


static int m6_demod_atsc_read_status(struct dvb_frontend *fe, fe_status_t * status)
{
	struct aml_fe *afe = fe->demodulator_priv;
	struct aml_fe_dev *dev = afe->dtv_demod;
	struct aml_demod_i2c demod_i2c;
	struct aml_demod_sta demod_sta;
	int ilock;
	unsigned char s=0;
	s = amdemod_atsc_stat_islock(dev);
	if(s==1)
	{
		ilock=1;
		*status = FE_HAS_LOCK|FE_HAS_SIGNAL|FE_HAS_CARRIER|FE_HAS_VITERBI|FE_HAS_SYNC;
	}
	else
	{
		ilock=0;
		*status = FE_TIMEDOUT;
	}
	if(last_lock != ilock){
		pr_error("%s.\n", ilock? "!!  >> LOCK << !!" : "!! >> UNLOCK << !!");
		last_lock = ilock;
	}
	
	return  0;
}

static int m6_demod_atsc_read_ber(struct dvb_frontend *fe, u32 * ber)
{
	struct aml_fe *afe = fe->demodulator_priv;
	struct aml_fe_dev *dev = afe->dtv_demod;
	struct aml_demod_sts demod_sts;
	struct aml_demod_i2c demod_i2c;
	struct aml_demod_sta demod_sta;

	check_atsc_fsm_status();
	return 0;
}

static int m6_demod_atsc_read_signal_strength(struct dvb_frontend *fe, u16 *strength)
{
	struct aml_fe *afe = fe->demodulator_priv;
	struct aml_fe_dev *dev = afe->dtv_demod;

	*strength=tuner_get_ch_power(dev);
	return 0;
}

static int m6_demod_atsc_read_snr(struct dvb_frontend *fe, u16 * snr)
{
	struct aml_fe *afe = fe->demodulator_priv;
	struct aml_fe_dev *dev = afe->dtv_demod;

	struct aml_demod_sts demod_sts;
	struct aml_demod_i2c demod_i2c;
	struct aml_demod_sta demod_sta;

	* snr=check_atsc_fsm_status();
	return 0;
}

static int m6_demod_atsc_read_ucblocks(struct dvb_frontend *fe, u32 * ucblocks)
{
	*ucblocks=0;
	return 0;
}

static int m6_demod_atsc_set_frontend(struct dvb_frontend *fe, struct dvb_frontend_parameters *p)
{
//	struct amlfe_state *state = fe->demodulator_priv;
	
	struct aml_demod_atsc param;
	struct aml_demod_sta demod_sta;
	struct aml_demod_sts demod_sts;
	struct aml_demod_i2c demod_i2c;
	struct aml_fe *afe = fe->demodulator_priv;
	struct aml_fe_dev *dev = afe->dtv_demod;
	
	demod_i2c.tuner=dev->drv->id;
	demod_i2c.addr=dev->i2c_addr;
	int error;
	int times = 2;
	
	memset(&param, 0, sizeof(param));
	param.ch_freq = p->frequency/1000;
	
	last_lock = -1;

retry:
	aml_dmx_before_retune(AM_TS_SRC_TS2, fe);
	aml_fe_analog_set_frontend(fe,p);
	atsc_set_ch(&demod_sta, &demod_i2c, &param);

	/*{
		int ret;
		ret = wait_event_interruptible_timeout(dev->lock_wq, amdemod_atsc_stat_islock(dev), 4*HZ);
		if(!ret)	pr_error("amlfe wait lock timeout.\n");
	}*/
//rsj_debug
		int count;
		for(count=0;count<10;count++){
			if(amdemod_atsc_stat_islock(dev)){
				printk("first lock success\n");
				break;
			}
				
			msleep(200);
		}	

	times--;
	if(amdemod_atsc_stat_islock(dev) && times){
		int lock;

		aml_dmx_start_error_check(AM_TS_SRC_TS2, fe);
		msleep(20);
		error = aml_dmx_stop_error_check(AM_TS_SRC_TS2, fe);
		lock  = amdemod_atsc_stat_islock(dev);
		if((error > 200) || !lock){
			pr_error("amlfe too many error, error count:%d lock statuc:%d, retry\n", error, lock);
			goto retry;
		}
	}

	aml_dmx_after_retune(AM_TS_SRC_TS2, fe);

	afe->params = *p;
//	pr_dbg("AML DEMOD => frequency=%d,symbol_rate=%d\r\n",p->frequency,p->u.qam.symbol_rate);
	return  0;

}

static int m6_demod_atsc_get_frontend(struct dvb_frontend *fe, struct dvb_frontend_parameters *p)
{//these content will be writed into eeprom .

	struct aml_fe *afe = fe->demodulator_priv;
	
	*p = afe->params;
	return 0;
}



int M6_Demod_Atsc_Init(struct aml_fe_dev *dev)
{
//	struct amlfe_state *state; //= fe->demodulator_priv;
	struct aml_demod_sys sys;
	struct aml_demod_i2c i2c;
//	struct aml_demod_sta demod_sta;
	
	pr_dbg("AML Demod ATSC init\r\n");

	memset(&sys, 0, sizeof(sys));
	memset(&i2c, 0, sizeof(i2c));
	memset(&demod_status, 0, sizeof(demod_status));
	// 0 -DVBC, 1-DVBT, ISDBT, 2-ATSC
	demod_status.dvb_mode = 2;
	sys.adc_clk=26000;
	sys.demod_clk=78000;
	demod_set_sys(&demod_status, &i2c, &sys);

//	state->sys = sys;

}






static int m6_demod_fe_get_ops(struct aml_fe_dev *dev, int mode, void *ops)
{
	struct dvb_frontend_ops *fe_ops = (struct dvb_frontend_ops*)ops;
	if(mode == AM_FE_OFDM){

	fe_ops->info.frequency_min = 51000000;
	fe_ops->info.frequency_max = 858000000;
	fe_ops->info.frequency_stepsize = 0;
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
	fe_ops->set_frontend = m6_demod_dvbt_set_frontend;
	fe_ops->get_frontend = m6_demod_dvbt_get_frontend;	
	fe_ops->read_status = m6_demod_dvbt_read_status;
	fe_ops->read_ber = m6_demod_dvbt_read_ber;
	fe_ops->read_signal_strength = m6_demod_dvbt_read_signal_strength;
	fe_ops->read_snr = m6_demod_dvbt_read_snr;
	fe_ops->read_ucblocks = m6_demod_dvbt_read_ucblocks;

	pr_dbg("=========================dvbt demod init\r\n");
	M6_Demod_Dvbt_Init(dev);
	}
	else if(mode == AM_FE_QAM){
	fe_ops->info.frequency_min = 51000000;
	fe_ops->info.frequency_max = 858000000;
	fe_ops->info.frequency_stepsize = 0;
	fe_ops->info.frequency_tolerance = 0;
	fe_ops->info.caps = FE_CAN_FEC_1_2 | FE_CAN_FEC_2_3 | FE_CAN_FEC_3_4 |
			FE_CAN_FEC_5_6 | FE_CAN_FEC_7_8 | FE_CAN_FEC_AUTO |
			FE_CAN_QPSK | FE_CAN_QAM_16 |FE_CAN_QAM_32|FE_CAN_QAM_128|FE_CAN_QAM_256|
			FE_CAN_QAM_64 | FE_CAN_QAM_AUTO |
			FE_CAN_TRANSMISSION_MODE_AUTO |
			FE_CAN_GUARD_INTERVAL_AUTO |
			FE_CAN_HIERARCHY_AUTO |
			FE_CAN_RECOVER |
			FE_CAN_MUTE_TS;

	fe_ops->release = m6_demod_dvbc_release;
	fe_ops->set_frontend = m6_demod_dvbc_set_frontend;
	fe_ops->get_frontend = m6_demod_dvbc_get_frontend;	
	fe_ops->read_status = m6_demod_dvbc_read_status;
	fe_ops->read_ber = m6_demod_dvbc_read_ber;
	fe_ops->read_signal_strength = m6_demod_dvbc_read_signal_strength;
	fe_ops->read_snr = m6_demod_dvbc_read_snr;
	fe_ops->read_ucblocks = m6_demod_dvbc_read_ucblocks;

	init_waitqueue_head(&dev->lock_wq);
	install_isr(dev);
	pr_dbg("=========================dvbc demod init\r\n");
	M6_Demod_Dvbc_Init(dev);
	}else if(mode == AM_FE_ATSC){
	
	fe_ops->info.frequency_min = 51000000;
	fe_ops->info.frequency_max = 858000000;
	fe_ops->info.frequency_stepsize = 0;
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

	fe_ops->release = m6_demod_atsc_release;
	fe_ops->set_frontend = m6_demod_atsc_set_frontend;
	fe_ops->get_frontend = m6_demod_atsc_get_frontend;	
	fe_ops->read_status = m6_demod_atsc_read_status;
	fe_ops->read_ber = m6_demod_atsc_read_ber;
	fe_ops->read_signal_strength = m6_demod_atsc_read_signal_strength;
	fe_ops->read_snr = m6_demod_atsc_read_snr;
	fe_ops->read_ucblocks = m6_demod_atsc_read_ucblocks;
	M6_Demod_Atsc_Init(dev);
	}
	return 0;
}

static int m6_demod_fe_resume(struct aml_fe_dev *dev)
{
	pr_dbg("m6_demod_fe_resume\n");
//	M6_Demod_Dvbc_Init(dev);
	return 0;

}

static int m6_demod_fe_suspend(struct aml_fe_dev *dev)
{
	return 0;
}

static int m6_demod_fe_enter_mode(struct aml_fe *fe, int mode)
{
	/*struct aml_fe_dev *dev=fe->dtv_demod;
	printk("fe->mode is %d",fe->mode);
	if(fe->mode==AM_FE_OFDM){
		M1_Demod_Dvbt_Init(dev);
	}else if(fe->mode==AM_FE_QAM){
		M1_Demod_Dvbc_Init(dev);
	}else if (fe->mode==AM_FE_ATSC){
		M6_Demod_Atsc_Init(dev);
	}*/
	return 0;
}

static int m6_demod_fe_leave_mode(struct aml_fe *fe, int mode)
{
	return 0;
}




static struct aml_fe_drv m6_demod_dtv_demod_drv = {
.id         = AM_DTV_DEMOD_M1,
.name       = "M6_DEMOD",
.capability = AM_FE_QPSK|AM_FE_QAM|AM_FE_ATSC|AM_FE_OFDM,
.get_ops    = m6_demod_fe_get_ops,
.suspend    = m6_demod_fe_suspend,
.resume     = m6_demod_fe_resume,
.enter_mode = m6_demod_fe_enter_mode,
.leave_mode = m6_demod_fe_leave_mode
};

static int __init m6demodfrontend_init(void)
{
	pr_dbg("register m6_demod demod driver\n");
	return aml_register_fe_drv(AM_DEV_DTV_DEMOD, &m6_demod_dtv_demod_drv);
}


static void __exit m6demodfrontend_exit(void)
{
	pr_dbg("unregister m6_demod demod driver\n");
	aml_unregister_fe_drv(AM_DEV_DTV_DEMOD, &m6_demod_dtv_demod_drv);
}

fs_initcall(m6demodfrontend_init);
module_exit(m6demodfrontend_exit);


MODULE_DESCRIPTION("m6_demod DVB-T/DVB-C Demodulator driver");
MODULE_AUTHOR("RSJ");
MODULE_LICENSE("GPL");


