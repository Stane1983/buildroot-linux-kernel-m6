/*
 * Silicon labs si2196 Tuner Device Driver
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
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/dvb/frontend.h>

/* Local Headers */
#include "../aml_fe.h"
#include "si2196_func.h"


#define SI2196_TUNER_I2C_NAME           "si2196_tuner_i2c"
#define TUNER_DEVICE_NAME                 "si2196"

static unsigned int si2196_debug = 0;
module_param(si2196_debug,uint,0664);
MODULE_PARM_DESC(si2196_debug,"\nenable print the lock state and tuner status\n");
#define siprintk   if(si2196_debug) printk
struct si2196_device_s *si2196_devp;
#define ERROR   1
/************************************************************************************************************************/
/** globals for Top Level routine */
static void si2196_set_frequency(unsigned int freq);
static ssize_t si2196_store(struct class *cls, struct class_attribute *attr, const char *buf, size_t count)
{
    int n = 0;
    unsigned char ret =0;
    char *buf_orig, *ps, *token;
    char *parm[4];
    unsigned int addr = 0, val = 0;
    buf_orig = kstrdup(buf, GFP_KERNEL);
    ps = buf_orig;
    while (1) {
        token = strsep(&ps, " \n");
        if (token == NULL)
            break;
        if (*token == '\0')
            continue;
        parm[n++] = token;
    }

    if (!strncmp(parm[0],"rs",strlen("rs")))
    {
        if (n != 2)
        {
            pr_err("read: invalid parameter\n");
            kfree(buf_orig);
            return count;
        }
        addr = simple_strtol(parm[1], NULL, 16);
        printk("%s 0x%x\n", parm[0], addr);
        si2196_get_property(&si2196_devp->tuner_client,0, addr, &si2196_devp->si_cmd_reply);
        printk("%s: 0x%x --> 0x%x\n", parm[0], addr, si2196_devp->si_cmd_reply.get_property.data);
    }
    else if (!strncmp(parm[0],"ws",strlen("ws")))
    {
        addr = simple_strtol(parm[1], NULL, 16);
        val  = simple_strtol(parm[2], NULL, 16);        
        printk("%s 0x%x 0x%x", parm[0], addr, val);
        si2196_set_property(&si2196_devp->tuner_client, 0, addr,val, &si2196_devp->si_cmd_reply);
        printk("%s: 0x%x <-- 0x%x\n", parm[0], addr, val);
    }
    else if(!strcmp(parm[0],"tune"))
    {
        val = simple_strtol(parm[1], NULL, 10);
        si2196_set_frequency(val);
    }
    else if(!strcmp(parm[0],"tuner_status"))
    {
        ret = si2196_sendcommand(&si2196_devp->tuner_client,SI2196_TUNER_STATUS_CMD, &si2196_devp->si_cmd, &si2196_devp->si_cmd_reply);
        if(!ret)
        {   
            pr_info("tuner_status 0x%x",SI2196_TUNER_STATUS_CMD);
            pr_info("tcint %u,rssilint %u,rssihint %u,vco_code %d,tc %u,rssil %u,rssih %u,rssi %d,freq %u,mode %u.\n",\
            si2196_devp->si_cmd_reply.tuner_status.tcint, si2196_devp->si_cmd_reply.tuner_status.rssilint, si2196_devp->si_cmd_reply.tuner_status.rssihint,\
            si2196_devp->si_cmd_reply.tuner_status.vco_code, si2196_devp->si_cmd_reply.tuner_status.tc,si2196_devp->si_cmd_reply.tuner_status.rssil,\
            si2196_devp->si_cmd_reply.tuner_status.rssih, si2196_devp->si_cmd_reply.tuner_status.rssi,si2196_devp->si_cmd_reply.tuner_status.freq,\
            si2196_devp->si_cmd_reply.tuner_status.mode);
        }
    }
    else if(!strncmp(parm[0],"atv_status",strlen("atv_status")))
    {
        ret = si2196_sendcommand(&si2196_devp->tuner_client, SI2196_ATV_STATUS_CMD, &si2196_devp->si_cmd, &si2196_devp->si_cmd_reply);
        if(!ret)
        {
            printk("atv_status 0x%x",SI2196_ATV_STATUS_CMD);
            pr_info("audio_chan_bw %u,chl %u,pcl %u,dl %u, snrl %u,snrh %u,video_snr %u,"\
                        "afc_freq %d,video_sc_spacing %d,video_sys %u,color %u, audio_sys %u\n",\
            si2196_devp->si_cmd_reply.atv_status.audio_chan_bw,si2196_devp->si_cmd_reply.atv_status.chl,\
            si2196_devp->si_cmd_reply.atv_status.pcl, si2196_devp->si_cmd_reply.atv_status.dl, si2196_devp->si_cmd_reply.atv_status.snrl,\
            si2196_devp->si_cmd_reply.atv_status.snrh, si2196_devp->si_cmd_reply.atv_status.video_snr, si2196_devp->si_cmd_reply.atv_status.afc_freq, \
            si2196_devp->si_cmd_reply.atv_status.video_sc_spacing, si2196_devp->si_cmd_reply.atv_status.video_sys, si2196_devp->si_cmd_reply.atv_status.color, \
            si2196_devp->si_cmd_reply.atv_status.audio_sys);
        }
    }
    else if(!strncmp(parm[0],"atv_restart",strlen("atv_restart")))
    {
        si2196_sendcommand(&si2196_devp->tuner_client, SI2196_ATV_RESTART_CMD, &si2196_devp->si_cmd, &si2196_devp->si_cmd_reply);
    }
    else if(!strncmp(parm[0],"sd_afc",strlen("sd_afc")))
    {
        ret = si2196_sendcommand(&si2196_devp->tuner_client, SI2196_SD_AFC_CMD, &si2196_devp->si_cmd, &si2196_devp->si_cmd_reply);
        if(!ret)
            pr_info("sd_afc 0x%x,afc %d.\n",SI2196_SD_AFC_CMD,si2196_devp->si_cmd_reply.sd_afc.afc);
    }
    else if(!strncmp(parm[0],"sd_carrier_cnr",strlen("sd_carrier_cnr")))
    {
        ret = si2196_sendcommand(&si2196_devp->tuner_client, SI2196_SD_CARRIER_CNR_CMD, &si2196_devp->si_cmd, &si2196_devp->si_cmd_reply);
        if(!ret)
            pr_info("sd_carrier_cnr 0x%x primary %u, secondary %u.\n",SI2196_SD_CARRIER_CNR_CMD,si2196_devp->si_cmd_reply.sd_carrier_cnr.primary,si2196_devp->si_cmd_reply.sd_carrier_cnr.secondary);
    }
    else if(!strncmp(parm[0],"sd_casd",strlen("sd_casd")))
    {
        ret = si2196_sendcommand(&si2196_devp->tuner_client, SI2196_SD_CASD_CMD, &si2196_devp->si_cmd, &si2196_devp->si_cmd_reply);
        if(!ret)
            pr_info("sd_casd 0x%x -->casd %u.\n",SI2196_SD_CASD_CMD, si2196_devp->si_cmd_reply.sd_casd.casd);
    }
    else if(!strncmp(parm[0],"sd_dual_mono_id_lvl",strlen("sd_dual_mono_id_lvl")))
    {
        ret = si2196_sendcommand(&si2196_devp->tuner_client, SI2196_SD_DUAL_MONO_ID_LVL_CMD, &si2196_devp->si_cmd, &si2196_devp->si_cmd_reply);
        if(!ret)
            pr_info("sd_dual_mono_id_lvl 0x%x id_lvl %u.\n",SI2196_SD_DUAL_MONO_ID_LVL_CMD, si2196_devp->si_cmd_reply.sd_dual_mono_id_lvl.id_lvl);
    }
    else if(!strncmp(parm[0],"sd_nicam_status",strlen("sd_nicam_status")))
    {
        ret = si2196_sendcommand(&si2196_devp->tuner_client, SI2196_SD_NICAM_STATUS_CMD, &si2196_devp->si_cmd, &si2196_devp->si_cmd_reply);
        if(!ret)
        {
            pr_info("sd_nicam_status 0x%x. ",SI2196_SD_NICAM_STATUS_CMD);
            pr_info("errors %u,locked %u, mode %u,mono_backup %u,rss %u.\n",si2196_devp->si_cmd_reply.sd_nicam_status.errors,\
                si2196_devp->si_cmd_reply.sd_nicam_status.locked,si2196_devp->si_cmd_reply.sd_nicam_status.mode,\
                si2196_devp->si_cmd_reply.sd_nicam_status.mono_backup,si2196_devp->si_cmd_reply.sd_nicam_status.rss);
        }
        
    }
    else if(!strncmp(parm[0],"sd_status",strlen("sd_status")))
    {
        ret = si2196_sendcommand(&si2196_devp->tuner_client, SI2196_SD_STATUS_CMD, &si2196_devp->si_cmd, &si2196_devp->si_cmd_reply);
        if(!ret)
        {
            pr_info("sd_status 0x%x. ",SI2196_SD_STATUS_CMD);
            pr_info("afcm %u,afcmint %u,agcs %u,agcsint %u,asdc %u,asdcint %u,nicam %u,nicamint %u,odm %u,"\
                "odmint %u,over_dev %u,pcm %u,pcmint %u,sap_detected %u,scm %u,scmint %u,sd_agc %u,sif_agc %u,"\
                "sound_mode_detected %u,sound_system_detected %u,sd_status_int %u.\n",
                si2196_devp->si_cmd_reply.sd_status.afcm,si2196_devp->si_cmd_reply.sd_status.afcmint,si2196_devp->si_cmd_reply.sd_status.agcs,\
                si2196_devp->si_cmd_reply.sd_status.agcsint,si2196_devp->si_cmd_reply.sd_status.asdc,si2196_devp->si_cmd_reply.sd_status.asdcint,\
                si2196_devp->si_cmd_reply.sd_status.nicam,si2196_devp->si_cmd_reply.sd_status.nicamint,si2196_devp->si_cmd_reply.sd_status.odm,\
                si2196_devp->si_cmd_reply.sd_status.odmint,si2196_devp->si_cmd_reply.sd_status.over_dev,si2196_devp->si_cmd_reply.sd_status.pcm,\
                si2196_devp->si_cmd_reply.sd_status.pcmint,si2196_devp->si_cmd_reply.sd_status.sap_detected,si2196_devp->si_cmd_reply.sd_status.scm, \
                si2196_devp->si_cmd_reply.sd_status.scmint,si2196_devp->si_cmd_reply.sd_status.sd_agc,si2196_devp->si_cmd_reply.sd_status.sif_agc,\
                si2196_devp->si_cmd_reply.sd_status.sound_mode_detected,si2196_devp->si_cmd_reply.sd_status.sound_system_detected,si2196_devp->si_cmd_reply.sd_status.ssint);
        }
    }
    else if(!strncmp(parm[0],"sd_stereo_id_lvl",strlen("sd_stereo_id_lvl")))
    {
        ret = si2196_sendcommand(&si2196_devp->tuner_client, SI2196_SD_STEREO_ID_LVL_CMD, &si2196_devp->si_cmd, &si2196_devp->si_cmd_reply);
        if(!ret)
            pr_info("sd_stereo_id_lvl 0x%x id_lvl %u.\n",SI2196_SD_STEREO_ID_LVL_CMD,si2196_devp->si_cmd_reply.sd_stereo_id_lvl.id_lvl);
    }
    else if(!strncmp(parm[0],"adac_powerup",strlen("adac_powerup")))
    {
        ret = si2196_sendcommand(&si2196_devp->tuner_client, SI2196_SD_ADAC_POWER_UP_CMD, &si2196_devp->si_cmd, &si2196_devp->si_cmd_reply);
    }
    else
        pr_err("invalid command\n");

    kfree(buf_orig);
    return count;
}
static ssize_t si2196_show(struct class *cls, struct class_attribute *attr,char *buff)
{
    size_t len = 0;
    
    return len;
}
static CLASS_ATTR(si2196_debug,0644,si2196_show,si2196_store);

