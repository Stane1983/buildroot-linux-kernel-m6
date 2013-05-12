/*
 *gc0307 - This code emulates a real video device with v4l2 api
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the BSD Licence, GNU General Public License
 * as published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version
 */
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/pci.h>
#include <linux/random.h>
#include <linux/version.h>
#include <linux/mutex.h>
#include <linux/videodev2.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/highmem.h>
#include <linux/freezer.h>
#include <media/videobuf-vmalloc.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <linux/wakelock.h>

#include <linux/i2c.h>
#include <media/v4l2-chip-ident.h>
#include <media/v4l2-i2c-drv.h>
#include <media/amlogic/aml_camera.h>

#include <mach/am_regs.h>
#include <mach/pinmux.h>
#include <media/amlogic/656in.h>
#include "common/plat_ctrl.h"
#include "common/vmapi.h"
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6
#include <mach/mod_gate.h>
#endif

#define GC0307_CAMERA_MODULE_NAME "gc0307"

/* Wake up at about 30 fps */
#define WAKE_NUMERATOR 30
#define WAKE_DENOMINATOR 1001
#define BUFFER_TIMEOUT     msecs_to_jiffies(500)  /* 0.5 seconds */

#define GC0307_CAMERA_MAJOR_VERSION 0
#define GC0307_CAMERA_MINOR_VERSION 7
#define GC0307_CAMERA_RELEASE 0
#define GC0307_CAMERA_VERSION \
	KERNEL_VERSION(GC0307_CAMERA_MAJOR_VERSION, GC0307_CAMERA_MINOR_VERSION, GC0307_CAMERA_RELEASE)

MODULE_DESCRIPTION("gc0307 On Board");
MODULE_AUTHOR("amlogic-sh");
MODULE_LICENSE("GPL v2");

static unsigned video_nr = -1;  /* videoX start number, -1 is autodetect. */

static unsigned debug;
//module_param(debug, uint, 0644);
//MODULE_PARM_DESC(debug, "activates debug info");

static unsigned int vid_limit = 16;
//module_param(vid_limit, uint, 0644);
//MODULE_PARM_DESC(vid_limit, "capture memory limit in megabytes");

static int gc0307_have_open=0;

static int gc0307_h_active=320;
static int gc0307_v_active=240;
static int gc0307_frame_rate = 150;

/* supported controls */
static struct v4l2_queryctrl gc0307_qctrl[] = {
	{
		.id            = V4L2_CID_DO_WHITE_BALANCE,
		.type          = V4L2_CTRL_TYPE_INTEGER,
		.name          = "white balance",
		.minimum       = 0,
		.maximum       = 6,
		.step          = 0x1,
		.default_value = 0,
		.flags         = V4L2_CTRL_FLAG_SLIDER,
	},{
		.id            = V4L2_CID_EXPOSURE,
		.type          = V4L2_CTRL_TYPE_INTEGER,
		.name          = "exposure",
		.minimum       = 0,
		.maximum       = 8,
		.step          = 0x1,
		.default_value = 4,
		.flags         = V4L2_CTRL_FLAG_SLIDER,
	},{
		.id            = V4L2_CID_COLORFX,
		.type          = V4L2_CTRL_TYPE_INTEGER,
		.name          = "effect",
		.minimum       = 0,
		.maximum       = 6,
		.step          = 0x1,
		.default_value = 0,
		.flags         = V4L2_CTRL_FLAG_SLIDER,
	},{
		.id            = V4L2_CID_WHITENESS,
		.type          = V4L2_CTRL_TYPE_INTEGER,
		.name          = "banding",
		.minimum       = 0,
		.maximum       = 1,
		.step          = 0x1,
		.default_value = 0,
		.flags         = V4L2_CTRL_FLAG_SLIDER,
	},{
		.id            = V4L2_CID_BLUE_BALANCE,
		.type          = V4L2_CTRL_TYPE_INTEGER,
		.name          = "scene mode",
		.minimum       = 0,
		.maximum       = 1,
		.step          = 0x1,
		.default_value = 0,
		.flags         = V4L2_CTRL_FLAG_SLIDER,
	},{
		.id            = V4L2_CID_HFLIP,
		.type          = V4L2_CTRL_TYPE_INTEGER,
		.name          = "flip on horizontal",
		.minimum       = 0,
		.maximum       = 1,
		.step          = 0x1,
		.default_value = 0,
		.flags         = V4L2_CTRL_FLAG_SLIDER,
	},{
		.id            = V4L2_CID_VFLIP,
		.type          = V4L2_CTRL_TYPE_INTEGER,
		.name          = "flip on vertical",
		.minimum       = 0,
		.maximum       = 1,
		.step          = 0x1,
		.default_value = 0,
		.flags         = V4L2_CTRL_FLAG_SLIDER,
	},{
		.id            = V4L2_CID_ZOOM_ABSOLUTE,
		.type          = V4L2_CTRL_TYPE_INTEGER,
		.name          = "Zoom, Absolute",
		.minimum       = 100,
		.maximum       = 300,
		.step          = 20,
		.default_value = 100,
		.flags         = V4L2_CTRL_FLAG_SLIDER,
	},{
		.id		= V4L2_CID_ROTATE,
		.type		= V4L2_CTRL_TYPE_INTEGER,
		.name		= "Rotate",
		.minimum	= 0,
		.maximum	= 270,
		.step		= 90,
		.default_value	= 0,
		.flags         = V4L2_CTRL_FLAG_SLIDER,
 	}
};

