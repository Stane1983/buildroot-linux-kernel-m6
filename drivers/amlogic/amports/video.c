/*
 * AMLOGIC Audio/Video streaming port driver.
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
 * Author:  Tim Yao <timyao@amlogic.com>
 *
 */

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <mach/am_regs.h>

#include <linux/string.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/major.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/ctype.h>
#include <linux/amports/ptsserv.h>
#include <linux/amports/timestamp.h>
#include <linux/amports/tsync.h>
#include <linux/amports/canvas.h>
#include <linux/amports/vframe.h>
#include <linux/amports/vframe_provider.h>
#include <linux/amports/vframe_receiver.h>
#include <linux/amports/amstream.h>
#include <linux/vout/vout_notify.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/clk.h>
#include <linux/logo/logo.h>
#if defined(CONFIG_AM_VECM)
#include <linux/aml_common.h>
#endif

#ifdef CONFIG_PM
#include <linux/delay.h>
#include <linux/pm.h>
#endif

#include <plat/fiq_bridge.h>
#include <asm/fiq.h>
#include <asm/uaccess.h>

#include "videolog.h"

#ifdef CONFIG_AM_VIDEO_LOG
#define AMLOG
#endif
#include <linux/amlog.h>
MODULE_AMLOG(LOG_LEVEL_ERROR, 0, LOG_DEFAULT_LEVEL_DESC, LOG_MASK_DESC);

#include "video.h"
#include "vpp.h"

#ifdef CONFIG_AM_DEINTERLACE
#include "deinterlace.h"
#endif

#include "linux/amports/ve.h"
#include "linux/amports/cm.h"

#include "ve_regs.h"
#include "amve.h"
#include "cm_regs.h"
#include "amcm.h"

static int debugflags=0;
#define DEBUG_FLAG_FFPLAY	(1<<0)
#define DEBUG_FLAG_CALC_PTS_INC	(1<<1)

#define RECEIVER_NAME "amvideo"
static int video_receiver_event_fun(int type, void* data, void*);

static const struct vframe_receiver_op_s video_vf_receiver =
{
    .event_cb = video_receiver_event_fun
};
static struct vframe_receiver_s video_vf_recv;

#define RECEIVER4OSD_NAME "amvideo4osd"
static int video4osd_receiver_event_fun(int type, void* data, void*);

static const struct vframe_receiver_op_s video4osd_vf_receiver =
{
    .event_cb = video4osd_receiver_event_fun
};
static struct vframe_receiver_s video4osd_vf_recv;

static struct vframe_provider_s * osd_prov = NULL;

#define DRIVER_NAME "amvideo"
#define MODULE_NAME "amvideo"
#define DEVICE_NAME "amvideo"

#ifdef CONFIG_AML_VSYNC_FIQ_ENABLE
#define FIQ_VSYNC
#endif

//#define SLOW_SYNC_REPEAT
//#define INTERLACE_FIELD_MATCH_PROCESS

#define M_PTS_SMOOTH_MAX 45000
#define M_PTS_SMOOTH_MIN 2250
#define M_PTS_SMOOTH_ADJUST 900
static u32 underflow;
static u32 next_peek_underflow;

#ifdef FIQ_VSYNC
#define BRIDGE_IRQ INT_TIMER_C
#define BRIDGE_IRQ_SET() WRITE_CBUS_REG(ISA_TIMERC, 1)
#endif

#define RESERVE_CLR_FRAME

#define EnableVideoLayer()  \
    do { SET_MPEG_REG_MASK(VPP_MISC, \
         VPP_VD1_PREBLEND | VPP_PREBLEND_EN | VPP_VD1_POSTBLEND); \
         if(debug_flag& DEBUG_FLAG_BLACKOUT){  \
            printk("EnableVideoLayer()\n"); \
         } \
    } while (0)

#define EnableVideoLayer2()  \
    do { SET_MPEG_REG_MASK(VPP_MISC, \
         VPP_VD2_PREBLEND | (0x1ff << VPP_VD2_ALPHA_BIT)); \
    } while (0)

#define DisableVideoLayer() \
    do { CLEAR_MPEG_REG_MASK(VPP_MISC, \
         VPP_VD1_PREBLEND|VPP_VD2_PREBLEND|VPP_VD2_POSTBLEND|VPP_VD1_POSTBLEND ); \
         if(debug_flag& DEBUG_FLAG_BLACKOUT){  \
            printk("DisableVideoLayer()\n"); \
         } \
    } while (0)

#define DisableVideoLayer_PREBELEND() \
    do { CLEAR_MPEG_REG_MASK(VPP_MISC, \
         VPP_VD1_PREBLEND|VPP_VD2_PREBLEND); \
    } while (0)

/*********************************************************/

#define VOUT_TYPE_TOP_FIELD 0
#define VOUT_TYPE_BOT_FIELD 1
#define VOUT_TYPE_PROG      2

#define VIDEO_DISABLE_NONE    0
#define VIDEO_DISABLE_NORMAL  1
#define VIDEO_DISABLE_FORNEXT 2

#define MAX_ZOOM_RATIO 300

#define DUR2PTS(x) ((x) - ((x) >> 4))
#define DUR2PTS_RM(x) ((x) & 0xf)

#ifdef VIDEO_PTS_CHASE
static int vpts_chase=0;
static int av_sync_flag=0;
static int vpts_chase_counter;
static int vpts_chase_pts_diff;
#endif

#define DEBUG_FLAG_BLACKOUT     0x1
#define DEBUG_FLAG_PRINT_TOGGLE_FRAME 0x2
#define DEBUG_FLAG_TOGGLE_SKIP_KEEP_CURRENT  0x10000
#define DEBUG_FLAG_TOGGLE_FRAME_PER_VSYNC    0x20000
static int debug_flag = 0x0;//DEBUG_FLAG_BLACKOUT;

static int vsync_enter_line_max = 0;
static int vsync_exit_line_max = 0;

static int video_3d_format = 0;

const char video_dev_id[] = "amvideo-dev";

#ifdef CONFIG_PM
typedef struct {
    int event;
    u32 vpp_misc;
} video_pm_state_t;

static video_pm_state_t pm_state;
#endif

static DEFINE_MUTEX(video_module_mutex);
static DEFINE_SPINLOCK(lock);
static u32 frame_par_ready_to_set, frame_par_force_to_set;
static u32 vpts_remainder;
static bool video_property_changed = false;
static u32 video_notify_flag = 0;
static int enable_video_discontinue_report = 1;

#ifdef CONFIG_POST_PROCESS_MANAGER_PPSCALER
static u32 video_scaler_mode = 0;
static int content_top = 0, content_left = 0, content_w = 0, content_h = 0;
static int scaler_pos_changed = 0;
#endif

#ifdef CONFIG_AM_VIDEO2
void set_clone_frame_rate(unsigned int frame_rate, unsigned int delay);
#endif
int video_property_notify(int flag)
{
    video_property_changed  = flag;
    return 0;
}

#ifdef CONFIG_POST_PROCESS_MANAGER_PPSCALER
int video_scaler_notify(int flag)
{
    video_scaler_mode  = flag;
    video_property_changed = true;
    return 0;
}

u32 amvideo_get_scaler_para(int* x, int* y, int* w, int* h, u32* ratio)
{
    *x = content_left;
    *y = content_top;
    *w = content_w;
    *h = content_h;
    //*ratio = 100;
    return video_scaler_mode;
}

void amvideo_set_scaler_para(int x, int y, int w, int h,int flag)
{
    mutex_lock(&video_module_mutex);
    if(w < 2)
        w = 0;
    if(h < 2)
        h = 0;
    if(flag){
        if((content_left!=x)||(content_top!=y)||(content_w!=w)||(content_h!=h))
            scaler_pos_changed = 1;
        content_left = x;
        content_top = y;
        content_w = w;
        content_h = h;
    }else{
        vpp_set_video_layer_position(x, y, w, h);
    }
    video_property_changed = true;
    mutex_unlock(&video_module_mutex);
    return;
}


u32 amvideo_get_scaler_mode(void)
{
    return video_scaler_mode;
}
#endif

/* display canvas */
static u32 disp_canvas_index[6] = {
    DISPLAY_CANVAS_BASE_INDEX,
    DISPLAY_CANVAS_BASE_INDEX + 1,
    DISPLAY_CANVAS_BASE_INDEX + 2,
    DISPLAY_CANVAS_BASE_INDEX + 3,
    DISPLAY_CANVAS_BASE_INDEX + 4,
    DISPLAY_CANVAS_BASE_INDEX + 5,
};
static u32 disp_canvas[2];
static u32 post_canvas = 0;
static ulong keep_y_addr = 0, *keep_y_addr_remap = NULL;
static ulong keep_u_addr = 0, *keep_u_addr_remap = NULL;
static ulong keep_v_addr = 0, *keep_v_addr_remap = NULL;
#define Y_BUFFER_SIZE   0x400000 // for 1920*1088
#define U_BUFFER_SIZE   0x100000  //compatible with NV21
#define V_BUFFER_SIZE   0x80000
#ifdef CONFIG_KEEP_FRAME_RESERVED
static uint y_buffer_size = 0;
static uint u_buffer_size = 0;
static uint v_buffer_size = 0;
#endif

/* zoom information */
static u32 zoom_start_x_lines;
static u32 zoom_end_x_lines;
static u32 zoom_start_y_lines;
static u32 zoom_end_y_lines;

/* wide settings */
static u32 wide_setting;

/* black out policy */
#if defined(CONFIG_JPEGLOGO)
static u32 blackout = 0;
#else
static u32 blackout = 1;
#endif
static u32 force_blackout = 0;

/* disable video */
static u32 disable_video = VIDEO_DISABLE_NONE;

/* test screen*/
static u32 test_screen = 0;

#ifdef SLOW_SYNC_REPEAT
/* video frame repeat count */
static u32 frame_repeat_count = 0;
#endif

/* vout */
static const vinfo_t *vinfo = NULL;

/* config */
static vframe_t *cur_dispbuf = NULL;
static vframe_t vf_local;
static u32 vsync_pts_inc;
static u32 vsync_pts_inc_upint = 0;

/* frame rate calculate */
static u32 last_frame_count = 0;
static u32 frame_count = 0;
static u32 last_frame_time = 0;
static u32 timer_count  =0 ;
static u32 vsync_count  =0 ;
static vpp_frame_par_t *cur_frame_par, *next_frame_par;
static vpp_frame_par_t frame_parms[2];

/* vsync pass flag */
static u32 wait_sync;

#ifdef FIQ_VSYNC
static bridge_item_t vsync_fiq_bridge;
#endif

/* trickmode i frame*/
u32 trickmode_i = 0;

/* trickmode ff/fb */
u32 trickmode_fffb = 0;
atomic_t trickmode_framedone = ATOMIC_INIT(0);
atomic_t video_unreg_flag = ATOMIC_INIT(0);
int trickmode_duration = 0;
int trickmode_duration_count = 0;
/* last_playback_filename */
char file_name[512];

/* video freerun mode */
#define FREERUN_NONE    0   // no freerun mode
#define FREERUN_NODUR   1   // freerun without duration
#define FREERUN_DUR     2   // freerun with duration
static u32 freerun_mode;

static const f2v_vphase_type_t vpp_phase_table[4][3] = {
    {F2V_P2IT,  F2V_P2IB,  F2V_P2P },   /* VIDTYPE_PROGRESSIVE */
    {F2V_IT2IT, F2V_IT2IB, F2V_IT2P},   /* VIDTYPE_INTERLACE_TOP */
    {F2V_P2IT,  F2V_P2IB,  F2V_P2P },
    {F2V_IB2IT, F2V_IB2IB, F2V_IB2P}    /* VIDTYPE_INTERLACE_BOTTOM */
};

static const u8 skip_tab[6] = { 0x24, 0x04, 0x68, 0x48, 0x28, 0x08 };
/* wait queue for poll */
static wait_queue_head_t amvideo_trick_wait;

static u32 vpts_ref = 0;
static u32 video_frame_repeat_count = 0;
static u32 smooth_sync_enable = 0;
#ifdef CONFIG_AM_VIDEO2
static int video_play_clone_rate = 60;
static int android_clone_rate = 30;
static int noneseamless_play_clone_rate = 5;
#endif

#if 0
/* video enhancement */
static struct ve_bext_s ve_bext;
static struct ve_dnlp_s ve_dnlp;
static struct ve_hsvs_s ve_hsvs;
static struct ve_ccor_s ve_ccor;
static struct ve_benh_s ve_demo;
static struct ve_demo_s ve_demo;

typedef struct ve_regs_s {
    unsigned val  : 32;
    unsigned reg  : 14;
    unsigned port :  2; // port port_addr            port_data            remark
                        // 0    NA                   NA                   direct access
                        // 1    VPP_CHROMA_ADDR_PORT VPP_CHROMA_DATA_PORT CM port registers
                        // 2    NA                   NA                   reserved
                        // 3    NA                   NA                   reserved
    unsigned bit  :  5;
    unsigned wid  :  5;
    unsigned mode :  1; // 0:read, 1:write
    unsigned rsv  :  5;
} ve_regs_t;

static uchar ve_dnlp_tgt[64], ve_dnlp_rt;
static ulong ve_dnlp_lpf[64], ve_dnlp_reg[16];

static ulong ve_benh_ve_benh_inv[32][2] = { // [0]: inv_10_0, [1]: inv_11
    {2047, 1}, {2047, 1}, {   0, 1}, {1365, 0}, {1024, 0}, { 819, 0}, { 683, 0}, { 585, 0},
    { 512, 0}, { 455, 0}, { 410, 0}, { 372, 0}, { 341, 0}, { 315, 0}, { 293, 0}, { 273, 0},
    { 256, 0}, { 241, 0}, { 228, 0}, { 216, 0}, { 205, 0}, { 195, 0}, { 186, 0}, { 178, 0},
    { 171, 0}, { 164, 0}, { 158, 0}, { 152, 0}, { 146, 0}, { 141, 0}, { 137, 0}, { 132, 0},
};

static ulong ve_reg_limit(ulong val, ulong wid)
{
    if (val < (1 << wid)) {
        return(val);
    } else {
        return((1 << wid) - 1);
    }
}

#endif

/*********************************************************/
static inline vframe_t *video_vf_peek(void)
{
#ifdef CONFIG_AM_DEINTERLACE
    int deinterlace_mode = get_deinterlace_mode();

    if (deinterlace_mode == 2) {
        int di_pre_recycle_buf = get_di_pre_recycle_buf();

        if (di_pre_recycle_buf == 1) {
            return peek_di_out_buf();
        } else if (di_pre_recycle_buf == 0) {
            return vf_peek(RECEIVER_NAME);
        }
    } else {
        return vf_peek(RECEIVER_NAME);
    }

    return NULL;
#else
    return vf_peek(RECEIVER_NAME);
#endif
}

static inline vframe_t *video_vf_get(void)
{
    vframe_t *vf = NULL;
#ifdef CONFIG_AM_DEINTERLACE
    int deinterlace_mode = get_deinterlace_mode();

    if (deinterlace_mode == 2) {
        int di_pre_recycle_buf = get_di_pre_recycle_buf();

        if (di_pre_recycle_buf == 1) {
            vframe_t *disp_buf = peek_di_out_buf();

            if (disp_buf) {
                set_post_di_mem(disp_buf->blend_mode);
                inc_field_counter();
            }

            return disp_buf;
        } else if (di_pre_recycle_buf == 0) {
            vf = vf_get(RECEIVER_NAME);

                if (vf) video_notify_flag |= VIDEO_NOTIFY_PROVIDER_GET;
                return vf;
            }
    } else {
        vf = vf_get(RECEIVER_NAME);

            if (vf) video_notify_flag |= VIDEO_NOTIFY_PROVIDER_GET;
            return vf;
        }

    return NULL;
#else
    vf = vf_get(RECEIVER_NAME);

    if (vf) video_notify_flag |= VIDEO_NOTIFY_PROVIDER_GET;
    return vf;

#endif
}
static int  vf_get_states(vframe_states_t *states)
{
    int ret = -1;
    unsigned long flags;
    struct vframe_provider_s *vfp;
    vfp = vf_get_provider(RECEIVER_NAME);
    spin_lock_irqsave(&lock, flags);
    if (vfp && vfp->ops && vfp->ops->vf_states) {
        ret=vfp->ops->vf_states(states, vfp->op_arg);
    }
    spin_unlock_irqrestore(&lock, flags);
    return ret;
}

static inline void video_vf_put(vframe_t *vf)
{
    struct vframe_provider_s *vfp = vf_get_provider(RECEIVER_NAME);
#ifdef CONFIG_AM_DEINTERLACE
    int deinterlace_mode = get_deinterlace_mode();

    if (deinterlace_mode == 2) {
        int di_pre_recycle_buf = get_di_pre_recycle_buf();

        if (di_pre_recycle_buf == 0) {
            if (vfp) {
                vf_put(vf, RECEIVER_NAME);
                video_notify_flag |= VIDEO_NOTIFY_PROVIDER_PUT;
            }
        }
    } else {
        if (vfp) {
            vf_put(vf, RECEIVER_NAME);
            video_notify_flag |= VIDEO_NOTIFY_PROVIDER_PUT;
        }
    }
#else
    if (vfp) {
        vf_put(vf, RECEIVER_NAME);
        video_notify_flag |= VIDEO_NOTIFY_PROVIDER_PUT;
    }
#endif
}

