/*
 * TVAFE char device driver.
 *
 * Copyright (c) 2010 Bo Yang <bo.yang@amlogic.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the smems of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */

/* Standard Linux headers */
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <asm/uaccess.h>
#include <linux/mutex.h>
#include <linux/mm.h>

/* Amlogic headers */
#include <linux/amports/canvas.h>
#include <mach/am_regs.h>
#include <linux/amports/vframe.h>

/* Local include */
#include <linux/tvin/tvin.h>
#include "../tvin_frontend.h"
#include "../tvin_global.h"
#include "../tvin_format_table.h"
#include "tvafe_regs.h"
#include "tvafe_adc.h"
#include "tvafe_cvd.h"
#include "tvafe_general.h"
#include "tvafe.h"

#define TVAFE_NAME               "tvafe"
#define TVAFE_DRIVER_NAME        "tvafe"
#define TVAFE_MODULE_NAME        "tvafe"
#define TVAFE_DEVICE_NAME        "tvafe"
#define TVAFE_CLASS_NAME         "tvafe"

static dev_t                     tvafe_devno;
static struct class              *tvafe_clsp;
static bool                      disableapi = 0;
static bool                      force_stable = false;
#define TVAFE_TIMER_INTERVAL    (HZ/100)   //10ms, #define HZ 100


/*default only one tvafe ,echo cvdfmt pali/palm/ntsc/secam >dir*/
static ssize_t tvafe_store(struct device *dev, struct device_attribute *attr,const char *buff,size_t count)
{
	unsigned char fmt_index = 0;

	struct tvafe_dev_s *devp;
	devp = dev_get_drvdata(dev);

	if(!strncmp(buff,"cvdfmt",strlen("cvdfmt")))
	{
		fmt_index = strlen("cvdfmt") + 1;
		if(!strncmp(buff+fmt_index,"ntscm",strlen("ntscm")))
			devp->tvafe.cvd2.manual_fmt = TVIN_SIG_FMT_CVBS_NTSC_M;
		else if(!strncmp(buff+fmt_index,"ntsc443",strlen("ntsc443")))
			devp->tvafe.cvd2.manual_fmt = TVIN_SIG_FMT_CVBS_NTSC_443;
		else if(!strncmp(buff+fmt_index,"pali",strlen("pali")))
			devp->tvafe.cvd2.manual_fmt = TVIN_SIG_FMT_CVBS_PAL_I;
		else if(!strncmp(buff+fmt_index,"palm",strlen("plam")))
			devp->tvafe.cvd2.manual_fmt = TVIN_SIG_FMT_CVBS_PAL_M;
		else if(!strncmp(buff+fmt_index,"pal60",strlen("pal60")))
			devp->tvafe.cvd2.manual_fmt = TVIN_SIG_FMT_CVBS_PAL_60;
		else if(!strncmp(buff+fmt_index,"palcn",strlen("palcn")))
			devp->tvafe.cvd2.manual_fmt = TVIN_SIG_FMT_CVBS_PAL_CN;
		else if(!strncmp(buff+fmt_index,"secam",strlen("secam")))
			devp->tvafe.cvd2.manual_fmt = TVIN_SIG_FMT_CVBS_SECAM;
		else if(!strncmp(buff+fmt_index,"null",strlen("null")))
			devp->tvafe.cvd2.manual_fmt = TVIN_SIG_FMT_NULL;
		else
			pr_info("%s:invaild command.",buff);
	}
	else if(!strncmp(buff,"disableapi",strlen("disableapi")))
		disableapi = simple_strtoul(buff+strlen("disableapi")+1,NULL,10);
    else if(!strncmp(buff,"force_stable",strlen("force_stable")))
        force_stable = simple_strtoul(buff+strlen("force_stable")+1,NULL,10);
	else if(!strncmp(buff,"cphasepr",strlen("cphasepr")))
	        tvafe_adc_comphase_pr();
        else if(!strncmp(buff,"vdin_bbld",strlen("vdin_bbld"))){
                tvin_vdin_bbar_init(devp->tvafe.parm.info.fmt);
                devp->tvafe.adc.vga_auto.phase_state == VGA_VDIN_BORDER_DET;
        }
        else if(!strncmp(buff,"pdown",strlen("pdown"))){
                tvafe_enable_module(false);
        }
	else if(!strncmp(buff, "vga_edid",strlen("vga_edid"))){
		struct tvafe_vga_edid_s edid;
		int i = 0;
		tvafe_vga_get_edid(&edid);
		for(i=0; i<32; i++)
		{
        		pr_info("0x%2x 0x%2x 0x%2x 0x%2x 0x%2x 0x%2x 0x%2x 0x%2x.\n", edid.value[(i<<3)+0],
				edid.value[(i<<3)+1], edid.value[(i<<3)+2], edid.value[(i<<3)+3], edid.value[(i<<3)+4],
				edid.value[(i<<3)+5], edid.value[(i<<3)+6], edid.value[(i<<3)+7] );
		}
	}
	else
		pr_info("[%s]:invaild command.\n",__func__);
	return count;
}
static ssize_t tvafe_show(struct device *dev,struct device_attribute *attr,char *buff)
{
	ssize_t len = 0;

	struct tvafe_dev_s *devp;
	devp = dev_get_drvdata(dev);
	switch(devp->tvafe.cvd2.manual_fmt)
	{
		case TVIN_SIG_FMT_CVBS_NTSC_M:
			len = sprintf(buff,"cvdfmt:%s.\n","ntscm");
			break;
		case TVIN_SIG_FMT_CVBS_NTSC_443:
			len = sprintf(buff,"cvdfmt:%s.\n","ntsc443");
			break;
		case TVIN_SIG_FMT_CVBS_PAL_I:
			len = sprintf(buff,"cvdfmt:%s.\n","pali");
			break;
		case TVIN_SIG_FMT_CVBS_PAL_M:
			len = sprintf(buff,"cvdfmt:%s.\n","palm");
			break;
		case TVIN_SIG_FMT_CVBS_PAL_60:
			len = sprintf(buff,"cvdfmt:%s.\n","pal60");
			break;
		case TVIN_SIG_FMT_CVBS_PAL_CN:
			len = sprintf(buff,"cvdfmt:%s.\n","palcn");
			break;
		case TVIN_SIG_FMT_CVBS_SECAM:
			len = sprintf(buff,"cvdfmt:%s.\n","secam");
			break;
		case TVIN_SIG_FMT_NULL:
			len = sprintf(buff,"cvdfmt:%s.\n","auto");
		default:
			len = sprintf(buff,"cvdfmt:%s.\n","invaild command");
			break;

	}
	if(disableapi)
		pr_info("[%s]:diableapi!!!.\n",__func__);
	return len;
}
static DEVICE_ATTR(debug,0644,tvafe_show,tvafe_store);
/*
 * tvafe 10ms timer handler
 */
