/*************************************************************
 * Amlogic 
 * vdac switch program
 *
 * Copyright (C) 2010 Amlogic, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the named License,
 * or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
 *
 * Author:   jets.yan@amlogic
 *		   
 *		   
 **************************************************************/
#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/io.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/ctype.h>
#include <linux/vout/vinfo.h>
#include <mach/vdac_switch.h>
#include  <linux/vout/vout_notify.h>
#include "tvmode.h"

#include <linux/amlog.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
static struct early_suspend early_suspend;
static int early_suspend_flag = 0;
#endif

typedef  struct {
	const vinfo_t *vinfo;
	char 	     name[20] ;
}disp_module_info_t ;


MODULE_AMLOG(0, 0xff, LOG_LEVEL_DESC, LOG_MASK_DESC);

static struct aml_vdac_switch_platform_data *vdac_switch_platdata = NULL;

#ifdef  CONFIG_PM
static int  meson_vdac_switch_suspend(struct platform_device *pdev, pm_message_t state);
static int  meson_vdac_switch_resume(struct platform_device *pdev);
#endif

static    disp_module_info_t    video_info;

static const vinfo_t tv_info[] = 
{
    { /* VMODE_480I */
		.name              = "480i",
		.mode              = VMODE_480I,
        .width             = 720,
        .height            = 480,
        .field_height      = 240,
        .aspect_ratio_num  = 4,
        .aspect_ratio_den  = 3,
        .sync_duration_num = 60,
        .sync_duration_den = 1,
    },
     { /* VMODE_480CVBS*/
		.name              = "480cvbs",
		.mode              = VMODE_480CVBS,
        .width             = 720,
        .height            = 480,
        .field_height      = 240,
        .aspect_ratio_num  = 4,
        .aspect_ratio_den  = 3,
        .sync_duration_num = 60,
        .sync_duration_den = 1,
    },
    { /* VMODE_480P */
		.name              = "480p",
		.mode              = VMODE_480P,
        .width             = 720,
        .height            = 480,
        .field_height      = 480,
        .aspect_ratio_num  = 4,
        .aspect_ratio_den  = 3,
        .sync_duration_num = 60,
        .sync_duration_den = 1,
    },
    { /* VMODE_576I */
		.name              = "576i",
		.mode              = VMODE_576I,
        .width             = 720,
        .height            = 576,
        .field_height      = 288,
        .aspect_ratio_num  = 4,
        .aspect_ratio_den  = 3,
        .sync_duration_num = 50,
        .sync_duration_den = 1,
    },
    { /* VMODE_576I */
		.name              = "576cvbs",
		.mode              = VMODE_576CVBS,
        .width             = 720,
        .height            = 576,
        .field_height      = 288,
        .aspect_ratio_num  = 4,
        .aspect_ratio_den  = 3,
        .sync_duration_num = 50,
        .sync_duration_den = 1,
    },
    { /* VMODE_576P */
		.name              = "576p",
		.mode              = VMODE_576P,
        .width             = 720,
        .height            = 576,
        .field_height      = 576,
        .aspect_ratio_num  = 4,
        .aspect_ratio_den  = 3,
        .sync_duration_num = 50,
        .sync_duration_den = 1,
    },
    { /* VMODE_720P */
		.name              = "720p",
		.mode              = VMODE_720P,
        .width             = 1280,
        .height            = 720,
        .field_height      = 720,
        .aspect_ratio_num  = 16,
        .aspect_ratio_den  = 9,
        .sync_duration_num = 60,
        .sync_duration_den = 1,
    },
    { /* VMODE_1080I */
		.name              = "1080i",
		.mode              = VMODE_1080I,
        .width             = 1920,
        .height            = 1080,
        .field_height      = 540,
        .aspect_ratio_num  = 16,
        .aspect_ratio_den  = 9,
        .sync_duration_num = 60,
        .sync_duration_den = 1,
    },
    { /* VMODE_1080P */
		.name              = "1080p",
		.mode              = VMODE_1080P,
        .width             = 1920,
        .height            = 1080,
        .field_height      = 1080,
        .aspect_ratio_num  = 16,
        .aspect_ratio_den  = 9,
        .sync_duration_num = 60,
        .sync_duration_den = 1,
    },
    { /* VMODE_720P_50hz */
		.name              = "720p50hz",
		.mode              = VMODE_720P_50HZ,
        .width             = 1280,
        .height            = 720,
        .field_height      = 720,
        .aspect_ratio_num  = 16,
        .aspect_ratio_den  = 9,
        .sync_duration_num = 50,
        .sync_duration_den = 1,
    },
    { /* VMODE_1080I_50HZ */
		.name              = "1080i50hz",
		.mode              = VMODE_1080I_50HZ,
        .width             = 1920,
        .height            = 1080,
        .field_height      = 540,
        .aspect_ratio_num  = 16,
        .aspect_ratio_den  = 9,
        .sync_duration_num = 50,
        .sync_duration_den = 1,
    },
    { /* VMODE_1080P_50HZ */
		.name              = "1080p50hz",
		.mode              = VMODE_1080P_50HZ,
        .width             = 1920,
        .height            = 1080,
        .field_height      = 1080,
        .aspect_ratio_num  = 16,
        .aspect_ratio_den  = 9,
        .sync_duration_num = 50,
        .sync_duration_den = 1,
    },
    { /* VMODE_1080P_24HZ */
		.name              = "1080p24hz",
		.mode              = VMODE_1080P_24HZ,
        .width             = 1920,
        .height            = 1080,
        .field_height      = 1080,
        .aspect_ratio_num  = 16,
        .aspect_ratio_den  = 9,
        .sync_duration_num = 24,
        .sync_duration_den = 1,
    },
    { /* VMODE_vga */
		.name              = "vga",
		.mode              = VMODE_VGA,
        .width             = 640,
        .height            = 480,
        .field_height      = 240,
        .aspect_ratio_num  = 4,
        .aspect_ratio_den  = 3,
        .sync_duration_num = 60,
        .sync_duration_den = 1,
    }, 
    { /* VMODE_SVGA */
		.name              = "svga",
		.mode              = VMODE_SVGA,
        .width             = 800,
        .height            = 600,
        .field_height      = 600,
        .aspect_ratio_num  = 4,
        .aspect_ratio_den  = 3,
        .sync_duration_num = 60,
        .sync_duration_den = 1,
    }, 
    { /* VMODE_XGA */
		.name              = "xga",
		.mode              = VMODE_XGA,
        .width             = 1024,
        .height            = 768,
        .field_height      = 768,
        .aspect_ratio_num  = 4,
        .aspect_ratio_den  = 3,
        .sync_duration_num = 60,
        .sync_duration_den = 1,
    }, 
    { /* VMODE_sxga */
		.name              = "sxga",
		.mode              = VMODE_SXGA,
        .width             = 1280,
        .height            = 1024,
        .field_height      = 1024,
        .aspect_ratio_num  = 5,
        .aspect_ratio_den  = 4,
        .sync_duration_num = 60,
        .sync_duration_den = 1,
    }, 
};