/* ---------------------------------------------------------------------- */

static int si2196_get_rf_strength(struct dvb_frontend *fe, u16 *strength)
{
    if(!strength)
        printk("[si2196..]%s null pointer error.\n",__func__);
    if(si2196_tuner_status(&si2196_devp->tuner_client, 1, &si2196_devp->si_cmd_reply))
    {
        printk("[si2196..] %s si2196_tuner_status error.\n",__func__);
        return -ERROR;
    }
    *strength = si2196_devp->si_cmd_reply.tuner_status.rssi;
    return 0;
}
static int si2196_get_tuner_status(struct dvb_frontend *fe, tuner_status_t*tuner_status)
{
    struct tuner_status_s*tmp_status = NULL;    
    si2196_cmdreplyobj_t  *si_cmd_reply = &si2196_devp->si_cmd_reply;
    if(!tuner_status)
    {
        printk("[si2196..] %s null pointer error.\n",__func__);
        return -ERROR;
    }    
    //intack = 1 (SI2196_TUNER_STATUS_CMD_INTACK_CLEAR)
    if(si2196_tuner_status(&si2196_devp->tuner_client, 1, si_cmd_reply))
    {
        printk("[si2196..] %s si2196 get tuner status error.\n",__func__);
        return -ERROR;
    }
    tmp_status = (struct tuner_status_s *)tuner_status;
    tmp_status->frequency = si_cmd_reply->tuner_status.freq;
    tmp_status->mode        = si_cmd_reply->tuner_status.mode;
    tmp_status->rssi           = si_cmd_reply->tuner_status.rssi;
    siprintk("[si2196..]%s rssi %d.threshold <%d.%d>.\n",__func__,si_cmd_reply->tuner_status.rssi,
                    si_cmd_reply->tuner_status.rssil,si_cmd_reply->tuner_status.rssih);
    if(si_cmd_reply->tuner_status.rssih ||si_cmd_reply->tuner_status.rssil)
        tmp_status->tuner_locked = 0;
    else
        tmp_status->tuner_locked = 1;
    return 0;

}
static int  si2196_tuner_fine_tune(struct dvb_frontend *fe, int offset_500hz)
{
    si2196_devp->si_cmd.fine_tune.offset_500hz = offset_500hz;
    siprintk("[si2196..]%s fine offset %d.\n",__func__,offset_500hz);
    if(si2196_sendcommand(&si2196_devp->tuner_client, SI2196_FINE_TUNE_CMD, &si2196_devp->si_cmd,&si2196_devp->si_cmd_reply)!=0)
    {
        pr_info("[si2196..]%s: si2196 fine tune error.\n",__func__);
        return -ERROR;
    }
    return 0;
}
/*static void si2196_set_mode(void)
{
    unsigned short mode = SI2196_DTV_MODE_PROP_MODULATION_MASK;
    switch(si2196_devp->parm.mode)
    {
        case AM_FE_QPSK:
            mode = SI2196_DTV_MODE_PROP_MODULATION_ISDBT;
            break;
        case AM_FE_QAM:
            mode = SI2196_DTV_MODE_PROP_MODULATION_DVBC;
            break;
	case AM_FE_OFDM:
            mode = SI2196_DTV_MODE_PROP_MODULATION_DVBT;
            break;
        case AM_FE_ATSC:
            mode = SI2196_DTV_MODE_PROP_MODULATION_ATSC;
            break;
	case AM_FE_ANALOG:
            mode = SI2196_DTV_MODE_PROP_MODULATION_DEFAULT;
            break;
        default:
            printk("[si2196..] %s no mode to match %d.\n",__func__, mode);
            break;
    }
    //default bandwidth 8M 3->SI2196_DTV_MODE_PROP
    if(si2196_set_property(&si2196_devp->tuner_client, 0, 0x703, mode<<4|8, &si2196_devp->si_cmd_reply))
        printk("[si2196..] %s mode %d si2196_set_property error.",__func__, mode);
}*/
static void si2196_set_frequency(unsigned int freq)
{
    int ret = 0;
    /*tune the tuner*/
    ret = si2196_tune(&si2196_devp->tuner_client, 1, freq, &si2196_devp->si_cmd_reply);
    if(ret)
        printk("[si2196..]%s: set frequency error.\n", __func__);
}
static void si2196_set_soundsys(void)
{
    unsigned char ret = 0;
    //SI2196_SD_SOUND_SYSTEM_PROP ->0xd04
    ret = si2196_set_property(&si2196_devp->tuner_client,0, 0xd04, si2196_devp->parm.soundsys, &si2196_devp->si_cmd_reply);
    if(ret)
        printk("[si2196..]%s: set sound system error.\n", __func__);
}
static void si2196_set_audmode(void)
{
    unsigned char ret = 0;
    //SI2196_SD_SOUND_MODE_PROP ->0xd05
    ret = si2196_set_property(&si2196_devp->tuner_client,0, 0xd05, si2196_devp->sparm.sound_mode, &si2196_devp->si_cmd_reply);
    if(ret)
        printk("[si2196..]%s: set sound mode error.\n", __func__);
}
static void si2196_set_lang(void)
{
    unsigned char ret = 0;
    //SI2196_SD_LANG_SELECT_PROP ->0xd07
    ret = si2196_set_property(&si2196_devp->tuner_client,0, 0xd07, si2196_devp->sparm.lang, &si2196_devp->si_cmd_reply);
    if(ret)
        printk("[si2196..]%s: set sound lang error.\n", __func__);
}
static void si2196_set_std(void)
{
    v4l2_std_id ptstd = si2196_devp->parm.std;
    unsigned char color_mode=0, video_sys=0, ret=0;
    /*set color standard of atv*/
   if (ptstd & (V4L2_COLOR_STD_PAL | V4L2_COLOR_STD_NTSC))
        color_mode = SI2196_ATV_VIDEO_MODE_PROP_COLOR_PAL_NTSC;
    else if (ptstd & (V4L2_COLOR_STD_SECAM))
        color_mode = SI2196_ATV_VIDEO_MODE_PROP_COLOR_SECAM;

    /* set audio standard of tuner*/
    if (ptstd & V4L2_STD_B)
    {
        video_sys = SI2196_ATV_VIDEO_MODE_PROP_VIDEO_SYS_B;
        si2196_devp->sparm.fre_offset = 2250000;
    }
    else if (ptstd & V4L2_STD_GH)
    {
        video_sys = SI2196_ATV_VIDEO_MODE_PROP_VIDEO_SYS_GH;
        si2196_devp->sparm.fre_offset = 2250000;
    }
    else if ((ptstd & V4L2_STD_NTSC) || ((ptstd & V4L2_STD_PAL_M)))
    {
        video_sys = SI2196_ATV_VIDEO_MODE_PROP_VIDEO_SYS_M;
        si2196_devp->sparm.fre_offset = 1750000;
    }
    else if (ptstd & (V4L2_STD_PAL_N | V4L2_STD_PAL_Nc))
    {
        video_sys = SI2196_ATV_VIDEO_MODE_PROP_VIDEO_SYS_N;
        si2196_devp->sparm.fre_offset = 1750000;
    }
    else if (ptstd & V4L2_STD_PAL_I)
    {
        video_sys = SI2196_ATV_VIDEO_MODE_PROP_VIDEO_SYS_I;
        si2196_devp->sparm.fre_offset = 2750000;
    }
    else if (ptstd & V4L2_STD_DK)
    {
        video_sys = SI2196_ATV_VIDEO_MODE_PROP_VIDEO_SYS_DK;
        si2196_devp->sparm.fre_offset = 2750000;
    }
    else if (ptstd & V4L2_STD_SECAM_L)
    {
        video_sys = SI2196_ATV_VIDEO_MODE_PROP_VIDEO_SYS_L;
        si2196_devp->sparm.fre_offset = 2750000;
    }
    else if (ptstd & V4L2_STD_SECAM_LC)
    {
        video_sys = SI2196_ATV_VIDEO_MODE_PROP_VIDEO_SYS_LP;
        si2196_devp->sparm.fre_offset = 2750000;
    }
    
    ret = si2196_set_property(&si2196_devp->tuner_client, 0, 0x604, color_mode<<4|video_sys, &si2196_devp->si_cmd_reply);
    if(ret)
        printk("[si2196..]%s:set std %ul error\n",__func__, (uint)ptstd);
    siprintk("[si2196..]%s: color %s,video_mode.video_sys:%s. fre_offset is %d.\n", __func__, v4l2_std_to_str(si2196_devp->parm.std&0xffffff),
                v4l2_std_to_str(si2196_devp->parm.std&0xff000000) ,si2196_devp->sparm.fre_offset);
    if(si2196_atv_restart(&si2196_devp->tuner_client, 0,  &si2196_devp->si_cmd_reply))
        printk("[si2196..]%s: atv restart error.\n", __func__);
}

