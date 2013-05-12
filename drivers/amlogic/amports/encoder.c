/*
 * AMLOGIC Audio/Video streaming port driver.
 *
 *
 * Author:  Simon Zheng <simon.zheng@amlogic.com>
 *
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <mach/am_regs.h>
#include <plat/io.h>
#include <linux/ctype.h>
#include <linux/amports/ptsserv.h>
#include <linux/amports/amstream.h>
#include <linux/amports/canvas.h>
#include <linux/amports/vframe.h>
#include <linux/amports/vframe_provider.h>
#include <linux/amports/vframe_receiver.h>
#include "vdec_reg.h"

#define ENC_CANVAS_OFFSET  AMVENC_CANVAS_INDEX

#define LOG_LEVEL_VAR 1
#define debug_level(level, x...) \	
	do { \		
		if (level >= LOG_LEVEL_VAR) \			
			printk(x); \	
	} while (0);

#define PUT_INTERVAL        (HZ/100)
#ifdef CONFIG_AM_VDEC_MJPEG_LOG
#define AMLOG
#define LOG_LEVEL_VAR       amlog_level_avc
#define LOG_MASK_VAR        amlog_mask_avc
#define LOG_LEVEL_ERROR     0
#define LOG_LEVEL_INFO      1
#define LOG_LEVEL_DESC  "0:ERROR, 1:INFO"
#endif
#include <linux/amlog.h>
MODULE_AMLOG(LOG_LEVEL_ERROR, 0, LOG_LEVEL_DESC, LOG_DEFAULT_MASK_DESC);

#include "encoder.h"
#include "amvdec.h"
#include "encoder_mc.h"
static avc_device_major = 0;
static struct class *amvenc_avc_class;
static struct device *amvenc_avc_dev;
#define DRIVER_NAME "amvenc_avc"
#define MODULE_NAME "amvenc_avc"
#define DEVICE_NAME "amvenc_avc"

/* protocol register usage
	#define ENCODER_STATUS            HENC_SCRATCH_0    : encode stage
	#define MEM_OFFSET_REG            HENC_SCRATCH_1    : assit buffer physical address
	#define DEBUG_REG  				  HENC_SCRATCH_2    : debug register
	#define MB_COUNT				  HENC_SCRATCH_3	: MB encoding number
*/

/*output buffer define*/
static unsigned BitstreamStart;  
static unsigned BitstreamEnd;  
static unsigned BitstreamIntAddr  ;   
/*input buffer define*/
static unsigned dct_buff_start_addr ;  
static unsigned dct_buff_end_addr   ;

/*deblock buffer define*/
static unsigned dblk_buf_addr     ;    
static unsigned dblk_buf_canvas		;	

/*reference buffer define*/
static unsigned ref_buf_addr ; //192
static unsigned ref_buf_canvas;  //((192<<16)|(192<<8)|(192<<0))

/*microcode assitant buffer*/
static unsigned assit_buffer_offset;
//static struct dec_sysinfo avc_amstream_dec_info;

static u32 stat;
static u32 cur_stage;          
static u32 frame_start;//0: processing 1:restart
static u32 quant = 28;
static u32 encoder_width = 1280;
static u32 encoder_height = 720;
static void avc_prot_init(void);
static void avc_local_init(void);
static u32 buf_start, buf_size;
static int idr_pic_id = 0;  //need reset as 0 for IDR
static u32 frame_number = 0 ;   //need plus each frame
static u32 pic_order_cnt_lsb = 0 ; //need reset as 0 for IDR and plus 2 for NON-IDR

static u32 log2_max_pic_order_cnt_lsb = 4 ;
static u32 log2_max_frame_num =4 ;
static u32 anc0_buffer_id =0;
static u32 qppicture  =26;

static const char avc_dec_id[] = "avc-dev";

static void avc_canvas_init(void);

void amvenc_reset(void);
static DEFINE_SPINLOCK(lock);

static void avc_put_timer_func(unsigned long arg)
{
    struct timer_list *timer = (struct timer_list *)arg;
    timer->expires = jiffies + PUT_INTERVAL;

    add_timer(timer);
}

int avc_dec_status(struct vdec_status *vstatus)
{
    return 0;
}

/*output stream buffer setting*/
static void avc_init_output_buffer(void)
{	
	WRITE_HREG(VLC_VB_MEM_CTL ,((1<<31)|(0x3f<<24)|(0x20<<16)|(2<<0)) );
	WRITE_HREG(VLC_VB_START_PTR, BitstreamStart);
	WRITE_HREG(VLC_VB_WR_PTR, BitstreamStart);
	WRITE_HREG(VLC_VB_SW_RD_PTR, BitstreamStart);	
	WRITE_HREG(VLC_VB_END_PTR, BitstreamEnd);
	WRITE_HREG(VLC_VB_CONTROL, 1);
	WRITE_HREG(VLC_VB_CONTROL, ((0<<14)|(7<<3)|(1<<1)|(0<<0)));
}

