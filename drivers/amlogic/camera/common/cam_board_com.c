/*******************************************************************
 *
 *  Copyright C 2010 by Amlogic, Inc. All Rights Reserved.
 *
 *  Description:
 *
 *  Author: Amlogic Software
 *  Created: 2013/1/31   18:20
 *
 *******************************************************************/
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/kernel.h>

#include <amlogic/aml_cam_dev.h>


static int __init aml_camera_read_buff(struct i2c_adapter *adapter, 
		unsigned short dev_addr, char *buf, int addr_len, int data_len)
{
	int  i2c_flag = -1;
	struct i2c_msg msgs[] = {
		{
			.addr	= dev_addr,
			.flags	= 0,
			.len	= addr_len,
			.buf	= buf,
		},{
			.addr	= dev_addr,
			.flags	= I2C_M_RD,
			.len	= data_len,
			.buf	= buf,
		}
	};

	i2c_flag = i2c_transfer(adapter, msgs, 2);

	return i2c_flag;
}

static int __init aml_i2c_get_byte(struct i2c_adapter *adapter, 
		unsigned short dev_addr, unsigned short addr)
{
	unsigned char buff[4];
	buff[0] = (unsigned char)((addr >> 8) & 0xff);
	buff[1] = (unsigned char)(addr & 0xff);
       
	if (aml_camera_read_buff(adapter, dev_addr, buff, 2, 1) <0)
		return -1;
	return buff[0];
}

static int aml_i2c_get_byte_add8(struct i2c_adapter *adapter, 
		unsigned short dev_addr, unsigned short addr)
{
	unsigned char buff[4];
	buff[0] = (unsigned char)(addr & 0xff);
       
	if (aml_camera_read_buff(adapter, dev_addr, buff, 1, 1) <0)
		return -1;
	return buff[0];
}

extern struct i2c_client *
i2c_new_existing_device(struct i2c_adapter *adap, 
			struct i2c_board_info const *info);


#ifdef CONFIG_VIDEO_AMLOGIC_CAPTURE_GC0307
int __init gc0307_v4l2_probe(struct i2c_adapter *adapter)
{
	int ret = 0;
	unsigned char reg;  
	reg = aml_i2c_get_byte_add8(adapter, 0x21, 0x00);
	if (reg == 0x99)
		ret = 1;
	return ret;
}
#endif

#ifdef CONFIG_VIDEO_AMLOGIC_CAPTURE_GC0308
int __init gc0308_v4l2_probe(struct i2c_adapter *adapter)
{
	int ret = 0;
	unsigned char reg;   
	reg = aml_i2c_get_byte_add8(adapter, 0x21, 0x00);
	if (reg == 0x9b)
		ret = 1;
	return ret;
}
#endif

#ifdef CONFIG_VIDEO_AMLOGIC_CAPTURE_GC0329
int __init gc0329_v4l2_probe(struct i2c_adapter *adapter)
{
	int ret = 0;
	unsigned char reg;  
	reg = aml_i2c_get_byte_add8(adapter, 0x31, 0x00);
	if (reg == 0xc0)
		ret = 1;
	return ret;
}
#endif

#ifdef CONFIG_VIDEO_AMLOGIC_CAPTURE_GC2015
int __init gc2015_v4l2_probe(struct i2c_adapter *adapter)
{
	int ret = 0;
	unsigned char reg[2];  
	reg[0] = aml_i2c_get_byte_add8(adapter, 0x30, 0x00);
	reg[1] = aml_i2c_get_byte_add8(adapter, 0x30, 0x01);
	if (reg[0] == 0x20 && reg[1] == 0x05)
		ret = 1;
	return ret;
}
#endif

#ifdef CONFIG_VIDEO_AMLOGIC_CAPTURE_GC2035
int __init gc2035_v4l2_probe(struct i2c_adapter *adapter)
{
	int ret = 0;
	unsigned char reg[2];  
	reg[0] = aml_i2c_get_byte_add8(adapter, 0x3c, 0xf0);
	reg[1] = aml_i2c_get_byte_add8(adapter, 0x3c, 0xf1);
	if (reg[0] == 0x20 && reg[1] == 0x35)
		ret = 1;
	return ret;
}
#endif

#ifdef CONFIG_VIDEO_AMLOGIC_CAPTURE_GT2005
int __init gt2005_v4l2_probe(struct i2c_adapter *adapter)
{
	int ret = 0;
	unsigned char reg[2];
	reg[0] = aml_i2c_get_byte(adapter, 0x3c, 0x0000);
	reg[1] = aml_i2c_get_byte(adapter, 0x3c, 0x0001);
	if (reg[0] == 0x51 && reg[1] == 0x38)
		ret = 1;
	return ret;
}
#endif