static int si2196_set_params(struct dvb_frontend *fe, struct dvb_frontend_parameters *parm)
{
	int strength;
   	if(FE_ANALOG==fe->ops.info.type){
    		if(si2196_devp->parm.soundsys  != parm->u.analog.soundsys)
    		{
        		if(parm->u.analog.soundsys == V4L2_TUNER_SYS_NULL)
            			//SI2196_SD_SOUND_SYSTEM_PROP_SYSTEM_AUTODETECT = 15
           	 		si2196_devp->parm.soundsys = 15;
        		else
            			si2196_devp->parm.soundsys  = parm->u.analog.soundsys;
        		siprintk("[si2196l..]%s set sound sys %s.\n",__func__,soundsys_to_str(si2196_devp->parm.soundsys));
        		si2196_set_soundsys();
    		}
    		else if(si2196_devp->parm.std  != parm->u.analog.std)
    		{
        		si2196_devp->parm.std  = parm->u.analog.std;
        		si2196_set_std();
    		}
    		else if(si2196_devp->parm.frequency  != parm->frequency)
    		{
        		si2196_devp->parm.frequency  = parm->frequency;
        		si2196_set_frequency(si2196_devp->parm.frequency + si2196_devp->sparm.fre_offset);
    		}
    		else if(si2196_devp->parm.audmode != parm->u.analog.audmode)
    		{
        		si2196_devp->parm.audmode = parm->u.analog.audmode;
       	 		switch(si2196_devp->parm.audmode)
        		{
            		case V4L2_TUNER_AUDMODE_NULL:
                		//SI2196_SD_SOUND_MODE_PROP_MODE_AUTODETECT = 0
                		si2196_devp->sparm.sound_mode = 0;
                		break;
            		case V4L2_TUNER_MODE_MONO:
                		//SI2196_SD_SOUND_MODE_PROP_MODE_MONO = 1
                		si2196_devp->sparm.sound_mode = 1;
                		break;
           		case V4L2_TUNER_MODE_STEREO:
               			 //SI2196_SD_SOUND_MODE_PROP_MODE_STREO = 3
                		si2196_devp->sparm.sound_mode = 3;
                		break;
            		case V4L2_TUNER_MODE_LANG2:
            		case V4L2_TUNER_MODE_SAP:
                		//SI2196_SD_LANG_SELECT_PROP_LANG_LANG_B
               			 si2196_devp->sparm.lang = 1;
                		break;
            		case V4L2_TUNER_MODE_LANG1:
               		 	//SI2196_SD_LANG_SELECT_PROP_LANG_LANG_A
                		si2196_devp->sparm.lang = 0;
                		break;
           		case V4L2_TUNER_MODE_LANG1_LANG2:
                		//SI2196_SD_SOUND_MODE_PROP_MODE_DUAL_MONO = 2
                		si2196_devp->sparm.sound_mode = 2;
                		//SI2196_SD_LANG_SELECT_PROP_LANG_DUAL_MONO = 2
                		si2196_devp->sparm.lang = 2;
                		break;
            		default:
                		break;
        		}
        		siprintk("[si2196..]%s set audmode %s.\n",__func__,audmode_to_str(si2196_devp->parm.audmode));
       	 		si2196_set_lang();
        		si2196_set_audmode();
    		}
 			 /*  else if(si2196_devp->parm.std  != parm->u.analog.std)
   			 {
      			  si2196_devp->parm.std  = parm->u.analog.std;
       		 	si2196_set_std();
   			 }*/
    		else
    		{
        		printk("[si2196..] %s no case catch error.\n",__func__);    
    		} 
   	}
   	else{
   		if(FE_QAM==fe->ops.info.type){
			if(QAM_128==parm->u.qam.modulation){
				si2196_devp->si_prop.dtv_lif_out.amp = 22;
				if(si2196_set_property(&si2196_devp->tuner_client, 0, 0x707, 22, &si2196_devp->si_cmd_reply))
					printk("[si2196..]%s set dtv lif out amp 22 error.\n",__func__);
			}
			else{
				si2196_devp->si_prop.dtv_lif_out.amp = 25;
				if(si2196_set_property(&si2196_devp->tuner_client, 0, 0x707, 25, &si2196_devp->si_cmd_reply))
					printk("[si2196...]%s set dtv lif out amp 25 error.\n",__func__);
			}
		}
		si2196_devp->parm.frequency = parm->frequency;
		si2196_set_frequency(si2196_devp->parm.frequency);
		printk("[si2196..]%s set frequency %u.\n",__func__,si2196_devp->parm.frequency);
		msleep(500);
		strength=100;
		if(si2196_tuner_status(&si2196_devp->tuner_client,
							SI2196_ATV_STATUS_CMD_INTACK_OK,
							&si2196_devp->si_cmd_reply)!=0){
       		 	printk("[si2196..]%s:get si2196 tuner status error!!!\n",__func__);
        		return -ERROR;
    		}
    		else
        		strength = si2196_devp->si_cmd_reply.tuner_status.rssi-256;
		printk("strength is %d\n",strength);
 	}
       return 0;
}


