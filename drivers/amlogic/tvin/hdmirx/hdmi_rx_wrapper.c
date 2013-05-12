/*
 * Amlogic M6TV
 * HDMI RX
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
 */


#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/mm.h>
#include <linux/major.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/cdev.h>
//#include <linux/amports/canvas.h>
#include <asm/uaccess.h>
#include <asm/delay.h>
#include <mach/regs.h>
#include <mach/clock.h>
#include <mach/register.h>
#include <mach/power_gate.h>


#include <linux/tvin/tvin.h>
/* Local include */
#include "hdmirx_drv.h"
#include "hdmi_rx_reg.h"

#define HDMI_STATE_CHECK_FREQ     (20*5)
#define HW_MONITOR_TIME_UNIT    (1000/HDMI_STATE_CHECK_FREQ)
//static int init = 0;
static int audio_enable = 1;
static int sample_rate_change_th = 100;
static int audio_stable_time = 1000; // audio_sample_rate is stable <--> enable audio gate
static int audio_sample_rate_stable_count_th = 15; // audio change <--> audio_sample_rate is stable
static unsigned local_port = 0;
static int sig_ready_count = 0;
static int sig_unlock_count = 0;
static int sig_unstable_count = 0;
/**
 *  HDMI RX parameters
 */
static int port_map = 0x3210;
static int cfg_clk = 25*1000; //62500
static int lock_thres = LOCK_THRES;
static int fast_switching = 1;
static int fsm_enhancement = 1;
static int port_select_ovr_en = 0;
static int phy_cmu_config_force_val = 0;  
static int phy_system_config_force_val = 0;
static int acr_mode = 0;                         // Select which ACR scheme:
                                                    // 0=Analog PLL based ACR;
                                                    // 1=Digital ACR.
static int edid_mode = 0;
static int switch_mode = 0x1;
static int force_vic = 0;
static int force_ready = 0;
static int force_state = 0;
static int force_format = 0;
static int audio_sample_rate = 0;
static int frame_rate = 0;
int hdcp_enable = 1;
int hdmirx_debug_flag = 0;
/* bit 0, printk; bit 8 enable irq log */
int hdmirx_log_flag = 0x1; //0x101;
int t3d_flash_flag = 0;
int t3d_flash_flag_me = 1;

/***********************
  TVIN driver interface
************************/
#define HDMIRX_HWSTATE_INIT                0
#define HDMIRX_HWSTATE_5V_LOW             1
#define HDMIRX_HWSTATE_5V_HIGH            2
#define HDMIRX_HWSTATE_HPD_READY          3
#define HDMIRX_HWSTATE_SIG_UNSTABLE        4
#define HDMIRX_HWSTATE_SIG_STABLE          5
#define HDMIRX_HWSTATE_SIG_READY           6



struct rx rx;

static unsigned long tmds_clock_old = 0;

/** TMDS clock delta [kHz] */
#define TMDS_CLK_DELTA			(125)
/** Pixel clock minimum [kHz] */
#define PIXEL_CLK_MIN			TMDS_CLK_MIN
/** Pixel clock maximum [kHz] */
#define PIXEL_CLK_MAX			TMDS_CLK_MAX
/** Horizontal resolution minimum */
#define HRESOLUTION_MIN			(320)
/** Horizontal resolution maximum */
#define HRESOLUTION_MAX			(4096)
/** Vertical resolution minimum */
#define VRESOLUTION_MIN			(240)
/** Vertical resolution maximum */
#define VRESOLUTION_MAX			(4455)
/** Refresh rate minimum [Hz] */
#define REFRESH_RATE_MIN		(100)
/** Refresh rate maximum [Hz] */
#define REFRESH_RATE_MAX		(25000)

#define TMDS_TOLERANCE  (4000)



static int hdmi_rx_ctrl_edid_update(void);
static void dump_state(unsigned char enable);
static unsigned int get_vic_from_timing(struct hdmi_rx_ctrl_video* video_par);
void hdmirx_hw_init2(void);




/**
 * Clock event handler
 * @param[in,out] ctx context information
 * @return error code
 */
 
static long hdmi_rx_ctrl_get_tmds_clk(struct hdmi_rx_ctrl *ctx)
{
	return ctx->tmds_clk;
}
 
static int clock_handler(struct hdmi_rx_ctrl *ctx)
{
	int error = 0;
	unsigned long tclk = 0;
	//struct hdmi_rx_ctrl_video v;
	if(hdmirx_log_flag&0x100)
	hdmirx_print("%s \n", __func__);

	if (ctx == 0)
	{
		return -EINVAL;
	}
	tclk = hdmi_rx_ctrl_get_tmds_clk(ctx);
	//hdmirx_get_video_info(ctx, &rx.video_params);
	if (((tmds_clock_old + TMDS_TOLERANCE) > tclk) &&
		((tmds_clock_old - TMDS_TOLERANCE) < tclk))
	{
		return 0;
	}
	if ((tclk == 0) && (tmds_clock_old != 0))
	{
		//if(video_format_change())
		//	rx.change = true;
		if(hdmirx_log_flag & 0x200){
		printk("HDMI mode change: clk change\n");
		}
		/* TODO Review if we need to turn off the display
		video_if_mode(false, 0, 0, 0);
		*/
#if 0
		/* workaround for sticky HDMI mode */
		error |= rx_ctrl_config(ctx, rx.port, &rx.hdcp);
#endif
	}
	else
	{
#if 0
		error |= hdmi_rx_phy_config(&rx.phy, rx.port, tclk, v.deep_color_mode);
#endif
	}
	tmds_clock_old = ctx->tmds_clk;
//#if MESSAGES
	//ctx->log_info("TMDS clock: %3u.%03uMHz",
	//		ctx->tmds_clk / 1000, ctx->tmds_clk % 1000);
//#endif
	return error;
}

static unsigned char is_3d_sig(void)
{
	if((rx.vendor_specific_info.identifier == 0x000c03)&&
	(rx.vendor_specific_info.hdmi_video_format == 0x2)){
		return 1;
	}
		return 0;
}


/*
*make sure if video format change
*/
bool video_format_change(void)
{
	if(is_3d_sig()){	//only for 3D sig
		if (((rx.cur_video_params.hactive + 5) < (rx.reltime_video_params.hactive)) ||
			((rx.cur_video_params.hactive - 5) > (rx.reltime_video_params.hactive)) ||
			((rx.cur_video_params.vactive + 5) < (rx.reltime_video_params.vactive)) ||
			((rx.cur_video_params.vactive - 5) > (rx.reltime_video_params.vactive)) ||
			(rx.cur_video_params.pixel_repetition != rx.reltime_video_params.pixel_repetition)) {

			if(hdmirx_log_flag&0x200){
				printk("HDMI mode change: hactive(%d=>%d), vactive(%d=>%d), pixel_repeat(%d=>%d), video_format(%d=>%d),video_VIC(%d=>%d)\n",
				rx.cur_video_params.hactive, rx.reltime_video_params.hactive,
				rx.cur_video_params.vactive, rx.reltime_video_params.vactive,
				rx.cur_video_params.pixel_repetition, rx.reltime_video_params.pixel_repetition,
				rx.cur_video_params.video_format, rx.reltime_video_params.video_format,
				rx.cur_video_params.video_mode, rx.reltime_video_params.video_mode);
			}
			return true;
			
		} else {
			return false;
		}
	} else {	// for 2d sig
		return true;
	}
}

/**
 * Video event handler
 * @param[in,out] ctx context information
 * @return error code
 */
static int video_handler(struct hdmi_rx_ctrl *ctx)
{

	int error = 0;
	int i = 0;
	struct hdmi_rx_ctrl_video v;

	if(hdmirx_log_flag&0x100)
	hdmirx_print("%s \n", __func__);
	if (ctx == 0)
	{
		return -EINVAL;
	}
	/* wait for the video mode is stable */
	for (i = 0; i < 5000000; i++)
	{
		;
	}

	error |= hdmirx_get_video_info(ctx, &v);
	if ((error == 0) &&
			(((rx.pre_video_params.hactive + 5) < (v.hactive)) ||
			((rx.pre_video_params.hactive - 5) > (v.hactive)) ||
			((rx.pre_video_params.vactive + 5) < (v.vactive)) ||
			((rx.pre_video_params.vactive - 5) > (v.vactive)) ||
			(rx.pre_video_params.pixel_repetition != v.pixel_repetition))
		)
	{

		if ((rx.state == HDMIRX_HWSTATE_SIG_READY)&&(t3d_flash_flag == 1)) {
			if (t3d_flash_flag_me == 1){
				printk("[HDMIRX]-----t3d_flash_flag set for test\n");
				t3d_flash_flag_me = 0;
			}
		} else {
			if (get_vic_from_timing(&v) !=0) {
				printk("VIC--->%d\n",v.video_mode);
				rx.change = true;
			}
		}
		if(hdmirx_log_flag&0x200){
			printk("HDMI mode change: hactive(%d=>%d), vactive(%d=>%d), pixel_repeat(%d=>%d), video_format(%d=>%d)\n",
			rx.pre_video_params.hactive, v.hactive,
			rx.pre_video_params.vactive, v.vactive,
			rx.pre_video_params.pixel_repetition, v.pixel_repetition,
			rx.pre_video_params.video_format, v.video_format);
		}

		if (get_vic_from_timing(&v) !=0) {
			rx.reltime_video_params.deep_color_mode = v.deep_color_mode;
			rx.reltime_video_params.htotal = v.htotal;
			rx.reltime_video_params.vtotal = v.vtotal;
			rx.reltime_video_params.pixel_clk = v.pixel_clk;
			rx.reltime_video_params.refresh_rate = v.refresh_rate;
			rx.reltime_video_params.hactive = v.hactive;
			rx.reltime_video_params.vactive = v.vactive;
			rx.reltime_video_params.pixel_repetition = v.pixel_repetition;
			rx.reltime_video_params.video_mode = v.video_mode;
			rx.reltime_video_params.video_format = v.video_format;
			rx.reltime_video_params.dvi = v.dvi;
		}
	}

	rx.pre_video_params.deep_color_mode = v.deep_color_mode;
	rx.pre_video_params.htotal = v.htotal;
	rx.pre_video_params.vtotal = v.vtotal;
	rx.pre_video_params.pixel_clk = v.pixel_clk;
	rx.pre_video_params.refresh_rate = v.refresh_rate;
	rx.pre_video_params.hactive = v.hactive;
	rx.pre_video_params.vactive = v.vactive;
	rx.pre_video_params.pixel_repetition = v.pixel_repetition;
	rx.pre_video_params.video_mode = v.video_mode;
	rx.pre_video_params.video_format = v.video_format;
	rx.pre_video_params.dvi = v.dvi;
	
	return error;
}

