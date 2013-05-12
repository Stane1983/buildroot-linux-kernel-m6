/*******************************************************************************

          (c) Copyright Explore Semiconductor, Inc. Limited 2005
                           ALL RIGHTS RESERVED

--------------------------------------------------------------------------------

  File        :  MHL_If.c

  Description :  EP94M1 IIC Interface

*******************************************************************************/

// standard Lib
#include <linux/string.h>

#include "MHL_If.h"
#include "CEC_Convert.h"
#include "EP94M3_I2C.h"

//--------------------------------------------------------------------------------------------------

#define MHL_CMD_RETRY_TIME		8
#define CBUS_TIME_OUT_CheckErr	0x50
#define CBUS_TIME_OUT_Normal	0x6E

#define RX_PHY_CTL_0			0x2D
#define RX_PHY_CTL_1			0x00
#define RX_PHY_CTL_2			0x01

#define RX_PHY_CTL_0_PP			0x0C

#define TX_PHY_CTL_0			0x10

typedef enum {
	RQ_STATUS_Success = 0,
	RQ_STATUS_Abort,
	RQ_STATUS_Timeout,
	RQ_STATUS_Error
} RQ_STATUS;

//--------------------------------------------------------------------------------------------------

unsigned char Device_Capability_Default[16] =
{
	0x00, // 0: DEV_STATE		x
	0x11, // 1: MHL_VERSION		-
	0x03, // 2: DEV_CAT			- POW[4], DEV_TYPE[3:0]
	0x01, // 3: ADOPTER_ID_H	- Explore ADOPTER ID is 263, then ADOPTER_ID_H = 0x01
	0x07, // 4: ADOPTER_ID_L	- Explore ADOPTER ID is 263, then ADOPTER_ID_L = 0x07.
	0x3F, // 5: VID_LINK_MODE	- SUPP_VGA[5], SUPP_ISLANDS[4], SUPP_PPIXEL[3], SUPP_YCBCR422[2], SUPP_YCBCR444[1], SUPP_RGB444[0]
	0x03, // 6: AUD_LINK_MODE	- AUD_8CH[1], AUD_2CH[0]
	0x00, // 7: VIDEO_TYPE		- SUPP_VT[7], VT_GAME[3], VT_CINEMA[2], VT_PHOTO[1], VT_GRAPHICS[0]
	0xC1, // 8: LOG_DEV_MAP		- LD_GUI[7], LD_SPEAKER[6], LD_RECORD[5], LD_TUNER[4], LD_MEDIA[3], LD_AUDIO[2], LD_VIDEO[1], LD_DISPLAY[0]
	0x0F, // 9: BANDWIDTH		- 0x00 = support all in EDID, 0x0F = 75MHz
	0x07, // A: FEATURE_FLAG	- SP_SUPPORT[2], RAP_SUPPORT[1], RCP_SUPPORT[0]
	0x00, // B: DEVICE_ID_H		-
	0x01, // C: DEVICE_ID_L		-
	0x10, // D: SCRATCHPAD_SIZE	-
	0x33, // E: INT_STAT_SIZE	- STAT_SIZE[7:4], INT_SIZE[3:0]
	0x00, // F: Reserved
};

//--------------------------------------------------------------------------------------------------

int i, j;
unsigned char status;
unsigned char Temp_Data[16];

unsigned char IIC_EP94M0_Addr = 0x98;		// IIC Address

//==================================================================================================

// Private Functions
unsigned char IIC_Access(unsigned char IICAddr, unsigned char RegAddr, unsigned char *Data, unsigned int Size);

// CBus Requester
RQ_STATUS MHL_CBus_RQ_Go(void);
RQ_STATUS MHL_CBus_RQ_Check(unsigned char Size, unsigned char *pData);

// Public Function Implementation
//--------------------------------------------------------------------------------------------------

// Hardware Interface

void EP94M0_Power_Down()
{
	// Software power down
	EP94M0_Reg_Set_Bit(EP94M0_General_Control_0, EP94M0_General_Control_0__PWR_DWN);
}

