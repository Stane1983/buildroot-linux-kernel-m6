/*******************************************************************
 *
 *  Copyright C 2010 by Amlogic, Inc. All Rights Reserved.
 *
 *  Description:
 *
 *  Author: Amlogic Software
 *  Created: 2010/4/1   19:46
 *
 *******************************************************************/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/time.h>
#include <linux/vout/vinfo.h>
#include <linux/vout/vout_notify.h>
#include <linux/platform_device.h>
#include <linux/amports/ptsserv.h>
#include <linux/amports/canvas.h>
#include <linux/amports/vframe.h>
#include <linux/amports/vframe_provider.h>
#include <linux/amports/vframe_receiver.h>
#include <mach/am_regs.h>
#include <linux/amlog.h>
#include <linux/ge2d/ge2d_main.h>
#include <linux/ge2d/ge2d.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/semaphore.h>
#include <linux/sched.h>
#include <linux/platform_device.h>
#include <linux/ge2d/ge2d_main.h>
#include <linux/ge2d/ge2d.h>
#include "vm_log.h"
#include "vm.h"
#include <linux/amlog.h>
#include <linux/ctype.h>
#include <linux/videodev2.h>
#include <media/videobuf-core.h>
#include <media/videobuf-dma-contig.h>
#include <media/videobuf-vmalloc.h>
#include <media/videobuf-dma-sg.h>
#include <media/videobuf-res.h>
#include <media/amlogic/656in.h>
#include <linux/ctype.h>

/*class property info.*/
#include "vmcls.h"
#include "vmapi.h"

static int task_running = 0;

#ifdef CONFIG_ARCH_MESON6
#define GE2D_NV
#endif

#if 0
static unsigned amlvm_time_log_enable = 0;
module_param(amlvm_time_log_enable, uint, 0644);
MODULE_PARM_DESC(amlvm_time_log_enable, "enable vm time log when get frames");
#endif

#define MAGIC_SG_MEM 0x17890714
#define MAGIC_DC_MEM 0x0733ac61
#define MAGIC_VMAL_MEM 0x18221223
#define MAGIC_RE_MEM 0x123039dc

#define MAX_VF_POOL_SIZE 8

#ifndef CONFIG_AMLOGIC_VM_DISABLE_VIDEOLAYER
/*same as tvin pool*/
static int VM_POOL_SIZE = 6 ;
static int VF_POOL_SIZE = 6;
static int VM_CANVAS_INDEX = 24;
/*same as tvin pool*/
#endif

static int vm_skip_count = 0 ; //skip 5 frames from vdin
static int test_zoom = 0;

static inline void vm_vf_put_from_provider(vframe_t *vf);
#ifndef CONFIG_AMLOGIC_VM_DISABLE_VIDEOLAYER
#define INCPTR(p) ptr_atomic_wrap_inc(&p)
#endif

#define VM_DEPTH_16_CANVAS 0x50         //for single canvas use ,RGB16, YUV422,etc

#define VM_DEPTH_24_CANVAS 0x52

#define VM_DEPTH_8_CANVAS_Y  0x54     // for Y/CbCr 4:2:0
#define VM_DEPTH_8_CANVAS_UV 0x55
#define VM_DEPTH_8_CANVAS_U 0x57
#define VM_DEPTH_8_CANVAS_V 0x58

#define VM_RES_CANVAS_INDEX 0x59
#define VM_RES_CANVAS_INDEX_U 0x5a
#define VM_RES_CANVAS_INDEX_V 0x5b
#define VM_RES_CANVAS_INDEX_UV 0x5c

#define VM_DMA_CANVAS_INDEX 0x5e

#define VM_CANVAS_MX 0x5f

#ifdef CONFIG_AMLOGIC_CAPTURE_FRAME_ROTATE
static int vmdecbuf_size[] ={
			0x1338c00,//5M
			0xc00000,//3M
			0x753000,//2M
			0x4b0000,//1M3
			0x300000,//1M
			0x12c000,//VGA
			0x4b000,//QVGA
			};
static struct v4l2_frmsize_discrete canvas_config_wh[]={
					{2592,2592},
					{2048,2048},
					{1600,1600},
					{1280,1280},
					{1024,1024},
					{640,640},
					{320,320},
				    };
#else
static int vmdecbuf_size[] ={
			0xE79C00,//5M
			0x900000,//3M
			0x591000,//2M
			0x384000,//1M3
			0x240000,//1M
			0xE1000,//VGA
			0x3C000,//QVGA
			};
static struct v4l2_frmsize_discrete canvas_config_wh[]={
					{2592,1952},
					{2048,1536},
					{1600,1216},
					{1280,960},
					{1024,768},
					{640,480},
					{320,256},
				    };
#endif
#define GE2D_ENDIAN_SHIFT        24
#define GE2D_ENDIAN_MASK            (0x1 << GE2D_ENDIAN_SHIFT)
#define GE2D_BIG_ENDIAN             (0 << GE2D_ENDIAN_SHIFT)
#define GE2D_LITTLE_ENDIAN          (1 << GE2D_ENDIAN_SHIFT)

#define PROVIDER_NAME "vm"
#define RECEIVER_NAME "vm"
static DEFINE_SPINLOCK(lock);

#ifndef CONFIG_AMLOGIC_VM_DISABLE_VIDEOLAYER
static inline void ptr_atomic_wrap_inc(u32 *ptr)
{
	u32 i = *ptr;
	i++;
	if (i >= VM_POOL_SIZE)
		i = 0;
	*ptr = i;
}
#endif

int start_vm_task(void) ;
int start_simulate_task(void);

#ifndef CONFIG_AMLOGIC_VM_DISABLE_VIDEOLAYER
static struct vframe_s vfpool[MAX_VF_POOL_SIZE];
/*static u32 vfpool_idx[MAX_VF_POOL_SIZE];*/
static s32 vfbuf_use[MAX_VF_POOL_SIZE];
static s32 fill_ptr, get_ptr, putting_ptr, put_ptr;
#endif
struct semaphore  vb_start_sema;
struct semaphore  vb_done_sema;

