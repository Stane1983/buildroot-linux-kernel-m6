/*
 * pass a frame of amlogic video codec device  to user in style of v4l2
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
#include <media/videobuf-res.h>
#include <media/videobuf-core.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <linux/types.h>
#include <linux/amports/canvas.h>
#include <linux/amports/vframe.h>
#include <linux/amports/vframe_provider.h>
#include <linux/amports/vframe_receiver.h>
#include <linux/ge2d/ge2d.h>
#include <linux/amports/timestamp.h>
#include <linux/kernel.h>
#include <linux/tvin/tvin_v4l2.h>
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6
#include <mach/mod_gate.h>
#endif

#define AVMLVIDEO2_MODULE_NAME "amlvideo2"

//#define MUTLI_NODE
#ifdef MUTLI_NODE
#define MAX_SUB_DEV_NODE 2
#else
#define MAX_SUB_DEV_NODE 1
#endif

/* Wake up at about 30 fps */
#define WAKE_NUMERATOR 30
#define WAKE_DENOMINATOR 1001

#define AMLVIDEO2_MAJOR_VERSION 0
#define AMLVIDEO2_MINOR_VERSION 7
#define AMLVIDEO2_RELEASE 1
#define AMLVIDEO2_VERSION \
	KERNEL_VERSION(AMLVIDEO2_MAJOR_VERSION, AMLVIDEO2_MINOR_VERSION, AMLVIDEO2_RELEASE)

#define MAGIC_SG_MEM 0x17890714
#define MAGIC_DC_MEM 0x0733ac61
#define MAGIC_VMAL_MEM 0x18221223
#define MAGIC_RE_MEM 0x123039dc

#ifdef MUTLI_NODE
#define RECEIVER_NAME0 "amlvideo2_0"
#define RECEIVER_NAME1 "amlvideo2_1"
#else
#define RECEIVER_NAME "amlvideo2"
#endif

#define VM_RES0_CANVAS_INDEX AMLVIDEO2_RES_CANVAS
#define VM_RES0_CANVAS_INDEX_U AMLVIDEO2_RES_CANVAS+1
#define VM_RES0_CANVAS_INDEX_V AMLVIDEO2_RES_CANVAS+2
#define VM_RES0_CANVAS_INDEX_UV AMLVIDEO2_RES_CANVAS+3

#define VM_RES1_CANVAS_INDEX AMLVIDEO2_RES_CANVAS+4
#define VM_RES1_CANVAS_INDEX_U AMLVIDEO2_RES_CANVAS+5
#define VM_RES1_CANVAS_INDEX_V AMLVIDEO2_RES_CANVAS+6
#define VM_RES1_CANVAS_INDEX_UV AMLVIDEO2_RES_CANVAS+7

#define DUR2PTS(x) ((x) - ((x) >> 4))
#define DUR2PTS_RM(x) ((x) & 0xf)

MODULE_DESCRIPTION("pass a frame of amlogic video2 codec device  to user in style of v4l2");
MODULE_AUTHOR("amlogic-sh");
MODULE_LICENSE("GPL");
static unsigned video_nr = 11;
//module_param(video_nr, uint, 0644);
//MODULE_PARM_DESC(video_nr, "videoX start number, 10 is defaut");

static unsigned debug=0;
//module_param(debug, uint, 0644);
//MODULE_PARM_DESC(debug, "activates debug info");

struct timeval thread_ts1;
struct timeval thread_ts2;
#define MIN_FRAMERATE 30

static unsigned int vid_limit = 16;
module_param(vid_limit, uint, 0644);
MODULE_PARM_DESC(vid_limit, "capture memory limit in megabytes");

static struct v4l2_fract amlvideo2_frmintervals_active = {
	.numerator = 1,
	.denominator = 30,
};

