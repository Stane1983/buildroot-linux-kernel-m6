/*****************************************************************
**
**  Copyright (C) 2010 Amlogic,Inc.
**  All rights reserved
**        Filename : amlfrontend.c
**
**  comment:
**        Driver for aml demodulator
** 
*****************************************************************/

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <linux/platform_device.h>

#include <mach/am_regs.h>

#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/jiffies.h>

#include "aml_demod.h"
#include "demod_func.h"
#include "../dvb/aml_dvb.h"
#include "amlfrontend.h"

#define AMDEMOD_FE_DEV_COUNT 1

#include <linux/errno.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <asm/uaccess.h>
#include <linux/kthread.h>

#include <linux/init.h>
#include <linux/dcache.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <asm/fcntl.h>
#include <asm/processor.h>



static int debug_amlfe = 1;
module_param(debug_amlfe, int, 0644);
MODULE_PARM_DESC(debug_amlfe, "turn on debugging (default: 0)");

static int autotune = 1;
module_param(autotune, int, 0644);
MODULE_PARM_DESC(autotune, "auto tune when lost lock (default: 1)");

#if 1
#define pr_dbg(fmt, args...) do{ if(debug_amlfe) printk("AMLFE: " fmt, ## args); } while(0)
#else
#define pr_dbg(fmt, args...)
#endif

#define pr_error(fmt, args...) printk(KERN_ERR "AMLFE: " fmt, ## args)



struct amlfe_state {
	struct amlfe_config config;
	struct aml_demod_sys sys;
	
	struct aml_demod_sta *sta;
	struct aml_demod_i2c *i2c;
	
	u32                       freq;
	fe_modulation_t     mode;
	u32                       symbol_rate;
	struct dvb_frontend fe;

	wait_queue_head_t  lock_wq;
};

MODULE_PARM_DESC(frontend_mode, "\n\t\t Frontend mode 0-DVBC, 1-DVBT, 2-ISDBT, 3-DTMB, 4-ATSC");
static int frontend_mode = -1;
module_param(frontend_mode, int, S_IRUGO);

MODULE_PARM_DESC(frontend_i2c, "\n\t\t IIc adapter id of frontend");
static int frontend_i2c = -1;
module_param(frontend_i2c, int, S_IRUGO);

MODULE_PARM_DESC(frontend_tuner, "\n\t\t Frontend tuner type 0-NULL, 1-DCT7070, 2-Maxliner, 3-FJ2207, 4-TD1316, 5-RDA5880, 6-TDA18273, 7-Si2176");
static int frontend_tuner = -1;
module_param(frontend_tuner, int, S_IRUGO);

MODULE_PARM_DESC(frontend_tuner_addr, "\n\t\t Tuner IIC address of frontend");
static int frontend_tuner_addr = -1;
module_param(frontend_tuner_addr, int, S_IRUGO);

static struct aml_fe amdemod_fe[1];/*[AMDEMOD_FE_DEV_COUNT] = 
	{[0 ... (AMDEMOD_FE_DEV_COUNT-1)] = {-1, NULL, NULL}};*/

#ifndef CONFIG_AM_DEMOD_DEBUG
static struct aml_demod_i2c demod_i2c;
static struct aml_demod_sta demod_sta;
#else
  struct aml_demod_i2c demod_i2c;
  struct aml_demod_sta demod_sta;
#endif

#ifdef CONFIG_AM_DEMOD_DEBUG
	int aml_demod_init(void);
	void aml_demod_exit(void);
#endif

static int last_lock=-1;
#ifdef CONFIG_AM_AMDEMOD_FPGA_VER
  
  int sdio_init(void){}
  int sdio_exit(void){}
  
#endif

#ifdef CONFIG_AM_AMDEMOD_FPGA_VER
  #include <linux/timer.h>

  static struct timer_list mon_timer;
  static int timer_running;
  extern int dvbt_isdbt_monitor(void);

  void fpga_monitor(unsigned long p)
  {
  	dvbt_isdbt_monitor();
	if(timer_running)
	       mod_timer(&mon_timer, jiffies + (HZ*1/1000));	
  }

  int fpga_moniotr_init(void)
  {
      printk("fpga monitor init.\n");
      init_timer(&mon_timer);
      mon_timer.function = &fpga_monitor;
      mon_timer.data = 0;
      return 0;
  }
  int fpga_monitor_exit(void)
  {
      printk("fpga monitor exit\n");
      del_timer_sync(&mon_timer);
      return 0;
  }
  int fpga_monitor_restart(void)
  {
      if(timer_running) {
	printk("fpga monitor stop\n");
        timer_running=0;
        del_timer_sync(&mon_timer);
        del_timer(&mon_timer);
      }

      printk("fpga moniotr start\n");
      timer_running = 1;
      mon_timer.expires = jiffies + (1*HZ/1000)/*1ms*/;
      add_timer(&mon_timer);
      return 0;
  }
  int fpga_monitor_stop(void)
  {
      timer_running = 0;
      return 0;
  }
#else
static irqreturn_t amdemod_isr(int irq, void *data)
{
	struct amlfe_state *state = data;

	#define dvb_isr_islock()	(((frontend_mode==0)&&dvbc_isr_islock()) \
								||((frontend_mode==1)&&dvbt_isr_islock()))
	#define dvb_isr_monitor() do { if(frontend_mode==1) dvbt_isr_monitor(); }while(0)
	#define dvb_isr_cancel()	do { if(frontend_mode==1) dvbt_isr_cancel(); }while(0)
	
	if(dvb_isr_islock()) {
		if(waitqueue_active(&state->lock_wq))
			wake_up_interruptible(&state->lock_wq);
	}

	dvb_isr_monitor();
	
	dvb_isr_cancel();
	
	return IRQ_HANDLED;
}
#endif

static int install_isr(struct amlfe_state *state)
{
#ifdef CONFIG_AM_AMDEMOD_FPGA_VER

	fpga_moniotr_init();

	return 0;

#else

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

#endif

}

