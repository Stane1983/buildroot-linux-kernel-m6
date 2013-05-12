/*
  Copyright (C) 2010 NXP B.V., All Rights Reserved.
  This source code and any compilation or derivative thereof is the proprietary
  information of NXP B.V. and is confidential in nature. Under no circumstances
  is this software to be  exposed to or placed under an Open Source License of
  any type without the expressed written permission of NXP B.V.
 *
 * \file          tmbslTDA18273_Instance.c
 *
 *                %version: 3 %
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
#include "tmUnitParams.h"

/*============================================================================*/
/* Project include files:                                                     */
/*============================================================================*/
#include "tmbslTDA18273.h"

#include "tmbslTDA18273_RegDef.h"
#include "tmbslTDA18273_Local.h"
#include "tmbslTDA18273_Instance.h"
#include "tmbslTDA18273_InstanceCustom.h"


/*============================================================================*/
/* Global data:                                                               */
/*============================================================================*/

/* Driver instance */
TDA18273Object_t gTDA18273Instance[TDA18273_MAX_UNITS] = 
{
    {
        (tmUnitSelect_t)(-1),               /* Unit not set */
        (tmUnitSelect_t)(-1),               /* UnitW not set */
        Null,                               /* pMutex */
        False,                              /* init (instance initialization default) */
        {                                   /* sIo */
            Null,
            Null
        },
        {                                   /* sTime */
            Null,
            Null
        },
        {                                   /* sDebug */
            Null
        },
        {                                   /* sMutex */
            Null,
            Null,
            Null,
            Null
        },
        TDA18273_INSTANCE_CUSTOM_0          /* Instance Customizable Settings */
    },
    {
        (tmUnitSelect_t)(-1),               /* Unit not set */
        (tmUnitSelect_t)(-1),               /* UnitW not set */
        Null,                               /* pMutex */
        False,                              /* init (instance initialization default) */
        {                                   /* sIo */
            Null,
            Null
        },
        {                                   /* sTime */
            Null,
            Null
        },
        {                                   /* sDebug */
            Null
        },
        {                                   /* sMutex */
            Null,
            Null,
            Null,
            Null
        },
        TDA18273_INSTANCE_CUSTOM_1          /* Instance Customizable Settings */
    }
};

/*============================================================================*/
/* Internal functions:                                                        */
/*============================================================================*/

/*============================================================================*/
/* FUNCTION:    iTDA18273_AllocInstance:                                      */
/*                                                                            */
/* DESCRIPTION: Allocates an instance.                                        */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*============================================================================*/
tmErrorCode_t
iTDA18273_AllocInstance(
    tmUnitSelect_t      tUnit,      /* I: Unit number */
    ppTDA18273Object_t  ppDrvObject /* I: Driver Object */
)
{
    tmErrorCode_t       err = TDA18273_ERR_ERR_NO_INSTANCES;
    pTDA18273Object_t   pObj = Null;
    UInt32              uLoopCounter = 0;

    /* Find a free instance */
    for (uLoopCounter = 0; uLoopCounter<TDA18273_MAX_UNITS; uLoopCounter++)
    {
        pObj = &gTDA18273Instance[uLoopCounter];
        if (pObj->init == False)
        {
            err = TM_OK;

            pObj->tUnit = tUnit;
            pObj->tUnitW = tUnit;

            err = iTDA18273_ResetInstance(pObj);

            if(err == TM_OK)
            {
                *ppDrvObject = pObj;
            }
            break;
        }
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    iTDA18273_DeAllocInstance:                                    */
/*                                                                            */
/* DESCRIPTION: Deallocates an instance.                                      */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*============================================================================*/
tmErrorCode_t
iTDA18273_DeAllocInstance(
    pTDA18273Object_t  pDrvObject  /* I: Driver Object */
)
{
    tmErrorCode_t   err = TM_OK;

    if(pDrvObject != Null)
    {
        pDrvObject->init = False;
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    iTDA18273_GetInstance:                                        */
/*                                                                            */
/* DESCRIPTION: Gets an instance.                                             */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*============================================================================*/
tmErrorCode_t
iTDA18273_GetInstance(
    tmUnitSelect_t      tUnit,      /* I: Unit number   */
    ppTDA18273Object_t  ppDrvObject /* I: Driver Object */
)
{
    tmErrorCode_t       err = TDA18273_ERR_ERR_NO_INSTANCES;
    pTDA18273Object_t   pObj = Null;
    UInt32              uLoopCounter = 0;

    /* Get instance */
    for (uLoopCounter = 0; uLoopCounter<TDA18273_MAX_UNITS; uLoopCounter++)
    {
        pObj = &gTDA18273Instance[uLoopCounter];
        if (pObj->init == True && pObj->tUnit == GET_INDEX_TYPE_TUNIT(tUnit))
        {
            pObj->tUnitW = tUnit;

            *ppDrvObject = pObj;
            err = TM_OK;
            break;
        }
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    iTDA18273_ResetInstance:                                      */
/*                                                                            */
/* DESCRIPTION: Resets an instance.                                           */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*============================================================================*/
tmErrorCode_t
iTDA18273_ResetInstance(
    pTDA18273Object_t  pDrvObject  /* I: Driver Object */
)
{
    tmErrorCode_t   err = TM_OK ;

    pDrvObject->curPowerState = TDA18273_INSTANCE_CUSTOM_CURPOWERSTATE_DEF;
    pDrvObject->curLLPowerState = TDA18273_INSTANCE_CUSTOM_CURLLPOWERSTATE_DEF;
    pDrvObject->uRF = TDA18273_INSTANCE_CUSTOM_RF_DEF;
    pDrvObject->uProgRF = TDA18273_INSTANCE_CUSTOM_PROG_RF_DEF;

    pDrvObject->StandardMode = TDA18273_INSTANCE_CUSTOM_STANDARDMODE_DEF;

    pDrvObject->eHwState = TDA18273_HwState_InitNotDone;

    return err;
}