#ifdef CONFIG_VIDEO_AMLOGIC_CAPTURE_OV2659
int __init ov2659_v4l2_probe(struct i2c_adapter *adapter)
{
	int ret = 0;
	unsigned char reg[2];   
	reg[0] = aml_i2c_get_byte(adapter, 0x30, 0x300a);
	reg[1] = aml_i2c_get_byte(adapter, 0x30, 0x300b);
	if (reg[0] == 0x26 && reg[1] == 0x56)
		ret = 1;
	return ret;
}
#endif

#ifdef CONFIG_VIDEO_AMLOGIC_CAPTURE_OV3640
int __init ov3640_v4l2_probe(struct i2c_adapter *adapter)
{
	int ret = 0;
	unsigned char reg[2];  
	reg[0] = aml_i2c_get_byte(adapter, 0x3c, 0x300a);
	reg[1] = aml_i2c_get_byte(adapter, 0x3c, 0x300b);
	if (reg[0] == 0x36 && reg[1] == 0x4c)
		ret = 1;
	return ret;
}
#endif

#ifdef CONFIG_VIDEO_AMLOGIC_CAPTURE_OV3660
int __init ov3660_v4l2_probe(struct i2c_adapter *adapter)
{
	int ret = 0;
	unsigned char reg[2];  
	reg[0] = aml_i2c_get_byte(adapter, 0x3c, 0x300a);
	reg[1] = aml_i2c_get_byte(adapter, 0x3c, 0x300b);
	if (reg[0] == 0x36 && reg[1] == 0x60)
		ret = 1;
	return ret;
}
#endif

#ifdef CONFIG_VIDEO_AMLOGIC_CAPTURE_OV5640
int __init ov5640_v4l2_probe(struct i2c_adapter *adapter)
{
	int ret = 0;
	unsigned char reg[2];   
	reg[0] = aml_i2c_get_byte(adapter, 0x3c, 0x300a);
	reg[1] = aml_i2c_get_byte(adapter, 0x3c, 0x300b);
	if (reg[0] == 0x56 && reg[1] == 0x40)
		ret = 1;
	return ret;
}
#endif

#ifdef CONFIG_VIDEO_AMLOGIC_CAPTURE_OV5642
int __init ov5642_v4l2_probe(struct i2c_adapter *adapter)
{
	int ret = 0;
	unsigned char reg[2];  
	reg[0] = aml_i2c_get_byte(adapter, 0x3c, 0x300a);
	reg[1] = aml_i2c_get_byte(adapter, 0x3c, 0x300b);
	if (reg[0] == 0x56 && reg[1] == 0x42)
		ret = 1;
	return ret;
}
#endif

#ifdef CONFIG_VIDEO_AMLOGIC_CAPTURE_OV7675
int __init ov7675_v4l2_probe(struct i2c_adapter *adapter)
{
	int ret = 0;
	unsigned char reg[2];   
	reg[0] = aml_i2c_get_byte_add8(adapter, 0x21, 0x0a);
	reg[1] = aml_i2c_get_byte_add8(adapter, 0x21, 0x0b);
	if (reg[0] == 0x76 && reg[1] == 0x73)
		ret = 1;
	return ret;
}
#endif

#ifdef CONFIG_VIDEO_AMLOGIC_CAPTURE_SP0838
int __init sp0838_v4l2_probe(struct i2c_adapter *adapter)
{
	int ret = 0;
	unsigned char reg;    
	reg = aml_i2c_get_byte_add8(adapter, 0x30, 0x02);
	if (reg == 0x27)
		ret = 1;
	return ret;
}
#endif

#ifdef CONFIG_VIDEO_AMLOGIC_CAPTURE_HI253
int __init hi253_v4l2_probe(struct i2c_adapter *adapter)
{
	int ret = 0;
	unsigned char reg;   
	reg = aml_i2c_get_byte_add8(adapter, 0x20, 0x04);
	if (reg == 0x92)
		ret = 1;
	return ret;
}
#endif

#ifdef CONFIG_VIDEO_AMLOGIC_CAPTURE_HM5065
int __init hm5065_v4l2_probe(struct i2c_adapter *adapter)
{
	int ret = 0;
	unsigned char reg[2];   
	reg[0] = aml_i2c_get_byte(adapter, 0x16, 0x0000);
	reg[1] = aml_i2c_get_byte(adapter, 0x16, 0x0001);
	if (reg[0] == 0x03 && reg[1] == 0x9e)
		ret = 1;
	return ret;
}
#endif

typedef int(*aml_cam_probe_fun_t)(struct i2c_adapter *);

typedef struct {
	unsigned char addr;
	aml_cam_probe_fun_t probe_func;
}aml_cam_dev_info_t;

