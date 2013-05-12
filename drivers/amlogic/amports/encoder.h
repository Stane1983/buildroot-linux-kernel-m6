
#ifndef __H264_H__
#define __H264_H__

#define VDEC_166M()  WRITE_MPEG_REG(HHI_VDEC_CLK_CNTL, (5 << 9) | (1 << 8) | (5))
#define VDEC_200M()  WRITE_MPEG_REG(HHI_VDEC_CLK_CNTL, (5 << 9) | (1 << 8) | (4))
#define VDEC_250M()  WRITE_MPEG_REG(HHI_VDEC_CLK_CNTL, (5 << 9) | (1 << 8) | (3))
#define VDEC_333M()  WRITE_MPEG_REG(HHI_VDEC_CLK_CNTL, (5 << 9) | (1 << 8) | (2))

#define HDEC_250M()   WRITE_MPEG_REG(HHI_VDEC_CLK_CNTL, (0 << 25) | (3 << 16) |(1 << 24) | (3  << 9)|(0<<0)|(1<<8) )
#define hvdec_clock_enable() \
    HDEC_250M(); \
    WRITE_VREG(DOS_GCLK_EN0, 0xffffffff)

#define AMVENC_AVC_IOC_MAGIC  'E'

#define AMVENC_AVC_IOC_GET_ADDR			 		_IOW(AMVENC_AVC_IOC_MAGIC, 0x00, unsigned int)
#define AMVENC_AVC_IOC_INPUT_UPDATE				_IOW(AMVENC_AVC_IOC_MAGIC, 0x01, unsigned int)
#define AMVENC_AVC_IOC_GET_STATUS				_IOW(AMVENC_AVC_IOC_MAGIC, 0x02, unsigned int)
#define AMVENC_AVC_IOC_NEW_CMD					_IOW(AMVENC_AVC_IOC_MAGIC, 0x03, unsigned int)
#define AMVENC_AVC_IOC_GET_STAGE				_IOW(AMVENC_AVC_IOC_MAGIC, 0x04, unsigned int)
#define AMVENC_AVC_IOC_GET_OUTPUT_SIZE			_IOW(AMVENC_AVC_IOC_MAGIC, 0x05, unsigned int)
#define AMVENC_AVC_IOC_SET_QUANT 				_IOW(AMVENC_AVC_IOC_MAGIC, 0x06, unsigned int)
#define AMVENC_AVC_IOC_SET_ENCODER_WIDTH 		_IOW(AMVENC_AVC_IOC_MAGIC, 0x07, unsigned int)
#define AMVENC_AVC_IOC_SET_ENCODER_HEIGHT 		_IOW(AMVENC_AVC_IOC_MAGIC, 0x08, unsigned int)
#define AMVENC_AVC_IOC_CONFIG_INIT 				_IOW(AMVENC_AVC_IOC_MAGIC, 0x09, unsigned int)
#define AMVENC_AVC_IOC_FLUSH_CACHE 				_IOW(AMVENC_AVC_IOC_MAGIC, 0x0a, unsigned int)
#define AMVENC_AVC_IOC_FLUSH_DMA 				_IOW(AMVENC_AVC_IOC_MAGIC, 0x0b, unsigned int)




// Memory Address 
///////////////////////////////////////////////////////////////////////////
#define MicrocodeStart        0x01cc0000
#define MicrocodeEnd          0x01cc3fff  // 4kx32bits
#define HencTopStart          0x01cc4000 
#define HencTopEnd            0x01cc4fff  // 128*32 = 0x1000
#define PredTopStart          0x01cc5000 
#define PredTopEnd            0x01cc5fff  // 128x32 = 0x1000 
#define MBBOT_START_0         0x01cc6000
#define MBBOT_START_1         0x01cc8000


#define MB_PER_DMA            (256*16/64) // 256 Lmem can hold MB_PER_DMA TOP Info 
#define MB_PER_DMA_COUNT_I    (MB_PER_DMA*(64/16))
#define MB_PER_DMA_P          (256*16/160) // 256 Lmem can hold MB_PER_DMA TOP Info 
#define MB_PER_DMA_COUNT_P    (MB_PER_DMA_P*(160/16))
#if 0
/*output buffer define*/
#define BitstreamStart        0x01e00000
#define BitstreamEnd          0x01e001f8  
#define BitstreamIntAddr      0x01e00010
/*input buffer define*/
#define dct_buff_start_addr   0x02000000 
#define dct_buff_end_addr     0x037ffff8

