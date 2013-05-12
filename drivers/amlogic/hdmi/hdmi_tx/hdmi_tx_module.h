#ifndef _HDMI_TX_MODULE_H
#define _HDMI_TX_MODULE_H
#include "hdmi_info_global.h"
#include <plat/hdmi_config.h>

/*****************************
*    hdmitx attr management 
******************************/

/************************************
*    hdmitx device structure
*************************************/
#define VIC_MAX_NUM 60
#define AUD_MAX_NUM 60
typedef struct
{
    unsigned char audio_format_code;
    unsigned char channel_num_max;
    unsigned char freq_cc;        
    unsigned char cc3;
} rx_audio_cap_t;

typedef struct rx_cap_
{
    unsigned char native_Mode;
    /*video*/
    unsigned char VIC[VIC_MAX_NUM];
    unsigned char VIC_count;
    unsigned char native_VIC;
    /*audio*/
    rx_audio_cap_t RxAudioCap[AUD_MAX_NUM];
    unsigned char AUD_count;
    unsigned char RxSpeakerAllocation;
    /*vendor*/    
    unsigned int IEEEOUI;
    unsigned char ReceiverBrandName[4];
    unsigned char ReceiverProductName[16];
    unsigned int ColorDeepSupport;
    unsigned int Max_TMDS_Clock; 
    unsigned int Video_Latency;
    unsigned int Audio_Latency;
    unsigned int Interlaced_Video_Latency;
    unsigned int Interlaced_Audio_Latency;
    unsigned int threeD_present;
    unsigned int threeD_Multi_present;
    unsigned int HDMI_VIC_LEN;
    unsigned int HDMI_3D_LEN;
    unsigned int threeD_Structure_ALL_15_0;
    unsigned int threeD_MASK_15_0;
    struct {
        unsigned char frame_packing;
        unsigned char top_and_bottom;
        unsigned char side_by_side;
    } support_3d_format[VIC_MAX_NUM];
}rx_cap_t;


#define EDID_MAX_BLOCK  20       //4
#define HDMI_TMP_BUF_SIZE            1024
typedef struct hdmi_tx_dev_s {
#ifdef AVOS
	  INT16U             task_id;
	  OS_STK             * taskstack;
#else
    struct cdev cdev;             /* The cdev structure */
    struct proc_dir_entry *proc_file;
    struct task_struct *task;
    struct task_struct *task_monitor;
#endif
    struct {
        void (*SetPacket)(int type, unsigned char* DB, unsigned char* HB);
        void (*SetAudioInfoFrame)(unsigned char* AUD_DB, unsigned char* CHAN_STAT_BUF);
        unsigned char (*GetEDIDData)(struct hdmi_tx_dev_s* hdmitx_device);
        int (*SetDispMode)(struct hdmi_tx_dev_s* hdmitx_device, Hdmi_tx_video_para_t *param);
        int (*SetAudMode)(struct hdmi_tx_dev_s* hdmitx_device, Hdmi_tx_audio_para_t* audio_param);
        void (*SetupIRQ)(struct hdmi_tx_dev_s* hdmitx_device);
        void (*DebugFun)(struct hdmi_tx_dev_s* hdmitx_device, const char * buf);
        void (*UnInit)(struct hdmi_tx_dev_s* hdmitx_device);
        int (*Cntl)(struct hdmi_tx_dev_s* hdmitx_device, int cmd, unsigned arg);
    }HWOp;

    struct hdmi_phy_set_data *brd_phy_data;
    
    //wait_queue_head_t   wait_queue;            /* wait queues */
    /*EDID*/
    unsigned cur_edid_block;
    unsigned cur_phy_block_ptr;
    unsigned char EDID_buf[EDID_MAX_BLOCK*128];
    unsigned char EDID_hash[20];
    rx_cap_t RXCap;
    int vic_count;
    /*audio*/
    Hdmi_tx_audio_para_t cur_audio_param;
    int audio_param_update_flag;
    /*status*/
#define DISP_SWITCH_FORCE       0
#define DISP_SWITCH_EDID        1    
    unsigned char disp_switch_config; /* 0, force; 1,edid */
    unsigned char cur_VIC;
    unsigned char unplug_powerdown;
    /**/
    unsigned char hpd_event; /* 1, plugin; 2, plugout */
    unsigned char hpd_state; /* 1, connect; 0, disconnect */
    unsigned char force_audio_flag;
    unsigned char mux_hpd_if_pin_high_flag; 
	unsigned char cec_func_flag;
    int  auth_process_timer;
    HDMI_TX_INFO_t hdmi_info;
    unsigned char tmp_buf[HDMI_TMP_BUF_SIZE];
    unsigned int  log;
    unsigned int  internal_mode_change;
}hdmitx_dev_t;

// HDMI LOG
#define HDMI_LOG_HDCP           (1 << 0)

#define HDMI_SOURCE_DESCRIPTION 0
#define HDMI_PACKET_VEND        1
#define HDMI_MPEG_SOURCE_INFO   2
#define HDMI_PACKET_AVI         3
#define HDMI_AUDIO_INFO         4
#define HDMI_AUDIO_CONTENT_PROTECTION   5
#define HDMI_PACKET_HBR         6

#ifdef AVOS
#define HDMI_PROCESS_DELAY  AVTimeDly(500)
#define AUTH_PROCESS_TIME   (4000/500)
#else
#define HDMI_PROCESS_DELAY  msleep(10)
#define AUTH_PROCESS_TIME   (1000/100)       // reduce a little time, previous setting is 4000/10
#endif        