#define dprintk(dev, level, fmt, arg...) \
	v4l2_dbg(level, debug, &dev->v4l2_dev, fmt, ## arg)

/* ------------------------------------------------------------------
	Basic structures
   ------------------------------------------------------------------*/

struct gc0307_fmt {
	char  *name;
	u32   fourcc;          /* v4l2 format id */
	int   depth;
};

static struct gc0307_fmt formats[] = {
	{
		.name     = "RGB565 (BE)",
		.fourcc   = V4L2_PIX_FMT_RGB565X, /* rrrrrggg gggbbbbb */
		.depth    = 16,
	},

	{
		.name     = "RGB888 (24)",
		.fourcc   = V4L2_PIX_FMT_RGB24, /* 24  RGB-8-8-8 */
		.depth    = 24,
	},
	{
		.name     = "BGR888 (24)",
		.fourcc   = V4L2_PIX_FMT_BGR24, /* 24  BGR-8-8-8 */
		.depth    = 24,
	},
	{
		.name     = "12  Y/CbCr 4:2:0",
		.fourcc   = V4L2_PIX_FMT_NV12,
		.depth    = 12,
	},
	{
		.name     = "12  Y/CbCr 4:2:0",
		.fourcc   = V4L2_PIX_FMT_NV21,
		.depth    = 12,
	},
	{
		.name     = "YUV420P",
		.fourcc   = V4L2_PIX_FMT_YUV420,
		.depth    = 12,
	},
	{
		.name     = "YVU420P",
		.fourcc   = V4L2_PIX_FMT_YVU420,
		.depth    = 12,
	}
};

static struct gc0307_fmt *get_format(struct v4l2_format *f)
{
	struct gc0307_fmt *fmt;
	unsigned int k;

	for (k = 0; k < ARRAY_SIZE(formats); k++) {
		fmt = &formats[k];
		if (fmt->fourcc == f->fmt.pix.pixelformat)
			break;
	}

	if (k == ARRAY_SIZE(formats))
		return NULL;

	return &formats[k];
}

struct sg_to_addr {
	int pos;
	struct scatterlist *sg;
};

/* buffer for one video frame */
struct gc0307_buffer {
	/* common v4l buffer stuff -- must be first */
	struct videobuf_buffer vb;

	struct gc0307_fmt        *fmt;
};

struct gc0307_dmaqueue {
	struct list_head       active;

	/* thread for generating video stream*/
	struct task_struct         *kthread;
	wait_queue_head_t          wq;
	/* Counters to control fps rate */
	int                        frame;
	int                        ini_jiffies;
};

static LIST_HEAD(gc0307_devicelist);

struct gc0307_device {
	struct list_head			gc0307_devicelist;
	struct v4l2_subdev			sd;
	struct v4l2_device			v4l2_dev;

	spinlock_t                 slock;
	struct mutex				mutex;

	int                        users;

	/* various device info */
	struct video_device        *vdev;

	struct gc0307_dmaqueue       vidq;

	/* Several counters */
	unsigned long              jiffies;

	/* Input Number */
	int			   input;

	/* platform device data from board initting. */
	aml_plat_cam_data_t platform_dev_data;

	/* wake lock */
	struct wake_lock	wake_lock;

	/* Control 'registers' */
	int 			   qctl_regs[ARRAY_SIZE(gc0307_qctrl)];
};

static inline struct gc0307_device *to_dev(struct v4l2_subdev *sd)
{
	return container_of(sd, struct gc0307_device, sd);
}

struct gc0307_fh {
	struct gc0307_device            *dev;

	/* video capture */
	struct gc0307_fmt            *fmt;
	unsigned int               width, height;
	struct videobuf_queue      vb_vidq;

	enum v4l2_buf_type         type;
	int			   input; 	/* Input Number on bars */
	int  stream_on;
};

static inline struct gc0307_fh *to_fh(struct gc0307_device *dev)
{
	return container_of(dev, struct gc0307_fh, dev);
}

static struct v4l2_frmsize_discrete gc0307_prev_resolution[2]= //should include 320x240 and 640x480, those two size are used for recording
{
	{320,240},
	{640,480},
};

static struct v4l2_frmsize_discrete gc0307_pic_resolution[1]=
{
	{640,480},
};

/* ------------------------------------------------------------------
	reg spec of GC0307
   ------------------------------------------------------------------*/

struct aml_camera_i2c_fig1_s GC0307_script[] = {
     // Initail Sequence Write In.
//========= close output            return err;
	{0x43  ,0x00}, 
	{0x44  ,0xa2}, 
	
	//========= close some functions
	// open them after configure their parmameters
	{0x40  ,0x10}, 
	{0x41  ,0x00}, 			
	{0x42  ,0x10},					  	
	{0x47  ,0x00}, //mode1,				  	
	{0x48  ,0xc7},//mode2,  0xc1
	{0x49  ,0x00}, //dither_mode 		
	{0x4a  ,0x00}, //clock_gating_en
	{0x4b  ,0x00}, //mode_reg3
	{0x4E  ,0x23},//sync mode   23 20120410
	{0x4F  ,0x01}, //AWB, AEC, every N frame	
	
	//========= frame timing
	{0x01  ,0x6a}, //HB // 1 
	{0x02  ,0x70}, //VB
	{0x1C  ,0x00}, //Vs_st
	{0x1D  ,0x00}, //Vs_et
	{0x10  ,0x00}, //high 4 bits of VB, HB
	{0x11  ,0x05}, //row_tail,  AD_pipe_number


                
	
	
	//========= windowing
	{0x05  ,0x00}, //row_start
	{0x06  ,0x00},
	{0x07  ,0x00}, //col start
	{0x08  ,0x00}, 
	{0x09  ,0x01}, //win height
       {0x0A,0xea}, //  ea   0xe8   james
	{0x0B  ,0x02}, //win width, pixel array only 640
	{0x0C  ,0x80},
	
	//========= analog
	{0x0D  ,0x22}, //rsh_width
					  
	{0x0E  ,0x02}, //CISCTL mode2,  

			  
	{0x12  ,0x70}, //7 hrst, 6_4 darsg,
	{0x13  ,0x00}, //7 CISCTL_restart, 0 apwd
	{0x14  ,0x00}, //NA
	{0x15  ,0xba}, //7_4 vref
	{0x16  ,0x13}, //5to4 _coln_r,  __1to0__da18
	{0x17  ,0x52}, //opa_r, ref_r, sRef_r
	//{0x18  ,0xc0}, //analog_mode, best case for left band.
{0x18,0x00},
	
	{0x1E  ,0x0d}, //tsp_width 		   
	{0x1F  ,0x32}, //sh_delay
	
	//========= offset
	{0x47  ,0x00},  //7__test_image, __6__fixed_pga, __5__auto_DN, __4__CbCr_fix, 
				//__3to2__dark_sequence, __1__allow_pclk_vcync, __0__LSC_test_image
	{0x19  ,0x06},  //pga_o			 
	{0x1a  ,0x06},  //pga_e			 
	
	{0x31  ,0x00},  //4	//pga_oFFset ,	 high 8bits of 11bits
	{0x3B  ,0x00},  //global_oFFset, low 8bits of 11bits
	
	{0x59  ,0x0f},  //offset_mode 	
	{0x58  ,0x88},  //DARK_VALUE_RATIO_G,  DARK_VALUE_RATIO_RB
	{0x57  ,0x08},  //DARK_CURRENT_RATE
	{0x56  ,0x77},  //PGA_OFFSET_EVEN_RATIO, PGA_OFFSET_ODD_RATIO
	
	//========= blk
	{0x35  ,0xd8},  //blk_mode

	{0x36  ,0x40},  
	
	{0x3C  ,0x00}, 
	{0x3D  ,0x00}, 
	{0x3E  ,0x00}, 
	{0x3F  ,0x00}, 
	
	{0xb5  ,0x70}, 
	{0xb6  ,0x40}, 
	{0xb7  ,0x00}, 
	{	0xb8  ,0x38}, 
	{	0xb9  ,0xc3}, 		  
	{	0xba  ,0x0f}, 
	
	{	0x7e  ,0x45}, 
	{	0x7f  ,0x66}, 
	
	{	0x5c  ,0x48}, //78
	{	0x5d  ,0x58}, //88
	
	
	//========= manual_gain 
	{0x61  ,0x80}, //manual_gain_g1	
	{0x63  ,0x90}, //manual_gain_r   80
	{0x65  ,0x98}, //manual_gai_b, 0xa0=1.25, 0x98=1.1875
	{0x67  ,0x80}, //manual_gain_g2
	{0x68  ,0x18}, //global_manual_gain	 2.4bits
	
	//=========CC _R
	{0x69  ,0x58},  //54
	{0x6A  ,0xf6},  //ff
	{0x6B  ,0xfb},  //fe
	{0x6C  ,0xf4},  //ff
	{0x6D  ,0x5a},  //5f
	{0x6E  ,0xe6},  //e1

	{0x6f  ,0x00}, 	
	
	//=========lsc							  
	{0x70  ,0x14}, 
	{0x71  ,0x1c}, 
	{0x72  ,0x20}, 
	
	{0x73  ,0x10}, 	
	{0x74  ,0x3c}, 
	{0x75  ,0x52}, 
	
	//=========dn																			 
	{0x7d  ,0x2f},  //dn_mode   	
	{0x80  ,0x0c}, //when auto_dn, check 7e,7f
	{0x81  ,0x0c},
	{0x82  ,0x44},
																						
	//dd																		   
	{0x83  ,0x18},  //DD_TH1 					  
	{0x84  ,0x18},  //DD_TH2 					  
	{0x85  ,0x04},  //DD_TH3 																							  
	{0x87  ,0x34},  //32 b DNDD_low_range X16,  DNDD_low_range_C_weight_center					
	
	   
	//=========intp-ee																		   
	{0x88  ,0x04},  													   
	{0x89  ,0x01},  										  
	{0x8a  ,0x50},//60  										   
	{0x8b  ,0x50},//60  										   
	{0x8c  ,0x07},  												 				  
																					  
	{0x50  ,0x0c},   						   		
	{0x5f  ,0x3c}, 																					 
																					 
	{0x8e  ,0x02},  															  
	{0x86  ,0x02},  																  
																					
	{0x51  ,0x20},  																
	{0x52  ,0x08},  
	{0x53  ,0x00}, 
	
	
	//========= YCP 
	//contrast_center																			  
	{0x77  ,0x80}, //contrast_center //0x80 20120416	
	{0x78  ,0x00}, //fixed_Cb																		  
	{0x79  ,0x00}, //fixed_Cr																		  
	{	0x7a  ,0x00}, //luma_offset 																																							
	{	0x7b  ,0x40}, //hue_cos 																		  
	{	0x7c  ,0x00}, //hue_sin 																		  
																							 
	//saturation																				  
	{	0xa0  ,0x40}, //global_saturation
	{	0xa1  ,0x40}, //luma_contrast																	  
	{	0xa2  ,0x34}, //saturation_Cb																	  
	{	0xa3  ,0x36}, //saturation_Cr
																				
	{0xa4  ,0xc8}, 																  
	{0xa5  ,0x02}, 
	{0xa6  ,0x28}, 																			  
	{0xa7  ,0x02}, 
	
	//skin																								  
	{0xa8  ,0xee}, 															  
	{0xa9  ,0x12}, 															  
	{0xaa  ,0x01}, 														  
	{0xab  ,0x20}, 													  
	{0xac  ,0xf0}, 														  
	{0xad  ,0x10}, 															  
		
	//========= ABS
	{	0xae  ,0x18}, 
	{	0xaf  ,0x74}, 
	{	0xb0  ,0xe0}, 	  
	{	0xb1  ,0x20}, 
	{	0xb2  ,0x6c}, 
	{	0xb3  ,0x40}, 
	{	0xb4  ,0x04}, 
		
	//========= AWB 
	{	0xbb  ,0x42}, 
	{	0xbc  ,0x60},
	{	0xbd  ,0x50},
	{	0xbe  ,0x50},
	
	{	0xbf  ,0x0c}, 
	{	0xc0  ,0x06}, 
	{	0xc1  ,0x60}, //////////////////70
	{	0xc2  ,0xf1},  //f1
	{	0xc3  ,0x40},
	{	0xc4  ,0x1c}, //18//20
	{	0xc5  ,0x56},  //33
	{	0xc6  ,0x1d}, 

	{	0xca  ,0x56}, //70
	{	0xcb  ,0x52}, //70
	{	0xcc  ,0x66}, //78
	
	{	0xcd  ,0x80}, //R_ratio 									 
	{	0xce  ,0x7e}, //G_ratio  , cold_white white 								   
	{	0xcf  ,0x80}, //B_ratio  	
	
	//=========  aecT  
	{	0x20  ,0x06},//0x02 
	{	0x21  ,0xc0}, 
	{	0x22  ,0x60},    
	{	0x23  ,0x88}, 
	{	0x24  ,0x96}, 
	{	0x25  ,0x30}, 
	{0x26  ,0xd0}, 
	{0x27  ,0x00}, 
	
	{0x28  ,0x02}, //AEC_exp_level_1bit11to8   
	{0x29  ,0x58}, //AEC_exp_level_1bit7to0	  
	{0x2a  ,0x03}, //AEC_exp_level_2bit11to8   
	{0x2b  ,0x84}, //AEC_exp_level_2bit7to0			 
	{0x2c  ,0x09}, //AEC_exp_level_3bit11to8   659 - 8FPS,  8ca - 6FPS  //	 
	{0x2d  ,0x60}, //AEC_exp_level_3bit7to0			 
	{0x2e  ,0x0a}, //AEC_exp_level_4bit11to8   4FPS 
	{0x2f  ,0x8c}, //AEC_exp_level_4bit7to0	 
	
	{0x30  ,0x20}, 						  
	{0x31  ,0x00}, 					   
	{0x32  ,0x1c}, 
	{0x33  ,0x90}, 			  
	{0x34  ,0x10},	
	
	{0xd0  ,0x34}, 
			   
	{0xd1  ,0x5c}, //AEC_target_Y	// 0x40_ 20120416					   
	{0xd2  ,0x61},//0xf2 	  
	{0xd4  ,0x96}, 
	{0xd5  ,0x01}, // william 0318
	{0xd6  ,0x96}, //antiflicker_step 					   
	{0xd7  ,0x03}, //AEC_exp_time_min ,william 20090312			   
	{0xd8  ,0x02}, 
			   
	{0xdd  ,0x12},//0x12 
	  															
	//========= measure window	            return err;									
	{0xe0  ,0x03}, 						 
	{0xe1  ,0x02}, 							 
	{0xe2  ,0x27}, 								 
	{0xe3  ,0x1e}, 				 
	{0xe8  ,0x3b}, 					 
	{0xe9  ,0x6e}, 						 
	{0xea  ,0x2c}, 					 
	{0xeb  ,0x50}, 					 
	{0xec  ,0x73}, 		 
	
	//========= close_frame													
	{0xed  ,0x00}, //close_frame_num1 ,can be use to reduce FPS				 
	{0xee  ,0x00}, //close_frame_num2  
	{0xef  ,0x00}, //close_frame_num
	
	// page1
	{0xf0  ,0x01}, //select page1 
	
	{0x00  ,0x20}, 							  
	{0x01  ,0x20}, 							  
	{0x02  ,0x20}, 									
	{0x03  ,0x20}, 							
	{0x04  ,0x78}, 
	{0x05  ,0x78}, 					 
	{0x06  ,0x78}, 								  
	{0x07  ,0x78}, 									 
	
	
	
	{0x10  ,0x04}, 						  
	{0x11  ,0x04},							  
	{0x12  ,0x04}, 						  
	{0x13  ,0x04}, 							  
	{0x14  ,0x01}, 							  
	{0x15  ,0x01}, 							  
	{0x16  ,0x01}, 						 
	{0x17  ,0x01}, 						 
		  
													 
	{0x20  ,0x00}, 					  
	{0x21  ,0x00}, 					  
	{0x22  ,0x00}, 						  
	{0x23  ,0x00}, 						  
	{0x24  ,0x00}, 					  
	{0x25  ,0x00}, 						  
	{0x26  ,0x00}, 					  
	{0x27  ,0x00},  						  
	
	{0x40  ,0x11}, 
	
	//=============================lscP 
	{0x45  ,0x05},  //0x06	 
	{0x46  ,0x05},//0x06 			 
	{0x47  ,0x05}, 
	
	{0x48  ,0x04}, 	
	{0x49  ,0x03}, 		 
	{0x4a  ,0x03}, 
	

	{0x62  ,0xd8}, 
	{0x63  ,0x24}, 
	{0x64  ,0x24},
	{0x65  ,0x24}, 
	{0x66  ,0xd8}, 
	{0x67  ,0x24},
	
	{0x5a  ,0x00}, 
	{0x5b  ,0x00}, 
	{0x5c  ,0x00}, 
	{0x5d  ,0x00}, 
	{0x5e  ,0x00}, 
	{0x5f  ,0x00}, 
	
	
	//============================= ccP 
	
	{0x69  ,0x03}, //cc_mode
		  
	//CC_G
	{0x70  ,0x5d}, 
	{0x71  ,0xed}, 
	{0x72  ,0xff}, 
	{0x73  ,0xe5}, 
	{0x74  ,0x5f}, 
	{0x75  ,0xe6}, 
	
      //CC_B
	{0x76  ,0x41}, 
	{0x77  ,0xef}, 
	{0x78  ,0xff}, 
	{0x79  ,0xff}, 
	{0x7a  ,0x5f}, 
	{0x7b  ,0xfa}, 	 
	
	
	//============================= AGP
	
	{0x7e  ,0x00},  
	{0x7f  ,0x20},  //x040
	{0x80  ,0x48},  
	{0x81  ,0x06},  
	{0x82  ,0x08},  
	
	{0x83  ,0x23},  
	{0x84  ,0x38},  
	{0x85  ,0x4F},  
	{0x86  ,0x61},  
	{0x87  ,0x72},  
	{0x88  ,0x80},  
	{0x89  ,0x8D},  
	{0x8a  ,0xA2},  
	{0x8b  ,0xB2},  
	{0x8c  ,0xC0},  
	{0x8d  ,0xCA},  
	{0x8e  ,0xD3},  
	{0x8f  ,0xDB},  
	{0x90  ,0xE2},  
	{0x91  ,0xED},  
	{0x92  ,0xF6},  
	{0x93  ,0xFD},  
	
	//about gamma1 is hex r oct
	{0x94  ,0x04},  
	{0x95  ,0x0E},  
	{0x96  ,0x1B},  
	{0x97  ,0x28},  
	{0x98  ,0x35},  
	{0x99  ,0x41},  
	{0x9a  ,0x4E},  
	{0x9b  ,0x67},  
	{0x9c  ,0x7E},  
	{0x9d  ,0x94},  
	{0x9e  ,0xA7},  
	{0x9f  ,0xBA},  
	{0xa0  ,0xC8},  
	{0xa1  ,0xD4},  
	{0xa2  ,0xE7},  
	{0xa3  ,0xF4},  
	{0xa4  ,0xFA}, 
	
	//========= open functions	
	{0xf0  ,0x00}, //set back to page0	
	{0x40  ,0x7e}, 
	{0x41  ,0x2F},

/////  ��ע�⣬����GC0307�ľ���ͷ�ת����Ҫͬʱ�޸������Ĵ���������:

	{0x0f, 0xa2},
	{0x45, 0x26},
	{0x47, 0x28},	
///banding setting   
        /*
	{  0x01  ,0xfa}, // 24M  
        {  0x02  ,0x70}, 
        {  0x10  ,0x01},   
        {  0xd6  ,0x64}, 
        {  0x28  ,0x02}, 
        {  0x29  ,0x58}, 
        {  0x2a  ,0x02}, 
        {  0x2b  ,0x58}, 
        {  0x2c  ,0x02}, 
        {  0x2d  ,0x58}, 
        {  0x2e  ,0x06}, 
        {  0x2f  ,0x40}, 
	*/
	{  0x01  ,0xfa}, // 24M  // 20120416
        {  0x02  ,0x70}, 
        {  0x10  ,0x01},   
        {  0xd6  ,0x32}, 
        {  0x28  ,0x02}, 
        {  0x29  ,0x58}, 
        {  0x2a  ,0x02}, 
        {  0x2b  ,0x58}, 
        {  0x2c  ,0x02}, 
        {  0x2d  ,0x58}, 
        {  0x2e  ,0x04}, 
        {  0x2f  ,0xb0}, 
	
	/************
       {0x0f, 0x02},//82
	{0x45, 0x24},
	{0x47, 0x20},	
	**************/
/////  ���ֲ�ͬ�ķ�ת�;����趨���ͻ���ֱ�Ӹ���!!!!!!


#if 0
//  IMAGE_NORMAL:
	{0x0f, 0xb2},
	{0x45, 0x27},
	{0x47, 0x2c},			

// IMAGE_H_MIRROR:
	{0x0f, 0xa2},
	{0x45, 0x26},
	{0x47, 0x28},	
	
// IMAGE_V_MIRROR:			
	{0x0f, 0x92},
	{0x45, 0x25},
	{0x47, 0x24},			

// IMAGE_HV_MIRROR:	   // 180
	{0x0f, 0x82},
	{0x45, 0x24},
	{0x47, 0x20},		
#endif
{0x43, 0x40},
	{0x44, 0xe3},
{0xff,0xff},



};

//load GT2005 parameters
void GC0307_init_regs(struct gc0307_device *dev)
{
    int i=0;//,j;
    unsigned char buf[2];
    struct i2c_client *client = v4l2_get_subdevdata(&dev->sd);

    while(1)
    {
        buf[0] = GC0307_script[i].addr;//(unsigned char)((GC0307_script[i].addr >> 8) & 0xff);
        //buf[1] = (unsigned char)(GC0307_script[i].addr & 0xff);
        buf[1] = GC0307_script[i].val;
        if(GC0307_script[i].val==0xff&&GC0307_script[i].addr==0xff){
            printk("GC0307_write_regs success in initial GC0307.\n");
            break;
        }
        if((i2c_put_byte_add8(client,buf, 2)) < 0){
            printk("fail in initial GC0307. \n");
            return;
        }
        i++;
    }
    aml_plat_cam_data_t* plat_dat= (aml_plat_cam_data_t*)client->dev.platform_data;
    if (plat_dat&&plat_dat->custom_init_script) {
        i=0;
        aml_camera_i2c_fig1_t*  custom_script = (aml_camera_i2c_fig1_t*)plat_dat->custom_init_script;
        while(1)
        {
            buf[0] = custom_script[i].addr;
            buf[1] = custom_script[i].val;
            if (custom_script[i].val==0xff&&custom_script[i].addr==0xff){
                printk("GC0307_write_custom_regs success in initial GC0307.\n");
                break;
            }
            if((i2c_put_byte_add8(client,buf, 2)) < 0){
                printk("fail in initial GC0307 custom_regs. \n");
                return;
            }
            i++;
        }
    }
    return;
}

static struct aml_camera_i2c_fig1_s resolution_320x240_script[] = {
	//==========page select 0
	{0xf0, 0x00},

	{0x0e, 0x0a}, //row even skip
	{0x43, 0xc0}, //more boundary mode opclk output enable
	{0x44, 0xe2}, // mtk is e0

	{0x45, 0x29}, //  2a    29  col subsample  
	
	{0x47, 0x28}, //  24  col subsample  




	
	{0x4e, 0x32}, //opclk gate in subsample  // mtk is 33


//==============flicker step
	{0x01, 0xd1},
	{0x02, 0x82},
	{0x10, 0x00},
	{0xd6, 0xce},

	{0x28, 0x02}, //AEC_exp_level_1bit11to8   // 33.3fps
	{0x29, 0x6a}, //AEC_exp_level_1bit7to0 
	{0x2a, 0x02}, //AEC_exp_level_2bit11to8   // 20fps
	{0x2b, 0x6a}, //AEC_exp_level_2bit7to0  
	{0x2c, 0x02}, //AEC_exp_level_3bit11to8    // 12.5fps
	{0x2d, 0x6a}, //AEC_exp_level_3bit7to0           
	{0x2e, 0x02}, //AEC_exp_level_4bit11to8   // 6.25fps
	{0x2f, 0x6a}, //AEC_exp_level_4bit7to0   

//========= measure window                       
	{0xe1, 0x01}, //big_win_y0                                                     
	{0xe3, 0x0f}, //432, big_win_y1    , height                                 
	{0xea, 0x16}, //small_win_height1                        
	{0xeb, 0x28}, //small_win_height2                        
	{0xec, 0x39}, //small_win_heigh3 //only for AWB 

//abs
	{0xae, 0x0c}, //black pixel target number

//awb
	{0xc3, 0x20}, //number limit

//lsc
	{0x74, 0x1e}, //lsc_row_center , 0x3c
	{0x75, 0x52}, //lsc_col_center , 0x52
	{0xff, 0xff}
};

static struct aml_camera_i2c_fig1_s resolution_640x480_script[] = {
//==========page select 0
	{0xf0, 0x00},

	{0x0e, 0x02}, //row even skip
	{0x43, 0x40}, //more boundary mode opclk output enable
	{0x44, 0xE3}, // mtk is e0



	{0x45, 0x26},
	{0x47, 0x28},	


	{0x4e, 0x23}, //opclk gate in subsample  // mtk is 33


//==============flicker step

	{  0x01  ,0xfa}, // 24M  // 20120416
        {  0x02  ,0x70}, 
        {  0x10  ,0x01},   
        {  0xd6  ,0x32}, 
        {  0x28  ,0x02}, 
        {  0x29  ,0x58}, 
        {  0x2a  ,0x02}, 
        {  0x2b  ,0x58}, 
        {  0x2c  ,0x02}, 
        {  0x2d  ,0x58}, 
        {  0x2e  ,0x04}, 
        {  0x2f  ,0xb0}, 
        

        



//========= measure window                       
	{0xe1, 0x02}, //big_win_y0                                                     
	{0xe3, 0x1e}, //432, big_win_y1    , height                                 
	{0xea, 0x2c}, //small_win_height1                        
	{0xeb, 0x50}, //small_win_height2                        
	{0xec, 0x73}, //small_win_heigh3 //only for AWB 

//abs
	{0xae, 0x18}, //black pixel target number

//awb
	{0xc3, 0x40}, //number limit

//lsc
	{0x74, 0x5f}, //lsc_row_center , 0x3c
	{0x75, 0xe6}, //lsc_col_center , 0x52
	{0xff, 0xff}

};

static void gc0307_set_resolution(struct gc0307_device *dev,int height,int width)
{
	int i=0;
    unsigned char buf[2];
	struct i2c_client *client = v4l2_get_subdevdata(&dev->sd);
	struct aml_camera_i2c_fig1_s* resolution_script;
	if (height >= 480) {
		printk("set resolution 640X480\n");
		resolution_script = resolution_640x480_script;
		gc0307_h_active = 640;
		gc0307_v_active = 480;
		gc0307_frame_rate = 150;
		//GC0307_init_regs(dev);
		return;
	} else {
		printk("set resolution 320X240\n");
		gc0307_h_active = 320;
		gc0307_v_active = 238;
		gc0307_frame_rate = 236;
		resolution_script = resolution_320x240_script;
}

	while(1) {
        buf[0] = resolution_script[i].addr;
        buf[1] = resolution_script[i].val;
        if(resolution_script[i].val==0xff&&resolution_script[i].addr==0xff) {
            break;
        }
        if((i2c_put_byte_add8(client,buf, 2)) < 0) {
            printk("fail in setting resolution \n");
            return;
        }
        i++;
    }

}
/*************************************************************************
* FUNCTION
*	set_GC0307_param_wb
*
* DESCRIPTION
*	GC0307 wb setting.
*
* PARAMETERS
*	none
*
* RETURNS
*	None
*
* GLOBALS AFFECTED  °×½ºâÊ
*
*************************************************************************/
void set_GC0307_param_wb(struct gc0307_device *dev,enum  camera_wb_flip_e para)
{
//	kal_uint16 rgain=0x80, ggain=0x80, bgain=0x80;
	struct i2c_client *client = v4l2_get_subdevdata(&dev->sd);

	unsigned char buf[4];

	unsigned char  temp_reg;
	//temp_reg=gc0307_read_byte(0x22);
	buf[0]=0x41;
	temp_reg=i2c_get_byte_add8(client,buf);

	printk(" camera set_GC0307_param_wb=%d. \n ",para);
	
	switch (para)
	{
		case CAM_WB_AUTO:
			buf[0]=0xc7;
			buf[1]=0x4c;
			i2c_put_byte_add8(client,buf,2);
			buf[0]=0xc8;
			buf[1]=0x40;
			i2c_put_byte_add8(client,buf,2);
			buf[0]=0xc9;
			buf[1]=0x4a;
			i2c_put_byte_add8(client,buf,2);
			buf[0]=0x41;
			buf[1]=temp_reg|0x04;
			i2c_put_byte_add8(client,buf,2);
			break;

		case CAM_WB_CLOUD:
			buf[0]=0x41;
			buf[1]=temp_reg&~0x04;
			i2c_put_byte_add8(client,buf,2);
			buf[0]=0xc7;
			buf[1]=0x5a;
			i2c_put_byte_add8(client,buf,2);
			buf[0]=0xc8;
			buf[1]=0x42;
			i2c_put_byte_add8(client,buf,2);
			buf[0]=0xc9;
			buf[1]=0x40;
			i2c_put_byte_add8(client,buf,2);
			break;

		case CAM_WB_DAYLIGHT:   // tai yang guang
			buf[0]=0x41;
			buf[1]=temp_reg&~0x04;
			i2c_put_byte_add8(client,buf,2);
			buf[0]=0xc7;
			buf[1]=0x50;
			i2c_put_byte_add8(client,buf,2);
			buf[0]=0xc8;
			buf[1]=0x45;
			i2c_put_byte_add8(client,buf,2);
			buf[0]=0xc9;
			buf[1]=0x40;
			i2c_put_byte_add8(client,buf,2);
			break;

		case CAM_WB_INCANDESCENCE:   // bai re guang
			buf[0]=0x41;
			buf[1]=temp_reg&~0x04;
			i2c_put_byte_add8(client,buf,2);
			buf[0]=0xc7;
			buf[1]=0x48;
			i2c_put_byte_add8(client,buf,2);
			buf[0]=0xc8;
			buf[1]=0x40;
			i2c_put_byte_add8(client,buf,2);
			buf[0]=0xc9;
			buf[1]=0x5c;
			i2c_put_byte_add8(client,buf,2);
			break;

		case CAM_WB_FLUORESCENT:   //ri guang deng
			buf[0]=0x41;
			buf[1]=temp_reg&~0x04;
			i2c_put_byte_add8(client,buf,2);
			buf[0]=0xc7;
			buf[1]=0x40;
			i2c_put_byte_add8(client,buf,2);
			buf[0]=0xc8;
			buf[1]=0x42;
			i2c_put_byte_add8(client,buf,2);
			buf[0]=0xc9;
			buf[1]=0x50;
			i2c_put_byte_add8(client,buf,2);
			break;
			
		case CAM_WB_TUNGSTEN:   // wu si deng
			buf[0]=0x41;
			buf[1]=temp_reg&~0x04;
			i2c_put_byte_add8(client,buf,2);
			buf[0]=0xc7;
			buf[1]=0x40;
			i2c_put_byte_add8(client,buf,2);
			buf[0]=0xc8;
			buf[1]=0x54;
			i2c_put_byte_add8(client,buf,2);
			buf[0]=0xc9;
			buf[1]=0x70;
			i2c_put_byte_add8(client,buf,2);
			break;

		case CAM_WB_MANUAL:
			// TODO
			break;
		default:
			break;
	}
//	kal_sleep_task(20);
}

/*************************************************************************
* FUNCTION
*	GC0307_night_mode
*
* DESCRIPTION
*	This function night mode of GC0307.
*
* PARAMETERS
*	none
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
void GC0307_night_mode(struct gc0307_device *dev,enum  camera_night_mode_flip_e enable)
{
    struct i2c_client *client = v4l2_get_subdevdata(&dev->sd);
	unsigned char buf[4];

	unsigned char  temp_reg;
	//temp_reg=gc0307_read_byte(0x22);
	//buf[0]=0x20;
	//temp_reg=i2c_get_byte_add8(client,buf);
	//temp_reg=0xff;

    if(enable)
    {
		buf[0]=0xdd;
		buf[1]=0x32;
		i2c_put_byte_add8(client,buf,2);
	
     }
    else
     {
		buf[0]=0xdd;
		buf[1]=0x12;
		i2c_put_byte_add8(client,buf,2);


	}

}
/*************************************************************************
* FUNCTION
*	GC0307_night_mode
*
* DESCRIPTION
*	This function night mode of GC0307.
*
* PARAMETERS
*	none
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/

void GC0307_set_param_banding(struct gc0307_device *dev,enum  camera_night_mode_flip_e banding)
{
    struct i2c_client *client = v4l2_get_subdevdata(&dev->sd);
    unsigned char buf[4];
   switch(banding){
        case CAM_BANDING_60HZ:
            buf[0]=0x01;
            buf[1]=0x32;
            i2c_put_byte_add8(client,buf,2);
            buf[0]=0x02;
            buf[1]=0x70;
            i2c_put_byte_add8(client,buf,2);
            buf[0]=0x10;
            buf[1]=0x01;
            i2c_put_byte_add8(client,buf,2);
            buf[0]=0xd6;
            buf[1]=0x32;
            i2c_put_byte_add8(client,buf,2);
            buf[0]=0x28;
            buf[1]=0x02;
            i2c_put_byte_add8(client,buf,2);
            buf[0]=0x29;
            buf[1]=0x58;
            i2c_put_byte_add8(client,buf,2);
            buf[0]=0x2a;
            buf[1]=0x02;
            i2c_put_byte_add8(client,buf,2);
            buf[0]=0x2b;
            buf[1]=0x58;
            i2c_put_byte_add8(client,buf,2);
            buf[0]=0x2c;
            buf[1]=0x02;
            i2c_put_byte_add8(client,buf,2);
            buf[0]=0x2d;
            buf[1]=0x58;
            i2c_put_byte_add8(client,buf,2);
            buf[0]=0x2e;
            buf[1]=0x04;
            i2c_put_byte_add8(client,buf,2);
            buf[0]=0x2f;
            buf[1]=0xb0;
            i2c_put_byte_add8(client,buf,2);
            break;
        case CAM_BANDING_50HZ:
            buf[0]=0x01;
            buf[1]=0xfa;
            i2c_put_byte_add8(client,buf,2);
            buf[0]=0x02;
            buf[1]=0x70;
            i2c_put_byte_add8(client,buf,2);
            buf[0]=0x10;
            buf[1]=0x01;
            i2c_put_byte_add8(client,buf,2);
            buf[0]=0xd6;
            buf[1]=0x32;
            i2c_put_byte_add8(client,buf,2);
            buf[0]=0x28;
            buf[1]=0x02;
            i2c_put_byte_add8(client,buf,2);
            buf[0]=0x29;
            buf[1]=0x58;
            i2c_put_byte_add8(client,buf,2);
            buf[0]=0x2a;
            buf[1]=0x02;
            i2c_put_byte_add8(client,buf,2);
            buf[0]=0x2b;
            buf[1]=0x58;//58
            i2c_put_byte_add8(client,buf,2);
            buf[0]=0x2c;
            buf[1]=0x02;
            i2c_put_byte_add8(client,buf,2);
            buf[0]=0x2d;
            buf[1]=0x58;
            i2c_put_byte_add8(client,buf,2);
            buf[0]=0x2e;
            buf[1]=0x04;
            i2c_put_byte_add8(client,buf,2);
            buf[0]=0x2f;
            buf[1]=0xb0;
            i2c_put_byte_add8(client,buf,2);
            break;
    }
}


/*************************************************************************
* FUNCTION
*	set_GC0307_param_exposure
*
* DESCRIPTION
*	GC0307 exposure setting.
*
* PARAMETERS
*	none
*
* RETURNS
*	None
*
* GLOBALS AFFECTED  Á¶ȵȼ¶ µ÷Îý***********************************************************************/
void set_GC0307_param_exposure(struct gc0307_device *dev,enum camera_exposure_e para)//Æ¹â½Ú{
	struct i2c_client *client = v4l2_get_subdevdata(&dev->sd);

	unsigned char buf1[2];
	unsigned char buf2[2];

	switch (para)
	{
		case EXPOSURE_N4_STEP:
			buf1[0]=0x7a;
			buf1[1]=0xc0;
			buf2[0]=0xd1;
			buf2[1]=0x30;
			break;
		case EXPOSURE_N3_STEP:
			buf1[0]=0x7a;
			buf1[1]=0xd0;
			buf2[0]=0xd1;
			buf2[1]=0x38;
			break;
		case EXPOSURE_N2_STEP:
			buf1[0]=0x7a;
			buf1[1]=0xe0;
			buf2[0]=0xd1;
			buf2[1]=0x40;
			break;
		case EXPOSURE_N1_STEP:
			buf1[0]=0x7a;
			buf1[1]=0xf0;
			buf2[0]=0xd1;
			buf2[1]=0x48;
			break;
		case EXPOSURE_0_STEP:
			buf1[0]=0x7a;
			buf1[1]=0x00; //0x00
			buf2[0]=0xd1;
			buf2[1]=0x58;
			break;
		case EXPOSURE_P1_STEP:
			buf1[0]=0x7a;
			buf1[1]=0x18; //0x10
			buf2[0]=0xd1;
			buf2[1]=0x60;
			break;
		case EXPOSURE_P2_STEP:
			buf1[0]=0x7a;
			buf1[1]=0x20;
			buf2[0]=0xd1;
			buf2[1]=0x68;
			break;
		case EXPOSURE_P3_STEP:
			buf1[0]=0x7a;
			buf1[1]=0x30;
			buf2[0]=0xd1;
			buf2[1]=0x70;
			break;
		case EXPOSURE_P4_STEP:
			buf1[0]=0x7a;
			buf1[1]=0x40;
			buf2[0]=0xd1;
			buf2[1]=0x78;
			break;
		default:
			buf1[0]=0x7a;
			buf1[1]=0x00;
			buf2[0]=0xd1;
			buf2[1]=0x58;
			break;
	}
	//msleep(300);
	i2c_put_byte_add8(client,buf1,2);
	i2c_put_byte_add8(client,buf2,2);
	mdelay(150);

}

