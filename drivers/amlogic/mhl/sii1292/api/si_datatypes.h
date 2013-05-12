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



#ifndef MHL_SII1292_DATA_TYPES_H
#define MHL_SII1292_DATA_TYPES_H

#include <linux/types.h>
#include <linux/kernel.h>

#if 0
typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long  uint32_t;

typedef signed char    int8_t;
typedef signed short   int16_t;
typedef signed long    int32_t;
#endif

#if 0
typedef enum
{
	false   = 0,
	true    = !(false)
} bool_t;
#endif
typedef bool bool_t;

typedef char BOOL;

#define	SUCCESS		0

//------------------------------------------------------------------------------
// Configuration defines used by hal_config.h
//------------------------------------------------------------------------------

#define ENABLE		(0xFF)
#define DISABLE		(0x00)

#define BIT0		(0x01)
#define BIT1		(0x02)
#define BIT2		(0x04)
#define BIT3		(0x08)
#define BIT4		(0x10)
#define BIT5		(0x20)
#define BIT6		(0x40)
#define BIT7		(0x80)

#endif /* MHL_SII1292_DATA_TYPES_H */