/*input dct buffer setting*/
static void avc_init_input_buffer(void)
{	
	WRITE_HREG(QDCT_MB_START_PTR ,dct_buff_start_addr );
	WRITE_HREG(QDCT_MB_END_PTR, dct_buff_end_addr);
	WRITE_HREG(QDCT_MB_WR_PTR, dct_buff_start_addr);
	WRITE_HREG(QDCT_MB_RD_PTR, dct_buff_start_addr);	
	WRITE_HREG(QDCT_MB_BUFF, 0);
}

/*input reference buffer setting*/
static void avc_init_reference_buffer(int canvas)
{	
	WRITE_HREG(ANC0_CANVAS_ADDR ,canvas);
	WRITE_HREG(VLC_HCMD_CONFIG ,0);
}

static void avc_init_assit_buffer()
{
	WRITE_HREG(MEM_OFFSET_REG,assit_buffer_offset);  //memory offset ?
}

/*deblock buffer setting, same as INI_CANVAS*/
static void avc_init_dblk_buffer(int canvas)
{
	WRITE_HREG(REC_CANVAS_ADDR,canvas);
	WRITE_HREG(DBKR_CANVAS_ADDR,canvas);
	WRITE_HREG(DBKW_CANVAS_ADDR,canvas);
}

/*same as INIT_ENCODER*/
static void avc_init_encoder(void)
{
	WRITE_HREG(VLC_TOTAL_BYTES, 0); 
	WRITE_HREG(VLC_CONFIG, 0x07);   
	WRITE_HREG(VLC_INT_CONTROL, 0);   
	//WRITE_HREG(ENCODER_STATUS,ENCODER_IDLE);
	WRITE_HREG(HCODEC_ASSIST_AMR1_INT0, 0x15);  
	WRITE_HREG(HCODEC_ASSIST_AMR1_INT1, 0x8);
	WRITE_HREG(HCODEC_ASSIST_AMR1_INT3, 0x14);
	WRITE_HREG(IDR_PIC_ID ,idr_pic_id);
	WRITE_HREG(FRAME_NUMBER ,frame_number);
	WRITE_HREG(PIC_ORDER_CNT_LSB,pic_order_cnt_lsb);
	log2_max_pic_order_cnt_lsb= 4;     
	log2_max_frame_num = 4;
	WRITE_HREG(LOG2_MAX_PIC_ORDER_CNT_LSB ,  log2_max_pic_order_cnt_lsb);
	WRITE_HREG(LOG2_MAX_FRAME_NUM , log2_max_frame_num);
	WRITE_HREG(ANC0_BUFFER_ID, anc0_buffer_id);
	WRITE_HREG(QPPICTURE, qppicture);	
}

/****************************************/
static void avc_canvas_init(void)
{
    int i;
    u32 canvas_width, canvas_height;
    u32 decbuf_size, decbuf_y_size, decbuf_uv_size;
    int dct_buf_size;
    int output_buf_size;
    int start_addr = buf_start;
    /*now support maximum size is 1280*720*/	
    canvas_width   = encoder_width;
    canvas_height  = encoder_height;

	/*input dct buffer config */ 
    dct_buf_size =   0x2f8000 ;//(1280>>4)*(720>>4)*864 0x2f7600
    dct_buff_start_addr = start_addr ;
    dct_buff_end_addr = buf_start + dct_buf_size -1 ;
    debug_level(0,"dct_buff_start_addr is %x \n",dct_buff_start_addr);

    decbuf_y_size  = 0xf0000;   //1280*720
    decbuf_uv_size = 0x80000;	//1280*720/2 : NV21
    decbuf_size    = 0x180000;	//1280*720*1.5	
    start_addr = buf_start + 0x300000;
	/*config as NV21*/
    for (i = 0; i < 2; i++) {
        canvas_config(3 * i + 0 + ENC_CANVAS_OFFSET,
                      start_addr + i * decbuf_size,
                      canvas_width, canvas_height,
                      CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);
        canvas_config(3 * i + 1 + ENC_CANVAS_OFFSET,
                      start_addr + i * decbuf_size + decbuf_y_size,
                      canvas_width , canvas_height / 2,
                      CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);
/*here the third plane use the same address as the second plane*/                      
        canvas_config(3 * i + 2 + ENC_CANVAS_OFFSET,
                      start_addr + i * decbuf_size + decbuf_y_size,
                      canvas_width , canvas_height / 2,
                      CANVAS_ADDR_NOWRAP, CANVAS_BLKMODE_LINEAR);

    }
    assit_buffer_offset = buf_start + 0x300000 + 0x300000 + 0x40000;
    debug_level(0,"assit_buffer_offset is %x \n",assit_buffer_offset);
	/*output stream buffer config*/
    output_buf_size = 0x100000;
    BitstreamStart  = buf_start + 0x700000;
    BitstreamEnd  =  BitstreamStart + output_buf_size -1;
    debug_level(0,"BitstreamStart is %x \n",BitstreamStart);    

    dblk_buf_canvas = ((ENC_CANVAS_OFFSET+2) <<16)|((ENC_CANVAS_OFFSET + 1) <<8)|(ENC_CANVAS_OFFSET);
    ref_buf_canvas = ((ENC_CANVAS_OFFSET +5) <<16)|((ENC_CANVAS_OFFSET + 4) <<8)|(ENC_CANVAS_OFFSET +3);
    debug_level(0,"dblk_buf_canvas is %d ; ref_buf_canvas is %d \n",dblk_buf_canvas , ref_buf_canvas);
}