void tvafe_timer_handler(unsigned long arg)
{
	struct tvafe_dev_s *devp = (struct tvafe_dev_s *)arg;
	struct tvafe_info_s *tvafe = &devp->tvafe;

	tvafe_vga_auto_adjust_handler(&tvafe->parm, &tvafe->adc);

	devp->timer.expires = jiffies + TVAFE_TIMER_INTERVAL;
	add_timer(&devp->timer);
}

/*
 * tvafe check support port
 */
int tvafe_dec_support(struct tvin_frontend_s *fe, enum tvin_port_e port)
{
	struct tvafe_dev_s *devp = container_of(fe, struct tvafe_dev_s, frontend);

	/* check afe port and index */
	if (((port < TVIN_PORT_VGA0) || (port > TVIN_PORT_SVIDEO7)) ||
			(fe->index != devp->index))
		return -1;

	return 0;
}

/*
 * tvafe open port and init register
 */
int tvafe_dec_open(struct tvin_frontend_s *fe, enum tvin_port_e port)
{
	struct tvafe_dev_s *devp = container_of(fe, struct tvafe_dev_s, frontend);
	struct tvafe_info_s *tvafe = &devp->tvafe;

	mutex_lock(&devp->afe_mutex);
	if (devp->flags& TVAFE_FLAG_DEV_OPENED)
	{
		pr_err("[tvafe..] %s(%d), %s opened already\n", __func__,
				devp->index, tvin_port_str(port));
		mutex_unlock(&devp->afe_mutex);
		return 1;
	}
	/* init variable */
	memset(tvafe, 0, sizeof(struct tvafe_info_s));
	/**enable and reset tvafe clock**/
	tvafe_enable_module(true);

    /**set cvd2 reset to high**/
    tvafe_cvd2_hold_rst(&tvafe->cvd2);

	/* init tvafe registers */
	tvafe_init_reg(&tvafe->cvd2, &devp->mem, port, devp->pinmux);

	tvafe->parm.port = port;

	/* timer */
	init_timer(&devp->timer);
	devp->timer.data = (ulong)devp;
	devp->timer.function = tvafe_timer_handler;
	devp->timer.expires = jiffies + (TVAFE_TIMER_INTERVAL);
	add_timer(&devp->timer);

	/* set the flag to enabble ioctl access */
	devp->flags|= TVAFE_FLAG_DEV_OPENED;
	pr_info("[tvafe..] %s open port:0x%x ok.\n", __func__, port);

	mutex_unlock(&devp->afe_mutex);
	return 0;
}

/*
 * tvafe start after signal stable
 */
void tvafe_dec_start(struct tvin_frontend_s *fe, enum tvin_sig_fmt_e fmt)
{
	struct tvafe_dev_s *devp = container_of(fe, struct tvafe_dev_s, frontend);
	struct tvafe_info_s *tvafe = &devp->tvafe;
	enum tvin_port_e port = devp->tvafe.parm.port;

	mutex_lock(&devp->afe_mutex);
	if(!(devp->flags & TVAFE_FLAG_DEV_OPENED))
	{
		pr_err("[tvafe..] tvafe_dec_start(%d) decode havn't opened\n", devp->index);
		mutex_unlock(&devp->afe_mutex);
		return;
	}

	if (devp->flags & TVAFE_FLAG_DEV_STARTED)
	{
		pr_err("[tvafe..] %s(%d), %s started already\n", __func__,
				devp->index, tvin_port_str(port));
		mutex_unlock(&devp->afe_mutex);
		return;
	}

	tvafe->parm.info.fmt = fmt;
	tvafe->parm.info.status = TVIN_SIG_STATUS_STABLE;

	devp->flags |= TVAFE_FLAG_DEV_STARTED;

	pr_info("[tvafe..] %s start fmt:%s ok.\n", __func__, tvin_sig_fmt_str(fmt));

	mutex_unlock(&devp->afe_mutex);
}

/*
 * tvafe stop port
 */
void tvafe_dec_stop(struct tvin_frontend_s *fe, enum tvin_port_e port)
{
	struct tvafe_dev_s *devp = container_of(fe, struct tvafe_dev_s, frontend);
	struct tvafe_info_s *tvafe = &devp->tvafe;

	mutex_lock(&devp->afe_mutex);
	if(!(devp->flags & TVAFE_FLAG_DEV_STARTED))
	{
		pr_err("[tvafe..] tvafe_dec_stop(%d) decode havn't started\n", devp->index);
		mutex_unlock(&devp->afe_mutex);
		return;
	}

	/* init variable */
	memset(&tvafe->adc, 0, sizeof(struct tvafe_adc_s));
	memset(&tvafe->cal, 0, sizeof(struct tvafe_cal_s));
	memset(&tvafe->comp_wss, 0, sizeof(struct tvafe_comp_wss_s));
	memset(&tvafe->cvd2.info, 0, sizeof(struct tvafe_cvd2_info_s));
	memset(&tvafe->parm.info, 0, sizeof(struct tvin_info_s));

	tvafe->parm.port = port;

	tvafe_adc_digital_reset();

	// need to do ...
	/** write 7740 register for cvbs clamp **/
	if ((port >= TVIN_PORT_CVBS0) && (port <= TVIN_PORT_SVIDEO7))
	{
		tvafe->cvd2.fmt_loop_cnt = 0;  //reset loop cnt after channel switch
#ifdef TVAFE_SET_CVBS_PGA_EN
		tvafe_cvd2_reset_pga();
#endif

#ifdef TVAFE_SET_CVBS_CDTO_EN
		tvafe_cvd2_set_default_cdto(&tvafe->cvd2);
#endif
		tvafe_cvd2_set_default_de(&tvafe->cvd2);
	}
	devp->flags &= (~TVAFE_FLAG_DEV_STARTED);

	pr_info("[tvafe..] %s stop port:0x%x ok.\n", __func__, port);

	mutex_unlock(&devp->afe_mutex);
}

