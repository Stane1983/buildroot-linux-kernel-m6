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


#include "../msc/si_apirap.h"
#include "../cbus/si_apicbus.h"
#include "../cec/si_apicpi.h"
#include "../cec/si_apicec.h"
#include "../hal/si_hal.h"
#include "../api/si_regio.h"
#include "../api/si_1292regs.h"
#include "../api/si_api1292.h"



uint8_t RapSubCodeCmd[] =
{
	MHL_RAP_CMD_POLL,
	MHL_RAP_CMD_CONTENT_ON,
	MHL_RAP_CMD_CONTENT_OFF
};

bool_t IsThisRapMsgInCategoryC(uint8_t rapStatusCode)
{
	int i;
	bool_t result = false;

	for( i = 0; i < 3; i++)
	{
		if ( rapStatusCode == RapSubCodeCmd[i])
		{
			result = true;
			break;
		}
	}

	return result;
}
/*****************************************************************************/
/**
 *  Cbus RAP operation: Send RAPK message
 *
 *
 *  @param[in]          channel		Cbus channel, always 0 in SiI1292
 *  @param[in]          statusCode		including no error, Unrecognized Action Code, Unsupported Action Code, responder busy
 *
 *  @return             Result
 *
 *****************************************************************************/
bool_t SI_RapSendAck(uint8_t channel, uint8_t statusCode)
{
	bool_t result = false;
	cbus_out_queue_t	req;

	req.command     = MSC_MSG;
	req.msgData[0]  = MHL_MSC_MSG_RAPK;
	req.msgData[1]  = statusCode;
	req.retry = 1;
	result = SI_CbusPushReqInOQ( channel, &req, false );

	DEBUG_PRINT(MSG_DBG, "RAP:: Sending RAPK --> Status Code: %02X\n", (int)statusCode);

    return( result);
}
/*****************************************************************************/
/**
 *  Cbus RAP operation: Send RAP message
 *
 *
 *  @param[in]          channel		Cbus channel, always 0 in SiI1292
 *  @param[in]          rapKeyCode	RAP Key ID
 *
 *  @return             Result
 *
 *****************************************************************************/
bool_t SI_RapSendMsg(uint8_t channel, uint8_t rapKeyCode)
{
	bool_t result = false;
	cbus_out_queue_t	req;

	req.command     = MSC_MSG;
	req.msgData[0]  = MHL_MSC_MSG_RAP;
	req.msgData[1]  = rapKeyCode;
	req.retry = 0;//Keno20120305, disable retry 1 to 0.
	result = SI_CbusPushReqInOQ( channel, &req, false );
	return( result);
}
/*****************************************************************************/
/**
 *  Cbus RAP operation: RAP Handler
 *
 *
 *  @param[in]          channel		Cbus channel, always 0 in SiI1292
 *
 *  @return             Result
 *
 *****************************************************************************/
bool_t SI_RapHandler(uint8_t channel)
{
	bool_t result = false;
	uint8_t command;
	uint8_t rapKeyCode;

	command = SI_CbusIQReqCmd(channel);
	rapKeyCode = SI_CbusIQReqData(channel);

	if (command == MHL_MSC_MSG_RAP)
	{

		DEBUG_PRINT(MSG_ALWAYS, "RAP:: RAP Received <-- Key ID: %02X\n", (int)rapKeyCode);

		if ( !IsThisRapMsgInCategoryC(rapKeyCode) )
		{
			DEBUG_PRINT( MSG_ALWAYS, "SI_RcpMsgHandler: unrecognized Key Code: %s", (int)rapKeyCode) ;
			SI_RapSendAck(channel, RAP_UNRECOGNIZED);
			SI_CbusIQCleanActiveReq(channel);
			return result;
		}

		switch ( rapKeyCode )
		{
			case MHL_RAP_CMD_POLL:
				SI_RapSendAck(channel, RAP_NOERROR);
				SI_CbusIQCleanActiveReq(channel);
				break;
			case MHL_RAP_CMD_CONTENT_ON:
				SiIRegioModify(REG_TX_SWING1, BIT_SWCTRL_EN, BIT_SWCTRL_EN);
				DEBUG_PRINT(MSG_ALWAYS, "RAP:: CONTENT ON!");
				SiIRegioWrite(0x00A, 0x3);/*Restore orignial value, see bug22585*/
				SI_RapSendAck(channel, RAP_NOERROR);
				SI_CbusIQCleanActiveReq(channel);
				break;
			case MHL_RAP_CMD_CONTENT_OFF:
				SiIRegioWrite(0x00A, 0x0);/*set it to 0 to enable TMDS Swing control, see bug22585*/
				SiIRegioModify(REG_TX_SWING1, BIT_SWCTRL_EN, 0);
				DEBUG_PRINT(MSG_ALWAYS, "RAP:: CONTENT OFF!");
				SI_RapSendAck(channel, RAP_NOERROR);
				SI_CbusIQCleanActiveReq(channel);
				break;
			default:
				DEBUG_PRINT( MSG_ALWAYS, "SI_RcpMsgHandler: unrecognized Key Code: %s", (int)rapKeyCode) ;
				SI_RapSendAck(channel, RAP_UNRECOGNIZED);
				SI_CbusIQCleanActiveReq(channel);
				break;
		}

	}
	else
	{
		DEBUG_PRINT( MSG_ALWAYS, "CBUS:: Received <-- MHL_MSC_MSG_RAPK\n");
		SI_CbusIQCleanActiveReq(channel);
		result = SI_RapAckRcvd(channel, rapKeyCode);
	}
	return result;

}
/*****************************************************************************/
/**
 *  Cbus RAP operation: Send RAP message
 *
 *
 *  @param[in]          channel		Cbus channel, always 0 in SiI1292
 *  @param[in]          rapKeyCode	RAP Key ID
 *
 *  @return             Result
 *
 *****************************************************************************/
