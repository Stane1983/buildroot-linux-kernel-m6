/*
 * Amlogic M6
 * frame buffer driver  -------it660x input
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

#include <linux/amports/amstream.h>
#include <linux/amports/ptsserv.h>
#include <linux/amports/canvas.h>
#include <linux/amports/vframe.h>
#include <linux/amports/vframe_provider.h>
#include <media/amlogic/656in.h>
#include <mach/am_regs.h>

#include "../tvin_global.h"
#include "../vdin_regs.h"
#include "../tvin_notifier.h"
#include "../vdin.h"
#include "../vdin_ctl.h"
#include "../tvin_format_table.h"
#include "dvin.h"

#include <linux/vout/vout_notify.h>

#define DEVICE_NAME "amvdec_it660xin"
#define MODULE_NAME "amvdec_it660xin"
#define IT660X_IRQ_NAME "amvdec_it660xin-irq"




typedef struct {
    unsigned char       rd_canvas_index;
    unsigned char       wr_canvas_index;
    unsigned char       buff_flag[IT660XIN_VF_POOL_SIZE];

    unsigned char       fmt_check_cnt;
    unsigned char       watch_dog;


    unsigned        pbufAddr;
    unsigned        decbuf_size;


    unsigned        active_pixel;
    unsigned        active_line;
    unsigned        dec_status : 1;
    unsigned        wrap_flag : 1;
    unsigned        canvas_total_count : 4;
    struct tvin_parm_s para ;



}amit660xin_t;


static struct vdin_dev_s *vdin_devp_it660x = NULL;


amit660xin_t amit660xin_dec_info = {
    .pbufAddr = 0x81000000,
    .decbuf_size = 0x70000,
    .active_pixel = 720,
    .active_line = 288,
    .watch_dog = 0,
    .para = {
        .port       = TVIN_PORT_NULL,
		.fmt_info.fmt = TVIN_SIG_FMT_NULL,
        .fmt_info.v_active=0,
        .fmt_info.h_active=0,
        .fmt_info.vsync_phase=0,
        .fmt_info.hsync_phase=0,        
        .fmt_info.frame_rate=0,
        .status     = TVIN_SIG_STATUS_NULL,
        .cap_addr   = 0,
        .flag       = 0,        //TVIN_PARM_FLAG_CAP
        .cap_size   = 0,
        .canvas_index   = 0,
     },

    .rd_canvas_index = 0xff - (IT660XIN_VF_POOL_SIZE + 2),
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
    .canvas_total_count = IT660XIN_VF_POOL_SIZE,
};


#ifdef HANDLE_IT660XIN_IRQ
static const char it660xin_dec_id[] = "it660xin-dev";
#endif


static vdin0_select_in (int vdin_sel, int comp0_switch, int comp1_switch, int comp2_switch, int decimate_ctrl, int width)
{
   WRITE_MPEG_REG_BITS(VDIN_COM_CTRL0, comp0_switch, 6, 2);
   WRITE_MPEG_REG_BITS(VDIN_COM_CTRL0, comp1_switch, 8, 2);
   WRITE_MPEG_REG_BITS(VDIN_COM_CTRL0, comp2_switch, 10, 2);
   WRITE_MPEG_REG_BITS(VDIN_COM_CTRL0, vdin_sel, 0, 4);
   WRITE_MPEG_REG_BITS(VDIN_COM_CTRL0, 1, 4, 1);       //common_din_enable

   switch (vdin_sel) {
     case (1):  //mpeg_in
       break;
     case (2):  //video1 in
       WRITE_MPEG_REG_BITS(VDIN_ASFIFO_CTRL0, 0x39, 2, 6); 
       break;
     case (3):  //video2 in
       WRITE_MPEG_REG_BITS(VDIN_ASFIFO_CTRL0, 0x39, 18, 6); 
       break;
     case (4):  //video3 in
       WRITE_MPEG_REG_BITS(VDIN_ASFIFO_CTRL1, 0x39, 2, 6); 
       break;
     case (5):  //video4 in
       WRITE_MPEG_REG_BITS(VDIN_ASFIFO_CTRL1, 0x39, 18, 6); 
       break;
     case (6):  //video5 in
       WRITE_MPEG_REG_BITS(VDIN_ASFIFO_CTRL2, 0x39, 2, 6); 
       break;
     case (7):  //video6 in
       WRITE_MPEG_REG_BITS(VDIN_ASFIFO_CTRL3, 0x39, 2, 6); 
       break;
     case (8):  //video7 in
       WRITE_MPEG_REG_BITS(VDIN_ASFIFO_CTRL3, 0x39, 10, 6); 
       break;
   }
  WRITE_MPEG_REG_BITS(VDIN_ASFIFO_CTRL2, decimate_ctrl, 16, 10); 
  WRITE_MPEG_REG(VDIN_INTF_WIDTHM1, width - 1);  
}


static void vdin0_set_matrix_rgb2ycbcr (int mode)
{
   WRITE_MPEG_REG_BITS(VDIN_MATRIX_CTRL, 1, 0, 1);

   if (mode == 0) //ycbcr not full range, 601 conversion 
   {

        WRITE_MPEG_REG(VDIN_MATRIX_PRE_OFFSET0_1, 0x0);
        WRITE_MPEG_REG(VDIN_MATRIX_PRE_OFFSET2, 0x0);
        
        //0.257     0.504   0.098
        //-0.148    -0.291  0.439
        //0.439     -0.368 -0.071
        WRITE_MPEG_REG(VDIN_MATRIX_COEF00_01, (0x107 << 16) |
                              0x204);
        WRITE_MPEG_REG(VDIN_MATRIX_COEF02_10, (0x64 << 16) |
                              0x1f68);
        WRITE_MPEG_REG(VDIN_MATRIX_COEF11_12, (0x1ed6 << 16) |
                                 0x1c2);
        WRITE_MPEG_REG(VDIN_MATRIX_COEF20_21, (0x1c2 << 16) | 
                             0x1e87);
        WRITE_MPEG_REG(VDIN_MATRIX_COEF22, 0x1fb7);
        WRITE_MPEG_REG(VDIN_MATRIX_OFFSET0_1, (0x40 << 16) | 0x0200);
        WRITE_MPEG_REG(VDIN_MATRIX_OFFSET2, 0x0200);
   } 
   else if (mode == 1) //ycbcr full range, 601 conversion
   {
   }

}


static void init_vdin(int width, int format)
{
    WRITE_MPEG_REG(PERIPHS_PIN_MUX_0, READ_MPEG_REG(PERIPHS_PIN_MUX_0)|
                              ((1 << 10)    | // pm_gpioA_lcd_in_de
                               (1 << 9)     | // pm_gpioA_lcd_in_vs
                               (1 << 8)     | // pm_gpioA_lcd_in_hs
                               (1 << 7)     | // pm_gpioA_lcd_in_clk
                               (1 << 6)));     // pm_gpioA_lcd_in
    
    WRITE_MPEG_REG_BITS(VDIN_ASFIFO_CTRL2, 0x39, 2, 6); 

    //if(format==TVIN_SIG_FMT_HDMI_1440x480I_60Hz){
    if(width == 1440){
        // ratio
        WRITE_CBUS_REG_BITS(VDIN_ASFIFO_CTRL2, 1, 16, 4);
    	// en
        WRITE_CBUS_REG_BITS(VDIN_ASFIFO_CTRL2, 1, 24, 1);
        // manual reset, rst = 1 & 0
        WRITE_CBUS_REG_BITS(VDIN_ASFIFO_CTRL2, 1, 25, 1);
        WRITE_CBUS_REG_BITS(VDIN_ASFIFO_CTRL2, 0, 25, 1);

        // output_width_m1
        WRITE_MPEG_REG(VDIN_INTF_WIDTHM1, width/2 - 1);  
    }
    else{
        WRITE_MPEG_REG_BITS(VDIN_ASFIFO_CTRL2, 0, 16, 10);
        WRITE_MPEG_REG(VDIN_INTF_WIDTHM1, width - 1);  
    }
    //vdin0_select_in(6, 2, 1, 0, 0, width);
    //vdin0_set_matrix_rgb2ycbcr(0);
}    

static void reset_it660xin_dec_parameter(void)
{
    amit660xin_dec_info.wrap_flag = 0;
    amit660xin_dec_info.rd_canvas_index = 0xff - (amit660xin_dec_info.canvas_total_count + 2);
    amit660xin_dec_info.wr_canvas_index =  0;
    amit660xin_dec_info.fmt_check_cnt = 0;
    amit660xin_dec_info.watch_dog = 0;

}

static void init_it660xin_dec_parameter(fmt_info_t fmt_info)
{
    amit660xin_dec_info.para.fmt_info.fmt= fmt_info.fmt;
	amit660xin_dec_info.para.fmt_info.frame_rate=fmt_info.frame_rate;
    switch(fmt_info.fmt)
    {
        case TVIN_SIG_FMT_NULL:
            amit660xin_dec_info.active_pixel = 0;
            amit660xin_dec_info.active_line = 0;               
            break;
        default:
            amit660xin_dec_info.active_pixel = fmt_info.h_active;
            //if(amit660xin_dec_info.para.fmt_info.fmt == TVIN_SIG_FMT_HDMI_1440x480I_60Hz){
            if(amit660xin_dec_info.active_pixel == 1440){
                amit660xin_dec_info.active_pixel = 1440/2;
            }
            amit660xin_dec_info.active_line = fmt_info.v_active;
			      amit660xin_dec_info.para.fmt_info.h_active = fmt_info.h_active;
            amit660xin_dec_info.para.fmt_info.v_active = fmt_info.v_active;
            amit660xin_dec_info.para.fmt_info.hsync_phase = fmt_info.hsync_phase;
            amit660xin_dec_info.para.fmt_info.vsync_phase = fmt_info.vsync_phase;
            pr_dbg("%dx%d input mode is selected for camera, \n",fmt_info.h_active,fmt_info.v_active);
            break;
    }

    return;
}

static int it660x_in_canvas_init(unsigned int mem_start, unsigned int mem_size)
{
    int i, canvas_start_index ;
    unsigned int canvas_width  = 1920 << 1;
    unsigned int canvas_height = 1080;
    unsigned decbuf_start = mem_start + IT660XIN_ANCI_DATA_SIZE;
    amit660xin_dec_info.decbuf_size   = 0x400000;

    i = (unsigned)((mem_size - IT660XIN_ANCI_DATA_SIZE) / amit660xin_dec_info.decbuf_size);

    amit660xin_dec_info.canvas_total_count = (IT660XIN_VF_POOL_SIZE > i)? i : IT660XIN_VF_POOL_SIZE;

    amit660xin_dec_info.pbufAddr  = mem_start;

    if(vdin_devp_it660x->index )
        canvas_start_index = tvin_canvas_tab[1][0];
    else
        canvas_start_index = tvin_canvas_tab[0][0];

    for ( i = 0; i < amit660xin_dec_info.canvas_total_count; i++)
    {
        canvas_config(canvas_start_index + i, decbuf_start + i * amit660xin_dec_info.decbuf_size,
            canvas_width, canvas_height, CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);
    }
    set_tvin_canvas_info(canvas_start_index ,amit660xin_dec_info.canvas_total_count );
    return 0;
}


static void it660x_release_canvas(void)
{
//    int i = 0;

//    for(;i < amit660xin_dec_info.canvas_total_count; i++)
//    {
//        canvas_release(VDIN_START_CANVAS + i);
//    }
    return;
}


static inline u32 it660x_index2canvas(u32 index)
{
    int tv_group_index ;

    if(vdin_devp_it660x->index )
        tv_group_index = 1;
    else
        tv_group_index = 0;

    if(index < amit660xin_dec_info.canvas_total_count)
        return tvin_canvas_tab[tv_group_index][index];
    else
        return 0xff;
}


static void start_amvdec_it660x_in(unsigned int mem_start, unsigned int mem_size,
                        tvin_port_t port, fmt_info_t fmt_info)
{

    if(amit660xin_dec_info.dec_status != 0)
    {
        pr_dbg("it660x_in is processing, don't do starting operation \n");
        return;
    }

    pr_dbg("start ");
    if(port == TVIN_PORT_DVIN0)
    {
        pr_dbg("it660x in decode. \n");
        it660x_in_canvas_init(mem_start, mem_size);
        init_it660xin_dec_parameter(fmt_info);
        init_vdin(fmt_info.h_active, amit660xin_dec_info.para.fmt_info.fmt);
        reset_it660xin_dec_parameter();

        amit660xin_dec_info.para.port = TVIN_PORT_DVIN0;
		    amit660xin_dec_info.wr_canvas_index = 0xff;
        amit660xin_dec_info.dec_status = 1;
    }
    return;
}

static void stop_amvdec_it660x_in(void)
{
    if(amit660xin_dec_info.dec_status)
    {
        pr_dbg("stop it660x_in decode. \n");

        it660x_release_canvas();

        amit660xin_dec_info.para.fmt_info.fmt= TVIN_SIG_FMT_NULL;
        amit660xin_dec_info.dec_status = 0;

    }
    else
    {
         pr_dbg("it660x_in is not started yet. \n");
    }

    return;
}

static void it660x_send_buff_to_display_fifo(vframe_t * info)
{
    if((amit660xin_dec_info.active_pixel == 720)&&(amit660xin_dec_info.para.fmt_info.fmt == TVIN_SIG_FMT_HDMI_1920x1080I_60Hz )){
        unsigned char field_status = READ_CBUS_REG_BITS(VDIN_COM_STATUS0, 0, 1);
        if(((READ_CBUS_REG(VDIN_WR_CTRL)>>24)&0x7) == 0x5){
            field_status = field_status?0:1;     
        }

        if(field_status == 1){
            info->type = VIDTYPE_VIU_SINGLE_PLANE | VIDTYPE_VIU_422 | VIDTYPE_VIU_FIELD | VIDTYPE_INTERLACE_BOTTOM ;
        }
        else{
            info->type = VIDTYPE_VIU_SINGLE_PLANE | VIDTYPE_VIU_422 | VIDTYPE_VIU_FIELD | VIDTYPE_INTERLACE_TOP ;
        }        
    }
    else if(amit660xin_dec_info.para.fmt_info.fmt == TVIN_SIG_FMT_HDMI_1920x1080I_60Hz ){
        unsigned char field_status = READ_CBUS_REG_BITS(VDIN_COM_STATUS0, 0, 1);
        if(((READ_CBUS_REG(VDIN_WR_CTRL)>>24)&0x7) == 0x5){
            field_status = field_status?0:1;     
        }
        //if(field_status == 1){
        if(field_status == 0){
            info->type = VIDTYPE_VIU_SINGLE_PLANE | VIDTYPE_VIU_422 | VIDTYPE_VIU_FIELD | VIDTYPE_INTERLACE_BOTTOM ;
        }
        else{
            info->type = VIDTYPE_VIU_SINGLE_PLANE | VIDTYPE_VIU_422 | VIDTYPE_VIU_FIELD | VIDTYPE_INTERLACE_TOP ;
        }        
    }
    else{
        info->type = VIDTYPE_VIU_SINGLE_PLANE | VIDTYPE_VIU_422 | VIDTYPE_VIU_FIELD | VIDTYPE_PROGRESSIVE ;
    }
    info->width= amit660xin_dec_info.active_pixel;
    //info->height = amit660xin_dec_info.active_line;
    info->height = amit660xin_dec_info.active_line - 1;    
    //info->duration = 960000/amit660xin_dec_info.para.fmt_info.frame_rate;
    info->duration = 0;
    info->canvas0Addr = info->canvas1Addr = it660x_index2canvas(amit660xin_dec_info.wr_canvas_index);
    info->pts = 0;
}


static void it660xin_wr_vdin_canvas(void)
{
    unsigned char canvas_id;
    amit660xin_dec_info.wr_canvas_index += 1;
    if(amit660xin_dec_info.wr_canvas_index > (amit660xin_dec_info.canvas_total_count -1) )
    {
        amit660xin_dec_info.wr_canvas_index = 0;
        amit660xin_dec_info.wrap_flag = 1;
    }
    canvas_id = it660x_index2canvas(amit660xin_dec_info.wr_canvas_index);

    vdin_set_canvas_id(vdin_devp_it660x->index, canvas_id);
}

static void it660x_wr_vdin_canvas(int index)
{
	vdin_set_canvas_id(vdin_devp_it660x->index, it660x_index2canvas(index));
}

static void it660x_in_dec_run(vframe_t * info)
{
	if (amit660xin_dec_info.wr_canvas_index == 0xff) {
		it660x_wr_vdin_canvas(0);
		amit660xin_dec_info.wr_canvas_index = 0;
		return;
	}
	
	it660x_send_buff_to_display_fifo(info);

	amit660xin_dec_info.wr_canvas_index++;
	if (amit660xin_dec_info.wr_canvas_index >= amit660xin_dec_info.canvas_total_count)
		amit660xin_dec_info.wr_canvas_index = 0;
		
	it660x_wr_vdin_canvas(amit660xin_dec_info.wr_canvas_index);

    return;
}

/*as use the spin_lock,
 *1--there is no sleep,
 *2--it is better to shorter the time,
*/
int amvdec_it660x_in_run(vdin_dev_t* devp,vframe_t *info)
{
    unsigned ccir656_status;
    unsigned char canvas_id = 0;
    unsigned char last_receiver_buff_index = 0;

    if(amit660xin_dec_info.dec_status == 0){
//        pr_error("it660xin decoder is not started\n");
        return -1;
    }
    
    if(amit660xin_dec_info.active_pixel == 0){
    	return -1;	
    }
    amit660xin_dec_info.watch_dog = 0;

    if(vdin_devp_it660x->para.flag & TVIN_PARM_FLAG_CAP)
    {
        if(amit660xin_dec_info.wr_canvas_index == 0)
              last_receiver_buff_index = amit660xin_dec_info.canvas_total_count - 1;
        else
              last_receiver_buff_index = amit660xin_dec_info.wr_canvas_index - 1;

        if(last_receiver_buff_index != amit660xin_dec_info.rd_canvas_index)
            canvas_id = last_receiver_buff_index;
        else
        {
            if(amit660xin_dec_info.wr_canvas_index == amit660xin_dec_info.canvas_total_count - 1)
                  canvas_id = 0;
            else
                  canvas_id = amit660xin_dec_info.wr_canvas_index + 1;
        }
        vdin_devp_it660x->para.cap_addr = amit660xin_dec_info.pbufAddr +
                (amit660xin_dec_info.decbuf_size * canvas_id) + IT660XIN_ANCI_DATA_SIZE ;
        vdin_devp_it660x->para.cap_size = amit660xin_dec_info.decbuf_size;
        vdin_devp_it660x->para.canvas_index =VDIN_START_CANVAS+ canvas_id;
        return 0;
    }

    if(amit660xin_dec_info.para.port == TVIN_PORT_DVIN0)
    {
        it660x_in_dec_run(info);
    }
    return 0;
}


