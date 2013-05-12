/**
Copyright (C) 2011 NXP B.V., All Rights Reserved.
This source code and any compilation or derivative thereof is the proprietary
information of NXP B.V. and is confidential in nature. Under no circumstances
is this software to be  exposed to or placed under an Open Source License of
any type without the expressed written permission of NXP B.V.
*
* \file          tmsysOM3912Tuner.h
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


#ifndef TMSYSOM3912TUNER_H
#define TMSYSOM3912TUNER_H

/*============================================================================*/
/*                       INCLUDE FILES                                        */
/*============================================================================*/


#ifdef __cplusplus
extern "C" {
#endif


/*============================================================================*/
/*                       MACRO DEFINITION                                     */
/*============================================================================*/


/*============================================================================*/
/*                       ENUM OR TYPE DEFINITION                              */
/*============================================================================*/



/*============================================================================*/
/*                       EXTERN DATA DEFINITION                               */
/*============================================================================*/

/*============================================================================*/
/*                       EXTERN FUNCTION PROTOTYPES                           */
/*============================================================================*/

extern tmErrorCode_t
tmsysOM3912GetTunerSWVersion
(
    ptmbslSWVersion_t pswTunerVersion  /* O: Receives Tuner SW Version  */
);

extern tmErrorCode_t
tmsysOM3912TunerOpen(
    tmUnitSelect_t              tUnit,      /*  I: Unit number */
    ptmbslFrontEndDependency_t  psSrvFunc   /*  I: setup parameters */
);

extern tmErrorCode_t
tmsysOM3912TunerClose
(
    tmUnitSelect_t tUnit   /*  I: Unit number */
);

extern tmErrorCode_t
tmsysOM3912TunerReset
(
    tmUnitSelect_t tUnit   /*  I: Unit number */
);

extern tmErrorCode_t
tmsysOM3912TunerSetPowerState(
    tmUnitSelect_t  tUnit,       /* I: Unit number */
    tmPowerState_t  ePowerState  /* I: Power state of tuner */
);

extern tmErrorCode_t
tmsysOM3912TunerGetLockStatus(
    tmUnitSelect_t        tUnit,      /* I: Unit number */
    ptmbslFrontEndState_t pLockStatus /* O: PLL Lock status */
);

extern tmErrorCode_t
tmsysOM3912TunerSetStandardMode(
    tmUnitSelect_t              tUnit,          /*  I: Unit number */
    tmbslFrontEndStandardMode_t StandardMode    /*  I: Standard mode of this device */
);

extern tmErrorCode_t
tmsysOM3912TunerSetRF(
    tmUnitSelect_t  tUnit,  /*  I: Unit number */
    UInt32          uRF     /*  I: RF frequency in hertz */
);

#ifdef __cplusplus
}
#endif

#endif /* TMSYSOM3912TUNER_H */
/*============================================================================*/
/*                            END OF FILE                                     */
/*============================================================================*/

