/*
 * Xuguang htm-9a-w125 Demodulation Device Driver
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
#include "tvin_demod.h"

/*********************IF configure*****************************/
//// first reg (b)
#define HTM_VideoTrapBypassOFF      0x00    // bit b0
#define HTM_VideoTrapBypassON       0x01    // bit b0

#define HTM_AutoMuteFmInactive      0x00    // bit b1
#define HTM_AutoMuteFmActive        0x02    // bit b1

#define HTM_Intercarrier            0x00    // bit b2
#define HTM_QSS                     0x04    // bit b2

#define HTM_PositiveAmTV            0x10    // bit b3:4
#define HTM_NegativeFmTV            0x00    // bit b3:4

#define HTM_ForcedMuteAudioON       0x20    // bit b5
#define HTM_ForcedMuteAudioOFF      0x00    // bit b5

#define HTM_OutputPort1Active       0x00    // bit b6
#define HTM_OutputPort1Inactive     0x40    // bit b6

#define HTM_OutputPort2Active       0x00    // bit b7
#define HTM_OutputPort2Inactive     0x80    // bit b7


//// second reg (c)
#define HTM_TopMask                 0x1f    // bit c0:4
#define HTM_TopDefault		        0x10    // bit c0:4

#define HTM_DeemphasisOFF           0x00    // bit c5
#define HTM_DeemphasisON            0x20    // bit c5

#define HTM_Deemphasis75            0x00    // bit c6
#define HTM_Deemphasis50            0x40    // bit c6

#define HTM_AudioGain0              0x00    // bit c7
#define HTM_AudioGain6              0x80    // bit c7


//// third reg (e)
#define HTM_AudioIF_4_5             0x00    // bit e0:1
#define HTM_AudioIF_5_5             0x01    // bit e0:1
#define HTM_AudioIF_6_0             0x02    // bit e0:1
#define HTM_AudioIF_6_5             0x03    // bit e0:1

/* Video IF selection in TV Mode (bit B3=0) */
#define HTM_VideoIF_58_75           0x00    // bit e2:4
#define HTM_VideoIF_45_75           0x04    // bit e2:4
#define HTM_VideoIF_38_90           0x08    // bit e2:4
#define HTM_VideoIF_38_00           0x0C    // bit e2:4
#define HTM_VideoIF_33_90           0x10    // bit e2:4
#define HTM_VideoIF_33_40           0x14    // bit e2:4

#define HTM_TunerGainNormal         0x00    // bit e5
#define HTM_TunerGainLow            0x20    // bit e5

#define HTM_Gating_18               0x00    // bit e6
#define HTM_Gating_36               0x40    // bit e6

#define HTM_AgcOutON                0x80    // bit e7
#define HTM_AgcOutOFF               0x00    // bit e7


#define HTM_DEMOD_I2C_NAME         "htm9aw125_demod_i2c"


typedef struct htm_if_param_s {
	tuner_std_id      std;
	unsigned char     b;
	unsigned char     c;
	unsigned char     e;
} htm_if_param_t;

static struct htm_if_param_s htm_if_param[] = {
	{
		.std   = TUNER_STD_PAL_BG,
		.b     = (  HTM_VideoTrapBypassOFF  |
		            HTM_AutoMuteFmInactive  |
			        HTM_QSS                 |
			        HTM_PositiveAmTV        |
			        HTM_ForcedMuteAudioOFF  |
			        HTM_OutputPort1Inactive |
			        HTM_OutputPort2Inactive),
		.c     = (  HTM_TopDefault          |
                    HTM_DeemphasisON        |
                    HTM_Deemphasis50        |
                    HTM_AudioGain6),
		.e     = (  HTM_Gating_36           |
                    HTM_AudioIF_5_5         |
			        HTM_VideoIF_38_90       |
			        HTM_AgcOutON),
	},
	{
		.std   = TUNER_STD_PAL_I,
		.b     = (  HTM_VideoTrapBypassOFF  |
		            HTM_AutoMuteFmInactive  |
			        HTM_QSS                 |
			        HTM_PositiveAmTV        |
			        HTM_ForcedMuteAudioOFF  |
			        HTM_OutputPort1Inactive |
			        HTM_OutputPort2Inactive),
		.c     = (  HTM_TopDefault          |
                    HTM_DeemphasisON        |
                    HTM_Deemphasis50        |
                    HTM_AudioGain6),
		.e     = (  HTM_AudioIF_6_0         |
			        HTM_VideoIF_38_90       |
			        HTM_AgcOutON ),
	},
	{
		.std   = TUNER_STD_PAL_DK,
		.b     = (  HTM_VideoTrapBypassOFF  |
		            HTM_AutoMuteFmInactive  |
			        HTM_QSS                 |
			        HTM_PositiveAmTV        |
			        HTM_ForcedMuteAudioOFF  |
			        HTM_OutputPort1Inactive |
			        HTM_OutputPort2Inactive),
		.c     = (  HTM_TopDefault          |
                    HTM_DeemphasisON        |
                    HTM_Deemphasis50        |
                    HTM_AudioGain6),
		.e     = (  HTM_AudioIF_6_5         |
			        HTM_VideoIF_38_90       |
			        HTM_AgcOutON),
	},
	{
		.std   = TUNER_STD_NTSC_M,
		.b     = (0x14),
		.c     = (0x30),
		.e     = (0x08),
	}
};

static tuner_std_id htm_demod_std = 0;


void tvin_demod_set_std(tuner_std_id ptstd)
{
    htm_demod_std = ptstd;
}

static struct i2c_client *tuner_afc_client = NULL;

