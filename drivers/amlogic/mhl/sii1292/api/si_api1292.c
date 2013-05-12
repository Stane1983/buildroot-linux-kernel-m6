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


#include "../api/si_regio.h"
#include "../api/si_api1292.h"
#include "../api/si_1292regs.h"
#include "../cbus/si_cbus_regs.h"
#include "../cbus/si_apicbus.h"
#include "../main/si_cp1292.h"
#include "../hal/si_hal.h"


#ifdef MSC_TESTER
extern void 	start_msc_tester(void);
#endif

//------------------------------------------------------------------------------
// Target System specific
//------------------------------------------------------------------------------

#if (FPGA_DEBUG_CODE == 1)
    char strDeviceID []             = "1292(FPGA)";
#else
    char strDeviceID []             = "1292";
#endif

//------------------------------------------------------------------------------
// Debug Trace
//------------------------------------------------------------------------------

#if (API_DEBUG_CODE==1)
    char        g_debugTraceVerStr[5]   = "0003";   // Changed each time the debug labels are
                                                    // changed to keep track of what values
                                                    // a specific version is monitoring.
    uint8_t     g_demFlags1           = 0;
    uint8_t     g_demFlags2           = DF2_MUTE_ON;
#endif

//------------------------------------------------------------------------------
// Device Event Monitor variables
//------------------------------------------------------------------------------

BOOL		    g_firstPass     = true;
uint32_t		g_pass			= 0;	// For time counting

static uint8_t  HDMI_connect_wait_count     = 0;	// For starter kiter

#ifdef SWWA
static uint8_t Tx_TMDS_toggle = 0;
#endif // SWWA

extern bool_t mhl_con;

//------------------------------------------------------------------------------
// Function:    SI_DeviceStatus
// Description: Return the requested status from the device.
//------------------------------------------------------------------------------

uint8_t SI_DeviceStatus ( uint8_t statusIndex )
{
    uint8_t regValue = 0;

    switch ( statusIndex )
    {

        case SI_DEV_IDH:
            regValue = SiIRegioRead( REG_DEV_IDH_RX );
            break;
        case SI_DEV_IDL:
            regValue = SiIRegioRead( REG_DEV_IDL_RX );
            break;
        case SI_DEV_REV:
            regValue = SiIRegioRead( REG_DEV_REV ) & 0x0F;
            break;

    }

    return( regValue );
}



//------------------------------------------------------------------------------
// Function:    GatherDeviceEvents
// Description:
//
//------------------------------------------------------------------------------

static uint8_t GatherDeviceEvents ( void )
{
	uint8_t			events = 0;
    uint8_t         uData;

    uData = SiIRegioRead( REG_INTR1 );

	if (uData & INT_CBUS_CON)
	{
		events |= MHL_EVENT;
		SiIRegioModify(REG_INTR1, INT_CBUS_CON, INT_CBUS_CON);
	}
	else
	{
		if (uData & INT_RPWR)
		{
			//events |= HDMI_EVENT;
			if (!mhl_con)
				events |= HDMI_EVENT;
			//HDMI_connect_wait_count = 1;

			SiIRegioModify(REG_INTR1, INT_RPWR, INT_RPWR);
		}
	}

	if (uData & INT_HPD)
	{
		events |= TV_EVENT;
		SiIRegioModify(REG_INTR1, INT_HPD, INT_HPD);
	}

	if (uData & INT_RSEN)
	{
		events |= RSEN_EVENT;
		SiIRegioModify(REG_INTR1, INT_RSEN, INT_RSEN);
	}

#ifdef SWWA
	if (uData & INT_SCDT)
	{
		events |= SCDT_EVENT;
		SiIRegioModify(REG_INTR1, INT_SCDT, INT_SCDT);
	}
#else // SWWA
		if (uData & INT_SCDT)
		{
		//	events |= SCDT_EVENT;

			//Keno20120315, fix MHL CTS4.3.24.1.
			if(mhl_con)
				SiIRegioModify(0x07A, 0x40, 0x40);
			else
				SiIRegioWrite(0x07A, 0xD0);

#ifdef BOARD_JULIPORT	   //for auto reset on SCDT
//		SiIRegioWrite(0x005, 0x28); // Hold CEC in Reset

	if(mhl_con)
		SiIRegioModify(0x07A, 0x40, 0x40);
	else
		SiIRegioWrite(0x07A, 0xD0);

		SiIRegioWrite(0x00C, 0x11); // DATA_MUX_1
		SiIRegioWrite(0x00D, 0x0B); // DATA_MUX_2

#endif

			SiIRegioModify(REG_INTR1, INT_SCDT, INT_SCDT);
		}
#endif
		if (uData & INT_CKDT)
		{
		//	events |= SCDT_EVENT;
			SiIRegioModify(REG_INTR1, INT_CKDT, INT_CKDT);
		}
	return events;

}