/*
 * tvafe close port
 */
void tvafe_dec_close(struct tvin_frontend_s *fe)
{
	struct tvafe_dev_s *devp = container_of(fe, struct tvafe_dev_s, frontend);
	struct tvafe_info_s *tvafe = &devp->tvafe;

	mutex_lock(&devp->afe_mutex);
	if(!(devp->flags & TVAFE_FLAG_DEV_OPENED))
	{
		pr_err("[tvafe..] tvafe_dec_close(%d) decode havn't opened\n", devp->index);
		mutex_unlock(&devp->afe_mutex);
		return;
	}

	del_timer_sync(&devp->timer);

	/**set cvd2 reset to high**/
	tvafe_cvd2_hold_rst(&tvafe->cvd2);
	/**disable av out**/
	tvafe_enable_avout(false);
#ifdef TVAFE_POWERDOWN_IN_IDLE
	/**disable tvafe clock**/
	tvafe_enable_module(false);
#endif

	/* init variable */
	memset(tvafe, 0, sizeof(struct tvafe_info_s));

	devp->flags &= (~TVAFE_FLAG_DEV_OPENED);

	pr_info("[tvafe..] %s close afe ok.\n", __func__);

	mutex_unlock(&devp->afe_mutex);
}

/*
 * tvafe vsync interrupt function
 */
int tvafe_dec_isr(struct tvin_frontend_s *fe, unsigned int hcnt64)
{
	struct tvafe_dev_s *devp = container_of(fe, struct tvafe_dev_s, frontend);
	struct tvafe_info_s *tvafe = &devp->tvafe;
	enum tvin_port_e port = tvafe->parm.port;

        if(!(devp->flags & TVAFE_FLAG_DEV_OPENED))
        {
                pr_err("[tvafe..] tvafe havn't opened, isr error!!!\n");
                return true;
        }

        if (force_stable)
            return 0;
	/* if there is any error or overflow, do some reset, then rerurn -1;*/
	if ((tvafe->parm.info.status != TVIN_SIG_STATUS_STABLE) ||
			(tvafe->parm.info.fmt == TVIN_SIG_FMT_NULL)) {
		return -1;
	}

	/* TVAFE CVD2 3D works abnormally => reset cvd2 */
	if ((port >= TVIN_PORT_CVBS0) && (port <= TVIN_PORT_CVBS7))
	{
		tvafe_cvd2_check_3d_comb(&tvafe->cvd2);
	}

#ifdef TVAFE_SET_CVBS_PGA_EN
	if ((port >= TVIN_PORT_CVBS0) && (port <= TVIN_PORT_SVIDEO7))
	{
		tvafe_cvd2_adj_pga(&tvafe->cvd2);
	}
#endif

#ifdef TVAFE_SET_CVBS_CDTO_EN
	if (tvafe->parm.info.fmt == TVIN_SIG_FMT_CVBS_PAL_I)
	{
		tvafe_cvd2_adj_cdto(&tvafe->cvd2, hcnt64);
	}
#endif

	//tvafe_adc_clamp_adjust(devp);

	/* TVAFE vs counter for VGA */
	if ((port >= TVIN_PORT_VGA0) && (port <= TVIN_PORT_VGA7))
	{                                
		tvafe_vga_vs_cnt(&tvafe->adc);                
                if(tvafe->adc.vga_auto.phase_state == VGA_VDIN_BORDER_DET){                                        
                        tvin_vdin_bar_detect(tvafe->parm.info.fmt,&tvafe->adc);
                }
                              
	}

	/* fetch WSS data must get them during VBI */
	if ((port >= TVIN_PORT_COMP0) && (port <= TVIN_PORT_COMP7))
	{
		tvafe_get_wss_data(&tvafe->comp_wss);
	}

	return 0;
}

static struct tvin_decoder_ops_s tvafe_dec_ops = {
	.support    = tvafe_dec_support,
	.open       = tvafe_dec_open,
	.start      = tvafe_dec_start,
	.stop       = tvafe_dec_stop,
	.close      = tvafe_dec_close,
	.decode_isr = tvafe_dec_isr,
};

/*
 * tvafe signal signal status: signal on/off
 */
bool tvafe_is_nosig(struct tvin_frontend_s *fe)
{
	bool ret = false;
	/* Get the per-device structure that contains this frontend */
	struct tvafe_dev_s *devp = container_of(fe, struct tvafe_dev_s, frontend);
	struct tvafe_info_s *tvafe = &devp->tvafe;
	enum tvin_port_e port = tvafe->parm.port;

    if(!(devp->flags & TVAFE_FLAG_DEV_OPENED))
    {
        pr_err("[tvafe..] tvafe havn't opened, check no sig error!!!\n");
        return true;
    }
    if (force_stable)
        return ret;

	if ((port >= TVIN_PORT_VGA0) && (port <= TVIN_PORT_COMP7))
		ret = tvafe_adc_no_sig();
	else if ((port >= TVIN_PORT_CVBS0) && (port <= TVIN_PORT_SVIDEO7))
	{
		ret = tvafe_cvd2_no_sig(&tvafe->cvd2, &devp->mem);

		/* normal sigal & adc reg error, reload source mux */
		if (tvafe->cvd2.info.adc_reload_en && !ret)
		{
			tvafe_set_source_muxing(port, devp->pinmux);
		}
	}

	return ret;
}

/*
 * tvafe signal mode status: change/unchange
 */
bool tvafe_fmt_chg(struct tvin_frontend_s *fe)
{
	bool ret = false;
	/* Get the per-device structure that contains this frontend */
	struct tvafe_dev_s *devp = container_of(fe, struct tvafe_dev_s, frontend);
	struct tvafe_info_s *tvafe = &devp->tvafe;
	enum tvin_port_e port = tvafe->parm.port;

    if(!(devp->flags & TVAFE_FLAG_DEV_OPENED))
    {
        pr_err("[tvafe..] tvafe havn't opened, get fmt chg error!!!\n");
        return true;
    }
    if (force_stable)
        return ret;

	if ((port >= TVIN_PORT_VGA0) && (port <= TVIN_PORT_COMP7))
		ret = tvafe_adc_fmt_chg(&tvafe->parm, &tvafe->adc);
	else  if ((port >= TVIN_PORT_CVBS0) && (port <= TVIN_PORT_SVIDEO7))
		ret = tvafe_cvd2_fmt_chg(&tvafe->cvd2);

	return ret;
}

