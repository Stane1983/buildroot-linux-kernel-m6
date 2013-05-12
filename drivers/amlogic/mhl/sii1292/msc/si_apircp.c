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


#include "../msc/si_apircp.h"
#include "../msc/si_apirap.h"
#include "../cec/si_apicpi.h"
#include "../cec/si_apicec.h"
#include "../cbus/si_apicbus.h"
#include "../hal/si_hal.h"
#include "../api/si_regio.h"
#include "../api/si_1292regs.h"
#include "../api/si_api1292.h"
#include "../main/si_cp1292.h"


//------------------------------------------------------------------------------
// Function:		 SI_IrRcpKeys
// Description:	 IR to RCP handler
// Parameters:  key: IR input Key
//				 eventType: Key Pressed or Key Released
// Return:		 false
//				 1. Cbus driver return error
//				 2. Unsupported Keys
//------------------------------------------------------------------------------

#if USE_Scaler_Standby_Mode
bool_t power_key_temp = 1;
#endif

bool_t SI_IrRcpKeys( uint8_t key, uint8_t eventType )
{
	 bool_t result = false;
	 uint8_t channel = SI_CbusPortToChannel(g_data.portSelect);

	 DEBUG_PRINT(MSG_DBG, "Receive IR code: 0x%02X, EventType: 0x%02X\n", (int)key, (int)eventType);

	 if (eventType == KEY_DOWN)
	 {
		 cbus_out_queue_t req;

#if USE_Scaler_Standby_Mode
{
	 if((key == 0x40) && (power_key_temp == 0)){//[RC0x0C] Standby <<>> [CEC0x40]poweron
		 SI_RapSendMsg(0, MHL_RAP_CMD_CONTENT_ON);
		 power_key_temp = 1;
		 DEBUG_PRINT(MSG_ALWAYS, "RAP:: Send MHL_RAP_CMD_CONTENT_ON\n");
		 return result;
	 }else if((key == 0x40) && (power_key_temp == 1)){//[RC0x0C] Standby <<>> [CEC0x40]poweroff
		 SI_RapSendMsg(0, MHL_RAP_CMD_CONTENT_OFF);
		 power_key_temp = 0;
		 DEBUG_PRINT(MSG_ALWAYS, "RAP:: Send MHL_RAP_CMD_CONTENT_OFF\n");
		 return result;
	 }
}
#endif
		 DEBUG_PRINT(MSG_DBG, "Converting incoming IR command to RCP message\n");
		 req.command = MSC_MSG;
		 req.msgData[0] = MHL_MSC_MSG_RCP;
		 req.msgData[1] = key;
		 req.retry = 0;//Keno20120302, modification 1 to 0 for don't retry

		 SI_CbusPushReqInOQ(channel, &req, false);

		 DEBUG_PRINT(MSG_DBG, "Sending RCP message --> Key ID: %02X\n", (int)req.msgData[1]);
	 }

	 return result;

}



/*****************************************************************************/
/**
 *  Cbus RCP operation: Send RCPE message
 *
 *
 *  @param[in] 	 channel	 Cbus channel, always 0 in SiI1292
 *  @param[in] 	 statusCode		 including no error, ineffective code, responder busy
 *
 *  @return		 Result
 *
 *****************************************************************************/
static uint8_t SI_RcpSendErr(uint8_t channel, uint8_t statusCode)
{
	 uint8_t result = SUCCESS;
	 cbus_out_queue_t req;

	 req.command	 = MSC_MSG;
	 req.msgData[0]  = MHL_MSC_MSG_RCPE;
	 req.msgData[1]  = statusCode;
	 req.retry = 1;
	 result = SI_CbusPushReqInOQ( channel, &req, false );

	 DEBUG_PRINT(MSG_DBG, "RCP:: Sending RCPE -> Status Code: %02X\n", (int)statusCode);

	 return (result);
}



/*****************************************************************************/
/**
 *  Cbus RCP operation: Send RCPK message
 *
 *
 *  @param[in] 	 channel	 Cbus channel, always 0 in SiI1292
 *  @param[in] 	 keyID			 RCP Key ID
 *
 *  @return		 Result
 *
 *****************************************************************************/
static uint8_t SI_RcpSendAck(uint8_t channel, uint8_t keyID)
{
	 uint8_t result = SUCCESS;
	 cbus_out_queue_t req;

	 req.command	 = MSC_MSG;
	 req.msgData[0]  = MHL_MSC_MSG_RCPK;
	 req.msgData[1]  = keyID;
	 req.retry = 1;
	 result = SI_CbusPushReqInOQ( channel, &req, false );

	 DEBUG_PRINT(MSG_DBG, "RCP:: Sending RCPK --> Key ID: %02X\n", (int)keyID);

	 return (result);
}



