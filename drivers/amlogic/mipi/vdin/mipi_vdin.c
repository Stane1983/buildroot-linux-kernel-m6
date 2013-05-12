/*******************************************************************
 *
 *  Copyright C 2012 by Amlogic, Inc. All Rights Reserved.
 *
 *  Description:
 *
 *  Author: Amlogic Software
 *  Created: 2012/3/13   19:46
 *
 *******************************************************************/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/etherdevice.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/wait.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/dma-mapping.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <asm/atomic.h>
#include <linux/spinlock.h>
#include <linux/amports/canvas.h>
#include <linux/amports/vframe.h>
#include <linux/amports/vframe_provider.h>
#include <linux/amports/vframe_receiver.h>
#include <linux/ge2d/ge2d_main.h>
#include <linux/ge2d/ge2d.h>

#include <linux/videodev2.h>
#include <mach/am_regs.h>
#include <mach/mipi_phy_reg.h>
#include <linux/mipi/am_mipi_csi2.h>

#include "../common/bufq.h"

#include "../../tvin_camera/tvin_global.h"
#include "../../tvin_camera/tvin_notifier.h"
#include "../../tvin_camera/vdin.h"
#include "../../tvin_camera/vdin_ctl.h"
#include "../../tvin_camera/tvin_format_table.h"

//#define TEST_FRAME_RATE

#define MIPI_SKIP_FRAME_NUM 1

#define AML_MIPI_DST_Y_CANVAS  MIPI_CANVAS_INDEX

#define MAX_MIPI_COUNT 1

//#define CANVAS_SIZE_6_500M (2592*2*1952*6 + BT656IN_ANCI_DATA_SIZE)
//#define CANVAS_SIZE_6_300M (2048*2*1536*6 + BT656IN_ANCI_DATA_SIZE)

#define RECEIVER_NAME   "mipi"

#ifdef TEST_FRAME_RATE
static int time_count = 0;
#endif

typedef struct vdin_ops_privdata_s {
    int                  dev_id;
    int                  vdin_num;
	
    am_csi2_hw_t hw_info;

    am_csi2_input_t input;
    am_csi2_output_t output;

    struct mutex              buf_lock; /* lock for buff */
    unsigned                     watchdog_cnt;
	
    mipi_buf_t                  out_buff;

    unsigned                    reset_flag;
    bool                           done_flag;
    bool                           run_flag;
    wait_queue_head_t	  complete;

    ge2d_context_t           *context;
    config_para_ex_t        ge2d_config;

    unsigned char             mipi_vdin_skip;

    unsigned char            wr_canvas_index;
    unsigned char            canvas_total_count;

    unsigned                   pbufAddr;
    unsigned                   decbuf_size;

    unsigned char mirror;

    struct tvin_parm_s      param;
} vdin_ops_privdata_t;

static int am_csi2_vdin_init(am_csi2_t* dev);
static int am_csi2_vdin_streamon(am_csi2_t* dev);
static int am_csi2_vdin_streamoff(am_csi2_t* dev);
static int am_csi2_vdin_fillbuff(am_csi2_t* dev);
static struct am_csi2_pixel_fmt* getPixelFormat(u32 fourcc, bool input);
static int am_csi2_vdin_uninit(am_csi2_t* dev);
static bool checkframe(int wr_id,am_csi2_input_t* input);

extern void convert422_to_nv21_vdin(unsigned char* src, unsigned char* dst_y, unsigned char *dst_uv, unsigned int size);
extern void swap_vdin_y(unsigned char* src, unsigned char* dst, unsigned int size);
extern void swap_vdin_uv(unsigned char* src, unsigned char* dst, unsigned int size);
static struct vdin_ops_privdata_s csi2_vdin_data[]=
{
    {
        .dev_id = -1,        
        .vdin_num = -1,
        .hw_info = {0},
        .input = {0},
        .output = {0},
        .watchdog_cnt = 0,
        .reset_flag = 0,
        .done_flag = true,
        .run_flag = false,
        .context = NULL,
        .mipi_vdin_skip = MIPI_SKIP_FRAME_NUM,
        .wr_canvas_index = 0xff,
        .canvas_total_count = 0,
        .pbufAddr = 0,
        .decbuf_size = 0,
        .mirror = 0,
    },
};

const struct am_csi2_ops_s am_csi2_vdin =
{
    .mode = AM_CSI2_VDIN,
    .getPixelFormat = getPixelFormat,
    .init = am_csi2_vdin_init,
    .streamon = am_csi2_vdin_streamon,
    .streamoff = am_csi2_vdin_streamoff,
    .fill = am_csi2_vdin_fillbuff,
    .uninit = am_csi2_vdin_uninit,
    .privdata = &csi2_vdin_data[0],
    .data_num = ARRAY_SIZE(csi2_vdin_data),
};


static const struct am_csi2_pixel_fmt am_csi2_input_pix_formats_vdin[] = 
{
    {
        .name = "4:2:2, packed, UYVY",
        .fourcc = V4L2_PIX_FMT_UYVY,
        .depth = 16,
    },
    {
        .name = "12  Y/CbCr 4:2:0",
        .fourcc = V4L2_PIX_FMT_NV12,
        .depth = 12,
    },
    {
        .name = "12  Y/CbCr 4:2:0",
        .fourcc = V4L2_PIX_FMT_NV21,
        .depth = 12,
    },
};

static const struct am_csi2_pixel_fmt am_csi2_output_pix_formats_vdin[] =
{
    {
        .name = "RGB565",
        .fourcc = V4L2_PIX_FMT_RGB565,
        .depth = 16,
    },
    {
        .name = "RGB888 (24)",
        .fourcc = V4L2_PIX_FMT_RGB24, /* 24  RGB-8-8-8 */
        .depth = 24,
    },
    {
        .name = "BGR888 (24)",
        .fourcc = V4L2_PIX_FMT_BGR24, /* 24  BGR-8-8-8 */
        .depth = 24,
    },
    {
        .name = "12  Y/CbCr 4:2:0",
        .fourcc = V4L2_PIX_FMT_NV12,
        .depth = 12,
    },
    {
        .name = "12  Y/CbCr 4:2:0",
        .fourcc = V4L2_PIX_FMT_NV21,
        .depth = 12,
    },
    {
        .name = "YUV420P",
        .fourcc = V4L2_PIX_FMT_YUV420,
        .depth = 12,
    },
};

/* ------------------------------------------------------------------
       vframe receiver callback
   ------------------------------------------------------------------*/