#ifdef  CONFIG_PM
static int  meson_vdac_switch_suspend(struct platform_device *pdev, pm_message_t state)
{	
#ifdef CONFIG_HAS_EARLYSUSPEND
    if (early_suspend_flag)
        return 0;
#endif

	return 0;
}

static int  meson_vdac_switch_resume(struct platform_device *pdev)
{
#ifdef CONFIG_HAS_EARLYSUSPEND
    if (early_suspend_flag)
        return 0;
#endif

	return 0;
}
#endif 


#ifdef CONFIG_HAS_EARLYSUSPEND
static void meson_vdac_switch_early_suspend(struct early_suspend *h)
{
    if (early_suspend_flag)
        return;

	early_suspend_flag = 1;
}

static void meson_vdac_switch_late_resume(struct early_suspend *h)
{
    if (!early_suspend_flag)
        return;

	early_suspend_flag = 0;
}
#endif

static const vinfo_t *vdac_switch_get_info(void)
{
	return video_info.vinfo;
}

static int vdac_switch_set_vmode(vmode_t mode)
{
	if ((mode&VMODE_MODE_BIT_MASK)> VMODE_SXGA)
		return -EINVAL;

	video_info.vinfo = &tv_info[mode];

	if(mode&VMODE_LOGO_BIT_MASK)
		return 0;

	
	if( (vdac_switch_platdata!=NULL)&&(vdac_switch_platdata->vdac_switch_change_type_func!=NULL) )
	{
		if( (mode==VMODE_480CVBS) || (mode==VMODE_576CVBS) )
		{
			printk("vdac_switch mode = %d, switch = CVBS\n",mode);
			vdac_switch_platdata->vdac_switch_change_type_func(VOUT_CVBS);
		}
		else if( mode <= VMODE_1080P_24HZ )
		{
			printk("vdac_switch mode = %d, switch = COMPONENT\n",mode);
			vdac_switch_platdata->vdac_switch_change_type_func(VOUT_COMPONENT);
		}
		else if( mode <= VMODE_SXGA )
		{
			printk("vdac_switch mode = %d, switch = VGA\n",mode);
			vdac_switch_platdata->vdac_switch_change_type_func(VOUT_VGA);
		}
		else
			;// TODO: how about the VMODE_LCD and VMODE_LVDS ?
	}

	return 0;
}