/*************************************************************************
* FUNCTION
*	set_GC0307_param_effect
*
* DESCRIPTION
*	GC0307 effect setting.
*
* PARAMETERS
*	none
*
* RETURNS
*	None
*
* GLOBALS AFFECTED  ÌЧ²Îý***********************************************************************/
void set_GC0307_param_effect(struct gc0307_device *dev,enum camera_effect_flip_e para)//ÌЧÉÖ
{
    struct i2c_client *client = v4l2_get_subdevdata(&dev->sd);
	unsigned char buf[4];
	unsigned char  temp_reg;
	
	buf[0]=0x47;
	temp_reg=i2c_get_byte_add8(client,buf);

	if((para == CAM_EFFECT_ENC_NORMAL) || (para == CAM_EFFECT_ENC_COLORINV))        
	{
		temp_reg = temp_reg & 0xef;
	}
	else
	{
       	 temp_reg = temp_reg | 0x10;
	}
	
	switch (para)
	{

		case CAM_EFFECT_ENC_NORMAL:
		    buf[0]=0x41;
		    buf[1]=0x3f;
		    i2c_put_byte_add8(client,buf,2);
			
		    buf[0]=0x40;
		    buf[1]=0x7e;
		    i2c_put_byte_add8(client,buf,2);
			
		    buf[0]=0x42;
		    buf[1]=0x10;
		    i2c_put_byte_add8(client,buf,2);
			
			buf[0]=0x47;		    
			buf[1]=temp_reg;
		    i2c_put_byte_add8(client,buf,2);
			
		    buf[0]=0x48;
		    buf[1]=0xc7;
		    i2c_put_byte_add8(client,buf,2);
			

		    buf[0]=0x8a;
		    buf[1]=0x50;
		    i2c_put_byte_add8(client,buf,2);
			
                  buf[0]=0x8b;
		    buf[1]=0x50;
		    i2c_put_byte_add8(client,buf,2);

			buf[0]=0x8c;
		    buf[1]=0x07;
		    i2c_put_byte_add8(client,buf,2);

			buf[0]=0x77;
		    buf[1]=0x80;
		    i2c_put_byte_add8(client,buf,2);

		    buf[0]=0xa1;
		    buf[1]=0x40; 
		    i2c_put_byte_add8(client,buf,2);


		   buf[0]=0x78;
		    buf[1]=0x00;
		    i2c_put_byte_add8(client,buf,2);

		    buf[0]=0x79;
		    buf[1]=0x00;
		    i2c_put_byte_add8(client,buf,2);
		   buf[0]=0x7a;
		    buf[1]=0x00;
		    i2c_put_byte_add8(client,buf,2);

		    buf[0]=0x7b;
		    buf[1]=0x40;
		    i2c_put_byte_add8(client,buf,2);

		   buf[0]=0x7c;
		    buf[1]=0x00;
		    i2c_put_byte_add8(client,buf,2);

		buf[0]=0x50;
		    buf[1]=0x0c;
		    i2c_put_byte_add8(client,buf,2);
			
		   buf[0]=0x40;
		    buf[1]=0x7e;
		    i2c_put_byte_add8(client,buf,2);
		   buf[0]=0x4e;
		    buf[1]=0x3f;
		    i2c_put_byte_add8(client,buf,2);

		    buf[0]=0xf0;
		    buf[1]=0x01;
		    i2c_put_byte_add8(client,buf,2);

		   buf[0]=0x80;
		    buf[1]=0xc8;
		    i2c_put_byte_add8(client,buf,2);			

		   buf[0]=0x7f;
		    buf[1]=0x00;
		    i2c_put_byte_add8(client,buf,2);
			
		    buf[0]=0xf0;
		    buf[1]=0x00;
		    i2c_put_byte_add8(client,buf,2);
			break;

		case CAM_EFFECT_ENC_GRAYSCALE:

		    buf[0]=0x41;
		    buf[1]=0x3f;
		    i2c_put_byte_add8(client,buf,2);
		    buf[0]=0x40;
		    buf[1]=0x7e;
		    i2c_put_byte_add8(client,buf,2);
		    buf[0]=0x42;
		    buf[1]=0x10;
		    i2c_put_byte_add8(client,buf,2);
			
			buf[0]=0x47;
		    buf[1]=temp_reg;
		    i2c_put_byte_add8(client,buf,2);
			buf[0]=0x48;
		    buf[1]=0xc7;
		    i2c_put_byte_add8(client,buf,2);
			buf[0]=0x8a;
		    buf[1]=0x60;
		    i2c_put_byte_add8(client,buf,2);
			buf[0]=0x8b;
		    buf[1]=0x60;
		    i2c_put_byte_add8(client,buf,2);

			buf[0]=0x8c;
		    buf[1]=0x07;
		    i2c_put_byte_add8(client,buf,2);

			buf[0]=0x50;
		    buf[1]=0x0c;
		    i2c_put_byte_add8(client,buf,2);
			buf[0]=0x77;
		    buf[1]=0x80;
		    i2c_put_byte_add8(client,buf,2);

		    buf[0]=0xa1;
		    buf[1]=0x40; 
		    i2c_put_byte_add8(client,buf,2);


		   buf[0]=0x78;
		    buf[1]=0x00;
		    i2c_put_byte_add8(client,buf,2);

		    buf[0]=0x79;
		    buf[1]=0x00;
		    i2c_put_byte_add8(client,buf,2);
		   buf[0]=0x7a;
		    buf[1]=0x00;
		    i2c_put_byte_add8(client,buf,2);

		    buf[0]=0x7b;
		    buf[1]=0x40;
		    i2c_put_byte_add8(client,buf,2);

		   buf[0]=0x7c;
		    buf[1]=0x00;
		    i2c_put_byte_add8(client,buf,2);

		  
			break;
		case CAM_EFFECT_ENC_SEPIA:
                  buf[0]=0x41;
		    buf[1]=0x3f;
		    i2c_put_byte_add8(client,buf,2);
		    buf[0]=0x40;
		    buf[1]=0x7e;
		    i2c_put_byte_add8(client,buf,2);
		    buf[0]=0x42;
		    buf[1]=0x10;
		    i2c_put_byte_add8(client,buf,2);
			buf[0]=0x47;
		    buf[1]=temp_reg;
		    i2c_put_byte_add8(client,buf,2);
			buf[0]=0x48;
		    buf[1]=0xc7;

		    i2c_put_byte_add8(client,buf,2);
			buf[0]=0x8a;
		    buf[1]=0x60;
		    i2c_put_byte_add8(client,buf,2);
			buf[0]=0x8b;
		    buf[1]=0x60;
		    i2c_put_byte_add8(client,buf,2);

			buf[0]=0x8c;
		    buf[1]=0x07;
		    i2c_put_byte_add8(client,buf,2);

			buf[0]=0x50;
		    buf[1]=0x0c;
		    i2c_put_byte_add8(client,buf,2);
			buf[0]=0x77;
		    buf[1]=0x80;
		    i2c_put_byte_add8(client,buf,2);

		    buf[0]=0xa1;
		    buf[1]=0x40; 
		    i2c_put_byte_add8(client,buf,2);


		   buf[0]=0x78;
		    buf[1]=0xc0;
		    i2c_put_byte_add8(client,buf,2);

		    buf[0]=0x79;
		    buf[1]=0x20;
		    i2c_put_byte_add8(client,buf,2);
		   buf[0]=0x7a;
		    buf[1]=0x00;
		    i2c_put_byte_add8(client,buf,2);

		    buf[0]=0x7b;
		    buf[1]=0x40;
		    i2c_put_byte_add8(client,buf,2);

		   buf[0]=0x7c;
		    buf[1]=0x00;
		    i2c_put_byte_add8(client,buf,2);

		  
			break;
		case CAM_EFFECT_ENC_COLORINV:

		 buf[0]=0x41;
		    buf[1]=0x6f;
		    i2c_put_byte_add8(client,buf,2);
		    buf[0]=0x40;
		    buf[1]=0x7e;
		    i2c_put_byte_add8(client,buf,2);
		    buf[0]=0x42;
		    buf[1]=0x10;
		    i2c_put_byte_add8(client,buf,2);
			buf[0]=0x47;
		    buf[1]=temp_reg;
		    i2c_put_byte_add8(client,buf,2);
			buf[0]=0x48;
		    buf[1]=0xc7;

		    i2c_put_byte_add8(client,buf,2);
			buf[0]=0x8a;
		    buf[1]=0x60;
		    i2c_put_byte_add8(client,buf,2);
			buf[0]=0x8b;
		    buf[1]=0x60;
		    i2c_put_byte_add8(client,buf,2);

			buf[0]=0x8c;
		    buf[1]=0x07;
		    i2c_put_byte_add8(client,buf,2);

			buf[0]=0x50;
		    buf[1]=0x0c;
		    i2c_put_byte_add8(client,buf,2);
			buf[0]=0x77;
		    buf[1]=0x80;
		    i2c_put_byte_add8(client,buf,2);

		    buf[0]=0xa1;
		    buf[1]=0x40; 
		    i2c_put_byte_add8(client,buf,2);


		   buf[0]=0x78;
		    buf[1]=0x00;
		    i2c_put_byte_add8(client,buf,2);

		    buf[0]=0x79;
		    buf[1]=0x00;
		    i2c_put_byte_add8(client,buf,2);
		   buf[0]=0x7a;
		    buf[1]=0x00;
		    i2c_put_byte_add8(client,buf,2);

		    buf[0]=0x7b;
		    buf[1]=0x40;
		    i2c_put_byte_add8(client,buf,2);

		   buf[0]=0x7c;
		    buf[1]=0x00;
		    i2c_put_byte_add8(client,buf,2);

			break;
		case CAM_EFFECT_ENC_SEPIAGREEN:
			 buf[0]=0x41;
		    buf[1]=0x3f;
		    i2c_put_byte_add8(client,buf,2);
		    buf[0]=0x40;
		    buf[1]=0x7e;
		    i2c_put_byte_add8(client,buf,2);
		    buf[0]=0x42;
		    buf[1]=0x10;
		    i2c_put_byte_add8(client,buf,2);
			buf[0]=0x47;
		    buf[1]=temp_reg;
		    i2c_put_byte_add8(client,buf,2);
			buf[0]=0x48;
		    buf[1]=0xc7;

		    i2c_put_byte_add8(client,buf,2);
			buf[0]=0x8a;
		    buf[1]=0x60;
		    i2c_put_byte_add8(client,buf,2);
			buf[0]=0x8b;
		    buf[1]=0x60;
		    i2c_put_byte_add8(client,buf,2);

			buf[0]=0x8c;
		    buf[1]=0x07;
		    i2c_put_byte_add8(client,buf,2);

			buf[0]=0x50;
		    buf[1]=0x0c;
		    i2c_put_byte_add8(client,buf,2);
			buf[0]=0x77;
		    buf[1]=0x80;
		    i2c_put_byte_add8(client,buf,2);

		    buf[0]=0xa1;
		    buf[1]=0x40; 
		    i2c_put_byte_add8(client,buf,2);


		   buf[0]=0x78;
		    buf[1]=0xc0;
		    i2c_put_byte_add8(client,buf,2);

		    buf[0]=0x79;
		    buf[1]=0xc0;
		    i2c_put_byte_add8(client,buf,2);
		   buf[0]=0x7a;
		    buf[1]=0x00;
		    i2c_put_byte_add8(client,buf,2);

		    buf[0]=0x7b;
		    buf[1]=0x40;
		    i2c_put_byte_add8(client,buf,2);

		   buf[0]=0x7c;
		    buf[1]=0x00;
		    i2c_put_byte_add8(client,buf,2);
			break;
		case CAM_EFFECT_ENC_SEPIABLUE:
		 buf[0]=0x41;
		    buf[1]=0x3f;
		    i2c_put_byte_add8(client,buf,2);
		    buf[0]=0x40;
		    buf[1]=0x7e;
		    i2c_put_byte_add8(client,buf,2);
		    buf[0]=0x42;
		    buf[1]=0x10;
		    i2c_put_byte_add8(client,buf,2);
			buf[0]=0x47;
		    buf[1]=temp_reg;
		    i2c_put_byte_add8(client,buf,2);
			buf[0]=0x48;
		    buf[1]=0xc7;
		    i2c_put_byte_add8(client,buf,2);
			buf[0]=0x8a;
		    buf[1]=0x60;
		    i2c_put_byte_add8(client,buf,2);
			buf[0]=0x8b;
		    buf[1]=0x60;
		    i2c_put_byte_add8(client,buf,2);

			buf[0]=0x8c;
		    buf[1]=0x07;
		    i2c_put_byte_add8(client,buf,2);

			buf[0]=0x50;
		    buf[1]=0x0c;
		    i2c_put_byte_add8(client,buf,2);
			buf[0]=0x77;
		    buf[1]=0x80;
		    i2c_put_byte_add8(client,buf,2);

		    buf[0]=0xa1;
		    buf[1]=0x40; 
		    i2c_put_byte_add8(client,buf,2);


		   buf[0]=0x78;
		    buf[1]=0x70;
		    i2c_put_byte_add8(client,buf,2);

		    buf[0]=0x79;
		    buf[1]=0x00;
		    i2c_put_byte_add8(client,buf,2);
		   buf[0]=0x7a;
		    buf[1]=0x00;
		    i2c_put_byte_add8(client,buf,2);

		    buf[0]=0x7b;
		    buf[1]=0x3f;
		    i2c_put_byte_add8(client,buf,2);

		   buf[0]=0x7c;
		    buf[1]=0xf5;
		    i2c_put_byte_add8(client,buf,2);

			break;
		default:
			break;
	}
}

