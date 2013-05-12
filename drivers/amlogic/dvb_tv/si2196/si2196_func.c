/*
 * Silicon labs si2196 Tuner Device Driver
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
#include <linux/delay.h>

/* Local Headers */
#include "si2196_func.h"


#define I2C_TRY_MAX_CNT       3  //max try counter

static unsigned int delay_det=65;
module_param(delay_det, uint, 0644);
MODULE_PARM_DESC(delay_det, "delay set freq time\n");
/* for si2196 tuner type */
unsigned char si2196_fw_1_2b1[] = {
0x04,0x01,0x01,0x00,0xAE,0x45,0xB9,0x6D,
0x05,0x02,0x2B,0x39,0xD2,0xE8,0xFD,0xF5,
0x05,0x1C,0x5D,0x8F,0x78,0x6D,0xA2,0x09,
0x2F,0x6D,0xA7,0x13,0x77,0x4A,0x13,0x7B,
0x05,0xCD,0x80,0xE8,0x92,0x77,0x83,0x87,
0x2A,0x5B,0xB0,0x1C,0x06,0x7E,0xE0,0xD1,
0x05,0x46,0x34,0xC6,0xFB,0x4B,0xEE,0x51,
0x2F,0xC3,0xFE,0xDA,0x23,0x94,0x03,0xF0,
0x29,0xBB,0x2D,0x32,0x76,0xA6,0x18,0x5B,
0x07,0x9E,0xE3,0xDB,0x83,0x25,0xCE,0xB4,
0x2A,0x7A,0x90,0xA4,0x43,0x25,0xD4,0x43,
0x07,0xD6,0xFB,0x49,0x26,0xD4,0x77,0xEE,
0x27,0x49,0x65,0x7D,0x5B,0x15,0x9C,0x9C,
0x2F,0x26,0x10,0x18,0x0B,0x97,0x20,0x5A,
0x27,0x82,0x9D,0xD5,0x31,0xA1,0x34,0x32,
0x27,0x38,0x32,0xCE,0xE4,0x9F,0x87,0x86,
0x25,0x37,0x4A,0x09,0xB8,0x15,0xB6,0x54,
0x0F,0x82,0xF9,0x3A,0xA6,0x8B,0x48,0x3C,
0x2F,0xBB,0x5E,0x73,0xE8,0xD3,0x3A,0xC9,
0x2F,0x4A,0x9D,0x95,0x17,0x6F,0x3C,0x78,
0x27,0x2E,0x6D,0xD5,0x79,0x29,0xAE,0x36,
0x2F,0xF7,0x3B,0x46,0x6C,0xED,0x2E,0x85,
0x0F,0xC6,0x4F,0x2C,0x99,0xA6,0xCA,0x5E,
0x0F,0x7C,0xB0,0xF0,0xE1,0xA1,0xB9,0x13,
0x0F,0x12,0xCC,0xD9,0x09,0x9F,0xF4,0xFF,
0x2A,0x7E,0x58,0x46,0x4A,0x18,0x6A,0xDC,
0x07,0xE4,0xCC,0x09,0x95,0x75,0x6F,0xAA,
0x07,0xFC,0x8B,0xF9,0x28,0xBA,0x76,0x78,
0x0F,0x4B,0xED,0x22,0x63,0xE9,0xC8,0x7A,
0x05,0x75,0xE1,0xDD,0xCF,0xCF,0xC6,0x24,
0x22,0x11,0x1D,0x10,0x67,0xCD,0x16,0x82,
0x05,0x05,0x47,0x99,0x54,0x5E,0xCB,0x43,
0x22,0x77,0xFE,0x42,0x43,0x77,0xE6,0x10,
0x05,0x04,0x0B,0xB7,0x77,0x4D,0x45,0x62,
0x22,0xF2,0x3D,0xF4,0xA4,0x3D,0x22,0x2C,
0x0F,0x24,0x1B,0x48,0x5F,0x52,0xCF,0x21,
0x05,0x98,0x79,0x03,0xC8,0x75,0xC7,0x5A
};


//#define NB_LINES (sizeof(firmwaretable)/(8*sizeof(unsigned char)))

#ifdef USING_ATV_FILTER
    static unsigned char dlif_vidfilt_table[] = {0};
    #define DLIF_VIDFILT_LINES (sizeof(dlif_vidfilt_table)/(8*sizeof(unsigned char)))
#endif

static si2196_common_reply_struct  reply;
/************************************************************************************************************************
  NAME:		   si2196_readcommandbytes function
  DESCRIPTION:Read inbbytes from the i2c device into pucdatabuffer, return number of bytes read
  Parameter: iI2CIndex, the index of the first byte to read.
  Parameter: inbbytes, the number of bytes to read.
  Parameter: *pucdatabuffer, a pointer to a buffer used to store the bytes
  Porting:    Replace with embedded system I2C read function
  Returns:    Actual number of bytes read.
************************************************************************************************************************/
static int si2196_readcommandbytes(struct i2c_client *si2196, int inbbytes, unsigned char *pucdatabuffer)
{
    int i2c_flag = 0;
    int i = 0;
    unsigned int i2c_try_cnt = I2C_TRY_MAX_CNT;

    struct i2c_msg msg[] = {
        {
            .addr  = si2196->addr,
            .flags  = I2C_M_RD,
            .len     = inbbytes,
            .buf     = pucdatabuffer,
        },
    };

repeat:
    i2c_flag = i2c_transfer(si2196->adapter, msg, 1);
    if (i2c_flag < 0) {
        pr_err("%s: error in read sli2176, %d byte(s) should be read,. \n", __func__, inbbytes);
        if (i++ < i2c_try_cnt) {
            pr_err("%s: error in read sli2176, try again!!!\n", __func__);
            goto repeat;
        }
        else
            return -EIO;
    }
    else {
        //pr_info("%s: read %d bytes\n", __func__, inbbytes);
        return inbbytes;
    }
}

/************************************************************************************************************************
  NAME:  si2196_writecommandbytes
  DESCRIPTION:  Write inbbytes from pucdatabuffer to the i2c device, return number of bytes written
  Porting:    Replace with inbbytes system I2C write function
  Returns:    Number of bytes written
************************************************************************************************************************/
static int si2196_writecommandbytes(struct i2c_client *si2196, int inbbytes, unsigned char *pucdatabuffer)
{
    int i2c_flag = 0;
    int i = 0;
    unsigned int i2c_try_cnt = I2C_TRY_MAX_CNT;

    struct i2c_msg msg[] = {
	    {
			.addr	= si2196->addr,
			.flags	= 0,    //|I2C_M_TEN,
			.len	= inbbytes,
			.buf	= pucdatabuffer,
		}

	};

repeat:
    i2c_flag = i2c_transfer(si2196->adapter, msg, 1);
    if (i2c_flag < 0) {
        pr_err("%s: error in write sli2176, %d byte(s) should be read,. \n", __func__, inbbytes);
        if (i++ < i2c_try_cnt) {
            pr_err("%s: error in wirte sli2176, try again!!!\n", __func__);
            goto repeat;
        }
        else
            return -EIO;
    }
    else {
        //pr_info("%s: write %d bytes\n", __func__, inbbytes);
        return inbbytes;
    }
}

/***********************************************************************************************************************
  sli2176_pollforcts function
  Use:        CTS checking function
              Used to check the CTS bit until it is set before sending the next command
  Comments:   The status byte definition being identical for all commands,
              using this function to fill the status structure hels reducing the code size
  Comments:   waitForCTS = 1 => I2C polling
              waitForCTS = 2 => INTB followed by a read (reading a HW byte using the cypress chip)
              max timeout = 100 ms

  Porting:    If reading INTB is not possible, the waitForCTS = 2 case can be removed

  Parameter: waitForCTS          a flag indicating if waiting for CTS is required
  Returns:   1 if the CTS bit is set, 0 otherwise
 ***********************************************************************************************************************/
static unsigned char si2196_pollforcts(struct i2c_client *si2196)
{
    unsigned char error_code = 0;
    unsigned char loop_count = 0;
    unsigned char rspbytebuffer[1];

    for (loop_count=0; loop_count<50; loop_count++) { /* wait a maximum of 50*25ms = 1.25s  */
        if (si2196_readcommandbytes(si2196, 1, rspbytebuffer) != 1)
            error_code = ERROR_SI2196_POLLING_CTS;
        else
            error_code = NO_SI2196_ERROR;
        if (error_code || (rspbytebuffer[0] & 0x80))
            goto exit;
        mdelay(2); /* CTS not set, wait 2ms and retry */
    }
    error_code = ERROR_SI2196_CTS_TIMEOUT;

exit:
    if (error_code)
        pr_info("%s: poll cts function error:%d!!!...............\n", __func__, error_code);

    return error_code;
}

/***********************************************************************************************************************
  SI2196_CurrentResponseStatus function
  Use:        status checking function
              Used to fill the SI2196_COMMON_REPLY_struct members with the ptDataBuffer byte's bits
  Comments:   The status byte definition being identical for all commands,
              using this function to fill the status structure hels reducing the code size

  Parameter: *ret          the SI2196_COMMON_REPLY_struct
  Parameter: ptDataBuffer  a single byte received when reading a command's response (the first byte)
  Returns:   0 if the err bit (bit 6) is unset, 1 otherwise
 ***********************************************************************************************************************/
static unsigned char si2196_currentresponsestatus(si2196_common_reply_struct *common_reply, unsigned char ptdatabuffer)
{
    /* _status_code_insertion_start */
    common_reply->tunint = ((ptdatabuffer >> 0 ) & 0x01);
    common_reply->atvint = ((ptdatabuffer >> 1 ) & 0x01);
    common_reply->dtvint = ((ptdatabuffer >> 2 ) & 0x01);
    common_reply->err    = ((ptdatabuffer >> 6 ) & 0x01);
    common_reply->cts    = ((ptdatabuffer >> 7 ) & 0x01);
    /* _status_code_insertion_point */
    return (common_reply->err ? ERROR_SI2196_ERR : NO_SI2196_ERROR);
}

/***********************************************************************************************************************
  si2196_pollforresponse function
  Use:        command response retrieval function
              Used to retrieve the command response in the provided buffer,
              poll for response either by I2C polling or wait for INTB
  Comments:   The status byte definition being identical for all commands,
              using this function to fill the status structure hels reducing the code size
  Comments:   waitForCTS = 1 => I2C polling
              waitForCTS = 2 => INTB followed by a read (reading a HW byte using the cypress chip)
              max timeout = 100 ms

  Porting:    If reading INTB is not possible, the waitForCTS = 2 case can be removed

  Parameter:  waitforresponse  a flag indicating if waiting for the response is required
  Parameter:  nbbytes          the number of response bytes to read
  Parameter:  pbytebuffer      a buffer into which bytes will be stored
  Returns:    0 if no error, an error code otherwise
 ***********************************************************************************************************************/
static unsigned char si2196_pollforresponse(struct i2c_client *si2196, unsigned char waitforresponse, unsigned int nbbytes, unsigned char *pbytebuffer, si2196_common_reply_struct *common_reply)
{
    unsigned char error_code;
    unsigned char loop_count;

    for (loop_count=0; loop_count<100; loop_count++) { /* wait a maximum of 50*2ms = 100ms                        */
        switch (waitforresponse) { /* type of response polling?                                */
            case 0 : /* no polling? valid option, but shouldn't have been called */
                error_code = NO_SI2196_ERROR; /* return no error                                          */
                goto exit;

            case 1 : /* I2C polling status?                                      */
                if (si2196_readcommandbytes(si2196, nbbytes, pbytebuffer) != nbbytes)
                    error_code = ERROR_SI2196_POLLING_RESPONSE;
                else
                    error_code = NO_SI2196_ERROR;
                if (error_code)
                    goto exit;	/* if error, exit with error code */
                if (pbytebuffer[0] & 0x80)	  /* CTS set? */
                {
                    error_code = si2196_currentresponsestatus(common_reply, pbytebuffer[0]);
                    goto exit; /* exit whether ERR set or not   */
                }
                break;

            default :
                error_code = ERROR_SI2196_PARAMETER_OUT_OF_RANGE; /* support debug of invalid CTS poll parameter   */
                goto exit;
        }
        mdelay(2); /* CTS not set, wait 2ms and retry                         */
    }
    error_code = ERROR_SI2196_CTS_TIMEOUT;

exit:
    return error_code;
}

/************************************************************************************************************************
  NAME: CheckStatus
  DESCRIPTION:     Read Si2170 STATUS byte and return decoded status
  Parameter:  Si2170 Context (I2C address)
  Parameter:  Status byte (TUNINT, ATVINT, DTVINT, ERR, CTS, CHLINT, and CHL flags).
  Returns:    Si2170/I2C transaction error code
************************************************************************************************************************/

int si2196_check_status(struct i2c_client *si2196, si2196_common_reply_struct *common_reply)
{
    unsigned char buffer[1];
    /* read STATUS byte */
    if (si2196_pollforresponse(si2196, 1, 1, buffer, common_reply) != 0)
    {
        return ERROR_SI2196_POLLING_RESPONSE;
    }
    pr_info("%s:status 0x%2x cts %d,err %d .\n",__func__,(*buffer),common_reply->cts,common_reply->err);
    return 0;
}

/***********************************************************************************************************************
  NAME: Si2170_L1_API_Patch
  DESCRIPTION: Patch information function
              Used to send a number of bytes to the Si2170. Useful to download the firmware.
  Parameter:   *api    a pointer to the api context to initialize
  Parameter:  waitForCTS flag for CTS checking prior to sending a Si2170 API Command
  Parameter:  waitForResponse flag for CTS checking and Response readback after sending Si2170 API Command
  Parameter:  number of bytes to transmit
  Parameter:  Databuffer containing the bytes to transfer in an unsigned char array.
  Returns:   0 if no error, else a nonzero int representing an error
 ***********************************************************************************************************************/
static unsigned char si2196_api_patch(struct i2c_client *si2196, int inbbytes, unsigned char *pucdatabuffer, si2196_common_reply_struct *common_reply)
{
    unsigned char res = 0;
    unsigned char error_code = 0;
    unsigned char rspbytebuffer[1];

    res = si2196_pollforcts(si2196);
    if (res != NO_SI2196_ERROR)
    {
        pr_info("%s: poll cts error:%d!!!!\n", __func__, res);
        return res;
    }

    res = si2196_writecommandbytes(si2196, inbbytes, pucdatabuffer);
    if (res!=inbbytes)
    {
        pr_info("%s: poll cts error:%d!!!!\n", __func__, ERROR_SI2196_SENDING_COMMAND);
       return ERROR_SI2196_SENDING_COMMAND;
    }

    error_code = si2196_pollforresponse(si2196, 1, 1, rspbytebuffer, common_reply);
    if (error_code)
        pr_info("%s: poll response error:%d!!!!\n", __func__, error_code);

    return error_code;
}


/* _commands_insertion_start */
#ifdef SI2196_AGC_OVERRIDE_CMD
/*---------------------------------------------------*/
/* SI2196_AGC_OVERRIDE COMMAND                     */
/*---------------------------------------------------*/
static unsigned char si2196_agc_override(struct i2c_client *si2196,
        unsigned char   force_max_gain,
        unsigned char   force_top_gain,
        si2196_cmdreplyobj_t *rsp)
{
    unsigned char error_code = 0;
    unsigned char cmdbytebuffer[2];
    unsigned char rspbytebuffer[1];

#ifdef DEBUG_RANGE_CHECK
    if ( (force_max_gain > SI2196_AGC_OVERRIDE_CMD_FORCE_MAX_GAIN_MAX)
            || (force_top_gain > SI2196_AGC_OVERRIDE_CMD_FORCE_TOP_GAIN_MAX) )
        return ERROR_SI2196_PARAMETER_OUT_OF_RANGE;
#endif /* DEBUG_RANGE_CHECK */

    error_code = si2196_pollforcts(si2196);
    if (error_code)
    {
        pr_info("%s: poll cts error:%d!!!!\n", __func__, error_code);
        goto exit;
    }

    cmdbytebuffer[0] = SI2196_AGC_OVERRIDE_CMD;
    cmdbytebuffer[1] = (unsigned char) ( ( force_max_gain & SI2196_AGC_OVERRIDE_CMD_FORCE_MAX_GAIN_MASK ) << SI2196_AGC_OVERRIDE_CMD_FORCE_MAX_GAIN_LSB|
                                         ( force_top_gain & SI2196_AGC_OVERRIDE_CMD_FORCE_TOP_GAIN_MASK ) << SI2196_AGC_OVERRIDE_CMD_FORCE_TOP_GAIN_LSB);

    if (si2196_writecommandbytes(si2196, 2, cmdbytebuffer) != 2)
    {
        error_code = ERROR_SI2196_SENDING_COMMAND;
        pr_info("%s: write command byte error:%d!!!!\n", __func__, error_code);
    }

    if (!error_code)
    {
        error_code = si2196_pollforresponse(si2196, 1, 1, rspbytebuffer, &reply);
        if (error_code)
            pr_info("%s: poll response error:%d!!!!\n", __func__, error_code);
        rsp->reply = &reply;
    }
exit:
    return error_code;
}
#endif /* SI2196_AGC_OVERRIDE_CMD */
#ifdef SI2196_ATV_CW_TEST_CMD
/*---------------------------------------------------*/
/* SI2196_ATV_CW_TEST COMMAND                      */
/*---------------------------------------------------*/
static unsigned char si2196_atv_cw_test(struct i2c_client *si2196,
        unsigned char   pc_lock,
        si2196_cmdreplyobj_t *rsp)
{
    unsigned char error_code = 0;
    unsigned char cmdbytebuffer[2];
    unsigned char rspbytebuffer[1];

#ifdef DEBUG_RANGE_CHECK
    if ( (pc_lock > SI2196_ATV_CW_TEST_CMD_PC_LOCK_MAX) )
        return ERROR_SI2196_PARAMETER_OUT_OF_RANGE;
#endif /* DEBUG_RANGE_CHECK */

    error_code = si2196_pollforcts(si2196);
    if (error_code) goto exit;

    cmdbytebuffer[0] = SI2196_ATV_CW_TEST_CMD;
    cmdbytebuffer[1] = (unsigned char) ( ( pc_lock & SI2196_ATV_CW_TEST_CMD_PC_LOCK_MASK ) << SI2196_ATV_CW_TEST_CMD_PC_LOCK_LSB);

    if (si2196_writecommandbytes(si2196, 2, cmdbytebuffer) != 2) error_code = ERROR_SI2196_SENDING_COMMAND;

    if (!error_code)
    {
        error_code = si2196_pollforresponse(si2196, 1, 1, rspbytebuffer, &reply);
        if (error_code)
            pr_info("%s: poll response error:%d!!!!\n", __func__, error_code);
        rsp->reply = &reply;
    }
exit:
    return error_code;
}
#endif /* SI2196_ATV_CW_TEST_CMD */
#ifdef SI2196_ATV_RESTART_CMD
/*---------------------------------------------------*/
/* SI2196_ATV_RESTART COMMAND                      */
/*---------------------------------------------------*/
unsigned char si2196_atv_restart(struct i2c_client *si2196,
        unsigned char   mode,
        si2196_cmdreplyobj_t *rsp)
{
    unsigned char error_code = 0;
    unsigned char cmdbytebuffer[2];
    unsigned char rspbytebuffer[1];

#ifdef DEBUG_RANGE_CHECK
    if ( (mode > SI2196_ATV_RESTART_CMD_MODE_MAX) )
        return ERROR_SI2196_PARAMETER_OUT_OF_RANGE;
#endif /* DEBUG_RANGE_CHECK */

    error_code = si2196_pollforcts(si2196);
    if (error_code)
    {
        pr_info("%s: poll cts error:%d!!!!\n", __func__, error_code);
        goto exit;
    }

    cmdbytebuffer[0] = SI2196_ATV_RESTART_CMD;
    cmdbytebuffer[1] = (unsigned char) ( ( mode & SI2196_ATV_RESTART_CMD_MODE_MASK ) << SI2196_ATV_RESTART_CMD_MODE_LSB);

    if (si2196_writecommandbytes(si2196, 2, cmdbytebuffer) != 2)
    {
        error_code = ERROR_SI2196_SENDING_COMMAND;
        pr_info("%s: write command byte error:%d!!!!\n", __func__, error_code);
    }
    mdelay(20);
    if (!error_code)
    {
        error_code = si2196_pollforresponse(si2196, 1, 1, rspbytebuffer, &reply);
        if (error_code)
            pr_info("%s: poll response error:%d!!!!\n", __func__, error_code);
        rsp->reply = &reply;
    }
exit:
    return error_code;
}
#endif /* SI2196_ATV_RESTART_CMD */
#ifdef SI2196_ATV_STATUS_CMD
/*---------------------------------------------------*/
/* SI2196_ATV_STATUS COMMAND                       */
/*---------------------------------------------------*/
unsigned char si2196_atv_status(struct i2c_client *si2196,
        unsigned char intack,
        si2196_cmdreplyobj_t *rsp)
{
    unsigned char error_code = 0;
    unsigned char cmdbytebuffer[2];
    unsigned char rspbytebuffer[11];

#ifdef DEBUG_RANGE_CHECK
    if ( (intack > SI2196_ATV_STATUS_CMD_INTACK_MAX) )
        return ERROR_SI2196_PARAMETER_OUT_OF_RANGE;
#endif /* DEBUG_RANGE_CHECK */

    error_code = si2196_pollforcts(si2196);
    if (error_code)
    {
        pr_info("%s: poll cts error:%d!!!!\n", __func__, error_code);
        goto exit;
    }

    cmdbytebuffer[0] = SI2196_ATV_STATUS_CMD;
    cmdbytebuffer[1] = (unsigned char) ( ( intack & SI2196_ATV_STATUS_CMD_INTACK_MASK ) << SI2196_ATV_STATUS_CMD_INTACK_LSB);

    if (si2196_writecommandbytes(si2196, 2, cmdbytebuffer) != 2)
    {
        error_code = ERROR_SI2196_SENDING_COMMAND;
        pr_info("%s: write command byte error:%d!!!!\n", __func__, error_code);
    }

    if (!error_code)
    {
        error_code = si2196_pollforresponse(si2196, 1, 11, rspbytebuffer, &reply);
        if (error_code)
            pr_info("%s: poll response error:%d!!!!\n", __func__, error_code);
        rsp->reply = &reply;
        if (!error_code)
        {
            rsp->atv_status.chlint           =   (( ( (rspbytebuffer[1]  )) >> SI2196_ATV_STATUS_RESPONSE_CHLINT_LSB           ) & SI2196_ATV_STATUS_RESPONSE_CHLINT_MASK           );
            rsp->atv_status.pclint           =   (( ( (rspbytebuffer[1]  )) >> SI2196_ATV_STATUS_RESPONSE_PCLINT_LSB           ) & SI2196_ATV_STATUS_RESPONSE_PCLINT_MASK           );
            rsp->atv_status.dlint             =   (( ( (rspbytebuffer[1]  )) >> SI2196_ATV_STATUS_RESPONSE_DLINT_LSB            ) & SI2196_ATV_STATUS_RESPONSE_DLINT_MASK            );
            rsp->atv_status.snrlint          =   (( ( (rspbytebuffer[1]  )) >> SI2196_ATV_STATUS_RESPONSE_SNRLINT_LSB          ) & SI2196_ATV_STATUS_RESPONSE_SNRLINT_MASK          );
            rsp->atv_status.snrhint         =   (( ( (rspbytebuffer[1]  )) >> SI2196_ATV_STATUS_RESPONSE_SNRHINT_LSB          ) & SI2196_ATV_STATUS_RESPONSE_SNRHINT_MASK          );
            rsp->atv_status.chl                =   (( ( (rspbytebuffer[2]  )) >> SI2196_ATV_STATUS_RESPONSE_CHL_LSB              ) & SI2196_ATV_STATUS_RESPONSE_CHL_MASK              );
            rsp->atv_status.pcl               =   (( ( (rspbytebuffer[2]  )) >> SI2196_ATV_STATUS_RESPONSE_PCL_LSB              ) & SI2196_ATV_STATUS_RESPONSE_PCL_MASK              );
            rsp->atv_status.dl                 =   (( ( (rspbytebuffer[2]  )) >> SI2196_ATV_STATUS_RESPONSE_DL_LSB               ) & SI2196_ATV_STATUS_RESPONSE_DL_MASK               );
            rsp->atv_status.snrl              =   (( ( (rspbytebuffer[2]  )) >> SI2196_ATV_STATUS_RESPONSE_SNRL_LSB             ) & SI2196_ATV_STATUS_RESPONSE_SNRL_MASK             );
            rsp->atv_status.snrh             =   (( ( (rspbytebuffer[2]  )) >> SI2196_ATV_STATUS_RESPONSE_SNRH_LSB             ) & SI2196_ATV_STATUS_RESPONSE_SNRH_MASK             );
            rsp->atv_status.video_snr     =   (( ( (rspbytebuffer[3]  )) >> SI2196_ATV_STATUS_RESPONSE_VIDEO_SNR_LSB        ) & SI2196_ATV_STATUS_RESPONSE_VIDEO_SNR_MASK        );
            rsp->atv_status.afc_freq       = (((( ( (rspbytebuffer[4]  ) | (rspbytebuffer[5]  << 8 )) >> SI2196_ATV_STATUS_RESPONSE_AFC_FREQ_LSB         ) & SI2196_ATV_STATUS_RESPONSE_AFC_FREQ_MASK) <<SI2196_ATV_STATUS_RESPONSE_AFC_FREQ_SHIFT ) >>SI2196_ATV_STATUS_RESPONSE_AFC_FREQ_SHIFT         );
            rsp->atv_status.video_sc_spacing = (((( ( (rspbytebuffer[6]  ) | (rspbytebuffer[7]  << 8 )) >> SI2196_ATV_STATUS_RESPONSE_VIDEO_SC_SPACING_LSB ) & SI2196_ATV_STATUS_RESPONSE_VIDEO_SC_SPACING_MASK) <<SI2196_ATV_STATUS_RESPONSE_VIDEO_SC_SPACING_SHIFT ) >>SI2196_ATV_STATUS_RESPONSE_VIDEO_SC_SPACING_SHIFT );
            rsp->atv_status.video_sys    =   (( ( (rspbytebuffer[8]  )) >> SI2196_ATV_STATUS_RESPONSE_VIDEO_SYS_LSB        ) & SI2196_ATV_STATUS_RESPONSE_VIDEO_SYS_MASK        );
            rsp->atv_status.color            =   (( ( (rspbytebuffer[8]  )) >> SI2196_ATV_STATUS_RESPONSE_COLOR_LSB            ) & SI2196_ATV_STATUS_RESPONSE_COLOR_MASK            );
            rsp->atv_status.trans            =   (( ( (rspbytebuffer[8]  )) >> SI2196_ATV_STATUS_RESPONSE_TRANS_LSB            ) & SI2196_ATV_STATUS_RESPONSE_TRANS_MASK            );
            rsp->atv_status.audio_sys    =   (( ( (rspbytebuffer[9]  )) >> SI2196_ATV_STATUS_RESPONSE_AUDIO_SYS_LSB        ) & SI2196_ATV_STATUS_RESPONSE_AUDIO_SYS_MASK        );
            rsp->atv_status.audio_chan_bw    =   (( ( (rspbytebuffer[10] )) >> SI2196_ATV_STATUS_RESPONSE_AUDIO_CHAN_BW_LSB    ) & SI2196_ATV_STATUS_RESPONSE_AUDIO_CHAN_BW_MASK    );
        }
    }
exit:
    return error_code;
}
#endif /* SI2196_ATV_STATUS_CMD */
#ifdef    SI2196_SD_STATUS_CMD
 /*---------------------------------------------------*/
