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

#ifndef  __CTC703_FUNC_H
#define __CTC703_FUNC_H
#include <linux/i2c.h>
#include "../aml_fe.h"

#define I2C_TRY_MAX_CNT                                     3

#define XC_RESULT_SUCCESS                               0
#define XC_RESULT_RESET_FAILURE                   1
#define XC_RESULT_I2C_WRITE_FAILURE            2
#define XC_RESULT_I2C_READ_FAILURE              3
#define XC_RESULT_OUT_OF_RANGE                    5

#define XC_MAX_I2C_WRITE_LENGTH                     64

#define XC_RESULT_SUCCESS                               0
#define XC_RESULT_RESET_FAILURE                   1
#define XC_RESULT_I2C_WRITE_FAILURE            2
#define XC_RESULT_I2C_READ_FAILURE              3
#define XC_RESULT_OUT_OF_RANGE                    5
//write only register define
#define XREG_INIT                                                   0x00
#define XREG_VIDEO_MODE                                   0x01
#define XREG_AUDIO_MODE                                   0x02
#define XREG_RF_FREQ                                          0x03
#define XREG_D_CODE                                            0x04
#define XREG_IF_OUT                                              0x05
#define XREG_SEEK_MODE                                     0x07
#define XREG_POWER_DOWN                                0x0A
#define XREG_SIGNALSOURCE                              0x0D
#define XREG_SMOOTHEDCVBS                             0x0E
#define XREG_XTALFREQ                                        0x0F
#define XREG_FINERFFREQ                                    0x10
#define XREG_DDIMODE                                          0x11
//read only register define
#define XREG_ADC_ENV                                          0x00
#define XREG_QUALITY                                           0x01
#define XREG_FRAME_LINES                                  0x02
#define XREG_HSYNC_FREQ                                  0x03
#define XREG_LOCK                                                 0x04
#define XREG_FREQ_ERROR                                  0x05
#define XREG_SNR                                                   0x06
#define XREG_VERSION                                          0x07
#define XREG_PRODUCT_ID                                   0x08
#define XREG_BUSY                                                 0x09
#define XREG_CLOCKFREQ                                     0x10
extern unsigned char xc7000_fw[16497]; 
extern void xc_set_i2c_client(struct i2c_client *client);

// Sends data bytes to xc4000 via I2C starting with
// bytes_to_send[0] and ending with bytes_to_send[nb_bytes_to_send-1]
extern int xc_send_i2c_data(unsigned char *bytes_to_send, int nb_bytes_to_send);

// Reads data bytes from xc4000 via I2C starting with
// bytes_received[0] and ending with bytes_received[nb_bytes_to_receive-1]
int xc_read_i2c_data( void *bytes_received, int nb_bytes_to_receive);

// Does hardware reset
extern int xc_reset(void);

// Waits for wait_ms milliseconds
extern void xc_wait(int wait_ms);

extern int xc_write_reg(unsigned short regAddr, unsigned short i2cData);

extern int xc_read_reg( unsigned short regAddr, unsigned short *i2cData);

// Download firmware
extern int xc_load_i2c_sequence(void);

// Perform calibration and initialize default parameter
extern int xc_initialize(void);

// Initialize device according to supplied tv mode.
extern int xc_set_tv_standard(v4l2_std_id id);

extern int xc_set_rf_frequency(long frequency_in_hz);

extern int xc_FineTune_RF_frequency(long frequency_in_hz);

extern int xc_set_if_frequency(long frequency_in_hz);

extern int  xc_get_clock_freq(unsigned short int *clock_freq);
// to set the TV channel.
//int xc_set_channel(long if_freq_khz, XC_CHANNEL* new_channel_ptr);

extern int xc_set_rf_mode(unsigned short SourceType);

// set crystal frequency, input value is KHz
extern int xc_set_Xtal_frequency(long xtalFreqInKHz);

extern int xc_set_DDIMode(unsigned short ddimode);

// Power-down device.
extern int xc_shutdown();

// Get ADC envelope value.
extern int xc_get_ADC_Envelope(unsigned short *adc_envelope);

// Get current frequency error.
extern int xc_get_frequency_error(int *frequency_error_mhz);

// Get lock status.
extern int xc_get_lock_status(unsigned short *lock_status);
extern unsigned short int WaitForLock();

// Get device version information.
extern int xc_get_version( unsigned char* hw_majorversion,unsigned char* hw_minorversion,
                        unsigned char* fw_majorversion, unsigned char* fw_minorversion);

// Get device product ID.
extern int xc_get_product_id(unsigned short *product_id);

// Get horizontal sync frequency.
extern int xc_get_hsync_freq(int *hsync_freq_hz);

// Get number of lines per frame.
extern int xc_get_frame_lines(unsigned short *frame_lines);

// Get quality estimate.
extern int xc_get_quality(unsigned short *quality);

// Get the Clock Freq of the tuner
extern int xc_get_clock_freq(unsigned short *clock_freq);

// tune a channel and return the lock status
extern bool xc_scan_channel(long chnl_freq);

extern int xc_write_seek_frequency(int numberOfSeek, short afc_range[]);

// change the channle, like xc_set_channel
extern void xc_channel_tuning(long rf_freq_khz, int if_freq_khz );

typedef struct ctc703_standard_s{
    v4l2_std_id       id;
    unsigned short audmode;
    unsigned short vidmode;
}ctc703_standard_t;

typedef struct ctc703_param_s{
    int reset_gpio;
    int reset_value;
}ctc703_param_t;
typedef struct ctc703_device_s{
    struct i2c_client tuner_client;
    struct class *clsp;
    struct analog_parameters parm;
    struct ctc703_param_s cpram;
}ctc703_device_t;

//---------------------------------------------------------------------------
#endif

