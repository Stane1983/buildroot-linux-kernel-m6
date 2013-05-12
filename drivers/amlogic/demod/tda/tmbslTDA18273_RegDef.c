/*
  Copyright (C) 2010 NXP B.V., All Rights Reserved.
  This source code and any compilation or derivative thereof is the proprietary
  information of NXP B.V. and is confidential in nature. Under no circumstances
  is this software to be  exposed to or placed under an Open Source License of
  any type without the expressed written permission of NXP B.V.
 *
 * \file          tmbslTDA18273_RegDef.c
 *
 *                %version: 3 %
 *
 * \date          %modify_time%
 *
 * \author        Christophe CAZETTES
 *
 * \brief         Describe briefly the purpose of this file.
 *
 * REFERENCE DOCUMENTS :
 *                TDA18273_Driver_User_Guide.pdf
 *
 * TVFE SW Arch V4 Template: Author Christophe CAZETTES
 *
 * \section info Change Information
 *
*/

/* File generated automatically from register description file */


/*============================================================================*/
/* Standard include files:                                                    */
/*============================================================================*/
#include "tmNxTypes.h"
#include "tmCompId.h"
#include "tmFrontEnd.h"
#include "tmbslFrontEndTypes.h"

/*============================================================================*/
/* Project include files:                                                     */
/*============================================================================*/
#include "tmbslTDA18273.h"
#include "tmbslTDA18273_RegDef.h"

/*============================================================================*/
/* Global data:                                                               */
/*============================================================================*/

/* TDA18273 Register ID_byte_1 0x00 */
const TDA18273_BitField_t gTDA18273_Reg_ID_byte_1 = { 0x00, 0x00, 0x08, 0x00 };
/* MS bit(s): Indicate if Device is a Master or a Slave */
/*  1 => Master device */
/*  0 => Slave device */
const TDA18273_BitField_t gTDA18273_Reg_ID_byte_1__MS = { 0x00, 0x07, 0x01, 0x00 };
/* Ident_1 bit(s): MSB of device identifier */
const TDA18273_BitField_t gTDA18273_Reg_ID_byte_1__Ident_1 = { 0x00, 0x00, 0x07, 0x00 };


/* TDA18273 Register ID_byte_2 0x01 */
const TDA18273_BitField_t gTDA18273_Reg_ID_byte_2 = { 0x01, 0x00, 0x08, 0x00 };
/* Ident_2 bit(s): LSB of device identifier */
const TDA18273_BitField_t gTDA18273_Reg_ID_byte_2__Ident_2 = { 0x01, 0x00, 0x08, 0x00 };


/* TDA18273 Register ID_byte_3 0x02 */
const TDA18273_BitField_t gTDA18273_Reg_ID_byte_3 = { 0x02, 0x00, 0x08, 0x00 };
/* Major_rev bit(s): Major revision of device */
const TDA18273_BitField_t gTDA18273_Reg_ID_byte_3__Major_rev = { 0x02, 0x04, 0x04, 0x00 };
/* Major_rev bit(s): Minor revision of device */
const TDA18273_BitField_t gTDA18273_Reg_ID_byte_3__Minor_rev = { 0x02, 0x00, 0x04, 0x00 };


/* TDA18273 Register Thermo_byte_1 0x03 */
const TDA18273_BitField_t gTDA18273_Reg_Thermo_byte_1 = { 0x03, 0x00, 0x08, 0x00 };
/* TM_D bit(s): Device temperature */
const TDA18273_BitField_t gTDA18273_Reg_Thermo_byte_1__TM_D = { 0x03, 0x00, 0x07, 0x00 };


/* TDA18273 Register Thermo_byte_2 0x04 */
const TDA18273_BitField_t gTDA18273_Reg_Thermo_byte_2 = { 0x04, 0x00, 0x08, 0x00 };
/* TM_ON bit(s): Set device temperature measurement to ON or OFF */
/*  1 => Temperature measurement ON */
/*  0 => Temperature measurement OFF */
const TDA18273_BitField_t gTDA18273_Reg_Thermo_byte_2__TM_ON = { 0x04, 0x00, 0x01, 0x00 };


/* TDA18273 Register Power_state_byte_1 0x05 */
const TDA18273_BitField_t gTDA18273_Reg_Power_state_byte_1 = { 0x05, 0x00, 0x08, 0x00 };
/* POR bit(s): Indicates that device just powered ON */
/*  1 => POR: No access done to device */
/*  0 => At least one access has been done to device */
const TDA18273_BitField_t gTDA18273_Reg_Power_state_byte_1__POR = { 0x05, 0x07, 0x01, 0x00 };
/* AGCs_Lock bit(s): Indicates that AGCs are locked */
/*  1 => AGCs locked */
/*  0 => AGCs not locked */
const TDA18273_BitField_t gTDA18273_Reg_Power_state_byte_1__AGCs_Lock = { 0x05, 0x02, 0x01, 0x00 };
/* Vsync_Lock bit(s): Indicates that VSync is locked */
/*  1 => VSync locked */
/*  0 => VSync not locked */
const TDA18273_BitField_t gTDA18273_Reg_Power_state_byte_1__Vsync_Lock = { 0x05, 0x01, 0x01, 0x00 };
/* LO_Lock bit(s): Indicates that LO is locked */
/*  1 => LO locked */
/*  0 => LO not locked */
const TDA18273_BitField_t gTDA18273_Reg_Power_state_byte_1__LO_Lock = { 0x05, 0x00, 0x01, 0x00 };


