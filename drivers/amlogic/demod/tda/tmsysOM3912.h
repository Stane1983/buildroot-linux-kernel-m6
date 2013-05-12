/**
  Copyright (C) 2010 NXP B.V., All Rights Reserved.
  This source code and any compilation or derivative thereof is the proprietary
  information of NXP B.V. and is confidential in nature. Under no circumstances
  is this software to be  exposed to or placed under an Open Source License of
  any type without the expressed written permission of NXP B.V.
 *
 * \file          tmsysOM3912.h
 *                %version: CFR_FEAP#2 %
 *
 * \date          %date_modified%
 *
 * \brief         Describe briefly the purpose of this file.
 *
 * REFERENCE DOCUMENTS :
 *
 * Detailed description may be added here.
 *
 * \section info Change Information
 *
 * \verbatim
   Date          Modified by CRPRNr  TASKNr  Maintenance description
   -------------|-----------|-------|-------|-----------------------------------
   -------------|-----------|-------|-------|-----------------------------------
   -------------|-----------|-------|-------|-----------------------------------
   \endverbatim
 *
*/


#ifndef TMSYSOM3912_H
#define TMSYSOM3912_H

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*/
/*                       INCLUDE FILES                                        */
/*============================================================================*/

/*============================================================================*/
/*                       MACRO DEFINITION                                     */
/*============================================================================*/

/* SW Error codes */
#define OM3912_ERR_BASE               (CID_COMP_TUNER | CID_LAYER_DEVLIB)
#define OM3912_ERR_COMP               (CID_COMP_TUNER | CID_LAYER_DEVLIB | TM_ERR_COMP_UNIQUE_START)

#define OM3912_ERR_BAD_UNIT_NUMBER    (OM3912_ERR_BASE + TM_ERR_BAD_UNIT_NUMBER)
#define OM3912_ERR_NOT_INITIALIZED    (OM3912_ERR_BASE + TM_ERR_NOT_INITIALIZED)
//#define OM3912_ERR_INIT_FAILED        (OM3912_ERR_BASE + TM_ERR_INIT_FAILED)
#define OM3912_ERR_BAD_PARAMETER      (OM3912_ERR_BASE + TM_ERR_BAD_PARAMETER)
#define OM3912_ERR_NOT_SUPPORTED      (OM3912_ERR_BASE + TM_ERR_NOT_SUPPORTED)
#define OM3912_ERR_HW                 (OM3912_ERR_COMP + 0x0001)
//#define OM3912_ERR_NOT_READY          (OM3912_ERR_COMP + 0x0002)
//#define OM3912_ERR_BAD_CRC            (OM3912_ERR_COMP + 0x0003)
//#define OM3912_ERR_BAD_VERSION        (OM3912_ERR_COMP + 0x0004)

/*============================================================================*/
/*                       ENUM OR TYPE DEFINITION                              */
/*============================================================================*/

/* OM3912 Device Types */
typedef enum _tmOM3912UnitDeviceType_t
{
    tmOM3912UnitDeviceTypeUnknown = 0,
    /** Tuner */
    tmOM3912UnitDeviceTypeTuner,
    tmOM3912UnitDeviceTypeMax
} tmOM3912UnitDeviceType_t, *ptmOM3912UnitDeviceType_t;

#ifdef TMFL_I2C_COMMUNICATION_MANAGED_OUTSIDE_OM

/** I2C hardware function */
typedef tmErrorCode_t   (*w_I2C)(tmUnitSelect_t tUnit, UInt32 AddrSize, UInt8* pAddr, UInt32 Len, UInt8* pData);

#endif

/*============================================================================*/
/*                       EXTERN DATA DEFINITION                               */
/*============================================================================*/



/*============================================================================*/
/*                       EXTERN FUNCTION PROTOTYPES                           */
/*============================================================================*/


extern tmErrorCode_t
tmsysOM3912GetSWVersion
(
    ptmsysSWVersion_t   pSWVersion  /* O: Receives SW Version  */
);

extern tmErrorCode_t
tmsysOM3912Init
(
    tmUnitSelect_t              tUnit,      /* I: FrontEnd unit number */
    tmbslFrontEndDependency_t*  psSrvFunc   /*  I: setup parameters */
);