static void get_mono_sound_mode(unsigned int *mode)
{
    if(si2196_atv_status(&si2196_devp->tuner_client, 0, &si2196_devp->si_cmd_reply)!=0)
    {    
            printk("[si2196..]%s:get si2196 atv status error.\n",__func__);    
            return;
    }
    switch(si2196_devp->si_cmd_reply.atv_status.video_sys)
    {
        case 0:
            *mode = V4L2_STD_B;
            break;
        case 1:
            *mode = V4L2_STD_DK;
            break;
        case 2:
            *mode = V4L2_STD_MN;
            break;
        case 3:
            *mode = V4L2_STD_DK;
            break;
        case 4:
            *mode = V4L2_STD_PAL_I;
            break;
        case 5:
            *mode = V4L2_STD_DK;
            break;
        case 6:
            *mode = V4L2_STD_SECAM_L;
            break;
        case 7:
            *mode = V4L2_STD_UNKNOWN;
	    break;
        default:
            break;        
    }
    siprintk("[si2196..]%s result is %s.\n",__func__,v4l2_std_to_str(*mode));
}
static int si2196_set_config(struct dvb_frontend *fe, void *arg)
{
   tuner_param_t *param = (tuner_param_t*)arg;
	if(!arg)
	{
		printk("[si2196..]%s null pointer error.\n",__func__);
		return -ERROR;
	}
	switch(param->cmd)
	{
		case TUNER_CMD_AUDIO_MUTE:
			//SI2196_SD_PORT_VOLUME_MASTER_PROP = 0x0d11
			if(si2196_set_property(&si2196_devp->tuner_client, 0,0xd11, -231, &si2196_devp->si_cmd_reply))
			{
				printk("[si2196..]%s mute tuner error.\n",__func__);
				return -ERROR;
			}
			break;
		case TUNER_CMD_AUDIO_ON:
			if(si2196_set_property(&si2196_devp->tuner_client, 0,0xd11, si2196_devp->sparm.m_volume.volume, &si2196_devp->si_cmd_reply))
			{
				printk("[si2196..]%s set tuner volume error.\n",__func__);
				return -ERROR;
			}
			break;
		case TUNER_CMD_TUNER_POWER_DOWN:
			if(si2196_power_down(&si2196_devp->tuner_client, &si2196_devp->si_cmd_reply))
			{
				printk("[si2196..]%s power down tuner error.\n",__func__);
				return -ERROR;
			}
			siprintk("[si2196..]%s power down tuner TUNER_CMD_TUNER_POWER_DOWN.\n",__func__);
			break;
		case TUNER_CMD_TUNER_POWER_ON:			
			if(si2196_init(&si2196_devp->tuner_client, &si2196_devp->si_cmd_reply))
			{
				printk("[si2196..]%s: si2196 initializate error.\n",__func__);
				return -ERROR;
			}
			if(si2196_configure(&si2196_devp->tuner_client,&si2196_devp->si_cmd_reply))
        		    printk("[si2196..]%s: si2196 configure atv&dtv error.\n",__func__);
			break;
		case TUNER_CMD_SET_VOLUME:
			if(param->parm > SI2196_SD_PORT_VOLUME_MASTER_PROP_VOLUME_VOLUME_MIN)
				param->parm = SI2196_SD_PORT_VOLUME_MASTER_PROP_VOLUME_VOLUME_MAX;
			si2196_devp->sparm.m_volume.volume = param->parm;
			if(si2196_set_property(&si2196_devp->tuner_client, 0,0xd11, si2196_devp->sparm.m_volume.volume, &si2196_devp->si_cmd_reply))
				printk("[si2196..]%s set tuner volume error.\n",__func__);
			break;
                case TUNER_CMD_GET_MONO_MODE:
                        get_mono_sound_mode(&param->parm);
                        break;
		default:
			break;
    }
    return 0;
}
static void si2196_analog_get_status(struct dvb_frontend *fe, void *status);