/* SI2196_SD_STATUS COMMAND                        */
/*---------------------------------------------------*/
unsigned char si2196_sd_status(struct i2c_client *si2196,si2196_cmdreplyobj_t *rsp, unsigned char   intack)
{
    unsigned char error_code = 0;
    unsigned char cmdByteBuffer[2];
    unsigned char rspByteBuffer[8];

  #ifdef   DEBUG_RANGE_CHECK
    if ((intack > SI2196_SD_STATUS_CMD_INTACK_MAX) )
    {
            error_code++; 
            pr_info("\nOut of range: ");
    }
    pr_info("INTACK %d ", intack );
    if (error_code) {
      pr_info("%s:%d out of range parameters\n", __func__,error_code);
      return ERROR_SI2196_PARAMETER_OUT_OF_RANGE;
    }
  #endif /* DEBUG_RANGE_CHECK */
    error_code = si2196_pollforcts(si2196);
    if (error_code)
    {
        pr_info("%s: poll cts error:%d!!!!\n", __func__, error_code);
        return error_code;
    }
    cmdByteBuffer[0] = SI2196_SD_STATUS_CMD;
    cmdByteBuffer[1] = (unsigned char) ( ( intack & SI2196_SD_STATUS_CMD_INTACK_MASK ) << SI2196_SD_STATUS_CMD_INTACK_LSB);

    if (si2196_writecommandbytes(si2196, 2, cmdByteBuffer) != 2) {
      pr_info("%s:Error writing SD_STATUS bytes!\n",__func__);
      return ERROR_SI2196_SENDING_COMMAND;
    }

    error_code = si2196_pollforresponse(si2196, 1, 8, rspByteBuffer, &reply);
    rsp->reply = &reply;
    if (error_code) {
      pr_info("%s:Error polling SD_STATUS response.\n",__func__);
      return error_code;
    }
    rsp->sd_status.asdcint               =   (( ( (rspByteBuffer[1]  )) >> SI2196_SD_STATUS_RESPONSE_ASDCINT_LSB               ) & SI2196_SD_STATUS_RESPONSE_ASDCINT_MASK               );
    rsp->sd_status.nicamint             =   (( ( (rspByteBuffer[1]  )) >> SI2196_SD_STATUS_RESPONSE_NICAMINT_LSB              ) & SI2196_SD_STATUS_RESPONSE_NICAMINT_MASK              );
    rsp->sd_status.pcmint                =   (( ( (rspByteBuffer[1]  )) >> SI2196_SD_STATUS_RESPONSE_PCMINT_LSB                ) & SI2196_SD_STATUS_RESPONSE_PCMINT_MASK                );
    rsp->sd_status.scmint                =   (( ( (rspByteBuffer[1]  )) >> SI2196_SD_STATUS_RESPONSE_SCMINT_LSB                ) & SI2196_SD_STATUS_RESPONSE_SCMINT_MASK                );
    rsp->sd_status.odmint                =   (( ( (rspByteBuffer[1]  )) >> SI2196_SD_STATUS_RESPONSE_ODMINT_LSB                ) & SI2196_SD_STATUS_RESPONSE_ODMINT_MASK                );
    rsp->sd_status.afcmint               =   (( ( (rspByteBuffer[1]  )) >> SI2196_SD_STATUS_RESPONSE_AFCMINT_LSB               ) & SI2196_SD_STATUS_RESPONSE_AFCMINT_MASK               );
    rsp->sd_status.ssint                   =   (( ( (rspByteBuffer[1]  )) >> SI2196_SD_STATUS_RESPONSE_SSINT_LSB                 ) & SI2196_SD_STATUS_RESPONSE_SSINT_MASK                 );
    rsp->sd_status.agcsint               =   (( ( (rspByteBuffer[1]  )) >> SI2196_SD_STATUS_RESPONSE_AGCSINT_LSB               ) & SI2196_SD_STATUS_RESPONSE_AGCSINT_MASK               );
    rsp->sd_status.asdc                   =   (( ( (rspByteBuffer[2]  )) >> SI2196_SD_STATUS_RESPONSE_ASDC_LSB                  ) & SI2196_SD_STATUS_RESPONSE_ASDC_MASK                  );
    rsp->sd_status.nicam                 =   (( ( (rspByteBuffer[2]  )) >> SI2196_SD_STATUS_RESPONSE_NICAM_LSB                 ) & SI2196_SD_STATUS_RESPONSE_NICAM_MASK                 );
    rsp->sd_status.pcm                   =   (( ( (rspByteBuffer[2]  )) >> SI2196_SD_STATUS_RESPONSE_PCM_LSB                   ) & SI2196_SD_STATUS_RESPONSE_PCM_MASK                   );
    rsp->sd_status.scm                   =   (( ( (rspByteBuffer[2]  )) >> SI2196_SD_STATUS_RESPONSE_SCM_LSB                   ) & SI2196_SD_STATUS_RESPONSE_SCM_MASK                   );
    rsp->sd_status.odm                   =   (( ( (rspByteBuffer[2]  )) >> SI2196_SD_STATUS_RESPONSE_ODM_LSB                   ) & SI2196_SD_STATUS_RESPONSE_ODM_MASK                   );
    rsp->sd_status.afcm                  =   (( ( (rspByteBuffer[2]  )) >> SI2196_SD_STATUS_RESPONSE_AFCM_LSB                  ) & SI2196_SD_STATUS_RESPONSE_AFCM_MASK                  );
    rsp->sd_status.agcs                  =   (( ( (rspByteBuffer[2]  )) >> SI2196_SD_STATUS_RESPONSE_AGCS_LSB                  ) & SI2196_SD_STATUS_RESPONSE_AGCS_MASK                  );
    rsp->sd_status.sound_mode_detected   =   (( ( (rspByteBuffer[3]  )) >> SI2196_SD_STATUS_RESPONSE_SOUND_MODE_DETECTED_LSB   ) & SI2196_SD_STATUS_RESPONSE_SOUND_MODE_DETECTED_MASK   );
    rsp->sd_status.sound_system_detected =   (( ( (rspByteBuffer[3]  )) >> SI2196_SD_STATUS_RESPONSE_SOUND_SYSTEM_DETECTED_LSB ) & SI2196_SD_STATUS_RESPONSE_SOUND_SYSTEM_DETECTED_MASK );
    rsp->sd_status.sap_detected          =   (( ( (rspByteBuffer[3]  )) >> SI2196_SD_STATUS_RESPONSE_SAP_DETECTED_LSB          ) & SI2196_SD_STATUS_RESPONSE_SAP_DETECTED_MASK          );
    rsp->sd_status.over_dev              =   (( ( (rspByteBuffer[4]  )) >> SI2196_SD_STATUS_RESPONSE_OVER_DEV_LSB              ) & SI2196_SD_STATUS_RESPONSE_OVER_DEV_MASK              );
    rsp->sd_status.sd_agc                =   (( ( (rspByteBuffer[5]  )) >> SI2196_SD_STATUS_RESPONSE_SD_AGC_LSB                ) & SI2196_SD_STATUS_RESPONSE_SD_AGC_MASK                );
    rsp->sd_status.sif_agc               =   (( ( (rspByteBuffer[6]  ) | (rspByteBuffer[7]  << 8 )) >> SI2196_SD_STATUS_RESPONSE_SIF_AGC_LSB               ) & SI2196_SD_STATUS_RESPONSE_SIF_AGC_MASK               );

    return NO_SI2196_ERROR;
}
#endif /* SI2196_SD_STATUS_CMD */
#ifdef SI2196_SD_NICAM_STATUS_CMD
unsigned char si2196_sd_nicam_status(struct i2c_client *si2196,si2196_cmdreplyobj_t *rsp)
{
    unsigned char error_code = 0;
    unsigned char cmdByteBuffer[1];
    unsigned char rspByteBuffer[4];

    error_code = si2196_pollforcts(si2196);
    if (error_code)
    {
        pr_info("%s: poll cts error:%d!!!!\n", __func__, error_code);
        return error_code;
    }
    cmdByteBuffer[0] = SI2196_SD_NICAM_STATUS_CMD;
    if (si2196_writecommandbytes(si2196, 1, cmdByteBuffer) != 1) {
      pr_info("%s:Error writing sd_nicam_status bytes!\n",__func__);
      return ERROR_SI2196_SENDING_COMMAND;
    }

    error_code = si2196_pollforresponse(si2196, 1, 4, rspByteBuffer, &reply);
    if (error_code) {
        pr_info("%s:Error polling.\n",__func__);
      return error_code;
    }
    rsp->sd_nicam_status.mode              =   (( ( (rspByteBuffer[1]  )) >> SI2196_SD_NICAM_STATUS_RESPONSE_MODE_LSB        ) & SI2196_SD_NICAM_STATUS_RESPONSE_MODE_MASK        );
    rsp->sd_nicam_status.mono_backup =   (( ( (rspByteBuffer[1]  )) >> SI2196_SD_NICAM_STATUS_RESPONSE_MONO_BACKUP_LSB ) & SI2196_SD_NICAM_STATUS_RESPONSE_MONO_BACKUP_MASK );
    rsp->sd_nicam_status.rss                  =   (( ( (rspByteBuffer[1]  )) >> SI2196_SD_NICAM_STATUS_RESPONSE_RSS_LSB         ) & SI2196_SD_NICAM_STATUS_RESPONSE_RSS_MASK         );
    rsp->sd_nicam_status.locked             =   (( ( (rspByteBuffer[1]  )) >> SI2196_SD_NICAM_STATUS_RESPONSE_LOCKED_LSB      ) & SI2196_SD_NICAM_STATUS_RESPONSE_LOCKED_MASK      );
    rsp->sd_nicam_status.errors             =   (( ( (rspByteBuffer[2]  ) | (rspByteBuffer[3]  << 8 )) >> SI2196_SD_NICAM_STATUS_RESPONSE_ERRORS_LSB      ) & SI2196_SD_NICAM_STATUS_RESPONSE_ERRORS_MASK      );
    rsp->reply  = &reply;
    return NO_SI2196_ERROR;
}
#endif /* SI2196_SD_NICAM_STATUS_CMD */
#ifdef    SI2196_SD_CARRIER_CNR_CMD
 /*---------------------------------------------------*/
/* SI2196_SD_CARRIER_CNR COMMAND                   */
/*---------------------------------------------------*/
unsigned char si2196_sd_carrier_cnr(struct i2c_client *si2196, si2196_cmdreplyobj_t *rsp)
{
    unsigned char error_code = 0;
    unsigned char cmdByteBuffer[1];
    unsigned char rspByteBuffer[3];
    
    cmdByteBuffer[0] = SI2196_SD_CARRIER_CNR_CMD;
    error_code = si2196_pollforcts(si2196);
    if (error_code)
    {
        pr_info("%s: poll cts error:%d!!!!\n", __func__, error_code);
        return error_code;
    }
    if (si2196_writecommandbytes(si2196, 1, cmdByteBuffer) != 1)
    {
        pr_info("%s:Error writing sd_carrier_cnr bytes!\n",__func__);
        return ERROR_SI2196_SENDING_COMMAND;
    }

    error_code = si2196_pollforresponse(si2196, 1, 3, rspByteBuffer, &reply);
    if (error_code) 
    {
        pr_info("%s: poll for response error:%d!!!!\n", __func__, error_code);
      return error_code;
    }

    rsp->sd_carrier_cnr.primary   =   (( ( (rspByteBuffer[1]  )) >> SI2196_SD_CARRIER_CNR_RESPONSE_PRIMARY_LSB   ) & SI2196_SD_CARRIER_CNR_RESPONSE_PRIMARY_MASK   );
    rsp->sd_carrier_cnr.secondary =   (( ( (rspByteBuffer[2]  )) >> SI2196_SD_CARRIER_CNR_RESPONSE_SECONDARY_LSB ) & SI2196_SD_CARRIER_CNR_RESPONSE_SECONDARY_MASK );
    rsp->reply = &reply;
    return NO_SI2196_ERROR;
}
#endif /* SI2196_SD_CARRIER_CNR_CMD */
#ifdef    SI2196_SD_AFC_CMD
 /*---------------------------------------------------*/
/* SI2196_SD_AFC COMMAND                           */
/*---------------------------------------------------*/
unsigned char si2196_sd_afc(struct i2c_client  *si2196, si2196_cmdreplyobj_t * rsp)
{
    unsigned char error_code = 0;
    unsigned char cmdByteBuffer[1];
    unsigned char rspByteBuffer[3];

    cmdByteBuffer[0] = SI2196_SD_AFC_CMD;
    error_code = si2196_pollforcts(si2196);
    if (error_code)
    {
        pr_info("%s: poll cts error:%d!!!!\n", __func__, error_code);
        return error_code;
    }
    if (si2196_writecommandbytes(si2196, 1, cmdByteBuffer) != 1)
    {
        pr_info("%s:Error writing sd_afc bytes!\n",__func__);
        return ERROR_SI2196_SENDING_COMMAND;
    }

    error_code = si2196_pollforresponse(si2196, 1, 3, rspByteBuffer, &reply);
    if (error_code) 
    {
      pr_info("%s:Error polling SD_AFC response.\n",__func__);
      return error_code;
    }

    rsp->sd_afc.afc = (((( ( (rspByteBuffer[2]  )) >> SI2196_SD_AFC_RESPONSE_AFC_LSB ) & SI2196_SD_AFC_RESPONSE_AFC_MASK) <<SI2196_SD_AFC_RESPONSE_AFC_SHIFT ) >>SI2196_SD_AFC_RESPONSE_AFC_SHIFT );
    rsp->reply = &reply;
    return NO_SI2196_ERROR;
}
#endif /* SI2196_SD_AFC_CMD */
#ifdef    SI2196_SD_STEREO_ID_LVL_CMD
 /*---------------------------------------------------*/
/* SI2196_SD_STEREO_ID_LVL COMMAND                 */
/*---------------------------------------------------*/
unsigned char si2196_sd_stereo_id_lvl (struct i2c_client *si2196,si2196_cmdreplyobj_t *rsp)
{
    unsigned char error_code = 0;
    unsigned char cmdByteBuffer[1];
    unsigned char rspByteBuffer[4];

    cmdByteBuffer[0] = SI2196_SD_STEREO_ID_LVL_CMD;
    error_code = si2196_pollforcts(si2196);
    if (error_code)
    {
        pr_info("%s: poll cts error:%d!!!!\n", __func__, error_code);
        return error_code;
    }
    if (si2196_writecommandbytes(si2196, 1, cmdByteBuffer) != 1)
    {
        pr_info("%s:Error writing SD_STEREO_ID_LVL bytes!\n",__func__);
        return ERROR_SI2196_SENDING_COMMAND;
    }

    error_code = si2196_pollforresponse(si2196, 1, 4, rspByteBuffer, &reply);
    if (error_code) 
    {
      pr_info("%s:Error polling SD_AFC response.\n",__func__);
      return error_code;
    }
    rsp->sd_stereo_id_lvl.id_lvl =   (( ( (rspByteBuffer[2]  ) | (rspByteBuffer[3]  << 8 )) >> SI2196_SD_STEREO_ID_LVL_RESPONSE_ID_LVL_LSB ) & SI2196_SD_STEREO_ID_LVL_RESPONSE_ID_LVL_MASK );
    rsp->reply = &reply;
    return NO_SI2196_ERROR;
}
#endif /* SI2196_SD_STEREO_ID_LVL_CMD */
#ifdef    SI2196_SD_DUAL_MONO_ID_LVL_CMD
 /*---------------------------------------------------*/
/* SI2196_SD_DUAL_MONO_ID_LVL COMMAND              */
/*---------------------------------------------------*/
unsigned char si2196_sd_dual_mono_id_lvl (struct i2c_client *si2196, si2196_cmdreplyobj_t *rsp)
{
    unsigned char error_code = 0;
    unsigned char cmdByteBuffer[1];
    unsigned char rspByteBuffer[4];
    
    cmdByteBuffer[0] = SI2196_SD_DUAL_MONO_ID_LVL_CMD;
    error_code = si2196_pollforcts(si2196);
    if (error_code)
    {
        pr_info("%s: poll cts error:%d!!!!\n", __func__, error_code);
        return error_code;
    }
    if (si2196_writecommandbytes(si2196, 1, cmdByteBuffer) != 1)
    {
        pr_info("%s:Error writing SI2196_SD_DUAL_MONO_ID_LVL bytes!\n",__func__);
        return ERROR_SI2196_SENDING_COMMAND;
    }

    error_code = si2196_pollforresponse(si2196, 1, 4, rspByteBuffer, &reply);
    if (error_code) 
    {
      pr_info("%s:Error polling SI2196_SD_DUAL_MONO_ID_LVL response.\n",__func__);
      return error_code;
    }
    rsp->sd_dual_mono_id_lvl.id_lvl =   (( ( (rspByteBuffer[2]  ) | (rspByteBuffer[3]  << 8 )) >> SI2196_SD_DUAL_MONO_ID_LVL_RESPONSE_ID_LVL_LSB ) & SI2196_SD_DUAL_MONO_ID_LVL_RESPONSE_ID_LVL_MASK );
    rsp->reply = &reply;
    return NO_SI2196_ERROR;
}
#endif /* SI2196_SD_DUAL_MONO_ID_LVL_CMD */
#ifdef    SI2196_SD_CASD_CMD
 /*---------------------------------------------------*/
/* SI2196_SD_CASD COMMAND                          */
/*---------------------------------------------------*/
unsigned char si2196_sd_casd(struct i2c_client *si2196, si2196_cmdreplyobj_t *rsp)
{
    unsigned char error_code = 0;
    unsigned char cmdByteBuffer[1];
    unsigned char rspByteBuffer[3];

    cmdByteBuffer[0] = SI2196_SD_CASD_CMD;
    error_code = si2196_pollforcts(si2196);
    if (error_code)
    {
        pr_info("%s: poll cts error:%d!!!!\n", __func__, error_code);
        return error_code;
    }
    if (si2196_writecommandbytes(si2196, 1, cmdByteBuffer) != 1)
    {
        pr_info("%s:Error writing SI2196_SD_CASD_CMD bytes!\n",__func__);
        return ERROR_SI2196_SENDING_COMMAND;
    }

    error_code = si2196_pollforresponse(si2196, 1, 3, rspByteBuffer, &reply);
    if (error_code) 
    {
      pr_info("%s:Error polling SI2196_SD_DUAL_MONO_ID_LVL response.\n",__func__);
      return error_code;
    }
    rsp->sd_casd.casd =   (( ( (rspByteBuffer[2]  )) >> SI2196_SD_CASD_RESPONSE_CASD_LSB ) & SI2196_SD_CASD_RESPONSE_CASD_MASK );
    rsp->reply = &reply;
    return NO_SI2196_ERROR;
}
#endif /* SI2196_SD_CASD_CMD */