void EP94M0_Power_Up()
{
	// Software power up
	EP94M0_Reg_Clear_Bit(EP94M0_General_Control_0, EP94M0_General_Control_0__PWR_DWN);

	// Disable the RQ Auto function
	MHL_Rx_RQ_AUTO_EN(0);

	// Set the CBUS Time-Out time
	Temp_Data[0] = CBUS_TIME_OUT_Normal;
	EP94M0_Reg_Write(EP94M3_CBUS_Time_Out, &Temp_Data[0], 1);

	// Check chip
	EP94M0_Reg_Read(EP94M3_CBUS_Time_Out, &Temp_Data[0], 1);
	if(Temp_Data[0] == CBUS_TIME_OUT_Normal) {

		DBG_printf("EP94M3 Set PHY Control\n");

		// Set the RX PHY Control
		Temp_Data[0] = RX_PHY_CTL_0;
		Temp_Data[1] = RX_PHY_CTL_1;
		Temp_Data[2] = RX_PHY_CTL_2;
		EP94M0_Reg_Write(EP94M0_RX_PHY_Control, &Temp_Data[0], 3);

		// Set the TX PHY Control
		Temp_Data[0] = TX_PHY_CTL_0;
		EP94M0_Reg_Write(EP94M0_TX_PHY_Control, &Temp_Data[0], 1);
	}

	// Disable the RSEN bypass
	EP94M0_Reg_Set_Bit(EP94M0_General_Control_0, EP94M0_General_Control_0__RSEN_BYP);

	// Interrupt Enable
	Temp_Data[0] = EP94M3_CBUS_MSC_Interrupt__MSG_IE | EP94M3_CBUS_MSC_Interrupt__INT_IE;

	// Check SP_SUPPORT bit
	if(Device_Capability_Default[FEATURE_FLAG] | 0x04) {
		// Enable Write Burst support
		Temp_Data[0] |= EP94M3_CBUS_MSC_Interrupt__WB_SPT;
	}
	EP94M0_Reg_Write(EP94M0_CBUS_MSC_Interrupt, &Temp_Data[0], 1);

	// Set BR_ADJ
	Temp_Data[0] = 42;
	EP94M0_Reg_Write(EP94M0_CBUS_BR_ADJ, &Temp_Data[0], 1);
	DBG_printf("EP94M3 Set BR_ADJ = %d\n",(int)Temp_Data[0]);

	// Set the CBUS Re-try time
	Temp_Data[0] = 0x20;
	EP94M0_Reg_Write(EP94M3_CBUS_TX_Re_Try, &Temp_Data[0], 1);

	//
	// Set Default Device Capability
	//
	memcpy(Temp_Data, Device_Capability_Default, sizeof(Device_Capability_Default));
	EP94M0_Reg_Write(EP94M0_CBUS_MSC_Dec_Capability, Temp_Data, sizeof(Device_Capability_Default));
}

void EP94M3_Rx_write_EDID(unsigned char Chip_Port, unsigned char *pData)
{
	// Disable EDID
	EP94M0_Reg_Read(EP94M3_EDID_Control, &Temp_Data[0], 1);
	Temp_Data[0] &= ~(EP94M3_EDID_Control__EDID_EN0 << Chip_Port);
	Temp_Data[0] &= ~EP94M3_EDID_Control__EDID_SEL;
	Temp_Data[0] |= Chip_Port;  // EDID SEL
	EP94M0_Reg_Write(EP94M3_EDID_Control, &Temp_Data[0], 1);

	// Write EDID
	EP94M0_Reg_Write(EP94M3_EDID_Data, pData, 256);

	// Enable EDID
	EP94M0_Reg_Set_Bit(EP94M3_EDID_Control, EP94M3_EDID_Control__EDID_EN0 << Chip_Port);
}

void EP94M3_Rx_Channel_Sel(unsigned char Chip_Port)
{
	EP94M0_Reg_Read(EP94M0_General_Control_0, &Temp_Data[0], 1);
	Temp_Data[0] &= ~EP94M3_General_Control_0__PORT_SEL;
	Temp_Data[0] |= Chip_Port;
	EP94M0_Reg_Write(EP94M0_General_Control_0, &Temp_Data[0], 1);
}