static struct vframe_receiver_s mipi_vf_recv[MAX_MIPI_COUNT];

static inline vframe_t *mipi_vf_peek(void)
{
    return vf_peek(RECEIVER_NAME);
}

static inline vframe_t *mipi_vf_get(void)
{
    return vf_get(RECEIVER_NAME);
}

static inline void mipi_vf_put(vframe_t *vf)
{
    struct vframe_provider_s *vfp = vf_get_provider(RECEIVER_NAME);
    if (vfp) {
        vf_put(vf, RECEIVER_NAME);
    }
    return;
}

static int mipi_vdin_receiver_event_fun(int type, void* data, void* private_data)
{
    vdin_ops_privdata_t*mipi_data = (vdin_ops_privdata_t*)private_data;
    switch(type){
        case VFRAME_EVENT_PROVIDER_VFRAME_READY:
            if(mipi_data){
                if(mipi_data->reset_flag == 1)
                    mipi_data->reset_flag = 0;
                if((mipi_vf_peek()!=NULL)&&(mipi_data->done_flag == false)){
                    mipi_data->done_flag = true;
                    wake_up_interruptible(&mipi_data->complete);
                }
            }
            break;
        case VFRAME_EVENT_PROVIDER_START:
            break;
        case VFRAME_EVENT_PROVIDER_UNREG:
            break;
        default:
            break;     
    }
    return 0;
}

static const struct vframe_receiver_op_s mipi_vf_receiver =
{
    .event_cb = mipi_vdin_receiver_event_fun
};

/* ------------------------------------------------------------------
      vdin callback
   ------------------------------------------------------------------*/

static int mipi_vdin_canvas_init(vdin_ops_privdata_t* data,unsigned int mem_start, unsigned int mem_size)
{
    int i, canvas_start_index ;
    unsigned int canvas_width  = (data->input.active_pixel*data->input.depth)>>3;
    unsigned int canvas_height = data->input.active_line;
    unsigned decbuf_start = mem_start;
    unsigned decbuf_size   = canvas_width*canvas_height + 0x1000;

#if 0
    if( mem_size >= CANVAS_SIZE_6_500M){
	canvas_width  = 2592 << 1;
	canvas_height = 1952;
	decbuf_size   = canvas_width*canvas_height + 0x1000;
	mipi_dbg("mipi_vdin_canvas_init--5M canvas config\n");
    }else if( mem_size >= CANVAS_SIZE_6_300M ){
	canvas_width  = 2048 << 1;
	canvas_height = 1536;
	decbuf_size  = canvas_width*canvas_height + 0x1000;
	mipi_dbg("mipi_vdin_canvas_init--3M canvas config\n");
    }
#endif

    i = (unsigned)(mem_size  / decbuf_size);

    data->canvas_total_count = (VDIN_VF_POOL_MAX_SIZE > i)? i : VDIN_VF_POOL_MAX_SIZE;
    data->decbuf_size = decbuf_size;

    if((data->param.port == TVIN_PORT_MIPI_NV12)||(data->param.port == TVIN_PORT_MIPI_NV21)){
        if(data->vdin_num)
            canvas_start_index = tvin_canvas_tab[1][0];
        else
            canvas_start_index = tvin_canvas_tab[0][0];

        for ( i = 0; i < data->canvas_total_count; i++){
            canvas_config(canvas_start_index + i, decbuf_start + i * data->decbuf_size,
                data->input.active_pixel, canvas_height, CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);
        }
        for ( i = 0; i < data->canvas_total_count; i++){
            canvas_config(VDIN_START_CANVAS_CHROMA_OFFSET+canvas_start_index + i, decbuf_start + (i * data->decbuf_size)+(data->input.active_line*data->input.active_pixel),
                data->input.active_pixel, canvas_height/2, CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);
        }
    }else{
        if(data->vdin_num)
            canvas_start_index = tvin_canvas_tab[1][0];
        else
            canvas_start_index = tvin_canvas_tab[0][0];

        for ( i = 0; i < data->canvas_total_count; i++){
            canvas_config(canvas_start_index + i, decbuf_start + i * data->decbuf_size,
                canvas_width, canvas_height, CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);
        }
    }

    set_tvin_canvas_info(canvas_start_index ,data->canvas_total_count);
    return 0;
}

static inline u32 mipi_index2canvas(vdin_ops_privdata_t* data,u32 index)
{
    int tv_group_index  = 0;
    u32 chroma_id = 0;
    u32 ret= 0xff;
    if(data->vdin_num)
        tv_group_index = 1;
    else
        tv_group_index = 0;

    if(index < data->canvas_total_count){
        ret =  tvin_canvas_tab[tv_group_index][index];

        if((data->param.port == TVIN_PORT_MIPI_NV12)||(data->param.port == TVIN_PORT_MIPI_NV21)){
            chroma_id = ret + VDIN_START_CANVAS_CHROMA_OFFSET;
            chroma_id = chroma_id<<8;
        }
    }
    return (u32)(chroma_id|ret);
}

static void mipi_send_buff_to_display_fifo(vdin_ops_privdata_t* data,vframe_t * info)
{ 
    if((data->param.port == TVIN_PORT_MIPI_NV12)||(data->param.port == TVIN_PORT_MIPI_NV21))
        info->type = VIDTYPE_PROGRESSIVE | VIDTYPE_VIU_FIELD | VIDTYPE_VIU_NV21;
    else
        info->type = VIDTYPE_VIU_SINGLE_PLANE | VIDTYPE_VIU_422 | VIDTYPE_VIU_FIELD | VIDTYPE_PROGRESSIVE ;
    info->width= data->hw_info.active_pixel;
    info->height = data->hw_info.active_line;
    info->duration = 960000/data->param.fmt_info.frame_rate;
    info->canvas0Addr = info->canvas1Addr = mipi_index2canvas(data,data->wr_canvas_index);
}

static void mipi_wr_vdin_canvas(vdin_ops_privdata_t* data, int index)
{
    vdin_set_canvas_id(data->vdin_num, mipi_index2canvas(data,index));
}