#define HDMITX_VER "2013Mar4a"

/************************************
*    hdmitx protocol level interface
*************************************/
extern void hdmitx_init_parameters(HDMI_TX_INFO_t *info);

extern int hdmitx_edid_parse(hdmitx_dev_t* hdmitx_device);

HDMI_Video_Codes_t hdmitx_edid_get_VIC(hdmitx_dev_t* hdmitx_device,const char* disp_mode, char force_flag);

HDMI_Video_Codes_t hdmitx_get_VIC(hdmitx_dev_t* hdmitx_device,const char* disp_mode);

extern int hdmitx_edid_dump(hdmitx_dev_t* hdmitx_device, char* buffer, int buffer_len);

extern void hdmitx_edid_clear(hdmitx_dev_t* hdmitx_device);

extern char* hdmitx_edid_get_native_VIC(hdmitx_dev_t* hdmitx_device);

extern int hdmitx_set_display(hdmitx_dev_t* hdmitx_device, HDMI_Video_Codes_t VideoCode);

extern int hdmi_set_3d(hdmitx_dev_t* hdmitx_device, int type, unsigned int param);

extern int hdmitx_set_audio(hdmitx_dev_t* hdmitx_device, Hdmi_tx_audio_para_t* audio_param);

extern int hdmi_print(int printk_flag, const char *fmt, ...);

extern  int hdmi_print_buf(char* buf, int len);

extern void hdmi_set_audio_para(int para);

extern void hdmitx_output_rgb(void);

extern int get_cur_vout_index(void);

/************************************
*    hdmitx hardware level interface
*************************************/
//#define DOUBLE_CLK_720P_1080I
extern unsigned char hdmi_pll_mode; /* 1, use external clk as hdmi pll source */

extern void HDMITX_M1A_Init(hdmitx_dev_t* hdmitx_device);

extern void HDMITX_M1B_Init(hdmitx_dev_t* hdmitx_device);

extern unsigned char hdmi_audio_off_flag;

#define HDMITX_HWCMD_POWERMODE_SWITCH    0x1
#define HDMITX_HWCMD_VDAC_OFF           0x2
#define HDMITX_HWCMD_MUX_HPD_IF_PIN_HIGH       0x3
#define HDMITX_HWCMD_TURNOFF_HDMIHW           0x4
#define HDMITX_HWCMD_MUX_HPD                0x5
#define HDMITX_HWCMD_PLL_MODE                0x6
#define HDMITX_HWCMD_TURN_ON_PRBS           0x7
#define HDMITX_FORCE_480P_CLK                0x8
#define HDMITX_OUTPUT_ENABLE                 0x9
    #define HDMITX_SET_AVMUTE                0x0
    #define HDMITX_CLEAR_AVMUTE              0x1
#define HDMITX_GET_AUTHENTICATE_STATE        0xa
#define HDMITX_SW_INTERNAL_HPD_TRIG          0xb
#define HDMITX_HWCMD_OSD_ENABLE              0xf
#define HDMITX_HDCP_CNTL                     0x10
    #define HDCP_OFF    0x0
    #define HDCP_ON     0x1
    #define IS_HDCP_ON  0x2
#define HDMITX_HDCP_MONITOR                  0x11
#define HDMITX_IP_INTR_MASN_RST              0x12
#define HDMITX_HWCMD_HPD_RESET               0X13
#define HDMITX_EARLY_SUSPEND_RESUME_CNTL     0x14
    #define HDMITX_EARLY_SUSPEND             0x1
    #define HDMITX_LATE_RESUME               0x2
#define HDMITX_IP_SW_RST                     0x15   // Refer to HDMI_OTHER_CTRL0 in hdmi_tx_reg.h
    #define TX_CREG_SW_RST      (1<<5)
    #define TX_SYS_SW_RST       (1<<4)
    #define CEC_CREG_SW_RST     (1<<3)
    #define CEC_SYS_SW_RST      (1<<2)
#define HDMITX_TMDS_PHY_CNTL                 0x16   // Refer to HDMI_OTHER_CTRL0 in hdmi_tx_reg.h
    #define PHY_OFF             0
    #define PHY_ON              1
#define HDMITX_AUDIO_CNTL                    0x17
    #define AUDIO_OFF           0
    #define AUDIO_ON            1
#define HMDITX_PHY_SUSPEND                   0x18
#define HDMITX_AVMUTE_CNTL                   0x19
    #define AVMUTE_SET          0   // set AVMUTE to 1
    #define AVMUTE_CLEAR        1   // set AVunMUTE to 1
    #define AVMUTE_OFF          2   // set both AVMUTE and AVunMUTE to 0
#define HDMITX_CBUS_RST                      0x1A
#define HDMITX_INTR_MASKN_CNTL               0x1B
    #define INTR_MASKN_ENABLE   0
    #define INTR_MASKN_DISABLE  1

#define HDMI_HDCP_DELAYTIME_AFTER_DISPLAY    20      // unit: ms

#define HDMITX_HDCP_MONITOR_BUF_SIZE         1024
typedef struct {
    char *hdcp_sub_name;
    unsigned hdcp_sub_addr_start;
    unsigned hdcp_sub_len;
}hdcp_sub_t;

#endif