//--------------------------------------------------------------------------------------------------
//
unsigned char MHL_Rx_Signal()
{
	EP94M0_Reg_Read(EP94M0_General_Control_1, &Temp_Data[0], 1);
	if(Temp_Data[0] & EP94M0_General_Control_1__RX_LINK_ON) {
		return 1;
	}
	return 0;
}

void MHL_Rx_MHL_Mode(unsigned char Enable)
{
	if(Enable) {
		// MHL mode
		EP94M0_Reg_Set_Bit(EP94M0_General_Control_0, EP94M3_General_Control_0__MHL_MODE);

		// DDC Switch Off
		EP94M0_Reg_Clear_Bit(EP94M3_EDID_Control, EP94M3_EDID_Control__DDC_SW_ON);
	}
	else {
		// HDMI mode
		EP94M0_Reg_Clear_Bit(EP94M0_General_Control_0, EP94M3_General_Control_0__MHL_MODE);

		// DDC Switch On
		EP94M0_Reg_Set_Bit(EP94M3_EDID_Control, EP94M3_EDID_Control__DDC_SW_ON);

		Temp_Data[0] = RX_PHY_CTL_0;
		Temp_Data[1] = RX_PHY_CTL_1;
		Temp_Data[2] = RX_PHY_CTL_2;
		EP94M0_Reg_Write(EP94M0_RX_PHY_Control, &Temp_Data[0], 3);
	}
}

void MHL_Rx_RQ_AUTO_EN(unsigned char Enalbe)
{
	EP94M0_Reg_Read(EP94M0_CBUS_RQ_Control, &Temp_Data[0], 1);
	if(Enalbe) {
		Temp_Data[0] |= EP94M0_CBUS_RQ_Control__RQ_AUTO_EN;
	}
	else {
		Temp_Data[0] &= ~EP94M0_CBUS_RQ_Control__RQ_AUTO_EN;
	}
	EP94M0_Reg_Write(EP94M0_CBUS_RQ_Control, &Temp_Data[0], 1);
}

unsigned char MHL_Rx_CBUS_Connect()
{
	EP94M0_Reg_Read(EP94M0_CBUS_RQ_Control, &Temp_Data[0], 1);
	if(Temp_Data[0] & EP94M0_CBUS_RQ_Control__CBUS_DSCed) {
		return 1;
	}
	else{
		return 0;
	}
}

unsigned char MHL_MSC_Get_Flags()
{
	EP94M0_Reg_Read(EP94M0_CBUS_MSC_Interrupt, &Temp_Data[0], 1);
	return Temp_Data[0];
}

void MHL_Clock_Mode(unsigned char Packed_Pixel_Mode)
{
	if(Packed_Pixel_Mode) {
		EP94M0_Reg_Set_Bit(EP94M0_General_Control_0, EP94M0_General_Control_0__PP_MODE);

		Temp_Data[0] = RX_PHY_CTL_0_PP;
		Temp_Data[1] = RX_PHY_CTL_1;
		Temp_Data[2] = RX_PHY_CTL_2;
		EP94M0_Reg_Write(EP94M0_RX_PHY_Control, &Temp_Data[0], 3);
	}
	else {
		EP94M0_Reg_Clear_Bit(EP94M0_General_Control_0, EP94M0_General_Control_0__PP_MODE);

		Temp_Data[0] = RX_PHY_CTL_0;
		Temp_Data[1] = RX_PHY_CTL_1;
		Temp_Data[2] = RX_PHY_CTL_2;
		EP94M0_Reg_Write(EP94M0_RX_PHY_Control, &Temp_Data[0], 3);
	}
}

