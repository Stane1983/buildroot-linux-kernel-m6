/*                                                                                                                                  
*                                                                                                                   
* Author: kele bai <kele.bai@amlogic.com>                                                                                     
*                                                                                                                                  
* Copyright (C) 2010 Amlogic Inc.                                                                                     
 *                                                                                                                                
* This program is free software; you can redistribute it and/or modify                                                    
* it under the terms of the GNU General Public License version 2 as                                                               
 * published by the Free Software Foundation.                                                                                    
*/  
#include <linux/delay.h>
#include <linux/i2c.h>
#include "ctc703_func.h"
#define CTC703_MAX_STANDARD_NUM     7
//only for mono
static struct i2c_client  *ctc703_client = NULL;
const static struct ctc703_standard_s tv_standard[CTC703_MAX_STANDARD_NUM]={
{V4L2_STD_PAL_M         , 0x0478, 0x8020 },
{V4L2_STD_PAL_N         , 0x0478, 0x8020 },
{V4L2_STD_NTSC_M      , 0x0478, 0x8020 },
{V4L2_STD_PAL_B          , 0x0878, 0x8045 },
{V4L2_STD_PAL_I           , 0x0e78, 0x8009 },
{V4L2_STD_PAL_DK       , 0x1478, 0x8009 },
{ V4L2_STD_SECAM_DK , 0x1478, 0x8009 }
};
void xc_set_i2c_client(struct i2c_client *client)
{
    ctc703_client = client;
}
int xc_send_i2c_data(unsigned char *bytes_to_send, int nb_bytes_to_send)
{
    int i2c_flag,i;
    i2c_flag=0;
    i=0;
    if(!ctc703_client)
    {
        printk("[ctc703..]%s i2c client hasn't init.\n",__func__);
        return XC_RESULT_I2C_WRITE_FAILURE;
    }
            struct i2c_msg msg[] = {
	        {
			.addr	= ctc703_client->addr,
			.flags	= 0,    //|I2C_M_TEN,
			.len	        = nb_bytes_to_send,
			.buf	        = bytes_to_send,
	        }

	    };

repeat:
    i2c_flag = i2c_transfer(ctc703_client->adapter, msg, 1);
    if (i2c_flag < 0) 
    {
        printk("%s: error in write addr 0x%x %d,byte(s) should be read,. \n", __func__, ctc703_client->addr,nb_bytes_to_send);
        if (i++ < I2C_TRY_MAX_CNT) {
            printk("%s: error in wirte addr 0x%x, try again!!!\n", __func__,ctc703_client->addr);
            goto repeat;
        }
        else
            return XC_RESULT_I2C_WRITE_FAILURE;
    }
    else 
    {
        //pr_info("%s: write %d bytes\n", __func__, inbbytes);
        return XC_RESULT_SUCCESS;
    }
}

int xc_read_i2c_data(void *bytes_received, int nb_bytes_to_receive)
{
     int i2c_flag = 0, i=0;
   if(!ctc703_client)
    {
        printk("[ctc703..]%s i2c client hasn't init.\n",__func__);
        return XC_RESULT_I2C_WRITE_FAILURE;
    }
    struct i2c_msg msg[] = {
	    {
			.addr	= ctc703_client->addr|0x1,
			.flags	= I2C_M_RD,
			.len	        = nb_bytes_to_receive,
			.buf	        = bytes_received,
	    }

	};
repeat:
    i2c_flag = i2c_transfer(ctc703_client->adapter, msg, 1);
    if (i2c_flag < 0) 
    {
        printk("%s: error in write addr 0x%x byte(s) should be read,. \n", __func__, ctc703_client->addr|0x1,nb_bytes_to_receive);
        if (i++ < I2C_TRY_MAX_CNT) {
            printk("%s: error in wirte addr 0x%x, try again!!!\n", __func__,ctc703_client->addr|0x1);
            goto repeat;
        }
        else
            return XC_RESULT_I2C_READ_FAILURE;
    }
    else 
    {
        //pr_info("%s: write %d bytes\n", __func__, inbbytes);
        return XC_RESULT_SUCCESS;
    }
}