static inline vframe_t *vm_vf_get_from_provider(void);
static inline vframe_t *vm_vf_peek_from_provider(void);
static inline void vm_vf_put_from_provider(vframe_t *vf);
static vframe_receiver_op_t* vf_vm_unreg_provider(void);
static vframe_receiver_op_t* vf_vm_reg_provider(void);
static void stop_vm_task(void) ;
#ifndef CONFIG_AMLOGIC_VM_DISABLE_VIDEOLAYER
static int prepare_vframe(vframe_t *vf);
#endif

/************************************************
*
*   buffer op for video sink.
*
*************************************************/
#ifdef CONFIG_AMLOGIC_VM_DISABLE_VIDEOLAYER
static vframe_t *local_vf_peek(void)
{
	vframe_t *vf = NULL;
	vf = vm_vf_peek_from_provider();
	if(vf){
		if(vm_skip_count > 0){
			vm_skip_count--;	
			vm_vf_get_from_provider();	
			vm_vf_put_from_provider(vf); 
			vf = NULL;						
		}
	}
	return vf;
}

static vframe_t *local_vf_get(void)
{
	return vm_vf_get_from_provider();
}

static void local_vf_put(vframe_t *vf)
{
	if(vf)
		vm_vf_put_from_provider(vf);
	return;
}
#else

static inline u32 index2canvas(u32 index)
{
	int i;
	int start_canvas, count;
	u32 canvas_tab[6] ;
	get_tvin_canvas_info(&start_canvas,&count);
	VM_POOL_SIZE  = count ;
	VF_POOL_SIZE  = count ;
	VM_CANVAS_INDEX = start_canvas;
	for(i =0; i< count; i++)
		canvas_tab[i] =  VM_CANVAS_INDEX +i;
	return canvas_tab[index];
}

static struct vframe_s* vm_vf_peek(void *op_arg)
{
	vframe_t *vf = NULL;
	vf = vm_vf_peek_from_provider();
	if(vf){
		if(vm_skip_count > 0){
			vm_skip_count--;
			vm_vf_get_from_provider();
			vm_vf_put_from_provider(vf);
			vf = NULL;
		}
	}
    return vf;
}

static struct vframe_s* vm_vf_get(void *op_arg)
{
	return vm_vf_get_from_provider();
}

static void vm_vf_put(vframe_t *vf, void* op_arg)
{
	prepare_vframe(vf);
}

static int vm_vf_states(vframe_states_t *states, void* op_arg)
{
	return 0;
}

static vframe_t *local_vf_peek(void)
{
	if (get_ptr == fill_ptr)
		return NULL;
	return &vfpool[get_ptr];
}

static vframe_t *local_vf_get(void)
{
	vframe_t *vf;

	if (get_ptr == fill_ptr)
		return NULL;
	vf = &vfpool[get_ptr];
	INCPTR(get_ptr);
	return vf;
}

static void local_vf_put(vframe_t *vf)
{
	int i;
	int  canvas_addr;
	if(!vf)
		return;
	INCPTR(putting_ptr);
	for (i = 0; i < VF_POOL_SIZE; i++) {
		canvas_addr = index2canvas(i);
		if(vf->canvas0Addr == canvas_addr ){
			vfbuf_use[i] = 0;
			vm_vf_put_from_provider(vf);
		}
	}
}
#endif

/*static int  local_vf_states(vframe_states_t *states)
{
    unsigned long flags;
    int i;
    spin_lock_irqsave(&lock, flags);
    states->vf_pool_size = VF_POOL_SIZE;

    i = put_ptr - fill_ptr;
    if (i < 0) i += VF_POOL_SIZE;
    states->buf_free_num = i;

    i = putting_ptr - put_ptr;
    if (i < 0) i += VF_POOL_SIZE;
    states->buf_recycle_num = i;

    i = fill_ptr - get_ptr;
    if (i < 0) i += VF_POOL_SIZE;
    states->buf_avail_num = i;

    spin_unlock_irqrestore(&lock, flags);
    return 0;
}*/

static int vm_receiver_event_fun(int type, void *data, void *private_data)
{
	switch(type){
	case VFRAME_EVENT_PROVIDER_VFRAME_READY:
		//up(&vb_start_sema);
		//printk("vdin frame ready !!!!!\n");
		break;
	case VFRAME_EVENT_PROVIDER_START:
		//printk("vm register!!!!!\n");
		vf_vm_reg_provider();
		vm_skip_count = 2; 
		test_zoom = 0;
		break;
	case VFRAME_EVENT_PROVIDER_UNREG:
		//printk("vm unregister!!!!!\n");
		vm_local_init();
		vf_vm_unreg_provider();
		//printk("vm unregister succeed!!!!!");
		break;
	default:
		break;
	}
	return 0;
}

static vframe_receiver_op_t vm_vf_receiver =
{
    .event_cb = vm_receiver_event_fun
};

#ifndef CONFIG_AMLOGIC_VM_DISABLE_VIDEOLAYER
static const struct vframe_operations_s vm_vf_provider =
{
	.peek = vm_vf_peek,
	.get  = vm_vf_get,
	.put  = vm_vf_put,
	.vf_states = vm_vf_states,
};

static struct vframe_provider_s vm_vf_prov;
#endif
static struct vframe_receiver_s vm_vf_recv;

#ifndef CONFIG_AMLOGIC_VM_DISABLE_VIDEOLAYER
int get_unused_vm_index(void)
{
	int i;
	for (i = 0; i < VF_POOL_SIZE; i++){
		if(vfbuf_use[i] == 0)
			return i;
	}
	return -1;
}
static int prepare_vframe(vframe_t *vf)
{
	vframe_t* new_vf;
	int index;

	index = get_unused_vm_index();
	if(index < 0)
		return -1;
	new_vf = &vfpool[fill_ptr];
	memcpy(new_vf, vf, sizeof(vframe_t));
	vfbuf_use[index]++;
	INCPTR(fill_ptr);
	return 0;
}
#endif

/*************************************************
*
*   buffer op for decoder, camera, etc.
*
*************************************************/
/* static const vframe_provider_t *vfp = NULL; */

void vm_local_init(void)
{
#ifndef CONFIG_AMLOGIC_VM_DISABLE_VIDEOLAYER
    int i;
    for(i=0;i<MAX_VF_POOL_SIZE;i++)
    {
        vfbuf_use[i] = 0;
    }
    fill_ptr=get_ptr=putting_ptr=put_ptr=0;
#endif
    return;
}

