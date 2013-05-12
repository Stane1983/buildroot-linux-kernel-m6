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


#ifndef __SI_APICBUS_H__
#define __SI_APICBUS_H__

#include "../api/si_datatypes.h"
#include "../cbus/si_cbusdefs.h"

#define SUPPORT_MSCMSG_IGNORE_LIST
//------------------------------------------------------------------------------
// API Manifest Constants
//------------------------------------------------------------------------------

#define CBUS_NOCHANNEL		0xFF
#define CBUS_NOPORT		0xFF

enum
{
    MHL_MSC_MSG_RCP             = 0x10,     // RCP sub-command
    MHL_MSC_MSG_RCPK            = 0x11,     // RCP Acknowledge sub-command
    MHL_MSC_MSG_RCPE            = 0x12,     // RCP Error sub-command
    MHL_MSC_MSG_RAP             = 0x20,     // RAP sub-command
    MHL_MSC_MSG_RAPK            = 0x21,     // RAP Acknowledge sub-command
};

enum
{
    MHL_MSC_MSG_NO_ERROR        = 0x00,     // RCP No Error
    MHL_MSC_MSG_ERROR_KEY_CODE  = 0x01,     // RCP Unrecognized Key Code
    MHL_MSC_MSG_BUSY            = 0x02,     // RCP Response busy
};

enum
{
    MHL_RCP_CMD_POLL            = 0x00,
    MHL_RCP_CMD_PLAY            = 0x01,
    MHL_RCP_CMD_PAUSE           = 0x02,
    MHL_RCP_CMD_STOP            = 0x03,
    MHL_RCP_CMD_FASTFORWARD     = 0x04,
    MHL_RCP_CMD_REWIND          = 0x05,
    MHL_RCP_CMD_CH_UP           = 0x06,
    MHL_RCP_CMD_CH_DOWN         = 0x07,
    MHL_RCP_CMD_MENU            = 0x10,
    MHL_RCP_CMD_EXIT            = 0x11,
    MHL_RCP_CMD_OK              = 0x12,
    MHL_RCP_CMD_LEFT            = 0x13,
    MHL_RCP_CMD_RIGHT           = 0x14,
    MHL_RCP_CMD_UP              = 0x15,
    MHL_RCP_CMD_DOWN            = 0x16,

    MHL_RCP_CMD_AUDIO_TRACK     = 0x20,
    MHL_RCP_CMD_SUBTITLES       = 0x21,

    MHL_RCP_CMD_PWR_ON          = 0x30,
    MHL_RCP_CMD_PWR_OFF         = 0x31,
    MHL_RCP_CMD_VOL_UP          = 0x32,
    MHL_RCP_CMD_VOL_DOWN        = 0x33,
    MHL_RCP_CMD_MUTE            = 0x34,
    MHL_RCP_CMD_UN_MUTE         = 0x35,
    MHL_RCP_CMD_ACTIVE_SOURCE   = 0x36,

    MHL_RCP_CMD_NUM_0           = 0x40,
    MHL_RCP_CMD_NUM_1           = 0x41,
    MHL_RCP_CMD_NUM_2           = 0x42,
    MHL_RCP_CMD_NUM_3           = 0x43,
    MHL_RCP_CMD_NUM_4           = 0x44,
    MHL_RCP_CMD_NUM_5           = 0x45,
    MHL_RCP_CMD_NUM_6           = 0x46,
    MHL_RCP_CMD_NUM_7           = 0x47,
    MHL_RCP_CMD_NUM_8           = 0x48,
    MHL_RCP_CMD_NUM_9           = 0x49,
    MHL_RCP_CMD_NUM_10          = 0x4A,
    MHL_RCP_CMD_NUM_11          = 0x4B,
    MHL_RCP_CMD_NUM_12          = 0x4C,

    MHL_RCP_CMD_KEY_RELEASED	= 0x80,
};