static void vpp_settings_h(vpp_frame_par_t *framePtr)
{
    vppfilter_mode_t *vpp_filter = &framePtr->vpp_filter;
    u32 r1, r2, r3;

    r1 = framePtr->VPP_hsc_linear_startp - framePtr->VPP_hsc_startp;
    r2 = framePtr->VPP_hsc_linear_endp   - framePtr->VPP_hsc_startp;
    r3 = framePtr->VPP_hsc_endp          - framePtr->VPP_hsc_startp;

    WRITE_MPEG_REG(VPP_POSTBLEND_VD1_H_START_END,
                   ((framePtr->VPP_hsc_startp & VPP_VD_SIZE_MASK) << VPP_VD1_START_BIT) |
                   ((framePtr->VPP_hsc_endp   & VPP_VD_SIZE_MASK) << VPP_VD1_END_BIT));

    WRITE_MPEG_REG(VPP_BLEND_VD2_H_START_END,
                   ((framePtr->VPP_hsc_startp & VPP_VD_SIZE_MASK) << VPP_VD1_START_BIT) |
                   ((framePtr->VPP_hsc_endp   & VPP_VD_SIZE_MASK) << VPP_VD1_END_BIT));

    WRITE_MPEG_REG(VPP_HSC_REGION12_STARTP,
                   (0 << VPP_REGION1_BIT) |
                   ((r1 & VPP_REGION_MASK) << VPP_REGION2_BIT));

    WRITE_MPEG_REG(VPP_HSC_REGION34_STARTP,
                   ((r2 & VPP_REGION_MASK) << VPP_REGION3_BIT) |
                   ((r3 & VPP_REGION_MASK) << VPP_REGION4_BIT));
    WRITE_MPEG_REG(VPP_HSC_REGION4_ENDP, r3);

    WRITE_MPEG_REG(VPP_HSC_START_PHASE_STEP,
                   vpp_filter->vpp_hf_start_phase_step);

    WRITE_MPEG_REG(VPP_HSC_REGION1_PHASE_SLOPE,
                   vpp_filter->vpp_hf_start_phase_slope);

    WRITE_MPEG_REG(VPP_HSC_REGION3_PHASE_SLOPE,
                   vpp_filter->vpp_hf_end_phase_slope);

    WRITE_MPEG_REG(VPP_LINE_IN_LENGTH, framePtr->VPP_line_in_length_);
    WRITE_MPEG_REG(VPP_PREBLEND_H_SIZE, framePtr->VPP_line_in_length_);
}

static void vpp_settings_v(vpp_frame_par_t *framePtr)
{
    vppfilter_mode_t *vpp_filter = &framePtr->vpp_filter;
    u32 r;

    r = framePtr->VPP_vsc_endp - framePtr->VPP_vsc_startp;

    WRITE_MPEG_REG(VPP_POSTBLEND_VD1_V_START_END,
                   ((framePtr->VPP_vsc_startp & VPP_VD_SIZE_MASK) << VPP_VD1_START_BIT) |
                   ((framePtr->VPP_vsc_endp   & VPP_VD_SIZE_MASK) << VPP_VD1_END_BIT));

    if((framePtr->VPP_post_blend_vd_v_end_ - framePtr->VPP_post_blend_vd_v_start_+1)>1080){
        WRITE_MPEG_REG(VPP_PREBLEND_VD1_V_START_END,
                   ((framePtr->VPP_post_blend_vd_v_start_ & VPP_VD_SIZE_MASK) << VPP_VD1_START_BIT) |
                   ((framePtr->VPP_post_blend_vd_v_end_   & VPP_VD_SIZE_MASK) << VPP_VD1_END_BIT));
    }else{
        WRITE_MPEG_REG(VPP_PREBLEND_VD1_V_START_END,
                   ((0 & VPP_VD_SIZE_MASK) << VPP_VD1_START_BIT) |
                   ((1079 & VPP_VD_SIZE_MASK) << VPP_VD1_END_BIT));
    }
    WRITE_MPEG_REG(VPP_BLEND_VD2_V_START_END,
                   (((framePtr->VPP_vsc_endp / 2) & VPP_VD_SIZE_MASK) << VPP_VD1_START_BIT) |
                   (((framePtr->VPP_vsc_endp) & VPP_VD_SIZE_MASK) << VPP_VD1_END_BIT));

    WRITE_MPEG_REG(VPP_VSC_REGION12_STARTP, 0);
    WRITE_MPEG_REG(VPP_VSC_REGION34_STARTP,
                   ((r & VPP_REGION_MASK) << VPP_REGION3_BIT) |
                   ((r & VPP_REGION_MASK) << VPP_REGION4_BIT));
    WRITE_MPEG_REG(VPP_VSC_REGION4_ENDP, r);

    WRITE_MPEG_REG(VPP_VSC_START_PHASE_STEP,
                   vpp_filter->vpp_vsc_start_phase_step);
}

static void zoom_display_horz(void)
{
    WRITE_MPEG_REG(VD1_IF0_LUMA_X0,
                   (zoom_start_x_lines << VDIF_PIC_START_BIT) |
                   (zoom_end_x_lines   << VDIF_PIC_END_BIT));

    WRITE_MPEG_REG(VD1_IF0_CHROMA_X0,
                   (zoom_start_x_lines / 2 << VDIF_PIC_START_BIT) |
                   (zoom_end_x_lines / 2   << VDIF_PIC_END_BIT));

    WRITE_MPEG_REG(VD1_IF0_LUMA_X1,
                   (zoom_start_x_lines << VDIF_PIC_START_BIT) |
                   (zoom_end_x_lines   << VDIF_PIC_END_BIT));

    WRITE_MPEG_REG(VD1_IF0_CHROMA_X1,
                   (zoom_start_x_lines / 2 << VDIF_PIC_START_BIT) |
                   (zoom_end_x_lines / 2   << VDIF_PIC_END_BIT));

    WRITE_MPEG_REG(VIU_VD1_FMT_W,
                   ((zoom_end_x_lines - zoom_start_x_lines + 1) << VD1_FMT_LUMA_WIDTH_BIT) |
                   ((zoom_end_x_lines / 2 - zoom_start_x_lines / 2 + 1) << VD1_FMT_CHROMA_WIDTH_BIT));

    WRITE_MPEG_REG(VD2_IF0_LUMA_X0,
                   (zoom_start_x_lines << VDIF_PIC_START_BIT) |
                   (zoom_end_x_lines   << VDIF_PIC_END_BIT));

    WRITE_MPEG_REG(VD2_IF0_CHROMA_X0,
                   (zoom_start_x_lines / 2 << VDIF_PIC_START_BIT) |
                   (zoom_end_x_lines / 2   << VDIF_PIC_END_BIT));

    WRITE_MPEG_REG(VD2_IF0_LUMA_X1,
                   (zoom_start_x_lines << VDIF_PIC_START_BIT) |
                   (zoom_end_x_lines   << VDIF_PIC_END_BIT));

    WRITE_MPEG_REG(VD2_IF0_CHROMA_X1,
                   (zoom_start_x_lines / 2 << VDIF_PIC_START_BIT) |
                   (zoom_end_x_lines / 2   << VDIF_PIC_END_BIT));

    WRITE_MPEG_REG(VIU_VD2_FMT_W,
                   ((zoom_end_x_lines - zoom_start_x_lines + 1) << VD1_FMT_LUMA_WIDTH_BIT) |
                   ((zoom_end_x_lines / 2 - zoom_start_x_lines / 2 + 1) << VD1_FMT_CHROMA_WIDTH_BIT));
}

static void zoom_display_vert(void)
{
    if ((cur_dispbuf) && (cur_dispbuf->type & VIDTYPE_MVC)) {
        WRITE_MPEG_REG(VD1_IF0_LUMA_Y0,
                       (zoom_start_y_lines * 2 << VDIF_PIC_START_BIT) |
                       (zoom_end_y_lines * 2   << VDIF_PIC_END_BIT));

        WRITE_MPEG_REG(VD1_IF0_CHROMA_Y0,
                       ((zoom_start_y_lines) << VDIF_PIC_START_BIT) |
                       ((zoom_end_y_lines)   << VDIF_PIC_END_BIT));

        WRITE_MPEG_REG(VD2_IF0_LUMA_Y0,
                       (zoom_start_y_lines * 2 << VDIF_PIC_START_BIT) |
                       (zoom_end_y_lines * 2   << VDIF_PIC_END_BIT));

        WRITE_MPEG_REG(VD2_IF0_CHROMA_Y0,
                       ((zoom_start_y_lines) << VDIF_PIC_START_BIT) |
                       ((zoom_end_y_lines)   << VDIF_PIC_END_BIT));
    } else {
        WRITE_MPEG_REG(VD1_IF0_LUMA_Y0,
                       (zoom_start_y_lines << VDIF_PIC_START_BIT) |
                       (zoom_end_y_lines   << VDIF_PIC_END_BIT));

        WRITE_MPEG_REG(VD1_IF0_CHROMA_Y0,
                       ((zoom_start_y_lines / 2) << VDIF_PIC_START_BIT) |
                       ((zoom_end_y_lines / 2)   << VDIF_PIC_END_BIT));

        WRITE_MPEG_REG(VD1_IF0_LUMA_Y1,
                       (zoom_start_y_lines << VDIF_PIC_START_BIT) |
                       (zoom_end_y_lines << VDIF_PIC_END_BIT));

        WRITE_MPEG_REG(VD1_IF0_CHROMA_Y1,
                       ((zoom_start_y_lines / 2) << VDIF_PIC_START_BIT) |
                       ((zoom_end_y_lines / 2) << VDIF_PIC_END_BIT));
    }
}

static void vsync_toggle_frame(vframe_t *vf)
{
    u32 first_picture = 0;
    frame_count++;
#ifdef CONFIG_AM_DEINTERLACE
    int deinterlace_mode = get_deinterlace_mode();
#endif
     if(debug_flag& DEBUG_FLAG_PRINT_TOGGLE_FRAME){
        printk("%s()\n", __func__);
     }

    if (trickmode_i || trickmode_fffb) {
        trickmode_duration_count = trickmode_duration;
    }

    if(vf->early_process_fun){
        if(vf->early_process_fun(vf->private_data) == 1){
            video_property_changed = true;
        }
    }
    else{
#ifndef CONFIG_AM_DEINTERLACE
        if(READ_MPEG_REG(DI_IF1_GEN_REG)&0x1){
            //disable post di
    	      WRITE_MPEG_REG(DI_POST_CTRL, 0x3 << 30);
            WRITE_MPEG_REG(DI_POST_SIZE, (32-1) | ((128-1) << 16));
	          WRITE_MPEG_REG(DI_IF1_GEN_REG, READ_MPEG_REG(DI_IF1_GEN_REG) & 0xfffffffe);
	      }
#endif
    }

    timer_count = 0 ;
    if ((vf->width == 0) && (vf->height == 0)) {
        amlog_level(LOG_LEVEL_ERROR, "Video: invalid frame dimension\n");
        return;
    }
    if ((cur_dispbuf) && (cur_dispbuf != &vf_local) && (cur_dispbuf != vf)
    &&(video_property_changed != 2)) {
        if(cur_dispbuf->source_type == VFRAME_SOURCE_TYPE_OSD){
            if (osd_prov && osd_prov->ops && osd_prov->ops->put){
                osd_prov->ops->put(cur_dispbuf, osd_prov->op_arg);
                if(debug_flag& DEBUG_FLAG_BLACKOUT){
                    printk("[video4osd] pre vframe is osd_vframe, put it\n");
                }
            }
            first_picture = 1;
            if(debug_flag& DEBUG_FLAG_BLACKOUT){
                printk("[video4osd] pre vframe is osd_vframe, clear it to NULL\n");
            }
        }
        else{
            video_vf_put(cur_dispbuf);
        }

    } else {
        first_picture = 1;
    }

    if (video_property_changed) {
        video_property_changed = false;
        first_picture = 1;
    }

    if(debug_flag& DEBUG_FLAG_BLACKOUT){
        if(first_picture){
            printk("[video4osd] first %s picture {%d,%d}\n", (vf->source_type==VFRAME_SOURCE_TYPE_OSD)?"OSD":"", vf->width, vf->height);
        }
    }
    /* switch buffer */
    post_canvas = vf->canvas0Addr;
    canvas_copy(vf->canvas0Addr & 0xff, disp_canvas_index[0]);
    canvas_copy((vf->canvas0Addr >> 8) & 0xff, disp_canvas_index[1]);
    canvas_copy((vf->canvas0Addr >> 16) & 0xff, disp_canvas_index[2]);
    canvas_copy(vf->canvas1Addr & 0xff, disp_canvas_index[3]);
    canvas_copy((vf->canvas1Addr >> 8) & 0xff, disp_canvas_index[4]);
    canvas_copy((vf->canvas1Addr >> 16) & 0xff, disp_canvas_index[5]);

    WRITE_MPEG_REG(VD1_IF0_CANVAS0, disp_canvas[0]);
    WRITE_MPEG_REG(VD1_IF0_CANVAS1, disp_canvas[1]);
    WRITE_MPEG_REG(VD2_IF0_CANVAS0, disp_canvas[1]);
    WRITE_MPEG_REG(VD2_IF0_CANVAS1, disp_canvas[1]);

    /* set video PTS */
    if (cur_dispbuf != vf) {
        if(vf->source_type != VFRAME_SOURCE_TYPE_OSD){
            if (vf->pts != 0) {
                amlog_mask(LOG_MASK_TIMESTAMP,
                           "vpts to vf->pts: 0x%x, scr: 0x%x, abs_scr: 0x%x\n",
                           vf->pts, timestamp_pcrscr_get(), READ_MPEG_REG(SCR_HIU));

                timestamp_vpts_set(vf->pts);
            } else if (cur_dispbuf) {
                amlog_mask(LOG_MASK_TIMESTAMP,
                           "vpts inc: 0x%x, scr: 0x%x, abs_scr: 0x%x\n",
                           timestamp_vpts_get() + DUR2PTS(cur_dispbuf->duration),
                           timestamp_pcrscr_get(), READ_MPEG_REG(SCR_HIU));

                timestamp_vpts_inc(DUR2PTS(cur_dispbuf->duration));

                vpts_remainder += DUR2PTS_RM(cur_dispbuf->duration);
                if (vpts_remainder >= 0xf) {
                    vpts_remainder -= 0xf;
                    timestamp_vpts_inc(-1);
                }
            }
        }
        else{
            first_picture = 1;
            if(debug_flag& DEBUG_FLAG_BLACKOUT){
                printk("[video4osd] cur vframe is osd_vframe, do not set PTS\n");
            }
        }
        vf->type_backup = vf->type;
    }

    /* enable new config on the new frames */
    if ((first_picture) ||
        (cur_dispbuf->bufWidth != vf->bufWidth) ||
        (cur_dispbuf->width != vf->width) ||
        (cur_dispbuf->height != vf->height) ||
        (cur_dispbuf->ratio_control != vf->ratio_control) ||
        ((cur_dispbuf->type_backup & VIDTYPE_INTERLACE) !=
         (vf->type_backup & VIDTYPE_INTERLACE))) {
        amlog_mask(LOG_MASK_FRAMEINFO,
                   "%s %dx%d ar=0x%x\n",
                   ((vf->type & VIDTYPE_TYPEMASK) == VIDTYPE_INTERLACE_TOP) ?
                   "interlace-top" :
                   ((vf->type & VIDTYPE_TYPEMASK) == VIDTYPE_INTERLACE_BOTTOM) ?
                   "interlace-bottom" :
                   "progressive",
                   vf->width,
                   vf->height,
                   vf->ratio_control);

        next_frame_par = (&frame_parms[0] == next_frame_par) ?
                         &frame_parms[1] : &frame_parms[0];

#ifdef CONFIG_AM_DEINTERLACE
        if ((deinterlace_mode != 0)
            && (vf->type & VIDTYPE_INTERLACE)
#if defined(CONFIG_AM_DEINTERLACE_SD_ONLY)
            && (vf->width <= 720)
#endif
           ) {
            vf->type &= ~VIDTYPE_TYPEMASK;

            if (deinterlace_mode == 1) {
                vf->type |= VIDTYPE_VIU_FIELD;
                inc_field_counter();
            }
        }
#endif

        vpp_set_filters(wide_setting, vf, next_frame_par, vinfo);

        /* apply new vpp settings */
        frame_par_ready_to_set = 1;
    } else {
#ifdef CONFIG_AM_DEINTERLACE
        if ((deinterlace_mode != 0)
            && (vf->type & VIDTYPE_INTERLACE)
#if defined(CONFIG_AM_DEINTERLACE_SD_ONLY)
            && (vf->width <= 720)
#endif
           ) {
            vf->type &= ~VIDTYPE_TYPEMASK;

            if (deinterlace_mode == 1) {
                vf->type |= VIDTYPE_VIU_FIELD;
                inc_field_counter();
            }
        }
#endif
    }

    cur_dispbuf = vf;

    if ((vf->type & VIDTYPE_NO_VIDEO_ENABLE) == 0) {
        if (disable_video == VIDEO_DISABLE_FORNEXT) {
            EnableVideoLayer();
            disable_video = VIDEO_DISABLE_NONE;
        }
        if (first_picture && (disable_video != VIDEO_DISABLE_NORMAL)) {
            EnableVideoLayer();

            if (cur_dispbuf->type & VIDTYPE_MVC)
                EnableVideoLayer2();
        }
    }

    if (first_picture) {
        frame_par_ready_to_set = 1;

#ifdef CONFIG_AM_DEINTERLACE
        if (deinterlace_mode != 2) {
            disable_deinterlace();
        } else {
            disable_post_deinterlace();
        }
#endif
#ifdef VIDEO_PTS_CHASE
	av_sync_flag=0;
#endif
    }
}