#ifdef SI2196_CONFIG_PINS_CMD
/*---------------------------------------------------*/
/* SI2196_CONFIG_PINS COMMAND                      */
/*---------------------------------------------------*/
static unsigned char si2196_config_pins(struct i2c_client *si2196,
        unsigned char   gpio1_mode,
        unsigned char   gpio1_read,
        unsigned char   gpio2_mode,
        unsigned char   gpio2_read,
        unsigned char   gpio3_mode,
        unsigned char   gpio3_read,
        unsigned char   bclk1_mode,
        unsigned char   bclk1_read,
        unsigned char   xout_mode,
        si2196_cmdreplyobj_t *rsp)
{
    unsigned char error_code = 0;
    unsigned char cmdbytebuffer[6];
    unsigned char rspbytebuffer[6];

#ifdef DEBUG_RANGE_CHECK
    if ( (gpio1_mode > SI2196_CONFIG_PINS_CMD_GPIO1_MODE_MAX)
            || (gpio1_read > SI2196_CONFIG_PINS_CMD_GPIO1_READ_MAX)
            || (gpio2_mode > SI2196_CONFIG_PINS_CMD_GPIO2_MODE_MAX)
            || (gpio2_read > SI2196_CONFIG_PINS_CMD_GPIO2_READ_MAX)
            || (gpio3_mode > SI2196_CONFIG_PINS_CMD_GPIO3_MODE_MAX)
            || (gpio3_read > SI2196_CONFIG_PINS_CMD_GPIO3_READ_MAX)
            || (bclk1_mode > SI2196_CONFIG_PINS_CMD_BCLK1_MODE_MAX)
            || (bclk1_read > SI2196_CONFIG_PINS_CMD_BCLK1_READ_MAX)
            || (xout_mode  > SI2196_CONFIG_PINS_CMD_XOUT_MODE_MAX ) )
        return ERROR_SI2196_PARAMETER_OUT_OF_RANGE;
#endif /* DEBUG_RANGE_CHECK */

    error_code = si2196_pollforcts(si2196);
    if (error_code)
    {
        pr_info("%s: poll cts error:%d!!!!\n", __func__, error_code);
        goto exit;
    }

    cmdbytebuffer[0] = SI2196_CONFIG_PINS_CMD;
    cmdbytebuffer[1] = (unsigned char) ( ( gpio1_mode & SI2196_CONFIG_PINS_CMD_GPIO1_MODE_MASK ) << SI2196_CONFIG_PINS_CMD_GPIO1_MODE_LSB|
                                         ( gpio1_read & SI2196_CONFIG_PINS_CMD_GPIO1_READ_MASK ) << SI2196_CONFIG_PINS_CMD_GPIO1_READ_LSB);
    cmdbytebuffer[2] = (unsigned char) ( ( gpio2_mode & SI2196_CONFIG_PINS_CMD_GPIO2_MODE_MASK ) << SI2196_CONFIG_PINS_CMD_GPIO2_MODE_LSB|
                                         ( gpio2_read & SI2196_CONFIG_PINS_CMD_GPIO2_READ_MASK ) << SI2196_CONFIG_PINS_CMD_GPIO2_READ_LSB);
    cmdbytebuffer[3] = (unsigned char) ( ( gpio3_mode & SI2196_CONFIG_PINS_CMD_GPIO3_MODE_MASK ) << SI2196_CONFIG_PINS_CMD_GPIO3_MODE_LSB|
                                         ( gpio3_read & SI2196_CONFIG_PINS_CMD_GPIO3_READ_MASK ) << SI2196_CONFIG_PINS_CMD_GPIO3_READ_LSB);
    cmdbytebuffer[4] = (unsigned char) ( ( bclk1_mode & SI2196_CONFIG_PINS_CMD_BCLK1_MODE_MASK ) << SI2196_CONFIG_PINS_CMD_BCLK1_MODE_LSB|
                                         ( bclk1_read & SI2196_CONFIG_PINS_CMD_BCLK1_READ_MASK ) << SI2196_CONFIG_PINS_CMD_BCLK1_READ_LSB);
    cmdbytebuffer[5] = (unsigned char) ( ( xout_mode  & SI2196_CONFIG_PINS_CMD_XOUT_MODE_MASK  ) << SI2196_CONFIG_PINS_CMD_XOUT_MODE_LSB );

    if (si2196_writecommandbytes(si2196, 6, cmdbytebuffer) != 6)
    {
        error_code = ERROR_SI2196_SENDING_COMMAND;
        pr_info("%s: write command byte error:%d!!!!\n", __func__, error_code);
    }

    if (!error_code)
    {
        error_code = si2196_pollforresponse(si2196, 1, 6, rspbytebuffer, &reply);
        if (error_code)
            pr_info("%s: poll response error:%d!!!!\n", __func__, error_code);
        rsp->reply = &reply;
        if (!error_code)
        {
            rsp->config_pins.gpio1_mode  =   (( ( (rspbytebuffer[1]  )) >> SI2196_CONFIG_PINS_RESPONSE_GPIO1_MODE_LSB  ) & SI2196_CONFIG_PINS_RESPONSE_GPIO1_MODE_MASK  );
            rsp->config_pins.gpio1_state =   (( ( (rspbytebuffer[1]  )) >> SI2196_CONFIG_PINS_RESPONSE_GPIO1_STATE_LSB ) & SI2196_CONFIG_PINS_RESPONSE_GPIO1_STATE_MASK );
            rsp->config_pins.gpio2_mode  =   (( ( (rspbytebuffer[2]  )) >> SI2196_CONFIG_PINS_RESPONSE_GPIO2_MODE_LSB  ) & SI2196_CONFIG_PINS_RESPONSE_GPIO2_MODE_MASK  );
            rsp->config_pins.gpio2_state =   (( ( (rspbytebuffer[2]  )) >> SI2196_CONFIG_PINS_RESPONSE_GPIO2_STATE_LSB ) & SI2196_CONFIG_PINS_RESPONSE_GPIO2_STATE_MASK );
            rsp->config_pins.gpio3_mode  =   (( ( (rspbytebuffer[3]  )) >> SI2196_CONFIG_PINS_RESPONSE_GPIO3_MODE_LSB  ) & SI2196_CONFIG_PINS_RESPONSE_GPIO3_MODE_MASK  );
            rsp->config_pins.gpio3_state =   (( ( (rspbytebuffer[3]  )) >> SI2196_CONFIG_PINS_RESPONSE_GPIO3_STATE_LSB ) & SI2196_CONFIG_PINS_RESPONSE_GPIO3_STATE_MASK );
            rsp->config_pins.bclk1_mode  =   (( ( (rspbytebuffer[4]  )) >> SI2196_CONFIG_PINS_RESPONSE_BCLK1_MODE_LSB  ) & SI2196_CONFIG_PINS_RESPONSE_BCLK1_MODE_MASK  );
            rsp->config_pins.bclk1_state =   (( ( (rspbytebuffer[4]  )) >> SI2196_CONFIG_PINS_RESPONSE_BCLK1_STATE_LSB ) & SI2196_CONFIG_PINS_RESPONSE_BCLK1_STATE_MASK );
            rsp->config_pins.xout_mode   =   (( ( (rspbytebuffer[5]  )) >> SI2196_CONFIG_PINS_RESPONSE_XOUT_MODE_LSB   ) & SI2196_CONFIG_PINS_RESPONSE_XOUT_MODE_MASK   );
        }
    }
exit:
    return error_code;
}
#endif /* SI2196_CONFIG_PINS_CMD */
#ifdef SI2196_EXIT_BOOTLOADER_CMD
/*---------------------------------------------------*/
/* SI2196_EXIT_BOOTLOADER COMMAND                  */
/*---------------------------------------------------*/
static unsigned char si2196_exit_bootloader(struct i2c_client *si2196,
        unsigned char   func,
        unsigned char   ctsien,
        si2196_cmdreplyobj_t *rsp)
{
    unsigned char error_code = 0;
    unsigned char cmdbytebuffer[2];
    unsigned char rspbytebuffer[1];

#ifdef DEBUG_RANGE_CHECK
    if ( (func   > SI2196_EXIT_BOOTLOADER_CMD_FUNC_MAX  )
            || (ctsien > SI2196_EXIT_BOOTLOADER_CMD_CTSIEN_MAX) )
        return ERROR_SI2196_PARAMETER_OUT_OF_RANGE;
#endif /* DEBUG_RANGE_CHECK */

    error_code = si2196_pollforcts(si2196);
    if (error_code) goto exit;

    cmdbytebuffer[0] = SI2196_EXIT_BOOTLOADER_CMD;
    cmdbytebuffer[1] = (unsigned char) ( ( func   & SI2196_EXIT_BOOTLOADER_CMD_FUNC_MASK   ) << SI2196_EXIT_BOOTLOADER_CMD_FUNC_LSB  |
                                         ( ctsien & SI2196_EXIT_BOOTLOADER_CMD_CTSIEN_MASK ) << SI2196_EXIT_BOOTLOADER_CMD_CTSIEN_LSB);

    if (si2196_writecommandbytes(si2196, 2, cmdbytebuffer) != 2) 
        error_code = ERROR_SI2196_SENDING_COMMAND;

    if (!error_code)
    {
        error_code = si2196_pollforresponse(si2196, 1, 1, rspbytebuffer, &reply);
        if (error_code)
            pr_info("%s: poll response error:%d!!!!\n", __func__, error_code);
        rsp->reply = &reply;
    }
exit:
    return error_code;
}
#endif /* SI2196_EXIT_BOOTLOADER_CMD */
#ifdef SI2196_FINE_TUNE_CMD
/*---------------------------------------------------*/
/* SI2196_FINE_TUNE COMMAND                        */
/*---------------------------------------------------*/
unsigned char si2196_fine_tune(struct i2c_client *si2196,
        unsigned char   reserved,
        int   offset_500hz,
        si2196_cmdreplyobj_t *rsp)
{
    unsigned char error_code = 0;
    unsigned char cmdbytebuffer[4];
    unsigned char rspbytebuffer[1];

#ifdef DEBUG_RANGE_CHECK
    if ( (reserved     > SI2196_FINE_TUNE_CMD_RESERVED_MAX    )
            || (offset_500hz > SI2196_FINE_TUNE_CMD_OFFSET_500HZ_MAX)  || (offset_500hz < SI2196_FINE_TUNE_CMD_OFFSET_500HZ_MIN) )
        return ERROR_SI2196_PARAMETER_OUT_OF_RANGE;
#endif /* DEBUG_RANGE_CHECK */

    error_code = si2196_pollforcts(si2196);
    if (error_code)
    {
        pr_info("%s: poll cts error:%d!!!!\n", __func__, error_code);
        goto exit;
    }

    cmdbytebuffer[0] = SI2196_FINE_TUNE_CMD;
    cmdbytebuffer[1] = (unsigned char) ( ( reserved     & SI2196_FINE_TUNE_CMD_RESERVED_MASK     ) << SI2196_FINE_TUNE_CMD_RESERVED_LSB    );
    cmdbytebuffer[2] = (unsigned char) ( ( offset_500hz & SI2196_FINE_TUNE_CMD_OFFSET_500HZ_MASK ) << SI2196_FINE_TUNE_CMD_OFFSET_500HZ_LSB);
    cmdbytebuffer[3] = (unsigned char) ((( offset_500hz & SI2196_FINE_TUNE_CMD_OFFSET_500HZ_MASK ) << SI2196_FINE_TUNE_CMD_OFFSET_500HZ_LSB)>>8);

    if (si2196_writecommandbytes(si2196, 4, cmdbytebuffer) != 4)
    {
        error_code = ERROR_SI2196_SENDING_COMMAND;
        pr_info("%s: write command byte error:%d!!!!\n", __func__, error_code);
    }

    if (!error_code)
    {
        error_code = si2196_pollforresponse(si2196, 1, 1, rspbytebuffer, &reply);
        if (error_code)
            pr_info("%s: poll response error:%d!!!!\n", __func__, error_code);
        rsp->reply = &reply;
    }
exit:
    return error_code;
}
#endif /* SI2196_FINE_TUNE_CMD */
#ifdef SI2196_GET_PROPERTY_CMD
/*---------------------------------------------------*/
/* SI2196_GET_PROPERTY COMMAND                     */
/*---------------------------------------------------*/
unsigned char si2196_get_property(struct i2c_client *si2196,
        unsigned char   reserved,
        unsigned int    prop,
        si2196_cmdreplyobj_t *rsp)
{
    unsigned char error_code = 0;
    unsigned char cmdbytebuffer[4];
    unsigned char rspbytebuffer[4];

#ifdef DEBUG_RANGE_CHECK
    if ( (reserved > SI2196_GET_PROPERTY_CMD_RESERVED_MAX) )
        return ERROR_SI2196_PARAMETER_OUT_OF_RANGE;
#endif /* DEBUG_RANGE_CHECK */

    error_code = si2196_pollforcts(si2196);
    if (error_code)
    {
        pr_info("%s: poll cts error:%d!!!!\n", __func__, error_code);
        goto exit;
    }

    cmdbytebuffer[0] = SI2196_GET_PROPERTY_CMD;
    cmdbytebuffer[1] = (unsigned char) ( ( reserved & SI2196_GET_PROPERTY_CMD_RESERVED_MASK ) << SI2196_GET_PROPERTY_CMD_RESERVED_LSB);
    cmdbytebuffer[2] = (unsigned char) ( ( prop     & SI2196_GET_PROPERTY_CMD_PROP_MASK     ) << SI2196_GET_PROPERTY_CMD_PROP_LSB    );
    cmdbytebuffer[3] = (unsigned char) ((( prop     & SI2196_GET_PROPERTY_CMD_PROP_MASK     ) << SI2196_GET_PROPERTY_CMD_PROP_LSB    )>>8);

    if (si2196_writecommandbytes(si2196, 4, cmdbytebuffer) != 4)
    {
        error_code = ERROR_SI2196_SENDING_COMMAND;
        pr_info("%s: write command byte error:%d!!!!\n", __func__, error_code);
    }

    if (!error_code)
    {
        error_code = si2196_pollforresponse(si2196, 1, 4, rspbytebuffer, &reply);
        if (error_code)
            pr_info("%s: poll response error:%d!!!!\n", __func__, error_code);
        rsp->reply = &reply;
        if (!error_code)
        {
            rsp->get_property.reserved =   (( ( (rspbytebuffer[1]  )) >> SI2196_GET_PROPERTY_RESPONSE_RESERVED_LSB ) & SI2196_GET_PROPERTY_RESPONSE_RESERVED_MASK );
            rsp->get_property.data     =   (( ( (rspbytebuffer[2]  ) | (rspbytebuffer[3]  << 8 )) >> SI2196_GET_PROPERTY_RESPONSE_DATA_LSB     ) & SI2196_GET_PROPERTY_RESPONSE_DATA_MASK     );
        }
    }
exit:
    return error_code;
}
#endif /* SI2196_GET_PROPERTY_CMD */
#ifdef SI2196_GET_REV_CMD
/*---------------------------------------------------*/
/* SI2196_GET_REV COMMAND                          */
/*---------------------------------------------------*/
static unsigned char si2196_get_rev(struct i2c_client *si2196,
        si2196_cmdreplyobj_t *rsp)
{
    unsigned char error_code = 0;
    unsigned char cmdbytebuffer[1];
    unsigned char rspbytebuffer[10];

    error_code = si2196_pollforcts(si2196);
    if (error_code) goto exit;

    cmdbytebuffer[0] = SI2196_GET_REV_CMD;

    if (si2196_writecommandbytes(si2196, 1, cmdbytebuffer) != 1) error_code = ERROR_SI2196_SENDING_COMMAND;

    if (!error_code)
    {
        error_code = si2196_pollforresponse(si2196, 1, 10, rspbytebuffer, &reply);
        if (error_code)
            pr_info("%s: poll response error:%d!!!!\n", __func__, error_code);
        rsp->reply = &reply;
        if (!error_code)
        {
            rsp->get_rev.pn       =   (( ( (rspbytebuffer[1]  )) >> SI2196_GET_REV_RESPONSE_PN_LSB       ) & SI2196_GET_REV_RESPONSE_PN_MASK       );
            rsp->get_rev.fwmajor  =   (( ( (rspbytebuffer[2]  )) >> SI2196_GET_REV_RESPONSE_FWMAJOR_LSB  ) & SI2196_GET_REV_RESPONSE_FWMAJOR_MASK  );
            rsp->get_rev.fwminor  =   (( ( (rspbytebuffer[3]  )) >> SI2196_GET_REV_RESPONSE_FWMINOR_LSB  ) & SI2196_GET_REV_RESPONSE_FWMINOR_MASK  );
            rsp->get_rev.patch    =   (( ( (rspbytebuffer[4]  ) | (rspbytebuffer[5]  << 8 )) >> SI2196_GET_REV_RESPONSE_PATCH_LSB    ) & SI2196_GET_REV_RESPONSE_PATCH_MASK    );
            rsp->get_rev.cmpmajor =   (( ( (rspbytebuffer[6]  )) >> SI2196_GET_REV_RESPONSE_CMPMAJOR_LSB ) & SI2196_GET_REV_RESPONSE_CMPMAJOR_MASK );
            rsp->get_rev.cmpminor =   (( ( (rspbytebuffer[7]  )) >> SI2196_GET_REV_RESPONSE_CMPMINOR_LSB ) & SI2196_GET_REV_RESPONSE_CMPMINOR_MASK );
            rsp->get_rev.cmpbuild =   (( ( (rspbytebuffer[8]  )) >> SI2196_GET_REV_RESPONSE_CMPBUILD_LSB ) & SI2196_GET_REV_RESPONSE_CMPBUILD_MASK );
            rsp->get_rev.chiprev  =   (( ( (rspbytebuffer[9]  )) >> SI2196_GET_REV_RESPONSE_CHIPREV_LSB  ) & SI2196_GET_REV_RESPONSE_CHIPREV_MASK  );
        }
    }
exit:
    return error_code;
}
#endif /* SI2196_GET_REV_CMD */
#ifdef SI2196_PART_INFO_CMD
/*---------------------------------------------------*/
/* SI2196_PART_INFO COMMAND                        */
/*---------------------------------------------------*/
static unsigned char si2196_part_info(struct i2c_client *si2196,
        si2196_cmdreplyobj_t *rsp)
{
    unsigned char error_code = 0;
    unsigned char cmdbytebuffer[1];
    unsigned char rspbytebuffer[13];

    error_code = si2196_pollforcts(si2196);
    if (error_code) goto exit;

    cmdbytebuffer[0] = SI2196_PART_INFO_CMD;

    if (si2196_writecommandbytes(si2196, 1, cmdbytebuffer) != 1) error_code = ERROR_SI2196_SENDING_COMMAND;

    if (!error_code)
    {
        error_code = si2196_pollforresponse(si2196, 1, 13, rspbytebuffer, &reply);
        if (error_code)
            pr_info("%s: poll response error:%d!!!!\n", __func__, error_code);
        rsp->reply = &reply;
        if (!error_code)
        {
            rsp->part_info.chiprev  =   (( ( (rspbytebuffer[1]  )) >> SI2196_PART_INFO_RESPONSE_CHIPREV_LSB  ) & SI2196_PART_INFO_RESPONSE_CHIPREV_MASK  );
            rsp->part_info.part     =   (( ( (rspbytebuffer[2]  )) >> SI2196_PART_INFO_RESPONSE_PART_LSB     ) & SI2196_PART_INFO_RESPONSE_PART_MASK     );
            rsp->part_info.pmajor   =   (( ( (rspbytebuffer[3]  )) >> SI2196_PART_INFO_RESPONSE_PMAJOR_LSB   ) & SI2196_PART_INFO_RESPONSE_PMAJOR_MASK   );
            rsp->part_info.pminor   =   (( ( (rspbytebuffer[4]  )) >> SI2196_PART_INFO_RESPONSE_PMINOR_LSB   ) & SI2196_PART_INFO_RESPONSE_PMINOR_MASK   );
            rsp->part_info.pbuild   =   (( ( (rspbytebuffer[5]  )) >> SI2196_PART_INFO_RESPONSE_PBUILD_LSB   ) & SI2196_PART_INFO_RESPONSE_PBUILD_MASK   );
            rsp->part_info.reserved =   (( ( (rspbytebuffer[6]  ) | (rspbytebuffer[7]  << 8 )) >> SI2196_PART_INFO_RESPONSE_RESERVED_LSB ) & SI2196_PART_INFO_RESPONSE_RESERVED_MASK );
            rsp->part_info.serial   =   (( ( (rspbytebuffer[8]  ) | (rspbytebuffer[9]  << 8 ) | (rspbytebuffer[10] << 16 ) | (rspbytebuffer[11] << 24 )) >> SI2196_PART_INFO_RESPONSE_SERIAL_LSB   ) & SI2196_PART_INFO_RESPONSE_SERIAL_MASK   );
            rsp->part_info.romid    =   (( ( (rspbytebuffer[12] )) >> SI2196_PART_INFO_RESPONSE_ROMID_LSB    ) & SI2196_PART_INFO_RESPONSE_ROMID_MASK    );
        }
    }
exit:
    return error_code;
}
#endif /* SI2196_PART_INFO_CMD */
#ifdef SI2196_POWER_DOWN_CMD
/*---------------------------------------------------*/
/* SI2196_POWER_DOWN COMMAND                       */
/*---------------------------------------------------*/
unsigned char si2196_power_down(struct i2c_client *si2196,
        si2196_cmdreplyobj_t *rsp)
{
    unsigned char error_code = 0;
    unsigned char cmdbytebuffer[1];
    unsigned char rspbytebuffer[1];

    error_code = si2196_pollforcts(si2196);
    if (error_code)
    {
        pr_info("%s: poll cts error:%d!!!!\n", __func__, error_code);
        goto exit;
    }

    cmdbytebuffer[0] = SI2196_POWER_DOWN_CMD;

    if (si2196_writecommandbytes(si2196, 1, cmdbytebuffer) != 1)
    {
        error_code = ERROR_SI2196_SENDING_COMMAND;
        pr_info("%s: write command byte error:%d!!!!\n", __func__, error_code);
    }

    if (!error_code)
    {
        error_code = si2196_pollforresponse(si2196, 1, 1, rspbytebuffer, &reply);
        if (error_code)
            pr_info("%s: poll response error:%d!!!!\n", __func__, error_code);
        rsp->reply = &reply;
    }
exit:
    return error_code;
}
#endif /* SI2196_POWER_DOWN_CMD */
#ifdef SI2196_POWER_UP_CMD
/*---------------------------------------------------*/
/* SI2196_POWER_UP COMMAND                         */
/*---------------------------------------------------*/
static unsigned char si2196_power_up(struct i2c_client *si2196,
        unsigned char   subcode,
        unsigned char   reserved1,
        unsigned char   reserved2,
        unsigned char   reserved3,
        unsigned char   clock_mode,
        unsigned char   clock_freq,
        unsigned char   addr_mode,
        unsigned char   func,
        unsigned char   ctsien,
        unsigned char   wake_up,
        si2196_cmdreplyobj_t *rsp)
{
    unsigned char error_code = 0;
    unsigned char cmdbytebuffer[9];
    unsigned char rspbytebuffer[1];

#ifdef DEBUG_RANGE_CHECK
    if ( (subcode    > SI2196_POWER_UP_CMD_SUBCODE_MAX   )  || (subcode    < SI2196_POWER_UP_CMD_SUBCODE_MIN   )
            || (reserved1  > SI2196_POWER_UP_CMD_RESERVED1_MAX )  || (reserved1  < SI2196_POWER_UP_CMD_RESERVED1_MIN )
            || (reserved2  > SI2196_POWER_UP_CMD_RESERVED2_MAX )
            || (reserved3  > SI2196_POWER_UP_CMD_RESERVED3_MAX )
            || (clock_mode > SI2196_POWER_UP_CMD_CLOCK_MODE_MAX)  || (clock_mode < SI2196_POWER_UP_CMD_CLOCK_MODE_MIN)
            || (clock_freq > SI2196_POWER_UP_CMD_CLOCK_FREQ_MAX)
            || (addr_mode  > SI2196_POWER_UP_CMD_ADDR_MODE_MAX )
            || (func       > SI2196_POWER_UP_CMD_FUNC_MAX      )
            || (ctsien     > SI2196_POWER_UP_CMD_CTSIEN_MAX    )
            || (wake_up    > SI2196_POWER_UP_CMD_WAKE_UP_MAX   )  || (wake_up    < SI2196_POWER_UP_CMD_WAKE_UP_MIN   ) )

    {
        if(SI2196_DEBUG)
            pr_info("%s: DEBUG_RANGE_CHECK!!!!\n", __func__);
        return ERROR_SI2196_PARAMETER_OUT_OF_RANGE;
    }
#endif /* DEBUG_RANGE_CHECK */

    error_code = si2196_pollforcts(si2196);
    if (error_code)
    {
        pr_info("%s: poll cts error:%d!!!!\n", __func__, error_code);
        goto exit;
    }

    cmdbytebuffer[0] = SI2196_POWER_UP_CMD;
    cmdbytebuffer[1] = (unsigned char) ( ( subcode    & SI2196_POWER_UP_CMD_SUBCODE_MASK    ) << SI2196_POWER_UP_CMD_SUBCODE_LSB   );
    cmdbytebuffer[2] = (unsigned char) ( ( reserved1  & SI2196_POWER_UP_CMD_RESERVED1_MASK  ) << SI2196_POWER_UP_CMD_RESERVED1_LSB );
    cmdbytebuffer[3] = (unsigned char) ( ( reserved2  & SI2196_POWER_UP_CMD_RESERVED2_MASK  ) << SI2196_POWER_UP_CMD_RESERVED2_LSB );
    cmdbytebuffer[4] = (unsigned char) ( ( reserved3  & SI2196_POWER_UP_CMD_RESERVED3_MASK  ) << SI2196_POWER_UP_CMD_RESERVED3_LSB );
    cmdbytebuffer[5] = (unsigned char) ( ( clock_mode & SI2196_POWER_UP_CMD_CLOCK_MODE_MASK ) << SI2196_POWER_UP_CMD_CLOCK_MODE_LSB|
                                                                ( clock_freq & SI2196_POWER_UP_CMD_CLOCK_FREQ_MASK ) << SI2196_POWER_UP_CMD_CLOCK_FREQ_LSB);
    cmdbytebuffer[6] = (unsigned char) ( ( addr_mode  & SI2196_POWER_UP_CMD_ADDR_MODE_MASK  ) << SI2196_POWER_UP_CMD_ADDR_MODE_LSB );
    cmdbytebuffer[7] = (unsigned char) ( ( func       & SI2196_POWER_UP_CMD_FUNC_MASK       ) << SI2196_POWER_UP_CMD_FUNC_LSB      |
                                                                ( ctsien     & SI2196_POWER_UP_CMD_CTSIEN_MASK     ) << SI2196_POWER_UP_CMD_CTSIEN_LSB    );
    cmdbytebuffer[8] = (unsigned char) ( ( wake_up    & SI2196_POWER_UP_CMD_WAKE_UP_MASK    ) << SI2196_POWER_UP_CMD_WAKE_UP_LSB   );

    if (si2196_writecommandbytes(si2196, 9, cmdbytebuffer) != 9)
    {
        error_code = ERROR_SI2196_SENDING_COMMAND;
        if (error_code)
            pr_info("%s: si2196_writecommandbytes!!!!\n", __func__);
    }

    if (!error_code)
    {
        error_code = si2196_pollforresponse(si2196, 1, 1, rspbytebuffer, &reply);
        if (error_code)
            pr_info("%s: poll response error:%d!!!!\n", __func__, error_code);

        rsp->reply = &reply;
    }
exit:
    return error_code;
}
#endif /* SI2196_POWER_UP_CMD */
#ifdef SI2196_SD_ADAC_POWER_UP_CMD
unsigned char si2196_adac_power_up(struct i2c_client *si2196, unsigned char duration, si2196_cmdreplyobj_t *rsp)
{
    unsigned char error_code = 0;
    unsigned char cmdbytebuffer[2];
    unsigned char rspbytebuffer[1];

    error_code = si2196_pollforcts(si2196);
    if (error_code)
    {
        pr_info("%s: poll cts error:%d!!!!\n", __func__, error_code);
        goto exit;
    }

    cmdbytebuffer[0] = SI2196_SD_ADAC_POWER_UP_CMD;
    cmdbytebuffer[1] = duration;
    if (si2196_writecommandbytes(si2196, 2, cmdbytebuffer) != 2)
    {
        error_code = ERROR_SI2196_SENDING_COMMAND;
        pr_info("%s: write command byte error:%d!!!!\n", __func__, error_code);
    }

    if (!error_code)
    {
        error_code = si2196_pollforresponse(si2196, 1, 1, rspbytebuffer, &reply);
        if (error_code)
            pr_info("%s: poll response error:%d!!!!\n", __func__, error_code);
        rsp->reply = &reply;
    }
exit:
    return error_code;
}
#endif
#ifdef SI2196_SET_PROPERTY_CMD
/*---------------------------------------------------*/
/* SI2196_SET_PROPERTY COMMAND                     */
/*---------------------------------------------------*/
unsigned char si2196_set_property(struct i2c_client *si2196,
        unsigned char   reserved,
        unsigned int    prop,
        unsigned int    data,
        si2196_cmdreplyobj_t *rsp)
{
    unsigned char error_code = 0;
    unsigned char cmdbytebuffer[6];
    unsigned char rspbytebuffer[4];

    error_code = si2196_pollforcts(si2196);
    if (error_code)
    {
        pr_info("%s: poll cts error:%d!!!!\n", __func__, error_code);
        goto exit;
    }

    cmdbytebuffer[0] = SI2196_SET_PROPERTY_CMD;
    cmdbytebuffer[1] = (unsigned char) ( ( reserved & SI2196_SET_PROPERTY_CMD_RESERVED_MASK ) << SI2196_SET_PROPERTY_CMD_RESERVED_LSB);
    cmdbytebuffer[2] = (unsigned char) ( ( prop     & SI2196_SET_PROPERTY_CMD_PROP_MASK     ) << SI2196_SET_PROPERTY_CMD_PROP_LSB    );
    cmdbytebuffer[3] = (unsigned char) ((( prop     & SI2196_SET_PROPERTY_CMD_PROP_MASK     ) << SI2196_SET_PROPERTY_CMD_PROP_LSB    )>>8);
    cmdbytebuffer[4] = (unsigned char) ( ( data     & SI2196_SET_PROPERTY_CMD_DATA_MASK     ) << SI2196_SET_PROPERTY_CMD_DATA_LSB    );
    cmdbytebuffer[5] = (unsigned char) ((( data     & SI2196_SET_PROPERTY_CMD_DATA_MASK     ) << SI2196_SET_PROPERTY_CMD_DATA_LSB    )>>8);

    if (si2196_writecommandbytes(si2196, 6, cmdbytebuffer) != 6)
    {
        error_code = ERROR_SI2196_SENDING_COMMAND;
        pr_info("%s: write command byte error:%d!!!!\n", __func__, error_code);
    }

    if (!error_code)
    {
        error_code = si2196_pollforresponse(si2196, 1, 4, rspbytebuffer, &reply);
        if (error_code)
            pr_info("%s: poll response error:%d!!!!\n", __func__, error_code);
        rsp->reply = &reply;
        if (!error_code)
        {
            rsp->set_property.reserved =   (( ( (rspbytebuffer[1]  )) >> SI2196_SET_PROPERTY_RESPONSE_RESERVED_LSB ) & SI2196_SET_PROPERTY_RESPONSE_RESERVED_MASK );
            rsp->set_property.data     =   (( ( (rspbytebuffer[2]  ) | (rspbytebuffer[3]  << 8 )) >> SI2196_SET_PROPERTY_RESPONSE_DATA_LSB     ) & SI2196_SET_PROPERTY_RESPONSE_DATA_MASK     );
        }
    }
exit:
    return error_code;
}
#endif /* SI2196_SET_PROPERTY_CMD */
#ifdef SI2196_STANDBY_CMD
/*---------------------------------------------------*/
/* SI2196_STANDBY COMMAND                          */
/*---------------------------------------------------*/
static unsigned char si2196_standby(struct i2c_client *si2196,
        si2196_cmdreplyobj_t *rsp)
{
    unsigned char error_code = 0;
    unsigned char cmdbytebuffer[1];
    unsigned char rspbytebuffer[1];

    error_code = si2196_pollforcts(si2196);
    if (error_code)
    {
        pr_info("%s: poll cts error:%d!!!!\n", __func__, error_code);
        goto exit;
    }

    cmdbytebuffer[0] = SI2196_STANDBY_CMD;

    if (si2196_writecommandbytes(si2196, 1, cmdbytebuffer) != 1)
    {
        error_code = ERROR_SI2196_SENDING_COMMAND;
        pr_info("%s: write command byte error:%d!!!!\n", __func__, error_code);
    }

    if (!error_code)
    {
        error_code = si2196_pollforresponse(si2196, 1, 1, rspbytebuffer, &reply);
        if (error_code)
            pr_info("%s: poll response error:%d!!!!\n", __func__, error_code);
        rsp->reply = &reply;
    }
exit:
    return error_code;
}
#endif /* SI2196_STANDBY_CMD */
#ifdef SI2196_TUNER_STATUS_CMD
/*---------------------------------------------------*/
/* SI2196_TUNER_STATUS COMMAND                     */
/*---------------------------------------------------*/
unsigned char si2196_tuner_status(struct i2c_client *si2196,
        unsigned char   intack,
        si2196_cmdreplyobj_t *rsp)
{
    unsigned char error_code = 0;
    unsigned char cmdbytebuffer[2];
    unsigned char rspbytebuffer[12];

#ifdef DEBUG_RANGE_CHECK
    if ( (intack > SI2196_TUNER_STATUS_CMD_INTACK_MAX) )
        return ERROR_SI2196_PARAMETER_OUT_OF_RANGE;
#endif /* DEBUG_RANGE_CHECK */

    error_code = si2196_pollforcts(si2196);
    if (error_code)
    {
        pr_info("%s: poll cts error:%d!!!!\n", __func__, error_code);
        goto exit;
    }

    cmdbytebuffer[0] = SI2196_TUNER_STATUS_CMD;
    cmdbytebuffer[1] = (unsigned char) ( ( intack & SI2196_TUNER_STATUS_CMD_INTACK_MASK ) << SI2196_TUNER_STATUS_CMD_INTACK_LSB);

    if (si2196_writecommandbytes(si2196, 2, cmdbytebuffer) != 2)
    {
        error_code = ERROR_SI2196_SENDING_COMMAND;
        pr_info("%s: write command byte error:%d!!!!\n", __func__, error_code);
    }

    if (!error_code)
    {
        error_code = si2196_pollforresponse(si2196, 1, 12, rspbytebuffer, &reply);
        if (error_code)
            pr_info("%s: poll response error:%d!!!!\n", __func__, error_code);
        rsp->reply = &reply;
        if (!error_code)
        {
            rsp->tuner_status.tcint          = (( ( (rspbytebuffer[1]  )) >> SI2196_TUNER_STATUS_RESPONSE_TCINT_LSB    ) & SI2196_TUNER_STATUS_RESPONSE_TCINT_MASK    );
            rsp->tuner_status.rssilint      = (( ( (rspbytebuffer[1]  )) >> SI2196_TUNER_STATUS_RESPONSE_RSSILINT_LSB ) & SI2196_TUNER_STATUS_RESPONSE_RSSILINT_MASK );
            rsp->tuner_status.rssihint     = (( ( (rspbytebuffer[1]  )) >> SI2196_TUNER_STATUS_RESPONSE_RSSIHINT_LSB ) & SI2196_TUNER_STATUS_RESPONSE_RSSIHINT_MASK );
            rsp->tuner_status.tc              = (( ( (rspbytebuffer[2]  )) >> SI2196_TUNER_STATUS_RESPONSE_TC_LSB       ) & SI2196_TUNER_STATUS_RESPONSE_TC_MASK       );
            rsp->tuner_status.rssil          = (( ( (rspbytebuffer[2]  )) >> SI2196_TUNER_STATUS_RESPONSE_RSSIL_LSB    ) & SI2196_TUNER_STATUS_RESPONSE_RSSIL_MASK    );
            rsp->tuner_status.rssih         = (( ( (rspbytebuffer[2]  )) >> SI2196_TUNER_STATUS_RESPONSE_RSSIH_LSB    ) & SI2196_TUNER_STATUS_RESPONSE_RSSIH_MASK    );
            rsp->tuner_status.rssi           = (((( ( (rspbytebuffer[3]  )) >> SI2196_TUNER_STATUS_RESPONSE_RSSI_LSB     ) & SI2196_TUNER_STATUS_RESPONSE_RSSI_MASK) <<SI2196_TUNER_STATUS_RESPONSE_RSSI_SHIFT ) >>SI2196_TUNER_STATUS_RESPONSE_RSSI_SHIFT     );
            rsp->tuner_status.freq          = (( ( (rspbytebuffer[4]  ) | (rspbytebuffer[5]  << 8 ) | (rspbytebuffer[6]  << 16 ) | (rspbytebuffer[7]  << 24 )) >> SI2196_TUNER_STATUS_RESPONSE_FREQ_LSB     ) & SI2196_TUNER_STATUS_RESPONSE_FREQ_MASK     );
            rsp->tuner_status.mode        = (( ( (rspbytebuffer[8]  )) >> SI2196_TUNER_STATUS_RESPONSE_MODE_LSB     ) & SI2196_TUNER_STATUS_RESPONSE_MODE_MASK     );
            rsp->tuner_status.vco_code = (((( ( (rspbytebuffer[10] ) | (rspbytebuffer[11] << 8 )) >> SI2196_TUNER_STATUS_RESPONSE_VCO_CODE_LSB ) & SI2196_TUNER_STATUS_RESPONSE_VCO_CODE_MASK) <<SI2196_TUNER_STATUS_RESPONSE_VCO_CODE_SHIFT ) >>SI2196_TUNER_STATUS_RESPONSE_VCO_CODE_SHIFT );
            //rsp->tuner_status.status       = &reply;
        }
    }
exit:
    return error_code;
}
#endif /* SI2196_TUNER_STATUS_CMD */
#ifdef SI2196_TUNER_TUNE_FREQ_CMD
/*---------------------------------------------------*/
/* SI2196_TUNER_TUNE_FREQ COMMAND                  */
/*---------------------------------------------------*/
unsigned char si2196_tuner_tune_freq(struct i2c_client *si2196,
        unsigned char   mode,
        unsigned long   freq,
        si2196_cmdreplyobj_t *rsp)
{
    unsigned char error_code = 0;
    unsigned char cmdbytebuffer[8];
    unsigned char rspbytebuffer[1];

#ifdef DEBUG_RANGE_CHECK
    if ( (mode > SI2196_TUNER_TUNE_FREQ_CMD_MODE_MAX)
            || (freq > SI2196_TUNER_TUNE_FREQ_CMD_FREQ_MAX)  || (freq < SI2196_TUNER_TUNE_FREQ_CMD_FREQ_MIN) )
        return ERROR_SI2196_PARAMETER_OUT_OF_RANGE;
#endif /* DEBUG_RANGE_CHECK */

    error_code = si2196_pollforcts(si2196);
    if (error_code)
    {
        pr_info("%s: poll cts error:%d!!!!\n", __func__, error_code);
        goto exit;
    }

    cmdbytebuffer[0] = SI2196_TUNER_TUNE_FREQ_CMD;
    cmdbytebuffer[1] = (unsigned char) ( ( mode & SI2196_TUNER_TUNE_FREQ_CMD_MODE_MASK ) << SI2196_TUNER_TUNE_FREQ_CMD_MODE_LSB);
    cmdbytebuffer[2] = (unsigned char)0x00;
    cmdbytebuffer[3] = (unsigned char)0x00;
    cmdbytebuffer[4] = (unsigned char) ( ( freq & SI2196_TUNER_TUNE_FREQ_CMD_FREQ_MASK ) << SI2196_TUNER_TUNE_FREQ_CMD_FREQ_LSB);
    cmdbytebuffer[5] = (unsigned char) ((( freq & SI2196_TUNER_TUNE_FREQ_CMD_FREQ_MASK ) << SI2196_TUNER_TUNE_FREQ_CMD_FREQ_LSB)>>8);
    cmdbytebuffer[6] = (unsigned char) ((( freq & SI2196_TUNER_TUNE_FREQ_CMD_FREQ_MASK ) << SI2196_TUNER_TUNE_FREQ_CMD_FREQ_LSB)>>16);
    cmdbytebuffer[7] = (unsigned char) ((( freq & SI2196_TUNER_TUNE_FREQ_CMD_FREQ_MASK ) << SI2196_TUNER_TUNE_FREQ_CMD_FREQ_LSB)>>24);

    if (si2196_writecommandbytes(si2196, 8, cmdbytebuffer) != 8)
    {
        error_code = ERROR_SI2196_SENDING_COMMAND;
        pr_info("%s: write command byte error:%d!!!!\n", __func__, error_code);
    }

    if (!error_code)
    {
        error_code = si2196_pollforresponse(si2196, 1, 1, rspbytebuffer, &reply);
        if (error_code)
            pr_info("%s: poll response error:%d!!!!\n", __func__, error_code);
        rsp->reply = &reply;
    }
    pr_info("%s.\n",__func__);
exit:
    return error_code;
}
#endif /* SI2196_TUNER_TUNE_FREQ_CMD */
/* _commands_insertion_point */

/* _send_command2_insertion_start */