/*
 * tvafe adc lock status: lock/unlock
 */
bool tvafe_pll_lock(struct tvin_frontend_s *fe)
{
	bool ret = true;

#if 0  //can not trust pll lock status
	/* Get the per-device structure that contains this frontend */
	struct tvafe_dev_s *devp = container_of(fe, struct tvafe_dev_s, frontend);
	struct tvafe_info_s *tvafe = &devp->tvafe;
	enum tvin_port_e port = tvafe->parm.port;

	if ((port >= TVIN_PORT_VGA0) && (port <= TVIN_PORT_COMP7))
		ret = tvafe_adc_get_pll_status();
#endif
	return (ret);
}

/*
 * tvafe search format number
 */
enum tvin_sig_fmt_e tvafe_get_fmt(struct tvin_frontend_s *fe)
{
	enum tvin_sig_fmt_e fmt = TVIN_SIG_FMT_NULL;
	/* Get the per-device structure that contains this frontend */
	struct tvafe_dev_s *devp = container_of(fe, struct tvafe_dev_s, frontend);
	struct tvafe_info_s *tvafe = &devp->tvafe;
	enum tvin_port_e port = tvafe->parm.port;

    if(!(devp->flags & TVAFE_FLAG_DEV_OPENED))
    {
        pr_err("[tvafe..] tvafe havn't opened, get sig fmt error!!!\n");
        return fmt;
    }

	if ((port >= TVIN_PORT_VGA0) && (port <= TVIN_PORT_COMP7))
		fmt = tvafe_adc_search_mode(&tvafe->parm, &tvafe->adc);
	else  if ((port >= TVIN_PORT_CVBS0) && (port <= TVIN_PORT_SVIDEO7))
		fmt = tvafe_cvd2_get_format(&tvafe->cvd2);

	tvafe->parm.info.fmt = fmt;

	pr_info("[tvafe..] %s fmt:%s. \n", __func__, tvin_sig_fmt_str(fmt));

	return fmt;
}

#ifdef TVAFE_SET_CVBS_MANUAL_FMT_POS
/*
 * tvafe cvbs video position setting by mode detection
 */
enum tvin_cvbs_pos_ctl_e tvafe_set_cvbs_fmt_pos(struct tvin_frontend_s *fe)
{
	/* Get the per-device structure that contains this frontend */
	struct tvafe_dev_s *devp = container_of(fe, struct tvafe_dev_s, frontend);
	struct tvafe_info_s *tvafe = &devp->tvafe;
	enum tvin_cvbs_pos_ctl_e cvbs_pos_ctl = TVIN_CVBS_POS_NULL;

	cvbs_pos_ctl = tvafe_cvd2_set_pos(&tvafe->cvd2);

	return (cvbs_pos_ctl);
}
#endif

/*
 * tvafe signal property: 2D/3D, color format, aspect ratio, pixel repeat
 */
void tvafe_get_sig_property(struct tvin_frontend_s *fe, struct tvin_sig_property_s *prop)
{
	struct tvafe_dev_s *devp = container_of(fe, struct tvafe_dev_s, frontend);
	struct tvafe_info_s *tvafe = &devp->tvafe;
	enum tvin_port_e port = tvafe->parm.port;

	prop->trans_fmt = TVIN_TFMT_2D;
	if ((port >= TVIN_PORT_VGA0) &&
			(port <= TVIN_PORT_VGA7))
		prop->color_format = TVIN_RGB444;
	else
		prop->color_format = TVIN_YUV444;
	prop->aspect_ratio = TVIN_ASPECT_NULL;
	prop->pixel_repeat = 0;
	prop->dvi_info = 0;
}
/*
 *get cvbs secam source's phase
 */
static bool tvafe_cvbs_get_secam_phase(struct tvin_frontend_s *fe)
{
	struct tvafe_dev_s *devp = container_of(fe, struct tvafe_dev_s, frontend);
	struct tvafe_info_s *tvafe = &devp->tvafe;

	if(tvafe->cvd2.config_fmt == TVIN_SIG_FMT_CVBS_SECAM)
		return tvafe->cvd2.hw.secam_phase;
	else
		return 0;

}

/*
 * tvafe set vga parameters: h/v position, phase, clock
 */
void tvafe_vga_set_parm(struct tvafe_vga_parm_s *vga_parm, struct tvin_frontend_s *fe)
{
	struct tvafe_dev_s *devp = container_of(fe, struct tvafe_dev_s, frontend);
	struct tvafe_info_s *tvafe = &devp->tvafe;

    if(!(devp->flags & TVAFE_FLAG_DEV_OPENED))
    {
        pr_err("[tvafe..] tvafe havn't opened, vga parm error!!!\n");
        return;
    }

	if(vga_parm == 0)
		tvafe_adc_set_param(&tvafe->parm, &tvafe->adc);
	else
		tvafe_adc_set_deparam(&tvafe->parm, &tvafe->adc);
}

/*
 * tvafe get vga parameters: h/v position, phase, clock
 */
void tvafe_vga_get_parm(struct tvafe_vga_parm_s *vga_parm, struct tvin_frontend_s *fe)
{
	struct tvafe_dev_s *devp = container_of(fe, struct tvafe_dev_s, frontend);
	struct tvafe_info_s *tvafe = &devp->tvafe;
	struct tvafe_vga_parm_s *parm = &tvafe->adc.vga_parm;

	vga_parm->clk_step     = parm->clk_step;
	vga_parm->phase        = parm->phase;
	vga_parm->hpos_step    = parm->hpos_step;
	vga_parm->vpos_step    = parm->vpos_step;
	vga_parm->vga_in_clean = parm->vga_in_clean;
}

/*
 * tvafe configure format Reg table
 */