static void init_scaler(void)
{
}

static void avc_local_init(void)
{

}
static int encoder_status;
static irqreturn_t enc_isr(int irq, void *dev_id)
{
	u32 reg;
	int temp_canvas;
	WRITE_HREG(HCODEC_ASSIST_MBOX1_CLR_REG, 1);
	encoder_status  = READ_HREG(ENCODER_STATUS);
	if((encoder_status == ENCODER_IDR_DONE)
	||(encoder_status == ENCODER_NON_IDR_DONE)){
	//||(encoder_status == ENCODER_SEQUENCE_DONE)
	//||(encoder_status == ENCODER_PICTURE_DONE)){
		temp_canvas = dblk_buf_canvas;
		dblk_buf_canvas = ref_buf_canvas;
		ref_buf_canvas = temp_canvas;   //current dblk buffer as next reference buffer
		debug_level(0,"encoder stage is %d\n",encoder_status);
	}
	if((encoder_status == ENCODER_IDR_DONE)
	||(encoder_status == ENCODER_NON_IDR_DONE)){
		frame_start = 1;
		frame_number ++;
		pic_order_cnt_lsb += 2;		
		debug_level(0,"encoder is done %d\n",encoder_status);
	}
	return IRQ_HANDLED;
}
int avc_endian = 6;
static void avc_prot_init(void)
{
	unsigned int data32;

	int pic_width, pic_height;
	int pic_mb_nr;
	int pic_mbx, pic_mby;
	int i_pic_qp, p_pic_qp;

	int i_pic_qp_c, p_pic_qp_c;
	pic_width  = encoder_width; 
	pic_height = encoder_height; 
	pic_mb_nr  = 0; 
	pic_mbx    = 0; 
	pic_mby    = 0; 
	i_pic_qp   = quant; 
	p_pic_qp   = quant; 
	WRITE_HREG(VLC_PIC_SIZE, pic_width | (pic_height<<16));	
	WRITE_HREG(VLC_PIC_POSITION, (pic_mb_nr<<16) | (pic_mby << 8) |  (pic_mbx <<0));	//start mb

    switch (i_pic_qp) {    // synopsys parallel_case full_case
      case 0 : i_pic_qp_c = 0; break;
      case 1 : i_pic_qp_c = 1; break;
      case 2 : i_pic_qp_c = 2; break;
      case 3 : i_pic_qp_c = 3; break;
      case 4 : i_pic_qp_c = 4; break;
      case 5 : i_pic_qp_c = 5; break;
      case 6 : i_pic_qp_c = 6; break;
      case 7 : i_pic_qp_c = 7; break;
      case 8 : i_pic_qp_c = 8; break;
      case 9 : i_pic_qp_c = 9; break;
      case 10 : i_pic_qp_c = 10; break;
      case 11 : i_pic_qp_c = 11; break;
      case 12 : i_pic_qp_c = 12; break;
      case 13 : i_pic_qp_c = 13; break;
      case 14 : i_pic_qp_c = 14; break;
      case 15 : i_pic_qp_c = 15; break;
      case 16 : i_pic_qp_c = 16; break;
      case 17 : i_pic_qp_c = 17; break;
      case 18 : i_pic_qp_c = 18; break;
      case 19 : i_pic_qp_c = 19; break;
      case 20 : i_pic_qp_c = 20; break;
      case 21 : i_pic_qp_c = 21; break;
      case 22 : i_pic_qp_c = 22; break;
      case 23 : i_pic_qp_c = 23; break;
      case 24 : i_pic_qp_c = 24; break;
      case 25 : i_pic_qp_c = 25; break;
      case 26 : i_pic_qp_c = 26; break;
      case 27 : i_pic_qp_c = 27; break;
      case 28 : i_pic_qp_c = 28; break;
      case 29 : i_pic_qp_c = 29; break;
      case 30 : i_pic_qp_c = 29; break;
      case 31 : i_pic_qp_c = 30; break;
      case 32 : i_pic_qp_c = 31; break;
      case 33 : i_pic_qp_c = 32; break;
      case 34 : i_pic_qp_c = 32; break;
      case 35 : i_pic_qp_c = 33; break;
      case 36 : i_pic_qp_c = 34; break;
      case 37 : i_pic_qp_c = 34; break;
      case 38 : i_pic_qp_c = 35; break;
      case 39 : i_pic_qp_c = 35; break;
      case 40 : i_pic_qp_c = 36; break;
      case 41 : i_pic_qp_c = 36; break;
      case 42 : i_pic_qp_c = 37; break;
      case 43 : i_pic_qp_c = 37; break;
      case 44 : i_pic_qp_c = 37; break;
      case 45 : i_pic_qp_c = 38; break;
      case 46 : i_pic_qp_c = 38; break;
      case 47 : i_pic_qp_c = 38; break;
      case 48 : i_pic_qp_c = 39; break;
      case 49 : i_pic_qp_c = 39; break;
      case 50 : i_pic_qp_c = 39; break;
    default : i_pic_qp_c = 39; break; // should only be 51 or more (when index_offset)
    }

    switch (p_pic_qp) {    // synopsys parallel_case full_case
      case 0 : p_pic_qp_c = 0; break;
      case 1 : p_pic_qp_c = 1; break;
      case 2 : p_pic_qp_c = 2; break;
      case 3 : p_pic_qp_c = 3; break;
      case 4 : p_pic_qp_c = 4; break;
      case 5 : p_pic_qp_c = 5; break;
      case 6 : p_pic_qp_c = 6; break;
      case 7 : p_pic_qp_c = 7; break;
      case 8 : p_pic_qp_c = 8; break;
      case 9 : p_pic_qp_c = 9; break;
      case 10 : p_pic_qp_c = 10; break;
      case 11 : p_pic_qp_c = 11; break;
      case 12 : p_pic_qp_c = 12; break;
      case 13 : p_pic_qp_c = 13; break;
      case 14 : p_pic_qp_c = 14; break;
      case 15 : p_pic_qp_c = 15; break;
      case 16 : p_pic_qp_c = 16; break;
      case 17 : p_pic_qp_c = 17; break;
      case 18 : p_pic_qp_c = 18; break;
      case 19 : p_pic_qp_c = 19; break;
      case 20 : p_pic_qp_c = 20; break;
      case 21 : p_pic_qp_c = 21; break;
      case 22 : p_pic_qp_c = 22; break;
      case 23 : p_pic_qp_c = 23; break;
      case 24 : p_pic_qp_c = 24; break;
      case 25 : p_pic_qp_c = 25; break;
      case 26 : p_pic_qp_c = 26; break;
      case 27 : p_pic_qp_c = 27; break;
      case 28 : p_pic_qp_c = 28; break;
      case 29 : p_pic_qp_c = 29; break;
      case 30 : p_pic_qp_c = 29; break;
      case 31 : p_pic_qp_c = 30; break;
      case 32 : p_pic_qp_c = 31; break;
      case 33 : p_pic_qp_c = 32; break;
      case 34 : p_pic_qp_c = 32; break;
      case 35 : p_pic_qp_c = 33; break;
      case 36 : p_pic_qp_c = 34; break;
      case 37 : p_pic_qp_c = 34; break;
      case 38 : p_pic_qp_c = 35; break;
      case 39 : p_pic_qp_c = 35; break;
      case 40 : p_pic_qp_c = 36; break;
      case 41 : p_pic_qp_c = 36; break;
      case 42 : p_pic_qp_c = 37; break;
      case 43 : p_pic_qp_c = 37; break;
      case 44 : p_pic_qp_c = 37; break;
      case 45 : p_pic_qp_c = 38; break;
      case 46 : p_pic_qp_c = 38; break;
      case 47 : p_pic_qp_c = 38; break;
      case 48 : p_pic_qp_c = 39; break;
      case 49 : p_pic_qp_c = 39; break;
      case 50 : p_pic_qp_c = 39; break;
    default : p_pic_qp_c = 39; break; // should only be 51 or more (when index_offset)
    }
	WRITE_HREG(QDCT_Q_QUANT_I,
				(i_pic_qp_c<<22) | 
                (i_pic_qp<<16) | 
                ((i_pic_qp_c%6)<<12)|((i_pic_qp_c/6)<<8)|((i_pic_qp%6)<<4)|((i_pic_qp/6)<<0));	

   WRITE_HREG(QDCT_Q_QUANT_P,                
				(p_pic_qp_c<<22) | 
                (p_pic_qp<<16) | 
                ((p_pic_qp_c%6)<<12)|((p_pic_qp_c/6)<<8)|((p_pic_qp%6)<<4)|((p_pic_qp/6)<<0));	



	//avc_init_input_buffer();


    WRITE_HREG(IGNORE_CONFIG , 
                (1<<31) | // ignore_lac_coeff_en
                (1<<26) | // ignore_lac_coeff_else (<1)
                (1<<21) | // ignore_lac_coeff_2 (<1)
                (2<<16) | // ignore_lac_coeff_1 (<2)
                (1<<15) | // ignore_cac_coeff_en
                (1<<10) | // ignore_cac_coeff_else (<1)
                (1<<5)  | // ignore_cac_coeff_2 (<1)
                (2<<0))    // ignore_cac_coeff_1 (<2)
                ; 

    WRITE_HREG(IGNORE_CONFIG_2,
                (1<<31) | // ignore_t_lac_coeff_en
                (1<<26) | // ignore_t_lac_coeff_else (<1)
                (1<<21) | // ignore_t_lac_coeff_2 (<1)
                (5<<16) | // ignore_t_lac_coeff_1 (<5)
                (0<<0))
                ; 

    WRITE_HREG(QDCT_MB_CONTROL,
                (1<<9) | // mb_info_soft_reset
                (1<<0) ) // mb read buffer soft reset
              ;
    WRITE_HREG(QDCT_MB_CONTROL,
              (0<<28) | // ignore_t_p8x8
              (0<<27) | // zero_mc_out_null_non_skipped_mb
              (0<<26) | // no_mc_out_null_non_skipped_mb
              (0<<25) | // mc_out_even_skipped_mb
              (0<<24) | // mc_out_wait_cbp_ready
              (0<<23) | // mc_out_wait_mb_type_ready
              (1<<22) | // i_pred_int_enable
              (1<<19) | // i_pred_enable
              (1<<20) | // ie_sub_enable
              (1<<18) | // iq_enable
              (1<<17) | // idct_enable
              (1<<14) | // mb_pause_enable
              (1<<13) | // q_enable
              (1<<12) | // dct_enable
              (1<<10) | // mb_info_en
              (avc_endian<<3) | // endian
              (1<<1) | // mb_read_en
              (0<<0))   // soft reset
            ; // soft reset
    //debug_level(0,"current endian is %d \n" , avc_endian);
    data32 = READ_HREG(VLC_CONFIG);
    data32 = data32 | (1<<0); // set pop_coeff_even_all_zero
    WRITE_HREG(VLC_CONFIG , data32);	
    
        /* clear mailbox interrupt */
    WRITE_HREG(HCODEC_ASSIST_MBOX1_CLR_REG, 1);

    /* enable mailbox interrupt */
    WRITE_HREG(HCODEC_ASSIST_MBOX1_MASK, 1);
    
}

