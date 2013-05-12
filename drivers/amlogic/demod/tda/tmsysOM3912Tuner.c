/**
Copyright (C) 2011 NXP B.V., All Rights Reserved.
This source code and any compilation or derivative thereof is the proprietary
information of NXP B.V. and is confidential in nature. Under no circumstances
is this software to be  exposed to or placed under an Open Source License of
any type without the expressed written permission of NXP B.V.
*
* \file          tmsysOM3912Tuner.c
*                %version: CFR_FEAP#6 %
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


/*============================================================================*/
/*                   STANDARD INCLUDE FILES                                   */
/*============================================================================*/


/*============================================================================*/
/*                   PROJECT INCLUDE FILES                                    */
/*============================================================================*/
#include "tmNxTypes.h"
#include "tmCompId.h"
#include "tmFrontEnd.h"
#include "tmbslFrontEndTypes.h"
#include "tmsysFrontEndTypes.h"
#include "tmUnitParams.h"

#include "tmbslTDA18273.h"

#include "tmsysOM3912.h"
#include "tmsysOM3912local.h"
#include "tmsysOM3912Tuner.h"

/*============================================================================*/
/*                   MACRO DEFINITION                                         */
/*============================================================================*/


/*============================================================================*/
/*                   TYPE DEFINITION                                          */
/*============================================================================*/



/*============================================================================*/
/*                   PUBLIC VARIABLES DEFINITION                              */
/*============================================================================*/

/*============================================================================*/
/*                   STATIC VARIABLES DECLARATIONS                            */
/*============================================================================*/

/*============================================================================*/
/*                       EXTERN FUNCTION PROTOTYPES                           */
/*============================================================================*/


/*============================================================================*/
/*                   STATIC FUNCTIONS DECLARATIONS                            */
/*============================================================================*/

/*============================================================================*/
/*                   PUBLIC FUNCTIONS DEFINITIONS                             */
/*============================================================================*/



/*============================================================================*/
/*                   PROJECT INCLUDE FILES                                    */
/*============================================================================*/


/*============================================================================*/
/*                   TYPE DEFINITION                                          */
/*============================================================================*/


/*============================================================================*/
/*                   STATIC VARIABLES DECLARATIONS                            */
/*============================================================================*/


/*============================================================================*/
/*                   PUBLIC FUNCTIONS DECLARATIONS                            */
/*============================================================================*/



/*============================================================================*/
/*                   STATIC FUNCTIONS DECLARATIONS                            */
/*============================================================================*/

/*============================================================================*/
/*                   PUBLIC FUNCTIONS DEFINITIONS                             */
/*============================================================================*/

tmErrorCode_t
tmsysOM3912GetTunerSWVersion
(
    ptmbslSWVersion_t pswTunerVersion  /* O: Receives Tuner SW Version  */
)
{
    tmErrorCode_t err = TM_OK;
    static char   TDA18273Name[] = "tmbslTDA18273\0";

    pswTunerVersion->pName = (char *)TDA18273Name;
    pswTunerVersion->nameBufSize = sizeof(TDA18273Name);
    err = tmbslTDA18273GetSWVersion(&pswTunerVersion->swVersion);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmbslTDA18273GetSWVersion failed."));

    return err;
}

tmErrorCode_t
tmsysOM3912TunerOpen(
    tmUnitSelect_t              tUnit,      /*  I: Unit number */
    ptmbslFrontEndDependency_t  psSrvFunc   /*  I: setup parameters */
)
{
    tmErrorCode_t err = TM_OK;

    err = tmbslTDA18273Init(tUnit, psSrvFunc);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmbslTDA18273Init(0x%08X) failed.", tUnit));

    return err;
}

tmErrorCode_t
tmsysOM3912TunerClose
(
    tmUnitSelect_t tUnit   /*  I: Unit number */
)
{
    tmErrorCode_t err = TM_OK;

    err = tmbslTDA18273DeInit(tUnit);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmbslTDA18273DeInit(0x%08X) failed.", tUnit));

    return err;
}

tmErrorCode_t
tmsysOM3912TunerReset
(
    tmUnitSelect_t tUnit   /*  I: Unit number */
)
{
    tmErrorCode_t err = TM_OK;

    /* Determine if we need to wait in Reset function */
    err = tmbslTDA18273SetIRQWait(tUnit, True);

    if(err == TM_OK)
    {
        err = tmbslTDA18273SetPowerState(tUnit, tmTDA18273_PowerNormalMode);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmbslTDA18273SetPowerState(0x%08X, tmPowerOn) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = tmbslTDA18273Reset(tUnit);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmbslTDA18273Reset(0x%08X) failed.", tUnit));
    }
    if (err == TM_OK)
    {
        err = tmbslTDA18273SetPowerState(tUnit, tmTDA18273_PowerStandbyWithXtalOn);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmbslTDA18273SetPowerState(0x%08X, tmPowerStandby) failed.", tUnit));
    }

    return err;
}

