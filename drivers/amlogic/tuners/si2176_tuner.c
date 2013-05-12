/*
 * Silicon labs si2176 Tuner Device Driver
 *
 * Author: Bobby Yang <bo.yang@amlogic.com>
 *
 *
 * Copyright (C) 2010 Amlogic Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


/* Standard Liniux Headers */
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/delay.h>

/* Amlogic Headers */
#include <linux/tvin/tvin.h>

/* Local Headers */
#include "tvin_tuner.h"
#include "tvin_demod.h"
#include "si2176_func.h"

#define SI2176_TUNER_I2C_NAME           "si2176_tuner_i2c"
#define TUNER_DEVICE_NAME               "si2176"

static struct i2c_client *tuner_client = NULL;

static si2176_common_reply_struct si_common_reply;
static si2176_propobj_t     si_prop;
static si2176_cmdreplyobj_t si_cmd_reply;
static tuner_std_id  si_std_id = 0;

/************************************************************************************************************************/
/** globals for Top Level routine */
unsigned int newfrequency;

/* ---------------------------------------------------------------------- */
static enum tuner_signal_status_e si2176_tuner_islocked(void)
{
    enum tuner_signal_status_e ret = TUNER_SIGNAL_STATUS_WEAK;
#if 0
    if (si_common_reply.tunint && si_common_reply.atvint)
    {
        pr_info("%s: Strong signal detected...tunint:%d.atvint:%d.....\n", __func__,
                            si_common_reply.tunint,
                            si_common_reply.atvint);
        ret = TUNER_SIGNAL_STATUS_STRONG;
    }
    else
        pr_info("%s: Weak signal detected.........\n", __func__);
#else
    si2176_tuner_status(tuner_client, SI2176_ATV_STATUS_CMD_INTACK_OK, &si_cmd_reply);
#if 0
    if (!si_cmd_reply.tuner_status.status->atvint || !si_cmd_reply.tuner_status.status->tunint)
    {
        pr_info("%s: tuner unstable atvint:%d tunint:%d............\n", __func__,
            si_cmd_reply.tuner_status.status->atvint,
            si_cmd_reply.tuner_status.status->tunint);
        return ret;
    }
#endif
    if (!si_cmd_reply.tuner_status.rssih && !si_cmd_reply.tuner_status.rssil)
    {
    	if(SI2176_DEBUG)
        pr_info("%s: Strong signal detected.........\n", __func__);
        ret = TUNER_SIGNAL_STATUS_STRONG;
    }
	else{
		if(SI2176_DEBUG)
        pr_info("%s: Weak signal rssih:%d rssil:%d............\n", __func__,
            si_cmd_reply.tuner_status.rssih,
            si_cmd_reply.tuner_status.rssil);
		}
#endif
    return ret;
}