void MHL_MSC_Reg_Update(unsigned char offset, unsigned char value)
{
	if(offset >= 0x40) {
		offset -= 0x40;
		// Scratchpad
		EP94M0_Reg_Read(EP94M0_CBUS_MSC_Dec_SrcPad, &Temp_Data[0], offset+1);
		Temp_Data[offset] = value;
		EP94M0_Reg_Write(EP94M0_CBUS_MSC_Dec_SrcPad, &Temp_Data[0], offset+1);
	}
	else if(offset >= 0x30) {
		offset -= 0x30;
		// Status Registers
		EP94M0_Reg_Read(EP94M0_CBUS_MSC_Dec_Status, &Temp_Data[0], offset+1);
		Temp_Data[offset] = value;
		EP94M0_Reg_Write(EP94M0_CBUS_MSC_Dec_Status, &Temp_Data[0], offset+1);
	}
	else if(offset >= 0x20) {
		offset -= 0x20;
		// Interrupt Registers
		EP94M0_Reg_Read(EP94M0_CBUS_MSC_Dec_Interrupt, &Temp_Data[0], offset+1);
		Temp_Data[offset] = value;
		EP94M0_Reg_Write(EP94M0_CBUS_MSC_Dec_Interrupt, &Temp_Data[0], offset+1);
	}
	else {
		// Capability Registers
		EP94M0_Reg_Read(EP94M0_CBUS_MSC_Dec_Capability, &Temp_Data[0], offset+1);
		Temp_Data[offset] = value;
		EP94M0_Reg_Write(EP94M0_CBUS_MSC_Dec_Capability, &Temp_Data[0], offset+1);
	}
}

unsigned char MHL_MSC_Reg_Read(unsigned char offset)
{
	if(offset >= 0x40) {
		offset -= 0x40;
		// Scratchpad
		EP94M0_Reg_Read(EP94M0_CBUS_MSC_Dec_SrcPad, &Temp_Data[0], offset+1);
	}
	else if(offset >= 0x30) {
		offset -= 0x30;
		// Status Registers
		EP94M0_Reg_Read(EP94M0_CBUS_MSC_Dec_Status, &Temp_Data[0], offset+1);
	}
	else if(offset >= 0x20) {
		offset -= 0x20;
		// Interrupt Registers
		EP94M0_Reg_Read(EP94M0_CBUS_MSC_Dec_Interrupt, &Temp_Data[0], offset+1);
	}
	else {
		// Capability Registers
		EP94M0_Reg_Read(EP94M0_CBUS_MSC_Dec_Capability, &Temp_Data[0], offset+1);
	}
	return Temp_Data[offset];
}

void MHL_RCP_RAP_Read(unsigned char *pData)
{
	EP94M0_Reg_Read(EP94M0_CBUS_MSC_RAP_RCP, pData, 2);
}

unsigned char MHL_MSC_Cmd_READ_DEVICE_CAP(unsigned char offset, unsigned char *pValue)
{
	RQ_STATUS error;

	for(i=0; i<MHL_CMD_RETRY_TIME; ++i) {

		//
		// Fill in the Command
		//

		// Size (TX Size is not including the Header)
		Temp_Data[1] = 1 | (2<<5); // TX Size | (RX Size << 5)

		// Header
		Temp_Data[2] = EP94M0_CBUS_RQ_HEADER__MSC_Packet | EP94M0_CBUS_RQ_HEADER__isCommand;

		// Command
		Temp_Data[3] = 0x61; // MSC_READ_DEVCAP

		// Data
		Temp_Data[4] = offset; // offset

		EP94M0_Reg_Write(EP94M0_CBUS_RQ_SIZE, &Temp_Data[1], 4);

		//
		// Start to Send the Command
		//

		// Set the CBUS Time-Out time
		Temp_Data[0] = CBUS_TIME_OUT_CheckErr;
		EP94M0_Reg_Write(EP94M3_CBUS_Time_Out, &Temp_Data[0], 1);

		error = MHL_CBus_RQ_Go();

		// Set the CBUS Time-Out time
		Temp_Data[0] = CBUS_TIME_OUT_Normal;
		EP94M0_Reg_Write(EP94M3_CBUS_Time_Out, &Temp_Data[0], 1);

		error = MHL_CBus_RQ_Check(1, pValue);
		if(!error) {
			if(i>0) DBG_printf("Retry %d READ_DEVICE_CAP Success\n", i);

			return 1;
		}
	}

	DBG_printf("READ_DEVICE_CAP Fail, offset = 0x%02hhx\n", offset);
	return 0;
}

