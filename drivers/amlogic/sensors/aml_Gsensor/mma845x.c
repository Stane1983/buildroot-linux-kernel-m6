/*
 *  mma845x.c - Linux kernel modules for 3-Axis Orientation/Motion
 *  Detection Sensor 
 *
 *  Copyright (C) 2011-2012 Amlogic Ltd.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#include "aml_Gsensor.h"

#define DBG_LEV 1

#define LOW_G_INTERRUPT				REL_Z
#define HIGH_G_INTERRUPT 			REL_HWHEEL
#define SLOP_INTERRUPT 				REL_DIAL
#define DOUBLE_TAP_INTERRUPT 			REL_WHEEL
#define SINGLE_TAP_INTERRUPT 			REL_MISC
#define ORIENT_INTERRUPT 			ABS_PRESSURE
#define FLAT_INTERRUPT 				ABS_DISTANCE

/* register enum for mma8453 registers */
enum {
	MMA845x_STATUS = 0x00,
       MMA845x_X_AXIS_MSB_REG,
	MMA845x_X_AXIS_LSB_REG,
	MMA845x_Y_AXIS_MSB_REG,
	MMA845x_Y_AXIS_LSB_REG,
	MMA845x_Z_AXIS_MSB_REG,
	MMA845x_Z_AXIS_LSB_REG,
	
	MMA845x_MODE_CTRL_REG = 0x0B,
	MMA845x_INT_SOURCE,
	MMA845x_WHO_AM_I,
	MMA845x_RANGE_SEL_REG,
	MMA845x_HP_FILTER_CUTOFF,
	
	MMA845x_PL_STATUS,
	MMA845x_PL_CFG,
	MMA845x_PL_COUNT,
	MMA845x_PL_BF_ZCOMP,
	MMA845x_PL_P_L_THS_REG,
	
	MMA845x_FF_MT_CFG,
	MMA845x_FF_MT_SRC,
	MMA845x_FF_MT_THS,
	MMA845x_FF_MT_COUNT,

	MMA845x_TRANSIENT_CFG = 0x1D,
	MMA845x_TRANSIENT_SRC,
	MMA845x_TRANSIENT_THS,
	MMA845x_TRANSIENT_COUNT,
	
	MMA845x_PULSE_CFG,
	MMA845x_PULSE_SRC,
	MMA845x_PULSE_THSX,
	MMA845x_PULSE_THSY,
	MMA845x_PULSE_THSZ,
	MMA845x_PULSE_TMLT,
	MMA845x_PULSE_LTCY,
	MMA845x_PULSE_WIND,
	
	MMA845x_ASLP_COUNT,
	MMA845x_CTRL_REG1, /* 2A */
	MMA845x_CTRL_REG2,
	MMA845x_CTRL_REG3,
	MMA845x_CTRL_REG4,
	MMA845x_CTRL_REG5,
	
	MMA845x_OFF_X,
	MMA845x_OFF_Y,
	MMA845x_OFF_Z,
	
	MMA845x_REG_END,
};

#define MMA845x_ACC_X_LSB__POS           6
#define MMA845x_ACC_X_LSB__LEN           2
#define MMA845x_ACC_X_LSB__MSK           0xC0
#define MMA845x_ACC_X_LSB__REG           MMA845x_X_AXIS_LSB_REG

#define MMA845x_ACC_X_MSB__POS           0
#define MMA845x_ACC_X_MSB__LEN           8
#define MMA845x_ACC_X_MSB__MSK           0xFF
#define MMA845x_ACC_X_MSB__REG           MMA845x_X_AXIS_MSB_REG

#define MMA845x_ACC_Y_LSB__POS           6
#define MMA845x_ACC_Y_LSB__LEN           2
#define MMA845x_ACC_Y_LSB__MSK           0xC0
#define MMA845x_ACC_Y_LSB__REG           MMA845x_Y_AXIS_LSB_REG

#define MMA845x_ACC_Y_MSB__POS           0
#define MMA845x_ACC_Y_MSB__LEN           8
#define MMA845x_ACC_Y_MSB__MSK           0xFF
#define MMA845x_ACC_Y_MSB__REG           MMA845x_Y_AXIS_MSB_REG