tmErrorCode_t
tmsysOM3912TunerSetPowerState(
    tmUnitSelect_t  tUnit,       /* I: Unit number */
    tmPowerState_t  ePowerState  /* I: Power state of tuner */
)
{
    tmErrorCode_t err = TM_OK;

    switch (ePowerState)
    {
        case tmPowerOn:
            /* Set TDA18273 power state to Normal Mode */
            err = tmbslTDA18273SetPowerState(tUnit, tmTDA18273_PowerNormalMode);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmbslTDA18273SetPowerState(0x%08X, tmPowerOn) failed.", tUnit));
        break;

        case tmPowerStandby:
            /* Set TDA18273 power state to standby with Xtal ON */
            err = tmbslTDA18273SetPowerState(tUnit, tmTDA18273_PowerStandbyWithXtalOn);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmbslTDA18273SetPowerState(0x%08X, tmPowerStandby) failed.", tUnit));
            break;

        case tmPowerSuspend:
        case tmPowerOff:
        default:
            err = OM3912_ERR_NOT_SUPPORTED;
            break;
    }

    return err;
}

tmErrorCode_t
tmsysOM3912TunerGetLockStatus(
    tmUnitSelect_t        tUnit,      /* I: Unit number */
    ptmbslFrontEndState_t pLockStatus /* O: PLL Lock status */
)
{
    tmErrorCode_t err = TM_OK;

    /* Get TDA18273 PLL Lock status */
    err = tmbslTDA18273GetLockStatus(tUnit, pLockStatus);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmbslTDA18273GetLockStatus(0x%08X) failed.", tUnit));

    return err;
}

tmErrorCode_t
tmsysOM3912TunerSetStandardMode(
    tmUnitSelect_t              tUnit,          /*  I: Unit number */
    tmbslFrontEndStandardMode_t StandardMode    /*  I: Standard mode of this device */
)
{
    tmErrorCode_t          err = TM_OK;
    TDA18273StandardMode_t tunerStandardMode;

    switch (StandardMode)
    {
        case tmbslFrontEndStandardModeUnknown:
            tunerStandardMode = TDA18273_StandardMode_Unknown;
            break;
        case tmbslFrontEndStandardQAM6MHz:
            tunerStandardMode = TDA18273_QAM_6MHz;
            break;
        case tmbslFrontEndStandardQAM8MHz:
            tunerStandardMode = TDA18273_QAM_8MHz;
            break;
        case tmbslFrontEndStandardATSC6MHz:
            tunerStandardMode = TDA18273_ATSC_6MHz;
            break;
        case tmbslFrontEndStandardISDBT6MHz:
            tunerStandardMode = TDA18273_ISDBT_6MHz;
            break;
        case tmbslFrontEndStandardDVBT1_7MHz:
            tunerStandardMode = TDA18273_DVBT_1_7MHz;
            break;
        case tmbslFrontEndStandardDVBT6MHz:
            tunerStandardMode = TDA18273_DVBT_6MHz;
            break;
        case tmbslFrontEndStandardDVBT7MHz:
            tunerStandardMode = TDA18273_DVBT_7MHz;
            break;
        case tmbslFrontEndStandardDVBT8MHz:
            tunerStandardMode = TDA18273_DVBT_8MHz;
            break;
        case tmbslFrontEndStandardDVBT10MHz:
            tunerStandardMode = TDA18273_DVBT_10MHz;
            break;
        case tmbslFrontEndStandardDMBT8MHz:
            tunerStandardMode = TDA18273_DMBT_8MHz;
            break;
        case tmbslFrontEndStandardFMRadio:
            tunerStandardMode = TDA18273_FM_Radio;
            break;
        case tmbslFrontEndStandardANLGMN:
            tunerStandardMode = TDA18273_ANLG_MN;
            break;
        case tmbslFrontEndStandardANLGB:
            tunerStandardMode = TDA18273_ANLG_B;
            break;
        case tmbslFrontEndStandardANLGGH:
            tunerStandardMode = TDA18273_ANLG_GH;
            break;
        case tmbslFrontEndStandardANLGI:
            tunerStandardMode = TDA18273_ANLG_I;
            break;
        case tmbslFrontEndStandardANLGDK:
            tunerStandardMode = TDA18273_ANLG_DK;
            break;
        case tmbslFrontEndStandardANLGL:
            tunerStandardMode = TDA18273_ANLG_L;
            break;
        case tmbslFrontEndStandardANLGLL:
            tunerStandardMode = TDA18273_ANLG_LL;
            break;
        case tmbslFrontEndStandardScanning:
            tunerStandardMode = TDA18273_Scanning;
            break;
        case tmbslFrontEndStandardScanXpress:
            tunerStandardMode = TDA18273_ScanXpress;
            break;
        default:
            tunerStandardMode = TDA18273_StandardMode_Max;
            break;
    }

    /* Set Tuner Standard mode */
    err = tmbslTDA18273SetStandardMode(tUnit, tunerStandardMode);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmbslTDA18273SetStandardMode(0x%08X) failed.", tUnit));

    return err;
}

tmErrorCode_t
tmsysOM3912TunerSetRF(
    tmUnitSelect_t  tUnit,  /*  I: Unit number */
    UInt32          uRF     /*  I: RF frequency in hertz */
)
{
    tmErrorCode_t err = TM_OK;

    /* Set Tuner RF */
    err = tmbslTDA18273SetRf(tUnit, uRF);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmbslTDA18273SetRf(0x%08X) failed.", tUnit));

    return err;
}


