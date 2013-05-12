/*
 * Xuguang htm-9a-w125 Tuner Device Driver
 *
 * Author: Bobby Yang <bo.yang@amlogic.com>
 *
 *
 * Copyright (C) 2010 Amlogic Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


/* Standard Liniux Headers */
#include <linux/module.h>
#include <linux/i2c.h>

/* Amlogic Headers */
#include <linux/tvin/tvin.h>

/* Local Headers */
#include "tvin_tuner.h"

#define HTM_TUNER_I2C_NAME              "htm9aw125_tuner_i2c"
#define TUNER_DEVICE_NAME               "htm9aw125"

/* tv tuner system standard selection for Xuguang htm-9a-w125
   this value takes the low bits of control byte 2
     standard 		    BG	    DK	    I	    NTSC-M
     picture carrier	38.90	38.90	38.90	38.90
     colour		        33.4	32.4	32.9	34.4
 */
#define HTM_CB_SET_PAL_I        0xCE
#define HTM_CB_SET_PAL_BGDK     0xCE
#define HTM_CB_SET_NTSC_M       0xCE
#define HTM_TUNER_FL            0x40

#define HTM_FREQ_HIGH           426250000
#define HTM_FREQ_LOW            143250000

#define HTM_STEP_SIZE_0         50000
#define HTM_STEP_SIZE_1         31250
#define HTM_STEP_SIZE_3         62500

typedef struct tuner_htm_s {
    tuner_std_id  std_id;
	struct tuner_freq_s freq;
    struct tuner_parm_s parm;
} tuner_fq1216me_t;

static struct tuner_htm_s   tuner_htm = {
    TUNER_STD_PAL_BG | TUNER_STD_PAL_DK |
    TUNER_STD_PAL_I  |TUNER_STD_NTSC_M,      //tuner_std_id  std_id;
    {48250000, 0},                          //u32 frequency;
    {48250000, 855250000, 0, 0, 0},         //struct tuner_parm_s parm;
};


static int htm_std_setup(u8 *cb, u32 *step_size)
{
	/* tv norm specific stuff for multi-norm tuners */
    *cb = 0x0;

	if (tuner_htm.std_id & (TUNER_STD_PAL_BG|TUNER_STD_PAL_DK))
	{
		*cb |= HTM_CB_SET_PAL_BGDK;        //(bit0)os == 0, normal mode
		                                    //(bit[5:3])T == 0, normal mode
		                                    //(bit[6])CP == 0, (charge pump current)fast tuning
        *step_size = HTM_STEP_SIZE_3;      //base on the(bit[2:1] of cb) RAS & RSB, set the step size
	}
	else if (tuner_htm.std_id & TUNER_STD_PAL_I)
	{
		*cb |= HTM_CB_SET_PAL_I;           //(bit0)os == 1, switch off the pll amplifier
		                                    //(bit[5:3])T == 0, normal mode
		                                    //(bit[6])CP == 1, (charge pump current) moderate tuning with slight better residual oscillator
        *step_size = HTM_STEP_SIZE_3;  //TUNER_STEP_SIZE_0;      //base on the(bit[2:1] of cb) RAS & RSB, set the step size
	}
	else if (tuner_htm.std_id & TUNER_STD_NTSC_M)
	{
		*cb |= HTM_CB_SET_NTSC_M;           //(bit0)os == 1, switch off the pll amplifier
		                                    //(bit[5:3])T == 0, normal mode
		                                    //(bit[6])CP == 1, (charge pump current) moderate tuning with slight better residual oscillator
        *step_size = HTM_STEP_SIZE_3;      //base on the(bit[2:1] of cb) RAS & RSB, set the step size
	}

	return 0;
}

static struct i2c_client *tuner_client = NULL;


static int  htm_tuner_read(char *buf, int len)
{
    int  i2c_flag = -1;
    int i = 0;
    unsigned int i2c_try_cnt = I2C_TRY_MAX_CNT;

    struct i2c_msg msg[] = {
		{
			.addr	= tuner_client->addr,
			.flags	= I2C_M_RD,
			.len	= len,
			.buf	= buf,
        },
	};

repeat:
	i2c_flag = i2c_transfer(tuner_client->adapter, msg, 1);
    if (i2c_flag < 0) {
        pr_info("error in read htm_tuner,  %d byte(s) should be read,. \n", len);
        if (i++ < i2c_try_cnt)
            goto repeat;
        return -EIO;
    }
    else
    {
        //pr_info("htm_tuner_read is ok. \n");
        return 0;
    }
}

static int  htm_tuner_write(char *buf, int len)
{
    int  i2c_flag = -1;
    int i = 0;
    unsigned int i2c_try_cnt = I2C_TRY_MAX_CNT;

    struct i2c_msg msg[] = {
	    {
			.addr	= tuner_client->addr,
			.flags	= 0,    //|I2C_M_TEN,
			.len	= len,
			.buf	= buf,
		}

	};

repeat:
    i2c_flag = i2c_transfer(tuner_client->adapter, msg, 1);
    if (i2c_flag < 0) {
        pr_info("error in writing htm_tuner,  %d byte(s) should be writen,. \n", len);
        if (i++ < i2c_try_cnt)
            goto repeat;
        return -EIO;
    }
    else
    {
        //pr_info("htm_tuner_write is ok. \n");
        return 0;
    }
}