static void init_mipi_vdin_parameter(vdin_ops_privdata_t* data,fmt_info_t fmt_info)
{
    data->param.fmt_info.fmt = fmt_info.fmt;
    data->param.fmt_info.frame_rate = fmt_info.frame_rate;
    switch(fmt_info.fmt){
        case TVIN_SIG_FMT_MAX+1:
        case 0xFFFF:     //default camera
            data->param.fmt_info.h_active = fmt_info.h_active;
            data->param.fmt_info.v_active = fmt_info.v_active;
            data->param.fmt_info.hsync_phase = fmt_info.hsync_phase;
            data->param.fmt_info.vsync_phase = fmt_info.vsync_phase;
            mipi_dbg("%dx%d input mode is selected for camera, \n",fmt_info.h_active,fmt_info.v_active);
            break;
        case TVIN_SIG_FMT_NULL:
        default:
            break;
    }
    return;
}

static void csi2_vdin_check(vdin_ops_privdata_t* data)
{
    data->watchdog_cnt++;
    if(data->watchdog_cnt > 20){
        am_mipi_csi2_uninit();
        data->hw_info.frame = NULL;
        data->mipi_vdin_skip = MIPI_SKIP_FRAME_NUM;
        data->reset_flag = 1;
        wake_up_interruptible(&data->complete);
        data->done_flag = true;
        am_mipi_csi2_init(&data->hw_info);
        data->watchdog_cnt = 0;
        data->param.status = TVIN_SIG_STATUS_NOSIG;
        mipi_error("[mipi_vdin]:csi2_vdin_check-- time out !!!!\n");
        return ;
    }

    if (data->run_flag != true)
        mipi_error("csi2_vdin_check: mipi still not run. \n");
    return ;
}

static void start_mipi_vdin(vdin_ops_privdata_t* data,unsigned int mem_start, unsigned int mem_size, fmt_info_t fmt_info,enum tvin_port_e port)
{
    if(data->run_flag == true){
        mipi_dbg("mipi_vdin is processing, don't do starting operation \n");
        return;
    }

    mipi_dbg("start_mipi_vdin. \n");
    data->param.port = port;
    mipi_vdin_canvas_init(data,mem_start, mem_size);
    init_mipi_vdin_parameter(data,fmt_info);
    data->wr_canvas_index = 0xff;
    data->reset_flag = 0;
    data->done_flag = true;
    data->hw_info.frame = NULL;
    data->mipi_vdin_skip = MIPI_SKIP_FRAME_NUM;
    am_mipi_csi2_init(&data->hw_info);
    data->watchdog_cnt = 0;
    data->run_flag = true;
    return;
}

static void stop_mipi_vdin(vdin_ops_privdata_t * data)
{
    if(data->run_flag == true){
        mipi_dbg("stop_mipi_vdin.\n");
        am_mipi_csi2_uninit();
        data->param.fmt_info.fmt= TVIN_SIG_FMT_NULL;
        data->run_flag = false;
    }else{
         mipi_error("stop_mipi_vdin is not started yet. \n");
    }
    return;
}

static int mipi_vdin_check_callback(struct notifier_block *block, unsigned long cmd , void *para)
{
    int ret = 0;
    vdin_dev_t *p = NULL;
    vdin_ops_privdata_t * data = NULL;
    switch(cmd){
        case TVIN_EVENT_INFO_CHECK:
            if(para != NULL){
                p = (vdin_dev_t*)para;
                if ((p->para.port != TVIN_PORT_MIPI)&&
                  (p->para.port != TVIN_PORT_MIPI_NV12)&&
                  (p->para.port != TVIN_PORT_MIPI_NV21)){
                    mipi_dbg("[mipi_vdin]: ignore TVIN_EVENT_INFO_CHECK (port=%d)\n", p->para.port);
                    return ret;
                }
                data = (vdin_ops_privdata_t*)p->priv_data;
                csi2_vdin_check(data);
                if ((data->param.fmt_info.fmt != p->para.fmt_info.fmt) ||
                    (data->param.status!= p->para.status)){
                    vdin_info_update(p, &data->param);
                }
                ret = NOTIFY_STOP_MASK;
            }
            break;
        default:
            break;
    }
    return ret;
}

static int mipi_vdin_run(vdin_dev_t* devp,struct vframe_s *vf)
{
    vdin_ops_privdata_t* data = (vdin_ops_privdata_t*)devp->priv_data;

    data->watchdog_cnt = 0;

    if (data->wr_canvas_index == 0xff) {
        mipi_wr_vdin_canvas(data,0);
        data->wr_canvas_index = 0;
        return 0;
    }

#ifdef TEST_FRAME_RATE
    if(time_count==0){
        mipi_error("[mipi_vdin]:mipi_vdin_run ---- time start\n");
    }
    time_count++;
    if(time_count>49){
        time_count = 0;
    }
#endif

    if(data->mipi_vdin_skip>0){
        data->mipi_vdin_skip--;
        vf->type = INVALID_VDIN_INPUT;
    }else if(checkframe(data->wr_canvas_index,&data->input) == true){
        mipi_send_buff_to_display_fifo(data,vf);
        data->wr_canvas_index++;
    }else{
        vf->type = INVALID_VDIN_INPUT;
    }

    if (data->wr_canvas_index >= data->canvas_total_count)
        data->wr_canvas_index = 0;

    mipi_wr_vdin_canvas(data,data->wr_canvas_index);

    //WRITE_CBUS_REG(CSI2_INTERRUPT_CTRL_STAT, (1 << CSI2_CFG_VS_FAIL_INTERRUPT_CLR) | (1 << CSI2_CFG_VS_FAIL_INTERRUPT));
    // Clear error flag
    WRITE_CBUS_REG(CSI2_ERR_STAT0, 0);
    return 0;
}

static struct tvin_dec_ops_s mipi_vdin_op = {
    .dec_run = mipi_vdin_run,
};

static struct notifier_block mipi_check_cb = {
    .notifier_call = mipi_vdin_check_callback,
};

