/*
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

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/etherdevice.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/dma-mapping.h>
#include <asm/delay.h>
#include <asm/atomic.h>
#include <linux/module.h>

#include <linux/amports/amstream.h>
#include <linux/amports/ptsserv.h>
#include <linux/amports/canvas.h>
#include <linux/amports/vframe.h>
#include <linux/amports/vframe_provider.h>
#include <media/amlogic/656in.h>
#include <mach/am_regs.h>
#include <linux/vout/lcdoutc.h>

#include "tvin_global.h"
#include "vdin_regs.h"
#include "tvin_notifier.h"
#include "vdin.h"
#include "vdin_ctl.h"
#include "tvin_format_table.h"

#include <linux/vout/vout_notify.h>

#define DEVICE_NAME "gamma_proc"
#define MODULE_NAME "gamma_proc"

static bool gamma_dbg_en = 0;
module_param(gamma_dbg_en, bool, 0664);
MODULE_PARM_DESC(gamma_dbg_en, "enable/disable gamma log");

static int gamma_type = 2;
module_param(gamma_type, int, 0664);
MODULE_PARM_DESC(gamma_type, "adjust gamma type");

typedef struct {
	unsigned char	   rd_canvas_index;
	unsigned char	   wr_canvas_index;
	unsigned char	   buff_flag[VIUIN_VF_POOL_SIZE];

	unsigned char	   fmt_check_cnt;
	unsigned char	   watch_dog;


	unsigned		pbufAddr;
	unsigned		decbuf_size;


	unsigned		active_pixel;
	unsigned		active_line;
	unsigned		dec_status : 1;
	unsigned		wrap_flag : 1;
	unsigned		canvas_total_count : 4;
	struct tvin_parm_s para ;
}amviuin_t;


static struct vdin_dev_s *vdin_devp_viu = NULL;


amviuin_t amviuin_dec_info = {
	.pbufAddr = 0x81000000,
	.decbuf_size = 0x70000,
	.active_pixel = 720,
	.active_line = 288,
	.watch_dog = 0,
	.para = {
		.port	   = TVIN_PORT_NULL,
		.fmt_info.fmt = TVIN_SIG_FMT_NULL,
		.fmt_info.v_active=0,
		.fmt_info.h_active=0,
		.fmt_info.vsync_phase=0,
		.fmt_info.hsync_phase=0,		
		.fmt_info.frame_rate=0,
		.status	 = TVIN_SIG_STATUS_NULL,
		.cap_addr   = 0,
		.flag	   = 0,		//TVIN_PARM_FLAG_CAP
		.cap_size   = 0,
		.canvas_index   = 0,
	 },

	.rd_canvas_index = 0xff - (VIUIN_VF_POOL_SIZE + 2),
	.wr_canvas_index =  0,

	.buff_flag = {
		VIDTYPE_INTERLACE_BOTTOM,
		VIDTYPE_INTERLACE_TOP,
		VIDTYPE_INTERLACE_BOTTOM,
		VIDTYPE_INTERLACE_TOP,
		VIDTYPE_INTERLACE_BOTTOM,
		VIDTYPE_INTERLACE_TOP},

	.fmt_check_cnt = 0,
	.dec_status = 0,
	.wrap_flag = 0,
	.canvas_total_count = VIUIN_VF_POOL_SIZE,
};

static void reset_viuin_dec_parameter(void)
{
	amviuin_dec_info.wrap_flag = 0;
	amviuin_dec_info.rd_canvas_index = 
		0xff - (amviuin_dec_info.canvas_total_count + 2);
	amviuin_dec_info.wr_canvas_index =  0;
	amviuin_dec_info.fmt_check_cnt = 0;
	amviuin_dec_info.watch_dog = 0;

}

static void init_viuin_dec_parameter(fmt_info_t fmt_info)
{
	amviuin_dec_info.para.fmt_info.fmt= fmt_info.fmt;
	amviuin_dec_info.para.fmt_info.frame_rate=fmt_info.frame_rate;
	switch(fmt_info.fmt)
	{
		case TVIN_SIG_FMT_MAX+1:
		case 0xFFFF:	 //default camera
			amviuin_dec_info.active_pixel = fmt_info.h_active;
			amviuin_dec_info.active_line = fmt_info.v_active;
			amviuin_dec_info.para.fmt_info.h_active = 
							fmt_info.h_active;
			amviuin_dec_info.para.fmt_info.v_active = 
							fmt_info.v_active;
			amviuin_dec_info.para.fmt_info.hsync_phase = 
							fmt_info.hsync_phase;
			amviuin_dec_info.para.fmt_info.vsync_phase = 
							fmt_info.vsync_phase;
			pr_dbg("%dx%d input mode is selected for camera, \n",
					fmt_info.h_active,fmt_info.v_active);
			break;
		case TVIN_SIG_FMT_NULL:
		default:	 
			amviuin_dec_info.active_pixel = 0;
			amviuin_dec_info.active_line = 0;			   
			break;
	}

	return;
}

static int viu_in_canvas_init(unsigned int mem_start, unsigned int mem_size)
{
	int i, canvas_start_index ;
	unsigned int canvas_width  = 1600 << 1;
	unsigned int canvas_height = 1200;
	unsigned decbuf_start = mem_start + VIUIN_ANCI_DATA_SIZE;
	amviuin_dec_info.decbuf_size   = 0x400000;

	i = (unsigned)((mem_size - VIUIN_ANCI_DATA_SIZE) 
				/ amviuin_dec_info.decbuf_size);

	amviuin_dec_info.canvas_total_count = 
			(VIUIN_VF_POOL_SIZE > i)? i : VIUIN_VF_POOL_SIZE;

	amviuin_dec_info.pbufAddr  = mem_start;

	if(vdin_devp_viu->index )
		canvas_start_index = tvin_canvas_tab[1][0];
	else
		canvas_start_index = tvin_canvas_tab[0][0];

	for ( i = 0; i < amviuin_dec_info.canvas_total_count; i++)
	{
		canvas_config(canvas_start_index + i, 
			decbuf_start + i * amviuin_dec_info.decbuf_size,
			canvas_width, canvas_height, CANVAS_ADDR_NOWRAP, 
			CANVAS_BLKMODE_LINEAR);
	}
	set_tvin_canvas_info(canvas_start_index ,
			amviuin_dec_info.canvas_total_count );
	return 0;
}


static void viu_release_canvas(void)
{

	return;
}


static void start_amvdec_viu_in(unsigned int mem_start, 
	unsigned int mem_size, tvin_port_t port, fmt_info_t fmt_info)
{

	if (amviuin_dec_info.dec_status != 0) {
		pr_dbg("viu_in is processing," 
			" don't do starting operation \n");
		return;
	}

	if (port == TVIN_PORT_VIU_GAMMA_PROC) {
		viu_in_canvas_init(mem_start, mem_size);
		init_viuin_dec_parameter(fmt_info);
		reset_viuin_dec_parameter();

		amviuin_dec_info.para.port = port;
		amviuin_dec_info.wr_canvas_index = 0xff;
		amviuin_dec_info.dec_status = 1;
	}
	return;
}

static void stop_amvdec_viu_in(void)
{
	if (amviuin_dec_info.dec_status) {
		pr_dbg("stop viu_in decode. \n");

		viu_release_canvas();

		amviuin_dec_info.para.fmt_info.fmt= TVIN_SIG_FMT_NULL;
		amviuin_dec_info.dec_status = 0;

	} else {
		 pr_dbg("viu_in is not started yet. \n");
	}

	return;
}

static unsigned char ve_dnlp_tgt[64];
static unsigned int ve_dnlp_white_factor;
static unsigned int ve_dnlp_rt;
static unsigned int ve_dnlp_rl;
static unsigned int ve_dnlp_black;
static unsigned int ve_dnlp_white;
static unsigned int ve_dnlp_luma_sum;
static ulong ve_dnlp_lpf[64], ve_dnlp_reg[16];
static struct vframe_prop_s grapic_prop;
static unsigned int backlight;

static void ve_dnlp_calculate_tgt(struct vframe_prop_s *prop)
{
    struct vframe_prop_s *p = prop;
    ulong data[5];
    ulong i = 0, j = 0, ave = 0, max = 0, div = 0;
    

    // old historic luma sum
    j = ve_dnlp_luma_sum;
    // new historic luma sum
    ve_dnlp_luma_sum = p->hist.luma_sum;

    // picture mode: freeze dnlp curve
    if (// new luma sum is 0, something is wrong, freeze dnlp curve
        (!ve_dnlp_luma_sum) ||
        // new luma sum is closed to old one, picture mode, freeze curve
        ((ve_dnlp_luma_sum < j + (j >> 5)) &&
         (ve_dnlp_luma_sum > j - (j >> 5))
        )
       )
        return;

    // get 5 regions
    for (i = 0; i < 5; i++)
    {
        j = 4 + 11 * i;
        data[i] = (ulong)p->hist.gamma[j     ] +
                  (ulong)p->hist.gamma[j +  1] +
                  (ulong)p->hist.gamma[j +  2] +
                  (ulong)p->hist.gamma[j +  3] +
                  (ulong)p->hist.gamma[j +  4] +
                  (ulong)p->hist.gamma[j +  5] +
                  (ulong)p->hist.gamma[j +  6] +
                  (ulong)p->hist.gamma[j +  7] +
                  (ulong)p->hist.gamma[j +  8] +
                  (ulong)p->hist.gamma[j +  9] +
                  (ulong)p->hist.gamma[j + 10];
    }

    // get max, ave, div
    for (i = 0; i < 5; i++)
    {
        if (max < data[i])
            max = data[i];
        ave += data[i];
        data[i] *= 5;
    }
    max *= 5;
    div = (max - ave > ave) ? max - ave : ave;

    // invalid histgram: freeze dnlp curve
    if (!max)
        return;

    // get 1st 4 points
    for (i = 0; i < 4; i++)
    {
        if (data[i] > ave)
            data[i] = 64 + (((data[i] - ave) << 1) + div) * ve_dnlp_rl / (div << 1);
        else if (data[i] < ave)
            data[i] = 64 - (((ave - data[i]) << 1) + div) * ve_dnlp_rl / (div << 1);
        else
            data[i] = 64;
        ve_dnlp_tgt[4 + 11 * (i + 1)] = ve_dnlp_tgt[4 + 11 * i] +
                                        ((44 * data[i] + 32) >> 6);
    }

    // fill in region 0 with black extension
    data[0] = ve_dnlp_black;
    if (data[0] > 16)
        data[0] = 16;
    data[0] = (ve_dnlp_tgt[15] - ve_dnlp_tgt[4]) * (16 - data[0]);
    for (j = 1; j <= 6; j++)
        ve_dnlp_tgt[4 + j] = ve_dnlp_tgt[4] + (data[0] * j + 88) / 176;
    data[0] = (ve_dnlp_tgt[15] - ve_dnlp_tgt[10]) << 1;
    for (j = 1; j <=4; j++)
        ve_dnlp_tgt[10 + j] = ve_dnlp_tgt[10] + (data[0] * j + 5) / 10;

    // fill in regions 1~3
    for (i = 1; i <= 3; i++)
    {
        data[i] = (ve_dnlp_tgt[11 * i + 15] - ve_dnlp_tgt[11 * i + 4]) << 1;
        for (j = 1; j <= 10; j++)
            ve_dnlp_tgt[11 * i + 4 + j] = ve_dnlp_tgt[11 * i + 4] + (data[i] * j + 11) / 22;
    }

    // fill in region 4 with white extension
    data[4] /= 5;
    data[4] = (ve_dnlp_white * ((ave << 4) - data[4] * ve_dnlp_white_factor)  + (ave << 3)) / (ave << 4);
    if (data[4] > 16)
        data[4] = 16;
    data[4] = (ve_dnlp_tgt[59] - ve_dnlp_tgt[48]) * (16 - data[4]);
    for (j = 1; j <= 6; j++)
        ve_dnlp_tgt[59 - j] = ve_dnlp_tgt[59] - (data[4] * j + 88) / 176;
    data[4] = (ve_dnlp_tgt[53] - ve_dnlp_tgt[48]) << 1;
    for (j = 1; j <= 4; j++)
        ve_dnlp_tgt[53 - j] = ve_dnlp_tgt[53] - (data[4] * j + 5) / 10;
        

}

static unsigned luma_sum;
static short slope_ref;
static unsigned int gamma_proc_enable = 0;

static const unsigned short base_gamma_table[256] = {
	0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
	32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,
	64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,
	96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
	128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
	160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
	192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
	224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255
};

static unsigned short gamma_table[256];
static unsigned short pdiv[256];

extern void set_lcd_gamma_table_lvds(u16 *data, u32 rgb_mask);
extern void _set_backlight_level(int level);

int gamma_adjust2( int nLow, int nHigh, int fAlphaL, int fAlphaH, int *pDiv, int *pLut)
{
	int i, j;
	int im1, im2, ip1, ip2;
	int nStep;


	// initial pGain
	int pGain[256];
	int pSlopeL[256];
	int pSlopeH[256];
	//pGain = (int *)calloc(256, sizeof(int));
	for (i = 0; i < 256; i++) {
		pGain[i] = 2048;
	}//i

	// basic low slope
	//int *pSlopeL = NULL;
	//pSlopeL = (int *)calloc(256, sizeof(int));

	if ( fAlphaL > 10 ) {
		nStep = (int)(((fAlphaL - 10) * 4 + 5)/10);
		for (i = 0; i < 256; i++){
			pSlopeL[i] = 2048 + (255 - i) * nStep;
		}//i
	} else {
		nStep = (int)(((10 - fAlphaL) * 4 + 5)/10);
		for (i = 0; i < 256; i++){
			pSlopeL[i] = 2048 - (255 - i) * nStep;
		}//i
	}

	// basic high slope
	//int *pSlopeH = NULL;
	//pSlopeH = (int *)calloc(256, sizeof(int));

	if ( fAlphaH > 10 ){
		nStep = (int)(((fAlphaH - 10) * 4 + 5)/10);
		for (i = 0; i < 256; i++){
			pSlopeH[i] = 2048 + (255 - i) * nStep;
		}//i
	} else {
		nStep = (int)(((10 - fAlphaH) * 4 + 5)/10);
		for (i = 0; i < 256; i++){
			pSlopeH[i] = 2048 - (255 - i) * nStep;
		}//i
	}

	// remapping to dark/bright curve
	for (i = 0; i < nLow; i++){
		j = (i * pDiv[nLow] + 1024) / 2048;
		pGain[i] = pSlopeL[j];
	}

	for (i = nHigh; i < 256; i++) {
		j = ((i - nHigh) * pDiv[255 - nHigh] + 1024) / 2048;
		pGain[i] = pSlopeH[j];
	}
	if (gamma_dbg_en) {

		for (i = 0; i < 256; i++) {
			printk("pLut11111[%d] = %d\n", i, pLut[i]);
		}	
	}

	// adjust lut
	for (i = 0; i < 256; i++) {
		if ( i < nHigh )
			pLut[i] = (pGain[i] * pLut[i] + 1024) / 2048;
		else
			pLut[i] = pLut[nHigh] + (pGain[i] * 
				(pLut[i] - pLut[nHigh]) + 1024) / 2048;
		//printf("%d ", pGain[i]);
	}

	if(gamma_dbg_en) {
		for (i = 0; i < 256; i++) {
			printk("pLut[%d] = %d\n", i, pLut[i]);
		}	
	}

	// 5 tap lpf
	for (i = 0; i < 256; i++) {
		im1 = i-1 < 0 ? 0 : i-1;
		im2 = i-2 < 0 ? 0 : i-2;
		ip1 = i+1 > 255 ? 255 : i+1;
		ip2 = i+2 > 255 ? 255 : i+2;

		pLut[i] = (pLut[im2] + 2 * pLut[im1] + 2 * pLut[i] +
					 2 * pLut[ip1] + pLut[ip2] + 4) /8;
		
	}

	return 0;
}

int nLow = 60;
int nHigh = 190;
int fAlphaL = 10;//<=2
int fAlphaH = 10;//>=0

int gamma_adjust(void)
{
	//printk("luma_sum = %d \n", luma_sum);
	int i, j;
	static debug = 0;
	unsigned long flags;
		// set parameters

	// caluclate and save to mem before
	//int *pLut = NULL;
	//pLut = (int *)calloc(256, sizeof(int));

	int pDiv[256];
	//pDiv = (int *)calloc(256, sizeof(int));
	for (i = 0; i < 256; i++){
		pDiv[i] = 256 * 2048 / (i+1);//256?
	}//i
	if (!gamma_proc_enable) 
		return 0;
	if (gamma_proc_enable == 2) {
		for (i = 0; i < 256; i++){
			gamma_table[i] = base_gamma_table[i]<<2;
			if(gamma_dbg_en)
			printk("gamma_table[%d] = %d\n",
					 i, gamma_table[i]);
		}
		local_irq_save(flags);
		set_lcd_gamma_table_lvds(gamma_table, LCD_H_SEL_R);
		set_lcd_gamma_table_lvds(gamma_table, LCD_H_SEL_G);
		set_lcd_gamma_table_lvds(gamma_table, LCD_H_SEL_B);
		gamma_proc_enable = 0;
		local_irq_restore(flags);
		return 0;
	}
	if (gamma_type == 1) {
		gamma_proc_enable = 0;
		for (i = 0; i < 256; i++) {
			gamma_table[i] = base_gamma_table[i]<<2;
		}	
		for (i = 1; i < 64; i++)
			for (j = 0; j < 4; j++) {
				gamma_table[i*4 + j] =
					 base_gamma_table[i*4 + j]*ve_dnlp_tgt[i]/i;
			}
		if (gamma_dbg_en) {
			for (i = 0; i < 256; i++){
				printk("gamma_table[%d] = %d\n",
						 i, gamma_table[i]);
			    }	
		}
		set_lcd_gamma_table_lvds(gamma_table, LCD_H_SEL_R);
		set_lcd_gamma_table_lvds(gamma_table, LCD_H_SEL_G);
		set_lcd_gamma_table_lvds(gamma_table, LCD_H_SEL_B);
		//printk("gamma_adjust\n");
	} else if (gamma_type == 2) {
		int pLut[256];
		gamma_proc_enable = 0;
		for (i = 0; i < 256; i++) {
			gamma_table[i] = base_gamma_table[i]<<2;
		}	
		for (i = 1; i < 64; i++)
			for (j = 0; j < 4; j++) {
				gamma_table[i*4 + j] =
					 base_gamma_table[i*4 + j]*ve_dnlp_tgt[i]/i;
			}
		for (i = 0; i < 256; i++) {
			pLut[i] = gamma_table[i];
			//printf("%d ", pLut[i]);
		}//i

		gamma_adjust2( nLow, nHigh, fAlphaL, fAlphaH, pDiv, pLut);
		for (i = 0; i < 256; i++){
			gamma_table[i] = pLut[i];
		}				
		set_lcd_gamma_table_lvds(gamma_table, LCD_H_SEL_R);
		set_lcd_gamma_table_lvds(gamma_table, LCD_H_SEL_G);
		set_lcd_gamma_table_lvds(gamma_table, LCD_H_SEL_B);
		//printk("gamma_adjust\n");
	}	
		
	return 0;
}

/*as use the spin_lock,
 *1--there is no sleep,
 *2--it is better to shorter the time,
*/
int amvdec_viu_in_run(vframe_t *info)
{
	unsigned ccir656_status;
	unsigned char canvas_id = 0;
	unsigned char last_receiver_buff_index = 0;
	
	//printk("bbb\n");
	if (amviuin_dec_info.dec_status == 0) {
		pr_error("viuin decoder is not started\n");
		return -1;
	}
	//printk("ccc\n");
	if(amviuin_dec_info.active_pixel == 0){
		return -1;	
	}
	
	amviuin_dec_info.watch_dog = 0;

	/*if(vdin_devp_viu->para.flag & TVIN_PARM_FLAG_CAP) {
		if(amviuin_dec_info.wr_canvas_index == 0)
			  last_receiver_buff_index = 
			  	amviuin_dec_info.canvas_total_count - 1;
		else
			  last_receiver_buff_index = 
			  	amviuin_dec_info.wr_canvas_index - 1;

		if(last_receiver_buff_index != 
				amviuin_dec_info.rd_canvas_index)
			canvas_id = last_receiver_buff_index;
		else {
			if(amviuin_dec_info.wr_canvas_index == 
				amviuin_dec_info.canvas_total_count - 1)
				canvas_id = 0;
			else
				canvas_id = 
				  	amviuin_dec_info.wr_canvas_index + 1;
		}
		vdin_devp_viu->para.cap_addr = amviuin_dec_info.pbufAddr +
				(amviuin_dec_info.decbuf_size * canvas_id) + 
				VIUIN_ANCI_DATA_SIZE ;
		vdin_devp_viu->para.cap_size = amviuin_dec_info.decbuf_size;
		vdin_devp_viu->para.canvas_index =VDIN_START_CANVAS+ canvas_id;
		return 0;
	}*/

	if (amviuin_dec_info.para.port == TVIN_PORT_VIU_GAMMA_PROC) {	
		extern get_graphic_prop(vframe_prop_t* prop, unsigned int offset);
		get_graphic_prop(&grapic_prop, 0);
		// calculate dnlp target data
		ve_dnlp_calculate_tgt(&grapic_prop);
		//printk("aaa\n");
		gamma_proc_enable = 1;
	}
	return 0;
}

