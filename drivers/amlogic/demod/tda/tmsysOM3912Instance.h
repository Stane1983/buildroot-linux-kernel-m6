/**
  Copyright (C) 2010 NXP B.V., All Rights Reserved.
  This source code and any compilation or derivative thereof is the proprietary
  information of NXP B.V. and is confidential in nature. Under no circumstances
  is this software to be  exposed to or placed under an Open Source License of
  any type without the expressed written permission of NXP B.V.
 *
 * \file          tmsysOM3912Instance.h
 *                %version: 1 %
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
#ifndef _TMSYSOM3912_INSTANCE_H //-----------------
#define _TMSYSOM3912_INSTANCE_H

#ifdef __cplusplus
extern "C" {
#endif


tmErrorCode_t OM3912AllocInstance (tmUnitSelect_t tUnit, pptmOM3912Object_t ppDrvObject);
tmErrorCode_t OM3912DeAllocInstance (tmUnitSelect_t tUnit);
tmErrorCode_t OM3912GetInstance (tmUnitSelect_t tUnit, pptmOM3912Object_t ppDrvObject);


#ifdef __cplusplus
}
#endif

#endif // _TMSYSOM3912_INSTANCE_H //---------------