bool_t SI_RapAckRcvd(uint8_t channel, uint8_t rapKeyCode)
{
	DEBUG_PRINT(MSG_DBG, "RAP:: RAPK Received <-- Status Code: %02X\n", (int)rapKeyCode);

	if (rapKeyCode)
		SI_RapErrProcess ( channel, rapKeyCode );
	else
		SI_CbusOQCleanActiveReq(channel);
	return SUCCESS;
}
/*****************************************************************************/
/**
 *  Cbus RAP operation:  RAP error process
 *
 *
 *  @param[in]          channel		Cbus channel, always 0 in SiI1292
 *  @param[in]          rapStatusCode	including Unrecognized Action Code, Unsupported Action Code, responder busy
 *
 *  @return             Result
 *
 *****************************************************************************/
bool_t SI_RapErrProcess(uint8_t channel, uint8_t rapStatusCode)
{
#ifdef CONFIG_MHL_SII1292_CEC
	SI_CpiData_t cecFrame;
#endif
	uint8_t retry;

	switch (rapStatusCode)
	{
		case RAP_UNRECOGNIZED:
		case RAP_UNSUPPORTED:

#ifdef SUPPORT_MSCMSG_IGNORE_LIST
			//add to ignore list
			SI_CbusOQCleanActiveReq(channel);
			if (rapStatusCode == MHL_RAP_CMD_CONTENT_ON)
				SI_CbusPushKeytoIgnoreList(channel, MHL_RAP_CMD_CONTENT_ON_IREG, CBUS_OUT);
			if (rapStatusCode == MHL_RAP_CMD_CONTENT_OFF)
				SI_CbusPushKeytoIgnoreList(channel, MHL_RAP_CMD_CONTENT_OFF_IREG, CBUS_OUT);
#endif
#if defined(CONFIG_MHL_SII1292_CEC)
			//feature abort
			cecFrame.opcode        = CECOP_FEATURE_ABORT;
			cecFrame.srcDestAddr   = MAKE_SRCDEST( g_cecAddress, CEC_LOGADDR_TV );
			cecFrame.args[0]       = rapStatusCode;
			cecFrame.args[1]       = CECAR_UNRECOG_OPCODE;
			cecFrame.argCount      = 2;
			SI_CpiWrite( &cecFrame );
#endif
			break;
		case RAP_BUSY:
			retry = SI_CbusOQReqRetry(channel);
			if (retry > 0)
			{
				SI_CbusOQReduceReqRetry(channel);
				SI_CbusOQSetReqStatus(channel, CBUS_REQ_PENDING);
			}
			else if (retry == 0)
			{
				SI_CbusOQCleanActiveReq(channel);
			}
			break;
	}
	return SUCCESS;
}
/*****************************************************************************/
/**
 *  Cbus RAP operation: RAP timeout handler
 *
 *
 *  @param[in]          channel		Cbus channel, always 0 in SiI1292
 *  @param[in]          direction		include CBUS_IN and CBUS_OUT
 *
 *  @return             Result
 *
 *****************************************************************************/