static int viuin_check_callback(struct notifier_block *block,
	 unsigned long cmd , void *para)
{
	int ret = 0;
	switch (cmd) {
	case TVIN_EVENT_INFO_CHECK:
		break;
	default:
		break;
	}
	return ret;
}

struct tvin_dec_ops_s viuin_op = {
	.dec_run = amvdec_viu_in_run,		
};


static struct notifier_block viuin_check_notifier = {
	.notifier_call  = viuin_check_callback,
};


static int viuin_notifier_callback(struct notifier_block *block, 
	unsigned long cmd , void *para)
{
	int ret = 0;
	vdin_dev_t *p = NULL;
	switch (cmd) {
	case TVIN_EVENT_DEC_START:
		if( para != NULL) {
			p = (vdin_dev_t*)para;
			if (p->para.port != TVIN_PORT_VIU_GAMMA_PROC) {
				pr_info("viuin: ignore TVIN_EVENT_DEC_START "
				"(port=%d)\n", p->para.port);
				return ret;
			}
			pr_info("viuin_notifier_callback, para = %x ,"
				"mem_start = %x, port = %d, format = %d,"
				" ca-_flag = %d.\n" ,
				(unsigned int)para, p->mem_start, 
				p->para.port, p->para.fmt_info.fmt,
				p->para.flag);
			vdin_devp_viu = p;
			start_amvdec_viu_in(p->mem_start,p->mem_size, 
					p->para.port, p->para.fmt_info);
			amviuin_dec_info.para.status = TVIN_SIG_STATUS_STABLE;
			vdin_info_update(p, &amviuin_dec_info.para);
			tvin_dec_register(p, &viuin_op);
			tvin_check_notifier_register(&viuin_check_notifier);
			ret = NOTIFY_STOP_MASK;
		}
		break;
	case TVIN_EVENT_DEC_STOP:
		if (para != NULL) {
			p = (vdin_dev_t*)para;
			pr_info("TVIN_EVENT_DEC_STOP, port=%d \n", 
				p->para.port);
			if (p->para.port != TVIN_PORT_VIU_GAMMA_PROC) {
				pr_info("viuin: ignore TVIN_EVENT_DEC_STOP, "
				"port=%d \n", p->para.port);
				return ret;
			}
			stop_amvdec_viu_in();
			tvin_dec_unregister(vdin_devp_viu);
			tvin_check_notifier_unregister(&viuin_check_notifier);
			vdin_devp_viu = NULL;
			ret = NOTIFY_STOP_MASK;
			gamma_proc_enable = 2;
		}
		break;

	default:
		break;
	}
	return ret;
}