static void ProcessMhlEvent (void)
{
	uint8_t uData;

	uData = SiIRegioRead(REG_CBUS_BUS_STATUS);
	//DEBUG_PRINT(MSG_ALWAYS, ("REG_CBUS_BUS_STATUS = %02X\n", (int)uData));

	if (uData & BIT_BUS_CONNECTED)
	{
		//Set g_currentInputMode to MHL, then let CBUS driver handle everything
		g_currentInputMode = MHL;

		//Enable MHL interrupts
		SiIRegioWrite(REG_INTR1_MASK, 0xFF);
		SiIRegioWrite(REG_CBUS_INTR_ENABLE, 0xFF);

		g_MHL_Source_Connected = true;
		DEBUG_PRINT(MSG_ALWAYS, "MHL Source device Connected\n");

		if(HDMI_connect_wait_count)
			HDMI_connect_wait_count = 0;	// So it's a MHL device, clean the HDMI event
		//init CPI core
#ifdef CONFIG_MHL_SII1292_CEC
		SI_CpiInit();
#endif

		GPIO_CLR(LED2_MHL);

		// Jin: For Bring-up
#if (FPGA_BUILD == 0)
		SiIRegioModify(0x073, 0x02, 0x02); // change mode to MHL
		SiIRegioModify(0x071, 0x02, 0x02);
		SiIRegioWrite(0x06C, 0x08);
		//SiIRegioWrite(0x009, 0x0);
#endif // FPGA_BUILD == 0

#ifdef MSC_TESTER
		start_msc_tester();
#endif // MSC_TESTER
	}
	else
	{
		g_MHL_Source_Connected = false;
		DEBUG_PRINT(MSG_ALWAYS, "MHL Source device Unconnected\n");
		GPIO_SET(LED2_MHL);
	}

}



static void ProcessHdmiEvent (void)
{
	uint8_t uData;
	static bool_t print_connected = false;

	uData = SiIRegioRead(REG_STATE);

	if (uData & BIT_PWR5V)
	{
		g_HDMI_Source_Connected = true;
		print_connected = true;
		DEBUG_PRINT(MSG_ALWAYS, "HDMI Source device Connected\n");
	}
	else
	{
		g_HDMI_Source_Connected = false;
		g_HDMI_Source_Valid = false;
		g_currentInputMode = MHL;

		if(print_connected)
		{
			DEBUG_PRINT(MSG_ALWAYS, "HDMI Source device Unconnected\n");
			print_connected = false;
		}
		// Disable HDMI Mode
		//SiIRegioModify(REG_SYS_CTRL1, BIT_HDMI_MODE|BIT_RX_CLK, 0);
		//Jin: bringup in stater kit
		SiIRegioWrite(REG_SYS_CTRL1, 0);
		// Disable Rx Termination Core
		SiIRegioModify(REG_RX_CTRL5, BIT_HDMI_RX_EN, 0);

		GPIO_SET(LED3_HDMI);
	}
}



