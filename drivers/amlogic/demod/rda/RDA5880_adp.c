
#include <linux/kernel.h>
#include <linux/delay.h>

#include "RDA5880_adp.h"

static struct aml_demod_i2c *i2c_adap;

int RDA5880e_Write_single(unsigned char reg, unsigned char high, unsigned char low)
{
    int ret;
    struct i2c_msg msg;
    struct aml_demod_i2c *adap = i2c_adap;
    __u8 val[3];

    val[0] = reg;
    val[1] = high;
    val[2] = low;

    msg.addr = adap->addr;
    msg.flags = 0;
    msg.len = 3;
    msg.buf = val;
    
    ret = am_demod_i2c_xfer(adap, &msg, 1);
    
    return (!ret);
}

unsigned short RDA5880e_Read(unsigned char reg)
{
    int ret;
    struct i2c_msg msg[2];
    struct aml_demod_i2c *adap = i2c_adap;
    unsigned char buf[2] = {0,0};

    msg[0].addr = adap->addr;
    msg[0].flags = 0;
    msg[0].len = 1;
    msg[0].buf = &reg;

    msg[1].addr = adap->addr;
    msg[1].flags = I2C_M_RD;
    msg[1].len = 2;
    msg[1].buf = buf;
    
    ret = am_demod_i2c_xfer(adap, msg, 2);

    return ((buf[0]<<8) | buf[1]);

}

void MsOS_DelayTask(int delay_ms)
{
	msleep(delay_ms);
}


int set_tuner_RDA5880(struct aml_demod_sta *demod_sta, 
			     struct aml_demod_i2c *adap)
{
    unsigned long ch_freq;
    int ch_if;
    int ch_bw;

    i2c_adap = adap;

    ch_freq = demod_sta->ch_freq; // kHz
    ch_if   = demod_sta->ch_if;   // kHz 
    ch_bw   = demod_sta->ch_bw; // kHz

    printk("Set Tuner RDA5880 FREQ(%ld kHz), IF(%d), BW(%d)\n", ch_freq, ch_if, ch_bw);

    tun_rda5880e_control_if(ch_freq, ch_if, ch_bw);

    return 0;
}

int init_tuner_RDA5880(struct aml_demod_sta *demod_sta, 
		     struct aml_demod_i2c *adap)
{
    printk("Initialize Tuner RDA5880\n");

    i2c_adap = adap;

    tun_rda5880e_init();
    tun_rda5880e_gain_initial();

    return 0;
}