static int notify_callback_v(struct notifier_block *block, 
	unsigned long cmd , void *p)
{
	return 0;
}


static struct notifier_block viuin_notifier = {
	.notifier_call  = viuin_notifier_callback,
};

static struct notifier_block notifier_nb_v = {
	.notifier_call	= notify_callback_v,
};

static ssize_t gamma_proc_show(struct class *cla, 
		struct class_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", gamma_proc_enable);
}

// [   28] en    0~1
// [27:24] rt    0~7
// [23:16] rl    0~255
// [15: 8] black 0~16
// [ 7: 0] white 0~16
//0x10200202

static ssize_t 
gamma_proc_store(struct class *cla,struct class_attribute *attr, 
				const char *buf, size_t count)
{
	size_t r;
	s32 val = 0;
	tvin_parm_t para;
	int en;
	static int gamma_proc_on = 0;

	r = sscanf(buf, "0x%x", &val);

	if (r != 1){
		return -EINVAL;
	}
	printk("val = %x, val>>28 = %d\n", val, val>>28);
	en = (val>>28)&0x1;
    
	if (en) {  
		ve_dnlp_rt = (val>>24)&0xf;  //7
		ve_dnlp_rl = (val>>16)&0xff; //0
		ve_dnlp_black = (val>>8)&0xff;   //2
		ve_dnlp_white = val&0xff;     //2
		if (ve_dnlp_rl > 64)
			ve_dnlp_rl = 64;
		if (ve_dnlp_black > 16)
			ve_dnlp_black = 16;
		if (ve_dnlp_white > 16)
			ve_dnlp_white = 16;
		
		para.port  = TVIN_PORT_VIU_GAMMA_PROC;
		para.fmt_info.fmt = TVIN_SIG_FMT_MAX+1;
		para.fmt_info.frame_rate = 50;
		para.fmt_info.h_active = 1024;
		para.fmt_info.v_active = 768;
		para.fmt_info.hsync_phase = 1;
		para.fmt_info.vsync_phase  = 0;	
		if (gamma_proc_on == 0) {
			aml_write_reg32(P_VPU_VIU_VENC_MUX_CTRL, (aml_read_reg32(P_VPU_VIU_VENC_MUX_CTRL)&(~0xff3))|0x880);
			start_tvin_service(0,&para);
			gamma_proc_on = 1;
			printk("start gamma calc function\n");
		}
	} else {
		if (gamma_proc_on) {
			stop_tvin_service(0);
			gamma_proc_on = 0;
			printk("stop gamma calc function\n");
		}
	}

	return count;
}