unsigned char v4l_2_gc0307(int val)
{
	int ret=val/0x20;
	if(ret<4) return ret*0x20+0x80;
	else if(ret<8) return ret*0x20+0x20;
	else return 0;
}

static int gc0307_setting(struct gc0307_device *dev,int PROP_ID,int value )
{
	int ret=0;
	unsigned char cur_val;
	struct i2c_client *client = v4l2_get_subdevdata(&dev->sd);
	switch(PROP_ID)  {
#if 0
	case V4L2_CID_BRIGHTNESS:
		dprintk(dev, 1, "setting brightned:%d\n",v4l_2_gc0307(value));
		ret=i2c_put_byte(client,0x0201,v4l_2_gc0307(value));
		break;
	case V4L2_CID_CONTRAST:
		ret=i2c_put_byte(client,0x0200, value);
		break;
	case V4L2_CID_SATURATION:
		ret=i2c_put_byte(client,0x0202, value);
		break;
	case V4L2_CID_HFLIP:    /* set flip on H. */
		ret=i2c_get_byte(client,0x0101);
		if(ret>0) {
			cur_val=(char)ret;
			if(value!=0)
				cur_val=cur_val|0x1;
			else
				cur_val=cur_val&0xFE;
			ret=i2c_put_byte(client,0x0101,cur_val);
			if(ret<0) dprintk(dev, 1, "V4L2_CID_HFLIP setting error\n");
		}  else {
			dprintk(dev, 1, "vertical read error\n");
		}
		break;
	case V4L2_CID_VFLIP:    /* set flip on V. */
		ret=i2c_get_byte(client,0x0101);
		if(ret>0) {
			cur_val=(char)ret;
			if(value!=0)
				cur_val=cur_val|0x10;
			else
				cur_val=cur_val&0xFD;
			ret=i2c_put_byte(client,0x0101,cur_val);
		} else {
			dprintk(dev, 1, "vertical read error\n");
		}
		break;
#endif
	case V4L2_CID_DO_WHITE_BALANCE:
		if(gc0307_qctrl[0].default_value!=value){
			gc0307_qctrl[0].default_value=value;
			set_GC0307_param_wb(dev,value);
			printk(KERN_INFO " set camera  white_balance=%d. \n ",value);
		}
		break;
	case V4L2_CID_EXPOSURE:
		if(gc0307_qctrl[1].default_value!=value){
			gc0307_qctrl[1].default_value=value;
			set_GC0307_param_exposure(dev,value);
			printk(KERN_INFO " set camera  exposure=%d. \n ",value);
		}
		break;
	case V4L2_CID_COLORFX:
		if(gc0307_qctrl[2].default_value!=value){
			gc0307_qctrl[2].default_value=value;
			set_GC0307_param_effect(dev,value);
			printk(KERN_INFO " set camera  effect=%d. \n ",value);
		}
		break;
	case V4L2_CID_WHITENESS:
		if(gc0307_qctrl[3].default_value!=value){
			gc0307_qctrl[3].default_value=value;
			GC0307_set_param_banding(dev,value);
			printk(KERN_INFO " set camera  banding=%d. \n ",value);
		}
		break;
	case V4L2_CID_BLUE_BALANCE:
		if(gc0307_qctrl[4].default_value!=value){
			gc0307_qctrl[4].default_value=value;
			GC0307_night_mode(dev,value);
			printk(KERN_INFO " set camera  scene mode=%d. \n ",value);
		}
		break;
	case V4L2_CID_HFLIP:    /* set flip on H. */          
		value = value & 0x3;
		if(gc0307_qctrl[5].default_value!=value){
			gc0307_qctrl[5].default_value=value;
			printk(" set camera  h filp =%d. \n ",value);
		}
		break;
	case V4L2_CID_VFLIP:    /* set flip on V. */         
		break;
	case V4L2_CID_ZOOM_ABSOLUTE:
		if(gc0307_qctrl[7].default_value!=value){
			gc0307_qctrl[7].default_value=value;
			//printk(KERN_INFO " set camera  zoom mode=%d. \n ",value);
		}
		break;
	case V4L2_CID_ROTATE:
		if(gc0307_qctrl[8].default_value!=value){
			gc0307_qctrl[8].default_value=value;
			printk(" set camera  rotate =%d. \n ",value);
		}
		break;
	default:
		ret=-1;
		break;
	}
	return ret;

}

