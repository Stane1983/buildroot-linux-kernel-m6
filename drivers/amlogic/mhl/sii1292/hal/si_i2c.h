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



#ifndef __MHL_SII1292_I2C_H
#define __MHL_SII1292_I2C_H

#include <linux/i2c.h>

#include "../api/si_datatypes.h"

extern struct i2c_adapter *adapter;

void sii_i2c_write_reg(uint8_t address, uint8_t reg, uint8_t val);
int sii_i2c_read_reg(uint8_t address, uint8_t reg);

uint8_t HalI2cReadByte( uint8_t device_id, uint8_t addr );
void HalI2cWriteByte( uint8_t device_id, uint8_t offset, uint8_t value );
bool_t HalI2cReadBlock( uint8_t device_id, uint8_t addr, uint8_t *p_data, uint16_t nbytes );
bool_t HalI2cWriteBlock( uint8_t device_id, uint8_t addr, uint8_t *p_data, uint16_t nbytes );

#endif /* __MHL_SII1292_I2C_H */