void xc_wait(int wait_ms)
{
  mdelay(wait_ms);
}

int xc_write_reg(unsigned short int regAddr, unsigned short int i2cData)
{
  unsigned char buf[4];
  int WatchDogTimer = 5;
  int result;
  buf[0] = (regAddr >> 8) & 0xFF;
  buf[1] = regAddr & 0xFF;
  buf[2] = (i2cData >> 8) & 0xFF;
  buf[3] = i2cData & 0xFF;
  result = xc_send_i2c_data(buf, 4);
  if ( result == XC_RESULT_SUCCESS)
  {
    // wait for busy flag to clear
    while ((WatchDogTimer > 0) && (result == XC_RESULT_SUCCESS))
    {
      buf[0] = 0;
      buf[1] = XREG_BUSY;
      result = xc_send_i2c_data(buf, 2);
      if (result == XC_RESULT_SUCCESS)
      {
        result = xc_read_i2c_data(buf, 2);
        if (result == XC_RESULT_SUCCESS)
        {
          if ((buf[0] == 0) && (buf[1] == 0))
          {
            // busy flag cleared
            break;
          }
          else
          {
            xc_wait(100);     // wait 5 ms
            WatchDogTimer--;
          }
        }
      }
    }
  }
  if (WatchDogTimer < 0)
  {
    result = XC_RESULT_I2C_WRITE_FAILURE;
  }
  return result;
}

int xc_read_reg(unsigned short regAddr, unsigned short int *i2cData)
{
  unsigned char buf[2];
  int result;

  buf[0] = (regAddr >> 8) & 0xFF;
  buf[1] = regAddr & 0xFF;
  result = xc_send_i2c_data(buf, 2);
  if (result!=XC_RESULT_SUCCESS)
    return result;

  result = xc_read_i2c_data(buf, 2);
  if (result!=XC_RESULT_SUCCESS)
    return result;

  *i2cData = buf[0] * 256 + buf[1];
  return result;
}

int xc_load_i2c_sequence()
{
  int i,nbytes_to_send,result;
  unsigned int length, pos, index;
  unsigned char buf[XC_MAX_I2C_WRITE_LENGTH];

  index=0;
  while ((xc7000_fw[index]!=0xFF) || (xc7000_fw[index+1]!=0xFF))
  {
    length = (xc7000_fw[index]<<8) + (xc7000_fw[index+1]);
    if (length==0x0000)
    {
      //this is in fact a RESET command
      result = xc_reset();
      index += 2;
      if (result!=XC_RESULT_SUCCESS)
        return result;
    }
    else if (length & 0x8000)
    {
      //this is in fact a WAIT command
      xc_wait(length & 0x7FFF);
      index += 2;
    }
    else
    {
      //send i2c data whilst ensuring individual transactions do
      //not exceed XC_MAX_I2C_WRITE_LENGTH bytes
      index += 2;
      buf[0] = xc7000_fw[index];
      buf[1] = xc7000_fw[index + 1];
      pos = 2;
      while (pos < length)
      {
        if ((length - pos) > XC_MAX_I2C_WRITE_LENGTH - 2)
        {
          nbytes_to_send = XC_MAX_I2C_WRITE_LENGTH;
        }
        else
        {
          nbytes_to_send = (length - pos + 2);
        }
        for (i=2; i<nbytes_to_send; i++)
        {
          buf[i] = xc7000_fw[index + pos + i - 2];
        }
        result = xc_send_i2c_data(buf, nbytes_to_send);

        if (result!=XC_RESULT_SUCCESS)
          return result;

        pos += nbytes_to_send - 2;
      }
      index += length;
    }
  }
  return XC_RESULT_SUCCESS;
}

