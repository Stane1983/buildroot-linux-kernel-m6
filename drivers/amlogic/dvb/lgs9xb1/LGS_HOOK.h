/***************************************************************************************
*					             
*                           (c) Copyright 2011, LegendSilicon, beijing, China
*
*                                        All Rights Reserved
* Description :
* Notice:
***************************************************************************************/
#ifndef _DEMOD_HOOK_H_
#define _DEMOD_HOOK_H_

#include "LGS_TYPES.h"
#include <linux/string.h>

extern LGS_WAIT				LGS_Wait;
extern LGS_REGISTER_READ	LGS_ReadRegister;
extern LGS_REGISTER_WRITE	LGS_WriteRegister;
extern LGS_REGISTER_READ_MULTIBYTE	LGS_ReadRegister_MultiByte;
extern LGS_REGISTER_WRITE_MULTIBYTE	LGS_WriteRegister_MultiByte;

LGS_RESULT LGS_RegisterSetBit(UINT8 secAddr, UINT8 regAddr, UINT8 bit);
LGS_RESULT LGS_RegisterClrBit(UINT8 secAddr, UINT8 regAddr, UINT8 bit);

#endif
