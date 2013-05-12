/*                                                                                                                                                                                                                                                   
* Author: kele bai <kele.bai@amlogic.com>                                                                                     
*                                                                                                                                  
* Copyright (C) 2010 Amlogic Inc.                                                                                     
 *                                                                                                                                
* This program is free software; you can redistribute it and/or modify                                                    
* it under the terms of the GNU General Public License version 2 as                                                               
 * published by the Free Software Foundation.                                                                                    
*/  

#include <linux/module.h>
#include <linux/err.h>
#include <linux/dvb/frontend.h>
#include "ctc703_func.h"
#include<mach/gpio_data.h>
#include<linux/delay.h>
#include<mach/gpio.h>

#define MODEULE_NAME    "ctc703"
#define CTC703_TUNER_I2C_NAME           "ctc703_tuner_i2c"
#define ERROR                  1
static bool ctc703_debug = false;
module_param(ctc703_debug,bool,0644);
MODULE_PARM_DESC(ctc703_debug,"\n enble /disable the ctc703 debug information.\n");

#define ctprintk  if(ctc703_debug) printk
static struct ctc703_device_s  *ctc_devp;
static void ctc703_set_std(void);
static int ctc703_get_atv_status(struct dvb_frontend *dvb_fe, atv_status_t *atv_status);
static int ctc703_get_afc(struct dvb_frontend *fe);
static int ctc703_get_snr(struct dvb_frontend *fe);

static void sound_store(const char *buff, v4l2_std_id *std)
{
        if(!strncmp(buff,"dk",2))
                *std |= V4L2_STD_PAL_DK;
        else if(!strncmp(buff,"bg",2))
                *std |= V4L2_STD_PAL_BG;
        else if(!strncmp(buff,"i",1))
                *std |= V4L2_STD_PAL_I;
        else if(!strncmp(buff,"n",1))
                *std |= V4L2_STD_PAL_N;
        else if(!strncmp(buff,"nm",2))
                *std |= V4L2_STD_NTSC_M;
        else if(!strncmp(buff,"pm",2))
                *std |= V4L2_STD_PAL_M;
        else if(!strncmp(buff,"l",1))
                *std |= V4L2_STD_SECAM_L;
        else if(!strncmp(buff,"lc",2))
                *std |= V4L2_STD_SECAM_LC;
        else
                pr_info("invaild command.\n");
}