static void viu_set_dcu(vpp_frame_par_t *frame_par, vframe_t *vf)
{
    u32 r;
    u32 vphase, vini_phase;
    u32 pat, loop;
    static const u32 vpat[] = {0, 0x8, 0x9, 0xa, 0xb, 0xc};

    r = (3 << VDIF_URGENT_BIT) |
        (17 << VDIF_HOLD_LINES_BIT) |
        VDIF_FORMAT_SPLIT  |
        VDIF_CHRO_RPT_LAST |
        VDIF_ENABLE |
        VDIF_RESET_ON_GO_FIELD;

    if ((vf->type & VIDTYPE_VIU_SINGLE_PLANE) == 0) {
        r |= VDIF_SEPARATE_EN;
    } else {
        if (vf->type & VIDTYPE_VIU_422) {
            r |= VDIF_FORMAT_422;
        } else {
            r |= VDIF_FORMAT_RGB888_YUV444 | VDIF_DEMUX_MODE_RGB_444;
        }
    }

    WRITE_MPEG_REG(VD1_IF0_GEN_REG, r);
    WRITE_MPEG_REG(VD2_IF0_GEN_REG, r);

#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6
    if (vf->type & VIDTYPE_VIU_NV21) {
        WRITE_MPEG_REG_BITS(VD1_IF0_GEN_REG2, 1,0,1);
    } else {
        WRITE_MPEG_REG_BITS(VD1_IF0_GEN_REG2, 0,0,1);
    }
#endif

    /* chroma formatter */
    if (vf->type & VIDTYPE_VIU_444) {
        WRITE_MPEG_REG(VIU_VD1_FMT_CTRL, HFORMATTER_YC_RATIO_1_1);
        WRITE_MPEG_REG(VIU_VD2_FMT_CTRL, HFORMATTER_YC_RATIO_1_1);
    } else if (vf->type & VIDTYPE_VIU_FIELD) {
        vini_phase = 0xc << VFORMATTER_INIPHASE_BIT;
        vphase = ((vf->type & VIDTYPE_VIU_422) ? 0x10 : 0x08) << VFORMATTER_PHASE_BIT;

        WRITE_MPEG_REG(VIU_VD1_FMT_CTRL,
                       HFORMATTER_YC_RATIO_2_1 | HFORMATTER_EN |
                       VFORMATTER_RPTLINE0_EN | vini_phase | vphase | VFORMATTER_EN);

        WRITE_MPEG_REG(VIU_VD2_FMT_CTRL,
                       HFORMATTER_YC_RATIO_2_1 | HFORMATTER_EN |
                       VFORMATTER_RPTLINE0_EN | vini_phase | vphase | VFORMATTER_EN);
    } else if (vf->type & VIDTYPE_MVC) {
        WRITE_MPEG_REG(VIU_VD1_FMT_CTRL,
                       HFORMATTER_YC_RATIO_2_1 |
                       HFORMATTER_EN |
                       VFORMATTER_RPTLINE0_EN |
                       (0xe << VFORMATTER_INIPHASE_BIT) |
                       (((vf->type & VIDTYPE_VIU_422) ? 0x10 : 0x08) << VFORMATTER_PHASE_BIT) |
                       VFORMATTER_EN);
        WRITE_MPEG_REG(VIU_VD2_FMT_CTRL,
                       HFORMATTER_YC_RATIO_2_1 |
                       HFORMATTER_EN |
                       VFORMATTER_RPTLINE0_EN |
                       (0xa << VFORMATTER_INIPHASE_BIT) |
                       (((vf->type & VIDTYPE_VIU_422) ? 0x10 : 0x08) << VFORMATTER_PHASE_BIT) |
                       VFORMATTER_EN);
    } else if ((vf->type & VIDTYPE_INTERLACE) &&
               (((vf->type & VIDTYPE_TYPEMASK) == VIDTYPE_INTERLACE_TOP))) {
        WRITE_MPEG_REG(VIU_VD1_FMT_CTRL,
                       HFORMATTER_YC_RATIO_2_1 |
                       HFORMATTER_EN |
                       VFORMATTER_RPTLINE0_EN |
                       (0xe << VFORMATTER_INIPHASE_BIT) |
                       (((vf->type & VIDTYPE_VIU_422) ? 0x10 : 0x08) << VFORMATTER_PHASE_BIT) |
                       VFORMATTER_EN);

        WRITE_MPEG_REG(VIU_VD2_FMT_CTRL,
                       HFORMATTER_YC_RATIO_2_1 |
                       HFORMATTER_EN |
                       VFORMATTER_RPTLINE0_EN |
                       (0xe << VFORMATTER_INIPHASE_BIT) |
                       (((vf->type & VIDTYPE_VIU_422) ? 0x10 : 0x08) << VFORMATTER_PHASE_BIT) |
                       VFORMATTER_EN);
    } else {
        WRITE_MPEG_REG(VIU_VD1_FMT_CTRL,
                       HFORMATTER_YC_RATIO_2_1 |
                       HFORMATTER_EN |
                       VFORMATTER_RPTLINE0_EN |
                       (0xa << VFORMATTER_INIPHASE_BIT) |
                       (((vf->type & VIDTYPE_VIU_422) ? 0x10 : 0x08) << VFORMATTER_PHASE_BIT) |
                       VFORMATTER_EN);

        WRITE_MPEG_REG(VIU_VD2_FMT_CTRL,
                       HFORMATTER_YC_RATIO_2_1 |
                       HFORMATTER_EN |
                       VFORMATTER_RPTLINE0_EN |
                       (0xa << VFORMATTER_INIPHASE_BIT) |
                       (((vf->type & VIDTYPE_VIU_422) ? 0x10 : 0x08) << VFORMATTER_PHASE_BIT) |
                       VFORMATTER_EN);
    }

    /* LOOP/SKIP pattern */
    pat = vpat[frame_par->vscale_skip_count];

    if (vf->type & VIDTYPE_VIU_FIELD) {
        loop = 0;

        if (vf->type & VIDTYPE_INTERLACE) {
            pat = vpat[frame_par->vscale_skip_count >> 1];
        }
    } else if (vf->type & VIDTYPE_MVC) {
        loop = 0x11;
        pat = 0x80;
    } else if ((vf->type & VIDTYPE_TYPEMASK) == VIDTYPE_INTERLACE_TOP) {
        loop = 0x11;
        pat <<= 4;
    } else {
        loop = 0;
    }

    WRITE_MPEG_REG(VD1_IF0_RPT_LOOP,
                   (loop << VDIF_CHROMA_LOOP1_BIT) |
                   (loop << VDIF_LUMA_LOOP1_BIT)   |
                   (loop << VDIF_CHROMA_LOOP0_BIT) |
                   (loop << VDIF_LUMA_LOOP0_BIT));

    WRITE_MPEG_REG(VD2_IF0_RPT_LOOP,
                   (loop << VDIF_CHROMA_LOOP1_BIT) |
                   (loop << VDIF_LUMA_LOOP1_BIT)   |
                   (loop << VDIF_CHROMA_LOOP0_BIT) |
                   (loop << VDIF_LUMA_LOOP0_BIT));

    WRITE_MPEG_REG(VD1_IF0_LUMA0_RPT_PAT,   pat);
    WRITE_MPEG_REG(VD1_IF0_CHROMA0_RPT_PAT, pat);
    WRITE_MPEG_REG(VD1_IF0_LUMA1_RPT_PAT,   pat);
    WRITE_MPEG_REG(VD1_IF0_CHROMA1_RPT_PAT, pat);

    if (vf->type & VIDTYPE_MVC)
        pat = 0x88;

    WRITE_MPEG_REG(VD2_IF0_LUMA0_RPT_PAT,   pat);
    WRITE_MPEG_REG(VD2_IF0_CHROMA0_RPT_PAT, pat);
    WRITE_MPEG_REG(VD2_IF0_LUMA1_RPT_PAT,   pat);
    WRITE_MPEG_REG(VD2_IF0_CHROMA1_RPT_PAT, pat);

    /* picture 0/1 control */
    if (((vf->type & VIDTYPE_INTERLACE) == 0) &&
        ((vf->type & VIDTYPE_VIU_FIELD) == 0) &&
        ((vf->type & VIDTYPE_MVC) == 0)) {
        /* progressive frame in two pictures */
        WRITE_MPEG_REG(VD1_IF0_LUMA_PSEL,
                       (2 << 26) |    /* two pic mode */
                       (2 << 24) |    /* use own last line */
                       (2 << 8)  |    /* toggle pic 0 and 1, use pic0 first */
                       (0x01));       /* loop pattern */
        WRITE_MPEG_REG(VD1_IF0_CHROMA_PSEL,
                       (2 << 26) |    /* two pic mode */
                       (2 << 24) |    /* use own last line */
                       (2 << 8)  |    /* toggle pic 0 and 1, use pic0 first */
                       (0x01));       /* loop pattern */
    } else {
        WRITE_MPEG_REG(VD1_IF0_LUMA_PSEL, 0);
        WRITE_MPEG_REG(VD1_IF0_CHROMA_PSEL, 0);
        WRITE_MPEG_REG(VD2_IF0_LUMA_PSEL, 0);
        WRITE_MPEG_REG(VD2_IF0_CHROMA_PSEL, 0);
    }
}

#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6
static int detect_vout_type(void)
{
    int vout_type = VOUT_TYPE_PROG;

    if ((vinfo) && (vinfo->field_height != vinfo->height)) {
        switch (vinfo->mode) {
            case VMODE_480I:
            case VMODE_480CVBS:
            case VMODE_576I:
            case VMODE_576CVBS:
                vout_type = (READ_CBUS_REG(ENCI_INFO_READ) & (1<<29)) ?
                             VOUT_TYPE_BOT_FIELD : VOUT_TYPE_TOP_FIELD;
                break;

            case VMODE_1080I:
            case VMODE_1080I_50HZ:
                //vout_type = (((READ_CBUS_REG(ENCI_INFO_READ) >> 16) & 0x1fff) < 562) ?
                vout_type = (((READ_CBUS_REG(ENCP_INFO_READ) >> 16) & 0x1fff) < 562) ?
                             VOUT_TYPE_TOP_FIELD : VOUT_TYPE_BOT_FIELD;
                break;

            default:
                break;
        }
    }

    return vout_type;
}

#else
static int detect_vout_type(void)
{
#if defined(CONFIG_AM_TCON_OUTPUT)
    return VOUT_TYPE_PROG;
#else
    int vout_type;
    int encp_enable = READ_MPEG_REG(ENCP_VIDEO_EN) & 1;

    if (encp_enable) {
        if (READ_MPEG_REG(ENCP_VIDEO_MODE) & (1 << 12)) {
            /* 1080I */
            if (READ_MPEG_REG(VENC_ENCP_LINE) < 562) {
                vout_type = VOUT_TYPE_TOP_FIELD;

            } else {
                vout_type = VOUT_TYPE_BOT_FIELD;
            }

        } else {
            vout_type = VOUT_TYPE_PROG;
        }

    } else {
        vout_type = (READ_MPEG_REG(VENC_STATA) & 1) ?
                    VOUT_TYPE_BOT_FIELD : VOUT_TYPE_TOP_FIELD;
    }

    return vout_type;
#endif
}
#endif

#ifdef INTERLACE_FIELD_MATCH_PROCESS
static inline bool interlace_field_type_match(int vout_type, vframe_t *vf)
{
    if (DUR2PTS(vf->duration) != vsync_pts_inc) {
        return false;
    }

    if ((vout_type == VOUT_TYPE_TOP_FIELD) &&
        ((vf->type & VIDTYPE_TYPEMASK) == VIDTYPE_INTERLACE_TOP)) {
        return true;
    } else if ((vout_type == VOUT_TYPE_BOT_FIELD) &&
               ((vf->type & VIDTYPE_TYPEMASK) == VIDTYPE_INTERLACE_BOTTOM)) {
        return true;
    }

    return false;
}
#endif

static int calc_hold_line(void)
{
    if ((READ_MPEG_REG(ENCI_VIDEO_EN) & 1) == 0) {
        return READ_MPEG_REG(ENCP_VIDEO_VAVON_BLINE) >> 1;
    } else {
        return READ_MPEG_REG(VFIFO2VD_LINE_TOP_START) >> 1;
    }
}

#ifdef SLOW_SYNC_REPEAT
/* add a new function to check if current display frame has been
displayed for its duration */
static inline bool duration_expire(vframe_t *cur_vf, vframe_t *next_vf, u32 dur)
{
    u32 pts;
    s32 dur_disp;
    static s32 rpt_tab_idx = 0;
    static const u32 rpt_tab[4] = {0x100, 0x100, 0x300, 0x300};

    if ((cur_vf == NULL) || (cur_dispbuf == &vf_local)) {
        return true;
    }

    pts = next_vf->pts;
    if (pts == 0) {
        dur_disp = DUR2PTS(cur_vf->duration);
    } else {
        dur_disp = pts - timestamp_vpts_get();
    }

    if ((dur << 8) >= (dur_disp * rpt_tab[rpt_tab_idx & 3])) {
        rpt_tab_idx = (rpt_tab_idx + 1) & 3;
        return true;
    } else {
        return false;
    }
}
#endif

#define VPTS_RESET_THRO

static inline bool vpts_expire(vframe_t *cur_vf, vframe_t *next_vf)
{
    u32 pts = next_vf->pts;
#ifdef VIDEO_PTS_CHASE
    u32 vid_pts, scr_pts;
#endif
    u32 systime;
    u32 adjust_pts, org_vpts;

    if(debug_flag & DEBUG_FLAG_TOGGLE_FRAME_PER_VSYNC){
        return true;
    }

    if (/*(cur_vf == NULL) || (cur_dispbuf == &vf_local) ||*/ debugflags & DEBUG_FLAG_FFPLAY) {
        return true;
    }

    if (FREERUN_NODUR == freerun_mode) {
        return true;
    }

    if ((trickmode_i == 1) || ((trickmode_fffb == 1))) {
        if (((0 == atomic_read(&trickmode_framedone)) || (trickmode_i == 1)) && (trickmode_duration_count <= 0)) {
            #if 0
            if (cur_vf) {
                pts = timestamp_vpts_get() + trickmode_duration;
            } else {
                return true;
            }
            #else
            return true;
            #endif
        } else {
            return false;
        }
    }

    if (next_vf->duration == 0) {

        return true;
    }

    systime = timestamp_pcrscr_get();

    if (((pts == 0) && (cur_dispbuf != &vf_local)) || (FREERUN_DUR == freerun_mode)) {
        pts = timestamp_vpts_get() + (cur_vf ? DUR2PTS(cur_vf->duration) : 0);
    }
    /* check video PTS discontinuity */
    else if ((enable_video_discontinue_report) &&
             (abs(systime - pts) > tsync_vpts_discontinuity_margin())) {
        pts = timestamp_vpts_get() + (cur_vf ? DUR2PTS(cur_vf->duration) : 0);
			//printk("system=0x%x vpts=0x%x\n", systime, timestamp_vpts_get());
        if ((int)(systime - pts) >= 0){
            tsync_avevent_locked(VIDEO_TSTAMP_DISCONTINUITY, next_vf->pts);
	    		printk(" discontinue, system=0x%x vpts=0x%x\n", systime, pts);
		    if(systime>next_vf->pts || next_vf->pts==0){// pts==0 is a keep frame maybe.
            	return true;
            }
            return false;
        }
    }

    if (vsync_pts_inc_upint) {
        vframe_states_t frame_states;

        if (vf_get_states(&frame_states) == 0) {
            if ((frame_states.buf_avail_num >= 3)) {
                return true;
            }
            else {
                if (cur_vf) {
                    pts -=  (next_vf->pts - cur_vf->pts)/ 2;
                    printk("next frame pts 0x%x, next_vf->pts 0x%x\n", pts, next_vf->pts);
                }
            } 
        }
    }

#ifdef VIDEO_PTS_CHASE
    vid_pts = timestamp_vpts_get();
    scr_pts = timestamp_pcrscr_get();
    vid_pts += vsync_pts_inc;

    if(av_sync_flag){
	    if(vpts_chase){
		    if((abs(vid_pts-scr_pts)<6000) || (abs(vid_pts-scr_pts)>90000)){
			    vpts_chase = 0;
			    printk("leave vpts chase mode, diff:%d\n", vid_pts-scr_pts);
		    }
	    }else if((abs(vid_pts-scr_pts)>9000) && (abs(vid_pts-scr_pts)<90000)){
		    vpts_chase = 1;
		    if(vid_pts<scr_pts)
			    vpts_chase_pts_diff = 50;
		    else
			    vpts_chase_pts_diff = -50;
		    vpts_chase_counter = ((int)(scr_pts-vid_pts))/vpts_chase_pts_diff;
		    printk("enter vpts chase mode, diff:%d\n", vid_pts-scr_pts);
	    }else if(abs(vid_pts-scr_pts)>=90000){
		    printk("video pts discontinue, diff:%d\n", vid_pts-scr_pts);
	    }
    }else{
	    vpts_chase = 0;
    }

    if(vpts_chase){
	    u32 curr_pts = scr_pts-vpts_chase_pts_diff*vpts_chase_counter;

	    //printk("vchase pts %d, %d, %d, %d, %d\n", curr_pts, scr_pts, curr_pts-scr_pts, vid_pts, vpts_chase_counter);
	    return ((int)(curr_pts-pts)) >= 0;
    }else{
	    int aud_start = (timestamp_apts_get()!=-1);

	    if(!av_sync_flag && aud_start && (abs(scr_pts-pts)<9000) && ((int)(scr_pts-pts)<0)){
		    av_sync_flag=1;
		    printk("av sync ok\n");
	    }
	    return ((int)(scr_pts-pts)) >= 0;
    }
#else
    if(smooth_sync_enable){
        org_vpts = timestamp_vpts_get();
        if((abs(org_vpts + vsync_pts_inc - systime) < M_PTS_SMOOTH_MAX)
            && (abs(org_vpts + vsync_pts_inc - systime) > M_PTS_SMOOTH_MIN)){

            if(!video_frame_repeat_count){
                vpts_ref = org_vpts;
                video_frame_repeat_count ++;
            }

            if((int)(org_vpts + vsync_pts_inc - systime) > 0){
                adjust_pts = vpts_ref + (vsync_pts_inc - M_PTS_SMOOTH_ADJUST) * video_frame_repeat_count;
            }else{
                adjust_pts = vpts_ref + (vsync_pts_inc + M_PTS_SMOOTH_ADJUST) * video_frame_repeat_count;
            }

            return ((int)(adjust_pts - pts) >= 0);
        }

        if(video_frame_repeat_count){
            vpts_ref = 0;
            video_frame_repeat_count = 0;
        }
    }

    return ((int)(timestamp_pcrscr_get() - pts) >= 0);
#endif
}

