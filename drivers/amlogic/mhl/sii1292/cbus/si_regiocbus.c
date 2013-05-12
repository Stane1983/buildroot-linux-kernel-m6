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


#include "../api/si_api1292.h"
#include "../api/si_regio.h"


#if (IS_TX == 1 )
static uint8_t l_cbusPortOffsets [ MHL_MAX_CHANNELS ] = { 0x00, 0x80 };
#else
static uint8_t l_cbusPortOffsets [ MHL_MAX_CHANNELS ] = { 0x00 };
#endif


//------------------------------------------------------------------------------
// Function:    SiIRegioCbusRead
// Description: Read a one byte CBUS register with port offset.
//              The register address parameter is translated into an I2C slave
//              address and offset. The I2C slave address and offset are used
//              to perform an I2C read operation.
//------------------------------------------------------------------------------
uint8_t SiIRegioCbusRead ( uint16_t regAddr, uint8_t port )
{
	return( SiIRegioRead( regAddr + l_cbusPortOffsets[ port] ));
}

//------------------------------------------------------------------------------
// Function:    SiIRegioCbusWrite
// Description: Write a one byte CBUS register with port offset.
//              The register address parameter is translated into an I2C
//              slave address and offset. The I2C slave address and offset
//              are used to perform an I2C write operation.
//------------------------------------------------------------------------------
void SiIRegioCbusWrite ( uint16_t regAddr, uint8_t channel, uint8_t value )
{
	SiIRegioWrite( regAddr + l_cbusPortOffsets[ channel], value );
}

//------------------------------------------------------------------------------
// Function:    SiIRegioCbusModify
// Description: Read/Write Modify CBUS channel 0 or CBUS channel 1
//------------------------------------------------------------------------------
void SiIRegioCbusModify ( uint16_t regAddr, uint8_t channel, uint8_t mask, uint8_t value )
{
	SiIRegioModify( regAddr + l_cbusPortOffsets[ channel], mask, value );
}