#define MMA845x_ACC_Z_LSB__POS           6
#define MMA845x_ACC_Z_LSB__LEN           2
#define MMA845x_ACC_Z_LSB__MSK           0xC0
#define MMA845x_ACC_Z_LSB__REG           MMA845x_Z_AXIS_LSB_REG

#define MMA845x_ACC_Z_MSB__POS           0
#define MMA845x_ACC_Z_MSB__LEN           8
#define MMA845x_ACC_Z_MSB__MSK           0xFF
#define MMA845x_ACC_Z_MSB__REG           MMA845x_Z_AXIS_MSB_REG

#define MMA845x_RANGE_SEL__POS             0
#define MMA845x_RANGE_SEL__LEN             2
#define MMA845x_RANGE_SEL__MSK             0x03
#define MMA845x_RANGE_SEL__REG             MMA845x_RANGE_SEL_REG

#define MMA845x_SYSMODE_CTRL__POS          0
#define MMA845x_SYSMODE_CTRL__LEN          2
#define MMA845x_SYSMODE_CTRL__MSK          0x03
#define MMA845x_SYSMODE_CTRL__REG          MMA845x_MODE_CTRL_REG

#define MMA845x_EN_SLEEP_CTRL__POS          0
#define MMA845x_EN_SLEEP_CTRL__LEN          1
#define MMA845x_EN_SLEEP_CTRL__MSK          0x01
#define MMA845x_EN_SLEEP_CTRL__REG          MMA845x_CTRL_REG1

#define MMA845x_EN_FREAD_CTRL__POS          1
#define MMA845x_EN_FREAD_CTRL__LEN          1
#define MMA845x_EN_FREAD_CTRL__MSK          0x02
#define MMA845x_EN_FREAD_CTRL__REG          MMA845x_CTRL_REG1

#define MMA845x_RANGE_2G                 0
#define MMA845x_RANGE_4G                 1
#define MMA845x_RANGE_8G                 2

//#define MMA845x_MODE_STANDBY      0
#define MMA845x_MODE_WAKE    1
#define MMA845x_MODE_SLEEP     0

//#define FASTREAD 1