static int it660xin_check_callback(struct notifier_block *block, unsigned long cmd , void *para)
{
    int ret = 0;
    switch(cmd)
    {
        case TVIN_EVENT_INFO_CHECK:
            break;
        default:
            break;
    }
    return ret;
}


struct tvin_dec_ops_s it660xin_op = {
        .dec_run = amvdec_it660x_in_run,        
    };


static struct notifier_block it660xin_check_notifier = {
    .notifier_call  = it660xin_check_callback,
};


static int it660xin_notifier_callback(struct notifier_block *block, unsigned long cmd , void *para)
{
    int ret = 0;
    vdin_dev_t *p = NULL;
    switch(cmd)
    {
        case TVIN_EVENT_DEC_START:
            if(para != NULL)
            {
                p = (vdin_dev_t*)para;
                if (p->para.port != TVIN_PORT_DVIN0)
                {
                    pr_info("it660xin: ignore TVIN_EVENT_DEC_START (port=%d)\n", p->para.port);
                    return ret;
                }
                pr_dbg("it660xin_notifier_callback, para = %x ,mem_start = %x, port = %d, format = %d, ca-_flag = %d.\n" ,
                        (unsigned int)para, p->mem_start, p->para.port, p->para.fmt_info.fmt, p->para.flag);
                vdin_devp_it660x = p;
                start_amvdec_it660x_in(p->mem_start,p->mem_size, p->para.port, p->para.fmt_info);
                amit660xin_dec_info.para.status = TVIN_SIG_STATUS_STABLE;
                vdin_info_update(p, &amit660xin_dec_info.para);
                tvin_dec_register(p, &it660xin_op);
                tvin_check_notifier_register(&it660xin_check_notifier);
                ret = NOTIFY_STOP_MASK;
            }
            break;

        case TVIN_EVENT_DEC_STOP:
            if(para != NULL)
            {
                p = (vdin_dev_t*)para;
                if(p->para.port != TVIN_PORT_DVIN0)
                {
                    pr_info("it660xin: ignore TVIN_EVENT_DEC_STOP, port=%d \n", p->para.port);
                    return ret;
                }
                stop_amvdec_it660x_in();
                tvin_dec_unregister(vdin_devp_it660x);
                tvin_check_notifier_unregister(&it660xin_check_notifier);
                vdin_devp_it660x = NULL;
                ret = NOTIFY_STOP_MASK;
            }
            break;

        default:
            break;
    }
    return ret;
}

