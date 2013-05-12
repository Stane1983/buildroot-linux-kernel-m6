/*
 * TVIN Modules Exported Header File
 *
 * Author: kele bai <kele.bai@amlogic.com>
 *       
 *
 * Copyright (C) 2010 Amlogic Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#ifndef __TVIN_V4L2_H
#define __TVIN_V4L2_H
#include "tvin.h"
/*
   below macro defined applied to camera driver
 */
typedef enum camera_light_mode_e {
        ADVANCED_AWB = 0,
        SIMPLE_AWB,
        MANUAL_DAY,
        MANUAL_A,
        MANUAL_CWF,
        MANUAL_CLOUDY,
}camera_light_mode_t;

typedef enum camera_saturation_e {
        SATURATION_N4_STEP = 0,
        SATURATION_N3_STEP,
        SATURATION_N2_STEP,
        SATURATION_N1_STEP,
        SATURATION_0_STEP,
        SATURATION_P1_STEP,
        SATURATION_P2_STEP,
        SATURATION_P3_STEP,
        SATURATION_P4_STEP,
}camera_saturation_t;


typedef enum camera_brightness_e {
        BRIGHTNESS_N4_STEP = 0,
        BRIGHTNESS_N3_STEP,
        BRIGHTNESS_N2_STEP,
        BRIGHTNESS_N1_STEP,
        BRIGHTNESS_0_STEP,
        BRIGHTNESS_P1_STEP,
        BRIGHTNESS_P2_STEP,
        BRIGHTNESS_P3_STEP,
        BRIGHTNESS_P4_STEP,
}camera_brightness_t;

typedef enum camera_contrast_e {
        CONTRAST_N4_STEP = 0,
        CONTRAST_N3_STEP,
        CONTRAST_N2_STEP,
        CONTRAST_N1_STEP,
        CONTRAST_0_STEP,
        CONTRAST_P1_STEP,
        CONTRAST_P2_STEP,
        CONTRAST_P3_STEP,
        CONTRAST_P4_STEP,
}camera_contrast_t;

typedef enum camera_hue_e {
        HUE_N180_DEGREE = 0,
        HUE_N150_DEGREE,
        HUE_N120_DEGREE,
        HUE_N90_DEGREE,
        HUE_N60_DEGREE,
        HUE_N30_DEGREE,
        HUE_0_DEGREE,
        HUE_P30_DEGREE,
        HUE_P60_DEGREE,
        HUE_P90_DEGREE,
        HUE_P120_DEGREE,
        HUE_P150_DEGREE,
}camera_hue_t;

typedef enum camera_special_effect_e {
        SPECIAL_EFFECT_NORMAL = 0,
        SPECIAL_EFFECT_BW,
        SPECIAL_EFFECT_BLUISH,
        SPECIAL_EFFECT_SEPIA,
        SPECIAL_EFFECT_REDDISH,
        SPECIAL_EFFECT_GREENISH,
        SPECIAL_EFFECT_NEGATIVE,
}camera_special_effect_t;

typedef enum camera_exposure_e {
        EXPOSURE_N4_STEP = 0,
        EXPOSURE_N3_STEP,
        EXPOSURE_N2_STEP,
        EXPOSURE_N1_STEP,
        EXPOSURE_0_STEP,
        EXPOSURE_P1_STEP,
        EXPOSURE_P2_STEP,
        EXPOSURE_P3_STEP,
        EXPOSURE_P4_STEP,
}camera_exposure_t;


typedef enum camera_sharpness_e {
        SHARPNESS_1_STEP = 0,
        SHARPNESS_2_STEP,
        SHARPNESS_3_STEP,
        SHARPNESS_4_STEP,
        SHARPNESS_5_STEP,
        SHARPNESS_6_STEP,
        SHARPNESS_7_STEP,
        SHARPNESS_8_STEP,
        SHARPNESS_AUTO_STEP,
}camera_sharpness_t;

typedef enum camera_mirror_flip_e {
        MF_NORMAL = 0,
        MF_MIRROR,
        MF_FLIP,
        MF_MIRROR_FLIP,
}camera_mirror_flip_t;


typedef enum camera_wb_flip_e {
        CAM_WB_AUTO = 0,
        CAM_WB_CLOUD,
        CAM_WB_DAYLIGHT,
        CAM_WB_INCANDESCENCE,
        CAM_WB_TUNGSTEN,
        CAM_WB_FLUORESCENT,
        CAM_WB_MANUAL,
        CAM_WB_SHADE,
        CAM_WB_TWILIGHT,
        CAM_WB_WARM_FLUORESCENT,
}camera_wb_flip_t;

typedef enum camera_focus_mode_e {
        CAM_FOCUS_MODE_RELEASE = 0,
        CAM_FOCUS_MODE_FIXED,
        CAM_FOCUS_MODE_INFINITY,
        CAM_FOCUS_MODE_AUTO,
        CAM_FOCUS_MODE_MACRO,
        CAM_FOCUS_MODE_EDOF,
        CAM_FOCUS_MODE_CONTI_VID,
        CAM_FOCUS_MODE_CONTI_PIC,
}camera_focus_mode_t;