static int vsi_handler(void)
{
	//hdmirx_read_vendor_specific_info_frame(&rx.vendor_specific_info.identifier);
	hdmirx_read_vendor_specific_info_frame(&rx.vendor_specific_info);
	return 0;
}

/**
 * Audio event handler
 * @param[in,out] ctx context information
 * @return error code
 */
#if 0
static int audio_handler(struct hdmi_rx_ctrl *ctx)
{
	int error = 0;
  /*
	struct hdmi_rx_ctrl_audio a;
  if(hdmirx_log_flag&0x100)
    hdmirx_print("%s \n", __func__);

	if (ctx == 0)
	{
		return -EINVAL;
	}
	error |= hdmi_rx_ctrl_get_audio(ctx, &a);
	if (error == 0)
	{
		ctx->log_info("Audio: CT=%u CC=%u SF=%u SS=%u CA=%u",
				a.coding_type, a.channel_count, a.sample_frequency,
				a.sample_size, a.channel_allocation);
	}
	*/
	//hdmirx_read_audio_info();
	
	return error;
}
#endif

static int hdmi_rx_ctrl_irq_handler(struct hdmi_rx_ctrl *ctx)
{
	int error = 0;
	unsigned i = 0;
	uint32_t intr = 0;
	uint32_t data = 0;
	unsigned long tclk = 0;
	unsigned long ref_clk;
	unsigned evaltime = 0;

	bool clk_handle_flag = false;
	bool video_handle_flag = false;
	bool audio_handle_flag = false;
	bool vsi_handle_flag = false;
	ref_clk = ctx->md_clk;
	intr = hdmirx_rd_dwc(RA_HDMI_ISTS) & hdmirx_rd_dwc(RA_HDMI_IEN);
	if (intr != 0) {
		hdmirx_wr_dwc(RA_HDMI_ICLR, intr);
		if (get(intr, CLK_CHANGE) != 0) {
			clk_handle_flag = true;
			evaltime = (ref_clk * 4095) / 158000;
			data = hdmirx_rd_dwc(RA_HDMI_CKM_RESULT);
			tclk = ((ref_clk * get(data, CLKRATE)) / evaltime);
			if(hdmirx_log_flag&0x100)
			hdmirx_print("HDMI CLK_CHANGE (%d) irq\n", tclk);
			if (tclk == 0) {
				error |= hdmirx_interrupts_hpd(false);
				error |= hdmirx_control_clk_range(TMDS_CLK_MIN, TMDS_CLK_MAX);
			}
			else {
				for (i = 0; i < TMDS_STABLE_TIMEOUT; i++) { /* time for TMDS to stabilise */
					;
				}
				tclk = ((ref_clk * get(data, CLKRATE)) / evaltime);
				error |= hdmirx_control_clk_range(tclk - TMDS_CLK_DELTA, tclk + TMDS_CLK_DELTA);
				error |= hdmirx_interrupts_hpd(true);
			}
			ctx->tmds_clk = tclk;
		}
		if (get(intr, DCM_CURRENT_MODE_CHG) != 0) {
			if(hdmirx_log_flag&0x100)
				hdmirx_print("HDMI DCM_CURRENT_MODE_CHG irq\n");
			video_handle_flag = true;
		}
		if (get(intr, AKSV_RCV) != 0) {
			if(hdmirx_log_flag&0x100)
				hdmirx_print("HDMI AKSV_RCV irq\n");
				//execute[hdmi_rx_ctrl_event_aksv_reception] = true;
		}
		ctx->debug_irq_hdmi++;
	}
	intr = hdmirx_rd_dwc(RA_MD_ISTS) & hdmirx_rd_dwc(RA_MD_IEN);
	if (intr != 0) {
	if(hdmirx_log_flag&0x100)
		hdmirx_print("HDMI MD irq %x\n", intr);
		hdmirx_wr_dwc(RA_MD_ICLR, intr);
		if (get(intr, VIDEO_MODE) != 0) {
			video_handle_flag = true;
		}
		ctx->debug_irq_video_mode++;
	}
	intr = hdmirx_rd_dwc(RA_PDEC_ISTS) & hdmirx_rd_dwc(RA_PDEC_IEN);
	if (intr != 0) {
		hdmirx_wr_dwc(RA_PDEC_ICLR, intr);
		if (get(intr, DVIDET|AVI_CKS_CHG) != 0) {
			if(hdmirx_log_flag&0x100)
				hdmirx_print("HDMI AVI_CKS_CHG irq\n");
			video_handle_flag = true;
		}
		if (get(intr, VSI_CKS_CHG) != 0) {
			if(hdmirx_log_flag&0x100)
				hdmirx_print("HDMI VSI_CKS_CHG irq\n");
			vsi_handle_flag = true;
		}
		if (get(intr, AIF_CKS_CHG) != 0) {
			if(hdmirx_log_flag&0x100)
				hdmirx_print("HDMI AIF_CKS_CHG irq\n");
			audio_handle_flag = true;
		}
		if (get(intr, PD_FIFO_NEW_ENTRY) != 0) {
			if(hdmirx_log_flag&0x100)
				hdmirx_print("HDMI PD_FIFO_NEW_ENTRY irq\n");
			//execute[hdmi_rx_ctrl_event_packet_reception] = true;
		}
		if (get(intr, PD_FIFO_OVERFL) != 0) {
			if(hdmirx_log_flag&0x100)
				hdmirx_print("HDMI PD_FIFO_OVERFL irq\n");
			error |= hdmirx_packet_fifo_rst();
		}
		ctx->debug_irq_packet_decoder++;
	}
	intr = hdmirx_rd_dwc(RA_AUD_CLK_ISTS) & hdmirx_rd_dwc(RA_AUD_CLK_IEN);
	if (intr != 0) {
		if(hdmirx_log_flag&0x100)
			hdmirx_print("HDMI RA_AUD_CLK irq\n");
		hdmirx_wr_dwc(RA_AUD_CLK_ICLR, intr);
		ctx->debug_irq_audio_clock++;
	}
	intr = hdmirx_rd_dwc(RA_AUD_FIFO_ISTS) & hdmirx_rd_dwc(RA_AUD_FIFO_IEN);
	if (intr != 0) {
		hdmirx_wr_dwc(RA_AUD_FIFO_ICLR, intr);
		if (get(intr, AFIF_OVERFL|AFIF_UNDERFL) != 0) {
			if(hdmirx_log_flag&0x100)
				hdmirx_print("HDMI AFIF_OVERFL|AFIF_UNDERFL irq\n");
			error |= hdmirx_audio_fifo_rst();
		}
		ctx->debug_irq_audio_fifo++;
	}

	if(clk_handle_flag){
		clock_handler(ctx);
	}
	if(video_handle_flag){
		video_handler(ctx);
	}
	if(vsi_handle_flag){
		vsi_handler();
	}
	return error;
}


static irqreturn_t irq_handler(int irq, void *params)
{

	int error = 0;
	unsigned long hdmirx_top_intr_stat;
	if (params == 0)
	{
		
		pr_info("%s: %s\n", __func__, "RX IRQ invalid parameter");
		return IRQ_HANDLED;
	}

	hdmirx_top_intr_stat = hdmirx_rd_top(HDMIRX_TOP_INTR_STAT);
	hdmirx_wr_top(HDMIRX_TOP_INTR_STAT_CLR, hdmirx_top_intr_stat); // clear interrupts in HDMIRX-TOP module 

	if (hdmirx_top_intr_stat & (0xf << 2)) {    // [5:2] hdmirx_5v_rise
		//rx.tx_5v_status = true;
		printk("HDMI 5v rise irq\n");
	} /* if (hdmirx_top_intr_stat & (0xf << 2)) // [5:2] hdmirx_5v_rise */

	if (hdmirx_top_intr_stat & (0xf << 6)) {    // [9:6] hdmirx_5v_fall
		//rx.tx_5v_status = false;
		printk("HDMI 5v fall irq\n");
	} /* if (hdmirx_top_intr_stat & (0xf << 6)) // [9:6] hdmirx_5v_fall */

	if(hdmirx_top_intr_stat & (1 << 31)){
		error = hdmi_rx_ctrl_irq_handler(&((struct rx *)params)->ctrl);
		if (error < 0)
		{
			if (error != -EPERM)
			{
				pr_info("%s: RX IRQ handler %d\n", __func__, error);
			}
		}
	}
	return IRQ_HANDLED;

}



typedef struct{
	unsigned int sample_rate;
	unsigned char aud_info_sf;
	unsigned char channel_status_id;
}sample_rate_info_t;