static int notify_callback_v(struct notifier_block *block, unsigned long cmd , void *p)
{
    return 0;
}


static struct notifier_block it660xin_notifier = {
    .notifier_call  = it660xin_notifier_callback,
};

static struct notifier_block notifier_nb_v = {
    .notifier_call    = notify_callback_v,
};


int it660xin_init(void)
{
    tvin_dec_notifier_register(&it660xin_notifier);

    vout_register_client(&notifier_nb_v);
    return 0;
}

int it660xin_remove(void)
{
    /* Remove the cdev */
    tvin_dec_notifier_unregister(&it660xin_notifier);
    vout_unregister_client(&notifier_nb_v);
}

static int amvdec_it660xin_probe(struct platform_device *pdev)
{
    int r = 0;

    pr_dbg("amvdec_it660xin probe start.\n");

    tvin_dec_notifier_register(&it660xin_notifier);

    vout_register_client(&notifier_nb_v);
    pr_dbg("amvdec_it660xin probe end.\n");
    return r;
}

static int amvdec_it660xin_remove(struct platform_device *pdev)
{
    /* Remove the cdev */
    tvin_dec_notifier_unregister(&it660xin_notifier);
    vout_unregister_client(&notifier_nb_v);
    return 0;
}



static struct platform_driver amvdec_it660xin_driver = {
    .probe      = amvdec_it660xin_probe,
    .remove     = amvdec_it660xin_remove,
    .driver     = {
        .name   = DEVICE_NAME,
    }
};

