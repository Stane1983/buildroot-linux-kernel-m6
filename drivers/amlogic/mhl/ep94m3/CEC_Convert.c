/*******************************************************************************

          (c) Copyright Explore Semiconductor, Inc. Limited 2005
                           ALL RIGHTS RESERVED

--------------------------------------------------------------------------------

  File        :  MHL_Controller.c

  Description :  Implement the MHL.RCP conversion

*******************************************************************************/


//#include "main.h"

#include "MHL_If.h"
#include "MHL_Bus.h"

#include "CEC_Convert.h"

//--------------------------------------------------------------------------------------------------


unsigned char Rx_EDID[256] =
{
// EDID - 720P, 480P, 576P
0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,   // address 0x00
0x17, 0x10, 0x51, 0x98, 0x01, 0x01, 0x01, 0x01,   //
0x34, 0x14, 0x01, 0x03, 0x80, 0x00, 0x00, 0x78,   // address 0x10
0x02, 0x0D, 0xC9, 0xA0, 0x57, 0x47, 0x98, 0x27,   //
0x12, 0x48, 0x4C, 0x20, 0x00, 0x00, 0x01, 0x01,   // address 0x20
0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,   //
0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x1D,   // address 0x30
0x00, 0x72, 0x51, 0xD0, 0x1E, 0x20, 0x6E, 0x28,   //
0x55, 0x00, 0xC4, 0x8E, 0x21, 0x00, 0x00, 0x1E,   // address 0x40
0x8C, 0x0A, 0xD0, 0x8A, 0x20, 0xE0, 0x2D, 0x10,   //
0x10, 0x3E, 0x96, 0x00, 0x04, 0x03, 0x00, 0x00,   // address 0x50
0x00, 0x18, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x45,   //
0x50, 0x20, 0x52, 0x58, 0x0A, 0x20, 0x20, 0x20,   // address 0x60
0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0xFD,   //
0x00, 0x3B, 0x3D, 0x0F, 0x2E, 0x08, 0x00, 0x0A,   // address 0x70
0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x01, 0x9E,   //
0x02, 0x03, 0x1A, 0x46, 0x47, 0x84, 0x13, 0x02,   // address 0x80
0x03, 0x11, 0x12, 0x01, 0x23, 0x09, 0x07, 0x07,   //
0x83, 0x01, 0x00, 0x00, 0x65, 0x03, 0x0C, 0x00,   // address 0x90
0x10, 0x00, 0x01, 0x1D, 0x00, 0x72, 0x51, 0xD0,   //
0x1E, 0x20, 0x6E, 0x28, 0x55, 0x00, 0x10, 0x09,   // address 0xA0
0x00, 0x00, 0x00, 0x1E, 0x01, 0x1D, 0x00, 0x72,   //
0x51, 0xD0, 0x1E, 0x20, 0x6E, 0x28, 0x55, 0x00,   // address 0xB0
0x10, 0x09, 0x00, 0x00, 0x00, 0x1E, 0x8C, 0x0A,   //
0xD0, 0x8A, 0x20, 0xE0, 0x2D, 0x10, 0x10, 0x3E,   // address 0xC0
0x96, 0x00, 0x10, 0x09, 0x00, 0x00, 0x00, 0x18,   //
0x8C, 0x0A, 0xD0, 0x8A, 0x20, 0xE0, 0x2D, 0x10,   // address 0xD0
0x10, 0x3E, 0x96, 0x00, 0x10, 0x09, 0x00, 0x00,   //
0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   // address 0xE0
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   //
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   // address 0xF0
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xAC,   //

};

//--------------------------------------------------------------------------------------------------
// Internal Variables

unsigned char is_HTPLG = 0;
unsigned char is_MHL_Ready = 0;

unsigned char Port_Sel = 0, Port_Sel_Backup = 0x0F;//, MHL_Sel = 0;

unsigned int HTPLG_TimeCount = 0;
unsigned int MHL_Ready_TimeCount = 0;

unsigned char RAP_RCP_Buf[2];

LINK_STATE Link_State = LINK_STATE_Start;