static void ProcessTvEvent (void)
{
	uint8_t uData;

	uData = SiIRegioRead(REG_STATE);
	//DEBUG_PRINT(MSG_ALWAYS, ("REG_STATE = %02X\n", (int)uData));

	SI_CbusInitDevCap(SI_CbusPortToChannel(g_data.portSelect));

	if (uData & BIT_HPD)
	{
#if !defined(CONFIG_MHL_SII1292_CEC)
		if(g_HPD_IN)
			return;
#endif

		g_HPD_IN = true; // true means high

		DEBUG_PRINT(MSG_ALWAYS, "HDMI Tx HPD Detected - 1\n");
#if defined(CONFIG_MHL_SII1292_CEC)
		//RG add code to read EDID from DS display

		//set I2C switch to DDC mode
		SiIRegioModify(REG_DDC_SW_CTRL, BIT_I2C_DDC_SW_EN, BIT_I2C_DDC_SW_EN);
		//read EDID information
		HalI2cWriteByte(0x60,0x00, 0x00);	//set segment pointer
		HalI2cWriteByte(0xA0,0x00, 0x00);	//set start address
		//Update PA
		g_cecPhysical = SI_EdidGetPA();
		if(!g_cecPhysical)
		{
			// change back to default value
			g_cecPhysical = 0x1000;
			DEBUG_PRINT(MSG_ALWAYS, "Error in reading CEC physical address, change it to default value...\n");
		}
		DEBUG_PRINT(MSG_ALWAYS, "CEC physical address: %04X\n", (int)g_cecPhysical);
		//Switch back

		//perform dummy read of 0xd0:0x84 as first read returns 0xff after SW is set to DDC position
		HalI2cReadByte(0xD0,0x84);
		//set I2C switch to DDC mode
		SiIRegioModify(REG_DDC_SW_CTRL, BIT_I2C_DDC_SW_EN, 0);

//		InitCECOnHPD();
#endif
	}
	else
	{
		if(!g_HPD_IN)
			return;

		g_HPD_IN = false; // false means low

		DEBUG_PRINT(MSG_ALWAYS, "HPD Undetected\n");
	}
    SiIRegioWrite(REG_CBUS_INTR_STATUS, BIT_MSC_MSG_RCV);//tiger , 12-07-2-11, bit3 hardware defualt value is 1,clear it when init;

#if (FPGA_BUILD == 0)
	if (g_currentInputMode == HDMI)
	{
		SiIRegioModify(0x073, 0x02, 0); // change mode to HDMI
		SiIRegioWrite(0x071, 0xA0);
		SiIRegioWrite(0x06C, 0x3F);
		SiIRegioWrite(0x070, 0xA8);
		SiIRegioWrite(0x009, 0x3E);
#ifdef BOARD_JULIPORT
//		SiIRegioWrite(0x005, 0x28); // Hold CEC in Reset
		SiIRegioWrite(0x07A, 0xD0);	//disable power out in HDMI input mode
		SiIRegioWrite(0x00C, 0x11); // DATA_MUX_1
		SiIRegioWrite(0x00D, 0x0B); // DATA_MUX_2
#endif
#ifdef BOARD_CP1292
		SiIRegioWrite(0x07A, 0xD0);
		SiIRegioWrite(0x00C, 0x45); // DATA_MUX_1
		SiIRegioWrite(0x00D, 0x0E); // DATA_MUX_2
#endif
	}
	else
	{
		SiIRegioModify(0x073, 0x02, 0x02); // change mode to MHL
		SiIRegioWrite(0x071, 0xA2);
		SiIRegioWrite(0x06C, 0x08);
		SiIRegioWrite(0x070, 0x48);
		SiIRegioWrite(0x009, 0x0);

		//Keno20120315, fix MHL CTS4.3.24.1.
		if(mhl_con)
			SiIRegioWrite(0x07A, 0x40);
		else
			SiIRegioWrite(0x07A, 0xD0);

#ifdef BOARD_JULIPORT
//		SiIRegioWrite(0x005, 0x28); // Hold CEC in Reset
	if(mhl_con)
		SiIRegioWrite(0x07A, 0x40);
	else
		SiIRegioWrite(0x07A, 0xD0);

		SiIRegioWrite(0x00C, 0x11); // DATA_MUX_1
		SiIRegioWrite(0x00D, 0x0B); // DATA_MUX_2
#endif
#ifdef BOARD_CP1292
		SiIRegioWrite(0x07A, 0xD0);
		SiIRegioWrite(0x00C, 0x45); // DATA_MUX_1
		SiIRegioWrite(0x00D, 0x0E); // DATA_MUX_2
#endif
	}
#endif // FPGA_BUILD == 0

}



static void ProcessRsenEvent (void)
{
	uint8_t uData;

	uData = SiIRegioRead(REG_STATE2);

	if(uData)
	{
		g_TX_RSEN = true;
	}
	else
	{
		g_TX_RSEN = false;
		g_TX_RSEN_Valid = false;
	}

}



#ifdef SWWA
static void ProcessScdtEvent (void)
{
	uint8_t uData;

	uData = SiIRegioRead(REG_STATE);

	if(uData & BIT_SCDT)
	{
		if(Tx_TMDS_toggle == 0)
		{
			//DEBUG_PRINT(MSG_ALWAYS, ("DEBUG_INFO: 0x51[7] Low\n"));
			// Turn off/on Tx TMDS Core For HDCP on-off issue, Bug #19931
			SiIRegioModify(REG_TX_SWING1, BIT_SWCTRL_EN, 0);

			Tx_TMDS_toggle = 2;
			//SiIRegioModify(REG_TX_SWING1, BIT_SWCTRL_EN, BIT_SWCTRL_EN);
			//DEBUG_PRINT(MSG_ALWAYS, ("DEBUG_INFO: 0x51[7] High\n"));
		}
	}

}
#endif // SWWA