unsigned char MHL_MSC_Cmd_HPD(unsigned char Set)
{
	RQ_STATUS error;

	for(i=0; i<MHL_CMD_RETRY_TIME; ++i) {

		//
		// Fill in the Command and Parameters
		//

		// Size (TX Size is not including the Header)
		Temp_Data[1] = 0 | (1<<5); // TX Size | (RX Size << 5)

		// Header
		Temp_Data[2] = EP94M0_CBUS_RQ_HEADER__MSC_Packet | EP94M0_CBUS_RQ_HEADER__isCommand;

		// Command
		if(Set)
			Temp_Data[3] = 0x64; // SET_HPD
		else
			Temp_Data[3] = 0x65; // CLEAR_HPD

		EP94M0_Reg_Write(EP94M0_CBUS_RQ_SIZE, &Temp_Data[1], 3);

		//
		// Start to Send the Command
		//

		error = MHL_CBus_RQ_Go();

		error = MHL_CBus_RQ_Check(0, NULL);
		if(!error) {
			if(i>0) DBG_printf("Retry %d SET_HPD Success\n", i);
			return 1;
		}
	}
	DBG_printf("SET_HPD / CLEAR_HPD Fail\n");
	return 0;
}

unsigned char MHL_MSC_Cmd_WRITE_STATE(unsigned char offset, unsigned char value)
{
	RQ_STATUS error;

	for(i=0; i<MHL_CMD_RETRY_TIME; ++i) {

		//
		// Fill in the Command and Parameters
		//

		// Size (TX Size is not including the Header)
		Temp_Data[1] = 2 | (1<<5); // TX Size | (RX Size << 5)

		// Header
		Temp_Data[2] = EP94M0_CBUS_RQ_HEADER__MSC_Packet | EP94M0_CBUS_RQ_HEADER__isCommand;

		// Command
		Temp_Data[3] = 0x60; // MSC_WRITE_STATE

		// Data
		Temp_Data[4] = offset; // offset
		Temp_Data[5] = value; // value

		EP94M0_Reg_Write(EP94M0_CBUS_RQ_SIZE, &Temp_Data[1], 5);

		//
		// Start to Send the Command
		//

		error = MHL_CBus_RQ_Go();

		error = MHL_CBus_RQ_Check(0, NULL);
		if(!error) {
			if(i>0) DBG_printf("Retry %d WRITE_STATE Success\n", i);

			return 1;
		}
	}
	DBG_printf("WRITE_STATE Fail\n");
	Link_State = LINK_STATE_Start;
	return 0;
}

unsigned char MHL_MSC_Cmd_MSC_MSG(unsigned char SubCmd, unsigned char ActionCode)
{
	RQ_STATUS error;

	for(i=0; i<MHL_CMD_RETRY_TIME; ++i) {

		//
		// Fill in the Command and Parameters
		//

		// Size (TX Size is not including the Header)
		Temp_Data[1] = 2 | (1<<5); // TX Size | (RX Size << 5)

		// Header
		Temp_Data[2] = EP94M0_CBUS_RQ_HEADER__MSC_Packet | EP94M0_CBUS_RQ_HEADER__isCommand;

		// Command
		Temp_Data[3] = 0x68; // MSC_MSG

		// Data
		Temp_Data[4] = SubCmd; // SubCmd
		Temp_Data[5] = ActionCode; // ActionCode

		EP94M0_Reg_Write(EP94M0_CBUS_RQ_SIZE, &Temp_Data[1], 5);

		//
		// Start to Send the Command
		//

		error = MHL_CBus_RQ_Go();

		error = MHL_CBus_RQ_Check(0, NULL);
		if(!error) {
			if(i>0) DBG_printf("Retry %d MSC_MSG Success\n", i);

			return 1;
		}
	}
	DBG_printf("MSC_MSG Fail\n");
	return 0;
}

