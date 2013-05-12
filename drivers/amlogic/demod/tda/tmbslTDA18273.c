/*
  Copyright (C) 2010 NXP B.V., All Rights Reserved.
  This source code and any compilation or derivative thereof is the proprietary
  information of NXP B.V. and is confidential in nature. Under no circumstances
  is this software to be  exposed to or placed under an Open Source License of
  any type without the expressed written permission of NXP B.V.
 *
 * \file          tmbslTDA18273.c
 *
 *                %version: CFR_FEAP#39 %
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

/*============================================================================*/
/* Standard include files:                                                    */
/*============================================================================*/
#include "tmNxTypes.h"
#include "tmCompId.h"
#include "tmFrontEnd.h"
#include "tmbslFrontEndTypes.h"

/*============================================================================*/
/* Project include files:                                                     */
/*============================================================================*/
#include "tmbslTDA18273.h"

#include "tmbslTDA18273_RegDef.h"
#include "tmbslTDA18273_Local.h"
#include "tmbslTDA18273_Instance.h"
#include "tmbslTDA18273_InstanceCustom.h"


/*============================================================================*/
/* Static internal functions:                                                 */
/*============================================================================*/
static tmErrorCode_t
iTDA18273_SetLLPowerState(pTDA18273Object_t pObj, TDA18273PowerState_t powerState);

static tmErrorCode_t
iTDA18273_OverrideBandsplit(pTDA18273Object_t pObj);

static tmErrorCode_t
iTDA18273_OverrideDigitalClock(pTDA18273Object_t pObj, UInt32 uRF);

static tmErrorCode_t
iTDA18273_OverrideICP(pTDA18273Object_t pObj, UInt32 freq);

static tmErrorCode_t
iTDA18273_OverrideWireless(pTDA18273Object_t pObj);

static tmErrorCode_t
iTDA18273_FirstPassPLD_CC(pTDA18273Object_t pObj);

static tmErrorCode_t
iTDA18273_LastPassPLD_CC(pTDA18273Object_t pObj);

static tmErrorCode_t
iTDA18273_WaitXtalCal_End(pTDA18273Object_t pObj, UInt32 timeOut, UInt32 waitStep);

static tmErrorCode_t
iTDA18273_CalculatePostDivAndPrescaler(UInt32 LO, Bool growingOrder, UInt8* PostDiv, UInt8* Prescaler);

static tmErrorCode_t
iTDA18273_FindPostDivAndPrescalerWithBetterMargin(UInt32 LO, UInt8* PostDiv, UInt8* Prescaler);

static tmErrorCode_t
iTDA18273_CalculateNIntKInt(UInt32 LO, UInt8 PostDiv, UInt8 Prescaler, UInt32* NInt, UInt32* KInt);

static tmErrorCode_t
iTDA18273_SetPLL(pTDA18273Object_t pObj);

static tmErrorCode_t
iTDA18273_FirstPassLNAPower(pTDA18273Object_t pObj);

static tmErrorCode_t
iTDA18273_LastPassLNAPower(pTDA18273Object_t pObj);

/*============================================================================*/
/* Static variables:                                                          */
/*============================================================================*/
typedef struct _TDA18273_PostDivPrescalerTableDef_
{
    UInt32 LO_max;
    UInt32 LO_min;
    UInt8 Prescaler;
    UInt8 PostDiv;
} TDA18273_PostDivPrescalerTableDef;

/* Table that maps LO vs Prescaler & PostDiv values */
static TDA18273_PostDivPrescalerTableDef PostDivPrescalerTable[35] =
{
    /* PostDiv 1 */
    {974000, 852250, 7, 1},
    {852250, 745719, 8, 1},
    {757556, 662861, 9, 1},
    {681800, 596575, 10, 1},
    {619818, 542341, 11, 1},
    {568167, 497146, 12, 1},
    {524462, 458904, 13, 1},
    /* PostDiv 2 */
    {487000, 426125, 7, 2},
    {426125, 372859, 8, 2},
    {378778, 331431, 9, 2},
    {340900, 298288, 10, 2},
    {309909, 271170, 11, 2},
    {284083, 248573, 12, 2},
    {262231, 229452, 13, 2},
    /* PostDiv 4 */
    {243500, 213063, 7, 4},
    {213063, 186430, 8, 4},
    {189389, 165715, 9, 4},
    {170450, 149144, 10, 4},
    {154955, 135585, 11, 4},
    {142042, 124286, 12, 4},
    {131115, 114726, 13, 4},
    /* PostDiv 8 */
    {121750, 106531, 7, 8},
    {106531, 93215, 8, 8},
    {94694, 82858, 9, 8},
    {85225, 74572, 10, 8},
    {77477, 67793, 11, 8},
    {71021, 62143, 12, 8},
    {65558, 57363, 13, 8},
    /* PostDiv 16 */
    {60875, 53266, 7, 16},
    {53266, 46607, 8, 16},
    {47347, 41429, 9, 16},
    {42613, 37286, 10, 16},
    {38739, 33896, 11, 16},
    {35510, 31072, 12, 16},
    {32779, 28681, 13, 16}
};

/* Middle of VCO frequency excursion : VCOmin + (VCOmax - VCOmin)/2 in KHz */
#define TDA18273_MIDDLE_FVCO_RANGE ((6818000 - 5965750) / 2 + 5965750)

/*============================================================================*/
/* Exported functions:                                                        */
/*============================================================================*/

