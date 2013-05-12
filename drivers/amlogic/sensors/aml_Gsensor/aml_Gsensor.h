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

#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/kernel.h>

struct amlGsensor_acc{
	s16	x;
	s16	y;
	s16	z;
} ;

struct amlGsensor_ops
{
    char *chip_name ;
    unsigned int chip_id ;
    unsigned short chip_id_addr ;
    unsigned short max_delay ;
    int (*init)(struct i2c_client *client);
    int (*set_range)(struct i2c_client *client, unsigned char Range);
    int (*get_range)(struct i2c_client *client, unsigned char *Range);
    int (*set_workmode)(struct i2c_client *client, unsigned char value);
    int (*get_workmode)(struct i2c_client *client, unsigned char *value);
    int (*start_work)(struct i2c_client *client);
    int (*stop_work)(struct i2c_client *client);
    int (*read_accel_x)(struct i2c_client *client, unsigned short *value);
    int (*read_accel_y)(struct i2c_client *client, unsigned short *value);
    int (*read_accel_z)(struct i2c_client *client, unsigned short *value);
    int (*read_accel_xyz)(struct i2c_client *client, struct amlGsensor_acc *acc);
    ssize_t (*set_register)(struct i2c_client *client,const char *buf);
    ssize_t (*all_registers_show)(struct i2c_client *client, char *buf);
};


#ifdef CONFIG_AML_SENSORS_MMA845X	/* Freescale accelerometer */
struct amlGsensor_ops *mma845x_get_ops(void);
#undef get_amlGsensor_ops
#define get_amlGsensor_ops mma845x_get_ops
#endif