void amvenc_reset(void)
{
    READ_VREG(DOS_SW_RESET1);
    READ_VREG(DOS_SW_RESET1);
    READ_VREG(DOS_SW_RESET1);
    WRITE_VREG(DOS_SW_RESET1, (1<<2)|(1<<6)|(1<<7)|(1<<8)|(1<<16)|(1<<17));
    WRITE_VREG(DOS_SW_RESET1, 0);
    READ_VREG(DOS_SW_RESET1);
    READ_VREG(DOS_SW_RESET1);
    READ_VREG(DOS_SW_RESET1);

}
void amvenc_start(void)
{
    READ_VREG(DOS_SW_RESET1);
    READ_VREG(DOS_SW_RESET1);
    READ_VREG(DOS_SW_RESET1);
    WRITE_VREG(DOS_SW_RESET1, (1<<12)|(1<<11));
    WRITE_VREG(DOS_SW_RESET1, 0);

    READ_VREG(DOS_SW_RESET1);
    READ_VREG(DOS_SW_RESET1);
    READ_VREG(DOS_SW_RESET1);

    WRITE_HREG(MPSR, 0x0001);
}

void amvenc_stop(void)
{
    ulong timeout = jiffies + HZ;

    WRITE_HREG(MPSR, 0);
    WRITE_HREG(CPSR, 0);
    while (READ_HREG(IMEM_DMA_CTRL) & 0x8000) {
        if (time_after(jiffies, timeout)) {
            break;
        }
    }
    READ_VREG(DOS_SW_RESET1);
    READ_VREG(DOS_SW_RESET1);
    READ_VREG(DOS_SW_RESET1);

    WRITE_VREG(DOS_SW_RESET1, (1<<12)|(1<<11)|(1<<2)|(1<<6)|(1<<7)|(1<<8)|(1<<16)|(1<<17));
    //WRITE_VREG(DOS_SW_RESET1, (1<<12)|(1<<11));
    WRITE_VREG(DOS_SW_RESET1, 0);

    READ_VREG(DOS_SW_RESET1);
    READ_VREG(DOS_SW_RESET1);
    READ_VREG(DOS_SW_RESET1);
}