enum
{
    MSC_WRITE_STAT              = 0x60,     // Two commands, one opCode
    MSC_SET_INT                 = 0x60,     // Two commands, one opCode
    MSC_READ_DEVCAP             = 0x61,
    MSC_GET_STATE               = 0x62,
    MSC_GET_VENDOR_ID           = 0x63,
    MSC_SET_HPD                 = 0x64,
    MSC_CLR_HPD                 = 0x65,
    MSC_SET_CEC_CAP_ID          = 0x66,
    MSC_GET_CEC_CAP_ID          = 0x67,
    MSC_MSG                     = 0x68,
    MSC_GET_VS_ABORT            = 0x69,
    MSC_GET_DDC_ABORT           = 0x6A,
    MSC_GET_MSC_ABORT           = 0x6B,
    MSC_WRITE_BURST             = 0x6C,
};

enum
{
    CBUS_TASK_IDLE,
    CBUS_TASK_TRANSLATION_LAYER_DONE,
    CBUS_TASK_WAIT_FOR_RESPONSE,
};

//
// CBUS module reports these error types
//
typedef enum
{
    STATUS_SUCCESS = 0,
    ERROR_CBUS_CAN_RETRY,
    ERROR_CBUS_ABORT,
    ERROR_CBUS_TIMEOUT,
    ERROR_CBUS_LINK_DOWN,
    ERROR_INVALID,
    ERROR_INIT,
    ERROR_WRITE_FAILED,
    ERROR_NO_HEARTBEAT,
    ERROR_WRONG_DIRECTION,
    ERROR_QUEUE_FULL,
    ERROR_MSC_MSG,
    ERROR_INVALID_MSC_CMD,
    ERROR_RET_HANDLER,
} CBUS_SOFTWARE_ERRORS_t;

typedef enum
{
    CBUS_IDLE           = 0,    // BUS idle
    CBUS_SENT,                  // Command sent
    CBUS_XFR_DONE,              // Translation layer complete
    CBUS_WAIT_RESPONSE,         // Waiting for response
} CBUS_STATE_t;

typedef enum
{
    CBUS_REQ_IDLE       = 0,
    CBUS_REQ_PENDING,           // Request is waiting to be processed
    CBUS_REQ_WAITING,			// Request is waiting response
} CBUS_REQ_t;

//------------------------------------------------------------------------------
// API typedefs
//------------------------------------------------------------------------------

//
// structure to hold command details from upper layer to CBUS module
//
typedef bool_t (*dataRetHandler_t)(uint8_t, uint8_t);

typedef struct
{
	uint8_t reqStatus;                      			// REQ_IDLE, REQ_PENDING, REQ_WAITING
	uint8_t command;                        			// MSC command
	uint8_t offsetData;                     			// Offset of register on CBUS
	uint8_t length;                         			// Only applicable to write burst. ignored otherwise.
	uint8_t msgData[MHL_MAX_BUFFER_SIZE];   			// Pointer to message data area.
	dataRetHandler_t dataRetHandler;			// Data return handler for MSC commands
	uint32_t timeout;									// timing control
	uint32_t base_time;									// record the base time
	bool_t  timer_set;									// flag for enable/disable timer
	uint8_t retry;										// to define whether this command has been retried, default false
	bool_t  cecReq;										// define whether the Req is from Cec
	bool_t  vip;										// define whether the Req is a high priority request
} cbus_out_queue_t;

typedef struct
{
	uint8_t reqStatus;                      			// REQ_IDLE, REQ_PENDING, REQ_WAITING
	uint8_t command;                        			// RAP or  RCP opcode
	uint8_t msgData;                     				// RAP or RCP data
	uint32_t timeout;									// timing control
	uint32_t base_time;									// record the base time
	bool_t  timer_set;									// flag for enable/disable timer
} cbus_in_queue_t;

#define CBUS_MAX_COMMAND_QUEUE      2

#define MSC_MSG_MAX_IGNORE_CMD		0x80