void tvafe_fmt_config(struct tvin_frontend_s *fe)
{
	struct tvafe_dev_s *devp = container_of(fe, struct tvafe_dev_s, frontend);
	struct tvafe_info_s *tvafe = &devp->tvafe;
	enum tvin_port_e port = tvafe->parm.port;


    if(!(devp->flags & TVAFE_FLAG_DEV_OPENED))
    {
        pr_err("[tvafe..] tvafe havn't opened, config fmt error!!!\n");
        return;
    }

	/*store the current fmt avoid configuration again*/
	if(tvafe->adc.current_fmt != tvafe->parm.info.fmt)
		tvafe->adc.current_fmt = tvafe->parm.info.fmt;
	else
	{
		pr_info("[tvafe..] %s,no use to config fmt:%s.\n",__func__, tvin_sig_fmt_str(tvafe->parm.info.fmt));
		return;
	}
	if ((port >= TVIN_PORT_VGA0) && (port <= TVIN_PORT_VGA7))
	{
		tvafe_set_vga_fmt(&tvafe->parm, &tvafe->cal, devp->pinmux);
		pr_info("[tvafe..] %s, config fmt:%s. \n", __func__, tvin_sig_fmt_str(tvafe->parm.info.fmt));
	}
	else if ((port >= TVIN_PORT_COMP0) && (port <= TVIN_PORT_COMP7))
	{
		tvafe_set_comp_fmt(&tvafe->parm, &tvafe->cal, devp->pinmux);
		pr_info("[tvafe..] %s, config fmt:%s. \n", __func__, tvin_sig_fmt_str(tvafe->parm.info.fmt));
	}
}

/*
 * tvafe calibration function called by vdin state machine
 */
bool tvafe_cal(struct tvin_frontend_s *fe)
{
	struct tvafe_dev_s *devp = container_of(fe, struct tvafe_dev_s, frontend);
	struct tvafe_info_s *tvafe = &devp->tvafe;

    if(!(devp->flags & TVAFE_FLAG_DEV_OPENED))
    {
        pr_err("[tvafe..] tvafe havn't opened, calibration error!!!\n");
        return false;
    }

	return tvafe_adc_cal(&tvafe->parm, &tvafe->cal);
}

/*
 * tvafe skip some frame after adjusting vga parameter to avoid picture flicker
 */
bool tvafe_check_frame_skip(struct tvin_frontend_s *fe)
{
	bool ret = false;
	struct tvafe_dev_s *devp = container_of(fe, struct tvafe_dev_s, frontend);
	struct tvafe_info_s *tvafe = &devp->tvafe;
	enum tvin_port_e port = tvafe->parm.port;


    if(!(devp->flags & TVAFE_FLAG_DEV_OPENED))
    {
        pr_err("[tvafe..] tvafe havn't opened, check frame error!!!\n");
        return ret;
    }

	if (((port >= TVIN_PORT_COMP0) && (port <= TVIN_PORT_COMP7)) ||
			((port >= TVIN_PORT_VGA0) && (port <= TVIN_PORT_VGA7))) {
		ret = tvafe_adc_check_frame_skip(&tvafe->adc);
	}

	return ret;
}

static struct tvin_state_machine_ops_s tvafe_sm_ops = {
	.nosig            = tvafe_is_nosig,
	.fmt_changed      = tvafe_fmt_chg,
	.get_fmt          = tvafe_get_fmt,
	.fmt_config       = tvafe_fmt_config,
	.adc_cal          = tvafe_cal,
	.pll_lock         = tvafe_pll_lock,
	.get_sig_propery  = tvafe_get_sig_property,
#ifdef TVAFE_SET_CVBS_MANUAL_FMT_POS
	.set_cvbs_fmt_pos = tvafe_set_cvbs_fmt_pos,
#endif
	.vga_set_param    = tvafe_vga_set_parm,
	.vga_get_param    = tvafe_vga_get_parm,
	.check_frame_skip = tvafe_check_frame_skip,
	.get_secam_phase = tvafe_cvbs_get_secam_phase,
};

static int tvafe_open(struct inode *inode, struct file *file)
{
	tvafe_dev_t *devp;

	/* Get the per-device structure that contains this cdev */
	devp = container_of(inode->i_cdev, tvafe_dev_t, cdev);
	file->private_data = devp;

	/* ... */

	pr_info("[tvafe..] %s: open device \n", __FUNCTION__);

	return 0;
}

static int tvafe_release(struct inode *inode, struct file *file)
{
	tvafe_dev_t *devp = file->private_data;

	file->private_data = NULL;

	/* Release some other fields */
	/* ... */

	pr_info("[tvafe..] tvafe: device %d release ok.\n", devp->index);

	return 0;
}