static void power_down_gc0307(struct gc0307_device *dev)
{
	struct i2c_client *client = v4l2_get_subdevdata(&dev->sd);
	unsigned char buf[4];
	buf[0]=0x13;
	buf[1]=0x01;
	i2c_put_byte_add8(client,buf,2);
	buf[0]=0x44;
	buf[1]=0x00;
	//i2c_put_byte_add8(client,buf,2);
	
	msleep(5);
	return;
}

/* ------------------------------------------------------------------
	DMA and thread functions
   ------------------------------------------------------------------*/

#define TSTAMP_MIN_Y	24
#define TSTAMP_MAX_Y	(TSTAMP_MIN_Y + 15)
#define TSTAMP_INPUT_X	10
#define TSTAMP_MIN_X	(54 + TSTAMP_INPUT_X)

static void gc0307_fillbuff(struct gc0307_fh *fh, struct gc0307_buffer *buf)
{
	struct gc0307_device *dev = fh->dev;
	void *vbuf = videobuf_to_vmalloc(&buf->vb);
	vm_output_para_t para = {0};
	dprintk(dev,1,"%s\n", __func__);
	if (!vbuf)
		return;
 /*  0x18221223 indicate the memory type is MAGIC_VMAL_MEM*/
	para.mirror = gc0307_qctrl[5].default_value&3;
	para.v4l2_format = fh->fmt->fourcc;
	para.v4l2_memory = 0x18221223;
	para.zoom = gc0307_qctrl[7].default_value;
	para.vaddr = (unsigned)vbuf;
	para.angle = gc0307_qctrl[8].default_value;
	vm_fill_buffer(&buf->vb,&para);
	buf->vb.state = VIDEOBUF_DONE;
}

