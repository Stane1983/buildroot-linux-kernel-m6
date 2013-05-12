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


#ifndef __SI_APICPI_H__
#define __SI_APICPI_H__

#include "../api/si_datatypes.h"

//-------------------------------------------------------------------------------
// CPI Enums and manifest constants
//-------------------------------------------------------------------------------

#define SII_MAX_CMD_SIZE 16

typedef enum
{
    SI_TX_WAITCMD,
    SI_TX_SENDING,
    SI_TX_SENDACKED,
    SI_TX_SENDFAILED
} SI_txState_t;

typedef enum
{
    SI_CEC_SHORTPULSE       = 0x80,
    SI_CEC_BADSTART         = 0x40,
    SI_CEC_RXOVERFLOW       = 0x20,
    SI_CEC_ERRORS           = (SI_CEC_SHORTPULSE | SI_CEC_BADSTART | SI_CEC_RXOVERFLOW)
} SI_cecError_t;

//-------------------------------------------------------------------------------
// CPI data structures
//-------------------------------------------------------------------------------

typedef struct
{
    uint8_t srcDestAddr;            // Source in upper nybble, dest in lower nybble
    uint8_t opcode;
    uint8_t args[ SII_MAX_CMD_SIZE ];
    uint8_t argCount;
   	uint8_t nextFrameArgCount;
} SI_CpiData_t;

typedef struct
{
    uint8_t rxState;
    uint8_t txState;
    uint8_t cecError;

} SI_CpiStatus_t;


//-------------------------------------------------------------------------------
// CPI Function Prototypes
//-------------------------------------------------------------------------------

BOOL SI_CpiRead( SI_CpiData_t *pCpi );
BOOL SI_CpiWrite( SI_CpiData_t *pCpi );
BOOL SI_CpiStatus( SI_CpiStatus_t *pCpiStat );

BOOL SI_CpiInit( void );
BOOL SI_CpiSetLogicalAddr( uint8_t logicalAddress );
uint8_t	SI_CpiGetLogicalAddr( void );
void SI_CpiSendPing( uint8_t bCECLogAddr );

#endif // _SI_APICPI_H_