static aml_cam_dev_info_t __initdata cam_devs[CAM_MAX_NUM] = {
#ifdef CONFIG_VIDEO_AMLOGIC_CAPTURE_GC0307
	{
		.addr = 0x21,
		.probe_func = gc0307_v4l2_probe,
	},
#endif
#ifdef CONFIG_VIDEO_AMLOGIC_CAPTURE_GC0308
	{
		.addr = 0x21,
		.probe_func = gc0308_v4l2_probe,
	},
#endif
#ifdef CONFIG_VIDEO_AMLOGIC_CAPTURE_GC0329
	{
		.addr = 0x31,
		.probe_func = gc0329_v4l2_probe,
	},
#endif
#ifdef CONFIG_VIDEO_AMLOGIC_CAPTURE_GC2015
	{
		.addr = 0x30,
		.probe_func = gc2015_v4l2_probe,
	},
#endif
#ifdef CONFIG_VIDEO_AMLOGIC_CAPTURE_GC2035
	{
		.addr = 0x3c,
		.probe_func = gc2035_v4l2_probe,
	},
#endif
#ifdef CONFIG_VIDEO_AMLOGIC_CAPTURE_GT2005
	{
		.addr = 0x3c,
		.probe_func = gt2005_v4l2_probe,
	},
#endif
#ifdef CONFIG_VIDEO_AMLOGIC_CAPTURE_OV2659
	{
		.addr = 0x30,
		.probe_func = ov2659_v4l2_probe,
	},
#endif
#ifdef CONFIG_VIDEO_AMLOGIC_CAPTURE_OV3640
	{
		.addr = 0x3c,
		.probe_func = ov3640_v4l2_probe,
	},
#endif
#ifdef CONFIG_VIDEO_AMLOGIC_CAPTURE_OV3660
	{
		.addr = 0x3c,
		.probe_func = ov3660_v4l2_probe,
	},
#endif
#ifdef CONFIG_VIDEO_AMLOGIC_CAPTURE_OV5640
	{
		.addr = 0x3c,
		.probe_func = ov5640_v4l2_probe,
	},
#endif
#ifdef CONFIG_VIDEO_AMLOGIC_CAPTURE_OV5642
	{
		.addr = 0x3c,
		.probe_func = ov5642_v4l2_probe,
	},
#endif
#ifdef CONFIG_VIDEO_AMLOGIC_CAPTURE_OV7675
	{
		.addr = 0x21,
		.probe_func = ov7675_v4l2_probe,
	},
#endif
#ifdef CONFIG_VIDEO_AMLOGIC_CAPTURE_SP0838
	{
		.addr = 0x30,
		.probe_func = sp0838_v4l2_probe,
	},
#endif
#ifdef CONFIG_VIDEO_AMLOGIC_CAPTURE_HI253
	{
		.addr = 0x20,
		.probe_func = hi253_v4l2_probe,
	},
#endif
#ifdef CONFIG_VIDEO_AMLOGIC_CAPTURE_HM5065
	{
		.addr = 0x16,
		.probe_func = hm5065_v4l2_probe,
	},
#endif
};


static struct list_head __initdata cam_head = LIST_HEAD_INIT(cam_head);

int __init reg_cam_to_i2c_bus(aml_cam_dev_t* dev)
{
	list_add(&dev->list, &cam_head);
	return 0;
}

static int __init aml_camera_dynamical_init(void)
{
	aml_cam_dev_t* dev_entry;
	struct i2c_board_info board_info;
	struct i2c_adapter *adapter;
	unsigned int type_index;
	aml_plat_cam_data_t* cur_plat_cam_data = NULL;
	list_for_each_entry(dev_entry, &cam_head, list) {
		adapter = i2c_get_adapter(dev_entry->i2c_bus_num);
		type_index = (unsigned int)dev_entry->type;
		cur_plat_cam_data = (aml_plat_cam_data_t*)dev_entry->data;
		if (cur_plat_cam_data && adapter && type_index >= 0
				&& type_index < CAM_MAX_NUM
				&& cur_plat_cam_data->device_init
				&& cur_plat_cam_data->device_uninit) {
			cur_plat_cam_data->device_init();
			if (cam_devs[type_index].probe_func(adapter) == 1) {
				cur_plat_cam_data->device_uninit();
				memset(&board_info, 0, sizeof(board_info));
				strncpy(board_info.type, dev_entry->name, 
					I2C_NAME_SIZE);
				board_info.addr = cam_devs[type_index].addr;
				board_info.platform_data = dev_entry->data;
				i2c_new_existing_device(adapter, &board_info);
			} else
				cur_plat_cam_data->device_uninit();
		}
	}	
	return 0;
}
late_initcall(aml_camera_dynamical_init);