//removed this when move to new v4l2
#define V4L2_CID_AUTO_FOCUS_START               (V4L2_CID_CAMERA_CLASS_BASE+28)
#define V4L2_CID_AUTO_FOCUS_STOP                (V4L2_CID_CAMERA_CLASS_BASE+29)
#define V4L2_CID_AUTO_FOCUS_STATUS              (V4L2_CID_CAMERA_CLASS_BASE+30)
#define V4L2_AUTO_FOCUS_STATUS_IDLE             (0 << 0)
#define V4L2_AUTO_FOCUS_STATUS_BUSY             (1 << 0)
#define V4L2_AUTO_FOCUS_STATUS_REACHED          (1 << 1)
#define V4L2_AUTO_FOCUS_STATUS_FAILED           (1 << 2)
//removed this when move to new v4l2

typedef enum camera_night_mode_flip_e {
        CAM_NM_AUTO = 0,
        CAM_NM_ENABLE,
}camera_night_mode_flip_t;

typedef enum camera_effect_flip_e {
        CAM_EFFECT_ENC_NORMAL = 0,
        CAM_EFFECT_ENC_GRAYSCALE,
        CAM_EFFECT_ENC_SEPIA,
        CAM_EFFECT_ENC_SEPIAGREEN,
        CAM_EFFECT_ENC_SEPIABLUE,
        CAM_EFFECT_ENC_COLORINV,
}camera_effect_flip_t;

typedef enum camera_banding_flip_e {
        CAM_BANDING_DISABLED = 0,
        CAM_BANDING_50HZ,
        CAM_BANDING_60HZ,
        CAM_BANDING_AUTO,
        CAM_BANDING_OFF,
}camera_banding_flip_t;

typedef struct camera_info_s {
        const char * camera_name;
        enum camera_saturation_e saturation;
        enum camera_brightness_e brighrness;
        enum camera_contrast_e contrast;
        enum camera_hue_e hue;
        //  enum camera_special_effect_e special_effect;
        enum camera_exposure_e exposure;
        enum camera_sharpness_e sharpness;
        enum camera_mirror_flip_e mirro_flip;
        enum tvin_sig_fmt_e resolution;
        enum camera_wb_flip_e white_balance;
        enum camera_night_mode_flip_e night_mode;
        enum camera_effect_flip_e effect;
        int qulity;
}camera_info_t;



// ***************************************************************************
// *** IOCTL command definitions *****************************************
// ***************************************************************************

#define CAMERA_IOC_MAGIC 'C'


#define CAMERA_IOC_START        _IOW(CAMERA_IOC_MAGIC, 0x01, struct camera_info_s)
#define CAMERA_IOC_STOP         _IO(CAMERA_IOC_MAGIC, 0x02)
#define CAMERA_IOC_SET_PARA     _IOW(CAMERA_IOC_MAGIC, 0x03, struct camera_info_s)
#define CAMERA_IOC_GET_PARA     _IOR(CAMERA_IOC_MAGIC, 0x04, struct camera_info_s)
#define CAMERA_IOC_START_CAPTURE_PARA     _IOR(CAMERA_IOC_MAGIC, 0x05, struct camera_info_s)
#define CAMERA_IOC_STOP_CAPTURE_PARA     _IOR(CAMERA_IOC_MAGIC, 0x06, struct camera_info_s)



//add for vdin called by backend driver
typedef struct vdin_parm_s {
        enum tvin_port_e     port;
        enum tvin_sig_fmt_e  fmt;//>max:use the information from parameter rather than format table
        enum tvin_color_fmt_e cfmt;//for camera input mainly,the data sequence is different
        enum tvin_scan_mode_e scan_mode;//1: progressive 2:interlaced
        unsigned char   hsync_phase;//1: inverted 0: original
        unsigned char   vsync_phase;//1: inverted 0: origianl
        unsigned short  hs_bp;//the horizontal start postion of bt656 window
        unsigned short  vs_bp;//the vertical start postion of bt656 window
        unsigned short  h_active;
        unsigned short  v_active;
        unsigned short  frame_rate;
        unsigned int    reserved;
} vdin_parm_t;

typedef struct vdin_v4l2_ops_s {
        int  (*start_tvin_service)(int no ,vdin_parm_t *para);
        int  (*stop_tvin_service)(int no);
        void (*set_tvin_canvas_info)(int start , int num);
        void (*get_tvin_canvas_info)(int* start , int* num);
        void *private;
}vdin_v4l2_ops_t;

/*
   macro defined applied to camera driver is ending
 */
extern int start_tvin_service(int no ,vdin_parm_t *para);
extern int stop_tvin_service(int no);
extern void set_tvin_canvas_info(int start , int num);
extern void get_tvin_canvas_info(int* start , int* num);
extern int v4l2_vdin_ops_init(vdin_v4l2_ops_t* vdin_v4l2p);
extern vdin_v4l2_ops_t *get_vdin_v4l2_ops(void);
#endif