static void ProcessDeviceEvents ( uint8_t events )
{

	if (events & MHL_EVENT)
		ProcessMhlEvent();

	if (events & HDMI_EVENT)
		ProcessHdmiEvent();

	if (events & TV_EVENT)
		ProcessTvEvent();

	if (events & RSEN_EVENT)
		ProcessRsenEvent();

#ifdef SWWA
	if (events & SCDT_EVENT)
		ProcessScdtEvent();
#endif // SWWA

}

static void TxRsenValidCheck (void)
{
	static bool_t timer_set = false;
	static uint32_t count = 0;

	if (g_TX_RSEN)
	{
		if (!timer_set)
		{
			timer_set = true;
			count = jiffies;
		}
		else
		{
			if( HalTimerDelay(count, DEM_RSEN_VALID_TIME) )
			{
				g_TX_RSEN_Valid = true;
				count = 0;
				timer_set= false;
				DEBUG_PRINT(MSG_ALWAYS, "TX RSEN Stable\n");
				DEBUG_PRINT(MSG_ALWAYS, "g_currentInputMode = %d\n",(int)g_currentInputMode);
				if (g_currentInputMode == HDMI)
					// enable Rx Core termination
					SiIRegioModify(REG_RX_CTRL5, BIT_HDMI_RX_EN, BIT_HDMI_RX_EN);
			}
		}
	}
    else
    {
        timer_set = false;
        count = 0;
    }

}

static void ClearCbusInterrupts(void)
{

    SiIRegioWrite( REG_CBUS_INTR_STATUS, 0xFF );
	SiIRegioWrite( REG_CBUS_LINK_STATUS_2, 0 );
    SiIRegioWrite( REG_CBUS_REQUESTER_ABORT_REASON, 0xFF );
    SiIRegioWrite( REG_CBUS_RESPONDER_ABORT_REASON, 0xFF );
    SiIRegioWrite( REG_CBUS_DDC_ABORT_REASON, 0xFF );
    SiIRegioWrite( REG_CBUS_CEC_ABORT_REASON, 0xFF );

}

static void HdmiSourceValidCheck (void)
{
	static bool_t timer_set = false;
	static uint32_t count = 0;

	if (g_HDMI_Source_Connected)
	{

		if (!timer_set)
		{
			timer_set = true;
			count = jiffies;
		}
        else
        {
			if( HalTimerDelay(count, DEM_HDMI_VALID_TIME) )
            {
                uint8_t uData;

                g_HDMI_Source_Valid = true;
                g_currentInputMode = HDMI;

                // Jin: Fro Bring-up
            #if (FPGA_BUILD == 0)
                SiIRegioModify(0x073, 0x02, 0); // change mode to HDMI
                SiIRegioWrite(0x071, 0xA0);
                SiIRegioWrite(0x06C, 0x3F);
                SiIRegioWrite(0x070, 0xA8);
                SiIRegioWrite(0x009, 0x3E);
            #endif // FPGA_BUILD == 0

                // Disable CBUS interrupt
                SiIRegioWrite(REG_INTR1_MASK, 0xFE);
                SiIRegioWrite(REG_CBUS_INTR_ENABLE, 0);

                // Clear CBUS interrupts
                uData = SiIRegioRead(REG_CBUS_INTR_STATUS);
                if(uData)
                    ClearCbusInterrupts();

                SiIRegioWrite(REG_INTR1, INT_CBUS);

                count = 0;
                DEBUG_PRINT(MSG_ALWAYS, "HDMI Source device Stable\n");

                GPIO_CLR(LED3_HDMI);
                // Confirm HDMI MODE
                //SiIRegioModify(REG_SYS_CTRL1, BIT_HDMI_MODE|BIT_RX_CLK, BIT_HDMI_MODE|BIT_RX_CLK);
                //Jin: bringup in starter kit
                SiIRegioWrite(REG_SYS_CTRL1, 0xF8);

                // Enable HDMI Rx Core Termination
                if (g_TX_RSEN_Valid)
                    SiIRegioModify(REG_RX_CTRL5, BIT_HDMI_RX_EN, BIT_HDMI_RX_EN);

            }
        }
	}
	else
    {
        timer_set = false;
		count = 0;	// re-count
    }

}