/* ---------------------------------------------------------------------- */
int tvin_set_tuner(void)
{
    int ret = 0;
#if 0
    si2176_tuner_status(tuner_client, SI2176_ATV_STATUS_CMD_INTACK_OK, &si_cmd_reply);
    si2176_atv_status(tuner_client, SI2176_ATV_STATUS_CMD_INTACK_OK, &si_cmd_reply);

    pr_info("...range_khz:%d, \n...atv_cvbs_out.amp:%d,\n...offset:%d,\n...atv_cvbs_out_fine.amp:%d,\n...offset:%d,\n...atv_min_lvl_lock.thrs:%d,\n...atv_rf_top.atv_rf_top:%d,\n...atv_rsq_rssi_threshold.hi:%d, \n...atv_rsq_rssi_threshold.lo:%d,\n...atv_rsq_rssi_threshold.hi:%d,\n...atv_rsq_rssi_threshold.lo:%d,\n...atv_rsq_snr_threshold.hi:%d,\n...atv_rsq_snr_threshold.lo:%d,\n...atv_video_equalizer.slope:%d,\n...atv_video_mode.color:%d,\n...atv_video_mode.invert_signal:%d,\n...atv_video_mode.trans:%d,\n...atv_video_mode.video_sys:%d,\n...atv_vsnr_cap.atv_vsnr_cap:%d,\n...atv_vsync_tracking.min_fields_to_unlock:%d,\n...atv_vsync_tracking.min_pulses_to_lock:%d,\n...tuner_blocked_vco.vco_code:%d \n",
            si_prop.atv_afc_range.range_khz,
            si_prop.atv_cvbs_out.amp,
            si_prop.atv_cvbs_out.offset,
            si_prop.atv_cvbs_out_fine.amp,
            si_prop.atv_cvbs_out_fine.offset,
            si_prop.atv_min_lvl_lock.thrs,
            si_prop.atv_rf_top.atv_rf_top,
            si_prop.atv_rsq_rssi_threshold.hi,
            si_prop.atv_rsq_rssi_threshold.lo,
            si_prop.atv_rsq_rssi_threshold.hi,
            si_prop.atv_rsq_rssi_threshold.lo,
            si_prop.atv_rsq_snr_threshold.hi,
            si_prop.atv_rsq_snr_threshold.lo,
            si_prop.atv_video_equalizer.slope,
            si_prop.atv_video_mode.color,
            si_prop.atv_video_mode.invert_signal,
            si_prop.atv_video_mode.trans,
            si_prop.atv_video_mode.video_sys,
            si_prop.atv_vsnr_cap.atv_vsnr_cap,
            si_prop.atv_vsync_tracking.min_fields_to_unlock,
            si_prop.atv_vsync_tracking.min_pulses_to_lock,
            si_prop.tuner_blocked_vco.vco_code);
    pr_info("...atv_status.chlint:%d, \n...atv_status.pclint:%d, \n...atv_status.dlint:%d, \n...atv_status.snrlint:%d, \n...atv_status.snrhint:%d, \n...atv_status.chl:%d, \n...atv_status.pcl:%d, \n...atv_status.dl:%d, \n...atv_status.snrl:%d, \n...atv_status.snrh:%d, \n...atv_status.video_snr:%d, \n...atv_status.afc_freq:%d, \n...atv_status.video_sc_spacing:%d, \n...atv_status.video_sys:%d, \n...atv_status.color:%d, \n...atv_status.trans:%d, \n...tuner_status.vco_code:%d, \n...tuner_status.tc:%d, \n...tuner_status.rssil:%d, \n...tuner_status.rssih:%d, \n...tuner_status.rssi:%d, \n...tuner_status.freq:%d, \n...tuner_status.mode:%d \n",
            si_cmd_reply.atv_status.chlint,
            si_cmd_reply.atv_status.pclint,
            si_cmd_reply.atv_status.dlint,
            si_cmd_reply.atv_status.snrlint,
            si_cmd_reply.atv_status.snrhint,
            si_cmd_reply.atv_status.chl,
            si_cmd_reply.atv_status.pcl,
            si_cmd_reply.atv_status.dl,
            si_cmd_reply.atv_status.snrl,
            si_cmd_reply.atv_status.snrh,
            si_cmd_reply.atv_status.video_snr,
            si_cmd_reply.atv_status.afc_freq,
            si_cmd_reply.atv_status.video_sc_spacing,
            si_cmd_reply.atv_status.video_sys,
            si_cmd_reply.atv_status.color,
            si_cmd_reply.atv_status.trans,
            si_cmd_reply.tuner_status.vco_code,
            si_cmd_reply.tuner_status.tc,
            si_cmd_reply.tuner_status.rssil,
            si_cmd_reply.tuner_status.rssih,
            si_cmd_reply.tuner_status.rssi,
            si_cmd_reply.tuner_status.freq,
            si_cmd_reply.tuner_status.mode);

    pr_info("...common_reply.atvint:%d, \n...common_reply.cts:%d, \n...common_reply.err:%d, \n...common_reply.tunint:%d, \n",
            si_common_reply.atvint,
            si_common_reply.cts,
            si_common_reply.err,
            si_common_reply.tunint);
#endif

    return ret;
}

char *tvin_tuenr_get_name(void)
{
    return TUNER_DEVICE_NAME;
}

void tvin_get_tuner( struct tuner_parm_s *ptp)
{
    ptp->rangehigh  = 48250000;
    ptp->rangelow   = 855250000;
    ptp->signal     = si2176_tuner_islocked();
}

void tvin_tuner_get_std(tuner_std_id *ptstd)
{
    *ptstd = si_std_id;
}

void tvin_tuner_set_std(tuner_std_id ptstd)
{
    si_std_id = ptstd;


	if (si_std_id & (TUNER_STD_PAL | TUNER_STD_NTSC))
	{
		si_prop.atv_video_mode.color = SI2176_ATV_VIDEO_MODE_PROP_COLOR_PAL_NTSC;
	}
	else if (si_std_id & (TUNER_STD_SECAM))
	{
		si_prop.atv_video_mode.color = SI2176_ATV_VIDEO_MODE_PROP_COLOR_SECAM;
	}
	if(SI2176_DEBUG)
    pr_info("%s: atv_video_mode.color:%d \n", __func__, si_prop.atv_video_mode.color);
}

