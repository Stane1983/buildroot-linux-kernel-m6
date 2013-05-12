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



#ifndef MHL_SII1292_REGIO_H
#define MHL_SII1292_REGIO_H

#include "../api/si_datatypes.h"

#define REGIO_SLAVE_PAGE_0          (0xD0)	// Main
#define REGIO_SLAVE_PAGE_1          (0x00)
#define REGIO_SLAVE_PAGE_2          (0x66)
#define REGIO_SLAVE_PAGE_3          (0x00)
#define REGIO_SLAVE_PAGE_4          (0x68)
#define REGIO_SLAVE_PAGE_5          (0x00)
#define REGIO_SLAVE_PAGE_6          (0x00)
#define REGIO_SLAVE_PAGE_7          (0x00)
#define REGIO_SLAVE_PAGE_8          (0xC0)      // CEC
#define REGIO_SLAVE_PAGE_9          (0xA0)      // EDID
#define REGIO_SLAVE_PAGE_A          (0x64)
#define REGIO_SLAVE_PAGE_B			(0x90)
#define REGIO_SLAVE_PAGE_C          (0xC8)      // CBUS
#define REGIO_SLAVE_PAGE_D          (0xD0)
#define REGIO_SLAVE_PAGE_E          (0xE8)
#define REGIO_SLAVE_PAGE_F          (0x00)

//-------------------------------------------------------------------------------
//  Silicon Image Device Register I/O Function Prototypes
//-------------------------------------------------------------------------------

uint8_t SiIRegioRead( uint16_t regAddr );
void    SiIRegioWrite( uint16_t regAddr, uint8_t value);
void    SiIRegioModify( uint16_t regAddr, uint8_t mask, uint8_t value);
void    SiIRegioBitToggle( uint16_t regAddr, uint8_t mask);
void    SiIRegioReadBlock( uint16_t regAddr, uint8_t* buffer, uint16_t length);
void    SiIRegioWriteBlock( uint16_t regAddr, uint8_t* buffer, uint16_t length);

void    SiIRegioSetBase( uint8_t page, uint8_t newID );

#endif /* MHL_SII1292_REGIO_H */