//------------------------------------------------------------------------------
// Function:    SI_DeviceEventMonitor
// Description: Monitors important events in the life of the Main pipe and
//              intervenes where necessary to keep it running clean.
//
// NOTE:        This function is designed to be called at 100 millisecond
//              intervals. If the calling interval is changed, the
//              DEM_POLLING_DELAY value must be changed also to maintain the
//              proper intervals.
//
//------------------------------------------------------------------------------

void SI_DeviceEventMonitor ( void )
{
	uint8_t events = 0;

	//DEBUG_PRINT(MSG_DBG, ("[%d] ", (int)g_pass));
	g_pass++;
	if((int)g_pass < 0)//if overflow
	{
		g_pass = 0;
	}

	if (g_firstPass)
	{
		BOOL mhl_cable_con = false;

		g_firstPass = false;

		//SiIRegioModify(REG_RX_MISC, BIT_PSCTRL_OEN|BIT_PSCTRL_OUT|BIT_PSCTRL_AUTO, BIT_PSCTRL_OUT);

		//enable all INT mask
		SiIRegioWrite(REG_INTR1_MASK, 0xFF);

#if 0//def BOARD_JULIPORT//Keno20120301, disable RK design, because it will effect that cannot display
		//enable mhl cable connect based on  V5DET 0x4E[0] mask at 0x4E[1], if high them MHL cable det.
		SiIRegioWrite(REG_INTR2_STATUS_MASK,0x02);
#endif

		//disable auto HPD/RSEN generation module
		SiIRegioModify(REG_RX_MISC, BIT_PSCTRL_OEN|BIT_PSCTRL_OUT|BIT_PSCTRL_AUTO|BIT_HPD_RSEN_ENABLE|BIT_5VDET_OVR, BIT_PSCTRL_OEN|BIT_PSCTRL_OUT|BIT_5VDET_OVR);

		mhl_cable_con = CpCableDetect();

		// Jin: for INT_RPWR/INT_CBUS doesn't show at first process
		{
			uint8_t	 uData = SiIRegioRead(REG_STATE);

			if (uData & BIT_PWR5V)
			{
				//HDMI_connect_wait_count = 1;
				if(!mhl_con)
					events |= HDMI_EVENT;
			}

			if(mhl_cable_con)
			{
				if (uData & BIT_HPD)
				{
					g_HPD_IN = true; // true means high

					DEBUG_PRINT(MSG_ALWAYS, "HDMI Tx HPD Detected - 2\n");
				}
			}

			uData = SiIRegioRead(REG_CBUS_BUS_STATUS);

			if (uData & BIT_BUS_CONNECTED)
				events |= MHL_EVENT;
		}

	}

#if 0//def BOARD_JULIPORT//Keno20120301, disable RK design, because it will effect that cannot display
		if(SiIRegioRead(REG_INTR2_STATUS_MASK) & 0x01)
		{
			CpCableDetect();
			SiIRegioModify(REG_INTR2_STATUS_MASK, 0x01, 0x01);
		}
#endif
#if 1//def BOARD_CP1292//Keno20120301, enable RK design, because it will effect that cannot display
		CpCableDetect();
#endif

   	events |= GatherDeviceEvents();       // Gather info for this pass
	//DEBUG_PRINT(MSG_ALWAYS, ("events = %02X\n", (int)events));

	/*if (HDMI_connect_wait_count)
	{
		if (HDMI_connect_wait_count == 5)	// wait at least 200ms for CBUS discovery
		{
			events |= HDMI_EVENT;
			HDMI_connect_wait_count = 0;
		}
		else
		{
			HDMI_connect_wait_count++;
		}
	}*/

	ProcessDeviceEvents(events);

#ifdef SWWA
	if (Tx_TMDS_toggle)
	{
		Tx_TMDS_toggle --;

		if (Tx_TMDS_toggle == 0)
		{
			//DEBUG_PRINT(MSG_ALWAYS, ("DEBUG_INFO: 0x51[7] High\n"));
			SiIRegioModify(REG_TX_SWING1, BIT_SWCTRL_EN, BIT_SWCTRL_EN);
		}
	}
#endif // SWWA

	// make sure TX_RSEN show 1s
	if (g_TX_RSEN_Valid == false)
		TxRsenValidCheck();

	if (g_HDMI_Source_Valid == false)
		HdmiSourceValidCheck();

}