int xc_initialize()
{
    int result;
    // if base firmware has changed, then do hardware reset and reload base
    // firmware file

       //reset function in frontend
       result = xc_reset();
       if (result!=XC_RESULT_SUCCESS)
           return result;
        
       result = xc_load_i2c_sequence();

       if (result!=XC_RESULT_SUCCESS)
           return result;
       xc_write_reg(XREG_INIT, 0);

    // if standard-specific firmware has changed then reload standard-specific firmware file
    /*if ( std_firmware_changed )
    {

        current_tv_mode_ptr = new_tv_mode_ptr;
        current_channel_map_ptr = new_channel_map_ptr;

        xc_set_tv_standard(new_tv_mode_ptr);

        //do not return error if channel is incorrect...
        xc_set_channel( current_IF_Freq_khz, current_channel_ptr );
        xc_wait(30);                                                     // wait 30ms
    }*/

    return XC_RESULT_SUCCESS;
}

int xc_set_tv_standard(v4l2_std_id id )
{
    int i=0, error_code=0;
    struct ctc703_standard_s *curr_standard = NULL;
    for(i=0; i<CTC703_MAX_STANDARD_NUM; i++)
    {
        if(id == tv_standard[i].id)
        {
            curr_standard = &tv_standard[i];
            break;
        }
    }
    if(i >= CTC703_MAX_STANDARD_NUM)
        printk("[ctc703..]%s not support %s.\n",__func__,v4l2_std_to_str(id));
    error_code = xc_write_reg(XREG_VIDEO_MODE, curr_standard->vidmode);
    if (error_code == XC_RESULT_SUCCESS)
        error_code = xc_write_reg(XREG_AUDIO_MODE, curr_standard->audmode);
    return error_code;
}

int xc_set_rf_frequency(long frequency_in_hz)
{
  unsigned int frequency_code;
  if ((frequency_in_hz>1023000000) || (frequency_in_hz<1000000))
    return XC_RESULT_OUT_OF_RANGE;

  frequency_code = (unsigned int)(frequency_in_hz / 15625L);
  return xc_write_reg(XREG_RF_FREQ ,frequency_code);
}

int xc_FineTune_RF_frequency(long frequency_in_hz)
{
  unsigned int frequency_code;
  if ((frequency_in_hz>1023000000) || (frequency_in_hz<1000000))
    return XC_RESULT_OUT_OF_RANGE;

  frequency_code = (unsigned int)(frequency_in_hz / 15625L);
  return xc_write_reg(XREG_FINERFFREQ ,frequency_code);
}

int xc_set_if_frequency(long frequency_in_hz)
{
  unsigned int frequency_code = ( (frequency_in_hz/1000) * 1024)/1000;
  return xc_write_reg(XREG_IF_OUT ,frequency_code);
}
/*
int xc_set_channel(long if_freq_khz, XC_CHANNEL * channel_ptr)
{
    long  rf_freq_khz;

    if ( channel_ptr != NULL )
    {
      rf_freq_khz = channel_ptr->frequency *250/16;            // convert from Mhz to Khz

      xc_set_if_frequency(if_freq_khz*1000);           // Set IF out Freq: base on SCode, DCode

      return xc_FineTune_RF_frequency(rf_freq_khz*1000 );      // XC7000 change Freq;
    }
  return XC_RESULT_OUT_OF_RANGE;
}
*/
int xc_set_Xtal_frequency(long xtalFreqInKHz)
{
  unsigned int xtalRatio = (32000 * 0x8000)/xtalFreqInKHz;
  return xc_write_reg(XREG_XTALFREQ ,xtalRatio);
}

int xc_set_rf_mode(unsigned short SourceType)
{
  return xc_write_reg(XREG_SIGNALSOURCE, SourceType);
}

int xc_set_DDIMode(unsigned short ddimode)
{
  return xc_write_reg(XREG_DDIMODE ,ddimode);
}

int xc_shutdown()
{
  return xc_write_reg(XREG_POWER_DOWN, 0);
}

int xc_get_ADC_Envelope(unsigned short *adc_envelope)
{
  return xc_read_reg(XREG_ADC_ENV, adc_envelope);
}