#ifdef CONFIG_CLK81_DFS
extern int check_and_set_clk81(void);
#endif

#ifdef CONFIG_GAMMA_PROC
extern int gamma_adjust(void);
#endif


static void vsync_notify(void)
{
    if (video_notify_flag & VIDEO_NOTIFY_TRICK_WAIT) {
        wake_up_interruptible(&amvideo_trick_wait);
        video_notify_flag &= ~VIDEO_NOTIFY_TRICK_WAIT;
    }
    if (video_notify_flag & VIDEO_NOTIFY_FRAME_WAIT) {
        video_notify_flag &= ~VIDEO_NOTIFY_FRAME_WAIT;
        vf_notify_provider(RECEIVER_NAME, VFRAME_EVENT_RECEIVER_FRAME_WAIT, NULL);
    }
#ifdef CONFIG_POST_PROCESS_MANAGER_PPSCALER
    if (video_notify_flag & VIDEO_NOTIFY_POS_CHANGED) {
        video_notify_flag &= ~VIDEO_NOTIFY_POS_CHANGED;
        vf_notify_provider(RECEIVER_NAME, VFRAME_EVENT_RECEIVER_POS_CHANGED, NULL);
    }
#endif
    if (video_notify_flag & (VIDEO_NOTIFY_PROVIDER_GET | VIDEO_NOTIFY_PROVIDER_PUT)) {
            int event = 0;

            if (video_notify_flag & VIDEO_NOTIFY_PROVIDER_GET)
                event |= VFRAME_EVENT_RECEIVER_GET;
            if (video_notify_flag & VIDEO_NOTIFY_PROVIDER_PUT)
                event |= VFRAME_EVENT_RECEIVER_PUT;

        vf_notify_provider(RECEIVER_NAME, event, NULL);

        video_notify_flag &= ~(VIDEO_NOTIFY_PROVIDER_GET | VIDEO_NOTIFY_PROVIDER_PUT);
    }
#ifdef CONFIG_CLK81_DFS
    check_and_set_clk81();
#endif

#ifdef CONFIG_GAMMA_PROC
    gamma_adjust();
#endif

}

#ifdef FIQ_VSYNC
static irqreturn_t vsync_bridge_isr(int irq, void *dev_id)
{
    vsync_notify();

    return IRQ_HANDLED;
}
#endif

int get_vsync_count(unsigned char reset)
{
    if(reset)
        vsync_count = 0;
    return vsync_count;
}
EXPORT_SYMBOL(get_vsync_count);

int get_vsync_pts_inc_mode(void)
{
    return vsync_pts_inc_upint;
}
EXPORT_SYMBOL(get_vsync_pts_inc_mode);

#ifdef FIQ_VSYNC
void vsync_fisr(void)
#else
static irqreturn_t vsync_isr(int irq, void *dev_id)
#endif
{
    int hold_line;
    int enc_line;
#ifdef CONFIG_AM_DEINTERLACE
    int deinterlace_mode;
#endif
    s32 i, vout_type;
    vframe_t *vf;
#ifdef CONFIG_AM_VIDEO_LOG
    int toggle_cnt;
#endif

#ifdef CONFIG_AM_DEINTERLACE
    deinterlace_mode = get_deinterlace_mode();
#endif

#ifdef CONFIG_AM_VIDEO_LOG
    toggle_cnt = 0;
#endif
    vsync_count ++;
    timer_count ++;

    enc_line = (READ_MPEG_REG(ENCL_INFO_READ)>>16)&0x1fff;
    if(enc_line > vsync_enter_line_max)
        vsync_enter_line_max = enc_line;
#if defined(CONFIG_AM_VECM)
	/* amvecm video latch function */
	amvecm_video_latch();
#endif
    vout_type = detect_vout_type();
    hold_line = calc_hold_line();

    if (vsync_pts_inc_upint) {
        timestamp_pcrscr_inc(vsync_pts_inc + 1);
        timestamp_apts_inc(vsync_pts_inc + 1);
    } else {
        timestamp_pcrscr_inc(vsync_pts_inc);
        timestamp_apts_inc(vsync_pts_inc);
    }

    if (trickmode_duration_count > 0) {
        trickmode_duration_count -= vsync_pts_inc;
    }

#ifdef VIDEO_PTS_CHASE
    if(vpts_chase){
	    vpts_chase_counter--;
    }
#endif

#ifdef SLOW_SYNC_REPEAT
    frame_repeat_count++;
#endif

    if(smooth_sync_enable){
        if(video_frame_repeat_count){
            video_frame_repeat_count++;
        }
    }

    if (atomic_read(&video_unreg_flag))
        goto exit;

    if (osd_prov && osd_prov->ops && osd_prov->ops->get){
        vf = osd_prov->ops->get(osd_prov->op_arg);
        if(vf){
            vf->source_type = VFRAME_SOURCE_TYPE_OSD;
            vsync_toggle_frame(vf);
            if(debug_flag& DEBUG_FLAG_BLACKOUT){
                printk("[video4osd] toggle osd_vframe {%d,%d}\n", vf->width, vf->height);
            }
            goto SET_FILTER;
        }
    }

    if ((!cur_dispbuf) || (cur_dispbuf == &vf_local)) {

        vf = video_vf_peek();

        if (vf) {
            tsync_avevent_locked(VIDEO_START,
                          (vf->pts) ? vf->pts : timestamp_vpts_get());

#ifdef SLOW_SYNC_REPEAT
            frame_repeat_count = 0;
#endif

        } else if ((cur_dispbuf == &vf_local) && (video_property_changed)) {
            if (!(blackout|force_blackout)) {
        			if((READ_MPEG_REG(DI_IF1_GEN_REG)&0x1)==0)
                {
                    /* setting video display property in unregister mode */
                    u32 cur_index = READ_MPEG_REG(VD1_IF0_CANVAS0);
                    cur_dispbuf->canvas0Addr = cur_index ;
                 }
                vsync_toggle_frame(cur_dispbuf);
            } else {
                video_property_changed = false;
            }
        } else {
           goto SET_FILTER;
        }
    }

    /* buffer switch management */
    vf = video_vf_peek();

    /* setting video display property in underflow mode */
    if ((!vf) && cur_dispbuf && (video_property_changed)) {
        vsync_toggle_frame(cur_dispbuf);
    }
    if (!vf) {
        underflow++;
    }

    while (vf) {
		//timer_count = 0 ;
        if (vpts_expire(cur_dispbuf, vf)
#ifdef INTERLACE_FIELD_MATCH_PROCESS
            || interlace_field_type_match(vout_type, vf)
#endif
           ) {
            amlog_mask(LOG_MASK_TIMESTAMP,
                       "VIDEO_PTS = 0x%x, cur_dur=0x%x, next_pts=0x%x, scr = 0x%x\n",
                       timestamp_vpts_get(),
                       (cur_dispbuf) ? cur_dispbuf->duration : 0,
                       vf->pts,
                       timestamp_pcrscr_get());

            amlog_mask_if(toggle_cnt > 0, LOG_MASK_FRAMESKIP, "skipped\n");
#if defined(CONFIG_AM_VECM)
			ve_on_vs(vf);
#endif
            vf = video_vf_get();
            force_blackout = 0;

#ifdef CONFIG_POST_PROCESS_MANAGER_3D_PROCESS
            video_3d_format = vf->trans_fmt;
#endif

            vsync_toggle_frame(vf);

            if (trickmode_fffb == 1) {
                atomic_set(&trickmode_framedone, 1);
                video_notify_flag |= VIDEO_NOTIFY_TRICK_WAIT;
                break;
            }

#ifdef SLOW_SYNC_REPEAT
            frame_repeat_count = 0;
#endif
            vf = video_vf_peek();
		if (!vf) {
			next_peek_underflow++;
		}

		        if(debug_flag & DEBUG_FLAG_TOGGLE_FRAME_PER_VSYNC){
		            break;
		        }
        } else {
#ifdef SLOW_SYNC_REPEAT
            /* check if current frame's duration has expired, in this example
             * it compares current frame display duration with 1/1/1/1.5 frame duration
             * every 4 frames there will be one frame play longer than usual.
             * you can adjust this array for any slow sync control as you want.
             * The playback can be smoother than previous method.
             */
            if (duration_expire(cur_dispbuf, vf, frame_repeat_count * vsync_pts_inc) && timestamp_pcrscr_enable_state()) {
                amlog_mask(LOG_MASK_SLOWSYNC,
                           "slow sync toggle, frame_repeat_count = %d\n",
                           frame_repeat_count);
                amlog_mask(LOG_MASK_SLOWSYNC,
                           "system time = 0x%x, video time = 0x%x\n",
                           timestamp_pcrscr_get(), timestamp_vpts_get());
                vf = video_vf_get();
                vsync_toggle_frame(vf);
                frame_repeat_count = 0;

                vf = video_vf_peek();
            } else
#endif
                /* setting video display property in pause mode */
                if (video_property_changed && cur_dispbuf) {
                    if (blackout|force_blackout) {
                        if (cur_dispbuf != &vf_local) {
                            vsync_toggle_frame(cur_dispbuf);
                        }
                    } else {
                        vsync_toggle_frame(cur_dispbuf);
                    }
                }

            break;
        }
#ifdef CONFIG_AM_VIDEO_LOG
        toggle_cnt++;
#endif
    }

SET_FILTER:
    /* filter setting management */
    if ((frame_par_ready_to_set) || (frame_par_force_to_set)) {
        cur_frame_par = next_frame_par;
    }

    if (cur_dispbuf) {
        f2v_vphase_t *vphase;
        u32 vin_type = cur_dispbuf->type & VIDTYPE_TYPEMASK;

#ifdef CONFIG_AM_DEINTERLACE
        if ((deinterlace_mode == 0) || (cur_dispbuf->duration == 0)
#if defined(CONFIG_AM_DEINTERLACE_SD_ONLY)
            || (cur_dispbuf->width > 720)
#endif
           )
#endif
        {
            viu_set_dcu(cur_frame_par, cur_dispbuf);
        }

        /* vertical phase */
        vphase = &cur_frame_par->VPP_vf_ini_phase_[vpp_phase_table[vin_type][vout_type]];
        WRITE_MPEG_REG(VPP_VSC_INI_PHASE, ((u32)(vphase->phase) << 8));

        if (vphase->repeat_skip >= 0) {
            /* skip lines */
            WRITE_MPEG_REG_BITS(VPP_VSC_PHASE_CTRL,
                                skip_tab[vphase->repeat_skip],
                                VPP_PHASECTL_INIRCVNUMT_BIT,
                                VPP_PHASECTL_INIRCVNUM_WID +
                                VPP_PHASECTL_INIRPTNUM_WID);

        } else {
            /* repeat first line */
            WRITE_MPEG_REG_BITS(VPP_VSC_PHASE_CTRL, 4,
                                VPP_PHASECTL_INIRCVNUMT_BIT,
                                VPP_PHASECTL_INIRCVNUM_WID);
            WRITE_MPEG_REG_BITS(VPP_VSC_PHASE_CTRL,
                                1 - vphase->repeat_skip,
                                VPP_PHASECTL_INIRPTNUMT_BIT,
                                VPP_PHASECTL_INIRPTNUM_WID);
        }
    }

    if (((frame_par_ready_to_set) || (frame_par_force_to_set)) &&
        (cur_frame_par)) {
        vppfilter_mode_t *vpp_filter = &cur_frame_par->vpp_filter;

        if (cur_dispbuf) {
            u32 zoom_start_y, zoom_end_y;

            if (cur_dispbuf->type & VIDTYPE_INTERLACE) {
                if (cur_dispbuf->type & VIDTYPE_VIU_FIELD) {
                    zoom_start_y = cur_frame_par->VPP_vd_start_lines_ >> 1;
                    zoom_end_y = (cur_frame_par->VPP_vd_end_lines_ + 1) >> 1;
                } else {
                    zoom_start_y = cur_frame_par->VPP_vd_start_lines_;
                    zoom_end_y = cur_frame_par->VPP_vd_end_lines_;
                }
            } else {
                if (cur_dispbuf->type & VIDTYPE_VIU_FIELD) {
                    zoom_start_y = cur_frame_par->VPP_vd_start_lines_;
                    zoom_end_y = cur_frame_par->VPP_vd_end_lines_;
                } else {
                    zoom_start_y = cur_frame_par->VPP_vd_start_lines_ >> 1;
                    zoom_end_y = (cur_frame_par->VPP_vd_end_lines_ + 1) >> 1;
                }
            }

            zoom_start_x_lines = cur_frame_par->VPP_hd_start_lines_;
            zoom_end_x_lines   = cur_frame_par->VPP_hd_end_lines_;
            zoom_display_horz();

            zoom_start_y_lines = zoom_start_y;
            zoom_end_y_lines   = zoom_end_y;
            zoom_display_vert();
        }

        /* vpp filters */
        SET_MPEG_REG_MASK(VPP_SC_MISC,
                          VPP_SC_TOP_EN | VPP_SC_VERT_EN | VPP_SC_HORZ_EN);

        /* horitontal filter settings */
        WRITE_MPEG_REG_BITS(VPP_SC_MISC,
                            vpp_filter->vpp_horz_coeff[0],
                            VPP_SC_HBANK_LENGTH_BIT,
                            VPP_SC_BANK_LENGTH_WID);

        if (vpp_filter->vpp_horz_coeff[1] & 0x8000) {
            WRITE_MPEG_REG(VPP_SCALE_COEF_IDX, VPP_COEF_HORZ | VPP_COEF_9BIT);
        } else {
            WRITE_MPEG_REG(VPP_SCALE_COEF_IDX, VPP_COEF_HORZ);
        }

        for (i = 0; i < (vpp_filter->vpp_horz_coeff[1] & 0xff); i++) {
            WRITE_MPEG_REG(VPP_SCALE_COEF, vpp_filter->vpp_horz_coeff[i + 2]);
        }

        /* vertical filter settings */
        WRITE_MPEG_REG_BITS(VPP_SC_MISC,
                            vpp_filter->vpp_vert_coeff[0],
                            VPP_SC_VBANK_LENGTH_BIT,
                            VPP_SC_BANK_LENGTH_WID);

        WRITE_MPEG_REG(VPP_SCALE_COEF_IDX, VPP_COEF_VERT);
        for (i = 0; i < vpp_filter->vpp_vert_coeff[1]; i++) {
            WRITE_MPEG_REG(VPP_SCALE_COEF,
                           vpp_filter->vpp_vert_coeff[i + 2]);
        }

        WRITE_MPEG_REG(VPP_PIC_IN_HEIGHT,
                       cur_frame_par->VPP_pic_in_height_);

        WRITE_MPEG_REG_BITS(VPP_HSC_PHASE_CTRL,
                            cur_frame_par->VPP_hf_ini_phase_,
                            VPP_HSC_TOP_INI_PHASE_BIT,
                            VPP_HSC_TOP_INI_PHASE_WID);
        WRITE_MPEG_REG(VPP_POSTBLEND_VD1_H_START_END,
                       ((cur_frame_par->VPP_post_blend_vd_h_start_ & VPP_VD_SIZE_MASK) << VPP_VD1_START_BIT) |
                       ((cur_frame_par->VPP_post_blend_vd_h_end_   & VPP_VD_SIZE_MASK) << VPP_VD1_END_BIT));
        WRITE_MPEG_REG(VPP_POSTBLEND_VD1_V_START_END,
                       ((cur_frame_par->VPP_post_blend_vd_v_start_ & VPP_VD_SIZE_MASK) << VPP_VD1_START_BIT) |
                       ((cur_frame_par->VPP_post_blend_vd_v_end_   & VPP_VD_SIZE_MASK) << VPP_VD1_END_BIT));
        WRITE_MPEG_REG(VPP_POSTBLEND_H_SIZE, cur_frame_par->VPP_post_blend_h_size_);

        if((cur_frame_par->VPP_post_blend_vd_v_end_ - cur_frame_par->VPP_post_blend_vd_v_start_+1)>1080){
            WRITE_MPEG_REG(VPP_PREBLEND_VD1_V_START_END,
                       ((cur_frame_par->VPP_post_blend_vd_v_start_ & VPP_VD_SIZE_MASK) << VPP_VD1_START_BIT) |
                       ((cur_frame_par->VPP_post_blend_vd_v_end_ & VPP_VD_SIZE_MASK) << VPP_VD1_END_BIT));
        }else{
            WRITE_MPEG_REG(VPP_PREBLEND_VD1_V_START_END,
                       ((0 & VPP_VD_SIZE_MASK) << VPP_VD1_START_BIT) |
                       ((1079 & VPP_VD_SIZE_MASK) << VPP_VD1_END_BIT));
        }

        vpp_settings_h(cur_frame_par);
        vpp_settings_v(cur_frame_par);

        frame_par_ready_to_set = 0;
        frame_par_force_to_set = 0;
    } /* VPP one time settings */

    wait_sync = 0;

#ifdef CONFIG_AM_DEINTERLACE
    if ((deinterlace_mode != 0) && cur_dispbuf && (cur_dispbuf->duration > 0)
#if defined(CONFIG_AM_DEINTERLACE_SD_ONLY)
        && (cur_dispbuf->width <= 720)
#endif
       ) {
        run_deinterlace(zoom_start_x_lines, zoom_end_x_lines, zoom_start_y_lines, zoom_end_y_lines, cur_dispbuf->type_backup, cur_dispbuf->blend_mode, hold_line);
    }
    else
#endif
    if(cur_dispbuf && cur_dispbuf->process_fun){
        /* for new deinterlace driver */
        cur_dispbuf->process_fun(cur_dispbuf->private_data, zoom_start_x_lines|(cur_frame_par->vscale_skip_count<<24), zoom_end_x_lines, zoom_start_y_lines, zoom_end_y_lines);
    }

exit:
    if(timer_count > 50){
        timer_count = 0 ;
        video_notify_flag |= VIDEO_NOTIFY_FRAME_WAIT;
#ifdef CONFIG_POST_PROCESS_MANAGER_PPSCALER
        if((video_scaler_mode)&&(scaler_pos_changed)){
            video_notify_flag |= VIDEO_NOTIFY_POS_CHANGED;
            scaler_pos_changed = 0;
        }else{
            scaler_pos_changed = 0;
            video_notify_flag &= ~VIDEO_NOTIFY_POS_CHANGED;
        }
#endif
    }

    enc_line = (READ_MPEG_REG(ENCL_INFO_READ)>>16)&0x1fff;
    if(enc_line > vsync_exit_line_max)
        vsync_exit_line_max = enc_line;

#ifdef FIQ_VSYNC
    if (video_notify_flag)
        fiq_bridge_pulse_trigger(&vsync_fiq_bridge);
#else
    if (video_notify_flag)
        vsync_notify();

    return IRQ_HANDLED;
#endif

}