#if 0
static void *mc_addr=NULL;
static dma_addr_t mc_addr_map;
#define MC_SIZE (4096 * 4)

s32 amvenc_loadmc(const u32 *p)
{
    ulong timeout;
    s32 ret = 0;
    return ret;
	if(mc_addr==NULL)
	{
		mc_addr = kmalloc(MC_SIZE, GFP_KERNEL);
		printk("Alloc new mc addr to %p\n",mc_addr);
	}
    if (!mc_addr) {
        return -ENOMEM;
    }

    memcpy(mc_addr, p, MC_SIZE);
    printk("address 0 is 0x%x\n", *((u32*)mc_addr));
    printk("address 1 is 0x%x\n", *((u32*)mc_addr + 1));
    printk("address 2 is 0x%x\n", *((u32*)mc_addr + 2));
    printk("address 3 is 0x%x\n", *((u32*)mc_addr + 3));

    mc_addr_map = dma_map_single(NULL, mc_addr, MC_SIZE, DMA_TO_DEVICE);
	printk("mc_addr_map is 0x%x\n" ,mc_addr_map);
    WRITE_HREG(MPSR, 0);
    WRITE_HREG(CPSR, 0);

    /* Read CBUS register for timing */
    timeout = READ_HREG(MPSR);
    timeout = READ_HREG(MPSR);

    timeout = jiffies + HZ;

    WRITE_HREG(IMEM_DMA_ADR, mc_addr_map);
    WRITE_HREG(IMEM_DMA_COUNT, 0x1000);
    WRITE_HREG(IMEM_DMA_CTRL, (0x8000 |  (7 << 16)));

    while (READ_HREG(IMEM_DMA_CTRL) & 0x8000) {
        if (time_before(jiffies, timeout)) {
            schedule();
        } else {
            printk("hcodec load mc error\n");
            ret = -EBUSY;
            break;
        }
    }

    dma_unmap_single(NULL, mc_addr_map, MC_SIZE, DMA_TO_DEVICE);
	kfree(mc_addr);
	mc_addr=NULL;	

    return ret;
}
#else
static __iomem *mc_addr=NULL;
static unsigned mc_addr_map;
#define MC_SIZE (4096 * 4)
s32 amvenc_loadmc(const u32 *p)
{
    ulong timeout;
    s32 ret = 0 ;
    
    mc_addr_map = assit_buffer_offset;
    mc_addr = ioremap_wc(mc_addr_map,MC_SIZE);
    memcpy(mc_addr, p, MC_SIZE);
    debug_level(0,"address 0 is 0x%x\n", *((u32*)mc_addr));
    debug_level(0,"address 1 is 0x%x\n", *((u32*)mc_addr + 1));
    debug_level(0,"address 2 is 0x%x\n", *((u32*)mc_addr + 2));
    debug_level(0,"address 3 is 0x%x\n", *((u32*)mc_addr + 3));
    WRITE_HREG(MPSR, 0);
    WRITE_HREG(CPSR, 0);

    /* Read CBUS register for timing */
    timeout = READ_HREG(MPSR);
    timeout = READ_HREG(MPSR);

    timeout = jiffies + HZ;

    WRITE_HREG(IMEM_DMA_ADR, mc_addr_map);
    WRITE_HREG(IMEM_DMA_COUNT, 0x1000);
    WRITE_HREG(IMEM_DMA_CTRL, (0x8000 |   (7 << 16)));

    while (READ_HREG(IMEM_DMA_CTRL) & 0x8000) {
        if (time_before(jiffies, timeout)) {
            schedule();
        } else {
            debug_level(1,"hcodec load mc error\n");
            ret = -EBUSY;
            break;
        }
    }
    iounmap(mc_addr);
    mc_addr=NULL;	

    return ret;
}
#endif
#define  DMC_SEC_PORT8_RANGE0  0x840
#define  DMC_SEC_CTRL  0x829
void enable_hcoder_ddr_access()
{
	WRITE_SEC_REG(DMC_SEC_PORT8_RANGE0 , 0xffff);
	WRITE_SEC_REG(DMC_SEC_CTRL , 0x80000000);
}