sample_rate_info_t sample_rate_info[]=
{
	{32000,  0x1,  0x3},
	{44100,  0x2,  0x0},
	{48000,  0x3,  0x2},
	{88200,  0x4,  0x8},
	{96000,  0x5,  0xa},
	{176400, 0x6,  0xc},
	{192000, 0x7,  0xe},
	//{768000, 0, 0x9},
	{0, 0, 0}
};

static int get_real_sample_rate(void)
{
    int i;
    int ret_sample_rate = rx.aud_info.audio_recovery_clock;
    for(i=0; sample_rate_info[i].sample_rate; i++){
        if(rx.aud_info.audio_recovery_clock > sample_rate_info[i].sample_rate){
            if((rx.aud_info.audio_recovery_clock-sample_rate_info[i].sample_rate)<sample_rate_change_th){
                ret_sample_rate = sample_rate_info[i].sample_rate;
                break;
            }
        }
        else{
            if((sample_rate_info[i].sample_rate - rx.aud_info.audio_recovery_clock)<sample_rate_change_th){
                ret_sample_rate = sample_rate_info[i].sample_rate;
                break;
            }
        }
    }
    return ret_sample_rate;
}

static unsigned char is_sample_rate_change(int sample_rate_pre, int sample_rate_cur)
{
    unsigned char ret = 0;
    if((sample_rate_cur!=0)&&
        (sample_rate_cur>31000)&&(sample_rate_cur<193000)){
        if(sample_rate_pre > sample_rate_cur){
            if((sample_rate_pre - sample_rate_cur)> sample_rate_change_th){
                ret = 1;
            }
        }
        else{
            if((sample_rate_cur - sample_rate_pre)> sample_rate_change_th){
                ret = 1;
            }
        }
    }
    return ret;
}

 
 
bool hdmirx_hw_check_frame_skip(void)
{
	if(force_state&0x10)
		return 0;
	
	return rx.change;
}

int hdmirx_hw_get_color_fmt(void)
{
	int color_format = 0;
	int format = rx.video_params.video_format;
	if(force_format&0x10){
		format = force_format&0xf;
	}
	switch(format){
	case 1:
		color_format = 3; /* YUV422 */
		break;
	case 2:
		color_format = 1; /* YUV444*/ 
		break;
	case 0:
	default:
		color_format = 0; /* RGB444 */
		break;
		}
	return color_format;
}

int hdmirx_hw_get_dvi_info(void)
{
	int ret = 0;
	
	unsigned int vic = rx.video_params.video_mode;
	if(rx.video_params.dvi||vic==0){
		ret = 1;
	}
	return ret;
}

int hdmirx_hw_get_3d_structure(unsigned char* _3d_structure, unsigned char* _3d_ext_data)
{
	if((rx.vendor_specific_info.identifier == 0x000c03)&&
	(rx.vendor_specific_info.hdmi_video_format == 0x2)){
	*_3d_structure = rx.vendor_specific_info._3d_structure;
	*_3d_ext_data = rx.vendor_specific_info._3d_ext_data;
	return 0;
	}
	return -1;
}

int hdmirx_hw_get_pixel_repeat(void)
{
	return (rx.video_params.pixel_repetition+1);
}


static unsigned char is_frame_packing(void)
{
	if((rx.vendor_specific_info.identifier == 0x000c03)&&
	(rx.vendor_specific_info.hdmi_video_format == 0x2)&&
	(rx.vendor_specific_info._3d_structure == 0x0)){
	return 1;
	}
	return 0;
}

typedef struct{
	unsigned int vic;
	unsigned char vesa_format;
	unsigned int ref_freq; /* 8 bit tmds clock */
	unsigned int active_pixels;
	unsigned int active_lines;
	unsigned int active_lines_fp;
}freq_ref_t;

static freq_ref_t freq_ref[]=
{
/* basic format*/
	{HDMI_640x480p60, 0, 25000, 640, 480, 480},
	{HDMI_480p60, 0, 27000, 720, 480, 1005},
	{HDMI_480p60_16x9, 0, 27000, 720, 480, 1005},
	{HDMI_480i60, 0, 27000, 1440, 240, 240},
	{HDMI_480i60_16x9, 0, 27000, 1440, 240, 240},
	{HDMI_576p50, 0, 27000, 720, 576, 1201},
	{HDMI_576p50_16x9, 0, 27000, 720, 576, 1201},
	{HDMI_576i50, 0, 27000, 1440, 288, 288},
	{HDMI_576i50_16x9, 0, 27000, 1440, 288, 288},
	{HDMI_576i50_16x9, 0, 27000, 1440, 145, 145},
	{HDMI_720p60, 0, 74250, 1280, 720, 1470},
	{HDMI_720p50, 0, 74250, 1280, 720, 1470},
	{HDMI_1080i60, 0, 74250, 1920, 540, 2228},
	{HDMI_1080i50, 0, 74250, 1920, 540, 2228},
	{HDMI_1080p60, 0, 148500, 1920, 1080, 1080},
	{HDMI_1080p24, 0, 74250, 1920, 1080, 2205},
	{HDMI_1080p25, 0, 74250, 1920, 1080, 2205},
	{HDMI_1080p30, 0, 74250, 1920, 1080, 2205},
	{HDMI_1080p50, 0, 148500, 1920, 1080, 1080},
/* extend format */
	{HDMI_1440x240p60, 0, 27000, 1440, 240, 240},      //vic 8
	{HDMI_1440x240p60_16x9, 0, 27000, 1440, 240, 240}, //vic 9
	{HDMI_2880x480i60, 0, 54000, 2880, 240, 240},      //vic 10
	{HDMI_2880x480i60_16x9, 0, 54000, 2880, 240, 240}, //vic 11
	{HDMI_2880x240p60, 0, 54000, 2880, 240, 240},      //vic 12
	{HDMI_2880x240p60_16x9, 0, 54000, 2880, 240, 240}, //vic 13
	{HDMI_1440x480p60, 0, 54000, 1440, 480, 480},      //vic 14
	{HDMI_1440x480p60_16x9, 0, 54000, 1440, 480, 480}, //vic 15

	{HDMI_1440x288p50, 0, 27000, 1440, 288, 288},      //vic 23
	{HDMI_1440x288p50_16x9, 0, 27000, 1440, 288, 288}, //vic 24
	{HDMI_2880x576i50, 0, 54000, 2880, 288, 288},      //vic 25
	{HDMI_2880x576i50_16x9, 0, 54000, 2880, 288, 288}, //vic 26
	{HDMI_2880x288p50, 0, 54000, 2880, 288, 288},      //vic 27
	{HDMI_2880x288p50_16x9, 0, 54000, 2880, 288, 288}, //vic 28
	{HDMI_1440x576p50, 0, 54000, 1440, 576, 576},      //vic 29
	{HDMI_1440x576p50_16x9, 0, 54000, 1440, 576, 576}, //vic 30

	{HDMI_2880x480p60, 0, 108000, 2880, 480, 480},     //vic 35
	{HDMI_2880x480p60_16x9, 0, 108000, 2880, 480, 480},//vic 36
	{HDMI_2880x576p50, 0, 108000, 2880, 576, 576},     //vic 37
	{HDMI_2880x576p50_16x9, 0, 108000, 2880, 576, 576},//vic 38
	{HDMI_1080i50_1250, 0, 72000, 1920, 540, 540},     //vic 39
	{HDMI_720p24, 0, 74250, 1280, 720, 1470},          //vic 60
	{HDMI_720p30, 0, 74250, 1280, 720, 1470},          //vic 62

/* vesa format*/
	{HDMI_800_600, 1, 0, 800, 600, 600},
	{HDMI_1024_768, 1, 0, 1024, 768, 768},
	{HDMI_720_400,  1, 0, 720, 400, 400},
	{HDMI_1280_768, 1, 0, 1280, 768, 768},
	{HDMI_1280_800, 1, 0, 1280, 800, 800},
	{HDMI_1280_960, 1, 0, 1280, 960, 960},
	{HDMI_1280_1024, 1, 0, 1280, 1024, 1024},
	{HDMI_1360_768, 1, 0, 1360, 768, 768},
	{HDMI_1366_768, 1, 0, 1366, 768, 768},
	{HDMI_1600_1200, 1, 0, 1600, 1200, 1200},
	{HDMI_1920_1200, 1, 0, 1920, 1200, 1200},
	{HDMI_1440_900, 1, 0, 1440, 900, 900},
	{HDMI_1400_1050, 1, 0, 1400, 1050, 1050},
	{HDMI_1680_1050, 1, 0, 1680, 1050, 1050},          //vic 79

	/* for AG-506 */
	{HDMI_480p60, 0, 27000, 720, 483, 483},
	{0, 0, 0}
};

unsigned int get_vic_from_timing(struct hdmi_rx_ctrl_video* video_par)
{
	int i;
	for(i = 0; freq_ref[i].vic; i++){
		if((video_par->hactive == freq_ref[i].active_pixels)&&
		    ((video_par->vactive == freq_ref[i].active_lines)||
		    (video_par->vactive == freq_ref[i].active_lines_fp))){
		    break;
		}
	}
	return freq_ref[i].vic;
}

enum tvin_sig_fmt_e hdmirx_hw_get_fmt(void)
{
	/* to do:
	TVIN_SIG_FMT_HDMI_1280x720P_24Hz_FRAME_PACKING,
	TVIN_SIG_FMT_HDMI_1280x720P_30Hz_FRAME_PACKING,

	TVIN_SIG_FMT_HDMI_1920x1080P_24Hz_FRAME_PACKING,
	TVIN_SIG_FMT_HDMI_1920x1080P_30Hz_FRAME_PACKING, // 150
	*/
	enum tvin_sig_fmt_e fmt = TVIN_SIG_FMT_NULL;
	unsigned int vic = rx.video_params.video_mode;