/* --------------------------------------------*/
/* SEND_COMMAND2 FUNCTION                      */
/* --------------------------------------------*/
unsigned char si2196_sendcommand(struct i2c_client *si2196, int cmd, si2196_cmdobj_t *c, si2196_cmdreplyobj_t *rsp)
{
    switch (cmd)
    {
#ifdef SI2196_AGC_OVERRIDE_CMD
        case SI2196_AGC_OVERRIDE_CMD:
            return si2196_agc_override(si2196, c->agc_override.force_max_gain, c->agc_override.force_top_gain, rsp);
        break;
#endif /*     SI2196_AGC_OVERRIDE_CMD */
#ifdef SI2196_ATV_CW_TEST_CMD
        case SI2196_ATV_CW_TEST_CMD:
            return si2196_atv_cw_test(si2196, c->atv_cw_test.pc_lock, rsp);
        break;
#endif /*     SI2196_ATV_CW_TEST_CMD */
#ifdef SI2196_ATV_RESTART_CMD
        case SI2196_ATV_RESTART_CMD:
            return si2196_atv_restart(si2196, c->atv_restart.mode, rsp);
        break;
#endif /*     SI2196_ATV_RESTART_CMD */
#ifdef SI2196_ATV_STATUS_CMD
        case SI2196_ATV_STATUS_CMD:
            return si2196_atv_status(si2196, c->atv_status.intack, rsp);
        break;
#endif /*     SI2196_ATV_STATUS_CMD */
#ifdef SI2196_CONFIG_PINS_CMD
        case SI2196_CONFIG_PINS_CMD:
            return si2196_config_pins(si2196, c->config_pins.gpio1_mode, c->config_pins.gpio1_read, c->config_pins.gpio2_mode, c->config_pins.gpio2_read, c->config_pins.gpio3_mode, c->config_pins.gpio3_read, c->config_pins.bclk1_mode, c->config_pins.bclk1_read, c->config_pins.xout_mode, rsp);
        break;
#endif /*     SI2196_CONFIG_PINS_CMD */
#ifdef SI2196_EXIT_BOOTLOADER_CMD
        case SI2196_EXIT_BOOTLOADER_CMD:
            return si2196_exit_bootloader(si2196, c->exit_bootloader.func, c->exit_bootloader.ctsien, rsp);
        break;
#endif /*     SI2196_EXIT_BOOTLOADER_CMD */
#ifdef SI2196_FINE_TUNE_CMD
        case SI2196_FINE_TUNE_CMD:
            return si2196_fine_tune(si2196, c->fine_tune.reserved, c->fine_tune.offset_500hz, rsp);
        break;
#endif /*     SI2196_FINE_TUNE_CMD */
#ifdef SI2196_GET_PROPERTY_CMD
        case SI2196_GET_PROPERTY_CMD:
            return si2196_get_property(si2196, c->get_property.reserved, c->get_property.prop, rsp);
        break;
#endif /*     SI2196_GET_PROPERTY_CMD */
#ifdef SI2196_GET_REV_CMD
        case SI2196_GET_REV_CMD:
            return si2196_get_rev(si2196, rsp);
        break;
#endif /*     SI2196_GET_REV_CMD */
#ifdef SI2196_PART_INFO_CMD
        case SI2196_PART_INFO_CMD:
            return si2196_part_info(si2196, rsp);
        break;
#endif /*     SI2196_PART_INFO_CMD */
#ifdef SI2196_POWER_DOWN_CMD
        case SI2196_POWER_DOWN_CMD:
            return si2196_power_down(si2196, rsp);
        break;
#endif /*     SI2196_POWER_DOWN_CMD */
#ifdef SI2196_POWER_UP_CMD
        case SI2196_POWER_UP_CMD:
            return si2196_power_up(si2196, c->power_up.subcode, c->power_up.reserved1, c->power_up.reserved2, c->power_up.reserved3, c->power_up.clock_mode, c->power_up.clock_freq, c->power_up.addr_mode, c->power_up.func, c->power_up.ctsien, c->power_up.wake_up, rsp);
        break;
#endif /*     SI2196_POWER_UP_CMD */
#ifdef SI2196_SET_PROPERTY_CMD
        case SI2196_SET_PROPERTY_CMD:
            return si2196_set_property(si2196, c->set_property.reserved, c->set_property.prop, c->set_property.data, rsp);
        break;
#endif /*     SI2196_SET_PROPERTY_CMD */
#ifdef SI2196_STANDBY_CMD
        case SI2196_STANDBY_CMD:
            return si2196_standby(si2196, rsp);
        break;
#endif /*     SI2196_STANDBY_CMD */
#ifdef SI2196_TUNER_STATUS_CMD
        case SI2196_TUNER_STATUS_CMD:
            return si2196_tuner_status(si2196, c->tuner_status.intack, rsp);
        break;
#endif /*     SI2196_TUNER_STATUS_CMD */
#ifdef SI2196_TUNER_TUNE_FREQ_CMD
        case SI2196_TUNER_TUNE_FREQ_CMD:
            return si2196_tuner_tune_freq(si2196, c->tuner_tune_freq.mode, c->tuner_tune_freq.freq, rsp);
        break;
#endif /*     SI2196_TUNER_TUNE_FREQ_CMD */
#ifdef SI2196_SD_ADAC_POWER_UP_CMD
        case SI2196_SD_ADAC_POWER_UP_CMD:
            return si2196_adac_power_up(si2196, c->sd_adac_power_up.duration, rsp);
        break;
#endif
#ifdef SI2196_SD_AFC_CMD
        case SI2196_SD_AFC_CMD:
            return si2196_sd_afc(si2196, rsp);
        break;
#endif /* SI2196_SD_AFC_CMD */
#ifdef SI2196_SD_CARRIER_CNR_CMD
        case SI2196_SD_CARRIER_CNR_CMD:
            return si2196_sd_carrier_cnr(si2196, rsp);
        break;
#endif /* SI2196_SD_CARRIER_CNR_CMD */
#ifdef SI2196_SD_CASD_CMD
        case SI2196_SD_CASD_CMD:
            return si2196_sd_casd(si2196,rsp);
        break;
#endif /* SI2196_SD_CASD_CMD */
#ifdef SI2196_SD_DUAL_MONO_ID_LVL_CMD
        case SI2196_SD_DUAL_MONO_ID_LVL_CMD:
            return si2196_sd_dual_mono_id_lvl(si2196, rsp);
        break;
#endif /* SI2196_SD_DUAL_MONO_ID_LVL_CMD */
#ifdef SI2196_SD_NICAM_STATUS_CMD
        case SI2196_SD_NICAM_STATUS_CMD:
            return si2196_sd_nicam_status(si2196,rsp);
#endif /* SI2196_SD_NICAM_STATUS_CMD */
#ifdef SI2196_SD_STATUS_CMD
        case SI2196_SD_STATUS_CMD:
            return si2196_sd_status(si2196,rsp,c->sd_status.intack);
        break;
#endif /* SI2196_SD_STATUS_CMD */
#ifdef SI2196_SD_STEREO_ID_LVL_CMD
        case SI2196_SD_STEREO_ID_LVL_CMD:
            return si2196_sd_stereo_id_lvl(si2196, rsp);
        break;
#endif /* SI2196_SD_STEREO_ID_LVL_CMD */
        default :
        break;
    }
    return 0;
}
/* _send_command2_insertion_point */

/***********************************************************************************************************************
  si2196_setproperty function
  Use:        property set function
              Used to call L1_SET_PROPERTY with the property Id and data provided.
  Comments:   This is a way to make sure CTS is polled when setting a property
  Parameter: *api     the SI2196 context
  Parameter: waitforcts flag to wait for a CTS before issuing the property command
  Parameter: waitforresponse flag to wait for a CTS after issuing the property command
  Parameter: prop     the property Id
  Parameter: data     the property bytes
  Returns:    0 if no error, an error code otherwise
 ***********************************************************************************************************************/
static unsigned char si2196_setproperty(struct i2c_client *si2196, unsigned int prop, int  data, si2196_cmdreplyobj_t *rsp)
{
    unsigned char  reserved          = 0;
    return si2196_set_property(si2196, reserved, prop, data, rsp);
}

/***********************************************************************************************************************
  si2196_getproperty function
  Use:        property get function
              Used to call L1_GET_PROPERTY with the property Id provided.
  Comments:   This is a way to make sure CTS is polled when retrieving a property
  Parameter: *api     the SI2196 context
  Parameter: waitforcts flag to wait for a CTS before issuing the property command
  Parameter: waitforresponse flag to wait for a CTS after issuing the property command
  Parameter: prop     the property Id
  Parameter: *data    a buffer to store the property bytes into
  Returns:    0 if no error, an error code otherwise
 ***********************************************************************************************************************/
static unsigned char si2196_getproperty(struct i2c_client *si2196, unsigned int prop, int *data, si2196_cmdreplyobj_t *rsp)
{
    unsigned char reserved = 0;
    unsigned char res;
    res = si2196_get_property(si2196, reserved, prop, rsp);
    *data = rsp->get_property.data;
    return res;
}

/* _set_property2_insertion_start */

