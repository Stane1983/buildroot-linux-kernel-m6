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


#ifndef __SI_API1292_H__
#define __SI_API1292_H__

#include "si_datatypes.h"


//------------------------------------------------------------------------------
//  Compile-time build options
//------------------------------------------------------------------------------
//#define MSC_TESTER

#define API_DEBUG_CODE          1       // Set to 0 to eliminate debug print messages from code
                                        // Set to 1 to allow debug print messages in code
#define FPGA_DEBUG_CODE         0       // Set to 0 to for normal operation
                                        // Set to 1 to to work with FPGA test board
//#define CHIP_REVISION           ES10    // Set according to silicon revision

#define TIME_STAMP
//#define SWWA						// For software workaround
#define SWWA_21970					// SWWA for Bug#21970
#define SWWA_22334					// SWWA for Bug#22334

/* Control for amount of text printed by CEC commands.  */




#define CEC_NO_TEXT         0
#define CEC_NO_NAMES        1
#define CEC_ALL_TEXT        2           // Not compatible with IS_CDC==1 for 8051
#define INCLUDE_CEC_NAMES   CEC_ALL_TEXT

//------------------------------------------------------------------------------
//  Chip revision constants
//------------------------------------------------------------------------------

#define ES00       0
#define ES01       1
#define ES10       2

#define SII_MAX_PORT    1

//------------------------------------------------------------------------------
// Target System specific
//------------------------------------------------------------------------------
#define IS_RX                   1       // Set to 1 to if code is for Sink device
#define IS_TX                   0       // Set to 1 to if code is for Source device

    /* The following flags determine what feature set is compiled.  */

#define IS_CEC                  0// 1       // Set to 1 to enable CEC support //Keno20120301, base on RK1292 CEC to do. If don't want to use CEC functino, please define to 0.
#define IS_CBUS                 1       // Set to 1 to enable MHL CBUS support
#define IS_IR               	0// 1       // Set to 1 to enable IR

#if IS_IR
//choose one, cannot both defined to 1.
#define IS_CP_Remote_Controller	1
#define IS_RK_Remote_Controller	0
#endif

//Keno20120306, add for reservation to SOC/Scaler use.
//CP/RK Remote Controller(RC) simulate SOC/Scaler RC to send CONTENT_ON/CONTENT_OFF to MHL Source.
//For MHL CTSv1.1, MHL Sink should not be tested it, therefore it can be disabled.
#define USE_Scaler_Standby_Mode 1

extern char strDeviceID [];

    // Debug Macros


#if (API_DEBUG_CODE==1)
    #define DF1_SW_HDCPGOOD         0x01
    #define DF1_HW_HDCPGOOD         0x02
    #define DF1_SCDT_INT            0x04
    #define DF1_SCDT_HI             0x08
    #define DF1_SCDT_STABLE         0x10
    #define DF1_HDCP_STABLE         0x20
    #define DF1_NON_HDCP_STABLE     0x40
    #define DF1_RI_CLEARED          0x80

    #define DF2_MP_AUTH             0x01
    #define DF2_MP_DECRYPT          0x02
    #define DF2_HPD                 0x04
    #define DF2_HDMI_MODE           0x08
    #define DF2_MUTE_ON             0x10
    #define DF2_PORT_SWITCH_REQ     0x20
    #define DF2_PIPE_PWR            0x40
    #define DF2_PORT_PWR_CHG        0x80

typedef struct
{
    char    debugTraceVerStr[5];    // Changed each time the debug labels are
                                    // changed to keep track of what values
                                    // a specific version is monitoring.
    uint8_t demFlags1;
    uint8_t demFlags2;
} DEBUG_DATA_t;

extern DEBUG_DATA_t g_dbg;

#endif

   // MSG API   - For Debug purposes

#define MSG_ALWAYS              KERN_ERR
#define MSG_STAT                KERN_INFO
#define MSG_DBG                 KERN_DEBUG

//void    *malloc( uint16_t byteCount );
//void    free( void *pData );

#if (INCLUDE_CEC_NAMES > CEC_NO_TEXT)
    #include "../cec/si_apicpi.h"
    #include "../main/si_cpcec.h"
