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

#ifndef __SI_CBUS_DEFS_H__
#define	__SI_CBUS_DEFS_H__

// Version that this chip supports
#define	MHL_VER_MAJOR		(0x01 << 4)	// bits 4..7
#define	MHL_VER_MINOR		0x00		// bits 0..3
#define	MHL_VERSION		(MHL_VER_MAJOR | MHL_VER_MINOR)

#define	CBUS_VER_MAJOR		(0x01 << 4)	// bits 4..7
#define	CBUS_VER_MINOR		0x00		// bits 0..3
#define MHL_CBUS_VERSION	(CBUS_VER_MAJOR | CBUS_VER_MINOR)

#define	MHL_DEV_CAT_SINK	0x01
#define	MHL_DEV_CAT_SOURCE	0x02
#define	MHL_DEV_CAT_DONGLE	0x03

#define	MHL_POWER_SUPPLY_CAPACITY		16		// 160 mA current
#define	MHL_POWER_SUPPLY_PROVIDED		16		// 160mA 0r 0 for Wolverine.
#define	MHL_VIDEO_LINK_MODE_SUPORT		SUPP_RGB444|SUPP_YCBCR444|SUPP_YCBCR422|SUPP_ISLANDS|SUPP_VGA
#define	MHL_AUDIO_LINK_MODE_SUPORT		AUD_2CH
#define	MHL_VIDEO_TYPE_SUPPORT			0

#define	MHL_BANDWIDTH_LIMIT				0x0F		// 75 MHz

#define	MHL_DEVCAP_SIZE					16
#define	MHL_INTERRUPT_SIZE				3
#define	MHL_STATUS_SIZE					3
#define MHL_INT_STATUS_SIZE				((MHL_INTERRUPT_SIZE<<4)|MHL_STATUS_SIZE)
#define MHL_SCRATCHPAD_SIZE				16
#define	MHL_MAX_BUFFER_SIZE				MHL_SCRATCHPAD_SIZE	// manually define highest number

#define MHL_RCP_SUPPORT					(0x01 << 0)
#define MHL_RAP_SUPPORT					(0x01 << 1)
#define MHL_SP_SUPPORT					(0x01 << 2)
#define	MHL_FEATURE_FLAG				(MHL_RCP_SUPPORT|MHL_RAP_SUPPORT|MHL_SP_SUPPORT)

#define MHL_DEVICE_ID_H					0x92
#define MHL_DEVICE_ID_L					0x31

#define MHL_REG_CHANGE_INT_OFFSET			0x20

#define	MHL_DEVICE_STATUS_CONNECTED_RDY_REG_OFFSET	0x30
#define	DCAP_RDY					0x01
#define	DSCR_RDY					0x02

#define	MHL_DEVICE_STATUS_LINK_MODE_REG_OFFSET		0x31
#define	PATH_EN						0x08

#define	MHL_DEV_CAP_DEV_STATE_REG_OFFSET		0x00

#define	MHL_DEV_CAP_MHL_VERSION_REG_OFFSET		0x01

#define	MHL_DEV_CAP_DEV_CAT_REG_OFFSET			0x02
#define	SOURCE_DEV					0x00
#define	SINGLE_INPUT_SINK				0x01
#define	MULTIPLE_INPUT_SINK				0x02
#define	UNPOWERED_DONGLE				0x03
#define	SELF_POWERED_DONGLE				0x04
#define	HDCP_REPEATER					0x05
#define	NON_DISPLAY_SINK				0x06
#define	POWER_NEUTRAL_SINK				0x07
#define	OTHER_DEV_CAT					0x80

#define	MHL_DEV_CAP_ADOPTER_IDH_REG_OFFSET		0x03

#define	MHL_DEV_CAP_ADOPTER_IDL_REG_OFFSET		0x04

#define	MHL_DEV_CAP_VID_LINK_MODE_REG_OFFSET		0x05
#define	SUPP_RGB444					(0x01 << 0)
#define	SUPP_YCBCR444					(0x01 << 1)
#define	SUPP_YCBCR422					(0x01 << 2)
#define	SUPP_PPIXEL					(0x01 << 3)
#define	SUPP_ISLANDS					(0x01 << 4)
#define SUPP_VGA					(0x01 << 5)

