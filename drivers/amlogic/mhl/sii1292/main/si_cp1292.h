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



#ifndef __MHL_SII1292_CP1292_H
#define __MHL_SII1292_CP1292_H

#include "../api/si_datatypes.h"


//------------------------------------------------------------------------------
//
//  Compile-time build options
//
//------------------------------------------------------------------------------

#define FPGA_BUILD          0           // 1 == FPGA, 0 == Silicon

//------------------------------------------------------------------------------
//
//  CP 1292 Starter Kit Manifest Constants
//
//------------------------------------------------------------------------------

//#define DEM_POLLING_DELAY   100         // Main Loop Polling interval (ms)


//-------------------------------------------------------------------------------
//  Chip Mode for CP 1292
//-------------------------------------------------------------------------------

typedef enum
{
	HDMI = 0,
	MHL
} MHL_Input_Mode_t;

//------------------------------------------------------------------------------
//  typedefs
//------------------------------------------------------------------------------

typedef struct
{
	uint8_t portSelect;
	uint8_t cecEnabled;
	uint8_t irEnabled;
	uint8_t edidLoad;
	bool_t	standby_mode;

} SI_CP1292Config_t;

//------------------------------------------------------------------------------
//  Global Data
//------------------------------------------------------------------------------

extern SI_CP1292Config_t    		g_data;

extern uint8_t 				g_currentInputMode;

extern bool_t 				g_MHL_Source_Connected;

extern bool_t 				g_HDMI_Source_Connected;
extern bool_t				g_HDMI_Source_Valid;

extern bool_t 				g_HPD_IN;
extern bool_t 				g_TX_RSEN;
extern bool_t 				g_TX_RSEN_Valid;

extern BOOL                 		g_deviceInterrupt;
extern uint8_t              		g_dbgPrinted;

extern uint8_t 				PeerDevCap[16];

extern uint32_t 			timer_count;

extern bool_t				g_RC5_Key_UP;

//-------------------------------------------------------------------------------
// Board-specific GPIO and sbit assigments
//-------------------------------------------------------------------------------
# if 0
#define DEV_INT                 	P3^2 // 1292 Interrupt pin
#define STANDBY_MODE			P0^7 // For testing wakeup pulse detection
#define LED7_GREEN			P2^0
#define LED7_AMBER			P2^1
#define LED2_MHL			P2^4
#define LED3_HDMI			P2^5
#define STOP_POLLING_MODE		P0^6 //Keno20120219, increase for stop whlie loop for debugging. if pin is high, code will run. else not.
#define DEBUGGING_MODE			P0^4 //Keno20120306, add for debugging


#define GPIO_SET( gpioNum )		gpio_##gpioNum = 1
#define GPIO_CLR( gpioNum )         	gpio_##gpioNum = 0
#define GPIO_BIT( gpioNum )         	gpio_##gpioNum
#define GPIO_READ( gpioNum )        	gpio_##gpioNum

sbit gpio_DEV_INT               	= DEV_INT;            // 1292 Interrupt pin
sbit gpio_STANDBY_MODE			= STANDBY_MODE;
sbit gpio_LED7_GREEN			= LED7_GREEN;
sbit gpio_LED7_AMBER			= LED7_AMBER;
sbit gpio_LED2_MHL			= LED2_MHL;
sbit gpio_LED3_HDMI			= LED3_HDMI;
sbit gpio_STOP_POLLING_MODE		= STOP_POLLING_MODE;
sbit gpio_DEBUGGING_MODE		= DEBUGGING_MODE;
#endif

#define DEV_INT                 	0
#define STANDBY_MODE			0
#define LED7_GREEN			0
#define LED7_AMBER			0
#define LED2_MHL			0
#define LED3_HDMI			0
#define STOP_POLLING_MODE		0
#define DEBUGGING_MODE			0

#define GPIO_SET( gpioNum )
#define GPIO_CLR( gpioNum )
#define GPIO_BIT( gpioNum )		0
#define GPIO_READ( gpioNum )		0


//-------------------------------------------------------------------------------
//  Function Prototypes
//-------------------------------------------------------------------------------

BOOL CpCheckExternalRequests( void );
void CpReadInitialSettings( void );
#if (FPGA_BUILD == 0)
void CpCheckOTPRev( void );
#endif // FPGA_BUILD == 0
void CpBlinkTilReset( uint8_t leds );

bool_t CpIrHandler( void );

// For MHL cable detect and wakeup pulse detect
BOOL CpCheckStandbyMode( void );
BOOL CpCableDetect( void );
BOOL CpWakePulseDetect( void );



/* si_cpCbus.c      */

void CpCbusHandler( void );
void CpCbusInitialize( void );

void BoardInit ( void );


#endif /* __MHL_SII1292_CP1292_H */

