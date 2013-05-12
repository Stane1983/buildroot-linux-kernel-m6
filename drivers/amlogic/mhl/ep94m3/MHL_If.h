/*******************************************************************************

          (c) Copyright Explore Semiconductor, Inc. Limited 2005
                           ALL RIGHTS RESERVED

--------------------------------------------------------------------------------

  File        :  EP94M0_IF.h

  Description :  Head file of EP94M0_IF Interface

*******************************************************************************/

#ifndef EP94M0_IF_H
#define EP94M0_IF_H

#include <linux/kernel.h>
#include "RegDef.h"


#if 0
#ifdef DBG
	#define DBG_printf(x) RS232_printf x
#else
	#define DBG_printf(x)
#endif
#endif

#if (1)
        #define DBG_printf(m, a...) pr_info("[MHL] %s " m, __func__, ##a)
#else
        #define DBG_printf(m, a...)
#endif


//==================================================================================================
//
// Protected Data Member
//

// Command and Data for the function

// MHL_MSC_Cmd_READ_DEVICE_CAP(BYTE offset, PBYTE pValue)
#define MSC_DEV_CAT					0X02
#define 	POW								0x10
#define		DEV_TYPE						0x0F

#define LOG_DEV_MAP					0x08
#define 	LD_DISPLAY						0X01
#define 	LD_VIDEO						0X02
#define 	LD_AUDIO						0X04
#define 	LD_MEDIA						0X08
#define 	LD_TUNER						0X10
#define 	LD_RECORD						0X20
#define 	LD_SPEAKER						0X40
#define 	LD_GUI							0X80

#define FEATURE_FLAG				0X0A
#define 	RCP_SUPPORT						0X01
#define 	RAP_SUPPORT						0X02
#define 	SP_SUPPORT						0X04

#define DEVICE_ID_H					0x0B
#define DEVICE_ID_L					0x0C

#define SCRATCHPAD_SIZE				0X0D

// MHL_MSC_Reg_Read(BYTE offset)
// MHL_MSC_Cmd_WRITE_STATE(BYTE offset, BYTE value)
#define MSC_RCHANGE_INT				0X20
#define 	DCAP_CHG						0x01
#define 	DSCR_CHG						0x02
#define 	REQ_WRT							0x04
#define 	GRT_WRT							0x08

#define MSC_DCHANGE_INT				0X21
#define 	EDID_CHG						0x02

#define MSC_STATUS_CONNECTED_RDY	0X30
#define 	DCAP_RDY						0x01

#define MSC_STATUS_LINK_MODE		0X31
#define 	CLK_MODE						0x07
#define 	CLK_MODE__Normal						0x03
#define 	CLK_MODE__PacketPixel					0x02
#define 	PATH_EN							0x08

#define MSC_SCRATCHPAD				0x40

//MHL_MSC_Cmd_MSC_MSG(BYTE SubCmd, BYTE ActionCode)
#define MSC_RAP						0x20
#define		RAP_POLL						0x00
#define		RAP_CONTENT_ON					0x10
#define		RAP_CONTENT_OFF					0x11

#define MSC_RAPK					0x21
#define		RAPK_No_Error					0x00
#define		RAPK_Unrecognized_Code			0x01
#define		RAPK_Unsupported_Code			0x02
#define		RAPK_Responder_Busy				0x03

#define MSC_RCP						0x10
#define 	RCP_Select						0x00
#define 	RCP_Up							0x01
#define 	RCP_Down						0x02
#define 	RCP_Left						0x03
#define 	RCP_Right						0x04
#define 	RCP_Right_Up					0x05
#define 	RCP_Right_Down					0x06
#define 	RCP_Left_Up						0x07
#define 	RCP_Left_Down					0x08
#define 	RCP_Root_Menu					0x09
#define 	RCP_Setup_Menu					0x0A
#define 	RCP_Contents_Menu				0x0B
#define 	RCP_Favorite_Menu				0x0C
#define 	RCP_Exit						0x0D

#define 	RCP_Numeric_0					0x20
#define 	RCP_Numeric_1					0x21
#define 	RCP_Numeric_2					0x22
#define 	RCP_Numeric_3					0x23
#define 	RCP_Numeric_4					0x24
#define 	RCP_Numeric_5					0x25
#define 	RCP_Numeric_6					0x26
#define 	RCP_Numeric_7					0x27
#define 	RCP_Numeric_8					0x28
#define 	RCP_Numeric_9					0x29
#define 	RCP_Dot							0x2A
#define 	RCP_Enter						0x2B
#define 	RCP_Clear						0x2C

#define 	RCP_Channel_Up					0x30
#define 	RCP_Channel_Down				0x31
#define 	RCP_Previous_Channel			0x32