void tvin_tuner_get_freq(struct tuner_freq_s *ptf)
{
    ptf->freq = newfrequency;
}

void tvin_tuner_set_freq(struct tuner_freq_s tf)
{
    newfrequency = tf.freq;
	if(SI2176_DEBUG)
    pr_info("%s: ptf->freq:%d \n", __func__, tf.freq);
}

int  tvin_set_demod(unsigned int bce)
{
    int ret = 0;

    ret = si2176_tune(tuner_client, SI2176_TUNER_TUNE_FREQ_CMD_MODE_ATV, newfrequency, &si_cmd_reply, &si_common_reply);
    if (ret)
        pr_info("%s: atv tune error:%d!!!!\n", __func__, ret);

    return ret;
}

void tvin_demod_set_std(tuner_std_id ptstd)
{
    si_std_id = ptstd;
    
    //audio mute judge
    if (si_std_id == 0) {
        si_prop.atv_af_out.volume = 0;
        if (SI2176_DEBUG) {
            pr_info("si2176 audio mute on, si_std_id = %d.\n", si_std_id);
        }
    } else {
        si_prop.atv_af_out.volume = SI2176_ATV_AF_OUT_PROP_VOLUME_VOLUME_MAX;
        if (SI2176_DEBUG) {
            pr_info("si2176 audio mute off, si_std_id = %d.\n", si_std_id);
        }
    }

    if (si_std_id & TUNER_STD_PAL_B)
        si_prop.atv_video_mode.video_sys = SI2176_ATV_VIDEO_MODE_PROP_VIDEO_SYS_B;
    else if (si_std_id & (TUNER_STD_PAL_G | TUNER_STD_PAL_H))
	    si_prop.atv_video_mode.video_sys = SI2176_ATV_VIDEO_MODE_PROP_VIDEO_SYS_GH;
    else if (si_std_id & (TUNER_STD_PAL_M  |
                          TUNER_STD_NTSC_M |
                          TUNER_STD_NTSC_M_JP))
	    si_prop.atv_video_mode.video_sys = SI2176_ATV_VIDEO_MODE_PROP_VIDEO_SYS_M;
    else if (si_std_id & TUNER_STD_PAL_N)
	    si_prop.atv_video_mode.video_sys = SI2176_ATV_VIDEO_MODE_PROP_VIDEO_SYS_N;
    else if (si_std_id & TUNER_STD_PAL_I)
	    si_prop.atv_video_mode.video_sys = SI2176_ATV_VIDEO_MODE_PROP_VIDEO_SYS_I;
    else if (si_std_id & (TUNER_STD_PAL_D  |
                          TUNER_STD_PAL_D1 |
                          TUNER_STD_PAL_DK))
	    si_prop.atv_video_mode.video_sys = SI2176_ATV_VIDEO_MODE_PROP_VIDEO_SYS_DK;
    else if (si_std_id & TUNER_STD_SECAM_L)
	    si_prop.atv_video_mode.video_sys = SI2176_ATV_VIDEO_MODE_PROP_VIDEO_SYS_L;
    else if (si_std_id & TUNER_STD_SECAM_LC)
	    si_prop.atv_video_mode.video_sys = SI2176_ATV_VIDEO_MODE_PROP_VIDEO_SYS_LP;
    if (si2176_atvconfig(tuner_client, &si_prop, &si_cmd_reply)!=0)
		{
			if(SI2176_DEBUG)
	    pr_info("%s:set std error\n",__func__);
    }
	if(SI2176_DEBUG)
    pr_info("%s: atv_video_mode.video_sys:%d ..........\n", __func__, si_prop.atv_video_mode.video_sys);

}

