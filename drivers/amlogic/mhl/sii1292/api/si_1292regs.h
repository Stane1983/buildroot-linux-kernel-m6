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


#ifndef __SI_1292REGS_H__
#define __SI_1292REGS_H__


#define SET_BITS    0xFF
#define CLEAR_BITS  0x00

//------------------------------------------------------------------------------
// NOTE: Register addresses are 16 bit values with page and offset combined.
//
// Examples:  0x005 = page 0, offset 0x05
//            0x1B6 = page 1, offset 0xB6
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Registers in Page 0  (0xD0)
//------------------------------------------------------------------------------

#define REG_DEV_IDL_RX		0x002
#define REG_DEV_IDH_RX		0x003
#define REG_DEV_REV		0x004



// Software Reset Register
#define REG_SRST		0x005
#define BIT_SWRST_AUTO		0x10    // Auto SW Reset
#define BIT_SWRST		0x01
#define BIT_CBUS_RST		0x04
#define BIT_CEC_RST		0x08


// System Status Register
#define REG_STATE		0x006
#define BIT_SCDT		0x01
#define BIT_CKDT		0x02
#define BIT_HPD			0x04
#define BIT_PWR5V		0x08
#define BIT_V5DET		0x10
#define BIT_MHL_STRAP		0x20

// System Status Register 2
#define REG_STATE2		0x007

// System Control 1 Register
#define REG_SYS_CTRL1		0x008
#define BIT_RX_CLK		0x02
#define BIT_HDMI_MODE		0x80

// Interrupt State Register
#define REG_INTR_STATE		0x043
#define BIT_INTR		0x01

// Interrupt Status Register
#define REG_INTR1		0x04C
#define INT_CBUS		0x01
#define INT_RPWR		0x02
#define INT_HPD			0x08
#define INT_CKDT		0x10
#define INT_RSEN		0x20
#define INT_SCDT		0x40
#define INT_CBUS_CON		0x80

// Interrupt Mask
#define REG_INTR1_MASK		0x04D

//Interrupt 2 Status and Mask Register
#define REG_INTR2_STATUS_MASK	0x04E
#define INT_V5DET		0x01
#define MASK_V5DET		0x02

// TMDS Tx Control Register
#define REG_TMDS_Tx		0x047

// TMDS Tx SW Control 1 Register
#define REG_TX_SWING1		0x051
#define BIT_SWCTRL_EN		0x80

// Rx Control5 Register
#define REG_RX_CTRL5		0x070
#define BIT_HDMI_RX_EN		0x80

// CBUS DISC OVR Register
#define REG_CBUS_DISC_OVR	0x073
#define BIT_STRAP_MHL_EN	0x01
#define BIT_STRAP_MHL_OVR	0x02

// Rx MISC Register
#define REG_RX_MISC		0x07A
#define BIT_HPD_RSEN_ENABLE	0x04
#define BIT_5VDET_OVR		0x08
#define BIT_PSCTRL_OEN		0x10
#define BIT_PSCTRL_OUT		0x20
#define BIT_PSCTRL_AUTO		0x80

// GPIO CTRL Register
#define REG_GPIO_CTRL		0x07F
#define BIT_GPIO0_OUTPUT_DISABLE	0x01
#define BIT_GPIO0_INPUT		0x40

// DDC Switch Register
#define REG_DDC_SW_CTRL		0x084
#define BIT_I2C_DDC_SW_EN	0x01

// OTP Revision Register
#define REG_OTP_REV		0x086
#define MASK_OTP_REV		0x0F

// DDC
#define REG_DDC_L1		0x984
// Jin: REG_DDC_PAH = REG_DDC_L1+ 10 + L1 + L2
// REG_DDC_L2 = REG_DDC_L1 + 1 + L1


#endif  // __SI_1292REGS_H__


