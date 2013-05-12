/*******************************************************************************

          (c) Copyright Explore Semiconductor, Inc. Limited 2005
                           ALL RIGHTS RESERVED

--------------------------------------------------------------------------------

  File        :  MHL_Controller.c

  Description :  Implement the CEC to MHL.RCP conversion

*******************************************************************************/

//#include "main.h"

#include "MHL_If.h"
#include "MHL_Bus.h"
#include "CEC_Convert.h"

//--------------------------------------------------------------------------------------------------

unsigned char is_CBUS_Connect = 0;
unsigned char is_DCAP_Ready = 0;
unsigned char is_DCAP_Change = 0;
unsigned char is_Signal = 0;

unsigned char Set_HPD = 0;
unsigned char Set_PATH = 0;

unsigned int Signal_TimeCount = 0, CBUS_Stable_TimeCount = 0;
unsigned char LinkMode = 0, LinkModeBackup = 0;

//--------------------------------------------------------------------------------------------------

// Private Function
void Exchange_DCAP(void);

//==================================================================================================
//
// Public Function Implementation
//

void MHL_Bus_Reset()
{
	is_CBUS_Connect = 0;
	is_DCAP_Ready = 0;
	is_DCAP_Change = 0;
	Set_HPD = 0;
	Set_PATH = 0;

	LinkModeBackup = 0;

	MHL_Clock_Mode(0);
}

void MHL_Bus_Timer()
{
	Signal_TimeCount++;
	CBUS_Stable_TimeCount++;
}