static long tvafe_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long ret = 0;
	unsigned char i,j;
	void __user *argp = (void __user *)arg;
	struct tvafe_dev_s *devp = file->private_data;
	struct tvafe_info_s *tvafe = &devp->tvafe;
	enum tvin_port_e port = tvafe->parm.port;
	enum tvin_sig_fmt_e fmt = tvafe->parm.info.fmt;
	struct tvafe_vga_edid_s edid;
	enum tvafe_cvbs_video_e cvbs_lock_status = TVAFE_CVBS_VIDEO_HV_UNLOCKED;

	if (_IOC_TYPE(cmd) != TVIN_IOC_MAGIC) {
		pr_err("%s invalid command: %u\n", __func__, cmd);
		return -ENOSYS;
	}

	//pr_info("[tvafe..] %s command: %u\n", __func__, cmd);
	if(disableapi)
		return -ENOSYS;
	mutex_lock(&devp->afe_mutex);

	/* EDID command !!! */
	if (((cmd != TVIN_IOC_S_AFE_VGA_EDID) && (cmd != TVIN_IOC_G_AFE_VGA_EDID)) &&
			(!(devp->flags & TVAFE_FLAG_DEV_OPENED))
	   )
	{
		pr_info("[tvafe..] %s, tvafe device is disable, ignore the command %d\n", __func__, cmd);
		mutex_unlock(&devp->afe_mutex);
		return -EPERM;
	}

	switch (cmd)
	{
		case TVIN_IOC_S_AFE_ADC_CAL:
			if (copy_from_user(&tvafe->cal.cal_val, argp, sizeof(struct tvafe_adc_cal_s)))
			{
				ret = -EFAULT;
				break;
			}

			tvafe->cal.cal_val.reserved |= TVAFE_ADC_CAL_VALID;
			tvafe_vga_auto_adjust_disable(&tvafe->adc);
			if ((port >= TVIN_PORT_COMP0) &&(port <= TVIN_PORT_COMP7))
			{

				//if((fmt>=TVIN_SIG_FMT_COMP_1080P_23HZ_D976)&&(fmt<=TVIN_SIG_FMT_COMP_1080P_60HZ_D000))
				//	tvafe->cal.cal_val.a_analog_clamp += 4;
				//else if((fmt>=TVIN_SIG_FMT_COMP_1080I_47HZ_D952)&&(fmt<=TVIN_SIG_FMT_COMP_1080I_60HZ_D000))
				//	tvafe->cal.cal_val.a_analog_clamp += 3;
				//else
				//	tvafe->cal.cal_val.a_analog_clamp += 2;
				tvafe->cal.cal_val.b_analog_clamp += 1;
				tvafe->cal.cal_val.c_analog_clamp += 1;
			}

			else if((port >= TVIN_PORT_VGA0) &&(port <= TVIN_PORT_VGA7))
			{

				//tvafe->cal.cal_val.a_analog_clamp += 2;
				tvafe->cal.cal_val.b_analog_clamp += 1;
				tvafe->cal.cal_val.c_analog_clamp += 1;
			}
			//pr_info("\nNot allow to use TVIN_IOC_S_AFE_ADC_CAL command!!!\n\n");
			tvafe_set_cal_value(&tvafe->cal);
#ifdef LOG_ADC_CAL
			pr_info("\nset_adc_cal\n");
			pr_info("A_cl = %4d %4d %4d\n",  (int)(tvafe->cal.cal_val.a_analog_clamp),
					(int)(tvafe->cal.cal_val.b_analog_clamp),
					(int)(tvafe->cal.cal_val.c_analog_clamp));
			pr_info("A_gn = %4d %4d %4d\n",  (int)(tvafe->cal.cal_val.a_analog_gain),
					(int)(tvafe->cal.cal_val.b_analog_gain),
					(int)(tvafe->cal.cal_val.c_analog_gain));
			pr_info("D_gn = %4d %4d %4d\n",  (int)(tvafe->cal.cal_val.a_digital_gain),
					(int)(tvafe->cal.cal_val.b_digital_gain),
					(int)(tvafe->cal.cal_val.c_digital_gain));
			pr_info("D_o1 = %4d %4d %4d\n", ((int)(tvafe->cal.cal_val.a_digital_offset1) << 21) >> 21,
					((int)(tvafe->cal.cal_val.b_digital_offset1) << 21) >> 21,
					((int)(tvafe->cal.cal_val.c_digital_offset1) << 21) >> 21);
			pr_info("D_o2 = %4d %4d %4d\n", ((int)(tvafe->cal.cal_val.a_digital_offset2) << 21) >> 21,
					((int)(tvafe->cal.cal_val.b_digital_offset2) << 21) >> 21,
					((int)(tvafe->cal.cal_val.c_digital_offset2) << 21) >> 21);
			pr_info("\n");
#endif

			break;

		case TVIN_IOC_G_AFE_ADC_CAL:
			{
				ret = tvafe_get_cal_value(&tvafe->cal);
#ifdef LOG_ADC_CAL
				pr_info("\nget_adc_cal\n");
				pr_info("A_cl = %4d %4d %4d\n",  (int)(tvafe->cal.cal_val.a_analog_clamp),
						(int)(tvafe->cal.cal_val.b_analog_clamp),
						(int)(tvafe->cal.cal_val.c_analog_clamp));
				pr_info("A_gn = %4d %4d %4d\n",  (int)(tvafe->cal.cal_val.a_analog_gain),
						(int)(tvafe->cal.cal_val.b_analog_gain),
						(int)(tvafe->cal.cal_val.c_analog_gain));
				pr_info("D_gn = %4d %4d %4d\n",  (int)(tvafe->cal.cal_val.a_digital_gain),
						(int)(tvafe->cal.cal_val.b_digital_gain),
						(int)(tvafe->cal.cal_val.c_digital_gain));
				pr_info("D_o1 = %4d %4d %4d\n", ((int)(tvafe->cal.cal_val.a_digital_offset1) << 21) >> 21,
						((int)(tvafe->cal.cal_val.b_digital_offset1) << 21) >> 21,
						((int)(tvafe->cal.cal_val.c_digital_offset1) << 21) >> 21);
				pr_info("D_o2 = %4d %4d %4d\n", ((int)(tvafe->cal.cal_val.a_digital_offset2) << 21) >> 21,
						((int)(tvafe->cal.cal_val.b_digital_offset2) << 21) >> 21,
						((int)(tvafe->cal.cal_val.c_digital_offset2) << 21) >> 21);
				pr_info("\n");
#endif
				if(ret)
				{
					ret = -EFAULT;
					pr_info("[tvafe..] %s, the command %d error,adc calibriation error.\n", __func__, cmd);
					break;
				}
				if (copy_to_user(argp, &tvafe->cal.cal_val, sizeof(struct tvafe_adc_cal_s)))
				{
					ret = -EFAULT;
					break;
				}

				break;
			}
		case TVIN_IOC_G_AFE_COMP_WSS:
			{
				if (copy_to_user(argp, &tvafe->comp_wss, sizeof(struct tvafe_comp_wss_s)))
				{
					ret = -EFAULT;
					break;
				}
				break;
			}
		case TVIN_IOC_S_AFE_VGA_EDID:
			{
				if (copy_from_user(&edid, argp, sizeof(struct tvafe_vga_edid_s)))
				{
					ret = -EFAULT;
					break;
				}
#ifdef LOG_VGA_EDID
				for(i=0; i<32; i++)
				{
					pr_info("0x%2x 0x%2x 0x%2x 0x%2x 0x%2x 0x%2x 0x%2x 0x%2x.\n", edid.value[(i<<3)+0],
							edid.value[(i<<3)+1], edid.value[(i<<3)+2], edid.value[(i<<3)+3], edid.value[(i<<3)+4],
							edid.value[(i<<3)+5], edid.value[(i<<3)+6], edid.value[(i<<3)+7] );
				}
#endif
				tvafe_vga_set_edid(&edid);

				break;
			}
		case TVIN_IOC_G_AFE_VGA_EDID:
			{
				tvafe_vga_get_edid(&edid);
				if (copy_to_user(argp, &edid, sizeof(struct tvafe_vga_edid_s)))
				{
					ret = -EFAULT;
					break;
				}
				break;
			}
		case TVIN_IOC_S_AFE_VGA_PARM:
			{
				if (copy_from_user(&tvafe->adc.vga_parm, argp, sizeof(struct tvafe_vga_parm_s)))
				{
					ret = -EFAULT;
					break;
				}
				tvafe_vga_auto_adjust_disable(&tvafe->adc);

				break;
			}
		case TVIN_IOC_G_AFE_VGA_PARM:
			{
				if (copy_to_user(argp, &tvafe->adc.vga_parm, sizeof(struct tvafe_vga_parm_s)))
				{
					ret = -EFAULT;
					break;
				}
				break;
			}
		case TVIN_IOC_S_AFE_VGA_AUTO:
			{
				ret = tvafe_vga_auto_adjust_enable(&tvafe->adc);
				break;
			}
		case TVIN_IOC_G_AFE_CMD_STATUS:
			{
				if (copy_to_user(argp, &tvafe->adc.cmd_status, sizeof(enum tvafe_cmd_status_e)))
				{
					ret = -EFAULT;
					break;
				}
				if ((tvafe->adc.cmd_status == TVAFE_CMD_STATUS_SUCCESSFUL) ||
						(tvafe->adc.cmd_status == TVAFE_CMD_STATUS_FAILED)     ||
						(tvafe->adc.cmd_status == TVAFE_CMD_STATUS_TERMINATED))
				{
					tvafe->adc.cmd_status = TVAFE_CMD_STATUS_IDLE;
				}

				break;
			}
		case TVIN_IOC_G_AFE_CVBS_LOCK:
			{
				cvbs_lock_status = tvafe_cvd2_get_lock_status(&tvafe->cvd2);
				if (copy_to_user(argp, &cvbs_lock_status, sizeof(int)))
				{
					ret = -EFAULT;
					break;
				}
				pr_info("[tvafe..] %s: get cvd2 lock status :%d. \n", __func__, cvbs_lock_status);
				break;
			}
		case TVIN_IOC_S_AFE_CVBS_STD:
			{
				enum tvin_sig_fmt_e fmt = TVIN_SIG_FMT_NULL;

				if (copy_from_user(&fmt, argp, sizeof(enum tvin_sig_fmt_e))) {
					ret = -EFAULT;
					break;
				}
				tvafe->cvd2.manual_fmt = fmt;
				pr_info("[tvafe..] %s: ioctl set cvd2 manual fmt:%s. \n", __func__, tvin_sig_fmt_str(fmt));
				break;
			}
		default:
			ret = -ENOIOCTLCMD;
			break;
	}

	mutex_unlock(&devp->afe_mutex);
	return ret;
}