static int mipi_vdin_notifier_callback(struct notifier_block *block, unsigned long cmd , void *para)
{
    int ret = 0;
    vdin_dev_t *p = NULL;
    vdin_ops_privdata_t * data = NULL;
    switch(cmd){
        case TVIN_EVENT_DEC_START:
            if(para != NULL){
                p = (vdin_dev_t*)para;
                if ((p->para.port != TVIN_PORT_MIPI)&&
                  (p->para.port != TVIN_PORT_MIPI_NV12)&&
                  (p->para.port != TVIN_PORT_MIPI_NV21)){
                    mipi_dbg("[mipi_vdin]: ignore TVIN_EVENT_DEC_START (port=%d)\n", p->para.port);
                    return ret;
                }
                mipi_dbg("mipi_vdin_notifier_callback, para = %x ,mem_start = %x, port = %d, format = %d, ca-_flag = %d.\n" ,
                        (unsigned int)para, p->mem_start, p->para.port, p->para.fmt_info.fmt, p->para.flag);
                data = (vdin_ops_privdata_t*)p->priv_data;
                start_mipi_vdin(data,p->mem_start,p->mem_size, p->para.fmt_info,p->para.port);
                data->param.status = TVIN_SIG_STATUS_STABLE;
                vdin_info_update(p, &data->param);
                tvin_dec_register(p, &mipi_vdin_op);
                tvin_check_notifier_register(&mipi_check_cb);
                ret = NOTIFY_STOP_MASK;
            }
            break;
        case TVIN_EVENT_DEC_STOP:
            if(para != NULL){
                p = (vdin_dev_t*)para;
                if ((p->para.port != TVIN_PORT_MIPI)&&
                  (p->para.port != TVIN_PORT_MIPI_NV12)&&
                  (p->para.port != TVIN_PORT_MIPI_NV21)){
                    mipi_dbg("[mipi_vdin]: ignore TVIN_EVENT_DEC_STOP (port=%d)\n", p->para.port);
                    return ret;
                }
                data = (vdin_ops_privdata_t*)p->priv_data;
                stop_mipi_vdin(data);
                tvin_dec_unregister(p);
                tvin_check_notifier_unregister(&mipi_check_cb);
                ret = NOTIFY_STOP_MASK;
            }
            break;
        default:
            break;
    }
    return ret;
}

static struct notifier_block mipi_notifier_cb = {
    .notifier_call = mipi_vdin_notifier_callback,
};

/* ------------------------------------------------------------------*/

static int get_input_format(int v4l2_format)
{
    int format = GE2D_FORMAT_M24_NV21;
    switch(v4l2_format){
        case V4L2_PIX_FMT_UYVY:
            format = GE2D_FORMAT_S16_YUV422;
            break;
        case V4L2_PIX_FMT_NV12:
            format = GE2D_FORMAT_M24_NV21;
            break;
        case V4L2_PIX_FMT_NV21:
            format = GE2D_FORMAT_M24_NV12;
            break;
        default:
            break;            
    }   
    return format;
}

static int get_output_format(int v4l2_format)
{
    int format = GE2D_FORMAT_M24_NV21;
    switch(v4l2_format){
        case V4L2_PIX_FMT_RGB565:
            format = GE2D_FORMAT_S16_RGB_565;
            break;
        case V4L2_PIX_FMT_BGR24:
            format = GE2D_FORMAT_S24_BGR;
            break;
        case V4L2_PIX_FMT_RGB24:
            format = GE2D_FORMAT_S24_RGB;
            break;
        case V4L2_PIX_FMT_NV12:
            format = GE2D_FORMAT_M24_NV21;
            break;
        case V4L2_PIX_FMT_NV21:
            format = GE2D_FORMAT_M24_NV12;
            break;
        case V4L2_PIX_FMT_YUV420:
            format = GE2D_FORMAT_S8_Y;
            break;
        default:
            break;            
    }   
    return format;
}

static int config_canvas_index(unsigned address,int v4l2_format,unsigned w,unsigned h,int id)
{
    int canvas = -1;
    unsigned char canvas_y = 0;
    switch(v4l2_format){
        case V4L2_PIX_FMT_RGB565:
        case V4L2_PIX_FMT_UYVY:
            canvas = AML_MIPI_DST_Y_CANVAS+(id*3);
            canvas_config(AML_MIPI_DST_Y_CANVAS+(id*3),(unsigned long)address,w*2, h, CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);
            break; 
        case V4L2_PIX_FMT_BGR24:
        case V4L2_PIX_FMT_RGB24:
            canvas = AML_MIPI_DST_Y_CANVAS+(id*3);
            canvas_config(AML_MIPI_DST_Y_CANVAS+(id*3),(unsigned long)address,w*3, h, CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);
            break; 
        case V4L2_PIX_FMT_NV12:
        case V4L2_PIX_FMT_NV21: 
            canvas_y = AML_MIPI_DST_Y_CANVAS+(id*3);
            canvas = (canvas_y | ((canvas_y+1)<<8));
            canvas_config(canvas_y,(unsigned long)address,w, h, CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);
            canvas_config(canvas_y+1,(unsigned long)(address+w*h),w, h/2, CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);
            break;
        case V4L2_PIX_FMT_YUV420:
            canvas_y = AML_MIPI_DST_Y_CANVAS+(id*3);
            canvas = (canvas_y | ((canvas_y+1)<<8)|((canvas_y+2)<<16));
            canvas_config(canvas_y,(unsigned long)address,w, h, CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);
            canvas_config(canvas_y+1,(unsigned long)(address+w*h),w/2, h/2, CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);
            canvas_config(canvas_y+2,(unsigned long)(address+w*h*5/4),w/2, h/2, CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);
            break;
        default:
            break;
    }
    return canvas;
}

static int calc_zoom(int* top ,int* left , int* bottom, int* right, int zoom)
{
    u32 screen_width, screen_height ;
    s32 start, end;
    s32 video_top, video_left, temp;
    u32 video_width, video_height;
    u32 ratio_x = 0;
    u32 ratio_y = 0;

    if(zoom<100)
        zoom = 100;

    video_top = *top;
    video_left = *left;
    video_width = *right - *left +1;
    video_height = *bottom - *top +1;

    screen_width = video_width * zoom / 100;
    screen_height = video_height * zoom / 100;

    ratio_x = (video_width << 18) / screen_width;
    if (ratio_x * screen_width < (video_width << 18)) {
        ratio_x++;
    }
    ratio_y = (video_height << 18) / screen_height;

    /* vertical */
    start = video_top + video_height / 2 - (video_height << 17)/ratio_y;
    end   = (video_height << 18) / ratio_y + start - 1;

    if (start < video_top) {
        temp = ((video_top - start) * ratio_y) >> 18;
        *top = temp;
    } else {
        *top = 0;
    }

    temp = *top + (video_height * ratio_y >> 18);
    *bottom = (temp <= (video_height - 1)) ? temp : (video_height - 1);

    /* horizontal */
    start = video_left + video_width / 2 - (video_width << 17) /ratio_x;
    end   = (video_width << 18) / ratio_x + start - 1;
    if (start < video_left) {
        temp = ((video_left - start) * ratio_x) >> 18;
        *left = temp;
    } else {
        *left = 0;
    }

    temp = *left + (video_width * ratio_x >> 18);
    *right = (temp <= (video_width - 1)) ? temp : (video_width - 1);
    return 0;
}