static ssize_t ctc703_show(struct class *cls,struct class_attribute * attr,char *buff)
{
    return 0;
}
static ssize_t ctc703_store(struct class *cls,struct class_attribute *attr,char *buf,size_t count)
{
    char *parm[4];
    char *buff,*buf_orig,*ps;
    unsigned int err_code=0,n=0;
    v4l2_std_id  std;
    atv_status_t  atv_status={0};
	//unsigned short lock = 0;
    int afc_fre = 0;
    unsigned short int  quality=0;
    unsigned short snr = 0;
    printk("buf=%s\n",buf);
    buf_orig = kstrdup(buf, GFP_KERNEL);
    ps = buf_orig;
    printk("ps=%s\n",ps);
    while (1) {
               buff= strsep(&ps, " \n");
               printk("buff=%s\n",buff);
                if (buff == NULL)
                        break;
                if (*buff == '\0')
                        continue;
                parm[n] = buff;
                printk("parm[%d]=%s\n",n,parm[n++]);
        }
  //  printk("parm[n]=%s\n",parm[n]);
    if(!strncmp(parm[0],"tune",strlen("tune")))
        {
            ctc_devp->parm.frequency = simple_strtol(parm[1], NULL, 10);
            printk("set the freq:%d\n",ctc_devp->parm.frequency );
            err_code = xc_set_rf_frequency(ctc_devp->parm.frequency);
            if(err_code)
                printk("[ctc703..]%s set frequency error %d.\n",__func__,err_code);

        }
     else if(!strncmp(parm[0],"std",strlen("std")))
        {
                if(!strncmp(parm[1],"pal",3))
                {
                        std= V4L2_COLOR_STD_PAL;
                        sound_store(parm[2],&std);
                }
                else if(!strncmp(parm[1],"ntsc",4))
                {
                        std= V4L2_COLOR_STD_NTSC;
                        sound_store(parm[2],&std);
                }
                else if(!strncmp(parm[1],"secam",5))
                {
                        std= V4L2_COLOR_STD_SECAM;
                        sound_store(parm[2],&std);
                }
                ctc_devp->parm.std  =std;
                ctc703_set_std();                
                printk("[ctc703..]%s set std color %s, audio type %s.\n",__func__,\
                                v4l2_std_to_str(0xff000000&ctc_devp->parm.std), v4l2_std_to_str(0xffffff&ctc_devp->parm.std));
        }
    else if(!strncmp(parm[0],"atv_status",strlen("atv_status")))
    {	
   	if (WaitForLock()== 1)
	{
		  atv_status.atv_lock= 1;
	}
	else 
		atv_status.atv_lock= 0;  
    	xc_get_frequency_error(&afc_fre); 
        atv_status.afc = afc_fre;
    	if(xc_read_reg(XREG_SNR,&snr))
    	{
        	printk("[ctc703..]%s get snr error.\n",__func__);
        	return 0;
    	}
        atv_status.snr =snr;
        xc_get_quality(&quality);
 //	atv_status->audmode = si2176_devp->si_cmd_reply.atv_status.audio_demod_mode;
        atv_status.std = ctc_devp->parm.std;
        printk("[si2176..]%s:tune %d afc %d_hz,snr %d,atv lock %d ,quality %d ,std color %s audio %s.\n",__func__,ctc_devp->parm.frequency,atv_status.afc, atv_status.snr,
                        atv_status.atv_lock,quality, v4l2_std_to_str(atv_status.std&0xff000000),v4l2_std_to_str(atv_status.std&0xffffff));

	 	}
	 else if(!strncmp(parm[0],"set_channel",strlen("set_channel")))
	 	{
	 	ctc_devp->parm.frequency = simple_strtol(parm[1], NULL, 10);
	 	printk("lock=%d\n",xc_scan_channel(ctc_devp->parm.frequency));
	 	}
        else
                printk("invalid command\n");
        return count;
}
static CLASS_ATTR(ctc703_debug, 0644, ctc703_show, ctc703_store);


static int ctc703_tuner_init_fe(struct aml_fe_dev *fe)
{
    if(!fe)
    {
        printk("[ctc703..]%s null pointer error.\n",__func__);
        return -ERROR;
    }
    ctc_devp->tuner_client.adapter = fe->i2c_adap;
    ctc_devp->tuner_client.addr = fe->i2c_addr;
    printk("i2c_addr=%d\n",fe->i2c_addr);
    xc_set_i2c_client(&ctc_devp->tuner_client);
    if(!sprintf(ctc_devp->tuner_client.name,CTC703_TUNER_I2C_NAME))
    {
        printk("[ctc703..]%s sprintf name error.\n",__func__);
    }
    //reset
    ctc_devp->cpram.reset_gpio = fe->reset_gpio;
    ctc_devp->cpram.reset_value = fe->reset_value;
   // xc_reset();
        
    printk("ctc703 init  is ok!\n");
    return 0;
}
int xc_reset()
{
   
       gpio_out(ctc_devp->cpram.reset_gpio,0);
       mdelay(100);
       gpio_out(ctc_devp->cpram.reset_gpio,1);
       printk("[%s]tuner reset:reset_gpio=[%d] \n",__func__,ctc_devp->cpram.reset_gpio);
    return XC_RESULT_SUCCESS;
}
static int ctc703_enter_mode(struct aml_fe *fe, int mode)
{
    //-/+ 1.25m
    unsigned short afc_range[]={0x50,0xfb0};
    xc_initialize();
    //set rf mode to off-air
    xc_set_rf_mode(0);
    xc_write_seek_frequency(2,afc_range);
    return 0;
}
static int ctc703_leave_mode(struct aml_fe *fe, int mode)
{
   if(xc_shutdown())
   {
        printk("[ctc703..]%s error.\n",__func__);
        return -ERROR;
   }
   return 0;
    
}
static int ctc703_suspend(struct aml_fe_dev *dev)
{
	 if(xc_shutdown())
   {
        printk("[ctc703..]%s error.\n",__func__);
        return -ERROR;
   }
   return 0;
}