static const vinfo_t *get_valid_vinfo(char  *mode)
{
	const vinfo_t * vinfo = NULL;
	int  i,count=ARRAY_SIZE(tv_info);
	int mode_name_len=0;
	
	for(i=0;i<count;i++)
	{
		if(strncmp(tv_info[i].name,mode,strlen(tv_info[i].name))==0)
		{
			if((vinfo==NULL)||(strlen(tv_info[i].name)>mode_name_len)){
			    vinfo = &tv_info[i];
			    mode_name_len = strlen(tv_info[i].name);
			}
		}
	}
	return vinfo;
}

static vmode_t vdac_switch_validate_vmode(char *mode)
{
	const vinfo_t *info = get_valid_vinfo(mode);

	if (info)
		return info->mode;
	
	return VMODE_MAX;
}

static int vdac_switch_vmode_is_supported(vmode_t mode)
{
	int  i,count=ARRAY_SIZE(tv_info);
	mode&=VMODE_MODE_BIT_MASK;
	for(i=0;i<count;i++)
	{
		if(tv_info[i].mode==mode)
		{
			return true;
		}
	}
	return false;
}

static int vdac_switch_module_disable(vmode_t cur_vmod)
{
	return 0;
}

#ifdef  CONFIG_PM
static int vdac_switch_suspend(void)
{
	return 0;
}

static int vdac_switch_resume(void)
{
	vdac_switch_set_vmode(video_info.vinfo->mode);
	return 0;
}
#endif 

static vout_server_t vdac_switch_server={
	.name = "vout_vdac_switch_server",
	.op = {	
		.get_vinfo=vdac_switch_get_info,
		.set_vmode=vdac_switch_set_vmode,
		.validate_vmode=vdac_switch_validate_vmode,
		.vmode_is_supported=vdac_switch_vmode_is_supported,
		.disable = vdac_switch_module_disable,
#ifdef  CONFIG_PM  
		.vout_suspend=vdac_switch_suspend,
		.vout_resume=vdac_switch_resume,
#endif	
	},
};


/*****************************************************************
**
**	vout driver interface  
**
******************************************************************/
static int meson_vdac_switch_probe(struct platform_device *pdev)
{
	int ret =-1;
	
	amlog_mask_level(LOG_MASK_INIT,LOG_LEVEL_HIGH,"start init vdac switch module \r\n");
#ifdef CONFIG_HAS_EARLYSUSPEND
    early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
    early_suspend.suspend = meson_vdac_switch_early_suspend;
    early_suspend.resume = meson_vdac_switch_late_resume;
	register_early_suspend(&early_suspend);
#endif

	vdac_switch_platdata = (struct aml_vdac_switch_platform_data*)pdev->dev.platform_data;

	ret = vout_register_server(&vdac_switch_server);
	if(ret)
	{
		amlog_mask_level(LOG_MASK_INIT,LOG_LEVEL_HIGH,"registe vdac_switch server fail \r\n");
	}
	else
	{
		amlog_mask_level(LOG_MASK_INIT,LOG_LEVEL_HIGH,"registe vdac_switch server ok \r\n");
	}

	memset(&video_info, 0, sizeof(disp_module_info_t));

	return ret;
}
static int meson_vdac_switch_remove(struct platform_device *pdev)
{
#ifdef CONFIG_HAS_EARLYSUSPEND
    unregister_early_suspend(&early_suspend);
#endif

	vdac_switch_platdata = NULL;
	memset(&video_info, 0, sizeof(disp_module_info_t));

	return 0;
}

static struct platform_driver vdac_switch_driver = {
    .probe      = meson_vdac_switch_probe,
    .remove     = meson_vdac_switch_remove,
#ifdef  CONFIG_PM      
    .suspend  =meson_vdac_switch_suspend,
    .resume    =meson_vdac_switch_resume,
#endif    
    .driver     = {
        .name   = "mesonvdacswitch",
    }
};

static int __init vdac_switch_init_module(void)
{
	int ret =0;
    
    printk("%s\n", __func__);
	if (platform_driver_register(&vdac_switch_driver)) 
	{
   		amlog_level(LOG_LEVEL_HIGH,"failed to register vdac switch driver\n");
    	ret= -ENODEV;
	}
	
	return ret;
}

static __exit void vdac_switch_exit_module(void)
{
	amlog_level(LOG_LEVEL_HIGH,"vdac_switch remove module.\n");

    platform_driver_unregister(&vdac_switch_driver);
}

arch_initcall(vdac_switch_init_module);
module_exit(vdac_switch_exit_module);

MODULE_DESCRIPTION("vdac switch module");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("jets.yan <jets.yan@amlogic.com>");