// Obtain current frequency error
// Refer to datasheet for values.
int xc_get_frequency_error(int *frequency_error_hz)
{
  unsigned short int data;
  short int signed_data;
  int result;

  result = xc_read_reg(XREG_FREQ_ERROR, &data);
  if (result)
    return result;

  signed_data = (short int)data;
  (*frequency_error_hz) = signed_data * 15625;

  return 0;
}

// Obtain current lock status.
// Refer to datasheet for values.
int xc_get_lock_status(unsigned short int *lock_status)
{
  return xc_read_reg(XREG_LOCK, lock_status);
}

// Obtain Version codes.
// Refer to datasheet for values.
int xc_get_version(unsigned char* hw_majorversion,
                       unsigned char* hw_minorversion,
                       unsigned char* fw_majorversion,
                       unsigned char* fw_minorversion)
{
  unsigned short int data;
  int result;

  result = xc_read_reg(XREG_VERSION, &data);
  if (result)
    return result;

    (*hw_majorversion) = (data>>12) & 0x0F;
    (*hw_minorversion) = (data>>8) & 0x0F;
    (*fw_majorversion) = (data>>4) & 0x0F;
    (*fw_minorversion) = (data) & 0x0F;

  return 0;
}

// Obtain Product ID.
// Refer to datasheet for values.
int xc_get_product_id(unsigned short int *product_id)
{
  return xc_read_reg(XREG_PRODUCT_ID, product_id);
}

// Obtain current horizontal video frequency.
// Refer to datasheet for values.
int xc_get_hsync_freq(int *hsync_freq_hz)
{
  unsigned short int regData;
  int result;

  result = xc_read_reg(XREG_HSYNC_FREQ, &regData);
  if (result)
    return result;
  (*hsync_freq_hz) = ((regData & 0x0fff) * 763)/100;
  return result;
}

 // Obtain current number of lines per frame.
 // Refer to datasheet for values.
int xc_get_frame_lines(unsigned short int *frame_lines)
{
  return xc_read_reg(XREG_FRAME_LINES, frame_lines);
}

// Obtain current video signal quality.
// Refer to datasheet for values.
int xc_get_quality(unsigned short int *quality)
{
  return xc_read_reg(XREG_QUALITY, quality);
}

// Get the Clock Freq of the tuner
int xc_get_clock_freq(unsigned short int *clock_freq)
{
  return xc_read_reg(XREG_CLOCKFREQ, clock_freq);
}

unsigned short int WaitForLock()
{
  unsigned short int lockState = 0;
  int watchDogCount = 40;
  while ((lockState == 0) && (watchDogCount > 0))
  {
    xc_get_lock_status(&lockState);
    if (lockState != 1)
    {
      xc_wait(5);      // wait 5 ms
      watchDogCount--;
    }
  }
  return lockState;
}

bool xc_scan_channel(long chnl_freq)
{
  	long freq_error;
  	long  min_freq_error;
	long max_freq_error;
  	unsigned short int quality=0, max_quality=0;
  int res=0;
  bool  chnl_found = false;

  if (xc_set_rf_frequency(chnl_freq) != XC_RESULT_SUCCESS)
    return false;
  if (WaitForLock()== 1)
  {
    chnl_found = true;
    //add new channel
  }
  return chnl_found;
}

//numberOfSeek declare the number of afc range,
int xc_write_seek_frequency(int numberOfSeek, short int afc_range[])
{
  // each unit of seekFreqcies is 15.625 KHz
  int seekIndex, commStatus = 0;
  commStatus = xc_write_reg(XREG_SEEK_MODE, numberOfSeek << 4);
  for (seekIndex=0; seekIndex < numberOfSeek; seekIndex++)
  {
    commStatus |= xc_write_reg(XREG_SEEK_MODE, (afc_range[seekIndex] << 4) + seekIndex + 1);
  }
  return commStatus;
}

// change the channle, like xc_set_channel
void xc_channel_tuning(long rf_freq_khz, int if_freq_khz )
{
  xc_set_if_frequency(if_freq_khz * 1000);
  xc_FineTune_RF_frequency(rf_freq_khz * 1000);
  xc_wait(10);                             // wait to make sure the channel is stable!
}