/* --------------------------------------------*/
/* SET_PROPERTY2 FUNCTION                      */
/* --------------------------------------------*/
unsigned char si2196_sendproperty(struct i2c_client *si2196, unsigned int prop, si2196_propobj_t *p, si2196_cmdreplyobj_t *rsp)
{
    int data = 0;
#ifdef    SI2196_GET_PROPERTY_STRING
    char msg[1000];
#endif /* SI2196_GET_PROPERTY_STRING */
    switch (prop)
    {
#ifdef SI2196_ATV_AFC_RANGE_PROP
        case SI2196_ATV_AFC_RANGE_PROP:
            data = (p->atv_afc_range.range_khz & SI2196_ATV_AFC_RANGE_PROP_RANGE_KHZ_MASK) << SI2196_ATV_AFC_RANGE_PROP_RANGE_KHZ_LSB ;
        break;
#endif /*     SI2196_ATV_AFC_RANGE_PROP */
#ifdef SI2196_ATV_AGC_SPEED_PROP
        case SI2196_ATV_AGC_SPEED_PROP:
            data = (p->atv_agc_speed.if_agc_speed & SI2196_ATV_AGC_SPEED_PROP_IF_AGC_SPEED_MASK) << SI2196_ATV_AGC_SPEED_PROP_IF_AGC_SPEED_LSB ;
        break;
#endif /*     SI2196_ATV_AGC_SPEED_PROP */
#ifdef SI2196_ATV_AUDIO_MODE_PROP
        case SI2196_ATV_AUDIO_MODE_PROP:
            data = (p->atv_audio_mode.audio_sys  & SI2196_ATV_AUDIO_MODE_PROP_AUDIO_SYS_FILT_MASK ) << SI2196_ATV_AUDIO_MODE_PROP_AUDIO_SYS_FILT_LSB  |
                       (p->atv_audio_mode.chan_bw    & SI2196_ATV_AUDIO_MODE_PROP_CHAN_BW_MASK   ) << SI2196_ATV_AUDIO_MODE_PROP_CHAN_BW_LSB ;
        break;
#endif /*     SI2196_ATV_AUDIO_MODE_PROP */
#ifdef SI2196_ATV_CVBS_OUT_PROP
        case SI2196_ATV_CVBS_OUT_PROP:
            data = (p->atv_cvbs_out.offset & SI2196_ATV_CVBS_OUT_PROP_OFFSET_MASK) << SI2196_ATV_CVBS_OUT_PROP_OFFSET_LSB  |
                       (p->atv_cvbs_out.amp    & SI2196_ATV_CVBS_OUT_PROP_AMP_MASK   ) << SI2196_ATV_CVBS_OUT_PROP_AMP_LSB ;
        break;
#endif /*     SI2196_ATV_CVBS_OUT_PROP */
#ifdef SI2196_ATV_CVBS_OUT_FINE_PROP
        case SI2196_ATV_CVBS_OUT_FINE_PROP:
            data = (p->atv_cvbs_out_fine.offset & SI2196_ATV_CVBS_OUT_FINE_PROP_OFFSET_MASK) << SI2196_ATV_CVBS_OUT_FINE_PROP_OFFSET_LSB  |
                       (p->atv_cvbs_out_fine.amp    & SI2196_ATV_CVBS_OUT_FINE_PROP_AMP_MASK   ) << SI2196_ATV_CVBS_OUT_FINE_PROP_AMP_LSB ;
        break;
#endif /*     SI2196_ATV_CVBS_OUT_FINE_PROP */
#ifdef SI2196_ATV_IEN_PROP
        case SI2196_ATV_IEN_PROP:
            data = (p->atv_ien.chlien  & SI2196_ATV_IEN_PROP_CHLIEN_MASK ) << SI2196_ATV_IEN_PROP_CHLIEN_LSB  |
                       (p->atv_ien.pclien  & SI2196_ATV_IEN_PROP_PCLIEN_MASK ) << SI2196_ATV_IEN_PROP_PCLIEN_LSB  |
                       (p->atv_ien.dlien   & SI2196_ATV_IEN_PROP_DLIEN_MASK  ) << SI2196_ATV_IEN_PROP_DLIEN_LSB  |
                       (p->atv_ien.snrlien & SI2196_ATV_IEN_PROP_SNRLIEN_MASK) << SI2196_ATV_IEN_PROP_SNRLIEN_LSB  |
                       (p->atv_ien.snrhien & SI2196_ATV_IEN_PROP_SNRHIEN_MASK) << SI2196_ATV_IEN_PROP_SNRHIEN_LSB ;
        break;
#endif /*     SI2196_ATV_IEN_PROP */
#ifdef SI2196_ATV_INT_SENSE_PROP
        case SI2196_ATV_INT_SENSE_PROP:
            data = (p->atv_int_sense.chlnegen  & SI2196_ATV_INT_SENSE_PROP_CHLNEGEN_MASK ) << SI2196_ATV_INT_SENSE_PROP_CHLNEGEN_LSB  |
                       (p->atv_int_sense.pclnegen  & SI2196_ATV_INT_SENSE_PROP_PCLNEGEN_MASK ) << SI2196_ATV_INT_SENSE_PROP_PCLNEGEN_LSB  |
                       (p->atv_int_sense.dlnegen   & SI2196_ATV_INT_SENSE_PROP_DLNEGEN_MASK  ) << SI2196_ATV_INT_SENSE_PROP_DLNEGEN_LSB  |
                       (p->atv_int_sense.snrlnegen & SI2196_ATV_INT_SENSE_PROP_SNRLNEGEN_MASK) << SI2196_ATV_INT_SENSE_PROP_SNRLNEGEN_LSB  |
                       (p->atv_int_sense.snrhnegen & SI2196_ATV_INT_SENSE_PROP_SNRHNEGEN_MASK) << SI2196_ATV_INT_SENSE_PROP_SNRHNEGEN_LSB  |
                       (p->atv_int_sense.chlposen  & SI2196_ATV_INT_SENSE_PROP_CHLPOSEN_MASK ) << SI2196_ATV_INT_SENSE_PROP_CHLPOSEN_LSB  |
                       (p->atv_int_sense.pclposen  & SI2196_ATV_INT_SENSE_PROP_PCLPOSEN_MASK ) << SI2196_ATV_INT_SENSE_PROP_PCLPOSEN_LSB  |
                       (p->atv_int_sense.dlposen   & SI2196_ATV_INT_SENSE_PROP_DLPOSEN_MASK  ) << SI2196_ATV_INT_SENSE_PROP_DLPOSEN_LSB  |
                       (p->atv_int_sense.snrlposen & SI2196_ATV_INT_SENSE_PROP_SNRLPOSEN_MASK) << SI2196_ATV_INT_SENSE_PROP_SNRLPOSEN_LSB  |
                       (p->atv_int_sense.snrhposen & SI2196_ATV_INT_SENSE_PROP_SNRHPOSEN_MASK) << SI2196_ATV_INT_SENSE_PROP_SNRHPOSEN_LSB ;
        break;
#endif /*     SI2196_ATV_INT_SENSE_PROP */
#ifdef SI2196_ATV_RF_TOP_PROP
        case SI2196_ATV_RF_TOP_PROP:
            data = (p->atv_rf_top.atv_rf_top & SI2196_ATV_RF_TOP_PROP_ATV_RF_TOP_MASK) << SI2196_ATV_RF_TOP_PROP_ATV_RF_TOP_LSB ;
        break;
#endif /*     SI2196_ATV_RF_TOP_PROP */
#ifdef SI2196_ATV_RSQ_RSSI_THRESHOLD_PROP
        case SI2196_ATV_RSQ_RSSI_THRESHOLD_PROP:
            data = (p->atv_rsq_rssi_threshold.lo & SI2196_ATV_RSQ_RSSI_THRESHOLD_PROP_LO_MASK) << SI2196_ATV_RSQ_RSSI_THRESHOLD_PROP_LO_LSB  |
                       (p->atv_rsq_rssi_threshold.hi & SI2196_ATV_RSQ_RSSI_THRESHOLD_PROP_HI_MASK) << SI2196_ATV_RSQ_RSSI_THRESHOLD_PROP_HI_LSB ;
        break;
#endif /*     SI2196_ATV_RSQ_RSSI_THRESHOLD_PROP */
#ifdef SI2196_ATV_RSQ_SNR_THRESHOLD_PROP
        case SI2196_ATV_RSQ_SNR_THRESHOLD_PROP:
            data = (p->atv_rsq_snr_threshold.lo & SI2196_ATV_RSQ_SNR_THRESHOLD_PROP_LO_MASK) << SI2196_ATV_RSQ_SNR_THRESHOLD_PROP_LO_LSB  |
                       (p->atv_rsq_snr_threshold.hi & SI2196_ATV_RSQ_SNR_THRESHOLD_PROP_HI_MASK) << SI2196_ATV_RSQ_SNR_THRESHOLD_PROP_HI_LSB ;
        break;
#endif /*     SI2196_ATV_RSQ_SNR_THRESHOLD_PROP */
#ifdef SI2196_ATV_SOUND_AGC_LIMIT_PROP
        case SI2196_ATV_SOUND_AGC_LIMIT_PROP:
            data = (p->atv_sound_agc_limit.max_gain & SI2196_ATV_SOUND_AGC_LIMIT_PROP_MAX_GAIN_MASK) << SI2196_ATV_SOUND_AGC_LIMIT_PROP_MAX_GAIN_LSB  |
                       (p->atv_sound_agc_limit.min_gain & SI2196_ATV_SOUND_AGC_LIMIT_PROP_MIN_GAIN_MASK) << SI2196_ATV_SOUND_AGC_LIMIT_PROP_MIN_GAIN_LSB ;
        break;
#endif /*     SI2196_ATV_SOUND_AGC_LIMIT_PROP */
#ifdef SI2196_ATV_VIDEO_EQUALIZER_PROP
        case SI2196_ATV_VIDEO_EQUALIZER_PROP:
            data = (p->atv_video_equalizer.slope & SI2196_ATV_VIDEO_EQUALIZER_PROP_SLOPE_MASK) << SI2196_ATV_VIDEO_EQUALIZER_PROP_SLOPE_LSB ;
        break;
#endif /*     SI2196_ATV_VIDEO_EQUALIZER_PROP */
#ifdef SI2196_ATV_VIDEO_MODE_PROP
        case SI2196_ATV_VIDEO_MODE_PROP:
            data = (p->atv_video_mode.video_sys       & SI2196_ATV_VIDEO_MODE_PROP_VIDEO_SYS_MASK      ) << SI2196_ATV_VIDEO_MODE_PROP_VIDEO_SYS_LSB  |
                       (p->atv_video_mode.color           & SI2196_ATV_VIDEO_MODE_PROP_COLOR_MASK          ) << SI2196_ATV_VIDEO_MODE_PROP_COLOR_LSB  |
                       (p->atv_video_mode.trans           & SI2196_ATV_VIDEO_MODE_PROP_TRANS_MASK          ) << SI2196_ATV_VIDEO_MODE_PROP_TRANS_LSB  |
                       (p->atv_video_mode.invert_signal   & SI2196_ATV_VIDEO_MODE_PROP_INVERT_SIGNAL_MASK  ) << SI2196_ATV_VIDEO_MODE_PROP_INVERT_SIGNAL_LSB ;
        break;
#endif /*     SI2196_ATV_VIDEO_MODE_PROP */
#ifdef SI2196_ATV_VSNR_CAP_PROP
        case SI2196_ATV_VSNR_CAP_PROP:
            data = (p->atv_vsnr_cap.atv_vsnr_cap & SI2196_ATV_VSNR_CAP_PROP_ATV_VSNR_CAP_MASK) << SI2196_ATV_VSNR_CAP_PROP_ATV_VSNR_CAP_LSB ;
        break;
#endif /*     SI2196_ATV_VSNR_CAP_PROP */
#ifdef SI2196_CRYSTAL_TRIM_PROP
        case SI2196_CRYSTAL_TRIM_PROP:
            data = (p->crystal_trim.xo_cap & SI2196_CRYSTAL_TRIM_PROP_XO_CAP_MASK) << SI2196_CRYSTAL_TRIM_PROP_XO_CAP_LSB ;
        break;
#endif
#ifdef SI2196_SD_AFC_MAX_PROP
        case  SI2196_SD_AFC_MAX_PROP_CODE:
            data = (p->sd_afc_max.max_afc & SI2196_SD_AFC_MAX_PROP_MAX_AFC_MASK) << SI2196_SD_AFC_MAX_PROP_MAX_AFC_LSB ;
        break;
#endif /*     SI2196_SD_AFC_MAX_PROP */
#ifdef SI2196_SD_AFC_MUTE_PROP
        case SI2196_SD_AFC_MUTE_PROP_CODE:
            data = (p->sd_afc_mute.mute_thresh   & SI2196_SD_AFC_MUTE_PROP_MUTE_THRESH_MASK  ) << SI2196_SD_AFC_MUTE_PROP_MUTE_THRESH_LSB  |
                       (p->sd_afc_mute.unmute_thresh & SI2196_SD_AFC_MUTE_PROP_UNMUTE_THRESH_MASK) << SI2196_SD_AFC_MUTE_PROP_UNMUTE_THRESH_LSB ;
        break;
#endif /*     SI2196_SD_AFC_MUTE_PROP */
#ifdef SI2196_SD_AGC_PROP
        case SI2196_SD_AGC_PROP_CODE:
       data = (p->sd_agc.gain   & SI2196_SD_AGC_PROP_GAIN_MASK) << SI2196_SD_AGC_PROP_GAIN_LSB  |
                  (p->sd_agc.freeze & SI2196_SD_AGC_PROP_FREEZE_MASK) << SI2196_SD_AGC_PROP_FREEZE_LSB ;
     break;
#endif /*     SI2196_SD_AGC_PROP */
#ifdef SI2196_SD_ASD_PROP
       case  SI2196_SD_ASD_PROP_CODE:
        data = (p->sd_asd.iterations      & SI2196_SD_ASD_PROP_ITERATIONS_MASK) << SI2196_SD_ASD_PROP_ITERATIONS_LSB  |
                  (p->sd_asd.enable_nicam_l  & SI2196_SD_ASD_PROP_ENABLE_NICAM_L_MASK ) << SI2196_SD_ASD_PROP_ENABLE_NICAM_L_LSB  |
                  (p->sd_asd.enable_nicam_dk & SI2196_SD_ASD_PROP_ENABLE_NICAM_DK_MASK) << SI2196_SD_ASD_PROP_ENABLE_NICAM_DK_LSB  |
                  (p->sd_asd.enable_nicam_i  & SI2196_SD_ASD_PROP_ENABLE_NICAM_I_MASK ) << SI2196_SD_ASD_PROP_ENABLE_NICAM_I_LSB  |
                  (p->sd_asd.enable_nicam_bg & SI2196_SD_ASD_PROP_ENABLE_NICAM_BG_MASK) << SI2196_SD_ASD_PROP_ENABLE_NICAM_BG_LSB  |
                  (p->sd_asd.enable_a2_m     & SI2196_SD_ASD_PROP_ENABLE_A2_M_MASK) << SI2196_SD_ASD_PROP_ENABLE_A2_M_LSB  |
                  (p->sd_asd.enable_a2_dk    & SI2196_SD_ASD_PROP_ENABLE_A2_DK_MASK) << SI2196_SD_ASD_PROP_ENABLE_A2_DK_LSB  |
                  (p->sd_asd.enable_a2_bg    & SI2196_SD_ASD_PROP_ENABLE_A2_BG_MASK) << SI2196_SD_ASD_PROP_ENABLE_A2_BG_LSB  |
                  (p->sd_asd.enable_eiaj     & SI2196_SD_ASD_PROP_ENABLE_EIAJ_MASK) << SI2196_SD_ASD_PROP_ENABLE_EIAJ_LSB  |
                  (p->sd_asd.enable_btsc     & SI2196_SD_ASD_PROP_ENABLE_BTSC_MASK) << SI2196_SD_ASD_PROP_ENABLE_BTSC_LSB ;
      break;
#endif /*     SI2196_SD_ASD_PROP */
#ifdef SI2196_SD_CARRIER_MUTE_PROP
      case SI2196_SD_CARRIER_MUTE_PROP_CODE:
        data = (p->sd_carrier_mute.primary_thresh   & SI2196_SD_CARRIER_MUTE_PROP_PRIMARY_THRESH_MASK  ) << SI2196_SD_CARRIER_MUTE_PROP_PRIMARY_THRESH_LSB  |
                   (p->sd_carrier_mute.secondary_thresh & SI2196_SD_CARRIER_MUTE_PROP_SECONDARY_THRESH_MASK) << SI2196_SD_CARRIER_MUTE_PROP_SECONDARY_THRESH_LSB ;
      break;
#endif /*     SI2196_SD_CARRIER_MUTE_PROP */
#ifdef SI2196_SD_I2S_PROP
     case  SI2196_SD_I2S_PROP_CODE:
       data = (p->sd_i2s.lrclk_pol      & SI2196_SD_I2S_PROP_LRCLK_POL_MASK     ) << SI2196_SD_I2S_PROP_LRCLK_POL_LSB  |
                  (p->sd_i2s.alignment      & SI2196_SD_I2S_PROP_ALIGNMENT_MASK     ) << SI2196_SD_I2S_PROP_ALIGNMENT_LSB  |
                  (p->sd_i2s.lrclk_rate     & SI2196_SD_I2S_PROP_LRCLK_RATE_MASK    ) << SI2196_SD_I2S_PROP_LRCLK_RATE_LSB  |
                  (p->sd_i2s.num_bits       & SI2196_SD_I2S_PROP_NUM_BITS_MASK      ) << SI2196_SD_I2S_PROP_NUM_BITS_LSB  |
                  (p->sd_i2s.sclk_rate      & SI2196_SD_I2S_PROP_SCLK_RATE_MASK     ) << SI2196_SD_I2S_PROP_SCLK_RATE_LSB  |
                  (p->sd_i2s.drive_strength & SI2196_SD_I2S_PROP_DRIVE_STRENGTH_MASK) << SI2196_SD_I2S_PROP_DRIVE_STRENGTH_LSB ;
     break;
#endif /*     SI2196_SD_I2S_PROP */
#ifdef   SI2196_SD_IEN_PROP
     case  SI2196_SD_IEN_PROP_CODE:
       data = (p->sd_ien.asdcien  & SI2196_SD_IEN_PROP_ASDCIEN_MASK ) << SI2196_SD_IEN_PROP_ASDCIEN_LSB  |
                  (p->sd_ien.nicamien & SI2196_SD_IEN_PROP_NICAMIEN_MASK) << SI2196_SD_IEN_PROP_NICAMIEN_LSB  |
                  (p->sd_ien.pcmien   & SI2196_SD_IEN_PROP_PCMIEN_MASK  ) << SI2196_SD_IEN_PROP_PCMIEN_LSB  |
                  (p->sd_ien.scmien   & SI2196_SD_IEN_PROP_SCMIEN_MASK  ) << SI2196_SD_IEN_PROP_SCMIEN_LSB  |
                  (p->sd_ien.odmien   & SI2196_SD_IEN_PROP_ODMIEN_MASK  ) << SI2196_SD_IEN_PROP_ODMIEN_LSB  |
                  (p->sd_ien.afcmien  & SI2196_SD_IEN_PROP_AFCMIEN_MASK ) << SI2196_SD_IEN_PROP_AFCMIEN_LSB  |
                  (p->sd_ien.ssien    & SI2196_SD_IEN_PROP_SSIEN_MASK   ) << SI2196_SD_IEN_PROP_SSIEN_LSB  |
                  (p->sd_ien.agcsien  & SI2196_SD_IEN_PROP_AGCSIEN_MASK ) << SI2196_SD_IEN_PROP_AGCSIEN_LSB ;
     break;
#endif /*     SI2196_SD_IEN_PROP */
#ifdef SI2196_SD_INT_SENSE_PROP
     case  SI2196_SD_INT_SENSE_PROP_CODE:
       data = (p->sd_int_sense.asdcnegen  & SI2196_SD_INT_SENSE_PROP_ASDCNEGEN_MASK ) << SI2196_SD_INT_SENSE_PROP_ASDCNEGEN_LSB  |
                  (p->sd_int_sense.nicamnegen & SI2196_SD_INT_SENSE_PROP_NICAMNEGEN_MASK) << SI2196_SD_INT_SENSE_PROP_NICAMNEGEN_LSB  |
                  (p->sd_int_sense.pcmnegen   & SI2196_SD_INT_SENSE_PROP_PCMNEGEN_MASK  ) << SI2196_SD_INT_SENSE_PROP_PCMNEGEN_LSB  |
                  (p->sd_int_sense.scmnegen   & SI2196_SD_INT_SENSE_PROP_SCMNEGEN_MASK  ) << SI2196_SD_INT_SENSE_PROP_SCMNEGEN_LSB  |
                  (p->sd_int_sense.odmnegen   & SI2196_SD_INT_SENSE_PROP_ODMNEGEN_MASK  ) << SI2196_SD_INT_SENSE_PROP_ODMNEGEN_LSB  |
                  (p->sd_int_sense.afcmnegen  & SI2196_SD_INT_SENSE_PROP_AFCMNEGEN_MASK ) << SI2196_SD_INT_SENSE_PROP_AFCMNEGEN_LSB  |
                  (p->sd_int_sense.agcsnegen  & SI2196_SD_INT_SENSE_PROP_AGCSNEGEN_MASK ) << SI2196_SD_INT_SENSE_PROP_AGCSNEGEN_LSB  |
                  (p->sd_int_sense.asdcposen  & SI2196_SD_INT_SENSE_PROP_ASDCPOSEN_MASK ) << SI2196_SD_INT_SENSE_PROP_ASDCPOSEN_LSB  |
                  (p->sd_int_sense.nicamposen & SI2196_SD_INT_SENSE_PROP_NICAMPOSEN_MASK) << SI2196_SD_INT_SENSE_PROP_NICAMPOSEN_LSB  |
                  (p->sd_int_sense.pcmposen   & SI2196_SD_INT_SENSE_PROP_PCMPOSEN_MASK  ) << SI2196_SD_INT_SENSE_PROP_PCMPOSEN_LSB  |
                  (p->sd_int_sense.scmposen   & SI2196_SD_INT_SENSE_PROP_SCMPOSEN_MASK  ) << SI2196_SD_INT_SENSE_PROP_SCMPOSEN_LSB  |
                  (p->sd_int_sense.odmposen   & SI2196_SD_INT_SENSE_PROP_ODMPOSEN_MASK  ) << SI2196_SD_INT_SENSE_PROP_ODMPOSEN_LSB  |
                  (p->sd_int_sense.afcmposen  & SI2196_SD_INT_SENSE_PROP_AFCMPOSEN_MASK ) << SI2196_SD_INT_SENSE_PROP_AFCMPOSEN_LSB  |
                  (p->sd_int_sense.agcsposen  & SI2196_SD_INT_SENSE_PROP_AGCSPOSEN_MASK ) << SI2196_SD_INT_SENSE_PROP_AGCSPOSEN_LSB ;
     break;
#endif /*     SI2196_SD_INT_SENSE_PROP */
#ifdef  SI2196_SD_LANG_SELECT_PROP
     case  SI2196_SD_LANG_SELECT_PROP_CODE:
       data = (p->sd_lang_select.lang & SI2196_SD_LANG_SELECT_PROP_LANG_MASK) << SI2196_SD_LANG_SELECT_PROP_LANG_LSB ;
     break;
#endif /*     SI2196_SD_LANG_SELECT_PROP */
#ifdef SI2196_SD_NICAM_PROP
     case SI2196_SD_NICAM_PROP_CODE:
       data = (p->sd_nicam.num_frames & SI2196_SD_NICAM_PROP_NUM_FRAMES_MASK) << SI2196_SD_NICAM_PROP_NUM_FRAMES_LSB  |
                  (p->sd_nicam.force_rss  & SI2196_SD_NICAM_PROP_FORCE_RSS_MASK ) << SI2196_SD_NICAM_PROP_FORCE_RSS_LSB ;
     break;
#endif /*     SI2196_SD_NICAM_PROP */
#ifdef SI2196_SD_NICAM_FAILOVER_THRESH_PROP
     case SI2196_SD_NICAM_FAILOVER_THRESH_PROP_CODE:
       data = (p->sd_nicam_failover_thresh.errors & SI2196_SD_NICAM_FAILOVER_THRESH_PROP_ERRORS_MASK) << SI2196_SD_NICAM_FAILOVER_THRESH_PROP_ERRORS_LSB ;
     break;
#endif /*     SI2196_SD_NICAM_FAILOVER_THRESH_PROP */
#ifdef SI2196_SD_NICAM_RECOVER_THRESH_PROP
     case SI2196_SD_NICAM_RECOVER_THRESH_PROP_CODE:
       data = (p->sd_nicam_recover_thresh.errors & SI2196_SD_NICAM_RECOVER_THRESH_PROP_ERRORS_MASK) << SI2196_SD_NICAM_RECOVER_THRESH_PROP_ERRORS_LSB ;
     break;
#endif /*     SI2196_SD_NICAM_RECOVER_THRESH_PROP */
#ifdef SI2196_SD_OVER_DEV_MODE_PROP
     case SI2196_SD_OVER_DEV_MODE_PROP_CODE:
       data = (p->sd_over_dev_mode.mode & SI2196_SD_OVER_DEV_MODE_PROP_MODE_MASK) << SI2196_SD_OVER_DEV_MODE_PROP_MODE_LSB ;
     break;
#endif /*     SI2196_SD_OVER_DEV_MODE_PROP */
#ifdef SI2196_SD_OVER_DEV_MUTE_PROP
     case SI2196_SD_OVER_DEV_MUTE_PROP_CODE:
       data = (p->sd_over_dev_mute.mute_thresh   & SI2196_SD_OVER_DEV_MUTE_PROP_MUTE_THRESH_MASK  ) << SI2196_SD_OVER_DEV_MUTE_PROP_MUTE_THRESH_LSB  |
                  (p->sd_over_dev_mute.unmute_thresh & SI2196_SD_OVER_DEV_MUTE_PROP_UNMUTE_THRESH_MASK) << SI2196_SD_OVER_DEV_MUTE_PROP_UNMUTE_THRESH_LSB ;
     break;
#endif /*     SI2196_SD_OVER_DEV_MUTE_PROP */
#ifdef SI2196_SD_PILOT_LVL_CTRL_PROP
     case SI2196_SD_PILOT_LVL_CTRL_PROP_CODE:
       data = (p->sd_pilot_lvl_ctrl.lose_lvl & SI2196_SD_PILOT_LVL_CTRL_PROP_LOSE_LVL_MASK) << SI2196_SD_PILOT_LVL_CTRL_PROP_LOSE_LVL_LSB  |
                  (p->sd_pilot_lvl_ctrl.acq_lvl  & SI2196_SD_PILOT_LVL_CTRL_PROP_ACQ_LVL_MASK ) << SI2196_SD_PILOT_LVL_CTRL_PROP_ACQ_LVL_LSB ;
     break;
#endif /*     SI2196_SD_PILOT_LVL_CTRL_PROP */
#ifdef SI2196_SD_PORT_CONFIG_PROP
     case SI2196_SD_PORT_CONFIG_PROP_CODE:
       data = (p->sd_port_config.port         & SI2196_SD_PORT_CONFIG_PROP_PORT_MASK        ) << SI2196_SD_PORT_CONFIG_PROP_PORT_LSB  |
                  (p->sd_port_config.balance_mode & SI2196_SD_PORT_CONFIG_PROP_BALANCE_MODE_MASK) << SI2196_SD_PORT_CONFIG_PROP_BALANCE_MODE_LSB ;
     break;
#endif /*     SI2196_SD_PORT_CONFIG_PROP */
#ifdef SI2196_SD_PORT_MUTE_PROP
     case SI2196_SD_PORT_MUTE_PROP_CODE:
       data = (p->sd_port_mute.mute & SI2196_SD_PORT_MUTE_PROP_MUTE_MASK) << SI2196_SD_PORT_MUTE_PROP_MUTE_LSB ;
     break;
#endif /*     SI2196_SD_PORT_MUTE_PROP */
#ifdef SI2196_SD_PORT_VOLUME_BALANCE_PROP
     case SI2196_SD_PORT_VOLUME_BALANCE_PROP_CODE:
       data = (p->sd_port_volume_balance.balance & SI2196_SD_PORT_VOLUME_BALANCE_PROP_BALANCE_MASK) << SI2196_SD_PORT_VOLUME_BALANCE_PROP_BALANCE_LSB ;
     break;
#endif /*     SI2196_SD_PORT_VOLUME_BALANCE_PROP */
#ifdef SI2196_SD_PORT_VOLUME_LEFT_PROP
     case SI2196_SD_PORT_VOLUME_LEFT_PROP_CODE:
       data = (p->sd_port_volume_left.volume & SI2196_SD_PORT_VOLUME_LEFT_PROP_VOLUME_MASK) << SI2196_SD_PORT_VOLUME_LEFT_PROP_VOLUME_LSB ;
     break;
    #endif /*     SI2196_SD_PORT_VOLUME_LEFT_PROP */
#ifdef SI2196_SD_PORT_VOLUME_MASTER_PROP
     case SI2196_SD_PORT_VOLUME_MASTER_PROP_CODE:
       data = (p->sd_port_volume_master.volume & SI2196_SD_PORT_VOLUME_MASTER_PROP_VOLUME_MASK) << SI2196_SD_PORT_VOLUME_MASTER_PROP_VOLUME_LSB ;
     break;
#endif /*     SI2196_SD_PORT_VOLUME_MASTER_PROP */
#ifdef SI2196_SD_PORT_VOLUME_RIGHT_PROP
     case SI2196_SD_PORT_VOLUME_RIGHT_PROP_CODE:
       data = (p->sd_port_volume_right.volume & SI2196_SD_PORT_VOLUME_RIGHT_PROP_VOLUME_MASK) << SI2196_SD_PORT_VOLUME_RIGHT_PROP_VOLUME_LSB ;
     break;
#endif /*     SI2196_SD_PORT_VOLUME_RIGHT_PROP */
#ifdef SI2196_SD_PRESCALER_AM_PROP
     case SI2196_SD_PRESCALER_AM_PROP_CODE:
       data = (p->sd_prescaler_am.gain & SI2196_SD_PRESCALER_AM_PROP_GAIN_MASK) << SI2196_SD_PRESCALER_AM_PROP_GAIN_LSB ;
     break;
#endif /*     SI2196_SD_PRESCALER_AM_PROP */
#ifdef SI2196_SD_PRESCALER_EIAJ_PROP
     case SI2196_SD_PRESCALER_EIAJ_PROP_CODE:
       data = (p->sd_prescaler_eiaj.gain & SI2196_SD_PRESCALER_EIAJ_PROP_GAIN_MASK) << SI2196_SD_PRESCALER_EIAJ_PROP_GAIN_LSB ;
     break;
#endif /*     SI2196_SD_PRESCALER_EIAJ_PROP */
#ifdef SI2196_SD_PRESCALER_FM_PROP
     case SI2196_SD_PRESCALER_FM_PROP_CODE:
       data = (p->sd_prescaler_fm.gain & SI2196_SD_PRESCALER_FM_PROP_GAIN_MASK) << SI2196_SD_PRESCALER_FM_PROP_GAIN_LSB ;
     break;
#endif /*     SI2196_SD_PRESCALER_FM_PROP */
#ifdef SI2196_SD_PRESCALER_NICAM_PROP
     case SI2196_SD_PRESCALER_NICAM_PROP_CODE:
       data = (p->sd_prescaler_nicam.gain & SI2196_SD_PRESCALER_NICAM_PROP_GAIN_MASK) << SI2196_SD_PRESCALER_NICAM_PROP_GAIN_LSB ;
     break;
    #endif /*     SI2196_SD_PRESCALER_NICAM_PROP */
#ifdef SI2196_SD_PRESCALER_SAP_PROP
     case SI2196_SD_PRESCALER_SAP_PROP_CODE:
       data = (p->sd_prescaler_sap.gain & SI2196_SD_PRESCALER_SAP_PROP_GAIN_MASK) << SI2196_SD_PRESCALER_SAP_PROP_GAIN_LSB ;
     break;
#endif /*     SI2196_SD_PRESCALER_SAP_PROP */
#ifdef SI2196_SD_SOUND_MODE_PROP
     case SI2196_SD_SOUND_MODE_PROP_CODE:
       data = (p->sd_sound_mode.mode & SI2196_SD_SOUND_MODE_PROP_MODE_MASK) << SI2196_SD_SOUND_MODE_PROP_MODE_LSB ;
     break;
#endif /*     SI2196_SD_SOUND_MODE_PROP */
#ifdef SI2196_SD_SOUND_SYSTEM_PROP
     case SI2196_SD_SOUND_SYSTEM_PROP_CODE:
       data = (p->sd_sound_system.system & SI2196_SD_SOUND_SYSTEM_PROP_SYSTEM_MASK) << SI2196_SD_SOUND_SYSTEM_PROP_SYSTEM_LSB ;
     break;
#endif /*     SI2196_SD_SOUND_SYSTEM_PROP */
#ifdef SI2196_SD_STEREO_DM_ID_LVL_ACQ_PROP
     case SI2196_SD_STEREO_DM_ID_LVL_ACQ_PROP_CODE:
       data = (p->sd_stereo_dm_id_lvl_acq.acq & SI2196_SD_STEREO_DM_ID_LVL_ACQ_PROP_ACQ_MASK) << SI2196_SD_STEREO_DM_ID_LVL_ACQ_PROP_ACQ_LSB ;
     break;
    #endif /*     SI2196_SD_STEREO_DM_ID_LVL_ACQ_PROP */
#ifdef SI2196_SD_STEREO_DM_ID_LVL_SHIFT_PROP
     case SI2196_SD_STEREO_DM_ID_LVL_SHIFT_PROP_CODE:
       data = (p->sd_stereo_dm_id_lvl_shift.acq_shift   & SI2196_SD_STEREO_DM_ID_LVL_SHIFT_PROP_ACQ_SHIFT_MASK  ) << SI2196_SD_STEREO_DM_ID_LVL_SHIFT_PROP_ACQ_SHIFT_LSB  |
                  (p->sd_stereo_dm_id_lvl_shift.track_shift & SI2196_SD_STEREO_DM_ID_LVL_SHIFT_PROP_TRACK_SHIFT_MASK) << SI2196_SD_STEREO_DM_ID_LVL_SHIFT_PROP_TRACK_SHIFT_LSB ;
     break;
#endif /*     SI2196_SD_STEREO_DM_ID_LVL_SHIFT_PROP */
#ifdef SI2196_SD_STEREO_DM_ID_LVL_TRACK_PROP
     case SI2196_SD_STEREO_DM_ID_LVL_TRACK_PROP_CODE:
       data = (p->sd_stereo_dm_id_lvl_track.track & SI2196_SD_STEREO_DM_ID_LVL_TRACK_PROP_TRACK_MASK) << SI2196_SD_STEREO_DM_ID_LVL_TRACK_PROP_TRACK_LSB ;
     break;
#endif /*     SI2196_SD_STEREO_DM_ID_LVL_TRACK_PROP */
#ifdef SI2196_DTV_LIF_FREQ_PROP
        case SI2196_DTV_LIF_FREQ_PROP:
            data = (p->dtv_lif_freq.offset & SI2196_DTV_LIF_FREQ_PROP_OFFSET_MASK) << SI2196_DTV_LIF_FREQ_PROP_OFFSET_LSB ;
        break;
#endif /*     SI2196_DTV_LIF_FREQ_PROP */
#ifdef SI2196_DTV_LIF_OUT_PROP
        case SI2196_DTV_LIF_OUT_PROP:
            data = (p->dtv_lif_out.offset & SI2196_DTV_LIF_OUT_PROP_OFFSET_MASK) << SI2196_DTV_LIF_OUT_PROP_OFFSET_LSB  |
                       (p->dtv_lif_out.amp    & SI2196_DTV_LIF_OUT_PROP_AMP_MASK   ) << SI2196_DTV_LIF_OUT_PROP_AMP_LSB ;
        break;
#endif /*     SI2196_DTV_LIF_OUT_PROP */
#ifdef SI2196_DTV_MODE_PROP
        case SI2196_DTV_MODE_PROP:
            data = (p->dtv_mode.bw              & SI2196_DTV_MODE_PROP_BW_MASK             ) << SI2196_DTV_MODE_PROP_BW_LSB  |
                       (p->dtv_mode.modulation      & SI2196_DTV_MODE_PROP_MODULATION_MASK     ) << SI2196_DTV_MODE_PROP_MODULATION_LSB  |
                       (p->dtv_mode.invert_spectrum & SI2196_DTV_MODE_PROP_INVERT_SPECTRUM_MASK) << SI2196_DTV_MODE_PROP_INVERT_SPECTRUM_LSB ;
        break;
#endif /*     SI2196_DTV_MODE_PROP */
#ifdef SI2196_DTV_RF_TOP_PROP
        case SI2196_DTV_RF_TOP_PROP:
            data = (p->dtv_rf_top.dtv_rf_top & SI2196_DTV_RF_TOP_PROP_DTV_RF_TOP_MASK) << SI2196_DTV_RF_TOP_PROP_DTV_RF_TOP_LSB ;
        break;
#endif /*     SI2196_DTV_RF_TOP_PROP */
#ifdef SI2196_DTV_RSQ_RSSI_THRESHOLD_PROP
        case SI2196_DTV_RSQ_RSSI_THRESHOLD_PROP:
            data = (p->dtv_rsq_rssi_threshold.lo & SI2196_DTV_RSQ_RSSI_THRESHOLD_PROP_LO_MASK) << SI2196_DTV_RSQ_RSSI_THRESHOLD_PROP_LO_LSB  |
                       (p->dtv_rsq_rssi_threshold.hi & SI2196_DTV_RSQ_RSSI_THRESHOLD_PROP_HI_MASK) << SI2196_DTV_RSQ_RSSI_THRESHOLD_PROP_HI_LSB ;
        break;
#endif /*     SI2196_DTV_RSQ_RSSI_THRESHOLD_PROP */
#ifdef SI2196_MASTER_IEN_PROP
        case SI2196_MASTER_IEN_PROP:
            data = (p->master_ien.tunien & SI2196_MASTER_IEN_PROP_TUNIEN_MASK) << SI2196_MASTER_IEN_PROP_TUNIEN_LSB  |
                       (p->master_ien.atvien & SI2196_MASTER_IEN_PROP_ATVIEN_MASK) << SI2196_MASTER_IEN_PROP_ATVIEN_LSB  |
                       (p->master_ien.dtvien & SI2196_MASTER_IEN_PROP_DTVIEN_MASK) << SI2196_MASTER_IEN_PROP_DTVIEN_LSB  |
                       (p->master_ien.sdien  & SI2196_MASTER_IEN_PROP_SDIEN_MASK ) << SI2196_MASTER_IEN_PROP_SDIEN_LSB  |
                       (p->master_ien.errien & SI2196_MASTER_IEN_PROP_ERRIEN_MASK) << SI2196_MASTER_IEN_PROP_ERRIEN_LSB  |
                       (p->master_ien.ctsien & SI2196_MASTER_IEN_PROP_CTSIEN_MASK) << SI2196_MASTER_IEN_PROP_CTSIEN_LSB ;
        break;
#endif /*     SI2196_MASTER_IEN_PROP */
#ifdef SI2196_TUNER_BLOCKED_VCO_PROP
        case SI2196_TUNER_BLOCKED_VCO_PROP:
            data = (p->tuner_blocked_vco.vco_code & SI2196_TUNER_BLOCKED_VCO_PROP_VCO_CODE_MASK) << SI2196_TUNER_BLOCKED_VCO_PROP_VCO_CODE_LSB ;
        break;
#endif /*     SI2196_TUNER_BLOCKED_VCO_PROP */
#ifdef SI2196_TUNER_IEN_PROP
        case SI2196_TUNER_IEN_PROP:
            data = (p->tuner_ien.tcien    & SI2196_TUNER_IEN_PROP_TCIEN_MASK   ) << SI2196_TUNER_IEN_PROP_TCIEN_LSB  |
                       (p->tuner_ien.rssilien & SI2196_TUNER_IEN_PROP_RSSILIEN_MASK) << SI2196_TUNER_IEN_PROP_RSSILIEN_LSB  |
                       (p->tuner_ien.rssihien & SI2196_TUNER_IEN_PROP_RSSIHIEN_MASK) << SI2196_TUNER_IEN_PROP_RSSIHIEN_LSB ;
        break;
#endif /*     SI2196_TUNER_IEN_PROP */
#ifdef SI2196_TUNER_INT_SENSE_PROP
        case SI2196_TUNER_INT_SENSE_PROP:
            data = (p->tuner_int_sense.tcnegen    & SI2196_TUNER_INT_SENSE_PROP_TCNEGEN_MASK   ) << SI2196_TUNER_INT_SENSE_PROP_TCNEGEN_LSB  |
                       (p->tuner_int_sense.rssilnegen & SI2196_TUNER_INT_SENSE_PROP_RSSILNEGEN_MASK) << SI2196_TUNER_INT_SENSE_PROP_RSSILNEGEN_LSB  |
                       (p->tuner_int_sense.rssihnegen & SI2196_TUNER_INT_SENSE_PROP_RSSIHNEGEN_MASK) << SI2196_TUNER_INT_SENSE_PROP_RSSIHNEGEN_LSB  |
                       (p->tuner_int_sense.tcposen    & SI2196_TUNER_INT_SENSE_PROP_TCPOSEN_MASK   ) << SI2196_TUNER_INT_SENSE_PROP_TCPOSEN_LSB  |
                       (p->tuner_int_sense.rssilposen & SI2196_TUNER_INT_SENSE_PROP_RSSILPOSEN_MASK) << SI2196_TUNER_INT_SENSE_PROP_RSSILPOSEN_LSB  |
                       (p->tuner_int_sense.rssihposen & SI2196_TUNER_INT_SENSE_PROP_RSSIHPOSEN_MASK) << SI2196_TUNER_INT_SENSE_PROP_RSSIHPOSEN_LSB ;
        break;
#endif /*     SI2196_TUNER_INT_SENSE_PROP */
#ifdef SI2196_TUNER_LO_INJECTION_PROP
        case SI2196_TUNER_LO_INJECTION_PROP:
            data = (p->tuner_lo_injection.band_1 & SI2196_TUNER_LO_INJECTION_PROP_BAND_1_MASK) << SI2196_TUNER_LO_INJECTION_PROP_BAND_1_LSB  |
                       (p->tuner_lo_injection.band_2 & SI2196_TUNER_LO_INJECTION_PROP_BAND_2_MASK) << SI2196_TUNER_LO_INJECTION_PROP_BAND_2_LSB  |
                       (p->tuner_lo_injection.band_3 & SI2196_TUNER_LO_INJECTION_PROP_BAND_3_MASK) << SI2196_TUNER_LO_INJECTION_PROP_BAND_3_LSB  |
                       (p->tuner_lo_injection.band_4 & SI2196_TUNER_LO_INJECTION_PROP_BAND_4_MASK) << SI2196_TUNER_LO_INJECTION_PROP_BAND_4_LSB  |
                       (p->tuner_lo_injection.band_5 & SI2196_TUNER_LO_INJECTION_PROP_BAND_5_MASK) << SI2196_TUNER_LO_INJECTION_PROP_BAND_5_LSB ;
        break;
#endif /*     SI2196_TUNER_LO_INJECTION_PROP */
#ifdef SI2196_ATV_VSYNC_TRACKING_PROP
        case SI2196_ATV_VSYNC_TRACKING_PROP:
            data = (p->atv_vsync_tracking.min_pulses_to_lock   & SI2196_ATV_VSYNC_TRACKING_PROP_MIN_PULSES_TO_LOCK_MASK  ) << SI2196_ATV_VSYNC_TRACKING_PROP_MIN_PULSES_TO_LOCK_LSB  |
                       (p->atv_vsync_tracking.max_relock_retries   & SI2196_ATV_VSYNC_TRACKING_PROP_MAX_RELOCK_RETRIES_MASK  ) << SI2196_ATV_VSYNC_TRACKING_PROP_MAX_RELOCK_RETRIES_LSB  |
                       (p->atv_vsync_tracking.min_fields_to_unlock & SI2196_ATV_VSYNC_TRACKING_PROP_MIN_FIELDS_TO_UNLOCK_MASK) << SI2196_ATV_VSYNC_TRACKING_PROP_MIN_FIELDS_TO_UNLOCK_LSB ;
         break;
#endif /*     SI2196_ATV_VSYNC_TRACKING_PROP */
#ifdef SI2196_ATV_SOUND_AGC_SPEED_PROP_CODE
        case SI2196_ATV_SOUND_AGC_SPEED_PROP_CODE:
            data = (p->atv_sound_agc_speed.system_l & SI2196_ATV_SOUND_AGC_SPEED_PROP_SYSTEM_L_MASK     ) << SI2196_ATV_SOUND_AGC_SPEED_PROP_SYSTEM_L_LSB  |
                       (p->atv_sound_agc_speed.other_systems & SI2196_ATV_SOUND_AGC_SPEED_PROP_OTHER_SYSTEMS_MASK) << SI2196_ATV_SOUND_AGC_SPEED_PROP_OTHER_SYSTEMS_LSB ;
        break;
#endif
        default :
        break;
    }
    return si2196_setproperty(si2196, prop , data, rsp);
}
/* _set_property2_insertion_point */

/* _get_property2_insertion_start */