typedef struct
{
	bool_t  connected;      							// True if a connected MHL port
	bool_t	connect_wait;								// True if wait enough time to send any command after cbus connection
	bool_t  hpd_state;									// For HPD
	bool_t  grt_wrt;									// For grant write scratch pad
	bool_t  path_enable;								// For path enable
	bool_t	connect_ready;								// For Connected Ready Register
	bool_t  dcap_rdy;									// For Peer Dcap Reg ready
	uint8_t	read_info;									// For read peer info
	bool_t  abort_wait;									// For ABORT_WAIT
	uint32_t base_time;									// record the base time
	uint8_t o_activeIndex;    							// Active queue entry for sending command queue.
	uint8_t i_activeIndex;								// Active queue entry for sending command queue.
	cbus_out_queue_t  o_queue[ CBUS_MAX_COMMAND_QUEUE ];// sending command queue (RAP/RCP/MSC)
	cbus_in_queue_t   i_queue[ CBUS_MAX_COMMAND_QUEUE ];// receiving command queue (RAP/RCP)
	// CBUS_MAX_COMMAND_QUEUE = 2, reason for i_queue is timeout may cause receving 2 or more commands
#ifdef SUPPORT_MSCMSG_IGNORE_LIST
	bool_t	o_mscmsg_ignore_list[ MSC_MSG_MAX_IGNORE_CMD ];	// sending msc msg ignore list
	bool_t	i_mscmsg_ignore_list[ MSC_MSG_MAX_IGNORE_CMD ];	// receiving msc msg ignore list
#endif
	bool_t	rcp_support;								// peer RCP_SUPPORT
	bool_t	rap_support;								// peer RAP_SUPPORT
} cbusChannelState_t;

enum
{
    KEY_NOT_IN_IGNORELIST = 0x00,
    KEY_IN_IGNORELIST = 0x01,
    KEY_STATUS_NOT_INITLIAED = 0x20,
};

typedef enum
{
    CBUS_IN       = 0,	// Receiving
    CBUS_OUT,           // Sending
    CBUS_INOUT,			// Both
} CBUS_Direction_t;


#define DCAP_READ_DONE  0x80
#define DCAP_ITEM_READ_DOING 0x40
#define DCAP_ITEM_INDEX_MASK 0x3F

//------------------------------------------------------------------------------
// API Function Templates
//------------------------------------------------------------------------------

bool_t      SI_CbusInitialize( void );
void 		SI_CbusInitDevCap( uint8_t channel );
uint8_t     SI_CbusHandler( uint8_t channel );
uint8_t     SI_CbusPortToChannel( uint8_t port );

/* CBUS Request functions.      */

uint8_t 	SI_CbusIQReqStatusCheck(uint8_t channel);
uint8_t 	SI_CbusOQReqStatusCheck(uint8_t channel);
uint8_t		SI_CbusIQReqStatus ( uint8_t channel );			// IQ Req->reqStatus
uint8_t 	SI_CbusOQReqStatus ( uint8_t channel );		// OQ Req->reqStatus
uint8_t 	SI_CbusIQReqCmd ( uint8_t channel );			// IQ Req->command
uint8_t 	SI_CbusOQReqCmd( uint8_t channel );			// OQ Req->command
uint8_t 	SI_CbusIQReqData ( uint8_t channel );			// IQ Req->Data
uint8_t 	SI_CbusOQReqData0( uint8_t channel );			// OQ Req->msgData[0]
uint8_t		SI_CbusOQReqData1( uint8_t channel );			// OQ Req->msgData[1]
cbus_in_queue_t *SI_CbusIQActiveReq ( uint8_t channel );	// IQ pReq
cbus_out_queue_t *SI_CbusOQActiveReq( uint8_t channel );	// OQ pReq
uint32_t 	SI_CbusIQReqTimeout ( uint8_t channel );		// IQ Req->timeout
uint32_t 	SI_CbusOQReqTimeout( uint8_t channel );		// OQ Req->timeout
uint8_t 	SI_CbusOQReqRetry( uint8_t channel );			// OQ Req->retry
void 		SI_CbusOQReduceReqRetry( uint8_t channel );
void 		SI_CbusIQCleanActiveReq( uint8_t channel );	// Clean IQ Req
void 		SI_CbusOQCleanActiveReq( uint8_t channel );	// Clean OQ Req
bool_t 		SI_CbusOQGetCecReq( uint8_t channel );
void 		SI_CbusOQSetCecReq( uint8_t channel , bool_t value );
void 		SI_CbusOQSetReqStatus( uint8_t channel, uint8_t reqStatus );
void 		SI_CbusOQSetReqTimeout( uint8_t channel, uint32_t value);
uint8_t 	SI_CbusOQReqData2( uint8_t channel );
void 		SI_CbusIQSetReqStatus( uint8_t channel, uint8_t reqStatus );
uint8_t 	SI_CbusPushReqInOQ( uint8_t channel, cbus_out_queue_t *pReq, bool_t vip );	// Push Req into OQ