static s32 avc_check()
{
	enable_hcoder_ddr_access();
	hvdec_clock_enable();
	return 0;
}

static s32 avc_init(void)
{
    int r;   
    avc_canvas_init();
    WRITE_HREG(HCODEC_ASSIST_MMC_CTRL1,0x2);
    debug_level(1,"start to load microcode\n");
    if (amvenc_loadmc(encoder_mc) < 0) {
        //amvdec_disable();
        return -EBUSY;
    }	
    debug_level(1,"succeed to load microcode\n");
    //avc_canvas_init();
    frame_start = 1;
    idr_pic_id = -1 ;	
    frame_number = 0 ;
    pic_order_cnt_lsb = 0 ;
    amvenc_reset();
    avc_init_encoder(); 
    avc_init_input_buffer();  //dct buffer setting
    avc_init_output_buffer();  //output stream buffer
    avc_prot_init();
    r = request_irq(INT_MAILBOX_1A, enc_isr, IRQF_SHARED, "enc-irq", (void *)avc_dec_id);	
    avc_init_dblk_buffer(dblk_buf_canvas);   //decoder buffer , need set before each frame start
    avc_init_reference_buffer(ref_buf_canvas); //reference  buffer , need set before each frame start
    avc_init_assit_buffer(); //assitant buffer for microcode
    //amvenc_start();
    return 0;
}

