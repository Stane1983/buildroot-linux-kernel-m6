/**
Copyright (C) 2010 NXP B.V., All Rights Reserved.
This source code and any compilation or derivative thereof is the proprietary
information of NXP B.V. and is confidential in nature. Under no circumstances
is this software to be  exposed to or placed under an Open Source License of
any type without the expressed written permission of NXP B.V.
*
* \file          tmsysOM3912_63.c
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

#include "tmsysOM3912.h"
#include "tmsysOM3912local.h"
#include "tmsysOM3912Instance.h"
#include "tmsysOM3912Tuner.h"

/*============================================================================*/
/*                   MACRO DEFINITION                                         */
/*============================================================================*/

#ifndef SIZEOF_ARRAY
#define SIZEOF_ARRAY(ar)        (sizeof(ar)/sizeof((ar)[0]))
#endif // !defined(SIZEOF_ARRAY)

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
static tmErrorCode_t OM3912Init(tmUnitSelect_t tUnit);
static tmErrorCode_t OM3912Reset(tmUnitSelect_t tUnit);
static tmErrorCode_t OM3912SetFrequency(tmUnitSelect_t tUnit, ptmTunerOnlyRequest_t pTuneRequest);

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


/*============================================================================*/
/*============================================================================*/
/* tmsysOM3912GetSWVersion                                                    */
/*============================================================================*/
tmErrorCode_t
tmsysOM3912GetSWVersion
(
    ptmsysSWVersion_t   pSWVersion  /* O: Receives SW Version  */
)
{
    tmErrorCode_t    err = TM_OK;
    static char      OM3912Name[] = "tmsysOM3912\0";
    tmbslSWVersion_t swTunerVersion;

    pSWVersion->arrayItemsNumber = 2;
    pSWVersion->swVersionArray[0].pName = (char *)OM3912Name;
    pSWVersion->swVersionArray[0].nameBufSize = sizeof(OM3912Name);
    
    pSWVersion->swVersionArray[0].swVersion.compatibilityNr = OM3912_SYS_COMP_NUM;
    pSWVersion->swVersionArray[0].swVersion.majorVersionNr = OM3912_SYS_MAJOR_VER;
    pSWVersion->swVersionArray[0].swVersion.minorVersionNr = OM3912_SYS_MINOR_VER;
    
    err = tmsysOM3912GetTunerSWVersion(&swTunerVersion);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmsysOM3912GetTunerSWVersion failed."));

    pSWVersion->swVersionArray[1].pName = swTunerVersion.pName;
    pSWVersion->swVersionArray[1].nameBufSize = swTunerVersion.nameBufSize;
    pSWVersion->swVersionArray[1].swVersion = swTunerVersion.swVersion;    

    return err;
}

/*============================================================================*/
/* tmsysOM3912Init                                                            */
/*============================================================================*/
tmErrorCode_t
tmsysOM3912Init
(
    tmUnitSelect_t              tUnit,      /* I: FrontEnd unit number */
    tmbslFrontEndDependency_t*  psSrvFunc   /*  I: setup parameters */
)
{
    ptmOM3912Object_t    pObj = Null;
    tmErrorCode_t        err = TM_OK;

    // pObj initialization
    err = OM3912GetInstance(tUnit, &pObj);

    /* check driver state */
    if (err == TM_OK || err == OM3912_ERR_NOT_INITIALIZED)
    {
        if (pObj != Null && pObj->init == True)
        {
            err = OM3912_ERR_NOT_INITIALIZED;
        }
        else 
        {
            /* initialize the Object */
            if (pObj == Null)
            {
                err = OM3912AllocInstance(tUnit, &pObj);
                if (err != TM_OK || pObj == Null)
                {
                    err = OM3912_ERR_NOT_INITIALIZED;        
                }
            }

            if (err == TM_OK)
            {
                // initialize the Object by default values
                pObj->sRWFunc = psSrvFunc->sIo;
                pObj->sTime = psSrvFunc->sTime;
                pObj->sDebug = psSrvFunc->sDebug;

                if(  psSrvFunc->sMutex.Init != Null
                    && psSrvFunc->sMutex.DeInit != Null
                    && psSrvFunc->sMutex.Acquire != Null
                    && psSrvFunc->sMutex.Release != Null)
                {
                    pObj->sMutex = psSrvFunc->sMutex;

                    err = pObj->sMutex.Init(&pObj->pMutex);
                }

                pObj->tUnitTuner = GET_INDEX_TUNIT(tUnit)|UNIT_PATH_TYPE_VAL(tmOM3912UnitDeviceTypeTuner);

                pObj->init = True;
                err = TM_OK;

                err = OM3912Init(tUnit);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "OM3912Init(0x%08X) failed.", tUnit));
           }
        }
    }

    return err;
}