static vframe_receiver_op_t* vf_vm_unreg_provider(void)
{
//    ulong flags;    
#ifndef CONFIG_AMLOGIC_VM_DISABLE_VIDEOLAYER
	vf_unreg_provider(&vm_vf_prov);
#endif
	stop_vm_task();
//    spin_lock_irqsave(&lock, flags); 
//    vfp = NULL;
//    spin_unlock_irqrestore(&lock, flags);
    return (vframe_receiver_op_t*)NULL;
}
EXPORT_SYMBOL(vf_vm_unreg_provider);

static vframe_receiver_op_t* vf_vm_reg_provider( )
{
    ulong flags;

    spin_lock_irqsave(&lock, flags);
    spin_unlock_irqrestore(&lock, flags);
    vm_buffer_init();
#ifndef CONFIG_AMLOGIC_VM_DISABLE_VIDEOLAYER
    vf_reg_provider(&vm_vf_prov);
#endif
    start_vm_task();   
#if 0   
    start_simulate_task();
#endif
    return &vm_vf_receiver;
}
EXPORT_SYMBOL(vf_vm_reg_provider);

/*static const struct vframe_provider_s * vm_vf_get_vfp_from_provider(void)
{
	return vfp;
} */

static inline vframe_t *vm_vf_peek_from_provider(void)
{
    struct vframe_provider_s *vfp;
    vframe_t *vf;

    vfp = vf_get_provider(RECEIVER_NAME);
    if (!(vfp && vfp->ops && vfp->ops->peek))
        return NULL;
    vf  = vfp->ops->peek(vfp->op_arg);
    return vf;
}

static inline vframe_t *vm_vf_get_from_provider(void)
{
    struct vframe_provider_s *vfp;

    vfp = vf_get_provider(RECEIVER_NAME);
    if (!(vfp && vfp->ops && vfp->ops->peek))
        return NULL;
    return vfp->ops->get(vfp->op_arg);
}

static inline void vm_vf_put_from_provider(vframe_t *vf)
{
	struct vframe_provider_s *vfp;
	vfp = vf_get_provider(RECEIVER_NAME);
	if (!(vfp && vfp->ops && vfp->ops->peek))
		return;
	vfp->ops->put(vf,vfp->op_arg);
}

/************************************************
*
*   main task functions.
*
*************************************************/
static int get_input_format(vframe_t* vf)
{
	int format= GE2D_FORMAT_M24_YUV420;

	if(vf->type&VIDTYPE_VIU_422)
		format =  GE2D_FORMAT_S16_YUV422;
	else
		format =  GE2D_FORMAT_M24_YUV420;

	return format;
}

#ifdef CONFIG_AMLOGIC_VM_DISABLE_VIDEOLAYER
static int calc_zoom(int* top ,int* left , int* bottom, int* right, int zoom)
{
    u32 screen_width, screen_height;
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
#endif

static int  get_input_frame(display_frame_t* frame ,vframe_t* vf,int zoom)
{
	int ret = 0;
	int top, left,  bottom ,right;
	if (!vf)
		return -1;

	frame->frame_top = 0;
	frame->frame_left = 0 ;
	frame->frame_width = vf->width;
	frame->frame_height = vf->height;
	top = 0;
	left = 0;
	bottom = vf->height -1;
	right = vf->width - 1;
#ifdef CONFIG_AMLOGIC_VM_DISABLE_VIDEOLAYER
	ret = calc_zoom(&top ,&left , &bottom, &right,zoom);
#else
	ret = get_curren_frame_para(&top ,&left , &bottom, &right);
#endif
	if(ret >= 0 ){
  		frame->content_top     =  top&(~1);
		frame->content_left    =  left&(~1);
		frame->content_width   =  vf->width - 2*frame->content_left ;
		frame->content_height  =  vf->height - 2*frame->content_top;
	}else{
		frame->content_top     = 0;
		frame->content_left    =  0 ;
		frame->content_width   = vf->width;
		frame->content_height  = vf->height;
	}
	return 0;
}

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
#ifdef GE2D_NV
		format = GE2D_FORMAT_M24_NV12;
		break;
#endif
	case V4L2_PIX_FMT_NV21:
#ifdef GE2D_NV
		format = GE2D_FORMAT_M24_NV21;
		break;
#endif
	case V4L2_PIX_FMT_YUV420:
	case V4L2_PIX_FMT_YVU420:
		format = GE2D_FORMAT_S8_Y;
		break;
	default:
	break;
	}
	return format;
}

static vm_output_para_t output_para = {0,0,0,0,0,0,-1,-1,0,0};

typedef struct vm_dma_contig_memory {
	u32 magic;
	void *vaddr;
	dma_addr_t dma_handle;
	unsigned long size;
	int is_userptr;
}vm_contig_memory_t;

int is_need_ge2d_pre_process(void)
{
	int ret = 0;
	switch(output_para.v4l2_format) {
	case  V4L2_PIX_FMT_RGB565X:
	case  V4L2_PIX_FMT_YUV444:
	case  V4L2_PIX_FMT_VYUY:
	case  V4L2_PIX_FMT_BGR24:
	case  V4L2_PIX_FMT_RGB24:
	case  V4L2_PIX_FMT_YUV420:
	case  V4L2_PIX_FMT_YVU420:
	case  V4L2_PIX_FMT_NV12:
	case  V4L2_PIX_FMT_NV21:
		ret = 1;
		break;
	default:
		break;
	}
	return ret;
}

int is_need_sw_post_process(void)
{
	int ret = 0;
	switch(output_para.v4l2_memory){
	case MAGIC_DC_MEM:
	case MAGIC_RE_MEM:
		goto exit;
		break;
	case MAGIC_SG_MEM:
	case MAGIC_VMAL_MEM:
	default:
		ret = 1;
		break;
	}
exit:
    return ret;
}