#ifdef WRITE_BURST_CODE
unsigned char MHL_MSC_Cmd_WRITE_BURST(unsigned char offset, unsigned char *pData, unsigned char size)
{
	RQ_STATUS error;
	size = min(size, 16);

	for(i=0; i<MHL_CMD_RETRY_TIME; ++i) {

		//
		// Fill in the Command and Parameters
		//

		// Size (TX Size is not including the Header)
		Temp_Data[0] = (1+size) | (0<<5); // TX Size | (RX Size << 5)

		// Header
		Temp_Data[1] = EP94M0_CBUS_RQ_HEADER__MSC_Packet | EP94M0_CBUS_RQ_HEADER__isCommand;

		// Command
		Temp_Data[2] = 0x6C; // MSC_WRITE_BURST

		// Data
		Temp_Data[3] = offset; // offset
		memcpy(&Temp_Data[4], pData, size);

		EP94M0_Reg_Write(EP94M0_CBUS_RQ_SIZE, Temp_Data, 4+size);

		//
		// Start to Send the Command
		//

		error = MHL_CBus_RQ_Go();
		if(!error) {

			//
			// Fill in the Command and Parameters
			//

			// Size (TX Size is not including the Header)
			Temp_Data[0] = (0) | (1<<5); // TX Size | (RX Size << 5)

			// Header
			Temp_Data[1] = EP94M0_CBUS_RQ_HEADER__MSC_Packet | EP94M0_CBUS_RQ_HEADER__isCommand;

			// Command
			Temp_Data[2] = 0x32; // EOF

			EP94M0_Reg_Write(EP94M0_CBUS_RQ_SIZE, Temp_Data, 3);

			//
			// Start to Send the Command
			//

			error = MHL_CBus_RQ_Go();
			if(!error) {
				error = MHL_CBus_RQ_Check(0, NULL);
				if(!error) {
					if(i>0) DBG_printf("Retry %d WRITE_BURST Success\n", i);
					return 1;
				}
			}
		}
		DBG_printf("WRITE_BURST error\n");

		// Send ABORT if fail
		if( error && (error != RQ_STATUS_Abort) ) {
			MHL_MSC_Cmd_ABORT();
		}
	}
	DBG_printf("WRITE_BURST Fail\n");
	return 0;
}
#endif

//--------------------------------------------------------------------------------------------------
// MHL Protected(Internal) Commands

void MHL_MSC_Cmd_ACK()
{
	//
	// Fill in the Command
	//

	// Size (TX Size is not including the Header)
	Temp_Data[1] = 0 | (0<<5); // TX Size | (RX Size << 5)

	// Header
	Temp_Data[2] = EP94M0_CBUS_RQ_HEADER__MSC_Packet | EP94M0_CBUS_RQ_HEADER__isCommand;

	// Command
	Temp_Data[3] = 0x33; // MSC_ACK

	EP94M0_Reg_Write(EP94M0_CBUS_RQ_SIZE, &Temp_Data[1], 3);

	//
	// Start to Send the Command
	//
	for(i=0; i<7; ++i) {
		if(!MHL_CBus_RQ_Go()) break;
	}
}

void MHL_MSC_Cmd_ACK_ACK()
{
	//
	// Fill in the Command
	//

	// Size (TX Size is not including the Header)
	Temp_Data[1] = 1 | (0<<5); // TX Size | (RX Size << 5)

	// Header
	Temp_Data[2] = EP94M0_CBUS_RQ_HEADER__MSC_Packet | EP94M0_CBUS_RQ_HEADER__isCommand | 0x08;

	// Command
	Temp_Data[3] = 0x33; // ACK
	Temp_Data[4] = 0x33; // ACK

	EP94M0_Reg_Write(EP94M0_CBUS_RQ_SIZE, &Temp_Data[1], 4);

	//
	// Start to Send the Command
	//
	MHL_CBus_RQ_Go();
}

void MHL_MSC_Cmd_ABORT()
{
	//
	// Fill in the Command
	//

	// Size (TX Size is not including the Header)
	Temp_Data[1] = 0 | (0<<5); // TX Size | (RX Size << 5)

	// Header
	Temp_Data[2] = EP94M0_CBUS_RQ_HEADER__MSC_Packet | EP94M0_CBUS_RQ_HEADER__isCommand;

	// Command
	Temp_Data[3] = 0x35; // MSC_ABORT

	EP94M0_Reg_Write(EP94M0_CBUS_RQ_SIZE, &Temp_Data[1], 3);

	//
	// Start to Send the Command
	//

	MHL_CBus_RQ_Go();

	delay_1ms(110); // delay 100ms
}