void tvin_demod_get_afc(struct tuner_parm_s *ptp)
{
#if 0
    if (si_common_reply.tunint && si_common_reply.atvint)
    {
        pr_info("%s: AFC valid!!!!...tunint:%d.atvint:%d.....\n", __func__,
                            si_common_reply.tunint,
                            si_common_reply.atvint);
        ptp->if_status = 0xE0;
    }
    else
    {
        ptp->if_status = 0;
        pr_info("%s: AFC bad.........\n", __func__);
    }
#else
    si2176_atv_status(tuner_client, SI2176_ATV_STATUS_CMD_INTACK_OK, &si_cmd_reply);
#if 0
    if (!si_cmd_reply.atv_status.status->atvint || !si_cmd_reply.atv_status.status->tunint)
    {
        pr_info("%s: demod unstable atvint:%d tunint:%d............\n", __func__,
            si_cmd_reply.atv_status.status->atvint,
            si_cmd_reply.atv_status.status->tunint);
        ptp->if_status = 0;
    }
#endif

    if (si_cmd_reply.atv_status.pcl && si_cmd_reply.atv_status.dl)
    {
        ptp->if_status = 0xE0;
		if(SI2176_DEBUG)
        pr_info("%s: AFC & demod locked:0x%x \n", __func__, ptp->if_status);
    }
	else
		{
		if(SI2176_DEBUG)
        pr_info("%s: AFC & demod unlocked atvint:%d tunint:%d \n", __func__,
            si_cmd_reply.atv_status.status->atvint,
            si_cmd_reply.atv_status.status->tunint);
		}
#endif
}
    
#ifdef CONFIG_TVIN_TUNER_SI2176
void si2176_get_tuner_status(struct si2176_tuner_status_s *si2176_s)
{

    if(si2176_tuner_status(tuner_client, SI2176_ATV_STATUS_CMD_INTACK_OK, &si_cmd_reply)!=0)
    {
        pr_info("%s:get si2176 tuner status error!!!\n",__func__);
        return;
    }
    si2176_s->si2176_tuner_s.tcint    = si_cmd_reply.tuner_status.tcint;
    si2176_s->si2176_tuner_s.rssilint = si_cmd_reply.tuner_status.rssilint;
    si2176_s->si2176_tuner_s.rssihint = si_cmd_reply.tuner_status.rssihint;
    si2176_s->si2176_tuner_s.vco_code = si_cmd_reply.tuner_status.vco_code;
    si2176_s->si2176_tuner_s.tc       = si_cmd_reply.tuner_status.tc;
    si2176_s->si2176_tuner_s.rssil    = si_cmd_reply.tuner_status.rssil;
    si2176_s->si2176_tuner_s.rssih    = si_cmd_reply.tuner_status.rssih;
    si2176_s->si2176_tuner_s.rssi     = si_cmd_reply.tuner_status.rssi;
    si2176_s->si2176_tuner_s.freq     = si_cmd_reply.tuner_status.freq;
    si2176_s->si2176_tuner_s.mode     = si_cmd_reply.tuner_status.mode;
    if(SI2176_DEBUG)
    {
    pr_info("%s:get si2176_tuner_status:tcint %u,rssilint %u,rssihint \
    %u,vco_code %d,tc %u,rssil %u,rssih %u,rssi %d,freq %u,mode %u.\n",\
    __func__,si2176_s->si2176_tuner_s.tcint,si2176_s->si2176_tuner_s.rssilint,\
    si2176_s->si2176_tuner_s.rssihint,si2176_s->si2176_tuner_s.vco_code,\
    si2176_s->si2176_tuner_s.tc,si2176_s->si2176_tuner_s.rssil,si2176_s->si2176_tuner_s.rssih,\
    si2176_s->si2176_tuner_s.rssi,si2176_s->si2176_tuner_s.freq,si2176_s->si2176_tuner_s.mode);
    }
    if(si2176_atv_status(tuner_client, SI2176_ATV_STATUS_CMD_INTACK_OK, &si_cmd_reply)!=0)
    {
        pr_info("%s:get si2176 atv status error!!!\n",__func__);
        return;
    }
    si2176_s->si2176_atv_s.chlint           = si_cmd_reply.atv_status.chlint;
    si2176_s->si2176_atv_s.pclint           = si_cmd_reply.atv_status.pclint;
    si2176_s->si2176_atv_s.dlint            = si_cmd_reply.atv_status.dlint;
    si2176_s->si2176_atv_s.snrlint          = si_cmd_reply.atv_status.snrlint;
    si2176_s->si2176_atv_s.snrhint          = si_cmd_reply.atv_status.snrhint;
    si2176_s->si2176_atv_s.audio_chan_bw    = si_cmd_reply.atv_status.audio_chan_bw;
    si2176_s->si2176_atv_s.chl              = si_cmd_reply.atv_status.chl;
    si2176_s->si2176_atv_s.pcl              = si_cmd_reply.atv_status.pcl;
    si2176_s->si2176_atv_s.dl               = si_cmd_reply.atv_status.dl;
    si2176_s->si2176_atv_s.snrl             = si_cmd_reply.atv_status.snrl;
    si2176_s->si2176_atv_s.snrh             = si_cmd_reply.atv_status.snrh;
    si2176_s->si2176_atv_s.video_snr        = si_cmd_reply.atv_status.video_snr;
    si2176_s->si2176_atv_s.afc_freq         = si_cmd_reply.atv_status.afc_freq;
    si2176_s->si2176_atv_s.video_sc_spacing = si_cmd_reply.atv_status.video_sc_spacing;
    si2176_s->si2176_atv_s.video_sys        = si_cmd_reply.atv_status.video_sys;
    si2176_s->si2176_atv_s.color            = si_cmd_reply.atv_status.color;
    si2176_s->si2176_atv_s.trans            = si_cmd_reply.atv_status.trans;
    si2176_s->si2176_atv_s.audio_sys        = si_cmd_reply.atv_status.audio_sys;
    si2176_s->si2176_atv_s.audio_demod_mode = si_cmd_reply.atv_status.audio_demod_mode;
    if(SI2176_DEBUG)
    {
    pr_info("%s:get_si2176_atv_status:chlint %u,pclint %u,dlint %u,snrlint \
    %u,snrhint %u,audio_chan_bw %u,chl %u,pcl %u,dl %u,snrl %u,snrh \
    %u,video_snr %u,afe_freq %d,video_sc_spacing %d,video_sys %u,color \
    %u,audio_sys %u,audio_demod_mode %u.\n",__func__,si2176_s->si2176_atv_s.chlint,si2176_s->si2176_atv_s.pclint,\
    si2176_s->si2176_atv_s.dlint,si2176_s->si2176_atv_s.snrlint,si2176_s->si2176_atv_s.snrhint,\
    si2176_s->si2176_atv_s.audio_chan_bw,si2176_s->si2176_atv_s.chl,si2176_s->si2176_atv_s.pcl,\
    si2176_s->si2176_atv_s.dl,si2176_s->si2176_atv_s.snrl,si2176_s->si2176_atv_s.snrh,\
    si2176_s->si2176_atv_s.video_snr,si2176_s->si2176_atv_s.afc_freq,si2176_s->si2176_atv_s.video_sc_spacing,\
    si2176_s->si2176_atv_s.video_sys,si2176_s->si2176_atv_s.color,si2176_s->si2176_atv_s.audio_sys,\
    si2176_s->si2176_atv_s.audio_demod_mode);
    }
}
#endif
static int si2176_tuner_probe(struct i2c_client *client, const struct i2c_device_id *id)
{

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        pr_info("%s: functionality check failed\n", __FUNCTION__);
        return -ENODEV;
    }
    tuner_client = client;
	if(SI2176_DEBUG)
    pr_info( "tuner_client->addr = 0x%x\n", tuner_client->addr);

    //int si2176
    si2176_init(tuner_client, &si_cmd_reply, &si_common_reply);
    si2176_configure(tuner_client, &si_prop, &si_cmd_reply, &si_common_reply);

	return 0;
}

