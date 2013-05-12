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

#ifndef __MHL_SII1292_API_RAP_H
#define __MHL_SII1292_API_RAP_H

#include <linux/types.h>

#include "../api/si_datatypes.h"

//------------------------------------------------------------------------------
// Module variables
//------------------------------------------------------------------------------

typedef struct
{
	uint8_t cecCommand;	//!< rc protocol command code
	uint8_t rapKeyCode;	//!< RCP CBUS Key Code
	char   rapName[15];
} SI_CecRapConversion_t;


#define CATEGORY_C_CMD_NUM	31
#define CECNOMATCH	0xFF
#define RAPNOMATCH	0xFF

#define RAP_NOERROR	   0x00
#define RAP_UNRECOGNIZED 0x01
#define RAP_UNSUPPORTED  0x02
#define RAP_BUSY		   0x03

#define MHL_RAP_CMD_POLL		0x00
#define MHL_RAP_CMD_CONTENT_ON		0x10
#define MHL_RAP_CMD_CONTENT_OFF		0x11

#define RAP_IN 0
#define RAP_OUT 1

#define MHL_RAP_CMD_CONTENT_ON_IREG 0x6B
#define MHL_RAP_CMD_CONTENT_OFF_IREG 0x6C


bool_t IsThisRapMsgInCategoryC(uint8_t rapStatusCode);
bool_t SI_RapHandler(uint8_t channel);
bool_t SI_RapCecTranslater(uint8_t channel, uint8_t rapKeyCode);
bool_t SI_RapTimeoutHandler(uint8_t channel, uint8_t direction);

#if !defined(CONFIG_MHL_SII1292_CEC)
#ifdef SUPPORT_MSCMSG_IGNORE_LIST
bool_t SI_RapKeyIDCheck(uint8_t channel, uint8_t keyID, uint8_t direction);
#else
bool_t SI_RapKeyIDCheck(uint8_t keyID, uint8_t direction);
#endif
#endif
bool_t SI_RapSendMsg(uint8_t channel, uint8_t rapKeyCode);
bool_t SI_RapSendAck(uint8_t channel, uint8_t rapStatusCode);
bool_t SI_RapAckRcvd(uint8_t channel, uint8_t rapKeyCode);
bool_t SI_RapErrProcess(uint8_t channel, uint8_t rapStatusCode);
uint8_t SI_RapTransferDoneHandler(uint8_t channel);
void SI_RapCecRetHandler(uint8_t channel, bool_t result);

#endif /* __MHL_SII1292_API_RAP_H */