//--------------------------------------------------------------------------------------------------
//

unsigned char HDMI_Tx_RSEN()
{
	// RSEN Detect
	EP94M0_Reg_Read(EP94M0_General_Control_1, &Temp_Data[0], 1);

	return (Temp_Data[0] & EP94M0_General_Control_1__TX_RSEN)? 1:0;
}


//--------------------------------------------------------------------------------------------------
//
// Hardware Interface
//

unsigned char EP94M0_Reg_Read(unsigned char RegAddr, unsigned char *Data, unsigned int Size)
{
	//return IIC_Access(IIC_EP94M0_Addr | 1, RegAddr, Data, Size);
	return EP94M3_I2C_Reg_Rd(RegAddr, Data, Size);
}

unsigned char EP94M0_Reg_Write(unsigned char RegAddr, unsigned char *Data, unsigned int Size)
{
	//return IIC_Access(IIC_EP94M0_Addr, RegAddr, Data, Size);
	return EP94M3_I2C_Reg_Wr(RegAddr, Data, Size);
}

unsigned char EP94M0_Reg_Set_Bit(unsigned char RegAddr, unsigned char BitMask)
{
	EP94M0_Reg_Read(RegAddr, Temp_Data, 1);

	// Write back to Reg Reg_Addr
	Temp_Data[0] |= BitMask;

	return EP94M0_Reg_Write(RegAddr, Temp_Data, 1);
}

unsigned char EP94M0_Reg_Clear_Bit(unsigned char RegAddr, unsigned char BitMask)
{
	EP94M0_Reg_Read(RegAddr, Temp_Data, 1);

	// Write back to Reg Reg_Addr
	Temp_Data[0] &= ~BitMask;

	return EP94M0_Reg_Write(RegAddr, Temp_Data, 1);
}

//==================================================================================================
//
// Private Functions
//

void delay_1ms(unsigned int DelayTime)
{
	unsigned int x;
	for(x=0; x<DelayTime; x++)
	{
	 	// Delay1ms();
	}
}

unsigned char IIC_Access(unsigned char IICAddr, unsigned char RegAddr, unsigned char *Data, unsigned int Size)
{
	/////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// IICAddr : IIC address
	// 		if(IICAddr & 0x01) ==> Read
	// 		else 			  ==> Write
	//
	// RegAddr : register address
	//
	// *Data : register value
	//
	// Size : register value length
	//
	// status : result
	// 		return 0; success
	// 		return 2; No_ACK
	// 		return 4; Arbitration Loss
	//
	/////////////////////////////////////////////////////////////////////////////////////////////////


	//SMBUS_master_sel(SMBUS_1, SMBUS_PortAlternative, SMBUS_DividedBy128); // for EP MCU use

	if(IICAddr & 0x01)
	{ 	// Read
		//status = SMBUS_master_rw_synchronous(SMBUS_1, IICAddr-1, &RegAddr, 1, SMBUS_SkipStop); // for EP MCU use
		//status |= SMBUS_master_rw_synchronous(SMBUS_1, IICAddr, Data, Size, SMBUS_Normal); // for EP MCU use
	}
	else
	{ 	// Write
		//status = SMBUS_master_rw_synchronous(SMBUS_1, IICAddr, &RegAddr, 1, SMBUS_SkipStop); // for EP MCU use
		//status |= SMBUS_master_rw_synchronous(SMBUS_1, IICAddr, Data, Size, SMBUS_SkipStart); // for EP MCU use
	}

	if(status) { // failed
		DBG_printf("Err: IIC failed %hhu, IICAddr=0x%02hhx, RegAddr=0x%02hhx\n", status, IICAddr, RegAddr);
	}

	return status;
}

