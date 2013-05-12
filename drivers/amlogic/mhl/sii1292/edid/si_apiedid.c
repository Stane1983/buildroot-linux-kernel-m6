/*
 * Silicon Image SiI1292 Device Driver
 *
 * Author: Amlogic, Inc.
 *
 * Copyright (C) 2012 Amlogic Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "../hal/si_hal.h"
#include "../api/si_regio.h"
#include "../api/si_1292regs.h"
#include "../api/si_api1292.h"
#include "../main/si_cp1292.h"

#include "../edid/si_apiedid.h"

#define VMASK 0xe0

//------------------------------------------------------------------------------
// Function:    	wait
// Description: 	a short delay
// Parameters:
// Return:
//------------------------------------------------------------------------------
static void wait(void)
{
	uint32_t i;

	while(i < 1000)
	{
		i++;
	}
}



//------------------------------------------------------------------------------
// Function:    	EdidGetPA
// Description: 	Get PA from total offset
// Parameters:		Len_value: total offset of Video Data Block
// Return:	 	PA value
//				0x0: error
//------------------------------------------------------------------------------
static uint16_t EdidGetPA( uint8_t Len_value )
{
	uint16_t PA_value = 0x0;
	uint8_t PA_Hbyte = 0x0;
	uint8_t PA_Lbyte = 0x0;

	PA_Hbyte = SiIRegioRead( REG_DDC_L1 + Len_value );
	// if the return value is 0, try read it again
	if(!PA_Hbyte)
	{
		wait();
		PA_Hbyte = SiIRegioRead( REG_DDC_L1 + Len_value );
	}

	PA_Lbyte = SiIRegioRead( REG_DDC_L1 + Len_value + 1 );
	// if the return value is 0, try read it again
	if(!PA_Lbyte)
	{
		wait();
		PA_Lbyte = SiIRegioRead( REG_DDC_L1 + Len_value + 1 );
	}

	PA_value = (PA_value | PA_Hbyte) << 8;
	PA_value |= PA_Lbyte;

	return PA_value;
}



//------------------------------------------------------------------------------
// Function:    	EdidGetLen
// Description: 	Get Ln number for calculating physical address byte offset
// Parameters:		Len : PA offset calculate
// Return:	 	Ln value
//				0x0: error
//				0xff: not wanted value
//------------------------------------------------------------------------------
static uint8_t EdidGetLen( uint8_t *Len )
{
	uint8_t value;
	uint8_t Ven_value = 0;
	uint8_t Len_value = 0;

	value = SiIRegioRead( REG_DDC_L1 + *Len );
	if(value == 0)
	{
		wait();
		value = SiIRegioRead( REG_DDC_L1 + *Len);
	// if the return value is 0, try read it again
	}
	if (value == 0)
	{
		return 0;
	}
	Len_value = value & LMASK;
	Ven_value = ((value & VMASK) >> 5);

	if (Ven_value == 3)
	{
		return Len_value;
	}

	*Len += (Len_value + 1);

	// not the video data block
	return 0xff;
}



//------------------------------------------------------------------------------
// Function:    	CheckPAOffsetValid
// Description: 	Check the validation of physical address byte offset
// Parameters:		len : PA offset calculate
// Return:	 	result
//				0x0: invalid
//				0x1: valid
//------------------------------------------------------------------------------
static uint8_t CheckPAOffsetValid (uint8_t len)
{
	uint8_t value1, value2, value3;

	value1 = SiIRegioRead( REG_DDC_L1 + len + 1);
	value2 = SiIRegioRead( REG_DDC_L1 + len + 2);
	value3 = SiIRegioRead( REG_DDC_L1 + len + 3);
	if(value1 != 0x03)
		return false;
	if(value2 != 0x0C)
		return false;
	if(value3 != 0x00)
		return false;
	return true;
}



//------------------------------------------------------------------------------
// Function:    	CheckPAValid
// Description: 	Check the validation of phsical address byte
// Parameters:		physical_address : physical address byte
// Return:	 	result
//				0x0: invalid
//				0x1: valid
//------------------------------------------------------------------------------
static uint8_t CheckPAValid (uint16_t physical_address)
{
	uint16_t p1,p2,p3,p4;
	p1 = ( physical_address  & 0xF000);
	p2 = ( physical_address  & 0x0F00);
	p3 = ( physical_address  & 0x00F0);
	p4 = ( physical_address  & 0x000F);

	if (p1 == 0)
	{
		return false;
	}
	if (p2 == 0)
	{
		if (p3 | p4)
			return false;
		else
			return true;
	}
	if (p3 == 0)
	{
		if (p4)
			return false;
		else
			return true;
	}
	return true;
}



//------------------------------------------------------------------------------
// Function:    	SI_EdidGetPA
// Description: 	Get physical address from downstream EDID
// Parameters:	void
// Return:	 	PA value
//				0x0: error
//------------------------------------------------------------------------------
uint16_t SI_EdidGetPA( void )
{
	uint16_t physical_address = 0x0000;
	uint8_t Len = 0;
	uint8_t status;
	bool_t ret;
	uint8_t i;

	for (i=0; i<10; i++)	// increase the loop count to 10 since some TV may have more blocks than 4 before VSDB
	{
		status = EdidGetLen(&Len);
		if (status == 0)
		{
			DEBUG_PRINT(MSG_ALWAYS, "Edid:: Error found in Reading L%d value\n", (int)i);
			return physical_address;
		}
		else if (status == 0xff)
		{
			continue;
		}
		else
		{
			ret = CheckPAOffsetValid(Len);
			if(ret == true)
			{
				physical_address = EdidGetPA(Len + 4);
				ret = CheckPAValid(physical_address);
				if (ret == true)
				{
					return physical_address;
				}
				else
				{
					DEBUG_PRINT(MSG_ALWAYS, "Edid:: Error found invalid PA value : %04X\n", (int)physical_address);
					physical_address = 0x0000;
					return physical_address;
				}
			}
			else
			{
				DEBUG_PRINT(MSG_ALWAYS, "Edid:: Error found invalid PA offset\n");
				return physical_address;
			}
		}
	}
	DEBUG_PRINT(MSG_ALWAYS, "Edid:: Error found no PA value\n");
	return physical_address;
}