static int ctc703_resume(struct aml_fe_dev *dev)
{
	    //-/+ 1.25m
    unsigned short afc_range[]={0x50,0xfb0};
    if(0 != xc_initialize())
	{
		printk("[%s]:xc_initialize is err\n",__func__);	
	}
    //set rf mode to off-air
    xc_set_rf_mode(0);
    xc_write_seek_frequency(2,afc_range);
	printk("[%s]: ctc tuner resume is ok",__func__);
    return 0;
}

static void ctc703_get_demod_state(struct dvb_frontend *fe, void *state);
    
static void ctc703_set_frequency(void)
{    
    int err_code = 0;
    ctprintk("[ctc703..]%s set frequency %u.\n",__func__,ctc_devp->parm.frequency);
    err_code = xc_set_rf_frequency(ctc_devp->parm.frequency);
	msleep(200);
    if(err_code)
        printk("[ctc703..]%s set frequency error %d.\n",__func__,err_code);
}
static void ctc703_set_std(void)
{
    int return_code = 0;
    ctprintk("[ctc703..]%s set std color %s, audio mode %s.\n",__func__,v4l2_std_to_str(ctc_devp->parm.std&0xff000000),
                v4l2_std_to_str(ctc_devp->parm.std&0xffffff));
	//	on the basis of audio mode  ,func can analysis the video mode and audio mode
    return_code = xc_set_tv_standard(ctc_devp->parm.std&0xffffff);
	msleep(50);
    if(return_code)
        printk("[ctc703..]%s set std error.\n",__func__);
}
static int ctc703_set_param(struct dvb_frontend *fe,struct dvb_frontend_parameters *param)
{
    struct dvb_analog_parameters *analog_param;
    if(!param)
    {
        printk("[ctc703..]%s null pointer error.\n",__func__);
        return -ERROR;
    }
    analog_param = &param->u.analog;
     if(analog_param->std != ctc_devp->parm.std)
    {
        ctc_devp->parm.std = analog_param->std;
        ctc703_set_std();
    }
    if(param->frequency != ctc_devp->parm.frequency)
    {
        ctc_devp->parm.frequency = param->frequency;
        ctc703_set_frequency();
    }
 
    return 0;
}
static int ctc703_tuner_get_ops(struct aml_fe_dev *fe, int mode, void* ops)
{
    struct dvb_tuner_ops *ctc703_tuner_ops  = (struct dvb_tuner_ops*)ops;
    ctc703_tuner_ops->info.frequency_min  = 1000000;
    ctc703_tuner_ops->info.frequency_max = 1023000000;
    ctc703_tuner_ops->set_params             = ctc703_set_param; 
    ctc703_tuner_ops->get_status=   ctc703_get_demod_state;
    
    
    return 0;
}
struct aml_fe_drv ctc703_tuner_drv ={
    .name           = "ctc703_tuner",
    .id                 = AM_TUNER_CTC703,
    .capability     = AM_FE_ANALOG|AM_FE_QPSK|AM_FE_QAM|AM_FE_ATSC|AM_FE_OFDM,
    .init               = ctc703_tuner_init_fe,
    .enter_mode = ctc703_enter_mode,
    .leave_mode = ctc703_leave_mode,
    .get_ops       = ctc703_tuner_get_ops,
    .suspend	=	ctc703_suspend,
    .resume		=	ctc703_resume,
};
static int ctc703_get_afc(struct dvb_frontend *fe)
{
    int afc_fre = 0;
    xc_get_frequency_error(&afc_fre);
    return afc_fre/1000;
}
static int ctc703_get_snr(struct dvb_frontend *fe)
{
    unsigned short snr = 0;
    if(xc_read_reg(XREG_SNR,&snr))
    {
        printk("[ctc703..]%s get snr error.\n",__func__);
        return 0;
    }
    return snr;
}
static void ctc703_get_demod_state(struct dvb_frontend *fe, void *state)
{
   // unsigned short lock = 0;    
    fe_status_t *status = (fe_status_t*)state;
    if(!state)
    {
        printk("[ctc703..] %s null pointer error.\n",__func__);
        return;
    }
  /* if(xc_get_lock_status(&lock))
    {
        printk("[ctc703..] %s get lock error.\n",__func__);
        return;
    }
    switch(lock)
    {
        case 0:
            *status = FE_TIMEDOUT;
            break;
        case 1:
            *status = FE_HAS_LOCK;
            break;
        case 2:
            *status = FE_REINIT;
            break;
        default:
            break;
    }
    */

    if (WaitForLock()== 1)
    {
	*status = FE_HAS_LOCK;
    }
    else 
	*status = FE_TIMEDOUT; 

    
}
    