#define	MHL_DEV_CAP_AUD_LINK_MODE_REG_OFFSET		0x06
#define	AUD_2CH						(0x01 << 0)
#define	AUD_8CH						(0x01 << 1)

#define	MHL_DEV_CAP_VIDEO_TYPE_REG_OFFSET		0x07
#define	VT_GRAPHICS					(0x01 << 0)
#define	VT_PHOTO					(0x01 << 1)
#define	VT_CINEMA					(0x01 << 2)
#define	VT_GAME						(0x01 << 3)
#define	SUPP_VT						(0x01 << 7)


#define	MHL_DEV_CAP_LOG_DEV_MAP_REG_OFFSET		0x08
#define	LD_DISPLAY					(0x01 << 0)
#define	LD_VIDEO					(0x01 << 1)
#define	LD_AUDIO					(0x01 << 2)
#define	LD_MEDIA					(0x01 << 3)
#define	LD_TUNER					(0x01 << 4)
#define	LD_RECORD					(0x01 << 5)
#define	LD_SPEAKER					(0x01 << 6)
#define	LD_GUI						(0x01 << 7)

#define	MHL_DEV_CAP_BANDWIDTH_REG_OFFSET		0x09

#define	MHL_DEV_CAP_FEATURE_FLAG_REG_OFFSET		0x0A
#define	RCP_SUPPORT					(0x01 << 0)
#define	RAP_SUPPORT					(0x01 << 1)



//------------------------------------------------------------------------------
//
// MHL Specs defined registers in device capability set
//
//
typedef struct {
	unsigned char	mhl_devcap_version;			// 0x00
	unsigned char	mhl_devcap_cbus_version;		// 0x01
	unsigned char	mhl_devcap_device_category;		// 0x02
	unsigned char	mhl_devcap_power_supply_capacity;	// 0x03
   	unsigned char	mhl_devcap_power_supply_provided;	// 0x04
   	unsigned char	mhl_devcap_video_link_mode_support;	// 0x05
   	unsigned char	mhl_devcap_audio_link_mode_support;	// 0x06
   	unsigned char	mhl_devcap_hdcp_status;			// 0x07
   	unsigned char	mhl_devcap_logical_device_map;		// 0x08
   	unsigned char	mhl_devcap_link_bandwidth_limit;	// 0x09
   	unsigned char	mhl_devcap_reserved_1;			// 0x0a
   	unsigned char	mhl_devcap_reserved_2;			// 0x0b
   	unsigned char	mhl_devcap_reserved_3;			// 0x0c
   	unsigned char	mhl_devcap_scratchpad_size;		// 0x0d
   	unsigned char	mhl_devcap_interrupt_size;		// 0x0e
   	unsigned char	mhl_devcap_devcap_size;			// 0x0f

} mhl_devcap_t;
//------------------------------------------------------------------------------
//
// MHL Specs defined registers for interrupts
//
//
typedef struct {

	unsigned char	mhl_intr_0;		// 0x00
	unsigned char	mhl_intr_1;		// 0x01
	unsigned char	mhl_intr_2;		// 0x02
	unsigned char	mhl_intr_3;		// 0x03

} mhl_interrupt_t;
//------------------------------------------------------------------------------
//
// MHL Specs defined registers for status
//
//
typedef struct {

	unsigned char	mhl_status_0;	// 0x00
	unsigned char	mhl_status_1;	// 0x01
	unsigned char	mhl_status_2;	// 0x02
	unsigned char	mhl_status_3;	// 0x03

} mhl_status_t;
//------------------------------------------------------------------------------
//
// MHL Specs defined registers for local scratchpad registers
//
//
typedef struct {

	unsigned char	mhl_scratchpad[16];

} mhl_scratchpad_t;
//------------------------------------------------------------------------------

//------- END OF DEFINES -------------
#endif	// __SI_CBUS_DEFS_H__