static int alloc_keep_buffer(void)
{
    amlog_mask(LOG_MASK_KEEPBUF, "alloc_keep_buffer\n");

    if(!keep_y_addr){
        keep_y_addr = __get_free_pages(GFP_KERNEL, get_order(Y_BUFFER_SIZE));
        if (!keep_y_addr) {
            amlog_mask(LOG_MASK_KEEPBUF, "%s: failed to alloc y addr\n", __FUNCTION__);
            goto err1;
        }

        keep_y_addr_remap = ioremap_nocache(virt_to_phys((u8 *)keep_y_addr), Y_BUFFER_SIZE);
        if (!keep_y_addr_remap) {
                amlog_mask(LOG_MASK_KEEPBUF, "%s: failed to remap y addr\n", __FUNCTION__);
                goto err2;
        }
    }

    if(!keep_u_addr){
        keep_u_addr = __get_free_pages(GFP_KERNEL, get_order(U_BUFFER_SIZE));
        if (!keep_u_addr) {
            amlog_mask(LOG_MASK_KEEPBUF, "%s: failed to alloc u addr\n", __FUNCTION__);
            goto err3;
        }

        keep_u_addr_remap = ioremap_nocache(virt_to_phys((u8 *)keep_u_addr), U_BUFFER_SIZE);
        if (!keep_u_addr_remap) {
            amlog_mask(LOG_MASK_KEEPBUF, "%s: failed to remap u addr\n", __FUNCTION__);
            goto err4;
        }
    }

    if(!keep_v_addr){
        keep_v_addr = __get_free_pages(GFP_KERNEL, get_order(V_BUFFER_SIZE));
        if (!keep_v_addr) {
            amlog_mask(LOG_MASK_KEEPBUF, "%s: failed to alloc v addr\n", __FUNCTION__);
            goto err5;
        }

        keep_v_addr_remap = ioremap_nocache(virt_to_phys((u8 *)keep_v_addr), U_BUFFER_SIZE);
        if (!keep_v_addr_remap) {
            amlog_mask(LOG_MASK_KEEPBUF, "%s: failed to remap v addr\n", __FUNCTION__);
            goto err6;
        }
    }
    return 0;

err6:
    free_pages(keep_v_addr, get_order(U_BUFFER_SIZE));
    keep_v_addr = 0;
err5:
    iounmap(keep_u_addr_remap);
    keep_u_addr_remap = NULL;
err4:
    free_pages(keep_u_addr, get_order(U_BUFFER_SIZE));
    keep_u_addr = 0;
err3:
    iounmap(keep_y_addr_remap);
    keep_y_addr_remap = NULL;
err2:
    free_pages(keep_y_addr, get_order(Y_BUFFER_SIZE));
    keep_y_addr = 0;
err1:
    return -ENOMEM;
}

void get_video_keep_buffer(ulong *addr, ulong *phys_addr)
{
    if (addr) {
        addr[0] = (ulong)keep_y_addr_remap;
        addr[1] = (ulong)keep_u_addr_remap;
        addr[2] = (ulong)keep_v_addr_remap;
    }
#ifdef CONFIG_KEEP_FRAME_RESERVED
    if (phys_addr) {
        phys_addr[0] = (keep_y_addr == 0) ? 0 : keep_y_addr;
        phys_addr[1] = (keep_u_addr == 0) ? 0 : keep_u_addr;
        phys_addr[2] = (keep_v_addr == 0) ? 0 : keep_v_addr;
    }
#else
    if (phys_addr) {
        phys_addr[0] = (keep_y_addr == 0) ? 0 : (ulong)virt_to_phys((u8 *)keep_y_addr);
        phys_addr[1] = (keep_u_addr == 0) ? 0 : (ulong)virt_to_phys((u8 *)keep_u_addr);
        phys_addr[2] = (keep_v_addr == 0) ? 0 : (ulong)virt_to_phys((u8 *)keep_v_addr);
    }
#endif
    if(debug_flag& DEBUG_FLAG_BLACKOUT){
        printk("%s: y=%lx u=%lx v=%lx\n", __func__, phys_addr[0], phys_addr[1], phys_addr[2]);
    }
}

/*********************************************************
 * FIQ Routines
 *********************************************************/

static void vsync_fiq_up(void)
{
#ifdef  FIQ_VSYNC
    request_fiq(INT_VIU_VSYNC, &vsync_fisr);
#else
    int r;
    r = request_irq(INT_VIU_VSYNC, &vsync_isr,
                    IRQF_SHARED, "vsync",
                    (void *)video_dev_id);
#endif
}

static void vsync_fiq_down(void)
{
#ifdef FIQ_VSYNC
    free_fiq(INT_VIU_VSYNC, &vsync_fisr);
#else
    free_irq(INT_VIU_VSYNC, (void *)video_dev_id);
#endif
}

int get_curren_frame_para(int* top ,int* left , int* bottom, int* right)
{
	if(!cur_frame_par){
		return -1;
	}
	*top    =  cur_frame_par->VPP_vd_start_lines_ ;
	*left   =  cur_frame_par->VPP_hd_start_lines_ ;
	*bottom =  cur_frame_par->VPP_vd_end_lines_ ;
	*right  =  cur_frame_par->VPP_hd_end_lines_;
	return 	0;
}

int get_current_vscale_skip_count(void)
{
    if(cur_frame_par)
		return cur_frame_par->vscale_skip_count;
	else
		return 0;
}
int query_video_status(int type , int* value)
{
	if(value == NULL){
		return -1;
	}
	switch(type){
		case 0:
			*value = trickmode_fffb ;
			break;
		default:
			break;
	}
	return 0;
}
static void video_vf_unreg_provider(void)
{
    ulong flags;

#ifdef CONFIG_AM_DEINTERLACE
    int deinterlace_mode = get_deinterlace_mode();
#endif

    atomic_set(&video_unreg_flag, 1);
    spin_lock_irqsave(&lock, flags);

    if (cur_dispbuf) {
        vf_local = *cur_dispbuf;
        cur_dispbuf = &vf_local;
    }

    if (trickmode_fffb) {
        atomic_set(&trickmode_framedone, 0);
    }

    if (blackout|force_blackout) {
#ifdef CONFIG_POST_PROCESS_MANAGER_PPSCALER
        if(video_scaler_mode)
            DisableVideoLayer_PREBELEND();
        else
            DisableVideoLayer();
#else
        DisableVideoLayer();
#endif
    }

    //if (!trickmode_fffb)
    if (cur_dispbuf)
    {
        vf_keep_current();
    }

    tsync_avevent(VIDEO_STOP, 0);

#ifdef CONFIG_AM_DEINTERLACE
    if (deinterlace_mode == 2) {
        disable_pre_deinterlace();
    }
#endif
    spin_unlock_irqrestore(&lock, flags);
    atomic_set(&video_unreg_flag, 0);

    enable_video_discontinue_report = 1;
}

static void video_vf_light_unreg_provider(void)
{
    ulong flags;

    spin_lock_irqsave(&lock, flags);

    if (cur_dispbuf) {
        vf_local = *cur_dispbuf;
        cur_dispbuf = &vf_local;
    }


    spin_unlock_irqrestore(&lock, flags);
}

static int video_receiver_event_fun(int type, void* data, void* private_data)
{
    if(type == VFRAME_EVENT_PROVIDER_UNREG){
        video_vf_unreg_provider();
#ifdef CONFIG_AM_VIDEO2
        set_clone_frame_rate(android_clone_rate, 200);
#endif
    }
    else if(type == VFRAME_EVENT_PROVIDER_LIGHT_UNREG){
        video_vf_light_unreg_provider();
    }
    else if(type == VFRAME_EVENT_PROVIDER_REG){
        enable_video_discontinue_report = 1;
#ifdef CONFIG_AM_VIDEO2
        char* provider_name = (char*)data;
        if(strncmp(provider_name, "decoder", 7)==0
            || strncmp(provider_name, "ppmgr", 5)==0
            || strncmp(provider_name, "deinterlace", 11)==0
            || strncmp(provider_name, "d2d3", 11)==0 ){
            set_clone_frame_rate(noneseamless_play_clone_rate, 0);
            set_clone_frame_rate(video_play_clone_rate, 100);
        }
#endif
        video_vf_light_unreg_provider();
    }
    else if(type == VFRAME_EVENT_PROVIDER_FORCE_BLACKOUT){
    	  force_blackout = 1;
        if(debug_flag& DEBUG_FLAG_BLACKOUT){
            printk("%s VFRAME_EVENT_PROVIDER_FORCE_BLACKOUT\n", __func__);
        }
    }
    return 0;
}

static int video4osd_receiver_event_fun(int type, void* data, void* private_data)
{
    if(type == VFRAME_EVENT_PROVIDER_UNREG){
        osd_prov = NULL;
        if(debug_flag& DEBUG_FLAG_BLACKOUT){
            printk("[video4osd] clear osd_prov\n");
        }
    }
    else if(type == VFRAME_EVENT_PROVIDER_REG){
        osd_prov = vf_get_provider(RECEIVER4OSD_NAME);
        if(debug_flag& DEBUG_FLAG_BLACKOUT){
            printk("[video4osd] set osd_prov\n");
        }
    }
    return 0;
}

unsigned int get_post_canvas(void)
{
    return post_canvas;
}

static int canvas_dup(ulong *dst, ulong src_paddr, ulong size)
{
    void __iomem *p = ioremap_wc(src_paddr, size);

    if (p) {
        memcpy(dst, p, size);
        iounmap(p);

        return 1;
    }

    return 0;
}

unsigned int vf_keep_current(void)
{
    u32 cur_index;
    u32 y_index, u_index, v_index;
    canvas_t cs0,cs1,cs2,cd;

    if (!cur_dispbuf)
        return 0;

    if(cur_dispbuf->source_type==VFRAME_SOURCE_TYPE_OSD)
        return 0;
    if(READ_MPEG_REG(DI_IF1_GEN_REG)&0x1){
        return 0;
    }
#ifdef CONFIG_AM_DEINTERLACE
    int deinterlace_mode = get_deinterlace_mode();
#endif
    if(debug_flag & DEBUG_FLAG_TOGGLE_SKIP_KEEP_CURRENT){
        return 0;
    }

    if (blackout|force_blackout) {
        return 0;
    }

    if (0 == (READ_MPEG_REG(VPP_MISC) & VPP_VD1_POSTBLEND)) {
        return 0;
    }

#ifdef CONFIG_AM_DEINTERLACE
    if ((deinterlace_mode != 0) && cur_dispbuf && (cur_dispbuf->duration > 0)
#if defined(CONFIG_AM_DEINTERLACE_SD_ONLY)
        && (cur_dispbuf->width <= 720)
#endif
       ) {
        return 0;
    }
#endif

    if (!keep_y_addr_remap) {
        //if (alloc_keep_buffer())
        return -1;
    }

    cur_index = READ_MPEG_REG(VD1_IF0_CANVAS0);
    y_index = cur_index & 0xff;
    u_index = (cur_index >> 8) & 0xff;
    v_index = (cur_index >> 16) & 0xff;

    if(debug_flag& DEBUG_FLAG_BLACKOUT){
    	printk("%s %lx %x\n", __func__, keep_y_addr, canvas_get_addr(y_index));
    }

    if ((cur_dispbuf->type & VIDTYPE_VIU_422) == VIDTYPE_VIU_422) {
	 canvas_read(y_index,&cd);
        if (keep_y_addr != canvas_get_addr(y_index) && /*must not the same address*/
            canvas_dup(keep_y_addr_remap, canvas_get_addr(y_index), (cd.width)*(cd.height))) {
            canvas_update_addr(y_index, (u32)keep_y_addr);
            if(debug_flag& DEBUG_FLAG_BLACKOUT){
                printk("%s: VIDTYPE_VIU_422\n", __func__);
            }
        }
    } else if ((cur_dispbuf->type & VIDTYPE_VIU_444) == VIDTYPE_VIU_444) {
    	 canvas_read(y_index,&cd);
        if (keep_y_addr != canvas_get_addr(y_index) && /*must not the same address*/
            canvas_dup(keep_y_addr_remap, canvas_get_addr(y_index), (cd.width)*(cd.height))){
            canvas_update_addr(y_index, (u32)keep_y_addr);
            if(debug_flag& DEBUG_FLAG_BLACKOUT){
                printk("%s: VIDTYPE_VIU_444\n", __func__);
            }
        }
    } else if((cur_dispbuf->type & VIDTYPE_VIU_NV21) == VIDTYPE_VIU_NV21){
        canvas_read(y_index,&cs0);
        canvas_read(u_index,&cs1);
        if (keep_y_addr != canvas_get_addr(y_index) && /*must not the same address*/
            canvas_dup(keep_y_addr_remap, canvas_get_addr(y_index), (cs0.width *cs0.height)) &&
            canvas_dup(keep_u_addr_remap, canvas_get_addr(u_index), (cs1.width *cs1.height))){
            canvas_update_addr(y_index, (u32)keep_y_addr);
            canvas_update_addr(u_index, (u32)keep_u_addr);
            if(debug_flag& DEBUG_FLAG_BLACKOUT){
                printk("%s: VIDTYPE_VIU_NV21\n", __func__);
            }
        }

    }else{
        canvas_read(y_index,&cs0);
        canvas_read(u_index,&cs1);
        canvas_read(v_index,&cs2);

        if (keep_y_addr != canvas_get_addr(y_index) && /*must not the same address*/
            canvas_dup(keep_y_addr_remap, canvas_get_addr(y_index), (cs0.width *cs0.height)) &&
            canvas_dup(keep_u_addr_remap, canvas_get_addr(u_index), (cs1.width *cs1.height)) &&
            canvas_dup(keep_v_addr_remap, canvas_get_addr(v_index), (cs2.width *cs2.height))) {
            canvas_update_addr(y_index, (u32)keep_y_addr);
            canvas_update_addr(u_index, (u32)keep_u_addr);
            canvas_update_addr(v_index, (u32)keep_v_addr);
            if(debug_flag& DEBUG_FLAG_BLACKOUT){
                printk("%s: VIDTYPE_VIU_420\n", __func__);
            }
        }
    }

    return 0;
}