	if(rx.video_params.dvi||vic==0){
		vic = get_vic_from_timing(&rx.video_params);
	}
	if(force_vic){
		vic = force_vic;
	}

	switch(vic){
		/* basic format */
		case HDMI_640x480p60:		    /*1*/
			fmt = TVIN_SIG_FMT_HDMI_640X480P_60HZ;
			break;
		case HDMI_480p60:                   /*2*/
		case HDMI_480p60_16x9:              /*3*/
			if(is_frame_packing()){
				fmt = TVIN_SIG_FMT_HDMI_720X480P_60HZ_FRAME_PACKING;
			}
			else{
				fmt = TVIN_SIG_FMT_HDMI_720X480P_60HZ;
			}
			break;
		case HDMI_720p60:                   /*4*/
			if(is_frame_packing()){
				fmt = TVIN_SIG_FMT_HDMI_1280X720P_60HZ_FRAME_PACKING;
			}
			else{
				fmt = TVIN_SIG_FMT_HDMI_1280X720P_60HZ;
			}
			break;
		case HDMI_1080i60:                  /*5*/
			if(is_frame_packing()){
				fmt = TVIN_SIG_FMT_HDMI_1920X1080I_60HZ_FRAME_PACKING;
			}
			else{
				fmt = TVIN_SIG_FMT_HDMI_1920X1080I_60HZ;
			}
			break;
		case HDMI_480i60:                   /*6*/
		case HDMI_480i60_16x9:              /*7*/
			fmt = TVIN_SIG_FMT_HDMI_1440X480I_60HZ;
			break;
		case HDMI_1080p60:		    /*16*/
			fmt = TVIN_SIG_FMT_HDMI_1920X1080P_60HZ;
			break;
		case HDMI_1080p24:		    /*32*/
			if(is_frame_packing()){
				fmt = TVIN_SIG_FMT_HDMI_1920X1080P_24HZ_FRAME_PACKING;
			}
			else{
				fmt = TVIN_SIG_FMT_HDMI_1920X1080P_24HZ;
			}
			break;
		case HDMI_576p50:		    /*17*/
		case HDMI_576p50_16x9:		    /*18*/
			if(is_frame_packing()){
				fmt = TVIN_SIG_FMT_HDMI_720X576P_50HZ_FRAME_PACKING;
			}
			else{
				fmt = TVIN_SIG_FMT_HDMI_720X576P_50HZ;
			}
			break;
		case HDMI_720p50:		    /*19*/
			if(is_frame_packing()){
				fmt = TVIN_SIG_FMT_HDMI_1280X720P_50HZ_FRAME_PACKING;
			}
			else{
				fmt = TVIN_SIG_FMT_HDMI_1280X720P_50HZ;
			}
			break;
		case HDMI_1080i50:		    /*20*/
			if(is_frame_packing()){
				fmt = TVIN_SIG_FMT_HDMI_1920X1080I_50HZ_FRAME_PACKING;
			}
			else{
				fmt = TVIN_SIG_FMT_HDMI_1920X1080I_50HZ_A;
			}
			break;
		case HDMI_576i50:		   /*21*/
		case HDMI_576i50_16x9:		   /*22*/
			fmt = TVIN_SIG_FMT_HDMI_1440X576I_50HZ;
			break;
		case HDMI_1080p50:		   /*31*/
			fmt = TVIN_SIG_FMT_HDMI_1920X1080P_50HZ;
			break;
		case HDMI_1080p25:		   /*33*/
			fmt = TVIN_SIG_FMT_HDMI_1920X1080P_25HZ;
			break;
		case HDMI_1080p30:		   /*34*/
			if(is_frame_packing()){
				fmt = TVIN_SIG_FMT_HDMI_1920X1080P_30HZ_FRAME_PACKING;
			}
			else{
				fmt = TVIN_SIG_FMT_HDMI_1920X1080P_30HZ;
			}
			break;
		case HDMI_720p24:		   /*60*/
			if(is_frame_packing()){
				fmt = TVIN_SIG_FMT_HDMI_1280X720P_24HZ_FRAME_PACKING;
			}
			else{
				fmt = TVIN_SIG_FMT_HDMI_1280X720P_24HZ;
			}
			break;
		case HDMI_720p30:		   /*62*/
			if(is_frame_packing()){
				fmt = TVIN_SIG_FMT_HDMI_1280X720P_30HZ_FRAME_PACKING;
			}
			else{
				fmt = TVIN_SIG_FMT_HDMI_1280X720P_30HZ;
			}
			break;

		/* extend format */
		case HDMI_1440x240p60:
		case HDMI_1440x240p60_16x9:
			fmt = TVIN_SIG_FMT_HDMI_1440X240P_60HZ;
			break;
		case HDMI_2880x480i60:
		case HDMI_2880x480i60_16x9:
			fmt = TVIN_SIG_FMT_HDMI_2880X480I_60HZ;
			break;
		case HDMI_2880x240p60:
		case HDMI_2880x240p60_16x9:
			fmt = TVIN_SIG_FMT_HDMI_2880X240P_60HZ;
			break;
		case HDMI_1440x480p60:
		case HDMI_1440x480p60_16x9:
			fmt = TVIN_SIG_FMT_HDMI_1440X480P_60HZ;
			break;
		case HDMI_1440x288p50:
		case HDMI_1440x288p50_16x9:
			fmt = TVIN_SIG_FMT_HDMI_1440X288P_50HZ;
			break;
		case HDMI_2880x576i50:
		case HDMI_2880x576i50_16x9:
			fmt = TVIN_SIG_FMT_HDMI_2880X576I_50HZ;
			break;
		case HDMI_2880x288p50:
		case HDMI_2880x288p50_16x9:
			fmt = TVIN_SIG_FMT_HDMI_2880X288P_50HZ;
			break;
		case HDMI_1440x576p50:
		case HDMI_1440x576p50_16x9:
			fmt = TVIN_SIG_FMT_HDMI_1440X576P_50HZ;
			break;

		case HDMI_2880x480p60:
		case HDMI_2880x480p60_16x9:
			fmt = TVIN_SIG_FMT_HDMI_2880X480P_60HZ;
			break;
		case HDMI_2880x576p50:
		case HDMI_2880x576p50_16x9:
			fmt = TVIN_SIG_FMT_HDMI_2880X576P_60HZ; //????, should be TVIN_SIG_FMT_HDMI_2880x576P_50Hz
			break;
		case HDMI_1080i50_1250:
			fmt = TVIN_SIG_FMT_HDMI_1920X1080I_50HZ_B;
			break;
		case HDMI_1080I120:	/*46*/
			fmt = TVIN_SIG_FMT_HDMI_1920X1080I_120HZ;
			break;
		case HDMI_720p120:	/*47*/
			fmt = TVIN_SIG_FMT_HDMI_1280X720P_120HZ;
			break;
		case HDMI_1080p120:	/*63*/
			fmt = TVIN_SIG_FMT_HDMI_1920X1080P_120HZ;
			break;

		/* vesa format*/
		case HDMI_800_600:	/*65*/
			fmt = TVIN_SIG_FMT_HDMI_800X600_00HZ;
			break;
		case HDMI_1024_768:	/*66*/
			fmt = TVIN_SIG_FMT_HDMI_1024X768_00HZ;
			break;
		case HDMI_720_400:
			fmt = TVIN_SIG_FMT_HDMI_720X400_00HZ;
			break;
		case HDMI_1280_768:
			fmt = TVIN_SIG_FMT_HDMI_1280X768_00HZ;
			break;
		case HDMI_1280_800:
			fmt = TVIN_SIG_FMT_HDMI_1280X800_00HZ;
			break;
		case HDMI_1280_960:
			fmt = TVIN_SIG_FMT_HDMI_1280X960_00HZ;
			break;
		case HDMI_1280_1024:
			fmt = TVIN_SIG_FMT_HDMI_1280X1024_00HZ;
			break;
		case HDMI_1360_768:
			fmt = TVIN_SIG_FMT_HDMI_1360X768_00HZ;
			break;
		case HDMI_1366_768:
			fmt = TVIN_SIG_FMT_HDMI_1366X768_00HZ;
			break;
		case HDMI_1600_1200:
			fmt = TVIN_SIG_FMT_HDMI_1600X1200_00HZ;
			break;
		case HDMI_1920_1200:
			fmt = TVIN_SIG_FMT_HDMI_1920X1200_00HZ;
			break;
		case HDMI_1440_900:            
			fmt = TVIN_SIG_FMT_HDMI_1440X900_00HZ;            
			break;	
		case HDMI_1400_1050:            
			fmt = TVIN_SIG_FMT_HDMI_1400X1050_00HZ;            
			break;
		case HDMI_1680_1050:            
			fmt = TVIN_SIG_FMT_HDMI_1680X1050_00HZ;            
			break;
		default:
			break;
		}

	return fmt;
}

bool hdmirx_hw_pll_lock(void)
{
	return (rx.state==HDMIRX_HWSTATE_SIG_READY);
}

bool hdmirx_hw_is_nosig(void)
{
	return rx.no_signal;
}