#endif


//------------------------------------------------------------------------------
//
//  Other Manifest Constants
//
//------------------------------------------------------------------------------

#define DEM_POLLING_DELAY   50         // Main Loop Polling interval (ms)

typedef enum
{
    SI_DEV_INPUT_CONNECTED  = 1,
    SI_DEV_BOOT_STATE_MACHINE,
    SI_DEV_NVRAM,
    SI_DEV_IDH,
    SI_DEV_IDL,
    SI_DEV_REV,
    SI_DEV_ACTIVE_PORT
} SI_DEV_STATUS;

//-------------------------------------------------------------------------------
//  API Function Prototypes
//-------------------------------------------------------------------------------

uint8_t SI_DeviceStatus( uint8_t statusIndex );
enum
{
    SI_PWRUP_FAIL_DEVID_READ    = 0x00,
    SI_PWRUP_FAIL_BOOT          = 0x01,
    SI_PWRUP_FAIL_NVRAM_INIT    = 0x02,
    SI_PWRUP_BASE_ADDR_B0       = 0xB0,
    SI_PWRUP_BASE_ADDR_B2       = 0xB2

};

void    SI_DeviceEventMonitor( void );

enum
{
    SI_HPD_ACTIVE               = 0x00,     // HPD HI, HDCP, EDID, RX Term enabled
    SI_HPD_INACTIVE,                        // HPD LOW, HDCP, RX Term disabled
    SI_HPD_ACTIVE_EX,                       // EDID, RX Term enabled
    SI_HPD_INACTIVE_EX,                     // HPD HI, HDCP, EDID, RX Term disabled
    SI_HPD_TOGGLE,                  // Special use for CBUS connect
};

enum
{
    SI_TERM_HDMI                = 0x00,     // Enable for HDMI mode
    SI_TERM_MHL                 = 0x55,     // Enable for MHL mode
    SI_TERM_DISABLE             = 0xFF,     // Disable
};

#define RCP_DEBUG                       0

#define MHL_MAX_CHANNELS                1   // Number of MDHI channels

#define CBUS_CMD_RESPONSE_TIMEOUT       10      // In 100ms increments
#define CBUS_CONNECTION_CHECK_INTERVAL  40      // In 100ms increments

#define	CBUS_FW_COMMAND_TIMEOUT_SECS	1		// No response for 1 second means we hold off and move
#define	CBUS_FW_INTR_POLL_MILLISECS		50		// wait this to issue next status poll i2c read
#define	CBUS_FW_HOLD_OFF_SECS			2		// Allow bus to quieten down when ABORTs are received.

//Events definition
#define MHL_EVENT				0x01
#define HDMI_EVENT				0x02
#define TV_EVENT				0x04
#define RSEN_EVENT				0x08
#define SCDT_EVENT				0x10

//Wake up pulse definition
#define MIN_WAKE_PULSE_WIDTH_1		5
#define MAX_WAKE_PULSE_WIDTH_1		40
#define MIN_WAKE_PULSE_WIDTH_2		20
#define MAX_WAKE_PULSE_WIDTH_2		100

#define DEM_HDMI_VALID_TIME		500    	// HDMI valid time 500ms
#define DEM_RSEN_VALID_TIME		1000	// RSEN Valid time 1s
#define DEM_MHL_WAIT_TIME		600	// MHL wait time after connection 600ms
#define DEM_MHL_RCP_TIMEOUT		1000	// MHL Rcp/Rap wait time
#define DEM_MSC_MAX_REPLY_DELAY		500	// MSC Max reply delay for transfer done, 500ms
#define DEM_CEC_FEATURE_ABORT_MAX_DELAY	800	// CEC Feature abort delay, 1000ms according to CEC spec Chapter7, but rcpk need within 1000ms, so set it 800ms
#define DEM_CBUS_ABORT_WAIT		2000	// CBUS Abort wait time, 2000ms

#define TIMER_DEM_CEC_FEATURE_ABORT_MAX_DELAY	12// (500  / DEM_POLLING_DELAY)	// CEC Feature abort delay, 500ms

#endif  // __SI_API1292_H__


