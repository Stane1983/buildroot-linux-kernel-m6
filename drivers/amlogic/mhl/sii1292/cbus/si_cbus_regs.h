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


#ifndef MHL_SII1292_CBUS_REGS_H
#define MHL_SII1292_CBUS_REGS_H

#define REG_CBUS_INTR_STATUS		0xC08
#define BIT_CONNECT_CHG			0x01
#define BIT_CEC_ABORT			0x02    // Responder aborted CEC command at translation layer
#define BIT_DDC_ABORT			0x04    // Responder aborted DDC command at translation layer
#define BIT_MSC_MSG_RCV			0x08    // Responder sent a VS_MSG packet (response data or command.)
#define BIT_MSC_XFR_DONE		0x10    // Responder sent ACK packet (not VS_MSG)
#define BIT_MSC_XFR_ABORT		0x20    // Command send aborted on TX side?????
#define BIT_MSC_ABORT			0x40    // Responder aborted MSC command at translation layer
#define BIT_PARITY_ERROR		0x80	// Link layer parity error interrupt status (normal operation mode parity error count reaches 15)

#define REG_CBUS_INTR_ENABLE		0xC09

#define REG_CBUS_BUS_STATUS		0xC0A
#define BIT_BUS_CONNECTED		0x01

#define REG_CBUS_CEC_ABORT_REASON		0xC0B
#define REG_CBUS_DDC_ABORT_REASON		0xC0C
#define REG_CBUS_REQUESTER_ABORT_REASON		0xC0D

#define REG_CBUS_RESPONDER_ABORT_REASON		0xC0E
#define	CBUSABORT_BIT_REQ_MAXFAIL		(0x01 << 0)
#define	CBUSABORT_BIT_PROTOCOL_ERROR		(0x01 << 1)
#define	CBUSABORT_BIT_REQ_TIMEOUT		(0x01 << 2)
#define	CBUSABORT_BIT_UNDEFINED_OPCODE		(0x01 << 3)
#define	CBUSABORT_BIT_PEER_ABORTED		(0x01 << 7)

#define REG_CBUS_CEC_LADDR_L			0xC10
#define REG_CBUS_CEC_LADDR_H			0xC11

#define REG_CBUS_PRI_START			0xC12
#define BIT_TRANSFER_PVT_CMD			0x01
#define BIT_SEND_MSC_MSG			0x02
#define	MSC_START_BIT_MSC_CMD			(0x01 << 0)
#define	MSC_START_BIT_VS_CMD			(0x01 << 1)
#define	MSC_START_BIT_READ_REG			(0x01 << 2)
#define	MSC_START_BIT_WRITE_REG			(0x01 << 3)
#define	MSC_START_BIT_WRITE_BURST		(0x01 << 4)

#define REG_CBUS_PRI_ADDR_CMD			0xC13
#define REG_CBUS_PRI_WR_DATA_1ST		0xC14
#define REG_CBUS_PRI_WR_DATA_2ND		0xC15
#define REG_CBUS_PRI_RD_DATA_1ST		0xC16

#define REG_CBUS_PRI_VS_CMD			0xC18
#define REG_CBUS_PRI_VS_DATA			0xC19

#define REG_CBUS_WRITE_BURST_LEN		0xC20
#define BIT_MSC_DONE_WITH_NACK			(0x01 << 6)

#define REG_CBUS_RETRY_INTERVAL			0xC1A
#define REG_CBUS_CEC_FAIL_LIMIT			0xC1B
#define REG_CBUS_DDC_FAIL_LIMIT			0xC1C
#define REG_CBUS_MSC_FAIL_LIMIT			0xC1D

#define	REG_CBUS_MSC_INT2_STATUS		0xC1E
#define	BIT_MSC_WRITE_BURST			(0x01 << 0)	// Write burst received
#define	BIT_MSC_INT2_HEARTBEAT_MAXFAIL		(0x01 << 1)	// Retry threshold exceeded for sending the Heartbeat
#define BIT_MSC_MR_SET_INT			(0x01 << 2)	// MSC responder interrupt posted by the peer
#define BIT_MSC_MR_WRITE_STATE			(0x01 << 3)	// MSC responder status changed by the peer
#define BIT_MSC_PEER_STATE_CHG			(0x01 << 4) // MSC received peer state changed

#define REG_MSC_INT2_ENABLE			0xC1F
#define	REG_MSC_WRITE_BURST_LEN			0xC20       // only for WRITE_BURST

#define	REG_MSC_HEARTBEAT_CONTROL		0xC21       // Periodic heart beat. TX sends GET_REV_ID MSC command
#define	MSC_HEARTBEAT_PERIOD_MASK		0x0F	// bits 3..0
#define	MSC_HEARTBEAT_FAIL_LIMIT_MASK		0x70	// bits 6..4
#define	MSC_HEARTBEAT_ENABLE			0x80	// bit 7

#define REG_MSC_TIMEOUT_LIMIT			0xC22
#define	MSC_TIMEOUT_LIMIT_MSB_MASK		(0x0F)	        // default is 1
#define	MSC_LEGACY_BIT				(0x01 << 7)	    // This should be cleared.

#define REG_CBUS_MSC_CTRL			0xC2E
#define BIT_DISABLE_PING			(0x01 << 0)	// Disable Ping command
#define BIT_DISABLE_CAP_ID_COMMANDS		(0x01 << 1) // Disable SET_CAP_ID (SET_CEC_CAP_ID) and GET_CAP_ID (GET_CEC_CAP_ID)
#define BIT_DISABLE_GET_VS_ABORT		(0x01 << 2) // Disable GET_VS_ABORT (GET_CEC_ABORT)
#define BIT_DISABLE_GET_DDC_ABORT		(0x01 << 3) // Disable GET_DDC_ABORT
#define BIT_DISABLE_CEC				(0x01 << 4) // Disable CEC on CBUS

#define REG_CBUS_LINK_LAYER_CTRL4		0xC32
#define REG_CBUS_LINK_LAYER_CTRL5		0xC33
#define REG_CBUS_LINK_LAYER_CTRL6		0xC34
#define REG_CBUS_LINK_LAYER_CTRL7		0xC35
#define REG_CBUS_LINK_STATUS_1			0xC37
#define REG_CBUS_LINK_STATUS_2			0xC38
#define REG_CBUS_LINK_LAYER_CTRL9		0xC39
#define REG_CBUS_LINK_LAYER_CTRL10		0xC3A
#define REG_CBUS_LINK_LAYER_CTRL11		0xC3B
#define REG_CBUS_LINK_LAYER_CTRL12		0xC3C

#define REG_CBUS_INT_REG_0			0xCA0
#define CBUS_INT_BIT_DCAP_CHG			0x01
#define CBUS_INT_BIT_DSCR_CHG			0x02
#define CBUS_INT_BIT_REQ_WRT			0x04
#define CBUS_INT_BIT_GRT_WRT			0x08


#define REG_CBUS_INT_REG_1			0xCA1
#define CBUS_INT1_BIT_EDID_CHG			0x01

#define REG_CBUS_STAT_REG_0			0xCB0
#define BIT_DCAP_RDY				0x01

#define REG_CBUS_STAT_REG_1			0xCB1
#define BIT_PATH_EN				0x08


#define REG_CBUS_SCRATCHPAD_0			0xCC0

#define REG_CBUS_DEVICE_CAP_0			0xC80
#define REG_CBUS_DEVICE_CAP_2			0xC82
#define BIT_POW					(0x01 << 4)	// Power output bit

#endif /* MHL_SII1292_CBUS_REGS_H */