static int ctc703_get_atv_status(struct dvb_frontend *dvb_fe, atv_status_t *atv_status)
{
 //   unsigned int lock_status = 0;
        if(!atv_status)
        {
            	printk("[ctc703..]%s: null pointer error.\n",__func__);
            	return -ERROR;
        }
        if (WaitForLock()== 1)
	{
		atv_status->atv_lock= 1;
	}
	else 
		atv_status->atv_lock= 0;  
	//xc_get_lock_status(&lock_status);
       // atv_status->atv_lock = lock_status;
        atv_status->afc = ctc703_get_afc(dvb_fe);
        atv_status->snr =ctc703_get_snr(dvb_fe);      
   //    atv_status->audmode = si2176_devp->si_cmd_reply.atv_status.audio_demod_mode;
        atv_status->std &= ctc_devp->parm.std;
        ctprintk("[ctc..]%s afc %d_khz,snr %d,atv lock %d ,std color %s audio %s.\n",\
		__func__,atv_status->afc, atv_status->snr,atv_status->atv_lock, \
		v4l2_std_to_str(atv_status->std&0xff000000),v4l2_std_to_str(atv_status->std&0xffffff));
        return 0;
}

static int ctc703_analog_get_ops(struct aml_fe_dev *fe,int mode, void *ops)
{
    struct analog_demod_ops *ctc703_demod_ops = (struct analog_demod_ops *)ops;
    ctc703_demod_ops->get_afc         = ctc703_get_afc;
    ctc703_demod_ops->get_snr         = ctc703_get_snr;
    ctc703_demod_ops->get_status    = ctc703_get_demod_state;
    ctc703_demod_ops->get_atv_status = ctc703_get_atv_status;
    return 0;
}
struct aml_fe_drv ctc703_demod_drv ={
    .name           = "ctc703_atv_demod",
    .id                 = AM_ATV_DEMOD_CTC703,
    .capability     = AM_FE_ANALOG,
    .get_ops       = ctc703_analog_get_ops,
};
static int __init ctc703_module_init(void)
{
    int ret=0;
    ctc_devp = kmalloc(sizeof(ctc703_device_t), GFP_KERNEL);
    if(!ctc_devp)
    {
        printk("[ctc703..] %s, no memory to allocate.\n",__func__);
        return -ENOMEM;
    }
    ctc_devp->clsp = class_create(THIS_MODULE, MODEULE_NAME);
    if(!ctc_devp->clsp)
    {
        printk("[ctc703..] %s create class error.\n",__func__);
    }
    if((ret=class_create_file(ctc_devp->clsp,&class_attr_ctc703_debug)) < 0)
    {
            printk("%s: fail to create ctc_debug attribute file.\n", __func__);
    }
    //class_create_file(struct class * class,const struct class_attribute * attr);
    aml_register_fe_drv(AM_DEV_TUNER, &ctc703_tuner_drv);    
    aml_register_fe_drv(AM_DEV_ATV_DEMOD, &ctc703_demod_drv);
    printk("[%s]:ctc703 tuner module  init\n",__func__);
  
    return 0;
    
}
static void __exit ctc703_module_exit(void)
{
    class_destroy(ctc_devp->clsp);
    if(ctc_devp)
        kfree(ctc_devp);
    aml_unregister_fe_drv(AM_DEV_TUNER, &ctc703_tuner_drv);    
    aml_unregister_fe_drv(AM_DEV_ATV_DEMOD, &ctc703_demod_drv);
    
}
fs_initcall(ctc703_module_init);
module_exit(ctc703_module_exit);