struct vdin_v4l2_ops_s vops;
/* supported controls */
//static struct v4l2_queryctrl amlvideo2_node_qctrl[] = {
//};
static struct v4l2_frmivalenum amlvideo2_frmivalenum[]={
    {
        .index      = 0,
        .pixel_format   = V4L2_PIX_FMT_NV21,
        .width      = 1920,
        .height     = 1080,
        .type       = V4L2_FRMIVAL_TYPE_DISCRETE,
        {
            .discrete   ={
                .numerator  = 1,
                .denominator    = 30,
            }
        }
    },{
        .index      = 1,
        .pixel_format   = V4L2_PIX_FMT_NV21,
        .width      = 1600,
        .height     = 1200,
        .type       = V4L2_FRMIVAL_TYPE_DISCRETE,
        {
            .discrete   ={
                .numerator  = 1,
                .denominator    = 5,
            }
        }
    },
};
#define dprintk(dev, level, fmt, arg...) \
	v4l2_dbg(level, debug, &dev->v4l2_dev, fmt, ## arg)

/* ------------------------------------------------------------------
	Basic structures
   ------------------------------------------------------------------*/

typedef enum{
	AML_PROVIDE_NONE  = 0,
	AML_PROVIDE_VDIN0  = 1,
	AML_PROVIDE_VDIN1  = 2,
	AML_PROVIDE_DECODE  = 3,
	AML_PROVIDE_MAX  = 4
}aml_provider_type;

struct amlvideo2_fmt {
	char  *name;
	u32   fourcc;          /* v4l2 format id */
	int   depth;
};

static struct amlvideo2_fmt formats[] = {
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
#if 0
	{
		.name     = "RGBA8888 (32)",
		.fourcc   = V4L2_PIX_FMT_RGB32, /* 24  RGBA-8-8-8-8 */
		.depth    = 32,
	},
	{
		.name     = "RGB565 (BE)",
		.fourcc   = V4L2_PIX_FMT_RGB565X, /* rrrrrggg gggbbbbb */
		.depth    = 16,
	},	
	{
		.name     = "BGR888 (24)",
		.fourcc   = V4L2_PIX_FMT_BGR24, /* 24  BGR-8-8-8 */
		.depth    = 24,
	},		
	{
		.name     = "YUV420P",
		.fourcc   = V4L2_PIX_FMT_YUV420,
		.depth    = 12,
	},
#endif
};

static struct amlvideo2_fmt *get_format(struct v4l2_format *f)
{
	struct amlvideo2_fmt *fmt;
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

/* buffer for one video frame */
struct amlvideo2_node_buffer {
	/* common v4l buffer stuff -- must be first */
	struct videobuf_buffer vb;

	struct amlvideo2_fmt        *fmt;
};

struct amlvideo2_node_dmaqueue {
	struct list_head       active;

	/* thread for generating video stream*/
	struct task_struct         *kthread;
	wait_queue_head_t          wq;
};

struct amlvideo2_device{
	struct mutex		   mutex;
	struct v4l2_device v4l2_dev;

	struct amlvideo2_node *node[MAX_SUB_DEV_NODE];
	int node_num;
};

struct amlvideo2_node {

	spinlock_t                 slock;
	struct mutex		   mutex;
	int vid;
	int                        users;
	
	struct amlvideo2_device *vid_dev;

	/* various device info */
	struct video_device        *vfd;

	struct amlvideo2_node_dmaqueue       vidq;

	/* Control 'registers' */
	//int 			   qctl_regs[ARRAY_SIZE(amlvideo2_node_qctrl[0])];
	struct videobuf_res_privdata res;
	struct vframe_receiver_s recv;
	//struct vframe_provider_s * provider;
	//aml_provider_type p_type;
	int provide_ready;
	
	ge2d_context_t *context;
};

struct amlvideo2_fh {
	struct amlvideo2_node   *node;

	/* video capture */
	struct amlvideo2_fmt    *fmt;
	unsigned int			width, height;
	struct videobuf_queue   vb_vidq;
	unsigned int            is_streamed_on;

	suseconds_t             frm_save_time_us; //us
	unsigned int            f_flags;
	enum v4l2_buf_type      type;
};

struct amlvideo2_output {
	int canvas_id;
	void* vbuf;
	int width;
	int height;
	u32 v4l2_format;
};

static struct v4l2_frmsize_discrete amlvideo2_prev_resolution[]= 
{
	{160,120},
	{320,240},
	{640,480},
	{1280,720},
};

static struct v4l2_frmsize_discrete amlvideo2_pic_resolution[]=
{
	{1280,960}
};

static unsigned print_cvs_idx=0;
module_param(print_cvs_idx, uint, 0644);
MODULE_PARM_DESC(print_cvs_idx, "print canvas index\n");

int get_amlvideo2_canvas_index(struct amlvideo2_output* output, int start_canvas)
{
	int canvas = start_canvas;
	int v4l2_format = output->v4l2_format;
	unsigned buf = (unsigned)output->vbuf;
	int width = output->width;
	int height = output->height;

	switch(v4l2_format){
	case V4L2_PIX_FMT_RGB565X:
	case V4L2_PIX_FMT_VYUY:
		canvas = start_canvas;
		canvas_config(canvas,
			(unsigned long)buf,
			width*2, height,
			CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);
		break;
	case V4L2_PIX_FMT_YUV444:
	case V4L2_PIX_FMT_BGR24:
	case V4L2_PIX_FMT_RGB24:
		canvas = start_canvas;
		canvas_config(canvas,
			(unsigned long)buf,
			width*3, height,
			CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);
		break; 
	case V4L2_PIX_FMT_NV12:
	case V4L2_PIX_FMT_NV21: 
		canvas_config(start_canvas,
			(unsigned long)buf,
			width, height,
			CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);
		canvas_config(start_canvas+3,
			(unsigned long)(buf+width*height),
			width, height/2,
			CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);
		canvas = start_canvas | ((start_canvas+3)<<8);
		break;
	case V4L2_PIX_FMT_YVU420:
	case V4L2_PIX_FMT_YUV420:
		canvas_config(start_canvas,
			(unsigned long)buf,
			width, height,
			CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);
		canvas_config(start_canvas+1,
			(unsigned long)(buf+width*height),
			width/2, height/2,
			CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);
		canvas_config(start_canvas+2,
			(unsigned long)(buf+width*height*5/4),
			width/2, height/2,
			CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);

        if(V4L2_PIX_FMT_YUV420 == v4l2_format){
            canvas = start_canvas|((start_canvas+1)<<8)|((start_canvas+2)<<16);
        }else{
            canvas = start_canvas|((start_canvas+2)<<8)|((start_canvas+1)<<16);
        }

		break;
	default:
		break;
	}
	if(print_cvs_idx==1){
		printk("v4l2_format=%x, canvas=%x\n", v4l2_format, canvas);
		print_cvs_idx = 0;
	}
	return canvas;
}

static unsigned int print_ifmt=0;
module_param(print_ifmt, uint, 0644);
MODULE_PARM_DESC(print_ifmt, "print input format\n");

static int get_input_format(vframe_t* vf)
{
	int format= GE2D_FORMAT_M24_NV21;
	if(vf->type&VIDTYPE_VIU_422){
		if(vf->type &VIDTYPE_INTERLACE_BOTTOM){
			format =  GE2D_FORMAT_S16_YUV422|(GE2D_FORMAT_S16_YUV422B & (3<<3));
		}else if(vf->type &VIDTYPE_INTERLACE_TOP){
			format =  GE2D_FORMAT_S16_YUV422|(GE2D_FORMAT_S16_YUV422T & (3<<3));
		}else{
			format =  GE2D_FORMAT_S16_YUV422;
		} 
	}else if(vf->type&VIDTYPE_VIU_NV21){
		if(vf->type &VIDTYPE_INTERLACE_BOTTOM){
			format =  GE2D_FORMAT_M24_NV21|(GE2D_FORMAT_M24_NV21B & (3<<3));
		}else if(vf->type &VIDTYPE_INTERLACE_TOP){
			format =  GE2D_FORMAT_M24_NV21|(GE2D_FORMAT_M24_NV21T & (3<<3));
		}else{
			format =  GE2D_FORMAT_M24_NV21;
		} 
	} else{
		if(vf->type &VIDTYPE_INTERLACE_BOTTOM){
			format =  GE2D_FORMAT_M24_YUV420|(GE2D_FMT_M24_YUV420B & (3<<3));
		}else if(vf->type &VIDTYPE_INTERLACE_TOP){
			format =  GE2D_FORMAT_M24_YUV420|(GE2D_FORMAT_M24_YUV420T & (3<<3));
		}else{
			format =  GE2D_FORMAT_M24_YUV420;
		}
	}
	if(print_ifmt == 1){
		printk("vf->type=%x, format=%x\n", vf->type, format);
		print_ifmt = 0;
	}
	return format;
}

static unsigned int print_ofmt=0;
module_param(print_ofmt, uint, 0644);
MODULE_PARM_DESC(print_ofmt, "print output format\n");

static int get_output_format(int v4l2_format)
{
	int format = GE2D_FORMAT_S24_YUV444;
	switch(v4l2_format){
		case V4L2_PIX_FMT_RGB565X:
			format = GE2D_FORMAT_S16_RGB_565;
			break;
		case V4L2_PIX_FMT_YUV444:
			format = GE2D_FORMAT_S24_YUV444;
			break;
		case V4L2_PIX_FMT_VYUY:
			format = GE2D_FORMAT_S16_YUV422;
			break;
		case V4L2_PIX_FMT_BGR24:
			format = GE2D_FORMAT_S24_RGB ;
			break;
		case V4L2_PIX_FMT_RGB24:
			format = GE2D_FORMAT_S24_BGR; 
			break;
		case V4L2_PIX_FMT_NV12:
			format = GE2D_FORMAT_M24_NV12;
			break;
		case V4L2_PIX_FMT_NV21:
			format = GE2D_FORMAT_M24_NV21;
			break;
		case V4L2_PIX_FMT_YUV420:
		case V4L2_PIX_FMT_YVU420:
			format = GE2D_FORMAT_S8_Y;
			break;
		default:
			break;            
	}   
	if(print_ofmt == 1){
            printk("outputformat, v4l2_format=%x, format=%x\n",
                    v4l2_format, format);
            print_ofmt = 0;
	}
	return format;
}


int amlvideo2_ge2d_pre_process(vframe_t* vf, ge2d_context_t *context,config_para_ex_t* ge2d_config, struct amlvideo2_output* output)
{
	int src_top ,src_left ,src_width, src_height;
	canvas_t cs0,cs1,cs2,cd;
	int current_mirror = 0;
	int cur_angle = 0;
	int output_canvas  = output->canvas_id;

	src_top = 0;
	src_left = 0;
	src_width = vf->width;
	src_height = vf->height;

	current_mirror = 0;
	cur_angle = 0;
	if(current_mirror == 1)
		cur_angle = (360 - cur_angle%360);
	else
		cur_angle = cur_angle%360;
	/* data operating. */ 
	ge2d_config->alu_const_color= 0;//0x000000ff;
	ge2d_config->bitmask_en  = 0;
	ge2d_config->src1_gb_alpha = 0;//0xff;
	ge2d_config->dst_xy_swap = 0;

	canvas_read(vf->canvas0Addr&0xff,&cs0);
	canvas_read((vf->canvas0Addr>>8)&0xff,&cs1);
	canvas_read((vf->canvas0Addr>>16)&0xff,&cs2);
	ge2d_config->src_planes[0].addr = cs0.addr;
	ge2d_config->src_planes[0].w = cs0.width;
	ge2d_config->src_planes[0].h = cs0.height;
	ge2d_config->src_planes[1].addr = cs1.addr;
	ge2d_config->src_planes[1].w = cs1.width;
	ge2d_config->src_planes[1].h = cs1.height;
	ge2d_config->src_planes[2].addr = cs2.addr;
	ge2d_config->src_planes[2].w = cs2.width;
	ge2d_config->src_planes[2].h = cs2.height;
	canvas_read(output_canvas&0xff,&cd);
	ge2d_config->dst_planes[0].addr = cd.addr;
	ge2d_config->dst_planes[0].w = cd.width;
	ge2d_config->dst_planes[0].h = cd.height;
	ge2d_config->src_key.key_enable = 0;
	ge2d_config->src_key.key_mask = 0;
	ge2d_config->src_key.key_mode = 0;
	ge2d_config->src_para.canvas_index=vf->canvas0Addr;
	ge2d_config->src_para.mem_type = CANVAS_TYPE_INVALID;
	ge2d_config->src_para.format = get_input_format(vf);
	ge2d_config->src_para.fill_color_en = 0;
	ge2d_config->src_para.fill_mode = 0;
	ge2d_config->src_para.x_rev = 0;
	ge2d_config->src_para.y_rev = 0;
	ge2d_config->src_para.color = 0xffffffff;
	ge2d_config->src_para.top = 0;
	ge2d_config->src_para.left = 0;
	ge2d_config->src_para.width = vf->width;
	ge2d_config->src_para.height = vf->height;
	/* printk("vf_width is %d , vf_height is %d \n",vf->width ,vf->height); */
	ge2d_config->src2_para.mem_type = CANVAS_TYPE_INVALID;
	ge2d_config->dst_para.canvas_index = output_canvas&0xff;

	if((output->v4l2_format!= V4L2_PIX_FMT_YUV420)
		&& (output->v4l2_format != V4L2_PIX_FMT_YVU420))
		ge2d_config->dst_para.canvas_index = output_canvas;

	ge2d_config->dst_para.mem_type = CANVAS_TYPE_INVALID;
	ge2d_config->dst_para.format = get_output_format(output->v4l2_format)|GE2D_LITTLE_ENDIAN;     
	ge2d_config->dst_para.fill_color_en = 0;
	ge2d_config->dst_para.fill_mode = 0;
	ge2d_config->dst_para.x_rev = 0;
	ge2d_config->dst_para.y_rev = 0;
	ge2d_config->dst_para.color = 0;
	ge2d_config->dst_para.top = 0;
	ge2d_config->dst_para.left = 0;
	ge2d_config->dst_para.width = output->width;
	ge2d_config->dst_para.height = output->height;

	if(current_mirror==1){
		ge2d_config->dst_para.x_rev = 1;
		ge2d_config->dst_para.y_rev = 0;
	}else if(current_mirror==2){
		ge2d_config->dst_para.x_rev = 0;
		ge2d_config->dst_para.y_rev = 1;
	}else{
		ge2d_config->dst_para.x_rev = 0;
		ge2d_config->dst_para.y_rev = 0;
	}

	if(cur_angle==90){
		ge2d_config->dst_xy_swap = 1;
		ge2d_config->dst_para.x_rev ^= 1;
	}else if(cur_angle==180){
		ge2d_config->dst_para.x_rev ^= 1;
		ge2d_config->dst_para.y_rev ^= 1;
	}else if(cur_angle==270){
		ge2d_config->dst_xy_swap = 1;
		ge2d_config->dst_para.y_rev ^= 1;
	}

	if(ge2d_context_config_ex(context,ge2d_config)<0) {
		printk("++ge2d configing error.\n");
		return -1;
	}
	stretchblt_noalpha(context,src_left ,src_top ,src_width, src_height,0,0,output->width,output->height);

	/* for cr of  yuv420p or yuv420sp. */
	if((output->v4l2_format==V4L2_PIX_FMT_YUV420)
		||(output->v4l2_format==V4L2_PIX_FMT_YVU420)){
		/* for cb. */
		canvas_read((output_canvas>>8)&0xff,&cd);
		ge2d_config->dst_planes[0].addr = cd.addr;
		ge2d_config->dst_planes[0].w = cd.width;
		ge2d_config->dst_planes[0].h = cd.height;
		ge2d_config->dst_para.canvas_index=(output_canvas>>8)&0xff;
		ge2d_config->dst_para.format=GE2D_FORMAT_S8_CB|GE2D_LITTLE_ENDIAN;
		ge2d_config->dst_para.width = output->width/2;
		ge2d_config->dst_para.height = output->height/2;
		ge2d_config->dst_xy_swap = 0;

		if(current_mirror==1){
			ge2d_config->dst_para.x_rev = 1;
			ge2d_config->dst_para.y_rev = 0;
		}else if(current_mirror==2){
			ge2d_config->dst_para.x_rev = 0;
			ge2d_config->dst_para.y_rev = 1;
		}else{
			ge2d_config->dst_para.x_rev = 0;
			ge2d_config->dst_para.y_rev = 0;
		}

		if(cur_angle==90){
			ge2d_config->dst_xy_swap = 1;
			ge2d_config->dst_para.x_rev ^= 1;
		}else if(cur_angle==180){
			ge2d_config->dst_para.x_rev ^= 1;
			ge2d_config->dst_para.y_rev ^= 1;
		}else if(cur_angle==270){
			ge2d_config->dst_xy_swap = 1;
			ge2d_config->dst_para.y_rev ^= 1;
		}
	
		if(ge2d_context_config_ex(context,ge2d_config)<0) {
			printk("++ge2d configing error.\n");
			return -1;
		}
		stretchblt_noalpha(context, src_left, src_top, src_width, src_height,
			0, 0, ge2d_config->dst_para.width,ge2d_config->dst_para.height);
	} 
	/* for cb of yuv420p or yuv420sp. */
	if(output->v4l2_format==V4L2_PIX_FMT_YUV420||
		output->v4l2_format==V4L2_PIX_FMT_YVU420) {
		canvas_read((output_canvas>>16)&0xff,&cd);
		ge2d_config->dst_planes[0].addr = cd.addr;
		ge2d_config->dst_planes[0].w = cd.width;
		ge2d_config->dst_planes[0].h = cd.height;
		ge2d_config->dst_para.canvas_index=(output_canvas>>16)&0xff;
		ge2d_config->dst_para.format=GE2D_FORMAT_S8_CR|GE2D_LITTLE_ENDIAN;
		ge2d_config->dst_para.width = output->width/2;
		ge2d_config->dst_para.height = output->height/2;
		ge2d_config->dst_xy_swap = 0;

		if(current_mirror==1){
			ge2d_config->dst_para.x_rev = 1;
			ge2d_config->dst_para.y_rev = 0;
		}else if(current_mirror==2){
			ge2d_config->dst_para.x_rev = 0;
			ge2d_config->dst_para.y_rev = 1;
		}else{
			ge2d_config->dst_para.x_rev = 0;
			ge2d_config->dst_para.y_rev = 0;
		}

		if(cur_angle==90){
			ge2d_config->dst_xy_swap = 1;
			ge2d_config->dst_para.x_rev ^= 1;
		}else if(cur_angle==180){
			ge2d_config->dst_para.x_rev ^= 1;
			ge2d_config->dst_para.y_rev ^= 1;
		}else if(cur_angle==270){
			ge2d_config->dst_xy_swap = 1;
			ge2d_config->dst_para.y_rev ^= 1;
		}

		if(ge2d_context_config_ex(context,ge2d_config)<0) {
			printk("++ge2d configing error.\n");
			return -1;
		}
		stretchblt_noalpha(context, src_left, src_top, src_width, src_height,
			0, 0, ge2d_config->dst_para.width, ge2d_config->dst_para.height);
	}
	return output_canvas;
}

int amlvideo2_sw_post_process(int canvas , int addr)
{
	return 0;
}

static int amlvideo2_fillbuff(struct amlvideo2_fh *fh, struct amlvideo2_node_buffer *buf, vframe_t *vf)
{
	config_para_ex_t ge2d_config;
	struct amlvideo2_output output;
	struct amlvideo2_node *node = fh->node;
	void *vbuf = NULL;
	int src_canvas = -1 ;
	int magic = 0;
	int ge2d_proc = 0;
	int sw_proc = 0;

	vbuf = (void *)videobuf_to_res(&buf->vb);

	dprintk(node->vid_dev,1,"%s\n", __func__);
	if (!vbuf)
		return -1;
	
	memset(&ge2d_config,0,sizeof(config_para_ex_t));
	memset(&output,0,sizeof(struct amlvideo2_output));

	output.v4l2_format= fh->fmt->fourcc;
	output.vbuf = vbuf;
	output.width = buf->vb.width;
	output.height = buf->vb.height;
	output.canvas_id = -1;

	magic = MAGIC_RE_MEM;
	switch(magic){
		case  MAGIC_RE_MEM:
#ifdef MUTLI_NODE
			output.canvas_id =  get_amlvideo2_canvas_index(&output,(node->vid==0)?VM_RES0_CANVAS_INDEX:VM_RES1_CANVAS_INDEX);
#else
			output.canvas_id =  get_amlvideo2_canvas_index(&output,VM_RES0_CANVAS_INDEX);
#endif
			break;
		case  MAGIC_VMAL_MEM:
			//canvas_index = get_amlvideo2_canvas_index(v4l2_format,&depth);
			//sw_proc = 1;
			//break;
		case  MAGIC_DC_MEM:
		case  MAGIC_SG_MEM:
		default:
			return -1;
	}

	switch(output.v4l2_format) {
		case  V4L2_PIX_FMT_RGB565X:
		case  V4L2_PIX_FMT_YUV444:
		case  V4L2_PIX_FMT_VYUY:
		case  V4L2_PIX_FMT_BGR24:
		case  V4L2_PIX_FMT_RGB24:
		case  V4L2_PIX_FMT_YUV420:
		case  V4L2_PIX_FMT_YVU420:
		case  V4L2_PIX_FMT_NV12:
		case  V4L2_PIX_FMT_NV21:
			ge2d_proc = 1;
			break;
		default:
			break;
	}
	src_canvas = vf->canvas0Addr;

	if(ge2d_proc)
		src_canvas = amlvideo2_ge2d_pre_process(vf,node->context,&ge2d_config,&output); 

	if ((sw_proc)&&(src_canvas>0))
		amlvideo2_sw_post_process(src_canvas ,(unsigned)vbuf);

	buf->vb.state = VIDEOBUF_DONE;
	do_gettimeofday(&buf->vb.ts);
	return 0;
}

static unsigned print_ivals=0;
module_param(print_ivals, uint, 0644);
MODULE_PARM_DESC(print_ivals, "print current intervals!!");

static void amlvideo2_thread_tick(struct amlvideo2_fh *fh)
{
	struct amlvideo2_node_buffer *buf = NULL;
	struct amlvideo2_node *node = fh->node;
	struct amlvideo2_node_dmaqueue *dma_q = &node->vidq;
	unsigned diff = 0;
	vframe_t *vf = NULL;

	unsigned long flags = 0;

	dprintk(node->vid_dev, 1, "Thread tick\n");


	if(!fh->is_streamed_on){
		dprintk(node->vid_dev, 1, "dev doesn't stream on\n");
		return ;
	}

	if(!node->provide_ready){
		dprintk(node->vid_dev, 1, "provide is not ready\n");
		return ;
	}

	spin_lock_irqsave(&node->slock, flags);
	if (list_empty(&dma_q->active)||(vf_peek(node->recv.name)== NULL)) {
		dprintk(node->vid_dev, 1, "No active queue to serve\n");
		goto unlock;
	}

	buf = list_entry(dma_q->active.next,
			 struct amlvideo2_node_buffer, vb.queue);
	vf = vf_get(node->recv.name);

	do_gettimeofday( &thread_ts2);
	diff = thread_ts2.tv_sec - thread_ts1.tv_sec;
	diff = diff*1000000 + thread_ts2.tv_usec - thread_ts1.tv_usec;
	if(diff < fh->frm_save_time_us ){

	    dprintk(node->vid_dev, 1,"buf is %x, time diff is %d\n", (unsigned)buf, diff);

	    goto ret_vfm;

	}
	memcpy( &thread_ts1, &thread_ts2, sizeof( struct timeval));

	if(1 == print_ivals){
	    printk("diff=%d, ivals=%ld\n", diff, fh->frm_save_time_us);
	    print_ivals = 0;
	}
	dprintk(node->vid_dev, 1, "%s\n", __func__);
	dprintk(node->vid_dev, 1, "list entry get buf is %x\n",(unsigned)buf);

	/* Nobody is waiting on this buffer, return */
	//if (!waitqueue_active(&buf->vb.done))
	//	goto unlock;

	buf->vb.state = VIDEOBUF_ACTIVE;
	list_del(&buf->vb.queue);

	/* Fill buffer */
	spin_unlock_irqrestore(&node->slock, flags);
	amlvideo2_fillbuff(fh, buf,vf);

	vf_put(vf,node->recv.name);
	dprintk(node->vid_dev, 1, "filled buffer %p\n", buf);

	if (waitqueue_active(&buf->vb.done)){
		wake_up(&buf->vb.done);
	}
	dprintk(node->vid_dev, 2, "[%p/%d] wakeup\n", buf, buf->vb.i);
	return;

ret_vfm:
	vf_put(vf,node->recv.name);
unlock:
	spin_unlock_irqrestore(&node->slock, flags);
	return;
}

static void amlvideo2_sleep(struct amlvideo2_fh *fh)
{
	struct amlvideo2_node *node = fh->node;
	struct amlvideo2_node_dmaqueue *dma_q = &node->vidq;

	DECLARE_WAITQUEUE(wait, current);

	dprintk(node->vid_dev, 1, "%s dma_q=0x%08lx\n", __func__,
		(unsigned long)dma_q);

	add_wait_queue(&dma_q->wq, &wait);
	if (kthread_should_stop())
		goto stop_task;

	/* Calculate time to wake up */
	//timeout = msecs_to_jiffies(frames_to_ms(1));

	amlvideo2_thread_tick(fh);

	schedule_timeout_interruptible(2);

stop_task:
	remove_wait_queue(&dma_q->wq, &wait);
	try_to_freeze();
}

static int amlvideo2_thread(void *data)
{
	struct amlvideo2_fh  *fh = data;
	struct amlvideo2_node *node = fh->node;

	dprintk(node->vid_dev, 1, "thread started\n");

	set_freezable();

	for (;;) {
		amlvideo2_sleep(fh);

		if (kthread_should_stop())
			break;
	}
	dprintk(node->vid_dev, 1, "thread: exit\n");
	return 0;
}

static int amlvideo2_start_thread(struct amlvideo2_fh *fh)
{
	struct amlvideo2_node *node = fh->node;
	struct amlvideo2_node_dmaqueue *dma_q = &node->vidq;

	dprintk(node->vid_dev, 1, "%s\n", __func__);

#ifdef MUTLI_NODE
	dma_q->kthread = kthread_run(amlvideo2_thread, fh, (node->vid==0)?"amvideo2-0":"amvideo2-1");
#else
	dma_q->kthread = kthread_run(amlvideo2_thread, fh, "amvideo2");
#endif

	if (IS_ERR(dma_q->kthread)) {
		v4l2_err(&node->vid_dev->v4l2_dev, "kernel_thread() failed\n");
		return PTR_ERR(dma_q->kthread);
	}
	/* Wakes thread */
	wake_up_interruptible(&dma_q->wq);

	dprintk(node->vid_dev, 1, "returning from %s\n", __func__);
	return 0;
}

static void amlvideo2_stop_thread(struct amlvideo2_node_dmaqueue  *dma_q)
{
	struct amlvideo2_node *node = container_of(dma_q, struct amlvideo2_node, vidq);

	dprintk(node->vid_dev, 1, "%s\n", __func__);
	/* shutdown control thread */
	if (dma_q->kthread) {
		kthread_stop(dma_q->kthread);
		dma_q->kthread = NULL;
	}
}

aml_provider_type get_provider_type(const char* name)
{
	aml_provider_type type = AML_PROVIDE_NONE;
	if(!name)
		return type;
	if(strcasecmp(name,"vdin0")){
		type = AML_PROVIDE_VDIN0;
	}else if(strcasecmp(name,"vdin1")){
		type = AML_PROVIDE_VDIN1;
	}else if(strncasecmp(name,"decode",7)){
		type = AML_PROVIDE_DECODE;
	}
	return type;	
}

static int video_receiver_event_fun(int type, void* data, void* private_data)
{
	struct amlvideo2_node  *node  = (struct amlvideo2_node  *)private_data;
	struct amlvideo2_fh * fh = (struct amlvideo2_fh *)container_of(node, struct amlvideo2_fh, node);

	switch(type) {
		case VFRAME_EVENT_PROVIDER_VFRAME_READY:
			node->provide_ready = 1;
			break;
		case VFRAME_EVENT_PROVIDER_QUREY_STATE:
			if(fh->is_streamed_on){
				return RECEIVER_ACTIVE ;		
			}else{
				return RECEIVER_INACTIVE;
			}
			break;   
		case VFRAME_EVENT_PROVIDER_START:
			break;
		case VFRAME_EVENT_PROVIDER_UNREG: 
			node->provide_ready = 0;
			break;
		case VFRAME_EVENT_PROVIDER_LIGHT_UNREG:
			break;
		case VFRAME_EVENT_PROVIDER_RESET:
			break;
		default:
			break;
	}    		
	return 0;
}

static const struct vframe_receiver_op_s video_vf_receiver =
{
    .event_cb = video_receiver_event_fun
};

/* ------------------------------------------------------------------
	Videobuf operations
   ------------------------------------------------------------------*/
static int
buffer_setup(struct videobuf_queue *vq, unsigned int *count, unsigned int *size)
{
	struct videobuf_res_privdata* res = (struct videobuf_res_privdata*)vq->priv_data;
	struct amlvideo2_fh  *fh = (struct amlvideo2_fh  *)res->priv;
	struct amlvideo2_node  *node  = fh->node;
	*size = (fh->width * fh->height * fh->fmt->depth)>>3;	
	if (0 == *count)
		*count = 32;

	while (*size * *count > vid_limit * 1024 * 1024)
		(*count)--;

	dprintk(node->vid_dev, 1, "%s, count=%d, size=%d\n", __func__,
		*count, *size);

	return 0;
}

static void free_buffer(struct videobuf_queue *vq, struct amlvideo2_node_buffer *buf)
{
	struct videobuf_res_privdata* res = (struct videobuf_res_privdata*)vq->priv_data;
	struct amlvideo2_fh  *fh = (struct amlvideo2_fh  *)res->priv;
	struct amlvideo2_node  *node  = fh->node;

	dprintk(node->vid_dev, 1, "%s, state: %i\n", __func__, buf->vb.state);
	videobuf_waiton(vq, &buf->vb, 0, 0);
	if (in_interrupt())
		BUG();
	videobuf_res_free(vq, &buf->vb);
	dprintk(node->vid_dev, 1, "free_buffer: freed\n");
	buf->vb.state = VIDEOBUF_NEEDS_INIT;
}

#define norm_maxw() 2000
#define norm_maxh() 1600

static int
buffer_prepare(struct videobuf_queue *vq, struct videobuf_buffer *vb,
						enum v4l2_field field)
{
	struct videobuf_res_privdata* res = (struct videobuf_res_privdata*)vq->priv_data;
	struct amlvideo2_fh  *fh = (struct amlvideo2_fh  *)res->priv;
	struct amlvideo2_node  *node  = fh->node;
	struct amlvideo2_node_buffer *buf = container_of(vb, struct amlvideo2_node_buffer, vb);
	int rc;
	dprintk(node->vid_dev, 1, "%s, field=%d\n", __func__, field);

	BUG_ON(NULL == fh->fmt);

	if (fh->width  < 48 || fh->width  > norm_maxw() ||
	    fh->height < 32 || fh->height > norm_maxh() )
		return -EINVAL;

	buf->vb.size = (fh->width*fh->height*fh->fmt->depth)>>3;
	if (0 != buf->vb.baddr  &&  buf->vb.bsize < buf->vb.size)
		return -EINVAL;
	/* These properties only change when queue is idle, see s_fmt */
	buf->fmt       = fh->fmt;
	buf->vb.width  = fh->width;
	buf->vb.height = fh->height;
	buf->vb.field  = field;
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
	struct amlvideo2_node_buffer    *buf  = container_of(vb, struct amlvideo2_node_buffer, vb);
	struct videobuf_res_privdata* res = (struct videobuf_res_privdata*)vq->priv_data;
	struct amlvideo2_fh  *fh = (struct amlvideo2_fh  *)res->priv;
	struct amlvideo2_node  *node  = fh->node;
	struct amlvideo2_node_dmaqueue *vidq = &node->vidq;
	dprintk(node->vid_dev, 1, "%s\n", __func__);
	buf->vb.state = VIDEOBUF_QUEUED;
	list_add_tail(&buf->vb.queue, &vidq->active);
}

static void buffer_release(struct videobuf_queue *vq,
			   struct videobuf_buffer *vb)
{
	struct amlvideo2_node_buffer   *buf  = container_of(vb, struct amlvideo2_node_buffer, vb);
	free_buffer(vq, buf);
}

static struct videobuf_queue_ops amlvideo2_qops = {
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
	struct amlvideo2_fh  *fh  = priv;
	struct amlvideo2_node *node = fh->node;

	strcpy(cap->driver, "amlvideo2");
	strcpy(cap->card, "amlvideo2");
	strlcpy(cap->bus_info, node->vid_dev->v4l2_dev.name, sizeof(cap->bus_info));
	cap->version = AMLVIDEO2_VERSION;
	cap->capabilities =	V4L2_CAP_VIDEO_CAPTURE |
				V4L2_CAP_STREAMING     |
				V4L2_CAP_READWRITE;
	return 0;
}

static int vidioc_enum_fmt_vid_cap(struct file *file, void  *priv,
					struct v4l2_fmtdesc *f)
{
	struct amlvideo2_fmt *fmt;

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
	struct amlvideo2_fh  *fh  = priv;
	f->fmt.pix.width        = fh->width;
	f->fmt.pix.height       = fh->height;
	f->fmt.pix.field        = fh->vb_vidq.field;
	f->fmt.pix.pixelformat  = fh->fmt->fourcc;
	f->fmt.pix.bytesperline = (f->fmt.pix.width * fh->fmt->depth) >> 3;
	f->fmt.pix.sizeimage =	f->fmt.pix.height * f->fmt.pix.bytesperline;
	return (0);
}

static int vidioc_try_fmt_vid_cap(struct file *file, void *priv,
			struct v4l2_format *f)
{
	struct amlvideo2_fh  *fh  = priv;
	struct amlvideo2_node *node = fh->node;
	struct amlvideo2_fmt *fmt = NULL;
	enum v4l2_field field;
	unsigned int maxw, maxh;
	
	fmt = get_format(f);
	if (!fmt) {
		dprintk(node->vid_dev, 1, "Fourcc format (0x%08x) invalid.\n", f->fmt.pix.pixelformat);
		return -EINVAL;
	}

	field = f->fmt.pix.field;

	if (field == V4L2_FIELD_ANY) {
		field = V4L2_FIELD_INTERLACED;
	} else if (V4L2_FIELD_INTERLACED != field) {
		dprintk(node->vid_dev, 1, "Field type invalid.\n");
		return -EINVAL;
	}

	maxw  = norm_maxw();
	maxh  = norm_maxh();

	f->fmt.pix.field = field;
	v4l_bound_align_image(&f->fmt.pix.width, 48, maxw, 2,
                  &f->fmt.pix.height, 32, maxh, 0, 0);
	f->fmt.pix.bytesperline =(f->fmt.pix.width * fmt->depth) >> 3;
	f->fmt.pix.sizeimage = 	f->fmt.pix.height * f->fmt.pix.bytesperline;
	return 0;
}

/*FIXME: This seems to be generic enough to be at videodev2 */
static int vidioc_s_fmt_vid_cap(struct file *file, void *priv,
					struct v4l2_format *f)
{
	int ret = 0;
	struct amlvideo2_fh  *fh  = priv;
	struct videobuf_queue *q = &fh->vb_vidq;

	ret = vidioc_try_fmt_vid_cap(file, fh, f);
	if (ret < 0)
		return ret;

	mutex_lock(&q->vb_lock);

	if (videobuf_queue_is_busy(&fh->vb_vidq)) {
		dprintk(fh->node->vid_dev, 1, "%s queue busy\n", __func__);
		ret = -EBUSY;
		goto out;
	}

	fh->fmt        = get_format(f);
	fh->width         = f->fmt.pix.width;
	fh->height        = f->fmt.pix.height;
	fh->vb_vidq.field = f->fmt.pix.field;
	fh->type          = f->type;
	ret = 0;
out:
	mutex_unlock(&q->vb_lock);
	return ret;
}

/*
 * Implement G/S_PARM.  There is a "high quality" mode we could try
 * to do someday; for now, we just do the frame rate tweak.
 * V4L2_CAP_TIMEPERFRAME need to be supported furthermore.
 */
static int vidioc_g_parm(struct file *file, void *priv,
				struct v4l2_streamparm *parms)
{
	struct v4l2_captureparm *cp = &parms->parm.capture;

	printk("vidioc_g_parm\n");
	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	memset(cp, 0, sizeof(struct v4l2_captureparm));
	cp->capability = V4L2_CAP_TIMEPERFRAME;

	cp->timeperframe = amlvideo2_frmintervals_active;
	printk("g_parm,deno=%d, numerator=%d\n",
		cp->timeperframe.denominator, cp->timeperframe.numerator );

	return 0;
}

static int vidioc_s_parm(struct file *file, void *priv,
                            struct v4l2_streamparm *parms)
{
    struct v4l2_fract timeperframe;
    struct amlvideo2_fh *fh = priv;
	struct amlvideo2_node *node   = fh->node;
    int max_ivals = 1000000/MIN_FRAMERATE;

    timeperframe = parms->parm.output.timeperframe;

	/*save the frame period. */
    if(0 == timeperframe.denominator){
        fh->frm_save_time_us = max_ivals;
        return -EINVAL;
    }
    fh->frm_save_time_us = timeperframe.numerator * 1000000
                                    / timeperframe.denominator;
    if(fh->frm_save_time_us > max_ivals){
        fh->frm_save_time_us = max_ivals;
    }

    dprintk(node->vid_dev, 1,"s_parm,type=%d\n", parms->type);

    return 0;
}


static int vidioc_reqbufs(struct file *file, void *priv,
			  struct v4l2_requestbuffers *p)
{
	struct amlvideo2_fh  *fh  = priv;

	return (videobuf_reqbufs(&fh->vb_vidq, p));
}

static int vidioc_querybuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	struct amlvideo2_fh  *fh  = priv;
	
	return (videobuf_querybuf(&fh->vb_vidq, p));
}

static int vidioc_qbuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	struct amlvideo2_fh *fh = priv;

	return (videobuf_qbuf(&fh->vb_vidq, p));
}