//--------------------------------------------------------------------------------------------------
// Private Function
void LINK_STATE_to_Start(void);
void LINK_STATE_to_HDMI_Mode(void);
void LINK_STATE_to_MHL_Mode(void);

//==================================================================================================
// Public Function Implementation

void Rx_HTPLUG(unsigned char HTP_Enable, unsigned char Rx_Port)
{
	if(HTP_Enable)
	{
		//GPIO_clear(GPIO_2, Bit1 | Bit2 | Bit3); // Rx Hot-Plug Enable (use GPIO set HDMI Hotplug pin to high)
		DBG_printf("LINK: Rx Hot-Plug Enable All\n");
	}
	else
	{
		switch(Rx_Port)
		{
			case 0:
				//GPIO_set(GPIO_2, Bit1); // Rx Hot-Plug Disable (use GPIO set HDMI Hotplug pin to low)
				DBG_printf("LINK: Rx Hot-Plug Disable 0\n");
				break;

			case 1:
				//GPIO_set(GPIO_2, Bit2); // Rx Hot-Plug Disable (use GPIO set HDMI Hotplug pin to low)
				DBG_printf("LINK: Rx Hot-Plug Disable 1\n");
				break;

			case 2:
				//GPIO_set(GPIO_2, Bit3); // Rx Hot-Plug Disable (use GPIO set HDMI Hotplug pin to low)
				DBG_printf("LINK: Rx Hot-Plug Disable 2\n");
				break;

			case Disable_All:
				//GPIO_set(GPIO_2, Bit1 | Bit2 | Bit3);
				DBG_printf("LINK: Rx Hot-Plug Disable All\n");
				break;
		}
	}
}

extern int work_mode;

unsigned char MHL_CDS_Check(unsigned char Rx_port)
{
#if 0
	switch(Rx_port) {
		case 0:
//			if(GPIO_get(GPIO_2, Bit4))	// check Rx 0 - CDS is high ? (use GPIO to detect)
			{
				return 1; // Rx connect to MHL device
			}
//			else
			{
				return 0; // Rx connect to HDMI or not connect
			}
			break;

		case 1:
//			if(GPIO_get(GPIO_2, Bit5))	// check Rx 1 - CDS is high ? (use GPIO to detect)
			{
				return 1; // Rx connect to MHL device
			}
//			else
			{
				return 0; // Rx connect to HDMI or not connect
			}
			break;

		case 2:
//			if(GPIO_get(GPIO_2, Bit6))	// check Rx 2 - CDS is high ? (use GPIO to detect)
			{
				return 1; // Rx connect to MHL device
			}
//			else
			{
				return 0; // Rx connect to HDMI or not connect
			}
			break;
	}
#endif
	return work_mode;
}

void RCP_Conversion_Init()
{
	unsigned char status;
	unsigned char Temp_Data[2];

	// check IIC addr
	do {
		// Find EP94M0 IIC Address
		IIC_EP94M0_Addr = 0x98;
		status = EP94M0_Reg_Read(EP94M0_General_Control_1, &Temp_Data[0], 1);

		if(status) { // failed

			IIC_EP94M0_Addr = 0x88;
			status = EP94M0_Reg_Read(EP94M0_General_Control_1, &Temp_Data[0], 1);

			if(status) { // failed again
				DBG_printf("Err: Cannot find EP94M3\n");
			}
			else {
				DBG_printf("EP94M3 addr = 0x%02X\n",(int)IIC_EP94M0_Addr);
			}
		}
		else {
			DBG_printf("EP94M3 addr = 0x%02X\n",(int)IIC_EP94M0_Addr);
		}

	} while(status);

	// Internal State Initial
	Link_State = LINK_STATE_Start;

	// write EDID data to EP94M3 RAM (3 port Rx)
	EP94M3_Rx_write_EDID(0, Rx_EDID);
	EP94M3_Rx_write_EDID(1, Rx_EDID);
	EP94M3_Rx_write_EDID(2, Rx_EDID);
	DBG_printf("LINK: Write EDID Done\n");

	// default use Rx Port 1
	EP94M3_Rx_Channel_Sel(1);

	LINK_STATE_to_Start();
}