EXPORT_SYMBOL(get_post_canvas);
EXPORT_SYMBOL(vf_keep_current);

u32 get_blackout_policy(void)
{
    return blackout;
}
EXPORT_SYMBOL(get_blackout_policy);

/*********************************************************
 * Utilities
 *********************************************************/
int _video_set_disable(u32 val)
{
    if ((val < VIDEO_DISABLE_NONE) || (val > VIDEO_DISABLE_FORNEXT)) {
        return -EINVAL;
    }

    disable_video = val;

    if (disable_video != VIDEO_DISABLE_NONE) {
#ifdef CONFIG_POST_PROCESS_MANAGER_PPSCALER
        if(video_scaler_mode)
            DisableVideoLayer_PREBELEND();
        else
            DisableVideoLayer();
#else
        DisableVideoLayer();
#endif

        if ((disable_video == VIDEO_DISABLE_FORNEXT) && cur_dispbuf && (cur_dispbuf != &vf_local))
            video_property_changed = true;

    } else {
        if (cur_dispbuf && (cur_dispbuf != &vf_local)) {
            EnableVideoLayer();
        }
    }

    return 0;
}

static void _set_video_crop(int *p)
{
    vpp_set_video_source_crop(p[0], p[1], p[2], p[3]);

    video_property_changed = true;
}

static void _set_video_window(int *p)
{
    int w, h;
    int *parsed = p;

    w = parsed[2] - parsed[0] + 1;
    h = parsed[3] - parsed[1] + 1;

#ifdef CONFIG_POST_PROCESS_MANAGER_PPSCALER
    if(video_scaler_mode){
        if ((w == 1) && (h == 1)){
            w= 0;
            h = 0;
        }
        if((content_left!=parsed[0])||(content_top!=parsed[1])||(content_w!=w)||(content_h!=h))
            scaler_pos_changed = 1;
        content_left = parsed[0];
        content_top = parsed[1];
        content_w = w;
        content_h = h;
        //video_notify_flag = video_notify_flag|VIDEO_NOTIFY_POS_CHANGED;
    }else
#endif
    {
        if ((w == 1) && (h == 1)) {
            w = h = 0;
            vpp_set_video_layer_position(parsed[0], parsed[1], 0, 0);
        } else if ((w > 0) && (h > 0)) {
            vpp_set_video_layer_position(parsed[0], parsed[1], w, h);
        }
    }
    video_property_changed = true;
}

/*********************************************************
 * /dev/amvideo APIs
 *********************************************************/
static int amvideo_open(struct inode *inode, struct file *file)
{
    return 0;
}

static int amvideo_release(struct inode *inode, struct file *file)
{
    if (blackout|force_blackout) {
        ///DisableVideoLayer();/*don't need it ,it have problem on  pure music playing*/
    }
    return 0;
}

static long amvideo_ioctl(struct file *file,
                          unsigned int cmd, ulong arg)
{
    long ret = 0;
    void *argp = (void *)arg;

    switch (cmd) {
    case AMSTREAM_IOC_TRICKMODE:
        if (arg == TRICKMODE_I) {
            trickmode_i = 1;
        } else if (arg == TRICKMODE_FFFB) {
            trickmode_fffb = 1;
        } else {
            trickmode_i = 0;
            trickmode_fffb = 0;
        }
        atomic_set(&trickmode_framedone, 0);
        tsync_trick_mode(trickmode_fffb);
        break;

    case AMSTREAM_IOC_TRICK_STAT:
        *((u32 *)arg) = atomic_read(&trickmode_framedone);
        break;

    case AMSTREAM_IOC_VPAUSE:
        tsync_avevent(VIDEO_PAUSE, arg);
        break;

    case AMSTREAM_IOC_AVTHRESH:
        tsync_set_avthresh(arg);
        break;

    case AMSTREAM_IOC_SYNCTHRESH:
        tsync_set_syncthresh(arg);
        break;

    case AMSTREAM_IOC_SYNCENABLE:
        tsync_set_enable(arg);
        break;

    case AMSTREAM_IOC_SET_SYNC_ADISCON:
        tsync_set_sync_adiscont(arg);
        break;

	 case AMSTREAM_IOC_SET_SYNC_VDISCON:
        tsync_set_sync_vdiscont(arg);
        break;

    case AMSTREAM_IOC_GET_SYNC_ADISCON:
        *((u32 *)arg) = tsync_get_sync_adiscont();
        break;

	case AMSTREAM_IOC_GET_SYNC_VDISCON:
        *((u32 *)arg) = tsync_get_sync_vdiscont();
        break;

	case AMSTREAM_IOC_GET_SYNC_ADISCON_DIFF:
		*((u32 *)arg) = tsync_get_sync_adiscont_diff();
		break;

	case AMSTREAM_IOC_GET_SYNC_VDISCON_DIFF:
		*((u32 *)arg) = tsync_get_sync_vdiscont_diff();
		break;

	case AMSTREAM_IOC_SET_SYNC_ADISCON_DIFF:
        tsync_set_sync_adiscont_diff(arg);
        break;

	 case AMSTREAM_IOC_SET_SYNC_VDISCON_DIFF:
        tsync_set_sync_vdiscont_diff(arg);
        break;

    case AMSTREAM_IOC_VF_STATUS: {
            vframe_states_t vfsta;
            vframe_states_t *states = (void *)arg;
            vf_get_states(&vfsta);

            if (states == NULL)
                return -EINVAL;

            states->vf_pool_size = vfsta.vf_pool_size;
            states->buf_avail_num = vfsta.buf_avail_num;
            states->buf_free_num = vfsta.buf_free_num;
            states->buf_recycle_num = vfsta.buf_recycle_num;
        }
        break;

    case AMSTREAM_IOC_GET_VIDEO_DISABLE:
        *((u32 *)arg) = disable_video;
        break;

    case AMSTREAM_IOC_SET_VIDEO_DISABLE:
        ret = _video_set_disable(arg);
        break;

    case AMSTREAM_IOC_GET_VIDEO_DISCONTINUE_REPORT:
        *((u32 *)arg) = enable_video_discontinue_report;
        break;

    case AMSTREAM_IOC_SET_VIDEO_DISCONTINUE_REPORT:
        enable_video_discontinue_report = (arg == 0) ? 0 : 1;
        break;

    case AMSTREAM_IOC_GET_VIDEO_AXIS:
        {
            int axis[4];
#ifdef CONFIG_POST_PROCESS_MANAGER_PPSCALER
            if (video_scaler_mode) {
                axis[0] = content_left;
                axis[1] = content_top;
                axis[2] = content_w;
                axis[3] = content_h;
            } else
#endif
            {
                vpp_get_video_layer_position(&axis[0], &axis[1], &axis[2], &axis[3]);
            }

            axis[2] = axis[0] + axis[2] - 1;
            axis[3] = axis[1] + axis[3] - 1;

            if (copy_to_user(argp, &axis[0], sizeof(axis)) != 0) {
                ret = -EFAULT;
            }
        }
        break;

    case AMSTREAM_IOC_SET_VIDEO_AXIS:
        {
            int axis[4];
            if (copy_from_user(axis, argp, sizeof(axis)) == 0) {
                _set_video_window(axis);
            } else {
                ret = -EFAULT;
            }
        }
        break;

    case AMSTREAM_IOC_GET_VIDEO_CROP:
        {
            int crop[4];
            {
                vpp_get_video_source_crop(&crop[0], &crop[1], &crop[2], &crop[3]);
            }

            if (copy_to_user(argp, &crop[0], sizeof(crop)) != 0) {
                ret = -EFAULT;
            }
        }
        break;

    case AMSTREAM_IOC_SET_VIDEO_CROP:
        {
            int crop[4];
            if (copy_from_user(crop, argp, sizeof(crop)) == 0) {
                _set_video_crop(crop);
            } else {
                ret = -EFAULT;
            }
        }
        break;

    case AMSTREAM_IOC_GET_SCREEN_MODE:
	if (copy_to_user(argp, &wide_setting, sizeof(u32)) != 0) {
            ret = -EFAULT;
        }
        break;

    case AMSTREAM_IOC_SET_SCREEN_MODE:
        {
            u32 mode;
            if (copy_from_user(&mode, argp, sizeof(u32)) == 0) {
               if (mode >= VIDEO_WIDEOPTION_MAX) {
                   ret = -EINVAL;
               } else if (mode != wide_setting) {
                   wide_setting = mode;
                   video_property_changed = true;
               }
            } else {
                ret = -EFAULT;
            }
        }
        break;


    case AMSTREAM_IOC_GET_BLACKOUT_POLICY:
	    if (copy_to_user(argp, &blackout, sizeof(u32)) != 0) {
            ret = -EFAULT;
        }
        break;

    case AMSTREAM_IOC_SET_BLACKOUT_POLICY:
        {
            u32 mode;
            if (copy_from_user(&mode, argp, sizeof(u32)) == 0) {
               if (mode > 2) {
                   ret = -EINVAL;
               } else {
                   blackout = mode;
               }
            } else {
                ret = -EFAULT;
            }
        }
        break;

    case AMSTREAM_IOC_CLEAR_VBUF:
        {
            unsigned long flags;
            spin_lock_irqsave(&lock, flags);
            cur_dispbuf = NULL;
            spin_unlock_irqrestore(&lock, flags);
        }
        break;

    case AMSTREAM_IOC_CLEAR_VIDEO:
        if (blackout) {
        #ifdef CONFIG_POST_PROCESS_MANAGER_PPSCALER
            if(video_scaler_mode)
                DisableVideoLayer_PREBELEND();
            else
                DisableVideoLayer();
        #else
            DisableVideoLayer();
        #endif
        }
        break;

    case AMSTREAM_IOC_SET_FREERUN_MODE:
        if (arg > FREERUN_DUR) {
            ret = -EFAULT;
        } else {
	        freerun_mode = arg;
        }
        break;

    case AMSTREAM_IOC_GET_FREERUN_MODE:
        *((u32 *)arg) = freerun_mode;
        break;

#if MESON_CPU_TYPE < MESON_CPU_TYPE_MESON6
    /**********************************************************************
    video enhancement ioctl
    **********************************************************************/
    case AMSTREAM_IOC_VE_DEBUG: {
        struct ve_regs_s data;
#if 0
        if (get_user((unsigned long long)data, (void __user *)arg))
#else
        if (copy_from_user(&data, (void __user *)arg, sizeof(struct ve_regs_s)))
#endif
        {
            ret = -EFAULT;
        } else {
            ve_set_regs(&data);
            if (!(data.mode)) { // read
#if 0
                if (put_user((unsigned long long)data, (void __user *)arg))
#else
                if (copy_to_user(&data, (void __user *)arg, sizeof(struct ve_regs_s)))
#endif
                {
                    ret = -EFAULT;
                }
            }
        }

        break;
    }
    case AMSTREAM_IOC_VE_BEXT: {
        struct ve_bext_s ve_bext;
        if (copy_from_user(&ve_bext, (void __user *)arg, sizeof(struct ve_bext_s))) {
            ret = -EFAULT;
            break;
        }
        ve_set_bext(&ve_bext);
        break;
    }

    case AMSTREAM_IOC_VE_DNLP: {
        struct ve_dnlp_s ve_dnlp;
        if (copy_from_user(&ve_dnlp, (void __user *)arg, sizeof(struct ve_dnlp_s))) {
            ret = -EFAULT;
            break;
        }
        ve_set_dnlp(&ve_dnlp);
        break;
    }

    case AMSTREAM_IOC_VE_HSVS: {
        struct ve_hsvs_s ve_hsvs;
        if (copy_from_user(&ve_hsvs, (void __user *)arg, sizeof(struct ve_hsvs_s))) {
            ret = -EFAULT;
            break;
        }
        ve_set_hsvs(&ve_hsvs);
        break;
    }

    case AMSTREAM_IOC_VE_CCOR: {
        struct ve_ccor_s ve_ccor;
        if (copy_from_user(&ve_ccor, (void __user *)arg, sizeof(struct ve_ccor_s))) {
            ret = -EFAULT;
            break;
        }
        ve_set_ccor(&ve_ccor);
        break;
    }

    case AMSTREAM_IOC_VE_BENH: {
        struct ve_benh_s ve_benh;
        if (copy_from_user(&ve_benh, (void __user *)arg, sizeof(struct ve_benh_s))) {
            ret = -EFAULT;
            break;
        }
        ve_set_benh(&ve_benh);
        break;
    }

    case AMSTREAM_IOC_VE_DEMO: {
        struct ve_demo_s ve_demo;
        if (copy_from_user(&ve_demo, (void __user *)arg, sizeof(struct ve_demo_s))) {
            ret = -EFAULT;
            break;
        }
        ve_set_demo(&ve_demo);
        break;
    }

    /**********************************************************************
    color management ioctl
    **********************************************************************/
    case AMSTREAM_IOC_CM_DEBUG: {
        struct cm_regs_s data;
        if (copy_from_user(&data, (void __user *)arg, sizeof(struct cm_regs_s))) {
            ret = -EFAULT;
        } else {
            cm_set_regs(&data);
            if (!(data.mode)) { // read
                if (copy_to_user(&data, (void __user *)arg, sizeof(struct cm_regs_s))) {
                    ret = -EFAULT;
                }
            }
        }
        break;
    }

    case AMSTREAM_IOC_CM_REGION: {
        struct cm_region_s cm_region;
        if (copy_from_user(&cm_region, (void __user *)arg, sizeof(struct cm_region_s))) {
            ret = -EFAULT;
            break;
        }
        cm_set_region(&cm_region);
        break;
    }

    case AMSTREAM_IOC_CM_TOP: {
        struct cm_top_s cm_top;
        if (copy_from_user(&cm_top, (void __user *)arg, sizeof(struct cm_top_s))) {
            ret = -EFAULT;
            break;
        }
        cm_set_top(&cm_top);
        break;
    }

    case AMSTREAM_IOC_CM_DEMO: {
        struct cm_demo_s cm_demo;
        if (copy_from_user(&cm_demo, (void __user *)arg, sizeof(struct cm_demo_s))) {
            ret = -EFAULT;
            break;
        }
        cm_set_demo(&cm_demo);
        break;
    }
#endif

    default:
        return -EINVAL;
    }

    return ret;
}

static unsigned int amvideo_poll(struct file *file, poll_table *wait_table)
{
    poll_wait(file, &amvideo_trick_wait, wait_table);

    if (atomic_read(&trickmode_framedone)) {
        atomic_set(&trickmode_framedone, 0);
        return POLLOUT | POLLWRNORM;
    }

    return 0;
}

const static struct file_operations amvideo_fops = {
    .owner    = THIS_MODULE,
    .open     = amvideo_open,
    .release  = amvideo_release,
    .unlocked_ioctl    = amvideo_ioctl,
    .poll     = amvideo_poll,
};

/*********************************************************
 * SYSFS property functions
 *********************************************************/
#define MAX_NUMBER_PARA 10
#define AMVIDEO_CLASS_NAME "video"

static int parse_para(const char *para, int para_num, int *result)
{
    char *endp;
    const char *startp = para;
    int *out = result;
    int len = 0, count = 0;

    if (!startp) {
        return 0;
    }

    len = strlen(startp);

    do {
        //filter space out
        while (startp && (isspace(*startp) || !isgraph(*startp)) && len) {
            startp++;
            len--;
        }

        if (len == 0) {
            break;
        }

        *out++ = simple_strtol(startp, &endp, 0);

        len -= endp - startp;
        startp = endp;
        count++;

    } while ((endp) && (count < para_num) && (len > 0));

    return count;
}

static void set_video_crop(const char *para)
{
    int parsed[4];

    if (likely(parse_para(para, 4, parsed) == 4)) {
        _set_video_crop(parsed);
    }
    amlog_mask(LOG_MASK_SYSFS,
               "video crop=>x0:%d,y0:%d,x1:%d,y1:%d\r\n ",
               parsed[0], parsed[1], parsed[2], parsed[3]);
}

static void set_video_window(const char *para)
{
    int parsed[4];

    if (likely(parse_para(para, 4, parsed) == 4)) {
        _set_video_window(parsed);
    }
    amlog_mask(LOG_MASK_SYSFS,
               "video=>x0:%d,y0:%d,x1:%d,y1:%d\r\n ",
               parsed[0], parsed[1], parsed[2], parsed[3]);
}

static ssize_t video_crop_show(struct class *cla, struct class_attribute *attr, char *buf)
{
    u32 t, l, b, r;

    vpp_get_video_layer_position(&t, &l, &b, &r);

    return snprintf(buf, 40, "%d %d %d %d\n", t, l, b, r);
}

static ssize_t video_crop_store(struct class *cla, struct class_attribute *attr, const char *buf,
                                size_t count)
{
    mutex_lock(&video_module_mutex);

    set_video_crop(buf);

    mutex_unlock(&video_module_mutex);

    return strnlen(buf, count);
}