static int vidioc_dqbuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	struct amlvideo2_fh  *fh = priv;

	return (videobuf_dqbuf(&fh->vb_vidq, p,
				file->f_flags & O_NONBLOCK));
}
#if 0
static int vidioc_qbuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	int ret = 0;
	if(ppmgrvf){
		vf_put(ppmgrvf, RECEIVER_NAME);
		vf_notify_provider(RECEIVER_NAME, VFRAME_EVENT_RECEIVER_PUT, NULL);
	}
	index = (index == 3 ? 0 : (index + 1));
	mutex_unlock(&vfpMutex);
	return ret;
}

static int vidioc_dqbuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	int ret = 0;
	static int diff = 0;
	int a = 0;
	int b = 0;
	if(!vf_peek(RECEIVER_NAME)) {
		return -EAGAIN;
	}
	mutex_lock(&vfpMutex);
	ppmgrvf = vf_get(RECEIVER_NAME);
	timestamp_vpts_inc(DUR2PTS(ppmgrvf->duration));
	
	if (unregFlag || startFlag) {
		printk("uReg");
		if (ppmgrvf->pts != 0)
			timestamp_vpts_set(ppmgrvf->pts);
		else
			timestamp_vpts_set(timestamp_pcrscr_get());			
		startFlag = 0;
		unregFlag = 0;
		index = 0;
		diff = 0;
	} else if (diff > 3600 && diff < 450000) {
		//printk("sleep:%d", diff/90);
		msleep(diff/90);
	} else if (diff < -11520) {
		int count = (-diff)>>13;
		//printk("count:%d", count);
		while (count--) {
			if(!vf_peek(RECEIVER_NAME)) {
				printk("break");
				break;
			} else {
				if(ppmgrvf){
					vf_put(ppmgrvf, RECEIVER_NAME);
					index = (index == 3 ? 0 : (index + 1));
					vf_notify_provider(RECEIVER_NAME, VFRAME_EVENT_RECEIVER_PUT, NULL);
				}
				ppmgrvf = vf_get(RECEIVER_NAME);
				timestamp_vpts_inc(DUR2PTS(ppmgrvf->duration));
			}			
		}
	}
	if (ppmgrvf->pts != 0) {
		timestamp_vpts_set(ppmgrvf->pts);
	}
	a = timestamp_vpts_get();
	b = timestamp_pcrscr_get();
	diff = a - b;
	//printk("-a:%d,b:%d-	", a, b);
	p->index = index;	
	p->timestamp.tv_sec = 0;
	p->timestamp.tv_usec = ppmgrvf->duration;
	return  ret;
}
#endif