static int si2196_tuner_get_ops(struct aml_fe_dev *fe, int mode, void *ops)
{
    struct dvb_tuner_ops *si2196_tuner_ops = (struct dvb_tuner_ops *)ops;
    if(!ops)
    {
        printk("[si2196..]%s null pointer error.\n",__func__);
        return -1;
    }
    si2196_tuner_ops->info.frequency_min    = SI2196_TUNER_TUNE_FREQ_CMD_FREQ_MIN;    
    si2196_tuner_ops->info.frequency_max    = SI2196_TUNER_TUNE_FREQ_CMD_FREQ_MAX;
    switch(mode)
    {
        case AM_FE_QPSK:
			//SI2196_DTV_MODE_PROP_MODULATION_ISDBT = 4
            si2196_devp->sparm.dtv_mode.modulation = 4;
            break;
        case AM_FE_QAM:
			//SI2196_DTV_MODE_PROP_MODULATION_DVBC = 3
            si2196_devp->sparm.dtv_mode.modulation = 3;
            break;
		case AM_FE_OFDM:
			//SI2196_DTV_MODE_PROP_MODULATION_DVBT = 2
            si2196_devp->sparm.dtv_mode.modulation = 2;
            break;
        case AM_FE_ATSC:
			//SI2196_DTV_MODE_PROP_MODULATION_ATSC = 0
            si2196_devp->sparm.dtv_mode.modulation = 0;
            break;
	case AM_FE_ANALOG:
            break;
        default:
            printk("[si2196..] %s no mode to match %d.\n",__func__, mode);
            break;
    }
    
    if(mode == AM_FE_ANALOG)
    {
        /*silicon tuner fine tune*/
        si2196_tuner_ops->fine_tune = si2196_tuner_fine_tune;
        //the box for expand
        si2196_tuner_ops->set_config = si2196_set_config;
        si2196_devp->parm.mode = SI2196_TUNER_TUNE_FREQ_CMD_MODE_ATV;
    }
    else
    {   if(si2196_set_property(&si2196_devp->tuner_client, 0, 0x703, 
    						si2196_devp->sparm.dtv_mode.modulation<<4|8, 
    						&si2196_devp->si_cmd_reply))
        	printk("[si2196..] %s mode %d si2196_set_property error.",__func__, mode);   
        si2196_devp->parm.mode = SI2196_TUNER_TUNE_FREQ_CMD_MODE_DTV;
    }
    si2196_tuner_ops->info.frequency_step = 0x0;
    si2196_tuner_ops->get_tuner_status = si2196_get_tuner_status;
    si2196_tuner_ops->get_rf_strength = si2196_get_rf_strength;
    si2196_tuner_ops->get_status = si2196_analog_get_status;
    /*set analog parameters such as frequency,std,mode,sound sys,sound mode*/
    si2196_tuner_ops->set_params = si2196_set_params;

}
static int si2196_tuner_fe_init(struct aml_fe_dev *fe)
{
    if(!fe)
    {
        printk("[si2196..]%s null pointer error.\n",__func__);
        return -ERROR;
    }
    si2196_devp->tuner_client.adapter =  fe->i2c_adap;
    si2196_devp->tuner_client.addr = fe->i2c_addr;
    if(!sprintf(si2196_devp->tuner_client.name, SI2196_TUNER_I2C_NAME))
    {
        printk("[si2196..]%s sprintf name error.\n",__func__);
    }
   si2196_devp->si_cmd_reply.reply = kmalloc(sizeof(si2196_common_reply_struct),GFP_KERNEL);
    /*reset silicon 2196 tuner*/
    gpio_direction_output(fe->reset_gpio, fe->reset_value);
    udelay(400);
    gpio_direction_output(fe->reset_gpio, !fe->reset_value);
    return 0;
}