static ssize_t video_axis_show(struct class *cla, struct class_attribute *attr, char *buf)
{
    int x, y, w, h;
#ifdef CONFIG_POST_PROCESS_MANAGER_PPSCALER
    if(video_scaler_mode){
        x = content_left;
        y = content_top;
        w = content_w;
        h = content_h;
    }else
#endif
    {
        vpp_get_video_layer_position(&x, &y, &w, &h);
    }
    return snprintf(buf, 40, "%d %d %d %d\n", x, y, x + w - 1, y + h - 1);
}

static ssize_t video_axis_store(struct class *cla, struct class_attribute *attr, const char *buf,
                                size_t count)
{
    mutex_lock(&video_module_mutex);

    set_video_window(buf);

    mutex_unlock(&video_module_mutex);

    return strnlen(buf, count);
}

static ssize_t video_global_offset_show(struct class *cla, struct class_attribute *attr, char *buf)
{
    int x, y;
    vpp_get_global_offset(&x, &y);

    return snprintf(buf, 40, "%d %d\n", x, y);
}

static ssize_t video_global_offset_store(struct class *cla, struct class_attribute *attr, const char *buf,
                                size_t count)
{
    int parsed[2];

    mutex_lock(&video_module_mutex);

    if (likely(parse_para(buf, 2, parsed) == 2)) {
        vpp_set_global_offset(parsed[0], parsed[1]);
        video_property_changed = true;

        amlog_mask(LOG_MASK_SYSFS,
                   "video_offset=>x0:%d,y0:%d\r\n ",
                   parsed[0], parsed[1]);
    }

    mutex_unlock(&video_module_mutex);

    return count;
}

static ssize_t video_zoom_show(struct class *cla, struct class_attribute *attr, char *buf)
{
    u32 r = vpp_get_zoom_ratio();

    return snprintf(buf, 40, "%d\n", r);
}

static ssize_t video_zoom_store(struct class *cla, struct class_attribute *attr, const char *buf,
                                size_t count)
{
    u32 r;
    char *endp;

    r = simple_strtoul(buf, &endp, 0);

    if ((r <= MAX_ZOOM_RATIO) && (r != vpp_get_zoom_ratio())) {
        vpp_set_zoom_ratio(r);
        video_property_changed = true;
    }

    return count;
}

static ssize_t video_screen_mode_show(struct class *cla, struct class_attribute *attr, char *buf)
{
    const char *wide_str[] = {"normal", "full stretch", "4-3", "16-9", "non-linear", "normal-noscaleup"};

    if (wide_setting < ARRAY_SIZE(wide_str)) {
        return sprintf(buf, "%d:%s\n", wide_setting, wide_str[wide_setting]);
    } else {
        return 0;
    }
}

static ssize_t video_screen_mode_store(struct class *cla, struct class_attribute *attr, const char *buf,
                                       size_t count)
{
    unsigned long mode;
    char *endp;

    mode = simple_strtol(buf, &endp, 0);

    if ((mode < VIDEO_WIDEOPTION_MAX) && (mode != wide_setting)) {
        wide_setting = mode;
        video_property_changed = true;
    }

    return count;
}

static ssize_t video_blackout_policy_show(struct class *cla, struct class_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", blackout);
}

static ssize_t video_blackout_policy_store(struct class *cla, struct class_attribute *attr, const char *buf,
        size_t count)
{
    size_t r;

    r = sscanf(buf, "%d", &blackout);

    if(debug_flag& DEBUG_FLAG_BLACKOUT){
        printk("%s(%d)\n", __func__, blackout);
    }
    if (r != 1) {
        return -EINVAL;
    }

    return count;
}

static ssize_t video_brightness_show(struct class *cla, struct class_attribute *attr, char *buf)
{
    s32 val = (READ_MPEG_REG(VPP_VADJ1_Y) >> 8) & 0x1ff;

    val = (val << 23) >> 23;

    return sprintf(buf, "%d\n", val);
}

static ssize_t video_brightness_store(struct class *cla, struct class_attribute *attr, const char *buf,
                                      size_t count)
{
    size_t r;
    int val;

    r = sscanf(buf, "%d", &val);
    if ((r != 1) || (val < -255) || (val > 255)) {
        return -EINVAL;
    }

    WRITE_MPEG_REG_BITS(VPP_VADJ1_Y, val, 8, 9);
    WRITE_MPEG_REG(VPP_VADJ_CTRL, VPP_VADJ1_EN);

    return count;
}

static ssize_t video_contrast_show(struct class *cla, struct class_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", (int)(READ_MPEG_REG(VPP_VADJ1_Y) & 0xff) - 0x80);
}

static ssize_t video_contrast_store(struct class *cla, struct class_attribute *attr, const char *buf,
                                    size_t count)
{
    size_t r;
    int val;

    r = sscanf(buf, "%d", &val);
    if ((r != 1) || (val < -127) || (val > 127)) {
        return -EINVAL;
    }

    val += 0x80;

    WRITE_MPEG_REG_BITS(VPP_VADJ1_Y, val, 0, 8);
    WRITE_MPEG_REG(VPP_VADJ_CTRL, VPP_VADJ1_EN);

    return count;
}

static ssize_t video_saturation_show(struct class *cla, struct class_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", READ_MPEG_REG(VPP_VADJ1_Y) & 0xff);
}

static ssize_t video_saturation_store(struct class *cla, struct class_attribute *attr, const char *buf,
                                      size_t count)
{
    size_t r;
    int val;

    r = sscanf(buf, "%d", &val);
    if ((r != 1) || (val < -127) || (val > 127)) {
        return -EINVAL;
    }

    WRITE_MPEG_REG_BITS(VPP_VADJ1_Y, val, 0, 8);
    WRITE_MPEG_REG(VPP_VADJ_CTRL, VPP_VADJ1_EN);

    return count;
}

// [   24] 1/enable, 0/disable
// [23:16] Y
// [15: 8] Cb
// [ 7: 0] Cr
static ssize_t video_test_screen_show(struct class *cla, struct class_attribute *attr, char *buf)
{
    return sprintf(buf, "0x%x\n", test_screen);
}

static ssize_t video_test_screen_store(struct class *cla, struct class_attribute *attr, const char *buf,
                                size_t count)
{
    size_t r;
    unsigned data = 0x0;
    r = sscanf(buf, "0x%x", &test_screen);
    if (r != 1)
        return -EINVAL;

    //vdin0 pre post blend enable or disabled
    data = READ_CBUS_REG(VPP_MISC);
    if (test_screen & 0x01000000)
        data |= VPP_VD1_PREBLEND;
    else
        data &= (~VPP_VD1_PREBLEND);

    if (test_screen & 0x02000000)
        data |= VPP_VD1_POSTBLEND;
    else
        data &= (~VPP_VD1_POSTBLEND);
    /*
    if (test_screen & 0x04000000)
        data |= VPP_VD2_PREBLEND;
    else
        data &= (~VPP_VD2_PREBLEND);

    if (test_screen & 0x08000000)
        data |= VPP_VD2_POSTBLEND;
    else
        data &= (~VPP_VD2_POSTBLEND);
    */
    // show test screen
    WRITE_CBUS_REG(VPP_DUMMY_DATA1, test_screen&0x00ffffff);

    WRITE_CBUS_REG(VPP_MISC, data);

    return count;
}

static ssize_t video_nonlinear_factor_show(struct class *cla, struct class_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", vpp_get_nonlinear_factor());
}

static ssize_t video_nonlinear_factor_store(struct class *cla, struct class_attribute *attr, const char *buf,                                size_t count)
{
    size_t r;
    u32 factor;

    r = sscanf(buf, "%d", &factor);
    if (r != 1)
        return -EINVAL;

    if (vpp_set_nonlinear_factor(factor) == 0)
        video_property_changed = true;

    return count;
}

static ssize_t video_disable_show(struct class *cla, struct class_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", disable_video);
}

static ssize_t video_disable_store(struct class *cla, struct class_attribute *attr, const char *buf,
                                   size_t count)
{
    size_t r;
    int val;
    if(debug_flag& DEBUG_FLAG_BLACKOUT){
        printk("%s(%s)\n", __func__, buf);
    }
    r = sscanf(buf, "%d", &val);
    if (r != 1) {
        return -EINVAL;
    }

    if (_video_set_disable(val) < 0) {
        return -EINVAL;
    }

    return count;
}

static ssize_t video_freerun_mode_show(struct class *cla, struct class_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", freerun_mode);
}

static ssize_t video_freerun_mode_store(struct class *cla, struct class_attribute *attr, const char *buf,
                                   size_t count)
{
    size_t r;

    r = sscanf(buf, "%d", &freerun_mode);

    if(debug_flag){
        printk("%s(%d)\n", __func__, freerun_mode);
    }
    if (r != 1) {
        return -EINVAL;
    }

    return count;
}

static ssize_t frame_addr_show(struct class *cla, struct class_attribute *attr, char *buf)
{
    canvas_t canvas;
    u32 addr[3];

    if (cur_dispbuf) {
        canvas_read(cur_dispbuf->canvas0Addr & 0xff, &canvas);
        addr[0] = canvas.addr;
        canvas_read((cur_dispbuf->canvas0Addr >> 8) & 0xff, &canvas);
        addr[1] = canvas.addr;
        canvas_read((cur_dispbuf->canvas0Addr >> 16) & 0xff, &canvas);
        addr[2] = canvas.addr;

        return sprintf(buf, "0x%x-0x%x-0x%x\n", addr[0], addr[1], addr[2]);
    }

    return sprintf(buf, "NA\n");
}

static ssize_t frame_canvas_width_show(struct class *cla, struct class_attribute *attr, char *buf)
{
    canvas_t canvas;
    u32 width[3];

    if (cur_dispbuf) {
        canvas_read(cur_dispbuf->canvas0Addr & 0xff, &canvas);
        width[0] = canvas.width;
        canvas_read((cur_dispbuf->canvas0Addr >> 8) & 0xff, &canvas);
        width[1] = canvas.width;
        canvas_read((cur_dispbuf->canvas0Addr >> 16) & 0xff, &canvas);
        width[2] = canvas.width;

        return sprintf(buf, "%d-%d-%d\n", width[0], width[1], width[2]);
    }

    return sprintf(buf, "NA\n");
}

static ssize_t frame_canvas_height_show(struct class *cla, struct class_attribute *attr, char *buf)
{
    canvas_t canvas;
    u32 height[3];

    if (cur_dispbuf) {
        canvas_read(cur_dispbuf->canvas0Addr & 0xff, &canvas);
        height[0] = canvas.height;
        canvas_read((cur_dispbuf->canvas0Addr >> 8) & 0xff, &canvas);
        height[1] = canvas.height;
        canvas_read((cur_dispbuf->canvas0Addr >> 16) & 0xff, &canvas);
        height[2] = canvas.height;

        return sprintf(buf, "%d-%d-%d\n", height[0], height[1], height[2]);
    }

    return sprintf(buf, "NA\n");
}

static ssize_t frame_width_show(struct class *cla, struct class_attribute *attr, char *buf)
{
    if (cur_dispbuf) {
        return sprintf(buf, "%d\n", cur_dispbuf->width);
    }

    return sprintf(buf, "NA\n");
}

static ssize_t frame_height_show(struct class *cla, struct class_attribute *attr, char *buf)
{
    if (cur_dispbuf) {
        return sprintf(buf, "%d\n", cur_dispbuf->height);
    }

    return sprintf(buf, "NA\n");
}

static ssize_t frame_format_show(struct class *cla, struct class_attribute *attr, char *buf)
{
    if (cur_dispbuf) {
        if ((cur_dispbuf->type & VIDTYPE_TYPEMASK) == VIDTYPE_INTERLACE_TOP) {
            return sprintf(buf, "interlace-top\n");
        } else if ((cur_dispbuf->type & VIDTYPE_TYPEMASK) == VIDTYPE_INTERLACE_BOTTOM) {
            return sprintf(buf, "interlace-bottom\n");
        } else {
            return sprintf(buf, "progressive\n");
        }
    }

    return sprintf(buf, "NA\n");
}

static ssize_t frame_aspect_ratio_show(struct class *cla, struct class_attribute *attr, char *buf)
{
    if (cur_dispbuf) {
        u32 ar = (cur_dispbuf->ratio_control & DISP_RATIO_ASPECT_RATIO_MASK)
                 >> DISP_RATIO_ASPECT_RATIO_BIT;

        if (ar) {
            return sprintf(buf, "0x%x\n", ar);
        } else
            return sprintf(buf, "0x%x\n",
                           (cur_dispbuf->width << 8) / cur_dispbuf->height);
    }

    return sprintf(buf, "NA\n");
}

static ssize_t frame_rate_show(struct class *cla, struct class_attribute* attr, char* buf)
{
    u32 cnt = frame_count - last_frame_count;
    u32 time = jiffies;
    u32 tmp = time;
    u32 rate = 0;
    u32 vsync_rate;
    size_t ret;
    time -= last_frame_time;
    last_frame_time = tmp;
    last_frame_count = frame_count;
    rate = 100*cnt * HZ / time;
    vsync_rate = 100*vsync_count * HZ / time;
    ret = sprintf(buf, "VFrame rate is %d.%02dfps, and the panel refresh rate is %d, duration is: %d,vsync_isr/s=%d.%02d,vsync_pts_inc=%d\n",
                  rate/100,rate%100, vinfo->sync_duration_num / vinfo->sync_duration_den, time,vsync_rate/100,vsync_rate%100,vsync_pts_inc);
    if((debugflags& DEBUG_FLAG_CALC_PTS_INC) && time>HZ*10 && vsync_rate>0){
	if((vsync_rate*vsync_pts_inc/100)!=90000){
	  vsync_pts_inc=90000*100/(vsync_rate);
	}
    }
    vsync_count=0;
    return ret;
}

static ssize_t vframe_states_show(struct class *cla, struct class_attribute* attr, char* buf)
{
    int ret = 0;
    vframe_states_t states;

    if (vf_get_states(&states) == 0) {
        ret += sprintf(buf + ret, "vframe_pool_size=%d\n", states.vf_pool_size);
        ret += sprintf(buf + ret, "vframe buf_free_num=%d\n", states.buf_free_num);
        ret += sprintf(buf + ret, "vframe buf_recycle_num=%d\n", states.buf_recycle_num);
        ret += sprintf(buf + ret, "vframe buf_avail_num=%d\n", states.buf_avail_num);

    } else {
        ret += sprintf(buf + ret, "vframe no states\n");
    }

    return ret;
}

static ssize_t device_resolution_show(struct class *cla, struct class_attribute* attr, char* buf)
{
    const vinfo_t *info = get_current_vinfo();

    if (info != NULL) {
        return sprintf(buf, "%dx%d\n", info->width, info->height);
    } else {
        return sprintf(buf, "0x0\n");
    }
}

static ssize_t video_filename_show(struct class *cla, struct class_attribute *attr, char *buf)
{
    return sprintf(buf, "%s\n", file_name);
}

static ssize_t video_filename_store(struct class *cla, struct class_attribute *attr, const char *buf,
                                   size_t count)
{
    size_t r;
    r = sscanf(buf, "%s", file_name);
    if (r != 1) {
        return -EINVAL;
    }
    return r;
}

static ssize_t video_debugflags_show(struct class *cla, struct class_attribute *attr, char *buf)
{
    int len=0;
    len+=sprintf(buf+len, "value=%d\n", debugflags);
    len+=sprintf(buf+len, "bit0:playing as fast!\n");
    len+=sprintf(buf+len, "bit1:enable calc pts inc in frame rate show\n");
    return len;
}

static ssize_t video_debugflags_store(struct class *cla, struct class_attribute *attr, const char *buf,
                                   size_t count)
{
    size_t r;
    int value=-1,seted=1;
    r = sscanf(buf, "%d",&value);
    if(r==1){
	debugflags=value;
	seted=1;
    }else{
	r = sscanf(buf, "0x%x",&value);
    	if(r==1){
		debugflags=value;
		seted=1;
    	}
    }

    if(seted){
	printk("debugflags changed to %d(%x)\n",debugflags,debugflags);
	return count;
    }else
    	return -EINVAL;
}

static ssize_t trickmode_duration_show(struct class *cla, struct class_attribute *attr, char *buf)
{
    return sprintf(buf, "trickmode frame duration %d\n", trickmode_duration/9000);
}

static ssize_t trickmode_duration_store(struct class *cla, struct class_attribute *attr, const char *buf,
        size_t count)
{
    size_t r;
    u32 s_value;

    r = sscanf(buf, "%d", &s_value);
    if (r != 1) {
        return -EINVAL;
    }
    trickmode_duration = s_value * 9000;

    return count;
}

static ssize_t video_vsync_pts_inc_upint_show(struct class *cla, struct class_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", vsync_pts_inc_upint);
}

static ssize_t video_vsync_pts_inc_upint_store(struct class *cla, struct class_attribute *attr, const char *buf,
        size_t count)
{
    size_t r;

    r = sscanf(buf, "%d", &vsync_pts_inc_upint);

    if(debug_flag){
        printk("%s(%d)\n", __func__, vsync_pts_inc_upint);
    }
    if (r != 1) {
        return -EINVAL;
    }

    return count;
}