static ssize_t env_backlight_show(struct class *cla, 
		struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", backlight);
}


static ssize_t env_backlight_store(struct class *cla, 
		struct class_attribute *attr, const char *buf, size_t count)
{
	size_t r;
	s32 val = 0;
	tvin_parm_t para;

	r = sscanf(buf, "0x%x", &val);

	if (r != 1) {
		return -EINVAL;
	}
	printk("val = %x\n", val);
	backlight = val;
    
	if (backlight > 255)
		backlight = 255;
	_set_backlight_level(backlight);
	if (backlight > 0xa0)
		fAlphaL = 17;
	else
		fAlphaL = 10;
		
	return count;
}

static struct class* gamma_proc_clsp;

static struct class_attribute gamma_proc_class_attrs[] = {
	__ATTR(gamma_proc, S_IRUGO | S_IWUSR, 
		gamma_proc_show, gamma_proc_store),
	__ATTR(env_backlight, S_IRUGO | S_IWUSR, 
		env_backlight_show, env_backlight_store),
	__ATTR_NULL,
};

static int gamma_proc_probe(struct platform_device *pdev)
{
	int ret = 0;
	int i;
	gamma_proc_clsp = class_create(THIS_MODULE, "aml_gamma_proc");
	if(IS_ERR(gamma_proc_clsp)){
		ret = PTR_ERR(gamma_proc_clsp);
		return ret;
	}
	for(i = 0; gamma_proc_class_attrs[i].attr.name; i++){
		if(class_create_file(gamma_proc_clsp, 
				&gamma_proc_class_attrs[i]) < 0)
			goto err;
	}
	tvin_dec_notifier_register(&viuin_notifier);
	vout_register_client(&notifier_nb_v);

	for (i = 0; i < 64; i++) {
		ve_dnlp_tgt[i] = i << 2;
		//ve_dnlp_lpf[i] = ve_dnlp_tgt[i] << ve_dnlp_rt;
	}
	return 0;
err:
	for(i=0; gamma_proc_class_attrs[i].attr.name; i++){
		class_remove_file(gamma_proc_clsp, 
				&gamma_proc_class_attrs[i]);
	}
	class_destroy(gamma_proc_clsp); 
	return -1;  
}