RQ_STATUS MHL_CBus_RQ_Go(void)
{
	int i;
	unsigned char RQ_Check = 0;

	// Set the CBUS Re-try time
	Temp_Data[0] = 0x00;
	EP94M0_Reg_Write(EP94M3_CBUS_TX_Re_Try, &Temp_Data[0], 1);

	//
	// Start to Send the Command
	//

	EP94M0_Reg_Set_Bit(EP94M0_CBUS_RQ_Control, EP94M0_CBUS_RQ_Control__RQ_START);

	//
	// Wait for Complete
	//

	for(i=0; i<400; i++) { //  timeout

		//delay_1ms(1);

		EP94M0_Reg_Read(EP94M0_CBUS_RQ_Control, &RQ_Check, 1);
		if(!(RQ_Check & EP94M0_CBUS_RQ_Control__RQ_START)) {
			if(RQ_Check & EP94M0_CBUS_RQ_Control__RQ_DONE) {
				break;
			}
		}
	}

	// Set the CBUS Re-try time
	Temp_Data[0] = 0x20;
	EP94M0_Reg_Write(EP94M3_CBUS_TX_Re_Try, &Temp_Data[0], 1);


	// Check Error
	if(RQ_Check & EP94M0_CBUS_RQ_Control__RQ_DONE) {
		if(!(RQ_Check & EP94M0_CBUS_RQ_Control__RQ_ERR)) {
			return RQ_STATUS_Success; // No error
		}
	}
	else {
		EP94M0_Reg_Set_Bit(EP94M0_CBUS_RQ_Control, EP94M0_CBUS_RQ_Control__RQ_ABORT);
//#ifdef MSC_DBG
//		DBG_printf(("Err: CBUS RQ Start - RQ_Timeout\n"));
//#endif
		return RQ_STATUS_Timeout;
	}

//#ifdef MSC_DBG
//	DBG_printf(("Err: CBUS RQ Start - RQ_ERR\n")); // Cannot show this message for the HTC Phone fix.
//#endif
	return RQ_STATUS_Error;
}

RQ_STATUS MHL_CBus_RQ_Check(unsigned char Size, unsigned char *pData)
{
	RQ_STATUS error = RQ_STATUS_Success;

	//
	// Check the read data
	//

	EP94M0_Reg_Read(EP94M0_CBUS_RQ_ACT_RX_SIZE, &Temp_Data[0], Size + 2);

	if( (Temp_Data[0]&0x03) == 0 ) {
		error = RQ_STATUS_Error;
#ifdef MSC_DBG
		DBG_printf("Err: CBUS RQ - No Data Received\n");
#endif
	}
	else if(!(Temp_Data[0] & 0x40)) { // CMD0 bit (shall be command)
#ifdef MSC_DBG
		DBG_printf("Err: CBUS RQ - CMD_BIT0_ERR, 0x%02hhx\n", Temp_Data[0]);
#endif
		error = RQ_STATUS_Error;
		MHL_MSC_Cmd_ABORT();
	}
	else {
		if( (Temp_Data[0]&0x03) != (Size + 1) ) { // ACT_RX_SIZE
#ifdef MSC_DBG
			DBG_printf("Err: CBUS RQ - RX_SIZE_ERR, 0x%02hhx\n", Temp_Data[0]);
#endif
			error = RQ_STATUS_Error;
			delay_1ms(30); // delay 30ms
			MHL_MSC_Cmd_ABORT();
		}
	}
	if(!error) {
		if(Temp_Data[1] == 0x35) { // ABORT
#ifdef MSC_DBG
			DBG_printf("Err: CBUS RQ - ABORT, 0x%02hhx\n", Temp_Data[1]);
#endif
			delay_1ms(2100); // Delay 2 second
			error = RQ_STATUS_Abort;
		}
		else if(Temp_Data[1] != 0x33) { // ACK
#ifdef MSC_DBG
			DBG_printf("Err: CBUS RQ - NOT_ACK, 0x%02hhx\n", Temp_Data[1]);
#endif
			error = RQ_STATUS_Abort;
		}
	}
	if((Temp_Data[0] & 0x03) > 1) {
		if((Temp_Data[0] & 0x80)) { // CMD1 bit (shall not be command)
#ifdef MSC_DBG
			DBG_printf("Err: CBUS RQ - CMD_BIT1_ERR, 0x%02hhx\n", Temp_Data[0]);
#endif
			error = RQ_STATUS_Error;
			MHL_MSC_Cmd_ABORT();
		}
	}

	if(!error) { // if No error
		// Copy the data
		if(pData) memcpy(pData, &Temp_Data[2], Size);
	}

	return error;
}