int get_canvas_index(int v4l2_format, int *depth)
{
	int canvas = VM_DEPTH_16_CANVAS;
	*depth = 16;
	switch(v4l2_format){
	case V4L2_PIX_FMT_RGB565X:
	case V4L2_PIX_FMT_VYUY:
		canvas = VM_DEPTH_16_CANVAS;
		*depth = 16 ;
		break;
	case V4L2_PIX_FMT_YUV444:
	case V4L2_PIX_FMT_BGR24:
	case V4L2_PIX_FMT_RGB24:
		canvas = VM_DEPTH_24_CANVAS;
		*depth = 24;
		break;
	case V4L2_PIX_FMT_NV12:
	case V4L2_PIX_FMT_NV21:
#ifdef GE2D_NV
		canvas = VM_DEPTH_8_CANVAS_Y | (VM_DEPTH_8_CANVAS_UV<<8);
#else
		canvas = VM_DEPTH_8_CANVAS_Y|(VM_DEPTH_8_CANVAS_U<<8)|(VM_DEPTH_8_CANVAS_V<<16);
#endif
		*depth = 12;
		break;
	case V4L2_PIX_FMT_YUV420:
	case V4L2_PIX_FMT_YVU420:
		canvas = VM_DEPTH_8_CANVAS_Y|(VM_DEPTH_8_CANVAS_U<<8)|(VM_DEPTH_8_CANVAS_V<<16);
		*depth = 12;
		break;
	default:
		break;
	}
	return canvas;
}

int get_canvas_index_res(int v4l2_format, int *depth, int width, int height, unsigned buf)
{
	int canvas = VM_RES_CANVAS_INDEX;
	*depth = 16;
	switch(v4l2_format){
	case V4L2_PIX_FMT_RGB565X:
	case V4L2_PIX_FMT_VYUY:
		canvas = VM_RES_CANVAS_INDEX;
		*depth = 16 ;
		canvas_config(canvas,
			(unsigned long)buf,
			width*2, height,
			CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);
		break;
	case V4L2_PIX_FMT_YUV444:
	case V4L2_PIX_FMT_BGR24:
	case V4L2_PIX_FMT_RGB24:
		canvas = VM_RES_CANVAS_INDEX;
		*depth = 24;
		canvas_config(canvas,
			(unsigned long)buf,
			width*3, height,
			CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);
		break; 
	case V4L2_PIX_FMT_NV12:
	case V4L2_PIX_FMT_NV21: 
		canvas_config(VM_RES_CANVAS_INDEX,
			(unsigned long)buf,
			width, height,
			CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);
		canvas_config(VM_RES_CANVAS_INDEX_UV,
			(unsigned long)(buf+width*height),
			width, height/2,
			CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);
		canvas = VM_RES_CANVAS_INDEX | (VM_RES_CANVAS_INDEX_UV<<8);
		*depth = 12;   
		break;
	case V4L2_PIX_FMT_YVU420:
	case V4L2_PIX_FMT_YUV420:
		canvas_config(VM_RES_CANVAS_INDEX,
			(unsigned long)buf,
			width, height,
			CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);
		canvas_config(VM_RES_CANVAS_INDEX_U,
			(unsigned long)(buf+width*height),
			width/2, height/2,
			CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);
		canvas_config(VM_RES_CANVAS_INDEX_V,
			(unsigned long)(buf+width*height*5/4),
			width/2, height/2,
			CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);
		canvas = VM_RES_CANVAS_INDEX|(VM_RES_CANVAS_INDEX_U<<8)|(VM_RES_CANVAS_INDEX_V<<16);
		*depth = 12;
		break;
	default:
	break;
	}
	return canvas;
}

int vm_fill_buffer(struct videobuf_buffer* vb , vm_output_para_t* para)
{
	//vm_contig_memory_t *mem = NULL;
	resource_size_t buf_start;
	int buf_size;
	int depth=0;
	int ret = -1;
	int canvas_index = -1 ;
	int v4l2_format = V4L2_PIX_FMT_YUV444;
	int magic = 0;
	struct videobuf_buffer buf={0};
	get_vm_buf_info( &buf_start,&buf_size, NULL);
	if(!para)
		return -1;
#if 0    
	if(!vb)
		goto exit;
#else
	if(!vb) {
		buf.width = 640;
		buf.height = 480;
		magic = MAGIC_VMAL_MEM ;
		v4l2_format =  V4L2_PIX_FMT_YUV444 ;
		vb = &buf;
	}
	if(!task_running){
		return ret;
	}
#endif
	v4l2_format = para->v4l2_format;
	magic = para->v4l2_memory;
	switch(magic){
	case   MAGIC_DC_MEM:
		//mem = vb->priv;
		canvas_config(VM_DMA_CANVAS_INDEX,
			  (dma_addr_t)para->vaddr,
			  vb->bytesperline, vb->height,
			  CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);        
		canvas_index =  VM_DMA_CANVAS_INDEX ;
		depth =  (vb->bytesperline <<3)/vb->width;
		break;
	case  MAGIC_RE_MEM:
		canvas_index =  get_canvas_index_res(v4l2_format,&depth,vb->width,vb->height,(unsigned)para->vaddr);
		break;
	case  MAGIC_SG_MEM:
	case  MAGIC_VMAL_MEM:
		if(buf_start && buf_size)
			canvas_index = get_canvas_index(v4l2_format,&depth);
		break;
	default:
		canvas_index = VM_DEPTH_16_CANVAS ;
		break;
	}
	output_para.width = vb->width;
	output_para.height = vb->height;
	output_para.bytesperline  = (vb->width *depth)>>3;
	output_para.index = canvas_index ;
	output_para.v4l2_format  = v4l2_format ;
	output_para.v4l2_memory   = magic ;
	output_para.mirror = para->mirror;
	output_para.zoom= para->zoom;
	output_para.angle= para->angle;
	output_para.vaddr = para->vaddr;
	up(&vb_start_sema);
	ret = down_interruptible(&vb_done_sema);
	return ret;
}