static int is_timing_ok(struct hdmi_rx_ctrl_video *video_par)
{
	int i;
	int ret = 1;
	int pixel_clk = hdmirx_get_pixel_clock();
	unsigned char is_dvi = video_par->dvi;
	if(video_par->video_mode==0){
		is_dvi = 1;
	}
	//frame_rate = (video_par->htotal==0||video_par->vtotal==0)?0:
	//        (pixel_clk/video_par->htotal)*100/video_par->vtotal;

	if ( 	/*video_par->pixel_clk < PIXEL_CLK_MIN || video_par->pixel_clk > PIXEL_CLK_MAX*/
		(pixel_clk/1000) < PIXEL_CLK_MIN || (pixel_clk/1000) > PIXEL_CLK_MAX
		|| video_par->hactive < HRESOLUTION_MIN
		|| video_par->hactive > HRESOLUTION_MAX
		|| video_par->htotal < (video_par->hactive + video_par->hoffset)
		|| video_par->vactive < VRESOLUTION_MIN
		|| video_par->vactive > VRESOLUTION_MAX
		|| video_par->vtotal < (video_par->vactive + video_par->voffset)
		/*|| video_par->refresh_rate < REFRESH_RATE_MIN
		|| video_par->refresh_rate > REFRESH_RATE_MAX
		*/) {
			if(hdmirx_log_flag&0x100)
				printk("%s timing error pixel clk %d hactive %d htotal %d(%d) vactive %d vtotal %d(%d) %ld\n", __func__,
					pixel_clk/1000, 
					video_par->hactive,  video_par->htotal, video_par->hactive+video_par->hoffset, 
					video_par->vactive,  video_par->vtotal, video_par->vactive+video_par->voffset, 
					video_par->refresh_rate);
			return 0;
	}

	if(is_dvi==0){
		for(i = 0; freq_ref[i].vic; i++){
			if(freq_ref[i].vic == video_par->video_mode){
				if((video_par->hactive == freq_ref[i].active_pixels)&&
				((video_par->vactive == freq_ref[i].active_lines)||
				(video_par->vactive == freq_ref[i].active_lines_fp))){
					break;
				}
			}
		}
		if(freq_ref[i].vic==0){
			ret = 0;
		}
	} else {
		if(get_vic_from_timing(video_par) == 0){
		ret = 0;
		}
	}
	return ret;
}

void hdmirx_hw_monitor(void)
{
	//int port;
	int pre_sample_rate;
	bool tx_5v_status;

	rx.tx_5v_status = (hdmirx_rd_top(HDMIRX_TOP_HPD_PWR5V)>>(20 + rx.port)&0x1)==0x1;
	tx_5v_status = rx.tx_5v_status|rx.tx_5v_status_pre;
	rx.tx_5v_status_pre = rx.tx_5v_status; 
	switch(rx.state){
	case HDMIRX_HWSTATE_INIT:
		hdmirx_hw_config();			
		hdmi_rx_ctrl_edid_update();
		rx.state = HDMIRX_HWSTATE_5V_LOW;
		hdmirx_print("[HDMIRX State] init->5v low\n");
		printk("Hdmirx driver version: %s\n", HDMIRX_VER);
		break;
	case HDMIRX_HWSTATE_5V_LOW:
		if(rx.tx_5v_status){
			rx.state = HDMIRX_HWSTATE_5V_HIGH;
			hdmirx_print("[HDMIRX State] 5v low->5v high\n");
		} else {
			rx.no_signal = true;
		}
		break;

	case HDMIRX_HWSTATE_5V_HIGH:
		if(!tx_5v_status){
			rx.no_signal = true;
			rx.state = HDMIRX_HWSTATE_INIT;
			hdmirx_print("[HDMIRX State] 5v high->init\n");
		} else {
			hdmirx_set_hpd(rx.port, 1);
			rx.state = HDMIRX_HWSTATE_HPD_READY;
			rx.no_signal = false;
			hdmirx_print("[HDMIRX State] 5v high->hpd ready\n");
		}
		break;

	case HDMIRX_HWSTATE_HPD_READY:
		if(!tx_5v_status){
			rx.no_signal = true;
			rx.state = HDMIRX_HWSTATE_INIT;
			hdmirx_set_hpd(rx.port, 0);
			hdmirx_print("[HDMIRX State] hpd ready ->init\n");
		} else {
			rx.state = HDMIRX_HWSTATE_SIG_UNSTABLE;
			sig_unstable_count = 0;
			hdmirx_print("[HDMIRX State] hpd ready->unstable\n");
		}
		break;

	case HDMIRX_HWSTATE_SIG_UNSTABLE:
		if(!tx_5v_status){
			rx.no_signal = true;
			rx.state = HDMIRX_HWSTATE_INIT;
			hdmirx_set_hpd(rx.port, 0);
			hdmirx_print("[HDMIRX State] unstable->init\n");
		} else {
			if (hdmirx_rd_dwc(RA_HDMI_PLL_LCK_STS) & 0x01){
				memset(&rx.vendor_specific_info, 0, sizeof(struct vendor_specific_info_s));
				rx.state = HDMIRX_HWSTATE_SIG_STABLE;
				rx.no_signal = false;
				rx.video_wait_time = 0;
				hdmirx_print("[HDMIRX State] unstable->stable\n");
			} else {
				sig_unstable_count++;
				if(sig_unstable_count > 200){
				rx.no_signal = true;
				}
			}
		}
	break;

	case HDMIRX_HWSTATE_SIG_STABLE:
		if(!tx_5v_status){
			rx.no_signal = true;
			rx.state = HDMIRX_HWSTATE_INIT;
			hdmirx_set_hpd(rx.port, 0);
			hdmirx_print("[HDMIRX State] stable->init\n");
		} else if (hdmirx_rd_dwc (RA_HDMI_PLL_LCK_STS) & 0x01){
			hdmirx_get_video_info(&rx.ctrl, &rx.video_params);
			if(is_timing_ok(&rx.video_params) || (force_ready)) {
				rx.ctrl.tmds_clk2 = hdmirx_get_tmds_clock();
				hdmirx_get_video_info(&rx.ctrl, &rx.pre_video_params);
				hdmirx_get_video_info(&rx.ctrl, &rx.cur_video_params);
				//hdmirx_read_vendor_specific_info_frame(&rx.vendor_specific_info);
				hdmirx_config_video(&rx.video_params);
				rx.change = 0;
				rx.state = HDMIRX_HWSTATE_SIG_READY;
				rx.no_signal = false;
				memset(&rx.aud_info, 0, sizeof(struct aud_info_s));
				hdmirx_print("[HDMIRX State] stable->ready\n");
				dump_state(0x1);
			}
		} else {
			rx.state = HDMIRX_HWSTATE_SIG_UNSTABLE;
			sig_unstable_count = 0;
			hdmirx_print("[HDMIRX State] stable->unstable\n");
		}
		break;
	case HDMIRX_HWSTATE_SIG_READY:
		sig_ready_count++;
		if(!tx_5v_status){
			rx.no_signal = true;
			rx.state = HDMIRX_HWSTATE_INIT;
			hdmirx_set_hpd(rx.port, 0);
			hdmirx_print("[HDMIRX State] ready->init\n");
		} else if ((hdmirx_rd_dwc (RA_HDMI_PLL_LCK_STS) & 0x01) == 0){
			sig_unlock_count++;
			if (sig_ready_count == 4) {
				if(sig_unlock_count >= 3)
				rx.state = HDMIRX_HWSTATE_SIG_UNSTABLE;
				sig_unstable_count = 0;
				sig_ready_count = 0;
				sig_unlock_count = 0;
			}
			//rx.state = HDMIRX_HWSTATE_SIG_UNSTABLE;
			hdmirx_print("[HDMIRX State] ready->unstable, pll unlock\n");
			//if(switch_mode&0x1){
			//	hdmirx_hw_config();			
			//	hdmi_rx_ctrl_edid_update();
			//}
		} else if(rx.change){
#if 1
			rx.state = HDMIRX_HWSTATE_SIG_UNSTABLE;
			sig_unstable_count = 0;
			hdmirx_print("[HDMIRX State] ready->unstable, mode change\n");
			if(switch_mode&0x1){
				hdmirx_hw_init2(); //for 480I & 480P NO audio
				hdmirx_hw_config();			
				hdmi_rx_ctrl_edid_update();
			}
#else
			rx.state = HDMIRX_HWSTATE_SIG_STABLE;
			rx.video_wait_time = 0;
			memset(&rx.vendor_specific_info, 0, sizeof(struct vendor_specific_info_s));
			hdmirx_print("[HDMIRX State] ready->stable, mode change\n");
#endif				
		} else {
			rx.no_signal = false;
			if (t3d_flash_flag != 1)
			hdmirx_get_video_info(&rx.ctrl, &rx.video_params);
			if(audio_enable){
				pre_sample_rate = rx.aud_info.real_sample_rate;
				hdmirx_read_audio_info(&rx.aud_info);
				rx.aud_info.real_sample_rate = get_real_sample_rate();

				if(is_sample_rate_change(pre_sample_rate, rx.aud_info.real_sample_rate)){
					//set_hdmi_audio_source_gate(0);
					//dump_audio_info();
					printk("[hdmirx-audio]:sample_rate_chg,pre:%d,cur:%d\n", pre_sample_rate, rx.aud_info.real_sample_rate);
					rx.audio_sample_rate_stable_count = 0;
				} else {
					if(rx.audio_sample_rate_stable_count < audio_sample_rate_stable_count_th){
						printk("[hdmirx-audio]:sample_rate_stable_count:%d\n", rx.audio_sample_rate_stable_count);
						rx.audio_sample_rate_stable_count++;
						if(rx.audio_sample_rate_stable_count==audio_sample_rate_stable_count_th){
							//mailbox_send_audiodsp(1, M2B_IRQ0_DSP_AUDIO_EFFECT, DSP_CMD_SET_HDMI_SR, (char *)&rx.aud_info.real_sample_rate,sizeof(rx.aud_info.real_sample_rate));
							dump_state(0x2);
							printk("[hdmirx-audio]:----audio stable\n");
							hdmirx_config_audio();
							audio_sample_rate = rx.aud_info.real_sample_rate;
							rx.audio_wait_time = audio_stable_time;
						}
					}
				}
				
				if(rx.audio_wait_time > 0 ){
					rx.audio_wait_time -= HW_MONITOR_TIME_UNIT;
					if(rx.audio_wait_time <= 0){
						//set_hdmi_audio_source_gate(1);
					}
				}
			}
		}

		//for debug
		//if ((hdmirx_rd_dwc (RA_HDMI_PLL_LCK_STS) & 0x01) == 0){
		//	hdmirx_print("[HDMIRX State] ready->unstable, pll unlock\n");
		//}
		
		if (sig_ready_count == 4) {
			sig_ready_count = 0;
			sig_unlock_count = 0;
		}
		break;

	default:
		if(!tx_5v_status){
			rx.no_signal = true;
			rx.state = HDMIRX_HWSTATE_INIT;
			hdmirx_set_hpd(rx.port, 0);
		}
		break;
	}	
	
	if(force_state&0x10){
		rx.state = force_state&0xf;
		rx.change = 0;
	}
}
/*
* EDID & hdcp
*/
struct hdmi_rx_ctrl_hdcp init_hdcp_data;
#define MAX_KEY_BUF_SIZE 512