static int si2196_enter_mode(struct aml_fe *fe, int mode)
{
       
       if(si2196_init(&si2196_devp->tuner_client, &si2196_devp->si_cmd_reply))
    {
        printk("[si2196..]%s si2196 initializate error.\n",__func__);
        return -ERROR;
    }
    if(si2196_configure(&si2196_devp->tuner_client,  &si2196_devp->si_cmd_reply))
    {
        printk("[si2196..]%s si2196 config error.\n",__func__);
        return -ERROR;
    }
    //duration = 20(SI2196_SD_ADAC_POWER_UP_CMD_DURATION_MIN)
    if(si2196_adac_power_up(&si2196_devp->tuner_client, 20, &si2196_devp->si_cmd_reply))
    {
        printk("[si2196..]%s si2196 adac power up error.\n",__func__);
        return -ERROR;
    }
        return 0;
}
static int si2196_leave_mode(struct aml_fe *fe, int mode)
{
        int err_code;
        err_code = si2196_power_down(&si2196_devp->tuner_client, &si2196_devp->si_cmd_reply);
        if(err_code)
        {
                printk("[si2196..]%s power down si2196 error.\n",__func__);
                return err_code;
        }
	siprintk("[si2196..]%s power down tuner TUNER_CMD_TUNER_POWER_DOWN.\n",__func__);
        return 0;
}
static struct aml_fe_drv si2196_tuner_drv = {
        .name = TUNER_DEVICE_NAME,    
        .id       = AM_TUNER_SI2196,
	.capability = AM_FE_ANALOG|AM_FE_QPSK|AM_FE_QAM|AM_FE_ATSC|AM_FE_OFDM,
        .get_ops = si2196_tuner_get_ops,
        .init       = si2196_tuner_fe_init,
        .enter_mode=si2196_enter_mode,
        .leave_mode=si2196_leave_mode
};
/*the analog driver*/
static int si2196_analog_get_afc(struct dvb_frontend *fe)
{    
    if(si2196_atv_status(&si2196_devp->tuner_client, 0, &si2196_devp->si_cmd_reply)!=0)
    {    
            printk("[si2196..]%s:get si2196 atv status error.\n",__func__);    
            return -ERROR;
    }
    if(si2196_debug)
        printk("[si2196..] %s afc: %d \n", __func__, si2196_devp->si_cmd_reply.atv_status.afc_freq);
    return si2196_devp->si_cmd_reply.atv_status.afc_freq;
}

