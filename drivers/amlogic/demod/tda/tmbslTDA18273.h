/*
  Copyright (C) 2010 NXP B.V., All Rights Reserved.
  This source code and any compilation or derivative thereof is the proprietary
  information of NXP B.V. and is confidential in nature. Under no circumstances
  is this software to be  exposed to or placed under an Open Source License of
  any type without the expressed written permission of NXP B.V.
 *
 * \file          tmbslTDA18273.h
 *
 *                %version: CFR_FEAP#21 %
 *
 * \date          %modify_time%
 *
 * \author        David LEGENDRE
 *                Michael VANNIER
 *                Christophe CAZETTES
 *
 * \brief         Describe briefly the purpose of this file.
 *
 * REFERENCE DOCUMENTS :
 *                TDA18273_Driver_User_Guide.pdf
 *
 * TVFE SW Arch V4 Template: Author Christophe CAZETTES
 *
 * \section info Change Information
 *
*/

#ifndef _TMBSL_TDA18273_H
#define _TMBSL_TDA18273_H

#ifdef __cplusplus
extern "C"
{
#endif

/*============================================================================*/
/* TDA18273 Error Codes                                                       */
/*============================================================================*/

#define TDA18273_ERR_BASE                       (CID_COMP_TUNER | CID_LAYER_BSL)
#define TDA18273_ERR_COMP                       (CID_COMP_TUNER | CID_LAYER_BSL | TM_ERR_COMP_UNIQUE_START)

#define TDA18273_ERR_BAD_UNIT_NUMBER            (TDA18273_ERR_BASE + TM_ERR_BAD_UNIT_NUMBER)
#define TDA18273_ERR_ERR_NO_INSTANCES           (TDA18273_ERR_BASE + TM_ERR_NO_INSTANCES)
#define TDA18273_ERR_NOT_INITIALIZED            (TDA18273_ERR_BASE + TM_ERR_NOT_INITIALIZED)
#define TDA18273_ERR_ALREADY_SETUP              (TDA18273_ERR_BASE + TM_ERR_ALREADY_SETUP)
#define TDA18273_ERR_INIT_FAILED                (TDA18273_ERR_BASE + TM_ERR_INIT_FAILED)
#define TDA18273_ERR_BAD_PARAMETER              (TDA18273_ERR_BASE + TM_ERR_BAD_PARAMETER)
#define TDA18273_ERR_NOT_SUPPORTED              (TDA18273_ERR_BASE + TM_ERR_NOT_SUPPORTED)
#define TDA18273_ERR_NULL_CONTROLFUNC           (TDA18273_ERR_BASE + TM_ERR_NULL_CONTROLFUNC)
#define TDA18273_ERR_HW_FAILED                  (TDA18273_ERR_COMP + 0x0001)
#define TDA18273_ERR_NOT_READY                  (TDA18273_ERR_COMP + 0x0002)
#define TDA18273_ERR_BAD_VERSION                (TDA18273_ERR_COMP + 0x0003)
#define TDA18273_ERR_STD_NOT_SET                (TDA18273_ERR_COMP + 0x0004)
#define TDA18273_ERR_RF_NOT_SET                 (TDA18273_ERR_COMP + 0x0005)

/*============================================================================*/
/* Types and defines:                                                         */
/*============================================================================*/

typedef enum _TDA18273PowerState_t {
    TDA18273_PowerNormalMode = 0,                                 /* Device normal mode */
    TDA18273_PowerStandbyWithXtalOn,                              /* Device standby mode with Xtal Output */
    TDA18273_PowerStandby,                                        /* Device standby mode */
    TDA18273_PowerMax
} TDA18273PowerState_t, *pTDA18273PowerState_t;

typedef enum _TDA18273StandardMode_t {
    TDA18273_StandardMode_Unknown = 0,                  /* Unknown standard */
    TDA18273_QAM_6MHz,                                  /* Digital TV QAM 6MHz */
    TDA18273_QAM_8MHz,                                  /* Digital TV QAM 8MHz */
    TDA18273_ATSC_6MHz,                                 /* Digital TV ATSC 6MHz */
    TDA18273_ISDBT_6MHz,                                /* Digital TV ISDBT 6MHz */
    TDA18273_DVBT_1_7MHz,                               /* Digital TV DVB-T/T2 6MHz */
    TDA18273_DVBT_6MHz,                                 /* Digital TV DVB-T/T2 6MHz */
    TDA18273_DVBT_7MHz,                                 /* Digital TV DVB-T/T2 7MHz */
    TDA18273_DVBT_8MHz,                                 /* Digital TV DVB-T/T2 8MHz */
    TDA18273_DVBT_10MHz,                                /* Digital TV DVB-T/T2 10MHz */     
    TDA18273_DMBT_8MHz,                                 /* Digital TV DMB-T 8MHz */
    TDA18273_FM_Radio,                                  /* Analog FM Radio */
    TDA18273_ANLG_MN,                                   /* Analog TV M/N */
    TDA18273_ANLG_B,                                    /* Analog TV B */
    TDA18273_ANLG_GH,                                   /* Analog TV G/H */
    TDA18273_ANLG_I,                                    /* Analog TV I */
    TDA18273_ANLG_DK,                                   /* Analog TV D/K */
    TDA18273_ANLG_L,                                    /* Analog TV L */
    TDA18273_ANLG_LL,                                   /* Analog TV L' */
    TDA18273_Scanning,                                  /* Analog Preset Blind Scanning */
    TDA18273_ScanXpress,                                /* ScanXpress */
    TDA18273_StandardMode_Max
} TDA18273StandardMode_t, *pTDA18273StandardMode_t;

#define isTDA18273_DGTL_STD(_CURSTD) ( (((_CURSTD)>=TDA18273_QAM_6MHz) && ((_CURSTD)<=TDA18273_DMBT_8MHz)) || ((_CURSTD)==TDA18273_ScanXpress) )
#define isTDA18273_ANLG_STD(_CURSTD) ( ((_CURSTD)>=TDA18273_FM_Radio) && ((_CURSTD)<=TDA18273_Scanning) )

/* Register Bit-Field Definition */
typedef struct _TDA18273_BitField_t
{
    UInt8   Address;
    UInt8   PositionInBits;
    UInt8   WidthInBits;
    UInt8   Attributes;
}
TDA18273_BitField_t, *pTDA18273_BitField_t;

/*============================================================================*/
/* Exported functions:                                                        */
/*============================================================================*/

tmErrorCode_t
tmbslTDA18273_Open(
    tmUnitSelect_t              tUnit,      /*  I: Unit number */
    tmbslFrontEndDependency_t*  psSrvFunc   /*  I: setup parameters */
);

tmErrorCode_t
tmbslTDA18273_Close(
    tmUnitSelect_t  tUnit   /*  I: Unit number */
);

tmErrorCode_t
tmbslTDA18273_GetSWVersion(
    ptmSWVersion_t  pSWVersion  /*  I: Receives SW Version */
);

tmErrorCode_t
tmbslTDA18273_GetSWSettingsVersion(
    ptmSWSettingsVersion_t pSWSettingsVersion   /* O: Receives SW Settings Version */
);

tmErrorCode_t
tmbslTDA18273_CheckHWVersion(
    tmUnitSelect_t tUnit    /* I: Unit number */
);

tmErrorCode_t
tmbslTDA18273_SetPowerState(
    tmUnitSelect_t  tUnit,      /* I: Unit number */
    tmPowerState_t  powerState  /* I: Power state */
);

tmErrorCode_t
tmbslTDA18273_GetPowerState(
    tmUnitSelect_t  tUnit,      /* I: Unit number */
    tmPowerState_t* pPowerState /* O: Power state */
);

tmErrorCode_t
tmbslTDA18273_SetLLPowerState(
    tmUnitSelect_t          tUnit,      /* I: Unit number */
    TDA18273PowerState_t  powerState  /* I: Power state of TDA18273 */
);

tmErrorCode_t
tmbslTDA18273_GetLLPowerState(
    tmUnitSelect_t          tUnit,      /* I: Unit number */
    TDA18273PowerState_t* pPowerState /* O: Power state of TDA18273 */
);

tmErrorCode_t
tmbslTDA18273_SetStandardMode(
    tmUnitSelect_t              tUnit,          /*  I: Unit number */
    TDA18273StandardMode_t    StandardMode    /*  I: Standard mode of this device */
);

tmErrorCode_t
tmbslTDA18273_GetStandardMode(
    tmUnitSelect_t              tUnit,          /*  I: Unit number */
    TDA18273StandardMode_t    *pStandardMode  /*  O: Standard mode of this device */
);

tmErrorCode_t
tmbslTDA18273_SetRF(
    tmUnitSelect_t  tUnit,  /*  I: Unit number */
    UInt32          uRF     /*  I: RF frequency in hertz */
);

tmErrorCode_t
tmbslTDA18273_GetRF(
    tmUnitSelect_t  tUnit,  /*  I: Unit number */
    UInt32*         pRF     /*  O: RF frequency in hertz */
);

tmErrorCode_t
tmbslTDA18273_HwInit(
    tmUnitSelect_t tUnit    /* I: Unit number */
);

tmErrorCode_t
tmbslTDA18273_GetIF(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt32*         puIF    /* O: IF Frequency in hertz */
);

tmErrorCode_t
tmbslTDA18273_SetIF(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt32          uIF     /* I: IF Frequency in hertz */
);

tmErrorCode_t
tmbslTDA18273_GetCF_Offset(
    tmUnitSelect_t  tUnit,      /* I: Unit number */
    UInt32*         puOffset    /* O: Center frequency offset in hertz */
);

tmErrorCode_t
tmbslTDA18273_GetLockStatus(
    tmUnitSelect_t          tUnit,      /* I: Unit number */
    tmbslFrontEndState_t*   pLockStatus /* O: PLL Lock status */
);

tmErrorCode_t
tmbslTDA18273_GetPowerLevel(
    tmUnitSelect_t  tUnit,      /* I: Unit number */
    UInt8*          pPowerLevel /* O: Power Level in 1/2 steps dBµV */
);

tmErrorCode_t
tmbslTDA18273_SetInternalVsync(
    tmUnitSelect_t  tUnit,      /* I: Unit number */
    Bool            bEnabled    /* I: Enable of disable the internal VSYNC */
);

tmErrorCode_t
tmbslTDA18273_GetInternalVsync(
    tmUnitSelect_t  tUnit,      /* I: Unit number */
    Bool*           pbEnabled   /* O: current status of the internal VSYNC */
);

tmErrorCode_t
tmbslTDA18273_SetIRQWait(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    Bool            bWait   /* I: Determine if we need to wait IRQ in driver functions */
);

tmErrorCode_t
tmbslTDA18273_GetIRQWait(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    Bool*           pbWait  /* O: Determine if we need to wait IRQ in driver functions */
);

tmErrorCode_t
tmbslTDA18273_SetPllManual(
    tmUnitSelect_t  tUnit,         /* I: Unit number */
    Bool            bOverridePLL   /* I: Determine if we need to put PLL in manual mode in SetRF */
);

tmErrorCode_t
tmbslTDA18273_GetPllManual(
    tmUnitSelect_t  tUnit,         /* I: Unit number */
    Bool*           pbOverridePLL  /* O: Determine if we need to put PLL in manual mode in SetRF */
);

tmErrorCode_t
tmbslTDA18273_SetIRQWaitHwInit(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    Bool            bWait   /* I: Determine if we need to wait IRQ in driver functions */
);

tmErrorCode_t
tmbslTDA18273_GetIRQWaitHwInit(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    Bool*           pbWait  /* O: Determine if we need to wait IRQ in driver functions */
);

tmErrorCode_t
tmbslTDA18273_GetIRQ(
    tmUnitSelect_t  tUnit  /* I: Unit number */,
    Bool*           pbIRQ  /* O: IRQ triggered */
);

tmErrorCode_t
tmbslTDA18273_WaitIRQ(
    tmUnitSelect_t  tUnit,      /* I: Unit number */
    UInt32          timeOut,    /* I: timeOut for IRQ wait */
    UInt32          waitStep,   /* I: wait step */
    UInt8           irqStatus   /* I: IRQs to wait */
);

tmErrorCode_t
tmbslTDA18273_GetXtalCal_End(
    tmUnitSelect_t  tUnit           /* I: Unit number */,
    Bool*           pbXtalCal_End   /* O: XtalCal_End triggered */
);

/* You can only add one step (-1 or +1) at a time along -125 KHz <-> -62.5 KHz <-> 0 <-> 62.5 KHz <-> 125 KHz */
/* After calling tmbslTDA18273_SetRF, step is reset at 0 */
tmErrorCode_t
tmbslTDA18273_SetFineRF(
    tmUnitSelect_t tUnit,      /* I: Unit number */
    Int8           step        /* I: step (-1, +1) */
);

tmErrorCode_t
tmbslTDA18273_Write(
    tmUnitSelect_t              tUnit,      /* I: Unit number */
    const TDA18273_BitField_t*  pBitField,  /* I: Bitfield structure */
    UInt8                       uData,      /* I: Data to write */
    tmbslFrontEndBusAccess_t    eBusAccess  /* I: Access to bus */
);

tmErrorCode_t
tmbslTDA18273_Read(
    tmUnitSelect_t              tUnit,      /* I: Unit number */
    const TDA18273_BitField_t*  pBitField,  /* I: Bitfield structure */
    UInt8*                      puData,     /* I: Data to read */
    tmbslFrontEndBusAccess_t    eBusAccess  /* I: Access to bus */
);


/*============================================================================*/
/* Legacy compatibility:                                                      */
/*============================================================================*/

#ifndef tmbslTDA18273Init
 #define tmbslTDA18273Init tmbslTDA18273_Open
#endif

#ifndef tmbslTDA18273DeInit
 #define tmbslTDA18273DeInit tmbslTDA18273_Close
#endif

#ifndef tmbslTDA18273GetSWVersion
 #define tmbslTDA18273GetSWVersion tmbslTDA18273_GetSWVersion
#endif

#ifndef tmbslTDA18273GetSWSettingsVersion
 #define tmbslTDA18273GetSWSettingsVersion tmbslTDA18273_GetSWSettingsVersion
#endif

#ifndef tmbslTDA18273CheckHWVersion
 #define tmbslTDA18273CheckHWVersion tmbslTDA18273_CheckHWVersion
#endif

#ifndef tmbslTDA18273SetPowerState
 #define tmbslTDA18273SetPowerState tmbslTDA18273_SetLLPowerState
#endif

#ifndef tmbslTDA18273GetPowerState
 #define tmbslTDA18273GetPowerState tmbslTDA18273_GetLLPowerState
#endif

#ifndef tmbslTDA18273SetLLPowerState
 #define tmbslTDA18273SetLLPowerState tmbslTDA18273_SetLLPowerState
#endif

#ifndef tmbslTDA18273GetLLPowerState
 #define tmbslTDA18273GetLLPowerState tmbslTDA18273_GetLLPowerState
#endif

#ifndef tmbslTDA18273SetStandardMode
 #define tmbslTDA18273SetStandardMode tmbslTDA18273_SetStandardMode
#endif

#ifndef tmbslTDA18273GetStandardMode
 #define tmbslTDA18273GetStandardMode tmbslTDA18273_GetStandardMode
#endif

#ifndef tmbslTDA18273SetRf
 #define tmbslTDA18273SetRf tmbslTDA18273_SetRF
#endif

#ifndef tmbslTDA18273GetRf
 #define tmbslTDA18273GetRf tmbslTDA18273_GetRF
#endif

#ifndef tmbslTDA18273Reset
 #define tmbslTDA18273Reset tmbslTDA18273_HwInit
#endif

#ifndef tmbslTDA18273GetIF
 #define tmbslTDA18273GetIF tmbslTDA18273_GetIF
#endif

#ifndef tmbslTDA18273GetCF_Offset
 #define tmbslTDA18273GetCF_Offset tmbslTDA18273_GetCF_Offset
#endif

#ifndef tmbslTDA18273GetLockStatus
 #define tmbslTDA18273GetLockStatus tmbslTDA18273_GetLockStatus
#endif

#ifndef tmbslTDA18273GetPowerLevel
 #define tmbslTDA18273GetPowerLevel tmbslTDA18273_GetPowerLevel
#endif

#ifndef tmbslTDA18273GetRSSI
 #define tmbslTDA18273GetRSSI tmbslTDA18273_GetRSSI
#endif

#ifndef tmbslTDA18273SetIRQWait
 #define tmbslTDA18273SetIRQWait tmbslTDA18273_SetIRQWait
#endif

#ifndef tmbslTDA18273GetIRQWait
 #define tmbslTDA18273GetIRQWait tmbslTDA18273_GetIRQWait
#endif

#ifndef tmbslTDA18273SetIRQWaitHwInit
 #define tmbslTDA18273SetIRQWaitHwInit tmbslTDA18273_SetIRQWaitHwInit
#endif

#ifndef tmbslTDA18273GetIRQWaitHwInit
 #define tmbslTDA18273GetIRQWaitHwInit tmbslTDA18273_GetIRQWaitHwInit
#endif

#ifndef tmbslTDA18273GetIRQ
 #define tmbslTDA18273GetIRQ tmbslTDA18273_GetIRQ
#endif

#ifndef tmbslTDA18273WaitIRQ
 #define tmbslTDA18273WaitIRQ tmbslTDA18273_WaitIRQ
#endif

#ifndef tmbslTDA18273GetXtalCal_End
 #define tmbslTDA18273GetXtalCal_End tmbslTDA18273_GetXtalCal_End
#endif

#ifndef tmbslTDA18273RFFineTuning
 #define tmbslTDA18273RFFineTuning tmbslTDA18273_SetFineRF
#endif

#ifndef tmbslTDA18273Write
 #define tmbslTDA18273Write tmbslTDA18273_Write
#endif

#ifndef tmbslTDA18273Read
 #define tmbslTDA18273Read tmbslTDA18273_Read
#endif

#define tmTDA18273PowerState_t TDA18273PowerState_t
#define tmTDA18273_PowerNormalMode TDA18273_PowerNormalMode
#define tmTDA18273_PowerStandbyWithXtalOn TDA18273_PowerStandbyWithXtalOn
#define tmTDA18273_PowerStandby TDA18273_PowerStandby
#define tmTDA18273_PowerMax TDA18273_PowerMax

#define tmTDA18273StandardMode_t TDA18273StandardMode_t
#define tmTDA18273_QAM_6MHz TDA18273_QAM_6MHz
#define tmTDA18273_QAM_8MHz TDA18273_QAM_8MHz
#define tmTDA18273_ATSC_6MHz TDA18273_ATSC_6MHz
#define tmTDA18273_ISDBT_6MHz TDA18273_ISDBT_6MHz
#define tmTDA18273_DVBT_1_7MHz TDA18273_DVBT_1_7MHz
#define tmTDA18273_DVBT_6MHz TDA18273_DVBT_6MHz
#define tmTDA18273_DVBT_7MHz TDA18273_DVBT_7MHz
#define tmTDA18273_DVBT_8MHz TDA18273_DVBT_8MHz
#define tmTDA18273_DVBT_10MHz TDA18273_DVBT_10MHz
#define tmTDA18273_DMBT_8MHz TDA18273_DMBT_8MHz
#define tmTDA18273_FM_Radio TDA18273_FM_Radio
#define tmTDA18273_ANLG_MN TDA18273_ANLG_MN
#define tmTDA18273_ANLG_B TDA18273_ANLG_B
#define tmTDA18273_ANLG_GH TDA18273_ANLG_GH
#define tmTDA18273_ANLG_I TDA18273_ANLG_I
#define tmTDA18273_ANLG_DK TDA18273_ANLG_DK
#define tmTDA18273_ANLG_L TDA18273_ANLG_L
#define tmTDA18273_ANLG_LL TDA18273_ANLG_LL
#define tmTDA18273_Scanning TDA18273_Scanning
#define tmTDA18273_ScanXpress TDA18273_ScanXpress
#define tmTDA18273_StandardMode_Max TDA18273_StandardMode_Max



#ifdef __cplusplus
}
#endif

#endif /* _TMBSL_TDA18273_H */