static char key_buf[MAX_KEY_BUF_SIZE];
static int key_size = 0;

#define MAX_EDID_BUF_SIZE 1024
static char edid_buf[MAX_EDID_BUF_SIZE];
static int edid_size = 0;

static unsigned char hdmirx_8bit_3d_edid_port1[] =
{
//8 bit only with 3D
0x00 ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff ,0x00 ,0x4d ,0x77 ,0x02 ,0x2c ,0x01 ,0x01 ,0x01 ,0x01,
0x01 ,0x15 ,0x01 ,0x03 ,0x80 ,0x85 ,0x4b ,0x78 ,0x0a ,0x0d ,0xc9 ,0xa0 ,0x57 ,0x47 ,0x98 ,0x27,
0x12 ,0x48 ,0x4c ,0x21 ,0x08 ,0x00 ,0x81 ,0x80 ,0x01 ,0x01 ,0x01 ,0x01 ,0x01 ,0x01 ,0x01 ,0x01,
0x01 ,0x01 ,0x01 ,0x01 ,0x01 ,0x01 ,0x02 ,0x3a ,0x80 ,0x18 ,0x71 ,0x38 ,0x2d ,0x40 ,0x58 ,0x2c,
0x45 ,0x00 ,0x30 ,0xeb ,0x52 ,0x00 ,0x00 ,0x1e ,0x01 ,0x1d ,0x00 ,0x72 ,0x51 ,0xd0 ,0x1e ,0x20,
0x6e ,0x28 ,0x55 ,0x00 ,0x30 ,0xeb ,0x52 ,0x00 ,0x00 ,0x1e ,0x00 ,0x00 ,0x00 ,0xfc ,0x00 ,0x53,
0x6b ,0x79 ,0x77 ,0x6f ,0x72 ,0x74 ,0x68 ,0x20 ,0x54 ,0x56 ,0x0a ,0x20 ,0x00 ,0x00 ,0x00 ,0xfd,
0x00 ,0x30 ,0x3e ,0x0e ,0x46 ,0x0f ,0x00 ,0x0a ,0x20 ,0x20 ,0x20 ,0x20 ,0x20 ,0x20 ,0x01 ,0xdc,
0x02 ,0x03 ,0x38 ,0xf0 ,0x53 ,0x1f ,0x10 ,0x14 ,0x05 ,0x13 ,0x04 ,0x20 ,0x22 ,0x3c ,0x3e ,0x12,
0x16 ,0x03 ,0x07 ,0x11 ,0x15 ,0x02 ,0x06 ,0x01 ,0x23 ,0x09 ,0x07 ,0x01 ,0x83 ,0x01 ,0x00 ,0x00,
0x74 ,0x03 ,0x0c ,0x00 ,0x10 ,0x00 ,0x88 ,0x2d ,0x2f ,0xd0 ,0x0a ,0x01 ,0x40 ,0x00 ,0x7f ,0x20,
0x30 ,0x70 ,0x80 ,0x90 ,0x76 ,0xe2 ,0x00 ,0xfb ,0x02 ,0x3a ,0x80 ,0xd0 ,0x72 ,0x38 ,0x2d ,0x40,
0x10 ,0x2c ,0x45 ,0x80 ,0x30 ,0xeb ,0x52 ,0x00 ,0x00 ,0x1e ,0x01 ,0x1d ,0x00 ,0xbc ,0x52 ,0xd0,
0x1e ,0x20 ,0xb8 ,0x28 ,0x55 ,0x40 ,0x30 ,0xeb ,0x52 ,0x00 ,0x00 ,0x1e ,0x01 ,0x1d ,0x80 ,0xd0,
0x72 ,0x1c ,0x16 ,0x20 ,0x10 ,0x2c ,0x25 ,0x80 ,0x30 ,0xeb ,0x52 ,0x00 ,0x00 ,0x9e ,0x00 ,0x00,
0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x8e
};


static unsigned char hdmirx_12bit_3d_edid_port1 [] =
{
0x00 ,0xff ,0xff ,0xff ,0xff ,0xff ,0xff ,0x00 ,0x4d ,0xd9 ,0x02 ,0x2c ,0x01 ,0x01 ,0x01 ,0x01,
0x01 ,0x15 ,0x01 ,0x03 ,0x80 ,0x85 ,0x4b ,0x78 ,0x0a ,0x0d ,0xc9 ,0xa0 ,0x57 ,0x47 ,0x98 ,0x27,
0x12 ,0x48 ,0x4c ,0x21 ,0x08 ,0x00 ,0x81 ,0x80 ,0x01 ,0x01 ,0x01 ,0x01 ,0x01 ,0x01 ,0x01 ,0x01,
0x01 ,0x01 ,0x01 ,0x01 ,0x01 ,0x01 ,0x02 ,0x3a ,0x80 ,0x18 ,0x71 ,0x38 ,0x2d ,0x40 ,0x58 ,0x2c,
0x45 ,0x00 ,0x30 ,0xeb ,0x52 ,0x00 ,0x00 ,0x1e ,0x01 ,0x1d ,0x00 ,0x72 ,0x51 ,0xd0 ,0x1e ,0x20,
0x6e ,0x28 ,0x55 ,0x00 ,0x30 ,0xeb ,0x52 ,0x00 ,0x00 ,0x1e ,0x00 ,0x00 ,0x00 ,0xfc ,0x00 ,0x53,
0x4f ,0x4e ,0x59 ,0x20 ,0x54 ,0x56 ,0x0a ,0x20 ,0x20 ,0x20 ,0x20 ,0x20 ,0x00 ,0x00 ,0x00 ,0xfd,
0x00 ,0x30 ,0x3e ,0x0e ,0x46 ,0x0f ,0x00 ,0x0a ,0x20 ,0x20 ,0x20 ,0x20 ,0x20 ,0x20 ,0x01 ,0x1c,
0x02 ,0x03 ,0x3b ,0xf0 ,0x53 ,0x1f ,0x10 ,0x14 ,0x05 ,0x13 ,0x04 ,0x20 ,0x22 ,0x3c ,0x3e ,0x12,
0x16 ,0x03 ,0x07 ,0x11 ,0x15 ,0x02 ,0x06 ,0x01 ,0x26 ,0x09 ,0x07 ,0x07 ,0x15 ,0x07 ,0x50 ,0x83,
0x01 ,0x00 ,0x00 ,0x74 ,0x03 ,0x0c ,0x00 ,0x20 ,0x00 ,0xb8 ,0x2d ,0x2f ,0xd0 ,0x0a ,0x01 ,0x40,
0x00 ,0x7f ,0x20 ,0x30 ,0x70 ,0x80 ,0x90 ,0x76 ,0xe2 ,0x00 ,0xfb ,0x02 ,0x3a ,0x80 ,0xd0 ,0x72,
0x38 ,0x2d ,0x40 ,0x10 ,0x2c ,0x45 ,0x80 ,0x30 ,0xeb ,0x52 ,0x00 ,0x00 ,0x1e ,0x01 ,0x1d ,0x00,
0xbc ,0x52 ,0xd0 ,0x1e ,0x20 ,0xb8 ,0x28 ,0x55 ,0x40 ,0x30 ,0xeb ,0x52 ,0x00 ,0x00 ,0x1e ,0x01,
0x1d ,0x80 ,0xd0 ,0x72 ,0x1c ,0x16 ,0x20 ,0x10 ,0x2c ,0x25 ,0x80 ,0x30 ,0xeb ,0x52 ,0x00 ,0x00,
0x9e ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0xd9,
};