void RCP_Conversion_Timer()
{
	HTPLG_TimeCount++;
	if(is_MHL_Ready) MHL_Ready_TimeCount++;

	MHL_Bus_Timer();
}

void Rx_Port_Sel(unsigned char Chip_Port)
{
	Port_Sel = Chip_Port;
}

void RCP_Conversion_Task()
{
	unsigned char RX_CDSENSE;

	RCP_Conversion_Timer();

	RX_CDSENSE = MHL_CDS_Check(Port_Sel_Backup);

	switch(Link_State) {

		case LINK_STATE_Start:

			DBG_printf("LINK: Start state\n");

			if(Port_Sel_Backup != Port_Sel) {
				Port_Sel_Backup = Port_Sel;

				// Switch port
				EP94M3_Rx_Channel_Sel(Port_Sel_Backup);
				DBG_printf("LINK: Rx Port Switched\n");
			}
			LINK_STATE_to_HDMI_Mode();
			break;

		case LINK_STATE_HDMI_Mode:
			if(!is_HTPLG) {
				if(HTPLG_TimeCount >= 500/MHL_BUS_TIMER_PERIOD) {

					EP94M0_Power_Up();

					Rx_HTPLUG(1, 0xFF);

					is_HTPLG = 1;
				}
			}
			else {
				HTPLG_TimeCount = 0;
			}

			//	Check CD_SENSE until 1=MHL, (0=HDMI or NO _CONNECT)
			if(RX_CDSENSE) {
				// MHL Cable detected
				is_MHL_Ready = 1;
			}
			else {
				// Normal HDMI cable detected
				is_MHL_Ready = 0;
				MHL_Ready_TimeCount = 0;
			}
			if(MHL_Ready_TimeCount >= 200/MHL_BUS_TIMER_PERIOD) {

				DBG_printf("LINK: CD_SENSE detect in HDMI Mode\n");
				LINK_STATE_to_MHL_Mode();
				break;
			}
			break;

		case LINK_STATE_MHL_Mode:

			//	Check CD_SENSE until 1=MHL, (0=HDMI or NO _CONNECT)
			if(!RX_CDSENSE) {

				DBG_printf("LINK: CD_SENSE missing in MHL Mode\n");
				LINK_STATE_to_HDMI_Mode();
				break;
			}
			else {
				MHL_Bus_Task();
			}
			break;
	}


	// Check Interrupt
	if(MHL_MSC_Get_Flags() & EP94M0_CBUS_MSC_Interrupt__MSG_F) {
		MHL_RCP_RAP_Read(RAP_RCP_Buf);
		switch(RAP_RCP_Buf[0]) {

			case MSC_RAP:
				DBG_printf("RAP Received: 0x%02hhx\n", RAP_RCP_Buf[1]);

				switch(RAP_RCP_Buf[1]) {

					case RAP_CONTENT_ON:

						// Power On TV

						break;
				}
				MHL_MSC_Cmd_MSC_MSG(MSC_RAPK, RAPK_No_Error);
				break;

			case MSC_RCP:
				DBG_printf("RCP Received: 0x%02hhx\n", RAP_RCP_Buf[1]);
				MHL_MSC_Cmd_MSC_MSG(MSC_RCPK, RAP_RCP_Buf[1]);
				break;

			case MSC_RAPK:
				if(RAP_RCP_Buf[1])	DBG_printf("RAP Err: 0x%02hhx\n", RAP_RCP_Buf[1]);
				break;

			case MSC_RCPE:
				if(RAP_RCP_Buf[1])	DBG_printf("RCP Err: 0x%02hhx\n", RAP_RCP_Buf[1]);
				break;
		}
	}


	// port changed
	if(Port_Sel_Backup != Port_Sel) {
		DBG_printf("LINK: Wait for Rx Port Switch\n");
		Link_State = LINK_STATE_Start;
	}

}

void LINK_STATE_to_Start(void)
{
	MHL_Rx_MHL_Mode(0); // Set HDMI Mode

	Rx_HTPLUG(0, Disable_All);

	EP94M0_Power_Down();

	is_HTPLG = 0;
	HTPLG_TimeCount = 0;

	DBG_printf("LINK: to Start\n");
	Link_State = LINK_STATE_Start;
}