static int si2196_analog_get_snr(struct dvb_frontend *fe)
{

    //0=SI2176_ATV_STATUS_CMD_INTACK_OK
    if(si2196_atv_status(&si2196_devp->tuner_client, 0, &si2196_devp->si_cmd_reply)!=0)
    {    
        printk("[si2196..]%s:get si2196 atv status error.\n",__func__);    
        return -ERROR;
    }
    if(si2196_debug)
        printk("[si2196..] %s snr: %d \n", __func__, si2196_devp->si_cmd_reply.atv_status.video_snr);
    return si2196_devp->si_cmd_reply.atv_status.video_snr;
}
static void si2196_analog_get_status(struct dvb_frontend *dvb_fe, void *stat)
{
	fe_status_t *status=(fe_status_t *)stat;
    if(!status)
    {
        printk("[si2196..]%s: null pointer error.\n",__func__);
        return;
    }
    //0=SI2176_ATV_STATUS_CMD_INTACK_OK
    if(si2196_atv_status(&si2196_devp->tuner_client, 0, &si2196_devp->si_cmd_reply)!=0)
    {    
        printk("[si2196..]%s:get si2196 atv status error.\n",__func__);    
        return;
    }
    else
    {
        if(si2196_devp->si_cmd_reply.atv_status.chl)
            *status = FE_HAS_LOCK;
        else
            *status = FE_TIMEDOUT;
    }
}


static int si2196_analog_get_atv_status(struct dvb_frontend *fe, atv_status_t *atv_status)
{
    struct atv_status_s *atv_sts = atv_status;
    if(!atv_status)
    {
        printk("[si2196..]%s: null pointer error.\n",__func__);
        return -ERROR;
    }
    if(si2196_atv_status(&si2196_devp->tuner_client, SI2196_ATV_STATUS_CMD_INTACK_OK, &si2196_devp->si_cmd_reply)!=0)
    {    
        pr_info("[si2196..]%s:get si2196 atv status error.\n",__func__);    
        return -ERROR;
    }
    else
    {
        atv_sts->afc = si2196_devp->si_cmd_reply.atv_status.afc_freq;
        atv_sts->snr =si2196_devp->si_cmd_reply.atv_status.video_snr;
        atv_sts->atv_lock = si2196_devp->si_cmd_reply.atv_status.chl;
        atv_sts->audmode = si2196_devp->si_cmd_reply.atv_status.audio_sys;
        atv_sts->std &= 0;
        if(si2196_devp->si_cmd_reply.atv_status.color)
            atv_sts->std |= V4L2_COLOR_STD_SECAM;
        else
            atv_sts->std = V4L2_COLOR_STD_PAL;
        switch(si2196_devp->si_cmd_reply.atv_status.video_sys)
        {
            case 0:
                atv_sts->std |=V4L2_STD_B;
                break;
            case 1:
               atv_sts->std |=V4L2_STD_GH; 
               break;
            case 2:
               atv_sts->std |=V4L2_STD_PAL_M; 
               break;
            case 3:
               atv_sts->std |=V4L2_STD_PAL_N; 
               break;
            case 4:
               atv_sts->std |=V4L2_STD_PAL_I; 
               break;
            case 5:
               atv_sts->std |=V4L2_STD_DK; 
               break;
            case 6:
               atv_sts->std |=V4L2_STD_SECAM_L; 
               break;
            case 7:
               atv_sts->std |=V4L2_STD_SECAM_LC; 
               break;
            default:
                break;
        }
        siprintk("[si2196..]%s chl is %d, afc %d, snr %d,color %s audmode %s.\n",
                    __func__,atv_sts->atv_lock,atv_sts->afc,atv_sts->snr,
                    v4l2_std_to_str(atv_sts->std&0xff000000),v4l2_std_to_str(atv_sts->std&0xffffff));
            
    }
    return 0;
}

