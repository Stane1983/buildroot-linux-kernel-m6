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


#ifndef __MHL_SII1292_CBUS_H
#define	__MHL_SII1292_CBUS_H

//------------------------------------------------------------------------------
//  1292 definitions
//------------------------------------------------------------------------------
//
// Where specific CBUS registers are located
//


#define CBUS_I2C_BUS_ADDRESS        (0xC8)  // I2C slave address for CBUS

#define CBUS_DEVCAP_BASE            (0x80)
#define CBUS_INTR_BASE              (0xA0)
#define CBUS_STAT_BASE              (0xB0)
#define CBUS_SCRATCHPAD_BASE        (0xC0)

// Chip specific MHL parameters.

#define	MHL_DEVICE_CATEGORY		(MHL_DEV_CAT_SINK)

#define	MHL_LOGICAL_DEVICE		(LD_DISPLAY | LD_SPEAKER)

#endif  // __MHL_SII1292_CBUS_H