void MHL_Bus_Task()
{
	// Check CBUS State
	if(MHL_Rx_CBUS_Connect()) {

		if(!is_CBUS_Connect) {

			/////////////////////////////////////////////////////////////////
			//
			MHL_MSC_Cmd_ACK(); // Send dummy ACK for compatibility
			//
			/////////////////////////////////////////////////////////////////

			// CBUS Connect
				is_CBUS_Connect = 1;
				DBG_printf("------------------------------\n");
				DBG_printf("LINK: CBUS Connect\n");

				// Set POW bit - The AC Power Supply is On
				MHL_MSC_Reg_Update(MSC_DEV_CAT, Device_Capability_Default[MSC_DEV_CAT] | POW);

				// Clear HPD
				MHL_MSC_Cmd_HPD(0);

				// Set DCAP_RDY
				if(MHL_MSC_Cmd_WRITE_STATE(MSC_STATUS_CONNECTED_RDY, DCAP_RDY)) {
					DBG_printf("LINK: Set DCAP_RDY OK\n");
				}
				else {
					DBG_printf("LINK: Set DCAP_RDY fail\n");
				}
				if(MHL_MSC_Cmd_WRITE_STATE(MSC_RCHANGE_INT, DCAP_CHG)) {
					DBG_printf("LINK: Set DCAP_CHG OK\n");
				}
				else {
					DBG_printf("LINK: Set DCAP_CHG fail\n");
				}

				Signal_TimeCount = 0;
				Set_HPD = 0;
				Set_PATH = 0;
		}
		else {
			CBUS_Stable_TimeCount = 0;

			if(!Set_PATH) {
				Set_PATH = 1;
				DBG_printf("-------------------------\n");

				// Set PATH_EN
				if(MHL_MSC_Cmd_WRITE_STATE(MSC_STATUS_LINK_MODE, PATH_EN)) {
					DBG_printf("LINK: Set PATH_EN OK\n");
				}
				else {
					DBG_printf("LINK: Set PATH_EN fail\n");
				}
			}

			// Propagate HPD
			if(!Set_HPD) {
				Set_HPD = 1;
				DBG_printf("-------------------------\n");

				// Set HPD
				if(MHL_MSC_Cmd_HPD(1)) {
					DBG_printf("LINK: Set HPD OK\n");
				}
				else {
					DBG_printf("LINK: Set HPD fail\n");
				}
			}
		}
	}
	else {
		if(is_CBUS_Connect) {
			// CBUS Disconnect
				is_CBUS_Connect = 0;
				DBG_printf("LINK: CBUS Disconnect\n");

				is_DCAP_Ready = 0;
				is_DCAP_Change = 0;
				LinkModeBackup = 0;
		}
		else {
			CBUS_Stable_TimeCount = 0;
		}
	}

	// Detect DACP Change
	if(is_DCAP_Change) {

		// Check DCAP_RDY and read Device Cap
		// (timeout if the signal from source is detected)
		if(MHL_MSC_Reg_Read(MSC_STATUS_CONNECTED_RDY) & DCAP_RDY) {
			is_DCAP_Change = 0;

			// Read DCAP
			if(is_DCAP_Ready == 0) {
				is_DCAP_Ready = 1;

				DBG_printf("LINK: Device Cap Ready\n");
				Exchange_DCAP();
			}
		}
	}
	else {
		unsigned char RCHANGE_INT_value;

		RCHANGE_INT_value = MHL_MSC_Reg_Read(MSC_RCHANGE_INT);

		// Check Device Cap change
		if(RCHANGE_INT_value & DCAP_CHG) {
			is_DCAP_Change = 1;
			is_DCAP_Ready = 0;
			DBG_printf("-------------------------\n");
			DBG_printf("LINK: Device Cap change\n");
		}
		// Scratchpad Transmit Handling
		if(RCHANGE_INT_value & GRT_WRT) {
#ifdef WRITE_BURST_CODE
			////////////////////////////////////////////////////////////////////////
			// Customer should implement their own code here
			//
			static unsigned int WriteBurstCount = 0;

			unsigned char BurstData[16] = {
				0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
			};
			BurstData[0] = Device_Capability_Default[3]; // ADOPTER_ID_H
			BurstData[1] = Device_Capability_Default[4]; // ADOPTER_ID_L
			BurstData[2] = WriteBurstCount >> 8;
			BurstData[3] = WriteBurstCount;

			DBG_printf("LINK: GRT_WRT %02bX\n", BurstData[2]);
			MHL_MSC_Cmd_WRITE_BURST(MSC_SCRATCHPAD, BurstData, 7);
			WriteBurstCount++;
			//
			////////////////////////////////////////////////////////////////////////
#endif
			MHL_MSC_Cmd_WRITE_STATE(MSC_RCHANGE_INT, DSCR_CHG);
		}
		// Scratchpad Receive Handling
		if(RCHANGE_INT_value & DSCR_CHG) { //  Higher priority
#ifdef WRITE_BURST_CODE
			////////////////////////////////////////////////////////////////////////
			// Customer should implement their own code here
			//
			int i;
			DBG_printf("LINK: DSCR_CHG\n");
			for(i=0; i< Device_Capability_Default[SCRATCHPAD_SIZE]; i++) {
				DBG_printf("%02hhx ", MHL_MSC_Reg_Read(MSC_SCRATCHPAD+i));
			} DBG_printf(("\n"));
			//
			////////////////////////////////////////////////////////////////////////
#endif
		}
		if(RCHANGE_INT_value & REQ_WRT) { // Lowest prioirty to Grant to Write
			DBG_printf("LINK:  REQ_WRT\n");
			MHL_MSC_Cmd_WRITE_STATE(MSC_RCHANGE_INT, GRT_WRT);
		}
	}

	// Check Link Mode and Path Enable
 	LinkMode = MHL_MSC_Reg_Read(MSC_STATUS_LINK_MODE);
	if(LinkModeBackup != LinkMode) {
		LinkModeBackup = LinkMode;

		// Switch the Clock Mode
		switch(LinkMode & CLK_MODE) {
			default:
			case CLK_MODE__Normal:
				MHL_Clock_Mode(0);
				DBG_printf("LINK: Set CLK_MODE__Normal\n");
				break;

			case CLK_MODE__PacketPixel:
				MHL_Clock_Mode(1);
				DBG_printf("LINK: Set CLK_MODE__PacketPixel\n");
				break;
		}

		// Update device information
		if(LinkMode & PATH_EN) {
			DBG_printf("LINK: Path Enable\n");
		}
		else {
			DBG_printf("LINK: Path Disable\n");
		}
	}

	// Check Signal
	if(MHL_Rx_Signal()) {
		if(!is_Signal) {
			if(Signal_TimeCount >= 3000/MHL_BUS_TIMER_PERIOD) {
				is_Signal = 1;
				DBG_printf("LINK: Rx Signal found\n");

				// DACP Change timeout if the signal from source is detected
				if(is_DCAP_Ready == 0) {
					is_DCAP_Ready = 1;

					DBG_printf("LINK: Exchange DCAP\n");
					Exchange_DCAP();
				}
				DBG_printf("-------------------------\n");
			}
		}
		else {
			Signal_TimeCount = 0;
		}
	}
	else {
		if(is_Signal) {
			if(Signal_TimeCount >= 300/MHL_BUS_TIMER_PERIOD) {
				is_Signal = 0;
				DBG_printf("LINK: Rx Signal missing\n");

				Link_State = LINK_STATE_Start;
			}
		}
		else {
			Signal_TimeCount = 0;
		}
	}
}

void Exchange_DCAP(void)
{
	unsigned char temp[2];

	MHL_MSC_Cmd_READ_DEVICE_CAP(FEATURE_FLAG, &temp[0]);
	if(temp[0] & RCP_SUPPORT) {
		DBG_printf("LINK: RCP support = 1\n");

		//	Setup and Enable RCP
		MHL_MSC_Cmd_READ_DEVICE_CAP(LOG_DEV_MAP, &temp[1]);
		if(temp[1] & LD_GUI) {
			// GUI = Menu Control
			DBG_printf("LINK: LD_GUI support = 1\n");
		}
	}
}

