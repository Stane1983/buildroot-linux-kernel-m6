/*******************************************************************************
 *
 * FILE NAME          : MxL101SF_OEM_Drv.h
 * 
 * AUTHOR             : Brenndon Lee
 *
 * DATE CREATED       : Jan/22/2008
 *
 * DESCRIPTION        : This file contains I2C-related constants 
 *
 *******************************************************************************
 *                Copyright (c) 2006, MaxLinear, Inc.
 ******************************************************************************/

#ifndef __MXL_CTRL_HWD_H__
#define __MXL_CTRL_HWD_H__

/******************************************************************************
    Include Header Files
    (No absolute paths - paths handled by make file)
******************************************************************************/

#include "MaxLinearDataTypes.h"
#include <linux/i2c.h>
#include "demod_MxL101SF.h"
/******************************************************************************
    Macros
******************************************************************************/

#define MxL_DLL_DEBUG0  printf

/******************************************************************************
    User-Defined Types (Typedefs)
******************************************************************************/

/******************************************************************************
    Global Variable Declarations
******************************************************************************/

/******************************************************************************
    Prototypes
******************************************************************************/
MXL_STATUS Ctrl_WriteRegister(UINT8 regAddr, UINT8 regData);
MXL_STATUS Ctrl_ReadRegister(UINT8 regAddr, UINT8 *dataPtr);
MXL_STATUS Ctrl_Sleep(UINT16 TimeinMilliseconds);
MXL_STATUS Ctrl_GetTime(UINT32 *TimeinMilliseconds);
extern int I2CWrite(UINT8 I2CSlaveAddr, UINT8 *pdata, int length);
extern int I2CRead(UINT8 I2CSlaveAddr, UINT8 *pdata, int length);


#endif /* __MXL_CTRL_HWD_H__*/

