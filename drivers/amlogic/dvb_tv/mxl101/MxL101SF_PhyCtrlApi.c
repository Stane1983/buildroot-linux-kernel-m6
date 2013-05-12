/*******************************************************************************
 *
 * FILE NAME          : MxL101SF_PhyCtrlApi.cpp
 * 
 * AUTHOR             : Brenndon Lee
 * DATE CREATED       : 1/22/2008
 *
 * DESCRIPTION        : This file contains control APIs that configure MxL101SF 
 *                      and read back the statistics through I2C interface.
 *                             
 *******************************************************************************
 *                Copyright (c) 2006, MaxLinear, Inc.
 ******************************************************************************/

#include "MxL101SF_PhyCtrlApi.h"
#include "MxL101SF_PhyDefs.h"
#include "MxL101SF_OEM_Drv.h"


#if 0
#define printdebug printf
#else
#define printdebug(a...)
#endif

const UINT8 MxLCtrlVersion[] = {6, 3, 8, 1};

// Variable to store tuned frequency for RSSI calculation
UINT32 MxLTunedFreq = 0; 

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxL101SF_API_SoftResetDevice
--| 
--| AUTHOR        : Brenndon Lee
--|
--| DATE CREATED  : 9/8/2008
--|
--| DESCRIPTION   : By writing any value into address 0xFF (AIC), all control 
--|                 registers are initialized to the default value.
--|                 AIC - Address Initiated Command
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxL101SF_API_SoftResetDevice(struct aml_fe_dev *fe)
{
  return Ctrl_ProgramRegisters(MxL_PhySoftReset, fe);
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxL101SF_API_GetChipInfo
--| 
--| AUTHOR        : Brenndon Lee
--|
--| DATE CREATED  : 2/7/2008
--|
--| DESCRIPTION   : This function returns MxL101SF Chip Id and version information.
--|                 Chip Id of MxL101SF has 0x61
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxL101SF_API_GetChipInfo(PMXL_DEV_INFO_T DevInfoPtr, struct aml_fe_dev *fe)
{
  UINT8 status = MXL_FALSE;
  UINT8 readBack;

  status = Ctrl_ReadRegister(CHIP_ID_REG, &readBack, fe);
  DevInfoPtr->DevId = readBack;

  status |= Ctrl_ReadRegister(TOP_CHIP_REV_ID_REG, &readBack, fe);
  DevInfoPtr->DevVer = (readBack & 0x0F);
  
  return (MXL_STATUS)status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxL101SF_API_SetMxLDeviceMode
--| 
--| AUTHOR        : Brenndon Lee
--|
--| DATE CREATED  : 1/15/2008
--|
--| DESCRIPTION   : MxL101SF has a built-in RF Tuner in addtion to DVB-T BB block
--|                 In tuner only mode, Digial BB will be disabled.
--|                 MXL_TUNER_MODE : Tuner mode, MXL_SOC_MODE : SOC mode
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxL101SF_API_SetMxLDeviceMode(PMXL_DEV_MODE_CFG_T DevModePtr, struct aml_fe_dev *fe)
{
  UINT8 status = MXL_FALSE;

  // Set Device mode
  if (DevModePtr->DeviceMode == MXL_SOC_MODE)
    status = Ctrl_ProgramRegisters(MxL_TunerDemodMode, fe);
  else if (DevModePtr->DeviceMode == MXL_TUNER_MODE)
    status = Ctrl_ProgramRegisters(MxL_TunerMode, fe);
  
  return (MXL_STATUS)status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxL101SF_API_TopMasterControl
--| 
--| AUTHOR        : Brenndon Lee
--|
--| DATE CREATED  : 4/17/2009
--|
--| DESCRIPTION   : Power up Tuner
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxL101SF_API_TopMasterControl(PMXL_TOP_MASTER_CFG_T TopMasterCtrlPtr, struct aml_fe_dev *fe)
{
  UINT8 status = MXL_FALSE;

  if (TopMasterCtrlPtr->TopMasterEnable == MXL_DISABLE)
    status = Ctrl_ProgramRegisters(MxL_TopMasterDisable, fe);
  else
    status = Ctrl_ProgramRegisters(MxL_TopMasterEnable, fe);
  
  return (MXL_STATUS)status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxL101SF_API_InitTunerDemod
--| 
--| AUTHOR        : Brenndon Lee
--|
--| DATE CREATED  : 7/30/2009
--|
--| DESCRIPTION   : Initializing Tuner and Demod block of MxL101SF
--|                 before tuning to a certain frequency.
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxL101SF_API_InitTunerDemod(struct aml_fe_dev *fe)
{
  return Ctrl_ProgramRegisters(MxL_101OverwriteDefault, fe);
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxL101SF_API_EnableMpegOutput
--| 
--| AUTHOR        : Brenndon Lee
--|
--| DATE CREATED  : 12/19/2008
--|
--| DESCRIPTION   : Enable TSIF for MPEG-2 data output
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxL101SF_API_EnableMpegOutput(struct aml_fe_dev *fe)
{
  return Ctrl_ProgramRegisters(MxL_MpegDataOutToTSIF, fe);
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxL101SF_API_ConfigMpegOut
--| 
--| AUTHOR        : Brenndon Lee
--|
--| DATE CREATED  : 1/15/2008
--|
--| DESCRIPTION   : For Transport stream output through ASI(MPEG TS) interface
--|                 the following parameters are needed to configure after or before
--|                 demod lock.
--|                  TS output mode : Seral or Parallel
--|                  CLK Phase : Normal or Inverted
--|                  MPEG Valid Polarity : Low or High
--|                  MPEG Sync Polarity  : Low or High
--|                  MPEG CLK Frequency  : 0 - 36.571429MHz
--|                                      : 1 - 2.285714MHz
--|                                      : 2 - 4.571429MHz
--|                                      : 3 - 6.857143MHz
--|                                      : 4 - 9.142857MHz
--|                                      : 5 - 13.714286MHz
--|                                      : 6 - 18.285714MHz
--|                                      : 7 - 27.428571MHz
--|                 * MxL101SF Feature
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxL101SF_API_ConfigMpegOut(PMXL_MPEG_CFG_T MpegOutCfgPtr, struct aml_fe_dev *fe)
{
  UINT8 status = MXL_TRUE;
  UINT8 mode, tmp;

  mode = 0xC0; // MPEG Out

  // Configure MPEG Clock phase
  if (MpegOutCfgPtr->MpegClkPhase == MPEG_CLK_IN_PHASE) 
    mode &= ~V6_INVERTED_CLK_PHASE;
  else 
    mode |= V6_INVERTED_CLK_PHASE;;

  if (MpegOutCfgPtr->MpegClkFreq > MPEG_CLOCK_27_428571MHz) 
    MpegOutCfgPtr->MpegClkFreq = MPEG_CLOCK_27_428571MHz;
  
  mode |= (((UINT8)MpegOutCfgPtr->MpegClkFreq) << 2);

  status = Ctrl_WriteRegister(V6_MPEG_OUT_CLK_INV_REG, mode, fe);

  // Configure data input mode, MPEG Valid polarity, MPEG Sync polarity
  // Get current configuration
  status |= Ctrl_ReadRegister(V6_MPEG_OUT_CTRL_REG, &mode, fe);
  mode &= 0xF0;

  // Data Input mode
  if (MpegOutCfgPtr->SerialOrPar == MPEG_DATA_PARALLEL) 
  {
    mode |= V6_MPEG_DATA_PARALLEL;
  }
  else 
  {
    mode |= V6_MPEG_DATA_SERIAL;

    // If serial interface is selected, configure MSB or LSB order in transmission
    status |= Ctrl_ReadRegister(V6_MPEG_INOUT_BIT_ORDER_CTRL_REG, &tmp, fe);

    if (MpegOutCfgPtr->LsbOrMsbFirst == MPEG_SERIAL_MSB_1ST) 
      tmp |= V6_MPEG_SER_MSB_FIRST;  
    else 
      tmp &= ~V6_MPEG_SER_MSB_FIRST; // LSB First

    status |= Ctrl_WriteRegister(V6_MPEG_INOUT_BIT_ORDER_CTRL_REG, tmp, fe);
  }
  
  // MPEG Sync polarity
  if (MpegOutCfgPtr->MpegSyncPol == MPEG_CLK_IN_PHASE) 
    mode &= ~V6_INVERTED_MPEG_SYNC;
  else 
    mode |= V6_INVERTED_MPEG_SYNC;

  // MPEG Valid polarity
  if (MpegOutCfgPtr->MpegValidPol == MPEG_CLK_IN_PHASE) 
    mode &= ~V6_INVERTED_MPEG_VALID; 
  else 
    mode |= V6_INVERTED_MPEG_VALID;

  status |= Ctrl_WriteRegister(V6_MPEG_OUT_CTRL_REG, mode, fe);

  return (MXL_STATUS)status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxL101SF_API_ConfigDevPowerSavingMode
--| 
--| AUTHOR        : Mahendra Kondur
--|
--| DATE CREATED  : 10/20/2009
--|
--| DESCRIPTION   : This function configures Standby mode and Sleep mode to
--|                 control power consumption.
--|
--| RETURN VALUE  : True or False
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxL101SF_API_ConfigDevPowerSavingMode(PMXL_PWR_MODE_CFG_T PwrModePtr, struct aml_fe_dev *fe)
{
  UINT8 status = MXL_FALSE;
  
  switch (PwrModePtr->PowerMode)
  {
    case STANDBY_ON:
    case SLEEP_ON:
      status = Ctrl_ProgramRegisters(MxL_101SuspendMode, fe);
      break;

    case STANDBY_OFF:
    case SLEEP_OFF:
      status = Ctrl_ProgramRegisters(MxL_101WakeUp, fe);
      status |= MxL101SF_API_SoftResetDevice(fe);
      status |= MxL101SF_API_InitTunerDemod(fe);
      break;

    default:
      break;
  }

  return (MXL_STATUS)status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxL101SF_API_TuneRF
--| 
--| AUTHOR        : Brenndon Lee
--|                 Mahendra Kondur - updated new topmastercontrol function name
--|                                   
--|
--| DATE CREATED  : 1/15/2008
--|                 10/23/2009
--|                 11/20/2009 - Settings for TPS Cell ID
--|
--| DESCRIPTION   : After power-on initialization, when channel frequency has 
--|                 changed, this function shall be called
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxL101SF_API_TuneRF(PMXL_RF_TUNE_CFG_T TuneParamPtr, struct aml_fe_dev *fe)
{
  PREG_CTRL_INFO_T ctrlArrayPtr;
  MXL_DEMOD_LOCK_STATUS_T cpLock;
  MXL_BOOL cpLockStatus = MXL_OFF;
  UINT8 status = MXL_FALSE;
  UINT8 mxl_mode;
  UINT8 regData = 0;
  UINT32 cellIdStartTime;
  UINT32 StartTime;
  UINT32 cellIdEndTime;
  UINT16 TimeOut;
  UINT8 agcLockStatus;
  printdebug("MxL101SF_API_TuneRF] in\n");
  // Stop Tune
  status = Ctrl_WriteRegister(START_TUNE_REG, 0, fe);

  // Check Device mode
  status |= Ctrl_ReadRegister(MXL_MODE_REG, &mxl_mode, fe);
printdebug("mxl_mode is %d,\n",mxl_mode);
  // Fill out registers for channel tune
  ctrlArrayPtr = Ctrl_PhyTune(TuneParamPtr->Frequency, TuneParamPtr->Bandwidth);
  
  if (!ctrlArrayPtr) return MXL_FALSE;
  
  // Program registers 
  status |= Ctrl_ProgramRegisters(ctrlArrayPtr, fe);

  // Enable TPS Cell ID feature
  printf("TuneParamPtr->TpsCellIdRbCtrl is %d\n",TuneParamPtr->TpsCellIdRbCtrl);
  if (TuneParamPtr->TpsCellIdRbCtrl == MXL_ENABLE)
    status |= Ctrl_ProgramRegisters(MxL_EnableCellId, fe); 

  // Start tune
  status |= Ctrl_ProgramRegisters(MxL_StartTune, fe);

 if ((mxl_mode & MXL_DEV_MODE_MASK) == MXL_SOC_MODE)
  {
    Ctrl_GetTime(&cellIdStartTime);
    Ctrl_GetTime(&StartTime);
    cellIdEndTime = cellIdStartTime;   
    TimeOut = 300;
  
    while ((cellIdEndTime - cellIdStartTime) < TimeOut)  
    {
      // Check for CP Lock
      status |= MxLWare_API_GetDemodStatus(MXL_DEMOD_CP_LOCK_REQ, &cpLock, fe);

      if (cpLock.Status == MXL_LOCKED) 
      {
        cpLockStatus = MXL_ON;
        break;
      }

      Ctrl_GetTime(&cellIdEndTime);
    }

    if (cpLockStatus == MXL_OFF) 
    {
      if (TuneParamPtr->TpsCellIdRbCtrl != MXL_ENABLE)
        status |= Ctrl_WriteRegister(START_TUNE_REG, 0, fe);

      status |= Ctrl_WriteRegister(0x00, 0x01, fe);
      status |= Ctrl_ReadRegister(0x81, &regData, fe);
      regData = regData & 0xE3;
      status |= Ctrl_WriteRegister(0x81, regData, fe);
      status |= Ctrl_WriteRegister(0x00, 0x00, fe);

      if (TuneParamPtr->TpsCellIdRbCtrl != MXL_ENABLE)
        status |= Ctrl_WriteRegister(START_TUNE_REG, 1, fe);
    }  
  
    // Prep Demod to extract TPS cell ID
    if (TuneParamPtr->TpsCellIdRbCtrl == MXL_ENABLE)
    {
      Ctrl_GetTime(&cellIdStartTime);
      Ctrl_GetTime(&StartTime);
      cellIdEndTime = cellIdStartTime;   
      TimeOut = 1024;

      // Check for AGC Lock
      status = Ctrl_ReadRegister(V6_AGC_LOCK_REG, &regData, fe);
      agcLockStatus = (regData & V6_AGC_LOCK_MASK) >> 5;

      if (agcLockStatus)
        TimeOut = 2048;

      while ((cellIdEndTime - cellIdStartTime) < 350)  
      {
        // Check for CP Lock
        status |= MxLWare_API_GetDemodStatus(MXL_DEMOD_CP_LOCK_REQ, &cpLock, fe);
        
        if (cpLock.Status == MXL_UNLOCKED) 
        {
          if ((cellIdEndTime - StartTime) < TimeOut)
            Ctrl_GetTime(&cellIdStartTime);
          else
            break;
        }
        Ctrl_GetTime(&cellIdEndTime);
      }

      status |= Ctrl_ProgramRegisters(MxL_DisableCellId, fe); 
	  
    }
  }
printdebug("MxL101SF_API_TuneRF] out\n");
  MxLTunedFreq = TuneParamPtr->Frequency;
  return (MXL_STATUS)status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxL101SF_API_DemodGetSNR
--| 
--| AUTHOR        : Brenndon Lee
--|                 Mahendra Kondur - Support for Interger calculation of SNR
--|
--| DATE CREATED  : 1/24/2008
--|                 1/12/2010
--|
--| DESCRIPTION   : This function returns SNR(Signal to Noise Ratio).
--|                 SNR is calculated as follows after reading 10bit register
--|                 Folting-point calculation:
--|                  SNR = 10 * SNR_REG_VALUE / 64 - 2.5  dB
--|                 Integer calculation: 
--|                  10000 x SNR = 1563 x SNR_REG_VALUE - 25000  dB 
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxL101SF_API_DemodGetSNR(PMXL_DEMOD_SNR_INFO_T SnrPtr, struct aml_fe_dev *fe)
{
  UINT8 status = MXL_FALSE;
  UINT32 snrCalc;
  UINT16 snrData;
  UINT8 rawData;

  status = Ctrl_ReadRegister(V6_SNR_RB_LSB_REG, &rawData, fe);
  snrData = rawData;
 // printf("[MxL101SF_API_DemodGetSNR] snrData_lsb  is %x\n",snrData);
  status |= Ctrl_ReadRegister(V6_SNR_RB_MSB_REG, &rawData, fe);
  snrData |= (rawData & 0x03) << 8;
//  printf("[MxL101SF_API_DemodGetSNR] snrData_msb  is %x\n",rawData);
  if (status == MXL_TRUE)
  {
    snrCalc =  CALCULATE_SNR(snrData);
    SnrPtr->SNR = snrCalc;
  }
  
  return (MXL_STATUS)status;       
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxL101SF_API_DemodGetBER
--| 
--| AUTHOR        : Brenndon Lee
--|                 Mahendra Kondur - Support for Interger calculation of BER
--|
--| DATE CREATED  : 1/24/2008
--|                 2/24/2010
--|
--| DESCRIPTION   : This function returns BER(Bit Error Ratio).
--|                 Floating-point calculation
--|                  BER = (AVG_ERRORS x 4)/(N_ACCUMULATE x 64 x 188 x 8)
--|                 Integer calculation
--|                  1e10 x BER = (AVG_ERRORS x 4 x 3247)
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxL101SF_API_DemodGetBER(PMXL_DEMOD_BER_INFO_T BerInfoPtr, struct aml_fe_dev *fe)
{
  UINT8 status;
  UINT32 berCalc;
  UINT32 n_accumulate;
  UINT16 avg_errors;
  UINT8 rawData;
  
  avg_errors = 0;
  status = Ctrl_ReadRegister(V6_RS_AVG_ERRORS_LSB_REG, &rawData, fe);
  avg_errors = rawData;

  status |= Ctrl_ReadRegister(V6_RS_AVG_ERRORS_MSB_REG, &rawData, fe);
  avg_errors |= (rawData << 8);

  status |= Ctrl_ReadRegister(V6_N_ACCUMULATE_REG, &rawData, fe);
  n_accumulate = rawData;

  berCalc = CALCULATE_BER(avg_errors, n_accumulate);
//printf("[MxL101SF_API_DemodGetBER] berCalc  is %x\n",berCalc);
  if (status == MXL_TRUE)
  {
    BerInfoPtr->BER = berCalc;
  }

  return (MXL_STATUS)status;       
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxL101SF_API_DemodGetSyncLockStatus
--| 
--| AUTHOR        : Brenndon Lee
--|
--| DATE CREATED  : 5/6/2009
--|
--| DESCRIPTION   : This function return SYNC_LOCK Status 
--|                 if 1, Locked
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxL101SF_API_DemodGetSyncLockStatus(PMXL_DEMOD_LOCK_STATUS_T SyncLockPtr, struct aml_fe_dev *fe)
{
  MXL_STATUS status;
  UINT8 rawData;
  
  status = Ctrl_ReadRegister(V6_SYNC_LOCK_REG, &rawData, fe);
printdebug("[MxL101SF_API_DemodGetSyncLockStatus] rawData  is %x\n",rawData);
  if (status == MXL_TRUE)
  { 
    if ((rawData & SYNC_LOCK_MASK) >> 4)
      SyncLockPtr->Status = MXL_LOCKED;
    else
      SyncLockPtr->Status = MXL_UNLOCKED;
  }

  return status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxL101SF_API_DemodGetRsLockStatus
--| 
--| AUTHOR        : Brenndon Lee
--|
--| DATE CREATED  : 4/30/2008
--|
--| DESCRIPTION   : This function returns RS Lock status
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxL101SF_API_DemodGetRsLockStatus(PMXL_DEMOD_LOCK_STATUS_T RsLockPtr, struct aml_fe_dev *fe)
{
  MXL_STATUS status;
  UINT8 rawData;

  status = Ctrl_ReadRegister(V6_RS_LOCK_DET_REG, &rawData, fe);
//printdebug("[MxL101SF_API_DemodGetRsLockStatus] rawData  is %x\n",rawData);
  if (status == MXL_TRUE)
  {
    if ((rawData & RS_LOCK_DET_MASK) >> 3)
      RsLockPtr->Status = MXL_LOCKED;
    else
      RsLockPtr->Status = MXL_UNLOCKED;
  }
 
  return status;       
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxL101SF_API_DemodGetCpLockStatus
--| 
--| AUTHOR        : Brenndon Lee
--|
--| DATE CREATED  : 4/30/2008
--|
--| DESCRIPTION   : This function returns CP Lock status
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxL101SF_API_DemodGetCpLockStatus(PMXL_DEMOD_LOCK_STATUS_T CpLockPtr, struct aml_fe_dev *fe)
{
  MXL_STATUS status;
  UINT8 rawData;

  status = Ctrl_ReadRegister(V6_CP_LOCK_DET_REG, &rawData, fe);
printdebug("[MxL101SF_API_DemodGetCpLockStatus] rawData  is %x\n",rawData);
  if (status == MXL_TRUE)
  {
    if((rawData & V6_CP_LOCK_DET_MASK) >> 2)
      CpLockPtr->Status = MXL_LOCKED;
    else
      CpLockPtr->Status = MXL_UNLOCKED;
  }
 
  return status;       
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxL101SF_API_DemodResetIrqStatus
--| 
--| AUTHOR        : Brenndon Lee
--|
--| DATE CREATED  : 1/24/2008
--|                 9/09/2008
--|
--| DESCRIPTION   : This function clears IRQ status registers
--|                 Writing 0xFF to this register will clear
--|                 the previous status
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxL101SF_API_DemodResetIrqStatus(struct aml_fe_dev *fe)
{
  return Ctrl_ProgramRegisters(MxL_IrqClear, fe);
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxL101SF_API_DemodGetTpsCodeRate
--| 
--| AUTHOR        : Brenndon Lee
--|
--| DATE CREATED  : 1/24/2008
--|
--| DESCRIPTION   : This function returns code rate of TPS parameters
--|                 bit<2:0> - 000:1/2, 001:2/3, 010:3/4, 011:5/6, 100:7/8
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxL101SF_API_DemodGetTpsCodeRate(PMXL_DEMOD_TPS_INFO_T TpsCodeRatePtr, struct aml_fe_dev *fe)
{
  MXL_STATUS status;
  UINT8 rawData;
  
  status = Ctrl_ReadRegister(V6_CODE_RATE_TPS_REG, &rawData, fe);
printdebug("[MxL101SF_API_DemodGetTpsCodeRate] rawData  is %x\n",rawData);
  if (status == MXL_TRUE)
    TpsCodeRatePtr->TpsInfo = rawData & V6_CODE_RATE_TPS_MASK;
  
  return status;       
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxL101SF_API_DemodGetTpsHierarchy
--| 
--| AUTHOR        : Brenndon Lee
--|
--| DATE CREATED  : 1/24/2008
--|
--| DESCRIPTION   : This function returns Hiearchy information of TPS parameters
--|                 bit<6:4> - 000:Non hierarchy, 001:1, 010:2, 011:4
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxL101SF_API_DemodGetTpsHierarchy(PMXL_DEMOD_TPS_INFO_T TpsHierarchyPtr, struct aml_fe_dev *fe)
{
  MXL_STATUS status;
  UINT8 rawData;

  status = Ctrl_ReadRegister(V6_TPS_HIERACHY_REG, &rawData, fe);
printdebug("[MxL101SF_API_DemodGetTpsHierarchy] rawData  is %x\n",rawData);
  if (status == MXL_TRUE)
    TpsHierarchyPtr->TpsInfo = (rawData & V6_TPS_HIERARCHY_INFO_MASK) >> 6;
  
  return status;       
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxL101SF_API_ConfigStreamPriority
--| 
--| AUTHOR        : Mahendra Kondur
--|
--| DATE CREATED  : 1/14/2010
--|
--| DESCRIPTION   : This API will config High priority or Low priority stream
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxL101SF_API_ConfigStreamPriority(PMXL_DEMOD_TS_PRIORITY_CFG_T TsPriorityPtr, struct aml_fe_dev *fe)
{
  UINT8 status;
  UINT8 rawData;

  status = Ctrl_ReadRegister(V6_TPS_HPORLP_REG, &rawData, fe);

  if (status == MXL_TRUE)
  {
    if (TsPriorityPtr->StreamPriority == LP_STREAM)
      rawData = (rawData | 0x80);
    else
      rawData = (rawData & 0x7F);

    status |= Ctrl_WriteRegister(V6_TPS_HPORLP_REG, rawData, fe);
  }
    
  return (MXL_STATUS)status;       
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxL101SF_API_DemodGetHierarchicalAlphaValue
--| 
--| AUTHOR        : Mahendra Kondur
--|
--| DATE CREATED  : 1/14/2010
--|
--| DESCRIPTION   : This function returns Hierarchical Alpha of TPS parameters
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxL101SF_API_DemodGetHierarchicalAlphaValue(PMXL_DEMOD_TPS_INFO_T TpsAlphaPtr, struct aml_fe_dev *fe)
{
  MXL_STATUS status;
  UINT8 rawData;

  status = Ctrl_ReadRegister(V6_TPS_ALPHA_REG, &rawData, fe);
printdebug("[MxL101SF_API_DemodGetHierarchicalAlphaValue] rawData  is %x\n",rawData);
  if (status == MXL_TRUE)
    TpsAlphaPtr->TpsInfo = (rawData & V6_TPS_ALPHA_MASK);
  
  return status;       
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxL101SF_API_DemodGetTpsConstellation
--| 
--| AUTHOR        : Brenndon Lee
--|
--| DATE CREATED  : 1/24/2008
--|
--| DESCRIPTION   : This function return Constellation status bit information
--|                 Constellation, 00 : QPSK, 01 : 16QAM, 10:64QAM   
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxL101SF_API_DemodGetTpsConstellation(PMXL_DEMOD_TPS_INFO_T TpsConstellationPtr, struct aml_fe_dev *fe)
{
  MXL_STATUS status;
  UINT8 tpsParams;
  
  status = Ctrl_ReadRegister(V6_MODORDER_TPS_REG, &tpsParams, fe);
 printdebug("[MxL101SF_API_DemodGetTpsConstellation] tpsParams  is %x\n",tpsParams);
  if (status == MXL_TRUE)
    TpsConstellationPtr->TpsInfo = (tpsParams & V6_PARAM_CONSTELLATION_MASK) >> 4 ;
  
  return status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxL101SF_API_DemodGetTpsFftMode
--| 
--| AUTHOR        : Brenndon Lee
--|
--| DATE CREATED  : 1/24/2008
--|
--| DESCRIPTION   : This function return FFT Mode status bit information
--|                 FFT Mode, 00:2K, 01:8K, 10:4K
--| 
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxL101SF_API_DemodGetTpsFftMode(PMXL_DEMOD_TPS_INFO_T TpsFftModePtr, struct aml_fe_dev *fe)
{
  MXL_STATUS status;
  UINT8 tpsParams;
  
  status = Ctrl_ReadRegister(V6_MODE_TPS_REG, &tpsParams, fe);
  printdebug("[MxL101SF_API_DemodGetTpsFftMode] tpsParams  is %x\n",tpsParams);
  if (status == MXL_TRUE)
    TpsFftModePtr->TpsInfo = (tpsParams & V6_PARAM_FFT_MODE_MASK) >> 2 ;
  
  return status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxL101SF_API_DemodGetTpsGuardInterval
--| 
--| AUTHOR        : Brenndon Lee
--|
--| DATE CREATED  : 1/24/2008
--|
--| DESCRIPTION   : This function return GI status bit information
--|                 00:1/32, 01:1/16, 10:1/8, 11:1/4
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxL101SF_API_DemodGetTpsGuardInterval(PMXL_DEMOD_TPS_INFO_T TpsGIPtr, struct aml_fe_dev *fe)
{
  MXL_STATUS status;
  UINT8 tpsParams;
  
  status = Ctrl_ReadRegister(V6_CP_TPS_REG, &tpsParams, fe);
 printdebug("[MxL101SF_API_DemodGetTpsGuardInterval] tpsParams  is %x\n",tpsParams);
  if (status == MXL_TRUE)
    TpsGIPtr->TpsInfo = (tpsParams & V6_PARAM_GI_MASK) >> 4 ;
  
  return status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxL101SF_API_DemodGetTpsLock
--| 
--| AUTHOR        : Brenndon Lee
--|
--| DATE CREATED  : 7/29/2008
--|
--| DESCRIPTION   : This function returns Tps lock status bit information.
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxL101SF_API_DemodGetTpsLock(PMXL_DEMOD_LOCK_STATUS_T TpsLockPtr, struct aml_fe_dev *fe)
{
  MXL_STATUS status;
  UINT8 tpsParams;
  
  status = Ctrl_ReadRegister(V6_TPS_LOCK_REG, &tpsParams, fe);
  printf("[MxL101SF_API_DemodGetTpsLock] tpsParams is %x\n",tpsParams);
  if (status == MXL_TRUE)
  {
    if((tpsParams & V6_PARAM_TPS_LOCK_MASK) >> 6)
      TpsLockPtr->Status = MXL_LOCKED;
    else
      TpsLockPtr->Status = MXL_UNLOCKED;
  }

  return status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxL101SF_API_DemodGetPacketErrorCount
--| 
--| AUTHOR        : Brenndon Lee
--|
--| DATE CREATED  : 3/13/2008
--|
--| DESCRIPTION   : This function returns TS Packet error count.
--|                 
--|                  PER Count = FEC_PER_COUNT * (2 ** (FEC_PER_SCALE * 4))
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxL101SF_API_DemodGetPacketErrorCount(PMXL_DEMOD_PEC_INFO_T PecInfoPtr, struct aml_fe_dev *fe)
{
  UINT8 status;
  UINT32 fec_per_count, fec_per_scale;
  UINT8 rawData;
  
  // FEC_PER_COUNT Register
  status = Ctrl_ReadRegister(V6_FEC_PER_COUNT_REG, &rawData, fe);
  fec_per_count = rawData;
 
  // FEC_PER_SCALE Register
  status |= Ctrl_ReadRegister(V6_FEC_PER_SCALE_REG, &rawData, fe);
  
  rawData &= V6_FEC_PER_SCALE_MASK;
  rawData *= 4;
  
  fec_per_scale = 1 << rawData;
  
  fec_per_count *= fec_per_scale;
  
  PecInfoPtr->PEC = fec_per_count;
  printdebug("[MxL101SF_API_DemodGetPacketErrorCount] fec_per_count  is %x\n",fec_per_count);
  return (MXL_STATUS)status;       
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxL101SF_API_DemodResetPacketErrorCount
--| 
--| AUTHOR        : Brenndon Lee
--|
--| DATE CREATED  : 3/13/2008
--|
--| DESCRIPTION   : This function resets TS Packet error count.
--|                 
--|                 After setting 7th bit of V5_PER_COUNT_RESET_REG,
--|                 it should be reset to 0.
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxL101SF_API_DemodResetPacketErrorCount(struct aml_fe_dev *fe)
{
  return Ctrl_ProgramRegisters(MxL_ResetPerCount, fe);
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxL101SF_API_TunerGetLockStatus
--| 
--| AUTHOR        : Brenndon Lee
--|
--| DATE CREATED  : 7/3/2008
--|
--| DESCRIPTION   : This function provides RF synthesizer lock status
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxL101SF_API_TunerGetLockStatus(PMXL_TUNER_LOCK_STATUS_T TunerLockStatusPtr, struct aml_fe_dev *fe)
{
  MXL_STATUS status;
  UINT8 data;

  // Initialize
  TunerLockStatusPtr->RefSynthLock = MXL_UNLOCKED;
  TunerLockStatusPtr->RfSynthLock = MXL_UNLOCKED;
  
  status = Ctrl_ReadRegister(V6_RF_LOCK_STATUS_REG, &data, fe);

  if (status == MXL_TRUE)
  {
    if ((data & 3) == 3)
      TunerLockStatusPtr->RefSynthLock = MXL_LOCKED;;  // Locked

    if ((data & 0xc) == 0xc)
      TunerLockStatusPtr->RfSynthLock = MXL_LOCKED;;  // Locked
  }

  return status;       
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxL101SF_API_TunerSetIFOutputFreq
--| 
--| AUTHOR        : Brenndon Lee
--|
--| DATE CREATED  : 7/7/2008
--|                 2/24/2010
--|
--| DESCRIPTION   : In Tuner only mode of MxL101SF, this function configures
--|                 IF output frequency
--|                 Support for IF Frequency in integer format
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxL101SF_API_TunerSetIFOutputFreq(PMXL_TUNER_IF_FREQ_T IfFreqParamPtr, struct aml_fe_dev *fe)
{
  UINT8 status;
  UINT8 control;
  UINT16 IFFCW;

  // Set IF polarity  
  if (IfFreqParamPtr->IF_Polarity == TUNER_NORMAL_IF_SPECTRUM) 
    control = TUNER_NORMAL_IF_SPECTRUM;
  else 
    control = TUNER_INVERT_IF_SPECTRUM;

  if (IfFreqParamPtr->IF_Index < IF_OTHER_35MHZ_45MHZ)
  {
    control |= (UINT8)IfFreqParamPtr->IF_Index;
  }

  status = Ctrl_WriteRegister(V6_TUNER_IF_SEL_REG, control, fe);

  IfFreqParamPtr->IF_Freq /= 1000000;
    
  if (IfFreqParamPtr->IF_Index == 0) 
  {
    control = 0x08;

    IFFCW = (UINT16)(IfFreqParamPtr->IF_Freq / (108 * 4096));
  }
  else if (IfFreqParamPtr->IF_Index == 15)
  {
    control = 0x08;

    IFFCW = (UINT16)(IfFreqParamPtr->IF_Freq / (216 * 4096));
  }
  else
  {
    control = 0;
    IFFCW = 0;
  }

  control |= (IFFCW >> 8);
  status |= Ctrl_WriteRegister(V6_TUNER_IF_FCW_BYP_REG, control, fe);

  control = IFFCW & 0x00FF;
  status |= Ctrl_WriteRegister(V6_TUNER_IF_FCW_REG, control, fe);

  return (MXL_STATUS)status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxL101SF_API_TunerLoopThruControl
--| 
--| AUTHOR        : Brenndon Lee
--|                 Mahendra
--|
--| DATE CREATED  : 7/7/2008
--|                 8/4/2009
--|
--| DESCRIPTION   : If loop through mode is enabled, RF signal from the antenna
--|                 is looped through to an external demodulator.
--|                  0 : Disable, 1: Enable, 
--|                 API is reorganised according to the MxL101SF control structure 
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxL101SF_API_TunerLoopThruControl(MXL_BOOL EnableDisable, struct aml_fe_dev *fe)
{
  MXL_STATUS status = MXL_TRUE;
  UINT8 control;

  if (EnableDisable == MXL_DISABLE) control = 0; // Disable
  else control = 1;             // Enable

  status = Ctrl_WriteRegister(V6_TUNER_LOOP_THRU_CTRL_REG, control, fe);

  return status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxL101SF_API_XtalSelect
--| 
--| AUTHOR        : Brenndon Lee
--|                 Mahendra Kondur
--|
--| DATE CREATED  : 7/7/2008
--|                 8/4/2009
--|
--| DESCRIPTION   : Select XTAL frequency of CLK out 
--|                 4 : 24MHz, 8 : 28.8MHz, 7 : 27 MHz, 12 : 48 MHz
--|                 API is reorganised according to the MxL1x1SF control structure 
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxL101SF_API_XtalSelect(MXL_XTAL_FREQ_E XtalFreq, struct aml_fe_dev *fe)
{
  UINT8 status = MXL_TRUE;
  
  status = Ctrl_WriteRegister(V6_DIG_CLK_FREQ_SEL_REG, (UINT8)XtalFreq, fe);
  status |= Ctrl_WriteRegister(V6_DIG_RFREFSELECT_REG, ((UINT8)XtalFreq)|0xA0, fe);

  return (MXL_STATUS)status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxL101SF_API_XtalClkOutGain
--| 
--| AUTHOR        : Brenndon Lee
--|                 Mahendra Kondur
--|
--| DATE CREATED  : 7/7/2008
--|                 8/4/2009
--|
--| DESCRIPTION   : If Xtal Clock out is enabled, then a valid clk out gain value
--|                 should be programmed to the chip.
--|                 API is reorganised according to the MxL1x1SF control structure 
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxL101SF_API_XtalClkOutGain(MXL_XTAL_CLK_OUT_GAIN_E ClkOutGain, struct aml_fe_dev *fe)
{
  if (ClkOutGain > CLK_OUT_18_75dB) ClkOutGain = CLK_OUT_18_75dB;

  return Ctrl_WriteRegister(V6_XTAL_CLK_OUT_GAIN_REG, (UINT8)ClkOutGain, fe);
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxL101SF_API_XtalClkOutControl
--| 
--| AUTHOR        : Brenndon Lee
--|                 Mahendra Kondur
--|
--| DATE CREATED  : 7/7/2008
--|                 8/4/2009
--|
--| DESCRIPTION   : XTAL Clock out control
--|                  0 : Disable, 1: Enable,
--|                 API is reorganised according to the MxL1x1SF control structure 
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxL101SF_API_XtalClkOutControl(MXL_BOOL EnableDisable, struct aml_fe_dev *fe)
{
  UINT8 status = MXL_TRUE;
  UINT8 control;

  status = Ctrl_ReadRegister(V6_DIG_XTAL_ENABLE_REG, &control, fe);

  if (EnableDisable == MXL_DISABLE) control &= 0xDF; // Disable
  else control |= 0x20;             // Enable

  status |= Ctrl_WriteRegister(V6_DIG_XTAL_ENABLE_REG, control, fe);

  return (MXL_STATUS)status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxL101SF_API_XtalBiasControl
--| 
--| AUTHOR        : Brenndon Lee
--|                 Mahendra Kondur
--|
--| DATE CREATED  : 7/7/2008
--|                 8/4/2009
--|
--| DESCRIPTION   : 0 : 200uA, 1 : 575 uA, ...
--|                 API is reorganised according to the MxL1x1SF control structure 
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxL101SF_API_XtalBiasControl(MXL_XTAL_BIAS_E XtalBias, struct aml_fe_dev *fe)
{
  MXL_STATUS status = MXL_TRUE;
  UINT8 control;

  control = 0x10;          // Xtal Enable

  if (XtalBias > I894_2uA) XtalBias = I894_2uA;

  control |= (UINT8)XtalBias;

  status = Ctrl_WriteRegister(V6_DIG_XTAL_BIAS_REG, control, fe);

  return status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxL101SF_API_XtalCapControl
--| 
--| AUTHOR        : Brenndon Lee
--|                 Mahendra Kondur
--|
--| DATE CREATED  : 7/7/2008
--|                 8/4/2009
--|
--| DESCRIPTION   : XTAL Clock Cap control
--|                 0 : 10pF, 1 : 1 pF, ...
--|                 API is reorganised according to the MxL1x1SF control structure 
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxL101SF_API_XtalCapControl(UINT8 XtalCapacitor, struct aml_fe_dev *fe)
{
  MXL_STATUS status = MXL_TRUE;

  if ((XtalCapacitor < 26) || (XtalCapacitor == 0x3F)) 
    status = Ctrl_WriteRegister(V6_XTAL_CAP_REG, XtalCapacitor, fe);

  return status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxL101SF_API_ConfigXtalSettings
--| 
--| AUTHOR        : Mahendra Kondur
--|
--| DATE CREATED  : 8/4/2009
--|
--| DESCRIPTION   : This API shall configure XTAL settings for MxL101SF
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxL101SF_API_ConfigXtalSettings(PMXL_XTAL_CFG_T XtalCfgPtr, struct aml_fe_dev *fe)
{
  UINT8 status;
  
  status = MxL101SF_API_TunerLoopThruControl(XtalCfgPtr->LoopThruEnable, fe);
  status |= MxL101SF_API_XtalSelect(XtalCfgPtr->XtalFreq, fe);
  status |= MxL101SF_API_XtalClkOutGain(XtalCfgPtr->XtalClkOutGain, fe);
  status |= MxL101SF_API_XtalClkOutControl(XtalCfgPtr->XtalClkOutEnable, fe);
  status |= MxL101SF_API_XtalBiasControl(XtalCfgPtr->XtalBiasCurrent, fe);
  status |= MxL101SF_API_XtalCapControl(XtalCfgPtr->XtalCap, fe);

  return (MXL_STATUS)status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxL101SF_API_GetTunerSignalStrength
--| 
--| AUTHOR        : Mahendra
--|
--| DATE CREATED  : 10/20/2009
--|
--| DESCRIPTION   : This API shall give MxL101SF's RF Signal Strength in dBm unit
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxL101SF_API_GetTunerSignalStrength(PMXL_SIGNAL_STATS_T SigQualityPtr, struct aml_fe_dev *fe)
{
  UINT8 status;
  UINT8 regData;
  UINT16 rxPwr;
  SINT16 rfPwrAdj = 0;

  status = Ctrl_WriteRegister(0x00, 0x02, fe);
  
  status |= Ctrl_ReadRegister(V6_DIG_RF_PWR_LSB_REG, &regData, fe);
  rxPwr = regData;

  status |= Ctrl_ReadRegister(V6_DIG_RF_PWR_MSB_REG, &regData, fe);
  rxPwr |= (regData & 0x07) << 8;
  MxLTunedFreq = MxLTunedFreq/1000000;
  
  if (MxLTunedFreq <= 131) rfPwrAdj = - 4;
  else if (MxLTunedFreq <= 143) rfPwrAdj = 40;
  else if (MxLTunedFreq <= 296) rfPwrAdj = 36;
  else if (MxLTunedFreq <= 308) rfPwrAdj = 41;
  else if (MxLTunedFreq <= 320) rfPwrAdj = 44;
  else if (MxLTunedFreq <= 332) rfPwrAdj = 52;
  else if (MxLTunedFreq <= 422) rfPwrAdj = 39;       
  else if (MxLTunedFreq <= 506) rfPwrAdj = 33;
  else if (MxLTunedFreq <= 566) rfPwrAdj = 25;
  else if (MxLTunedFreq <= 650) rfPwrAdj = 20;        
  else if (MxLTunedFreq <= 800) rfPwrAdj = 14;
  else if (MxLTunedFreq <= 860) rfPwrAdj = 21;            
  else if (MxLTunedFreq > 860) rfPwrAdj = 29;

  SigQualityPtr->SignalStrength = ((rxPwr + rfPwrAdj) / 8) - 119;

  status |= Ctrl_WriteRegister(0x00, 0x00, fe);

  return (MXL_STATUS)status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxL101SF_API_ConfigCableSettings
--| 
--| AUTHOR        : Mahendra
--|
--| DATE CREATED  : 11/05/2009
--|
--| DESCRIPTION   : This API shall configure DVBC settings on MxL101SF
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxL101SF_API_ConfigCableSettings(struct aml_fe_dev *fe)
{
  return Ctrl_ProgramRegisters(MxL_CableSettings, fe);
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxL101SF_API_ConfigGPOPins
--| 
--| AUTHOR        : Mahendra
--|
--| DATE CREATED  : 10/02/2009
--|
--| DESCRIPTION   : API to confugure GPIO Pins of MxL101SF
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxL101SF_API_ConfigGPOPins(PMXL_DEV_GPO_CFG_T GpoPinCfgPtr, struct aml_fe_dev *fe)
{
  UINT8 status = MXL_FALSE;
  UINT8 regData = 0;
  UINT8 gpoMask = 0;

  status = Ctrl_ReadRegister(V6_GPO_CTRL_REG, &regData, fe); 

  switch(GpoPinCfgPtr->GpoPinId)
  {
    case MXL_GPO_0:
      gpoMask = V6_GPO_0_MASK;
      break;

    case MXL_GPO_1:
      gpoMask = V6_GPO_1_MASK;
      break;
    
    default:
      break;
  }

  if (GpoPinCfgPtr->GpoPinCtrl)
    regData |= gpoMask;
  else
    regData &= ~gpoMask;

  status |= Ctrl_WriteRegister(V6_GPO_CTRL_REG, regData, fe);  
  
  return (MXL_STATUS)status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxL101SF_API_DemodGetCellId
--| 
--| AUTHOR        : Mahendra Kondur
--|
--| DATE CREATED  : 11/19/2009
--|
--| DESCRIPTION   : This API Enables TPS Cell ID Feature
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxL101SF_API_DemodGetCellId(PMXL_DEMOD_CELL_ID_INFO_T tpsCellId, struct aml_fe_dev *fe)
{
  UINT8 status;
  UINT8 regData;
  UINT16 CellId;

  status = Ctrl_WriteRegister(0x00, 0x02, fe);
  
  status |= Ctrl_ReadRegister(0x98, &regData, fe);
  CellId = regData;

  status |= Ctrl_ReadRegister(0x99, &regData, fe);
  CellId |= regData << 8;

  tpsCellId->TpsCellId = CellId;

  status |= Ctrl_WriteRegister(0x00, 0x00, fe);
printdebug("[MxL101SF_API_DemodGetCellId] CellId  is %x\n",CellId);
  return (MXL_STATUS)status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxL101SF_API_GetAGCLock
--| 
--| AUTHOR        : Brenndon Lee
--|
--| DATE CREATED  : 1/15/2010
--|
--| DESCRIPTION   : This function return AGC_LOCK status bit information
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxL101SF_API_GetAGCLock(PMXL_DEMOD_LOCK_STATUS_T AgcLockPtr, struct aml_fe_dev *fe)
{
  MXL_STATUS status;
  UINT8 rawData;
  
  status = Ctrl_ReadRegister(V6_AGC_LOCK_REG, &rawData, fe);

  if (status == MXL_TRUE)
  {
    if((rawData & V6_AGC_LOCK_MASK) >> 5)
      AgcLockPtr->Status = MXL_LOCKED;
    else
      AgcLockPtr->Status = MXL_UNLOCKED;
  }

  return status;
}
/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxL101SF_API_GetFECLock
--| 
--| AUTHOR        : Brenndon Lee
--|
--| DATE CREATED  : 1/15/2010
--|
--| DESCRIPTION   : This function return FEC_LOCK status bit information
--|                 if set to 1, FEC_LOCK
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxL101SF_API_GetFECLock(PMXL_DEMOD_LOCK_STATUS_T FecLockPtr, struct aml_fe_dev *fe)
{
  MXL_STATUS status;
  UINT8 rawData;
  
  status = Ctrl_ReadRegister(V6_IRQ_STATUS_REG, &rawData, fe);

  if (status == MXL_TRUE)
  {
    if((rawData & IRQ_MASK_FEC_LOCK) >> 4)
      FecLockPtr->Status = MXL_LOCKED;
    else
      FecLockPtr->Status = MXL_UNLOCKED;
  }

  return status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxL101SF_API_ConfigSpectrum
--| 
--| AUTHOR        : Mahendra Kondur
--|
--| DATE CREATED  : 4/19/2010
--|
--| DESCRIPTION   : This function will configures MxL101SF to process inverted
--|                 signal.
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxL101SF_API_ConfigSpectrum(PMXL_DEMOD_SPECTRUM_CFG_T SpectrumCfgPtr, struct aml_fe_dev *fe)
{
  UINT8 status;
  UINT8 rawData;
  
  status = Ctrl_ReadRegister(V6_SPECTRUM_CTRL_REG, &rawData, fe);

  if (status == MXL_TRUE)
  {
    if(SpectrumCfgPtr->SpectrumCfg == MXL_ENABLE)
      rawData &= ~V6_SPECTRUM_MASK;
    else
      rawData |= V6_SPECTRUM_MASK;

    status |= Ctrl_WriteRegister(V6_SPECTRUM_CTRL_REG, rawData, fe);
  }

  return (MXL_STATUS)status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxL101SF_API_ConfigChannelScan
--| 
--| AUTHOR        : Mahendra Kondur
--|
--| DATE CREATED  : 2/22/2010
--|
--| DESCRIPTION   : This function Enables or Disables Channel Scan functionality
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxL101SF_API_ConfigChannelScan(PMXL_TUNER_CHAN_SCAN_CFG_T ChanScanCfgPtr, struct aml_fe_dev *fe)
{
  UINT8 Status = MXL_FALSE;

  if (ChanScanCfgPtr->ChanScanCfg == MXL_ENABLE)
    Ctrl_ProgramRegisters(MxL_EnableChanScan, fe);
  else
    Ctrl_ProgramRegisters(MxL_DisableChanScan, fe);

  return (MXL_STATUS)Status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxL101SF_API_GetChannelOffset
--| 
--| AUTHOR        : Brenndon Lee
--|
--| DATE CREATED  : 2/22/2010
--|
--| DESCRIPTION   : This function returns offset for a channel 
--|                 Frequency offset will be valid only if FEC lock has been achieved
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxL101SF_API_GetChannelOffset(PMXL_TUNER_CHAN_OFFSET_T ChanOffsetPtr, struct aml_fe_dev *fe)
{
  UINT8 status;
  UINT8 rawData;
  SINT16 freqOffset;

  status = Ctrl_WriteRegister(0, 2, fe); 

  // Read 0x9B registe first, otherwise V6_FREQ_OFFSET_LSB_REG/V6_FREQ_OFFSET_LSB_REG will return 0
  status |= Ctrl_ReadRegister(0x9B, &rawData, fe); 
  status |= Ctrl_ReadRegister(V6_FREQ_OFFSET_LSB_REG, &rawData, fe);
  freqOffset = rawData;
  status |= Ctrl_ReadRegister(V6_FREQ_OFFSET_MSB_REG, &rawData, fe);
  status |= Ctrl_WriteRegister(0, 0, fe); 

  freqOffset |= (rawData << 8);
  freqOffset &= 0x3FF;

  // Check the sign bit
  if (freqOffset & 0x0200) 
  {
    // Negative number, extend sign bit
    freqOffset |= 0xFC00;
  }

  ChanOffsetPtr->ChanOffset = (freqOffset * 2048) + 2048;

  return (MXL_STATUS)status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxL101SF_API_CheckChannel
--| 
--| AUTHOR        : Mahendra Kondur
--|
--| DATE CREATED  : 02/22/2010
--|                 05/01/2010
--|
--| DESCRIPTION   : This API will check for presence of singnal at different lock
--|                 levels (i.e CP, TPS, RS, FEC) for a given frequency.
--|                 - Removed checking for FEC lock.
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxL101SF_API_CheckChannel(PMXL_TUNER_CHECK_CHAN_REQ_T ChanScanCtrlPtr, struct aml_fe_dev *fe)
{
  UINT8 status;
  UINT32 StartTime;
  UINT32 EndTime;
  UINT16 TimeOut = 512;
  MXL_DEMOD_LOCK_STATUS_T  demodLockStatus;
  MXL_BOOL agcLockStatus = MXL_OFF;
  MXL_RF_TUNE_CFG_T mxlChanCfg;

 

  mxlChanCfg.Frequency = ChanScanCtrlPtr->Frequency;
  mxlChanCfg.Bandwidth = ChanScanCtrlPtr->Bandwidth;
  mxlChanCfg.TpsCellIdRbCtrl = ChanScanCtrlPtr->TpsCellIdRbCtrl;
  Ctrl_GetTime(&StartTime);
  EndTime = StartTime;

  MxL101SF_API_TuneRF((PMXL_RF_TUNE_CFG_T)&mxlChanCfg, fe);

 // Reset IRQ status
  status = MxL101SF_API_DemodResetIrqStatus(fe);
  ChanScanCtrlPtr->ChanPresent = MXL_FALSE;

  while ((EndTime - StartTime) < TimeOut)
  {
    status |= MxL101SF_API_GetAGCLock(&demodLockStatus, fe);
    if (demodLockStatus.Status == MXL_LOCKED) 
    {
      agcLockStatus = MXL_ON;
      TimeOut = 2048;
    }
    else
    {
      agcLockStatus = MXL_OFF;
      if (ChanScanCtrlPtr->TpsCellIdRbCtrl == MXL_ENABLE)
        TimeOut = 1024;
      else
        TimeOut = 512;
    }

    // Check if channel is available. Start with CP Lock
    status |= MxL101SF_API_DemodGetCpLockStatus(&demodLockStatus, fe);
    
    if (demodLockStatus.Status == MXL_LOCKED) 
    {
      TimeOut = TimeOut - (EndTime - StartTime);
      break;
    }

     Ctrl_GetTime(&EndTime);
  }

  if (ChanScanCtrlPtr->ChanScanCtrl == MXL_BREAK_AT_CP_LOCK)
  {
    // Break if interested to check channel status only till CP Lock
    if (demodLockStatus.Status == MXL_LOCKED) 
      ChanScanCtrlPtr->ChanPresent = MXL_TRUE;
    
    goto EXIT;
  }

  if (demodLockStatus.Status == MXL_LOCKED) 
  {
    // CP is locked, check TPS Lock 
    TimeOut = TimeOut + 512;
    EndTime = StartTime;
    while ((EndTime - StartTime) < TimeOut)  // < 1536ms (1024+512)
    {
      status |= MxL101SF_API_DemodGetTpsLock(&demodLockStatus, fe);

      if (demodLockStatus.Status == MXL_LOCKED) 
      {
        break;
      }

       Ctrl_GetTime(&EndTime);
    }

    if (ChanScanCtrlPtr->ChanScanCtrl == MXL_BREAK_AT_TPS_LOCK)
    {
      // Break if interested to check channel status only till TPS Lock
      if (demodLockStatus.Status == MXL_LOCKED) 
        ChanScanCtrlPtr->ChanPresent = MXL_TRUE;
      
      goto EXIT;
    }
  
    if (demodLockStatus.Status == MXL_LOCKED) 
    {
      // TPS is locked, check RS Lock 
      TimeOut = TimeOut + 3464;
      EndTime = StartTime;
  
      while ((EndTime - StartTime) < TimeOut) // < 5 secs (1024+512+3464)
      {
        status |= MxL101SF_API_DemodGetRsLockStatus(&demodLockStatus, fe);

        if (demodLockStatus.Status == MXL_LOCKED) 
        {
          break;
        }

         Ctrl_GetTime(&EndTime);
      }

      if (ChanScanCtrlPtr->ChanScanCtrl == MXL_BREAK_AT_RS_LOCK)
      {
        // Break if interested to check channel status only till RS Lock
        if (demodLockStatus.Status == MXL_LOCKED) 
          ChanScanCtrlPtr->ChanPresent = MXL_TRUE;
        
        goto EXIT;
      }
      if (demodLockStatus.Status == MXL_LOCKED) 
      {
        ChanScanCtrlPtr->ChanPresent = MXL_TRUE;
      } // RS Lock
    } // TPS Lock
  } // CP Lock
  
EXIT:
  return (MXL_STATUS)status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare_API_ConfigDevice
--| 
--| AUTHOR        : Mahendra Kondur
--|
--| DATE CREATED  : 08/08/2009
--|
--| DESCRIPTION   : The general device configuration shall be handled 
--|                 through this API
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxLWare_API_ConfigDevice(MXL_CMD_TYPE_E CmdType, void *ParamPtr, struct aml_fe_dev *fe)
{
  MXL_STATUS status = MXL_TRUE;

  switch (CmdType)
  {
    case MXL_DEV_SOFT_RESET_CFG:
      status = MxL101SF_API_SoftResetDevice(fe);
      break;

    case MXL_DEV_OPERATIONAL_MODE_CFG:
      status = MxL101SF_API_SetMxLDeviceMode((PMXL_DEV_MODE_CFG_T)ParamPtr, fe);
      break;

    case MXL_DEV_XTAL_SETTINGS_CFG:
      status = MxL101SF_API_ConfigXtalSettings((PMXL_XTAL_CFG_T)ParamPtr, fe);
      break;

    case MXL_DEV_101SF_OVERWRITE_DEFAULTS_CFG:
      status = MxL101SF_API_InitTunerDemod(fe);
      break;

    case MXL_DEV_101SF_POWER_MODE_CFG:
      status = MxL101SF_API_ConfigDevPowerSavingMode((PMXL_PWR_MODE_CFG_T)ParamPtr, fe);
      break;

    case MXL_DEV_MPEG_OUT_CFG:
      status = MxL101SF_API_ConfigMpegOut((PMXL_MPEG_CFG_T)ParamPtr, fe);
      break;

    case MXL_DEV_GPO_PINS_CFG:
      status = MxL101SF_API_ConfigGPOPins((PMXL_DEV_GPO_CFG_T)ParamPtr, fe);
      break;

    case MXL_DEV_CABLE_CFG:
      status = MxL101SF_API_ConfigCableSettings(fe);
      break;

    default:
      break;
  }

  return status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare_API_GetDeviceStatus
--| 
--| AUTHOR        : Mahendra Kondur
--|
--| DATE CREATED  : 08/08/2009
--|
--| DESCRIPTION   : The general device inquiries shall be handled 
--|                 through this API
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxLWare_API_GetDeviceStatus(MXL_CMD_TYPE_E CmdType, void *ParamPtr, struct aml_fe_dev *fe)
{
  MXL_STATUS status = MXL_TRUE;

  switch (CmdType)
  {
    case MXL_DEV_ID_VERSION_REQ:
      status = MxL101SF_API_GetChipInfo((PMXL_DEV_INFO_T)ParamPtr, fe);
      break;

    default:
      break;
  }

  return status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare_API_GetDemodStatus
--| 
--| AUTHOR        : Mahendra Kondur
--| 
--| DATE CREATED  : 08/08/2009
--|
--| DESCRIPTION   : The demod specific inquiries shall be handled 
--|                 through this API
--|                 - Support for MXL_DEMOD_TPS_HIERARCHICAL_ALPHA_REQ
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxLWare_API_GetDemodStatus(MXL_CMD_TYPE_E CmdType, void *ParamPtr, struct aml_fe_dev *fe)
{
  MXL_STATUS status = MXL_TRUE;

  switch (CmdType)
  {
    case MXL_DEMOD_SNR_REQ:
      status = MxL101SF_API_DemodGetSNR((PMXL_DEMOD_SNR_INFO_T)ParamPtr, fe);
      break;

    case MXL_DEMOD_BER_REQ:
      status = MxL101SF_API_DemodGetBER((PMXL_DEMOD_BER_INFO_T)ParamPtr, fe);
      break;

    case MXL_DEMOD_TPS_CODE_RATE_REQ:
      status = MxL101SF_API_DemodGetTpsCodeRate((PMXL_DEMOD_TPS_INFO_T)ParamPtr, fe);
      break;

    case MXL_DEMOD_TPS_HIERARCHY_REQ:
      status = MxL101SF_API_DemodGetTpsHierarchy((PMXL_DEMOD_TPS_INFO_T)ParamPtr, fe);
      break;
      
    case MXL_DEMOD_TPS_CONSTELLATION_REQ:
      status = MxL101SF_API_DemodGetTpsConstellation((PMXL_DEMOD_TPS_INFO_T)ParamPtr, fe);
      break;

    case MXL_DEMOD_TPS_FFT_MODE_REQ:
      status = MxL101SF_API_DemodGetTpsFftMode((PMXL_DEMOD_TPS_INFO_T)ParamPtr, fe);
      break;
  
    case MXL_DEMOD_TPS_HIERARCHICAL_ALPHA_REQ:
      status = MxL101SF_API_DemodGetHierarchicalAlphaValue((PMXL_DEMOD_TPS_INFO_T)ParamPtr, fe);
      break;

    case MXL_DEMOD_TPS_GUARD_INTERVAL_REQ:
      status = MxL101SF_API_DemodGetTpsGuardInterval((PMXL_DEMOD_TPS_INFO_T)ParamPtr, fe);
      break;

    case MXL_DEMOD_TPS_LOCK_REQ:
      status = MxL101SF_API_DemodGetTpsLock((PMXL_DEMOD_LOCK_STATUS_T)ParamPtr, fe);
      break;

    case MXL_DEMOD_TPS_CELL_ID_REQ:
      status = MxL101SF_API_DemodGetCellId((PMXL_DEMOD_CELL_ID_INFO_T)ParamPtr, fe);
      break;

    case MXL_DEMOD_PACKET_ERROR_COUNT_REQ:
      status = MxL101SF_API_DemodGetPacketErrorCount((PMXL_DEMOD_PEC_INFO_T)ParamPtr, fe);
      break;

    case MXL_DEMOD_SYNC_LOCK_REQ:
      status = MxL101SF_API_DemodGetSyncLockStatus((PMXL_DEMOD_LOCK_STATUS_T)ParamPtr, fe);
      break;
      
    case MXL_DEMOD_RS_LOCK_REQ:
      status = MxL101SF_API_DemodGetRsLockStatus((PMXL_DEMOD_LOCK_STATUS_T)ParamPtr, fe);
      break;

    case MXL_DEMOD_CP_LOCK_REQ:
      status = MxL101SF_API_DemodGetCpLockStatus((PMXL_DEMOD_LOCK_STATUS_T)ParamPtr, fe);
      break;

    default:
      break;
  }

  return status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare_API_ConfigDemod
--| 
--| AUTHOR        : Mahendra Kondur
--|
--| DATE CREATED  : 08/08/2009
--|                 03/12/2010
--|                 04/20/2010
--|
--| DESCRIPTION   : The demod block specific configuration shall be handled 
--|                 through this API
--|                 - Support for MXL_DEMOD_STREAM_PRIORITY_CFG
--|                 - Support for MXL_DEMOD_SPECTRUM_CFG
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxLWare_API_ConfigDemod(MXL_CMD_TYPE_E CmdType, void *ParamPtr, struct aml_fe_dev *fe)
{
  MXL_STATUS status = MXL_TRUE;

  switch (CmdType)
  {
    case MXL_DEMOD_RESET_IRQ_CFG:
      status = MxL101SF_API_DemodResetIrqStatus(fe);
      break;

    case MXL_DEMOD_RESET_PEC_CFG:  
      status = MxL101SF_API_DemodResetPacketErrorCount(fe);
      break;

    case MXL_DEMOD_TS_PRIORITY_CFG:
      status = MxL101SF_API_ConfigStreamPriority((PMXL_DEMOD_TS_PRIORITY_CFG_T)ParamPtr, fe);
      break;

    case MXL_DEMOD_SPECTRUM_CFG:
      status = MxL101SF_API_ConfigSpectrum((PMXL_DEMOD_SPECTRUM_CFG_T)ParamPtr, fe);
      break;

    default:
      break;
  }

  return status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare_API_ConfigTuner
--| 
--| AUTHOR        : Mahendra Kondur
--|
--| DATE CREATED  : 08/08/2009
--|                 02/22/2010
--|
--| DESCRIPTION   : The tuner block specific configuration shall be handled 
--|                 through this API
--|                 Support for MXL_TUNER_CHAN_SCAN_CFG
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxLWare_API_ConfigTuner(MXL_CMD_TYPE_E CmdType, void *ParamPtr, struct aml_fe_dev *fe)
{
  MXL_STATUS status = MXL_TRUE;
  
  switch (CmdType)
  {
    case MXL_TUNER_TOP_MASTER_CFG:
      status = MxL101SF_API_TopMasterControl((PMXL_TOP_MASTER_CFG_T)ParamPtr, fe);
      break;

    case MXL_TUNER_CHAN_TUNE_CFG:
      status = MxL101SF_API_TuneRF((PMXL_RF_TUNE_CFG_T)ParamPtr, fe);
      break;

    case MXL_TUNER_IF_OUTPUT_FREQ_CFG: 
      status = MxL101SF_API_TunerSetIFOutputFreq((PMXL_TUNER_IF_FREQ_T)ParamPtr, fe);
      break;

    case MXL_TUNER_CHAN_SCAN_CFG:
      status = MxL101SF_API_ConfigChannelScan((PMXL_TUNER_CHAN_SCAN_CFG_T)ParamPtr, fe);
      break;

    default:
      break;
  }
    
  return status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare_API_GetTunerStatus
--| 
--| AUTHOR        : Mahendra Kondur
--|
--| DATE CREATED  : 08/08/2009
--|                 02/22/2010
--|
--| DESCRIPTION   : The tuner specific inquiries shall be handled 
--|                 through this API
--|                 Support for MXL_TUNER_CHAN_OFFSET_REQ &
--|                 MXL_TUNER_CHECK_CHAN_REQ
--|
--| RETURN VALUE  : MXL_STATUS
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxLWare_API_GetTunerStatus(MXL_CMD_TYPE_E CmdType, void *ParamPtr, struct aml_fe_dev *fe)
{
  MXL_STATUS status = MXL_TRUE;

  switch (CmdType)
  {
    case MXL_TUNER_LOCK_STATUS_REQ:
      status = MxL101SF_API_TunerGetLockStatus((PMXL_TUNER_LOCK_STATUS_T)ParamPtr, fe);
      break;

    case MXL_TUNER_SIGNAL_STRENGTH_REQ:
      status = MxL101SF_API_GetTunerSignalStrength((PMXL_SIGNAL_STATS_T)ParamPtr, fe);
      break;

    case MXL_TUNER_CHAN_OFFSET_REQ:
      status = MxL101SF_API_GetChannelOffset((PMXL_TUNER_CHAN_OFFSET_T)ParamPtr, fe);
      break;      

   case MXL_TUNER_CHECK_CHAN_STATUS_REQ:
      status = MxL101SF_API_CheckChannel((PMXL_TUNER_CHECK_CHAN_REQ_T)ParamPtr, fe);
      break;

   default:
      break;
  }

  return status;
}