#ifdef CONFIG_VIDEO_V4L1_COMPAT
static int vidiocgmbuf(struct file *file, void *priv, struct video_mbuf *mbuf)
{
	struct amlvideo2_fh  *fh  = priv;

	return videobuf_cgmbuf(&fh->vb_vidq, mbuf, 8);
}
#endif

static int vidioc_streamon(struct file *file, void *priv, enum v4l2_buf_type i)
{
	int ret;
	struct amlvideo2_fh  *fh  = priv;
	vdin_parm_t para;
	if ((fh->type != V4L2_BUF_TYPE_VIDEO_CAPTURE) || (i != fh->type))
		return -EINVAL;
	ret = videobuf_streamon(&fh->vb_vidq);
	if(ret == 0){
		if(1){
			memset( &para, 0, sizeof( para ));
			para.port  = TVIN_PORT_VIU;
			para.fmt = TVIN_SIG_FMT_MAX;
			para.frame_rate = 60;//175
			para.h_active = 1920;//fh->width;
			para.v_active = 1080;//fh->height;
			para.hsync_phase = 0;//0x1
			para.vsync_phase  = 1;//0x1
			para.hs_bp = 0;
			para.vs_bp = 0;
			para.cfmt = TVIN_NV21;//TVIN_YUV422;
			para.scan_mode = TVIN_SCAN_MODE_PROGRESSIVE;	
			para.reserved = 0; //skip_num
			//start_tvin_service((fh->node->vid== 0)?0:1,&para);
			//start_tvin_service(1,&para);
			vops.start_tvin_service(1,&para);
		}
#if 0
		if((fh->node->p_type == AML_PROVIDE_VDIN0)||(fh->node->p_type == AML_PROVIDE_VDIN1)){
			memset( &para, 0, sizeof( para ));
        		//para.port  = (fh->node->p_type == AML_PROVIDE_VDIN0)?TVIN_PORT_CAMERA:TVIN_PORT_VIU;
        		para.port  = TVIN_PORT_VIU;
        		para.fmt = TVIN_SIG_FMT_MAX;
			para.frame_rate = 60;//175
			para.h_active = 1920;//fh->width;
			para.v_active = 1080;//fh->height;
			para.hsync_phase = 0;//0x1
			para.vsync_phase  = 1;//0x1
			para.hs_bp = 0;
			para.vs_bp = 0;
			para.cfmt = TVIN_YUV422;
			para.scan_mode = TVIN_SCAN_MODE_PROGRESSIVE;	
			para.reserved = 0; //skip_num
			start_tvin_service((fh->node->vid== 0)?0:1,&para);
			//start_tvin_service(1,&para);
		}else if(fh->node->p_type == AML_PROVIDE_DECODE){
			//do nothing;
			int event = 0;
			vf_notify_provider(fh->node->recv.name, event, NULL); 
		}
#endif
		fh->is_streamed_on = 1;
	}
	return ret;
}