static int gamma_proc_remove(struct platform_device *pdev)
{
	/* Remove the cdev */
	tvin_dec_notifier_unregister(&viuin_notifier);
	vout_unregister_client(&notifier_nb_v);
	return 0;
}

static struct platform_driver gamma_proc_driver = {
	.probe	= gamma_proc_probe,
	.remove	= gamma_proc_remove,
	.driver	= {
		.name	= DEVICE_NAME,
	}
};

static struct platform_device* gamma_proc_device = NULL;

static int __init gamma_proc_init_module(void)
{
	pr_dbg("gamma_proc module init\n");
	gamma_proc_device = platform_device_alloc(DEVICE_NAME,0);
	if (!gamma_proc_device) {
		pr_error("failed to alloc gamma_proc_device\n");
		return -ENOMEM;
	}

	if (platform_device_add(gamma_proc_device)) {
		platform_device_put(gamma_proc_device);
		pr_error("failed to add gamma_proc_device\n");
		return -ENODEV;
	}
	if (platform_driver_register(&gamma_proc_driver)) {
		pr_error("failed to register gamma_proc driver\n");

		platform_device_del(gamma_proc_device);
		platform_device_put(gamma_proc_device);
		return -ENODEV;
	}

	return 0;
}

static void __exit gamma_proc_exit_module(void)
{
	pr_dbg("gamma_proc module remove.\n");
	platform_driver_unregister(&gamma_proc_driver);
	return ;
}


module_init(gamma_proc_init_module);
module_exit(gamma_proc_exit_module);