/* --------------------------------------------*/
/* GET_PROPERTY2 FUNCTION                       */
/* --------------------------------------------*/
unsigned char si2196_receiveproperty(struct i2c_client *si2196, unsigned int prop, si2196_propobj_t *p, si2196_cmdreplyobj_t *rsp)
{
    int data, res;

    res = si2196_getproperty(si2196, prop, &data, rsp);

    if (res!=NO_SI2196_ERROR)
        return res;
    switch (prop)
    {
#ifdef SI2196_ATV_AFC_RANGE_PROP
        case SI2196_ATV_AFC_RANGE_PROP:
            p->atv_afc_range.range_khz = (data >> SI2196_ATV_AFC_RANGE_PROP_RANGE_KHZ_LSB) & SI2196_ATV_AFC_RANGE_PROP_RANGE_KHZ_MASK;
        break;
#endif /*     SI2196_ATV_AFC_RANGE_PROP */
#ifdef SI2196_ATV_AF_OUT_PROP
        case SI2196_ATV_AF_OUT_PROP:
            p->atv_af_out.volume = (data >> SI2196_ATV_AF_OUT_PROP_VOLUME_LSB) & SI2196_ATV_AF_OUT_PROP_VOLUME_MASK;
        break;
#endif /*     SI2196_ATV_AF_OUT_PROP */
#ifdef SI2196_ATV_AGC_SPEED_PROP
        case SI2196_ATV_AGC_SPEED_PROP:
            p->atv_agc_speed.if_agc_speed = (data >> SI2196_ATV_AGC_SPEED_PROP_IF_AGC_SPEED_LSB) & SI2196_ATV_AGC_SPEED_PROP_IF_AGC_SPEED_MASK;
        break;
#endif /*     SI2196_ATV_AGC_SPEED_PROP */
#ifdef SI2196_ATV_AUDIO_MODE_PROP
        case SI2196_ATV_AUDIO_MODE_PROP:
            p->atv_audio_mode.audio_sys      = (data >> SI2196_ATV_AUDIO_MODE_PROP_AUDIO_SYS_FILT_LSB ) & SI2196_ATV_AUDIO_MODE_PROP_AUDIO_SYS_FILT_MASK;
            p->atv_audio_mode.chan_bw        = (data >> SI2196_ATV_AUDIO_MODE_PROP_CHAN_BW_LSB   ) & SI2196_ATV_AUDIO_MODE_PROP_CHAN_BW_MASK;
        break;
#endif /*     SI2196_ATV_AUDIO_MODE_PROP */
#ifdef SI2196_ATV_CVBS_OUT_PROP
        case SI2196_ATV_CVBS_OUT_PROP:
            p->atv_cvbs_out.offset = (data >> SI2196_ATV_CVBS_OUT_PROP_OFFSET_LSB) & SI2196_ATV_CVBS_OUT_PROP_OFFSET_MASK;
            p->atv_cvbs_out.amp   = (data >> SI2196_ATV_CVBS_OUT_PROP_AMP_LSB   ) & SI2196_ATV_CVBS_OUT_PROP_AMP_MASK;
        break;
#endif /*     SI2196_ATV_CVBS_OUT_PROP */
#ifdef SI2196_ATV_CVBS_OUT_FINE_PROP
        case SI2196_ATV_CVBS_OUT_FINE_PROP:
            p->atv_cvbs_out_fine.offset = (data >> SI2196_ATV_CVBS_OUT_FINE_PROP_OFFSET_LSB) & SI2196_ATV_CVBS_OUT_FINE_PROP_OFFSET_MASK;
            p->atv_cvbs_out_fine.amp   = (data >> SI2196_ATV_CVBS_OUT_FINE_PROP_AMP_LSB   ) & SI2196_ATV_CVBS_OUT_FINE_PROP_AMP_MASK;
        break;
#endif /*     SI2196_ATV_CVBS_OUT_FINE_PROP */
#ifdef SI2196_ATV_IEN_PROP
        case SI2196_ATV_IEN_PROP:
            p->atv_ien.chlien   = (data >> SI2196_ATV_IEN_PROP_CHLIEN_LSB ) & SI2196_ATV_IEN_PROP_CHLIEN_MASK;
            p->atv_ien.pclien   = (data >> SI2196_ATV_IEN_PROP_PCLIEN_LSB ) & SI2196_ATV_IEN_PROP_PCLIEN_MASK;
            p->atv_ien.dlien     = (data >> SI2196_ATV_IEN_PROP_DLIEN_LSB  ) & SI2196_ATV_IEN_PROP_DLIEN_MASK;
            p->atv_ien.snrlien  = (data >> SI2196_ATV_IEN_PROP_SNRLIEN_LSB) & SI2196_ATV_IEN_PROP_SNRLIEN_MASK;
            p->atv_ien.snrhien = (data >> SI2196_ATV_IEN_PROP_SNRHIEN_LSB) & SI2196_ATV_IEN_PROP_SNRHIEN_MASK;
        break;
#endif /*     SI2196_ATV_IEN_PROP */
#ifdef SI2196_ATV_INT_SENSE_PROP
        case SI2196_ATV_INT_SENSE_PROP:
            p->atv_int_sense.chlnegen  = (data >> SI2196_ATV_INT_SENSE_PROP_CHLNEGEN_LSB ) & SI2196_ATV_INT_SENSE_PROP_CHLNEGEN_MASK;
            p->atv_int_sense.pclnegen  = (data >> SI2196_ATV_INT_SENSE_PROP_PCLNEGEN_LSB ) & SI2196_ATV_INT_SENSE_PROP_PCLNEGEN_MASK;
            p->atv_int_sense.dlnegen    = (data >> SI2196_ATV_INT_SENSE_PROP_DLNEGEN_LSB  ) & SI2196_ATV_INT_SENSE_PROP_DLNEGEN_MASK;
            p->atv_int_sense.snrlnegen = (data >> SI2196_ATV_INT_SENSE_PROP_SNRLNEGEN_LSB) & SI2196_ATV_INT_SENSE_PROP_SNRLNEGEN_MASK;
            p->atv_int_sense.snrhnegen = (data >> SI2196_ATV_INT_SENSE_PROP_SNRHNEGEN_LSB) & SI2196_ATV_INT_SENSE_PROP_SNRHNEGEN_MASK;
            p->atv_int_sense.chlposen   = (data >> SI2196_ATV_INT_SENSE_PROP_CHLPOSEN_LSB ) & SI2196_ATV_INT_SENSE_PROP_CHLPOSEN_MASK;
            p->atv_int_sense.pclposen   = (data >> SI2196_ATV_INT_SENSE_PROP_PCLPOSEN_LSB ) & SI2196_ATV_INT_SENSE_PROP_PCLPOSEN_MASK;
            p->atv_int_sense.dlposen     = (data >> SI2196_ATV_INT_SENSE_PROP_DLPOSEN_LSB  ) & SI2196_ATV_INT_SENSE_PROP_DLPOSEN_MASK;
            p->atv_int_sense.snrlposen  = (data >> SI2196_ATV_INT_SENSE_PROP_SNRLPOSEN_LSB) & SI2196_ATV_INT_SENSE_PROP_SNRLPOSEN_MASK;
            p->atv_int_sense.snrhposen = (data >> SI2196_ATV_INT_SENSE_PROP_SNRHPOSEN_LSB) & SI2196_ATV_INT_SENSE_PROP_SNRHPOSEN_MASK;
        break;
#endif /*     SI2196_ATV_INT_SENSE_PROP */
#ifdef SI2196_ATV_MIN_LVL_LOCK_PROP
        case SI2196_ATV_MIN_LVL_LOCK_PROP:
            p->atv_min_lvl_lock.thrs = (data >> SI2196_ATV_MIN_LVL_LOCK_PROP_THRS_LSB) & SI2196_ATV_MIN_LVL_LOCK_PROP_THRS_MASK;
        break;
#endif /*     SI2196_ATV_MIN_LVL_LOCK_PROP */
#ifdef SI2196_ATV_RF_TOP_PROP
        case SI2196_ATV_RF_TOP_PROP:
            p->atv_rf_top.atv_rf_top = (data >> SI2196_ATV_RF_TOP_PROP_ATV_RF_TOP_LSB) & SI2196_ATV_RF_TOP_PROP_ATV_RF_TOP_MASK;
        break;
#endif /*     SI2196_ATV_RF_TOP_PROP */
#ifdef SI2196_ATV_RSQ_RSSI_THRESHOLD_PROP
        case SI2196_ATV_RSQ_RSSI_THRESHOLD_PROP:
            p->atv_rsq_rssi_threshold.lo = (data >> SI2196_ATV_RSQ_RSSI_THRESHOLD_PROP_LO_LSB) & SI2196_ATV_RSQ_RSSI_THRESHOLD_PROP_LO_MASK;
            p->atv_rsq_rssi_threshold.hi = (data >> SI2196_ATV_RSQ_RSSI_THRESHOLD_PROP_HI_LSB) & SI2196_ATV_RSQ_RSSI_THRESHOLD_PROP_HI_MASK;
        break;
#endif /*     SI2196_ATV_RSQ_RSSI_THRESHOLD_PROP */
#ifdef SI2196_ATV_RSQ_SNR_THRESHOLD_PROP
        case SI2196_ATV_RSQ_SNR_THRESHOLD_PROP:
            p->atv_rsq_snr_threshold.lo = (data >> SI2196_ATV_RSQ_SNR_THRESHOLD_PROP_LO_LSB) & SI2196_ATV_RSQ_SNR_THRESHOLD_PROP_LO_MASK;
            p->atv_rsq_snr_threshold.hi = (data >> SI2196_ATV_RSQ_SNR_THRESHOLD_PROP_HI_LSB) & SI2196_ATV_RSQ_SNR_THRESHOLD_PROP_HI_MASK;
        break;
#endif /*     SI2196_ATV_RSQ_SNR_THRESHOLD_PROP */
#ifdef SI2196_ATV_SIF_OUT_PROP
        case SI2196_ATV_SIF_OUT_PROP:
            p->atv_sif_out.offset  = (data >> SI2196_ATV_SIF_OUT_PROP_OFFSET_LSB) & SI2196_ATV_SIF_OUT_PROP_OFFSET_MASK;
            p->atv_sif_out.amp    = (data >> SI2196_ATV_SIF_OUT_PROP_AMP_LSB   ) & SI2196_ATV_SIF_OUT_PROP_AMP_MASK;
        break;
#endif /*     SI2196_ATV_SIF_OUT_PROP */
#ifdef SI2196_ATV_SOUND_AGC_LIMIT_PROP
        case SI2196_ATV_SOUND_AGC_LIMIT_PROP:
            p->atv_sound_agc_limit.max_gain = (data >> SI2196_ATV_SOUND_AGC_LIMIT_PROP_MAX_GAIN_LSB) & SI2196_ATV_SOUND_AGC_LIMIT_PROP_MAX_GAIN_MASK;
            p->atv_sound_agc_limit.min_gain  = (data >> SI2196_ATV_SOUND_AGC_LIMIT_PROP_MIN_GAIN_LSB) & SI2196_ATV_SOUND_AGC_LIMIT_PROP_MIN_GAIN_MASK;
        break;
#endif /*     SI2196_ATV_SOUND_AGC_LIMIT_PROP */
#ifdef SI2196_ATV_VIDEO_EQUALIZER_PROP
        case SI2196_ATV_VIDEO_EQUALIZER_PROP:
            p->atv_video_equalizer.slope = (data >> SI2196_ATV_VIDEO_EQUALIZER_PROP_SLOPE_LSB) & SI2196_ATV_VIDEO_EQUALIZER_PROP_SLOPE_MASK;
        break;
#endif /*     SI2196_ATV_VIDEO_EQUALIZER_PROP */
#ifdef SI2196_ATV_VIDEO_MODE_PROP
        case SI2196_ATV_VIDEO_MODE_PROP:
            p->atv_video_mode.video_sys     = (data >> SI2196_ATV_VIDEO_MODE_PROP_VIDEO_SYS_LSB      ) & SI2196_ATV_VIDEO_MODE_PROP_VIDEO_SYS_MASK;
            p->atv_video_mode.color             = (data >> SI2196_ATV_VIDEO_MODE_PROP_COLOR_LSB          ) & SI2196_ATV_VIDEO_MODE_PROP_COLOR_MASK;
            p->atv_video_mode.trans             = (data >> SI2196_ATV_VIDEO_MODE_PROP_TRANS_LSB          ) & SI2196_ATV_VIDEO_MODE_PROP_TRANS_MASK;
            p->atv_video_mode.invert_signal = (data >> SI2196_ATV_VIDEO_MODE_PROP_INVERT_SIGNAL_LSB  ) & SI2196_ATV_VIDEO_MODE_PROP_INVERT_SIGNAL_MASK;
        break;
#endif /*     SI2196_ATV_VIDEO_MODE_PROP */
#ifdef SI2196_ATV_VSNR_CAP_PROP
        case SI2196_ATV_VSNR_CAP_PROP:
            p->atv_vsnr_cap.atv_vsnr_cap = (data >> SI2196_ATV_VSNR_CAP_PROP_ATV_VSNR_CAP_LSB) & SI2196_ATV_VSNR_CAP_PROP_ATV_VSNR_CAP_MASK;
        break;
#endif /*     SI2196_ATV_VSNR_CAP_PROP */
#ifdef SI2196_ATV_VSYNC_TRACKING_PROP
        case SI2196_ATV_VSYNC_TRACKING_PROP:
            p->atv_vsync_tracking.min_pulses_to_lock   = (data >> SI2196_ATV_VSYNC_TRACKING_PROP_MIN_PULSES_TO_LOCK_LSB  ) & SI2196_ATV_VSYNC_TRACKING_PROP_MIN_PULSES_TO_LOCK_MASK;
            p->atv_vsync_tracking.min_fields_to_unlock = (data >> SI2196_ATV_VSYNC_TRACKING_PROP_MIN_FIELDS_TO_UNLOCK_LSB) & SI2196_ATV_VSYNC_TRACKING_PROP_MIN_FIELDS_TO_UNLOCK_MASK;
        break;
#endif /*     SI2196_ATV_VSYNC_TRACKING_PROP */
#ifdef SI2196_CRYSTAL_TRIM_PROP
        case SI2196_CRYSTAL_TRIM_PROP:
            p->crystal_trim.xo_cap = (data >> SI2196_CRYSTAL_TRIM_PROP_XO_CAP_LSB) & SI2196_CRYSTAL_TRIM_PROP_XO_CAP_MASK;
        break;
#endif /*     SI2196_CRYSTAL_TRIM_PROP */
#ifdef SI2196_MASTER_IEN_PROP
        case SI2196_MASTER_IEN_PROP:
            p->master_ien.tunien = (data >> SI2196_MASTER_IEN_PROP_TUNIEN_LSB) & SI2196_MASTER_IEN_PROP_TUNIEN_MASK;
            p->master_ien.atvien = (data >> SI2196_MASTER_IEN_PROP_ATVIEN_LSB) & SI2196_MASTER_IEN_PROP_ATVIEN_MASK;
            p->master_ien.dtvien = (data >> SI2196_MASTER_IEN_PROP_DTVIEN_LSB) & SI2196_MASTER_IEN_PROP_DTVIEN_MASK;
            p->master_ien.errien = (data >> SI2196_MASTER_IEN_PROP_ERRIEN_LSB) & SI2196_MASTER_IEN_PROP_ERRIEN_MASK;
            p->master_ien.ctsien = (data >> SI2196_MASTER_IEN_PROP_CTSIEN_LSB) & SI2196_MASTER_IEN_PROP_CTSIEN_MASK;
        break;
#endif /*     SI2196_MASTER_IEN_PROP */
#ifdef SI2196_TUNER_BLOCKED_VCO_PROP
        case SI2196_TUNER_BLOCKED_VCO_PROP:
            p->tuner_blocked_vco.vco_code = (data >> SI2196_TUNER_BLOCKED_VCO_PROP_VCO_CODE_LSB) & SI2196_TUNER_BLOCKED_VCO_PROP_VCO_CODE_MASK;
        break;
#endif /*     SI2196_TUNER_BLOCKED_VCO_PROP */
#ifdef SI2196_TUNER_IEN_PROP
        case SI2196_TUNER_IEN_PROP:
            p->tuner_ien.tcien      = (data >> SI2196_TUNER_IEN_PROP_TCIEN_LSB   ) & SI2196_TUNER_IEN_PROP_TCIEN_MASK;
            p->tuner_ien.rssilien  = (data >> SI2196_TUNER_IEN_PROP_RSSILIEN_LSB) & SI2196_TUNER_IEN_PROP_RSSILIEN_MASK;
            p->tuner_ien.rssihien = (data >> SI2196_TUNER_IEN_PROP_RSSIHIEN_LSB) & SI2196_TUNER_IEN_PROP_RSSIHIEN_MASK;
        break;
#endif /*     SI2196_TUNER_IEN_PROP */
#ifdef SI2196_TUNER_INT_SENSE_PROP
        case SI2196_TUNER_INT_SENSE_PROP:
            p->tuner_int_sense.tcnegen      = (data >> SI2196_TUNER_INT_SENSE_PROP_TCNEGEN_LSB   ) & SI2196_TUNER_INT_SENSE_PROP_TCNEGEN_MASK;
            p->tuner_int_sense.rssilnegen  = (data >> SI2196_TUNER_INT_SENSE_PROP_RSSILNEGEN_LSB) & SI2196_TUNER_INT_SENSE_PROP_RSSILNEGEN_MASK;
            p->tuner_int_sense.rssihnegen = (data >> SI2196_TUNER_INT_SENSE_PROP_RSSIHNEGEN_LSB) & SI2196_TUNER_INT_SENSE_PROP_RSSIHNEGEN_MASK;
            p->tuner_int_sense.tcposen      = (data >> SI2196_TUNER_INT_SENSE_PROP_TCPOSEN_LSB   ) & SI2196_TUNER_INT_SENSE_PROP_TCPOSEN_MASK;
            p->tuner_int_sense.rssilposen  = (data >> SI2196_TUNER_INT_SENSE_PROP_RSSILPOSEN_LSB) & SI2196_TUNER_INT_SENSE_PROP_RSSILPOSEN_MASK;
            p->tuner_int_sense.rssihposen = (data >> SI2196_TUNER_INT_SENSE_PROP_RSSIHPOSEN_LSB) & SI2196_TUNER_INT_SENSE_PROP_RSSIHPOSEN_MASK;
        break;
#endif /*     SI2196_TUNER_INT_SENSE_PROP */
#ifdef SI2196_TUNER_LO_INJECTION_PROP
        case SI2196_TUNER_LO_INJECTION_PROP:
            p->tuner_lo_injection.band_1 = (data >> SI2196_TUNER_LO_INJECTION_PROP_BAND_1_LSB) & SI2196_TUNER_LO_INJECTION_PROP_BAND_1_MASK;
            p->tuner_lo_injection.band_2 = (data >> SI2196_TUNER_LO_INJECTION_PROP_BAND_2_LSB) & SI2196_TUNER_LO_INJECTION_PROP_BAND_2_MASK;
            p->tuner_lo_injection.band_3 = (data >> SI2196_TUNER_LO_INJECTION_PROP_BAND_3_LSB) & SI2196_TUNER_LO_INJECTION_PROP_BAND_3_MASK;
            p->tuner_lo_injection.band_4 = (data >> SI2196_TUNER_LO_INJECTION_PROP_BAND_4_LSB) & SI2196_TUNER_LO_INJECTION_PROP_BAND_4_MASK;
            p->tuner_lo_injection.band_5 = (data >> SI2196_TUNER_LO_INJECTION_PROP_BAND_5_LSB) & SI2196_TUNER_LO_INJECTION_PROP_BAND_5_MASK;
        break;
#endif /*     SI2196_TUNER_LO_INJECTION_PROP */
        default :
        break;
    }
    return res;
}
void si2196_setupsddefaults (si2196_propobj_t *prop)
{
  prop->sd_prescaler_am.gain                     =-6; /* (default    -6) */
  prop->sd_prescaler_eiaj.gain                    = 14; /* (default    14) */
  prop->sd_prescaler_fm.gain                      = 2; /* (default     2) */
  prop->sd_prescaler_nicam.gain                = -9; /* (default    -9) */
  prop->sd_prescaler_sap.gain                    = 2; /* (default     2) */
  
  prop->sd_sound_mode.mode                    = SI2196_SD_SOUND_MODE_PROP_MODE_AUTODETECT ; /* (default 'AUTODETECT') */
  prop->sd_sound_system.system               = SI2196_SD_SOUND_SYSTEM_PROP_SYSTEM_AUTODETECT ; /* (default 'AUTODETECT') */

  prop->sd_i2s.lrclk_pol                               = SI2196_SD_I2S_PROP_LRCLK_POL_LEFT_0_RIGHT_1      ; /* (default 'LEFT_0_RIGHT_1') */
  prop->sd_i2s.alignment                             = SI2196_SD_I2S_PROP_ALIGNMENT_I2S                 ; /* (default 'I2S') */
  prop->sd_i2s.lrclk_rate                              = SI2196_SD_I2S_PROP_LRCLK_RATE_48KHZ              ; /* (default '48KHZ') */
  prop->sd_i2s.num_bits                              = SI2196_SD_I2S_PROP_NUM_BITS_24                   ; /* (default '24') */
  prop->sd_i2s.sclk_rate                              = SI2196_SD_I2S_PROP_SCLK_RATE_64X                 ; /* (default '64X') */
  prop->sd_i2s.drive_strength                      = 1; /* (default     1) */

  prop->sd_over_dev_mode.mode               = SI2196_SD_OVER_DEV_MODE_PROP_MODE_AUTODETECT ; /* (default 'AUTODETECT') */

  prop->sd_over_dev_mute.mute_thresh     = 20; /* (default    20) */
  prop->sd_over_dev_mute.unmute_thresh = 18; /* (default    18) */

  prop->sd_afc_mute.mute_thresh               = 150; /* (default   150) */
  prop->sd_afc_mute.unmute_thresh           = 130; /* (default   130) */
  
  prop->sd_carrier_mute.primary_thresh      =    22; /* (default    22) */
  prop->sd_carrier_mute.secondary_thresh =    26; /* (default    26) */

  prop->sd_asd.iterations                             = 3; /* (default     3) */
  prop->sd_asd.enable_nicam_l                   = SI2196_SD_ASD_PROP_ENABLE_NICAM_L_ENABLE  ; /* (default 'ENABLE') */
  prop->sd_asd.enable_nicam_dk                = SI2196_SD_ASD_PROP_ENABLE_NICAM_DK_ENABLE ; /* (default 'ENABLE') */
  prop->sd_asd.enable_nicam_i                   = SI2196_SD_ASD_PROP_ENABLE_NICAM_I_ENABLE  ; /* (default 'ENABLE') */
  prop->sd_asd.enable_nicam_bg                = SI2196_SD_ASD_PROP_ENABLE_NICAM_BG_ENABLE ; /* (default 'ENABLE') */
  prop->sd_asd.enable_a2_m                       = SI2196_SD_ASD_PROP_ENABLE_A2_M_ENABLE     ; /* (default 'ENABLE') */
  prop->sd_asd.enable_a2_dk                      = SI2196_SD_ASD_PROP_ENABLE_A2_DK_ENABLE    ; /* (default 'ENABLE') */
  prop->sd_asd.enable_a2_bg                      = SI2196_SD_ASD_PROP_ENABLE_A2_BG_ENABLE    ; /* (default 'ENABLE') */
  prop->sd_asd.enable_eiaj                          = SI2196_SD_ASD_PROP_ENABLE_EIAJ_ENABLE     ; /* (default 'ENABLE') */
  prop->sd_asd.enable_btsc                         = SI2196_SD_ASD_PROP_ENABLE_BTSC_ENABLE     ; /* (default 'ENABLE') */

  prop->sd_ien.asdcien                                = SI2196_SD_IEN_PROP_ASDCIEN_ENABLE  ; /* (default 'DISABLE') */
  prop->sd_ien.nicamien                              = SI2196_SD_IEN_PROP_NICAMIEN_DISABLE ; /* (default 'DISABLE') */
  prop->sd_ien.pcmien                                 = SI2196_SD_IEN_PROP_PCMIEN_DISABLE   ; /* (default 'DISABLE') */
  prop->sd_ien.scmien                                 = SI2196_SD_IEN_PROP_SCMIEN_DISABLE   ; /* (default 'DISABLE') */
  prop->sd_ien.odmien                                 = SI2196_SD_IEN_PROP_ODMIEN_DISABLE   ; /* (default 'DISABLE') */
  prop->sd_ien.afcmien                                = SI2196_SD_IEN_PROP_AFCMIEN_DISABLE  ; /* (default 'DISABLE') */
  prop->sd_ien.ssien                                    = SI2196_SD_IEN_PROP_SSIEN_DISABLE    ; /* (default 'DISABLE') */
  prop->sd_ien.agcsien                                = SI2196_SD_IEN_PROP_AGCSIEN_DISABLE  ; /* (default 'DISABLE') */

  prop->sd_int_sense.asdcnegen              = SI2196_SD_INT_SENSE_PROP_ASDCNEGEN_DISABLE  ; /* (default 'DISABLE') */
  prop->sd_int_sense.nicamnegen            = SI2196_SD_INT_SENSE_PROP_NICAMNEGEN_DISABLE ; /* (default 'DISABLE') */
  prop->sd_int_sense.pcmnegen               = SI2196_SD_INT_SENSE_PROP_PCMNEGEN_DISABLE   ; /* (default 'DISABLE') */
  prop->sd_int_sense.scmnegen               = SI2196_SD_INT_SENSE_PROP_SCMNEGEN_DISABLE   ; /* (default 'DISABLE') */
  prop->sd_int_sense.odmnegen               = SI2196_SD_INT_SENSE_PROP_ODMNEGEN_DISABLE   ; /* (default 'DISABLE') */
  prop->sd_int_sense.afcmnegen              = SI2196_SD_INT_SENSE_PROP_AFCMNEGEN_DISABLE  ; /* (default 'DISABLE') */
  prop->sd_int_sense.agcsnegen               = SI2196_SD_INT_SENSE_PROP_AGCSNEGEN_DISABLE  ; /* (default 'DISABLE') */
  prop->sd_int_sense.asdcposen               = SI2196_SD_INT_SENSE_PROP_ASDCPOSEN_ENABLE   ; /* (default 'ENABLE') */
  prop->sd_int_sense.nicamposen             = SI2196_SD_INT_SENSE_PROP_NICAMPOSEN_ENABLE  ; /* (default 'ENABLE') */
  prop->sd_int_sense.pcmposen                = SI2196_SD_INT_SENSE_PROP_PCMPOSEN_ENABLE    ; /* (default 'ENABLE') */
  prop->sd_int_sense.scmposen                = SI2196_SD_INT_SENSE_PROP_SCMPOSEN_ENABLE    ; /* (default 'ENABLE') */
  prop->sd_int_sense.odmposen                = SI2196_SD_INT_SENSE_PROP_ODMPOSEN_ENABLE    ; /* (default 'ENABLE') */
  prop->sd_int_sense.afcmposen               = SI2196_SD_INT_SENSE_PROP_AFCMPOSEN_ENABLE   ; /* (default 'ENABLE') */
  prop->sd_int_sense.agcsposen               = SI2196_SD_INT_SENSE_PROP_AGCSPOSEN_ENABLE   ; /* (default 'ENABLE') */

  
  prop->sd_pilot_lvl_ctrl.lose_lvl                   = 26; /* (default    26) */
  prop->sd_pilot_lvl_ctrl.acq_lvl                    = 53; /* (default    53) */

  
  prop->sd_port_volume_left.volume           = 0; /* (default     0) */
  prop->sd_port_volume_right.volume         = 0; /* (default     0) */

  
  prop->sd_port_config.balance_mode        = SI2196_SD_PORT_CONFIG_PROP_BALANCE_MODE_BALANCE ; /* (default 'BALANCE') */
  prop->sd_port_config.port                         = SI2196_SD_PORT_CONFIG_PROP_PORT_DAC;//SI2196_SD_PORT_CONFIG_PROP_PORT_I2S             ; /* (default 'I2S') */
  prop->sd_port_mute.mute                         = SI2196_SD_PORT_MUTE_PROP_MUTE_MUTE_NONE ; /* (default 'MUTE_NONE') */
  prop->sd_port_volume_balance.balance  = 0; /* (default     0) */

  prop->sd_nicam_failover_thresh.errors    = 768; /* (default   768) */
  prop->sd_nicam_recover_thresh.errors    = 160; /* (default   160) */

  prop->sd_lang_select.lang                       = SI2196_SD_LANG_SELECT_PROP_LANG_LANG_A ; /* (default 'LANG_A') */

  prop->sd_nicam.num_frames                   = 200; /* (default   200) */
  prop->sd_nicam.force_rss                        = SI2196_SD_NICAM_PROP_FORCE_RSS_NORMAL  ; /* (default 'NORMAL') */

  prop->sd_port_volume_master.volume     = 20; /* (default     0) */
  prop->sd_afc_max.max_afc                      = 150; /* (default   150) */
  prop->sd_agc.gain                                    = 16; /* (default    16) */
  prop->sd_agc.freeze                                 = SI2196_SD_AGC_PROP_FREEZE_NORMAL ; /* (default 'NORMAL') */

  prop->sd_stereo_dm_id_lvl_acq.acq          =  1000; /* (default  1000) */

  prop->sd_stereo_dm_id_lvl_shift.acq_shift  = 7; /* (default     7) */
  prop->sd_stereo_dm_id_lvl_shift.track_shift = 1; /* (default     1) */

  prop->sd_stereo_dm_id_lvl_track.track        = 500; /* (default   500) */

};