static int ge2d_process(vdin_ops_privdata_t* data, vframe_t* in, am_csi2_frame_t* out, ge2d_context_t *context,config_para_ex_t* ge2d_config)
{
    int ret = -1;
    int src_top = 0,src_left = 0  ,src_width = 0, src_height = 0;
    canvas_t cs,cd ;
    int current_mirror = 0;
    int dst_canvas = -1;
    int cur_angle = data->output.angle;

    if((!data)||(!in)||(!out)){
        mipi_error("[mipi_vdin]:ge2d_process-- pointer NULL !!!!\n");
        return ret;
    }

    current_mirror = data->mirror;

    src_top = 0;
    src_left = 0;

    src_width = in->width;
    src_height = in->height ;

    if(data->output.zoom>100){
        int bottom = 0, right = 0;
        bottom = src_height -src_top -1;
        right = src_width -src_left -1;
        calc_zoom(&src_top, &src_left, &bottom, &right, data->output.zoom);
        src_width = right - src_left + 1;
        src_height = bottom - src_top + 1;
    }

    memset(ge2d_config,0,sizeof(config_para_ex_t));

    dst_canvas = out->index;
    if(dst_canvas<0){
        mipi_error("[mipi_vdin]:ge2d_process-- dst canvas invaild !!!!\n");
        return ret;
    }

    cur_angle = (360 - cur_angle%360);
    /* data operating. */ 
    ge2d_config->alu_const_color= 0;//0x000000ff;
    ge2d_config->bitmask_en  = 0;
    ge2d_config->src1_gb_alpha = 0;//0xff;
    ge2d_config->dst_xy_swap = 0;

    canvas_read(in->canvas0Addr&0xff,&cs);
    ge2d_config->src_planes[0].addr = cs.addr;
    ge2d_config->src_planes[0].w = cs.width;
    ge2d_config->src_planes[0].h = cs.height;
    canvas_read(dst_canvas&0xff,&cd);
    ge2d_config->dst_planes[0].addr = cd.addr;
    ge2d_config->dst_planes[0].w = cd.width;
    ge2d_config->dst_planes[0].h = cd.height;
    ge2d_config->src_key.key_enable = 0;
    ge2d_config->src_key.key_mask = 0;
    ge2d_config->src_key.key_mode = 0;
    ge2d_config->src_para.canvas_index=in->canvas0Addr;
    ge2d_config->src_para.mem_type = CANVAS_TYPE_INVALID;
    ge2d_config->src_para.format = get_input_format(data->input.fourcc);
    ge2d_config->src_para.fill_color_en = 0;
    ge2d_config->src_para.fill_mode = 0;
    ge2d_config->src_para.x_rev = 0;
    ge2d_config->src_para.y_rev = 0;
    ge2d_config->src_para.color = 0xffffffff;
    ge2d_config->src_para.top = 0;
    ge2d_config->src_para.left = 0;
    ge2d_config->src_para.width = in->width;
    ge2d_config->src_para.height = in->height;
    ge2d_config->src2_para.mem_type = CANVAS_TYPE_INVALID;
    ge2d_config->dst_para.canvas_index = dst_canvas&0xff;

    if(data->output.fourcc != V4L2_PIX_FMT_YUV420)
        ge2d_config->dst_para.canvas_index = dst_canvas;

    ge2d_config->dst_para.mem_type = CANVAS_TYPE_INVALID;
    ge2d_config->dst_para.format = get_output_format(data->output.fourcc)|GE2D_LITTLE_ENDIAN;
    ge2d_config->dst_para.fill_color_en = 0;
    ge2d_config->dst_para.fill_mode = 0;
    ge2d_config->dst_para.x_rev = 0;
    ge2d_config->dst_para.y_rev = 0;
    ge2d_config->dst_para.color = 0;
    ge2d_config->dst_para.top = 0;
    ge2d_config->dst_para.left = 0;
    ge2d_config->dst_para.width = data->output.output_pixel;
    ge2d_config->dst_para.height = data->output.output_line;

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
        mipi_error("[mipi_vdin]:ge2d_process-- ge2d configing error!!!!\n");
        return ret;
    }
    mipi_dbg("[mipi_vdin]:ge2d_process, src: %d,%d-%dx%d. dst:%dx%d , dst canvas:0x%x\n",
		src_left ,src_top ,src_width, src_height,
		ge2d_config->dst_para.width,ge2d_config->dst_para.height, ge2d_config->dst_para.canvas_index);
    stretchblt_noalpha(context,src_left ,src_top ,src_width, src_height,0,0,ge2d_config->dst_para.width,ge2d_config->dst_para.height );

	/* for cr of  yuv420p. */
    if(data->output.fourcc == V4L2_PIX_FMT_YUV420) {
        /* for cb. */
        canvas_read((dst_canvas>>8)&0xff,&cd);
        ge2d_config->dst_planes[0].addr = cd.addr;
        ge2d_config->dst_planes[0].w = cd.width;
        ge2d_config->dst_planes[0].h = cd.height;
        ge2d_config->dst_para.canvas_index=(dst_canvas>>8)&0xff;
        ge2d_config->dst_para.format=GE2D_FORMAT_S8_CB |GE2D_LITTLE_ENDIAN;
        ge2d_config->dst_para.width = data->output.output_pixel/2;
        ge2d_config->dst_para.height = data->output.output_line/2;
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
            mipi_error("[mipi_vdin]:ge2d_process-- ge2d configing error!!!!\n");
            return ret;
        }
        mipi_dbg("[mipi_vdin]:ge2d_process, src: %d,%d-%dx%d. dst:%dx%d\n",
		src_left ,src_top ,src_width, src_height,
		ge2d_config->dst_para.width,ge2d_config->dst_para.height );
        stretchblt_noalpha(context, src_left, src_top, src_width, src_height,
          0, 0, ge2d_config->dst_para.width,ge2d_config->dst_para.height);


        canvas_read((dst_canvas>>16)&0xff,&cd);
        ge2d_config->dst_planes[0].addr = cd.addr;
        ge2d_config->dst_planes[0].w = cd.width;
        ge2d_config->dst_planes[0].h = cd.height;
        ge2d_config->dst_para.canvas_index=(dst_canvas>>16)&0xff;
        ge2d_config->dst_para.format=GE2D_FORMAT_S8_CR|GE2D_LITTLE_ENDIAN;
        ge2d_config->dst_para.width = data->output.output_pixel/2;
        ge2d_config->dst_para.height = data->output.output_line/2;
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
            mipi_error("[mipi_vdin]:ge2d_process-- ge2d configing error!!!!\n");
            return ret;
        }
        stretchblt_noalpha(context, src_left, src_top, src_width, src_height,
          0, 0, ge2d_config->dst_para.width,ge2d_config->dst_para.height);
    }
    return 0;
}