static int si2196_analog_get_sd_status(struct dvb_frontend *fe, sound_status_t *sd_status)
{
    unsigned char ret = 0;    
    sound_status_t *tmp_status = sd_status;
    if(!tmp_status)
    {
        printk("[si2196..] %s null pointer error.\n",__func__);
        return -ERROR;
    }
    ret = si2196_sd_status(&si2196_devp->tuner_client, &si2196_devp->si_cmd_reply, SI2196_SD_STATUS_CMD_INTACK_OK);
    if(ret)
    {
        printk("[si2196..] %s si2196 get sd status error.\n",__func__);
        return -ERROR;
    }
    tmp_status->sound_sys = si2196_devp->si_cmd_reply.sd_status.sound_system_detected;
    tmp_status->sound_mode = si2196_devp->si_cmd_reply.sd_status.sound_mode_detected;
    switch(tmp_status->sound_mode)
    {
        //SI2196_SD_STATUS_RESPONSE_SOUND_MODE_DETECTED_MONO = 1
        case 1:
            tmp_status->sound_mode = V4L2_TUNER_MODE_MONO;
            break;
        //SI2196_SD_STATUS_RESPONSE_SOUND_MODE_DETECTED_DUAL_MONO = 2
        case 2:            
            tmp_status->sound_mode = V4L2_TUNER_MODE_LANG1_LANG2;
            break;
        //SI2196_SD_STATUS_RESPONSE_SOUND_MODE_DETECTED_STEREO = 3
        case 3:
            tmp_status->sound_mode = V4L2_TUNER_MODE_STEREO;
            break;
        default:
            tmp_status->sound_mode = V4L2_TUNER_AUDMODE_NULL;
            break;
    }
    if(si2196_devp->si_cmd_reply.sd_status.sap_detected)
            tmp_status->sound_mode = V4L2_TUNER_MODE_SAP;
    siprintk("[si2196..]%s sound sys %s. sound mode %s.\n",__func__,soundsys_to_str(tmp_status->sound_sys), 
                audmode_to_str(tmp_status->sound_mode));
    return 0;
    
}
static int si2196_analog_get_ops(struct aml_fe_dev *dev, int mode, void *ops)
{    
    struct analog_demod_ops *si2196_analog_ops = (struct analog_demod_ops*)ops;
    if(!ops)
    {
        printk("[si2196..]%s null pointer error.\n",__func__);
        return -ERROR;
    }
    si2196_analog_ops->get_afc               = si2196_analog_get_afc;
    si2196_analog_ops->get_snr               = si2196_analog_get_snr;
    si2196_analog_ops->get_status      =  si2196_analog_get_status;
    si2196_analog_ops->get_atv_status   = si2196_analog_get_atv_status;
    si2196_analog_ops->get_sd_status    = si2196_analog_get_sd_status;
    return 0;
}
static struct aml_fe_drv si2196_analog_drv = {
    .name    = "si2196_atv_demod",
    .capability = AM_FE_ANALOG,
    .id      = AM_ATV_DEMOD_SI2196 ,
    .get_ops = si2196_analog_get_ops
};

static int __init si2196_tuner_init(void)
{
    int ret = 0;    
    si2196_devp = kmalloc(sizeof(si2196_device_t), GFP_KERNEL);
    if(!si2196_devp)
    {
        pr_info("[si2196..] %s:allocate memory error.\n",__func__);
        return -ENOMEM;
    }
    si2196_devp->clsp = class_create(THIS_MODULE,TUNER_DEVICE_NAME);
    if(!si2196_devp->clsp)
    	{
        pr_info("[%s]:create class error.",__func__);
    	  kfree(si2196_devp);
        si2196_devp = NULL;
        return PTR_ERR(si2196_devp->clsp);
	}
    class_create_file(si2196_devp->clsp,&class_attr_si2196_debug);
    aml_register_fe_drv(AM_DEV_TUNER,   &si2196_tuner_drv);
    aml_register_fe_drv(AM_DEV_ATV_DEMOD, &si2196_analog_drv);
    printk("[si2196..]%s.\n",__func__);
    return ret;
}

static void __exit si2196_tuner_exit(void)
{
    if(SI2196_DEBUG)
        pr_info( "%s . \n", __FUNCTION__ );
    class_destroy(si2196_devp->clsp);
    aml_unregister_fe_drv(AM_DEV_TUNER,   &si2196_tuner_drv);
    aml_unregister_fe_drv(AM_DEV_ATV_DEMOD, &si2196_analog_drv);
    kfree(si2196_devp);
}

MODULE_AUTHOR("kele bai <kele.bai@amlogic.com>");
MODULE_DESCRIPTION("Silicon si2196 tuner i2c device driver");
MODULE_LICENSE("GPL");
fs_initcall(si2196_tuner_init);
module_exit(si2196_tuner_exit);