/* ---------------------------------------------------------------------- */
static enum tuner_signal_status_e tuner_islocked(void)
{
    unsigned char status = 0;

    htm_tuner_read(&status, 1);

    if ((status & HTM_TUNER_FL) != 0)
        return TUNER_SIGNAL_STATUS_STRONG;
    else
        return TUNER_SIGNAL_STATUS_WEAK;
}

/* ---------------------------------------------------------------------- */
int tvin_set_tuner(void)
{
	u8 buffer[5] = {0};
	u8 ab, cb, bb;
	u32 div, step_size = 0;
	unsigned int f_if;
    int  i2c_flag;

/* IFPCoff = Video Intermediate Frequency - Vif:
		940  =16*58.75  NTSC/J (Japan)
		732  =16*45.75  M/N STD
		622.4=16*38.90  B/G D/K I, L STD
		592  =16*37.00  D China
		590  =16.36.875 B Australia
		171.2=16*10.70  FM Radio (at set_radio_freq)
*/

    f_if = 38900000;

    if (tuner_htm.freq.freq < HTM_FREQ_LOW)
        bb = 0x01;          //low band
    else if (tuner_htm.freq.freq > HTM_FREQ_HIGH)
        bb = 0x08;          //high band
    else
        bb = 0x02;          //mid band

	//tuner_dbg("Freq= %d.%02d MHz, V_IF=%d.%02d MHz, "
	//	  "Offset=%d.%02d MHz, div=%0d\n",
	//	  params->frequency / 16, params->frequency % 16 * 100 / 16,
	//	  IFPCoff / 16, IFPCoff % 16 * 100 / 16,
	//	  offset / 16, offset % 16 * 100 / 16, div);

	/* tv norm specific stuff for multi-norm tuners */
	htm_std_setup(&cb, &step_size);
    if (step_size > 0)
        div = (tuner_htm.freq.freq + f_if)/step_size;  //with 62.5Khz step
    else
        div = (tuner_htm.freq.freq + f_if)/HTM_STEP_SIZE_0;  //with 50Khz step

    ab = 0x60;      //external AGC

	buffer[0] = (div >> 8) & 0xff;
	buffer[1] = div & 0xff;
	buffer[2] = cb;
	buffer[3] = bb;

    i2c_flag = htm_tuner_write(buffer, 4);

    return i2c_flag;
}

char *tvin_tuenr_get_name(void)
{
    return TUNER_DEVICE_NAME;
}

void tvin_get_tuner( struct tuner_parm_s *ptp)
{
    ptp->rangehigh  = tuner_htm.parm.rangehigh;
    ptp->rangelow   = tuner_htm.parm.rangelow;
    ptp->signal     = tuner_islocked();
}


void tvin_tuner_get_std(tuner_std_id *ptstd)
{
    *ptstd = tuner_htm.std_id;
}

void tvin_tuner_set_std(tuner_std_id ptstd)
{
    tuner_htm.std_id = ptstd;
}

void tvin_tuner_get_freq(struct tuner_freq_s *ptf)
{
    ptf->freq = tuner_htm.freq.freq;
}

void tvin_tuner_set_freq(struct tuner_freq_s tf)
{
    tuner_htm.freq.freq = tf.freq;
}

static int htm_tuner_probe(struct i2c_client *client, const struct i2c_device_id *id)
{

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        pr_info("%s: functionality check failed\n", __FUNCTION__);
        return -ENODEV;
    }
    tuner_client = client;
    printk( "tuner_client->addr = %x\n", tuner_client->addr);

	return 0;
}

static int htm_tuner_remove(struct i2c_client *client)
{
    pr_info("%s driver removed ok.\n", client->name);
	return 0;
}

static const struct i2c_device_id htm_tuner_id[] = {
	{HTM_TUNER_I2C_NAME, 0},
	{ }
};
MODULE_DEVICE_TABLE(i2c, htm_tuner_id);

static struct i2c_driver htm_tuner_driver = {
	.driver = {
        .owner  = THIS_MODULE,
		.name   = HTM_TUNER_I2C_NAME,
	},
	.probe		= htm_tuner_probe,
	.remove		= htm_tuner_remove,
	.id_table	= htm_tuner_id,
};

static int __init htm_tuner_init(void)
{
    int ret = 0;
    pr_info( "%s . \n", __FUNCTION__ );

    ret = i2c_add_driver(&htm_tuner_driver);

    if (ret < 0 || tuner_client == NULL) {
        pr_err("tuner: failed to add i2c driver. \n");
        ret = -ENOTSUPP;
    }

	return ret;
}

static void __exit htm_tuner_exit(void)
{
    pr_info( "%s . \n", __FUNCTION__ );
    i2c_del_driver(&htm_tuner_driver);
}

MODULE_AUTHOR("Bobby Yang <bo.yang@amlogic.com>");
MODULE_DESCRIPTION("Xuguang htm-9a-w125 tuner i2c device driver");
MODULE_LICENSE("GPL");

module_init(htm_tuner_init);
module_exit(htm_tuner_exit);