static bool is_need_ge2d_process(vdin_ops_privdata_t* data)
{
    bool ret = false;
    if(!data){
        mipi_error("[mipi_vdin]:is_need_ge2d_process-- pointer NULL !!!!\n");
        return ret;
    }
    // check the color space
    if(data->input.fourcc == data->output.fourcc){
        ret = false;
    }else if((data->input.fourcc == V4L2_PIX_FMT_UYVY)&&((data->output.fourcc == V4L2_PIX_FMT_NV12)||(data->output.fourcc == V4L2_PIX_FMT_NV21))){
        ret = false;
    }else{
        ret = true;
    }

    //check the size
    if(data->input.active_pixel!=data->output.output_pixel){
        ret |= true;
    }else if(data->input.active_line<data->output.output_line){
        ret |= true;
    }

    if(data->mirror)
        ret |= true;

    if(data->output.zoom>100)
        ret |= true;

    if(data->output.angle)
        ret |= true;

    if((data->input.active_pixel&0x1f)||(data->output.output_pixel&0x1f))
        ret |= true;
    return ret;
}

static int sw_process(vdin_ops_privdata_t* data, vframe_t* in)
{
    canvas_t cs0,cs1;
    void __iomem * buffer_src_y = NULL;
    void __iomem * buffer_src_uv = NULL;
    int ret = -1;
    int i=0, src_width = 0, src_height = 0;
    int poss=0,posd=0,bytesperline = 0;

    if((!data)||(!in)){
        mipi_error("[mipi_vdin]:sw_process-- pointer NULL !!!!\n");
        return ret;
    }

    if(!in->canvas0Addr){
        mipi_error("[mipi_vdin]:sw_process-- invalid canvas !!!!\n");
        return ret;
    }

    src_width = in->width;
    src_height = in->height;
    if((src_width!=data->output.output_pixel)||(src_height<data->output.output_line)){
        mipi_error("[mipi_vdin]:sw_process-- size error !!!!\n");
        return ret;
    }

    canvas_read(in->canvas0Addr&0xff,&cs0);
    buffer_src_y = ioremap_wc(cs0.addr,cs0.width*cs0.height);
    if(!buffer_src_y) {
        if(buffer_src_y)
            iounmap(buffer_src_y);
        mipi_error("[mipi_vdin]:sw_process---mapping buffer error\n");
        return ret;
    }
    if(data->output.fourcc == data->input.fourcc){
        if((data->output.fourcc == V4L2_PIX_FMT_NV12)||(data->output.fourcc == V4L2_PIX_FMT_NV21)) {
            unsigned uv_width = data->output.output_pixel;
            unsigned uv_height = data->output.output_line>>1;
            for(i=data->output.output_line;i>0;i--) { /* copy y */
                swap_vdin_y((unsigned char *)(data->output.vaddr+posd),(unsigned char *)(buffer_src_y+poss),data->output.output_pixel);
                poss += cs0.width;
                posd += data->output.output_pixel;
            }
            canvas_read((in->canvas0Addr>>8)&0xff,&cs1);
            buffer_src_uv = ioremap_wc(cs1.addr,cs1.width*cs1.height);
            poss = 0;
            for(i=uv_height; i > 0; i--){ 
                swap_vdin_uv((unsigned char *)(data->output.vaddr+posd), (unsigned char *)(buffer_src_uv+poss), uv_width);
                poss += cs1.width;
                posd += uv_width;
            }
            iounmap(buffer_src_uv);
            ret = 0;
        }else{
            bytesperline = (data->output.output_pixel*data->output.depth)>>3;
            for(i=data->output.output_line;i>0;i--) { /* copy y */
                memcpy((void *)(data->output.vaddr+posd),(void *)(buffer_src_y+poss),bytesperline);
                poss += cs0.width;
                posd += bytesperline;
            }
            ret = 0;
        }
    }else if((data->input.fourcc == V4L2_PIX_FMT_UYVY)&&((data->output.fourcc == V4L2_PIX_FMT_NV12)||(data->output.fourcc == V4L2_PIX_FMT_NV21))){
        unsigned char* src_line = (unsigned char *)buffer_src_y;
        unsigned char* dst_y = (unsigned char *)data->output.vaddr;
        unsigned char* dst_uv = dst_y+(data->output.output_pixel*data->output.output_line);
        for(i=data->output.output_line;i>0;i-=2) { /* copy y */
            convert422_to_nv21_vdin(src_line, dst_y, dst_uv, data->output.output_pixel);
            src_line += (data->output.output_pixel*4);
            dst_y+= (data->output.output_pixel*2);
            dst_uv+=data->output.output_pixel;
        }
        ret = 0;
    }else{
        //to do
        mipi_error("[mipi_vdin]:sw_process---format is not match.input:0x%x,output:0x%x\n",data->input.fourcc,data->output.fourcc);
    }
    iounmap(buffer_src_y);
    return ret;
}

static struct am_csi2_pixel_fmt* getPixelFormat(u32 fourcc, bool input)
{
    struct am_csi2_pixel_fmt* r = NULL;
    int i = 0;
    if(input){  //mipi input format support
        for( i =0;i<ARRAY_SIZE(am_csi2_input_pix_formats_vdin);i++){
            if(am_csi2_input_pix_formats_vdin[i].fourcc == fourcc){
                r = (struct am_csi2_pixel_fmt*)&am_csi2_input_pix_formats_vdin[i];
                break;
            }
        }
    }else{     //mipi output format support, can be converted by ge2d
        for( i =0;i<ARRAY_SIZE(am_csi2_output_pix_formats_vdin);i++){
            if(am_csi2_output_pix_formats_vdin[i].fourcc == fourcc){
                r = (struct am_csi2_pixel_fmt*)&am_csi2_output_pix_formats_vdin[i];
                break;
            }
        }
    }
    return r;
}