static struct class_attribute amvideo_class_attrs[] = {
    __ATTR(axis,
    S_IRUGO | S_IWUSR,
    video_axis_show,
    video_axis_store),
    __ATTR(crop,
    S_IRUGO | S_IWUSR,
    video_crop_show,
    video_crop_store),
    __ATTR(global_offset,
    S_IRUGO | S_IWUSR,
    video_global_offset_show,
    video_global_offset_store),
    __ATTR(screen_mode,
    S_IRUGO | S_IWUSR,
    video_screen_mode_show,
    video_screen_mode_store),
    __ATTR(blackout_policy,
    S_IRUGO | S_IWUSR,
    video_blackout_policy_show,
    video_blackout_policy_store),
    __ATTR(disable_video,
    S_IRUGO | S_IWUSR,
    video_disable_show,
    video_disable_store),
    __ATTR(zoom,
    S_IRUGO | S_IWUSR,
    video_zoom_show,
    video_zoom_store),
    __ATTR(brightness,
    S_IRUGO | S_IWUSR,
    video_brightness_show,
    video_brightness_store),
    __ATTR(contrast,
    S_IRUGO | S_IWUSR,
    video_contrast_show,
    video_contrast_store),
    __ATTR(saturation,
    S_IRUGO | S_IWUSR,
    video_saturation_show,
    video_saturation_store),
    __ATTR(test_screen,
    S_IRUGO | S_IWUSR,
    video_test_screen_show,
    video_test_screen_store),
     __ATTR(file_name,
    S_IRUGO | S_IWUSR,
    video_filename_show,
    video_filename_store),
    __ATTR(debugflags,
    S_IRUGO | S_IWUSR,
    video_debugflags_show,
    video_debugflags_store),
    __ATTR(trickmode_duration,
    S_IRUGO | S_IWUSR,
    trickmode_duration_show,
    trickmode_duration_store),
    __ATTR(nonlinear_factor,
    S_IRUGO | S_IWUSR,
    video_nonlinear_factor_show,
    video_nonlinear_factor_store),
    __ATTR(freerun_mode,
    S_IRUGO | S_IWUSR,
    video_freerun_mode_show,
    video_freerun_mode_store),
    __ATTR(vsync_pts_inc_upint,
    S_IRUGO | S_IWUSR,
    video_vsync_pts_inc_upint_show,
    video_vsync_pts_inc_upint_store),
    __ATTR_RO(device_resolution),
    __ATTR_RO(frame_addr),
    __ATTR_RO(frame_canvas_width),
    __ATTR_RO(frame_canvas_height),
    __ATTR_RO(frame_width),
    __ATTR_RO(frame_height),
    __ATTR_RO(frame_format),
    __ATTR_RO(frame_aspect_ratio),
    __ATTR_RO(frame_rate),
    __ATTR_RO(vframe_states),
    __ATTR_NULL
};

#ifdef CONFIG_PM
static int amvideo_class_suspend(struct device *dev, pm_message_t state)
{
    pm_state.event = state.event;

    if (state.event == PM_EVENT_SUSPEND) {
        pm_state.vpp_misc = READ_MPEG_REG(VPP_MISC);
        DisableVideoLayer();
        msleep(50);
    }

    return 0;
}

#ifdef CONFIG_SCREEN_ON_EARLY
extern void osd_resume_early(void);
extern void vout_pll_resume_early(void);
extern void resume_vout_early(void);
extern int power_key_pressed;
#endif

static int amvideo_class_resume(struct device *dev)
{
    if (pm_state.event == PM_EVENT_SUSPEND) {
        WRITE_MPEG_REG(VPP_MISC, pm_state.vpp_misc);
        pm_state.event = -1;
    }

#ifdef CONFIG_SCREEN_ON_EARLY
	if(power_key_pressed){
		vout_pll_resume_early();
		osd_resume_early();
		resume_vout_early();
		power_key_pressed = 0;
	}
#endif

    return 0;
}
#endif

static struct class amvideo_class = {
        .name = AMVIDEO_CLASS_NAME,
        .class_attrs = amvideo_class_attrs,
#ifdef CONFIG_PM
        .suspend = amvideo_class_suspend,
        .resume = amvideo_class_resume,
#endif
    };

static struct device *amvideo_dev;

int vout_notify_callback(struct notifier_block *block, unsigned long cmd , void *para)
{
    const vinfo_t *info;
    ulong flags;

    switch (cmd)
    {
  	case  VOUT_EVENT_MODE_CHANGE:
	info = get_current_vinfo();
	spin_lock_irqsave(&lock, flags);
  	vinfo = info;
	/* pre-calculate vsync_pts_inc in 90k unit */
    	vsync_pts_inc = 90000 * vinfo->sync_duration_den / vinfo->sync_duration_num;
	spin_unlock_irqrestore(&lock, flags);
	break;
	case VOUT_EVENT_OSD_PREBLEND_ENABLE:
	vpp_set_osd_layer_preblend(para);
	break;
	case VOUT_EVENT_OSD_DISP_AXIS:
	vpp_set_osd_layer_position(para);
	break;
    }
    return 0;
}

static struct notifier_block vout_notifier = {
    .notifier_call  = vout_notify_callback,
};

vframe_t* get_cur_dispbuf(void)
{
	return  cur_dispbuf;
}

static void vout_hook(void)
{
    vout_register_client(&vout_notifier);

    vinfo = get_current_vinfo();

    if (!vinfo) {
        set_current_vmode(VMODE_720P);

        vinfo = get_current_vinfo();
    }

    if (vinfo) {
        vsync_pts_inc = 90000 * vinfo->sync_duration_den / vinfo->sync_duration_num;
    }

#ifdef CONFIG_AM_VIDEO_LOG
    if (vinfo) {
        amlog_mask(LOG_MASK_VINFO, "vinfo = %p\n", vinfo);
        amlog_mask(LOG_MASK_VINFO, "display platform %s:\n", vinfo->name);
        amlog_mask(LOG_MASK_VINFO, "\tresolution %d x %d\n", vinfo->width, vinfo->height);
        amlog_mask(LOG_MASK_VINFO, "\taspect ratio %d : %d\n", vinfo->aspect_ratio_num, vinfo->aspect_ratio_den);
        amlog_mask(LOG_MASK_VINFO, "\tsync duration %d : %d\n", vinfo->sync_duration_num, vinfo->sync_duration_den);
    }
#endif
}

/*********************************************************/
static int __init video_early_init(void)
{
    logo_object_t  *init_logo_obj=NULL;
#ifdef CONFIG_AM_LOGO
    init_logo_obj = get_current_logo_obj();
#endif

#ifdef CONFIG_ARCH_MESON6TV
    WRITE_MPEG_REG_BITS(VPP_OFIFO_SIZE, 0x77f,
                        VPP_OFIFO_SIZE_BIT, VPP_OFIFO_SIZE_WID);
#endif
    if(NULL==init_logo_obj || !init_logo_obj->para.loaded)
    {
#ifndef CONFIG_ARCH_MESON6TV
    	WRITE_MPEG_REG_BITS(VPP_OFIFO_SIZE, 0x300,
                        VPP_OFIFO_SIZE_BIT, VPP_OFIFO_SIZE_WID);
#endif
   	CLEAR_MPEG_REG_MASK(VPP_VSC_PHASE_CTRL, VPP_PHASECTL_TYPE_INTERLACE);
#ifndef CONFIG_FB_AML_TCON
    	SET_MPEG_REG_MASK(VPP_MISC, VPP_OUT_SATURATE);
#endif
    	WRITE_MPEG_REG(VPP_HOLD_LINES, 0x08080808);
    }
    return 0;
}
static int __init video_init(void)
{
    int r = 0;
/*
#ifdef CONFIG_ARCH_MESON1
    ulong clk = clk_get_rate(clk_get_sys("clk_other_pll", NULL));
#elif !defined(CONFIG_ARCH_MESON3) && !defined(CONFIG_ARCH_MESON6)
    ulong clk = clk_get_rate(clk_get_sys("clk_misc_pll", NULL));
#endif
*/
#ifdef CONFIG_ARCH_MESON1
    ulong clk = clk_get_rate(clk_get_sys("clk_other_pll", NULL));
#elif defined(CONFIG_ARCH_MESON2)
    ulong clk = clk_get_rate(clk_get_sys("clk_misc_pll", NULL));
#endif

//#if !defined(CONFIG_ARCH_MESON3) && !defined(CONFIG_ARCH_MESON6)
#if MESON_CPU_TYPE <= MESON_CPU_TYPE_MESON2
    /* MALI clock settings */
    if ((clk <= 750000000) &&
        (clk >= 600000000)) {
        WRITE_CBUS_REG(HHI_MALI_CLK_CNTL,
                       (2 << 9)    |   // select misc pll as clock source
                       (1 << 8)    |   // enable clock gating
                       (2 << 0));      // Misc clk / 3
    } else {
        WRITE_CBUS_REG(HHI_MALI_CLK_CNTL,
                       (3 << 9)    |   // select DDR clock as clock source
                       (1 << 8)    |   // enable clock gating
                       (1 << 0));      // DDR clk / 2
    }
#endif

#ifdef RESERVE_CLR_FRAME
    alloc_keep_buffer();
#endif

    DisableVideoLayer();
    cur_dispbuf = NULL;

#ifdef FIQ_VSYNC
    /* enable fiq bridge */
	vsync_fiq_bridge.handle = vsync_bridge_isr;
	vsync_fiq_bridge.key=(u32)vsync_bridge_isr;
	vsync_fiq_bridge.name="vsync_bridge_isr";

    r = register_fiq_bridge_handle(&vsync_fiq_bridge);

    if (r) {
        amlog_level(LOG_LEVEL_ERROR, "video fiq bridge register error.\n");
        r = -ENOENT;
        goto err0;
    }
#endif

    /* sysfs node creation */
    r = class_register(&amvideo_class);
    if (r) {
        amlog_level(LOG_LEVEL_ERROR, "create video class fail.\n");
#ifdef FIQ_VSYNC
        free_irq(BRIDGE_IRQ, (void *)video_dev_id);
#else
        free_irq(INT_VIU_VSYNC, (void *)video_dev_id);
#endif
        goto err1;
    }

    /* create video device */
    r = register_chrdev(AMVIDEO_MAJOR, "amvideo", &amvideo_fops);
    if (r < 0) {
        amlog_level(LOG_LEVEL_ERROR, "Can't register major for amvideo device\n");
        goto err2;
    }

    amvideo_dev = device_create(&amvideo_class, NULL,
                                MKDEV(AMVIDEO_MAJOR, 0), NULL,
                                DEVICE_NAME);

    if (IS_ERR(amvideo_dev)) {
        amlog_level(LOG_LEVEL_ERROR, "Can't create amvideo device\n");
        goto err3;
    }

    init_waitqueue_head(&amvideo_trick_wait);

    vout_hook();

    disp_canvas[0] = (disp_canvas_index[2] << 16) | (disp_canvas_index[1] << 8) | disp_canvas_index[0];
    disp_canvas[1] = (disp_canvas_index[5] << 16) | (disp_canvas_index[4] << 8) | disp_canvas_index[3];

    vsync_fiq_up();

    vf_receiver_init(&video_vf_recv, RECEIVER_NAME, &video_vf_receiver, NULL);
    vf_reg_receiver(&video_vf_recv);

    vf_receiver_init(&video4osd_vf_recv, RECEIVER4OSD_NAME, &video4osd_vf_receiver, NULL);
    vf_reg_receiver(&video4osd_vf_recv);

#ifdef CONFIG_AM_VIDEO2
    set_clone_frame_rate(android_clone_rate, 0);
#endif

    return (0);

err3:
    unregister_chrdev(AMVIDEO_MAJOR, DEVICE_NAME);

err2:
#ifdef FIQ_VSYNC
    unregister_fiq_bridge_handle(&vsync_fiq_bridge);
#endif

err1:
    class_unregister(&amvideo_class);

#ifdef FIQ_VSYNC
err0:
#endif
    return r;
}

static void __exit video_exit(void)
{
    vf_unreg_receiver(&video_vf_recv);

    vf_unreg_receiver(&video4osd_vf_recv);

    DisableVideoLayer();

    vsync_fiq_down();

    device_destroy(&amvideo_class, MKDEV(AMVIDEO_MAJOR, 0));

    unregister_chrdev(AMVIDEO_MAJOR, DEVICE_NAME);

#ifdef FIQ_VSYNC
    unregister_fiq_bridge_handle(&vsync_fiq_bridge);
#endif

    class_unregister(&amvideo_class);
}
#ifdef CONFIG_KEEP_FRAME_RESERVED
static int video_probe(struct platform_device *pdev)
{
    int ret = 0;
    struct resource *res = NULL;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res) {
        amlog_level(LOG_LEVEL_ERROR, , "%s: failed to remap y addr\n", __FUNCTION__);
        ret = -ENOMEM;
        goto fail_get_res;
    }
    pr_info("%s y start %x, end %x\n", __func__, res->start, res->end);
    keep_y_addr = res->start;
    y_buffer_size = res->end - res->start + 1;
    keep_y_addr_remap = ioremap_nocache(keep_y_addr, y_buffer_size);
    if (!keep_y_addr_remap) {
        amlog_level(LOG_LEVEL_ERROR, ,"%s: failed to remap y addr\n", __FUNCTION__);
        ret = -ENOMEM;
        goto fail_ioremap;
    }

    res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
    if (!res) {
        amlog_level(LOG_LEVEL_ERROR,  "video: can't get memory resource\n");
        ret = -ENOMEM;
        goto fail_get_res;
    }
    pr_info("%s u start %x, end %x\n", __func__, res->start, res->end);
    keep_u_addr = res->start;
    u_buffer_size = res->end - res->start + 1;
    keep_u_addr_remap = ioremap_nocache(keep_u_addr, u_buffer_size);
    if (!keep_u_addr_remap) {
        amlog_level(LOG_LEVEL_ERROR,  "%s: failed to remap u addr\n", __FUNCTION__);
        ret = -ENOMEM;
        goto fail_ioremap;
    }

    res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
    if (!res) {
        amlog_level(LOG_LEVEL_ERROR,  "video: can't get memory resource\n");
        ret = -ENOMEM;
        goto fail_get_res;
    }
    pr_info("%s v start %x, end %x\n", __func__, res->start, res->end);
    keep_v_addr = res->start;
    v_buffer_size = res->end - res->start + 1;
    keep_v_addr_remap = ioremap_nocache(keep_v_addr, v_buffer_size);
    if (!keep_v_addr_remap) {
         amlog_level(LOG_LEVEL_ERROR,  "%s: failed to remap v addr\n", __FUNCTION__);
        ret = -ENOMEM;
         goto fail_ioremap;
    }
    goto add_video_init;

fail_get_res:
fail_ioremap:
    if(keep_v_addr_remap)
         iounmap(keep_v_addr_remap);
    keep_v_addr_remap = NULL;
    keep_v_addr = 0;
    if(keep_u_addr_remap)
        iounmap(keep_u_addr_remap);
    keep_u_addr_remap = NULL;
    keep_u_addr = 0;
    if(keep_y_addr_remap)
        iounmap(keep_y_addr_remap);
    keep_y_addr_remap = NULL;
    keep_y_addr = 0;

add_video_init:
    video_init();
    return ret;

}

static int video_remove(struct platform_device *pdev)
{
    video_exit();
    return 0;
}

static struct platform_driver video_plat_driver = {
    .probe		= video_probe,
    .remove 	= video_remove,
    .driver 	         = {
    .name	         = "video",
    }
};

static int __init video_drv_init(void)
{
    int ret = 0;
    ret = platform_driver_register(&video_plat_driver);
    if (ret != 0) {
        printk(KERN_ERR "failed to register video module, error %d\n", ret);
        return -ENODEV;
    }
    return ret;
}
static void __exit video_drv_exit(void)
{
    platform_driver_unregister(&video_plat_driver);
}
#endif

MODULE_PARM_DESC(debug_flag, "\n debug_flag\n");
module_param(debug_flag, uint, 0664);

MODULE_PARM_DESC(vsync_enter_line_max, "\n vsync_enter_line_max\n");
module_param(vsync_enter_line_max, uint, 0664);

MODULE_PARM_DESC(vsync_exit_line_max, "\n vsync_exit_line_max\n");
module_param(vsync_exit_line_max, uint, 0664);

module_param(underflow, uint, 0664);
MODULE_PARM_DESC(underflow, "\n Underflow count \n");

module_param(next_peek_underflow, uint, 0664);
MODULE_PARM_DESC(skip, "\n Underflow count \n");

MODULE_PARM_DESC(video_3d_format, "\n video_3d_format\n");
module_param(video_3d_format, uint, 0664);

arch_initcall(video_early_init);

#ifdef CONFIG_KEEP_FRAME_RESERVED
module_init(video_drv_init);
module_exit(video_drv_exit);
#else
module_init(video_init);
module_exit(video_exit);
#endif

MODULE_PARM_DESC(smooth_sync_enable, "\n smooth_sync_enable\n");
module_param(smooth_sync_enable, uint, 0664);

#ifdef CONFIG_AM_VIDEO2
MODULE_PARM_DESC(video_play_clone_rate, "\n video_play_clone_rate\n");
module_param(video_play_clone_rate, uint, 0664);

MODULE_PARM_DESC(android_clone_rate, "\n android_clone_rate\n");
module_param(android_clone_rate, uint, 0664);

MODULE_PARM_DESC(noneseamless_play_clone_rate, "\n noneseamless_play_clone_rate\n");
module_param(noneseamless_play_clone_rate, uint, 0664);

#endif

MODULE_DESCRIPTION("AMLOGIC video output driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tim Yao <timyao@amlogic.com>");
