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

#ifndef __MHL_SII1292_API_RCP_H
#define __MHL_SII1292_API_RCP_H

#include <linux/types.h>

#include "../api/si_datatypes.h"


//------------------------------------------------------------------------------
// Module variables
//------------------------------------------------------------------------------

typedef struct
{
	uint8_t cecCommand;	//!< rc protocol command code
	uint8_t rcpKeyCode;	//!< RCP CBUS Key Code
	char   rcpName[15];
} SI_CecRcpConversion_t;


#define RCPE_INEFFECTIVE_KEY_CODE		0x01
#define RCPE_RESPONDER_BUSY			0x02

#define RCP_KEYCODE_NUM				0x80

bool_t 		SI_IrRcpKeys(uint8_t key, uint8_t eventType);
uint8_t 	SI_RcpAckRcvd(uint8_t channel, uint8_t KeyID);
uint8_t 	SI_RcpErrRcvd(uint8_t channel, uint8_t statusCode);
uint8_t 	SI_RcpTimeoutHandler(uint8_t channel, uint8_t direction);
uint8_t 	SI_RcpTransferDoneHandler(uint8_t channel);
void 		SI_RcpCecRetHandler(uint8_t channel, bool_t result);
uint8_t 	SI_RcpHandler(uint8_t channel);

#endif /* __MHL_SII1292_API_RCP_H */