static bool checkframe(int wr_id,am_csi2_input_t* input)
{
    unsigned data1 = 0;
    unsigned data2 = 0;
    bool ret = false;
    am_csi2_frame_t frame;

    frame.w = READ_CBUS_REG(CSI2_PIC_SIZE_STAT)&0xffff;
    frame.h = (READ_CBUS_REG(CSI2_PIC_SIZE_STAT)&0xffff0000)>>16;
    frame.err = READ_CBUS_REG(CSI2_ERR_STAT0);
    data1 = READ_CBUS_REG(CSI2_DATA_TYPE_IN_MEM);
    data2 = READ_CBUS_REG(CSI2_GEN_STAT0);

    if(frame.err){
        mipi_error("[mipi_vdin]:checkframe error---pixel cnt:%d, line cnt:%d. error state:0x%x. wr_index:%d, mem type:0x%x, status:0x%x\n",
                      frame.w,frame.h,frame.err,wr_id,data1,data2);
    }else{
        mipi_dbg("[mipi_vdin]:checkframe---pixel cnt:%d, line cnt:%d. wr_index:%d, mem type:0x%x, status:0x%x\n",
                      frame.w,frame.h,wr_id,data1,data2);
        ret = true;
    } 
    return ret;
}

static int fill_buff_from_canvas(am_csi2_frame_t* frame, am_csi2_output_t* output)
{

    int ret = -1;
    void __iomem * buffer_y_start = NULL;
    void __iomem * buffer_u_start = NULL;
    void __iomem * buffer_v_start = NULL;
    canvas_t canvas_work_y;
    canvas_t canvas_work_u;
    canvas_t canvas_work_v;
    int i=0,poss=0,posd=0,bytesperline = 0;
    if((!frame)||(!output->vaddr)||(frame->index<0)){
        mipi_error("[mipi_vdin]:fill_buff_from_canvas---pionter error\n");
        return ret;
    }
	
    canvas_read(frame->index&0xff,&canvas_work_y);
    buffer_y_start = ioremap_wc(frame->ddr_address,output->frame_size);
    //buffer_y_start = ioremap_wc(canvas_work_y.addr,canvas_work_y.width*canvas_work_y.height);

    if(buffer_y_start == NULL) {
        mipi_error("[mipi_vdin]:fill_buff_from_canvas---mapping buffer error\n");
        return ret;
    }
    mipi_dbg("[mipi_vdin]:fill_buff_from_canvas:frame->ddr_address:0x%x,canvas y:0x%x--%dx%d\n",(unsigned long)frame->ddr_address,(unsigned long)canvas_work_y.addr,canvas_work_y.width,canvas_work_y.height);
    if((output->fourcc == V4L2_PIX_FMT_BGR24)||
      (output->fourcc == V4L2_PIX_FMT_RGB24)||
      (output->fourcc == V4L2_PIX_FMT_RGB565)) {
        bytesperline = (output->output_pixel*output->depth)>>3;
        for(i=0;i<output->output_line;i++) {
            memcpy((void *)(output->vaddr+posd),(void *)(buffer_y_start+poss),bytesperline);
            poss += canvas_work_y.width;
            posd += bytesperline;
        }
        ret = 0;
    } else if((output->fourcc == V4L2_PIX_FMT_NV12)||(output->fourcc == V4L2_PIX_FMT_NV21)) {
        unsigned uv_width = output->output_pixel;
        unsigned uv_height = output->output_line>>1;
        for(i=output->output_line;i>0;i--) { /* copy y */
            memcpy((void *)(output->vaddr+posd),(void *)(buffer_y_start+poss),output->output_pixel);
            poss += canvas_work_y.width;
            posd += output->output_pixel;
        }
        canvas_read((frame->index>>8)&0xff,&canvas_work_u);
        buffer_u_start = ioremap_wc(canvas_work_u.addr,canvas_work_u.width*canvas_work_u.height);
        mipi_dbg("[mipi_vdin]:fill_buff_from_canvas:canvas u:0x%x--%dx%d\n",(unsigned long)canvas_work_u.addr,canvas_work_u.width,canvas_work_u.height);
        poss = 0;
        for(i=uv_height; i > 0; i--){
            memcpy((void *)(output->vaddr+posd), (void *)(buffer_u_start+poss), uv_width);
            poss += canvas_work_u.width;
            posd += uv_width;
        }
        iounmap(buffer_u_start);
        ret = 0;
    } else if (output->fourcc == V4L2_PIX_FMT_YUV420) {
        int uv_width = output->output_pixel>>1;
        int uv_height = output->output_line>>1;
        for(i=output->output_line;i>0;i--) { /* copy y */
            memcpy((void *)(output->vaddr+posd),(void *)(buffer_y_start+poss),output->output_pixel);
            poss += canvas_work_y.width;
            posd += output->output_pixel;
        }
        canvas_read((frame->index>>8)&0xff,&canvas_work_u);
        buffer_u_start = ioremap_wc(canvas_work_u.addr,canvas_work_u.width*canvas_work_u.height);
        poss = 0;
        for(i=uv_height; i > 0; i--){
            memcpy((void *)(output->vaddr+posd), (void *)(buffer_u_start+poss), uv_width);
            poss += canvas_work_u.width;
            posd += uv_width;
        }
        canvas_read((frame->index>>16)&0xff,&canvas_work_v);
        poss = 0;
        buffer_u_start = ioremap_wc(canvas_work_v.addr,canvas_work_v.width*canvas_work_v.height);
        for(i=uv_height; i > 0; i--){
            memcpy((void *)(output->vaddr+posd), (void *)(buffer_v_start+poss), uv_width);
            poss += canvas_work_v.width;
            posd += uv_width;
        }
        iounmap(buffer_u_start);
        iounmap(buffer_v_start);
        ret = 0;
    }
    iounmap(buffer_y_start);
    return ret;
}

static int am_csi2_vdin_init(am_csi2_t* dev)
{
    int ret = -1;
    vdin_ops_privdata_t* data = (vdin_ops_privdata_t*)dev->ops->privdata;

    if(dev->id>=dev->ops->data_num){
        mipi_error("[mipi_vdin]:am_csi2_vdin_init ---- id error.\n");
        goto err_exit;
    }

    data = &data[dev->id];
    data->dev_id = dev->id;

    data->context = create_ge2d_work_queue();
    if(!data->context){
        ret =  -ENOENT;
        mipi_error("[mipi_vdin]:am_csi2_vdin_init ---- ge2d context register error.\n");
        goto err_exit;
    }

    data->hw_info.lanes = dev->client->lanes;
    data->hw_info.channel = dev->client->channel;
    data->hw_info.ui_val = dev->ui_val;
    data->hw_info.hs_freq = dev->hs_freq;
    data->hw_info.clock_lane_mode = dev->clock_lane_mode;
    data->hw_info.mode = AM_CSI2_VDIN;

    mutex_init(&data->buf_lock);
    init_waitqueue_head (&data->complete);
    data->out_buff.q_lock= __SPIN_LOCK_UNLOCKED(data->out_buff.q_lock);
    data->run_flag = false;
    tvin_dec_notifier_register(&mipi_notifier_cb);

    vf_receiver_init(&mipi_vf_recv[data->dev_id], RECEIVER_NAME, &mipi_vf_receiver, (void*)data);
    vf_reg_receiver(&mipi_vf_recv[data->dev_id]);

    init_am_mipi_csi2_clock();
    ret = 0;
    mipi_dbg("[mipi_vdin]:am_csi2_vdin_init ok.\n");
err_exit:
    return ret;
}

