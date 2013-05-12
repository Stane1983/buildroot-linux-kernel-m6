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



 // Register Slave Address page map


#include <linux/types.h>

#include "../api/si_regio.h"
#include "../hal/si_i2c.h"


static uint8_t l_regioDecodePage [16] =
{
	REGIO_SLAVE_PAGE_0,
	REGIO_SLAVE_PAGE_1,
	REGIO_SLAVE_PAGE_2,
	REGIO_SLAVE_PAGE_3,
	REGIO_SLAVE_PAGE_4,
	REGIO_SLAVE_PAGE_5,
	REGIO_SLAVE_PAGE_6,
	REGIO_SLAVE_PAGE_7,
	REGIO_SLAVE_PAGE_8,
	REGIO_SLAVE_PAGE_9,
	REGIO_SLAVE_PAGE_A,
	REGIO_SLAVE_PAGE_B,
	REGIO_SLAVE_PAGE_C,
	REGIO_SLAVE_PAGE_D,
	REGIO_SLAVE_PAGE_E,
	REGIO_SLAVE_PAGE_F
};


//------------------------------------------------------------------------------
// Function:    SiIRegioRead
// Description: Read a one byte register.
//              The register address parameter is translated into an I2C slave
//              address and offset. The I2C slave address and offset are used
//              to perform an I2C read operation.
//------------------------------------------------------------------------------
uint8_t SiIRegioRead ( uint16_t regAddr )
{

    return (uint8_t)( HalI2cReadByte( l_regioDecodePage[ regAddr >> 8], (uint8_t)regAddr ));
}

//------------------------------------------------------------------------------
// Function:    SiIRegioWrite
// Description: Write a one byte register.
//              The register address parameter is translated into an I2C
//              slave address and offset. The I2C slave address and offset
//              are used to perform an I2C write operation.
//------------------------------------------------------------------------------
void SiIRegioWrite ( uint16_t regAddr, uint8_t value )
{

    HalI2cWriteByte( l_regioDecodePage[ regAddr >> 8], (uint8_t)regAddr, value );
}

//------------------------------------------------------------------------------
// Function:    SiIRegioReadBlock
// Description: Read a block of registers starting with the specified register.
//              The register address parameter is translated into an I2C
//              slave address and offset.
//              The block of data bytes is read from the I2C slave address
//              and offset.
//------------------------------------------------------------------------------
void SiIRegioReadBlock ( uint16_t regAddr, uint8_t* buffer, uint16_t length)
{
    bool_t result;
    result = HalI2cReadBlock( l_regioDecodePage[ regAddr >> 8], (uint8_t)regAddr, buffer, length);
}

//------------------------------------------------------------------------------
// Function:    SiIRegioWriteBlock
// Description: Write a block of registers starting with the specified register.
//              The register address parameter is translated into an I2C slave
//              address and offset.
//              The block of data bytes is written to the I2C slave address
//              and offset.
//------------------------------------------------------------------------------
void SiIRegioWriteBlock ( uint16_t regAddr, uint8_t* buffer, uint16_t length)
{
    bool_t result;
    result = HalI2cWriteBlock( l_regioDecodePage[ regAddr >> 8], (uint8_t)regAddr, buffer, length);
}

//------------------------------------------------------------------------------
// Function:    SiIRegioBitToggle
// Description: Toggle a bit or bits in a register
//              The register address parameter is translated into an I2C
//              slave address and offset.
//              The I2C slave address and offset are used to perform I2C
//              read and write operations.
//
//              All bits specified in the mask are first set and then cleared
//              in the register.
//              This is a common operation for toggling a bit manually.
//------------------------------------------------------------------------------
void SiIRegioBitToggle ( uint16_t regAddr, uint8_t mask)
{
    uint8_t slaveID, abyte;

    slaveID = l_regioDecodePage[ regAddr >> 8];

    abyte = HalI2cReadByte( slaveID, (uint8_t)regAddr );
    abyte |=  mask;                                         //first set the bits in mask
    HalI2cWriteByte( slaveID, (uint8_t)regAddr, abyte);        //write register with bits set
    abyte &= ~mask;                                         //then clear the bits in mask
    HalI2cWriteByte( slaveID, (uint8_t)regAddr, abyte);        //write register with bits clear
}

//------------------------------------------------------------------------------
// Function:    SiIRegioModify
// Description: Modify a one byte register under mask.
//              The register address parameter is translated into an I2C
//              slave address and offset. The I2C slave address and offset are
//              used to perform I2C read and write operations.
//
//              All bits specified in the mask are set in the register
//              according to the value specified.
//              A mask of 0x00 does not change any bits.
//              A mask of 0xFF is the same a writing a byte - all bits
//              are set to the value given.
//              When only some bits in the mask are set, only those bits are
//              changed to the values given.
//------------------------------------------------------------------------------
void SiIRegioModify ( uint16_t regAddr, uint8_t mask, uint8_t value)
{
    uint8_t slaveID, abyte;

    slaveID = l_regioDecodePage[ regAddr >> 8];

    abyte = HalI2cReadByte( slaveID, (uint8_t)regAddr );
    abyte &= (~mask);                                       //first clear all bits in mask
    abyte |= (mask & value);                                //then set bits from value
    HalI2cWriteByte( slaveID, (uint8_t)regAddr, abyte);
}