/*****************************************************************************/
/**
 *  Cbus RCP operation: RCPK received handler
 *
 *
 *  @param[in] 	 channel	 Cbus channel, always 0 in SiI1292
 *  @param[in] 	 keyID			 RCP Key ID
 *
 *  @return		 Result
 *
 *****************************************************************************/
uint8_t SI_RcpAckRcvd(uint8_t channel, uint8_t keyID)
{
	 uint8_t result = SUCCESS;
	 uint8_t command, vsCmd, vsData, reqStatus;

	 command = SI_CbusOQReqCmd(channel);
	 vsCmd	 = SI_CbusOQReqData0(channel);
	 vsData  = SI_CbusOQReqData1(channel);
	 reqStatus = SI_CbusOQReqStatus(channel);

	 DEBUG_PRINT(MSG_DBG, "RCP:: RCPK Received <-- Key ID: %02X\n", (int)keyID);

	 if ( (command == MSC_MSG) && (vsCmd == MHL_MSC_MSG_RCP) && (vsData == keyID) )
	 {
		 if(reqStatus == CBUS_REQ_WAITING)
		 {
			 // If the req status is waiting, it means it didn't receive RCPE before RCPK
			 // If the req status is pending, there're 2 situations:
			 // 1. it received RCPE (Responder busy), and is waiting to retry.
			 // 2. it received RCPE (Ineffective KeyID), the current req is a new req.
			 SI_CbusOQCleanActiveReq(channel);
		 }
	 }

	 return (result);

}



/*****************************************************************************/
/**
 *  Cbus RCP operation: RCPE received handler
 *
 *
 *  @param[in] 	 channel	 Cbus channel, always 0 in SiI1292
 *  @param[in] 	 statusCode		 RCPE status code
 *
 *  @return		 Result
 *
 *****************************************************************************/