static int vidioc_streamoff(struct file *file, void *priv, enum v4l2_buf_type i)
{
	int ret;
	struct amlvideo2_fh  *fh  = priv;

	if ((fh->type != V4L2_BUF_TYPE_VIDEO_CAPTURE) || (i != fh->type))
		return -EINVAL;
	ret = videobuf_streamoff(&fh->vb_vidq);
	if(ret == 0){
		//stop_tvin_service((fh->node->vid== 0)?0:1);
		//stop_tvin_service(1);
		vops.stop_tvin_service(1);
		fh->is_streamed_on = 0;
	}
	return ret;
}

static int vidioc_enum_framesizes(struct file *file, void *fh,struct v4l2_frmsizeenum *fsize)
{
	int ret = 0,i=0;
	struct amlvideo2_fmt *fmt = NULL;
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
		if (fsize->index >= ARRAY_SIZE(amlvideo2_prev_resolution))
			return -EINVAL;
		frmsize = &amlvideo2_prev_resolution[fsize->index];
		fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
		fsize->discrete.width = frmsize->width;
		fsize->discrete.height = frmsize->height;
	}
	else if(fmt->fourcc == V4L2_PIX_FMT_RGB24){
		if (fsize->index >= ARRAY_SIZE(amlvideo2_pic_resolution))
			return -EINVAL;
		frmsize = &amlvideo2_pic_resolution[fsize->index];
		fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
		fsize->discrete.width = frmsize->width;
		fsize->discrete.height = frmsize->height;
	}
	return ret;
}
static int vidioc_enum_frameintervals(struct file *file, void *priv,
                               struct v4l2_frmivalenum *fival)
{
    unsigned int k;