static int  htm_afc_read(char *buf, int len)
{
    int  i2c_flag = -1;
    int i = 0;
    unsigned int i2c_try_cnt = I2C_TRY_MAX_CNT;

    struct i2c_msg msg[] = {
        {
			.addr	= tuner_afc_client->addr,
			.flags	= I2C_M_RD,
			.len	= len,
			.buf	= buf,
        },
	};

repeat:
	i2c_flag = i2c_transfer(tuner_afc_client->adapter, msg, 1);
    if (i2c_flag < 0) {
        pr_err("error in read htm_afc, %d byte(s) should be read,. \n", len);
        if (i++ < i2c_try_cnt)
            goto repeat;
        return -EIO;
    }
    else
    {
        //pr_info("htm_afc_read is ok. \n");
        return 0;
    }
}

static int  htm_afc_write(char *buf, int len)
{
    int  i2c_flag = -1;
    int i = 0;
    unsigned int i2c_try_cnt = I2C_TRY_MAX_CNT;

    struct i2c_msg msg[] = {
        {
			.addr	= tuner_afc_client->addr,
			.flags	= 0,    //|I2C_M_TEN,
			.len	= len,
			.buf	= buf,
		}
	};

repeat:
    i2c_flag = i2c_transfer(tuner_afc_client->adapter, msg, 1);
    if (i2c_flag < 0) {
        pr_err("error in writing htm_afc, %d byte(s) should be writen,. \n", len);
        if (i++ < i2c_try_cnt)
            goto repeat;
        return -EIO;
    }
    else
    {
        //pr_info("htm_afc_write is ok. \n");
        return 0;
    }
}



void tvin_demod_get_afc(struct tuner_parm_s *ptp)
{
#if 0
	static int afc_bits_2_hz[] = {
		 -12500,  -37500,  -62500,  -87500,
		-112500, -137500, -162500, -187500,
		 187500,  162500,  137500,  112500,
		  87500,   62500,   37500,  12500
	};
#endif
    int i2c_flag;
    unsigned char status;

    i2c_flag = htm_afc_read(&status, 1);
    if(i2c_flag == 0)
        ptp->if_status = status;	//afc_bits_2_hz[(status >> 1) & 0x0F];
}


int tvin_set_demod(unsigned int bce)
{
	struct htm_if_param_s *param = NULL;
	unsigned char buf[4] = {0, 0, 0, 0};
	int i, i2c_flag = -1;

    if (bce != 0)
    {
    	buf[1] = (unsigned char)((bce&0xff0000) >> 16);//param->b;
    	buf[2] = (unsigned char)((bce&0x00ff00) >> 8);//param->c;
    	buf[3] = (unsigned char)((bce&0x0000ff) >> 0);//param->e;

        //pr_info( " %s: b=0x%x, c=0x%x, e=0x%x\n", __func__, buf[1], buf[2], buf[3]);

        i2c_flag = htm_afc_write(buf, 4);

        return i2c_flag;
    }
	for (i = 0; i < ARRAY_SIZE(htm_if_param); i++) {
		if (htm_if_param[i].std & htm_demod_std) {
			param = htm_if_param + i;
			break;
		}
	}

	if (NULL == param) {
		//tuner_dbg("Unsupported tvnorm entry - audio muted\n");
		/*audio muted*/
	    buf[1] = htm_if_param[0].b | HTM_ForcedMuteAudioON;
	    buf[2] = htm_if_param[0].c;
	    buf[3] = htm_if_param[0].e;

        i2c_flag = htm_afc_write(buf, 4);
		return -1;
	}

	//tuner_dbg("configure for: %s\n", norm->name);
	buf[1] = param->b;
	buf[2] = param->c;
	buf[3] = param->e;

    i2c_flag = htm_afc_write(buf, 4);

    return i2c_flag;
}

static int htm_demod_probe(struct i2c_client *client, const struct i2c_device_id *id)
{

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        pr_info("%s: functionality check failed\n", __FUNCTION__);
        return -ENODEV;
    }
    tuner_afc_client = client;

    pr_info( " %s: tuner_demod_client->addr = %x\n", __FUNCTION__, tuner_afc_client->addr);

    return 0;
}

static int htm_demod_remove(struct i2c_client *client)
{
    pr_info("%s driver removed ok.\n", client->name);
	return 0;
}

static const struct i2c_device_id htm_demod_id[] = {
    {HTM_DEMOD_I2C_NAME, 0},
	{ }
};

static struct i2c_driver htm_demod_driver = {
    .driver = {
        .owner  = THIS_MODULE,
		.name   = HTM_DEMOD_I2C_NAME,
	},
	.probe		= htm_demod_probe,
	.remove		= htm_demod_remove,
	.id_table	= htm_demod_id,
};

static int __init htm_demod_init(void)
{
    int ret = 0;
    pr_info( "%s . \n", __FUNCTION__ );

    ret = i2c_add_driver(&htm_demod_driver);

    if (ret < 0 || tuner_afc_client == NULL) {
        pr_err("tuner demod: failed to add i2c driver. \n");
        ret = -ENOTSUPP;
    }

	return ret;
}

static void __exit htm_demod_exit(void)
{
    pr_info( "%s . \n", __FUNCTION__ );

	i2c_del_driver(&htm_demod_driver);
}

MODULE_AUTHOR("Bobby Yang <bo.yang@amlogic.com>");
MODULE_DESCRIPTION("Xuguang htm-9a-w125 demodulation i2c device driver");
MODULE_LICENSE("GPL");

module_init(htm_demod_init);
module_exit(htm_demod_exit);