uint8_t SI_RcpErrRcvd(uint8_t channel, uint8_t statusCode)
{
	 uint8_t result = SUCCESS;

	 DEBUG_PRINT(MSG_DBG, "RCP:: RCPE Received <-- Status Code: %02X\n", (int)statusCode);

	 if (statusCode == RCPE_INEFFECTIVE_KEY_CODE)
	 {
#ifdef SUPPORT_MSCMSG_IGNORE_LIST
		 uint8_t msgData1;

		 msgData1 = SI_CbusOQReqData1(channel);
		 SI_CbusPushKeytoIgnoreList(channel, msgData1, CBUS_OUT);
#endif
	 }
	 else if (statusCode == RCPE_RESPONDER_BUSY)
	 {
		 uint8_t retry;

		 retry = SI_CbusOQReqRetry(channel);
		 if (retry > 0)
		 {
			 SI_CbusOQReduceReqRetry(channel);
			 SI_CbusOQSetReqStatus(channel, CBUS_REQ_PENDING);
			 SI_CbusOQSetReqTimeout(channel, 0);
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
 *  Cbus RCP operation: RCP timeout handler
 *
 *
 *  @param[in] 	 channel	 Cbus channel, always 0 in SiI1292
 *  @param[in] 	 direction		 CBUS_IN or CBUS_OUT
 *
 *  @return		 Result
 *
 *****************************************************************************/
uint8_t SI_RcpTimeoutHandler(uint8_t channel, uint8_t direction)
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
 *  Cbus RCP operation: RCP transfer done handler
 *
 *
 *  @param[in] 	 channel	 Cbus channel, always 0 in SiI1292
 *
 *  @return		 Result
 *
 *****************************************************************************/
uint8_t SI_RcpTransferDoneHandler(uint8_t channel)
{
	 uint8_t result = SUCCESS;
	 uint8_t command;

	 command = SI_CbusOQReqData0(channel);
	 if (command == MHL_MSC_MSG_RCP)
	 {
		 SI_CbusOQSetReqTimeout(channel, DEM_MHL_RCP_TIMEOUT);
	 }
	 else if (command == MHL_MSC_MSG_RCPK)
	 {
		 SI_CbusOQCleanActiveReq(channel);
	 }
	 else if (command == MHL_MSC_MSG_RCPE)
	 {
		 uint8_t uData;

		 uData = SI_CbusIQReqData(channel);//tiger sync to 9292
		 SI_CbusOQCleanActiveReq(channel);
		 result = SI_RcpSendAck(channel, uData);
	 }

	 return (result);

}



/*****************************************************************************/
/**
 *  Cbus RCP operation: RCP Cec return handler
 *
 *
 *  @param[in] 	 channel	 Cbus channel, always 0 in SiI1292
 *  @param[in] 	 success	 CEC return success or not
 *
 *  @return		 Result
 *
 *****************************************************************************/
void SI_RcpCecRetHandler(uint8_t channel, bool_t result)
{
	 uint8_t uData;

	 uData = SI_CbusIQReqData(channel);

	 if (result)
	 {
		 SI_RcpSendAck(channel, uData);
		 SI_CbusIQCleanActiveReq(channel);
	 }
	 else
	 {
		 SI_RcpSendErr(channel, RCPE_INEFFECTIVE_KEY_CODE);
#ifdef SUPPORT_MSCMSG_IGNORE_LIST
		 SI_CbusPushKeytoIgnoreList(channel, uData, CBUS_IN);
#endif
		 SI_CbusIQCleanActiveReq(channel);		 // SI_RcpSendErr will save the RCP Key ID temporaily in OQ.offsetData
	 }
}



/*****************************************************************************/
/**
 *  Cbus RCP operation: RCP handler
 *
 *
 *  @param[in] 	 channel	 Cbus channel, always 0 in SiI1292
 *
 *  @return		 Result
 *
 *****************************************************************************/
uint8_t SI_RcpHandler(uint8_t channel)
{
	 uint8_t result = SUCCESS;
	 uint8_t command, uData;

	 command = SI_CbusIQReqCmd(channel);
	 uData = SI_CbusIQReqData(channel);

	 if (command == MHL_MSC_MSG_RCP)
	 {
		 bool_t ret;
#ifdef SUPPORT_MSCMSG_IGNORE_LIST
		 ret = SI_CbusKeyIDCheck(channel, (uData & 0x7F), CBUS_IN);
#else
		 ret = SI_CbusKeyIDCheck((uData & 0x7F), CBUS_IN);
#endif
		 if (ret)
			 result = SI_RcpSendErr(channel, RCPE_INEFFECTIVE_KEY_CODE);
		 else
			 result = SI_RcpSendAck(channel, uData);

		 DEBUG_PRINT(MSG_DBG, "RCP:: RCP Received <-- Key ID: %02X\n", (int)uData);
		 SI_CbusIQCleanActiveReq(channel);

	 }
	 else if (command == MHL_MSC_MSG_RCPK)
	 {
		 uint8_t cmd, msgData0, msgData1, reqStatus;

		 cmd = SI_CbusOQReqCmd(channel);
		 msgData0 = SI_CbusOQReqData0(channel);
		 msgData1 = SI_CbusOQReqData0(channel);
		 reqStatus = SI_CbusOQReqStatus(channel);

		 DEBUG_PRINT(MSG_DBG, "RCP:: RCPK Received <-- Key ID: %02X\n", (int)uData);

		 if ( (cmd == MSC_MSG) && (msgData0 == MHL_MSC_MSG_RCP) && (msgData1 == uData) )
		 {
			 if(reqStatus == CBUS_REQ_WAITING)
			 {
				 // If the req status is waiting, it means it didn't receive RCPE before RCPK
				 // If the req status is pending, there're 2 situations:
				 // 1. it received RCPE (Responder busy), and is waiting to retry.
				 // 2. it received RCPE (Ineffective KeyID), the current req is a new req.
				 SI_CbusOQCleanActiveReq(channel);
			 }
		 }

		 SI_CbusIQCleanActiveReq(channel);
	 }
	 else if (command == MHL_MSC_MSG_RCPE)
	 {
		 DEBUG_PRINT(MSG_DBG, "RCP:: RCPE Received <-- Status Code: %02X\n", (int)uData);

		 if (uData == RCPE_INEFFECTIVE_KEY_CODE)
		 {
#ifdef SUPPORT_MSCMSG_IGNORE_LIST
			 uint8_t msgData1;

			 msgData1 = SI_CbusOQReqData1(channel);

			 SI_CbusPushKeytoIgnoreList(channel, msgData1, CBUS_OUT);
#endif
		 }
		 else if (uData == RCPE_RESPONDER_BUSY)
		 {
			 uint8_t retry;

			 retry = SI_CbusOQReqRetry(channel);
			 if (retry > 0)
			 {
				 SI_CbusOQReduceReqRetry(channel);
				 SI_CbusOQSetReqStatus(channel, CBUS_REQ_PENDING);
				 SI_CbusOQSetReqTimeout(channel, 0);
			 }
			 else if (retry == 0)
			 {
				 SI_CbusOQCleanActiveReq(channel);
			 }
		 }

		 SI_CbusIQCleanActiveReq(channel);
	 }

	 return (result);

}