void amvenc_avc_start_cmd(int cmd)
{
	int status= 0;
	int need_start_cpu = 0;
	if((cmd == ENCODER_IDR)||(cmd == ENCODER_SEQUENCE)){
		pic_order_cnt_lsb = 0;	
		frame_number = 0;
	}
	if(frame_number > 65535){
		frame_number = 0;
	}	
	if(frame_start){
		frame_start = 0;
		if(cmd != ENCODER_NON_IDR){
			idr_pic_id ++;
		}
		if(idr_pic_id > 255){
			idr_pic_id = 0;
		}		
		encoder_status = ENCODER_IDLE ;
		//WRITE_HREG(HENC_SCRATCH_3,0);  //mb count 
		//WRITE_HREG(VLC_TOTAL_BYTES ,0); //offset in bitstream buffer
		//amvenc_reset();
		amvenc_stop();
		need_start_cpu = 1;
		avc_init_encoder();
		avc_init_input_buffer();
		avc_init_output_buffer();		
		avc_prot_init();
		avc_init_assit_buffer(); 
		debug_level(0,"begin to new frame %d\n");
	}
	avc_init_dblk_buffer(dblk_buf_canvas);   
	avc_init_reference_buffer(ref_buf_canvas); 
       WRITE_HREG(ENCODER_STATUS , cmd);
	if(need_start_cpu){
	    amvenc_start();	
	}
	debug_level(0,"amvenc_avc_start\n");
}

void amvenc_avc_stop(void)
{
	//WRITE_HREG(MPSR, 0);	
	amvenc_stop();
	debug_level(1,"amvenc_avc_stop\n");
}
static int amvenc_avc_open(struct inode *inode, struct file *file)
{
    int r = 0;
    debug_level(1,"avc open\n");
    if (avc_check() < 0) {
        amlog_level(LOG_LEVEL_ERROR, "amvenc_avc init failed.\n");

        return -ENODEV;
    }
    return r;
}

static int amvenc_avc_release(struct inode *inode, struct file *file)
{
    free_irq(INT_MAILBOX_1A, (void *)avc_dec_id);
    //amvdec_disable();
    amvenc_avc_stop();
    debug_level(1,"avc release\n");
    return 0;
}
static void dma_flush(unsigned buf_start , unsigned buf_size )
{
    //dma_sync_single_for_cpu(amvenc_avc_dev,buf_start, buf_size, DMA_TO_DEVICE);
	dma_sync_single_for_device(amvenc_avc_dev,buf_start ,buf_size, DMA_TO_DEVICE);
}

static void cache_flush(unsigned buf_start , unsigned buf_size )
{
	dma_sync_single_for_cpu(amvenc_avc_dev , buf_start, buf_size, DMA_FROM_DEVICE);
	//dma_sync_single_for_device(amvenc_avc_dev ,buf_start , buf_size, DMA_FROM_DEVICE);
}

static long amvenc_avc_ioctl(struct file *file,
                           unsigned int cmd, ulong arg)
{
    int r = 0;
    int amrisc_cmd = 0;
    unsigned* offset;
    unsigned* addr_info;
    unsigned buf_start;
    switch (cmd) {
    case AMVENC_AVC_IOC_GET_ADDR:
    // *((unsigned*)arg) 
		if((ref_buf_canvas & 0xff) == (ENC_CANVAS_OFFSET)){
			 *((unsigned*)arg)  = 1;
		}else{
			 *((unsigned*)arg)  = 2;	
		}
		break;
    case AMVENC_AVC_IOC_INPUT_UPDATE:
		offset  = (unsigned*)arg ;
		WRITE_HREG(QDCT_MB_WR_PTR, (dct_buff_start_addr+ *offset));
		break;    
    case AMVENC_AVC_IOC_NEW_CMD:
		amrisc_cmd = *((unsigned*)arg) ;
		amvenc_avc_start_cmd(amrisc_cmd);
		break;
    case AMVENC_AVC_IOC_GET_STAGE:
		*((unsigned*)arg)  = encoder_status;
		break; 
	case AMVENC_AVC_IOC_GET_OUTPUT_SIZE:	
		offset = READ_HREG(VLC_TOTAL_BYTES);
		*((unsigned*)arg) = offset;
		break;
	case AMVENC_AVC_IOC_SET_QUANT:
		quant = *((unsigned*)arg) ;
		break;
	case AMVENC_AVC_IOC_SET_ENCODER_WIDTH:
		encoder_width = *((unsigned*)arg) ;
		break;
	case AMVENC_AVC_IOC_SET_ENCODER_HEIGHT:
		encoder_height = *((unsigned*)arg) ;
		break;	
	case AMVENC_AVC_IOC_CONFIG_INIT:
	    avc_init();
		break;		
	case AMVENC_AVC_IOC_FLUSH_CACHE:
		addr_info  = (unsigned*)arg ;
		switch(addr_info[0]){
			case 0:
			buf_start = dct_buff_start_addr;
			break;
			case 1:
			buf_start = dct_buff_start_addr + 0x300000;
			break;
			case 2:
			buf_start = dct_buff_start_addr + 0x480000;
			break;
			case 3:
			buf_start = BitstreamStart ;
			break;
			default:
			buf_start = dct_buff_start_addr;
			break;
		}
	    dma_flush(buf_start + addr_info[1] ,addr_info[2] - addr_info[1]);
		break;
	case AMVENC_AVC_IOC_FLUSH_DMA:
	    addr_info  = (unsigned*)arg ;
		addr_info  = (unsigned*)arg ;
		switch(addr_info[0]){
			case 0:
			buf_start = dct_buff_start_addr;
			break;
			case 1:
			buf_start = dct_buff_start_addr + 0x300000;
			break;
			case 2:
			buf_start = dct_buff_start_addr + 0x480000;
			break;
			case 3:
			buf_start = BitstreamStart ;
			break;
			default:
			buf_start = dct_buff_start_addr;
			break;
		}	    
		cache_flush(buf_start + addr_info[1] ,addr_info[2] - addr_info[1]);
		break;
     default:
		break;
	}
    return r;
}



