/*
 * TVIN Demod Device Driver
 *
 * Author: Bobby Yang <bo.yang@amlogic.com>
 *
 *
 * Copyright (C) 2010 Amlogic Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#ifndef __TVIN_DEMOD_H
#define __TVIN_DEMOD_H

/* Standard Liniux Headers */
#include <linux/i2c.h>

/* Amlogic Headers */
#include <linux/tvin/tvin.h>

#define I2C_TRY_MAX_CNT       3  //max try counter


extern int  tvin_set_demod(unsigned int bce);
extern void tvin_demod_set_std(tuner_std_id ptstd);
extern void tvin_demod_get_afc(struct tuner_parm_s *ptp);

#endif //__TVIN_DEMOD_H