bool_t SI_RapTimeoutHandler(uint8_t channel, uint8_t direction)
{
	uint8_t result = SUCCESS;

	if (direction == CBUS_IN)
	{
		SI_CbusIQCleanActiveReq(channel);
	}
	else if (direction == CBUS_OUT)
	{
		uint8_t retry;

		retry = SI_CbusOQReqRetry(channel);
		if (retry > 0)
		{
			SI_CbusOQReduceReqRetry(channel);
			SI_CbusOQSetReqStatus(channel, CBUS_REQ_PENDING);
		}
		else if (retry == 0)
		{
			SI_CbusOQCleanActiveReq(channel);
		}
	}

	return (result);
}
/*****************************************************************************/
/**
 *  Cbus RAP operation: RAP transfer donw handler
 *
 *
 *  @param[in]          channel		Cbus channel, always 0 in SiI1292
 *
 *  @return             Result
 *
 *****************************************************************************/
uint8_t SI_RapTransferDoneHandler(uint8_t channel)
{
	uint8_t result = SUCCESS;
	uint8_t command;

	command = SI_CbusOQReqData0(channel);
	if (command == MHL_MSC_MSG_RAP)
	{
		SI_CbusOQSetReqTimeout(channel, DEM_MHL_RCP_TIMEOUT);
	}
	else if (command == MHL_MSC_MSG_RAPK)
	{
		SI_CbusOQCleanActiveReq(channel);
	}

	return (result);

}

/*****************************************************************************/
/**
 *  Cbus RAP operation: RAP ignore keyID check
 *
 *
 *  @param[in]          channel		Cbus channel, always 0 in SiI1292
 *  @param[in]          keyID			RAP Key ID
 *  @param[in]          direction		include CBUS_IN and CBUS_OUT
 *
 *  @return             Result
 *
 *****************************************************************************/
#if 0//Keno20120301, disable RK design, because it will effect that cannot display
#ifdef SUPPORT_MSCMSG_IGNORE_LIST
bool_t SI_RapKeyIDCheck(uint8_t channel, uint8_t keyID, uint8_t direction)
#else
bool_t SI_RapKeyIDCheck(uint8_t keyID, uint8_t direction)
#endif
{
	bool_t ret;

	ret = false;
	if (keyID == MHL_RAP_CMD_CONTENT_ON)
#ifdef SUPPORT_MSCMSG_IGNORE_LIST
		ret = SI_CbusKeyIDCheck(channel, MHL_RAP_CMD_CONTENT_ON_IREG, direction);
#else
		ret = SI_CbusKeyIDCheck(MHL_RAP_CMD_CONTENT_ON_IREG, direction);
#endif
	if (keyID == MHL_RAP_CMD_CONTENT_OFF)
#ifdef SUPPORT_MSCMSG_IGNORE_LIST
		ret = SI_CbusKeyIDCheck(channel, MHL_RAP_CMD_CONTENT_OFF_IREG, direction);
#else
		ret = SI_CbusKeyIDCheck(MHL_RAP_CMD_CONTENT_OFF_IREG, direction);
#endif

	return (ret);
}
#endif

/*****************************************************************************/
/**
 *  Cbus RAP operation: RAP cec return handler
 *
 *
 *  @param[in]          channel		Cbus channel, always 0 in SiI1292
 *  @param[in]          success		if return successfully
 *
 *  @return             Result
 *
 *****************************************************************************/
void SI_RapCecRetHandler(uint8_t channel, bool_t result)
{
	uint8_t uData;

	uData = SI_CbusIQReqData(channel);

	if (result)
	{
		SI_RapSendAck(channel, RAP_NOERROR);
		SI_CbusIQCleanActiveReq(channel);
	}
	else
	{
		SI_RapSendAck(channel, RAP_UNSUPPORTED);

#ifdef SUPPORT_MSCMSG_IGNORE_LIST
		if (uData == MHL_RAP_CMD_CONTENT_ON)
			SI_CbusPushKeytoIgnoreList(channel, MHL_RAP_CMD_CONTENT_ON_IREG, CBUS_IN);
		if (uData == MHL_RAP_CMD_CONTENT_OFF)
			SI_CbusPushKeytoIgnoreList(channel, MHL_RAP_CMD_CONTENT_OFF_IREG, CBUS_IN);
#endif
		SI_CbusIQCleanActiveReq(channel);		// SI_RcpSendErr will save the RCP Key ID temporaily in OQ.offsetData
	}
}

