////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2008 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// (¡§MStar Confidential Information¡¨) by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef _MXL101SF_H_
#define _MXL101SF_H_

#include "MaxLinearDataTypes.h"

#include <linux/dvb/frontend.h>
#include "../../../media/dvb/dvb-core/dvb_frontend.h"
#include "../aml_fe.h"
#define printf printk

extern void MxL101SF_Init(struct aml_fe_dev *fe);
extern void MxL101SF_Tune(UINT32 u32TunerFreq, UINT8 u8BandWidth, struct aml_fe_dev *fe);
extern UINT32 MxL101SF_GetSNR(struct aml_fe_dev *fe);
extern MXL_BOOL MxL101SF_GetLock(struct aml_fe_dev *fe);
extern UINT32 MxL101SF_GetBER(struct aml_fe_dev *fe);
extern SINT32 MxL101SF_GetSigStrength(struct aml_fe_dev *fe);
extern UINT16 MxL101SF_GetTPSCellID(struct aml_fe_dev *fe);
extern void MxL101SF_PowerOnOff(UINT8 u8PowerOn, struct aml_fe_dev *fe);
extern void Mxl101SF_Debug(struct aml_fe_dev *fe);
#endif

