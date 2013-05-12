/**
Copyright (C) 2010 NXP B.V., All Rights Reserved.
This source code and any compilation or derivative thereof is the proprietary
information of NXP B.V. and is confidential in nature. Under no circumstances
is this software to be  exposed to or placed under an Open Source License of
any type without the expressed written permission of NXP B.V.
*
* \file          tmsysOM3912local.h
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


#ifndef TMSYSOM3912LOCAL_H
#define TMSYSOM3912LOCAL_H

/*============================================================================*/
/*                       INCLUDE FILES                                        */
/*============================================================================*/


#ifdef __cplusplus
extern "C" {
#endif


    /*============================================================================*/
    /*                       MACRO DEFINITION                                     */
    /*============================================================================*/

#define OM3912_SYS_COMP_NUM         2
#define OM3912_SYS_MAJOR_VER        2
#define OM3912_SYS_MINOR_VER        0

#define OM3912_MUTEX_TIMEOUT       TMBSL_FRONTEND_MUTEX_TIMEOUT_INFINITE

#define POBJ_SRVFUNC_SIO pObj->sRWFunc
#define POBJ_SRVFUNC_STIME pObj->sTime
#define P_DBGPRINTEx tmsysOM3912Print
#define P_DBGPRINTVALID (True)

/*-------------*/
/* ERROR CODES */
/*-------------*/

/*---------*/
/* CHANNEL */
/*---------*/

#define OM3912_MAX_UNITS 2

/*-----------------------------------------------*/
/*  DEFAULT VALUES FOR CONFIGURATION MANAGEMENT  */
/*-----------------------------------------------*/

/*---------*/
/*  INDEX  */
/*---------*/


/*------------------*/
/*special config    */
/*------------------*/

/*------------------*/
/*  DEFAULT VALUES  */
/*------------------*/

/*----------------*/
/*  DEFINE MASKS  */
/*----------------*/


/*---------------*/
/*  DEFINE BITS  */
/*---------------*/
/*Special Config*/

/*============================================================================*/
/*                       ENUM OR TYPE DEFINITION                              */
/*============================================================================*/

typedef struct _tmOM3912Object_t
{
    tmUnitSelect_t              tUnit;
    tmUnitSelect_t              tUnitW;
    tmUnitSelect_t              tUnitTuner;
    ptmbslFrontEndMutexHandle   pMutex;
    Bool                        init;
    tmbslFrontEndIoFunc_t       sRWFunc;
    tmbslFrontEndTimeFunc_t     sTime;
    tmbslFrontEndDebugFunc_t    sDebug;
    tmbslFrontEndMutexFunc_t    sMutex;
    tmPowerState_t              powerState;
    Bool                        resetDone;
    UInt32                      uHwAddress[tmOM3912UnitDeviceTypeMax];
    tmTuneReqType_t             lastTuneReqType;
} tmOM3912Object_t, *ptmOM3912Object_t,**pptmOM3912Object_t;



/*============================================================================*/
/*                       EXTERN DATA DEFINITION                               */
/*============================================================================*/

/*============================================================================*/
/*                       EXTERN FUNCTION PROTOTYPES                           */
/*============================================================================*/

extern tmErrorCode_t OM3912MutexAcquire(ptmOM3912Object_t   pObj, UInt32 timeOut);
extern tmErrorCode_t OM3912MutexRelease(ptmOM3912Object_t   pObj);

#ifdef __cplusplus
}
#endif

#endif /* TMSYSOM3912LOCAL_H */
/*============================================================================*/
/*                            END OF FILE                                     */
/*============================================================================*/