/************************************************************************************************************************
  NAME: si2196_SetupATVDefaults
  DESCRIPTION: Setup si2196 ATV startup configuration
  This is a list of all the ATV configuration properties.   Depending on your application, only a subset may be required.
  The properties are stored in the global property structure 'prop'.  The function ATVConfig(..) must be called
  after any properties are modified.
  Parameter:  none
  Returns:    0 if successful
  Programming Guide Reference:    Flowchart A.5 (ATV Setup flowchart)

************************************************************************************************************************/
static int si2196_setupatvdefaults(si2196_propobj_t *prop)
{
    prop->atv_cvbs_out.amp                                     = 100;
    prop->atv_cvbs_out.offset                                   =  30;
    prop->atv_cvbs_out_fine.amp                              = 100;
    prop->atv_cvbs_out_fine.offset                            = 0;
    prop->atv_sound_agc_limit.min_gain                   = -84;
    prop->atv_sound_agc_limit.max_gain                  = 30;
    prop->atv_sound_agc_speed.system_l                = 5; /* (default     5) */
    prop->atv_sound_agc_speed.other_systems       = 4; /* (default     4) */
    prop->atv_afc_range.range_khz                          = 2000;
    prop->atv_rf_top.atv_rf_top                                  = SI2196_ATV_RF_TOP_PROP_ATV_RF_TOP_AUTO;
    
    prop->atv_vsync_tracking.min_pulses_to_lock    = 2;
    prop->atv_vsync_tracking.max_relock_retries     = 0;
    prop->atv_vsync_tracking.min_fields_to_unlock  = 16;
    
    prop->atv_vsnr_cap.atv_vsnr_cap                       = 0; /* (default     0) */
    prop->atv_vsnr_cap.frontend_noise                    = 0; /* (default     0) */
    prop->atv_vsnr_cap.backend_noise                    = 0;

    prop->atv_agc_speed.if_agc_speed                    = 178;//patch for skyworth
    
    prop->atv_rsq_rssi_threshold.hi                          = 0;
    prop->atv_rsq_rssi_threshold.lo                          = -127;
    
    prop->atv_rsq_snr_threshold.hi                          = 60;//40;
    prop->atv_rsq_snr_threshold.lo                          = 10;//25;
    
    prop->atv_video_equalizer.slope                        = 0;
    
    prop->atv_int_sense.chlnegen                   = SI2196_ATV_INT_SENSE_PROP_CHLNEGEN_DISABLE;
    prop->atv_int_sense.chlposen                   = SI2196_ATV_INT_SENSE_PROP_CHLPOSEN_ENABLE;
    prop->atv_int_sense.dlnegen                     = SI2196_ATV_INT_SENSE_PROP_DLNEGEN_DISABLE;
    prop->atv_int_sense.dlposen                     = SI2196_ATV_INT_SENSE_PROP_DLPOSEN_ENABLE;
    prop->atv_int_sense.pclnegen                   = SI2196_ATV_INT_SENSE_PROP_PCLNEGEN_DISABLE;
    prop->atv_int_sense.pclposen                   = SI2196_ATV_INT_SENSE_PROP_PCLPOSEN_ENABLE;
    prop->atv_int_sense.snrhnegen                = SI2196_ATV_INT_SENSE_PROP_SNRHNEGEN_DISABLE;
    prop->atv_int_sense.snrhposen                = SI2196_ATV_INT_SENSE_PROP_SNRHPOSEN_ENABLE;
    prop->atv_int_sense.snrlnegen                 = SI2196_ATV_INT_SENSE_PROP_SNRLNEGEN_DISABLE;
    prop->atv_int_sense.snrlposen                 = SI2196_ATV_INT_SENSE_PROP_SNRLPOSEN_ENABLE;

    prop->atv_ien.chlien                                 = SI2196_ATV_IEN_PROP_CHLIEN_ENABLE;     /* enable only CHL to drive ATVINT */
    prop->atv_ien.dlien                                   = SI2196_ATV_IEN_PROP_DLIEN_ENABLE;
    prop->atv_ien.pclien                                 = SI2196_ATV_IEN_PROP_PCLIEN_ENABLE;
    prop->atv_ien.snrhien                               = SI2196_ATV_IEN_PROP_SNRHIEN_ENABLE;
    prop->atv_ien.snrlien                                = SI2196_ATV_IEN_PROP_SNRLIEN_ENABLE;

    prop->atv_audio_mode.audio_sys            = 0;//SI2196_ATV_AUDIO_MODE_PROP_AUDIO_SYS_FILT_MONO;
    prop->atv_audio_mode.chan_bw              = SI2196_ATV_AUDIO_MODE_PROP_CHAN_BW_DEFAULT;

    prop->atv_video_mode.video_sys             = 0;//SI2196_ATV_VIDEO_MODE_PROP_VIDEO_SYS_DK;
    prop->atv_video_mode.trans                     = SI2196_ATV_VIDEO_MODE_PROP_TRANS_CABLE;//SI2196_ATV_VIDEO_MODE_PROP_TRANS_TERRESTRIAL;
    prop->atv_video_mode.color                     = SI2196_ATV_VIDEO_MODE_PROP_COLOR_PAL_NTSC;
    prop->atv_video_mode.invert_signal         = 0; /* (default     0) */
    return 0;
}
/************************************************************************************************************************
  NAME: si2196_SetupCommonDefaults
  DESCRIPTION: Setup si2196 Common startup configuration
  This is a list of all the common configuration properties.   Depending on your application, only a subset may be required.
  The properties are stored in the global property structure 'prop'.  The function CommonConfig(..) must be called
  after any of these properties are modified.
  Parameter:  none
  Returns:    0 if successful
  Programming Guide Reference:    Flowchart A.6a (Common setup flowchart)
************************************************************************************************************************/
static int si2196_setupcommondefaults(si2196_propobj_t *prop)
{
    /**** Enable Interrupt Sources *******/
    prop->master_ien.atvien = SI2196_MASTER_IEN_PROP_ATVIEN_ON;
    prop->master_ien.ctsien = SI2196_MASTER_IEN_PROP_CTSIEN_ON;
    prop->master_ien.dtvien = SI2196_MASTER_IEN_PROP_DTVIEN_OFF;//SI2196_MASTER_IEN_PROP_DTVIEN_ON;
    prop->master_ien.errien  = SI2196_MASTER_IEN_PROP_ERRIEN_ON;
    prop->master_ien.sdien  = SI2196_MASTER_IEN_PROP_SDIEN_OFF; 
    prop->master_ien.tunien = SI2196_MASTER_IEN_PROP_TUNIEN_ON;
    /* Crystal Trim adjustment */
    prop->crystal_trim.xo_cap = 8;
    return 0;
}
/************************************************************************************************************************
  NAME: si2196_SetupTunerDefaults
  DESCRIPTION: Setup si2196 Tuner startup configuration
  This is a list of all the Tuner configuration properties.   Depending on your application, only a subset may be required.
  The properties are stored in the global property structure 'prop'.  The function TunerConfig(..) must be called
  after any of these properties are modified.
  Parameter:  none
  Returns:    0 if successful
  Programming Guide Reference:    Flowchart A.6a (Tuner setup flowchart)
************************************************************************************************************************/
static int si2196_setuptunerdefaults(si2196_propobj_t *prop)
{

    /* Setting si2196_TUNER_IEN_PROP property */
    prop->tuner_ien.tcien               = SI2196_TUNER_IEN_PROP_TCIEN_ENABLE; /* enable only TC to drive TUNINT */
    prop->tuner_ien.rssilien           = SI2196_TUNER_IEN_PROP_RSSILIEN_ENABLE;
    prop->tuner_ien.rssihien          = SI2196_TUNER_IEN_PROP_RSSIHIEN_DISABLE;

    /* Setting si2196_TUNER_BLOCK_VCO_PROP property */
    prop->tuner_blocked_vco.vco_code   = 0x8000;

    /* Setting si2196_TUNER_INT_SENSE_PROP property */
    prop->tuner_int_sense.rssihnegen  = SI2196_TUNER_INT_SENSE_PROP_RSSIHNEGEN_DISABLE;
    prop->tuner_int_sense.rssihposen  = SI2196_TUNER_INT_SENSE_PROP_RSSIHPOSEN_ENABLE;
    prop->tuner_int_sense.rssilnegen   = SI2196_TUNER_INT_SENSE_PROP_RSSILNEGEN_DISABLE;
    prop->tuner_int_sense.rssilposen   = SI2196_TUNER_INT_SENSE_PROP_RSSILPOSEN_ENABLE;
    prop->tuner_int_sense.tcnegen      = SI2196_TUNER_INT_SENSE_PROP_RSSIHNEGEN_DISABLE;
    prop->tuner_int_sense.tcposen      = SI2196_TUNER_INT_SENSE_PROP_TCPOSEN_ENABLE;


   /* Setting si2196_TUNER_LO_INJECTION_PROP property */
    prop->tuner_lo_injection.band_1     = SI2196_TUNER_LO_INJECTION_PROP_BAND_1_HIGH_SIDE;
    prop->tuner_lo_injection.band_2     = SI2196_TUNER_LO_INJECTION_PROP_BAND_2_LOW_SIDE;
    prop->tuner_lo_injection.band_3     = SI2196_TUNER_LO_INJECTION_PROP_BAND_3_LOW_SIDE;
    prop->tuner_lo_injection.band_4     = SI2196_TUNER_LO_INJECTION_PROP_BAND_4_LOW_SIDE;
    prop->tuner_lo_injection.band_5     = SI2196_TUNER_LO_INJECTION_PROP_BAND_5_LOW_SIDE;

    return 0;
}
#if 0
void si2196_setupdtvdefaults (si2196_propobj_t *prop)
{
  prop->dtv_agc_freeze_input.level               = SI2196_DTV_AGC_FREEZE_INPUT_PROP_LEVEL_LOW  ; /* (default 'LOW') */
  prop->dtv_agc_freeze_input.pin                 = SI2196_DTV_AGC_FREEZE_INPUT_PROP_PIN_NONE   ; /* (default 'NONE') */

  prop->dtv_agc_speed.if_agc_speed               = SI2196_DTV_AGC_SPEED_PROP_IF_AGC_SPEED_AUTO ; /* (default 'AUTO') */
  prop->dtv_agc_speed.agc_decim                  = SI2196_DTV_AGC_SPEED_PROP_AGC_DECIM_OFF     ; /* (default 'OFF') */

  prop->dtv_config_if_port.dtv_out_type          = SI2196_DTV_CONFIG_IF_PORT_PROP_DTV_OUT_TYPE_LIF_IF1   ; /* (default 'LIF_IF1') */
  prop->dtv_config_if_port.dtv_agc_source        =     0; /* (default     0) */

  prop->dtv_ext_agc.min_10mv                     =    50; /* (default    50) */
  prop->dtv_ext_agc.max_10mv                     =   200; /* (default   200) */

  prop->dtv_filter_select.filter                 = SI2196_DTV_FILTER_SELECT_PROP_FILTER_DEFAULT ; /* (default 'DEFAULT') */

  prop->dtv_ien.chlien                           = SI2196_DTV_IEN_PROP_CHLIEN_ENABLE ; /* (default 'DISABLE') */

  prop->dtv_initial_agc_speed.if_agc_speed       = SI2196_DTV_INITIAL_AGC_SPEED_PROP_IF_AGC_SPEED_AUTO ; /* (default 'AUTO') */
  prop->dtv_initial_agc_speed.agc_decim          = SI2196_DTV_INITIAL_AGC_SPEED_PROP_AGC_DECIM_OFF     ; /* (default 'OFF') */

  prop->dtv_initial_agc_speed_period.period      =     0; /* (default     0) */

  prop->dtv_internal_zif.atsc                    = SI2196_DTV_INTERNAL_ZIF_PROP_ATSC_LIF   ; /* (default 'LIF') */
  prop->dtv_internal_zif.qam_us              = SI2196_DTV_INTERNAL_ZIF_PROP_QAM_US_LIF ; /* (default 'LIF') */
  prop->dtv_internal_zif.dvbt                    = SI2196_DTV_INTERNAL_ZIF_PROP_DVBT_LIF   ; /* (default 'LIF') */
  prop->dtv_internal_zif.dvbc                    = SI2196_DTV_INTERNAL_ZIF_PROP_DVBC_LIF   ; /* (default 'LIF') */
  prop->dtv_internal_zif.isdbt                   = SI2196_DTV_INTERNAL_ZIF_PROP_ISDBT_LIF  ; /* (default 'LIF') */
  prop->dtv_internal_zif.isdbc                   = SI2196_DTV_INTERNAL_ZIF_PROP_ISDBC_LIF  ; /* (default 'LIF') */
  prop->dtv_internal_zif.dtmb                    = SI2196_DTV_INTERNAL_ZIF_PROP_DTMB_LIF   ; /* (default 'LIF') */

  prop->dtv_int_sense.chlnegen                   = SI2196_DTV_INT_SENSE_PROP_CHLNEGEN_DISABLE ; /* (default 'DISABLE') */
  prop->dtv_int_sense.chlposen                   = SI2196_DTV_INT_SENSE_PROP_CHLPOSEN_ENABLE  ; /* (default 'ENABLE') */

  prop->dtv_lif_freq.offset                      =  5000; /* (default  5000) */

  prop->dtv_lif_out.offset                       =   148; /* (default   148) */
  prop->dtv_lif_out.amp                          =    27; /* (default    27) */

  prop->dtv_mode.bw                              = SI2196_DTV_MODE_PROP_BW_BW_8MHZ              ; /* (default 'BW_8MHZ') */
  prop->dtv_mode.modulation                      = SI2196_DTV_MODE_PROP_MODULATION_DVBT         ; /* (default 'DVBT') */
  prop->dtv_mode.invert_spectrum                 =     0; /* (default     0) */

  prop->dtv_rf_top.dtv_rf_top                    = SI2196_DTV_RF_TOP_PROP_DTV_RF_TOP_AUTO ; /* (default 'AUTO') */

  prop->dtv_rsq_rssi_threshold.lo                =   -80; /* (default   -80) */
  prop->dtv_rsq_rssi_threshold.hi                =     0; /* (default     0) */

}

/*****************************************************************************************
 NAME: SI2196_downloadDTVProperties
  DESCRIPTION: Setup SI2196 DTV properties configuration
  This function will download all the DTV configuration properties.
  The function SetupDTVDefaults() should be called before the first call to this function.
  Parameter:  Pointer to SI2196 Context
  Returns:    I2C transaction error code, NO_SI2196_ERROR if successful
  Programming Guide Reference:    DTV setup flowchart
******************************************************************************************/
int  si2196_dtvconfig(si2196_propobj_t *prop)
{
#ifdef    SI2196_DTV_AGC_FREEZE_INPUT_PROP
  if (SI2196_L1_SetProperty2(api, SI2196_DTV_AGC_FREEZE_INPUT_PROP_CODE        ) != NO_SI2196_ERROR) {return ERROR_SI2196_SENDING_COMMAND;}
#endif /* SI2196_DTV_AGC_FREEZE_INPUT_PROP */
#ifdef    SI2196_DTV_AGC_SPEED_PROP
  if (SI2196_L1_SetProperty2(api, SI2196_DTV_AGC_SPEED_PROP_CODE               ) != NO_SI2196_ERROR) {return ERROR_SI2196_SENDING_COMMAND;}
#endif /* SI2196_DTV_AGC_SPEED_PROP */
#ifdef    SI2196_DTV_CONFIG_IF_PORT_PROP
  if (SI2196_L1_SetProperty2(api, SI2196_DTV_CONFIG_IF_PORT_PROP_CODE          ) != NO_SI2196_ERROR) {return ERROR_SI2196_SENDING_COMMAND;}
#endif /* SI2196_DTV_CONFIG_IF_PORT_PROP */
#ifdef    SI2196_DTV_EXT_AGC_PROP
  if (SI2196_L1_SetProperty2(api, SI2196_DTV_EXT_AGC_PROP_CODE                 ) != NO_SI2196_ERROR) {return ERROR_SI2196_SENDING_COMMAND;}
#endif /* SI2196_DTV_EXT_AGC_PROP */
#ifdef    SI2196_DTV_FILTER_SELECT_PROP
  if (SI2196_L1_SetProperty2(api, SI2196_DTV_FILTER_SELECT_PROP_CODE           ) != NO_SI2196_ERROR) {return ERROR_SI2196_SENDING_COMMAND;}
#endif /* SI2196_DTV_FILTER_SELECT_PROP */
#ifdef    SI2196_DTV_IEN_PROP
  if (SI2196_L1_SetProperty2(api, SI2196_DTV_IEN_PROP_CODE                     ) != NO_SI2196_ERROR) {return ERROR_SI2196_SENDING_COMMAND;}
#endif /* SI2196_DTV_IEN_PROP */
#ifdef    SI2196_DTV_INITIAL_AGC_SPEED_PROP
  if (SI2196_L1_SetProperty2(api, SI2196_DTV_INITIAL_AGC_SPEED_PROP_CODE       ) != NO_SI2196_ERROR) {return ERROR_SI2196_SENDING_COMMAND;}
#endif /* SI2196_DTV_INITIAL_AGC_SPEED_PROP */
#ifdef    SI2196_DTV_INITIAL_AGC_SPEED_PERIOD_PROP
  if (SI2196_L1_SetProperty2(api, SI2196_DTV_INITIAL_AGC_SPEED_PERIOD_PROP_CODE) != NO_SI2196_ERROR) {return ERROR_SI2196_SENDING_COMMAND;}
#endif /* SI2196_DTV_INITIAL_AGC_SPEED_PERIOD_PROP */
#ifdef    SI2196_DTV_INTERNAL_ZIF_PROP
  if (SI2196_L1_SetProperty2(api, SI2196_DTV_INTERNAL_ZIF_PROP_CODE            ) != NO_SI2196_ERROR) {return ERROR_SI2196_SENDING_COMMAND;}
#endif /* SI2196_DTV_INTERNAL_ZIF_PROP */
#ifdef    SI2196_DTV_INT_SENSE_PROP
  if (SI2196_L1_SetProperty2(api, SI2196_DTV_INT_SENSE_PROP_CODE               ) != NO_SI2196_ERROR) {return ERROR_SI2196_SENDING_COMMAND;}
#endif /* SI2196_DTV_INT_SENSE_PROP */
#ifdef    SI2196_DTV_LIF_FREQ_PROP
  if (SI2196_L1_SetProperty2(api, SI2196_DTV_LIF_FREQ_PROP_CODE                ) != NO_SI2196_ERROR) {return ERROR_SI2196_SENDING_COMMAND;}
#endif /* SI2196_DTV_LIF_FREQ_PROP */
#ifdef    SI2196_DTV_LIF_OUT_PROP
  if (SI2196_L1_SetProperty2(api, SI2196_DTV_LIF_OUT_PROP_CODE                 ) != NO_SI2196_ERROR) {return ERROR_SI2196_SENDING_COMMAND;}
#endif /* SI2196_DTV_LIF_OUT_PROP */
#ifdef    SI2196_DTV_MODE_PROP
  if (SI2196_L1_SetProperty2(api, SI2196_DTV_MODE_PROP_CODE                    ) != NO_SI2196_ERROR) {return ERROR_SI2196_SENDING_COMMAND;}
#endif /* SI2196_DTV_MODE_PROP */
#ifdef    SI2196_DTV_RF_TOP_PROP
  if (SI2196_L1_SetProperty2(api, SI2196_DTV_RF_TOP_PROP_CODE                  ) != NO_SI2196_ERROR) {return ERROR_SI2196_SENDING_COMMAND;}
#endif /* SI2196_DTV_RF_TOP_PROP */
#ifdef    SI2196_DTV_RSQ_RSSI_THRESHOLD_PROP
  if (SI2196_L1_SetProperty2(api, SI2196_DTV_RSQ_RSSI_THRESHOLD_PROP_CODE      ) != NO_SI2196_ERROR) {return ERROR_SI2196_SENDING_COMMAND;}
#endif /* SI2196_DTV_RSQ_RSSI_THRESHOLD_PROP */
return NO_SI2196_ERROR;
};
#endif


/************************************************************************************************************************
  NAME: si2196_ATVConfig
  DESCRIPTION: Setup si2196 ATV properties configuration
  This function will download all the ATV configuration properties stored in the global structure 'prop.
  Depending on your application, only a subset may be required to be modified.
  The function si2196_SetupATVDefaults() should be called before the first call to this function. Afterwards
  to change a property change the appropriate element in the property structure 'prop' and call this routine.
  Use the comments above the SetProperty2 calls as a guide to the parameters which are changed.
  Parameter:  Pointer to si2196 Context (I2C address)
  Returns:    I2C transaction error code, 0 if successful
  Programming Guide Reference:    Flowchart A.5 (ATV setup flowchart)
************************************************************************************************************************/
int si2196_atvconfig(struct i2c_client *si2196, si2196_propobj_t *prop, si2196_cmdreplyobj_t *rsp)
{

   /* Set ATV_CVBS_OUT property */
   //prop.atv_cvbs_out.amp,
   //prop.atv_cvbs_out.offset
   if (si2196_sendproperty(si2196, SI2196_ATV_CVBS_OUT_PROP, prop, rsp) != 0)
   {
        return ERROR_SI2196_SENDING_COMMAND;
   }

   /* Set the ATV_SIF_OUT property */
   //prop.atv_sif_out.amp,
   //prop.atv_sif_out.offset
   /*if (si2196_sendproperty(si2196, SI2196_ATV_SIF_OUT_PROP, prop, rsp) != 0)
   {
        return ERROR_SI2196_SENDING_COMMAND;
   }*/

   /* Set the si2196_ATV_SOUND_AGC_LIMIT property */
   //prop.atv_sif_out.min_gain,
   //prop.atv_sif_out.max_gain
   if (si2196_sendproperty(si2196, SI2196_ATV_SOUND_AGC_LIMIT_PROP, prop, rsp) != 0)
   {
        return ERROR_SI2196_SENDING_COMMAND;
   }
   /* Set the ATV_AFC_RANGE property */
   //prop.atv_afc_range.range_khz
   if (si2196_sendproperty(si2196, SI2196_ATV_AFC_RANGE_PROP, prop, rsp) != 0)
   {
        return ERROR_SI2196_SENDING_COMMAND;
   }

   /* Set the ATV_RF_TOP property */
   //prop.atv_rf_top.atv_rf_top
   if (si2196_sendproperty(si2196, SI2196_ATV_RF_TOP_PROP, prop, rsp) != 0)
   {
        return ERROR_SI2196_SENDING_COMMAND;
   }

   /* Set the ATV_VSYNC_TRACKING property */
   //prop.atv_vsync_tracking.min_pulses_to_lock,
   //prop.atv_vsync_tracking.min_fields_to_unlock
   if (si2196_sendproperty(si2196, SI2196_ATV_VSYNC_TRACKING_PROP, prop, rsp) != 0)
   {
        return ERROR_SI2196_SENDING_COMMAND;
   }

   /* Set the ATV_VSNR_CAP property */
   //prop.atv_vsnr_cap.atv_vsnr_cap
   if (si2196_sendproperty(si2196, SI2196_ATV_VSNR_CAP_PROP, prop, rsp) != 0)
   {
        return ERROR_SI2196_SENDING_COMMAND;
   }

   /* Set the ATV_CVBS_OUT_FINE property */
   //prop.atv_cvbs_out_fine.amp,
   //prop.atv_cvbs_out_fine.offset
   if (si2196_sendproperty(si2196, SI2196_ATV_CVBS_OUT_FINE_PROP, prop, rsp) != 0)
   {
        return ERROR_SI2196_SENDING_COMMAND;
   }

   /* Set the ATV_AGC_SPEED property */
   //prop.atv_agc_speed.if_agc_speed
   if (si2196_sendproperty(si2196, SI2196_ATV_AGC_SPEED_PROP, prop, rsp) != 0)
   {
        return ERROR_SI2196_SENDING_COMMAND;
   }

   /*Set the ATV_RSSI_RSQ_THRESHOLD property */
   //prop.atv_rsq_rssi_threshold.hi,
   //prop.atv_rsq_rssi_threshold.lo
   if (si2196_sendproperty(si2196, SI2196_ATV_RSQ_RSSI_THRESHOLD_PROP, prop, rsp) != 0)
   {
        return ERROR_SI2196_SENDING_COMMAND;
   }

   /*Set the ATV_SNR_RSQ_THRESHOLD property */
   //prop.atv_rsq_snr_threshold.hi,
   //prop.atv_rsq_snr_threshold.lo
   if (si2196_sendproperty(si2196, SI2196_ATV_RSQ_SNR_THRESHOLD_PROP, prop, rsp) != 0)
   {
        return ERROR_SI2196_SENDING_COMMAND;
   }

   /*Set the ATV_VIDEO_EQUALIZER property */
    //prop.atv_video_equalizer.slope
   if (si2196_sendproperty(si2196, SI2196_ATV_VIDEO_EQUALIZER_PROP, prop, rsp) != 0)
   {
        return ERROR_SI2196_SENDING_COMMAND;
   }

   /*Set the ATV_INT_SENSE property */
   //prop.atv_int_sense.chlnegen,
   //prop.atv_int_sense.chlposen,
   //prop.atv_int_sense.dlnegen,
   //prop.atv_int_sense.dlposen,
   //prop.atv_int_sense.pclnegen,
   //prop.atv_int_sense.pclposen,
   //prop.atv_int_sense.snrhnegen,
   //prop.atv_int_sense.snrhposen,
   //prop.atv_int_sense.snrlnegen,
   //prop.atv_int_sense.snrlposen
    if (si2196_sendproperty(si2196, SI2196_ATV_INT_SENSE_PROP, prop, rsp) != 0)
    {
            return ERROR_SI2196_SENDING_COMMAND;
    }
    /* setup ATV_IEN_PROP  IEN properties to enable ATVINT on CHL  */

    /* prop.atv_ien.chlien,
    prop.atv_ien.pclien,
    prop.atv_ien.dlien ,
    prop.atv_ien.snrlien,
    prop.atv_ien.snrhien */
    if (si2196_sendproperty(si2196, SI2196_ATV_IEN_PROP, prop, rsp) != 0)
    {
        return ERROR_SI2196_SENDING_COMMAND;
    }

    /* Set the ATV_AUDIO_MODE property */
    /*rop.atv_audio_mode.audio_sys,
        prop.atv_audio_mode.chan_bw,
        prop.atv_audio_mode.demod_mode  */
    if (si2196_sendproperty(si2196, SI2196_ATV_AUDIO_MODE_PROP, prop, rsp) != 0)
    {
            return ERROR_SI2196_SENDING_COMMAND;
    }

   /* Set the ATV_VIDEO_MODE property */
    /*prop.atv_video_mode.video_sys,
        prop.atv_video_mode.trans,
        prop.atv_video_mode.color  */
    if (si2196_sendproperty(si2196, SI2196_ATV_VIDEO_MODE_PROP, prop, rsp) != 0)
    {
        return ERROR_SI2196_SENDING_COMMAND;
    }
#ifdef SI2196_ATV_SOUND_AGC_SPEED_PROP
  if (si2196_sendproperty(si2196, SI2196_ATV_SOUND_AGC_SPEED_PROP_CODE, prop, rsp) != 0) 
    {
        return ERROR_SI2196_SENDING_COMMAND;
    }
#endif /* SI2196_ATV_SOUND_AGC_SPEED_PROP */
    return 0;
}

/************************************************************************************************************************
  NAME: si2196_CommonConfig
  DESCRIPTION: Setup si2196 Common properties configuration
  This function will download all the DTV configuration properties stored in the global structure 'prop.
  Depending on your application, only a subset may be required to be modified.
  The function si2196_SetupCommonDefaults() should be called before the first call to this function. Afterwards
  to change a property change the appropriate element in the property structure 'prop' and call this routine.
  Use the comments above the si2196_sendproperty calls as a guide to the parameters which are changed.
  Parameter:  Pointer to si2196 Context (I2C address)
  Returns:    I2C transaction error code, 0 if successful
  Programming Guide Reference:    Flowchart A.6a (Common setup flowchart)
************************************************************************************************************************/
int si2196_commonconfig(struct i2c_client *si2196, si2196_propobj_t *prop, si2196_cmdreplyobj_t *rsp)
{

    /* Setting si2196_MASTER_IEN_PROP property */
    /***
        prop.master_ien.atvien,
        prop.master_ien.ctsien,
        prop.master_ien.dtvien,
        prop.master_ien.errien,
        prop.master_ien.tunien  */

    if (si2196_sendproperty(si2196, SI2196_MASTER_IEN_PROP, prop, rsp) != 0)
    {
        return ERROR_SI2196_SENDING_COMMAND;
    }

    /* Setting si2196_CRYSTAL_TRIM_PROP */
    //rop.crystal_trim.xo_cap
    if (si2196_sendproperty(si2196, SI2196_CRYSTAL_TRIM_PROP, prop, rsp) != 0)
    {
        return ERROR_SI2196_SENDING_COMMAND;
    }

    return 0;
}
/*****************************************************************************************
 NAME: SI2196_downloadSDProperties
  DESCRIPTION: Setup SI2196 SD properties configuration
  This function will download all the SD configuration properties.
  The function SetupSDDefaults() should be called before the first call to this function.
  Parameter:  Pointer to SI2196 Context
  Returns:    I2C transaction error code, NO_SI2196_ERROR if successful
  Programming Guide Reference:    SD setup flowchart
******************************************************************************************/
int  si2196_stereoconfig(struct i2c_client *si2196, si2196_propobj_t *prop, si2196_cmdreplyobj_t *rsp)
{
#ifdef    SI2196_SD_AFC_MAX_PROP
  if (si2196_sendproperty(si2196, SI2196_SD_AFC_MAX_PROP_CODE,prop,rsp) != NO_SI2196_ERROR)
    return ERROR_SI2196_SENDING_COMMAND;
#endif /* SI2196_SD_AFC_MAX_PROP */
#ifdef    SI2196_SD_AFC_MUTE_PROP
  if (si2196_sendproperty(si2196, SI2196_SD_AFC_MUTE_PROP_CODE,prop,rsp) != NO_SI2196_ERROR)
    return ERROR_SI2196_SENDING_COMMAND;
#endif /* SI2196_SD_AFC_MUTE_PROP */
#ifdef    SI2196_SD_AGC_PROP
  if (si2196_sendproperty(si2196, SI2196_SD_AGC_PROP_CODE,prop,rsp) != NO_SI2196_ERROR)
      return ERROR_SI2196_SENDING_COMMAND;
#endif /* SI2196_SD_AGC_PROP */
#ifdef    SI2196_SD_ASD_PROP
  if (si2196_sendproperty(si2196, SI2196_SD_ASD_PROP_CODE,prop,rsp) != NO_SI2196_ERROR)
       return ERROR_SI2196_SENDING_COMMAND;
#endif /* SI2196_SD_ASD_PROP */
#ifdef    SI2196_SD_CARRIER_MUTE_PROP
  if (si2196_sendproperty(si2196, SI2196_SD_CARRIER_MUTE_PROP,prop,rsp) != NO_SI2196_ERROR)
       return ERROR_SI2196_SENDING_COMMAND;
#endif /* SI2196_SD_CARRIER_MUTE_PROP */
#ifdef    SI2196_SD_I2S_PROP
  if (si2196_sendproperty(si2196, SI2196_SD_I2S_PROP,prop,rsp) != NO_SI2196_ERROR)
       return ERROR_SI2196_SENDING_COMMAND;
#endif /* SI2196_SD_I2S_PROP */
#ifdef    SI2196_SD_IEN_PROP
  if (si2196_sendproperty(si2196, SI2196_SD_IEN_PROP,prop,rsp) != NO_SI2196_ERROR)
       return ERROR_SI2196_SENDING_COMMAND;
#endif /* SI2196_SD_IEN_PROP */
#ifdef    SI2196_SD_INT_SENSE_PROP
  if (si2196_sendproperty(si2196, SI2196_SD_INT_SENSE_PROP,prop,rsp) != NO_SI2196_ERROR)
       return ERROR_SI2196_SENDING_COMMAND;
#endif /* SI2196_SD_INT_SENSE_PROP */
#ifdef    SI2196_SD_LANG_SELECT_PROP
  if (si2196_sendproperty(si2196, SI2196_SD_LANG_SELECT_PROP,prop,rsp) != NO_SI2196_ERROR)
       return ERROR_SI2196_SENDING_COMMAND;
#endif /* SI2196_SD_LANG_SELECT_PROP */
#ifdef    SI2196_SD_NICAM_PROP
  if (si2196_sendproperty(si2196, SI2196_SD_NICAM_PROP,prop,rsp) != NO_SI2196_ERROR)
       return ERROR_SI2196_SENDING_COMMAND;
#endif /* SI2196_SD_NICAM_PROP */
#ifdef    SI2196_SD_NICAM_FAILOVER_THRESH_PROP
  if (si2196_sendproperty(si2196, SI2196_SD_NICAM_FAILOVER_THRESH_PROP,prop,rsp) != NO_SI2196_ERROR)
       return ERROR_SI2196_SENDING_COMMAND;
#endif /* SI2196_SD_NICAM_FAILOVER_THRESH_PROP */
#ifdef    SI2196_SD_NICAM_RECOVER_THRESH_PROP
  if (si2196_sendproperty(si2196, SI2196_SD_NICAM_RECOVER_THRESH_PROP,prop,rsp) != NO_SI2196_ERROR)
       return ERROR_SI2196_SENDING_COMMAND;
#endif /* SI2196_SD_NICAM_RECOVER_THRESH_PROP */
#ifdef    SI2196_SD_OVER_DEV_MODE_PROP
  if (si2196_sendproperty(si2196, SI2196_SD_OVER_DEV_MODE_PROP,prop,rsp) != NO_SI2196_ERROR)
       return ERROR_SI2196_SENDING_COMMAND;
#endif /* SI2196_SD_OVER_DEV_MODE_PROP */
#ifdef    SI2196_SD_OVER_DEV_MUTE_PROP
  if (si2196_sendproperty(si2196, SI2196_SD_OVER_DEV_MUTE_PROP,prop,rsp) != NO_SI2196_ERROR)
       return ERROR_SI2196_SENDING_COMMAND;
#endif /* SI2196_SD_OVER_DEV_MUTE_PROP */
#ifdef    SI2196_SD_PILOT_LVL_CTRL_PROP
  if (si2196_sendproperty(si2196, SI2196_SD_PILOT_LVL_CTRL_PROP,prop,rsp) != NO_SI2196_ERROR)
       return ERROR_SI2196_SENDING_COMMAND;
#endif /* SI2196_SD_PILOT_LVL_CTRL_PROP */
#ifdef    SI2196_SD_PORT_CONFIG_PROP
  if (si2196_sendproperty(si2196, SI2196_SD_PORT_CONFIG_PROP,prop,rsp) != NO_SI2196_ERROR)
       return ERROR_SI2196_SENDING_COMMAND;
#endif /* SI2196_SD_PORT_CONFIG_PROP */
#ifdef    SI2196_SD_PORT_MUTE_PROP
  if (si2196_sendproperty(si2196, SI2196_SD_PORT_MUTE_PROP,prop,rsp) != NO_SI2196_ERROR)
       return ERROR_SI2196_SENDING_COMMAND;
#endif /* SI2196_SD_PORT_MUTE_PROP */
#ifdef    SI2196_SD_PORT_VOLUME_BALANCE_PROP
  if (si2196_sendproperty(si2196, SI2196_SD_PORT_VOLUME_BALANCE_PROP,prop,rsp) != NO_SI2196_ERROR)
       return ERROR_SI2196_SENDING_COMMAND;
#endif /* SI2196_SD_PORT_VOLUME_BALANCE_PROP */
#ifdef    SI2196_SD_PORT_VOLUME_LEFT_PROP
  if (si2196_sendproperty(si2196, SI2196_SD_PORT_VOLUME_LEFT_PROP,prop,rsp) != NO_SI2196_ERROR)
       return ERROR_SI2196_SENDING_COMMAND;
#endif /* SI2196_SD_PORT_VOLUME_LEFT_PROP */
#ifdef    SI2196_SD_PORT_VOLUME_MASTER_PROP
  if (si2196_sendproperty(si2196, SI2196_SD_PORT_VOLUME_MASTER_PROP,prop,rsp) != NO_SI2196_ERROR)
       return ERROR_SI2196_SENDING_COMMAND;
#endif /* SI2196_SD_PORT_VOLUME_MASTER_PROP */
#ifdef    SI2196_SD_PORT_VOLUME_RIGHT_PROP
  if (si2196_sendproperty(si2196, SI2196_SD_PORT_VOLUME_RIGHT_PROP,prop,rsp) != NO_SI2196_ERROR)
       return ERROR_SI2196_SENDING_COMMAND;
#endif /* SI2196_SD_PORT_VOLUME_RIGHT_PROP */
#ifdef    SI2196_SD_PRESCALER_AM_PROP
  if (si2196_sendproperty(si2196, SI2196_SD_PRESCALER_AM_PROP,prop,rsp) != NO_SI2196_ERROR)
       return ERROR_SI2196_SENDING_COMMAND;
#endif /* SI2196_SD_PRESCALER_AM_PROP */
#ifdef    SI2196_SD_PRESCALER_EIAJ_PROP
  if (si2196_sendproperty(si2196, SI2196_SD_PRESCALER_EIAJ_PROP,prop,rsp) != NO_SI2196_ERROR)
       return ERROR_SI2196_SENDING_COMMAND;
#endif /* SI2196_SD_PRESCALER_EIAJ_PROP */
#ifdef    SI2196_SD_PRESCALER_FM_PROP
  if (si2196_sendproperty(si2196, SI2196_SD_PRESCALER_FM_PROP,prop,rsp) != NO_SI2196_ERROR)
       return ERROR_SI2196_SENDING_COMMAND;
#endif /* SI2196_SD_PRESCALER_FM_PROP */
#ifdef    SI2196_SD_PRESCALER_NICAM_PROP
  if (si2196_sendproperty(si2196, SI2196_SD_PRESCALER_NICAM_PROP,prop,rsp) != NO_SI2196_ERROR)
        return ERROR_SI2196_SENDING_COMMAND;
#endif /* SI2196_SD_PRESCALER_NICAM_PROP */
#ifdef    SI2196_SD_PRESCALER_SAP_PROP
  if (si2196_sendproperty(si2196, SI2196_SD_PRESCALER_SAP_PROP,prop,rsp) != NO_SI2196_ERROR)
         return ERROR_SI2196_SENDING_COMMAND;
#endif /* SI2196_SD_PRESCALER_SAP_PROP */
#ifdef    SI2196_SD_SOUND_MODE_PROP
  if (si2196_sendproperty(si2196, SI2196_SD_SOUND_MODE_PROP,prop,rsp) != NO_SI2196_ERROR)
         return ERROR_SI2196_SENDING_COMMAND;
#endif /* SI2196_SD_SOUND_MODE_PROP */
#ifdef    SI2196_SD_SOUND_SYSTEM_PROP
  if (si2196_sendproperty(si2196, SI2196_SD_SOUND_SYSTEM_PROP,prop,rsp) != NO_SI2196_ERROR)
         return ERROR_SI2196_SENDING_COMMAND;
#endif /* SI2196_SD_SOUND_SYSTEM_PROP */
#ifdef    SI2196_SD_STEREO_DM_ID_LVL_ACQ_PROP
  if (si2196_sendproperty(si2196, SI2196_SD_STEREO_DM_ID_LVL_ACQ_PROP,prop,rsp) != NO_SI2196_ERROR)
         return ERROR_SI2196_SENDING_COMMAND;
#endif /* SI2196_SD_STEREO_DM_ID_LVL_ACQ_PROP */
#ifdef    SI2196_SD_STEREO_DM_ID_LVL_SHIFT_PROP
  if (si2196_sendproperty(si2196, SI2196_SD_STEREO_DM_ID_LVL_SHIFT_PROP,prop,rsp) != NO_SI2196_ERROR)
         return ERROR_SI2196_SENDING_COMMAND;
#endif /* SI2196_SD_STEREO_DM_ID_LVL_SHIFT_PROP */
#ifdef    SI2196_SD_STEREO_DM_ID_LVL_TRACK_PROP
  if (si2196_sendproperty(si2196, SI2196_SD_STEREO_DM_ID_LVL_TRACK_PROP,prop,rsp) != NO_SI2196_ERROR)
         return ERROR_SI2196_SENDING_COMMAND;
#endif /* SI2196_SD_STEREO_DM_ID_LVL_TRACK_PROP */
    return NO_SI2196_ERROR;
};

