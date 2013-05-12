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

#ifndef AM_MIPI_CSI2
#define AM_MIPI_CSI2

#define CSI2_BUF_POOL_SIZE    6
#define CSI2_OUTPUT_BUF_POOL_SIZE   1

#define AM_CSI2_FLAG_NULL                   0x00000000
#define AM_CSI2_FLAG_INITED               0x00000001
#define AM_CSI2_FLAG_DEV_READY        0x00000002
#define AM_CSI2_FLAG_STARTED            0x00000004

enum am_csi2_mode {
	AM_CSI2_ALL_MEM,
	AM_CSI2_VDIN,
};

struct am_csi2_pixel_fmt {
    char  *name;
    u32   fourcc;          /* v4l2 format id */
    int    depth;
};

struct am_csi2_camera_para {
    const char* name;
    unsigned output_pixel;
    unsigned output_line;
    unsigned active_pixel;
    unsigned active_line;
    unsigned frame_rate;
    unsigned ui_val; //ns
    unsigned hs_freq; //hz
    unsigned char clock_lane_mode;
    unsigned char mirror;
    unsigned zoom;
    unsigned angle;
    struct am_csi2_pixel_fmt* in_fmt;
    struct am_csi2_pixel_fmt* out_fmt;
};

struct am_csi2_client_config {
    enum am_csi2_mode mode;
    unsigned char lanes;	    /* 0..3 */
    unsigned char channel;    /* bitmask[3:0] */
    int vdin_num;
    void *pdev;	/* client platform device */
};

struct am_csi2_pdata {
    struct am_csi2_client_config *clients;
    int num_clients;
};

typedef struct am_csi2_s am_csi2_t;

typedef struct am_csi2_ops_s {
    enum am_csi2_mode mode;
    struct am_csi2_pixel_fmt* (*getPixelFormat)(u32 fourcc,bool input);
    int (*init) (am_csi2_t* dev);
    int (*streamon) (am_csi2_t* dev);
    int (*streamoff ) (am_csi2_t* dev);
    int (*fill) (am_csi2_t* dev);
    int (*uninit) (am_csi2_t* dev);
    void* privdata;
    int data_num;
} am_csi2_ops_t;

typedef struct am_csi2_frame_s {
    unsigned ddr_address;
    int index;
    unsigned status;
    unsigned w;
    unsigned h;
    int read_cnt;
    unsigned err;
}am_csi2_frame_t;

typedef struct am_csi2_output_s {
    void * vaddr;
    unsigned output_pixel;
    unsigned output_line;
    u32   fourcc;          /* v4l2 format id */
    int    depth;
    unsigned frame_size;
    unsigned char frame_available;
    unsigned zoom;
    unsigned angle;
    am_csi2_frame_t frame[CSI2_OUTPUT_BUF_POOL_SIZE];
}am_csi2_output_t;

typedef struct am_csi2_input_s {
    unsigned active_pixel;
    unsigned active_line;
    u32   fourcc;          /* v4l2 format id */
    int    depth;
    unsigned frame_size;
    unsigned char frame_available;
    am_csi2_frame_t frame[CSI2_BUF_POOL_SIZE];
}am_csi2_input_t;

typedef struct am_csi2_hw_s {
    unsigned char lanes;
    unsigned char channel;
    unsigned char mode;
    unsigned char clock_lane_mode; // 0 clock gate 1: always on
    am_csi2_frame_t* frame;
    unsigned active_pixel;
    unsigned active_line;
    unsigned frame_size;
    unsigned ui_val; //ns
    unsigned hs_freq; //hz
    unsigned urgent;
}am_csi2_hw_t;

struct am_csi2_s{
    int id;	
    struct platform_device *pdev;
    struct am_csi2_client_config *client;

    struct mutex lock;
#ifdef CONFIG_MEM_MIPI
    int irq;
#endif
    unsigned pbufAddr;
    unsigned decbuf_size;

    unsigned frame_rate;
    unsigned ui_val; //ns
    unsigned hs_freq; //hz
    unsigned char clock_lane_mode; // 0 clock gate 1: always on
    unsigned char mirror;

    unsigned status;

    am_csi2_input_t input;
    am_csi2_output_t output;
    struct am_csi2_ops_s *ops;
};

//#define MIPI_DEBUG
#ifdef MIPI_DEBUG
#define mipi_dbg(fmt, args...) printk(fmt,## args)
#else
#define mipi_dbg(fmt, args...)
#endif
#define mipi_error(fmt, args...) printk(fmt,## args)

//struct device;
//struct v4l2_device;

#define mipi_csi2_wr_reg(addr, data) *((volatile unsigned long *) (addr)) = data;