static int hdmi_rx_ctrl_edid_update(void)
{
	int i, ram_addr, byte_num;
	unsigned int value;
	unsigned char check_sum;
	//printk("HDMI_OTHER_CTRL2=%x\n", hdmi_rd_reg(OTHER_BASE_ADDR+HDMI_OTHER_CTRL2));

	if(edid_size>4 && edid_buf[0]=='E' && edid_buf[1]=='D' && edid_buf[2]=='I' && edid_buf[3]=='D'){ 
		hdmirx_print("edid: use custom edid\n");
		check_sum = 0;
		for (i = 0; i < (edid_size-4); i++)
		{
			value = edid_buf[i+4];
			if(((i+1)&0x7f)!=0){
				check_sum += value;
				check_sum &= 0xff;
			}
			else{
				if(value != ((0x100-check_sum)&0xff)){
					hdmirx_print("HDMIRX: origin edid[%d] checksum %x is incorrect, change to %x\n",
					i, value, (0x100-check_sum)&0xff);
				}
				value = (0x100-check_sum)&0xff;
				check_sum = 0;
		 	}
		    ram_addr = HDMIRX_TOP_EDID_OFFSET+i;
		    hdmirx_wr_top(ram_addr, value);
		}
	}
	else{
		if((edid_mode&0xf) == 0){
			unsigned char * p_edid_array;
			byte_num = sizeof(hdmirx_8bit_3d_edid_port1)/sizeof(unsigned char);
			p_edid_array =  hdmirx_8bit_3d_edid_port1;

			/*recalculate check sum*/
			for(check_sum = 0,i=0;i<127;i++){
				check_sum += p_edid_array[i];
				check_sum &= 0xff;
			}
			p_edid_array[127] = (0x100-check_sum)&0xff;

			for(check_sum = 0,i=128;i<255;i++){
				check_sum += p_edid_array[i];
				check_sum &= 0xff;
			}
			p_edid_array[255] = (0x100-check_sum)&0xff;
			/**/

			for (i = 0; i < byte_num; i++){
				value = p_edid_array[i];
				ram_addr = HDMIRX_TOP_EDID_OFFSET+i;
				hdmirx_wr_top(ram_addr, value);
			}
		}
		else if((edid_mode&0xf) == 1){
			byte_num = sizeof(hdmirx_12bit_3d_edid_port1)/sizeof(unsigned char);

			for (i = 0; i < byte_num; i++){
				value = hdmirx_12bit_3d_edid_port1[i];
				ram_addr = HDMIRX_TOP_EDID_OFFSET+i;
				hdmirx_wr_top(ram_addr, value);
			}
		}
	}
	return 0;
}

static void set_hdcp(struct hdmi_rx_ctrl_hdcp *hdcp, const unsigned char* b_key)
{
    int i,j;
    memset(&init_hdcp_data, 0, sizeof(struct hdmi_rx_ctrl_hdcp));
    for(i=0,j=0; i<80; i+=2,j+=7){
        hdcp->keys[i+1] = b_key[j]|(b_key[j+1]<<8)|(b_key[j+2]<<16)|(b_key[j+3]<<24);
        hdcp->keys[i+0] = b_key[j+4]|(b_key[j+5]<<8)|(b_key[j+6]<<16);
    }
    hdcp->bksv[1] = b_key[j]|(b_key[j+1]<<8)|(b_key[j+2]<<16)|(b_key[j+3]<<24);
    hdcp->bksv[0] = b_key[j+4];
    
}    

int hdmirx_read_key_buf(char* buf, int max_size)
{
	if(key_size > max_size){
		pr_err("Error: %s, key size %d is larger than the buf size of %d\n", __func__,  key_size, max_size);
		return 0;
	}
	memcpy(buf, key_buf, key_size);
	pr_info("HDMIRX: read key buf\n");
	return key_size;
}

void hdmirx_fill_key_buf(const char* buf, int size)
{
	if(size > MAX_KEY_BUF_SIZE){
		pr_err("Error: %s, key size %d is larger than the max size of %d\n", __func__,  size, MAX_KEY_BUF_SIZE);
		return;
	}
	if(buf[0]=='k' && buf[1]=='e' && buf[2]=='y'){
	    set_hdcp(&init_hdcp_data, buf+3);            
	}
	else{
		memcpy(key_buf, buf, size);
		key_size = size;
		pr_info("HDMIRX: fill key buf, size %d\n", size);
	}
}

int hdmirx_read_edid_buf(char* buf, int max_size)
{
	if(edid_size > max_size){
		pr_err("Error: %s, edid size %d is larger than the buf size of %d\n", __func__,  edid_size, max_size);
		return 0;
	}
	memcpy(buf, edid_buf, edid_size);
	pr_info("HDMIRX: read edid buf\n");
	return edid_size;
}

void hdmirx_fill_edid_buf(const char* buf, int size)
{
	if(size > MAX_EDID_BUF_SIZE){
		pr_err("Error: %s, edid size %d is larger than the max size of %d\n", __func__,  size, MAX_EDID_BUF_SIZE);
		return;
	}
	memcpy(edid_buf, buf, size);
	edid_size = size;
	pr_info("HDMIRX: fill edid buf, size %d\n", size);
}

/********************
    debug functions
*********************/
int hdmirx_hw_dump_reg(unsigned char* buf, int size)
{
	return 0;
}

static void dump_state(unsigned char enable)
{
	int error = 0;
	//int i = 0;
	struct hdmi_rx_ctrl_video v;
  	static struct aud_info_s a;
	struct vendor_specific_info_s vsi;
  	if(enable&1){
    		hdmirx_get_video_info(&rx.ctrl, &v);
      		printk("[HDMI info]error %d video_format %d VIC %d dvi %d interlace %d\nhtotal %d vtotal %d hactive %d vactive %d pixel_repetition %d\npixel_clk %ld deep_color %d refresh_rate %ld\n",
        		error,
        		v.video_format, v.video_mode, v.dvi, v.interlaced, 
        		v.htotal, v.vtotal, v.hactive, v.vactive, v.pixel_repetition,
        		v.pixel_clk, v.deep_color_mode, v.refresh_rate);
     		hdmirx_read_vendor_specific_info_frame(&vsi);  
      		printk("Vendor Specific Info ID=%x, hdmi_video_format=%x, 3d_struct=%x, 3d_ext=%x\n",
       		vsi.identifier, vsi.hdmi_video_format, vsi._3d_structure, vsi._3d_ext_data);
  	}
  	if(enable&2){
      		hdmirx_read_audio_info(&a);
    		printk("AudioInfo: CT=%u CC=%u SF=%u SS=%u CA=%u",
    			a.coding_type, a.channel_count, a.sample_frequency,
    			a.sample_size, a.channel_allocation);
    
      		printk("CTS=%d, N=%d, recovery clock is %d\n", a.cts, a.n, a.audio_recovery_clock);
  	}
 	printk("TMDS clock = %d, Pixel clock = %d\n", hdmirx_get_tmds_clock(), hdmirx_get_pixel_clock());
  
  	printk("rx.no_signal=%d, rx.state=%d, fmt=0x%x\n", rx.no_signal, rx.state, hdmirx_hw_get_fmt());
}

void dump_reg(void)
{
	int i = 0;

	printk("\n\n*******Controller registers********\n");
	printk("[addr ]  addr + 0x0,  addr + 0x4,  addr + 0x8,  addr + 0xc\n\n");
	for(i = 0; i <= 0xffc; ){
		printk("[0x%-3x]  0x%-8x,  0x%-8x,  0x%-8x,  0x%-8x\n",i,hdmirx_rd_dwc(i),hdmirx_rd_dwc(i+4),hdmirx_rd_dwc(i+8),hdmirx_rd_dwc(i+12));
		i = i + 16;
	}
	printk("\n\n*******PHY registers********\n");
	printk("[addr ]  addr + 0x0,  addr + 0x1,  addr + 0x2,  addr + 0x3\n\n");
	for(i = 0; i <= 0x9a; ){
		printk("[0x%-3x]  0x%-8x,  0x%-8x,  0x%-8x,  0x%-8x\n",i,hdmirx_rd_phy(i),hdmirx_rd_phy(i+1),hdmirx_rd_phy(i+2),hdmirx_rd_phy(i+3));
		i = i + 4;
	}
}