/*for decoder input processing
    1. output window should 1:1 as source frame size
    2. keep the frame ratio
    3. input format should be YUV420 , output format should be YUV444
*/
int vm_ge2d_pre_process(vframe_t* vf, ge2d_context_t *context,config_para_ex_t* ge2d_config)
{
	int ret;
	int src_top ,src_left ,src_width, src_height;
	canvas_t cs0,cs1,cs2,cd;
	int current_mirror = 0;
	int cur_angle = 0;
	display_frame_t input_frame={0};
	ret = get_input_frame(&input_frame , vf,output_para.zoom);
	src_top = input_frame.content_top;
	src_left = input_frame.content_left;
	src_width = input_frame.content_width;
	src_height = input_frame.content_height;
	if(test_zoom){
		test_zoom = 0;
		printk("top is %d , left is %d , width is %d , height is %d\n",input_frame.content_top ,input_frame.content_left,input_frame.content_width,input_frame.content_height);
	}
#ifdef CONFIG_AMLOGIC_VM_DISABLE_VIDEOLAYER
	current_mirror = output_para.mirror;
	if(current_mirror < 0)
		current_mirror = 0;
#else
	current_mirror = camera_mirror_flag;
#endif

#ifdef CONFIG_AMLOGIC_CAPTURE_FRAME_ROTATE
	cur_angle = output_para.angle;
	if(current_mirror == 1)
		cur_angle = (360 - cur_angle%360);
	else
		cur_angle = cur_angle%360;
#endif
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
	canvas_read(output_para.index&0xff,&cd);
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
	ge2d_config->dst_para.canvas_index = output_para.index&0xff;
#ifdef GE2D_NV
	if((output_para.v4l2_format != V4L2_PIX_FMT_YUV420)
		&& (output_para.v4l2_format != V4L2_PIX_FMT_YVU420))
		ge2d_config->dst_para.canvas_index = output_para.index;
#endif
	ge2d_config->dst_para.mem_type = CANVAS_TYPE_INVALID;
	ge2d_config->dst_para.format = get_output_format(output_para.v4l2_format)|GE2D_LITTLE_ENDIAN;
	ge2d_config->dst_para.fill_color_en = 0;
	ge2d_config->dst_para.fill_mode = 0;
	ge2d_config->dst_para.x_rev = 0;
	ge2d_config->dst_para.y_rev = 0;
	ge2d_config->dst_para.color = 0;
	ge2d_config->dst_para.top = 0;
	ge2d_config->dst_para.left = 0;
	ge2d_config->dst_para.width = output_para.width;
	ge2d_config->dst_para.height = output_para.height;

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
	stretchblt_noalpha(context,src_left ,src_top ,src_width, src_height,0,0,output_para.width,output_para.height);

	/* for cr of  yuv420p or yuv420sp. */
	if((output_para.v4l2_format==V4L2_PIX_FMT_YUV420)
		||(output_para.v4l2_format==V4L2_PIX_FMT_YVU420)){
		/* for cb. */
		canvas_read((output_para.index>>8)&0xff,&cd);
		ge2d_config->dst_planes[0].addr = cd.addr;
		ge2d_config->dst_planes[0].w = cd.width;
		ge2d_config->dst_planes[0].h = cd.height;
		ge2d_config->dst_para.canvas_index=(output_para.index>>8)&0xff;
		ge2d_config->dst_para.format=GE2D_FORMAT_S8_CB|GE2D_LITTLE_ENDIAN;
		ge2d_config->dst_para.width = output_para.width/2;
		ge2d_config->dst_para.height = output_para.height/2;
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
#ifndef GE2D_NV
	else if (output_para.v4l2_format==V4L2_PIX_FMT_NV12||
				output_para.v4l2_format==V4L2_PIX_FMT_NV21) {
		canvas_read((output_para.index>>8)&0xff,&cd);
		ge2d_config->dst_planes[0].addr = cd.addr;
		ge2d_config->dst_planes[0].w = cd.width;
		ge2d_config->dst_planes[0].h = cd.height;
		ge2d_config->dst_para.canvas_index=(output_para.index>>8)&0xff;
		ge2d_config->dst_para.format=GE2D_FORMAT_S8_CB|GE2D_LITTLE_ENDIAN;
		ge2d_config->dst_para.width = output_para.width/2;
		ge2d_config->dst_para.height = output_para.height/2;
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
		stretchblt_noalpha(context,src_left ,src_top ,src_width, src_height,0,0,ge2d_config->dst_para.width,ge2d_config->dst_para.height);
	}
#endif

	/* for cb of yuv420p or yuv420sp. */
	if(output_para.v4l2_format==V4L2_PIX_FMT_YUV420||
		output_para.v4l2_format==V4L2_PIX_FMT_YVU420
#ifndef GE2D_NV
		||output_para.v4l2_format==V4L2_PIX_FMT_NV12||
		output_para.v4l2_format==V4L2_PIX_FMT_NV21
#endif
			) {
		canvas_read((output_para.index>>16)&0xff,&cd);
		ge2d_config->dst_planes[0].addr = cd.addr;
		ge2d_config->dst_planes[0].w = cd.width;
		ge2d_config->dst_planes[0].h = cd.height;
		ge2d_config->dst_para.canvas_index=(output_para.index>>16)&0xff;
		ge2d_config->dst_para.format=GE2D_FORMAT_S8_CR|GE2D_LITTLE_ENDIAN;
		ge2d_config->dst_para.width = output_para.width/2;
		ge2d_config->dst_para.height = output_para.height/2;
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
	return output_para.index;
}

int vm_sw_post_process(int canvas , int addr)
{
	int poss=0,posd=0;
	int i=0;
	void __iomem * buffer_y_start;
	void __iomem * buffer_u_start;
	void __iomem * buffer_v_start = 0;
	struct io_mapping *mapping_wc;
	int offset = 0;
	canvas_t canvas_work_y;
	canvas_t canvas_work_u;
	canvas_t canvas_work_v;
    if(!addr){
        return -1;
    }
	get_vm_buf_info( NULL, NULL, &mapping_wc);
	if(!mapping_wc){
		return -1;
	}
	
	canvas_read(canvas&0xff,&canvas_work_y);
	offset = 0;
	buffer_y_start = io_mapping_map_atomic_wc( mapping_wc, offset);
	if(buffer_y_start == NULL) {
		printk(" vm.postprocess:mapping buffer error\n");
		return -1;
	}
	if (output_para.v4l2_format == V4L2_PIX_FMT_BGR24||
				output_para.v4l2_format == V4L2_PIX_FMT_RGB24||
				output_para.v4l2_format== V4L2_PIX_FMT_RGB565X) {
		for(i=0;i<output_para.height;i++) {
			memcpy((void *)(addr+poss),(void *)(buffer_y_start+posd),output_para.bytesperline);
			poss+=output_para.bytesperline;
			posd+= canvas_work_y.width;
		}
		io_mapping_unmap_atomic( buffer_y_start );

	} else if (output_para.v4l2_format== V4L2_PIX_FMT_NV12||
				output_para.v4l2_format== V4L2_PIX_FMT_NV21) {
#ifdef GE2D_NV
		unsigned uv_width = output_para.width;
		unsigned uv_height = output_para.height>>1;
		posd = 0;
		for(i=output_para.height;i>0;i--) { /* copy y */
			memcpy((void *)(addr+poss),(void *)(buffer_y_start+posd),output_para.width);
			poss+=output_para.width;
			posd+= canvas_work_y.width;
		}
		io_mapping_unmap_atomic( buffer_y_start );

		posd=0;
		canvas_read((canvas>>8)&0xff,&canvas_work_u);
		offset = canvas_work_u.addr - canvas_work_y.addr;
		buffer_u_start = io_mapping_map_atomic_wc( mapping_wc, offset);
		for(i=uv_height; i > 0; i--) { /* copy uv */
			memcpy((void *)(addr+poss), (void *)(buffer_u_start+posd), uv_width);
			poss += uv_width;
			posd+= canvas_work_u.width;
		}

		io_mapping_unmap_atomic( buffer_u_start );
#else
		char* dst_buff=NULL;
		char* src_buff=NULL;
		char* src2_buff=NULL;
		canvas_read((canvas>>8)&0xff,&canvas_work_u);
		poss = posd = 0 ;
		for(i=0;i<output_para.height;i+=2) { /* copy y */
			memcpy((void *)(addr+poss),(void *)(buffer_y_start+posd),output_para.width);
			poss+=output_para.width;
			posd+= canvas_work_y.width;
			memcpy((void *)(addr+poss),(void *)(buffer_y_start+posd),output_para.width);
			poss+=output_para.width;
			posd+= canvas_work_y.width;
		}	
		io_mapping_unmap_atomic( buffer_y_start );

		posd=0;
		canvas_read((canvas>>16)&0xff,&canvas_work_v);
		offset = canvas_work_u.addr - canvas_work_y.addr;
		buffer_u_start = io_mapping_map_atomic_wc( mapping_wc, offset);
		offset = canvas_work_v.addr - canvas_work_y.addr;
		buffer_v_start = io_mapping_map_atomic_wc( mapping_wc, offset );

		dst_buff= (char*)addr+output_para.width* output_para.height;
		src_buff = (char*)buffer_u_start;
		src2_buff= (char*)buffer_v_start;
		if(output_para.v4l2_format== V4L2_PIX_FMT_NV12) {
			for(i = 0 ;i < output_para.height/2; i++){
				interleave_uv(src_buff, src2_buff, dst_buff, output_para.width/2);
				src_buff +=  canvas_work_u.width;
				src2_buff += 	canvas_work_v.width;
				dst_buff += output_para.width;
			}
		} else {
			for(i = 0 ;i < output_para.height/2; i++){
				interleave_uv(src2_buff, src_buff, dst_buff, output_para.width/2);
				src_buff +=  canvas_work_u.width;
				src2_buff += 	canvas_work_v.width;
				dst_buff += output_para.width;
			}
		}


		io_mapping_unmap_atomic( buffer_u_start );
		io_mapping_unmap_atomic( buffer_v_start );
#endif
	} else if ( (output_para.v4l2_format == V4L2_PIX_FMT_YUV420)
			||(output_para.v4l2_format == V4L2_PIX_FMT_YVU420)){
		int uv_width = output_para.width>>1;
		int uv_height = output_para.height>>1;

		posd=0;
		for(i=output_para.height;i>0;i--) { /* copy y */
			memcpy((void *)(addr+poss),(void *)(buffer_y_start+posd),output_para.width);
			poss+=output_para.width;
			posd+= canvas_work_y.width;
		}
		io_mapping_unmap_atomic( buffer_y_start );
    	
		posd=0;
		canvas_read((canvas>>8)&0xff,&canvas_work_u);
		offset = canvas_work_u.addr - canvas_work_y.addr;
		buffer_u_start = io_mapping_map_atomic_wc( mapping_wc, offset);

		canvas_read((canvas>>16)&0xff,&canvas_work_v);
		offset = canvas_work_v.addr - canvas_work_y.addr;
		buffer_v_start = io_mapping_map_atomic_wc( mapping_wc, offset );

		if(output_para.v4l2_format == V4L2_PIX_FMT_YUV420){
			for(i=uv_height;i>0;i--) { /* copy y */
				memcpy((void *)(addr+poss),(void *)(buffer_u_start+posd),uv_width);
				poss+=uv_width;
				posd+= canvas_work_u.width;
			}
			posd=0;
			for(i=uv_height;i>0;i--) { /* copy y */
				memcpy((void *)(addr+poss),(void *)(buffer_v_start+posd),uv_width);
				poss+=uv_width;
				posd+= canvas_work_v.width;
			}
		}else{
			for(i=uv_height;i>0;i--) { /* copy v */
				memcpy((void *)(addr+poss),(void *)(buffer_v_start+posd),uv_width);
				poss+=uv_width;
				posd+= canvas_work_v.width;
			}
			posd=0;
			for(i=uv_height;i>0;i--) { /* copy u */
				memcpy((void *)(addr+poss),(void *)(buffer_u_start+posd),uv_width);
				poss+=uv_width;
				posd+= canvas_work_u.width;
			}
		}
		io_mapping_unmap_atomic( buffer_u_start );
		io_mapping_unmap_atomic( buffer_v_start );
	}
	return 0;
}

static struct task_struct *task=NULL;
static struct task_struct *simulate_task_fd=NULL;

/* static int reset_frame = 1; */
static int vm_task(void *data) {
	int ret = 0;
	vframe_t *vf;
	int src_canvas;
	int timer_count = 0 ;
struct sched_param param = {.sched_priority = MAX_RT_PRIO - 1 };
	ge2d_context_t *context=create_ge2d_work_queue();
	config_para_ex_t ge2d_config;

#ifdef CONFIG_AMLCAP_LOG_TIME_USEFORFRAMES
	struct timeval start;
	struct timeval end;
	unsigned long time_use=0;
#endif

	memset(&ge2d_config,0,sizeof(config_para_ex_t));
	amlog_level(LOG_LEVEL_HIGH,"vm task is running\n ");
    sched_setscheduler(current, SCHED_FIFO, &param);
    allow_signal(SIGTERM);
	while(1) {
		ret = down_interruptible(&vb_start_sema);
		timer_count = 0;
        if (kthread_should_stop()){
            up(&vb_done_sema);
            break;
        }

		/* wait for frame from 656 provider until 500ms runs out */
		vf = local_vf_peek();
		while((vf == NULL) && (timer_count < 200)) {
			if(!task_running){
	            up(&vb_done_sema);
	            goto vm_exit;
	            break;
			}
			vf = local_vf_peek();
			timer_count++;
			msleep(5);
		}

		/* start to convert frame. */
#ifdef CONFIG_AMLCAP_LOG_TIME_USEFORFRAMES
		do_gettimeofday(&start);
#endif
		vf = local_vf_get();
		if (vf) {
			src_canvas = vf->canvas0Addr;

			/* step1 convert 422 format to other format.*/
			if (is_need_ge2d_pre_process())
				src_canvas = vm_ge2d_pre_process(vf,context,&ge2d_config);
			local_vf_put(vf);
#ifdef CONFIG_AMLCAP_LOG_TIME_USEFORFRAMES
			do_gettimeofday(&end);
			time_use = (end.tv_sec - start.tv_sec)*1000 +
					(end.tv_usec - start.tv_usec) / 1000;
			printk("step 1, ge2d use: %ldms\n", time_use);
			do_gettimeofday(&start);
#endif

			/* step2 copy to user memory. */
			if (is_need_sw_post_process())
				vm_sw_post_process(src_canvas ,output_para.vaddr);
#ifdef CONFIG_AMLCAP_LOG_TIME_USEFORFRAMES
			do_gettimeofday(&end);
			time_use = (end.tv_sec - start.tv_sec) * 1000+
					(end.tv_usec - start.tv_usec) / 1000;
			printk("step 2, memcpy use: %ldms\n", time_use);
#endif
		}
        if (kthread_should_stop()){
            up(&vb_done_sema);
            break;
        }
		up(&vb_done_sema);
	}
vm_exit:
	destroy_ge2d_work_queue(context);
    while(!kthread_should_stop()){
	/* 	   may not call stop, wait..
                   it is killed by SIGTERM,eixt on down_interruptible
		   if not call stop,this thread may on do_exit and
		   kthread_stop may not work good;
	*/
	    msleep(10);
    }
	return ret;
}

/*simulate v4l2 device to request filling buffer,only for test use*/
static int simulate_task(void *data)
{
	while (1) {
		msleep(50);    
		vm_fill_buffer(NULL,NULL);
		printk("simulate succeed\n");
	}
	return 0;
}

/************************************************
*
*   init functions.
*
*************************************************/
int vm_buffer_init(void)
{
	int i;
	u32 canvas_width, canvas_height;
	u32 decbuf_size;
	resource_size_t buf_start;
	unsigned int buf_size;
	int buf_num = 0;
	int local_pool_size = 0;

	get_vm_buf_info(&buf_start,&buf_size, NULL);
	sema_init(&vb_start_sema,0);
	sema_init(&vb_done_sema,0);

	if(!buf_start || !buf_size)
		goto exit;

	for(i=0; i<ARRAY_SIZE(vmdecbuf_size);i++){
		if( buf_size >= vmdecbuf_size[i])
			break;
	}
	if(i==ARRAY_SIZE(vmdecbuf_size)){
		printk("vmbuf size=%d less than the smallest vmbuf size%d\n",
			buf_size, vmdecbuf_size[i-1]);
		return -1;
	}

	canvas_width = canvas_config_wh[i].width;//1920;
	canvas_height = canvas_config_wh[i].height;//1200;
	decbuf_size = vmdecbuf_size[i];//0x700000;
	buf_num  = buf_size/decbuf_size;

	if(buf_num > 0)
		local_pool_size   = 1;
	else {
		local_pool_size = 0;
		printk("need at least one buffer to handle 1920*1080 data.\n");
	}

	for (i = 0; i < local_pool_size; i++)
	{
		canvas_config((VM_DEPTH_16_CANVAS+i),
			(unsigned long)(buf_start + i * decbuf_size),
			canvas_width*2, canvas_height,
			CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);
		canvas_config((VM_DEPTH_24_CANVAS+i),
			(unsigned long)(buf_start + i * decbuf_size),
			canvas_width*3, canvas_height,
			CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);
		canvas_config((VM_DEPTH_8_CANVAS_Y+ i),
			(unsigned long)(buf_start + i*decbuf_size/2),
			canvas_width, canvas_height,
			CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);
		canvas_config(VM_DEPTH_8_CANVAS_UV + i,
			(unsigned long)(buf_start + (i+1)*decbuf_size/2),
			canvas_width, canvas_height/2,
			CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);

		canvas_config((VM_DEPTH_8_CANVAS_U + i),
			(unsigned long)(buf_start + (i+1)*decbuf_size/2),
			canvas_width/2, canvas_height/2,
			CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);
		canvas_config((VM_DEPTH_8_CANVAS_V + i),
			(unsigned long)(buf_start + (i+3)*decbuf_size/4),
			canvas_width/2, canvas_height/2,
			CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);
#ifndef CONFIG_AMLOGIC_VM_DISABLE_VIDEOLAYER
		vfbuf_use[i] = 0;
#endif
	}
exit:
	return 0;

}

int start_vm_task(void) {
	/* init the device. */
	vm_local_init();
	if(!task) {
		task=kthread_create(vm_task,0,"vm");
		if(IS_ERR(task)) {
			amlog_level(LOG_LEVEL_HIGH, "thread creating error.\n");
			return -1;
		}
		wake_up_process(task);
	}
    task_running = 1;
	return 0;
}

int start_simulate_task(void)
{
	if(!simulate_task_fd) {
		simulate_task_fd=kthread_create(simulate_task,0,"vm");
		if(IS_ERR(simulate_task_fd)) {
			amlog_level(LOG_LEVEL_HIGH, "thread creating error.\n");
			return -1;
		}
		wake_up_process(simulate_task_fd);
	}
	return 0;
}


void stop_vm_task(void) {
    if(task){
        task_running = 0;
        send_sig(SIGTERM, task, 1);
        up(&vb_start_sema);
        kthread_stop(task);
        task = NULL;
    }
    vm_local_init();
}


/***********************************************************************
*
* global status.
*
************************************************************************/

static int vm_enable_flag=0;

int get_vm_status() {
	return vm_enable_flag;
}

void set_vm_status(int flag) {
	if(flag >= 0)
		vm_enable_flag=flag;
	else
		vm_enable_flag=0;
}

/***********************************************************************
*
* file op section.
*
************************************************************************/

typedef  struct {
	char  			name[20];
	unsigned int 		open_count;
	int	 		major;
	unsigned  int 		dbg_enable;
	struct class 		*cla;
	struct device		*dev;
	resource_size_t buffer_start;
	unsigned int buffer_size;
	struct io_mapping *mapping;
}vm_device_t;

static vm_device_t  vm_device;
void set_vm_buf_info(resource_size_t start,unsigned int size) {
	vm_device.buffer_start=start;
	vm_device.buffer_size=size;
	vm_device.mapping = io_mapping_create_wc( start, size );
	amlog_level(LOG_LEVEL_HIGH,"#############%p\n",vm_device.mapping);
}

void get_vm_buf_info(resource_size_t* start,unsigned int* size,struct io_mapping **mapping) {
	if(start)
		*start = vm_device.buffer_start;
	if(size)
		*size = vm_device.buffer_size;
	if( mapping )
		*mapping = vm_device.mapping;
}

static int vm_open(struct inode *inode, struct file *file)
{
	 ge2d_context_t *context=NULL;
	 amlog_level(LOG_LEVEL_LOW,"open one vm device\n");
	 file->private_data=context;
	 vm_device.open_count++;
	 return 0;
}

static long vm_ioctl(struct file *filp, unsigned int cmd, unsigned long args)
{
	int  ret=0 ;
	ge2d_context_t *context;
	void  __user* argp;

	context=(ge2d_context_t *)filp->private_data;
	argp =(void __user*)args;
	switch (cmd)
   	{
	case VM_IOC_2OSD0:
		break;
	case VM_IOC_ENABLE_PP:
		break;
	case VM_IOC_CONFIG_FRAME:
		break;
	default :
		return -ENOIOCTLCMD;
	}
 	return ret;
}

static int vm_release(struct inode *inode, struct file *file)
{
	ge2d_context_t *context=(ge2d_context_t *)file->private_data;

	if(context && (0==destroy_ge2d_work_queue(context)))
	{
		vm_device.open_count--;

		return 0;
	}
	amlog_level(LOG_LEVEL_LOW,"release one vm device\n");
	return -1;
}

/***********************************************************************
*
* file op init section.
*
************************************************************************/

static const struct file_operations vm_fops = {
	.owner = THIS_MODULE,
	.open = vm_open,
	.unlocked_ioctl = vm_ioctl,
	.release = vm_release,
};

int init_vm_device(void)
{
	int  ret=0;

	strcpy(vm_device.name,"vm");
	ret=register_chrdev(0,vm_device.name,&vm_fops);
	if(ret <=0)
	{
		amlog_level(LOG_LEVEL_HIGH,"register vm device error\r\n");
		return  ret ;
	}
	vm_device.major=ret;
	vm_device.dbg_enable=0;
	amlog_level(LOG_LEVEL_LOW,"vm_dev major:%d\r\n",ret);

	vm_device.cla = init_vm_cls();
	if(vm_device.cla == NULL)
		return -1;
	vm_device.dev=device_create(vm_device.cla,NULL,MKDEV(vm_device.major,0)
						,NULL,vm_device.name);
	if (IS_ERR(vm_device.dev)) {
		amlog_level(LOG_LEVEL_HIGH,"create vm device error\n");
		goto unregister_dev;
	}

	if(vm_buffer_init()<0) goto unregister_dev;
#ifndef CONFIG_AMLOGIC_VM_DISABLE_VIDEOLAYER
	vf_provider_init(&vm_vf_prov, PROVIDER_NAME ,&vm_vf_provider, NULL);	
#endif
	//vf_reg_provider(&vm_vf_prov);
	vf_receiver_init(&vm_vf_recv, RECEIVER_NAME, &vm_vf_receiver, NULL);
	vf_reg_receiver(&vm_vf_recv);
	return 0;

unregister_dev:
	class_unregister(vm_device.cla);
	return -1;
}

int uninit_vm_device(void)
{
	stop_vm_task();
	if(vm_device.cla)
	{
		if(vm_device.dev)
		device_destroy(vm_device.cla, MKDEV(vm_device.major, 0));
		class_unregister(vm_device.cla);
	}

	unregister_chrdev(vm_device.major, vm_device.name);
	return  0;
}

/*******************************************************************
 *
 * interface for Linux driver
 *
 * ******************************************************************/

MODULE_AMLOG(AMLOG_DEFAULT_LEVEL, 0xff, LOG_LEVEL_DESC, LOG_MASK_DESC);

/* for driver. */
static int vm_driver_probe(struct platform_device *pdev)
{
	char* buf_start;
	unsigned int buf_size;
	struct resource *mem;

	if (!(mem = platform_get_resource(pdev, IORESOURCE_MEM, 0)))
	{
		buf_start = 0;
		buf_size = 0;
	} else {
		buf_start = (char *)mem->start;
		buf_size = mem->end - mem->start + 1;
	}
	set_vm_buf_info(mem->start,buf_size);
	init_vm_device();
	return 0;
}

static int vm_drv_remove(struct platform_device *plat_dev)
{
	uninit_vm_device();
	io_mapping_free( vm_device.mapping);
	return 0;
}

/* general interface for a linux driver .*/
static struct platform_driver vm_drv = {
	.probe  = vm_driver_probe,
	.remove = vm_drv_remove,
	.driver = {
		.name = "vm",
		.owner = THIS_MODULE,
	}
};

static int __init
vm_init_module(void)
{
	int err;

	amlog_level(LOG_LEVEL_HIGH,"vm_init\n");
	if ((err = platform_driver_register(&vm_drv))) {
		printk(KERN_ERR "Failed to register vm driver (error=%d\n", err);
		return err;
	}

	return err;
}

static void __exit
vm_remove_module(void)
{
	platform_driver_unregister(&vm_drv);
	amlog_level(LOG_LEVEL_HIGH,"vm module removed.\n");
}

module_init(vm_init_module);
module_exit(vm_remove_module);

MODULE_DESCRIPTION("Amlogic Video Input Manager");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Simon Zheng <simon.zheng@amlogic.com>");