/************************************************************************************************************************
  NAME: si2196_TunerConfig
  DESCRIPTION: Setup si2196 Tuner (RF to IF analog path) properties configuration
  This function will download all the DTV configuration properties stored in the global structure 'prop.
  Depending on your application, only a subset may be required to be modified.
  The function si2196_SetupTunerDefaults() should be called before the first call to this function. Afterwards
  to change a property change the appropriate element in the property structure 'prop' and call this routine.
  Use the comments above the si2196_sendproperty calls as a guide to the parameters which are changed.
  Parameter:  Pointer to si2196 Context (I2C address)
  Returns:    I2C transaction error code, 0 if successful
  Programming Guide Reference:    Flowchart A.6a (Tuner setup flowchart)
************************************************************************************************************************/
int si2196_tunerconfig(struct i2c_client *si2196, si2196_propobj_t *prop, si2196_cmdreplyobj_t *rsp)
{
        /* Set TUNER_IEN property */
        /*rop.tuner_ien.tcien,
        prop.tuner_ien.rssilien,
        prop.tuner_ien.rssihien */
        if (si2196_sendproperty(si2196, SI2196_TUNER_IEN_PROP, prop, rsp) != 0)
        {
            return ERROR_SI2196_SENDING_COMMAND;
        }

        /* Setting TUNER_BLOCKED_VCO property */
        /*rop.tuner_blocked_vco.vco_code, */
        if (si2196_sendproperty(si2196, SI2196_TUNER_BLOCKED_VCO_PROP, prop, rsp) != 0)
        {
            return ERROR_SI2196_SENDING_COMMAND;
        }
        /* Setting si2196_TUNER_INT_SENSE_PROP property */
        /*    rop.tuner_int_sense.rssihnegen,
            prop.tuner_int_sense.rssihposen,
            prop.tuner_int_sense.rssilnegen,
            prop.tuner_int_sense.rssilposen,
            prop.tuner_int_sense.tcnegen,
            prop.tuner_int_sense.tcposen */

        if (si2196_sendproperty(si2196, SI2196_TUNER_INT_SENSE_PROP, prop, rsp) != 0)
        {
            return ERROR_SI2196_SENDING_COMMAND;
        }

        /* Setting si2196_TUNER_LO_INJECTION_PROP property */
        /*rop.tuner_lo_injection.band_1,
        prop.tuner_lo_injection.band_2,
        prop.tuner_lo_injection.band_3,
        prop.tuner_lo_injection.band_4,
        prop.tuner_lo_injection.band_5 */
        if (si2196_sendproperty(si2196, SI2196_TUNER_LO_INJECTION_PROP, prop, rsp) != 0)
        {
            return ERROR_SI2196_SENDING_COMMAND;
        }


    return 0;
}
/************************************************************************************************************************
NAME:Si2196_SetupDTVDefaults
DESCRIPTION:Setup si2196 DTV startup configuration
This is a list of all the DTV configuration properties. Depending on your application,only a subset maybe required.
The properties are stored in the global property structure 'prop'. The function DTVConfig(....) must be called
after any properties are modified.
parameter:none
returns:0 if successful
programming Guide reference: Flowchart A.6(DTV setup flowchart)
************************************************************************************************************************/
static int si2196_setupdtvdefaults(si2196_propobj_t* prop,int ch_mode)
{
	prop->dtv_config_if_port.dtv_out_type  = SI2196_DTV_CONFIG_IF_PORT_PROP_DTV_OUT_TYPE_LIF_IF1;
	prop->dtv_config_if_port.dtv_agc_source = SI2196_DTV_CONFIG_IF_PORT_PROP_DTV_AGC_SOURCE_INTERNAL;
	prop->dtv_lif_freq.offset = 5000;
	prop->dtv_mode.bw= SI2196_DTV_MODE_PROP_BW_BW_8MHZ;
	prop->dtv_mode.invert_spectrum = SI2196_DTV_MODE_PROP_INVERT_SPECTRUM_NORMAL;
	//set in the get tuner ops
	//prop->dtv_mode.modulation = SI2196_DTV_MODE_PROP_MODULATION_DVBC;
	prop->dtv_rsq_rssi_threshold.hi=0;
    	prop->dtv_rsq_rssi_threshold.lo=-80;
    	prop->dtv_ext_agc.max_10mv=200;
    	prop->dtv_ext_agc.min_10mv=50;
    	if(ch_mode == 4)
        prop->dtv_lif_out.amp=22;//27;//23;
    	else
        prop->dtv_lif_out.amp=25;//27;
        
   	 prop->dtv_lif_out.offset=148;
    	prop->dtv_agc_speed.agc_decim=SI2196_DTV_AGC_SPEED_PROP_AGC_DECIM_OFF;
    	prop->dtv_agc_speed.if_agc_speed=SI2196_DTV_AGC_SPEED_PROP_IF_AGC_SPEED_AUTO;
    	prop->dtv_rf_top.dtv_rf_top=SI2196_DTV_RF_TOP_PROP_DTV_RF_TOP_AUTO;
    	prop->dtv_int_sense.chlnegen=SI2196_DTV_INT_SENSE_PROP_CHLNEGEN_DISABLE;
    	prop->dtv_int_sense.chlposen=SI2196_DTV_INT_SENSE_PROP_CHLPOSEN_ENABLE;
    	prop->dtv_ien.chlien  = SI2196_DTV_IEN_PROP_CHLIEN_ENABLE;      /* enable only CHL to drive DTVINT */
    	return 0;
}
/************************************************************************************************************************
NAME: SI2196_DTV_Config
DESCRIPTION:Setup Si2196 DTV properties configuration
This function will download all the DTV configuration properties stored in the global structure 'prop'
Depending on your application, only a subset may be required to be modified.
  The function Si2196_SetupDTVDefaults() should be called before the first call to this function. Afterwards
  to change a property change the appropriate element in the property structure 'prop' and call this routine.
  Use the comments above the Si2196_L1_SetProperty2 calls as a guide to the parameters which are changed.
  Parameter:  Pointer to Si2196 Context (I2C address)
  Returns:    I2C transaction error code, 0 if successful
  Programming Guide Reference:    Flowchart A.6 (DTV setup flowchart)
  ************************************************************************************************************************/
  int si2196_dtvconfig(struct i2c_client *si2196,si2196_propobj_t *prop,si2196_cmdreplyobj_t *rsp)
{
	/*DTV_CONFIG_IF_PORT_PROP property */
	/*prop.dtv_config_if_port.dtv_out_type,
	prop.dtv_config_if_port.dtv_agc_source */
    if (si2196_sendproperty(si2196, SI2196_DTV_CONFIG_IF_PORT_PROP, prop, rsp) != 0){
        return ERROR_SI2196_SENDING_COMMAND;
    }
	/* Setting DTV_LIF_FREQ_PROP */
     //prop.dtv_lif_freq.offset
    if (si2196_sendproperty(si2196, SI2196_DTV_LIF_FREQ_PROP, prop, rsp) != 0){
        return ERROR_SI2196_SENDING_COMMAND;
    }
	/* Setting DTV_MODE_PROP property */
 	 /*	prop.dtv_mode.bw,
	prop.dtv_mode.invert_spectrum,
	prop.dtv_mode.modulation*/
    if (si2196_sendproperty(si2196, SI2196_DTV_MODE_PROP, prop, rsp) != 0){
        return ERROR_SI2196_SENDING_COMMAND;
    }
	/* Setting DTV_RSQ_RSSI_THRESHOLD property */
	//	prop.dtv_rsq_rssi_threshold.hi,
	//	prop.dtv_rsq_rssi_threshold.lo
    if (si2196_sendproperty(si2196, SI2196_DTV_RSQ_RSSI_THRESHOLD_PROP, prop, rsp) != 0){
        return ERROR_SI2196_SENDING_COMMAND;
    }
	/* Setting DTV_EXT_AGC property */
	//	prop.dtv_ext_agc.max_10mv,
	//	prop.dtv_ext_agc.min_10mv
    if (si2196_sendproperty(si2196, SI2196_DTV_EXT_AGC_PROP, prop, rsp) != 0){
        return ERROR_SI2196_SENDING_COMMAND;
    }
	/* Setting DTV_LIF_OUT property */
	//	prop.dtv_lif_out.amp,
	//	prop.dtv_lif_out.offset
    if (si2196_sendproperty(si2196, SI2196_DTV_LIF_OUT_PROP, prop, rsp) != 0){
        return ERROR_SI2196_SENDING_COMMAND;
    }
	/* Setting DTV_AGC_SPEED property */
	//	prop.dtv_agc_speed.agc_decim,
	//	prop.dtv_agc_speed.if_agc_speed
    if (si2196_sendproperty(si2196, SI2196_DTV_AGC_SPEED_PROP, prop, rsp) != 0){
        return ERROR_SI2196_SENDING_COMMAND;
    }
	/* Setting DTV_RF_TOP property */
	//	prop.dtv_rf_top.dtv_rf_top
    if (si2196_sendproperty(si2196, SI2196_DTV_RF_TOP_PROP, prop, rsp) != 0){
        return ERROR_SI2196_SENDING_COMMAND;
    }
	 /* Setting DTV_INT_SENSE property */
	//	prop.dtv_int_sense.chlnegen,
	//	prop.dtv_int_sense.chlposen
    if (si2196_sendproperty(si2196, SI2196_DTV_INT_SENSE_PROP, prop, rsp) != 0){
        return ERROR_SI2196_SENDING_COMMAND;
    }
	  /* Set DTV_IEN property */
	/*	prop.dtv_ien.chlien */
    if (si2196_sendproperty(si2196, SI2196_DTV_IEN_PROP, prop, rsp) != 0){
        return ERROR_SI2196_SENDING_COMMAND;
    }
   return 0;
}


/************************************************************************************************************************
  NAME: si2196_Tune
  DESCRIPTIION: Tune si2196 in specified mode (ATV/DTV) at center frequency, wait for TUNINT and xTVINT with timeout

  Parameter:  Pointer to si2196 Context (I2C address)
  Parameter:  Mode (ATV or DTV) use si2196_TUNER_TUNE_FREQ_CMD_MODE_ATV or si2196_TUNER_TUNE_FREQ_CMD_MODE_DTV constants
  Parameter:  frequency (Hz) as a unsigned long integer
  Parameter:  rsp - commandResp structure to returns tune status info.
  Returns:    0 if channel found.  A nonzero value means either an error occurred or channel not locked.
  Programming Guide Reference:    Flowchart A.7 (Tune flowchart)
************************************************************************************************************************/
int si2196_tune(struct i2c_client *si2196, unsigned char mode, unsigned long freq, si2196_cmdreplyobj_t *rsp)
{
    int return_code = 0,count = 0;

    if (si2196_tuner_tune_freq(si2196, mode, freq, rsp) != 0)
    {
        pr_info("%s: tuner tune freq error:%d!!!!\n", __func__, ERROR_SI2196_SENDING_COMMAND);
        return ERROR_SI2196_SENDING_COMMAND;
    }
    for (count=50; count ;count--)
    {
        return_code = si2196_pollforcts(si2196);
        if (!return_code)
            break;
        mdelay(2);
    }
    if (!count)
    {
        pr_info("%s: poll cts error:%d!!!!\n", __func__, return_code);
        return return_code;
    }
	mdelay(delay_det);
#if 0
    /* wait for TUNINT, timeout is 100ms */
    for (count=50; count ;count--)
    {
        if ((return_code = si2196_check_status(si2196, common_reply)) != 0)
            return return_code;
        if (common_reply->tunint)
            break;
        mdelay(2);
    }
    if (!count)
    {
        pr_info("%s: ERROR_SI2196_TUNINT_TIMEOUT error:%d!!!!\n", __func__, ERROR_SI2196_TUNINT_TIMEOUT);
        return ERROR_SI2196_TUNINT_TIMEOUT;
    }
    /* wait for xTVINT, timeout is 150ms for ATVINT and 6 ms for DTVINT */
    count = ((mode==SI2196_TUNER_TUNE_FREQ_CMD_MODE_ATV) ? 75 : 3);
    for (;count ;count--)
    {
        if ((return_code = si2196_check_status(si2196, common_reply)) != 0)
            return return_code;
        if ((mode==SI2196_TUNER_TUNE_FREQ_CMD_MODE_ATV) ? common_reply->atvint : common_reply->dtvint)
            break;
        mdelay(2);
    }
    if (!count)
    {
        pr_info("%s: ERROR_SI2196_XTVINT_TIMEOUT error:%d!!!!\n", __func__, ERROR_SI2196_XTVINT_TIMEOUT);
        return ERROR_SI2196_XTVINT_TIMEOUT;
    }
    /*wait for sdint ,timeout is 200ms for sdint */
    count = ((mode==SI2196_TUNER_TUNE_FREQ_CMD_MODE_ATV) ? 100 : 0);
    for (;count ;count--)
    {
        if ((return_code = si2196_check_status(si2196, common_reply)) != 0)
            return return_code;
        if (common_reply->sdint)
            break;
        mdelay(2);
    }
    if (!count)
    {
        pr_info("%s: ERROR_SI2196_XSDINT_TIMEOUT error:%d!!!!\n", __func__, ERROR_SI2196_XTVINT_TIMEOUT);
        return ERROR_SI2196_XTVINT_TIMEOUT;
    }
#endif
    return return_code;
}

/************************************************************************************************************************
  NAME: si2196_LoadVideofilter
  DESCRIPTION:        Load video filters from vidfiltTable in si2196_write_xTV_video_coeffs.h file into si2196
  Programming Guide Reference:    Flowchart A.4 (Download Video Filters flowchart)

  Parameter:  si2196 Context (I2C address)
  Parameter:  pointer to video filter table array
  Parameter:  number of lines in video filter table array (size in bytes / BYTES_PER_LINE)
  Returns:    si2196/I2C transaction error code, 0 if successful
************************************************************************************************************************/
int si2196_loadvideofilter(struct i2c_client *si2196, unsigned char* vidfilttable, int lines, si2196_common_reply_struct *common_reply)
{
#define BYTES_PER_LINE 8
    int line;

    /* for each 8 bytes in VIDFILT_TABLE */
    for (line = 0; line < lines; line++)
    {
        /* send that 8 byte I2C command to si2196 */
        if (si2196_api_patch(si2196, BYTES_PER_LINE, vidfilttable+BYTES_PER_LINE*line, &reply) != 0)
        {
            return ERROR_SI2196_SENDING_COMMAND;
        }
    }
    common_reply = &reply;
    return 0;
}
/************************************************************************************************************************
  NAME: si2196_Configure
  DESCRIPTION: Setup si2196 video filters, GPIOs/clocks, Common Properties startup, Tuner startup, ATV startup, and DTV startup.
  Parameter:  Pointer to si2196 Context (I2C address)
  Parameter:  rsp - commandResp structure buffer.
  Returns:    I2C transaction error code, 0 if successful
************************************************************************************************************************/
int si2196_configure(struct i2c_client *si2196, si2196_cmdreplyobj_t *rsp)
{
    int return_code = 0;
    si2196_propobj_t prop;
    if(SI2196_DEBUG)
        pr_info("%s: start!!!\n", __func__);

    /* load ATV video filter file */
#ifdef USING_ATV_FILTER
    if ((return_code = si2196_loadvideofilter(si2196, dlif_vidfilt_table, DLIF_VIDFILT_LINES, rsp->reply) != 0)
        return return_code;
#endif

    /* Set the GPIO Pins using the CONFIG_PINS command*/
    if (si2196_config_pins(si2196,      /* turn off BCLK1 and XOUT */
                           SI2196_CONFIG_PINS_CMD_GPIO1_MODE_NO_CHANGE,
                           SI2196_CONFIG_PINS_CMD_GPIO1_READ_DO_NOT_READ,
                           SI2196_CONFIG_PINS_CMD_GPIO2_MODE_NO_CHANGE,
                           SI2196_CONFIG_PINS_CMD_GPIO2_READ_DO_NOT_READ,
                           SI2196_CONFIG_PINS_CMD_GPIO3_MODE_NO_CHANGE,
                           SI2196_CONFIG_PINS_CMD_GPIO3_READ_DO_NOT_READ,
                           SI2196_CONFIG_PINS_CMD_BCLK1_MODE_DISABLE,
                           SI2196_CONFIG_PINS_CMD_BCLK1_READ_DO_NOT_READ,
                           SI2196_CONFIG_PINS_CMD_XOUT_MODE_XOUT,
                           rsp) !=0)
    {
        pr_info("%s: config pins error:%d!!!!\n", __func__, ERROR_SI2196_SENDING_COMMAND);
        return ERROR_SI2196_SENDING_COMMAND;
    }
#if  1
    /* Set Common Properties startup configuration */
    si2196_setupcommondefaults(&prop);
    if ((return_code = si2196_commonconfig(si2196, &prop, rsp)) != 0)
    {
       pr_info("%s: setup command defaults error:%d!!!!\n", __func__, return_code);
       return return_code;
    }
    
    /* Set Tuner Properties startup configuration */
    si2196_setuptunerdefaults(&prop);
    if ((return_code = si2196_tunerconfig(si2196, &prop, rsp)) != 0)
    {
        pr_info("%s: setup tuner defaults error:%d!!!!\n", __func__, return_code);
        return return_code;
    }
    /* Set ATV startup configuration */
    si2196_setupatvdefaults(&prop);
    if ((return_code = si2196_atvconfig(si2196, &prop, rsp)) != 0)
    {
        pr_info("%s: setup atv defaults error:%d!!!!\n", __func__, return_code);
        return return_code;
    }
    /*set dtv startup configuration*/
    //si2196_setupdtvdefaults(prop);
    //si2196_dtvconfig(prop);
    /*set stereo demod default value*/
    si2196_setupsddefaults(&prop);
    if((return_code = si2196_stereoconfig(si2196, &prop, rsp))!=0)
    {
        pr_info("%s: setup stereo defaults error:%d!!!!\n", __func__, return_code);
        return return_code;
    }
#endif

    return return_code;
}
/************************************************************************************************************************
  NAME: si2196_LoadFirmware
  DESCRIPTON: Load firmware from FIRMWARE_TABLE array in si2196_Firmware_x_y_build_z.h file into si2196
              Requires si2196 to be in bootloader mode after PowerUp
  Programming Guide Reference:    Flowchart A.3 (Download FW PATCH flowchart)

  Parameter:  si2196 Context (I2C address)
  Parameter:  pointer to firmware table array
  Parameter:  number of lines in firmware table array (size in bytes / BYTES_PER_LINE)
  Returns:    si2196/I2C transaction error code, 0 if successful
************************************************************************************************************************/
int si2196_loadfirmware(struct i2c_client *si2196, unsigned char* firmwaretable, int lines, si2196_common_reply_struct *common_reply)
{
    int return_code = 0;
    int line;

    /* for each 8 bytes in FIRMWARE_TABLE */
    for (line = 0; line < lines; line++)
    {
        /* send that 8 byte I2C command to si2196 */
        if (si2196_api_patch(si2196, 8, firmwaretable+8*line, common_reply) != 0)
        {
            return ERROR_SI2196_LOADING_FIRMWARE;
        }
    }
    return return_code;
}

/************************************************************************************************************************
  NAME: si2196_StartFirmware
  DESCRIPTION: Start si2196 firmware (put the si2196 into run mode)
  Parameter:   si2196 Context (I2C address)
  Parameter (passed by Reference): 	ExitBootloadeer Response Status byte : tunint, atvint, dtvint, err, cts
  Returns:     I2C transaction error code, 0 if successful
************************************************************************************************************************/
int si2196_startfirmware(struct i2c_client *si2196, si2196_cmdreplyobj_t *rsp)
{

    if (si2196_exit_bootloader(si2196, SI2196_EXIT_BOOTLOADER_CMD_FUNC_TUNER, SI2196_EXIT_BOOTLOADER_CMD_CTSIEN_OFF, rsp) != 0)
    {
        return ERROR_SI2196_STARTING_FIRMWARE;
    }

    return 0;
}
/************************************************************************************************************************
  NAME: si2196_PowerUpWithPatch
  DESCRIPTION: Send si2196 API PowerUp Command with PowerUp to bootloader,
  Check the Chip rev and part, and ROMID are compared to expected values.
  Load the Firmware Patch then Start the Firmware.
  Programming Guide Reference:    Flowchart A.2a (POWER_UP with patch flowchart)

  Parameter:  si2196 Context (I2C address)
  Returns:    si2196/I2C transaction error code, 0 if successful
************************************************************************************************************************/
int si2196_powerupwithpatch(struct i2c_client *si2196, si2196_cmdreplyobj_t *rsp, si2196_common_reply_struct *common_reply)
{
    int return_code = 0, num = 0;

    return_code = si2196_power_up(si2196,          /* always wait for CTS prior to POWER_UP command */
                                  SI2196_POWER_UP_CMD_SUBCODE_CODE,
                                  SI2196_POWER_UP_CMD_RESERVED1_RESERVED,
                                  SI2196_POWER_UP_CMD_RESERVED2_RESERVED,
                                  SI2196_POWER_UP_CMD_RESERVED3_RESERVED,
                                  SI2196_POWER_UP_CMD_CLOCK_MODE_XTAL,
                                  SI2196_POWER_UP_CMD_CLOCK_FREQ_CLK_24MHZ,
                                  SI2196_POWER_UP_CMD_ADDR_MODE_CURRENT,
                                  SI2196_POWER_UP_CMD_FUNC_BOOTLOADER,        /* start in bootloader mode */
                                  SI2196_POWER_UP_CMD_CTSIEN_DISABLE,
                                  SI2196_POWER_UP_CMD_WAKE_UP_WAKE_UP,
                                  rsp);
    if (return_code)
        pr_info("%s: si2196_power_up error:%d!!!\n", __func__, return_code);

#if 0
    /* Get the Part Info from the chip.   This command is only valid in Bootloader mode */
    if (si2196_part_info(si2196, &rsp) != 0)
        return ERROR_SI2196_SENDING_COMMAND;
    /* Check the Chip revision, part and ROMID against expected values and report an error if incompatible */
    if (rsp.part_info.chiprev != chiprev)
        return ERROR_SI2196_INCOMPATIBLE_PART;
    /* Part Number is represented as the last 2 digits */
    if (rsp.part_info.part != part)
        return ERROR_SI2196_INCOMPATIBLE_PART;
    /* Part Major Version in ASCII */
    if (rsp.part_info.pmajor != partmajorversion)
        return ERROR_SI2196_INCOMPATIBLE_PART;

#endif
    mdelay(25);
 /* Load the Firmware */
        num = sizeof(si2196_fw_1_2b1)/(8*sizeof(unsigned char));
        return_code = si2196_loadfirmware(si2196, si2196_fw_1_2b1, num, common_reply);


    if (return_code != 0)
    {
        pr_info("%s: si2196_loadfirmware error:%d!!!\n", __func__, return_code);
        return return_code;
    }

    /*Start the Firmware */
    if ((return_code = si2196_startfirmware(si2196, rsp)) != 0) /* Start firmware */
    {
        pr_info("%s: si2196_startfirmware error:%d!!!\n", __func__, return_code);
        return return_code;
    }
    mdelay(50);
    if (si2196_get_rev(si2196, rsp) != 0)
    {
        pr_info("%s: si2196_get_rev error:%d!!!\n", __func__, ERROR_SI2196_SENDING_COMMAND);
        return ERROR_SI2196_SENDING_COMMAND;
    }
    else
    {
        if(SI2196_DEBUG)
            {
                pr_info("%s: rsp.get_rev.pn :      %d \n", __func__, rsp->get_rev.pn);
                pr_info("%s: rsp.get_rev.fwmajor : %d \n", __func__, rsp->get_rev.fwmajor);
                pr_info("%s: rsp.get_rev.fwminor : %d \n", __func__, rsp->get_rev.fwminor);
                pr_info("%s: rsp.get_rev.patch :   %d \n", __func__, rsp->get_rev.patch);
                pr_info("%s: rsp.get_rev.cmpmajor :%d \n", __func__, rsp->get_rev.cmpmajor);
                pr_info("%s: rsp.get_rev.cmpminor :%d \n", __func__, rsp->get_rev.cmpminor);
                pr_info("%s: rsp.get_rev.cmpbuild :%d \n", __func__, rsp->get_rev.cmpbuild);
                pr_info("%s: rsp.get_rev.chiprev : %d \n", __func__, (u32)rsp->get_rev.chiprev);
            }
    }

    return return_code;
}


/************************************************************************************************************************
  NAME: si2196_Init
  DESCRIPTION:Reset and Initialize si2196
  Parameter:  si2196 Context (I2C address)
  Returns:    I2C transaction error code, 0 if successful
************************************************************************************************************************/
int si2196_init(struct i2c_client *si2196, si2196_cmdreplyobj_t *rsp)
{
    int return_code = 0;

    /* Reset si2196 */
    /* TODO: SendRSTb requires porting to toggle the RSTb line low -> high */
    //sendrstb();

    return_code = si2196_powerupwithpatch(si2196, rsp, rsp->reply);
    if (return_code)		/* PowerUp into bootloader */
    {
        pr_info("%s: init si2196 error!!!\n", __func__);
    }
    /* At this point, FW is loaded and started.  Return 0*/
    return return_code;
}