/* TDA18273 Register Power_state_byte_2 0x06 */
const TDA18273_BitField_t gTDA18273_Reg_Power_state_byte_2 = { 0x06, 0x00, 0x08, 0x00 };
/* SM bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_Power_state_byte_2__SM = { 0x06, 0x01, 0x01, 0x00 };
/* SM_XT bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_Power_state_byte_2__SM_XT = { 0x06, 0x00, 0x01, 0x00 };


/* TDA18273 Register Input_Power_Level_byte 0x07 */
const TDA18273_BitField_t gTDA18273_Reg_Input_Power_Level_byte = { 0x07, 0x00, 0x08, 0x00 };
/* Power_Level bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_Input_Power_Level_byte__Power_Level = { 0x07, 0x00, 0x08, 0x00 };


/* TDA18273 Register IRQ_status 0x08 */
const TDA18273_BitField_t gTDA18273_Reg_IRQ_status = { 0x08, 0x00, 0x08, 0x00 };
/* IRQ_status bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_IRQ_status__IRQ_status = { 0x08, 0x07, 0x01, 0x00 };
/* MSM_XtalCal_End bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_IRQ_status__MSM_XtalCal_End = { 0x08, 0x05, 0x01, 0x00 };
/* MSM_RSSI_End bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_IRQ_status__MSM_RSSI_End = { 0x08, 0x04, 0x01, 0x00 };
/* MSM_LOCalc_End bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_IRQ_status__MSM_LOCalc_End = { 0x08, 0x03, 0x01, 0x00 };
/* MSM_RFCal_End bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_IRQ_status__MSM_RFCal_End = { 0x08, 0x02, 0x01, 0x00 };
/* MSM_IRCAL_End bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_IRQ_status__MSM_IRCAL_End = { 0x08, 0x01, 0x01, 0x00 };
/* MSM_RCCal_End bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_IRQ_status__MSM_RCCal_End = { 0x08, 0x00, 0x01, 0x00 };


/* TDA18273 Register IRQ_enable 0x09 */
const TDA18273_BitField_t gTDA18273_Reg_IRQ_enable = { 0x09, 0x00, 0x08, 0x00 };
/* IRQ_Enable bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_IRQ_enable__IRQ_Enable = { 0x09, 0x07, 0x01, 0x00 };
/* MSM_XtalCal_Enable bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_IRQ_enable__MSM_XtalCal_Enable = { 0x09, 0x05, 0x01, 0x00 };
/* MSM_RSSI_Enable bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_IRQ_enable__MSM_RSSI_Enable = { 0x09, 0x04, 0x01, 0x00 };
/* MSM_LOCalc_Enable bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_IRQ_enable__MSM_LOCalc_Enable = { 0x09, 0x03, 0x01, 0x00 };
/* MSM_RFCal_Enable bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_IRQ_enable__MSM_RFCal_Enable = { 0x09, 0x02, 0x01, 0x00 };
/* MSM_IRCAL_Enable bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_IRQ_enable__MSM_IRCAL_Enable = { 0x09, 0x01, 0x01, 0x00 };
/* MSM_RCCal_Enable bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_IRQ_enable__MSM_RCCal_Enable = { 0x09, 0x00, 0x01, 0x00 };


/* TDA18273 Register IRQ_clear 0x0A */
const TDA18273_BitField_t gTDA18273_Reg_IRQ_clear = { 0x0A, 0x00, 0x08, 0x00 };
/* IRQ_Clear bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_IRQ_clear__IRQ_Clear = { 0x0A, 0x07, 0x01, 0x00 };
/* MSM_XtalCal_Clear bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_IRQ_clear__MSM_XtalCal_Clear = { 0x0A, 0x05, 0x01, 0x00 };
/* MSM_RSSI_Clear bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_IRQ_clear__MSM_RSSI_Clear = { 0x0A, 0x04, 0x01, 0x00 };
/* MSM_LOCalc_Clear bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_IRQ_clear__MSM_LOCalc_Clear = { 0x0A, 0x03, 0x01, 0x00 };
/* MSM_RFCal_Clear bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_IRQ_clear__MSM_RFCal_Clear = { 0x0A, 0x02, 0x01, 0x00 };
/* MSM_IRCAL_Clear bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_IRQ_clear__MSM_IRCAL_Clear = { 0x0A, 0x01, 0x01, 0x00 };
/* MSM_RCCal_Clear bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_IRQ_clear__MSM_RCCal_Clear = { 0x0A, 0x00, 0x01, 0x00 };


/* TDA18273 Register IRQ_set 0x0B */
const TDA18273_BitField_t gTDA18273_Reg_IRQ_set = { 0x0B, 0x00, 0x08, 0x00 };
/* IRQ_Set bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_IRQ_set__IRQ_Set = { 0x0B, 0x07, 0x01, 0x00 };
/* MSM_XtalCal_Set bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_IRQ_set__MSM_XtalCal_Set = { 0x0B, 0x05, 0x01, 0x00 };
/* MSM_RSSI_Set bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_IRQ_set__MSM_RSSI_Set = { 0x0B, 0x04, 0x01, 0x00 };
/* MSM_LOCalc_Set bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_IRQ_set__MSM_LOCalc_Set = { 0x0B, 0x03, 0x01, 0x00 };
/* MSM_RFCal_Set bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_IRQ_set__MSM_RFCal_Set = { 0x0B, 0x02, 0x01, 0x00 };
/* MSM_IRCAL_Set bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_IRQ_set__MSM_IRCAL_Set = { 0x0B, 0x01, 0x01, 0x00 };
/* MSM_RCCal_Set bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_IRQ_set__MSM_RCCal_Set = { 0x0B, 0x00, 0x01, 0x00 };


/* TDA18273 Register AGC1_byte_1 0x0C */
const TDA18273_BitField_t gTDA18273_Reg_AGC1_byte_1 = { 0x0C, 0x00, 0x08, 0x00 };
/* AGC1_TOP bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_AGC1_byte_1__AGC1_TOP = { 0x0C, 0x00, 0x04, 0x00 };


/* TDA18273 Register AGC1_byte_2 0x0D */
const TDA18273_BitField_t gTDA18273_Reg_AGC1_byte_2 = { 0x0D, 0x00, 0x08, 0x00 };
/* AGC1_Top_Mode_Val bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_AGC1_byte_2__AGC1_Top_Mode_Val = { 0x0D, 0x03, 0x02, 0x00 };
/* AGC1_Top_Mode bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_AGC1_byte_2__AGC1_Top_Mode = { 0x0D, 0x00, 0x03, 0x00 };


/* TDA18273 Register AGC2_byte_1 0x0E */
const TDA18273_BitField_t gTDA18273_Reg_AGC2_byte_1 = { 0x0E, 0x00, 0x08, 0x00 };
/* AGC2_TOP bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_AGC2_byte_1__AGC2_TOP = { 0x0E, 0x00, 0x03, 0x00 };


/* TDA18273 Register AGCK_byte_1 0x0F */
const TDA18273_BitField_t gTDA18273_Reg_AGCK_byte_1 = { 0x0F, 0x00, 0x08, 0x00 };
/* AGCs_Up_Step_assym bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_AGCK_byte_1__AGCs_Up_Step_assym = { 0x0F, 0x06, 0x02, 0x00 };
/* Pulse_Shaper_Disable bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_AGCK_byte_1__Pulse_Shaper_Disable = { 0x0F, 0x04, 0x01, 0x00 };
/* AGCK_Step bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_AGCK_byte_1__AGCK_Step = { 0x0F, 0x02, 0x02, 0x00 };
/* AGCK_Mode bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_AGCK_byte_1__AGCK_Mode = { 0x0F, 0x00, 0x02, 0x00 };


/* TDA18273 Register RF_AGC_byte 0x10 */
const TDA18273_BitField_t gTDA18273_Reg_RF_AGC_byte = { 0x10, 0x00, 0x08, 0x00 };
/* PD_AGC_Adapt3x bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RF_AGC_byte__PD_AGC_Adapt3x = { 0x10, 0x06, 0x02, 0x00 };
/* RFAGC_Adapt_TOP bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RF_AGC_byte__RFAGC_Adapt_TOP = { 0x10, 0x04, 0x02, 0x00 };
/* RFAGC_Low_BW bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RF_AGC_byte__RFAGC_Low_BW = { 0x10, 0x03, 0x01, 0x00 };
/* RFAGC_Top bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RF_AGC_byte__RFAGC_Top = { 0x10, 0x00, 0x03, 0x00 };


/* TDA18273 Register W_Filter_byte 0x11 */
const TDA18273_BitField_t gTDA18273_Reg_W_Filter_byte = { 0x11, 0x00, 0x08, 0x00 };
/* VHF_III_mode bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_W_Filter_byte__VHF_III_mode = { 0x11, 0x07, 0x01, 0x00 };
/* RF_Atten_3dB bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_W_Filter_byte__RF_Atten_3dB = { 0x11, 0x06, 0x01, 0x00 };
/* W_Filter_Enable bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_W_Filter_byte__W_Filter_Enable = { 0x11, 0x05, 0x01, 0x00 };
/* W_Filter_Bypass bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_W_Filter_byte__W_Filter_Bypass = { 0x11, 0x04, 0x01, 0x00 };
/* W_Filter bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_W_Filter_byte__W_Filter = { 0x11, 0x02, 0x02, 0x00 };
/* W_Filter_Offset bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_W_Filter_byte__W_Filter_Offset = { 0x11, 0x00, 0x02, 0x00 };


/* TDA18273 Register IR_Mixer_byte_1 0x12 */
const TDA18273_BitField_t gTDA18273_Reg_IR_Mixer_byte_1 = { 0x12, 0x00, 0x08, 0x00 };
/* S2D_Gain bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_IR_Mixer_byte_1__S2D_Gain = { 0x12, 0x04, 0x02, 0x00 };
/* IR_Mixer_Top bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_IR_Mixer_byte_1__IR_Mixer_Top = { 0x12, 0x00, 0x04, 0x00 };


/* TDA18273 Register AGC5_byte_1 0x13 */
const TDA18273_BitField_t gTDA18273_Reg_AGC5_byte_1 = { 0x13, 0x00, 0x08, 0x00 };
/* AGCs_Do_Step_assym bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_AGC5_byte_1__AGCs_Do_Step_assym = { 0x13, 0x05, 0x02, 0x00 };
/* AGC5_Ana bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_AGC5_byte_1__AGC5_Ana = { 0x13, 0x04, 0x01, 0x00 };
/* AGC5_TOP bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_AGC5_byte_1__AGC5_TOP = { 0x13, 0x00, 0x04, 0x00 };


/* TDA18273 Register IF_AGC_byte 0x14 */
const TDA18273_BitField_t gTDA18273_Reg_IF_AGC_byte = { 0x14, 0x00, 0x08, 0x00 };
/* IFnotchToRSSI bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_IF_AGC_byte__IFnotchToRSSI = { 0x14, 0x07, 0x01, 0x00 };
/* LPF_DCOffset_Corr bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_IF_AGC_byte__LPF_DCOffset_Corr = { 0x14, 0x06, 0x01, 0x00 };
/* IF_level bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_IF_AGC_byte__IF_level = { 0x14, 0x00, 0x03, 0x00 };


/* TDA18273 Register IF_Byte_1 0x15 */
const TDA18273_BitField_t gTDA18273_Reg_IF_Byte_1 = { 0x15, 0x00, 0x08, 0x00 };
/* IF_HP_Fc bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_IF_Byte_1__IF_HP_Fc = { 0x15, 0x06, 0x02, 0x00 };
/* IF_ATSC_Notch bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_IF_Byte_1__IF_ATSC_Notch = { 0x15, 0x05, 0x01, 0x00 };
/* LP_FC_Offset bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_IF_Byte_1__LP_FC_Offset = { 0x15, 0x03, 0x02, 0x00 };
/* LP_Fc bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_IF_Byte_1__LP_Fc = { 0x15, 0x00, 0x03, 0x00 };


/* TDA18273 Register Reference_Byte 0x16 */
const TDA18273_BitField_t gTDA18273_Reg_Reference_Byte = { 0x16, 0x00, 0x08, 0x00 };
/* Digital_Clock_Mode bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_Reference_Byte__Digital_Clock_Mode = { 0x16, 0x06, 0x02, 0x00 };
/* XTout bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_Reference_Byte__XTout = { 0x16, 0x00, 0x02, 0x00 };


/* TDA18273 Register IF_Frequency_byte 0x17 */
const TDA18273_BitField_t gTDA18273_Reg_IF_Frequency_byte = { 0x17, 0x00, 0x08, 0x00 };
/* IF_Freq bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_IF_Frequency_byte__IF_Freq = { 0x17, 0x00, 0x08, 0x00 };


/* TDA18273 Register RF_Frequency_byte_1 0x18 */
const TDA18273_BitField_t gTDA18273_Reg_RF_Frequency_byte_1 = { 0x18, 0x00, 0x08, 0x00 };
/* RF_Freq_1 bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RF_Frequency_byte_1__RF_Freq_1 = { 0x18, 0x00, 0x04, 0x00 };


/* TDA18273 Register RF_Frequency_byte_2 0x19 */
const TDA18273_BitField_t gTDA18273_Reg_RF_Frequency_byte_2 = { 0x19, 0x00, 0x08, 0x00 };
/* RF_Freq_2 bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RF_Frequency_byte_2__RF_Freq_2 = { 0x19, 0x00, 0x08, 0x00 };


/* TDA18273 Register RF_Frequency_byte_3 0x1A */
const TDA18273_BitField_t gTDA18273_Reg_RF_Frequency_byte_3 = { 0x1A, 0x00, 0x08, 0x00 };
/* RF_Freq_3 bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RF_Frequency_byte_3__RF_Freq_3 = { 0x1A, 0x00, 0x08, 0x00 };


/* TDA18273 Register MSM_byte_1 0x1B */
const TDA18273_BitField_t gTDA18273_Reg_MSM_byte_1 = { 0x1B, 0x00, 0x08, 0x00 };
/* RSSI_Meas bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_MSM_byte_1__RSSI_Meas = { 0x1B, 0x07, 0x01, 0x00 };
/* RF_CAL_AV bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_MSM_byte_1__RF_CAL_AV = { 0x1B, 0x06, 0x01, 0x00 };
/* RF_CAL bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_MSM_byte_1__RF_CAL = { 0x1B, 0x05, 0x01, 0x00 };
/* IR_CAL_Loop bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_MSM_byte_1__IR_CAL_Loop = { 0x1B, 0x04, 0x01, 0x00 };
/* IR_Cal_Image bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_MSM_byte_1__IR_Cal_Image = { 0x1B, 0x03, 0x01, 0x00 };
/* IR_CAL_Wanted bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_MSM_byte_1__IR_CAL_Wanted = { 0x1B, 0x02, 0x01, 0x00 };
/* RC_Cal bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_MSM_byte_1__RC_Cal = { 0x1B, 0x01, 0x01, 0x00 };
/* Calc_PLL bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_MSM_byte_1__Calc_PLL = { 0x1B, 0x00, 0x01, 0x00 };


/* TDA18273 Register MSM_byte_2 0x1C */
const TDA18273_BitField_t gTDA18273_Reg_MSM_byte_2 = { 0x1C, 0x00, 0x08, 0x00 };
/* XtalCal_Launch bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_MSM_byte_2__XtalCal_Launch = { 0x1C, 0x01, 0x01, 0x00 };
/* MSM_Launch bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_MSM_byte_2__MSM_Launch = { 0x1C, 0x00, 0x01, 0x00 };


/* TDA18273 Register PowerSavingMode 0x1D */
const TDA18273_BitField_t gTDA18273_Reg_PowerSavingMode = { 0x1D, 0x00, 0x08, 0x00 };
/* PSM_AGC1 bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_PowerSavingMode__PSM_AGC1 = { 0x1D, 0x07, 0x01, 0x00 };
/* PSM_Bandsplit_Filter bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_PowerSavingMode__PSM_Bandsplit_Filter = { 0x1D, 0x05, 0x02, 0x00 };
/* PSM_RFpoly bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_PowerSavingMode__PSM_RFpoly = { 0x1D, 0x04, 0x01, 0x00 };
/* PSM_Mixer bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_PowerSavingMode__PSM_Mixer = { 0x1D, 0x03, 0x01, 0x00 };
/* PSM_Ifpoly bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_PowerSavingMode__PSM_Ifpoly = { 0x1D, 0x02, 0x01, 0x00 };
/* PSM_Lodriver bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_PowerSavingMode__PSM_Lodriver = { 0x1D, 0x00, 0x02, 0x00 };


/* TDA18273 Register Power_Level_byte_2 0x1E */
const TDA18273_BitField_t gTDA18273_Reg_Power_Level_byte_2 = { 0x1E, 0x00, 0x08, 0x00 };
/* PD_PLD_read bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_Power_Level_byte_2__PD_PLD_read = { 0x1E, 0x07, 0x01, 0x00 };
/* IR_Target bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_Power_Level_byte_2__PLD_Temp_Slope = { 0x1E, 0x05, 0x02, 0x00 };
/* IR_GStep bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_Power_Level_byte_2__PLD_Gain_Corr = { 0x1E, 0x00, 0x05, 0x00 };


/* TDA18273 Register Adapt_Top_byte 0x1F */
const TDA18273_BitField_t gTDA18273_Reg_Adapt_Top_byte = { 0x1F, 0x00, 0x08, 0x00 };
/* Fast_Mode_AGC bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_Adapt_Top_byte__Fast_Mode_AGC = { 0x1F, 0x06, 0x01, 0x00 };
/* Range_LNA_Adapt bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_Adapt_Top_byte__Range_LNA_Adapt = { 0x1F, 0x05, 0x01, 0x00 };
/* Index_K_LNA_Adapt bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_Adapt_Top_byte__Index_K_LNA_Adapt = { 0x1F, 0x03, 0x02, 0x00 };
/* Index_K_Top_Adapt bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_Adapt_Top_byte__Index_K_Top_Adapt = { 0x1F, 0x01, 0x02, 0x00 };
/* Ovld_Udld_FastUp bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_Adapt_Top_byte__Ovld_Udld_FastUp = { 0x1F, 0x00, 0x01, 0x00 };


/* TDA18273 Register Vsync_byte 0x20 */
const TDA18273_BitField_t gTDA18273_Reg_Vsync_byte = { 0x20, 0x00, 0x08, 0x00 };
/* Neg_modulation bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_Vsync_byte__Neg_modulation = { 0x20, 0x07, 0x01, 0x00 };
/* Tracer_Step bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_Vsync_byte__Tracer_Step = { 0x20, 0x05, 0x02, 0x00 };
/* Vsync_int bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_Vsync_byte__Vsync_int = { 0x20, 0x04, 0x01, 0x00 };
/* Vsync_Thresh bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_Vsync_byte__Vsync_Thresh = { 0x20, 0x02, 0x02, 0x00 };
/* Vsync_Len bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_Vsync_byte__Vsync_Len = { 0x20, 0x00, 0x02, 0x00 };


/* TDA18273 Register Vsync_Mgt_byte 0x21 */
const TDA18273_BitField_t gTDA18273_Reg_Vsync_Mgt_byte = { 0x21, 0x00, 0x08, 0x00 };
/* PD_Vsync_Mgt bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_Vsync_Mgt_byte__PD_Vsync_Mgt = { 0x21, 0x07, 0x01, 0x00 };
/* PD_Ovld bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_Vsync_Mgt_byte__PD_Ovld = { 0x21, 0x06, 0x01, 0x00 };
/* PD_Ovld_RF bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_Vsync_Mgt_byte__PD_Ovld_RF = { 0x21, 0x05, 0x01, 0x00 };
/* AGC_Ovld_TOP bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_Vsync_Mgt_byte__AGC_Ovld_TOP = { 0x21, 0x02, 0x03, 0x00 };
/* Up_Step_Ovld bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_Vsync_Mgt_byte__Up_Step_Ovld = { 0x21, 0x01, 0x01, 0x00 };
/* AGC_Ovld_Timer bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_Vsync_Mgt_byte__AGC_Ovld_Timer = { 0x21, 0x00, 0x01, 0x00 };


/* TDA18273 Register IR_Mixer_byte_2 0x22 */
const TDA18273_BitField_t gTDA18273_Reg_IR_Mixer_byte_2 = { 0x22, 0x00, 0x08, 0x00 };
/* IR_Mixer_loop_off bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_IR_Mixer_byte_2__IR_Mixer_loop_off = { 0x22, 0x07, 0x01, 0x00 };
/* IR_Mixer_Do_step bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_IR_Mixer_byte_2__IR_Mixer_Do_step = { 0x22, 0x05, 0x02, 0x00 };
/* Hi_Pass bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_IR_Mixer_byte_2__Hi_Pass = { 0x22, 0x01, 0x01, 0x00 };
/* IF_Notch bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_IR_Mixer_byte_2__IF_Notch = { 0x22, 0x00, 0x01, 0x00 };


/* TDA18273 Register AGC1_byte_3 0x23 */
const TDA18273_BitField_t gTDA18273_Reg_AGC1_byte_3 = { 0x23, 0x00, 0x08, 0x00 };
/* AGC1_loop_off bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_AGC1_byte_3__AGC1_loop_off = { 0x23, 0x07, 0x01, 0x00 };
/* AGC1_Do_step bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_AGC1_byte_3__AGC1_Do_step = { 0x23, 0x05, 0x02, 0x00 };
/* Force_AGC1_gain bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_AGC1_byte_3__Force_AGC1_gain = { 0x23, 0x04, 0x01, 0x00 };
/* AGC1_Gain bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_AGC1_byte_3__AGC1_Gain = { 0x23, 0x00, 0x04, 0x00 };


/* TDA18273 Register RFAGCs_Gain_byte_1 0x24 */
const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_1 = { 0x24, 0x00, 0x08, 0x00 };
/* PLD_DAC_Scale bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_1__PLD_DAC_Scale = { 0x24, 0x07, 0x01, 0x00 };
/* PLD_CC_Enable bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_1__PLD_CC_Enable = { 0x24, 0x06, 0x01, 0x00 };
/* PLD_Temp_Enable bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_1__PLD_Temp_Enable = { 0x24, 0x05, 0x01, 0x00 };
/* TH_AGC_Adapt34 bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_1__TH_AGC_Adapt34 = { 0x24, 0x04, 0x01, 0x00 };
/* RFAGC_Sense_Enable bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_1__RFAGC_Sense_Enable = { 0x24, 0x02, 0x01, 0x00 };
/* RFAGC_K_Bypass bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_1__RFAGC_K_Bypass = { 0x24, 0x01, 0x01, 0x00 };
/* RFAGC_K_8 bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_1__RFAGC_K_8 = { 0x24, 0x00, 0x01, 0x00 };


/* TDA18273 Register RFAGCs_Gain_byte_2 0x25 */
const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_2 = { 0x25, 0x00, 0x08, 0x00 };
/* RFAGC_K bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_2__RFAGC_K = { 0x25, 0x00, 0x08, 0x00 };


/* TDA18273 Register AGC5_byte_2 0x26 */
const TDA18273_BitField_t gTDA18273_Reg_AGC5_byte_2 = { 0x26, 0x00, 0x08, 0x00 };
/* AGC5_loop_off bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_AGC5_byte_2__AGC5_loop_off = { 0x26, 0x07, 0x01, 0x00 };
/* AGC5_Do_step bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_AGC5_byte_2__AGC5_Do_step = { 0x26, 0x05, 0x02, 0x00 };
/* Force_AGC5_gain bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_AGC5_byte_2__Force_AGC5_gain = { 0x26, 0x03, 0x01, 0x00 };
/* AGC5_Gain bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_AGC5_byte_2__AGC5_Gain = { 0x26, 0x00, 0x03, 0x00 };


/* TDA18273 Register RF_Cal_byte_1 0x27 */
const TDA18273_BitField_t gTDA18273_Reg_RF_Cal_byte_1 = { 0x27, 0x00, 0x08, 0x00 };
/* RFCAL_Offset_Cprog0 bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RF_Cal_byte_1__RFCAL_Offset_Cprog0 = { 0x27, 0x06, 0x02, 0x00 };
/* RFCAL_Offset_Cprog1 bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RF_Cal_byte_1__RFCAL_Offset_Cprog1 = { 0x27, 0x04, 0x02, 0x00 };
/* RFCAL_Offset_Cprog2 bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RF_Cal_byte_1__RFCAL_Offset_Cprog2 = { 0x27, 0x02, 0x02, 0x00 };
/* RFCAL_Offset_Cprog3 bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RF_Cal_byte_1__RFCAL_Offset_Cprog3 = { 0x27, 0x00, 0x02, 0x00 };


/* TDA18273 Register RF_Cal_byte_2 0x28 */
const TDA18273_BitField_t gTDA18273_Reg_RF_Cal_byte_2 = { 0x28, 0x00, 0x08, 0x00 };
/* RFCAL_Offset_Cprog4 bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RF_Cal_byte_2__RFCAL_Offset_Cprog4 = { 0x28, 0x06, 0x02, 0x00 };
/* RFCAL_Offset_Cprog5 bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RF_Cal_byte_2__RFCAL_Offset_Cprog5 = { 0x28, 0x04, 0x02, 0x00 };
/* RFCAL_Offset_Cprog6 bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RF_Cal_byte_2__RFCAL_Offset_Cprog6 = { 0x28, 0x02, 0x02, 0x00 };
/* RFCAL_Offset_Cprog7 bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RF_Cal_byte_2__RFCAL_Offset_Cprog7 = { 0x28, 0x00, 0x02, 0x00 };


/* TDA18273 Register RF_Cal_byte_3 0x29 */
const TDA18273_BitField_t gTDA18273_Reg_RF_Cal_byte_3 = { 0x29, 0x00, 0x08, 0x00 };
/* RFCAL_Offset_Cprog8 bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RF_Cal_byte_3__RFCAL_Offset_Cprog8 = { 0x29, 0x06, 0x02, 0x00 };
/* RFCAL_Offset_Cprog9 bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RF_Cal_byte_3__RFCAL_Offset_Cprog9 = { 0x29, 0x04, 0x02, 0x00 };
/* RFCAL_Offset_Cprog10 bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RF_Cal_byte_3__RFCAL_Offset_Cprog10 = { 0x29, 0x02, 0x02, 0x00 };
/* RFCAL_Offset_Cprog11 bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RF_Cal_byte_3__RFCAL_Offset_Cprog11 = { 0x29, 0x00, 0x02, 0x00 };


/* TDA18273 Register Bandsplit_Filter_byte 0x2A */
const TDA18273_BitField_t gTDA18273_Reg_Bandsplit_Filter_byte = { 0x2A, 0x00, 0x08, 0x00 };
/* Bandsplit_Filter_SubBand bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_Bandsplit_Filter_byte__Bandsplit_Filter_SubBand = { 0x2A, 0x00, 0x02, 0x00 };


/* TDA18273 Register RF_Filters_byte_1 0x2B */
const TDA18273_BitField_t gTDA18273_Reg_RF_Filters_byte_1 = { 0x2B, 0x00, 0x08, 0x00 };
/* RF_Filter_Bypass bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RF_Filters_byte_1__RF_Filter_Bypass = { 0x2B, 0x07, 0x01, 0x00 };
/* AGC2_loop_off bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RF_Filters_byte_1__AGC2_loop_off = { 0x2B, 0x05, 0x01, 0x00 };
/* Force_AGC2_gain bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RF_Filters_byte_1__Force_AGC2_gain = { 0x2B, 0x04, 0x01, 0x00 };
/* RF_Filter_Gv bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RF_Filters_byte_1__RF_Filter_Gv = { 0x2B, 0x02, 0x02, 0x00 };
/* RF_Filter_Band bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RF_Filters_byte_1__RF_Filter_Band = { 0x2B, 0x00, 0x02, 0x00 };


/* TDA18273 Register RF_Filters_byte_2 0x2C */
const TDA18273_BitField_t gTDA18273_Reg_RF_Filters_byte_2 = { 0x2C, 0x00, 0x08, 0x00 };
/* RF_Filter_Cap bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RF_Filters_byte_2__RF_Filter_Cap = { 0x2C, 0x00, 0x08, 0x00 };


/* TDA18273 Register RF_Filters_byte_3 0x2D */
const TDA18273_BitField_t gTDA18273_Reg_RF_Filters_byte_3 = { 0x2D, 0x00, 0x08, 0x00 };
/* AGC2_Do_step bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RF_Filters_byte_3__AGC2_Do_step = { 0x2D, 0x06, 0x02, 0x00 };
/* Gain_Taper bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RF_Filters_byte_3__Gain_Taper = { 0x2D, 0x00, 0x06, 0x00 };


/* TDA18273 Register RF_Band_Pass_Filter_byte 0x2E */
const TDA18273_BitField_t gTDA18273_Reg_RF_Band_Pass_Filter_byte = { 0x2E, 0x00, 0x08, 0x00 };
/* RF_BPF_Bypass bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RF_Band_Pass_Filter_byte__RF_BPF_Bypass = { 0x2E, 0x07, 0x01, 0x00 };
/* RF_BPF bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RF_Band_Pass_Filter_byte__RF_BPF = { 0x2E, 0x00, 0x03, 0x00 };


/* TDA18273 Register CP_Current_byte 0x2F */
const TDA18273_BitField_t gTDA18273_Reg_CP_Current_byte = { 0x2F, 0x00, 0x08, 0x00 };
/* LO_CP_Current bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_CP_Current_byte__LO_CP_Current = { 0x2F, 0x07, 0x01, 0x00 };
/* N_CP_Current bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_CP_Current_byte__N_CP_Current = { 0x2F, 0x00, 0x07, 0x00 };


/* TDA18273 Register AGCs_DetOut_byte 0x30 */
const TDA18273_BitField_t gTDA18273_Reg_AGCs_DetOut_byte = { 0x30, 0x00, 0x08, 0x00 };
/* Up_AGC5 bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_AGCs_DetOut_byte__Up_AGC5 = { 0x30, 0x07, 0x01, 0x00 };
/* Do_AGC5 bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_AGCs_DetOut_byte__Do_AGC5 = { 0x30, 0x06, 0x01, 0x00 };
/* Up_AGC4 bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_AGCs_DetOut_byte__Up_AGC4 = { 0x30, 0x05, 0x01, 0x00 };
/* Do_AGC4 bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_AGCs_DetOut_byte__Do_AGC4 = { 0x30, 0x04, 0x01, 0x00 };
/* Up_AGC2 bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_AGCs_DetOut_byte__Up_AGC2 = { 0x30, 0x03, 0x01, 0x00 };
/* Do_AGC2 bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_AGCs_DetOut_byte__Do_AGC2 = { 0x30, 0x02, 0x01, 0x00 };
/* Up_AGC1 bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_AGCs_DetOut_byte__Up_AGC1 = { 0x30, 0x01, 0x01, 0x00 };
/* Do_AGC1 bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_AGCs_DetOut_byte__Do_AGC1 = { 0x30, 0x00, 0x01, 0x00 };


/* TDA18273 Register RFAGCs_Gain_byte_3 0x31 */
const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_3 = { 0x31, 0x00, 0x08, 0x00 };
/* AGC2_Gain_Read bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_3__AGC2_Gain_Read = { 0x31, 0x04, 0x02, 0x00 };
/* AGC1_Gain_Read bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_3__AGC1_Gain_Read = { 0x31, 0x00, 0x04, 0x00 };


/* TDA18273 Register RFAGCs_Gain_byte_4 0x32 */
const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_4 = { 0x32, 0x00, 0x08, 0x00 };
/* Cprog_Read bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_4__Cprog_Read = { 0x32, 0x00, 0x08, 0x00 };


/* TDA18273 Register RFAGCs_Gain_byte_5 0x33 */
const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_5 = { 0x33, 0x00, 0x08, 0x00 };
/* RFAGC_Read_K_8 bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_5__RFAGC_Read_K_8 = { 0x33, 0x07, 0x01, 0x00 };
/* Do_AGC1bis bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_5__Do_AGC1bis = { 0x33, 0x06, 0x01, 0x00 };
/* AGC1_Top_Adapt_Low bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_5__AGC1_Top_Adapt_Low = { 0x33, 0x05, 0x01, 0x00 };
/* Up_LNA_Adapt bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_5__Up_LNA_Adapt = { 0x33, 0x04, 0x01, 0x00 };
/* Do_LNA_Adapt bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_5__Do_LNA_Adapt = { 0x33, 0x03, 0x01, 0x00 };
/* TOP_AGC3_Read bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_5__TOP_AGC3_Read = { 0x33, 0x00, 0x03, 0x00 };


/* TDA18273 Register RFAGCs_Gain_byte_6 0x34 */
const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_6 = { 0x34, 0x00, 0x08, 0x00 };
/* RFAGC_Read_K bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_6__RFAGC_Read_K = { 0x34, 0x00, 0x08, 0x00 };


/* TDA18273 Register IFAGCs_Gain_byte 0x35 */
const TDA18273_BitField_t gTDA18273_Reg_IFAGCs_Gain_byte = { 0x35, 0x00, 0x08, 0x00 };
/* AGC5_Gain_Read bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_IFAGCs_Gain_byte__AGC5_Gain_Read = { 0x35, 0x03, 0x03, 0x00 };
/* AGC4_Gain_Read bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_IFAGCs_Gain_byte__AGC4_Gain_Read = { 0x35, 0x00, 0x03, 0x00 };


/* TDA18273 Register RSSI_byte_1 0x36 */
const TDA18273_BitField_t gTDA18273_Reg_RSSI_byte_1 = { 0x36, 0x00, 0x08, 0x00 };
/* RSSI bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RSSI_byte_1__RSSI = { 0x36, 0x00, 0x08, 0x00 };


/* TDA18273 Register RSSI_byte_2 0x37 */
const TDA18273_BitField_t gTDA18273_Reg_RSSI_byte_2 = { 0x37, 0x00, 0x08, 0x00 };
/* RSSI_AV bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RSSI_byte_2__RSSI_AV = { 0x37, 0x05, 0x01, 0x00 };
/* RSSI_Cap_Reset_En bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RSSI_byte_2__RSSI_Cap_Reset_En = { 0x37, 0x03, 0x01, 0x00 };
/* RSSI_Cap_Val bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RSSI_byte_2__RSSI_Cap_Val = { 0x37, 0x02, 0x01, 0x00 };
/* RSSI_Ck_Speed bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RSSI_byte_2__RSSI_Ck_Speed = { 0x37, 0x01, 0x01, 0x00 };
/* RSSI_Dicho_not bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_RSSI_byte_2__RSSI_Dicho_not = { 0x37, 0x00, 0x01, 0x00 };


/* TDA18273 Register Misc_byte 0x38 */
const TDA18273_BitField_t gTDA18273_Reg_Misc_byte = { 0x38, 0x00, 0x08, 0x00 };
/* RFCALPOR_I2C bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_Misc_byte__RFCALPOR_I2C = { 0x38, 0x06, 0x01, 0x00 };
/* PD_Underload bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_Misc_byte__PD_Underload = { 0x38, 0x05, 0x01, 0x00 };
/* DDS_Polarity bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_Misc_byte__DDS_Polarity = { 0x38, 0x04, 0x01, 0x00 };
/* IRQ_Mode bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_Misc_byte__IRQ_Mode = { 0x38, 0x01, 0x01, 0x00 };
/* IRQ_Polarity bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_Misc_byte__IRQ_Polarity = { 0x38, 0x00, 0x01, 0x00 };


/* TDA18273 Register rfcal_log_0 0x39 */
const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_0 = { 0x39, 0x00, 0x08, 0x00 };
/* rfcal_log_0 bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_0__rfcal_log_0 = { 0x39, 0x00, 0x08, 0x00 };


/* TDA18273 Register rfcal_log_1 0x3A */
const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_1 = { 0x3A, 0x00, 0x08, 0x00 };
/* rfcal_log_1 bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_1__rfcal_log_1 = { 0x3A, 0x00, 0x08, 0x00 };


/* TDA18273 Register rfcal_log_2 0x3B */
const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_2 = { 0x3B, 0x00, 0x08, 0x00 };
/* rfcal_log_2 bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_2__rfcal_log_2 = { 0x3B, 0x00, 0x08, 0x00 };


/* TDA18273 Register rfcal_log_3 0x3C */
const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_3 = { 0x3C, 0x00, 0x08, 0x00 };
/* rfcal_log_3 bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_3__rfcal_log_3 = { 0x3C, 0x00, 0x08, 0x00 };


/* TDA18273 Register rfcal_log_4 0x3D */
const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_4 = { 0x3D, 0x00, 0x08, 0x00 };
/* rfcal_log_4 bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_4__rfcal_log_4 = { 0x3D, 0x00, 0x08, 0x00 };


/* TDA18273 Register rfcal_log_5 0x3E */
const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_5 = { 0x3E, 0x00, 0x08, 0x00 };
/* rfcal_log_5 bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_5__rfcal_log_5 = { 0x3E, 0x00, 0x08, 0x00 };


/* TDA18273 Register rfcal_log_6 0x3F */
const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_6 = { 0x3F, 0x00, 0x08, 0x00 };
/* rfcal_log_6 bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_6__rfcal_log_6 = { 0x3F, 0x00, 0x08, 0x00 };


/* TDA18273 Register rfcal_log_7 0x40 */
const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_7 = { 0x40, 0x00, 0x08, 0x00 };
/* rfcal_log_7 bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_7__rfcal_log_7 = { 0x40, 0x00, 0x08, 0x00 };


/* TDA18273 Register rfcal_log_8 0x41 */
const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_8 = { 0x41, 0x00, 0x08, 0x00 };
/* rfcal_log_8 bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_8__rfcal_log_8 = { 0x41, 0x00, 0x08, 0x00 };


/* TDA18273 Register rfcal_log_9 0x42 */
const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_9 = { 0x42, 0x00, 0x08, 0x00 };
/* rfcal_log_9 bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_9__rfcal_log_9 = { 0x42, 0x00, 0x08, 0x00 };


/* TDA18273 Register rfcal_log_10 0x43 */
const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_10 = { 0x43, 0x00, 0x08, 0x00 };
/* rfcal_log_10 bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_10__rfcal_log_10 = { 0x43, 0x00, 0x08, 0x00 };


/* TDA18273 Register rfcal_log_11 0x44 */
const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_11 = { 0x44, 0x00, 0x08, 0x00 };
/* rfcal_log_11 bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_11__rfcal_log_11 = { 0x44, 0x00, 0x08, 0x00 };



/* TDA18273 Register Main_Post_Divider_byte 0x51 */
const TDA18273_BitField_t gTDA18273_Reg_Main_Post_Divider_byte = { 0x51, 0x00, 0x08, 0x00 };
/* LOPostDiv bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_Main_Post_Divider_byte__LOPostDiv = { 0x51, 0x04, 0x03, 0x00 };
/* LOPresc bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_Main_Post_Divider_byte__LOPresc = { 0x51, 0x00, 0x04, 0x00 };


/* TDA18273 Register Sigma_delta_byte_1 0x52 */
const TDA18273_BitField_t gTDA18273_Reg_Sigma_delta_byte_1 = { 0x52, 0x00, 0x08, 0x00 };
/* LO_Int bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_Sigma_delta_byte_1__LO_Int = { 0x52, 0x00, 0x07, 0x00 };


/* TDA18273 Register Sigma_delta_byte_2 0x53 */
const TDA18273_BitField_t gTDA18273_Reg_Sigma_delta_byte_2 = { 0x53, 0x00, 0x08, 0x00 };
/* LO_Frac_2 bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_Sigma_delta_byte_2__LO_Frac_2 = { 0x53, 0x00, 0x07, 0x00 };


/* TDA18273 Register Sigma_delta_byte_3 0x54 */
const TDA18273_BitField_t gTDA18273_Reg_Sigma_delta_byte_3 = { 0x54, 0x00, 0x08, 0x00 };
/* LO_Frac_1 bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_Sigma_delta_byte_3__LO_Frac_1 = { 0x54, 0x00, 0x08, 0x00 };


/* TDA18273 Register Sigma_delta_byte_4 0x55 */
const TDA18273_BitField_t gTDA18273_Reg_Sigma_delta_byte_4 = { 0x55, 0x00, 0x08, 0x00 };
/* LO_Frac_0 bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_Sigma_delta_byte_4__LO_Frac_0 = { 0x55, 0x00, 0x08, 0x00 };


/* TDA18273 Register Sigma_delta_byte_5 0x56 */
const TDA18273_BitField_t gTDA18273_Reg_Sigma_delta_byte_5 = { 0x56, 0x00, 0x08, 0x00 };
/* N_K_correct_manual bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_Sigma_delta_byte_5__N_K_correct_manual = { 0x56, 0x01, 0x01, 0x00 };
/* LO_Calc_Disable bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_Sigma_delta_byte_5__LO_Calc_Disable = { 0x56, 0x00, 0x01, 0x00 };


/* TDA18273 Register Regulators_byte 0x58 */
const TDA18273_BitField_t gTDA18273_Reg_Regulators_byte = { 0x58, 0x00, 0x08, 0x00 };
/* RF_Reg bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_Regulators_byte__RF_Reg = { 0x58, 0x02, 0x02, 0x00 };


/* TDA18273 Register IR_Cal_byte_5 0x5B */
const TDA18273_BitField_t gTDA18273_Reg_IR_Cal_byte_5 = { 0x5B, 0x00, 0x08, 0x00 };
/* Mixer_Gain_Bypass bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_IR_Cal_byte_5__Mixer_Gain_Bypass = { 0x5B, 0x07, 0x01, 0x00 };
/* IR_Mixer_Gain bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_IR_Cal_byte_5__IR_Mixer_Gain = { 0x5B, 0x04, 0x03, 0x00 };


/* TDA18273 Register Power_Down_byte_2 0x5F */
const TDA18273_BitField_t gTDA18273_Reg_Power_Down_byte_2 = { 0x5F, 0x00, 0x08, 0x00 };
/* PD_LNA bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_Power_Down_byte_2__PD_LNA = { 0x5F, 0x07, 0x01, 0x00 };
/* PD_Det4 bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_Power_Down_byte_2__PD_Det4 = { 0x5F, 0x03, 0x01, 0x00 };
/* PD_Det3 bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_Power_Down_byte_2__PD_Det3 = { 0x5F, 0x02, 0x01, 0x00 };
/* PD_Det1 bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_Power_Down_byte_2__PD_Det1 = { 0x5F, 0x00, 0x01, 0x00 };

/* TDA18273 Register Power_Down_byte_3 0x60 */
const TDA18273_BitField_t gTDA18273_Reg_Power_Down_byte_3 = { 0x60, 0x00, 0x08, 0x00 };
/* Force_Soft_Reset bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_Power_Down_byte_3__Force_Soft_Reset = { 0x60, 0x01, 0x01, 0x00 };
/* Soft_Reset bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_Power_Down_byte_3__Soft_Reset = { 0x60, 0x00, 0x01, 0x00 };

/* TDA18273 Register Charge_pump_byte 0x64 */
const TDA18273_BitField_t gTDA18273_Reg_Charge_pump_byte = { 0x64, 0x00, 0x08, 0x00 };
/* ICP_Bypass bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_Charge_pump_byte__ICP_Bypass = { 0x64, 0x07, 0x01, 0x00 };
/* ICP bit(s):  */
const TDA18273_BitField_t gTDA18273_Reg_Charge_pump_byte__ICP = { 0x64, 0x00, 0x02, 0x00 };