static struct platform_device* it660xin_device = NULL;

static int __init amvdec_it660xin_init_module(void)
{
    pr_dbg("amvdec_it660xin module init\n");
#if 1
	  it660xin_device = platform_device_alloc(DEVICE_NAME,0);
    if (!it660xin_device) {
        pr_error("failed to alloc it660xin_device\n");
        return -ENOMEM;
    }

    if(platform_device_add(it660xin_device)){
        platform_device_put(it660xin_device);
        pr_error("failed to add it660xin_device\n");
        return -ENODEV;
    }
    if (platform_driver_register(&amvdec_it660xin_driver)) {
        pr_error("failed to register amvdec_it660xin driver\n");

        platform_device_del(it660xin_device);
        platform_device_put(it660xin_device);
        return -ENODEV;
    }

#else
    if (platform_driver_register(&amvdec_it660xin_driver)) {
        pr_error("failed to register amvdec_it660xin driver\n");
        return -ENODEV;
    }
#endif
    return 0;
}

static void __exit amvdec_it660xin_exit_module(void)
{
    pr_dbg("amvdec_it660xin module remove.\n");
    platform_driver_unregister(&amvdec_it660xin_driver);
    return ;
}

module_init(amvdec_it660xin_init_module);
module_exit(amvdec_it660xin_exit_module);