static int tvafe_mmap(struct file *file, struct vm_area_struct * vma)
{
	unsigned long start, len, off;
	unsigned long pfn, size;
	tvafe_dev_t *devp = file->private_data;

	if (vma->vm_pgoff > (~0UL >> PAGE_SHIFT)) {
		return -EINVAL;
	}

	/* capture the vbi data  */
	start = (devp->mem.start + (DECODER_VBI_ADDR_OFFSET << 3)) & PAGE_MASK;
	len = PAGE_ALIGN((start & ~PAGE_MASK) + (DECODER_VBI_VBI_SIZE << 3));

	off = vma->vm_pgoff << PAGE_SHIFT;

	if ((vma->vm_end - vma->vm_start + off) > len) {
		return -EINVAL;
	}

	off += start;
	vma->vm_pgoff = off >> PAGE_SHIFT;

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	vma->vm_flags |= VM_IO | VM_RESERVED;

	size = vma->vm_end - vma->vm_start;
	pfn  = off >> PAGE_SHIFT;

	if (io_remap_pfn_range(vma, vma->vm_start, pfn, size, vma->vm_page_prot))
		return -EAGAIN;

	return 0;
}

/* File operations structure. Defined in linux/fs.h */
static struct file_operations tvafe_fops = {
	.owner   = THIS_MODULE,         /* Owner */
	.open    = tvafe_open,          /* Open method */
	.release = tvafe_release,       /* Release method */
	.unlocked_ioctl   = tvafe_ioctl,         /* Ioctl method */
	.mmap    = tvafe_mmap,
	/* ... */
};

static int tvafe_add_cdev(struct cdev *cdevp, struct file_operations *fops,
		int minor)
{
	int ret;
	dev_t devno = MKDEV(MAJOR(tvafe_devno), minor);
	cdev_init(cdevp, fops);
	cdevp->owner = THIS_MODULE;
	ret = cdev_add(cdevp, devno, 1);
	return ret;
}

static struct device * tvafe_create_device(struct device *parent, int id)
{
	dev_t devno = MKDEV(MAJOR(tvafe_devno),  id);
	return device_create(tvafe_clsp, parent, devno, NULL, "%s0",
			TVAFE_DEVICE_NAME);
	/* @to do this after Middleware API modified */
	/*return device_create(tvafe_clsp, parent, devno, NULL, "%s",
	  TVAFE_DEVICE_NAME); */
}

static void tvafe_delete_device(int minor)
{
	dev_t devno = MKDEV(MAJOR(tvafe_devno), minor);
	device_destroy(tvafe_clsp, devno);
}