int hdmirx_debug(const char* buf, int size)
{
	char tmpbuf[128];
	int i = 0;
	unsigned int adr;
	unsigned int value = 0;

	while((buf[i]) && (buf[i] != ',') && (buf[i] != ' ')) {
		tmpbuf[i]=buf[i];
		i++;
	}
	tmpbuf[i] = 0;
	if(strncmp(tmpbuf, "hpd", 3)==0){
        hdmirx_set_hpd(rx.port, tmpbuf[3]=='0'?0:1);
  }
	else if(strncmp(tmpbuf, "set_color_depth", 15) == 0) {
		//hdmirx_config_color_depth(tmpbuf[15]-'0');
		//printk("set color depth %c\n", tmpbuf[15]);
	} else if (strncmp(tmpbuf, "reset", 5) == 0) {
		if(tmpbuf[5] == '0') {
		}
		else if(tmpbuf[5] == '1') {
		}
		else if(tmpbuf[5] == '2') {
		}
		else if(tmpbuf[5] == '3') {
		}
	} else if (strncmp(tmpbuf, "set_state", 9) == 0) {
		//rx.state = simple_strtoul(tmpbuf+9, NULL, 10);
		//printk("set state %d\n", rx.state);
	} else if (strncmp(tmpbuf, "test", 4) == 0) {
	       // printk("hdcp ctrl %x\n", hdmirx_rd_dwc(0xc0));
            //test();
		//test_flag = simple_strtoul(tmpbuf+4, NULL, 10);;
		//printk("test %d\n", test_flag);
	} else if (strncmp(tmpbuf, "state", 5) == 0) {
		    dump_state(0xff);
	} else if (strncmp(tmpbuf, "pause", 5) == 0) {
		//sm_pause = simple_strtoul(tmpbuf+5, NULL, 10);
		//printk("%s the state machine\n", sm_pause?"pause":"enable");
	} else if (strncmp(tmpbuf, "reg", 3) == 0) {
		dump_reg();
	} else if (strncmp(tmpbuf, "log", 3) == 0) {

	} else if (strncmp(tmpbuf, "clock", 5) == 0) {
		value = simple_strtoul(tmpbuf + 5, NULL, 10);
		printk("clock[%d] = %d\n", value, hdmirx_get_clock(value));
	} else if (strncmp(tmpbuf, "sample_rate", 11) == 0) {
			//nothing
	} else if (strncmp(tmpbuf, "prbs", 4) == 0) {
	  //turn_on_prbs_mode(simple_strtoul(tmpbuf+4, NULL, 10));
	} else if (tmpbuf[0] == 'w') {
		adr = simple_strtoul(tmpbuf + 2, NULL, 16);
		value = simple_strtoul(buf + i + 1, NULL, 16);
		if(buf[1] == 'h') {
    	adr = simple_strtoul(tmpbuf + 3, NULL, 16);
			if(buf[2] == 't') {
		    		hdmirx_wr_top(adr, value);
				pr_info("write %x to hdmirx TOP reg[%x]\n",value,adr);
			} else if (buf[2] == 'd') {
		    		hdmirx_wr_dwc(adr, value);
				pr_info("write %x to hdmirx DWC reg[%x]\n",value,adr);
		    	} else if(buf[2] == 'p') {
		    		hdmirx_wr_phy(adr, value);
				pr_info("write %x to hdmirx PHY reg[%x]\n",value,adr);
		    	}
		} else if (buf[1] == 'c') {
			WRITE_MPEG_REG(adr, value);
			pr_info("write %x to CBUS reg[%x]\n",value,adr);
		} else if (buf[1] == 'p') {
			WRITE_APB_REG(adr, value);
			pr_info("write %x to APB reg[%x]\n",value,adr);
		}else if (buf[1] == 'l') {
			WRITE_MPEG_REG(MDB_CTRL, 2);
			WRITE_MPEG_REG(MDB_ADDR_REG, adr);
			WRITE_MPEG_REG(MDB_DATA_REG, value);
			pr_info("write %x to LMEM[%x]\n",value,adr);
		}else if(buf[1] == 'r') {
			WRITE_MPEG_REG(MDB_CTRL, 1);
			WRITE_MPEG_REG(MDB_ADDR_REG, adr);
			WRITE_MPEG_REG(MDB_DATA_REG, value);
			pr_info("write %x to amrisc reg [%x]\n",value,adr);
		}
	} else if (tmpbuf[0] == 'r') {
		adr = simple_strtoul(tmpbuf + 2, NULL, 16);
		if(buf[1] == 'h') {
		  adr = simple_strtoul(tmpbuf + 3, NULL, 16);
			if(buf[2] == 't') {
				value = hdmirx_rd_top(adr);
				pr_info("hdmirx TOP reg[%x]=%x\n",adr, value);
			} else if (buf[2] == 'd') {
			    	value = hdmirx_rd_dwc(adr);
				pr_info("hdmirx DWC reg[%x]=%x\n",adr, value);
			} else if(buf[2] == 'p') {
			    	value = hdmirx_rd_phy(adr);
				pr_info("hdmirx PHY reg[%x]=%x\n",adr, value);
			    }
		}
		else if (buf[1] == 'c') {
		    value = READ_MPEG_REG(adr);
		    pr_info("CBUS reg[%x]=%x\n", adr, value);
		} else if (buf[1] == 'p') {
		    value = READ_APB_REG(adr);
		    pr_info("APB reg[%x]=%x\n", adr, value);
		} else if (buf[1] == 'l') {
		    WRITE_MPEG_REG(MDB_CTRL, 2);
		    WRITE_MPEG_REG(MDB_ADDR_REG, adr);
		    value = READ_MPEG_REG(MDB_DATA_REG);
		    pr_info("LMEM[%x]=%x\n", adr, value);
		} else if (buf[1]=='r') {
		    WRITE_MPEG_REG(MDB_CTRL, 1);
		    WRITE_MPEG_REG(MDB_ADDR_REG, adr);
		    value = READ_MPEG_REG(MDB_DATA_REG);
		    pr_info("amrisc reg[%x]=%x\n", adr, value);
		}
	    } else if (tmpbuf[0] == 'v'){
		printk("------------------\n");
		printk("Hdmirx driver version: %s\n", HDMIRX_VER);
		printk("------------------\n");
	    }
	return 0;
}

void hdmirx_hw_init2(void)
{
    memset(&rx, 0, sizeof(struct rx));
    memset(&rx.pre_video_params, 0, sizeof(struct hdmi_rx_ctrl_video));
    memcpy(&rx.hdcp, &init_hdcp_data, sizeof(struct hdmi_rx_ctrl_hdcp));  

    rx.phy.cfg_clk = cfg_clk;
    rx.phy.lock_thres = lock_thres;
    rx.phy.fast_switching = fast_switching;   
    rx.phy.fsm_enhancement = fsm_enhancement;
    rx.phy.port_select_ovr_en = port_select_ovr_en;
    rx.phy.phy_cmu_config_force_val = phy_cmu_config_force_val;
    rx.phy.phy_system_config_force_val = phy_system_config_force_val;
    rx.ctrl.md_clk = 24000;
    rx.ctrl.tmds_clk = 0;
    rx.ctrl.tmds_clk2 = 0;
    rx.ctrl.acr_mode = acr_mode;

    rx.port = local_port;
    
    hdmirx_set_pinmux();
    hdmirx_set_hpd(rx.port, 0);
    
    hdmirx_print("%s %d\n", __func__, rx.port);

}


/***********************
    hdmirx_hw_init
    hdmirx_hw_uninit
    hdmirx_hw_enable
    hdmirx_hw_disable
    hdmirx_irq_init
*************************/
void hdmirx_hw_init(tvin_port_t port)
{
    memset(&rx, 0, sizeof(struct rx));
    memset(&rx.pre_video_params, 0, sizeof(struct hdmi_rx_ctrl_video));
    memcpy(&rx.hdcp, &init_hdcp_data, sizeof(struct hdmi_rx_ctrl_hdcp));  

    rx.phy.cfg_clk = cfg_clk;
    rx.phy.lock_thres = lock_thres;
    rx.phy.fast_switching = fast_switching;   
    rx.phy.fsm_enhancement = fsm_enhancement;
    rx.phy.port_select_ovr_en = port_select_ovr_en;
    rx.phy.phy_cmu_config_force_val = phy_cmu_config_force_val;
    rx.phy.phy_system_config_force_val = phy_system_config_force_val;
    rx.ctrl.md_clk = 24000;
    rx.ctrl.tmds_clk = 0;
    rx.ctrl.tmds_clk2 = 0;
    rx.ctrl.acr_mode = acr_mode;

    rx.port = (port_map>>((port - TVIN_PORT_HDMI0)<<2))&0xf;
    local_port = rx.port;
    hdmirx_set_pinmux();
    hdmirx_set_hpd(rx.port, 0);
    
    hdmirx_print("%s %d\n", __func__, rx.port);

}

 
void hdmirx_hw_uninit(void)
{
    hdmirx_wr_top(HDMIRX_TOP_INTR_MASKN, 0);
    
    hdmirx_interrupts_cfg(false);
    
    rx.ctrl.status = 0;
    rx.ctrl.tmds_clk = 0;
    //ctx->bsp_reset(true);
    
    hdmirx_phy_reset(true);
    hdmirx_phy_pddq(1);
}

void hdmirx_hw_enable(void)
{
}

void hdmirx_hw_disable(unsigned char flag)
{
}

void hdmirx_irq_init(void)
{
    if(request_irq(AM_IRQ1(HDMIRX_IRQ), &irq_handler, IRQF_SHARED, "hdmirx", (void *)&rx)){
    	hdmirx_print(__func__, "RX IRQ request");
    }
}

MODULE_PARM_DESC(port_map, "\n port_map \n");
module_param(port_map, int, 0664);

MODULE_PARM_DESC(cfg_clk, "\n cfg_clk \n");
module_param(cfg_clk, int, 0664);

MODULE_PARM_DESC(lock_thres, "\n lock_thres \n");
module_param(lock_thres, int, 0664);

MODULE_PARM_DESC(fast_switching, "\n fast_switching \n");
module_param(fast_switching, int, 0664);

MODULE_PARM_DESC(fsm_enhancement, "\n fsm_enhancement \n");
module_param(fsm_enhancement, int, 0664);

MODULE_PARM_DESC(port_select_ovr_en, "\n port_select_ovr_en \n");
module_param(port_select_ovr_en, int, 0664);

MODULE_PARM_DESC(phy_cmu_config_force_val, "\n phy_cmu_config_force_val \n");
module_param(phy_cmu_config_force_val, int, 0664);

MODULE_PARM_DESC(phy_system_config_force_val, "\n phy_system_config_force_val \n");
module_param(phy_system_config_force_val, int, 0664);

MODULE_PARM_DESC(acr_mode, "\n acr_mode \n");
module_param(acr_mode, int, 0664);

MODULE_PARM_DESC(hdcp_enable, "\n hdcp_enable \n");
module_param(hdcp_enable, int, 0664);

MODULE_PARM_DESC(audio_sample_rate, "\n audio_sample_rate \n");
module_param(audio_sample_rate, int, 0664);

MODULE_PARM_DESC(frame_rate, "\n frame_rate \n");
module_param(frame_rate, int, 0664);

MODULE_PARM_DESC(edid_mode, "\n edid_mode \n");
module_param(edid_mode, int, 0664);

MODULE_PARM_DESC(switch_mode, "\n switch_mode \n");
module_param(switch_mode, int, 0664);

MODULE_PARM_DESC(force_vic, "\n force_vic \n");
module_param(force_vic, int, 0664);

MODULE_PARM_DESC(force_ready, "\n force_ready \n");
module_param(force_ready, int, 0664);

MODULE_PARM_DESC(force_state, "\n force_state \n");
module_param(force_state, int, 0664);

MODULE_PARM_DESC(force_format, "\n force_format \n");
module_param(force_format, int, 0664);

MODULE_PARM_DESC(hdmirx_log_flag, "\n hdmirx_log_flag \n");
module_param(hdmirx_log_flag, int, 0664);

MODULE_PARM_DESC(hdmirx_debug_flag, "\n hdmirx_debug_flag \n");
module_param(hdmirx_debug_flag, int, 0664);

MODULE_PARM_DESC(t3d_flash_flag, "\n hdmirx_debug_flag \n");
module_param(t3d_flash_flag, int, 0664);