static void gc0307_thread_tick(struct gc0307_fh *fh)
{
	struct gc0307_buffer *buf;
	struct gc0307_device *dev = fh->dev;
	struct gc0307_dmaqueue *dma_q = &dev->vidq;

	unsigned long flags = 0;

	dprintk(dev, 1, "Thread tick\n");

	spin_lock_irqsave(&dev->slock, flags);
	if (list_empty(&dma_q->active)) {
		dprintk(dev, 1, "No active queue to serve\n");
		goto unlock;
	}

	buf = list_entry(dma_q->active.next,
			 struct gc0307_buffer, vb.queue);
    dprintk(dev, 1, "%s\n", __func__);
    dprintk(dev, 1, "list entry get buf is %x\n",(unsigned)buf);

	/* Nobody is waiting on this buffer, return */
	if (!waitqueue_active(&buf->vb.done))
		goto unlock;

	list_del(&buf->vb.queue);

	do_gettimeofday(&buf->vb.ts);

	/* Fill buffer */
	spin_unlock_irqrestore(&dev->slock, flags);
	gc0307_fillbuff(fh, buf);
	dprintk(dev, 1, "filled buffer %p\n", buf);

	wake_up(&buf->vb.done);
	dprintk(dev, 2, "[%p/%d] wakeup\n", buf, buf->vb. i);
	return;
unlock:
	spin_unlock_irqrestore(&dev->slock, flags);
	return;
}

#define frames_to_ms(frames)					\
	((frames * WAKE_NUMERATOR * 1000) / WAKE_DENOMINATOR)

static void gc0307_sleep(struct gc0307_fh *fh)
{
	struct gc0307_device *dev = fh->dev;
	struct gc0307_dmaqueue *dma_q = &dev->vidq;

	DECLARE_WAITQUEUE(wait, current);

	dprintk(dev, 1, "%s dma_q=0x%08lx\n", __func__,
		(unsigned long)dma_q);

	add_wait_queue(&dma_q->wq, &wait);
	if (kthread_should_stop())
		goto stop_task;

	/* Calculate time to wake up */
	//timeout = msecs_to_jiffies(frames_to_ms(1));

	gc0307_thread_tick(fh);

	schedule_timeout_interruptible(2);

stop_task:
	remove_wait_queue(&dma_q->wq, &wait);
	try_to_freeze();
}

static int gc0307_thread(void *data)
{
	struct gc0307_fh  *fh = data;
	struct gc0307_device *dev = fh->dev;

	dprintk(dev, 1, "thread started\n");

	set_freezable();

	for (;;) {
		gc0307_sleep(fh);

		if (kthread_should_stop())
			break;
	}
	dprintk(dev, 1, "thread: exit\n");
	return 0;
}

static int gc0307_start_thread(struct gc0307_fh *fh)
{
	struct gc0307_device *dev = fh->dev;
	struct gc0307_dmaqueue *dma_q = &dev->vidq;

	dma_q->frame = 0;
	dma_q->ini_jiffies = jiffies;

	dprintk(dev, 1, "%s\n", __func__);

	dma_q->kthread = kthread_run(gc0307_thread, fh, "gc0307");

	if (IS_ERR(dma_q->kthread)) {
		v4l2_err(&dev->v4l2_dev, "kernel_thread() failed\n");
		return PTR_ERR(dma_q->kthread);
	}
	/* Wakes thread */
	wake_up_interruptible(&dma_q->wq);

	dprintk(dev, 1, "returning from %s\n", __func__);
	return 0;
}

static void gc0307_stop_thread(struct gc0307_dmaqueue  *dma_q)
{
	struct gc0307_device *dev = container_of(dma_q, struct gc0307_device, vidq);

	dprintk(dev, 1, "%s\n", __func__);
	/* shutdown control thread */
	if (dma_q->kthread) {
		kthread_stop(dma_q->kthread);
		dma_q->kthread = NULL;
	}
}

/* ------------------------------------------------------------------
	Videobuf operations
   ------------------------------------------------------------------*/
static int
buffer_setup(struct videobuf_queue *vq, unsigned int *count, unsigned int *size)
{
	struct gc0307_fh  *fh = vq->priv_data;
	struct gc0307_device *dev  = fh->dev;
    //int bytes = fh->fmt->depth >> 3 ;
	*size = fh->width*fh->height*fh->fmt->depth >> 3;
	if (0 == *count)
		*count = 32;

	while (*size * *count > vid_limit * 1024 * 1024)
		(*count)--;

	dprintk(dev, 1, "%s, count=%d, size=%d\n", __func__,
		*count, *size);

	return 0;
}

static void free_buffer(struct videobuf_queue *vq, struct gc0307_buffer *buf)
{
	struct gc0307_fh  *fh = vq->priv_data;
	struct gc0307_device *dev  = fh->dev;

	dprintk(dev, 1, "%s, state: %i\n", __func__, buf->vb.state);

	if (in_interrupt())
		BUG();

	videobuf_vmalloc_free(&buf->vb);
	dprintk(dev, 1, "free_buffer: freed\n");
	buf->vb.state = VIDEOBUF_NEEDS_INIT;
}

#define norm_maxw() 1024
#define norm_maxh() 768
static int
buffer_prepare(struct videobuf_queue *vq, struct videobuf_buffer *vb,
						enum v4l2_field field)
{
	struct gc0307_fh     *fh  = vq->priv_data;
	struct gc0307_device    *dev = fh->dev;
	struct gc0307_buffer *buf = container_of(vb, struct gc0307_buffer, vb);
	int rc;
    //int bytes = fh->fmt->depth >> 3 ;
	dprintk(dev, 1, "%s, field=%d\n", __func__, field);

	BUG_ON(NULL == fh->fmt);

	if (fh->width  < 48 || fh->width  > norm_maxw() ||
	    fh->height < 32 || fh->height > norm_maxh())
		return -EINVAL;

	buf->vb.size = fh->width*fh->height*fh->fmt->depth >> 3;
	if (0 != buf->vb.baddr  &&  buf->vb.bsize < buf->vb.size)
		return -EINVAL;

	/* These properties only change when queue is idle, see s_fmt */
	buf->fmt       = fh->fmt;
	buf->vb.width  = fh->width;
	buf->vb.height = fh->height;
	buf->vb.field  = field;

