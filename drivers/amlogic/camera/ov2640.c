/*
 *ov2640 - This code emulates a real video device with v4l2 api
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

#ifdef CONFIG_ARCH_MESON6
#include <mach/mod_gate.h>
#endif

#define OV2640_CAMERA_MODULE_NAME "ov2640"

/* Wake up at about 30 fps */
#define WAKE_NUMERATOR 30
#define WAKE_DENOMINATOR 1001
#define BUFFER_TIMEOUT     msecs_to_jiffies(500)  /* 0.5 seconds */

#define OV2640_CAMERA_MAJOR_VERSION 0
#define OV2640_CAMERA_MINOR_VERSION 7
#define OV2640_CAMERA_RELEASE 0
#define OV2640_CAMERA_VERSION \
	KERNEL_VERSION(OV2640_CAMERA_MAJOR_VERSION, OV2640_CAMERA_MINOR_VERSION, OV2640_CAMERA_RELEASE)

MODULE_DESCRIPTION("ov2640 On Board");
MODULE_AUTHOR("amlogic-sh");
MODULE_LICENSE("GPL v2");

static unsigned video_nr = -1;  /* videoX start number, -1 is autodetect. */

static unsigned debug;
//module_param(debug, uint, 0644);
//MODULE_PARM_DESC(debug, "activates debug info");

static unsigned int vid_limit = 16;
static unsigned int ov2640_have_opened = 0;
static struct i2c_client *this_client;


typedef enum resulution_size_type{
	SIZE_NULL = 0,
	SIZE_QVGA_320x240,
	SIZE_VGA_640X480,
	SIZE_UXGA_1600X1200,
	SIZE_QXGA_2048X1536,
	SIZE_QSXGA_2560X2048,
} resulution_size_type_t;

typedef struct resolution_param {
	struct v4l2_frmsize_discrete frmsize;
	struct v4l2_frmsize_discrete active_frmsize;
	int active_fps;
	resulution_size_type_t size_type;
	struct aml_camera_i2c_fig1_s* reg_script;
} resolution_param_t;



//module_param(vid_limit, uint, 0644);
//MODULE_PARM_DESC(vid_limit, "capture memory limit in megabytes");


/* supported controls */
static struct v4l2_queryctrl ov2640_qctrl[] = {
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

struct ov2640_fmt {
	char  *name;
	u32   fourcc;          /* v4l2 format id */
	int   depth;
};

static struct ov2640_fmt formats[] = {
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

static struct ov2640_fmt *get_format(struct v4l2_format *f)
{
	struct ov2640_fmt *fmt;
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
struct ov2640_buffer {
	/* common v4l buffer stuff -- must be first */
	struct videobuf_buffer vb;

	struct ov2640_fmt        *fmt;
};

struct ov2640_dmaqueue {
	struct list_head       active;

	/* thread for generating video stream*/
	struct task_struct         *kthread;
	wait_queue_head_t          wq;
	/* Counters to control fps rate */
	int                        frame;
	int                        ini_jiffies;
};

static LIST_HEAD(ov2640_devicelist);

struct ov2640_device {
	struct list_head			ov2640_devicelist;
	struct v4l2_subdev			sd;
	struct v4l2_device			v4l2_dev;

	spinlock_t                 slock;
	struct mutex				mutex;

	int                        users;

	/* various device info */
	struct video_device        *vdev;

	struct ov2640_dmaqueue       vidq;

	/* Several counters */
	unsigned long              jiffies;

	/* Input Number */
	int			   input;

	/* platform device data from board initting. */
	aml_plat_cam_data_t platform_dev_data;
	
	/* current resolution param for preview and capture */
	resolution_param_t* cur_resolution_param;
	
	/* wake lock */
	struct wake_lock	wake_lock;

	/* Control 'registers' */
	int 			   qctl_regs[ARRAY_SIZE(ov2640_qctrl)];
};

static inline struct ov2640_device *to_dev(struct v4l2_subdev *sd)
{
	return container_of(sd, struct ov2640_device, sd);
}

struct ov2640_fh {
	struct ov2640_device            *dev;

	/* video capture */
	struct ov2640_fmt            *fmt;
	unsigned int               width, height;
	struct videobuf_queue      vb_vidq;

	enum v4l2_buf_type         type;
	int			   input; 	/* Input Number on bars */
	int  stream_on;
};

static inline struct ov2640_fh *to_fh(struct ov2640_device *dev)
{
	return container_of(dev, struct ov2640_fh, dev);
}

/* ------------------------------------------------------------------
	reg spec of OV2640
   ------------------------------------------------------------------*/

#if 1

struct aml_camera_i2c_fig1_s OV2640_script[] = {
#if 0
	{0xff,0x00}, 
	{0x2c,0xff}, 
	{0x2e,0xdf}, 
	{0xff,0x01}, 
	{0x3c,0x32}, 
	{0x11,0x00}, 
	{0x09,0x03}, 
	{0x04,0xf8}, 
	{0x13,0xf5}, 
	{0x14,0x48}, 
	{0x2c,0x0c}, 
	{0x33,0x78}, 
	{0x3a,0x33}, 
	{0x3b,0xfB}, 
	{0x3e,0x00}, 
	{0x43,0x11}, 
	{0x16,0x10}, 
	{0x39,0x02}, 
	{0x35,0x88}, 
	{0x22,0x09}, 
	{0x37,0x40}, 
	{0x23,0x00}, 
	{0x34,0xa0}, 
	{0x36,0x1a}, 
	{0x06,0x02}, 
	{0x07,0xc0}, 
	{0x0d,0xb7}, 
	{0x0e,0x01}, 
	{0x4c,0x00}, 
	{0x4a,0x81}, 
	{0x21,0x99}, 
	{0x24,0x40}, 
	{0x25,0x38}, 
	{0x26,0x82}, 
	{0x5c,0x00}, 
	{0x63,0x00}, 
	{0x46,0x00}, 
	{0x0c,0x3c}, 
	{0x61,0x70}, 
	{0x62,0x80}, 
	{0x7c,0x05},	
	{0x20,0x80}, 
	{0x28,0x30}, 
	{0x6c,0x00}, 
	{0x6d,0x80}, 
	{0x6e,0x00}, 
	{0x70,0x02}, 
	{0x71,0x94}, 
	{0x73,0xc1}, 
	{0x3d,0x38}, 
	{0x5a,0x9b}, 
	{0x4f,0x7c}, 
	{0x50,0x67}, 
	{0xff,0x00}, 
	{0xe5,0x7f}, 
	{0xf9,0xc0}, 
	{0x41,0x24}, 
	{0xe0,0x14}, 
	{0x76,0xff}, 
	{0x33,0xa0}, 
	{0x42,0x20}, 
	{0x43,0x18}, 
	{0x4c,0x00}, 
	{0x87,0xd0}, 
	{0x88,0x3f}, 
	{0xd7,0x03}, 
	{0xd9,0x10}, 
	{0xd3,0x82}, 
	{0xc8,0x08}, 
	{0xc9,0x80}, 
	{0x7c,0x00}, 
	{0x7d,0x00}, 
	{0x7c,0x03}, 
	{0x7d,0x48}, 
	{0x7d,0x48}, 
	{0x7c,0x08}, 
	{0x7d,0x20}, 
	{0x7d,0x10}, 
	{0x7d,0x0e}, 
	{0x90,0x00}, 
	{0x91,0x0e}, 
	{0x91,0x1a}, 
	{0x91,0x31}, 
	{0x91,0x5a}, 
	{0x91,0x69}, 
	{0x91,0x75}, 
	{0x91,0x7e}, 
	{0x91,0x88}, 
	{0x91,0x8f}, 
	{0x91,0x96}, 
	{0x91,0xa3}, 
	{0x91,0xaf}, 
	{0x91,0xc4}, 
	{0x91,0xd7}, 
	{0x91,0xe8}, 
	{0x91,0x20}, 
	{0x92,0x00}, 
	{0x93,0x06}, 
	{0x93,0xe3}, 
	{0x93,0x05}, 
	{0x93,0x05}, 
	{0x93,0x00}, 
	{0x93,0x04}, 
	{0x93,0x00}, 
	{0x93,0x00}, 
	{0x93,0x00}, 
	{0x93,0x00}, 
	{0x93,0x00}, 
	{0x93,0x00}, 
	{0x93,0x00}, 
	{0x96,0x00}, 
	{0x97,0x08}, 
	{0x97,0x19}, 
	{0x97,0x02}, 
	{0x97,0x0c}, 
	{0x97,0x24}, 
	{0x97,0x30}, 
	{0x97,0x28}, 
	{0x97,0x26}, 
	{0x97,0x02}, 
	{0x97,0x98}, 
	{0x97,0x80}, 
	{0x97,0x00}, 
	{0x97,0x00}, 
	{0xc3,0xed}, 
	{0xa4,0x00}, 
	{0xa8,0x00}, 
	{0xc5,0x11}, 
	{0xc6,0x51}, 
	{0xbf,0x80}, 
	{0xc7,0x10}, 
	{0xb6,0x66}, 
	{0xb8,0xA5}, 
	{0xb7,0x64}, 
	{0xb9,0x7C}, 
	{0xb3,0xaf}, 
	{0xb4,0x97}, 
	{0xb5,0xFF}, 
	{0xb0,0xC5}, 
	{0xb1,0x94}, 
	{0xb2,0x0f}, 
	{0xc4,0x5c}, 
	{0xe0,0x04}, 
	{0xc0,0xc8}, 
	{0xc1,0x96}, 
	{0x86,0x3d}, 
	{0x50,0x89}, 
	{0x51,0x90}, 
	{0x52,0x2c}, 
	{0x53,0x00}, 
	{0x54,0x00}, 
	{0x55,0x88}, 
	{0x57,0x00}, 
	{0x5a,0xa0}, 
	{0x5b,0x78}, 
	{0x5c,0x00}, 
	{0xe0,0x00}, 
	{0xc3,0xed}, 
	{0x7f,0x00}, 
	{0xda,0x00}, 
	{0xe5,0x1f}, 
	{0xe1,0x67}, 
	{0xe0,0x00}, 
	{0xdd,0x7f}, 
	{0x05,0x00}, 
#else
	{0xff,0x01},
	{0x12,0x80},
	{0xff,0x00},
	{0x2c,0xff},
	{0x2e,0xdf},
	{0xff,0x01},
	
