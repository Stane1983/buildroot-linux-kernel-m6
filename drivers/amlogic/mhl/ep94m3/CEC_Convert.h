/*******************************************************************************

          (c) Copyright Explore Semiconductor, Inc. Limited 2005
                           ALL RIGHTS RESERVED

--------------------------------------------------------------------------------

  File        :  MHL_Controller.h

  Description :  Head file of MHL_Controller Interface

*******************************************************************************/

#ifndef _CEC_CONVERT_H
#define _CEC_CONVERT_H


//==================================================================================================
//

typedef enum {
	LINK_STATE_Start = 0,
	LINK_STATE_HDMI_Mode,
	LINK_STATE_MHL_Mode
} LINK_STATE;

#define Disable_All 0xFF

//==================================================================================================
//
// Protected Data Member
//

extern LINK_STATE Link_State;

//==================================================================================================
//
// Public Functions
//
extern void Rx_HTPLUG(unsigned char HTP_Enable, unsigned char Rx_Port);
extern unsigned char MHL_CDS_Check(unsigned char Rx_port);

extern void RCP_Conversion_Init(void);
extern void RCP_Conversion_Timer(void);// every 10ms
extern void Rx_Port_Sel(unsigned char Chip_Port);
extern void RCP_Conversion_Task(void);


#endif // _CEC_CONVERT_H