static void uninstall_isr(struct amlfe_state *state)
{
#ifdef CONFIG_AM_AMDEMOD_FPGA_VER

	fpga_monitor_exit();

#else

	pr_dbg("amdemod irq unregister[IRQ(%d)].\n", INT_DEMOD);

	free_irq(INT_DEMOD, (void*)state);

#endif	
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

static int amdemod_stat_islock(struct amlfe_state *state, int mode)
{
	struct aml_demod_sts demod_sts;

	if(mode==0){
		/*DVBC*/
		//dvbc_status(state->sta, state->i2c, &demod_sts);
		demod_sts.ch_sts = apb_read_reg(0, 0x18);
		return (demod_sts.ch_sts&0x1);
	} else if (mode==1){
		/*DVBT*/
		return dvbt_get_status_ops()->get_status(state->sta, state->i2c);
	} else if (mode==2){
		/*ISDBT*/
		return ((apb_read_reg(2, 0x2a)&0xf) == 0x9);  
	} else if (mode==3){
		/*DTMB*/
		return (((apb_read_reg(3,227))>> 24)& 0x1);  
	}
	else if (mode==4){
		/*ATSC*/
		
		return (apb_read_reg(4,(0x0980))==0x79);;

	}
	return 0;
}
#define amdemod_dvbc_stat_islock(state)  amdemod_stat_islock((state), 0)
#define amdemod_dvbt_stat_islock(state)  amdemod_stat_islock((state), 1)
#define amdemod_isdbt_stat_islock(state)  amdemod_stat_islock((state), 2)
#define amdemod_dtmb_stat_islock(state)  amdemod_stat_islock((state), 3)
#define amdemod_atsc_stat_islock(state)  amdemod_stat_islock((state), 4)

static int amdemod_frontend_param_equal(struct dvb_frontend_parameters *p1,
			struct dvb_frontend_parameters *p2, 
			int mode)
{
#define peq(_p1, _p2, _f) ((_p1)->_f == (_p2)->_f)
#define eq(f) peq(p1,p2, f)

	if(!eq(frequency) || !eq(inversion))
		return 0;

	switch(mode) {
		/*DVBC*/
		case 0:
			return eq(u.qam.symbol_rate) && eq(u.qam.fec_inner) && eq(u.qam.modulation);
			break;
		case 1:
			return eq(u.ofdm.bandwidth) && eq(u.ofdm.code_rate_HP) && eq(u.ofdm.code_rate_LP)
				&& eq(u.ofdm.constellation) && eq(u.ofdm.transmission_mode)
				&& eq(u.ofdm.guard_interval) && eq(u.ofdm.hierarchy_information);
			break;
		case 2:
			return eq(u.ofdm.bandwidth) && eq(u.ofdm.code_rate_HP) && eq(u.ofdm.code_rate_LP)
				&& eq(u.ofdm.constellation) && eq(u.ofdm.transmission_mode)
				&& eq(u.ofdm.guard_interval) && eq(u.ofdm.hierarchy_information);
			break;
		case 3:
			return eq(u.ofdm.bandwidth) && eq(u.ofdm.code_rate_HP) && eq(u.ofdm.code_rate_LP)
				&& eq(u.ofdm.constellation) && eq(u.ofdm.transmission_mode)
				&& eq(u.ofdm.guard_interval) && eq(u.ofdm.hierarchy_information);
			break;
		case 4:
			return eq(u.ofdm.bandwidth) && eq(u.ofdm.code_rate_HP) && eq(u.ofdm.code_rate_LP)
				&& eq(u.ofdm.constellation) && eq(u.ofdm.transmission_mode)
				&& eq(u.ofdm.guard_interval) && eq(u.ofdm.hierarchy_information);
			break;
		default:
			break;
			
	}
	return !memcmp(p1, p2, sizeof(struct dvb_frontend_parameters));
}
#define amdemod_dvbc_frontend_param_equal(p1, p2)  amdemod_frontend_param_equal((p1), (p2), 0)
#define amdemod_dvbt_frontend_param_equal(p1, p2)  amdemod_frontend_param_equal((p1), (p2), 1)
#define amdemod_isdbt_frontend_param_equal(p1, p2)  amdemod_frontend_param_equal((p1), (p2), 2)
#define amdemod_dtmb_frontend_param_equal(p1, p2)  amdemod_frontend_param_equal((p1), (p2), 3)
#define amdemod_atsc_frontend_param_equal(p1, p2)  amdemod_frontend_param_equal((p1), (p2), 4)


static int aml_fe_dvb_i2c_gate_ctrl(struct dvb_frontend *fe, int enable)
{
	//struct amlfe_state *state = fe->demodulator_priv;

	return 0;
}

static int aml_fe_dvbc_sleep(struct dvb_frontend *fe)
{
	//struct amlfe_state *state = fe->demodulator_priv;
	return 0;
}

static int aml_fe_dvbt_sleep(struct dvb_frontend *fe)
{
	dvbt_shutdown();
	return 0;
}

static int aml_fe_isdbt_sleep(struct dvb_frontend *fe)
{
	//isdbt_shutdown();
	return 0;
}

static int aml_fe_dtmb_sleep(struct dvb_frontend *fe)
{
	//dtmb_shutdown();
	return 0;
}

static int aml_fe_atsc_sleep(struct dvb_frontend *fe)
{
	//atsc_shutdown();
	return 0;
}




static int _prepare_i2c(struct dvb_frontend *fe, struct aml_demod_i2c *i2c)
{
	struct amlfe_state *state = fe->demodulator_priv;
	struct i2c_adapter *adapter;
	
	adapter = i2c_get_adapter(state->config.i2c_id);
	i2c->i2c_id = state->config.i2c_id;
	i2c->i2c_priv = adapter;
	if(!adapter){
		pr_error("can not get i2c adapter[%d] \n", state->config.i2c_id);
		return -1;
	}
	return 0;
}

static int aml_fe_dvbc_init(struct dvb_frontend *fe)
{
	struct amlfe_state *state = fe->demodulator_priv;
	struct aml_demod_sys sys;
	struct aml_demod_i2c i2c;
	
	pr_dbg("AML Demod DVB-C init\r\n");

	memset(&sys, 0, sizeof(sys));
	memset(&i2c, 0, sizeof(i2c));
	
    	sys.clk_en = 1;
	sys.clk_src = 0;
	sys.clk_div = 0;
	sys.pll_n = 1;
	sys.pll_m = 50;
	sys.pll_od = 0;
	sys.pll_sys_xd = 20;
	sys.pll_adc_xd = 21;
	sys.agc_sel = 2;
	sys.adc_en = 1;
	sys.i2c = (long)&i2c;
	sys.debug = 0;
	i2c.tuner = state->config.tuner_type;
	i2c.addr = state->config.tuner_addr;

	_prepare_i2c(fe, &i2c);
	
	demod_set_sys(state->sta, state->i2c, &sys);

	state->sys = sys;
	
	return 0;
}

static int aml_fe_dvbc_read_status(struct dvb_frontend *fe, fe_status_t * status)
{
	struct amlfe_state *state = fe->demodulator_priv;
	struct aml_demod_sts demod_sts;
	int ilock;
	
	dvbc_status(state->sta, state->i2c, &demod_sts);
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

static int aml_fe_dvbc_read_ber(struct dvb_frontend *fe, u32 * ber)
{
	struct amlfe_state *state = fe->demodulator_priv;
	struct aml_demod_sts demod_sts;

	dvbc_status(state->sta, state->i2c, &demod_sts);
	*ber = demod_sts.ch_ber;
	
	return 0;
}

static int aml_fe_dvbc_read_signal_strength(struct dvb_frontend *fe, u16 *strength)
{
	struct amlfe_state *state = fe->demodulator_priv;
	struct aml_demod_sts demod_sts;
	
	dvbc_status(state->sta, state->i2c, &demod_sts);
	*strength=demod_sts.ch_pow& 0xffff;

	return 0;
}

static int aml_fe_dvbc_read_snr(struct dvb_frontend *fe, u16 * snr)
{
	struct amlfe_state *state = fe->demodulator_priv;
	struct aml_demod_sts demod_sts;
	
	dvbc_status(state->sta, state->i2c, &demod_sts);
	*snr = demod_sts.ch_snr/100;
	
	return 0;
}

static int aml_fe_dvbc_read_ucblocks(struct dvb_frontend *fe, u32 * ucblocks)
{
	struct amlfe_state *state = fe->demodulator_priv;
	struct aml_demod_sts demod_sts;
	dvbc_status(state->sta, state->i2c, &demod_sts);
	*ucblocks = demod_sts.ch_per;
	return 0;
}

static int aml_fe_dvbc_set_frontend(struct dvb_frontend *fe, struct dvb_frontend_parameters *p)
{
	struct amlfe_state *state = fe->demodulator_priv;
	struct aml_demod_dvbc param;//mode 0:16, 1:32, 2:64, 3:128, 4:256
	
	memset(&param, 0, sizeof(param));
	param.ch_freq = p->frequency/1000;
	param.mode = amdemod_qam(p->u.qam.modulation);
	param.symb_rate = p->u.qam.symbol_rate/1000;
	
	last_lock = -1;

	dvbc_set_ch(state->sta, state->i2c, &param);

	{
		int ret;
		ret = wait_event_interruptible_timeout(state->lock_wq, amdemod_dvbc_stat_islock(state), 2*HZ);
		if(!ret)	pr_error("amlfe wait lock timeout.\n");
	}

	state->freq=p->frequency;
	state->mode=p->u.qam.modulation ;
	state->symbol_rate=p->u.qam.symbol_rate;
	
	pr_dbg("AML DEMOD => frequency=%d,symbol_rate=%d\r\n",p->frequency,p->u.qam.symbol_rate);
	return  0;
}

static int aml_fe_dvbc_get_frontend(struct dvb_frontend *fe, struct dvb_frontend_parameters *p)
{//these content will be writed into eeprom .

	struct amlfe_state *state = fe->demodulator_priv;
	
	p->frequency=state->freq;
	p->u.qam.modulation=state->mode;
	p->u.qam.symbol_rate=state->symbol_rate;
	
	return 0;
}

static int aml_fe_dvbt_init(struct dvb_frontend *fe)
{
	struct amlfe_state *state = fe->demodulator_priv;
	struct aml_demod_sys sys;
	struct aml_demod_i2c i2c;
	
	pr_dbg("AML Demod DVB-T init\r\n");

	memset(&sys, 0, sizeof(sys));
	memset(&i2c, 0, sizeof(i2c));
	
    	sys.clk_en = 1;
	sys.clk_src = 0;
	sys.clk_div = 0;
	sys.pll_n = 1;
	sys.pll_m = 50;
	sys.pll_od = 0;
	sys.pll_sys_xd = 20;//20;
	if (state->config.tuner_type ==5)  sys.pll_adc_xd = 14;// 42.857M
	else	                             sys.pll_adc_xd = 21;// 28.571M
	sys.agc_sel = 2;
	sys.adc_en = 1;
	sys.i2c = (long)&i2c;
	sys.debug = 0;
	i2c.tuner = state->config.tuner_type;
	i2c.addr = state->config.tuner_addr;
	
	_prepare_i2c(fe, &i2c);
#ifndef CONFIG_AM_DEMOD_FPGA_VER
	demod_set_sys(state->sta, state->i2c, &sys);
#endif
	state->sys = sys;
	
	return 0;
}


static int aml_fe_dvbt_read_status(struct dvb_frontend *fe, fe_status_t * status)
{
	struct amlfe_state *state = fe->demodulator_priv;
	int ilock;
	
	if(dvbt_get_status_ops()->get_status(state->sta, state->i2c))
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
		pr_dbg("%s.\n", ilock? "!!  >> LOCK << !!" : "!! >> UNLOCK << !!");
		last_lock = ilock;
	}
	
	return  0;
}

static int aml_fe_dvbt_read_ber(struct dvb_frontend *fe, u32 * ber)
{
	struct amlfe_state *state = fe->demodulator_priv;
	*ber = dvbt_get_status_ops()->get_ber(state->sta, state->i2c)&0xffff;
	return 0;
}

static int aml_fe_dvbt_read_signal_strength(struct dvb_frontend *fe, u16 *strength)
{
	struct amlfe_state *state = fe->demodulator_priv;
	*strength=dvbt_get_status_ops()->get_strength(state->sta, state->i2c);
	return 0;
}

static int aml_fe_dvbt_read_snr(struct dvb_frontend *fe, u16 * snr)
{
	struct amlfe_state *state = fe->demodulator_priv;
	*snr = dvbt_get_status_ops()->get_snr(state->sta, state->i2c);
	return 0;
}


static int aml_fe_dvbt_read_ucblocks(struct dvb_frontend *fe, u32 * ucblocks)
{
	struct amlfe_state *state = fe->demodulator_priv;
	*ucblocks = dvbt_get_status_ops()->get_ucblocks(state->sta, state->i2c);
	return 0;
}


static int amdemod_dvbt_tune(struct amlfe_state *state, struct dvb_frontend_parameters *p)
{
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
	
	dvbt_set_ch(state->sta, state->i2c, &param);
#if 0
        {
                int ret;
                ret = wait_event_interruptible_timeout(state->lock_wq, amdemod_dvbt_stat_islock(state), 2*HZ);
                if(!ret)        pr_error("amlfe wait lock timeout.\n");
        }
#else
        {
                int wait = 1000/20;//1s
                while (wait)
                {
                    if(amdemod_dvbt_stat_islock(state)) break;
                    wait--;
                    msleep(2);              //Delay 20 ms   
                }
        }
#endif

	return 0;
}



static int aml_fe_dvbt_set_frontend(struct dvb_frontend *fe, struct dvb_frontend_parameters *p)
{
	struct amlfe_state *state = fe->demodulator_priv;

	amdemod_dvbt_tune(state, p);
	
	state->freq=p->frequency;
	state->mode=p->u.ofdm.bandwidth;
	
	pr_dbg("AML DEMOD => frequency=%d,bw=%d\r\n",p->frequency,p->u.ofdm.bandwidth);
	return  0;
}

static enum dvbfe_algo aml_fe_dvbt_get_frontend_algo(struct dvb_frontend *fe)
{
	return DVBFE_ALGO_HW;
}

static int aml_fe_dvbt_tune(struct dvb_frontend* fe,
		    struct dvb_frontend_parameters* params,
		    unsigned int mode_flags,
		    unsigned int *delay,
		    fe_status_t *status)
{
	static struct dvb_frontend_parameters last_params;
	
	struct amlfe_state *state = fe->demodulator_priv;
	int locked = amdemod_dvbt_stat_islock(state);
	int retune = (params && !amdemod_dvbt_frontend_param_equal(params, &last_params))? 1 : 0;
	int check_delay;
	
	pr_dbg("dvbt tune retune:%d, locked:0x%x\n", retune, locked);

	if(!locked && !retune) {
		/*Fixme: frequency offset correction*/
	}
	
	if(!locked || retune) {
		amdemod_dvbt_tune(state, params?params:&last_params);
		check_delay = 5 * HZ;
	} else {
		check_delay = HZ;
	}

	if(amdemod_dvbt_stat_islock(state))
		*status = (FE_HAS_LOCK|FE_HAS_SIGNAL|FE_HAS_CARRIER|FE_HAS_VITERBI|FE_HAS_SYNC);
	else
		*status = FE_TIMEDOUT;

	*delay = check_delay;

	if(retune) {
		state->freq=params->frequency;
		state->mode=params->u.ofdm.bandwidth;

		memcpy(&last_params, params, sizeof(struct dvb_frontend_parameters));
	}
	
	return 0;
}

static int aml_fe_dvbt_get_frontend(struct dvb_frontend *fe, struct dvb_frontend_parameters *p)
{//these content will be writed into eeprom .

	struct amlfe_state *state = fe->demodulator_priv;
      	int  code_rate_HP, code_rate_LP, constellation, 
		transmission_mode, guard_interval, hierarchy_information;
	fe_code_rate_t code_rate;
	
	p->frequency=state->freq;
	p->u.ofdm.bandwidth=state->mode;

	dvbt_get_params(state->sta, state->i2c, 
		&code_rate_HP, &code_rate_LP, &constellation, 
		&transmission_mode, &guard_interval, &hierarchy_information);

	/*1/2:2/3:3/4:5/6:7/8*/
	
	switch(code_rate_HP) {
		case 0: code_rate = FEC_1_2; break;
		case 1: code_rate = FEC_2_3; break;
		case 2: code_rate = FEC_3_4; break;
		case 3: code_rate = FEC_5_6; break;
		case 4: code_rate = FEC_7_8; break;
		default: code_rate = FEC_NONE; break;
	}
	p->u.ofdm.code_rate_HP = code_rate;
	
	switch(code_rate_LP) {
		case 0: code_rate = FEC_1_2; break;
		case 1: code_rate = FEC_2_3; break;
		case 2: code_rate = FEC_3_4; break;
		case 3: code_rate = FEC_5_6; break;
		case 4: code_rate = FEC_7_8; break;
		default: code_rate = FEC_NONE; break;
	}
	p->u.ofdm.code_rate_LP = code_rate;

	/*QPSK/16QAM/64QAM*/
	switch(constellation) {
		case 0: p->u.ofdm.constellation = QPSK; break;
		case 1: p->u.ofdm.constellation = QAM_16; break;
		case 2: p->u.ofdm.constellation = QAM_64; break;
		default: p->u.ofdm.constellation = QPSK; break;
	}

	/*2K/8K/4K*/
	switch(transmission_mode) {
		case 0: p->u.ofdm.transmission_mode = TRANSMISSION_MODE_2K; break;
		case 1: p->u.ofdm.transmission_mode = TRANSMISSION_MODE_8K; break;
		case 2: p->u.ofdm.transmission_mode = TRANSMISSION_MODE_4K; break;
		default: p->u.ofdm.transmission_mode = TRANSMISSION_MODE_AUTO; break;
	}

	/*1/32:1/16:1/8:1/4*/
	switch(guard_interval) {
		case 0: p->u.ofdm.guard_interval = GUARD_INTERVAL_1_32; break;
		case 1: p->u.ofdm.guard_interval = GUARD_INTERVAL_1_16; break;
		case 2: p->u.ofdm.guard_interval = GUARD_INTERVAL_1_8; break;
		case 3: p->u.ofdm.guard_interval = GUARD_INTERVAL_1_4; break;
		default: p->u.ofdm.guard_interval = GUARD_INTERVAL_AUTO; break;
	}

	/*1/2/4*/
	switch(hierarchy_information) {
		case 0: p->u.ofdm.hierarchy_information = HIERARCHY_1; break;
		case 1: p->u.ofdm.hierarchy_information = HIERARCHY_2; break;
		case 2: p->u.ofdm.hierarchy_information = HIERARCHY_4; break;
		default: p->u.ofdm.hierarchy_information = HIERARCHY_NONE; break;
	}

	return 0;
}

static void aml_fe_dvb_release(struct dvb_frontend *fe)
{
	struct amlfe_state *state = fe->demodulator_priv;

	uninstall_isr(state);
	
	kfree(state);
}

static int firstone=0;
static int aml_fe_isdbt_init(struct dvb_frontend *fe)
{
	struct amlfe_state *state = fe->demodulator_priv;
	struct aml_demod_sys sys;
	struct aml_demod_i2c i2c;
	
	pr_dbg("AML Demod ISDB-T init\r\n");
	state->i2c->tuner=7;
	firstone=1;
	state->config.tuner_type=7;

	memset(&sys, 0, sizeof(sys));
	memset(&i2c, 0, sizeof(i2c));
	
    	sys.clk_en = 1;
	sys.clk_src = 0;
	sys.clk_div = 0;
	sys.pll_n = 1;
	sys.pll_m = 50;
	sys.pll_od = 0;
	sys.pll_sys_xd = 20;
	sys.pll_adc_xd = 21;
	sys.agc_sel = 2;
	sys.adc_en = 1;
	sys.i2c = (long)&i2c;
	sys.debug = 0;
	i2c.tuner = state->config.tuner_type;
	i2c.addr = state->config.tuner_addr;

	_prepare_i2c(fe, &i2c);
#ifndef CONFIG_AM_DEMOD_FPGA_VER
	demod_set_sys(state->sta, state->i2c, &sys);
#endif
	state->sys = sys;
	
	return 0;
}

static void aml_fe_isdbt_release(struct dvb_frontend *fe)
{
	struct amlfe_state *state = fe->demodulator_priv;

	uninstall_isr(state);

#if defined(CONFIG_AM_AMDEMOD_FPGA_VER)
	printk("sdio_exit for fpga\n");
	extern int  sdio_exit(void);
	sdio_exit();
#endif	  

	kfree(state);
}

static int aml_fe_isdbt_read_status(struct dvb_frontend *fe, fe_status_t * status)
{
	struct amlfe_state *state = fe->demodulator_priv;
	int ilock;
	
	if(dvbt_get_status_ops()->get_status(state->sta, state->i2c))
	{
		printk("lock success\n");
		ilock=1;
		*status = FE_HAS_LOCK|FE_HAS_SIGNAL|FE_HAS_CARRIER|FE_HAS_VITERBI|FE_HAS_SYNC;
	}
	else
	{
		ilock=0;
		*status = FE_TIMEDOUT;
	}

	if(last_lock != ilock){
		pr_dbg("%s.\n", ilock? "!!  >> LOCK << !!" : "!! >> UNLOCK << !!");
		last_lock = ilock;
	}
	
	return  0;
}

static int aml_fe_isdbt_read_ber(struct dvb_frontend *fe, u32 * ber)
{
	struct amlfe_state *state = fe->demodulator_priv;
	*ber = dvbt_get_status_ops()->get_ber(state->sta, state->i2c)&0xffff;
	return 0;
}

static int aml_fe_isdbt_read_signal_strength(struct dvb_frontend *fe, u16 *strength)
{
	struct amlfe_state *state = fe->demodulator_priv;
	*strength=dvbt_get_status_ops()->get_strength(state->sta, state->i2c);
	return 0;
}

static int aml_fe_isdbt_read_snr(struct dvb_frontend *fe, u16 * snr)
{
	struct amlfe_state *state = fe->demodulator_priv;
	*snr = dvbt_get_status_ops()->get_snr(state->sta, state->i2c);
	return 0;
}


static int aml_fe_isdbt_read_ucblocks(struct dvb_frontend *fe, u32 * ucblocks)
{
	struct amlfe_state *state = fe->demodulator_priv;
	*ucblocks = dvbt_get_status_ops()->get_ucblocks(state->sta, state->i2c);
	return 0;
}


static int layer=7;/*3Layer*/


void app_apb_write_reg(int addr, int data)
{
	    struct aml_demod_reg reg;
	    reg.mode = 4;
	    reg.addr = addr;
	    reg.val  = data;
		apb_write_reg(reg.mode,reg.addr,reg.val);
}
int app_apb_read_reg(int addr)
{
	    struct aml_demod_reg reg;
	    reg.mode = 4;
	    reg.addr = addr;
		reg.val=apb_read_reg(reg.mode,addr);
	    return ((int)(reg.val));
}

/*\/\/ SW ZIGZAG \/\/*/
static int amdemod_isdbt_tune(struct amlfe_state *state, struct dvb_frontend_parameters *p)
{
	
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
	param.bw = 0;
	param.sr = 2;
	param.ifreq = 1;
	//param.bw = p->u.ofdm.bandwidth;
	param.agc_mode = 0;
	/*ISDBT or DVBT : 0 is QAM, 1 is DVBT, 2 is ISDBT, 3 is NTSC */
	param.dat0 = 2;
	param.layer = 0;
	
	last_lock = -1;
	
	dvbt_set_ch(state->sta, state->i2c, &param);
#ifdef CONFIG_AM_AMDEMOD_FPGA_VER
	fpga_monitor_restart();

	{
		int wait = 8;//2000/20;//1s
		while (wait)
	 	{
		    if(amdemod_isdbt_stat_islock(state)) break;
	            wait--;
	          
	            app_apb_write_reg(71, app_apb_read_reg(71) |  (1 << 24)); // write 1
                   app_apb_write_reg(71, app_apb_read_reg(71) &~ (1 << 24)); // write 0
                     msleep(500);              //Delay 20 ms   
	        }
	}
#else
	{
		int ret;
		ret = wait_event_interruptible_timeout(state->lock_wq, amdemod_isdbt_stat_islock(state), 2*HZ);
		if(!ret)	pr_error("amlfe wait lock timeout.\n");
	}
#endif

	state->freq=p->frequency;
	state->mode=p->u.ofdm.bandwidth;
	
	pr_dbg("AML DEMOD => frequency=%d,bw=%d\r\n",p->frequency,p->u.ofdm.bandwidth);

	return 0;
}






static int aml_fe_isdbt_set_frontend(struct dvb_frontend *fe, struct dvb_frontend_parameters *p)
{
	struct amlfe_state *state = fe->demodulator_priv;
	amdemod_isdbt_tune(state, p);
	
	state->freq=p->frequency;
	state->mode=p->u.ofdm.bandwidth;
	
	pr_dbg("AML DEMOD => frequency=%d,bw=%d\r\n",p->frequency,p->u.ofdm.bandwidth);
	return  0;
}
/*/\/\ SW ZIGZAG /\/\ */

static enum dvbfe_algo aml_fe_isdbt_get_frontend_algo(struct dvb_frontend *fe)
{
	return DVBFE_ALGO_HW;
}

static int aml_fe_isdbt_tune(struct dvb_frontend* fe,
		    struct dvb_frontend_parameters* params,
		    unsigned int mode_flags,
		    unsigned int *delay,
		    fe_status_t *status)
{
	static struct dvb_frontend_parameters last_params;
	
	struct amlfe_state *state = fe->demodulator_priv;
	int locked = amdemod_isdbt_stat_islock(state);
	int retune = (params && !amdemod_isdbt_frontend_param_equal(params, &last_params))? 1 : 0;
	int check_delay;
	
	pr_dbg("isdbt tune retune:%d, locked:0x%x\n", retune, locked);

	if(!locked && !retune) {
		/*Fixme: frequency offset correction*/
	}
	if((autotune && !locked) || retune) {
	//	if(firstone)
		amdemod_isdbt_tune(state, params?params:&last_params);
		firstone=0;
		check_delay = 5 * HZ;
	} else {
		check_delay = HZ;
	}
	mdelay(2000);
	if(amdemod_isdbt_stat_islock(state))
		*status = (FE_HAS_LOCK|FE_HAS_SIGNAL|FE_HAS_CARRIER|FE_HAS_VITERBI|FE_HAS_SYNC);
	else
		*status = FE_TIMEDOUT;

	*delay = check_delay;

	if(retune) {
		state->freq=params->frequency;
		state->mode=params->u.ofdm.bandwidth;

		memcpy(&last_params, params, sizeof(struct dvb_frontend_parameters));
	}
	
	return 0;
}

static int aml_fe_isdbt_get_frontend(struct dvb_frontend *fe, struct dvb_frontend_parameters *p)
{//these content will be writed into eeprom .

	struct amlfe_state *state = fe->demodulator_priv;
      	int  code_rate_HP, code_rate_LP, constellation, 
		transmission_mode, guard_interval, hierarchy_information;
	fe_code_rate_t code_rate;
	
	p->frequency=state->freq;
	p->u.ofdm.bandwidth=state->mode;

	dvbt_get_params(state->sta, state->i2c, 
		&code_rate_HP, &code_rate_LP, &constellation, 
		&transmission_mode, &guard_interval, &hierarchy_information);

	/*1/2:2/3:3/4:5/6:7/8*/
	
	switch(code_rate_HP) {
		case 0: code_rate = FEC_1_2; break;
		case 1: code_rate = FEC_2_3; break;
		case 2: code_rate = FEC_3_4; break;
		case 3: code_rate = FEC_5_6; break;
		case 4: code_rate = FEC_7_8; break;
		default: code_rate = FEC_NONE; break;
	}
	p->u.ofdm.code_rate_HP = code_rate;
	
	switch(code_rate_LP) {
		case 0: code_rate = FEC_1_2; break;
		case 1: code_rate = FEC_2_3; break;
		case 2: code_rate = FEC_3_4; break;
		case 3: code_rate = FEC_5_6; break;
		case 4: code_rate = FEC_7_8; break;
		default: code_rate = FEC_NONE; break;
	}
	p->u.ofdm.code_rate_LP = code_rate;

	/*QPSK/16QAM/64QAM*/
	switch(constellation) {
		case 0: p->u.ofdm.constellation = QPSK; break;
		case 1: p->u.ofdm.constellation = QAM_16; break;
		case 2: p->u.ofdm.constellation = QAM_64; break;
		default: p->u.ofdm.constellation = QPSK; break;
	}

	/*2K/8K/4K*/
	switch(transmission_mode) {
		case 0: p->u.ofdm.transmission_mode = TRANSMISSION_MODE_2K; break;
		case 1: p->u.ofdm.transmission_mode = TRANSMISSION_MODE_8K; break;
		case 2: p->u.ofdm.transmission_mode = TRANSMISSION_MODE_4K; break;
		default: p->u.ofdm.transmission_mode = TRANSMISSION_MODE_AUTO; break;
	}

	/*1/32:1/16:1/8:1/4*/
	switch(guard_interval) {
		case 0: p->u.ofdm.guard_interval = GUARD_INTERVAL_1_32; break;
		case 1: p->u.ofdm.guard_interval = GUARD_INTERVAL_1_16; break;
		case 2: p->u.ofdm.guard_interval = GUARD_INTERVAL_1_8; break;
		case 3: p->u.ofdm.guard_interval = GUARD_INTERVAL_1_4; break;
		default: p->u.ofdm.guard_interval = GUARD_INTERVAL_AUTO; break;
	}

	/*1/2/4*/
	switch(hierarchy_information) {
		case 0: p->u.ofdm.hierarchy_information = HIERARCHY_1; break;
		case 1: p->u.ofdm.hierarchy_information = HIERARCHY_2; break;
		case 2: p->u.ofdm.hierarchy_information = HIERARCHY_4; break;
		default: p->u.ofdm.hierarchy_information = HIERARCHY_NONE; break;
	}

	return 0;
}

int aml_fe_isdbt_set_property(struct dvb_frontend* fe, struct dtv_property* tvp)
{
       if(tvp->cmd == DTV_ISDBT_LAYER_ENABLED) {
	   	/*bit0:LayerA, bit1:LayerB, bit2:LayerC*/
		layer = tvp->u.data;
	   	printk("fend set prop = %d\n", layer);
              apb_write_reg(2, 0x69, (apb_read_reg(2, 0x69)&~(7 << 9)) | ((layer&7) <<9));
              printk("layer set to 0x%x\n", layer);
       }
       return 0;
}

int aml_fe_isdbt_get_property(struct dvb_frontend* fe, struct dtv_property* tvp)
{
       return 0;
}


//dtmb 20120419

static int aml_fe_dtmb_init(struct dvb_frontend *fe)
{
	struct amlfe_state *state = fe->demodulator_priv;
	struct aml_demod_sys sys;
	struct aml_demod_i2c i2c;
	
	pr_dbg("AML Demod DTMB init\r\n");
	state->i2c->tuner=7;
	firstone=1;
	state->config.tuner_type=7;

	memset(&sys, 0, sizeof(sys));
	memset(&i2c, 0, sizeof(i2c));
	
    sys.clk_en = 1;
	sys.clk_src = 0;
	sys.clk_div = 0;
	sys.pll_n = 1;
	sys.pll_m = 50;
	sys.pll_od = 0;
	sys.pll_sys_xd = 20;
	sys.pll_adc_xd = 21;
	sys.agc_sel = 2;
	sys.adc_en = 1;
	sys.i2c = (long)&i2c;
	sys.debug = 0;
	i2c.tuner = state->config.tuner_type;
	i2c.addr = state->config.tuner_addr;

	_prepare_i2c(fe, &i2c);
#ifndef CONFIG_AM_DEMOD_FPGA_VER
	demod_set_sys(state->sta, state->i2c, &sys);
#endif
	state->sys = sys;
	
	return 0;
}

static void aml_fe_dtmb_release(struct dvb_frontend *fe)
{
	struct amlfe_state *state = fe->demodulator_priv;

	uninstall_isr(state);

#if defined(CONFIG_AM_AMDEMOD_FPGA_VER)
	printk("sdio_exit for fpga\n");
	extern int  sdio_exit(void);
	sdio_exit();
#endif	  

	kfree(state);
}

static int aml_fe_dtmb_read_status(struct dvb_frontend *fe, fe_status_t * status)
{
	struct amlfe_state *state = fe->demodulator_priv;
	int ilock;
	
	if(dvbt_get_status_ops()->get_status(state->sta, state->i2c))
	{
		printk("lock success\n");
		ilock=1;
		*status = FE_HAS_LOCK|FE_HAS_SIGNAL|FE_HAS_CARRIER|FE_HAS_VITERBI|FE_HAS_SYNC;
	}
	else
	{
		ilock=0;
		*status = FE_TIMEDOUT;
	}

	if(last_lock != ilock){
		pr_dbg("%s.\n", ilock? "!!  >> LOCK << !!" : "!! >> UNLOCK << !!");
		last_lock = ilock;
	}
	
	return  0;
}

static int aml_fe_dtmb_read_ber(struct dvb_frontend *fe, u32 * ber)
{
	struct amlfe_state *state = fe->demodulator_priv;
	*ber = dvbt_get_status_ops()->get_ber(state->sta, state->i2c)&0xffff;
	return 0;
}

static int aml_fe_dtmb_read_signal_strength(struct dvb_frontend *fe, u16 *strength)
{
	struct amlfe_state *state = fe->demodulator_priv;
	*strength=dvbt_get_status_ops()->get_strength(state->sta, state->i2c);
	return 0;
}

static int aml_fe_dtmb_read_snr(struct dvb_frontend *fe, u16 * snr)
{
	struct amlfe_state *state = fe->demodulator_priv;
	*snr = dvbt_get_status_ops()->get_snr(state->sta, state->i2c);
	return 0;
}


static int aml_fe_dtmb_read_ucblocks(struct dvb_frontend *fe, u32 * ucblocks)
{
	struct amlfe_state *state = fe->demodulator_priv;
	*ucblocks = dvbt_get_status_ops()->get_ucblocks(state->sta, state->i2c);
	return 0;
}


/*\/\/ SW ZIGZAG \/\/*/
static int amdemod_dtmb_tune(struct amlfe_state *state, struct dvb_frontend_parameters *p)
{
	
	struct aml_demod_dtmb param;
	
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
	param.bw = 0;
	param.sr = 2;
	param.ifreq = 1;
	//param.bw = p->u.ofdm.bandwidth;
	param.agc_mode = 0;
	/*DTMB : 0 is QAM, 1 is DVBT, 2 is ISDBT, 3 is DTMB ,4 is ATSC */
	param.dat0 = 3;
	last_lock = -1;
	dtmb_set_ch(state->sta, state->i2c, &param);
#ifdef CONFIG_AM_AMDEMOD_FPGA_VER
	fpga_monitor_restart();
	 int che_snr;
	{
		int wait = 8;//2000/20;//1s
		while (wait)
	 	{
		    if(amdemod_dtmb_stat_islock(state)) break;
	            wait--;
	           
    			che_snr = app_apb_read_reg(227) & 0xffffff;
				che_snr = che_snr/1000;
    			printk("che_snr is 10*log10(%d) ------ time_di reset %d ------\n",che_snr, wait);
	            app_apb_write_reg(71, app_apb_read_reg(71) |  (1 << 24)); // write 1
                app_apb_write_reg(71, app_apb_read_reg(71) &~ (1 << 24)); // write 0
                msleep(500);              //Delay 20 ms   
	     }
	}
#else
	{
		int ret;
		ret = wait_event_interruptible_timeout(state->lock_wq, amdemod_dtmb_stat_islock(state), 2*HZ);
		if(!ret)	pr_error("amlfe wait lock timeout.\n");
	}
#endif

	state->freq=p->frequency;
	state->mode=p->u.ofdm.bandwidth;
	
	pr_dbg("AML DEMOD => frequency=%d,bw=%d\r\n",p->frequency,p->u.ofdm.bandwidth);

	return 0;
}






static int aml_fe_dtmb_set_frontend(struct dvb_frontend *fe, struct dvb_frontend_parameters *p)
{
	struct amlfe_state *state = fe->demodulator_priv;
	amdemod_dtmb_tune(state, p);
	
	state->freq=p->frequency;
	state->mode=p->u.ofdm.bandwidth;
	
	pr_dbg("AML DEMOD => frequency=%d,bw=%d\r\n",p->frequency,p->u.ofdm.bandwidth);
	return  0;
}
/*/\/\ SW ZIGZAG /\/\ */

static enum dvbfe_algo aml_fe_dtmb_get_frontend_algo(struct dvb_frontend *fe)
{
	return DVBFE_ALGO_HW;
}

static int aml_fe_dtmb_tune(struct dvb_frontend* fe,
		    struct dvb_frontend_parameters* params,
		    unsigned int mode_flags,
		    unsigned int *delay,
		    fe_status_t *status)
{
	static struct dvb_frontend_parameters last_params;
	
	struct amlfe_state *state = fe->demodulator_priv;
	int i;
	int locked;
	for(i=100;i>0;i--){
	locked = amdemod_dtmb_stat_islock(state);
	msleep(100);
	if(locked==1)
		break;
	}
	int retune = (params && !amdemod_dtmb_frontend_param_equal(params, &last_params))? 1 : 0;
	int check_delay;
	
	//pr_dbg("dtmb tune retune:%d, locked:0x%x\n", retune, locked);

	if(!locked && !retune) {
		/*Fixme: frequency offset correction*/
	}
	if((autotune && !locked) || retune) {
		amdemod_dtmb_tune(state, params?params:&last_params);
		check_delay = 5 * HZ;
	} else {
		check_delay = HZ;
	}
	msleep(800);
	if(amdemod_dtmb_stat_islock(state))
		*status = (FE_HAS_LOCK|FE_HAS_SIGNAL|FE_HAS_CARRIER|FE_HAS_VITERBI|FE_HAS_SYNC);
	else
		*status = FE_TIMEDOUT;

	*delay = check_delay;

	if(retune) {
		state->freq=params->frequency;
		state->mode=params->u.ofdm.bandwidth;

		memcpy(&last_params, params, sizeof(struct dvb_frontend_parameters));
	}
	
	return 0;
}

static int aml_fe_dtmb_get_frontend(struct dvb_frontend *fe, struct dvb_frontend_parameters *p)
{//these content will be writed into eeprom .

	struct amlfe_state *state = fe->demodulator_priv;
      	int  code_rate_HP, code_rate_LP, constellation, 
		transmission_mode, guard_interval, hierarchy_information;
	fe_code_rate_t code_rate;
	
	p->frequency=state->freq;
	p->u.ofdm.bandwidth=state->mode;

	dvbt_get_params(state->sta, state->i2c, 
		&code_rate_HP, &code_rate_LP, &constellation, 
		&transmission_mode, &guard_interval, &hierarchy_information);

	/*1/2:2/3:3/4:5/6:7/8*/
	
	switch(code_rate_HP) {
		case 0: code_rate = FEC_1_2; break;
		case 1: code_rate = FEC_2_3; break;
		case 2: code_rate = FEC_3_4; break;
		case 3: code_rate = FEC_5_6; break;
		case 4: code_rate = FEC_7_8; break;
		default: code_rate = FEC_NONE; break;
	}
	p->u.ofdm.code_rate_HP = code_rate;
	
	switch(code_rate_LP) {
		case 0: code_rate = FEC_1_2; break;
		case 1: code_rate = FEC_2_3; break;
		case 2: code_rate = FEC_3_4; break;
		case 3: code_rate = FEC_5_6; break;
		case 4: code_rate = FEC_7_8; break;
		default: code_rate = FEC_NONE; break;
	}
	p->u.ofdm.code_rate_LP = code_rate;

	/*QPSK/16QAM/64QAM*/
	switch(constellation) {
		case 0: p->u.ofdm.constellation = QPSK; break;
		case 1: p->u.ofdm.constellation = QAM_16; break;
		case 2: p->u.ofdm.constellation = QAM_64; break;
		default: p->u.ofdm.constellation = QPSK; break;
	}

	/*2K/8K/4K*/
	switch(transmission_mode) {
		case 0: p->u.ofdm.transmission_mode = TRANSMISSION_MODE_2K; break;
		case 1: p->u.ofdm.transmission_mode = TRANSMISSION_MODE_8K; break;
		case 2: p->u.ofdm.transmission_mode = TRANSMISSION_MODE_4K; break;
		default: p->u.ofdm.transmission_mode = TRANSMISSION_MODE_AUTO; break;
	}

	/*1/32:1/16:1/8:1/4*/
	switch(guard_interval) {
		case 0: p->u.ofdm.guard_interval = GUARD_INTERVAL_1_32; break;
		case 1: p->u.ofdm.guard_interval = GUARD_INTERVAL_1_16; break;
		case 2: p->u.ofdm.guard_interval = GUARD_INTERVAL_1_8; break;
		case 3: p->u.ofdm.guard_interval = GUARD_INTERVAL_1_4; break;
		default: p->u.ofdm.guard_interval = GUARD_INTERVAL_AUTO; break;
	}

	/*1/2/4*/
	switch(hierarchy_information) {
		case 0: p->u.ofdm.hierarchy_information = HIERARCHY_1; break;
		case 1: p->u.ofdm.hierarchy_information = HIERARCHY_2; break;
		case 2: p->u.ofdm.hierarchy_information = HIERARCHY_4; break;
		default: p->u.ofdm.hierarchy_information = HIERARCHY_NONE; break;
	}

	return 0;
}


//atsc 20120704


static int aml_fe_atsc_init(struct dvb_frontend *fe)
{
	struct amlfe_state *state = fe->demodulator_priv;
	struct aml_demod_sys sys;
	struct aml_demod_i2c i2c;
	
	pr_dbg("AML Demod ATSC init\r\n");
	memset(&sys, 0, sizeof(sys));
	memset(&i2c, 0, sizeof(i2c));
	
    sys.clk_en = 1;
	sys.clk_src = 0;
	sys.clk_div = 0;
	sys.pll_n = 1;
	sys.pll_m = 50;
	sys.pll_od = 0;
	sys.pll_sys_xd = 20;
	sys.pll_adc_xd = 21;
	sys.agc_sel = 2;
	sys.adc_en = 1;
	sys.i2c = (long)&i2c;
	sys.debug = 0;
	i2c.tuner = state->config.tuner_type;
	i2c.addr = state->config.tuner_addr;

	_prepare_i2c(fe, &i2c);
	demod_set_sys(state->sta, state->i2c, &sys);
	state->sys = sys;
	
	return 0;
}

static void aml_fe_atsc_release(struct dvb_frontend *fe)
{
	struct amlfe_state *state = fe->demodulator_priv;

	uninstall_isr(state);

#if defined(CONFIG_AM_AMDEMOD_FPGA_VER)
	printk("sdio_exit for fpga\n");
	extern int  sdio_exit(void);
	sdio_exit();
#endif	  

	kfree(state);
}

static int aml_fe_atsc_read_status(struct dvb_frontend *fe, fe_status_t * status)
{
	struct amlfe_state *state = fe->demodulator_priv;
	int ilock;
	int lockstatus;
	lockstatus=apb_read_reg(4,(0x0980));
	printk("lockstatus is %x\n",lockstatus);
//	if(dvbt_get_status_ops()->get_status(state->sta, state->i2c))
	if(apb_read_reg(4,(0x0980))==0x79)
	{
		printk("lock success\n");
		ilock=1;
		*status = FE_HAS_LOCK|FE_HAS_SIGNAL|FE_HAS_CARRIER|FE_HAS_VITERBI|FE_HAS_SYNC;
	}
	else
	{
		ilock=0;
		*status = FE_TIMEDOUT;
	}

	if(last_lock != ilock){
		pr_dbg("%s.\n", ilock? "!!  >> LOCK << !!" : "!! >> UNLOCK << !!");
		last_lock = ilock;
	}
	
	return  0;
}

static int aml_fe_atsc_read_ber(struct dvb_frontend *fe, u32 * ber)
{
	struct amlfe_state *state = fe->demodulator_priv;
	*ber = dvbt_get_status_ops()->get_ber(state->sta, state->i2c)&0xffff;
	return 0;
}

static int aml_fe_atsc_read_signal_strength(struct dvb_frontend *fe, u16 *strength)
{
	struct amlfe_state *state = fe->demodulator_priv;
	*strength=dvbt_get_status_ops()->get_strength(state->sta, state->i2c);
	return 0;
}

static int aml_fe_atsc_read_snr(struct dvb_frontend *fe, u16 * snr)
{
	struct amlfe_state *state = fe->demodulator_priv;
	*snr = dvbt_get_status_ops()->get_snr(state->sta, state->i2c);
	return 0;
}


static int aml_fe_atsc_read_ucblocks(struct dvb_frontend *fe, u32 * ucblocks)
{
	struct amlfe_state *state = fe->demodulator_priv;
	*ucblocks = dvbt_get_status_ops()->get_ucblocks(state->sta, state->i2c);
	return 0;
}



/*\/\/ SW ZIGZAG \/\/*/
static int amdemod_atsc_tune(struct amlfe_state *state, struct dvb_frontend_parameters *p)
{
	
	struct aml_demod_atsc param;
	
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
	param.dat0 = 4;

	last_lock = -1;
	atsc_set_ch(state->sta, state->i2c, &param);
#ifdef CONFIG_AM_AMDEMOD_FPGA_VER
	fpga_monitor_restart();

	{
		int wait = 10;//60/20;//1s
		while (wait)
	 	{
		    if(amdemod_atsc_stat_islock(state)) break;
				printk("lock status is %x\n",apb_read_reg(4,(0x0980)));
	            wait--;
	            msleep(200);              //Delay 20 ms   
	        }
	}
#else
	{
		int ret;
		ret = wait_event_interruptible_timeout(state->lock_wq, amdemod_atsc_stat_islock(state), 2*HZ);
		if(!ret)	pr_error("amlfe wait lock timeout.\n");
	}
#endif
	monitor_atsc();
	state->freq=p->frequency;
	state->mode=p->u.ofdm.bandwidth;
	
	pr_dbg("AML DEMOD => frequency=%d,bw=%d\r\n",p->frequency,p->u.ofdm.bandwidth);

	return 0;
}






static int aml_fe_atsc_set_frontend(struct dvb_frontend *fe, struct dvb_frontend_parameters *p)
{
	struct amlfe_state *state = fe->demodulator_priv;
	amdemod_atsc_tune(state, p);
	
	state->freq=p->frequency;
	state->mode=p->u.ofdm.bandwidth;
	
	pr_dbg("AML DEMOD => frequency=%d,bw=%d\r\n",p->frequency,p->u.ofdm.bandwidth);
	return  0;
}
/*/\/\ SW ZIGZAG /\/\ */

static enum dvbfe_algo aml_fe_atsc_get_frontend_algo(struct dvb_frontend *fe)
{
	return DVBFE_ALGO_HW;
}

static int aml_fe_atsc_tune(struct dvb_frontend* fe,
		    struct dvb_frontend_parameters* params,
		    unsigned int mode_flags,
		    unsigned int *delay,
		    fe_status_t *status)
{
	static struct dvb_frontend_parameters last_params;
	
	struct amlfe_state *state = fe->demodulator_priv;
	int locked = amdemod_atsc_stat_islock(state);
	int retune = (params && !amdemod_atsc_frontend_param_equal(params, &last_params))? 1 : 0;
	int check_delay;
	int lockstatus;
	lockstatus=apb_read_reg(4,(0x0980));
	printk("lockstatus is %x\n",lockstatus);
	pr_dbg("atsc tune retune:%d, locked:0x%x\n", retune, locked);

	if(!locked && !retune) {
		/*Fixme: frequency offset correction*/
	}
	if((autotune && !locked) || retune) {
		amdemod_atsc_tune(state, params?params:&last_params);
		check_delay = 5 * HZ;
	} else {
		check_delay = HZ;
	}
	msleep(800);
	if(amdemod_atsc_stat_islock(state))
		*status = (FE_HAS_LOCK|FE_HAS_SIGNAL|FE_HAS_CARRIER|FE_HAS_VITERBI|FE_HAS_SYNC);
	else
		*status = FE_TIMEDOUT;

	*delay = check_delay;

	if(retune) {
		state->freq=params->frequency;
		state->mode=params->u.ofdm.bandwidth;

		memcpy(&last_params, params, sizeof(struct dvb_frontend_parameters));
	}
	
	return 0;
}

static int aml_fe_atsc_get_frontend(struct dvb_frontend *fe, struct dvb_frontend_parameters *p)
{//these content will be writed into eeprom .

	struct amlfe_state *state = fe->demodulator_priv;
      	int  code_rate_HP, code_rate_LP, constellation, 
		transmission_mode, guard_interval, hierarchy_information;
	fe_code_rate_t code_rate;
	
	p->frequency=state->freq;
	p->u.ofdm.bandwidth=state->mode;

	dvbt_get_params(state->sta, state->i2c, 
		&code_rate_HP, &code_rate_LP, &constellation, 
		&transmission_mode, &guard_interval, &hierarchy_information);

	/*1/2:2/3:3/4:5/6:7/8*/
	
	switch(code_rate_HP) {
		case 0: code_rate = FEC_1_2; break;
		case 1: code_rate = FEC_2_3; break;
		case 2: code_rate = FEC_3_4; break;
		case 3: code_rate = FEC_5_6; break;
		case 4: code_rate = FEC_7_8; break;
		default: code_rate = FEC_NONE; break;
	}
	p->u.ofdm.code_rate_HP = code_rate;
	
	switch(code_rate_LP) {
		case 0: code_rate = FEC_1_2; break;
		case 1: code_rate = FEC_2_3; break;
		case 2: code_rate = FEC_3_4; break;
		case 3: code_rate = FEC_5_6; break;
		case 4: code_rate = FEC_7_8; break;
		default: code_rate = FEC_NONE; break;
	}
	p->u.ofdm.code_rate_LP = code_rate;

	/*QPSK/16QAM/64QAM*/
	switch(constellation) {
		case 0: p->u.ofdm.constellation = QPSK; break;
		case 1: p->u.ofdm.constellation = QAM_16; break;
		case 2: p->u.ofdm.constellation = QAM_64; break;
		default: p->u.ofdm.constellation = QPSK; break;
	}

	/*2K/8K/4K*/
	switch(transmission_mode) {
		case 0: p->u.ofdm.transmission_mode = TRANSMISSION_MODE_2K; break;
		case 1: p->u.ofdm.transmission_mode = TRANSMISSION_MODE_8K; break;
		case 2: p->u.ofdm.transmission_mode = TRANSMISSION_MODE_4K; break;
		default: p->u.ofdm.transmission_mode = TRANSMISSION_MODE_AUTO; break;
	}

	/*1/32:1/16:1/8:1/4*/
	switch(guard_interval) {
		case 0: p->u.ofdm.guard_interval = GUARD_INTERVAL_1_32; break;
		case 1: p->u.ofdm.guard_interval = GUARD_INTERVAL_1_16; break;
		case 2: p->u.ofdm.guard_interval = GUARD_INTERVAL_1_8; break;
		case 3: p->u.ofdm.guard_interval = GUARD_INTERVAL_1_4; break;
		default: p->u.ofdm.guard_interval = GUARD_INTERVAL_AUTO; break;
	}

	/*1/2/4*/
	switch(hierarchy_information) {
		case 0: p->u.ofdm.hierarchy_information = HIERARCHY_1; break;
		case 1: p->u.ofdm.hierarchy_information = HIERARCHY_2; break;
		case 2: p->u.ofdm.hierarchy_information = HIERARCHY_4; break;
		default: p->u.ofdm.hierarchy_information = HIERARCHY_NONE; break;
	}

	return 0;
}



int aml_fe_atsc_get_property(struct dvb_frontend* fe, struct dtv_property* tvp)
{
       return 0;
}







static struct dvb_frontend_ops aml_fe_dvbc_ops;
static struct dvb_frontend_ops aml_fe_dvbt_ops;
static struct dvb_frontend_ops aml_fe_isdbt_ops;
static struct dvb_frontend_ops aml_fe_dtmb_ops;
static struct dvb_frontend_ops aml_fe_atsc_ops;

static struct dvb_tuner_ops aml_fe_tuner_ops;

struct dvb_frontend *aml_fe_dvbc_attach(const struct amlfe_config *config)
{
	struct amlfe_state *state = NULL;
	struct dvb_tuner_info *tinfo;
	
	/* allocate memory for the internal state */
	
	state = kmalloc(sizeof(struct amlfe_state), GFP_KERNEL);
	if (state == NULL)
		return NULL;

	/* setup the state */
	state->config = *config;

	/* create dvb_frontend */
	memcpy(&state->fe.ops, &aml_fe_dvbc_ops, sizeof(struct dvb_frontend_ops));

	/*set tuner parameters*/
	tinfo = tuner_get_info(state->config.tuner_type, 0);
	memcpy(&state->fe.ops.tuner_ops, &aml_fe_tuner_ops,
	       sizeof(struct dvb_tuner_ops));
	memcpy(&state->fe.ops.tuner_ops.info, tinfo,
		sizeof(state->fe.ops.tuner_ops.info));
	
	state->sta = &demod_sta;
	state->i2c = &demod_i2c;

	init_waitqueue_head(&state->lock_wq);

	install_isr(state);
	
	state->fe.demodulator_priv = state;
	
	return &state->fe;
}

EXPORT_SYMBOL(aml_fe_dvbc_attach);

struct dvb_frontend *aml_fe_dvbt_attach(const struct amlfe_config *config)
{
	struct amlfe_state *state = NULL;
	struct dvb_tuner_info *tinfo;

	/* allocate memory for the internal state */
	
	state = kmalloc(sizeof(struct amlfe_state), GFP_KERNEL);
	if (state == NULL)
		return NULL;

	/* setup the state */
	state->config = *config;


	/* create dvb_frontend */
	memcpy(&state->fe.ops, &aml_fe_dvbt_ops, sizeof(struct dvb_frontend_ops));

	/*set tuner parameters*/
	tinfo = tuner_get_info(state->config.tuner_type, 1);
	memcpy(&state->fe.ops.tuner_ops, &aml_fe_tuner_ops,
	       sizeof(struct dvb_tuner_ops));
	memcpy(&state->fe.ops.tuner_ops.info, tinfo,
		sizeof(state->fe.ops.tuner_ops.info));

	state->sta = &demod_sta;
	state->i2c = &demod_i2c;

	init_waitqueue_head(&state->lock_wq);

	install_isr(state);
	
	state->fe.demodulator_priv = state;
	
	return &state->fe;
}

EXPORT_SYMBOL(aml_fe_dvbt_attach);

struct dvb_frontend *aml_fe_isdbt_attach(const struct amlfe_config *config)
{
	struct amlfe_state *state = NULL;
	struct dvb_tuner_info *tinfo;

	/* allocate memory for the internal state */
	
	state = kmalloc(sizeof(struct amlfe_state), GFP_KERNEL);
	if (state == NULL)
		return NULL;

	/* setup the state */
	state->config = *config;


	/* create dvb_frontend */
	memcpy(&state->fe.ops, &aml_fe_isdbt_ops, sizeof(struct dvb_frontend_ops));

	/*set tuner parameters*/
	tinfo = tuner_get_info(state->config.tuner_type, 1);
	memcpy(&state->fe.ops.tuner_ops, &aml_fe_tuner_ops,
	       sizeof(struct dvb_tuner_ops));
	memcpy(&state->fe.ops.tuner_ops.info, tinfo,
		sizeof(state->fe.ops.tuner_ops.info));

	state->sta = &demod_sta;
	state->i2c = &demod_i2c;

	init_waitqueue_head(&state->lock_wq);

	install_isr(state);

#if defined(CONFIG_AM_AMDEMOD_FPGA_VER)
	printk("sdio_init for fpga\n");
	sdio_init();
#endif	  

	state->fe.demodulator_priv = state;
	
	return &state->fe;
}

EXPORT_SYMBOL(aml_fe_isdbt_attach);


struct dvb_frontend *aml_fe_dtmb_attach(const struct amlfe_config *config)
{
	struct amlfe_state *state = NULL;
	struct dvb_tuner_info *tinfo;

	/* allocate memory for the internal state */
	
	state = kmalloc(sizeof(struct amlfe_state), GFP_KERNEL);
	if (state == NULL)
		return NULL;

	/* setup the state */
	state->config = *config;


	/* create dvb_frontend */
	memcpy(&state->fe.ops, &aml_fe_dtmb_ops, sizeof(struct dvb_frontend_ops));

	/*set tuner parameters*/
	tinfo = tuner_get_info(state->config.tuner_type, 1);
	memcpy(&state->fe.ops.tuner_ops, &aml_fe_tuner_ops,
	       sizeof(struct dvb_tuner_ops));
	memcpy(&state->fe.ops.tuner_ops.info, tinfo,
		sizeof(state->fe.ops.tuner_ops.info));

	state->sta = &demod_sta;
	state->i2c = &demod_i2c;

	init_waitqueue_head(&state->lock_wq);

	install_isr(state);

#if defined(CONFIG_AM_AMDEMOD_FPGA_VER)
	printk("sdio_init for fpga\n");
	sdio_init();
#endif	  

	state->fe.demodulator_priv = state;
	
	return &state->fe;
}

EXPORT_SYMBOL(aml_fe_dtmb_attach);


struct dvb_frontend *aml_fe_atsc_attach(const struct amlfe_config *config)
{
	struct amlfe_state *state = NULL;
	struct dvb_tuner_info *tinfo;

	/* allocate memory for the internal state */
	
	state = kmalloc(sizeof(struct amlfe_state), GFP_KERNEL);
	if (state == NULL)
		return NULL;

	/* setup the state */
	state->config = *config;


	/* create dvb_frontend */
	memcpy(&state->fe.ops, &aml_fe_atsc_ops, sizeof(struct dvb_frontend_ops));

	/*set tuner parameters*/
	tinfo = tuner_get_info(state->config.tuner_type, 1);
	memcpy(&state->fe.ops.tuner_ops, &aml_fe_tuner_ops,
	       sizeof(struct dvb_tuner_ops));
	memcpy(&state->fe.ops.tuner_ops.info, tinfo,
		sizeof(state->fe.ops.tuner_ops.info));

	state->sta = &demod_sta;
	state->i2c = &demod_i2c;

	init_waitqueue_head(&state->lock_wq);

	install_isr(state);

#if defined(CONFIG_AM_AMDEMOD_FPGA_VER)
	printk("sdio_init for fpga\n");
	sdio_init();
#endif	  

	state->fe.demodulator_priv = state;
	
	return &state->fe;
}

EXPORT_SYMBOL(aml_fe_atsc_attach);




static struct dvb_frontend_ops aml_fe_dvbc_ops = {

	.info = {
		 .name = "AMLOGIC DVB-C",
		 .type = FE_QAM,
		 .frequency_min = 47400000,
		.frequency_max = 862000000,
		 .frequency_stepsize = 62500,
		 .symbol_rate_min = 3600000,
		 .symbol_rate_max = 71400000,
		 .caps = FE_CAN_QAM_16 | FE_CAN_QAM_32 | FE_CAN_QAM_64 |
		 FE_CAN_QAM_128 | FE_CAN_QAM_256 | FE_CAN_FEC_AUTO
	},

	.release = aml_fe_dvb_release,

	.init = aml_fe_dvbc_init,
	.sleep = aml_fe_dvbc_sleep,
	.i2c_gate_ctrl = aml_fe_dvb_i2c_gate_ctrl,

	.set_frontend = aml_fe_dvbc_set_frontend,
	.get_frontend = aml_fe_dvbc_get_frontend,

	.read_status = aml_fe_dvbc_read_status,
	.read_ber = aml_fe_dvbc_read_ber,
	.read_signal_strength =aml_fe_dvbc_read_signal_strength,
	.read_snr = aml_fe_dvbc_read_snr,
	.read_ucblocks = aml_fe_dvbc_read_ucblocks,
};

static struct dvb_frontend_ops aml_fe_dvbt_ops = {

	.info = {
		 .name = "AMLOGIC DVB-T",
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

	.release = aml_fe_dvb_release,

	.init = aml_fe_dvbt_init,
	.sleep = aml_fe_dvbt_sleep,
	.i2c_gate_ctrl = aml_fe_dvb_i2c_gate_ctrl,

	.set_frontend = aml_fe_dvbt_set_frontend,
	.get_frontend = aml_fe_dvbt_get_frontend,

	/*custom algo for amldemod-dvbt*/
	.get_frontend_algo = aml_fe_dvbt_get_frontend_algo,
	.tune = aml_fe_dvbt_tune,
	
	.read_status = aml_fe_dvbt_read_status,
	.read_ber = aml_fe_dvbt_read_ber,
	.read_signal_strength =aml_fe_dvbt_read_signal_strength,
	.read_snr = aml_fe_dvbt_read_snr,
	.read_ucblocks = aml_fe_dvbt_read_ucblocks,
};

static struct dvb_frontend_ops aml_fe_isdbt_ops = {

	.info = {
		 .name = "AMLOGIC ISDB-T",
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

	.release = aml_fe_isdbt_release,

	.init = aml_fe_isdbt_init,
	.sleep = aml_fe_isdbt_sleep,
	.i2c_gate_ctrl = aml_fe_dvb_i2c_gate_ctrl,

	.set_frontend = aml_fe_isdbt_set_frontend,
	.get_frontend = aml_fe_isdbt_get_frontend,
	
	/*custom algo for amldemod-dvbt*/
	.get_frontend_algo = aml_fe_isdbt_get_frontend_algo,
	.tune = aml_fe_isdbt_tune,
	
	.read_status = aml_fe_isdbt_read_status,
	.read_ber = aml_fe_isdbt_read_ber,
	.read_signal_strength =aml_fe_isdbt_read_signal_strength,
	.read_snr = aml_fe_isdbt_read_snr,
	.read_ucblocks = aml_fe_isdbt_read_ucblocks,

       .set_property = aml_fe_isdbt_set_property,
       .get_property = aml_fe_isdbt_get_property,
	
};


static struct dvb_frontend_ops aml_fe_dtmb_ops = {

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

	.release = aml_fe_dtmb_release,

	.init = aml_fe_dtmb_init,
	.sleep = aml_fe_dtmb_sleep,
	.i2c_gate_ctrl = aml_fe_dvb_i2c_gate_ctrl,

	.set_frontend = aml_fe_dtmb_set_frontend,
	.get_frontend = aml_fe_dtmb_get_frontend,
	
	/*custom algo for amldemod-dvbt*/
	.get_frontend_algo = aml_fe_dtmb_get_frontend_algo,
	.tune = aml_fe_dtmb_tune,
	
	.read_status = aml_fe_dtmb_read_status,
	.read_ber = aml_fe_dtmb_read_ber,
	.read_signal_strength =aml_fe_dtmb_read_signal_strength,
	.read_snr = aml_fe_dtmb_read_snr,
	.read_ucblocks = aml_fe_dtmb_read_ucblocks,


	
};

static struct dvb_frontend_ops aml_fe_atsc_ops = {

	.info = {
		 .name = "AMLOGIC ATSC",
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

	.release = aml_fe_atsc_release,

	.init = aml_fe_atsc_init,
	.sleep = aml_fe_atsc_sleep,
	.i2c_gate_ctrl = aml_fe_dvb_i2c_gate_ctrl,

	.set_frontend = aml_fe_atsc_set_frontend,
	.get_frontend = aml_fe_atsc_get_frontend,
	
	/*custom algo for amldemod-dvbt*/
	.get_frontend_algo = aml_fe_atsc_get_frontend_algo,
	.tune = aml_fe_atsc_tune,
	
	.read_status = aml_fe_atsc_read_status,
	.read_ber = aml_fe_atsc_read_ber,
	.read_signal_strength =aml_fe_atsc_read_signal_strength,
	.read_snr = aml_fe_atsc_read_snr,
	.read_ucblocks = aml_fe_atsc_read_ucblocks,

	
};





static int amdemod_fe_cfg_get(struct aml_dvb *advb, struct platform_device *pdev, int id, struct amlfe_config **pcfg)
{
	int ret=0;
	struct resource *res;
	char buf[32];
	struct amlfe_config *cfg;

	cfg = kzalloc(sizeof(struct amlfe_config), GFP_KERNEL);
	if (!cfg)
		return -ENOMEM;

	cfg->fe_mode = frontend_mode;
	if(cfg->fe_mode == -1) {
		snprintf(buf, sizeof(buf), "frontend%d_mode", id);
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM, buf);
		if (!res) {
			pr_error("cannot get resource \"%s\"\n", buf);
			ret = -EINVAL;
			goto err_resource;
		}
		cfg->fe_mode = res->start;
		frontend_mode = cfg->fe_mode;
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
		frontend_i2c = cfg->i2c_id;
	}
	
	cfg->tuner_type = frontend_tuner;
	if(cfg->tuner_type == -1) {
		snprintf(buf, sizeof(buf), "frontend%d_tuner", id);
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM, buf);
		if (!res) {
			pr_error("cannot get resource \"%s\"\n", buf);
			ret = -EINVAL;
			goto err_resource;
		}
		cfg->tuner_type = res->start;
		frontend_tuner = cfg->tuner_type;
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
		frontend_tuner_addr = cfg->tuner_addr;
	}

	*pcfg = cfg;
	
	return 0;

err_resource:
	kfree(cfg);
	return ret;

}

static void amdemod_fe_cfg_put(struct amlfe_config *cfg)
{
	if(cfg) 
		kfree(cfg);
}

static int amdemod_fe_register(struct aml_dvb *advb, struct aml_fe *fe, struct amlfe_config *cfg)
{
	int ret=0;
	struct dvb_frontend_ops *ops;
	cfg->fe_mode = 4;

	printk("===============\n");
	printk("0-dvbc, 1-dvbt, 2-isdbt, 3-dtmb, 4-atsc\n");
	printk("cfg->fe_mode %d \n",cfg->fe_mode);
	printk("===============\n");
	if(cfg->fe_mode == 0)
		fe->fe = aml_fe_dvbc_attach(cfg);
	else if(cfg->fe_mode == 1)
		fe->fe = aml_fe_dvbt_attach(cfg);
	else if(cfg->fe_mode == 2)
		fe->fe = aml_fe_isdbt_attach(cfg);
	else if(cfg->fe_mode == 3)
		fe->fe = aml_fe_dtmb_attach(cfg);
	else if(cfg->fe_mode == 4)
		fe->fe = aml_fe_atsc_attach(cfg);
	
	if (!fe->fe) {
		return -ENOMEM;
	}
	
	if ((ret=dvb_register_frontend(&advb->dvb_adapter, fe->fe))) {
		pr_error("frontend registration failed!");
		ops = &fe->fe->ops;
		if (ops->release != NULL)
			ops->release(fe->fe);
		fe->fe = NULL;
	}
	
	last_lock = -1;

	return ret;
}

static void amdemod_fe_release(struct aml_dvb *advb, struct aml_fe *fe)
{
	if(fe && fe->fe) {
		pr_dbg("release AML frontend %d\n", fe->id);
		dvb_unregister_frontend(fe->fe);
		dvb_frontend_detach(fe->fe);
		amdemod_fe_cfg_put(fe->cfg);
		fe->fe = NULL;
		fe->cfg = NULL;
		fe->id = -1;
	}
}


static int amdemod_fe_init(struct aml_dvb *advb, struct platform_device *pdev, struct aml_fe *fe, int id)
{
	int ret=0;
	struct amlfe_config *cfg = NULL;
	
	pr_dbg("init Amdemod frontend %d\n", id);

	ret = amdemod_fe_cfg_get(advb, pdev, id, &cfg);
	if(ret != 0){
		goto err_init;
	}

	ret = amdemod_fe_register(advb, fe, cfg);
	if(ret != 0){
		goto err_init;
	}

	fe->id = id;
	fe->cfg = cfg;
	
	return 0;

err_init:
	amdemod_fe_cfg_put(cfg);
	return ret;

}




static int amlfe_change_mode(struct aml_fe *fe, int mode)
{
	int ret;
	struct aml_dvb *dvb = aml_get_dvb_device();
	struct amlfe_config *cfg = (struct amlfe_config *)fe->cfg;

	if(fe->fe)
	{
		dvb_unregister_frontend(fe->fe);
		dvb_frontend_detach(fe->fe);
	}
	
	cfg->fe_mode = mode;
	
	ret = amdemod_fe_register(dvb, fe, cfg);
	if(ret < 0) {
	}
		
	return ret;
}

static ssize_t amlfe_show_mode(struct class *class, struct class_attribute *attr,char *buf)
{
	int ret;
	struct aml_fe *fe = container_of(class,struct aml_fe,class);
	struct amlfe_config *cfg = (struct amlfe_config *)fe->cfg;

	ret = sprintf(buf, ":%d\n0-DVBC, 1-DVBT, 2-ISDBT, 3-DTMB, 4-ATSC\n", cfg->fe_mode);
	return ret;
}

static ssize_t amlfe_store_mode(struct class *class,struct class_attribute *attr,
                          const char *buf,
                          size_t size)
{
	int mode = simple_strtol(buf,0,16);
//	printk("dtmb mode go go go!!! mode is %d\n",mode);
	if(mode<0 || mode>4){
		pr_error("fe mode must 0~4\n");
	}else{
		if(frontend_mode != mode){
			struct aml_fe *fe = container_of(class,struct aml_fe,class);
			if(amlfe_change_mode(fe, mode)!=0){
				pr_error("fe mode change failed.\n");
			}else{
				frontend_mode = mode;
			}
		}
	}
	
	return size;
}

#ifdef CONFIG_AM_DEMOD_DEBUG
static int debugif = 0;
static ssize_t aml_demod_debugif_set(struct class *cla,struct class_attribute *attr,
									const char *buf,
                                size_t count)
{
	size_t r;
	int enable=0;

	r = sscanf(buf, "%d", &enable);
	if (r != 1)
		return -EINVAL;

	if(enable != debugif) {
		if(enable)
			aml_demod_init();
		else
			aml_demod_exit();
		debugif = enable;
	}

	return count;
}


static ssize_t aml_demod_debugif_show(struct class *class, struct class_attribute *attr,char *buf)
{
    return sprintf(buf, "debug IF: %s\n", debugif?"on":"off");
}
#endif

static struct class_attribute amlfe_class_attrs[] = {
	__ATTR(mode,  S_IRUGO | S_IWUSR, amlfe_show_mode, amlfe_store_mode),	
#ifdef CONFIG_AM_DEMOD_DEBUG
	__ATTR(debugif, S_IRUGO | S_IWUSR, aml_demod_debugif_show, aml_demod_debugif_set),		
#endif
	__ATTR_NULL
};

static int amlfe_register_class(struct aml_fe *fe)
{
#define CLASS_NAME_LEN 48
	int ret;
	struct class *clp;
	
	clp = &(fe->class);

	clp->name = kzalloc(CLASS_NAME_LEN,GFP_KERNEL);
	if (!clp->name)
		return -ENOMEM;
	
	snprintf((char *)clp->name, CLASS_NAME_LEN, "amlfe-%d", fe->id);
	clp->owner = THIS_MODULE;
	clp->class_attrs = amlfe_class_attrs;
	ret = class_register(clp);
	if (ret)
		kfree(clp->name);
	
	return 0;
}

static int amlfe_unregister_class(struct aml_fe *fe)
{
	class_unregister(&fe->class);
	kzfree(fe->class.name);
	return 0;
}

static int amdemod_fe_probe(struct platform_device *pdev)
{
	struct aml_dvb *dvb = aml_get_dvb_device();

	pr_dbg("amdemod_fe_probe \n");
	
	if(amdemod_fe_init(dvb, pdev, &amdemod_fe[0], 0)<0)
		return -ENXIO;

	platform_set_drvdata(pdev, &amdemod_fe[0]);

	amlfe_register_class(&amdemod_fe[0]);

	return 0;
}

static int amdemod_fe_remove(struct platform_device *pdev)
{
	struct aml_fe *fe = platform_get_drvdata(pdev);
	struct aml_dvb *dvb = aml_get_dvb_device();

	platform_set_drvdata(pdev, NULL);

	amlfe_unregister_class(fe);
	
	amdemod_fe_release(dvb, fe);

	return 0;
}

static struct platform_driver aml_fe_driver = {
	.probe		= amdemod_fe_probe,
	.remove		= amdemod_fe_remove,	
	.driver		= {
		.name	= "amlfe",
		.owner	= THIS_MODULE,
	}
};

static int __init amlfrontend_init(void)
{
	pr_dbg("Amlogic Demod DVB-T/C Init\n");
	return platform_driver_register(&aml_fe_driver);
}


static void __exit amlfrontend_exit(void)
{
	pr_dbg("Amlogic Demod DVB-T/C Exit\n");
	platform_driver_unregister(&aml_fe_driver);
}

late_initcall(amlfrontend_init);
module_exit(amlfrontend_exit);


MODULE_DESCRIPTION("AML DEMOD DVB-C/T Demodulator driver");
MODULE_LICENSE("GPL");