	//precalculate_bars(fh);

	if (VIDEOBUF_NEEDS_INIT == buf->vb.state) {
		rc = videobuf_iolock(vq, &buf->vb, NULL);
		if (rc < 0)
			goto fail;
	}

	buf->vb.state = VIDEOBUF_PREPARED;

	return 0;

fail:
	free_buffer(vq, buf);
	return rc;
}

static void
buffer_queue(struct videobuf_queue *vq, struct videobuf_buffer *vb)
{
	struct gc0307_buffer    *buf  = container_of(vb, struct gc0307_buffer, vb);
	struct gc0307_fh        *fh   = vq->priv_data;
	struct gc0307_device       *dev  = fh->dev;
	struct gc0307_dmaqueue *vidq = &dev->vidq;

	dprintk(dev, 1, "%s\n", __func__);
	buf->vb.state = VIDEOBUF_QUEUED;
	list_add_tail(&buf->vb.queue, &vidq->active);
}

static void buffer_release(struct videobuf_queue *vq,
			   struct videobuf_buffer *vb)
{
	struct gc0307_buffer   *buf  = container_of(vb, struct gc0307_buffer, vb);
	struct gc0307_fh       *fh   = vq->priv_data;
	struct gc0307_device      *dev  = (struct gc0307_device *)fh->dev;

	dprintk(dev, 1, "%s\n", __func__);

	free_buffer(vq, buf);
}

static struct videobuf_queue_ops gc0307_video_qops = {
	.buf_setup      = buffer_setup,
	.buf_prepare    = buffer_prepare,
	.buf_queue      = buffer_queue,
	.buf_release    = buffer_release,
};

/* ------------------------------------------------------------------
	IOCTL vidioc handling
   ------------------------------------------------------------------*/
static int vidioc_querycap(struct file *file, void  *priv,
					struct v4l2_capability *cap)
{
	struct gc0307_fh  *fh  = priv;
	struct gc0307_device *dev = fh->dev;

	strcpy(cap->driver, "gc0307");
	strcpy(cap->card, "gc0307");
	strlcpy(cap->bus_info, dev->v4l2_dev.name, sizeof(cap->bus_info));
	cap->version = GC0307_CAMERA_VERSION;
	cap->capabilities =	V4L2_CAP_VIDEO_CAPTURE |
				V4L2_CAP_STREAMING     |
				V4L2_CAP_READWRITE;
	return 0;
}

static int vidioc_enum_fmt_vid_cap(struct file *file, void  *priv,
					struct v4l2_fmtdesc *f)
{
	struct gc0307_fmt *fmt;

	if (f->index >= ARRAY_SIZE(formats))
		return -EINVAL;

	fmt = &formats[f->index];

	strlcpy(f->description, fmt->name, sizeof(f->description));
	f->pixelformat = fmt->fourcc;
	return 0;
}

static int vidioc_g_fmt_vid_cap(struct file *file, void *priv,
					struct v4l2_format *f)
{
	struct gc0307_fh *fh = priv;

	f->fmt.pix.width        = fh->width;
	f->fmt.pix.height       = fh->height;
	f->fmt.pix.field        = fh->vb_vidq.field;
	f->fmt.pix.pixelformat  = fh->fmt->fourcc;
	f->fmt.pix.bytesperline =
		(f->fmt.pix.width * fh->fmt->depth) >> 3;
	f->fmt.pix.sizeimage =
		f->fmt.pix.height * f->fmt.pix.bytesperline;

	return (0);
}

static int vidioc_try_fmt_vid_cap(struct file *file, void *priv,
			struct v4l2_format *f)
{
	struct gc0307_fh  *fh  = priv;
	struct gc0307_device *dev = fh->dev;
	struct gc0307_fmt *fmt;
	enum v4l2_field field;
	unsigned int maxw, maxh;

	fmt = get_format(f);
	if (!fmt) {
		dprintk(dev, 1, "Fourcc format (0x%08x) invalid.\n",
			f->fmt.pix.pixelformat);
		return -EINVAL;
	}

	field = f->fmt.pix.field;

	if (field == V4L2_FIELD_ANY) {
		field = V4L2_FIELD_INTERLACED;
	} else if (V4L2_FIELD_INTERLACED != field) {
		dprintk(dev, 1, "Field type invalid.\n");
		return -EINVAL;
	}

	maxw  = norm_maxw();
	maxh  = norm_maxh();

	f->fmt.pix.field = field;
	v4l_bound_align_image(&f->fmt.pix.width, 48, maxw, 2,
			      &f->fmt.pix.height, 32, maxh, 0, 0);
	f->fmt.pix.bytesperline =
		(f->fmt.pix.width * fmt->depth) >> 3;
	f->fmt.pix.sizeimage =
		f->fmt.pix.height * f->fmt.pix.bytesperline;

	return 0;
}

/*FIXME: This seems to be generic enough to be at videodev2 */
static int vidioc_s_fmt_vid_cap(struct file *file, void *priv,
					struct v4l2_format *f)
{
	struct gc0307_fh *fh = priv;
	struct videobuf_queue *q = &fh->vb_vidq;

	int ret = vidioc_try_fmt_vid_cap(file, fh, f);
	if (ret < 0)
		return ret;

	mutex_lock(&q->vb_lock);

	if (videobuf_queue_is_busy(&fh->vb_vidq)) {
		dprintk(fh->dev, 1, "%s queue busy\n", __func__);
		ret = -EBUSY;
		goto out;
	}

	fh->fmt           = get_format(f);
	fh->width         = f->fmt.pix.width;
	fh->height        = f->fmt.pix.height;
	fh->vb_vidq.field = f->fmt.pix.field;
	fh->type          = f->type;
#if 1
	if(f->fmt.pix.pixelformat==V4L2_PIX_FMT_RGB24){
		gc0307_set_resolution(fh->dev,fh->height,fh->width);
	} else {
		gc0307_set_resolution(fh->dev,fh->height,fh->width);
	}
#endif
	ret = 0;
out:
	mutex_unlock(&q->vb_lock);

	return ret;
}

static int vidioc_reqbufs(struct file *file, void *priv,
			  struct v4l2_requestbuffers *p)
{
	struct gc0307_fh  *fh = priv;

	return (videobuf_reqbufs(&fh->vb_vidq, p));
}

static int vidioc_querybuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	struct gc0307_fh  *fh = priv;

	return (videobuf_querybuf(&fh->vb_vidq, p));
}

static int vidioc_qbuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	struct gc0307_fh *fh = priv;

	return (videobuf_qbuf(&fh->vb_vidq, p));
}

static int vidioc_dqbuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	struct gc0307_fh  *fh = priv;

	return (videobuf_dqbuf(&fh->vb_vidq, p,
				file->f_flags & O_NONBLOCK));
}

#ifdef CONFIG_VIDEO_V4L1_COMPAT
static int vidiocgmbuf(struct file *file, void *priv, struct video_mbuf *mbuf)
{
	struct gc0307_fh  *fh = priv;

	return videobuf_cgmbuf(&fh->vb_vidq, mbuf, 8);
}
#endif

static int vidioc_streamon(struct file *file, void *priv, enum v4l2_buf_type i)
{
	struct gc0307_fh  *fh = priv;
    tvin_parm_t para;
    int ret = 0 ;
	if (fh->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;
	if (i != fh->type)
		return -EINVAL;

    para.port  = TVIN_PORT_CAMERA;
    para.fmt_info.fmt = TVIN_SIG_FMT_MAX+1;//TVIN_SIG_FMT_MAX+1;TVIN_SIG_FMT_CAMERA_1280X720P_30Hz
	para.fmt_info.frame_rate = gc0307_frame_rate;//150;
	para.fmt_info.h_active = gc0307_h_active;
	para.fmt_info.v_active = gc0307_v_active;
	para.fmt_info.hsync_phase = 0;
	para.fmt_info.vsync_phase  = 1;	
	ret =  videobuf_streamon(&fh->vb_vidq);
	if(ret == 0){
    start_tvin_service(0,&para);
	    fh->stream_on        = 1;
	}
	return ret;
}

static int vidioc_streamoff(struct file *file, void *priv, enum v4l2_buf_type i)
{
	struct gc0307_fh  *fh = priv;

    int ret = 0 ;
	if (fh->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;
	if (i != fh->type)
		return -EINVAL;
	ret = videobuf_streamoff(&fh->vb_vidq);
	if(ret == 0 ){
    stop_tvin_service(0);
	    fh->stream_on        = 0;
	}
	return ret;
}

static int vidioc_enum_framesizes(struct file *file, void *fh,struct v4l2_frmsizeenum *fsize)
{
	int ret = 0,i=0;
	struct gc0307_fmt *fmt = NULL;
	struct v4l2_frmsize_discrete *frmsize = NULL;
	for (i = 0; i < ARRAY_SIZE(formats); i++) {
		if (formats[i].fourcc == fsize->pixel_format){
			fmt = &formats[i];
			break;
		}
	}
	if (fmt == NULL)
		return -EINVAL;
	if ((fmt->fourcc == V4L2_PIX_FMT_NV21)
		||(fmt->fourcc == V4L2_PIX_FMT_NV12)
		||(fmt->fourcc == V4L2_PIX_FMT_YUV420)
		||(fmt->fourcc == V4L2_PIX_FMT_YVU420)
		){
		if (fsize->index >= ARRAY_SIZE(gc0307_prev_resolution))
			return -EINVAL;
		frmsize = &gc0307_prev_resolution[fsize->index];
		fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
		fsize->discrete.width = frmsize->width;
		fsize->discrete.height = frmsize->height;
	}
	else if(fmt->fourcc == V4L2_PIX_FMT_RGB24){
		if (fsize->index >= ARRAY_SIZE(gc0307_pic_resolution))
			return -EINVAL;
		frmsize = &gc0307_pic_resolution[fsize->index];
		fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
		fsize->discrete.width = frmsize->width;
		fsize->discrete.height = frmsize->height;
	}
	return ret;
}

static int vidioc_s_std(struct file *file, void *priv, v4l2_std_id *i)
{
	return 0;
}

/* only one input in this sample driver */
static int vidioc_enum_input(struct file *file, void *priv,
				struct v4l2_input *inp)
{
	//if (inp->index >= NUM_INPUTS)
		//return -EINVAL;

	inp->type = V4L2_INPUT_TYPE_CAMERA;
	inp->std = V4L2_STD_525_60;
	sprintf(inp->name, "Camera %u", inp->index);

	return (0);
}

static int vidioc_g_input(struct file *file, void *priv, unsigned int *i)
{
	struct gc0307_fh *fh = priv;
	struct gc0307_device *dev = fh->dev;

	*i = dev->input;

	return (0);
}

static int vidioc_s_input(struct file *file, void *priv, unsigned int i)
{
	struct gc0307_fh *fh = priv;
	struct gc0307_device *dev = fh->dev;

	//if (i >= NUM_INPUTS)
		//return -EINVAL;

	dev->input = i;
	//precalculate_bars(fh);

	return (0);
}

	/* --- controls ---------------------------------------------- */
static int vidioc_queryctrl(struct file *file, void *priv,
			    struct v4l2_queryctrl *qc)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(gc0307_qctrl); i++)
		if (qc->id && qc->id == gc0307_qctrl[i].id) {
			memcpy(qc, &(gc0307_qctrl[i]),
				sizeof(*qc));
			return (0);
		}

	return -EINVAL;
}

static int vidioc_g_ctrl(struct file *file, void *priv,
			 struct v4l2_control *ctrl)
{
	struct gc0307_fh *fh = priv;
	struct gc0307_device *dev = fh->dev;
	int i;

	for (i = 0; i < ARRAY_SIZE(gc0307_qctrl); i++)
		if (ctrl->id == gc0307_qctrl[i].id) {
			ctrl->value = dev->qctl_regs[i];
			return 0;
		}