#define mma845x_GET_BITSLICE(regvar, bitname)\
	((regvar & bitname##__MSK) >> bitname##__POS)


#define mma845x_SET_BITSLICE(regvar, bitname, val)\
	((regvar & ~bitname##__MSK) | ((val<<bitname##__POS)&bitname##__MSK))

unsigned char ug_FastRead = 0;

unsigned char g_mode;


static int mma845x_smbus_read_byte(struct i2c_client *client,
		unsigned char reg_addr, unsigned char *data)
{
	s32 dummy;
	dummy = i2c_smbus_read_byte_data(client, reg_addr);
	if (dummy < 0)
		return -1;
	*data = dummy & 0x000000ff;

	return 0;
}

static int mma845x_smbus_write_byte(struct i2c_client *client,
		unsigned char reg_addr, unsigned char *data)
{
	s32 dummy;
	dummy = i2c_smbus_write_byte_data(client, reg_addr, *data);
	if (dummy < 0)
		return -1;
	return 0;
}

static int mma845x_smbus_read_byte_block(struct i2c_client *client,
		unsigned char reg_addr, unsigned char *data, unsigned char len)
{
	s32 dummy;
	dummy = i2c_smbus_read_i2c_block_data(client, reg_addr, len, data);
	if (dummy < 0)
		return -1;
	return 0;
}

static int mma845x_write_reg(struct i2c_client *client, unsigned char addr,
		unsigned char *data)
{
	int comres = 0;
	comres = mma845x_smbus_write_byte(client, addr, data);

	return comres;
}

static int mma845x_set_range(struct i2c_client *client, unsigned char Range)
{
	int comres = 0;
	unsigned char data1=0;


	if (Range < 4) {
		comres = mma845x_smbus_read_byte(client,
				MMA845x_RANGE_SEL_REG, &data1);
		switch (Range) {
		case MMA845x_RANGE_2G:
			data1  = mma845x_SET_BITSLICE(data1,
					MMA845x_RANGE_SEL, 0);
			break;
		case MMA845x_RANGE_4G:
			data1  = mma845x_SET_BITSLICE(data1,
					MMA845x_RANGE_SEL, 5);
			break;
		case MMA845x_RANGE_8G:
			data1  = mma845x_SET_BITSLICE(data1,
					MMA845x_RANGE_SEL, 8);
			break;
		default:
			break;
		}
		comres += mma845x_smbus_write_byte(client,
				MMA845x_RANGE_SEL_REG, &data1);
            g_mode = Range ;
	} else{
		comres = -1;
	}

	return comres;
}

static int mma845x_get_range(struct i2c_client *client, unsigned char *Range)
{
	int comres = 0;
	unsigned char data=0;


	comres = mma845x_smbus_read_byte(client, MMA845x_RANGE_SEL__REG,
			&data);
	data = mma845x_GET_BITSLICE(data, MMA845x_RANGE_SEL);
	*Range = data;

	return comres;
}



static int mma845x_set_workmode(struct i2c_client *client, unsigned char Mode)
{
    int comres = 0;
    unsigned char data1=0;

    comres=mma845x_smbus_write_byte(client,MMA845x_EN_SLEEP_CTRL__REG, &data1);
    if(comres<0){
        printk("mma845x_set_workmode, fialed to get  MMA845x_EN_SLEEP_CTRL__REG \n");
        return -1;
    }

    if (Mode < 3) {
        switch (Mode) {
            case MMA845x_MODE_WAKE:
                data1  = mma845x_SET_BITSLICE(data1,MMA845x_EN_SLEEP_CTRL, 1);
                #ifdef FASTREAD
                data1  = mma845x_SET_BITSLICE(data1,MMA845x_EN_FREAD_CTRL, 1);
                #endif
                break;
            case MMA845x_MODE_SLEEP:
                data1 = mma845x_SET_BITSLICE(data1,MMA845x_EN_SLEEP_CTRL,0);
                break;
            default:
                break;
        }
        if(DBG_LEV)
            printk("mma845x_set_workmode: %d \n", data1);
        ug_FastRead = mma845x_GET_BITSLICE(data1,MMA845x_EN_FREAD_CTRL);
        comres += mma845x_smbus_write_byte(client,MMA845x_EN_SLEEP_CTRL__REG, &data1);
    } else{
        comres = -1;
    }

    return comres;
}

static int mma845x_get_workmode(struct i2c_client *client, unsigned char *Mode)
{
	int comres = 0;


	comres = mma845x_smbus_read_byte(client,
			MMA845x_SYSMODE_CTRL__REG, Mode);
	*Mode  = mma845x_GET_BITSLICE(*Mode,MMA845x_SYSMODE_CTRL);


	return comres;
}

static int mma845x_start_work(struct i2c_client *client)
{
    mma845x_set_workmode(client, MMA845x_MODE_WAKE);
    
    return mma845x_get_workmode(client, &g_mode);
}

static int mma845x_stop_work(struct i2c_client *client)
{
    return mma845x_set_workmode(client, MMA845x_MODE_SLEEP);
}

static int mma845x_read_accel_x(struct i2c_client *client, short *a_x)
{
    int comres;
    short x ;
    unsigned char data[2];

    comres = mma845x_smbus_read_byte_block(client, MMA845x_ACC_X_LSB__REG,
    		data, 2);
    if( !ug_FastRead ){
        x = ((data[0] << 8) & 0xff00) | data[1];
    }
    else{
        x = ((data[0] << 8) & 0xff00) ;
    }

    *a_x = (short)(x) >> 6;

    return comres;
}
static int mma845x_read_accel_y(struct i2c_client *client, short *a_y)
{
    int comres;
    short y;
    unsigned char data[2];

    comres = mma845x_smbus_read_byte_block(client, MMA845x_ACC_Y_LSB__REG,
    		data, 2);
    if( !ug_FastRead ){
        y = ((data[0] << 8) & 0xff00) | data[1];
    }
    else{
        y = ((data[0] << 8) & 0xff00) ;
    }

    *a_y = (short)(y) >> 6;

    return comres;
}

static int mma845x_read_accel_z(struct i2c_client *client, short *a_z)
{
    int comres;
    short z;
    unsigned char data[2];

    comres = mma845x_smbus_read_byte_block(client, MMA845x_ACC_Z_LSB__REG,
    		data, 2);
    if( !ug_FastRead ){
        z = ((data[0] << 8) & 0xff00) | data[1];
    }
    else{
        z = ((data[0] << 8) & 0xff00) ;
    }

    *a_z= (short)(z) >> 6;

    return comres;
}


static int mma845x_read_accel_xyz(struct i2c_client *client,struct amlGsensor_acc *acc)
{
    int comres;
    char mode = 0 ;
    unsigned char data[7];

    comres = mma845x_smbus_read_byte_block(client,
    		MMA845x_STATUS, data, 7);

    if( !ug_FastRead ){
        acc->x = ((data[1] << 8) & 0xff00) | data[2];
        acc->y = ((data[3] << 8) & 0xff00) | data[4];
        acc->z = ((data[5] << 8) & 0xff00) | data[6];
    }
    else{
        acc->x = ((data[1] << 8) & 0xff00) ;
        acc->y = ((data[3] << 8) & 0xff00) ;
        acc->z = ((data[5] << 8) & 0xff00) ;
    }

    acc->x = (short)(acc->x) >> 6;
    acc->y = (short)(acc->y) >> 6;
    acc->z = (short)(acc->z) >> 6;

    if (g_mode == MMA845x_RANGE_4G){
    	(acc->x)=(acc->x)<<1;
    	(acc->y)=(acc->y)<<1;
    	(acc->z)=(acc->z)<<1;
    }
    else if (g_mode == MMA845x_RANGE_8G){
    	(acc->x)=(acc->x)<<2;
    	(acc->y)=(acc->y)<<2;
    	(acc->z)=(acc->z)<<2;
    }

    return comres;
}

static ssize_t mma845x_register_store(struct i2c_client *client,const char *buf)
{
    int address, value;
    sscanf(buf, "%x %x", &address, &value);
    printk("going to set val[0x%x] > reg[0x%x] \n",value, address);
    if (mma845x_write_reg(client, (unsigned char)address,
    		(unsigned char *)&value) < 0){
        printk("failed to set val[0x%x] > reg[0x%x] \n",value, address);
        return -EINVAL;
    }

    return 0;
}

static ssize_t mma845x_register_show(struct i2c_client *client,	char *buf)
{
    size_t count = 0;
    u8 reg[0x31];
    int i;
    
    for (i = 0 ; i < MMA845x_MODE_CTRL_REG; i++) {
        mma845x_smbus_read_byte(client, i, reg+i);

        count += sprintf(&buf[count], "0x%x: 0x%02x\n", i, reg[i]);
    }
    for (i = MMA845x_MODE_CTRL_REG ; i < MMA845x_TRANSIENT_CFG; i++) {
        mma845x_smbus_read_byte(client, i, reg+i);

        count += sprintf(&buf[count], "0x%x: 0x%02x\n", i, reg[i]);
    }
    for (i = MMA845x_TRANSIENT_CFG ; i < MMA845x_REG_END; i++) {
        mma845x_smbus_read_byte(client, i, reg+i);

        count += sprintf(&buf[count], "0x%x: 0x%02x\n", i, reg[i]);
    }
    
    return count;
}

static int mma845x_init(struct i2c_client *client)
{
    return mma845x_set_range(client, MMA845x_RANGE_2G);
}

struct amlGsensor_ops mma845x_ops =
{
    .chip_name = "mma845x" ,
    .chip_id = 0x2a ,
    .chip_id_addr = MMA845x_WHO_AM_I  ,
    .max_delay = 200 ,
    .init = mma845x_init ,
    .set_range = mma845x_set_range ,
    .get_range = mma845x_get_range ,
    .set_workmode = mma845x_set_workmode ,
    .get_workmode = mma845x_get_workmode ,
    .start_work = mma845x_start_work ,
    .stop_work = mma845x_stop_work ,
    .read_accel_x = mma845x_read_accel_x ,
    .read_accel_y = mma845x_read_accel_y ,
    .read_accel_z = mma845x_read_accel_z ,
    .read_accel_xyz = mma845x_read_accel_xyz ,
    .set_register = mma845x_register_store,
    .all_registers_show = mma845x_register_show,
};

struct amlGsensor_ops *mma845x_get_ops(void)
{
	return &mma845x_ops;
}
EXPORT_SYMBOL(mma845x_get_ops);

