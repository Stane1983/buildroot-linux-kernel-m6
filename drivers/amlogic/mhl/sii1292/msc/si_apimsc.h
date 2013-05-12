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

#ifndef __MHL_SII1292_API_MSC_H
#define __MHL_SII1292_API_MSC_H

#include <linux/types.h>

#include "../api/si_datatypes.h"

bool_t  SI_MscClrHpd(uint8_t channel, uint8_t uData);
bool_t  SI_MscSetHpd(uint8_t channel, uint8_t uData);
bool_t 	SI_MscGrtWrt(uint8_t channel, uint8_t uData);
bool_t  SI_MscPathEnable(uint8_t channel, uint8_t uData);
bool_t  SI_MscPathDisable(uint8_t channel, uint8_t uData);
bool_t  SI_MscConnectReady(uint8_t channel, uint8_t uData);
bool_t  SI_MscReadBandwidth(uint8_t channel, uint8_t uData);
bool_t  SI_MscReadVideoType(uint8_t channel, uint8_t uData);
bool_t  SI_MscReadAudioLinkMode(uint8_t channel, uint8_t uData);
bool_t  SI_MscReadVideoLinkMode(uint8_t channel, uint8_t uData);
bool_t  SI_MscReadLogDevMap(uint8_t channel, uint8_t uData);
bool_t  SI_MscReadDevCat(uint8_t channel, uint8_t uData);
bool_t  SI_MscReadMhlVersion(uint8_t channel, uint8_t uData);
bool_t  SI_MscReadFeatureFlag(uint8_t channel, uint8_t uData);
void 	SI_MscStartGetDevInfo(uint8_t channel);
bool_t  SI_WriteBurstDataDone(uint8_t channel, uint8_t uData);
void 	SI_MscInitialize(void);

#endif /* __MHL_SII1292_API_MSC_H */