	{0x03,0x4f},// 0x8f peak
	{0x0f,0x4b},
	
	
	{0x3c,0x32},
	{0x11,0x00},
	{0x09,0x03},
	{0x04,0xa8},//b7,b6 directs
	{0x13,0xe5},
	{0x14,0x28}, //0x48 peak
	{0x2c,0x0c},
	{0x33,0x78},
	{0x3a,0x33},
	{0x3b,0xfB},
	{0x3e,0x00},
	{0x43,0x11},
	{0x16,0x10},
	{0x39,0x02},
	{0x35,0x88},
	{0x22,0x09},
	{0x37,0x40},
	{0x23,0x00},
	{0x34,0xa0},
	{0x36,0x1a},
	{0x06,0x02},
	{0x07,0xc0},
	{0x0d,0xb7},
	{0x0e,0x01},
	{0x4c,0x00},
	{0x4a,0x81},
	{0x21,0x99},
	//aec
	//{{0x24},{0x58}},
	//{{0x25},{0x50}},
	//{{0x26},{0x92}},
	
	//{{0x24, 0x70}},
	//{{0x25, 0x60}},
	//{{0x26, 0xa4}},    
	{0x24,0x48},
	{0x25,0x38},//40
	{0x26,0x82},//82 
	
	{0x5c,0x00},
	{0x63,0x00},
	{0x46,0x3f},
	{0x0c,0x3c},
	{0x61,0x70},
	{0x62,0x80},
	{0x7c,0x05},
	{0x20,0x80},
	{0x28,0x30},
	{0x6c,0x00},
	{0x6d,0x80},
	{0x6e,0x00},
	{0x70,0x02},
	{0x71,0x94},
	{0x73,0xc1},
	{0x3d,0x34},
	{0x5a,0x57},
	{0x4f,0xbb},
	{0x50,0x9c},
	{0xff,0x00},
	{0xe5,0x7f},
	{0xf9,0xc0},
	{0x41,0x24},
	{0xe0,0x14},
	{0x76,0xff},
	{0x33,0xa0},
	{0x42,0x20},
	{0x43,0x18},
	{0x4c,0x00},
	{0x87,0xd0},
	{0x88,0x3f},
	{0xd7,0x03},
	{0xd9,0x10},
	{0xd3,0x82},
	{0xc8,0x08},
	{0xc9,0x80},
	//
	//{{0xff},{0x00}}, //added by peak on 20120409
	{0x7c,0x00},
	{0x7d,0x02},//0x00 peak//0x07,è·±￡?ú±eμ????tà???±??2??
	{0x7c,0x03},
	{0x7d,0x28},//0x48//0x40 ?a???μò??-oüD?á?,3y·????ú±eμ????tà?ó?D′á?
	{0x7d,0x28},//0x48 peak//0x40 ?a???μò??-oüD?á?,3y·????ú±eμ????tà?ó?D′á?
	
	// removed by peak on 20120409
	
	{0x7c,0x08},  
	{0x7d,0x20},
	{0x7d,0x10},//0x10
	{0x7d,0x0e},//0x0e
	
	//contrast added by peak on 20120409
	// {{0x7c},{0x00}},
	//{{0x7d},{0x04}},//0x48//0x40
	//{{0x7c},{0x07}},//0x48 peak//0x40
	//{{0x7d},{0x20}},
	//{{0x7d},{0x28}},
	//{{0x7d},{0x0c}},//0x10
	//{{0x7d},{0x06}},//0x0e
	
	//{{0xff, 0x01}},// added by peak on 20120409
	
	{0x90,0x00},
	{0x91,0x0e},
	{0x91,0x1a},//e3
	{0x91,0x31},
	{0x91,0x5a},
	{0x91,0x69},
	{0x91,0x75},
	{0x91,0x7e},
	{0x91,0x88},
	{0x91,0x8f},
	{0x91,0x96},
	{0x91,0xa3},
	{0x91,0xaf},
	{0x91,0xc4},
	{0x91,0xd7},
	{0x91,0xe8},
	{0x91,0x20},
	
	{0x90,0x00}, 
	{0x91,0x04}, 
	{0x91,0x0c}, 
	{0x91,0x20}, 
	{0x91,0x4c}, 
	{0x91,0x60}, 
	{0x91,0x74}, 
	{0x91,0x82}, 
	{0x91,0x8e}, 
	{0x91,0x97}, 
	{0x91,0x9f}, 
	{0x91,0xaa}, 
	{0x91,0xb4}, 
	{0x91,0xc8}, 
	{0x91,0xd9}, 
	{0x91,0xe8}, 
	{0x91,0x20}, 
	
	
	
	{0x92,0x00},
	{0x93,0x06},
	{0x93,0xc8},//e3
	{0x93,0x05},
	{0x93,0x05},
	{0x93,0x00},
	{0x93,0x04},
	{0x93,0x00},
	{0x93,0x00},
	{0x93,0x00},
	{0x93,0x00},
	{0x93,0x00},
	{0x93,0x00},
	{0x93,0x00},
	
	{0x96,0x00},
	{0x97,0x08},
	{0x97,0x19},
	{0x97,0x02},
	{0x97,0x0c},
	{0x97,0x24},
	{0x97,0x30},
	{0x97,0x28},
	{0x97,0x26},
	{0x97,0x02},
	{0x97,0x98},
	{0x97,0x80},
	{0x97,0x00},
	{0x97,0x00},
	{0xc3,0xef},//ed
	{0xa4,0x00},
	{0xa8,0x00},
	
	{0xbf,0x00},
	{0xba,0xdc},
	{0xbb,0x08},
	{0xb6,0x20},
	{0xb8,0x30},
	{0xb7,0x20},
	{0xb9,0x30},
	{0xb3,0xb4},
	{0xb4,0xca},
	{0xb5,0x34},
	{0xb0,0x46},
	{0xb1,0x46},
	{0xb2,0x06},
	{0xc7,0x00},
	{0xc6,0x51},
	{0xc5,0x11},
	{0xc4,0x9c},
	///  
	{0xc0,0xc8},
	{0xc1,0x96},
	{0x86,0x3d},
	{0x50,0x92},
	{0x51,0x90},
	{0x52,0x2c},
	{0x53,0x00},
	{0x54,0x00},
	{0x55,0x88},
	{0x57,0x00},
	{0x5a,0x50},
	{0x5b,0x3c},
	{0x5c,0x00},
	{0xc3,0xed},
	{0x7f,0x00},
	{0xda,0x00},
	{0xe5,0x1f},
	{0xe1,0x67},
	{0xe0,0x00},
	{0xdd,0x7f},
	{0x05,0x00},
	
	
#if 1
	{0xff,0x01},
	{0x5d,0x55},//0x00
	//{{0x5e, 0x7d}},//0x3c
	//{{0x5f, 0x7d}},//0x28
	//{{0x60, 0x55}},//0x55
	{0x5e,0x55},//0x3c
	{0x5f,0x55},//0x28
	{0x60,0x55},//0x55
	   