//#define MIPI_CSI2_HOST_BASE_ADDR    0xc8008000
#define MIPI_CSI2_HOST_BASE_ADDR    APB_REG_ADDR(0x8000)//0xf3008000

// MIPI-CSI2 host registers
#define MIPI_CSI2_HOST_VERSION          (MIPI_CSI2_HOST_BASE_ADDR+0x000) 
#define MIPI_CSI2_HOST_N_LANES          (MIPI_CSI2_HOST_BASE_ADDR+0x004) 
#define MIPI_CSI2_HOST_PHY_SHUTDOWNZ    (MIPI_CSI2_HOST_BASE_ADDR+0x008) 
#define MIPI_CSI2_HOST_DPHY_RSTZ        (MIPI_CSI2_HOST_BASE_ADDR+0x00c) 
#define MIPI_CSI2_HOST_CSI2_RESETN      (MIPI_CSI2_HOST_BASE_ADDR+0x010) 
#define MIPI_CSI2_HOST_PHY_STATE        (MIPI_CSI2_HOST_BASE_ADDR+0x014) 
#define MIPI_CSI2_HOST_DATA_IDS_1       (MIPI_CSI2_HOST_BASE_ADDR+0x018) 
#define MIPI_CSI2_HOST_DATA_IDS_2       (MIPI_CSI2_HOST_BASE_ADDR+0x01c) 
#define MIPI_CSI2_HOST_ERR1             (MIPI_CSI2_HOST_BASE_ADDR+0x020) 
#define MIPI_CSI2_HOST_ERR2             (MIPI_CSI2_HOST_BASE_ADDR+0x024) 
#define MIPI_CSI2_HOST_MASK1            (MIPI_CSI2_HOST_BASE_ADDR+0x028) 
#define MIPI_CSI2_HOST_MASK2            (MIPI_CSI2_HOST_BASE_ADDR+0x02c) 
#define MIPI_CSI2_HOST_PHY_TST_CTRL0    (MIPI_CSI2_HOST_BASE_ADDR+0x030) 
#define MIPI_CSI2_HOST_PHY_TST_CTRL1    (MIPI_CSI2_HOST_BASE_ADDR+0x034) 

//#define CSI2_CLK_RESET                             0x2a00
#define CSI2_CFG_CLK_AUTO_GATE_OFF     2
#define CSI2_CFG_CLK_ENABLE                    1
#define CSI2_CFG_SW_RESET                       0

//#define CSI2_GEN_CTRL0                             0x2a01
#define CSI2_CFG_CLR_WRRSP                      27
#define CSI2_CFG_DDR_EN                            26
#define CSI2_CFG_A_BRST_NUM                   20  //25:20
#define CSI2_CFG_A_ID                                14  //19:14
#define CSI2_CFG_URGENT_EN                     13
#define CSI2_CFG_DDR_ADDR_LPBK             12
#define CSI2_CFG_BUFFER_PIC_SIZE           11
#define CSI2_CFG_422TO444_MODE             10
#define CSI2_CFG_INV_FIELD                        9
#define CSI2_CFG_INTERLACE_EN                 8
#define CSI2_CFG_FORCE_LINE_COUNT         7
#define CSI2_CFG_FORCE_PIX_COUNT           6
#define CSI2_CFG_COLOR_EXPAND                 5
#define CSI2_CFG_ALL_TO_MEM                     4
#define CSI2_CFG_VIRTUAL_CHANNEL_EN     0  //3:0

//#define CSI2_FORCE_PIC_SIZE                        0x2a02
#define CSI2_CFG_LINE_COUNT                     16 //31:16
#define CSI2_CFG_PIX_COUNT                       0 //15:0

//#define CSI2_INTERRUPT_CTRL_STAT                   0x2a05
#define CSI2_CFG_VS_RISE_INTERRUPT_CLR         18 
#define CSI2_CFG_VS_FAIL_INTERRUPT_CLR         17 
#define CSI2_CFG_FIELD_DONE_INTERRUPT_CLR  16 
#define CSI2_CFG_VS_RISE_INTERRUPT                 2 
#define CSI2_CFG_VS_FAIL_INTERRUPT                 1 
#define CSI2_CFG_FIELD_DONE_INTERRUPT          0 

extern int start_mipi_csi2_service(struct am_csi2_camera_para *para);
extern int stop_mipi_csi2_service(struct am_csi2_camera_para *para);
extern void am_mipi_csi2_init(am_csi2_hw_t* info);
extern void am_mipi_csi2_uninit(void);
extern void init_am_mipi_csi2_clock(void);

#endif