void LINK_STATE_to_HDMI_Mode(void)
{
	// Disable MHL functions

	MHL_Rx_MHL_Mode(0); // Set HDMI Mode

	EP94M0_Power_Down();

	Rx_HTPLUG(0, Port_Sel_Backup);

	is_HTPLG = 0;
	HTPLG_TimeCount = 0;

	is_MHL_Ready = 0;
	MHL_Ready_TimeCount = 0;

	DBG_printf("LINK: to HDMI Mode\n");
	Link_State = LINK_STATE_HDMI_Mode;
}

void LINK_STATE_to_MHL_Mode(void)
{
	// Disable HDMI functions

	Rx_HTPLUG(0, Port_Sel_Backup);

	MHL_Rx_MHL_Mode(1); // Set MHL Mode

	EP94M0_Power_Up();

	MHL_Bus_Reset();

	DBG_printf("LINK: to MHL Mode\n");
	Link_State = LINK_STATE_MHL_Mode;
}

/*
void RCP_Conversion_Proc()
{

	// Menu Control

		case EPCC_UI_COMMAND_Select:
			DBG_printf(("Select\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Select);
			break;

		case EPCC_UI_COMMAND_Up:
			DBG_printf(("Up\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Up);
			break;

		case EPCC_UI_COMMAND_Down:
			DBG_printf(("Down\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Down);
			break;

		case EPCC_UI_COMMAND_Left:
			DBG_printf(("Left\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Left);
			break;

		case EPCC_UI_COMMAND_Right:
			DBG_printf(("Right\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Right);
			break;

		case EPCC_UI_COMMAND_Right_Up:
			DBG_printf(("Right Up\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Right_Up);
			break;

		case EPCC_UI_COMMAND_Right_Down:
			DBG_printf(("Right Down\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Right_Down);
			break;

		case EPCC_UI_COMMAND_Left_Up:
			DBG_printf(("Left Up\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Left_Up);
			break;

		case EPCC_UI_COMMAND_Left_Down:
			DBG_printf(("Left Down\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Left_Down);
			break;

		case EPCC_UI_COMMAND_Root_Menu:
			DBG_printf(("Root Menu\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Root_Menu);
			break;

		case EPCC_UI_COMMAND_Setup_Menu:
			DBG_printf(("Setup Menu\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Setup_Menu);
			break;

		case EPCC_UI_COMMAND_Contents_Menu:
			DBG_printf(("Contents Menu\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Contents_Menu);
			break;

		case EPCC_UI_COMMAND_Favorite_Menu:
			DBG_printf(("Favorite Menu\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Favorite_Menu);
			break;

		case EPCC_UI_COMMAND_Exit:
			DBG_printf(("Exit\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Exit);
			break;

		case EPCC_UI_COMMAND_0:
			DBG_printf(("0\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Numeric_0);
			break;

		case EPCC_UI_COMMAND_1:
			DBG_printf(("1\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Numeric_1);
			break;

		case EPCC_UI_COMMAND_2:
			DBG_printf(("2\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Numeric_2);
			break;

		case EPCC_UI_COMMAND_3:
			DBG_printf(("3\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Numeric_3);
			break;

		case EPCC_UI_COMMAND_4:
			DBG_printf(("4\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Numeric_4);
			break;

		case EPCC_UI_COMMAND_5:
			DBG_printf(("5\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Numeric_5);
			break;

		case EPCC_UI_COMMAND_6:
			DBG_printf(("6\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Numeric_6);
			break;

		case EPCC_UI_COMMAND_7:
			DBG_printf(("7\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Numeric_7);
			break;

		case EPCC_UI_COMMAND_8:
			DBG_printf(("8\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Numeric_8);
			break;

		case EPCC_UI_COMMAND_9:
			DBG_printf(("9\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Numeric_9);
			break;

		case EPCC_UI_COMMAND_Dot:
			DBG_printf((".\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Dot);
			break;

		case EPCC_UI_COMMAND_Enter:
			DBG_printf(("Enter\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Enter);
			break;

		case EPCC_UI_COMMAND_Clear:
			DBG_printf(("Clear\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Clear);
			break;

		case EPCC_UI_COMMAND_Channel_Up:
			DBG_printf(("Channel Up\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Channel_Up);
			break;

		case EPCC_UI_COMMAND_Channel_Down:
			DBG_printf(("Channel Down\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Channel_Down);
			break;

		case EPCC_UI_COMMAND_Previous_Channel:
			DBG_printf(("Previous Channel\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Previous_Channel);
			break;

		case EPCC_UI_COMMAND_Volume_Up:
			DBG_printf(("Volume Up\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Volume_Up);
			break;

		case EPCC_UI_COMMAND_Volume_Down:
			DBG_printf(("Volume Down\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Volume_Down);
			break;

		case EPCC_UI_COMMAND_Mute:
			DBG_printf(("Mute\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Mute);
			break;

		case EPCC_UI_COMMAND_Play:
			DBG_printf(("Play\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Play);
			break;

		case EPCC_UI_COMMAND_Stop:
			DBG_printf(("Stop\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Stop);
			break;

		case EPCC_UI_COMMAND_Pause:
			DBG_printf(("Pause\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Pause);
			break;

		case EPCC_UI_COMMAND_Record:
			DBG_printf(("Record\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Record);
			break;

		case EPCC_UI_COMMAND_Rewind:
			DBG_printf(("Rewind\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Rewind);
			break;

		case EPCC_UI_COMMAND_Fast_Forward:
			DBG_printf(("Fast Forward\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Fast_Forward);
			break;

		case EPCC_UI_COMMAND_Eject:
			DBG_printf(("Eject\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Eject);
			break;

		case EPCC_UI_COMMAND_Forward:
			DBG_printf(("Forward\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Forward);
			break;

		case EPCC_UI_COMMAND_Backward:
			DBG_printf(("Backward\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Backward);
			break;

		case EPCC_UI_COMMAND_Angle:
			DBG_printf(("Angle\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Angle);
			break;

		case EPCC_UI_COMMAND_Sub_Picture:
			DBG_printf(("Subpicture\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Subpicture);
			break;

		case EPCC_UI_COMMAND_Play_Function:
			DBG_printf(("Play Function\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Play_Function);
			break;

		case EPCC_UI_COMMAND_Pause_Play_Function:
			DBG_printf(("Pause Play Function\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Pause_Play_Function);
			break;

		case EPCC_UI_COMMAND_Record_Function:
			DBG_printf(("Record Function\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Record_Function);
			break;

		case EPCC_UI_COMMAND_Pause_Record_Function:
			DBG_printf(("Pause Record Function\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Pause_Record_Function);
			break;

		case EPCC_UI_COMMAND_Stop_Function:
			DBG_printf(("Stop Function\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Stop_Function);
			break;

		case EPCC_UI_COMMAND_Mute_Function:
			DBG_printf(("Mute Function\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Mute_Function);
			break;

		case EPCC_UI_COMMAND_Restore_Volume_Function:
			DBG_printf(("Restore Volume Function\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Restore_Volume_Function);
			break;

		case EPCC_UI_COMMAND_Tune_Function:
			DBG_printf(("Tune Function\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Tune_Function);
			break;

		case EPCC_UI_COMMAND_Select_Media_Function:
			DBG_printf(("Select Media Function\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_Select_Media_Function);
			break;

		case EPCC_UI_COMMAND_F1_Blue:
			DBG_printf(("F1 Blue\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_F1);
			break;

		case EPCC_UI_COMMAND_F2_Red:
			DBG_printf(("F2 Red\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_F2);
			break;

		case EPCC_UI_COMMAND_F3_Green:
			DBG_printf(("F3 Green\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_F3);
			break;

		case EPCC_UI_COMMAND_F4_Yellow:
			DBG_printf(("F4 Yellow\n"));
			MHL_MSC_Cmd_MSC_MSG(MSC_RCP, RCP_F4);
			break;

		default:
			DBG_printf(("Not Implemented\n"));
}
*/

