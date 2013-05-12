/*
  Copyright (C) 2010 NXP B.V., All Rights Reserved.
  This source code and any compilation or derivative thereof is the proprietary
  information of NXP B.V. and is confidential in nature. Under no circumstances
  is this software to be  exposed to or placed under an Open Source License of
  any type without the expressed written permission of NXP B.V.
 *
 * \file          tmbslTDA18273_Instance.h
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

#ifndef _TMBSL_TDA18273_INSTANCE_H
#define _TMBSL_TDA18273_INSTANCE_H

#ifdef __cplusplus
extern "C"
{
#endif


tmErrorCode_t
iTDA18273_AllocInstance(tmUnitSelect_t tUnit, ppTDA18273Object_t ppDrvObject);

tmErrorCode_t
iTDA18273_DeAllocInstance(pTDA18273Object_t pDrvObject);

tmErrorCode_t
iTDA18273_GetInstance(tmUnitSelect_t tUnit, ppTDA18273Object_t ppDrvObject);

tmErrorCode_t
iTDA18273_ResetInstance(pTDA18273Object_t pDrvObject);


#ifdef __cplusplus
}
#endif

#endif /* _TMBSL_TDA18273_INSTANCE_H */