	{0xff,0x00},
	{0xc3,0xef},
	{0xa6,0x00},
	{0xa7,0x0f},
	{0xa7,0x4e},
	{0xa7,0x7a},
	{0xa7,0x33},
	{0xa7,0x00},
	{0xa7,0x23},
	{0xa7,0x27},
	{0xa7,0x3a},
	{0xa7,0x70},
	{0xa7,0x33},
	{0xa7,0x00},//L
	{0xa7,0x23},
	{0xa7,0x20},
	{0xa7,0x0c},
	{0xa7,0x66},
	{0xa7,0x33},
	{0xa7,0x00},
	{0xa7,0x23},
	{0xc3,0xef},
#endif
	
	
#if 1
	{0xff,0x00},
	{0x92,0x00},
	{0x93,0x06}, //0x06 peak
	{0x93,0xe3},//e
	{0x93,0x05},
	{0x93,0x03},
	{0x93,0x00},
	{0x93,0x04},
#endif
	
	//{{0x03, 0x0f}},
	
	{0xe0, 0x04},
	{0xc0, 0xc8},
	{0xc1, 0x96},
	{0x86, 0x3d},
	{0x50, 0x89},
	{0x51, 0x90},
	{0x52, 0x2c},
	{0x53, 0x00},
	{0x54, 0x00},
	{0x55, 0x88},
	{0x57, 0x00},
	{0x5a, 0xa0},
	{0x5b, 0x78},
	{0x5c, 0x00},
	{0xd3, 0x82},
	{0xe0, 0x00},
#endif

	{0xff, 0xff},
};

struct aml_camera_i2c_fig1_s OV2640_preview_script[] = {
#if 0
	{0xff,0x00},    
	{0xe0,0x04},
	{0xc0,0xc8},    
	{0xc1,0x96}, 
	{0x86,0x3d},   
	{0x50,0x89},  
	{0x51,0x90},  
	{0x52,0x2c},
	{0x53,0x00},  
	{0x54,0x00},
	{0x55,0x88},    
	{0x57,0x00},   
	{0x5a,0xa0},  
	{0x5b,0x78},  
	{0x5c,0x00},    
	{0xd3,0x82},    
	{0xe0,0x00},
#else
	{0xe0, 0x04},
	{0xc0, 0xc8},
	{0xc1, 0x96},
	{0x86, 0x3d},
	{0x50, 0x89},
	{0x51, 0x8c},//90
	{0x52, 0x29},//2c
	{0x53, 0x0c},//00
	{0x54, 0x0c},//00
	{0x55, 0x88},
	{0x57, 0x00},
	{0x5a, 0xa0},
	{0x5b, 0x78},
	{0x5c, 0x00},
	{0xd3, 0x82},
	{0xe0, 0x00},
#endif
	
	{0xff, 0xff},
};

struct aml_camera_i2c_fig1_s OV2640_preview_qvga_script[] = {
#if 0
	{0xff,0x00},    
	{0xe0,0x04},
	{0xc0,0xc8},    
	{0xc1,0x96}, 
	{0x86,0x3d},   
	{0x50,0x89},  
	{0x51,0x90},  
	{0x52,0x2c},
	{0x53,0x00},  
	{0x54,0x00},
	{0x55,0x88},    
	{0x57,0x00},   
	{0x5a,0xa0},  
	{0x5b,0x78},  
	{0x5c,0x00},    
	{0xd3,0x82},    
	{0xe0,0x00},
#else
	{0xe0, 0x04},
	{0xc0, 0xc8},
	{0xc1, 0x96},
	{0x86, 0x3d},
	{0x50, 0x89},
	{0x51, 0x90},
	{0x52, 0x2c},
	{0x53, 0x00},
	{0x54, 0x00},
	{0x55, 0x88},
	{0x57, 0x00},
	{0x5a, 0x50},
	{0x5b, 0x3c},
	{0x5c, 0x00},
	{0xd3, 0x82},
	{0xe0, 0x00},
#endif
	
	{0xff, 0xff},
};

struct aml_camera_i2c_fig1_s OV2640_capture_script[] = {
#if 0
	{0xff,0x01},
	{0x12,0x00},
	{0x11,0x00},
	{0x3d,0x38},
	{0x17,0x11},
	{0x18,0x75},
	{0x19,0x01},
	{0x1a,0x97},
	{0x32,0x36},
	{0x37,0x40},
	{0x4f,0x7c},
	{0x50,0x67},
	{0x5a,0x9b},
	{0x6d,0x80},
	{0x3d,0x38},
	{0x39,0x02},
	{0x35,0x88},
	{0x22,0x0a},
	{0x37,0x40},
	{0x23,0x00},
	{0x34,0xa0},
	{0x36,0x1a},
	{0x06,0x02},
	{0x07,0xc0},
	{0x0d,0xb7},
	{0x0e,0x01}, 
	{0x4c,0x00},
	{0xff,0x00},
	{0xe0,0x04},
	{0xc0,0xc8},
	{0xc1,0x96},
	{0x86,0x3d},
	{0x50,0x89},
	{0x51,0x90},
	{0x52,0x2c},
	{0x53,0x00},
	{0x54,0x00},
	{0x55,0x88},
	{0x57,0x00},
	{0x5a,0xa0},
	{0x5b,0x78},
	{0x5c,0x00},
	{0xd3,0x82},
	{0xe0,0x00},
#else
	{0xff,0x00},
	{0xe0,0x04},
	{0xc0,0xc8},
	{0xc1,0x96},
	{0x86,0x3d},
	{0x50,0x00},
	{0x51,0x90},
	{0x52,0x2c},
	{0x53,0x00},
	{0x54,0x00},
	{0x55,0x88},
	{0x57,0x00},
	{0x5a,0x90},
	{0x5b,0x2c},
	{0x5c,0x05},
	{0xd3,0x82},
	{0xe0,0x00},
#endif
	
