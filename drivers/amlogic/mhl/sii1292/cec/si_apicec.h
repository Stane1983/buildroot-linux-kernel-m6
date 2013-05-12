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


#ifndef __SI_APICEC_H__
#define __SI_APICEC_H__

#include "../api/si_datatypes.h"

#ifdef CONFIG_MHL_SII1292_CEC

//-------------------------------------------------------------------------------
// CPI Enums, typedefs, and manifest constants
//-------------------------------------------------------------------------------

#define MAKE_SRCDEST( src, dest)    (( src << 4) | dest )

#define SII_NUMBER_OF_PORTS         1 //for Jubilee
#define SII_EHDMI_PORT              (1)

enum
{
    SI_CECTASK_IDLE,
    SI_CECTASK_ENUMERATE,
    SI_CECTASK_TXREPORT,
    SI_CECTASK_ONETOUCHPLAY,
    SI_CECTASK_SETOSDNAME,
};

typedef struct
{
    uint8_t deviceType;     // 0 - Device is a TV.
                            // 1 - Device is a Recording device
                            // 2 - Device is a reserved device
                            // 3 - Device is a Tuner
                            // 4 - Device is a Playback device
                            // 5 - Device is an Audio System
    uint8_t  cecLA;         // CEC Logical address of the device.
    uint16_t cecPA;         // CEC Physical address of the device.
} CEC_DEVICE;

extern uint8_t  g_cecAddress;       // Initiator
extern uint16_t g_cecPhysical;      // For TV, the physical address is 0.0.0.0

//------------------------------------------------------------------------------
// Data
//------------------------------------------------------------------------------

extern CEC_DEVICE   g_childPortList [SII_NUMBER_OF_PORTS];

//------------------------------------------------------------------------------
// API Function Templates
//------------------------------------------------------------------------------

void        si_CecSendMessage( uint8_t opCode, uint8_t dest );
void        SI_CecSendUserControlPressed( uint8_t keyCode );
void        SI_CecSendUserControlReleased( void );
BOOL	    SI_CecOneTouchPlay ( void );
BOOL        SI_CecEnumerate( void );
uint8_t     SI_CecHandler( uint8_t currentPort, BOOL returnTask );
uint8_t     SI_CecGetPowerState( void );
void        SI_CecSetPowerState( uint8_t newPowerState );
void        SI_CecSourceRemoved( uint8_t portIndex );
uint16_t    SI_CecGetDevicePA( void );
void        SI_CecSetDevicePA( uint16_t devPa );
BOOL        SI_CecInit( void );
uint8_t     SI_CecMscMsgConvertHandler(SI_CpiData_t *pCpi, uint8_t channel);
#endif
#endif // __SI_APICEC_H__

