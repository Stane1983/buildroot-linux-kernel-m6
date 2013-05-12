/*
 
 Driver APIs for MxL5007 Tuner
 
 Copyright, Maxlinear, Inc.
 All Rights Reserved
 
 File Name:      MxL_User_Define.c

 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//									   //
//	I2C Functions (implement by customer)				   //
//									   //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/i2c.h>

#define TUNER_IIC_ADDR   (0xC0>>1)

/******************************************************************************
**
**  Name: MxL_I2C_Write
**
**  Description:    I2C write operations
**
**  Parameters:    	
**	DeviceAddr	- MxL5007 Device address
**	pArray		- Write data array pointer
**	count		- total number of array
**
**  Returns:        0 if success
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   12-16-2007   khuang initial release.
**
******************************************************************************/
int MxL_I2C_Write(u32 I2C_adap, char* pArray, int count)
{
    int ret;
    struct i2c_msg msg;

    msg.addr = TUNER_IIC_ADDR;
    msg.flags = 0;
    msg.buf = pArray;
    msg.len = count;

	ret = i2c_transfer((struct i2c_adapter *)I2C_adap, &msg, 1);
	if(ret<0) {
		printk(" %s:tuner i2c writeReg error, errno is %d \n", __FUNCTION__, ret);
		return 1;
	}

   return 0;
}

/******************************************************************************
**
**  Name: MxL_I2C_Read
**
**  Description:    I2C read operations
**
**  Parameters:    	
**	DeviceAddr	- MxL5007 Device address
**	Addr		- register address for read
**	*Data		- data return
**
**  Returns:        0 if success
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   12-16-2007   khuang initial release.
**
******************************************************************************/
int MxL_I2C_Read(u32 I2C_adap, int Addr, char* mData)
{
    int ret;
    unsigned char data[2];
    struct i2c_msg msg[2];

    data[0] = 0xfb;
    data[1] = Addr&0xff;

    msg[0].addr = TUNER_IIC_ADDR;
    msg[0].flags = 0;
    msg[0].buf = data;
    msg[0].len = 2;


    msg[1].addr = TUNER_IIC_ADDR;
    msg[1].flags = I2C_M_RD;
    msg[1].buf = mData;
    msg[1].len = 1;

	ret = i2c_transfer((struct i2c_adapter *)I2C_adap, msg, 2);

	if(ret<0) {
		printk(" %s:tuner i2c readReg error, errno is %d \n", __FUNCTION__, ret);
		return 1;
	}

   return 0;
}

/******************************************************************************
**
**  Name: MxL_Delay
**
**  Description:    Delay function in milli-second
**
**  Parameters:    	
**	mSec		- milli-second to delay
**
**  Returns:        0
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   12-16-2007   khuang initial release.
**
******************************************************************************/
void MxL_Delay(int mSec)
{
    //mdelay(mSec);
    msleep(mSec);
}