#ifdef SUPPORT_MSCMSG_IGNORE_LIST
void 		SI_CbusPushKeytoIgnoreList(uint8_t channel, uint8_t keyID, uint8_t direction);
void        SI_CbusUpdateIgnoreListInitStatus(uint8_t channel, uint8_t keyID, uint8_t direction);
bool_t 		SI_RcpIgnoreKeyIDCheck(uint8_t channel, uint8_t keyID, uint8_t direction);
void		SI_CbusResetMscMsgIgnoreList(uint8_t channel, uint8_t direction);
#endif

#ifdef SUPPORT_MSCMSG_IGNORE_LIST
bool_t 		SI_CbusKeyIDCheck(uint8_t channel, uint8_t keyID, uint8_t direction);
#else
bool_t 		SI_CbusKeyIDCheck(uint8_t keyID, uint8_t direction);
#endif

void		SI_CbusCecRetHandler(uint8_t channel, bool_t result);
uint8_t 	SI_CbusMscMsgTimeoutHandler(uint8_t channel, uint8_t direction);
uint8_t		CbusWriteCommand ( int channel, cbus_out_queue_t *pReq  );
uint8_t		CbusProcessFailureInterrupts ( uint8_t channel, uint8_t intStatus, uint8_t inResult );

/* CBUS Channel functions.      */

uint8_t		SI_CbusChannelToPort( uint8_t channel );
uint8_t		SI_CbusChannelStatus( uint8_t channel );
bool_t		SI_CbusChannelConnected( uint8_t channel );
void 		SI_CbusInitParam( uint8_t channel );
bool_t 		SI_CbusGetConnectWait( uint8_t channel );
bool_t 		SI_CbusGetHpdState( uint8_t channel );
bool_t 		SI_CbusGetGrtWrt( uint8_t channel );
void 		SI_CbusSetGrtWrt( uint8_t channel , bool_t value );
bool_t 		SI_CbusGetPathEnable( uint8_t channel );
bool_t 		SI_CbusGetConnectReady( uint8_t channel );
bool_t 		SI_CbusGetAbortWait( uint8_t channel );
bool_t 		SI_CbusGetDcapRdy( uint8_t channel );
uint8_t 	SI_CbusGetReadInfo( uint8_t channel );
void 		SI_CbusSetConnectWait( uint8_t channel , bool_t value );
void 		SI_CbusSetHpdState( uint8_t channel , bool_t value );
void 		SI_CbusSetPathEnable( uint8_t channel , bool_t value );
void 		SI_CbusSetConnectReady( uint8_t channel , bool_t value );
void 		SI_CbusSetAbortWait( uint8_t channel , bool_t value );
void 		SI_CbusSetDcapRdy( uint8_t channel , bool_t value );
void 		SI_CbusSetReadInfo( uint8_t channel , uint8_t value );
void 		SI_CbusSetRcpSupport( uint8_t channel , bool_t value );
void 		SI_CbusSetRapSupport( uint8_t channel , bool_t value );
//uint8_t 		SI_CbusIgnoreListCheck(uint8_t channel, uint8_t keyID, uint8_t direction);

/* RCP Message send functions.  */

uint8_t		SI_CbusMscMsgTransferDone( uint8_t channel );
bool_t		SI_CbusUpdateBusStatus( uint8_t channel );


#endif  // __SI_APICBUS_H__

