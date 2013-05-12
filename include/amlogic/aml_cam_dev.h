#ifndef __AML_CAM_DEV__
#define __AML_CAM_DEV__
#include <linux/list.h>
#include <media/amlogic/aml_camera.h>

#define AML_I2C_BUS_A 0
#define AML_I2C_BUS_B 1
#define AML_I2C_BUS_AO 2

typedef enum {
#ifdef CONFIG_VIDEO_AMLOGIC_CAPTURE_GC0307
	CAM_GC0307 = 0,
#endif
#ifdef CONFIG_VIDEO_AMLOGIC_CAPTURE_GC0308
	CAM_GC0308,
#endif
#ifdef CONFIG_VIDEO_AMLOGIC_CAPTURE_GC0329
	CAM_GC0329,
#endif
#ifdef CONFIG_VIDEO_AMLOGIC_CAPTURE_GC2015
	CAM_GC2015,
#endif
#ifdef CONFIG_VIDEO_AMLOGIC_CAPTURE_GC2035
	CAM_GC2035,
#endif
#ifdef CONFIG_VIDEO_AMLOGIC_CAPTURE_GT2005
	CAM_GT2005,
#endif
#ifdef CONFIG_VIDEO_AMLOGIC_CAPTURE_OV2659
	CAM_OV2659,
#endif
#ifdef CONFIG_VIDEO_AMLOGIC_CAPTURE_OV3640
	CAM_OV3640,
#endif
#ifdef CONFIG_VIDEO_AMLOGIC_CAPTURE_OV3660
	CAM_OV3660,
#endif
#ifdef CONFIG_VIDEO_AMLOGIC_CAPTURE_OV5640
	CAM_OV5640,
#endif
#ifdef CONFIG_VIDEO_AMLOGIC_CAPTURE_OV5642
	CAM_OV5642,
#endif
#ifdef CONFIG_VIDEO_AMLOGIC_CAPTURE_OV7675
	CAM_OV7675,
#endif
#ifdef CONFIG_VIDEO_AMLOGIC_CAPTURE_SP0838
	CAM_SP0838,
#endif
#ifdef CONFIG_VIDEO_AMLOGIC_CAPTURE_HI253
	CAM_HI253,
#endif
#ifdef CONFIG_VIDEO_AMLOGIC_CAPTURE_HM5065
	CAM_HM5065,
#endif
	CAM_MAX_NUM
} aml_cam_type_t;

typedef struct {
	struct list_head list;
	aml_cam_type_t type;
	char* name;
	unsigned char i2c_bus_num;
	void* data;
}aml_cam_dev_t;

extern int reg_cam_to_i2c_bus(aml_cam_dev_t* dev);

#define CONFIG_CAM_DEV(cam, dev_name, dev_type) \
static aml_cam_dev_t __initdata cam##_dev = { \
	.type = dev_type, \
	.name = #dev_name, \
	.i2c_bus_num = AML_I2C_BUS_A, \
	.data = (void*)&video_##cam##_data, \
}; \
static int __init cam##_dev_config(void) \
{ \
	return reg_cam_to_i2c_bus(&cam##_dev); \
} \
arch_initcall(cam##_dev_config) \


#endif /* __AML_CAM_DEV__ */