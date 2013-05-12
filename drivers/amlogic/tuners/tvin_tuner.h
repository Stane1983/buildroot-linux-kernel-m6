/*
 * TVIN Tuner Device Driver
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


#ifndef __TVIN_TUNER_H
#define __TVIN_TUNER_H


/* Standard Liniux Headers */
#include <linux/i2c.h>

/* Amlogic Headers */
#include <linux/tvin/tvin.h>

#define I2C_TRY_MAX_CNT       3  //max try counter

extern int  tvin_set_tuner(void);
extern void tvin_get_tuner(struct tuner_parm_s *ptp);
extern void tvin_tuner_get_std(tuner_std_id *ptstd);
extern void tvin_tuner_set_std(tuner_std_id ptstd);
extern void tvin_tuner_get_freq(struct tuner_freq_s *ptf);
extern void tvin_tuner_set_freq(struct tuner_freq_s tf);
extern char *tvin_tuenr_get_name(void);
#ifdef CONFIG_TVIN_TUNER_SI2176
extern void si2176_get_tuner_status(struct si2176_tuner_status_s *si2176_s);
#endif

#endif //__TVIN_TUNER_H