    if(fival->index > ARRAY_SIZE(amlvideo2_frmivalenum))
        return -EINVAL;

    for(k =0; k< ARRAY_SIZE(amlvideo2_frmivalenum); k++)
    {
        if( (fival->index==amlvideo2_frmivalenum[k].index)&&
                (fival->pixel_format ==amlvideo2_frmivalenum[k].pixel_format )&&
                (fival->width==amlvideo2_frmivalenum[k].width)&&
                (fival->height==amlvideo2_frmivalenum[k].height)){
            memcpy( fival, &amlvideo2_frmivalenum[k], sizeof(struct v4l2_frmivalenum));
            return 0;
        }
    }

    return -EINVAL;

}

static int vidioc_s_std(struct file *file, void *priv, v4l2_std_id *i)
{
	return 0;
}

/* --- controls ---------------------------------------------- */
static int vidioc_queryctrl(struct file *file, void *priv,
			    struct v4l2_queryctrl *qc)
{
	//int i;
	//struct amlvideo2_fh  *fh  = priv;
	//struct amlvideo2_node *node = fh->node;
#if 0
	for (i = 0; i < ARRAY_SIZE(amlvideo2_node_qctrl[0]); i++)
		if (qc->id && qc->id == amlvideo2_node_qctrl[node->vid][i].id) {
			memcpy(qc, &(amlvideo2_node_qctrl[node->vid][i]),
				sizeof(*qc));
			return (0);
		}
#endif
	return -EINVAL;
}