/*deblock buffer define*/
#define dblk_addr          0x1d00000
#define DBLK_CANVAS			0x000102

/*reference buffer define*/
#define enc_canvas_start 192
#define enc_canvas_add   ((192<<16)|(192<<8)|(192<<0))
#endif
/********************************************
 *  Interrupt
********************************************/
#define VB_FULL_REQ            0x01
#define VLC_REQ                0x02
#define MAIN_REQ               0x04
#define QDCT_REQ               0x08

/********************************************
 *  Regsiter
********************************************/
#define COMMON_REG_0              r0
#define COMMON_REG_1              r1

#define VB_FULL_REG_0             r2

#define PROCESS_VLC_REG           r3

#define MAIN_REG_0                r8
#define MAIN_REG_1                r9
#define MAIN_REG_2                r10
#define MAIN_REG_3                r11
#define MAIN_REG_4                r12
#define MAIN_REG_5                r13
#define MAIN_REG_6                r14
#define MAIN_REG_7                r15

#define VLC_REG_0                 r8
#define VLC_REG_1                 r9
#define VLC_REG_2                 r10
#define VLC_REG_3                 r11
#define VLC_REG_4                 r12
#define VLC_REG_5                 r13
#define VLC_REG_6                 r14
#define VLC_REG_7                 r15
#define VLC_REG_8                 r16
#define VLC_REG_9                 r17
#define VLC_REG_10                r18
#define VLC_REG_11                r19
#define VLC_REG_12                r20

#define QDCT_REG_0                r8
#define QDCT_REG_1                r9
#define QDCT_REG_2                r10
#define QDCT_REG_3                r11
#define QDCT_REG_4                r12
#define QDCT_REG_5                r13
#define QDCT_REG_6                r14
#define QDCT_REG_7                r15

#define TOP_INFO_0                r26 
#define TOP_INFO_1                r27 
#define TOP_INFO_1_NEXT           r28 
#define TOP_MV_0                  r29 
#define TOP_MV_1                  r30 
#define TOP_MV_2                  r31 
#define TOP_MV_3                  r32 
#define MAIN_LOOP_REG_0			  r33   //determine encoder stage

#define vr00                      r8
#define vr01                      r9
#define vr02                      r10
#define vr03                      r11
#define MEM_OFFSET                r12

/********************************************
 *  AV Scratch Register Re-Define
********************************************/
#define ENCODER_STATUS            HENC_SCRATCH_0
#define MEM_OFFSET_REG            HENC_SCRATCH_1
#define DEBUG_REG  				  HENC_SCRATCH_2  //0X0ac2
#define MB_COUNT				  HENC_SCRATCH_3
#define IDR_INIT_COUNT			  HENC_SCRATCH_4
#define IDR_PIC_ID      		  HENC_SCRATCH_5
#define FRAME_NUMBER			  HENC_SCRATCH_6
#define PIC_ORDER_CNT_LSB		  HENC_SCRATCH_7
#define LOG2_MAX_PIC_ORDER_CNT_LSB  HENC_SCRATCH_8
#define LOG2_MAX_FRAME_NUM  		HENC_SCRATCH_9
#define ANC0_BUFFER_ID      		HENC_SCRATCH_A
#define QPPICTURE           		HENC_SCRATCH_B


//---------------------------------------------------
// ENCODER_STATUS define
//---------------------------------------------------
#define ENCODER_IDLE              0
#define ENCODER_SEQUENCE          1
#define ENCODER_PICTURE           2
#define ENCODER_IDR               3
#define ENCODER_NON_IDR           4
#define ENCODER_MB_HEADER         5
#define ENCODER_MB_DATA           6

#define ENCODER_SEQUENCE_DONE          7
#define ENCODER_PICTURE_DONE           8
#define ENCODER_IDR_DONE               9
#define ENCODER_NON_IDR_DONE           10
#define ENCODER_MB_HEADER_DONE         11
#define ENCODER_MB_DATA_DONE           12
//---------------------------------------------------
// NAL start code define
//---------------------------------------------------
/* defines for H.264 */
#define Coded_slice_of_a_non_IDR_picture      1               
#define Coded_slice_of_an_IDR_picture         5               
#define Supplemental_enhancement_information  6
#define Sequence_parameter_set                7    
#define Picture_parameter_set                 8               

/* defines for H.264 slice_type */
#define I_Slice                               2
#define P_Slice                               5
#define B_Slice                               6