	return -EINVAL;
}

static int vidioc_s_ctrl(struct file *file, void *priv,
				struct v4l2_control *ctrl)
{
	struct gc0307_fh *fh = priv;
	struct gc0307_device *dev = fh->dev;
	int i;

	for (i = 0; i < ARRAY_SIZE(gc0307_qctrl); i++)
		if (ctrl->id == gc0307_qctrl[i].id) {
			if (ctrl->value < gc0307_qctrl[i].minimum ||
			    ctrl->value > gc0307_qctrl[i].maximum ||
			    gc0307_setting(dev,ctrl->id,ctrl->value)<0) {
				return -ERANGE;
			}
			dev->qctl_regs[i] = ctrl->value;
			return 0;
		}
	return -EINVAL;
}

/* ------------------------------------------------------------------
	File operations for the device
   ------------------------------------------------------------------*/

static int gc0307_open(struct file *file)
{
	struct gc0307_device *dev = video_drvdata(file);
	struct gc0307_fh *fh = NULL;
	int retval = 0;
	gc0307_have_open=1;
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6
	switch_mod_gate_by_name("ge2d", 1);
#endif	
	if(dev->platform_dev_data.device_init) {
		dev->platform_dev_data.device_init();
		printk("+++found a init function, and run it..\n");
	}
	GC0307_init_regs(dev);
	msleep(100);//40
	mutex_lock(&dev->mutex);
	dev->users++;
	if (dev->users > 1) {
		dev->users--;
		mutex_unlock(&dev->mutex);
		return -EBUSY;
	}

	dprintk(dev, 1, "open %s type=%s users=%d\n",
		video_device_node_name(dev->vdev),
		v4l2_type_names[V4L2_BUF_TYPE_VIDEO_CAPTURE], dev->users);

    	/* init video dma queues */
	INIT_LIST_HEAD(&dev->vidq.active);
	init_waitqueue_head(&dev->vidq.wq);
    spin_lock_init(&dev->slock);
	/* allocate + initialize per filehandle data */
	fh = kzalloc(sizeof(*fh), GFP_KERNEL);
	if (NULL == fh) {
		dev->users--;
		retval = -ENOMEM;
	}
	mutex_unlock(&dev->mutex);

	if (retval)
		return retval;

	wake_lock(&(dev->wake_lock));
	file->private_data = fh;
	fh->dev      = dev;

	fh->type     = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fh->fmt      = &formats[0];
	fh->width    = 640;
	fh->height   = 480;
	fh->stream_on = 0 ;
	/* Resets frame counters */
	dev->jiffies = jiffies;

//    TVIN_SIG_FMT_CAMERA_640X480P_30Hz,
//    TVIN_SIG_FMT_CAMERA_800X600P_30Hz,
//    TVIN_SIG_FMT_CAMERA_1024X768P_30Hz, // 190
//    TVIN_SIG_FMT_CAMERA_1920X1080P_30Hz,
//    TVIN_SIG_FMT_CAMERA_1280X720P_30Hz,

	videobuf_queue_vmalloc_init(&fh->vb_vidq, &gc0307_video_qops,
			NULL, &dev->slock, fh->type, V4L2_FIELD_INTERLACED,
			sizeof(struct gc0307_buffer), fh,NULL);

	gc0307_start_thread(fh);

	return 0;
}

static ssize_t
gc0307_read(struct file *file, char __user *data, size_t count, loff_t *ppos)
{
	struct gc0307_fh *fh = file->private_data;

	if (fh->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		return videobuf_read_stream(&fh->vb_vidq, data, count, ppos, 0,
					file->f_flags & O_NONBLOCK);
	}
	return 0;
}

static unsigned int
gc0307_poll(struct file *file, struct poll_table_struct *wait)
{
	struct gc0307_fh        *fh = file->private_data;
	struct gc0307_device       *dev = fh->dev;
	struct videobuf_queue *q = &fh->vb_vidq;

	dprintk(dev, 1, "%s\n", __func__);

	if (V4L2_BUF_TYPE_VIDEO_CAPTURE != fh->type)
		return POLLERR;

	return videobuf_poll_stream(file, q, wait);
}

static int gc0307_close(struct file *file)
{
	struct gc0307_fh         *fh = file->private_data;
	struct gc0307_device *dev       = fh->dev;
	struct gc0307_dmaqueue *vidq = &dev->vidq;
	struct video_device  *vdev = video_devdata(file);
	gc0307_have_open=0;

	gc0307_stop_thread(vidq);
	videobuf_stop(&fh->vb_vidq);
	if(fh->stream_on){
	    stop_tvin_service(0);
	}
	videobuf_mmap_free(&fh->vb_vidq);

	kfree(fh);

	mutex_lock(&dev->mutex);
	dev->users--;
	mutex_unlock(&dev->mutex);

	dprintk(dev, 1, "close called (dev=%s, users=%d)\n",
		video_device_node_name(vdev), dev->users);
#if 1
	gc0307_qctrl[0].default_value=0;
	gc0307_qctrl[1].default_value=4;
	gc0307_qctrl[2].default_value=0;
	gc0307_qctrl[3].default_value=0;
	gc0307_qctrl[4].default_value=0;

	gc0307_qctrl[5].default_value=0;
	gc0307_qctrl[7].default_value=100;
	gc0307_qctrl[8].default_value=0;
	//power_down_gc0307(dev);
#endif
	if(dev->platform_dev_data.device_uninit) {
		dev->platform_dev_data.device_uninit();
		printk("+++found a uninit function, and run it..\n");
	}
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6
	switch_mod_gate_by_name("ge2d", 0);
#endif	
	wake_unlock(&(dev->wake_lock));
	return 0;
}

static int gc0307_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct gc0307_fh  *fh = file->private_data;
	struct gc0307_device *dev = fh->dev;
	int ret;

	dprintk(dev, 1, "mmap called, vma=0x%08lx\n", (unsigned long)vma);

	ret = videobuf_mmap_mapper(&fh->vb_vidq, vma);

	dprintk(dev, 1, "vma start=0x%08lx, size=%ld, ret=%d\n",
		(unsigned long)vma->vm_start,
		(unsigned long)vma->vm_end-(unsigned long)vma->vm_start,
		ret);

	return ret;
}

static const struct v4l2_file_operations gc0307_fops = {
	.owner		= THIS_MODULE,
	.open           = gc0307_open,
	.release        = gc0307_close,
	.read           = gc0307_read,
	.poll		= gc0307_poll,
	.ioctl          = video_ioctl2, /* V4L2 ioctl handler */
	.mmap           = gc0307_mmap,
};

static const struct v4l2_ioctl_ops gc0307_ioctl_ops = {
	.vidioc_querycap      = vidioc_querycap,
	.vidioc_enum_fmt_vid_cap  = vidioc_enum_fmt_vid_cap,
	.vidioc_g_fmt_vid_cap     = vidioc_g_fmt_vid_cap,
	.vidioc_try_fmt_vid_cap   = vidioc_try_fmt_vid_cap,
	.vidioc_s_fmt_vid_cap     = vidioc_s_fmt_vid_cap,
	.vidioc_reqbufs       = vidioc_reqbufs,
	.vidioc_querybuf      = vidioc_querybuf,
	.vidioc_qbuf          = vidioc_qbuf,
	.vidioc_dqbuf         = vidioc_dqbuf,
	.vidioc_s_std         = vidioc_s_std,
	.vidioc_enum_input    = vidioc_enum_input,
	.vidioc_g_input       = vidioc_g_input,
	.vidioc_s_input       = vidioc_s_input,
	.vidioc_queryctrl     = vidioc_queryctrl,
	.vidioc_g_ctrl        = vidioc_g_ctrl,
	.vidioc_s_ctrl        = vidioc_s_ctrl,
	.vidioc_streamon      = vidioc_streamon,
	.vidioc_streamoff     = vidioc_streamoff,
	.vidioc_enum_framesizes = vidioc_enum_framesizes,
#ifdef CONFIG_VIDEO_V4L1_COMPAT
	.vidiocgmbuf          = vidiocgmbuf,
#endif
};

static struct video_device gc0307_template = {
	.name		= "gc0307_v4l",
	.fops           = &gc0307_fops,
	.ioctl_ops 	= &gc0307_ioctl_ops,
	.release	= video_device_release,

	.tvnorms              = V4L2_STD_525_60,
	.current_norm         = V4L2_STD_NTSC_M,
};

static int gc0307_g_chip_ident(struct v4l2_subdev *sd, struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	return v4l2_chip_ident_i2c_client(client, chip, V4L2_IDENT_GC0307, 0);
}

static const struct v4l2_subdev_core_ops gc0307_core_ops = {
	.g_chip_ident = gc0307_g_chip_ident,
};

static const struct v4l2_subdev_ops gc0307_ops = {
	.core = &gc0307_core_ops,
};

static int gc0307_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	aml_plat_cam_data_t* plat_dat;
	int err;
	struct gc0307_device *t;
	struct v4l2_subdev *sd;
	v4l_info(client, "chip found @ 0x%x (%s)\n",
			client->addr << 1, client->adapter->name);
	t = kzalloc(sizeof(*t), GFP_KERNEL);
	if (t == NULL)
		return -ENOMEM;
	sd = &t->sd;
	v4l2_i2c_subdev_init(sd, client, &gc0307_ops);
	plat_dat= (aml_plat_cam_data_t*)client->dev.platform_data;
	
	/* test if devices exist. */
#ifdef CONFIG_VIDEO_AMLOGIC_CAPTURE_PROBE
	unsigned char buf[4]; 
	buf[0]=0;
	plat_dat->device_init();
	err=i2c_get_byte_add8(client,buf);
	plat_dat->device_uninit();
	if(err<0) return  -ENODEV;
#endif
	/* Now create a video4linux device */
	mutex_init(&t->mutex);

	/* Now create a video4linux device */
	t->vdev = video_device_alloc();
	if (t->vdev == NULL) {
		kfree(t);
		kfree(client);
		return -ENOMEM;
	}
	memcpy(t->vdev, &gc0307_template, sizeof(*t->vdev));

	video_set_drvdata(t->vdev, t);

	wake_lock_init(&(t->wake_lock),WAKE_LOCK_SUSPEND, "gc0307");

	/* Register it */
	if (plat_dat) {
		t->platform_dev_data.device_init=plat_dat->device_init;
		t->platform_dev_data.device_uninit=plat_dat->device_uninit;
		if(plat_dat->video_nr>=0)  video_nr=plat_dat->video_nr;
			if(t->platform_dev_data.device_init) {
			t->platform_dev_data.device_init();
			printk("+++found a init function, and run it..\n");
		    }
			//power_down_gc0307(t);
			if(t->platform_dev_data.device_uninit) {
			t->platform_dev_data.device_uninit();
			printk("+++found a uninit function, and run it..\n");
		    }
	}
	err = video_register_device(t->vdev, VFL_TYPE_GRABBER, video_nr);
	if (err < 0) {
		video_device_release(t->vdev);
		kfree(t);
		return err;
	}

	return 0;
}

static int gc0307_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct gc0307_device *t = to_dev(sd);

	video_unregister_device(t->vdev);
	v4l2_device_unregister_subdev(sd);
	wake_lock_destroy(&(t->wake_lock));
	kfree(t);
	return 0;
}

static const struct i2c_device_id gc0307_id[] = {
	{ "gc0307_i2c", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, gc0307_id);

static struct v4l2_i2c_driver_data v4l2_i2c_data = {
	.name = "gc0307",
	.probe = gc0307_probe,
	.remove = gc0307_remove,
	.id_table = gc0307_id,
};