static int si2176_tuner_remove(struct i2c_client *client)
{
	if(SI2176_DEBUG)
    pr_info("%s driver removed ok.\n", client->name);
	return 0;
}

static const struct i2c_device_id si2176_tuner_id[] = {
	{SI2176_TUNER_I2C_NAME, 0},
	{ }
};
MODULE_DEVICE_TABLE(i2c, si2176_tuner_id);

static struct i2c_driver si2176_tuner_driver = {
	.driver = {
        .owner  = THIS_MODULE,
		.name   = SI2176_TUNER_I2C_NAME,
	},
	.probe		= si2176_tuner_probe,
	.remove		= si2176_tuner_remove,
	.id_table	= si2176_tuner_id,
};

static int __init si2176_tuner_init(void)
{
    int ret = 0;

    ret = i2c_add_driver(&si2176_tuner_driver);

    if (ret < 0 || tuner_client == NULL) {
        pr_err("tuner: failed to add i2c driver. \n");
        ret = -ENOTSUPP;
    }

	return ret;
}

static void __exit si2176_tuner_exit(void)
{
	if(SI2176_DEBUG)
    pr_info( "%s . \n", __FUNCTION__ );
    i2c_del_driver(&si2176_tuner_driver);
}

MODULE_AUTHOR("Bobby Yang <bo.yang@amlogic.com>");
MODULE_DESCRIPTION("Xuguang htm-9a-w125 tuner i2c device driver");
MODULE_LICENSE("GPL");

module_init(si2176_tuner_init);
module_exit(si2176_tuner_exit);