#define P_Slice_0                             0
#define B_Slice_1                             1
#define I_Slice_7                             7

#define nal_reference_idc_idr     3
#define nal_reference_idc_non_idr 2

#define SEQUENCE_NAL ((nal_reference_idc_idr<<5) | Sequence_parameter_set) 
#define PICTURE_NAL  ((nal_reference_idc_idr<<5) | Picture_parameter_set) 
#define IDR_NAL      ((nal_reference_idc_idr<<5) | Coded_slice_of_an_IDR_picture) 
#define NON_IDR_NAL  ((nal_reference_idc_non_idr<<5) | Coded_slice_of_a_non_IDR_picture) 

/********************************************
 *  Local Memory
********************************************/
//#define INTR_MSK_SAVE                  0x000
//#define QPPicture                      0x001 
//#define i_pred_mbx                     0x002
//#define i_pred_mby                     0x003
//#define log2_max_pic_order_cnt_lsb     0x004
//#define log2_max_frame_num             0x005
//#define frame_num                      0x006
//#define idr_pic_id                     0x007
//#define pic_order_cnt_lsb              0x008
//#define picture_type                   0x009
//#define top_mb_begin                   0x00a
//#define top_mb_end                     0x00b
//#define pic_width_in_mbs_minus1        0x00c
//#define pic_height_in_map_units_minus1 0x00d
//#define anc0_buffer_id                 0x00e

#define HENC_TOP_LMEM_BEGIN            0x300

/********************************************
* defines for HENC command 
********************************************/
#define HENC_SEND_MB_TYPE_COMMAND           1
#define HENC_SEND_I_PRED_MODE_COMMAND       2
#define HENC_SEND_C_I_PRED_MODE_COMMAND     3
#define HENC_SEND_CBP_COMMAND               4
#define HENC_SEND_DELTA_QP_COMMAND          5
#define HENC_SEND_COEFF_COMMAND             6
#define HENC_SEND_SKIP_COMMAND              7
#define HENC_SEND_B8_MODE_COMMAND           8
#define HENC_SEND_MVD_COMMAND               9
#define HENC_SEND_MB_DONE_COMMAND          15

/* defines for picture type*/
#define HENC_I_PICTURE      0
#define HENC_P_PICTURE      1
#define HENC_B_PICTURE      2

/********************************************
* defines for H.264 mb_type 
********************************************/
#define HENC_MB_Type_PBSKIP                      0x0
#define HENC_MB_Type_PSKIP                       0x0
#define HENC_MB_Type_BSKIP_DIRECT                0x0
#define HENC_MB_Type_P16x16                      0x1
#define HENC_MB_Type_P16x8                       0x2
#define HENC_MB_Type_P8x16                       0x3
#define HENC_MB_Type_SMB8x8                      0x4
#define HENC_MB_Type_SMB8x4                      0x5
#define HENC_MB_Type_SMB4x8                      0x6
#define HENC_MB_Type_SMB4x4                      0x7
#define HENC_MB_Type_P8x8                        0x8
#define HENC_MB_Type_I4MB                        0x9
#define HENC_MB_Type_I16MB                       0xa
#define HENC_MB_Type_IBLOCK                      0xb
#define HENC_MB_Type_SI4MB                       0xc
#define HENC_MB_Type_I8MB                        0xd
#define HENC_MB_Type_IPCM                        0xe
#define HENC_MB_Type_AUTO                        0xf

///////////////////////////////////////////////////////////////////////////
// TOP/LEFT INFO Define
///////////////////////////////////////////////////////////////////////////

// For I Slice
#define DEFAULT_INTRA_TYPE      0xffff
#define DEFAULT_CBP_BLK         0x0000
#define DEFAULT_C_NNZ           0x0000 
#define DEFAULT_Y_NNZ           0x0000 

#define DEFAULT_MVX             0x8000
#define DEFAULT_MVY             0x4000

// For I Slice
// Bit[31:16] Reserved
// Bit[15:0] IntraType 
//`define     HENC_TOP_INFO_0        8'h37 
//`define     HENC_LEFT_INFO_0       8'h38 

// For I Slice
// Bit[31:24] V_nnz
// Bit[23:16] U_nnz
// Bit[15:0]  Y_nnz 
//`define     HENC_TOP_INFO_1        8'h39 
//`define     HENC_LEFT_INFO_2       8'h3a 

///////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////

#endif
