/***************************************************************************************
*					             
*			(c) Copyright 2011, LegendSilicon, beijing, China
*
*				         All Rights Reserved
* Description :
* Notice:
***************************************************************************************/
#ifndef _LGS_9X_H_
#define _LGS_9X_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "LGS_TYPES.h"
#include "LGS_DEMOD.h"
#include "LGS_HOOK.h"

#define  LGS9XSECADDR1       (0x3A>>1)
#define  LGS9XSECADDR2       (0x3E>>1)

LGS_RESULT LGS9X_Init(DemodInitPara *para);
LGS_RESULT LGS9X_SetPnmdAutoMode(DEMOD_WORK_MODE workMode);
LGS_RESULT LGS9X_CheckLocked(DEMOD_WORK_MODE workMode, UINT8 *locked1, UINT8 *locked2, UINT16 waitms);
LGS_RESULT LGS9X_I2CEchoOn();
LGS_RESULT LGS9X_I2CEchoOff();
LGS_RESULT LGS9X_GetConfig(DEMOD_WORK_MODE workMode, DemodConfig *pDemodConfig);
LGS_RESULT LGS9X_SetConfig(DEMOD_WORK_MODE workMode, DemodConfig *pDemodConfig);
LGS_RESULT LGS9X_GetSignalStrength(DEMOD_WORK_MODE workMode, UINT32 *SignalStrength);
LGS_RESULT LGS9X_GetDtmbSignalQuality(DEMOD_WORK_MODE workMode,UINT8 *signalQuality);
LGS_RESULT LGS9X_SoftReset();
LGS_RESULT LGS9X_GetBER(DEMOD_WORK_MODE workMode,UINT16 delaytime,UINT8 *ber);




#ifdef __cplusplus
}
#endif

#endif