static int avc_mmap(struct file *filp, struct vm_area_struct *vma)
{
    unsigned long off = vma->vm_pgoff << PAGE_SHIFT;
    unsigned vma_size = vma->vm_end - vma->vm_start;

    if (vma_size == 0) {
        debug_level(1,"vma_size is 0 \n");
        return -EAGAIN;
    }
    off += buf_start;
    debug_level(0,"vma_size is %d , off is %d \n" , vma_size ,off);
    vma->vm_flags |= VM_RESERVED | VM_IO;
    //vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
    if (remap_pfn_range(vma, vma->vm_start, off >> PAGE_SHIFT,
                        vma->vm_end - vma->vm_start, vma->vm_page_prot)) {
        debug_level(1,"set_cached: failed remap_pfn_range\n");
        return -EAGAIN;
    }
    return 0;

}

const static struct file_operations amvenc_avc_fops = {
    .owner    = THIS_MODULE,
    .open     = amvenc_avc_open,
    .mmap     = avc_mmap,
    .release  = amvenc_avc_release,
    .unlocked_ioctl    = amvenc_avc_ioctl,
};

int  init_avc_device(void)
{
	int  r =0;
    r =register_chrdev(0,DEVICE_NAME,&amvenc_avc_fops);
    if(r  <=0) 
    {
        amlog_level(LOG_LEVEL_HIGH,"register amvenc_avc device error\r\n");
        return  r  ;
    }
    avc_device_major= r ;
    
    amvenc_avc_class = class_create(THIS_MODULE, DEVICE_NAME);

    amvenc_avc_dev = device_create(amvenc_avc_class, NULL,
                                  MKDEV(avc_device_major, 0), NULL,
                                  DEVICE_NAME);    
}
int uninit_avc_device()
{
    device_destroy(amvenc_avc_class, MKDEV(avc_device_major, 0));

    class_destroy(amvenc_avc_class);

    unregister_chrdev(avc_device_major, DEVICE_NAME);	
}

static int amvenc_avc_probe(struct platform_device *pdev)
{
    struct resource *mem;

    amlog_level(LOG_LEVEL_INFO, "amvenc_avc probe start.\n");

    if (!(mem = platform_get_resource(pdev, IORESOURCE_MEM, 0))) {
        amlog_level(LOG_LEVEL_ERROR, "amvenc_avc memory resource undefined.\n");
        return -EFAULT;
    }

    buf_start = mem->start;
    buf_size  = mem->end - mem->start + 1;

    //memcpy(&avc_amstream_dec_info, (void *)mem[1].start, sizeof(avc_amstream_dec_info));
    init_avc_device();
    //avc_canvas_init();
    amlog_level(LOG_LEVEL_INFO, "amvenc_avc probe end.\n");

    return 0;
}

static int amvenc_avc_remove(struct platform_device *pdev)
{
    uninit_avc_device();
    amlog_level(LOG_LEVEL_INFO, "amvenc_avc remove.\n");
    return 0;
}

/****************************************/

static struct platform_driver amvenc_avc_driver = {
    .probe      = amvenc_avc_probe,
    .remove     = amvenc_avc_remove,
    .driver     = {
        .name   = DRIVER_NAME,
    }
};
static struct codec_profile_t amvenc_avc_profile = {
	.name = "avc",
	.profile = ""
};
static int __init amvenc_avc_driver_init_module(void)
{
    amlog_level(LOG_LEVEL_INFO, "amvenc_avc module init\n");

    if (platform_driver_register(&amvenc_avc_driver)) {
        amlog_level(LOG_LEVEL_ERROR, "failed to register amvenc_avc driver\n");
        return -ENODEV;
    }
	vcodec_profile_register(&amvenc_avc_profile);
    return 0;
}

static void __exit amvenc_avc_driver_remove_module(void)
{
    amlog_level(LOG_LEVEL_INFO, "amvenc_avc module remove.\n");
	
    platform_driver_unregister(&amvenc_avc_driver);
}

/****************************************/

module_param(stat, uint, 0664);
MODULE_PARM_DESC(stat, "\n amvenc_avc stat \n");

module_init(amvenc_avc_driver_init_module);
module_exit(amvenc_avc_driver_remove_module);

MODULE_DESCRIPTION("AMLOGIC AVC Video Encoder Driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("simon.zheng <simon.zheng@amlogic.com>");