extern tmErrorCode_t
tmsysOM3912DeInit
(
    tmUnitSelect_t  tUnit   /* I: FrontEnd unit number */
);

extern tmErrorCode_t
tmsysOM3912Reset
(
    tmUnitSelect_t  tUnit   /* I: FrontEnd unit number */
);

extern tmErrorCode_t
tmsysOM3912SetPowerState
(
    tmUnitSelect_t  tUnit,      /* I: FrontEnd unit number */
    tmPowerState_t  ePowerState /* I: Power state of the device */
);

extern tmErrorCode_t
tmsysOM3912GetPowerState
(
    tmUnitSelect_t  tUnit,      /* I: FrontEnd unit number */
    ptmPowerState_t pPowerState /* O: Power state of the device */
);

extern tmErrorCode_t
tmsysOM3912SendRequest
(
    tmUnitSelect_t  tUnit,              /* I: FrontEnd unit number */
    pVoid           pTuneRequest,       /* I/O: Tune Request Structure pointer */
    UInt32          dwTuneRequestSize,  /* I: Tune Request Structure size */
    tmTuneReqType_t tTuneReqType        /* I: Tune Request Type */
);

extern tmErrorCode_t
tmsysOM3912SetI2CSwitchState
(
    tmUnitSelect_t                  tUnit,  /* I: FrontEnd unit number */
    tmsysFrontEndI2CSwitchState_t   eState  /* I: I2C switch state */
);

extern tmErrorCode_t
tmsysOM3912GetI2CSwitchState
(
    tmUnitSelect_t                  tUnit,
    tmsysFrontEndI2CSwitchState_t*  peState,
    UInt32*                         puI2CSwitchCounter
);

extern tmErrorCode_t
tmsysOM3912GetLockStatus
(
    tmUnitSelect_t          tUnit,      /* I: FrontEnd unit number */
    tmsysFrontEndState_t*   pLockStatus /* O: Lock status */
);

extern tmErrorCode_t
tmsysOM3912GetSignalStrength
(
    tmUnitSelect_t  tUnit,      /* I: FrontEnd unit number */
    UInt32          *pStrength  /* I/O: Signal Strength pointer */
);

extern tmErrorCode_t
tmsysOM3912GetSignalQuality
(
    tmUnitSelect_t  tUnit,      /* I: FrontEnd unit number */
    UInt32          *pQuality   /* I/O: Signal Quality pointer */
);

extern tmErrorCode_t
tmsysOM3912GetDeviceUnit
(
    tmUnitSelect_t              tUnit,      /* I: FrontEnd unit number */
    tmOM3912UnitDeviceType_t    deviceType, /* I: Device Type  */
    ptmUnitSelect_t             ptUnit      /* O: Device unit number */
);

extern tmErrorCode_t
tmsysOM3912SetHwAddress
(
    tmUnitSelect_t              tUnit,      /* I: FrontEnd unit number */
    tmOM3912UnitDeviceType_t    deviceType, /* I: Device Type  */
    UInt32                      uHwAddress  /* I: Hardware Address */
);

extern tmErrorCode_t
tmsysOM3912GetHwAddress
(
    tmUnitSelect_t              tUnit,      /* I: FrontEnd unit number */
    tmOM3912UnitDeviceType_t deviceType, /* I: Device Type  */
    UInt32*                     puHwAddress /* O: Hardware Address */
);

extern tmErrorCode_t tmsysOM3912I2CRead(tmUnitSelect_t tUnit, UInt32 AddrSize, UInt8* pAddr, UInt32 ReadLen, UInt8* pData);

extern tmErrorCode_t tmsysOM3912I2CWrite(tmUnitSelect_t tUnit, UInt32 AddrSize, UInt8* pAddr, UInt32 WriteLen, UInt8* pData);

extern tmErrorCode_t tmsysOM3912Wait(tmUnitSelect_t tUnit, UInt32 tms);

extern tmErrorCode_t tmsysOM3912Print(UInt32 level, const char* format, ...);


#ifdef __cplusplus
}
#endif

#endif /* TMSYSOM3912_H */
/*============================================================================*/
/*                            END OF FILE                                     */
/*============================================================================*/

