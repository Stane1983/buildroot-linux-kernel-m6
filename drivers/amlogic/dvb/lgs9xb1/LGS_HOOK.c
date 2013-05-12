/***************************************************************************************
*					             
*			(c) Copyright 2011, LegendSilicon, beijing, China
*
*				         All Rights Reserved
*
* Description :
*
***************************************************************************************/
#include "LGS_HOOK.h"

LGS_WAIT		LGS_Wait = 0;
LGS_REGISTER_READ	LGS_ReadRegister = 0;
LGS_REGISTER_WRITE	LGS_WriteRegister = 0;

LGS_REGISTER_READ_MULTIBYTE	 LGS_ReadRegister_MultiByte = 0;
LGS_REGISTER_WRITE_MULTIBYTE LGS_WriteRegister_MultiByte = 0;

void LGS_DemodRegisterWait(LGS_WAIT wait)
{
	LGS_Wait = wait;
}

void LGS_DemodRegisterRegisterAccess(LGS_REGISTER_READ pread, LGS_REGISTER_WRITE pwrite, LGS_REGISTER_READ_MULTIBYTE	preadm, LGS_REGISTER_WRITE_MULTIBYTE pwritem)
{
	LGS_ReadRegister = pread;
	LGS_WriteRegister= pwrite;

	LGS_ReadRegister_MultiByte = preadm;
	LGS_WriteRegister_MultiByte = pwritem;
}

LGS_RESULT LGS_RegisterSetBit( UINT8 secAddr, UINT8 regAddr, UINT8 bit)
{
	UINT8		dat;
	LGS_RESULT	err = LGS_NO_ERROR;

	LGS_ReadRegister(secAddr, regAddr, &dat );
	dat |= bit;
	LGS_WriteRegister( secAddr, regAddr, dat );
	return err;
}

LGS_RESULT LGS_RegisterClrBit(UINT8 secAddr, UINT8 regAddr, UINT8 bit)
{
	UINT8		dat;
	LGS_RESULT	err = LGS_NO_ERROR;

	LGS_ReadRegister( secAddr, regAddr, &dat );
	dat &= ~(bit);
	LGS_WriteRegister( secAddr, regAddr, dat );
	return err;
}