#define 	RCP_Volume_Up					0x41
#define 	RCP_Volume_Down					0x42
#define 	RCP_Mute						0x43
#define 	RCP_Play						0x44
#define 	RCP_Stop						0x45
#define 	RCP_Pause						0x46
#define 	RCP_Record						0x47
#define 	RCP_Rewind						0x48
#define 	RCP_Fast_Forward				0x49
#define 	RCP_Eject						0x4A
#define 	RCP_Forward						0x4B
#define 	RCP_Backward					0x4C

#define 	RCP_Angle						0x50
#define 	RCP_Subpicture					0x51

#define 	RCP_Play_Function				0x60
#define 	RCP_Pause_Play_Function			0x61
#define 	RCP_Record_Function				0x62
#define 	RCP_Pause_Record_Function		0x63
#define 	RCP_Stop_Function				0x64
#define 	RCP_Mute_Function				0x65
#define 	RCP_Restore_Volume_Function		0x66
#define 	RCP_Tune_Function				0x67
#define 	RCP_Select_Media_Function		0x68

#define 	RCP_F1							0x71
#define 	RCP_F2							0x72
#define 	RCP_F3							0x73
#define 	RCP_F4							0x74

#define MSC_RCPK					0x11
#define MSC_RCPE					0x12
#define 	RCPK_No_Error					0x00
#define 	RCPK_Ineffective_Code			0x01
#define 	RCPK_Responder_Busy				0x02

// Return value for the function
typedef enum {
	MHL_NOT_CONNECTED = 0, 	// Not Connected.
	MHL_CBUS_CONNECTED, 	// CBUS is connected.
	MHL_HOTPLUG_DETECT	 	// Hot-Plug signal is detected.
} MHL_CONNECTION_STATUS;

typedef enum {
	HDMI_NO_CONNECT = 0, 	// No Connection.
	HDMI_HOTPLUG_DETECT, 	// Hot-Plug signal is detected.
	HDMI_EDID_DETECT 		// Connection is confirmed by avaliable EDID.
} HDMI_CONNECTION_STATUS;


// code
extern unsigned char Device_Capability_Default[];
extern unsigned char IIC_EP94M0_Addr;

//==================================================================================================
//
// Public Functions
//

//--------------------------------------------------------------------------------------------------
//
// General
//

// All Interface Inital
extern void EP94M0_Power_Down(void);
extern void EP94M0_Power_Up(void);
extern void EP94M3_Rx_write_EDID(unsigned char Chip_Port, unsigned char *pData);
extern void EP94M3_Rx_Channel_Sel(unsigned char Chip_Port);

extern void delay_1ms(unsigned int DelayTime);

//--------------------------------------------------------------------------------------------------
//
// MHL Interface
//

extern unsigned char MHL_Rx_Signal(void);
extern void MHL_Rx_MHL_Mode(unsigned char Enable);
extern void MHL_Rx_RQ_AUTO_EN(unsigned char Enalbe);
extern unsigned char MHL_Rx_CBUS_Connect(void);
extern unsigned char MHL_MSC_Get_Flags(void);
extern void MHL_Clock_Mode(unsigned char Packed_Pixel_Mode);

// Read/Write Reg (CBus Responder)
extern void MHL_MSC_Reg_Update(unsigned char offset, unsigned char value); // Update Device Cap
extern unsigned char MHL_MSC_Reg_Read(unsigned char offset); // Read Status, Read Interrupt
extern void MHL_RCP_RAP_Read(unsigned char *pData);

// Send Command (CBus Requester)
extern unsigned char MHL_MSC_Cmd_READ_DEVICE_CAP(unsigned char offset, unsigned char *pValue);
extern unsigned char MHL_MSC_Cmd_HPD(unsigned char Set);
extern unsigned char MHL_MSC_Cmd_WRITE_STATE(unsigned char offset, unsigned char value);
extern unsigned char MHL_MSC_Cmd_MSC_MSG(unsigned char SubCmd, unsigned char ActionCode);
extern unsigned char MHL_MSC_Cmd_WRITE_BURST(unsigned char offset, unsigned char *pData, unsigned char size);

// Protected Functions
extern void MHL_MSC_Cmd_ACK(void);
extern void MHL_MSC_Cmd_ACK_ACK(void);
extern void MHL_MSC_Cmd_ABORT(void);


//--------------------------------------------------------------------------------------------------
//
// HDMI Transmiter Interface
//

// Common
extern unsigned char HDMI_Tx_RSEN(void);


//--------------------------------------------------------------------------------------------------
//
// Hardware Interface
//

// EP94M0
extern unsigned char EP94M0_Reg_Read(unsigned char ByteAddr, unsigned char *Data, unsigned int Size);
extern unsigned char EP94M0_Reg_Write(unsigned char ByteAddr, unsigned char *Data, unsigned int Size);
extern unsigned char EP94M0_Reg_Set_Bit(unsigned char ByteAddr, unsigned char BitMask);
extern unsigned char EP94M0_Reg_Clear_Bit(unsigned char ByteAddr, unsigned char BitMask);



#endif // EP94M0_IF_H


