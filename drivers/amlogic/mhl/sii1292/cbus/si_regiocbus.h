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


#ifndef __MHL_SII1292_REGIOCBUS_H
#define __MHL_SII1292_REGIOCBUS_H

#include "../api/si_datatypes.h"


uint8_t SiIRegioCbusRead ( uint16_t regAddr, uint8_t channel );
void SiIRegioCbusWrite ( uint16_t regAddr, uint8_t channel, uint8_t value );
void SiIRegioCbusModify ( uint16_t regAddr, uint8_t channel, uint8_t mask, uint8_t value );

#endif /* __MHL_SII1292_REGIOCBUS_H */