static int vidioc_g_ctrl(struct file *file, void *priv,
			 struct v4l2_control *ctrl)
{
	//int i;
	//struct amlvideo2_fh  *fh  = priv;
	//struct amlvideo2_node *node = fh->node;
#if 0
	for (i = 0; i < ARRAY_SIZE(amlvideo2_node_qctrl[0]); i++)
		if (ctrl->id == amlvideo2_node_qctrl[node->vid][i].id) {
			ctrl->value = node->qctl_regs[i];
			return 0;
		}
#endif
	return -EINVAL;
}

static int vidioc_s_ctrl(struct file *file, void *priv,
				struct v4l2_control *ctrl)
{
	//int i;
	//struct amlvideo2_fh  *fh  = priv;
	//struct amlvideo2_node *node = fh->node;
#if 0
	for (i = 0; i < ARRAY_SIZE(amlvideo2_node_qctrl[0]); i++)
		if (ctrl->id == amlvideo2_node_qctrl[node->vid][i].id) {
			if (ctrl->value < amlvideo2_node_qctrl[node->vid][i].minimum ||
			    ctrl->value > amlvideo2_node_qctrl[node->vid][i].maximum) {
				return -ERANGE;
			}
			node->qctl_regs[i] = ctrl->value;
			return 0;
		}
#endif
	return -EINVAL;
}

/* ------------------------------------------------------------------
	File operations for the device
   ------------------------------------------------------------------*/
static int amlvideo2_open(struct file *file)
{
	struct amlvideo2_node *node = video_drvdata(file);
	struct amlvideo2_fh *fh = NULL;
	struct videobuf_res_privdata* res = NULL;

	mutex_lock(&node->mutex);
	node->users++;
	if (node->users > 1) {
		node->users--;
		mutex_unlock(&node->mutex);
		return -EBUSY;
	}

#if 0
	node->provider = vf_get_provider((node->vid==0)?RECEIVER_NAME0:RECEIVER_NAME1);

	if (node->provider == NULL) {
		node->users--;
		mutex_unlock(&node->mutex);
		return -ENODEV;
	}
	node->p_type = get_provider_type(node->provider->name);
	if((node->p_type == AML_PROVIDE_NONE)||(node->p_type>=AML_PROVIDE_MAX)){
		node->users--;
		node->provider = NULL;
		mutex_unlock(&node->mutex);
		return -ENODEV;
	}
#endif

	fh = kzalloc(sizeof(*fh), GFP_KERNEL);
	if (NULL == fh) {
		node->users--;
		//node->provider  = NULL;
		mutex_unlock(&node->mutex);
		return -ENOMEM;
	}
	mutex_unlock(&node->mutex);
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6
	switch_mod_gate_by_name("ge2d", 1);
#endif

	file->private_data = fh;
	fh->node      = node;

	fh->type     = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fh->fmt      = &formats[0];
	fh->width    = 1920;
	fh->height   = 1080;
	fh->f_flags  = file->f_flags;
	
	fh->node->res.priv = (void*)fh;
	res = &fh->node->res;
	videobuf_queue_res_init(&fh->vb_vidq, &amlvideo2_qops,
					NULL, &node->slock, fh->type, V4L2_FIELD_INTERLACED,
					sizeof(struct amlvideo2_node_buffer), (void*)res, NULL);

	v4l2_vdin_ops_init(&vops);
	do_gettimeofday( &thread_ts1);
	fh->frm_save_time_us = 1000000/MIN_FRAMERATE;
	amlvideo2_start_thread(fh);
	return 0;
}

static ssize_t
amlvideo2_read(struct file *file, char __user *data, size_t count, loff_t *ppos)
{
	struct amlvideo2_fh *fh = file->private_data;

	if (fh->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		return videobuf_read_stream(&fh->vb_vidq, data, count, ppos, 0,
					file->f_flags & O_NONBLOCK);
	}
	return 0;
}

static unsigned int
amlvideo2_poll(struct file *file, struct poll_table_struct *wait)
{
	struct amlvideo2_fh *fh = file->private_data;
	struct amlvideo2_node *node   = fh->node;
	struct videobuf_queue *q = &fh->vb_vidq;

	dprintk(node->vid_dev, 1, "%s\n", __func__);

	if (V4L2_BUF_TYPE_VIDEO_CAPTURE != fh->type)
		return POLLERR;

	return videobuf_poll_stream(file, q, wait);
}

static int amlvideo2_close(struct file *file)
{
	struct amlvideo2_fh *fh = file->private_data;
	struct amlvideo2_node *node       = fh->node;
	amlvideo2_stop_thread(&node->vidq);
	videobuf_stop(&fh->vb_vidq);
	videobuf_mmap_free(&fh->vb_vidq);
	kfree(fh);
	mutex_lock(&node->mutex);
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6
	switch_mod_gate_by_name("ge2d", 0);
#endif	
	node->users--;
	amlvideo2_frmintervals_active.numerator = 1;
	amlvideo2_frmintervals_active.denominator = 30;
	//node->provider = NULL;
	mutex_unlock(&node->mutex);
	return 0;
}

static int amlvideo2_mmap(struct file *file, struct vm_area_struct *vma)
{
	int ret = 0;
	struct amlvideo2_fh *fh = file->private_data;
	struct amlvideo2_node *node = fh->node;

	dprintk(node->vid_dev, 1, "mmap called, vma=0x%08lx\n", (unsigned long)vma);

	ret = videobuf_mmap_mapper(&fh->vb_vidq, vma);

	dprintk(node->vid_dev, 1, "vma start=0x%08lx, size=%ld, ret=%d\n",
		(unsigned long)vma->vm_start,
		(unsigned long)vma->vm_end-(unsigned long)vma->vm_start,
		ret);

	return ret;
}

static const struct v4l2_file_operations amlvideo2_fops = {
	.owner		= THIS_MODULE,
	.open           = amlvideo2_open,
	.release        = amlvideo2_close,
	.read           = amlvideo2_read,
	.poll		= amlvideo2_poll,
	.ioctl          = video_ioctl2, /* V4L2 ioctl handler */
	.mmap           = amlvideo2_mmap,
};

