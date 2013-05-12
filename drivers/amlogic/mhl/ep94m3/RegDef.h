/*******************************************************************************

          (c) Copyright Explore Semiconductor, Inc. Limited 2005
                           ALL RIGHTS RESERVED

--------------------------------------------------------------------------------

 Please review the terms of the license agreement before using this file.
 If you are not an authorized user, please destroy this source code file
 and notify Explore Semiconductor Inc. immediately that you inadvertently
 received an unauthorized copy.

--------------------------------------------------------------------------------

  File        :  EP4M0RegDef.h

  Description :  Register Address definitions of EP94M0 \ EP94M3.

*******************************************************************************/

#ifndef EP94M0_REGDEF_H
#define EP94M0_REGDEF_H

//////////////////////////////////////////////////////////////////////////////////////
// Registers									Word	BitMask
//

#define EP94M0_General_Control_0				0x40
#define EP94M0_General_Control_0__RSEN_BYP				0x80
#define EP94M0_General_Control_0__PP_MODE				0x40
#define EP94M3_General_Control_0__MHL_MODE				0x20
#define EP94M3_General_Control_0__CBUS_HPD				0x10
#define EP94M0_General_Control_0__SOFT_RST				0x08
#define EP94M0_General_Control_0__PWR_DWN				0x04
#define EP94M3_General_Control_0__PORT_SEL				0x03

#define EP94M3_EDID_Control						0x42
#define EP94M3_EDID_Control__SDA_MODE					0x80
#define EP94M3_EDID_Control__SCL_MODE					0x40
#define EP94M3_EDID_Control__DDC_SW_ON					0x20
#define EP94M3_EDID_Control__EDID_EN2					0x10
#define EP94M3_EDID_Control__EDID_EN1					0x08
#define EP94M3_EDID_Control__EDID_EN0					0x04
#define EP94M3_EDID_Control__EDID_SEL					0x03

#define EP94M0_General_Control_1				0x4B
#define EP94M0_General_Control_1__TX_RSEN				0x80
#define EP94M0_General_Control_1__RX_LINK_ON			0x20
#define EP94M0_General_Control_1__RX_DE_ON				0x10
#define EP94M0_General_Control_1__TX_Test				0x08
#define EP94M0_General_Control_1__DK_A					0x07

#define EP94M0_RX_PHY_Control					0x4C // There are 3 bytes in this address
#define EP94M0_TX_PHY_Control					0x4D // There are 3 bytes in this address

//////////////////////////////////////////////////////////////////////////////////////
// Test

#define EP94M0_Serial_Data_Stream				0x4E // There are 6 bytes in this address

//////////////////////////////////////////////////////////////////////////////////////
// MHL add-on

#define EP94M0_CBUS_MSC_Dec_Capability			0x50 // There are 16 bytes in this address
#define EP94M0_CBUS_MSC_Dec_Interrupt			0x51 // There are  4 bytes in this address
#define EP94M0_CBUS_MSC_Dec_Status				0x52 // There are  4 bytes in this address
#define EP94M0_CBUS_MSC_Dec_SrcPad				0x53 // There are 16 bytes in this address
#define EP94M0_CBUS_MSC_RAP_RCP					0x54 // There are  2 bytes in this address

#define EP94M0_CBUS_MSC_Interrupt				0x55
#define EP94M3_CBUS_MSC_Interrupt__INT_POL				0x80
#define EP94M3_CBUS_MSC_Interrupt__WB_SPT				0x40

#define EP94M0_CBUS_MSC_Interrupt__MSG_IE				0x40
#define EP94M0_CBUS_MSC_Interrupt__SCR_IE				0x20
#define EP94M0_CBUS_MSC_Interrupt__INT_IE				0x10

#define EP94M3_CBUS_MSC_Interrupt__MSG_IE				0x04
#define EP94M3_CBUS_MSC_Interrupt__SCR_IE				0x02
#define EP94M3_CBUS_MSC_Interrupt__INT_IE				0x01

#define EP94M0_CBUS_MSC_Interrupt__MSG_F				0x04
#define EP94M0_CBUS_MSC_Interrupt__SCR_F				0x02
#define EP94M0_CBUS_MSC_Interrupt__INT_F				0x01

#define EP94M0_CBUS_RQ_Control					0x56
#define EP94M0_CBUS_RQ_Control__RQ_DONE					0x80
#define EP94M0_CBUS_RQ_Control__RQ_ERR					0x40
#define EP94M0_CBUS_RQ_Control__CBUS_DSCed				0x20
#define EP94M0_CBUS_RQ_Control__RQ_AUTO_EN				0x08
#define EP94M0_CBUS_RQ_Control__CBUS_TRI				0x04
#define EP94M0_CBUS_RQ_Control__RQ_ABORT				0x02
#define EP94M0_CBUS_RQ_Control__RQ_START				0x01

#define EP94M0_CBUS_RQ_SIZE						0x57
#define EP94M0_CBUS_RQ_SIZE__RX_SIZE					0x60
#define EP94M0_CBUS_RQ_SIZE__TX_SIZE					0x1F
//#define EP94M0_CBUS_RQ_HEADER					0x57
#define EP94M0_CBUS_RQ_HEADER__DDC_Packet				0x00
#define EP94M0_CBUS_RQ_HEADER__VS_Packet				0x02
#define EP94M0_CBUS_RQ_HEADER__MSC_Packet				0x04
#define EP94M0_CBUS_RQ_HEADER__isCommand				0x01
//#define EP94M0_CBUS_RQ_CMD					0x57
//#define EP94M0_CBUS_RQ_TD						0x57

#define EP94M0_CBUS_RQ_ACT_RX_SIZE				0x58
//#define EP94M0_CBUS_RQ_RD						0x58

#define EP94M0_CBUS_Vendor_ID					0x59

#define EP94M0_CBUS_BR_ADJ						0x5A

#define EP94M3_CBUS_TX_Re_Try					0x5B

#define EP94M3_CBUS_Time_Out					0x5C

#define EP94M3_EDID_Data			 			0xFF			// 256 Byte

#endif