/*============================================================================*/
/* FUNCTION:    tmbslTDA18273_Open:                                           */
/*                                                                            */
/* DESCRIPTION: Opens driver setup environment.                               */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmbslTDA18273_Open(
    tmUnitSelect_t              tUnit,      /* I: FrontEnd unit number */
    tmbslFrontEndDependency_t*  psSrvFunc   /* I: setup parameters */
)
{
    pTDA18273Object_t   pObj = Null;
    tmErrorCode_t       err = TM_OK;

    /* Test parameter(s) */
    if(psSrvFunc == Null)
    {
        err = TDA18273_ERR_BAD_PARAMETER;
    }

    if(err == TM_OK)
    {
        /* Get a driver instance */
        err = iTDA18273_GetInstance(tUnit, &pObj);
    }

    /* Check driver instance state */
    if(err == TM_OK || err == TDA18273_ERR_ERR_NO_INSTANCES)
    {
        if(P_OBJ_VALID && pObj->init == True)
        {
            err = TDA18273_ERR_ALREADY_SETUP;
        }
        else 
        {
            if(P_OBJ_VALID == False)
            {
                /* Try to allocate an instance of the driver */
                err = iTDA18273_AllocInstance(tUnit, &pObj);
                if(err != TM_OK || pObj == Null)
                {
                    err = TDA18273_ERR_ERR_NO_INSTANCES;
                }
            }

            if(err == TM_OK)
            {
                /* Initialize the Object by default values */
                P_SIO = P_FUNC_SIO(psSrvFunc);
                P_STIME = P_FUNC_STIME(psSrvFunc);
                P_SDEBUG = P_FUNC_SDEBUG(psSrvFunc);

#ifdef _TVFE_IMPLEMENT_MUTEX
                if(    P_FUNC_SMUTEX_OPEN_VALID(psSrvFunc)
                    && P_FUNC_SMUTEX_CLOSE_VALID(psSrvFunc)
                    && P_FUNC_SMUTEX_ACQUIRE_VALID(psSrvFunc)
                    && P_FUNC_SMUTEX_RELEASE_VALID(psSrvFunc) )
                {
                    P_SMUTEX = psSrvFunc->sMutex;

                    err = P_SMUTEX_OPEN(&P_MUTEX);
                    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "Mutex_Open(0x%08X) failed.", tUnit));
                }
#endif

                tmDBGPRINTEx(DEBUGLVL_INOUT, "tmbslTDA18273_Open(0x%08X)", tUnit);

                if(err == TM_OK)
                {
                    pObj->init = True;
                }
            }
        }
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmbslTDA18273_Close:                                          */
/*                                                                            */
/* DESCRIPTION: Closes driver setup environment.                              */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmbslTDA18273_Close(
    tmUnitSelect_t  tUnit   /* I: FrontEnd unit number */
)
{
    pTDA18273Object_t   pObj = Null;
    tmErrorCode_t       err = TM_OK;

    /* Get a driver instance */
    err = iTDA18273_GetInstance(tUnit, &pObj);

    if(err == TM_OK)
    {
#ifdef _TVFE_IMPLEMENT_MUTEX
        /* Try to acquire driver mutex */
        err = iTDA18273_MutexAcquire(pObj, TDA18273_MUTEX_TIMEOUT);

        if(err == TM_OK)
        {
#endif
            tmDBGPRINTEx(DEBUGLVL_INOUT, "tmbslTDA18273_Close(0x%08X)", tUnit);

#ifdef _TVFE_IMPLEMENT_MUTEX
            P_SMUTEX_ACQUIRE = Null;

            /* Release driver mutex */
            (void)iTDA18273_MutexRelease(pObj);

            if(P_SMUTEX_CLOSE_VALID && P_MUTEX_VALID)
            {
                err = P_SMUTEX_CLOSE(P_MUTEX);
            }

            P_SMUTEX_OPEN = Null;
            P_SMUTEX_CLOSE = Null;
            P_SMUTEX_RELEASE = Null;

            P_MUTEX = Null;
        }
#endif

        err = iTDA18273_DeAllocInstance(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmbslTDA18273_GetSWVersion:                                   */
/*                                                                            */
/* DESCRIPTION: Gets the versions of the driver.                              */
/*                                                                            */
/* RETURN:      TM_OK                                                         */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmbslTDA18273_GetSWVersion(
    ptmSWVersion_t  pSWVersion  /* I: Receives SW Version */
)
{
    pSWVersion->compatibilityNr = TDA18273_COMP_NUM;
    pSWVersion->majorVersionNr  = TDA18273_MAJOR_VER;
    pSWVersion->minorVersionNr  = TDA18273_MINOR_VER;

    return TM_OK;
}

/*============================================================================*/
/* FUNCTION:    tmbslTDA18273_GetSWSettingsVersion:                           */
/*                                                                            */
/* DESCRIPTION: Returns the version of the driver settings.                   */
/*                                                                            */
/* RETURN:      TM_OK                                                         */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmbslTDA18273_GetSWSettingsVersion(
    ptmSWSettingsVersion_t pSWSettingsVersion   /* O: Receives SW Settings Version */
)
{
    pSWSettingsVersion->customerNr      = TDA18273_SETTINGS_CUSTOMER_NUM;
    pSWSettingsVersion->projectNr       = TDA18273_SETTINGS_PROJECT_NUM;
    pSWSettingsVersion->majorVersionNr  = TDA18273_SETTINGS_MAJOR_VER;
    pSWSettingsVersion->minorVersionNr  = TDA18273_SETTINGS_MINOR_VER;

    return TM_OK;
}

/*============================================================================*/
/* FUNCTION:    tmbslTDA18273_CheckHWVersion:                                 */
/*                                                                            */
/* DESCRIPTION: Checks TDA18273 HW Version.                                   */
/*                                                                            */
/* RETURN:      TM_OK                                                         */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmbslTDA18273_CheckHWVersion(
    tmUnitSelect_t tUnit    /* I: Unit number */
)
{
    pTDA18273Object_t   pObj = Null;
    tmErrorCode_t       err = TM_OK;
    UInt16              uIdentity = 0;
    UInt8               ID_byte_1 = 0;
    UInt8               ID_byte_2 = 0;
    UInt8               majorRevision = 0;
    UInt8               minorRevision = 0;

    /* Get a driver instance */
    err = iTDA18273_GetInstance(tUnit, &pObj);

    _MUTEX_ACQUIRE(TDA18273)

    tmDBGPRINTEx(DEBUGLVL_INOUT, "tmbslTDA18273_CheckHWVersion(0x%08X)", tUnit);

    err = iTDA18273_ReadRegMap(pObj, gTDA18273_Reg_ID_byte_1.Address, TDA18273_REG_DATA_LEN(gTDA18273_Reg_ID_byte_1__Ident_1, gTDA18273_Reg_ID_byte_3));
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_ReadRegMap(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = iTDA18273_Read(pObj, &gTDA18273_Reg_ID_byte_1__Ident_1, &ID_byte_1, Bus_None);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Read(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = iTDA18273_Read(pObj, &gTDA18273_Reg_ID_byte_2__Ident_2, &ID_byte_2, Bus_None);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Read(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        /* Construct Identity */
        uIdentity = (ID_byte_1 << 8) | ID_byte_2;

        if(uIdentity == 18273)
        {
            /* TDA18273 found. Check Major & Minor Revision*/
            err = iTDA18273_Read(pObj, &gTDA18273_Reg_ID_byte_3__Major_rev, &majorRevision, Bus_None);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Read(0x%08X) failed.", tUnit));

            err = iTDA18273_Read(pObj, &gTDA18273_Reg_ID_byte_3__Minor_rev, &minorRevision, Bus_None);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Read(0x%08X) failed.", tUnit));

            switch (majorRevision)
            {
                case 1:
                {
                    switch (minorRevision)
                    {
                        case 1:
                            /* ES2 is supported */
                            err = TM_OK;
                            break;

                        default:
                            /* Only TDA18273 ES2 are supported */
                            err = TDA18273_ERR_BAD_VERSION;
                            break;
                    }
                }
                break;

                default:
                    /* Only TDA18273 ES2 are supported */
                    err = TDA18273_ERR_BAD_VERSION;
                    break;
            }
        }
        else
        {
            err = TDA18273_ERR_BAD_VERSION;
        }
    }

    _MUTEX_RELEASE(TDA18273)

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmbslTDA18273_SetPowerState                                   */
/*                                                                            */
/* DESCRIPTION: Sets the power state.                                         */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmbslTDA18273_SetPowerState(
    tmUnitSelect_t  tUnit,      /* I: Unit number */
    tmPowerState_t  powerState  /* I: Power state */
 )
{
    pTDA18273Object_t   pObj = Null;
    tmErrorCode_t       err = TM_OK;

    /* Get a driver instance */
    err = iTDA18273_GetInstance(tUnit, &pObj);

    _MUTEX_ACQUIRE(TDA18273)

    tmDBGPRINTEx(DEBUGLVL_INOUT, "tmbslTDA18273_SetPowerState(0x%08X)", tUnit);

    if(powerState>=tmPowerMax)
    {
        err = TDA18273_ERR_BAD_PARAMETER;
    }

    if(err == TM_OK)
    {
        if(pObj->mapLLPowerState[powerState] != pObj->curLLPowerState)
        {
            err = iTDA18273_SetLLPowerState(pObj, pObj->mapLLPowerState[powerState]);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_SetLLPowerState(0x%08X, %d) failed.", tUnit, (int)pObj->mapLLPowerState[powerState]));

            if(err == TM_OK)
            {
                /* Store power state in driver instance */
                pObj->curPowerState = powerState;
            }
        }
    }

    _MUTEX_RELEASE(TDA18273)

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmbslTDA18273_GetPowerState:                                  */
/*                                                                            */
/* DESCRIPTION: Gets the power state.                                         */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmbslTDA18273_GetPowerState(
    tmUnitSelect_t  tUnit,      /* I: Unit number */
    tmPowerState_t* pPowerState /* O: Power state */
)
{
    pTDA18273Object_t   pObj = Null;
    tmErrorCode_t       err = TM_OK;

    /* Get a driver instance */
    err = iTDA18273_GetInstance(tUnit, &pObj);

    _MUTEX_ACQUIRE(TDA18273)

    tmDBGPRINTEx(DEBUGLVL_INOUT, "tmbslTDA18273_GetPowerState(0x%08X)", tUnit);

    /* Test parameter(s) */
    if(pPowerState == Null)
    {
        err = TDA18273_ERR_BAD_PARAMETER;
    }

    if(err == TM_OK)
    {
        *pPowerState = pObj->curPowerState;
    }

    _MUTEX_RELEASE(TDA18273)

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmbslTDA18273_SetLLPowerState                                 */
/*                                                                            */
/* DESCRIPTION: Sets the power state.                                         */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmbslTDA18273_SetLLPowerState(
    tmUnitSelect_t          tUnit,      /* I: Unit number */
    TDA18273PowerState_t    powerState  /* I: Power state of TDA18273 */
 )
{
    pTDA18273Object_t   pObj = Null;
    tmErrorCode_t       err = TM_OK;

    /* Get a driver instance */
    err = iTDA18273_GetInstance(tUnit, &pObj);

    _MUTEX_ACQUIRE(TDA18273)

    tmDBGPRINTEx(DEBUGLVL_INOUT, "tmbslTDA18273_SetLLPowerState(0x%08X)", tUnit);

    pObj->curPowerState = tmPowerMax;

    err = iTDA18273_SetLLPowerState(pObj, powerState);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_SetLLPowerState(0x%08X, %d) failed.", tUnit, (int)powerState));

    _MUTEX_RELEASE(TDA18273)

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmbslTDA18273_GetLLPowerState                                 */
/*                                                                            */
/* DESCRIPTION: Gets the power state.                                         */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmbslTDA18273_GetLLPowerState(
    tmUnitSelect_t          tUnit,      /* I: Unit number */
    TDA18273PowerState_t*   pPowerState /* O: Power state of TDA18273 */
)
{
    pTDA18273Object_t   pObj = Null;
    tmErrorCode_t       err = TM_OK;
    UInt8               uValue = 0;


    /* Get a driver instance */
    err = iTDA18273_GetInstance(tUnit, &pObj);

    _MUTEX_ACQUIRE(TDA18273)

    tmDBGPRINTEx(DEBUGLVL_INOUT, "tmbslTDA18273_GetLLPowerState(0x%08X)", tUnit);

    /* Test parameter(s) */
    if(pPowerState == Null)
    {
        err = TDA18273_ERR_BAD_PARAMETER;
    }

    if(err == TM_OK)
    {
        err = iTDA18273_Read(pObj, &gTDA18273_Reg_Power_state_byte_2, &uValue, Bus_RW);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Read(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        if(!(uValue&(TDA18273_SM|TDA18273_SM_XT)))
        {
            *pPowerState = TDA18273_PowerNormalMode;
        }
        else if( (uValue&TDA18273_SM) && !(uValue&TDA18273_SM_XT) )
        {
            *pPowerState = TDA18273_PowerStandbyWithXtalOn;
        }
        else if( (uValue&TDA18273_SM) && (uValue&TDA18273_SM_XT) )
        {
            *pPowerState = TDA18273_PowerStandby;
        }
        else
        {
            err = TDA18273_ERR_NOT_SUPPORTED;
        }
    }

    _MUTEX_RELEASE(TDA18273)

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmbslTDA18273_SetStandardMode                                 */
/*                                                                            */
/* DESCRIPTION: Sets the standard mode.                                       */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmbslTDA18273_SetStandardMode(
    tmUnitSelect_t          tUnit,          /* I: Unit number */
    TDA18273StandardMode_t  StandardMode    /* I: Standard mode of this device */
)
{
    pTDA18273Object_t           pObj = Null;
    tmErrorCode_t               err = TM_OK;
    pTDA18273StdCoefficients    prevPStandard = Null;

    /* Get a driver instance */
    err = iTDA18273_GetInstance(tUnit, &pObj);

    _MUTEX_ACQUIRE(TDA18273)

    tmDBGPRINTEx(DEBUGLVL_INOUT, "tmbslTDA18273_SetStandardMode(0x%08X)", tUnit);

    /* Check if Hw is ready to operate */
    err = iTDA18273_CheckHwState(pObj, TDA18273_HwStateCaller_SetStd);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_CheckHwState(0x%08X) failed.", pObj->tUnitW));

    if(err == TM_OK)
    {
        /* Store standard mode */
        pObj->StandardMode = StandardMode;

        /* Reset standard map pointer */
        prevPStandard = pObj->pStandard;
        pObj->pStandard = Null;

        if(pObj->StandardMode>TDA18273_StandardMode_Unknown && pObj->StandardMode<TDA18273_StandardMode_Max)
        {
            /* Update standard map pointer */
            pObj->pStandard = &pObj->Std_Array[pObj->StandardMode - 1];

            /****************************************************************/
            /* IF SELECTIVITY Settings                                      */
            /****************************************************************/

            /* Set LPF */
            err = iTDA18273_Write(pObj, &gTDA18273_Reg_IF_Byte_1__LP_Fc, pObj->pStandard->LPF, Bus_None);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));

            if(err == TM_OK)
            {
                /* Set LPF Offset */
                err = iTDA18273_Write(pObj, &gTDA18273_Reg_IF_Byte_1__LP_FC_Offset, pObj->pStandard->LPF_Offset, Bus_None);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
            }

            if(err == TM_OK)
            {
                /* Set DC_Notch_IF_PPF */
                err = iTDA18273_Write(pObj, &gTDA18273_Reg_IR_Mixer_byte_2__IF_Notch, pObj->pStandard->DC_Notch_IF_PPF, Bus_None);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
            }

            if(err == TM_OK)
            {
                /* Enable/disable HPF */
                if(pObj->pStandard->IF_HPF == TDA18273_IF_HPF_Disabled )
                {
                    err = iTDA18273_Write(pObj, &gTDA18273_Reg_IR_Mixer_byte_2__Hi_Pass, 0x00, Bus_None);
                    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
                }
                else
                {
                    err = iTDA18273_Write(pObj, &gTDA18273_Reg_IR_Mixer_byte_2__Hi_Pass, 0x01, Bus_None);
                    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));

                    if(err == TM_OK)
                    {
                        /* Set IF HPF */
                        err = iTDA18273_Write(pObj, &gTDA18273_Reg_IF_Byte_1__IF_HP_Fc, (UInt8)(pObj->pStandard->IF_HPF - 1), Bus_None);
                        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
                    }
                }
            }

            if(err == TM_OK)
            {
                /* Set IF Notch */
                err = iTDA18273_Write(pObj, &gTDA18273_Reg_IF_Byte_1__IF_ATSC_Notch, pObj->pStandard->IF_Notch, Bus_None);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
            }

            if(err == TM_OK)
            {
                /* Set IF notch to RSSI */
                err = iTDA18273_Write(pObj, &gTDA18273_Reg_IF_AGC_byte__IFnotchToRSSI, pObj->pStandard->IFnotchToRSSI, Bus_None);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
            }

            /****************************************************************/
            /* AGC TOP Settings                                             */
            /****************************************************************/

            if(err == TM_OK)
            {
                /* Set AGC1 TOP I2C DN/UP */
                err = iTDA18273_Write(pObj, &gTDA18273_Reg_AGC1_byte_1__AGC1_TOP, pObj->pStandard->AGC1_TOP_I2C_DN_UP, Bus_None);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
            }

            if(err == TM_OK)
            {
                /* Set AGC1 Adapt TOP DN/UP */
                err = iTDA18273_Write(pObj, &gTDA18273_Reg_AGC1_byte_2__AGC1_Top_Mode_Val, pObj->pStandard->AGC1_Adapt_TOP_DN_UP, Bus_None);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
            }

			if(err == TM_OK)
            {
                /* Set AGC1 DN Time Constant */
                err = iTDA18273_Write(pObj, &gTDA18273_Reg_AGC1_byte_3__AGC1_Do_step, pObj->pStandard->AGC1_DN_Time_Constant, Bus_None);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
            }

            if(err == TM_OK)
            {
                /* Set AGC1 mode */
                err = iTDA18273_Write(pObj, &gTDA18273_Reg_AGC1_byte_2__AGC1_Top_Mode, pObj->pStandard->AGC1_Mode, Bus_None);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
            }

            if(err == TM_OK)
            {
                /* Set Range_LNA_Adapt */
                err = iTDA18273_Write(pObj, &gTDA18273_Reg_Adapt_Top_byte__Range_LNA_Adapt, pObj->pStandard->Range_LNA_Adapt, Bus_None);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
            }

            if(err == TM_OK)
            {
                /* Set LNA_Adapt_RFAGC_Gv_Threshold */
                err = iTDA18273_Write(pObj, &gTDA18273_Reg_Adapt_Top_byte__Index_K_LNA_Adapt, pObj->pStandard->LNA_Adapt_RFAGC_Gv_Threshold, Bus_None);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
            }

            if(err == TM_OK)
            {
                /* Set AGC1_Top_Adapt_RFAGC_Gv_Threshold */
                err = iTDA18273_Write(pObj, &gTDA18273_Reg_Adapt_Top_byte__Index_K_Top_Adapt, pObj->pStandard->AGC1_Top_Adapt_RFAGC_Gv_Threshold, Bus_None);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
            }

            if(err == TM_OK)
            {
                /* Set AGC2 TOP DN/UP */
                err = iTDA18273_Write(pObj, &gTDA18273_Reg_AGC2_byte_1__AGC2_TOP, pObj->pStandard->AGC2_TOP_DN_UP, Bus_None);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
            }

			if(err == TM_OK)
            {
                /* Set AGC2 DN Time Constant */
                err = iTDA18273_Write(pObj, &gTDA18273_Reg_RF_Filters_byte_3__AGC2_Do_step, pObj->pStandard->AGC2_DN_Time_Constant, Bus_None);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
            }

            if(err == TM_OK)
            {
                /* Set AGC4 TOP DN/UP */
                err = iTDA18273_Write(pObj, &gTDA18273_Reg_IR_Mixer_byte_1__IR_Mixer_Top, pObj->pStandard->AGC4_TOP_DN_UP, Bus_None);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
            }

            if(err == TM_OK)
            {
                /* Set AGC5 TOP DN/UP */
                err = iTDA18273_Write(pObj, &gTDA18273_Reg_AGC5_byte_1__AGC5_TOP, pObj->pStandard->AGC5_TOP_DN_UP, Bus_None);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
            }

            if(err == TM_OK)
            {
                /* Set AGC3_Top_Adapt_Algorithm */
                err = iTDA18273_Write(pObj, &gTDA18273_Reg_RF_AGC_byte__PD_AGC_Adapt3x, pObj->pStandard->AGC3_Top_Adapt_Algorithm, Bus_None);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
            }

            if(err == TM_OK)
            {
                /* Set AGC Overload TOP */
                err = iTDA18273_Write(pObj, &gTDA18273_Reg_Vsync_Mgt_byte__AGC_Ovld_TOP, pObj->pStandard->AGC_Overload_TOP, Bus_None);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
            }

            if(err == TM_OK)
            {
                /* Set Adapt TOP 34 Gain Threshold */
                err = iTDA18273_Write(pObj, &gTDA18273_Reg_RFAGCs_Gain_byte_1__TH_AGC_Adapt34, pObj->pStandard->TH_AGC_Adapt34, Bus_None);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
            }

            if(err == TM_OK)
            {
                /* Set RF atten 3dB */
                err = iTDA18273_Write(pObj, &gTDA18273_Reg_W_Filter_byte__RF_Atten_3dB, pObj->pStandard->RF_Atten_3dB, Bus_None);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
            }

            if(err == TM_OK)
            {
                /* Set IF Output Level */
                err = iTDA18273_Write(pObj, &gTDA18273_Reg_IF_AGC_byte__IF_level, pObj->pStandard->IF_Output_Level, Bus_None);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
            }

            if(err == TM_OK)
            {
                /* Set S2D gain */
                err = iTDA18273_Write(pObj, &gTDA18273_Reg_IR_Mixer_byte_1__S2D_Gain, pObj->pStandard->S2D_Gain, Bus_None);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
            }

            if(err == TM_OK)
            {
                /* Set Negative modulation, write into register directly because vsync_int bit is checked afterwards */
                err = iTDA18273_Write(pObj, &gTDA18273_Reg_Vsync_byte__Neg_modulation, pObj->pStandard->Negative_Modulation, Bus_RW);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
            }

            /****************************************************************/
            /* GSK Settings                                                 */
            /****************************************************************/

            if(err == TM_OK)
            {
                /* Set AGCK Step */
                err = iTDA18273_Write(pObj, &gTDA18273_Reg_AGCK_byte_1__AGCK_Step, pObj->pStandard->AGCK_Steps, Bus_None);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
            }

            if(err == TM_OK)
            {
                /* Set AGCK Time Constant */
                err = iTDA18273_Write(pObj, &gTDA18273_Reg_AGCK_byte_1__AGCK_Mode, pObj->pStandard->AGCK_Time_Constant, Bus_None);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
            }

            if(err == TM_OK)
            {
                /* Set AGC5 HPF */
                UInt8 wantedValue = pObj->pStandard->AGC5_HPF;

                if(pObj->pStandard->AGC5_HPF == TDA18273_AGC5_HPF_Enabled)
                {
                    UInt8 checked = 0;

                    /* Check if Internal Vsync is selected */
                    err = iTDA18273_Read(pObj, &gTDA18273_Reg_Vsync_byte__Vsync_int, &checked, Bus_RW);
                    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Read(0x%08X) failed.", pObj->tUnitW));

                    if(err == TM_OK && checked == 0)
                    {
                        /* Internal Vsync is OFF, so override setting to OFF */
                        wantedValue = TDA18273_AGC5_HPF_Disabled;
                    }
                }

                if(err == TM_OK)
                {
                    err = iTDA18273_Write(pObj, &gTDA18273_Reg_AGC5_byte_1__AGC5_Ana, wantedValue, Bus_None);
                    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
                }
            }

            if(err == TM_OK)
            {
                /* Set Pulse Shaper Disable */
                err = iTDA18273_Write(pObj, &gTDA18273_Reg_AGCK_byte_1__Pulse_Shaper_Disable, pObj->pStandard->Pulse_Shaper_Disable, Bus_None);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
            }

            /****************************************************************/
            /* H3H5 Settings                                                */
            /****************************************************************/

            if(err == TM_OK)
            {
                /* Set VHF_III_Mode */
                err = iTDA18273_Write(pObj, &gTDA18273_Reg_W_Filter_byte__VHF_III_mode, pObj->pStandard->VHF_III_Mode, Bus_None);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
            }

            /****************************************************************/
            /* PLL Settings                                                 */
            /****************************************************************/

            if(err == TM_OK)
            {
                /* Set LO_CP_Current */
                err = iTDA18273_Write(pObj, &gTDA18273_Reg_CP_Current_byte__LO_CP_Current, pObj->pStandard->LO_CP_Current, Bus_None);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
            }

            /****************************************************************/
            /* IF Settings                                                  */
            /****************************************************************/

            if(err == TM_OK)
            {
                /* Set IF */
                err = iTDA18273_Write(pObj, &gTDA18273_Reg_IF_Frequency_byte__IF_Freq, (UInt8)((pObj->pStandard->IF - pObj->pStandard->CF_Offset)/50000), Bus_None);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));

                /* Update IF in pObj */
                pObj->uIF = pObj->pStandard->IF;
            }

			/****************************************************************/
            /* MISC Settings                                                */
            /****************************************************************/
			
			if(err == TM_OK)
            {
                /* Set PD Underload */
                err = iTDA18273_Write(pObj, &gTDA18273_Reg_Misc_byte__PD_Underload, pObj->pStandard->PD_Underload, Bus_None);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
            }

			/****************************************************************/
            /* Update Registers                                             */
            /****************************************************************/

            if(err == TM_OK)
            {
                /* Write AGC1_byte_1 (0x0C) to IF_Byte_1 (0x15) Registers */
                err = iTDA18273_WriteRegMap(pObj, gTDA18273_Reg_AGC1_byte_1.Address, TDA18273_REG_DATA_LEN(gTDA18273_Reg_AGC1_byte_1, gTDA18273_Reg_IF_Byte_1));
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_WriteRegMap(0x%08X) failed.", pObj->tUnitW));
            }

            if(err == TM_OK)
            {
                /* Write IF_Frequency_byte (0x17) Register */
                err = iTDA18273_WriteRegMap(pObj, gTDA18273_Reg_IF_Frequency_byte.Address, 1);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_WriteRegMap(0x%08X) failed.", pObj->tUnitW));
            }

            if(err == TM_OK)
            {
                /* Write Adapt_Top_byte (0x1F) Register */
                err = iTDA18273_WriteRegMap(pObj, gTDA18273_Reg_Adapt_Top_byte.Address, 1);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_WriteRegMap(0x%08X) failed.", pObj->tUnitW));
            }

            if(err == TM_OK)
            {
                /* Write Vsync_byte (0x20) to RFAGCs_Gain_byte_1 (0x24) Registers */
                err = iTDA18273_WriteRegMap(pObj, gTDA18273_Reg_Vsync_byte.Address, TDA18273_REG_DATA_LEN(gTDA18273_Reg_Vsync_byte, gTDA18273_Reg_RFAGCs_Gain_byte_1));
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_WriteRegMap(0x%08X) failed.", pObj->tUnitW));
            }

            if(err == TM_OK)
            {
                /* Write RF_Filters_byte_3 (0x2D) Register */
                err = iTDA18273_WriteRegMap(pObj, gTDA18273_Reg_RF_Filters_byte_3.Address, 1);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_WriteRegMap(0x%08X) failed.", pObj->tUnitW));
            }

            if(err == TM_OK)
            {
                /* Write CP_Current_byte (0x2F) Register */
                err = iTDA18273_WriteRegMap(pObj, gTDA18273_Reg_CP_Current_byte.Address, 1);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_WriteRegMap(0x%08X) failed.", pObj->tUnitW));
            }

			if(err == TM_OK)
            {
                /* Write Misc_byte (0x38) Register */
                err = iTDA18273_WriteRegMap(pObj, gTDA18273_Reg_Misc_byte.Address, 1);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_WriteRegMap(0x%08X) failed.", pObj->tUnitW));
            }
        }

        /* Update driver state machine */
        pObj->eHwState = TDA18273_HwState_SetStdDone;
    }

    _MUTEX_RELEASE(TDA18273)

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmbslTDA18273_GetStandardMode                                 */
/*                                                                            */
/* DESCRIPTION: Gets the standard mode.                                       */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmbslTDA18273_GetStandardMode(
    tmUnitSelect_t          tUnit,          /* I: Unit number */
    TDA18273StandardMode_t  *pStandardMode  /* O: Standard mode of this device */
)
{
    pTDA18273Object_t   pObj = Null;
    tmErrorCode_t       err = TM_OK;

    /* Get a driver instance */
    err = iTDA18273_GetInstance(tUnit, &pObj);

    _MUTEX_ACQUIRE(TDA18273)

    tmDBGPRINTEx(DEBUGLVL_INOUT, "tmbslTDA18273_GetStandardMode(0x%08X)", tUnit);

    if(pStandardMode == Null)
    {
        err = TDA18273_ERR_BAD_PARAMETER;
    }

    if(err == TM_OK)
    {
        /* Get standard mode */
        *pStandardMode = pObj->StandardMode;
    }

    _MUTEX_RELEASE(TDA18273)

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmbslTDA18273_SetRF:                                          */
/*                                                                            */
/* DESCRIPTION: Tunes to a RF.                                                */
/*                                                                            */
/* RETURN:      TM_OK                                                         */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmbslTDA18273_SetRF(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt32          uRF     /* I: RF frequency in hertz */
)
{
    pTDA18273Object_t   pObj = Null;
    tmErrorCode_t       err = TM_OK;

    /* Get a driver instance */
    err = iTDA18273_GetInstance(tUnit, &pObj);

    _MUTEX_ACQUIRE(TDA18273)

    tmDBGPRINTEx(DEBUGLVL_INOUT, "tmbslTDA18273_SetRF(0x%08X)", tUnit);

    /* Test parameter(s) */
    if(   pObj->StandardMode<=TDA18273_StandardMode_Unknown
       || pObj->StandardMode>=TDA18273_StandardMode_Max
       || pObj->pStandard == Null)
    {
        err = TDA18273_ERR_STD_NOT_SET;
    }

    if(err == TM_OK)
    {
        /* Check if Hw is ready to operate */
        err = iTDA18273_CheckHwState(pObj, TDA18273_HwStateCaller_SetRF);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_CheckHwState(0x%08X) failed.", pObj->tUnitW));
    }

    if(err == TM_OK)
    {
        pObj->uRF = uRF;
        pObj->uProgRF = pObj->uRF + pObj->pStandard->CF_Offset;

        err = iTDA18273_SetRF(pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_SetRF(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /* Update driver state machine */
            pObj->eHwState = TDA18273_HwState_SetRFDone;
        }
    }

    _MUTEX_RELEASE(TDA18273)

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmbslTDA18273_GetRF:                                          */
/*                                                                            */
/* DESCRIPTION: Gets tuned RF.                                                */
/*                                                                            */
/* RETURN:      TM_OK                                                         */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmbslTDA18273_GetRF(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt32*         puRF    /* O: RF frequency in hertz */
)
{
    pTDA18273Object_t   pObj = Null;
    tmErrorCode_t       err = TM_OK;

    /* Get a driver instance */
    err = iTDA18273_GetInstance(tUnit, &pObj);

    _MUTEX_ACQUIRE(TDA18273)

    tmDBGPRINTEx(DEBUGLVL_INOUT, "tmbslTDA18273_GetRF(0x%08X)", tUnit);

    if(puRF == Null)
    {
        err = TDA18273_ERR_BAD_PARAMETER;
    }

    if(err == TM_OK)
    {
        /* Get RF */
        *puRF = pObj->uRF;
    }

    _MUTEX_RELEASE(TDA18273)

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmbslTDA18273_HwInit:                                         */
/*                                                                            */
/* DESCRIPTION: Initializes TDA18273 Hardware.                                */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmbslTDA18273_HwInit(
    tmUnitSelect_t  tUnit   /* I: Unit number */
)
{
    pTDA18273Object_t   pObj = Null;
    tmErrorCode_t       err = TM_OK;

    /* Get a driver instance */
    err = iTDA18273_GetInstance(tUnit, &pObj);

    _MUTEX_ACQUIRE(TDA18273)

    tmDBGPRINTEx(DEBUGLVL_INOUT, "tmbslTDA18273_HwInit(0x%08X)", tUnit);

    /* Reset standard mode & Hw State */
    pObj->StandardMode = TDA18273_StandardMode_Max;
    pObj->eHwState = TDA18273_HwState_InitNotDone;

    /***************************************************************/
    /* Workaround for failing XTALCAL_End & RFCAL_End & PLD spread */
    /* Perform a SW_RESET and manually launch the CALs             */
    /***************************************************************/

    /* Set digital clock mode to 16 Mhz before resetting the IC to avoid unclocking the digital part */
    err = iTDA18273_Write(pObj, &gTDA18273_Reg_Reference_Byte__Digital_Clock_Mode, 0x00, Bus_RW);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));

    /* Perform a SW reset to reset the digital calibrations & the IC machine state */
    if(err == TM_OK)
    {
        err = iTDA18273_Write(pObj, &gTDA18273_Reg_Power_Down_byte_3, 0x03, Bus_RW);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
    }

    if(err == TM_OK)
    {
        err = iTDA18273_Write(pObj, &gTDA18273_Reg_Power_Down_byte_3, 0x00, Bus_NoRead);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
    }

    if(err == TM_OK)
    {
        /* Set power state on */
        err = iTDA18273_SetLLPowerState(pObj, TDA18273_PowerNormalMode);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_SetLLPowerState(0x%08X, PowerNormalMode) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        pObj->curPowerState = tmPowerOn;
    }

    /* Only if tuner has a XTAL */
    if (pObj->bBufferMode == False)
    {
        /* Reset XTALCAL_End bit */
        if(err == TM_OK)
        {
            /* Set IRQ_clear */
            err = iTDA18273_Write(pObj, &gTDA18273_Reg_IRQ_clear, TDA18273_IRQ_Global|TDA18273_IRQ_XtalCal|TDA18273_IRQ_HwInit|TDA18273_IRQ_IrCal, Bus_NoRead);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
        }

        /* Launch XTALCAL */
        if(err == TM_OK)
        {
            err = iTDA18273_Write(pObj, &gTDA18273_Reg_MSM_byte_2__XtalCal_Launch, 0x01, Bus_NoRead);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
        }

        if(err == TM_OK)
        {
            err = iTDA18273_Write(pObj, &gTDA18273_Reg_MSM_byte_2__XtalCal_Launch, 0x00, Bus_None);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
        }
        
        /* Wait XTALCAL_End bit */
        if(err == TM_OK)
        {
            err = iTDA18273_WaitXtalCal_End(pObj, 100, 10);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_WaitXtalCal_End(0x%08X) failed.", pObj->tUnitW));
        }
    }

    /***************************************************************/
    /* End of workaround                                           */
    /***************************************************************/

    if(err == TM_OK)
    {
        /* Read all bytes */
        err = iTDA18273_ReadRegMap(pObj, 0x00, TDA18273_REG_MAP_NB_BYTES);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_ReadRegMap(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        /* Check if Calc_PLL algorithm is in automatic mode */
        err = iTDA18273_CheckCalcPLL(pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_CheckCalcPLL(0x%08X) failed.", pObj->tUnitW));
    }

    if(err == TM_OK)
    {
        /****************************************************/
        /* Change POR values                                */
        /****************************************************/

        /* Up_Step_Ovld: POR = 1 -> 0 */
        err = iTDA18273_Write(pObj, &gTDA18273_Reg_Vsync_Mgt_byte__Up_Step_Ovld, 0x00, Bus_NoRead);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));

        if(err == TM_OK)
        {
            /* PLD_CC_Enable: POR = 1 -> 0 */
            err = iTDA18273_Write(pObj, &gTDA18273_Reg_RFAGCs_Gain_byte_1__PLD_CC_Enable, 0x00, Bus_NoRead);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
        }

        if(err == TM_OK)
        {
            /* RFCAL_Offset0 : 0 */
            err = iTDA18273_Write(pObj, &gTDA18273_Reg_RF_Cal_byte_1__RFCAL_Offset_Cprog0, 0x00, Bus_None);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));

            if(err == TM_OK)
            {
                /* RFCAL_Offset1 : 0 */
                err = iTDA18273_Write(pObj, &gTDA18273_Reg_RF_Cal_byte_1__RFCAL_Offset_Cprog1, 0x00, Bus_None);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
            }

            if(err == TM_OK)
            {
                /* RFCAL_Offset2 : 0 */
                err = iTDA18273_Write(pObj, &gTDA18273_Reg_RF_Cal_byte_1__RFCAL_Offset_Cprog2, 0x00, Bus_None);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
            }

            if(err == TM_OK)
            {
                /* RFCAL_Offset3 : 0 */
                err = iTDA18273_Write(pObj, &gTDA18273_Reg_RF_Cal_byte_1__RFCAL_Offset_Cprog3, 0x00, Bus_None);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
            }

            if(err == TM_OK)
            {
                /* RFCAL_Offset4 : 3 */
                err = iTDA18273_Write(pObj, &gTDA18273_Reg_RF_Cal_byte_2__RFCAL_Offset_Cprog4, 0x03, Bus_None);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
            }

            if(err == TM_OK)
            {
                /* RFCAL_Offset5 : 0 */
                err = iTDA18273_Write(pObj, &gTDA18273_Reg_RF_Cal_byte_2__RFCAL_Offset_Cprog5, 0x00, Bus_None);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
            }

            if(err == TM_OK)
            {
                /* RFCAL_Offset6 : 3 */
                err = iTDA18273_Write(pObj, &gTDA18273_Reg_RF_Cal_byte_2__RFCAL_Offset_Cprog6, 0x03, Bus_None);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
            }

            if(err == TM_OK)
            {
                /* RFCAL_Offset7 : 3 */
                err = iTDA18273_Write(pObj, &gTDA18273_Reg_RF_Cal_byte_2__RFCAL_Offset_Cprog7, 0x03, Bus_None);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
            }

            if(err == TM_OK)
            {
                /* RFCAL_Offset8 : 1 */
                err = iTDA18273_Write(pObj, &gTDA18273_Reg_RF_Cal_byte_3__RFCAL_Offset_Cprog8, 0x01, Bus_None);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
            }

            if(err == TM_OK)
            {
                /* Write RF_Cal_byte_1 (0x27) to RF_Cal_byte_3 (0x29) Registers */
                err = iTDA18273_WriteRegMap(pObj, gTDA18273_Reg_RF_Cal_byte_1.Address, TDA18273_REG_DATA_LEN(gTDA18273_Reg_RF_Cal_byte_1, gTDA18273_Reg_RF_Cal_byte_3));
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_WriteRegMap(0x%08X) failed.", pObj->tUnitW));
            }
        }

        if(err == TM_OK)
        {
            /* PLD_Temp_Enable: POR = 1 -> 0 */
            err = iTDA18273_Write(pObj, &gTDA18273_Reg_RFAGCs_Gain_byte_1__PLD_Temp_Enable, 0x00, Bus_NoRead);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
        }

        if(err == TM_OK)
        {
            /* Power Down Vsync Management: POR = 0 -> 1 */
            err = iTDA18273_Write(pObj, &gTDA18273_Reg_Vsync_Mgt_byte__PD_Vsync_Mgt, 0x01, Bus_NoRead);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
        }
    }

    if(err == TM_OK)
    {
        /* Set IRQ_clear */
        err = iTDA18273_Write(pObj, &gTDA18273_Reg_IRQ_clear, TDA18273_IRQ_Global|TDA18273_IRQ_HwInit|TDA18273_IRQ_IrCal, Bus_NoRead);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
    }

    if(err == TM_OK)
    {
        /* Launch tuner calibration */

        /* Set state machine (all CALs except IRCAL) and Launch it */
        if(err == TM_OK)
        {
            err = iTDA18273_SetMSM(pObj, TDA18273_MSM_HwInit, True);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_SetMSM(0x%08X, TDA18273_MSM_HwInit) failed.", pObj->tUnitW));
        }

        /* Inform that init phase has started */
        if(err == TM_OK)
        {
            pObj->eHwState = TDA18273_HwState_InitPending;
        }

        if(err == TM_OK && pObj->bIRQWaitHwInit)
        {
            /* State reached after 500 ms max */
            err = iTDA18273_WaitIRQ(pObj, 500, 10, TDA18273_IRQ_HwInit);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_WaitIRQ(0x%08X) failed.", tUnit));
        }

        if(err == TM_OK)
        {
            /* Set IRQ_clear */
            err = iTDA18273_Write(pObj, &gTDA18273_Reg_IRQ_clear, TDA18273_IRQ_Global|TDA18273_IRQ_HwInit|TDA18273_IRQ_IrCal, Bus_NoRead);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
        }

        /* Launch IRCALs after all other CALs are finished */
        if(err == TM_OK)
        {
            err = iTDA18273_SetMSM(pObj, TDA18273_MSM_IrCal, True);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_SetMSM(0x%08X, TDA18273_MSM_IrCal) failed.", pObj->tUnitW));
        }

        if(err == TM_OK && pObj->bIRQWaitHwInit)
        {
            /* State reached after 500 ms max, 10 ms step due to CAL ~ 30ms */
            err = iTDA18273_WaitIRQ(pObj, 500, 10, TDA18273_IRQ_IrCal);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_WaitIRQ(0x%08X) failed.", tUnit));
        }

        if(err == TM_OK && pObj->eHwState == TDA18273_HwState_InitPending)
        {
            pObj->eHwState = TDA18273_HwState_InitDone;
        }
    }

    _MUTEX_RELEASE(TDA18273)

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmbslTDA18273_GetIF:                                          */
/*                                                                            */
/* DESCRIPTION: Gets programmed IF.                                           */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmbslTDA18273_GetIF(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt32*         puIF    /* O: IF Frequency in hertz */
)
{
    pTDA18273Object_t   pObj = Null;
    tmErrorCode_t       err = TM_OK;

    /* Get a driver instance */
    err = iTDA18273_GetInstance(tUnit, &pObj);

    _MUTEX_ACQUIRE(TDA18273)

    tmDBGPRINTEx(DEBUGLVL_INOUT, "tmbslTDA18273_GetIF(0x%08X)", tUnit);

    /* Test parameter(s) */
    if(   pObj->StandardMode<=TDA18273_StandardMode_Unknown
        || pObj->StandardMode>=TDA18273_StandardMode_Max
        || pObj->pStandard == Null
        || puIF == Null)
    {
        err = TDA18273_ERR_BAD_PARAMETER;
    }

    if(err == TM_OK)
    {
        *puIF = pObj->uIF - pObj->pStandard->CF_Offset;
    }

    _MUTEX_RELEASE(TDA18273)

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmbslTDA18273_SetIF:                                          */
/*                                                                            */
/* DESCRIPTION: Sets programmed IF.                                           */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmbslTDA18273_SetIF(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt32          uIF     /* I: IF Frequency in hertz */
)
{
    pTDA18273Object_t   pObj = Null;
    tmErrorCode_t       err = TM_OK;

    /* Get a driver instance */
    err = iTDA18273_GetInstance(tUnit, &pObj);

    _MUTEX_ACQUIRE(TDA18273)

    tmDBGPRINTEx(DEBUGLVL_INOUT, "tmbslTDA18273_SetIF(0x%08X)", tUnit);

    /* Test parameter(s) */
    if(   pObj->StandardMode<=TDA18273_StandardMode_Unknown
        || pObj->StandardMode>=TDA18273_StandardMode_Max
        || pObj->pStandard == Null)
    {
        err = TDA18273_ERR_BAD_PARAMETER;
    }

    if(err == TM_OK)
    {
        /* Update IF in pObj structure */
        pObj->uIF = uIF + pObj->pStandard->CF_Offset;

        /* Update IF in TDA18273 registers */
        err = iTDA18273_Write(pObj, &gTDA18273_Reg_IF_Frequency_byte__IF_Freq, (UInt8)(uIF/50000), Bus_NoRead);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
    }

    _MUTEX_RELEASE(TDA18273)

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmbslTDA18273_GetCF_Offset:                                   */
/*                                                                            */
/* DESCRIPTION: Gets CF Offset.                                               */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmbslTDA18273_GetCF_Offset(
    tmUnitSelect_t  tUnit,      /* I: Unit number */
    UInt32*         puOffset    /* O: Center frequency offset in hertz */
)
{
    pTDA18273Object_t   pObj = Null;
    tmErrorCode_t       err = TM_OK;

    /* Get a driver instance */
    err = iTDA18273_GetInstance(tUnit, &pObj);

    _MUTEX_ACQUIRE(TDA18273)

    tmDBGPRINTEx(DEBUGLVL_INOUT, "tmbslTDA18273_GetCF_Offset(0x%08X)", tUnit);

        /* Test parameter(s) */
        if(   pObj->StandardMode<=TDA18273_StandardMode_Unknown
            || pObj->StandardMode>=TDA18273_StandardMode_Max
            || pObj->pStandard == Null
            || puOffset == Null)
        {
            err = TDA18273_ERR_BAD_PARAMETER;
        }

        if(err == TM_OK)
        {
            *puOffset = pObj->pStandard->CF_Offset;
        }

    _MUTEX_RELEASE(TDA18273)

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmbslTDA18273_GetLockStatus:                                  */
/*                                                                            */
/* DESCRIPTION: Gets PLL Lock Status.                                         */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmbslTDA18273_GetLockStatus(
    tmUnitSelect_t          tUnit,      /* I: Unit number */
    tmbslFrontEndState_t*   pLockStatus /* O: PLL Lock status */
)
{
    pTDA18273Object_t   pObj = Null;
    tmErrorCode_t       err = TM_OK;
    UInt8               uValue = 0, uValueLO = 0;

    /* Get a driver instance */
    err = iTDA18273_GetInstance(tUnit, &pObj);

    _MUTEX_ACQUIRE(TDA18273)

    tmDBGPRINTEx(DEBUGLVL_INOUT, "tmbslTDA18273_GetLockStatus(0x%08X)", tUnit);

    if( pLockStatus == Null )
    {
        err = TDA18273_ERR_BAD_PARAMETER;
    }

    if(err == TM_OK)
    {
        err = iTDA18273_Read(pObj, &gTDA18273_Reg_Power_state_byte_1__LO_Lock, &uValueLO, Bus_RW);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Read(0x%08X) failed.", pObj->tUnitW));

        if(err == TM_OK)
        {
            err = iTDA18273_Read(pObj, &gTDA18273_Reg_IRQ_status__IRQ_status, &uValue, Bus_RW);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Read(0x%08X) failed.", pObj->tUnitW));
        }

        if(err == TM_OK)
        {
            uValue = uValue & uValueLO;

            *pLockStatus =  (uValue)? tmbslFrontEndStateLocked : tmbslFrontEndStateNotLocked;
        }
        else
        {
            *pLockStatus = tmbslFrontEndStateUnknown;
        }
    }

    _MUTEX_RELEASE(TDA18273)

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmbslTDA18273_GetPowerLevel:                                  */
/*                                                                            */
/* DESCRIPTION: Gets HW Power Level.   1/2 steps dBµV                         */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmbslTDA18273_GetPowerLevel
(
    tmUnitSelect_t  tUnit,      /* I: Unit number */
    UInt8*          pPowerLevel /* O: Power Level in 1/2 steps dBµV */
)
{
    pTDA18273Object_t   pObj = Null;
    tmErrorCode_t       err = TM_OK;

    /* Get a driver instance */
    err = iTDA18273_GetInstance(tUnit, &pObj);

    _MUTEX_ACQUIRE(TDA18273)

    tmDBGPRINTEx(DEBUGLVL_INOUT, "tmbslTDA18273_GetPowerLevel(0x%08X)", tUnit);

    /* Test parameter(s) */
    if( pPowerLevel == Null ||
        pObj->StandardMode<=TDA18273_StandardMode_Unknown ||
        pObj->StandardMode>=TDA18273_StandardMode_Max ||
        pObj->pStandard == Null)
    {
        err = TDA18273_ERR_BAD_PARAMETER;
    }

    if(err == TM_OK)
    {
        *pPowerLevel = 0;

        /* Implement PLD CC algorithm to increase PLD read immunity to interferer */
        /*
            -PLD CC ON.
            -Wait 40ms.
            -PLD read standard.
            -PLD CC OFF. 
        */

        /* Do the algorithm only if asked and not in analog mode */
        if( (pObj->PLD_CC_algorithm == True) && isTDA18273_ANLG_STD(pObj->StandardMode) == False )
        {
            /* PLD_CC_Enable: 1 */
            err = iTDA18273_Write(pObj, &gTDA18273_Reg_RFAGCs_Gain_byte_1__PLD_CC_Enable, 0x01, Bus_NoRead);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", tUnit));

            if(err == TM_OK)
            {
                /* Wait 40 ms */
                err = iTDA18273_Wait(pObj, 40);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Wait(0x%08X) failed.", tUnit));
            }
        }

        if(err == TM_OK)
        {
            /* Read Power Level */
            err = iTDA18273_Read(pObj, &gTDA18273_Reg_Input_Power_Level_byte__Power_Level, pPowerLevel, Bus_RW);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Read(0x%08X) failed.", tUnit));
        }

        if( (err == TM_OK) && (pObj->uProgRF <= 145408000))
        {
            /* RF <= 145.408 MHz, then apply minus 3 dB */
            /* 1 step = 0.5 dB */
            if (*pPowerLevel >= 6)
                *pPowerLevel -= 6;
            else
                *pPowerLevel = 0;
        }

        /* Finish the algorithm only if asked and not in analog mode */
        if( (pObj->PLD_CC_algorithm == True) && isTDA18273_ANLG_STD(pObj->StandardMode) == False )
        {
            /* PLD_CC_Enable: 0 */
            err = iTDA18273_Write(pObj, &gTDA18273_Reg_RFAGCs_Gain_byte_1__PLD_CC_Enable, 0x00, Bus_NoRead);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", tUnit));
        }
    }

    _MUTEX_RELEASE(TDA18273)

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmbslTDA18273_SetInternalVsync:                               */
/*                                                                            */
/* DESCRIPTION: Enables or disable the internal VSYNC                         */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmbslTDA18273_SetInternalVsync(
    tmUnitSelect_t  tUnit,      /* I: Unit number */
    Bool            bEnabled    /* I: Enable of disable the internal VSYNC */
)
{
    pTDA18273Object_t   pObj = Null;
    tmErrorCode_t       err = TM_OK;

    /* Get a driver instance */
    err = iTDA18273_GetInstance(tUnit, &pObj);

    _MUTEX_ACQUIRE(TDA18273)

    tmDBGPRINTEx(DEBUGLVL_INOUT, "tmbslTDA182I4_SetInternalVsync(0x%08X)", tUnit);

    if(err == TM_OK)
    {
        err = iTDA18273_Write(pObj, &gTDA18273_Reg_Vsync_byte__Vsync_int, ((bEnabled == True) ? 1 : 0), Bus_RW);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", tUnit));
    }

    _MUTEX_RELEASE(TDA18273)

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmbslTDA18273_GetInternalVsync:                               */
/*                                                                            */
/* DESCRIPTION: Get the current status of the internal VSYNC                  */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmbslTDA18273_GetInternalVsync(
    tmUnitSelect_t  tUnit,      /* I: Unit number */
    Bool*           pbEnabled   /* O: current status of the internal VSYNC */
)
{
    pTDA18273Object_t   pObj = Null;
    tmErrorCode_t       err = TM_OK;
    UInt8               uValue;

    /* Get a driver instance */
    err = iTDA18273_GetInstance(tUnit, &pObj);

    _MUTEX_ACQUIRE(TDA18273)

    tmDBGPRINTEx(DEBUGLVL_INOUT, "tmbslTDA182I4_SetInternalVsync(0x%08X)", tUnit);

    if(pbEnabled == Null)
    {
        err = TDA18273_ERR_BAD_PARAMETER;
    }

    if(err == TM_OK)
    {
        err = iTDA18273_Read(pObj, &gTDA18273_Reg_Vsync_byte__Vsync_int, &uValue, Bus_RW);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Read(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        if (uValue == 1)
        {
            *pbEnabled = True;
        }
        else
        {
            *pbEnabled = False;
        }
    }

    _MUTEX_RELEASE(TDA18273)

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmbslTDA18273_SetPllManual:                                   */
/*                                                                            */
/* DESCRIPTION: Sets bOverridePLL flag.                                       */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmbslTDA18273_SetPllManual(
    tmUnitSelect_t  tUnit,         /* I: Unit number */
    Bool            bOverridePLL   /* I: Determine if we need to put PLL in manual mode in SetRF */
)
{
    pTDA18273Object_t   pObj = Null;
    tmErrorCode_t       err = TM_OK;

    /* Get a driver instance */
    err = iTDA18273_GetInstance(tUnit, &pObj);

    _MUTEX_ACQUIRE(TDA18273)

    tmDBGPRINTEx(DEBUGLVL_INOUT, "tmbslTDA18273_SetPllManual(0x%08X)", tUnit);

    pObj->bOverridePLL = bOverridePLL;

    _MUTEX_RELEASE(TDA18273)

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmbslTDA18273_SetIRQWait:                                     */
/*                                                                            */
/* DESCRIPTION: Sets IRQWait flag.                                            */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmbslTDA18273_SetIRQWait(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    Bool            bWait   /* I: Determine if we need to wait IRQ in driver functions */
)
{
    pTDA18273Object_t   pObj = Null;
    tmErrorCode_t       err = TM_OK;

    /* Get a driver instance */
    err = iTDA18273_GetInstance(tUnit, &pObj);

    _MUTEX_ACQUIRE(TDA18273)

    tmDBGPRINTEx(DEBUGLVL_INOUT, "tmbslTDA18273_SetIRQWait(0x%08X)", tUnit);

    pObj->bIRQWait = bWait;

    _MUTEX_RELEASE(TDA18273)

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmbslTDA18273_GetPllManual:                                   */
/*                                                                            */
/* DESCRIPTION: Gets bOverridePLL flag.                                       */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmbslTDA18273_GetPllManual(
    tmUnitSelect_t  tUnit,         /* I: Unit number */
    Bool*           pbOverridePLL  /* O: Determine if we need to put PLL in manual mode in SetRF */
)
{
    pTDA18273Object_t   pObj = Null;
    tmErrorCode_t       err = TM_OK;

    /* Get a driver instance */
    err = iTDA18273_GetInstance(tUnit, &pObj);

    _MUTEX_ACQUIRE(TDA18273)

    tmDBGPRINTEx(DEBUGLVL_INOUT, "tmbslTDA18273_GetPllManual(0x%08X)", tUnit);

    if(pbOverridePLL == Null)
    {
        err = TDA18273_ERR_BAD_PARAMETER;
    }

    if(err == TM_OK)
    {
        *pbOverridePLL = pObj->bOverridePLL;
    }

    _MUTEX_RELEASE(TDA18273)

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmbslTDA18273_GetIRQWait:                                     */
/*                                                                            */
/* DESCRIPTION: Gets IRQWait flag.                                            */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmbslTDA18273_GetIRQWait(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    Bool*           pbWait  /* O: Determine if we need to wait IRQ in driver functions */
)
{
    pTDA18273Object_t   pObj = Null;
    tmErrorCode_t       err = TM_OK;

    /* Get a driver instance */
    err = iTDA18273_GetInstance(tUnit, &pObj);

    _MUTEX_ACQUIRE(TDA18273)

    tmDBGPRINTEx(DEBUGLVL_INOUT, "tmbslTDA18273_GetIRQWait(0x%08X)", tUnit);

    if(pbWait == Null)
    {
        err = TDA18273_ERR_BAD_PARAMETER;
    }

    if(err == TM_OK)
    {
        *pbWait = pObj->bIRQWait;
    }

    _MUTEX_RELEASE(TDA18273)

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmbslTDA18273_SetIRQWaitHwInit:                               */
/*                                                                            */
/* DESCRIPTION: Sets IRQWait flag.                                            */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmbslTDA18273_SetIRQWaitHwInit(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    Bool            bWait   /* I: Determine if we need to wait IRQ in driver functions */
)
{
    pTDA18273Object_t   pObj = Null;
    tmErrorCode_t       err = TM_OK;

    /* Get a driver instance */
    err = iTDA18273_GetInstance(tUnit, &pObj);

    _MUTEX_ACQUIRE(TDA18273)

    tmDBGPRINTEx(DEBUGLVL_INOUT, "tmbslTDA18273_SetIRQWaitHwInit(0x%08X)", tUnit);

    pObj->bIRQWaitHwInit = bWait;

    _MUTEX_RELEASE(TDA18273)

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmbslTDA18273_GetIRQWaitHwInit:                               */
/*                                                                            */
/* DESCRIPTION: Gets IRQWait flag.                                            */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmbslTDA18273_GetIRQWaitHwInit(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    Bool*           pbWait  /* O: Determine if we need to wait IRQ in driver functions */
)
{
    pTDA18273Object_t   pObj = Null;
    tmErrorCode_t       err = TM_OK;

    /* Get a driver instance */
    err = iTDA18273_GetInstance(tUnit, &pObj);

    _MUTEX_ACQUIRE(TDA18273)

    tmDBGPRINTEx(DEBUGLVL_INOUT, "tmbslTDA18273_GetIRQWaitHwInit(0x%08X)", tUnit);

    if(pbWait == Null)
    {
        err = TDA18273_ERR_BAD_PARAMETER;
    }

    if(err == TM_OK)
    {
        *pbWait = pObj->bIRQWaitHwInit;
    }

    _MUTEX_RELEASE(TDA18273)

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmbslTDA18273_GetIRQ:                                         */
/*                                                                            */
/* DESCRIPTION: Gets IRQ status.                                              */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmbslTDA18273_GetIRQ(
    tmUnitSelect_t  tUnit   /* I: Unit number */,
    Bool*           pbIRQ   /* O: IRQ triggered */
)
{
    pTDA18273Object_t   pObj = Null;
    tmErrorCode_t       err = TM_OK;
    UInt8               uValue = 0;

    /* Get a driver instance */
    err = iTDA18273_GetInstance(tUnit, &pObj);

    _MUTEX_ACQUIRE(TDA18273)

    tmDBGPRINTEx(DEBUGLVL_INOUT, "tmbslTDA18273_GetIRQ(0x%08X)", tUnit);

    if(pbIRQ == Null)
    {
        err = TDA18273_ERR_BAD_PARAMETER;
    }

    if(err == TM_OK)
    {
        *pbIRQ = 0;

        err = iTDA18273_Read(pObj, &gTDA18273_Reg_IRQ_status__IRQ_status, &uValue, Bus_RW);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Read(0x%08X) failed.", pObj->tUnitW));

        if(err == TM_OK)
        {
            *pbIRQ = uValue;
        }
    }

    _MUTEX_RELEASE(TDA18273)

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmbslTDA18273_WaitIRQ:                                        */
/*                                                                            */
/* DESCRIPTION: Waits for the IRQ to raise.                                   */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmbslTDA18273_WaitIRQ(
    tmUnitSelect_t  tUnit,      /* I: Unit number */
    UInt32          timeOut,    /* I: timeOut for IRQ wait */
    UInt32          waitStep,   /* I: wait step */
    UInt8           irqStatus   /* I: IRQs to wait */
)
{
    pTDA18273Object_t   pObj = Null;
    tmErrorCode_t       err = TM_OK;

    /* Get a driver instance */
    err = iTDA18273_GetInstance(tUnit, &pObj);

    _MUTEX_ACQUIRE(TDA18273)

    tmDBGPRINTEx(DEBUGLVL_INOUT, "tmbslTDA18273_WaitIRQ(0x%08X)", tUnit);

    err = iTDA18273_WaitIRQ(pObj, timeOut, waitStep, irqStatus);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_WaitIRQ(0x%08X) failed.", tUnit));

    _MUTEX_RELEASE(TDA18273)

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmbslTDA18273_GetXtalCal_End:                                 */
/*                                                                            */
/* DESCRIPTION: Gets XtalCal_End status.                                      */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmbslTDA18273_GetXtalCal_End(
    tmUnitSelect_t  tUnit           /* I: Unit number */,
    Bool*           pbXtalCal_End   /* O: XtalCal_End triggered */
)
{
    pTDA18273Object_t   pObj = Null;
    tmErrorCode_t       err = TM_OK;
    UInt8               uValue = 0;

    /* Get a driver instance */
    err = iTDA18273_GetInstance(tUnit, &pObj);

    _MUTEX_ACQUIRE(TDA18273)

    tmDBGPRINTEx(DEBUGLVL_INOUT, "tmbslTDA18273_GetXtalCal_End(0x%08X)", tUnit);

    if(pbXtalCal_End == Null)
    {
        err = TDA18273_ERR_BAD_PARAMETER;
    }

    if(err == TM_OK)
    {
        *pbXtalCal_End = 0;

        err = iTDA18273_Read(pObj, &gTDA18273_Reg_IRQ_status__MSM_XtalCal_End, &uValue, Bus_RW);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Read(0x%08X) failed.", pObj->tUnitW));

        if(err == TM_OK)
        {
            *pbXtalCal_End = uValue;
        }
    }

    _MUTEX_RELEASE(TDA18273)

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmbslTDA18273_SetFineRF:                                      */
/*                                                                            */
/* DESCRIPTION: Fine tunes RF with given step.                                */
/*              (tmbslTDA18273_SetRF must be called before calling this API)  */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmbslTDA18273_SetFineRF(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    Int8            step    /* I: step (-1, +1) */
)
{
    pTDA18273Object_t   pObj = Null;
    tmErrorCode_t       err = TM_OK;

    /* LO wanted = RF wanted + IF in KHz */
    UInt32 LO = 0;

    /* PostDiv */
    UInt8 PostDiv = 0;
    UInt8 LOPostDiv = 0;

    /* Prescaler */
    UInt8 Prescaler = 0;

    /* Algorithm that calculates N, K */
    UInt32 N_int = 0;
    UInt32 K_int = 0;

    UInt8 i = 0;

    /* Get a driver instance */
    err = iTDA18273_GetInstance(tUnit, &pObj);

    _MUTEX_ACQUIRE(TDA18273)

    tmDBGPRINTEx(DEBUGLVL_INOUT, "tmbslTDA18273_SetFineRF(0x%08X)", tUnit);

    /* Test parameter(s) */
    if(   pObj->StandardMode<=TDA18273_StandardMode_Unknown
        || pObj->StandardMode>=TDA18273_StandardMode_Max
        || pObj->pStandard == Null)
    {
        err = TDA18273_ERR_STD_NOT_SET;
    }

    if(err == TM_OK)
    {
        /* Check if Hw is ready to operate */
        err = iTDA18273_CheckHwState(pObj, TDA18273_HwStateCaller_SetFineRF);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_CheckHwState(0x%08X) failed.", pObj->tUnitW));
    }

    if(err == TM_OK)
    {
        /* Write the offset into 4 equal steps of 15.625 KHz = 62.5 KHz*/
        for (i=0; i < 4; i++)
        {
            /* Calculate wanted LO = RF + IF */
            pObj->uRF += step*15625;
            pObj->uProgRF += step*15625;
            LO = (pObj->uRF + pObj->uIF)/1000;

            /* Don't touch on Prescaler and PostDiv programmed during setRF */
            err = iTDA18273_Read(pObj, &gTDA18273_Reg_Main_Post_Divider_byte__LOPostDiv, &LOPostDiv, Bus_RW);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Read(0x%08X) failed.", pObj->tUnitW));

            if(err == TM_OK)
            {
                err = iTDA18273_Read(pObj, &gTDA18273_Reg_Main_Post_Divider_byte__LOPresc, &Prescaler, Bus_RW);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Read(0x%08X) failed.", pObj->tUnitW));
            }

            if (err == TM_OK)
            {
                /* Decode PostDiv */
                switch(LOPostDiv)
                {
                    case 1:
                        PostDiv = 1;
                        break;
                    case 2:
                        PostDiv = 2;
                        break;
                    case 3:
                        PostDiv = 4;
                        break;
                    case 4:
                        PostDiv = 8;
                        break;
                    case 5:
                        PostDiv = 16;
                        break;
                    default:
                        err = TDA18273_ERR_BAD_PARAMETER;
                        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmbslTDA18273_SetFineRF(0x%08X) LO_PostDiv value is wrong.", tUnit));
                        break;
                }

                /* Calculate N & K values of the PLL */
                err = iTDA18273_CalculateNIntKInt(LO, PostDiv, Prescaler, &N_int, &K_int);

                /* Affect registers */
                if(err == TM_OK)
                {
                    err = iTDA18273_Write(pObj, &gTDA18273_Reg_Sigma_delta_byte_4__LO_Frac_0, (UInt8)(K_int & 0xFF), Bus_None);
                    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
                }

                if(err == TM_OK)
                {
                    err = iTDA18273_Write(pObj, &gTDA18273_Reg_Sigma_delta_byte_3__LO_Frac_1, (UInt8)((K_int >> 8) & 0xFF), Bus_None);
                    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
                }

                if(err == TM_OK)
                {
                    err = iTDA18273_Write(pObj, &gTDA18273_Reg_Sigma_delta_byte_2__LO_Frac_2, (UInt8)((K_int >> 16) & 0xFF), Bus_None);
                    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
                }

                if(err == TM_OK)
                {
                    err = iTDA18273_Write(pObj, &gTDA18273_Reg_Sigma_delta_byte_1__LO_Int, (UInt8)(N_int & 0xFF), Bus_None);
                    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
                }

                if(err == TM_OK)
                {
                    /* Force manual selection mode : 0x3 at @0x56 */
                    err = iTDA18273_Write(pObj, &gTDA18273_Reg_Sigma_delta_byte_5__N_K_correct_manual, 0x01, Bus_None);
                    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));

                    if(err == TM_OK)
                    {
                        /* Force manual selection mode : 0x3 at @0x56 */
                        err = iTDA18273_Write(pObj, &gTDA18273_Reg_Sigma_delta_byte_5__LO_Calc_Disable, 0x01, Bus_None);
                        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
                    }
                }

                /* Set the new PLL values */
                if(err == TM_OK)
                {
                    /* Write bytes Sigma_delta_byte_1 (0x52) to Sigma_delta_byte_5 (0x56) */
                    err = iTDA18273_WriteRegMap(pObj, gTDA18273_Reg_Sigma_delta_byte_1.Address, TDA18273_REG_DATA_LEN(gTDA18273_Reg_Sigma_delta_byte_1, gTDA18273_Reg_Sigma_delta_byte_5));
                    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
                }
            }
        }

        /* Update driver state machine */
        pObj->eHwState = TDA18273_HwState_SetFineRFDone;
    }

    _MUTEX_RELEASE(TDA18273)

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmbslTDA18273_Write                                           */
/*                                                                            */
/* DESCRIPTION: Writes in TDA18273 hardware                                   */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmbslTDA18273_Write(
    tmUnitSelect_t              tUnit,      /* I: Unit number */
    const TDA18273_BitField_t*  pBitField,  /* I: Bitfield structure */
    UInt8                       uData,      /* I: Data to write */
    tmbslFrontEndBusAccess_t                eBusAccess  /* I: Access to bus */
)
{
    pTDA18273Object_t   pObj = Null;
    tmErrorCode_t       err = TM_OK;

    /* Get a driver instance */
    err = iTDA18273_GetInstance(tUnit, &pObj);

    _MUTEX_ACQUIRE(TDA18273)

    tmDBGPRINTEx(DEBUGLVL_INOUT, "tmbslTDA18273_Write(0x%08X)", tUnit);

    err = iTDA18273_Write(pObj, pBitField, uData, eBusAccess);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));

    _MUTEX_RELEASE(TDA18273)

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmbslTDA18273_Read                                            */
/*                                                                            */
/* DESCRIPTION: Reads in TDA18273 hardware                                    */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmbslTDA18273_Read(
    tmUnitSelect_t              tUnit,      /* I: Unit number */
    const TDA18273_BitField_t*  pBitField,  /* I: Bitfield structure */
    UInt8*                      puData,     /* I: Data to read */
    tmbslFrontEndBusAccess_t                eBusAccess  /* I: Access to bus */
)
{
    pTDA18273Object_t   pObj = Null;
    tmErrorCode_t       err = TM_OK;

    /* Get a driver instance */
    err = iTDA18273_GetInstance(tUnit, &pObj);

    _MUTEX_ACQUIRE(TDA18273)

    tmDBGPRINTEx(DEBUGLVL_INOUT, "tmbslTDA18273_Read(0x%08X)", tUnit);

    err = iTDA18273_Read(pObj, pBitField, puData, eBusAccess);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Read(0x%08X) failed.", pObj->tUnitW));

    _MUTEX_RELEASE(TDA18273)

    return err;
}

/*============================================================================*/
/* Internal functions:                                                        */
/*============================================================================*/

/*============================================================================*/
/* FUNCTION:    iTDA18273_CheckHwState                                        */
/*                                                                            */
/* DESCRIPTION: Checks if Hw is ready to operate.                             */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
iTDA18273_CheckHwState(
    pTDA18273Object_t       pObj,   /* I: Driver object */
    TDA18273HwStateCaller_t caller  /* I: Caller API */
)
{
    tmErrorCode_t   err = TM_OK;

    tmDBGPRINTEx(DEBUGLVL_INOUT, "iTDA18273_CheckHwState(0x%08X)", pObj->tUnitW);

    switch(pObj->eHwState)
    {
        case TDA18273_HwState_InitNotDone:
            switch(caller)
            {
                case TDA18273_HwStateCaller_SetPower:
                    break;

                default:
                    err = TDA18273_ERR_NOT_INITIALIZED;
                    break;
            }
            break;

        case TDA18273_HwState_InitDone:
            switch(caller)
            {
                case TDA18273_HwStateCaller_SetRF:
                case TDA18273_HwStateCaller_SetFineRF:
                    /* SetStandardMode API must be called before calling SetRF and SetFineRF */
                    err = TDA18273_ERR_STD_NOT_SET;
                    break;

                default:
                    break;
            }
            break;

        case TDA18273_HwState_SetStdDone:
            switch(caller)
            {
                case TDA18273_HwStateCaller_SetFineRF:
                    /* SetRF API must be called before calling SetFineRF */
                    err = TDA18273_ERR_RF_NOT_SET;
                    break;

                default:
                    break;
            }
            break;

        case TDA18273_HwState_SetRFDone:
        case TDA18273_HwState_SetFineRFDone:
            break;

        case TDA18273_HwState_InitPending:
            /* Hw Init pending. Check if IRQ triggered. No wait. */
            err = iTDA18273_WaitIRQ(pObj, 1, 1, TDA18273_IRQ_HwInit);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_WaitIRQ(0x%08X) failed.", pObj->tUnitW));

            if(err == TM_OK)
            {
                /* Init Done. Call iTDA18273_CheckHwState to check after Hw Initialization. */
                err = iTDA18273_CheckHwState(pObj, caller);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_CheckHwState(0x%08X) failed.", pObj->tUnitW));
                break;
            }
        default:
            err = TDA18273_ERR_NOT_READY;
            break;
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    iTDA18273_CheckCalcPLL                                        */
/*                                                                            */
/* DESCRIPTION: Checks if CalcPLL Algo is enabled. Enable it if not.          */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
iTDA18273_CheckCalcPLL(
    pTDA18273Object_t   pObj    /* I: Driver object */
)
{
    tmErrorCode_t   err = TM_OK;
    UInt8           uValue = 0;

    tmDBGPRINTEx(DEBUGLVL_INOUT, "iTDA18273_CheckCalcPLL(0x%08X)", pObj->tUnitW);

    /* Check if Calc_PLL algorithm is in automatic mode */
    err = iTDA18273_Read(pObj, &gTDA18273_Reg_Sigma_delta_byte_5__LO_Calc_Disable, &uValue, Bus_None);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Read(0x%08X) failed.", pObj->tUnitW));

    if(err == TM_OK && uValue != 0x00)
    {
        /* Enable Calc_PLL algorithm by putting PLL in automatic mode */
        err = iTDA18273_Write(pObj, &gTDA18273_Reg_Sigma_delta_byte_5__N_K_correct_manual, 0x00, Bus_None);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));

        if(err == TM_OK)
        {
            err = iTDA18273_Write(pObj, &gTDA18273_Reg_Sigma_delta_byte_5__LO_Calc_Disable, 0x00, Bus_NoRead);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
        }
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    iTDA18273_SetLLPowerState                                     */
/*                                                                            */
/* DESCRIPTION: Sets the power state.                                         */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/*============================================================================*/
static tmErrorCode_t
iTDA18273_SetLLPowerState(
    pTDA18273Object_t       pObj,       /* I: Driver object */
    TDA18273PowerState_t    powerState  /* I: Power state of TDA18273 */
)
{
    tmErrorCode_t   err = TM_OK;
    UInt8           uValue = 0;

    tmDBGPRINTEx(DEBUGLVL_INOUT, "iTDA18273_SetLLPowerState(0x%08X)", pObj->tUnitW);

    if(err == TM_OK)
    {
        /* Check if Hw is ready to operate */
        err = iTDA18273_CheckHwState(pObj, TDA18273_HwStateCaller_SetPower);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_CheckHwState(0x%08X) failed.", pObj->tUnitW));
    }

    if(err == TM_OK)
    {
        switch(powerState)
        {
            case TDA18273_PowerNormalMode:
                /* If we come from any standby mode, then power on the IC with LNA off */
                /* Then powering on LNA with the minimal gain on AGC1 to avoid glitches at RF input will */
                /* be done during SetRF */
                if(pObj->curLLPowerState != TDA18273_PowerNormalMode)
                {
                    /* Workaround to limit the spurs occurence on RF input, do it before entering normal mode */
                    /* PD LNA */
                    err = iTDA18273_FirstPassLNAPower(pObj);
                    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_FirstPassLNAPower(0x%08X) failed.", pObj->tUnitW));  
                }

                err = iTDA18273_Write(pObj, &gTDA18273_Reg_Power_state_byte_2, TDA18273_SM_NONE, Bus_RW);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));

                if(err == TM_OK)
                {
                    /* Set digital clock mode to sub-LO if normal mode is entered */
                    err = iTDA18273_Write(pObj, &gTDA18273_Reg_Reference_Byte__Digital_Clock_Mode, 0x03, Bus_RW);
                    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
                }

                /* Reset uValue to use it as a flag for below test */
                uValue = 0;
                break;

            case TDA18273_PowerStandbyWithXtalOn:
                uValue = TDA18273_SM;
                break;

            case TDA18273_PowerStandby:
            default:
                /* power state not supported */
                uValue = TDA18273_SM|TDA18273_SM_XT;
                break;
        }

        if(err == TM_OK && uValue!=0)
        {
            /* Set digital clock mode to 16 Mhz before entering standby */
            err = iTDA18273_Write(pObj, &gTDA18273_Reg_Reference_Byte__Digital_Clock_Mode, 0x00, Bus_RW);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));

            if(err == TM_OK)
            {
                err = iTDA18273_Write(pObj, &gTDA18273_Reg_Power_state_byte_2, uValue, Bus_RW);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
            }
        }

        if(err == TM_OK)
        {
            /* Store low-level power state in driver instance */
            pObj->curLLPowerState = powerState;
        }
    }

    return err;
}


/*============================================================================*/
/* FUNCTION:    iTDA18273_SetRF:                                              */
/*                                                                            */
/* DESCRIPTION: Tunes to a RF.                                                */
/*                                                                            */
/* RETURN:      TM_OK                                                         */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
iTDA18273_SetRF(
    pTDA18273Object_t   pObj    /* I: Driver object */
)
{
    tmErrorCode_t   err = TM_OK;

    tmDBGPRINTEx(DEBUGLVL_INOUT, "iTDA18273_SetRF(0x%08X)", pObj->tUnitW);

    if(pObj->curPowerState != tmPowerOn)
    {
        /* Set power state ON */
        err = iTDA18273_SetLLPowerState(pObj, TDA18273_PowerNormalMode);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_SetLLPowerState(0x%08X, PowerNormalMode) failed.", pObj->tUnitW));
    }

    /* Workaround to limit the spurs occurence on RF input */
    /* Check if LNA is powered down, if yes, power it up */
    if(err == TM_OK)
    {
        pObj->curPowerState = tmPowerOn;

        err = iTDA18273_LastPassLNAPower(pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_LastPassLNAPower(0x%08X) failed.", pObj->tUnitW));
    }

    if(err == TM_OK)
    {
        /* Check if Calc_PLL algorithm is in automatic mode */
        err = iTDA18273_CheckCalcPLL(pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_CheckCalcPLL(0x%08X) failed.", pObj->tUnitW));
    }

    if(err == TM_OK)
    {
        /* Setting Bandsplit parameters */
        err = iTDA18273_OverrideBandsplit(pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_OverrideBandsplit(0x%08X) failed.", pObj->tUnitW));
    }

    if(err == TM_OK)
    {
        /* Implement PLD read immunity against interferers: first pass before channel change */
        err = iTDA18273_FirstPassPLD_CC(pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_FirstPassPLD_CC(0x%08X) failed.", pObj->tUnitW));
    }

    if(err == TM_OK)
    {
        /* Set RF frequency */
        err = iTDA18273_SetRF_Freq(pObj, pObj->uProgRF);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_SetRF_Freq(0x%08X) failed.", pObj->tUnitW));
    }

    /* Implement PLD read immunity against interferers: last pass after channel change */
    if(err == TM_OK)
    {
        err = iTDA18273_LastPassPLD_CC(pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_LastPassPLD_CC(0x%08X) failed.", pObj->tUnitW));
    }

    /* Bypass ROM settings for wireless filters */
    if(err == TM_OK)
    {
        err = iTDA18273_OverrideWireless(pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_OverrideWireless(0x%08X) failed.", pObj->tUnitW));
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    iTDA18273_SetRF_Freq                                          */
/*                                                                            */
/* DESCRIPTION: Sets Tuner Frequency registers.                               */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
iTDA18273_SetRF_Freq(
    pTDA18273Object_t   pObj,   /* I: Driver object */
    UInt32              uRF     /* I: Wanted frequency */
)
{
    tmErrorCode_t   err = TM_OK;
    UInt32          uRFLocal = 0;

    tmDBGPRINTEx(DEBUGLVL_INOUT, "iTDA18273_SetRF_Freq(0x%08X)", pObj->tUnitW);

	/* Set the proper settings depending on the standard & RF frequency */

    /****************************************************************/
    /* AGC TOP Settings                                             */
    /****************************************************************/
	if(err == TM_OK)
    {
        /* Set AGC3 RF AGC Top */
        if (uRF < pObj->pStandard->Freq_Start_LTE)
        {
			err = iTDA18273_Write(pObj, &gTDA18273_Reg_RF_AGC_byte__RFAGC_Top, pObj->pStandard->AGC3_TOP_I2C_Low_Band, Bus_RW);
			tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
		}
		else
		{
			err = iTDA18273_Write(pObj, &gTDA18273_Reg_RF_AGC_byte__RFAGC_Top, pObj->pStandard->AGC3_TOP_I2C_High_Band, Bus_RW);
			tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
		}
    }

    if(err == TM_OK)
    {
        /* Set AGC3 Adapt TOP */
		if (uRF < pObj->pStandard->Freq_Start_LTE)
        {
			err = iTDA18273_Write(pObj, &gTDA18273_Reg_RF_AGC_byte__RFAGC_Adapt_TOP, pObj->pStandard->AGC3_Adapt_TOP_Low_Band, Bus_NoRead);
			tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
		}
		else
		{
			err = iTDA18273_Write(pObj, &gTDA18273_Reg_RF_AGC_byte__RFAGC_Adapt_TOP, pObj->pStandard->AGC3_Adapt_TOP_High_Band, Bus_NoRead);
			tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
		}
    }

    /* Set IRQ_clear */
    err = iTDA18273_Write(pObj, &gTDA18273_Reg_IRQ_clear, TDA18273_IRQ_Global|TDA18273_IRQ_SetRF, Bus_NoRead);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));

    /* Set RF */
    uRFLocal = (uRF + 500) / 1000;

    if(err == TM_OK)
    {
        err = iTDA18273_Write(pObj, &gTDA18273_Reg_RF_Frequency_byte_1__RF_Freq_1, (UInt8)((uRFLocal & 0x00FF0000) >> 16), Bus_None);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
    }

    if(err == TM_OK)
    {
        err = iTDA18273_Write(pObj, &gTDA18273_Reg_RF_Frequency_byte_2__RF_Freq_2, (UInt8)((uRFLocal & 0x0000FF00) >> 8), Bus_None);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
    }

    if(err == TM_OK)
    {
        err = iTDA18273_Write(pObj, &gTDA18273_Reg_RF_Frequency_byte_3__RF_Freq_3, (UInt8)(uRFLocal & 0x000000FF), Bus_None);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
    }

    if(err == TM_OK)
    {
        err = iTDA18273_WriteRegMap(pObj, gTDA18273_Reg_RF_Frequency_byte_1.Address, TDA18273_REG_DATA_LEN(gTDA18273_Reg_RF_Frequency_byte_1, gTDA18273_Reg_RF_Frequency_byte_3));
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_WriteRegMap(0x%08X) failed.", pObj->tUnitW));
    }

    if(err == TM_OK)
    {
        /* Set state machine and Launch it */
        err = iTDA18273_SetMSM(pObj, TDA18273_MSM_SetRF, True);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_SetMSM(0x%08X, TDA18273_MSM_SetRF) failed.", pObj->tUnitW));
    }

    if(err == TM_OK && pObj->bIRQWait)
    {
        /* Wait for IRQ to trigger */
        err = iTDA18273_WaitIRQ(pObj, 50, 5, TDA18273_IRQ_SetRF);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_WaitIRQ(0x%08X, 50, 5, TDA18273_IRQ_SetRF) failed.", pObj->tUnitW));
    }

    if(err == TM_OK && pObj->bOverridePLL)
    {
        /* Override the calculated PLL to get the best margin in case fine tuning is used */
        /* which means set the PLL in manual mode that provides the best occurence of LO tuning (+-2 MHz) */
        /* without touching PostDiv and Prescaler */
        err = iTDA18273_SetPLL(pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_SetPLL failed.", pObj->tUnitW));
    }

    if(err == TM_OK)
    {
        /* Override ICP */
        err = iTDA18273_OverrideICP(pObj, pObj->uProgRF);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_OverrideICP(0x%08X) failed.", pObj->tUnitW));
    }

    if(err == TM_OK)
    {
        /* Override Digital Clock */
        err = iTDA18273_OverrideDigitalClock(pObj, pObj->uProgRF);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_OverrideDigitalClock(0x%08X) failed.", pObj->tUnitW));
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    iTDA18273_OverrideDigitalClock                                */
/*                                                                            */
/* DESCRIPTION: Overrides Digital clock.                                      */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/*============================================================================*/
static tmErrorCode_t
iTDA18273_OverrideDigitalClock(
    pTDA18273Object_t   pObj,   /* I: Driver object */
    UInt32              uRF     /* I: Wanted RF */
)
{
    tmErrorCode_t   err = TM_OK;
    UInt8           uDigClock = 0;
    UInt8           uPrevDigClock = 0;
    UInt8           uProgIF = 0;

    /* LO < 55 MHz then Digital Clock set to 16 MHz else subLO */

    tmDBGPRINTEx(DEBUGLVL_INOUT, "iTDA18273_OverrideDigitalClock(0x%08X)", pObj->tUnitW);

    /* Read Current IF */
    err = iTDA18273_Read(pObj, &gTDA18273_Reg_IF_Frequency_byte__IF_Freq, &uProgIF, Bus_NoWrite);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Read(0x%08X) failed.", pObj->tUnitW));

    /* Read Current Digital Clock */
    err = iTDA18273_Read(pObj, &gTDA18273_Reg_Reference_Byte__Digital_Clock_Mode, &uPrevDigClock, Bus_NoWrite);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Read(0x%08X) failed.", pObj->tUnitW));

    /* LO = RF + IF */
    if ((uRF + (uProgIF*50000)) < 55000000)
    {
        uDigClock = 0; /* '00' = 16 MHz */
    }
    else
    {
        uDigClock = 3; /* '11' = subLO */
    }

    if(err == TM_OK && (uPrevDigClock != uDigClock) )
    {
        /* Set Digital Clock bits */
        err = iTDA18273_Write(pObj, &gTDA18273_Reg_Reference_Byte__Digital_Clock_Mode, uDigClock, Bus_NoRead);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    iTDA18273_OverrideICP                                         */
/*                                                                            */
/* DESCRIPTION: Overrides ICP.                                                */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/*============================================================================*/
static tmErrorCode_t
iTDA18273_OverrideICP(
    pTDA18273Object_t   pObj,   /* I: Driver object */
    UInt32              uRF     /* I: Wanted frequency */
)
{
    tmErrorCode_t   err = TM_OK;
    UInt32          uIF = 0;
    UInt8           ProgIF = 0;
    UInt8           LOPostdiv = 0;
    UInt8           LOPrescaler = 0;
    UInt32          FVCO = 0;
    UInt8           uICPBypass = 0;
    UInt8           ICP = 0;
    UInt8           uPrevICP = 0;

    tmDBGPRINTEx(DEBUGLVL_INOUT, "iTDA18273_OverrideICP(0x%08X)", pObj->tUnitW);

    /*
    if fvco<6352 MHz ==> icp = 150µ (register = 01b)
    if fvco<6592 MHz ==> icp = 300µ (register = 10b)
    500µA elsewhere (register 00b)

    Reminder : fvco = postdiv*presc*(RFfreq+IFfreq) 
    */

    /* Read PostDiv et Prescaler */
    err = iTDA18273_Read(pObj, &gTDA18273_Reg_Main_Post_Divider_byte, &LOPostdiv, Bus_RW);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Read(0x%08X) failed.", pObj->tUnitW));

    if(err == TM_OK)
    {
        /* PostDiv */
        err = iTDA18273_Read(pObj, &gTDA18273_Reg_Main_Post_Divider_byte__LOPostDiv, &LOPostdiv, Bus_NoRead);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Read(0x%08X) failed.", pObj->tUnitW));

        if(err == TM_OK)
        {
            /* Prescaler */
            err = iTDA18273_Read(pObj, &gTDA18273_Reg_Main_Post_Divider_byte__LOPresc, &LOPrescaler, Bus_NoRead);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Read(0x%08X) failed.", pObj->tUnitW));
        }

        if(err == TM_OK)
        {
            /* IF */
            err = iTDA18273_Read(pObj, &gTDA18273_Reg_IF_Frequency_byte__IF_Freq, &ProgIF, Bus_NoRead);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Read(0x%08X) failed.", pObj->tUnitW));
        }

        if(err == TM_OK)
        {
            /* Decode IF */
            uIF = ProgIF*50000;
            
            /* Decode PostDiv */
            switch(LOPostdiv)
            {
                case 1:
                    LOPostdiv = 1;
                    break;
                case 2:
                    LOPostdiv = 2;
                    break;
                case 3:
                    LOPostdiv = 4;
                    break;
                case 4:
                    LOPostdiv = 8;
                    break;
                case 5:
                    LOPostdiv = 16;
                    break;
                default:
                    err = TDA18273_ERR_BAD_PARAMETER;
                    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_OverrideICP(0x%08X) LOPostDiv value is wrong.", pObj->tUnitW));
                    break;
            }
        }
        if(err == TM_OK)
        {
            /* Calculate FVCO in MHz*/
            FVCO = LOPostdiv * LOPrescaler * ((uRF + uIF) / 1000000);

            /* Set correct ICP */

            if(FVCO < 6352)
            {
                /* Set ICP to 01 (= 150)*/
                ICP = 0x01;
            }
            else if(FVCO < 6592)
            {
                /* Set ICP to 10 (= 300)*/
                ICP = 0x02;
            }
            else
            {
                /* Set ICP to 00 (= 500)*/
                ICP = 0x00;
            }


            /* Get ICP_bypass bit */
            err = iTDA18273_Read(pObj, &gTDA18273_Reg_Charge_pump_byte__ICP_Bypass, &uICPBypass, Bus_RW);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Read(0x%08X) failed.", pObj->tUnitW));

            if(err == TM_OK)
            {
                /* Get ICP */
                err = iTDA18273_Read(pObj, &gTDA18273_Reg_Charge_pump_byte__ICP, &uPrevICP, Bus_NoRead);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Read(0x%08X) failed.", pObj->tUnitW));
            }

            if(err == TM_OK && (uICPBypass == False || uPrevICP != ICP) )
            {
                /* Set ICP_bypass bit */
                err = iTDA18273_Write(pObj, &gTDA18273_Reg_Charge_pump_byte__ICP_Bypass, 0x01, Bus_None);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));

                if(err == TM_OK)
                {
                    /* Set ICP */
                    err = iTDA18273_Write(pObj, &gTDA18273_Reg_Charge_pump_byte__ICP, ICP, Bus_None);
                    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
                }

                if(err == TM_OK)
                {
                    /* Write Charge_pump_byte register */
                    err = iTDA18273_WriteRegMap(pObj, gTDA18273_Reg_Charge_pump_byte.Address, 1);
                    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_WriteRegMap(0x%08X) failed.", pObj->tUnitW));
                }
            }
        }
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    iTDA18273_OverrideBandsplit                                   */
/*                                                                            */
/* DESCRIPTION: Overrides Bandsplit settings.                                 */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/*============================================================================*/
static tmErrorCode_t
iTDA18273_OverrideBandsplit(
    pTDA18273Object_t   pObj    /* I: Driver object */
)
{
    tmErrorCode_t   err = TM_OK;
    UInt8           Bandsplit = 0;
    UInt8           uPrevPSM_Bandsplit_Filter = 0;
    UInt8           PSM_Bandsplit_Filter = 0;

    tmDBGPRINTEx(DEBUGLVL_INOUT, "iTDA18273_OverrideBandsplit(0x%08X)", pObj->tUnitW);

    /* Setting PSM bandsplit at -3.9 mA for some RF frequencies */

    err = iTDA18273_Read(pObj, &gTDA18273_Reg_Bandsplit_Filter_byte__Bandsplit_Filter_SubBand, &Bandsplit, Bus_None);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Read(0x%08X) failed.", pObj->tUnitW));

    if(err == TM_OK)
    {
        err = iTDA18273_Read(pObj, &gTDA18273_Reg_PowerSavingMode__PSM_Bandsplit_Filter, &uPrevPSM_Bandsplit_Filter, Bus_None);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Read(0x%08X) failed.", pObj->tUnitW));
    }

    if(err == TM_OK)
    {
        switch(Bandsplit)
        {
        default:
        case 0:
            /* LPF0 133MHz - LPF1 206MHz - HPF0 422MHz */
            if(pObj->uProgRF < 133000000)
            {
                /* Set PSM bandsplit at -3.9 mA */
                PSM_Bandsplit_Filter = 0x03;
            }
            else
            {
                /* Set PSM bandsplit at nominal */
                PSM_Bandsplit_Filter = 0x02;
            }
            break;

        case 1:
            /* LPF0 139MHz - LPF1 218MHz - HPF0 446MHz */
            if(pObj->uProgRF < 139000000)
            {
                /* Set PSM bandsplit at -3.9 mA */
                PSM_Bandsplit_Filter = 0x03;
            }
            else
            {
                /* Set PSM bandsplit at nominal */
                PSM_Bandsplit_Filter = 0x02;
            }
            break;

        case 2:
            /* LPF0 145MHz - LPF1 230MHz - HPF0 470MHz */
            if(pObj->uProgRF < 145000000)
            {
                /* Set PSM bandsplit at -3.9 mA */
                PSM_Bandsplit_Filter = 0x03;
            }
            else
            {
                /* Set PSM bandsplit at nominal */
                PSM_Bandsplit_Filter = 0x02;
            }
            break;

        case 3:
            /* LPF0 151MHz - LPF1 242MHz - HPF0 494MHz */
            if(pObj->uProgRF < 151000000)
            {
                /* Set PSM bandsplit at -3.9 mA */
                PSM_Bandsplit_Filter = 0x03;
            }
            else
            {
                /* Set PSM bandsplit at nominal */
                PSM_Bandsplit_Filter = 0x02;
            }
            break;
        }

        if(uPrevPSM_Bandsplit_Filter != PSM_Bandsplit_Filter)
        {
            /* Write PSM bandsplit */
            err = iTDA18273_Write(pObj, &gTDA18273_Reg_PowerSavingMode__PSM_Bandsplit_Filter, PSM_Bandsplit_Filter, Bus_NoRead);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
        }
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    iTDA18273_OverrideWireless                                    */
/*                                                                            */
/* DESCRIPTION: Overrides Wireless settings.                                  */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/*============================================================================*/
static tmErrorCode_t
iTDA18273_OverrideWireless(
    pTDA18273Object_t   pObj    /* I: Driver object */
)
{
    tmErrorCode_t   err = TM_OK;
    UInt8           uPrevW_Filter_byte = 0;
    UInt8           uW_Filter_byte = 0;
	UInt8           W_Filter_enable = 0;
    UInt8           W_Filter = 0;
    UInt8           W_Filter_Bypass = 0;
    UInt8           W_Filter_Offset = 0;

    tmDBGPRINTEx(DEBUGLVL_INOUT, "iTDA18273_OverrideWireless(0x%08X)", pObj->tUnitW);

	/* For RF frequency in the range [562.8MHz, 720.4MHz] (boundary frequencies included) and Standard DVB-T/T2 1.7MHz, 6MHz, 7MHz & 8MHz */
	/* the wireless filter shall be disabled (W_Filter_enable bit = 0) */
	if (
		(
		 (pObj->StandardMode == TDA18273_DVBT_1_7MHz) ||
		 (pObj->StandardMode == TDA18273_DVBT_6MHz) ||
		 (pObj->StandardMode == TDA18273_DVBT_7MHz) ||
		 (pObj->StandardMode == TDA18273_DVBT_8MHz)
		) &&
		(
		 (pObj->uProgRF >= 562800000) && (pObj->uProgRF <= 720400000)
		)
	   )
	{
		/* Disable Wireless filter */
		W_Filter_enable = 0;
	}
	else
	{
		/* Enable Wireless filter */
		W_Filter_enable = 1;
	}

    /* Bypass ROM for wireless filters */
    /* WF7 = 1.7GHz - 1.98GHz */
    /* WF8 = 1.98GHz - 2.1GHz */
    /* WF9 = 2.1GHz - 2.4GHz */
    /* For all frequencies requiring WF7 and WF8, add -8% shift */
    /* For all frequencies requiring WF9, change to WF8 and add +4% shift */

    /* Check for filter WF9 */
    if(
        ((pObj->uProgRF > 474000000) && (pObj->uProgRF < 536000000)) ||
        ((pObj->uProgRF > 794000000) && (pObj->uProgRF < 866000000))
        )
    {
        /* ROM is selecting WF9 */

        /* Bypass to WF8 */
        W_Filter_Bypass = 0x01;
        W_Filter = 0x01;

        /* Apply +4% shift */
        W_Filter_Offset = 0x00;
    }
    else
    {
        /* Let ROM do the job */
        W_Filter_Bypass = 0x00;
        W_Filter = 0x00;

        /* Check for filter WF7 & WF8 */
        if(
            ((pObj->uProgRF > 336000000) && (pObj->uProgRF < 431000000)) ||
            ((pObj->uProgRF > 563500000) && (pObj->uProgRF < 721000000))
            )
        {
            /* ROM is selecting WF7 or WF8 */

            /* Apply -8% shift */
            W_Filter_Offset = 0x03;
        }
        else
        {
            /* Nominal */
            W_Filter_Offset = 0x01;
        }
    }

    /* Read current W_Filter_byte */
    err = iTDA18273_Read(pObj, &gTDA18273_Reg_W_Filter_byte, &uPrevW_Filter_byte, Bus_None);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Read(0x%08X) failed.", pObj->tUnitW));

    if(err == TM_OK)
    {
        /* Set Wireless Filter Bypass */
        err = iTDA18273_Write(pObj, &gTDA18273_Reg_W_Filter_byte__W_Filter_Bypass, W_Filter_Bypass, Bus_None);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));

        if(err == TM_OK)
        {
            /* Set Wireless Filter */
            err = iTDA18273_Write(pObj, &gTDA18273_Reg_W_Filter_byte__W_Filter, W_Filter, Bus_None);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
        }

        if(err == TM_OK)
        {
            /* Set Wireless Filter Offset */
            err = iTDA18273_Write(pObj, &gTDA18273_Reg_W_Filter_byte__W_Filter_Offset, W_Filter_Offset, Bus_None);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
        }

        if(err == TM_OK)
        {
            /* Set Wireless filters ON or OFF */
            err = iTDA18273_Write(pObj, &gTDA18273_Reg_W_Filter_byte__W_Filter_Enable, W_Filter_enable, Bus_None);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
        }

        if(err == TM_OK)
        {
            /* Read above-modified W_Filter_byte */
            err = iTDA18273_Read(pObj, &gTDA18273_Reg_W_Filter_byte, &uW_Filter_byte, Bus_None);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Read(0x%08X) failed.", pObj->tUnitW));
        }

        if(err == TM_OK && uPrevW_Filter_byte != uW_Filter_byte)
        {
            /* W_Filter_byte changed: Update it */
            err = iTDA18273_WriteRegMap(pObj, gTDA18273_Reg_W_Filter_byte.Address, 1);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_WriteRegMap(0x%08X) failed.", pObj->tUnitW));
        }
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    iTDA18273_FirstPassPLD_CC                                     */
/*                                                                            */
/* DESCRIPTION: Implements the first pass of the PLD_CC algorithm.            */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/*============================================================================*/
static tmErrorCode_t
iTDA18273_FirstPassPLD_CC(
    pTDA18273Object_t   pObj    /* I: Driver object */
)
{
    tmErrorCode_t   err = TM_OK;

    tmDBGPRINTEx(DEBUGLVL_INOUT, "iTDA18273_FirstPassPLD_CC(0x%08X)", pObj->tUnitW);

    /* Implement PLD CC algorithm to increase PLD read immunity to interferer */
    /* 
        - set AGCK mode to 8ms.
        - PLD CC ON.
        - Set RF
        - Loop of Read of AGC Lock:
            - if((AGClock green) or  TIMEOUT(200ms)) then next Step Else (Wait 1ms and next Read)
        - Wait 20ms.
        - PLD CC OFF.
        - Wait 1ms.
        - set AGCK mode back to initial mode
    */

    /* Do the algorithm only if asked and not in analog mode */
    if( (pObj->PLD_CC_algorithm == True) && isTDA18273_DGTL_STD(pObj->StandardMode) )
    {
        /* Set AGCK Time Constant */
        err = iTDA18273_Write(pObj, &gTDA18273_Reg_AGCK_byte_1__AGCK_Mode, TDA18273_AGCK_Time_Constant_8_192ms, Bus_NoRead);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));

        if(err == TM_OK)
        {
            /* PLD_CC_Enable: 1 */
            err = iTDA18273_Write(pObj, &gTDA18273_Reg_RFAGCs_Gain_byte_1__PLD_CC_Enable, 0x01, Bus_NoRead);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
        }
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    iTDA18273_LastPassPLD_CC                                      */
/*                                                                            */
/* DESCRIPTION: Implements the last pass of the PLD_CC algorithm.             */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/*============================================================================*/
static tmErrorCode_t
iTDA18273_LastPassPLD_CC(
    pTDA18273Object_t   pObj    /* I: Driver object */
)
{
    tmErrorCode_t   err = TM_OK;
    UInt8           counter = 200; /* maximum 200 loops so max wait time = 200ms */
    UInt8           agcs_lock = 0;

    tmDBGPRINTEx(DEBUGLVL_INOUT, "iTDA18273_LastPassPLD_CC(0x%08X)", pObj->tUnitW);

    /* Finish the algorithm only if asked and not in analog mode */
    if( (pObj->PLD_CC_algorithm == True) && isTDA18273_DGTL_STD(pObj->StandardMode) )
    {
        /* Get initial AGCs_Lock */
        if(err == TM_OK)
        {
            err = iTDA18273_Read(pObj, &gTDA18273_Reg_Power_state_byte_1__AGCs_Lock, &agcs_lock, Bus_RW);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Read(0x%08X) failed.", pObj->tUnitW));
        }

        /* Perform the loop to detect AGCs_Lock = 1 or error or timeout */
        while((err == TM_OK) && ((--counter)>0) && (agcs_lock == 0))
        {
            /* Wait 1 ms */
            err = iTDA18273_Wait(pObj, 1);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Wait(0x%08X) failed.", pObj->tUnit));

            if(err == TM_OK)
            {
                err = iTDA18273_Read(pObj, &gTDA18273_Reg_Power_state_byte_1__AGCs_Lock, &agcs_lock, Bus_RW);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Read(0x%08X) failed.", pObj->tUnitW));
            }
        }

        /* Wait 20 ms */
        if(err == TM_OK)
        {
            err = iTDA18273_Wait(pObj, 20);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Wait(0x%08X) failed.", pObj->tUnit));
        }

        /* PLD_CC_Enable: 0 */
        if(err == TM_OK)
        {
            err = iTDA18273_Write(pObj, &gTDA18273_Reg_RFAGCs_Gain_byte_1__PLD_CC_Enable, 0x00, Bus_NoRead);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
        }
        
        /* Wait 1 ms */
        if(err == TM_OK)
        {
            err = iTDA18273_Wait(pObj, 1);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Wait(0x%08X) failed.", pObj->tUnit));
        }

        /* Set AGCK Time Constant */
        if(err == TM_OK)
        {
            err = iTDA18273_Write(pObj, &gTDA18273_Reg_AGCK_byte_1__AGCK_Mode, pObj->pStandard->AGCK_Time_Constant, Bus_NoRead);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
        }
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    iTDA18273_FirstPassLNAPower                                   */
/*                                                                            */
/* DESCRIPTION: Power down the LNA                                            */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
static tmErrorCode_t
iTDA18273_FirstPassLNAPower(
    pTDA18273Object_t   pObj    /* I: Driver object */
)
{
    tmErrorCode_t err = TM_OK;

    /* PD LNA */
    err = iTDA18273_Write(pObj, &gTDA18273_Reg_Power_Down_byte_2__PD_LNA, 0x1, Bus_RW);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));

    if(err == TM_OK)
    {
        /* PD Detector AGC1 */
        err = iTDA18273_Write(pObj, &gTDA18273_Reg_Power_Down_byte_2__PD_Det1, 0x1, Bus_NoRead);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
    }

    if(err == TM_OK)
    {
        /* AGC1 Detector loop off */
        err = iTDA18273_Write(pObj, &gTDA18273_Reg_AGC1_byte_3__AGC1_loop_off, 0x1, Bus_RW);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    iTDA18273_LastPassLNAPower                                    */
/*                                                                            */
/* DESCRIPTION: Power up the LNA                                              */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
static tmErrorCode_t
iTDA18273_LastPassLNAPower(
    pTDA18273Object_t   pObj    /* I: Driver object */
)
{
    tmErrorCode_t err = TM_OK;
    UInt8 uValue;

    /* Check if LNA is PD */
    err = iTDA18273_Read(pObj, &gTDA18273_Reg_Power_Down_byte_2__PD_LNA, &uValue, Bus_NoWrite);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));

    if ((uValue == 1) && (err == TM_OK))
    {
        /* LNA is Powered Down, so power it up */

        /* Force gain to -10dB */
        err = iTDA18273_Write(pObj, &gTDA18273_Reg_AGC1_byte_3__AGC1_Gain, 0x0, Bus_RW);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
        
        /* Force LNA gain control */
        if(err == TM_OK)
        {
            err = iTDA18273_Write(pObj, &gTDA18273_Reg_AGC1_byte_3__Force_AGC1_gain, 0x1, Bus_NoRead);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
        }

        /* PD LNA */
        if(err == TM_OK)
        {
            err = iTDA18273_Write(pObj, &gTDA18273_Reg_Power_Down_byte_2__PD_LNA, 0x0, Bus_NoRead);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
        }

        /* Release LNA gain control */
        if(err == TM_OK)
        {
            err = iTDA18273_Write(pObj, &gTDA18273_Reg_AGC1_byte_3__Force_AGC1_gain, 0x0, Bus_NoRead);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
        }

        if(err == TM_OK)
        {
            /* PD Detector AGC1 */
            err = iTDA18273_Write(pObj, &gTDA18273_Reg_Power_Down_byte_2__PD_Det1, 0x0, Bus_NoRead);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
        }

        if(err == TM_OK)
        {
            /* AGC1 Detector loop off */
            err = iTDA18273_Write(pObj, &gTDA18273_Reg_AGC1_byte_3__AGC1_loop_off, 0x0, Bus_NoRead);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
        }
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    iTDA18273_SetPLL                                              */
/*                                                                            */
/* DESCRIPTION: Set the PLL in manual mode                                    */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
static tmErrorCode_t
iTDA18273_SetPLL(
    pTDA18273Object_t   pObj    /* I: Driver object */
)
{
    tmErrorCode_t err = TM_OK;

    /* LO wanted = RF wanted + IF in KHz */
    UInt32 LO = 0;

    /* Algorithm that calculates PostDiv */
    UInt8 PostDiv = 0; /* absolute value */
    UInt8 LOPostDiv = 0; /* register value */

    /* Algorithm that calculates Prescaler */
    UInt8 Prescaler = 0;

    /* Algorithm that calculates N, K */
    UInt32 N_int = 0;
    UInt32 K_int = 0;

    tmDBGPRINTEx(DEBUGLVL_INOUT, "iTDA18273_SetPLL(0x%08X)", pObj->tUnitW);

    /* Calculate wanted LO = RF + IF in Hz */
    LO = (pObj->uRF + pObj->uIF) / 1000;

    /* Calculate the best PostDiv and Prescaler : the ones that provide the best margin */
    /* in case of fine tuning +-2 MHz */
    err = iTDA18273_FindPostDivAndPrescalerWithBetterMargin(LO, &PostDiv, &Prescaler);

    if (err == TM_OK)
    {
        /* Program the PLL only if valid values are found, in that case err == TM_OK */

        /* Decode PostDiv */
        switch(PostDiv)
        {
            case 1:
                LOPostDiv = 1;
                break;
            case 2:
                LOPostDiv = 2;
                break;
            case 4:
                LOPostDiv = 3;
                break;
            case 8:
                LOPostDiv = 4;
                break;
            case 16:
                LOPostDiv = 5;
                break;
            default:
                err = TDA18273_ERR_BAD_PARAMETER;
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_SetPLL(0x%08X) *PostDiv value is wrong.", pObj->tUnitW));
                break;
        }

        /* Affect register map without writing into IC */
        if(err == TM_OK)
        {
            err = iTDA18273_Write(pObj, &gTDA18273_Reg_Main_Post_Divider_byte__LOPostDiv, LOPostDiv, Bus_None);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
        }

        if(err == TM_OK)
        {
            err = iTDA18273_Write(pObj, &gTDA18273_Reg_Main_Post_Divider_byte__LOPresc, Prescaler, Bus_None);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
        }

        if(err == TM_OK)
        {
            /* Calculate N & K values of the PLL */
            err = iTDA18273_CalculateNIntKInt(LO, PostDiv, Prescaler, &N_int, &K_int);

            /* Affect registers map without writing to IC */
            if(err == TM_OK)
            {
                err = iTDA18273_Write(pObj, &gTDA18273_Reg_Sigma_delta_byte_4__LO_Frac_0, (UInt8)(K_int & 0xFF), Bus_None);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
            }

            if(err == TM_OK)
            {
                err = iTDA18273_Write(pObj, &gTDA18273_Reg_Sigma_delta_byte_3__LO_Frac_1, (UInt8)((K_int >> 8) & 0xFF), Bus_None);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
            }

            if(err == TM_OK)
            {
                err = iTDA18273_Write(pObj, &gTDA18273_Reg_Sigma_delta_byte_2__LO_Frac_2, (UInt8)((K_int >> 16) & 0xFF), Bus_None);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
            }

            if(err == TM_OK)
            {
                err = iTDA18273_Write(pObj, &gTDA18273_Reg_Sigma_delta_byte_1__LO_Int, (UInt8)(N_int & 0xFF), Bus_None);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
            }

            if(err == TM_OK)
            {
                /* Force manual selection mode : 0x3 at @0x56 */
                err = iTDA18273_Write(pObj, &gTDA18273_Reg_Sigma_delta_byte_5__N_K_correct_manual, 0x01, Bus_None);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));

                if(err == TM_OK)
                {
                    /* Force manual selection mode : 0x3 at @0x56 */
                    err = iTDA18273_Write(pObj, &gTDA18273_Reg_Sigma_delta_byte_5__LO_Calc_Disable, 0x01, Bus_None);
                    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
                }
            }

            if(err == TM_OK)
            {
                /* Write bytes Main_Post_Divider_byte (0x51) to Sigma_delta_byte_5 (0x56) */
                err = iTDA18273_WriteRegMap(pObj, gTDA18273_Reg_Main_Post_Divider_byte.Address, TDA18273_REG_DATA_LEN(gTDA18273_Reg_Main_Post_Divider_byte, gTDA18273_Reg_Sigma_delta_byte_5));
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
            }
        }
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    iTDA18273_CalculatePostDivAndPrescaler                        */
/*                                                                            */
/* DESCRIPTION: Calculate PostDiv and Prescaler by starting from lowest value */
/*              of LO or not                                                  */
/*              LO must be passed in Hz                                       */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
static tmErrorCode_t
iTDA18273_FindPostDivAndPrescalerWithBetterMargin(
    UInt32 LO,          /* In Hz */
    UInt8* PostDiv,     /* Directly the value to set in the register  */
    UInt8* Prescaler    /* Directly the value to set in the register  */
)
{
    /* Initialize to error in case no valuable values are found */
    tmErrorCode_t err = TM_OK;

    UInt8 PostDivGrowing;
    UInt8 PrescalerGrowing;
    UInt8 PostDivDecreasing;
    UInt8 PrescalerDecreasing;
    UInt32 FCVOGrowing = 0;
    UInt32 DistanceFCVOGrowing = 0;
    UInt32 FVCODecreasing = 0;
    UInt32 DistanceFVCODecreasing = 0;

    /* Get the 2 possible values for PostDiv & Prescaler to find the one
    which provides the better margin on LO */
    err = iTDA18273_CalculatePostDivAndPrescaler(LO, True, &PostDivGrowing, &PrescalerGrowing);
    if (err == TM_OK)
    {
        /* Calculate corresponding FVCO value in kHz */
        FCVOGrowing = LO * PrescalerGrowing * PostDivGrowing;
    }

    err = iTDA18273_CalculatePostDivAndPrescaler(LO, False, &PostDivDecreasing, &PrescalerDecreasing);
    if (err == TM_OK)
    {
        /* Calculate corresponding FVCO value in kHz */
        FVCODecreasing = LO * PrescalerDecreasing * PostDivDecreasing;
    }

    /* Now take the values that are providing the better margin, the goal is +-2 MHz on LO */
    /* So take the point that is the nearest of (FVCOmax - FVCOmin)/2 = 6391,875 MHz */
    if (FCVOGrowing != 0)
    {
        if (FCVOGrowing >= TDA18273_MIDDLE_FVCO_RANGE)
        {
            DistanceFCVOGrowing = FCVOGrowing - TDA18273_MIDDLE_FVCO_RANGE;
        }
        else
        {
            DistanceFCVOGrowing = TDA18273_MIDDLE_FVCO_RANGE - FCVOGrowing;
        }
    }

    if (FVCODecreasing != 0)
    {
        if (FVCODecreasing >= TDA18273_MIDDLE_FVCO_RANGE)
        {
            DistanceFVCODecreasing = FVCODecreasing - TDA18273_MIDDLE_FVCO_RANGE;
        }
        else
        {
            DistanceFVCODecreasing = TDA18273_MIDDLE_FVCO_RANGE - FVCODecreasing;
        }
    }

    if (FCVOGrowing == 0)
    {
        if (FVCODecreasing == 0)
        {
            /* No value at all are found */
            err = TDA18273_ERR_BAD_PARAMETER;
        }
        else
        {
            /* No value in growing mode, so take the decreasing ones */
            *PostDiv = PostDivDecreasing;
            *Prescaler = PrescalerDecreasing;
        }
    }
    else
    {
        if (FVCODecreasing == 0)
        {
            /* No value in decreasing mode, so take the growing ones */
            *PostDiv = PostDivGrowing;
            *Prescaler = PrescalerGrowing;
        }
        else
        {
            /* Find the value which are the nearest of the middle of VCO range */
            if (DistanceFCVOGrowing <= DistanceFVCODecreasing)
            {
                *PostDiv = PostDivGrowing;
                *Prescaler = PrescalerGrowing;
            }
            else
            {
                *PostDiv = PostDivDecreasing;
                *Prescaler = PrescalerDecreasing;
            }
        }
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    iTDA18273_CalculateNIntKInt                                   */
/*                                                                            */
/* DESCRIPTION: Calculate PLL N & K values                                    */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
static tmErrorCode_t
iTDA18273_CalculateNIntKInt(
    UInt32 LO, 
    UInt8 PostDiv, 
    UInt8 Prescaler, 
    UInt32* NInt, 
    UInt32* KInt
)
{
    tmErrorCode_t err = TM_OK;

    /* Algorithm that calculates N_K */
    UInt32 FVCO = 0;
    UInt32 N_K_prog = 0;

    /* Algorithm that calculates N, K corrected */
    UInt32 Nprime = 0;
    UInt32 KforceK0_1 = 0;
    UInt32 K2msb = 0;
    UInt32 N0 = 0;
    UInt32 Nm1 = 0;

    /* Calculate N_K_Prog */
    FVCO = LO * Prescaler * PostDiv;
    N_K_prog = (FVCO * 128) / 125;

    /* Calculate N & K corrected values */
    Nprime = N_K_prog & 0xFF0000;

    /* Force LSB to 1 */
    KforceK0_1 = 2*(((N_K_prog - Nprime) << 7) / 2) + 1;

    /* Check MSB bit around 2 */
    K2msb = KforceK0_1 >> 21;
    if (K2msb < 1)
    {
        N0 = 1;
    }
    else
    {
        if (K2msb >= 3)
        {
            N0 = 1;
        }
        else
        {
            N0 = 0;
        }
    }
    if (K2msb < 1)
    {
        Nm1 = 1;
    }
    else
    {
        Nm1 = 0;
    }

    /* Calculate N */
    *NInt = (2 * ((Nprime >> 16) - Nm1) + N0) - 128;

    /* Calculate K */
    if (K2msb < 1)
    {
        *KInt = KforceK0_1 + (2 << 21);
    }
    else
    {
        if (K2msb >= 3)
        {
            *KInt = KforceK0_1 - (2 << 21);
        }
        else
        {
            *KInt = KforceK0_1;
        }
    }

    /* Force last 7 bits of K_int to 0x5D, as the IC is doing for spurs optimization */
    *KInt &= 0xFFFFFF80;
    *KInt |= 0x5D;

    return err;
}

/*============================================================================*/
/* FUNCTION:    iTDA18273_CalculatePostDivAndPrescaler                        */
/*                                                                            */
/* DESCRIPTION: Calculate PostDiv and Prescaler by starting from lowest value */
/*              of LO or not                                                  */
/*              LO must be passed in Hz                                       */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
static tmErrorCode_t
iTDA18273_CalculatePostDivAndPrescaler(
    UInt32 LO,         /* In Hz */
    Bool growingOrder, /* Start from LO = 28.681 kHz or LO = 974 kHz */
    UInt8* PostDiv,    /* Absolute value */
    UInt8* Prescaler   /* Absolute value  */
)
{
    tmErrorCode_t err = TM_OK;
    Int8 index;
    Int8 sizeTable = sizeof(PostDivPrescalerTable) / sizeof(TDA18273_PostDivPrescalerTableDef);

    if (growingOrder == True)
    {
        /* Start from LO = 28.681 kHz */
        for (index = (sizeTable - 1); index >= 0; index--)
        {
            if (
                (LO > PostDivPrescalerTable[index].LO_min) &&
                (LO < PostDivPrescalerTable[index].LO_max)
               )
            {
                /* We are at correct index in the table */
                break;
            }
        }
    }
    else
    {
        /* Start from LO = 974000 kHz */
        for (index = 0; index < sizeTable; index++)
        {
            if (
                (LO > PostDivPrescalerTable[index].LO_min) &&
                (LO < PostDivPrescalerTable[index].LO_max)
               )
            {
                /* We are at correct index in the table */
                break;
            }
        }
    }

    if ((index == -1) || (index == sizeTable))
    {
        err = TDA18273_ERR_BAD_PARAMETER;
    }
    else
    {
        /* Write Prescaler */
        *Prescaler = PostDivPrescalerTable[index].Prescaler;

        /* Decode PostDiv */
        *PostDiv = PostDivPrescalerTable[index].PostDiv;
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    iTDA18273_WaitXtalCal_End                                     */
/*                                                                            */
/* DESCRIPTION: Wait the XtalCal_End to trigger                               */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
static tmErrorCode_t
iTDA18273_WaitXtalCal_End(
    pTDA18273Object_t pObj, /* I: Driver object */
    UInt32 timeOut,         /* I: timeout */
    UInt32 waitStep         /* I: wait step */
)
{
    tmErrorCode_t   err = TM_OK;
    UInt32          counter = timeOut/waitStep; /* Wait max timeOut/waitStep ms */
    UInt8           uMSM_XtalCal_End = 0;

    tmDBGPRINTEx(DEBUGLVL_INOUT, "iTDA18273_WaitXtalCal_End(0x%08X)", pObj->tUnitW);

    while(counter > 0)
    {
        /* Don't check for I2C error because it can occur, just after POR, that I2C is not yet available */
        /* MSM_XtalCal_End must occur between POR and POR+70ms */
        /* Check for XtalCal End */
        err = iTDA18273_Read(pObj, &gTDA18273_Reg_IRQ_status__MSM_XtalCal_End, &uMSM_XtalCal_End, Bus_RW);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Read(0x%08X) failed.", pObj->tUnitW));

        if(uMSM_XtalCal_End)
        {
            /* MSM_XtalCal_End is triggered => exit */
            break;
        }

        if(counter)
        {
            /* Decrease the counter */
            counter--;

            /* Wait for a step */
            err = iTDA18273_Wait(pObj, waitStep);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Wait(0x%08X) failed.", pObj->tUnitW));
        }
    }

    if(counter == 0)
    {
        err = TDA18273_ERR_NOT_READY;
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    iTDA18273_SetMSM                                              */
/*                                                                            */
/* DESCRIPTION: Set the MSM bit(s).                                           */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
iTDA18273_SetMSM(
    pTDA18273Object_t   pObj,   /* I: Driver object */
    UInt8               uValue, /* I: Item value */
    Bool                bLaunch /* I: Launch MSM */
)
{
    tmErrorCode_t   err = TM_OK;

    tmDBGPRINTEx(DEBUGLVL_INOUT, "iTDA18273_SetMSM(0x%08X)", pObj->tUnitW);

    /* Set state machine and Launch it */
    err = iTDA18273_Write(pObj, &gTDA18273_Reg_MSM_byte_1, uValue, Bus_None);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));

    if(err == TM_OK && bLaunch)
    {
        err = iTDA18273_Write(pObj, &gTDA18273_Reg_MSM_byte_2__MSM_Launch, 0x01, Bus_None);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
    }

    if(err == TM_OK)
    {
        err = iTDA18273_WriteRegMap(pObj, gTDA18273_Reg_MSM_byte_1.Address, bLaunch?0x02:0x01);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_WriteRegMap(0x%08X) failed.", pObj->tUnitW));
    }

    if(err == TM_OK && bLaunch)
    {
        err = iTDA18273_Write(pObj, &gTDA18273_Reg_MSM_byte_2__MSM_Launch, 0x00, Bus_None);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Write(0x%08X) failed.", pObj->tUnitW));
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    iTDA18273_WaitIRQ                                             */
/*                                                                            */
/* DESCRIPTION: Wait the IRQ to trigger                                       */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
iTDA18273_WaitIRQ(
    pTDA18273Object_t   pObj,       /* I: Driver object */
    UInt32              timeOut,    /* I: timeout */
    UInt32              waitStep,   /* I: wait step */
    UInt8               irqStatus   /* I: IRQs to wait */
)
{
    tmErrorCode_t   err = TM_OK;
    UInt32          counter = timeOut/waitStep; /* Wait max timeOut/waitStep ms */
    UInt8           uIRQ = 0;
    UInt8           uIRQStatus = 0;
    Bool            bIRQTriggered = False;

    tmDBGPRINTEx(DEBUGLVL_INOUT, "iTDA18273_WaitIRQ(0x%08X)", pObj->tUnitW);

    while(err == TM_OK && (counter--)>0)
    {
        err = iTDA18273_Read(pObj, &gTDA18273_Reg_IRQ_status__IRQ_status, &uIRQ, Bus_RW);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Read(0x%08X) failed.", pObj->tUnitW));

        if(err == TM_OK && uIRQ == 1)
        {
            bIRQTriggered = True;
        }

        if(bIRQTriggered)
        {
            /* IRQ triggered => Exit */
            break;
        }

        if(err == TM_OK && irqStatus != 0x00)
        {
            err = iTDA18273_Read(pObj, &gTDA18273_Reg_IRQ_status, &uIRQStatus, Bus_None);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Read(0x%08X) failed.", pObj->tUnitW));

            if(irqStatus == uIRQStatus)
            {
                bIRQTriggered = True;
            }
        }

        if(counter)
        {
            err = iTDA18273_Wait(pObj, waitStep);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "iTDA18273_Wait(0x%08X) failed.", pObj->tUnitW));
        }
    }

    if(err == TM_OK && bIRQTriggered == False)
    {
        err = TDA18273_ERR_NOT_READY;
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    iTDA18273_Write                                               */
/*                                                                            */
/* DESCRIPTION: Writes in TDA18273 hardware                                   */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
iTDA18273_Write(
    pTDA18273Object_t           pObj,       /* I: Driver object */
    const TDA18273_BitField_t*  pBitField, /* I: Bitfield structure */
    UInt8                       uData,      /* I: Data to write */
    tmbslFrontEndBusAccess_t                eBusAccess  /* I: Access to bus */
)
{
    tmErrorCode_t   err = TM_OK;
    UInt8           RegAddr = 0;
    UInt32          DataLen = 1;
    UInt8           RegData = 0;
    pUInt8          pRegData = Null;
    UInt32          RegMask = 0;

    tmDBGPRINTEx(DEBUGLVL_INOUT, "iTDA18273_Write(0x%08X)", pObj->tUnitW);

    if(pBitField == Null)
    {
        err = TDA18273_ERR_BAD_PARAMETER;
    }

    if(err == TM_OK)
    {
        /* Set Register Address */
        RegAddr = pBitField->Address;

        if(RegAddr < TDA18273_REG_MAP_NB_BYTES)
        {
            pRegData = (UInt8 *)(&(pObj->RegMap)) + RegAddr;
        }
        else
        {
            pRegData = &RegData;
        }

        if( (eBusAccess&Bus_NoRead) == False && P_SIO_READ_VALID)
        {
            /* Read data from TDA18273 */
            err = P_SIO_READ(pObj->tUnitW, TDA18273_REG_ADD_SZ, &RegAddr, DataLen, pRegData);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "IO_Read(0x%08X, 1, 0x%02X, %d) failed.", pObj->tUnitW, RegAddr, DataLen));
        }

        if(err == TM_OK)
        {
            RegMask = ( (1 << pBitField->WidthInBits) - 1);
            /* Limit uData to WidthInBits */
            uData &= RegMask;

            /* Set Data */
            RegMask = RegMask << pBitField->PositionInBits;
            *pRegData &= (UInt8)(~RegMask);
            *pRegData |= uData << pBitField->PositionInBits;

            if( (eBusAccess&Bus_NoWrite) == False && P_SIO_WRITE_VALID)
            {
                /* Write data to TDA18273 */
                err = P_SIO_WRITE(pObj->tUnitW, TDA18273_REG_ADD_SZ, &RegAddr, DataLen, pRegData);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "IO_Write(0x%08X, 1, 0x%02X, %d) failed.", pObj->tUnitW, RegAddr, DataLen));
            }
        }
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    iTDA18273_Read                                                */
/*                                                                            */
/* DESCRIPTION: Reads in TDA18273 hardware                                    */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
iTDA18273_Read(
    pTDA18273Object_t           pObj,       /* I: Driver object */
    const TDA18273_BitField_t*  pBitField, /* I: Bitfield structure */
    UInt8*                      puData,     /* I: Data to read */
    tmbslFrontEndBusAccess_t                eBusAccess  /* I: Access to bus */
)
{
    tmErrorCode_t   err = TM_OK;
    UInt8           RegAddr = 0;
    UInt32          DataLen = 1;
    UInt8           RegData = 0;
    pUInt8          pRegData = Null;
    UInt32          RegMask = 0;

    tmDBGPRINTEx(DEBUGLVL_INOUT, "iTDA18273_Read(0x%08X)", pObj->tUnitW);

    if(pBitField == Null)
    {
        err = TDA18273_ERR_BAD_PARAMETER;
    }

    if(err == TM_OK)
    {
        /* Set Register Address */
        RegAddr = pBitField->Address;

        if(RegAddr < TDA18273_REG_MAP_NB_BYTES)
        {
            pRegData = (UInt8 *)(&(pObj->RegMap)) + RegAddr;
        }
        else
        {
            pRegData = &RegData;
        }

        if( (eBusAccess&Bus_NoRead) == False && P_SIO_READ_VALID)
        {
            /* Read data from TDA18273 */
            err = P_SIO_READ(pObj->tUnitW, TDA18273_REG_ADD_SZ, &RegAddr, DataLen, pRegData);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "IO_Read(0x%08X, 1, 0x%02X, %d) failed.", pObj->tUnitW, RegAddr, DataLen));
        }

        if(err == TM_OK && puData != Null)
        {
            /* Copy Raw Data */
            *puData = *pRegData;

            /* Get Data */
            RegMask = ( (1 << pBitField->WidthInBits) - 1) << pBitField->PositionInBits;
            *puData &= (UInt8)RegMask;
            *puData = (*puData) >> pBitField->PositionInBits;
        }
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    iTDA18273_WriteRegMap                                         */
/*                                                                            */
/* DESCRIPTION: Writes driver RegMap cached data to TDA18273 hardware.        */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
iTDA18273_WriteRegMap(
    pTDA18273Object_t   pObj,       /* I: Driver object */
    UInt8               uAddress,   /* I: Data to write */
    UInt32              uWriteLen   /* I: Number of data to write */
)
{
    tmErrorCode_t   err = TM_OK;
    pUInt8          pRegData = Null;

    tmDBGPRINTEx(DEBUGLVL_INOUT, "iTDA18273_WriteRegMap(0x%08X)", pObj->tUnitW);

    if( uAddress < TDA18273_REG_MAP_NB_BYTES &&
        (uAddress + uWriteLen) <= TDA18273_REG_MAP_NB_BYTES )
    {
        pRegData = (UInt8 *)(&(pObj->RegMap)) + uAddress;
    }
    else
    {
        err = TDA18273_ERR_BAD_PARAMETER;
    }

    if(err == TM_OK && P_SIO_WRITE_VALID)
    {
        /* Write data to TDA18273 */
        err = P_SIO_WRITE(pObj->tUnitW, TDA18273_REG_ADD_SZ, &uAddress, uWriteLen, pRegData);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "IO_Write(0x%08X, 1, 0x%02X, %d) failed.", pObj->tUnitW, uAddress, uWriteLen));
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    iTDA18273_ReadRegMap                                          */
/*                                                                            */
/* DESCRIPTION: Reads driver RegMap cached data from TDA18273 hardware.       */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
iTDA18273_ReadRegMap(
    pTDA18273Object_t   pObj,       /* I: Driver object */
    UInt8               uAddress,   /* I: Data to read */
    UInt32              uReadLen    /* I: Number of data to read */
)
{
    tmErrorCode_t   err = TM_OK;
    pUInt8          pRegData = Null;

    tmDBGPRINTEx(DEBUGLVL_INOUT, "iTDA18273_ReadRegMap(0x%08X)", pObj->tUnitW);

    if( uAddress < TDA18273_REG_MAP_NB_BYTES &&
       (uAddress + uReadLen) <= TDA18273_REG_MAP_NB_BYTES )
    {
        pRegData = (UInt8 *)(&(pObj->RegMap)) + uAddress;
    }
    else
    {
        err = TDA18273_ERR_BAD_PARAMETER;
    }

    if(err == TM_OK && P_SIO_READ_VALID)
    {
        /* Read data from TDA18273 */
        err = P_SIO_READ(pObj->tUnitW, TDA18273_REG_ADD_SZ, &uAddress, uReadLen, pRegData);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "IO_Read(0x%08X, 1, 0x%02X, %d) failed.", pObj->tUnitW, uAddress, uReadLen));
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    iTDA18273_Wait                                                */
/*                                                                            */
/* DESCRIPTION: Waits for requested time.                                     */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t 
iTDA18273_Wait(
    pTDA18273Object_t   pObj,   /* I: Driver object */
    UInt32              Time    /* I: time to wait for */
)
{
    tmErrorCode_t   err = TDA18273_ERR_NULL_CONTROLFUNC;

    tmDBGPRINTEx(DEBUGLVL_INOUT, "iTDA18273_Wait(0x%08X)", pObj->tUnitW);

    if(P_STIME_WAIT_VALID)
    {
        /* Wait Time ms */
        err = P_STIME_WAIT(pObj->tUnitW, Time);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "TIME_Wait(0x%08X, %d) failed.", pObj->tUnitW, Time));
    }

    return err;
}

#ifdef _TVFE_IMPLEMENT_MUTEX

/*============================================================================*/
/* FUNCTION:    iTDA18273_MutexAcquire:                                       */
/*                                                                            */
/* DESCRIPTION: Acquires driver mutex.                                        */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*============================================================================*/
tmErrorCode_t
iTDA18273_MutexAcquire(
    pTDA18273Object_t   pObj,
    UInt32              timeOut
)
{
    tmErrorCode_t   err = TDA18273_ERR_NULL_CONTROLFUNC;

    tmDBGPRINTEx(DEBUGLVL_INOUT, "iTDA18273_MutexAcquire(0x%08X)", pObj->tUnitW);

    if(P_SMUTEX_ACQUIRE_VALID && P_MUTEX_VALID)
    {
        err = P_SMUTEX_ACQUIRE(P_MUTEX, timeOut);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "Mutex_Acquire(0x%08X, %d) failed.", pObj->tUnitW, timeOut));
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    iTDA18273_MutexRelease:                                       */
/*                                                                            */
/* DESCRIPTION: Releases driver mutex.                                        */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*============================================================================*/
tmErrorCode_t
iTDA18273_MutexRelease(
    pTDA18273Object_t   pObj
)
{
    tmErrorCode_t   err = TDA18273_ERR_NULL_CONTROLFUNC;

    tmDBGPRINTEx(DEBUGLVL_INOUT, "iTDA18273_MutexRelease(0x%08X)", pObj->tUnitW);

    if(P_SMUTEX_RELEASE_VALID && P_MUTEX_VALID)
    {
        err = P_SMUTEX_RELEASE(P_MUTEX);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "Mutex_Release(0x%08X) failed.", pObj->tUnitW));
    }

    return err;
}

#endif
