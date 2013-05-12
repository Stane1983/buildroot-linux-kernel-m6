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


#include "../api/si_datatypes.h"
#include "../api/si_api1292.h"
#include "../api/si_regio.h"
#include "../main/si_cp1292.h"
#include "../hal/si_hal.h"


//------------------------------------------------------------------------------
// Global Data
//------------------------------------------------------------------------------

SI_CP1292Config_t   g_data;
uint8_t g_currentInputMode = MHL;
bool_t g_MHL_Source_Connected = false;
bool_t g_HDMI_Source_Connected = false;
bool_t g_HDMI_Source_Valid = false;

bool_t g_HPD_IN = false;
bool_t g_TX_RSEN = false;
bool_t g_TX_RSEN_Valid = false;

uint8_t PeerDevCap[16] = {0};

bool_t g_RC5_Key_UP = false;//Keno20120219, when press remote controller key button, this moment has 2 status, one is press down, other is bounce. keu up is bounce.

//------------------------------------------------------------------------------
// Function:    CpDeviceInit
// Description: Initialize the 1292 deviice
//------------------------------------------------------------------------------
BOOL CpDeviceInit ( void )
{
    uint16_t    temp;
    BOOL        success = true;

    /* Perform a hard reset on the device to ensure that it is in a known   */
    /* state (also downloads a fresh copy of EDID from NVRAM).              */

    DEBUG_PRINT( MSG_ALWAYS,"\nPower up Initialize...");

    temp = ((uint16_t)SI_DeviceStatus( SI_DEV_IDH)) << 8;
    temp |= SI_DeviceStatus( SI_DEV_IDL );
    DEBUG_PRINT( MSG_ALWAYS, "Silicon Image Device: %04X, rev %02X\n\n", temp, (int)(SI_DeviceStatus( SI_DEV_REV ) & 0x0F) );

	g_data.portSelect = 0;

	// Jin: For manually PS_CNTL, avioding power-on time HDMI_DET pin voltage high issue.
	// Disable Q3 FET first
	//SiIRegioModify(REG_RX_MISC, BIT_PSCTRL_OEN|BIT_PSCTRL_OUT|BIT_PSCTRL_AUTO, BIT_PSCTRL_OUT);

    /*if ( temp != 0x1292 && temp != 0x9292 )
    {
        success = false;
    }*/

    return( success );
}


//------------------------------------------------------------------------------
// Function:    BoardInit
// Description: One time initialization at startup
//------------------------------------------------------------------------------
void BoardInit ( void )
{

    /* FPGA Initialization. */

#if (FPGA_BUILD == 1)
    char        i;
    DEBUG_PRINT( MSG_ALWAYS, "\n" );
    for ( i = 6; i >= 0; i-- )
    {
        DEBUG_PRINT( MSG_ALWAYS, "%d...", (int)i );
        HalTimerWait( 1000 );
    }
    DEBUG_PRINT( MSG_ALWAYS, "\n" );

    //DEBUG_PRINT( MSG_ALWAYS, ("FPGA HAL Version: %d.%02d\n", (int)HalVersionFPGA( 1 ), (int)HalVersionFPGA( 0 )));
#else
    //DEBUG_PRINT( MSG_ALWAYS, ("HAL Version: %d.%02d\n", (int)HalVersion( 1 ), (int)HalVersion( 0 )));
#endif

    /* Power up the chip, check it's type, and initialize to default values.    */
#if 0
    if ( !CpDeviceInit())
    {
        DEBUG_PRINT( MSG_ALWAYS, "\n!!!!Device Init failure, halting...\n" );
        CpBlinkTilReset( 0x06 );
    }
#endif

#if (FPGA_BUILD == 0)
	CpCheckOTPRev();						// check OTP Rev and update the registers
#ifdef BOARD_JULIPORT
//		SiIRegioWrite(0x005, 0x28); // Hold CEC in Reset
		SiIRegioWrite(0x00C, 0x11); // DATA_MUX_1
		SiIRegioWrite(0x00D, 0x0B); // DATA_MUX_2

#endif
#endif // FPGA_BUILD == 0
    CpReadInitialSettings();                // Read Jumper settings
#if defined(CONFIG_MHL_SII1292_CEC)
    if ( g_data.cecEnabled )
    {
        SI_CecInit();
		DEBUG_PRINT( MSG_ALWAYS, "CEC initial Logical Address is 0x%02X \n\n", (int)g_cecAddress);
    }
#endif

}