/*============================================================================*/
/* tmsysOM3912DeInit                                                          */
/*============================================================================*/
tmErrorCode_t
tmsysOM3912DeInit
(
    tmUnitSelect_t  tUnit   /* I: FrontEnd unit number */
)
{
    ptmOM3912Object_t    pObj = Null;
    tmErrorCode_t        err = TM_OK;

    err = OM3912GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "OM3912GetInstance(0x%08X) failed.", tUnit));

    /************************************************************************/
    /* Close Tuner low layer setup                                          */
    /************************************************************************/
    if(err == TM_OK)
    {
        if(pObj->sMutex.DeInit != Null)
        {
            if(pObj->pMutex != Null)
            {
                err = pObj->sMutex.DeInit(pObj->pMutex);
            }

            pObj->sMutex.Init = Null;
            pObj->sMutex.DeInit = Null;
            pObj->sMutex.Acquire = Null;
            pObj->sMutex.Release = Null;

            pObj->pMutex = Null;
        }
        
        err = tmsysOM3912TunerClose(pObj->tUnitTuner);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmsysOM3912TunerDeInit(0x%08X) failed.", pObj->tUnitTuner));
    }
 
    err = OM3912DeAllocInstance(tUnit);

    return err;
}

/*============================================================================*/
/* tmsysOM3912Reset                                                           */
/*============================================================================*/
tmErrorCode_t
tmsysOM3912Reset
(
    tmUnitSelect_t  tUnit   /* I: FrontEnd unit number */
)
{
    ptmOM3912Object_t    pObj = Null;
    tmErrorCode_t        err = OM3912_ERR_NOT_INITIALIZED;

    /* check input parameters */
    err = OM3912GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "OM3912GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = OM3912MutexAcquire(pObj, OM3912_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        err = OM3912Reset(tUnit);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "OM3912Reset(0x%08X) failed.", tUnit));

        (void)OM3912MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* tmsysOM3912SetPowerState                                                   */
/*============================================================================*/
tmErrorCode_t
tmsysOM3912SetPowerState
(
    tmUnitSelect_t  tUnit,      /* I: FrontEnd unit number */
    tmPowerState_t  ePowerState /* I: Power state of the device */
)
{
    ptmOM3912Object_t    pObj = Null;
    tmErrorCode_t        err = TM_OK;

    err = OM3912GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "OM3912GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = OM3912MutexAcquire(pObj, OM3912_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        switch(ePowerState)
        {
            case tmPowerOn:
            case tmPowerStandby:
                if (err == TM_OK)
                {
                    pObj->powerState = ePowerState;

                    /* Set tuner power state to Normal Mode */
                    err = tmsysOM3912TunerSetPowerState(pObj->tUnitTuner, pObj->powerState);
                    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmsysOM3912TunerSetPowerState(0x%08X, tmPowerOn) failed.", pObj->tUnitTuner));
                }
                break;

            case tmPowerSuspend:
            case tmPowerOff:
            default:
                err = OM3912_ERR_NOT_SUPPORTED;
                break;
        }

        if (err == TM_OK)
        {
            pObj->lastTuneReqType = TRT_UNKNOWN;
        }

        (void)OM3912MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* tmsysOM3912SetPowerState                                                   */
/*============================================================================*/
tmErrorCode_t
tmsysOM3912GetPowerState
(
    tmUnitSelect_t  tUnit,      /* I: FrontEnd unit number */
    ptmPowerState_t pPowerState /* O: Power state of the device */
)
{
    ptmOM3912Object_t    pObj = Null;
    tmErrorCode_t        err = TM_OK;

    err = OM3912GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "OM3912GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = OM3912MutexAcquire(pObj, OM3912_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        *pPowerState = pObj->powerState;

        (void)OM3912MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* tmsysOM3912SendRequest                                                     */
/*============================================================================*/
tmErrorCode_t
tmsysOM3912SendRequest
(
    tmUnitSelect_t  tUnit,              /* I: FrontEnd unit number */
    pVoid           pTuneRequest,       /* I/O: Tune Request Structure pointer */
    UInt32          dwTuneRequestSize,  /* I: Tune Request Structure size */
    tmTuneReqType_t tTuneReqType        /* I: Tune Request Type */
)
{
    ptmOM3912Object_t    pObj = Null;
    tmErrorCode_t        err = TM_OK;

    if(err == TM_OK)
    {
        err = OM3912GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "OM3912GetInstance(0x%08X) failed.", tUnit));
    }

    // check pointer
    if(!pTuneRequest)
    {
        tmDBGPRINTEx(DEBUGLVL_ERROR, "Error: Invalid Pointer!");
        err = OM3912_ERR_BAD_PARAMETER;
    }

    if(err == TM_OK)
    {
        err = OM3912MutexAcquire(pObj, OM3912_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        switch(tTuneReqType)
        {
            case TRT_TUNER_ONLY:
                if( dwTuneRequestSize != sizeof(tmTunerOnlyRequest_t) )
                {
                    tmDBGPRINTEx(DEBUGLVL_ERROR, "Error: Bad parameter");
                    err = OM3912_ERR_BAD_PARAMETER;
                }
                else
                {
                    if (err == TM_OK)
                    {
                        ptmTunerOnlyRequest_t pTunerOnlyRequest = (ptmTunerOnlyRequest_t)(pTuneRequest);

                        err = OM3912SetFrequency(tUnit, pTunerOnlyRequest);
                        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "Error: OM3912SetFrequency failed"));
                    }
                }
                break;

            default:
                tmDBGPRINTEx(DEBUGLVL_ERROR, "Error: Unsupported tune request");
                err = OM3912_ERR_NOT_SUPPORTED;
                break;
        }

        (void)OM3912MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* tmsysOM3912SetI2CSwitchState                                               */
/*============================================================================*/
tmErrorCode_t
tmsysOM3912SetI2CSwitchState
(
    tmUnitSelect_t                  tUnit,  /* I: FrontEnd unit number */
    tmsysFrontEndI2CSwitchState_t   eState  /* I: I2C switch state */
)
{
    return OM3912_ERR_NOT_SUPPORTED;
}

/*============================================================================*/
/* tmsysOM3912GetI2CSwitchState                                               */
/*============================================================================*/
tmErrorCode_t
tmsysOM3912GetI2CSwitchState
(
    tmUnitSelect_t                  tUnit,
    tmsysFrontEndI2CSwitchState_t*  peState,
    UInt32*                         puI2CSwitchCounter
)
{
    return OM3912_ERR_NOT_SUPPORTED;
}

/*============================================================================*/
/* tmsysOM3912GetLockStatus                                                   */
/*============================================================================*/
tmErrorCode_t
tmsysOM3912GetLockStatus
(
    tmUnitSelect_t          tUnit,      /* I: FrontEnd unit number */
    tmsysFrontEndState_t*   pLockStatus /* O: Lock status */
)
{
    ptmOM3912Object_t    pObj = Null;
    tmErrorCode_t        err = TM_OK;
    tmbslFrontEndState_t lockStatus = tmbslFrontEndStateUnknown;

    err = OM3912GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "OM3912GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = OM3912MutexAcquire(pObj, OM3912_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Get tuner PLL Lock status */
        err = tmsysOM3912TunerGetLockStatus(pObj->tUnitTuner, &lockStatus);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmsysOM3912TunerGetLockStatus(0x%08X) failed.", pObj->tUnitTuner));

        if(err == TM_OK)
        {
           *pLockStatus = (tmsysFrontEndState_t)lockStatus;
        }

        (void)OM3912MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* tmsysOM3912GetSignalStrength                                               */
/*============================================================================*/
tmErrorCode_t
tmsysOM3912GetSignalStrength
(
    tmUnitSelect_t  tUnit,      /* I: FrontEnd unit number */
    UInt32          *pStrength  /* I/O: Signal Strength pointer */
)
{
    return OM3912_ERR_NOT_SUPPORTED;
}

/*============================================================================*/
/* tmsysOM3912GetSignalQuality                                                */
/*============================================================================*/
tmErrorCode_t
tmsysOM3912GetSignalQuality
(
    tmUnitSelect_t  tUnit,      /* I: FrontEnd unit number */
    UInt32          *pQuality   /* I/O: Signal Quality pointer */
)
{
   return OM3912_ERR_NOT_SUPPORTED;
}

/*============================================================================*/
/* tmsysOM3912GetDeviceUnit                                                   */
/*============================================================================*/
tmErrorCode_t
tmsysOM3912GetDeviceUnit
(
    tmUnitSelect_t              tUnit,      /* I: FrontEnd unit number */
    tmOM3912UnitDeviceType_t deviceType, /* I: Device Type  */
    ptmUnitSelect_t             ptUnit      /* O: Device unit number */
)
{
    ptmOM3912Object_t   pObj = Null;
    tmErrorCode_t       err = TM_OK;

    err = OM3912GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "OM3912GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = OM3912MutexAcquire(pObj, OM3912_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        if(ptUnit!=Null)
        {
            switch(deviceType)
            {
                default:
                case tmOM3912UnitDeviceTypeUnknown:
                    err = OM3912_ERR_BAD_PARAMETER;
                    break;

                case tmOM3912UnitDeviceTypeTuner:
                    *ptUnit = pObj->tUnitTuner;
                    break;
            }
        }
        
        (void)OM3912MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* tmsysOM3912SetHwAddress                                                    */
/*============================================================================*/
tmErrorCode_t
tmsysOM3912SetHwAddress
(
    tmUnitSelect_t              tUnit,      /* I: FrontEnd unit number */
    tmOM3912UnitDeviceType_t    deviceType, /* I: Device Type  */
    UInt32                      uHwAddress  /* I: Hardware Address */
)
{
    ptmOM3912Object_t           pObj = Null;
    tmErrorCode_t               err = TM_OK;

    err = OM3912GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "OM3912GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = OM3912MutexAcquire(pObj, OM3912_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        if(deviceType>tmOM3912UnitDeviceTypeUnknown && deviceType<tmOM3912UnitDeviceTypeMax)
        {
            pObj->uHwAddress[deviceType] = uHwAddress;
        }
        else
        {
            err = OM3912_ERR_BAD_PARAMETER;
        }

        (void)OM3912MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* tmsysOM3912GetHwAddress                                                    */
/*============================================================================*/
extern tmErrorCode_t
tmsysOM3912GetHwAddress
(
    tmUnitSelect_t              tUnit,      /* I: FrontEnd unit number */
    tmOM3912UnitDeviceType_t    deviceType, /* I: Device Type  */
    UInt32*                     puHwAddress /* O: Hardware Address */
)
{
    ptmOM3912Object_t           pObj = Null;
    tmErrorCode_t               err = TM_OK;

    err = OM3912GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "OM3912GetInstance(0x%08X) failed.", tUnit));

    /* Check incoming pointer */
    if(err == TM_OK && !puHwAddress)
    {
        tmDBGPRINTEx(DEBUGLVL_TERSE,("Error: Invalid Pointer!"));
        err = OM3912_ERR_BAD_PARAMETER;
    }

    if(err == TM_OK)
    {
        err = OM3912MutexAcquire(pObj, OM3912_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        if(deviceType>tmOM3912UnitDeviceTypeUnknown && deviceType<tmOM3912UnitDeviceTypeMax)
        {
            *puHwAddress = pObj->uHwAddress[deviceType];
        }
        else
        {
            err = OM3912_ERR_BAD_PARAMETER;
        }

        (void)OM3912MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/*                   STATIC FUNCTIONS DEFINITIONS                             */
/*============================================================================*/

/*============================================================================*/
/* OM3912Init                                                                 */
/*============================================================================*/
static tmErrorCode_t
OM3912Init
(
    tmUnitSelect_t  tUnit   /* I: FrontEnd unit number */
)
{
    ptmOM3912Object_t        pObj = Null;
    tmErrorCode_t               err = TM_OK;
    tmbslFrontEndDependency_t   sSrvFunc;   /* setup parameters */

    err = OM3912GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "OM3912GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        /* Fill function pointers structure */
        sSrvFunc.sIo                    = pObj->sRWFunc;
        sSrvFunc.sTime                  = pObj->sTime;
        sSrvFunc.sDebug                 = pObj->sDebug;
        sSrvFunc.sMutex                 = pObj->sMutex;
        sSrvFunc.dwAdditionalDataSize   = 0;
        sSrvFunc.pAdditionalData        = Null;

        /************************************************************************/
        /* Tuner low layer setup                                                */
        /************************************************************************/

        err = tmsysOM3912TunerOpen(pObj->tUnitTuner, &sSrvFunc);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmsysOM3912TunerOpen(0x%08X) failed.", pObj->tUnitTuner));
    }

    return err;
}

/*============================================================================*/
/* OM3912Reset                                                                */
/*============================================================================*/
static tmErrorCode_t
OM3912Reset
(
    tmUnitSelect_t  tUnit   /* I: FrontEnd unit number */
)
{
    ptmOM3912Object_t   pObj = Null;
    tmErrorCode_t       err = TM_OK;

    err = OM3912GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "OM3912GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = tmsysOM3912SetPowerState(pObj->tUnit, tmPowerStandby);
    }

    if(err == TM_OK)
    {
        pObj->resetDone = False;

        /************************************************************************/
        /* Tuner initialization                                                 */
        /************************************************************************/
        
        /* Reset the tuner */
        err = tmsysOM3912TunerReset(pObj->tUnitTuner);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmsysOM3912TunerReset(0x%08X) failed.", pObj->tUnitTuner));

        if (err != TM_OK)
        {
            tmDBGPRINTEx(DEBUGLVL_ERROR, "tmsysOM3912TunerReset(0x%08X) failed.", pObj->tUnitTuner);
            /* Open I²C switch to stop Tuner access */
            (void)tmsysOM3912SetI2CSwitchState(pObj->tUnit, tmsysFrontEndI2CSwitchStateReset);
        }
    }
    return err;
}

/*============================================================================*/
/* OM3912SetFrequency                                                         */
/*============================================================================*/
static tmErrorCode_t
OM3912SetFrequency
(
    tmUnitSelect_t          tUnit,          /* I: FrontEnd unit number */
    ptmTunerOnlyRequest_t   pTuneRequest    /* I/O: Tuner Tune Request Structure pointer */
)
{
    ptmOM3912Object_t           pObj = Null;
    tmErrorCode_t               err = TM_OK;
    tmbslFrontEndState_t        eTunerPLLLock = tmbslFrontEndStateUnknown;

    err = OM3912GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "OM3912GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        tmDBGPRINTEx(DEBUGLVL_TERSE, "\n\n===========================================================================");
        tmDBGPRINTEx(DEBUGLVL_TERSE, " OM3912SetFrequency(0x%08X) is called with following parameters:", pObj->tUnit);
        tmDBGPRINTEx(DEBUGLVL_TERSE, "===========================================================================");
        tmDBGPRINTEx(DEBUGLVL_TERSE, "     Frequency:           %d Hz", pTuneRequest->dwFrequency);
        tmDBGPRINTEx(DEBUGLVL_TERSE, "     Standard:            %d",    pTuneRequest->dwStandard);
        tmDBGPRINTEx(DEBUGLVL_TERSE, "===========================================================================");
    }

    if(err == TM_OK && pObj->lastTuneReqType != TRT_TUNER_ONLY)
    {
        tmDBGPRINTEx(DEBUGLVL_TERSE,("Configuring Tuner!"));

        if(err == TM_OK)
        {
            err = tmsysOM3912TunerSetPowerState(pObj->tUnitTuner, tmPowerOn);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmsysOM3912TunerSetPowerState(0x%08X, PowerOn) failed.", pObj->tUnitTuner));
        }

        if(err == TM_OK)
        {
            pObj->powerState = tmPowerOn;
            pObj->lastTuneReqType = TRT_TUNER_ONLY;
        }
    }

    /************************************************************************/
    /* Program Tuner                                                        */
    /************************************************************************/

    if(err == TM_OK)
    {
        /* Set Tuner Standard mode */
        err = tmsysOM3912TunerSetStandardMode(pObj->tUnitTuner, pTuneRequest->dwStandard);
        if (err != TM_OK)
        {
            tmDBGPRINTEx(DEBUGLVL_ERROR, "tmsysOM3912TunerSetStandardMode(0x%08X, %d) failed.", pObj->tUnitTuner, pTuneRequest->dwStandard);
        }
    }

    if(err == TM_OK)
    {
        /* Set Tuner RF */
        err = tmsysOM3912TunerSetRF(pObj->tUnitTuner, pTuneRequest->dwFrequency);
        if (err != TM_OK)
        {
            tmDBGPRINTEx(DEBUGLVL_ERROR, "tmsysOM3912TunerSetRF(0x%08X, %d) failed.", pObj->tUnitTuner, pTuneRequest->dwFrequency);
        }
    }
    
    if(err == TM_OK)
    {
        /* Get Tuner PLL Lock status */
        err = tmsysOM3912TunerGetLockStatus(pObj->tUnitTuner, &eTunerPLLLock);
        if (err != TM_OK)
        {
            tmDBGPRINTEx(DEBUGLVL_ERROR, "tmsysOM3912TunerGetLockStatus(0x%08X) failed.", pObj->tUnitTuner);
        }
    }

    if(err == TM_OK)
    {
        tmDBGPRINTEx(DEBUGLVL_TERSE, "Tuner(0x%08X) PLL Lock:%d.", pObj->tUnitTuner, eTunerPLLLock);
    }
    if(err == TM_OK)
    {
        pTuneRequest->eTunerLock = (tmsysFrontEndState_t)eTunerPLLLock;
    }

    /* Print the result of the Manager function */
    switch(eTunerPLLLock)
    {
        case tmbslFrontEndStateLocked:
            if(err == TM_OK)
            {
                tmDBGPRINTEx(DEBUGLVL_TERSE, "Tuner 0x%08X LOCKED.", pObj->tUnit);
            }
            if(err == TM_OK)
            {
                tmDBGPRINTEx(DEBUGLVL_TERSE, "===========================================================================");
                tmDBGPRINTEx(DEBUGLVL_TERSE, " OM3912SetFrequency(0x%08X) found following parameters:", pObj->tUnit);
                tmDBGPRINTEx(DEBUGLVL_TERSE, "===========================================================================");
                tmDBGPRINTEx(DEBUGLVL_TERSE, "     Frequency:      %d Hz", pTuneRequest->dwFrequency);
                tmDBGPRINTEx(DEBUGLVL_TERSE, "===========================================================================");
            }

            break;

        case tmbslFrontEndStateNotLocked:
            if(err == TM_OK)
            {
                tmDBGPRINTEx(DEBUGLVL_TERSE, "Tuner 0x%08X NOT LOCKED.", pObj->tUnit);
            }
            break;

        case tmbslFrontEndStateSearching:
        default:
            if(err == TM_OK)
            {
                tmDBGPRINTEx(DEBUGLVL_ERROR, "Tuner 0x%08X TIMEOUT.", pObj->tUnit);
            }
            break;
    }

    return err;
}

/*============================================================================*/
/* OM3912MutexAcquire                                                         */
/*============================================================================*/
extern tmErrorCode_t
OM3912MutexAcquire
(
    ptmOM3912Object_t    pObj,
    UInt32               timeOut
)
{
    tmErrorCode_t   err = TM_OK;

    if(pObj->sMutex.Acquire != Null && pObj->pMutex != Null)
    {
        err = pObj->sMutex.Acquire(pObj->pMutex, timeOut);
    }

    return err;
}

/*============================================================================*/
/* OM3912MutexRelease                                                         */
/*============================================================================*/
extern tmErrorCode_t
OM3912MutexRelease
(
    ptmOM3912Object_t    pObj
)
{
    tmErrorCode_t err = TM_OK;

    if(pObj->sMutex.Release != Null && pObj->pMutex != Null)
    {
        err = pObj->sMutex.Release(pObj->pMutex);
    }

    return err;
}