static const struct v4l2_ioctl_ops amlvideo2_ioctl_ops = {
	.vidioc_querycap      = vidioc_querycap,
	.vidioc_enum_fmt_vid_cap  = vidioc_enum_fmt_vid_cap,
	.vidioc_g_fmt_vid_cap     = vidioc_g_fmt_vid_cap,
	.vidioc_try_fmt_vid_cap   = vidioc_try_fmt_vid_cap,
	.vidioc_s_fmt_vid_cap     = vidioc_s_fmt_vid_cap,
	.vidioc_g_parm 		  = vidioc_g_parm,
	.vidioc_s_parm 		  = vidioc_s_parm,
	.vidioc_reqbufs       = vidioc_reqbufs,
	.vidioc_querybuf      = vidioc_querybuf,
	.vidioc_qbuf          = vidioc_qbuf,
	.vidioc_dqbuf         = vidioc_dqbuf,
	.vidioc_s_std         = vidioc_s_std,
	.vidioc_queryctrl     = vidioc_queryctrl,
	.vidioc_g_ctrl        = vidioc_g_ctrl,
	.vidioc_s_ctrl        = vidioc_s_ctrl,
	.vidioc_streamon      = vidioc_streamon,
	.vidioc_streamoff     = vidioc_streamoff,
	.vidioc_enum_framesizes = vidioc_enum_framesizes,
    .vidioc_enum_frameintervals =vidioc_enum_frameintervals,
#ifdef CONFIG_VIDEO_V4L1_COMPAT
	.vidiocgmbuf          = vidiocgmbuf,
#endif
};

static struct video_device amlvideo2_template = {
	.name		= "amlvideo2",
	.fops           = &amlvideo2_fops,
	.ioctl_ops 	= &amlvideo2_ioctl_ops,
	.release	= video_device_release,

	.tvnorms              = V4L2_STD_525_60,
	.current_norm         = V4L2_STD_NTSC_M,
};

/* -----------------------------------------------------------------
	Initialization and module stuff
   ------------------------------------------------------------------*/

static int amlvideo2_release_node(struct amlvideo2_device *vid_dev)
{
	int i = 0;
	struct video_device *vfd = NULL;

	for(i = 0; i<vid_dev->node_num;i++){
		if(vid_dev->node[i]){
			vfd = vid_dev->node[i]->vfd;
			video_device_release(vfd);
			vf_unreg_receiver(&vid_dev->node[i]->recv);
			if(vid_dev->node[i]->context)
				destroy_ge2d_work_queue(vid_dev->node[i]->context);
			kfree(vid_dev->node[i]);
			vid_dev->node[i] = NULL;
		}
	}

	return 0;
}

static int __init amlvideo2_create_node(struct platform_device *pdev)
{
	int ret = 0, i = 0;
	struct video_device *vfd = NULL;
	struct amlvideo2_node* vid_node = NULL;
	struct resource *res = NULL;
	struct v4l2_device *v4l2_dev = platform_get_drvdata(pdev);
	struct amlvideo2_device *vid_dev = container_of(v4l2_dev,
			struct amlvideo2_device, v4l2_dev);

	vid_dev->node_num = pdev->num_resources;
	if(vid_dev->node_num>MAX_SUB_DEV_NODE)
		vid_dev->node_num = MAX_SUB_DEV_NODE;

	for(i = 0; i<vid_dev->node_num;i++){
		vid_dev->node[i] = NULL;
		ret = -ENOMEM;		
		res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if(!res)
			break;

		vid_node = kzalloc(sizeof(struct amlvideo2_node), GFP_KERNEL);
		if(!vid_node)
			break;

		vid_node->res.start = res->start;
		vid_node->res.end =res->end;
		vid_node->res.magic = MAGIC_RE_MEM;
		vid_node->res.priv = NULL;		
		vid_node->context = create_ge2d_work_queue();
		if(!vid_node->context){
			kfree(vid_node);
			break;
		}
		/* init video dma queues */
		INIT_LIST_HEAD(&vid_node->vidq.active);
		init_waitqueue_head(&vid_node->vidq.wq);

		/* initialize locks */
		spin_lock_init(&vid_node->slock);
		mutex_init(&vid_node->mutex);


		vfd = video_device_alloc();
		if (!vfd){
			destroy_ge2d_work_queue(vid_node->context);
			kfree(vid_node);
			break;
		}
		*vfd = amlvideo2_template;
		vfd->debug = debug;
		ret = video_register_device(vfd, VFL_TYPE_GRABBER, video_nr);
		if (ret < 0){
			ret = -ENODEV;
			video_device_release(vfd);
			destroy_ge2d_work_queue(vid_node->context);
			kfree(vid_node);
			break;
		}

		video_set_drvdata(vfd, vid_node);

		/* Set all controls to their default value. */
		//for (j = 0; j < ARRAY_SIZE(amlvideo2_node_qctrl[0]); j++)
		//	vid_node->qctl_regs[i] = amlvideo2_node_qctrl[i][j].default_value;

		vid_node->vfd = vfd;
		vid_node->vid = i;
		vid_node->users = 0;
		vid_node->vid_dev = vid_dev;
		video_nr++;
#ifdef MUTLI_NODE
		vf_receiver_init(&vid_node->recv, (i==0)?RECEIVER_NAME0:RECEIVER_NAME1, &video_vf_receiver, (void*)vid_node);
#else
		vf_receiver_init(&vid_node->recv, RECEIVER_NAME, &video_vf_receiver, (void*)vid_node);
#endif
       	vf_reg_receiver(&vid_node->recv);
		vid_dev->node[i] = vid_node;
		v4l2_info(&vid_dev->v4l2_dev, "V4L2 device registered as %s\n",
		  	video_device_node_name(vfd));
		ret = 0;
	}

	if(ret)
		amlvideo2_release_node(vid_dev);
	
	return ret;
}

static int __init amlvideo2_driver_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct amlvideo2_device *dev = NULL;
	if (pdev->num_resources == 0) {
		dev_err(&pdev->dev, "probed for an unknown device\n");
		return -ENODEV;
	}

	dev = kzalloc(sizeof(struct amlvideo2_device), GFP_KERNEL);
	if (dev == NULL)
		return -ENOMEM;

	memset(dev,0,sizeof(struct amlvideo2_device));
	snprintf(dev->v4l2_dev.name, sizeof(dev->v4l2_dev.name),
			"%s", AVMLVIDEO2_MODULE_NAME);

	if (v4l2_device_register(&pdev->dev, &dev->v4l2_dev) < 0) {
		dev_err(&pdev->dev, "v4l2_device_register failed\n");
		ret = -ENODEV;
		goto probe_err0;
	}

	mutex_init(&dev->mutex);
	video_nr = 11;

	ret = amlvideo2_create_node(pdev);
	if (ret)
		goto probe_err1;

	return 0;

probe_err1:
	v4l2_device_unregister(&dev->v4l2_dev);

probe_err0:
	kfree(dev);
	return ret;
}

static int amlvideo2_drv_remove(struct platform_device *pdev)
{
	struct v4l2_device *v4l2_dev = platform_get_drvdata(pdev);
	struct amlvideo2_device *vid_dev = container_of(v4l2_dev, struct
			amlvideo2_device, v4l2_dev);

	amlvideo2_release_node(vid_dev);
	v4l2_device_unregister(v4l2_dev);
	kfree(vid_dev);
	return 0;
}


/* general interface for a linux driver .*/
static struct platform_driver amlvideo2_drv = {
	.probe  = amlvideo2_driver_probe,
	.remove = amlvideo2_drv_remove,
	.driver = {
		.name = "amlvideo2",
		.owner = THIS_MODULE,
	}
};


static int __init amlvideo2_init(void)
{
	int err;

	//amlog_level(LOG_LEVEL_HIGH,"amlvideo2_init\n");
	if ((err = platform_driver_register(&amlvideo2_drv))) {
		printk(KERN_ERR "Failed to register amlvideo2 driver (error=%d\n", err);
		return err;
	}

	return err;
}

static void __exit amlvideo2_exit(void)
{
	platform_driver_unregister(&amlvideo2_drv);
	//amlog_level(LOG_LEVEL_HIGH,"amlvideo2 module removed.\n");
}

module_init(amlvideo2_init);
module_exit(amlvideo2_exit);