static int tvafe_drv_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct tvafe_dev_s *tdevp;
	struct resource *res;
	//struct tvin_frontend_s * frontend;

	/* allocate memory for the per-device structure */
	tdevp = kmalloc(sizeof(struct tvafe_dev_s), GFP_KERNEL);
	if (!tdevp){
		pr_err("tvafe: failed to allocate memory for tvafe device\n");
		goto fail_kmalloc_tdev;
	}
	memset(tdevp, 0, sizeof(struct tvafe_dev_s));

	/*@to get from bsp*/
	if(pdev->id == -1){
		tdevp->index = 0;
	}
	else{
		pr_err("%s: failed to get device id\n", __func__);
		goto fail_get_id;
	}
	tdevp->flags = 0;

	/* create cdev and reigser with sysfs */
	ret = tvafe_add_cdev(&tdevp->cdev, &tvafe_fops, tdevp->index);
	if (ret) {
		pr_err("%s: failed to add cdev\n", __func__);
		goto fail_add_cdev;
	}	
	/* create /dev nodes */
	tdevp->dev = tvafe_create_device(NULL, tdevp->index);
	if (IS_ERR(tdevp->dev)) {
		pr_err("tvafe: failed to create device node\n");
		/* @todo do with error */
		ret = PTR_ERR(tdevp->dev);
		goto fail_create_device;
	}

	/*create sysfs attribute files*/
	ret = device_create_file(tdevp->dev,&dev_attr_debug);
	if(ret < 0) {
		pr_err("tvafe: fail to create dbg attribute file\n");
		goto fail_create_dbg_file;
	}
	
	/* get device memory */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		pr_err("tvafe: can't get memory resource\n");
		ret = -EFAULT;
		goto fail_get_resource_mem;
	}

	tdevp->mem.start = res->start;
	tdevp->mem.size = res->end - res->start + 1;
	pr_info(" tvafe cvd memory addr is:0x%x, cvd mem_size is:0x%x . \n",
			tdevp->mem.start,
			tdevp->mem.size);
	tdevp->pinmux = pdev->dev.platform_data;
	if (!tdevp->pinmux) {
		pr_err("tvafe: no platform data!\n");
		ret = -ENODEV;
	}

	/* frontend */
	tvin_frontend_init(&tdevp->frontend, &tvafe_dec_ops, &tvafe_sm_ops, tdevp ->index);
	sprintf(tdevp->frontend.name, "%s", TVAFE_NAME);
	tvin_reg_frontend(&tdevp->frontend);

	mutex_init(&tdevp->afe_mutex);

	/* set APB bus register accessing error exception */
	tvafe_set_apb_bus_err_ctrl();	
	dev_set_drvdata(tdevp->dev, tdevp);
        platform_set_drvdata(pdev,tdevp);

    /**disable tvafe clock**/
    tvafe_enable_module(false);

	pr_info("tvafe: driver probe ok\n");
	return 0;

fail_create_dbg_file:
fail_get_resource_mem:
	tvafe_delete_device(tdevp->index);
fail_create_device:
	cdev_del(&tdevp->cdev);
fail_add_cdev:
fail_get_id:
	kfree(tdevp);
fail_kmalloc_tdev:
	return ret;

}

static int tvafe_drv_remove(struct platform_device *pdev)
{
	struct tvafe_dev_s *tdevp;
	tdevp = platform_get_drvdata(pdev);

	mutex_destroy(&tdevp->afe_mutex);
	tvin_unreg_frontend(&tdevp->frontend);
	device_remove_file(tdevp->dev, &dev_attr_debug);
	tvafe_delete_device(tdevp->index);
	cdev_del(&tdevp->cdev);
	kfree(tdevp);
	pr_info("tvafe: driver removed ok.\n");
	return 0;
}

#ifdef CONFIG_PM
static int tvafe_drv_suspend(struct platform_device *pdev,pm_message_t state)
{
	struct tvafe_dev_s *tdevp;
	tdevp = platform_get_drvdata(pdev);

	/* close afe port first */
    if (tdevp->flags & TVAFE_FLAG_DEV_OPENED)
    {
        pr_info("tvafe: suspend module, close afe port first\n");
        //tdevp->flags &= (~TVAFE_FLAG_DEV_OPENED);
        del_timer_sync(&tdevp->timer);

        /**set cvd2 reset to high**/
        tvafe_cvd2_hold_rst(&tdevp->tvafe.cvd2);
        /**disable av out**/
        tvafe_enable_avout(false);
    }

    /*disable and reset tvafe clock*/
    tvafe_enable_module(false);

    pr_info("tvafe: suspend module\n");

	return 0;
}

static int tvafe_drv_resume(struct platform_device *pdev)
{
	struct tvafe_dev_s *tdevp;
	tdevp = platform_get_drvdata(pdev);

        /*disable and reset tvafe clock*/
        tvafe_enable_module(true);
	pr_info("tvafe: resume module\n");
	return 0;
}
#endif

static struct platform_driver tvafe_driver = {
	.probe      = tvafe_drv_probe,
	.remove     = tvafe_drv_remove,
#ifdef CONFIG_PM
	.suspend    = tvafe_drv_suspend,
	.resume     = tvafe_drv_resume,
#endif
	.driver     = {
		.name   = TVAFE_DRIVER_NAME,
	}
};

static int __init tvafe_drv_init(void)
{
	int ret = 0;

	ret = alloc_chrdev_region(&tvafe_devno, 0, 1, TVAFE_NAME);
	if (ret < 0) {
		pr_err("%s: failed to allocate major number\n", __func__);
		goto fail_alloc_cdev_region;
	}
	pr_info("%s: major %d\n", __func__, MAJOR(tvafe_devno));

	tvafe_clsp = class_create(THIS_MODULE, TVAFE_NAME);
	if (IS_ERR(tvafe_clsp))
	{
		ret = PTR_ERR(tvafe_clsp);
		pr_err("%s: failed to create class\n", __func__);
		goto fail_class_create;
	}

	ret = platform_driver_register(&tvafe_driver);
	if (ret != 0) {
		pr_err("%s: failed to register driver\n", __func__);
		goto fail_pdrv_register;
	}
	pr_info("tvafe: tvafe_init.\n");
	return 0;

fail_pdrv_register:
	class_destroy(tvafe_clsp);
fail_class_create:
	unregister_chrdev_region(tvafe_devno, 1);
fail_alloc_cdev_region:
	return ret;


}

static void __exit tvafe_drv_exit(void)
{
	class_destroy(tvafe_clsp);
	unregister_chrdev_region(tvafe_devno, 1);
	platform_driver_unregister(&tvafe_driver);
	pr_info("tvafe: tvafe_exit.\n");
}

module_init(tvafe_drv_init);
module_exit(tvafe_drv_exit);

MODULE_DESCRIPTION("AMLOGIC TVAFE driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Xu Lin <lin.xu@amlogic.com>");