static int am_csi2_vdin_streamon(am_csi2_t* dev)
{
    vdin_ops_privdata_t* data = (vdin_ops_privdata_t*)dev->ops->privdata;
    tvin_parm_t para;
    int i = 0;
    data = &data[dev->id];
    
    data->hw_info.active_line = dev->input.active_line;
    data->hw_info.active_pixel= dev->input.active_pixel;
    data->hw_info.frame_size = dev->input.frame_size;
    data->hw_info.urgent = 0;

    memcpy(&data->input,&dev->input,sizeof(am_csi2_input_t));
    memcpy(&data->output,&dev->output,sizeof(am_csi2_output_t));
    data->output.vaddr = NULL;

    bufq_init(&data->out_buff,&(data->output.frame[0]),data->output.frame_available);
    for(i = 0;i<data->output.frame_available;i++){
        data->output.frame[i].index = config_canvas_index(data->output.frame[i].ddr_address,data->output.fourcc,data->output.output_pixel,data->output.output_line,i);
        if(data->output.frame[i].index<0){
            mipi_error("[mipi_vdin]:am_csi2_vdin_streamon ---- canvas config error. \n");
            return -1;
        }
    }

    data->mirror = dev->mirror;

    if(dev->client->vdin_num>=0)
        data->vdin_num = dev->client->vdin_num;
    else
        data->vdin_num = 0;
    if(data->input.fourcc == V4L2_PIX_FMT_NV12)
        para.port  = TVIN_PORT_MIPI_NV12;
    else if(data->input.fourcc == V4L2_PIX_FMT_NV21)
        para.port  = TVIN_PORT_MIPI_NV21;
    else
        para.port  = TVIN_PORT_MIPI;
    para.fmt_info.fmt = TVIN_SIG_FMT_MAX+1;//TVIN_SIG_FMT_MAX+1;;TVIN_SIG_FMT_CAMERA_1280X720P_30Hz
    para.fmt_info.hsync_phase = 0;
    para.fmt_info.vsync_phase = 0;
    para.fmt_info.frame_rate = dev->frame_rate;
    para.fmt_info.h_active = data->input.active_pixel;
    para.fmt_info.v_active = data->input.active_line;
    para.data = (void *)data;
    start_tvin_service(data->vdin_num,&para);
    msleep(100);
    mipi_dbg("[mipi_vdin]:am_csi2_vdin_streamon ok.\n");
    return 0;
}

static int am_csi2_vdin_streamoff(am_csi2_t* dev)
{
    vdin_ops_privdata_t* data = (vdin_ops_privdata_t*)dev->ops->privdata;

    data = &data[dev->id];
    mutex_lock(&data->buf_lock);
    //data->reset_flag = 1;
    //wake_up_interruptible(&data->complete);

    stop_tvin_service(data->vdin_num);
    mutex_unlock(&data->buf_lock);
    mipi_dbg("[mipi_vdin]:am_csi2_vdin_streamoff ok.\n");
    return 0;
}

static int am_csi2_vdin_fillbuff(am_csi2_t* dev)
{
    vdin_ops_privdata_t* data = (vdin_ops_privdata_t*)dev->ops->privdata;
    int ret = 0;
    vframe_t* frame = NULL;
    unsigned long timeout = msecs_to_jiffies(500) + 1;
    data = &data[dev->id];

    mutex_lock(&data->buf_lock);

    data->output.vaddr = dev->output.vaddr;
    data->output.zoom = dev->output.zoom;
    data->output.angle= dev->output.angle;

    mipi_dbg("[mipi_vdin]:am_csi2_vdin_fillbuff. address:0x%x. size:%dx%d, depth:%d,zoom level:%d, angle:%d.\n",
		(u32)data->output.vaddr,data->output.output_pixel,data->output.output_line,data->output.depth,data->output.zoom,data->output.angle);

    if(mipi_vf_peek()==NULL){
        data->done_flag = false;
        wait_event_interruptible_timeout(data->complete,(data->reset_flag)||(data->done_flag==true),timeout);
    }
    if(!data->reset_flag){
        am_csi2_frame_t* temp_frame = NULL;
        frame= mipi_vf_get();
        if(frame){
            if(is_need_ge2d_process(data))
                temp_frame = bufq_pop_free(&data->out_buff);
            if(temp_frame){
                mipi_dbg("[mipi_vdin]:am_csi2_vdin_fillbuff ---need ge2d to pre process\n");
                memset(&(data->ge2d_config),0,sizeof(config_para_ex_t));
                ret = ge2d_process(data,frame,temp_frame,data->context,&(data->ge2d_config));
            }
            if(temp_frame){
                ret = fill_buff_from_canvas(temp_frame,&(data->output));
                bufq_push_free(&data->out_buff, temp_frame);
            }else{
                ret = sw_process(data,frame);
            }
            mipi_vf_put(frame);
        }
    }
    mutex_unlock(&data->buf_lock);
    return ret;
}

static int am_csi2_vdin_uninit(am_csi2_t* dev)
{
    vdin_ops_privdata_t* data = (vdin_ops_privdata_t*)dev->ops->privdata;

    data = &data[dev->id];
    data->reset_flag = 2;
    wake_up_interruptible(&data->complete);
    if(data->context)
        destroy_ge2d_work_queue(data->context);
    mutex_destroy(&data->buf_lock);

    vf_unreg_receiver(&mipi_vf_recv[data->dev_id]);

    tvin_dec_notifier_unregister(&mipi_notifier_cb);
    am_mipi_csi2_uninit();
    data->vdin_num = -1;
    mipi_dbg("[mipi_vdin]:am_csi2_vdin_uninit ok.\n");
    return 0;
}

