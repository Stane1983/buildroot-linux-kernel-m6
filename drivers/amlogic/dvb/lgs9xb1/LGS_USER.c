
#include <linux/i2c.h>
#include <linux/delay.h>
#include "lgsfrontend.h"
#include "LGS_TYPES.h"

extern int lgs9x_get_fe_config(struct  lgs9x_fe_config *cfg);

LGS_RESULT lgs_read(UINT8 secAddr, UINT8 regAddr, UINT8 *pregVal)
{
	int ret = 0;
	UINT8 rAddr = regAddr & 0xff;
	struct i2c_msg msg[2];
	struct lgs9x_fe_config cfg;

	memset(msg, 0, sizeof(msg));
	lgs9x_get_fe_config(&cfg);

	msg[0].addr = secAddr;	
	msg[0].flags = 0;
	msg[0].buf = &rAddr;
	msg[0].len = 1;

	msg[1].addr = secAddr;	
	msg[1].flags  |=  I2C_M_RD;
	msg[1].buf = pregVal;
	msg[1].len = 1;

	ret = i2c_transfer((struct i2c_adapter *)cfg.i2c_adapter, msg, 2);
	if(ret<0){
		printk(" %s: readReg error, errno is %d \n", __FUNCTION__, ret);
		return LGS_I2C_READ_ERROR;
	}else
		return LGS_NO_ERROR;
}

LGS_RESULT lgs_write(UINT8 secAddr, UINT8 regAddr, UINT8 regVal)
{
	int ret = 0;
	UINT8 buf[2];
	struct i2c_msg msg;
	struct lgs9x_fe_config cfg;

	buf[0] = regAddr & 0xff;
	buf[1] = regVal & 0xff;
	memset(&msg, 0, sizeof(msg));
	lgs9x_get_fe_config(&cfg);

	msg.addr = secAddr;	
	msg.flags = 0;
	msg.buf = buf;
	msg.len = 2;

	ret = i2c_transfer((struct i2c_adapter *)cfg.i2c_adapter, &msg, 1);
	if(ret<0){
		printk(" %s: writeReg error, errno is %d \n", __FUNCTION__, ret);
		return LGS_I2C_WRITE_ERROR;
	}else
		return LGS_NO_ERROR;
}

LGS_RESULT lgs_read_multibyte(UINT8 secAddr, UINT8 regAddr, UINT8 *pregVal, UINT32 len)
{
	int ret = 0;
	UINT8 rAddr = regAddr & 0xff;
	struct i2c_msg msg[2];
	struct lgs9x_fe_config cfg;

	memset(msg, 0, sizeof(msg));
	lgs9x_get_fe_config(&cfg);

	msg[0].addr = secAddr;	
	msg[0].flags = 0;
	msg[0].buf = &rAddr;
	msg[0].len = 1;

	msg[1].addr = secAddr;	
	msg[1].flags  |=  I2C_M_RD;
	msg[1].buf = pregVal;
	msg[1].len = len;

	ret = i2c_transfer((struct i2c_adapter *)cfg.i2c_adapter, msg, 2);
	if(ret<0){
		printk(" %s: readReg error, errno is %d \n", __FUNCTION__, ret);
		return LGS_I2C_READ_ERROR;
	}else
		return LGS_NO_ERROR;
}

LGS_RESULT lgs_write_multibyte(UINT8 secAddr, UINT8 regAddr, UINT8 *pregVal, UINT32 len)
{
	int i, ret = 0;
	UINT8 buf[len+1];
	struct i2c_msg msg;
	struct lgs9x_fe_config cfg;

	buf[0] = regAddr & 0xff;
	for (i=0; i<len; i++) buf[1+i] = pregVal[i];
	memset(&msg, 0, sizeof(msg));
	lgs9x_get_fe_config(&cfg);

	msg.addr = secAddr;	
	msg.flags = 0;
	msg.buf = buf;
	msg.len = len+1;

	ret = i2c_transfer((struct i2c_adapter *)cfg.i2c_adapter, &msg, 1);
	if(ret<0){
		printk(" %s: writeReg error, errno is %d \n", __FUNCTION__, ret);
		return LGS_I2C_WRITE_ERROR;
	}else
		return LGS_NO_ERROR;
}


void lgs_wait(UINT16 millisecond)
{
	msleep(millisecond);
}