void config_dvin (unsigned long hs_pol_inv_,             // Invert HS polarity, for HW regards HS active high.
                  unsigned long vs_pol_inv_,             // Invert VS polarity, for HW regards VS active high.
                  unsigned long de_pol_inv_,             // Invert DE polarity, for HW regards DE active high.
                  unsigned long field_pol_inv_,          // Invert FIELD polarity, for HW regards odd field when high.
                  unsigned long ext_field_sel_,          // FIELD source select:
                                                        // 1=Use external FIELD signal, ignore internal FIELD detection result;
                                                        // 0=Use internal FIELD detection result, ignore external input FIELD signal.
                  unsigned long de_mode_,                // DE mode control:
                                                        // 0=Ignore input DE signal, use internal detection to to determine active pixel;
                                                        // 1=Rsrv;
                                                        // 2=During internal detected active region, if input DE goes low, replace input data with the last good data;
                                                        // 3=Active region is determined by input DE, no internal detection.
                  unsigned long data_comp_map_,          // Map input data to form YCbCr.
                                                        // Use 0 if input is YCbCr;
                                                        // Use 1 if input is YCrCb;
                                                        // Use 2 if input is CbCrY;
                                                        // Use 3 if input is CbYCr;
                                                        // Use 4 if input is CrYCb;
                                                        // Use 5 if input is CrCbY;
                                                        // 6,7=Rsrv.
                  unsigned long mode_422to444_,          // 422 to 444 conversion control:
                                                        // 0=No convertion; 1=Rsrv;
                                                        // 2=Convert 422 to 444, use previous C value;
                                                        // 3=Convert 422 to 444, use average C value.
                  unsigned long dvin_clk_inv_,           // Invert dvin_clk_in for ease of data capture.
                  unsigned long vs_hs_tim_ctrl_,         // Controls which edge of HS/VS (post polarity control) the active pixel/line is related:
                                                        // Bit 0: HS and active pixel relation.
                                                        //  0=Start of active pixel is counted from the rising edge of HS;
                                                        //  1=Start of active pixel is counted from the falling edge of HS;
                                                        // Bit 1: VS and active line relation.
                                                        //  0=Start of active line is counted from the rising edge of VS;
                                                        //  1=Start of active line is counted from the falling edge of VS.
                  unsigned long hs_lead_vs_odd_min_,     // For internal FIELD detection:
                                                        // Minimum clock cycles allowed for HS active edge to lead before VS active edge in odd field. Failing it the field is even.
                  unsigned long hs_lead_vs_odd_max_,     // For internal FIELD detection:
                                                        // Maximum clock cycles allowed for HS active edge to lead before VS active edge in odd field. Failing it the field is even.
                  unsigned long active_start_pix_fe_,    // Number of clock cycles between HS active edge to first active pixel, in even field.
                  unsigned long active_start_pix_fo_,    // Number of clock cycles between HS active edge to first active pixel, in odd field.
                  unsigned long active_start_line_fe_,   // Number of clock cycles between VS active edge to first active line, in even field.
                  unsigned long active_start_line_fo_,   // Number of clock cycles between VS active edge to first active line, in odd field.
                  unsigned long line_width_,             // Number_of_pixels_per_line
                  unsigned long field_height_)           // Number_of_lines_per_field
{
    unsigned long data32;

#ifdef DEBUG_DVIN  
        hs_pol_inv = hs_pol_inv_;
        vs_pol_inv = vs_pol_inv_;          
        de_pol_inv = de_pol_inv_;          
        field_pol_inv = field_pol_inv_;       
        ext_field_sel = ext_field_sel_;       
        de_mode = de_mode_;             
        data_comp_map = data_comp_map_;       
        mode_422to444 = mode_422to444_;       
        dvin_clk_inv = dvin_clk_inv_;        
        vs_hs_tim_ctrl = vs_hs_tim_ctrl_;      
        hs_lead_vs_odd_min = hs_lead_vs_odd_min_;  
        hs_lead_vs_odd_max = hs_lead_vs_odd_max_;  
        active_start_pix_fe = active_start_pix_fe_; 
        active_start_pix_fo = active_start_pix_fo_; 
        active_start_line_fe = active_start_line_fe_;
        active_start_line_fo = active_start_line_fo_;
        line_width = line_width_;          
        field_height = field_height_;
#endif


    printk("%s: %d %d %d %d\n",  __func__, active_start_pix_fe_, active_start_line_fe_,  line_width_, field_height_);  
    // Program reg DVIN_CTRL_STAT: disable DVIN
    WRITE_MPEG_REG(DVIN_CTRL_STAT, 0);

    // Program reg DVIN_FRONT_END_CTRL
    data32 = 0;
    data32 |= (hs_pol_inv_       & 0x1)  << 0;
    data32 |= (vs_pol_inv_       & 0x1)  << 1;
    data32 |= (de_pol_inv_       & 0x1)  << 2;
    data32 |= (field_pol_inv_    & 0x1)  << 3;
    data32 |= (ext_field_sel_    & 0x1)  << 4;
    data32 |= (de_mode_          & 0x3)  << 5;
    data32 |= (mode_422to444_    & 0x3)  << 7;
    data32 |= (dvin_clk_inv_     & 0x1)  << 9;
    data32 |= (vs_hs_tim_ctrl_   & 0x3)  << 10;
    WRITE_MPEG_REG(DVIN_FRONT_END_CTRL, data32);
    
    // Program reg DVIN_HS_LEAD_VS_ODD
    data32 = 0;
    data32 |= (hs_lead_vs_odd_min_ & 0xfff) << 0;
    data32 |= (hs_lead_vs_odd_max_ & 0xfff) << 16;
    WRITE_MPEG_REG(DVIN_HS_LEAD_VS_ODD, data32);

    // Program reg DVIN_ACTIVE_START_PIX
    data32 = 0;
    data32 |= (active_start_pix_fe_ & 0xfff) << 0;
    data32 |= (active_start_pix_fo_ & 0xfff) << 16;
    WRITE_MPEG_REG(DVIN_ACTIVE_START_PIX, data32);
    
    // Program reg DVIN_ACTIVE_START_LINE
    data32 = 0;
    data32 |= (active_start_line_fe_ & 0xfff) << 0;
    data32 |= (active_start_line_fo_ & 0xfff) << 16;
    WRITE_MPEG_REG(DVIN_ACTIVE_START_LINE, data32);
    
    // Program reg DVIN_DISPLAY_SIZE
    data32 = 0;
    data32 |= ((line_width_-1)   & 0xfff) << 0;
    data32 |= ((field_height_-1) & 0xfff) << 16;
    WRITE_MPEG_REG(DVIN_DISPLAY_SIZE, data32);
    
    // Program reg DVIN_CTRL_STAT, and enable DVIN
    data32 = 0;
    data32 |= 1                     << 0;
    data32 |= (data_comp_map_ & 0x7) << 1;
    WRITE_MPEG_REG(DVIN_CTRL_STAT, data32);
} /* config_dvin */