	{0xff, 0xff},	
};


static resolution_param_t  prev_resolution_array[] = {
	{
		.frmsize		= {640, 480},
		.active_frmsize	= {640, 478},
		.active_fps		= 236,
		.size_type		= SIZE_VGA_640X480,
		.reg_script		= OV2640_preview_script,
	}, {
		.frmsize		= {320, 240},
		.active_frmsize	= {320, 240},
		.active_fps		= 236,
		.size_type		= SIZE_QVGA_320x240,
		.reg_script		= OV2640_preview_qvga_script,
	},
};

static resolution_param_t  capture_resolution_array[] = {
	{
		.frmsize		= {1600, 1200},
		.active_frmsize		= {1600, 1198},
		.active_fps		= 150,
		.size_type		= SIZE_UXGA_1600X1200,
		.reg_script		= OV2640_capture_script,
	}
};
	


static resulution_size_type_t get_size_type(int width, int height)
{
	resulution_size_type_t rv = SIZE_NULL;
	if (width * height >= 1600 * 1200)
		rv = SIZE_UXGA_1600X1200;
	else if (width * height >= 600 * 400)
		rv = SIZE_VGA_640X480;
	else if (width * height >= 300 * 200)
		rv = SIZE_QVGA_320x240;
	return rv;
}

static resolution_param_t* get_resolution_param(struct ov2640_device *dev, int is_capture, int width, int height)
{
	int i = 0;
	int arry_size = 0;
	resolution_param_t* tmp_resolution_param = NULL;
	resulution_size_type_t res_type = SIZE_NULL;
	res_type = get_size_type(width, height);
	if (res_type == SIZE_NULL)
		return NULL;
	if (is_capture) {
		tmp_resolution_param = capture_resolution_array;
		arry_size = sizeof(capture_resolution_array);
	} else {
		tmp_resolution_param = prev_resolution_array;
		arry_size = sizeof(prev_resolution_array);
	}
	
	for (i = 0; i < arry_size; i++) {
		if (tmp_resolution_param[i].size_type == res_type)
			return &tmp_resolution_param[i];
	}
	return NULL;
}

static int set_resolution_param(struct ov2640_device *dev, resolution_param_t* res_param)
{
	struct i2c_client *client = v4l2_get_subdevdata(&dev->sd);
	int rc = -1;
	if (!res_param->reg_script) {
		printk("error, resolution reg script is NULL\n");
		return -1;
	}
	int i=0;
	while(1) {
		if (res_param->reg_script[i].val==0xff&&res_param->reg_script[i].addr==0xff) {
			printk("setting resolutin param complete\n");
			break;
		}
		if((i2c_put_byte_add8_new(client, res_param->reg_script[i].addr, res_param->reg_script[i].val)) < 0) {
			printk("fail in setting resolution param. i=%d\n",i);
			break;
		}
		i++;
	}
	dev->cur_resolution_param = res_param;
}

//load GT2005 parameters
void OV2640_init_regs(struct ov2640_device *dev)
{
	int i=0;//,j;
	unsigned char buf[2];
	struct i2c_client *client = v4l2_get_subdevdata(&dev->sd);
	
	while(1)
	{
		buf[0] = OV2640_script[i].addr;
		buf[1] = OV2640_script[i].val;
		i++;
		if (OV2640_script[i].val == 0xff 
				&& OV2640_script[i].addr == 0xff) {
		    	printk("OV2640_write_regs success" \
		    			" in initial OV2640.\n");
		 	break;
		}
		if((i2c_put_byte_add8(client,buf, 2)) < 0) {
		    	printk("fail in initial OV2640.i=%d \n",i);
			return;
		}
		if (i == 1)
			msleep(40);
	}

	return;
}

#endif

/*************************************************************************
* FUNCTION
*	set_OV2640_param_wb
*
* DESCRIPTION
*	OV2640 wb setting.
*
* PARAMETERS
*	none
*
* RETURNS
*	None
*
* GLOBALS AFFECTED  白平衡参数
*
*************************************************************************/
void set_OV2640_param_wb(struct ov2640_device *dev,enum  camera_wb_flip_e para)
{
//	kal_uint16 rgain=0x80, ggain=0x80, bgain=0x80;
	struct i2c_client *client = v4l2_get_subdevdata(&dev->sd);

	unsigned char buf[4];

	unsigned char  temp_reg;
	//temp_reg=ov2640_read_byte(0x22);
	buf[0]=0x13;
	buf[1]=0;
	temp_reg=i2c_get_byte_add8(client,buf);
	temp_reg=0xe7;
	printk(" camera set_OV2640_param_wb=%d. \n ",temp_reg);
	switch (para) {
	case CAM_WB_AUTO:
		buf[0]=0xff;
		buf[1]=0x00;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0xc7;
		buf[1]=0x00;
		i2c_put_byte_add8(client,buf,2);   // Enable AWB 
		break;

	case CAM_WB_CLOUD:
		buf[0]=0xff;
		buf[1]=0x00;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0xc7;
		buf[1]=0x40;
		i2c_put_byte_add8(client,buf,2);  
		buf[0]=0xcc;
		buf[1]=0x65;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0xcd;
		buf[1]=0x41;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0xce;
		buf[1]=0x4f;
		i2c_put_byte_add8(client,buf,2); 
		break;

	case CAM_WB_DAYLIGHT:   // tai yang guang
		buf[0]=0xff;
		buf[1]=0x00;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0xc7;
		buf[1]=0x40;
		i2c_put_byte_add8(client,buf,2);  
		buf[0]=0xcc;
		buf[1]=0x5e;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0xcd;
		buf[1]=0x41;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0xce;
		buf[1]=0x54;
		i2c_put_byte_add8(client,buf,2); 
		break;

	case CAM_WB_INCANDESCENCE:   // bai re guang
		buf[0]=0xff;
		buf[1]=0x00;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0xc7;
		buf[1]=0x40;
		i2c_put_byte_add8(client,buf,2);  
		buf[0]=0xcc;
		buf[1]=0x42;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0xcd;
		buf[1]=0x3f;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0xce;
		buf[1]=0x71;
		i2c_put_byte_add8(client,buf,2); 
		break;

	case CAM_WB_FLUORESCENT:   //ri guang deng
		buf[0]=0xff;
		buf[1]=0x00;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0xc7;
		buf[1]=0x40;
		i2c_put_byte_add8(client,buf,2);  
		buf[0]=0xcc;
		buf[1]=0x52;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0xcd;
		buf[1]=0x41;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0xce;
		buf[1]=0x66;
		i2c_put_byte_add8(client,buf,2); 
		break;

	case CAM_WB_TUNGSTEN:   // wu si deng
		buf[0]=0xff;
		buf[1]=0x00;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0xc7;
		buf[1]=0x40;
		i2c_put_byte_add8(client,buf,2);  
		buf[0]=0xcc;
		buf[1]=0x52;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0xcd;
		buf[1]=0x41;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0xce;
		buf[1]=0x66;
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
*	OV2640_night_mode
*
* DESCRIPTION
*	This function night mode of OV2640.
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
void OV2640_night_mode(struct ov2640_device *dev,enum  camera_night_mode_flip_e enable)
{
	struct i2c_client *client = v4l2_get_subdevdata(&dev->sd);
	unsigned char buf[4];

	unsigned char  temp_reg;


}
/*************************************************************************
* FUNCTION
*	OV2640_night_mode
*
* DESCRIPTION
*	This function night mode of OV2640.
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

void OV2640_set_param_banding(struct ov2640_device *dev,enum  camera_night_mode_flip_e banding)
{
    struct i2c_client *client = v4l2_get_subdevdata(&dev->sd);
	unsigned char buf[4];
	int temp;
	switch(banding)
		{
		case CAM_BANDING_60HZ:
			
			i2c_put_byte_add8(client,0xff,0x01);
			temp = i2c_get_byte(client, 0x0c);
			temp = temp & 0xfb; // banding 60, bit[2] = 0
			i2c_put_byte(client, 0x3b, temp);			
			i2c_put_byte_add8(client,0x4e,0x00);
			i2c_put_byte_add8(client,0x50,0xa8);
			
			break;
		case CAM_BANDING_50HZ:
			
			i2c_put_byte_add8(client,0xff,0x01);
			temp = i2c_get_byte(client, 0x3b);
			temp = temp | 0x04; // banding 50, bit[2] = 1
			i2c_put_byte(client, 0x3b, temp);				
			i2c_put_byte_add8(client,0x4e,0x00);
			i2c_put_byte_add8(client,0x4f,0xca);
			
			break;

		}

}


/*************************************************************************
* FUNCTION
*	set_OV2640_param_exposure
*
* DESCRIPTION
*	OV2640 exposure setting.
*
* PARAMETERS
*	none
*
* RETURNS
*	None
*
* GLOBALS AFFECTED  亮度等级 调节参数
*
*************************************************************************/
void set_OV2640_param_exposure(struct ov2640_device *dev,enum camera_exposure_e para)//曝光调节
{
	struct i2c_client *client = v4l2_get_subdevdata(&dev->sd);

	unsigned char buf[4];
#if 0
	switch (para) {
	case EXPOSURE_N4_STEP:
		buf[0]=0xff;
		buf[1]=0x01;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x24;
		buf[1]=0x28;
		i2c_put_byte_add8(client,buf,2);  
		buf[0]=0x25;
		buf[1]=0x20;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x26;
		buf[1]=0x41;
		i2c_put_byte_add8(client,buf,2); 
		break;
			
	case EXPOSURE_N3_STEP:
		buf[0]=0xff;
		buf[1]=0x01;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x24;
		buf[1]=0x30;
		i2c_put_byte_add8(client,buf,2);  
		buf[0]=0x25;
		buf[1]=0x28;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x26;
		buf[1]=0x51;
		i2c_put_byte_add8(client,buf,2); 
		break;
		
	case EXPOSURE_N2_STEP:
		buf[0]=0xff;
		buf[1]=0x01;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x24;
		buf[1]=0x38;
		i2c_put_byte_add8(client,buf,2);  
		buf[0]=0x25;
		buf[1]=0x28;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x26;
		buf[1]=0x61;
		i2c_put_byte_add8(client,buf,2); 
		break;
		
	case EXPOSURE_N1_STEP:
		buf[0]=0xff;
		buf[1]=0x01;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x24;
		buf[1]=0x40;
		i2c_put_byte_add8(client,buf,2);  
		buf[0]=0x25;
		buf[1]=0x38;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x26;
		buf[1]=0x62;
		i2c_put_byte_add8(client,buf,2); 
		break;
		
	case EXPOSURE_0_STEP:
		buf[0]=0xff;
		buf[1]=0x01;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x24;
		buf[1]=0x48;
		i2c_put_byte_add8(client,buf,2);  
		buf[0]=0x25;
		buf[1]=0x38;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x26;
		buf[1]=0x72;
		i2c_put_byte_add8(client,buf,2); 
		break;
		
	case EXPOSURE_P1_STEP:
		buf[0]=0xff;
		buf[1]=0x01;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x24;
		buf[1]=0x50;
		i2c_put_byte_add8(client,buf,2);  
		buf[0]=0x25;
		buf[1]=0x48;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x26;
		buf[1]=0x83;
		i2c_put_byte_add8(client,buf,2); 
		break;
		
	case EXPOSURE_P2_STEP:
		buf[0]=0xff;
		buf[1]=0x01;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x24;
		buf[1]=0x58;
		i2c_put_byte_add8(client,buf,2);  
		buf[0]=0x25;
		buf[1]=0x50;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x26;
		buf[1]=0x83;
		i2c_put_byte_add8(client,buf,2); 
		break;
		
	case EXPOSURE_P3_STEP:
		buf[0]=0xff;
		buf[1]=0x01;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x24;
		buf[1]=0x60;
		i2c_put_byte_add8(client,buf,2);  
		buf[0]=0x25;
		buf[1]=0x50;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x26;
		buf[1]=0x93;
		i2c_put_byte_add8(client,buf,2); 
		break;
		break;
		
	case EXPOSURE_P4_STEP:
		buf[0]=0xff;
		buf[1]=0x01;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x24;
		buf[1]=0x68;
		i2c_put_byte_add8(client,buf,2);  
		buf[0]=0x25;
		buf[1]=0x58;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x26;
		buf[1]=0xa4;
		i2c_put_byte_add8(client,buf,2); 
		break;
		
	default:
		buf[0]=0xff;
		buf[1]=0x01;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x24;
		buf[1]=0x50;
		i2c_put_byte_add8(client,buf,2);  
		buf[0]=0x25;
		buf[1]=0x48;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x26;
		buf[1]=0x83;
		i2c_put_byte_add8(client,buf,2); 
		break;
	}
	#else  // use sde brightness
	
		buf[0]=0xff;
		buf[1]=0x00;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x7c;
		buf[1]=0x00;
		i2c_put_byte_add8(client,buf,2);  
		buf[0]=0x7d;
		buf[1]=0x06;
		i2c_put_byte_add8(client,buf,2); 
	switch (para) {
				
	case EXPOSURE_N4_STEP:
		buf[0]=0xff;
		buf[1]=0x00;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x7c;
		buf[1]=0x09;
		i2c_put_byte_add8(client,buf,2);  
		buf[0]=0x7d;
		buf[1]=0x40;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x7d;
		buf[1]=0x0e;
		i2c_put_byte_add8(client,buf,2); 
		break;
			
	case EXPOSURE_N3_STEP:
		buf[0]=0xff;
		buf[1]=0x00;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x7c;
		buf[1]=0x09;
		i2c_put_byte_add8(client,buf,2);  
		buf[0]=0x7d;
		buf[1]=0x30;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x7d;
		buf[1]=0x0e;
		i2c_put_byte_add8(client,buf,2); 
		break;
		
	case EXPOSURE_N2_STEP:
		buf[0]=0xff;
		buf[1]=0x00;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x7c;
		buf[1]=0x09;
		i2c_put_byte_add8(client,buf,2);  
		buf[0]=0x7d;
		buf[1]=0x20;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x7d;
		buf[1]=0x0e;
		i2c_put_byte_add8(client,buf,2); 
		break;
		
	case EXPOSURE_N1_STEP:
		buf[0]=0xff;
		buf[1]=0x00;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x7c;
		buf[1]=0x09;
		i2c_put_byte_add8(client,buf,2);  
		buf[0]=0x7d;
		buf[1]=0x10;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x7d;
		buf[1]=0x0e;
		i2c_put_byte_add8(client,buf,2); 
		break;
		
	case EXPOSURE_0_STEP:
		buf[0]=0xff;
		buf[1]=0x00;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x7c;
		buf[1]=0x09;
		i2c_put_byte_add8(client,buf,2);  
		buf[0]=0x7d;
		buf[1]=0x00;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x7d;
		buf[1]=0x0e;
		i2c_put_byte_add8(client,buf,2); 
		break;
		
	case EXPOSURE_P1_STEP:
		buf[0]=0xff;
		buf[1]=0x00;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x7c;
		buf[1]=0x09;
		i2c_put_byte_add8(client,buf,2);  
		buf[0]=0x7d;
		buf[1]=0x10;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x7d;
		buf[1]=0x06;
		i2c_put_byte_add8(client,buf,2); 
		break;
		
	case EXPOSURE_P2_STEP:
		buf[0]=0xff;
		buf[1]=0x00;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x7c;
		buf[1]=0x09;
		i2c_put_byte_add8(client,buf,2);  
		buf[0]=0x7d;
		buf[1]=0x20;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x7d;
		buf[1]=0x06;
		i2c_put_byte_add8(client,buf,2); 
		break;
		
	case EXPOSURE_P3_STEP:
		buf[0]=0xff;
		buf[1]=0x00;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x7c;
		buf[1]=0x09;
		i2c_put_byte_add8(client,buf,2);  
		buf[0]=0x7d;
		buf[1]=0x30;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x7d;
		buf[1]=0x06;
		i2c_put_byte_add8(client,buf,2); 
		break;
		
	case EXPOSURE_P4_STEP:
		buf[0]=0xff;
		buf[1]=0x00;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x7c;
		buf[1]=0x09;
		i2c_put_byte_add8(client,buf,2);  
		buf[0]=0x7d;
		buf[1]=0x40;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x7d;
		buf[1]=0x06;
		i2c_put_byte_add8(client,buf,2); 
		break;
		
	default:
		buf[0]=0xff;
		buf[1]=0x00;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x7c;
		buf[1]=0x09;
		i2c_put_byte_add8(client,buf,2);  
		buf[0]=0x7d;
		buf[1]=0x00;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x7d;
		buf[1]=0x0e;
		i2c_put_byte_add8(client,buf,2); 
		break;
	}
	#endif
	i2c_put_byte_add8(client,buf,2);

}

/*************************************************************************
* FUNCTION
*	set_OV2640_param_effect
*
* DESCRIPTION
*	OV2640 effect setting.
*
* PARAMETERS
*	none
*
* RETURNS
*	None
*
* GLOBALS AFFECTED  特效参数
*
*************************************************************************/
void set_OV2640_param_effect(struct ov2640_device *dev,enum camera_effect_flip_e para)//特效设置
{
    struct i2c_client *client = v4l2_get_subdevdata(&dev->sd);
	unsigned char buf[4];
	switch (para){
	case CAM_EFFECT_ENC_NORMAL:
		buf[0]=0xff;
		buf[1]=0x00;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x7c;
		buf[1]=0x00;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x7d;
		buf[1]=0x06;
		i2c_put_byte_add8(client,buf,2); 
		break;
		
	case CAM_EFFECT_ENC_GRAYSCALE:
		buf[0]=0xff;
		buf[1]=0x00;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x7c;
		buf[1]=0x00;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x7d;
		buf[1]=0x20;
		i2c_put_byte_add8(client,buf,2); 
		break;
		
	case CAM_EFFECT_ENC_SEPIA:
		buf[0]=0xff;
		buf[1]=0x00;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x7c;
		buf[1]=0x00;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x7d;
		buf[1]=0x18;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x7c;
		buf[1]=0x05;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x7d;
		buf[1]=0x40;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x7d;
		buf[1]=0xa0;
		i2c_put_byte_add8(client,buf,2); 
		break;
		
	case CAM_EFFECT_ENC_COLORINV:
		buf[0]=0xff;
		buf[1]=0x00;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x7c;
		buf[1]=0x00;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x7d;
		buf[1]=0x40;
		i2c_put_byte_add8(client,buf,2); 
		break;
		
	case CAM_EFFECT_ENC_SEPIAGREEN:
		buf[0]=0xff;
		buf[1]=0x00;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x7c;
		buf[1]=0x00;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x7d;
		buf[1]=0x18;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x7c;
		buf[1]=0x05;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x7d;
		buf[1]=0x40;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x7d;
		buf[1]=0x40;
		i2c_put_byte_add8(client,buf,2); 
		break;
		
	case CAM_EFFECT_ENC_SEPIABLUE:
		buf[0]=0xff;
		buf[1]=0x00;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x7c;
		buf[1]=0x00;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x7d;
		buf[1]=0x18;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x7c;
		buf[1]=0x05;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x7d;
		buf[1]=0xa0;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x7d;
		buf[1]=0x40;
		i2c_put_byte_add8(client,buf,2); 
		break;
		
	default:
		buf[0]=0xff;
		buf[1]=0x00;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x7c;
		buf[1]=0x00;
		i2c_put_byte_add8(client,buf,2); 
		buf[0]=0x7d;
		buf[1]=0x06;
		i2c_put_byte_add8(client,buf,2); 
		break;
	}

}

static int ov2640_setting(struct ov2640_device *dev,int PROP_ID,int value )
{
	int ret=0;
	unsigned char cur_val;
	struct i2c_client *client = v4l2_get_subdevdata(&dev->sd);
	switch(PROP_ID)  {
	case V4L2_CID_BRIGHTNESS:
		break;
	case V4L2_CID_CONTRAST:
		//ret=i2c_put_byte(client,0x0200, value);
		break;
	case V4L2_CID_SATURATION:
		//ret=i2c_put_byte(client,0x0202, value);
		break;
	case V4L2_CID_DO_WHITE_BALANCE:
		if(ov2640_qctrl[0].default_value!=value){
			ov2640_qctrl[0].default_value=value;
			set_OV2640_param_wb(dev,value);
			printk(KERN_INFO " set camera  white_balance=%d. \n ",value);
        	}
		break;
	case V4L2_CID_EXPOSURE:
		if(ov2640_qctrl[1].default_value!=value){
			ov2640_qctrl[1].default_value=value;
			set_OV2640_param_exposure(dev,value);
			printk(KERN_INFO " set camera  exposure=%d. \n ",value);
        	}
		break;
	case V4L2_CID_COLORFX:
		if(ov2640_qctrl[2].default_value!=value){
			ov2640_qctrl[2].default_value=value;
			set_OV2640_param_effect(dev,value);
			printk(KERN_INFO " set camera  effect=%d. \n ",value);
        	}
		break;
	case V4L2_CID_WHITENESS:
		 if(ov2640_qctrl[3].default_value!=value){
			ov2640_qctrl[3].default_value=value;
			OV2640_set_param_banding(dev,value);
			printk(KERN_INFO " set camera  banding=%d. \n ",value);
        	}
		break;
	case V4L2_CID_HFLIP:
		value = value & 0x3;
		if(ov2640_qctrl[4].default_value!=value){
			ov2640_qctrl[4].default_value=value;
			printk(" set camera  h filp =%d. \n ",value);
        	}
		break;
	case V4L2_CID_VFLIP:    /* set flip on V. */         
		break;
	case V4L2_CID_ZOOM_ABSOLUTE:
		if(ov2640_qctrl[6].default_value!=value){
			ov2640_qctrl[6].default_value=value;
			//printk(KERN_INFO " set camera  zoom mode=%d. \n ",value);
        	}
		break;
	case V4L2_CID_ROTATE:
		 if(ov2640_qctrl[7].default_value!=value){
			ov2640_qctrl[7].default_value=value;
			printk(" set camera  rotate =%d. \n ",value);
        	}
		break;
	default:
		ret=-1;
		break;
	}
	return ret;

}

static void power_down_ov2640(struct ov2640_device *dev)
{
	struct i2c_client *client = v4l2_get_subdevdata(&dev->sd);
	unsigned char buf[4];
	//buf[0]=0x12;
	//buf[1]=0x80;
	//i2c_put_byte_add8(client,buf,2);
	msleep(5);
	//buf[0]=0xb8;
	//buf[1]=0x12;
	//i2c_put_byte_add8(client,buf,2);
	msleep(1);
	return;
}

/* ------------------------------------------------------------------
	DMA and thread functions
   ------------------------------------------------------------------*/

#define TSTAMP_MIN_Y	24
#define TSTAMP_MAX_Y	(TSTAMP_MIN_Y + 15)
#define TSTAMP_INPUT_X	10
#define TSTAMP_MIN_X	(54 + TSTAMP_INPUT_X)

static void ov2640_fillbuff(struct ov2640_fh *fh, struct ov2640_buffer *buf)
{
	struct ov2640_device *dev = fh->dev;
	void *vbuf = videobuf_to_vmalloc(&buf->vb);
	vm_output_para_t para = {0};
	dprintk(dev,1,"%s\n", __func__);
	if (!vbuf)
		return;
	/*  0x18221223 indicate the memory type is MAGIC_VMAL_MEM*/
	para.mirror = ov2640_qctrl[4].default_value&3;
	para.v4l2_format = fh->fmt->fourcc;
	para.v4l2_memory = 0x18221223;
	para.zoom = ov2640_qctrl[6].default_value;
	para.angle = ov2640_qctrl[7].default_value;
	para.vaddr = (unsigned)vbuf;
	vm_fill_buffer(&buf->vb,&para);
	buf->vb.state = VIDEOBUF_DONE;
}

static void ov2640_thread_tick(struct ov2640_fh *fh)
{
	struct ov2640_buffer *buf;
	struct ov2640_device *dev = fh->dev;
	struct ov2640_dmaqueue *dma_q = &dev->vidq;

	unsigned long flags = 0;

	dprintk(dev, 1, "Thread tick\n");

	spin_lock_irqsave(&dev->slock, flags);
	if (list_empty(&dma_q->active)) {
		dprintk(dev, 1, "No active queue to serve\n");
		goto unlock;
	}

	buf = list_entry(dma_q->active.next,
			 struct ov2640_buffer, vb.queue);
	dprintk(dev, 1, "%s\n", __func__);
	dprintk(dev, 1, "list entry get buf is %x\n",(unsigned)buf);

	/* Nobody is waiting on this buffer, return */
	if (!waitqueue_active(&buf->vb.done))
		goto unlock;

	list_del(&buf->vb.queue);

	do_gettimeofday(&buf->vb.ts);

	/* Fill buffer */
	spin_unlock_irqrestore(&dev->slock, flags);
	ov2640_fillbuff(fh, buf);
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

static void ov2640_sleep(struct ov2640_fh *fh)
{
	struct ov2640_device *dev = fh->dev;
	struct ov2640_dmaqueue *dma_q = &dev->vidq;

	DECLARE_WAITQUEUE(wait, current);

	dprintk(dev, 1, "%s dma_q=0x%08lx\n", __func__,
		(unsigned long)dma_q);

	add_wait_queue(&dma_q->wq, &wait);
	if (kthread_should_stop())
		goto stop_task;

	/* Calculate time to wake up */
	//timeout = msecs_to_jiffies(frames_to_ms(1));

	ov2640_thread_tick(fh);

	schedule_timeout_interruptible(2);

stop_task:
	remove_wait_queue(&dma_q->wq, &wait);
	try_to_freeze();
}

static int ov2640_thread(void *data)
{
	struct ov2640_fh  *fh = data;
	struct ov2640_device *dev = fh->dev;

	dprintk(dev, 1, "thread started\n");

	set_freezable();

	for (;;) {
		ov2640_sleep(fh);

		if (kthread_should_stop())
			break;
	}
	dprintk(dev, 1, "thread: exit\n");
	return 0;
}

static int ov2640_start_thread(struct ov2640_fh *fh)
{
	struct ov2640_device *dev = fh->dev;
	struct ov2640_dmaqueue *dma_q = &dev->vidq;

	dma_q->frame = 0;
	dma_q->ini_jiffies = jiffies;

	dprintk(dev, 1, "%s\n", __func__);

	dma_q->kthread = kthread_run(ov2640_thread, fh, "ov2640");

	if (IS_ERR(dma_q->kthread)) {
		v4l2_err(&dev->v4l2_dev, "kernel_thread() failed\n");
		return PTR_ERR(dma_q->kthread);
	}
	/* Wakes thread */
	wake_up_interruptible(&dma_q->wq);

	dprintk(dev, 1, "returning from %s\n", __func__);
	return 0;
}

static void ov2640_stop_thread(struct ov2640_dmaqueue  *dma_q)
{
	struct ov2640_device *dev = container_of(dma_q, struct ov2640_device, vidq);

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
	struct ov2640_fh  *fh = vq->priv_data;
	struct ov2640_device *dev  = fh->dev;
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

static void free_buffer(struct videobuf_queue *vq, struct ov2640_buffer *buf)
{
	struct ov2640_fh  *fh = vq->priv_data;
	struct ov2640_device *dev  = fh->dev;

	dprintk(dev, 1, "%s, state: %i\n", __func__, buf->vb.state);

	if (in_interrupt())
		BUG();

	videobuf_vmalloc_free(&buf->vb);
	dprintk(dev, 1, "free_buffer: freed\n");
	buf->vb.state = VIDEOBUF_NEEDS_INIT;
}

#define norm_maxw() 1600
#define norm_maxh() 1600
static int
buffer_prepare(struct videobuf_queue *vq, struct videobuf_buffer *vb,
						enum v4l2_field field)
{
	struct ov2640_fh     *fh  = vq->priv_data;
	struct ov2640_device    *dev = fh->dev;
	struct ov2640_buffer *buf = container_of(vb, struct ov2640_buffer, vb);
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
	struct ov2640_buffer    *buf  = container_of(vb, struct ov2640_buffer, vb);
	struct ov2640_fh        *fh   = vq->priv_data;
	struct ov2640_device       *dev  = fh->dev;
	struct ov2640_dmaqueue *vidq = &dev->vidq;

	dprintk(dev, 1, "%s\n", __func__);
	buf->vb.state = VIDEOBUF_QUEUED;
	list_add_tail(&buf->vb.queue, &vidq->active);
}

static void buffer_release(struct videobuf_queue *vq,
			   struct videobuf_buffer *vb)
{
	struct ov2640_buffer   *buf  = container_of(vb, struct ov2640_buffer, vb);
	struct ov2640_fh       *fh   = vq->priv_data;
	struct ov2640_device      *dev  = (struct ov2640_device *)fh->dev;

	dprintk(dev, 1, "%s\n", __func__);

	free_buffer(vq, buf);
}

static struct videobuf_queue_ops ov2640_video_qops = {
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
	struct ov2640_fh  *fh  = priv;
	struct ov2640_device *dev = fh->dev;

	strcpy(cap->driver, "ov2640");
	strcpy(cap->card, "ov2640");
	strlcpy(cap->bus_info, dev->v4l2_dev.name, sizeof(cap->bus_info));
	cap->version = OV2640_CAMERA_VERSION;
	cap->capabilities =	V4L2_CAP_VIDEO_CAPTURE |
				V4L2_CAP_STREAMING     |
				V4L2_CAP_READWRITE;
	return 0;
}

static int vidioc_enum_fmt_vid_cap(struct file *file, void  *priv,
					struct v4l2_fmtdesc *f)
{
	struct ov2640_fmt *fmt;

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
	struct ov2640_fh *fh = priv;

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
	struct ov2640_fh  *fh  = priv;
	struct ov2640_device *dev = fh->dev;
	struct ov2640_fmt *fmt;
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
	struct ov2640_fh *fh = priv;
	struct videobuf_queue *q = &fh->vb_vidq;
	struct ov2640_device *dev = fh->dev;
	resolution_param_t* res_param = NULL;

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
	
	if(f->fmt.pix.pixelformat==V4L2_PIX_FMT_RGB24){
		res_param = get_resolution_param(dev, 1, fh->width,fh->height);
		if (!res_param) {
			printk("error, resolution param not get\n");
			goto out;
		}
		//Get_preview_exposure_gain(dev);
		set_resolution_param(dev, res_param);

		//cal_exposure(dev);
	} else {
		res_param = get_resolution_param(dev, 0, fh->width,fh->height);
		if (!res_param) {
			printk("error, resolution param not get\n");
			goto out;
		}
		set_resolution_param(dev, res_param);
	}
	ret = 0;
out:
	mutex_unlock(&q->vb_lock);

	return ret;
}

static int vidioc_reqbufs(struct file *file, void *priv,
			  struct v4l2_requestbuffers *p)
{
	struct ov2640_fh  *fh = priv;

	return (videobuf_reqbufs(&fh->vb_vidq, p));
}

static int vidioc_querybuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	struct ov2640_fh  *fh = priv;

	return (videobuf_querybuf(&fh->vb_vidq, p));
}

static int vidioc_qbuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	struct ov2640_fh *fh = priv;

	return (videobuf_qbuf(&fh->vb_vidq, p));
}

static int vidioc_dqbuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	struct ov2640_fh  *fh = priv;

	return (videobuf_dqbuf(&fh->vb_vidq, p,
				file->f_flags & O_NONBLOCK));
}

#ifdef CONFIG_VIDEO_V4L1_COMPAT
static int vidiocgmbuf(struct file *file, void *priv, struct video_mbuf *mbuf)
{
	struct ov2640_fh  *fh = priv;

	return videobuf_cgmbuf(&fh->vb_vidq, mbuf, 8);
}
#endif

static int vidioc_streamon(struct file *file, void *priv, enum v4l2_buf_type i)
{
	struct ov2640_fh  *fh = priv;
	tvin_parm_t para;
	int ret = 0 ;
	if (fh->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;
	if (i != fh->type)
		return -EINVAL;

	para.port  = TVIN_PORT_CAMERA_YUYV;
	para.fmt_info.fmt = TVIN_SIG_FMT_MAX+1;//TVIN_SIG_FMT_MAX+1;TVIN_SIG_FMT_CAMERA_1280X720P_30Hz
	if (fh->dev->cur_resolution_param) {
		para.fmt_info.frame_rate = fh->dev->cur_resolution_param->active_fps;//175;
		para.fmt_info.h_active = fh->dev->cur_resolution_param->active_frmsize.width;
		para.fmt_info.v_active = fh->dev->cur_resolution_param->active_frmsize.height;
	} else {
		para.fmt_info.frame_rate = 175;
		para.fmt_info.h_active = 640;
		para.fmt_info.v_active = 478;
	}
	para.fmt_info.hsync_phase = 1;
	para.fmt_info.vsync_phase  = 0;	
	ret =  videobuf_streamon(&fh->vb_vidq);
	if(ret == 0){
		start_tvin_service(0,&para);
		fh->stream_on        = 1;
	}
	return ret;
}

static int vidioc_streamoff(struct file *file, void *priv, enum v4l2_buf_type i)
{
	struct ov2640_fh  *fh = priv;

    int ret = 0 ;
	if (fh->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;
	if (i != fh->type)
		return -EINVAL;
	ret = videobuf_streamoff(&fh->vb_vidq);
	if(ret == 0 ){
		stop_tvin_service(0);
		fh->stream_on = 0;
	}
	return ret;
}

static int vidioc_enum_framesizes(struct file *file, void *fh,struct v4l2_frmsizeenum *fsize)
{
	int ret = 0,i=0;
	struct ov2640_fmt *fmt = NULL;
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
		printk("ov2640_prev_resolution[fsize->index]   before fsize->index== %d\n",fsize->index);//potti
		if (fsize->index >= ARRAY_SIZE(prev_resolution_array))
			return -EINVAL;
		frmsize = &prev_resolution_array[fsize->index].frmsize;
		printk("ov2640_prev_resolution[fsize->index]   after fsize->index== %d\n",fsize->index);
		fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
		fsize->discrete.width = frmsize->width;
		fsize->discrete.height = frmsize->height;
	} else if (fmt->fourcc == V4L2_PIX_FMT_RGB24){
		printk("ov2640_pic_resolution[fsize->index]   before fsize->index== %d\n",fsize->index);
		if (fsize->index >= ARRAY_SIZE(capture_resolution_array))
			return -EINVAL;
		frmsize = &capture_resolution_array[fsize->index].frmsize;
		printk("ov2640_pic_resolution[fsize->index]   after fsize->index== %d\n",fsize->index);    
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
	struct ov2640_fh *fh = priv;
	struct ov2640_device *dev = fh->dev;

	*i = dev->input;

	return (0);
}

static int vidioc_s_input(struct file *file, void *priv, unsigned int i)
{
	struct ov2640_fh *fh = priv;
	struct ov2640_device *dev = fh->dev;

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

	for (i = 0; i < ARRAY_SIZE(ov2640_qctrl); i++)
		if (qc->id && qc->id == ov2640_qctrl[i].id) {
			memcpy(qc, &(ov2640_qctrl[i]),
				sizeof(*qc));
			return (0);
		}

	return -EINVAL;
}

static int vidioc_g_ctrl(struct file *file, void *priv,
			 struct v4l2_control *ctrl)
{
	struct ov2640_fh *fh = priv;
	struct ov2640_device *dev = fh->dev;
	int i;

	for (i = 0; i < ARRAY_SIZE(ov2640_qctrl); i++)
		if (ctrl->id == ov2640_qctrl[i].id) {
			ctrl->value = dev->qctl_regs[i];
			return 0;
		}

	return -EINVAL;
}

static int vidioc_s_ctrl(struct file *file, void *priv,
				struct v4l2_control *ctrl)
{
	struct ov2640_fh *fh = priv;
	struct ov2640_device *dev = fh->dev;
	int i;

	for (i = 0; i < ARRAY_SIZE(ov2640_qctrl); i++)
		if (ctrl->id == ov2640_qctrl[i].id) {
			if (ctrl->value < ov2640_qctrl[i].minimum ||
			    ctrl->value > ov2640_qctrl[i].maximum ||
			    ov2640_setting(dev,ctrl->id,ctrl->value)<0) {
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

static int ov2640_open(struct file *file)
{
	struct ov2640_device *dev = video_drvdata(file);
	struct ov2640_fh *fh = NULL;
	int retval = 0;
	ov2640_have_opened=1;
#ifdef CONFIG_ARCH_MESON6
	switch_mod_gate_by_name("ge2d", 1);
#endif
	if(dev->platform_dev_data.device_init) {
		dev->platform_dev_data.device_init();
		printk("+++found a init function, and run it..\n");
	}
	OV2640_init_regs(dev);
	msleep(40);
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

	videobuf_queue_vmalloc_init(&fh->vb_vidq, &ov2640_video_qops,
			NULL, &dev->slock, fh->type, V4L2_FIELD_INTERLACED,
			sizeof(struct ov2640_buffer), fh,NULL);

	ov2640_start_thread(fh);

	return 0;
}

static ssize_t
ov2640_read(struct file *file, char __user *data, size_t count, loff_t *ppos)
{
	struct ov2640_fh *fh = file->private_data;

	if (fh->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		return videobuf_read_stream(&fh->vb_vidq, data, count, ppos, 0,
					file->f_flags & O_NONBLOCK);
	}
	return 0;
}

static unsigned int
ov2640_poll(struct file *file, struct poll_table_struct *wait)
{
	struct ov2640_fh        *fh = file->private_data;
	struct ov2640_device       *dev = fh->dev;
	struct videobuf_queue *q = &fh->vb_vidq;

	dprintk(dev, 1, "%s\n", __func__);

	if (V4L2_BUF_TYPE_VIDEO_CAPTURE != fh->type)
		return POLLERR;

	return videobuf_poll_stream(file, q, wait);
}

static int ov2640_close(struct file *file)
{
	struct ov2640_fh         *fh = file->private_data;
	struct ov2640_device *dev       = fh->dev;
	struct ov2640_dmaqueue *vidq = &dev->vidq;
	struct video_device  *vdev = video_devdata(file);
	ov2640_have_opened=0;

	ov2640_stop_thread(vidq);
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
	ov2640_qctrl[0].default_value=0;
	ov2640_qctrl[1].default_value=4;
	ov2640_qctrl[2].default_value=0;
	ov2640_qctrl[3].default_value=0;
	
	ov2640_qctrl[4].default_value=0;
	ov2640_qctrl[6].default_value=100;
	ov2640_qctrl[7].default_value=0;

	power_down_ov2640(dev);
#endif
	if(dev->platform_dev_data.device_uninit) {
		dev->platform_dev_data.device_uninit();
		printk("+++found a uninit function, and run it..\n");
	}
#ifdef CONFIG_ARCH_MESON6
	switch_mod_gate_by_name("ge2d", 0);
#endif	
	wake_unlock(&(dev->wake_lock));
	return 0;
}

static int ov2640_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct ov2640_fh  *fh = file->private_data;
	struct ov2640_device *dev = fh->dev;
	int ret;

	dprintk(dev, 1, "mmap called, vma=0x%08lx\n", (unsigned long)vma);

	ret = videobuf_mmap_mapper(&fh->vb_vidq, vma);

	dprintk(dev, 1, "vma start=0x%08lx, size=%ld, ret=%d\n",
		(unsigned long)vma->vm_start,
		(unsigned long)vma->vm_end-(unsigned long)vma->vm_start,
		ret);

	return ret;
}

static const struct v4l2_file_operations ov2640_fops = {
	.owner		= THIS_MODULE,
	.open           = ov2640_open,
	.release        = ov2640_close,
	.read           = ov2640_read,
	.poll		= ov2640_poll,
	.ioctl          = video_ioctl2, /* V4L2 ioctl handler */
	.mmap           = ov2640_mmap,
};

static const struct v4l2_ioctl_ops ov2640_ioctl_ops = {
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

static struct video_device ov2640_template = {
	.name		= "ov2640_v4l",
	.fops           = &ov2640_fops,
	.ioctl_ops 	= &ov2640_ioctl_ops,
	.release	= video_device_release,

	.tvnorms              = V4L2_STD_525_60,
	.current_norm         = V4L2_STD_NTSC_M,
};

static int ov2640_g_chip_ident(struct v4l2_subdev *sd, struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	return v4l2_chip_ident_i2c_client(client, chip, V4L2_IDENT_OV2640, 0);
}

static const struct v4l2_subdev_core_ops ov2640_core_ops = {
	.g_chip_ident = ov2640_g_chip_ident,
};

static const struct v4l2_subdev_ops ov2640_ops = {
	.core = &ov2640_core_ops,
};

static int ov2640_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	aml_plat_cam_data_t* plat_dat;
	int err;
	struct ov2640_device *t;
	struct v4l2_subdev *sd;
	v4l_info(client, "chip found @ 0x%x (%s)\n",
			client->addr << 1, client->adapter->name);
	t = kzalloc(sizeof(*t), GFP_KERNEL);
	if (t == NULL)
		return -ENOMEM;
	sd = &t->sd;
	v4l2_i2c_subdev_init(sd, client, &ov2640_ops);
	mutex_init(&t->mutex);
	this_client=client;

	/* Now create a video4linux device */
	t->vdev = video_device_alloc();
	if (t->vdev == NULL) {
		kfree(t);
		kfree(client);
		return -ENOMEM;
	}
	memcpy(t->vdev, &ov2640_template, sizeof(*t->vdev));

	video_set_drvdata(t->vdev, t);

	wake_lock_init(&(t->wake_lock),WAKE_LOCK_SUSPEND, "ov2640");
	/* Register it */
	plat_dat= (aml_plat_cam_data_t*)client->dev.platform_data;
	if (plat_dat) {
		t->platform_dev_data.device_init=plat_dat->device_init;
		t->platform_dev_data.device_uninit=plat_dat->device_uninit;
		t->platform_dev_data.device_probe=plat_dat->device_probe;
		if(plat_dat->video_nr>=0)  video_nr=plat_dat->video_nr;
		power_down_ov2640(t);
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

static int ov2640_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ov2640_device *t = to_dev(sd);

	video_unregister_device(t->vdev);
	v4l2_device_unregister_subdev(sd);
	wake_lock_destroy(&(t->wake_lock));
	kfree(t);
	return 0;
}

static const struct i2c_device_id ov2640_id[] = {
	{ "ov2640_i2c", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ov2640_id);

static struct v4l2_i2c_driver_data v4l2_i2c_data = {
	.name = "ov2640",
	.probe = ov2640_probe,
	.remove = ov2640_remove,
	.id_table = ov2640_id,
};

