/*
  Copyright (C) 2010 NXP B.V., All Rights Reserved.
  This source code and any compilation or derivative thereof is the proprietary
  information of NXP B.V. and is confidential in nature. Under no circumstances
  is this software to be  exposed to or placed under an Open Source License of
  any type without the expressed written permission of NXP B.V.
 *
 * \file          tmbslTDA18273_RegDef.h
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

#ifndef _TMBSL_TDA18273_REGDEF_H
#define _TMBSL_TDA18273_REGDEF_H

#ifdef __cplusplus
extern "C"
{
#endif

/*============================================================================*/
/* Registers definitions:                                                     */
/*============================================================================*/

#define TDA18273_REG_ADD_SZ                             (0x01)
#define TDA18273_REG_DATA_MAX_SZ                        (0x01)
#define TDA18273_REG_MAP_NB_BYTES                       (0x6D)

#define TDA18273_REG_DATA_LEN(_FIRST_REG, _LAST_REG)    ( (_LAST_REG.Address - _FIRST_REG.Address) + 1)


/* TDA18273 Register ID_byte_1 0x00 */
extern const TDA18273_BitField_t gTDA18273_Reg_ID_byte_1;
/* MS bit(s): Indicate if Device is a Master or a Slave*/
/*  1 => Master device */
/*  0 => Slave device */
extern const TDA18273_BitField_t gTDA18273_Reg_ID_byte_1__MS;
/* Ident_1 bit(s): MSB of device identifier */
extern const TDA18273_BitField_t gTDA18273_Reg_ID_byte_1__Ident_1;


/* TDA18273 Register ID_byte_2 0x00 */
extern const TDA18273_BitField_t gTDA18273_Reg_ID_byte_2;
/* Ident_2 bit(s): LSB of device identifier */
extern const TDA18273_BitField_t gTDA18273_Reg_ID_byte_2__Ident_2;


/* TDA18273 Register ID_byte_3 0x02 */
extern const TDA18273_BitField_t gTDA18273_Reg_ID_byte_3;
/* Major_rev bit(s): Major revision of device */
extern const TDA18273_BitField_t gTDA18273_Reg_ID_byte_3__Major_rev;
/* Major_rev bit(s): Minor revision of device */
extern const TDA18273_BitField_t gTDA18273_Reg_ID_byte_3__Minor_rev;


/* TDA18273 Register Thermo_byte_1 0x03 */
extern const TDA18273_BitField_t gTDA18273_Reg_Thermo_byte_1;
/* TM_D bit(s): Device temperature */
extern const TDA18273_BitField_t gTDA18273_Reg_Thermo_byte_1__TM_D;


/* TDA18273 Register Thermo_byte_2 0x04 */
extern const TDA18273_BitField_t gTDA18273_Reg_Thermo_byte_2;
/* TM_ON bit(s): Set device temperature measurement to ON or OFF */
/*  1 => Temperature measurement ON */
/*  0 => Temperature measurement OFF */
extern const TDA18273_BitField_t gTDA18273_Reg_Thermo_byte_2__TM_ON;


/* TDA18273 Register Power_state_byte_1 0x05 */
extern const TDA18273_BitField_t gTDA18273_Reg_Power_state_byte_1;
/* POR bit(s): Indicates that device just powered ON */
/*  1 => POR: No access done to device */
/*  0 => At least one access has been done to device */
extern const TDA18273_BitField_t gTDA18273_Reg_Power_state_byte_1__POR;
/* AGCs_Lock bit(s): Indicates that AGCs are locked */
/*  1 => AGCs locked */
/*  0 => AGCs not locked */
extern const TDA18273_BitField_t gTDA18273_Reg_Power_state_byte_1__AGCs_Lock;
/* Vsync_Lock bit(s): Indicates that VSync is locked */
/*  1 => VSync locked */
/*  0 => VSync not locked */
extern const TDA18273_BitField_t gTDA18273_Reg_Power_state_byte_1__Vsync_Lock;
/* LO_Lock bit(s): Indicates that LO is locked */
/*  1 => LO locked */
/*  0 => LO not locked */
extern const TDA18273_BitField_t gTDA18273_Reg_Power_state_byte_1__LO_Lock;


/* TDA18273 Register Power_state_byte_2 0x06 */
extern const TDA18273_BitField_t gTDA18273_Reg_Power_state_byte_2;
/* SM bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_Power_state_byte_2__SM;
/* SM_XT bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_Power_state_byte_2__SM_XT;


/* TDA18273 Register Input_Power_Level_byte 0x07 */
extern const TDA18273_BitField_t gTDA18273_Reg_Input_Power_Level_byte;
/* Power_Level bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_Input_Power_Level_byte__Power_Level;


/* TDA18273 Register IRQ_status 0x08 */
extern const TDA18273_BitField_t gTDA18273_Reg_IRQ_status;
/* IRQ_status bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_IRQ_status__IRQ_status;
/* MSM_XtalCal_End bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_IRQ_status__MSM_XtalCal_End;
/* MSM_RSSI_End bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_IRQ_status__MSM_RSSI_End;
/* MSM_LOCalc_End bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_IRQ_status__MSM_LOCalc_End;
/* MSM_RFCal_End bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_IRQ_status__MSM_RFCal_End;
/* MSM_IRCAL_End bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_IRQ_status__MSM_IRCAL_End;
/* MSM_RCCal_End bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_IRQ_status__MSM_RCCal_End;


/* TDA18273 Register IRQ_enable 0x09 */
extern const TDA18273_BitField_t gTDA18273_Reg_IRQ_enable;
/* IRQ_Enable bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_IRQ_enable__IRQ_Enable;
/* MSM_XtalCal_Enable bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_IRQ_enable__MSM_XtalCal_Enable;
/* MSM_RSSI_Enable bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_IRQ_enable__MSM_RSSI_Enable;
/* MSM_LOCalc_Enable bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_IRQ_enable__MSM_LOCalc_Enable;
/* MSM_RFCal_Enable bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_IRQ_enable__MSM_RFCal_Enable;
/* MSM_IRCAL_Enable bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_IRQ_enable__MSM_IRCAL_Enable;
/* MSM_RCCal_Enable bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_IRQ_enable__MSM_RCCal_Enable;


/* TDA18273 Register IRQ_clear 0x0A */
extern const TDA18273_BitField_t gTDA18273_Reg_IRQ_clear;
/* IRQ_Clear bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_IRQ_clear__IRQ_Clear;
/* MSM_XtalCal_Clear bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_IRQ_clear__MSM_XtalCal_Clear;
/* MSM_RSSI_Clear bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_IRQ_clear__MSM_RSSI_Clear;
/* MSM_LOCalc_Clear bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_IRQ_clear__MSM_LOCalc_Clear;
/* MSM_RFCal_Clear bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_IRQ_clear__MSM_RFCal_Clear;
/* MSM_IRCAL_Clear bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_IRQ_clear__MSM_IRCAL_Clear;
/* MSM_RCCal_Clear bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_IRQ_clear__MSM_RCCal_Clear;


/* TDA18273 Register IRQ_set 0x0B */
extern const TDA18273_BitField_t gTDA18273_Reg_IRQ_set;
/* IRQ_Set bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_IRQ_set__IRQ_Set;
/* MSM_XtalCal_Set bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_IRQ_set__MSM_XtalCal_Set;
/* MSM_RSSI_Set bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_IRQ_set__MSM_RSSI_Set;
/* MSM_LOCalc_Set bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_IRQ_set__MSM_LOCalc_Set;
/* MSM_RFCal_Set bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_IRQ_set__MSM_RFCal_Set;
/* MSM_IRCAL_Set bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_IRQ_set__MSM_IRCAL_Set;
/* MSM_RCCal_Set bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_IRQ_set__MSM_RCCal_Set;


/* TDA18273 Register AGC1_byte_1 0x0C */
extern const TDA18273_BitField_t gTDA18273_Reg_AGC1_byte_1;
/* AGC1_TOP bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_AGC1_byte_1__AGC1_TOP;


/* TDA18273 Register AGC1_byte_2 0x0D */
extern const TDA18273_BitField_t gTDA18273_Reg_AGC1_byte_2;
/* AGC1_Top_Mode_Val bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_AGC1_byte_2__AGC1_Top_Mode_Val;
/* AGC1_Top_Mode bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_AGC1_byte_2__AGC1_Top_Mode;


/* TDA18273 Register AGC2_byte_1 0x0E */
extern const TDA18273_BitField_t gTDA18273_Reg_AGC2_byte_1;
/* AGC2_TOP bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_AGC2_byte_1__AGC2_TOP;


/* TDA18273 Register AGCK_byte_1 0x0F */
extern const TDA18273_BitField_t gTDA18273_Reg_AGCK_byte_1;
/* AGCs_Up_Step_assym bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_AGCK_byte_1__AGCs_Up_Step_assym;
/* Pulse_Shaper_Disable bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_AGCK_byte_1__Pulse_Shaper_Disable;
/* AGCK_Step bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_AGCK_byte_1__AGCK_Step;
/* AGCK_Mode bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_AGCK_byte_1__AGCK_Mode;


/* TDA18273 Register RF_AGC_byte 0x10 */
extern const TDA18273_BitField_t gTDA18273_Reg_RF_AGC_byte;
/* PD_AGC_Adapt3x bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RF_AGC_byte__PD_AGC_Adapt3x;
/* RFAGC_Adapt_TOP bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RF_AGC_byte__RFAGC_Adapt_TOP;
/* RFAGC_Low_BW bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RF_AGC_byte__RFAGC_Low_BW;
/* RFAGC_Top bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RF_AGC_byte__RFAGC_Top;


/* TDA18273 Register W_Filter_byte 0x11 */
extern const TDA18273_BitField_t gTDA18273_Reg_W_Filter_byte;
/* VHF_III_mode bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_W_Filter_byte__VHF_III_mode;
/* RF_Atten_3dB bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_W_Filter_byte__RF_Atten_3dB;
/* W_Filter_Enable bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_W_Filter_byte__W_Filter_Enable;
/* W_Filter_Bypass bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_W_Filter_byte__W_Filter_Bypass;
/* W_Filter bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_W_Filter_byte__W_Filter;
/* W_Filter_Offset bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_W_Filter_byte__W_Filter_Offset;


/* TDA18273 Register IR_Mixer_byte_1 0x12 */
extern const TDA18273_BitField_t gTDA18273_Reg_IR_Mixer_byte_1;
/* S2D_Gain bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_IR_Mixer_byte_1__S2D_Gain;
/* IR_Mixer_Top bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_IR_Mixer_byte_1__IR_Mixer_Top;


/* TDA18273 Register AGC5_byte_1 0x13 */
extern const TDA18273_BitField_t gTDA18273_Reg_AGC5_byte_1;
/* AGCs_Do_Step_assym bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_AGC5_byte_1__AGCs_Do_Step_assym;
/* AGC5_Ana bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_AGC5_byte_1__AGC5_Ana;
/* AGC5_TOP bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_AGC5_byte_1__AGC5_TOP;


/* TDA18273 Register IF_AGC_byte 0x14 */
extern const TDA18273_BitField_t gTDA18273_Reg_IF_AGC_byte;
/* IFnotchToRSSI bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_IF_AGC_byte__IFnotchToRSSI;
/* IF_level bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_IF_AGC_byte__LPF_DCOffset_Corr;
/* IF_level bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_IF_AGC_byte__IF_level;


/* TDA18273 Register IF_Byte_1 0x15 */
extern const TDA18273_BitField_t gTDA18273_Reg_IF_Byte_1;
/* IF_HP_Fc bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_IF_Byte_1__IF_HP_Fc;
/* IF_ATSC_Notch bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_IF_Byte_1__IF_ATSC_Notch;
/* LP_FC_Offset bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_IF_Byte_1__LP_FC_Offset;
/* LP_Fc bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_IF_Byte_1__LP_Fc;


/* TDA18273 Register Reference_Byte 0x16 */
extern const TDA18273_BitField_t gTDA18273_Reg_Reference_Byte;
/* Digital_Clock_Mode bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_Reference_Byte__Digital_Clock_Mode;
/* XTout bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_Reference_Byte__XTout;


/* TDA18273 Register IF_Frequency_byte 0x17 */
extern const TDA18273_BitField_t gTDA18273_Reg_IF_Frequency_byte;
/* IF_Freq bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_IF_Frequency_byte__IF_Freq;


/* TDA18273 Register RF_Frequency_byte_1 0x18 */
extern const TDA18273_BitField_t gTDA18273_Reg_RF_Frequency_byte_1;
/* RF_Freq_1 bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RF_Frequency_byte_1__RF_Freq_1;


/* TDA18273 Register RF_Frequency_byte_2 0x19 */
extern const TDA18273_BitField_t gTDA18273_Reg_RF_Frequency_byte_2;
/* RF_Freq_2 bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RF_Frequency_byte_2__RF_Freq_2;


/* TDA18273 Register RF_Frequency_byte_3 0x1A */
extern const TDA18273_BitField_t gTDA18273_Reg_RF_Frequency_byte_3;
/* RF_Freq_3 bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RF_Frequency_byte_3__RF_Freq_3;


/* TDA18273 Register MSM_byte_1 0x1B */
extern const TDA18273_BitField_t gTDA18273_Reg_MSM_byte_1;
/* RSSI_Meas bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_MSM_byte_1__RSSI_Meas;
/* RF_CAL_AV bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_MSM_byte_1__RF_CAL_AV;
/* RF_CAL bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_MSM_byte_1__RF_CAL;
/* IR_CAL_Loop bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_MSM_byte_1__IR_CAL_Loop;
/* IR_Cal_Image bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_MSM_byte_1__IR_Cal_Image;
/* IR_CAL_Wanted bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_MSM_byte_1__IR_CAL_Wanted;
/* RC_Cal bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_MSM_byte_1__RC_Cal;
/* Calc_PLL bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_MSM_byte_1__Calc_PLL;


/* TDA18273 Register MSM_byte_2 0x1C */
extern const TDA18273_BitField_t gTDA18273_Reg_MSM_byte_2;
/* XtalCal_Launch bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_MSM_byte_2__XtalCal_Launch;
/* MSM_Launch bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_MSM_byte_2__MSM_Launch;


/* TDA18273 Register PowerSavingMode 0x1D */
extern const TDA18273_BitField_t gTDA18273_Reg_PowerSavingMode;
/* PSM_AGC1 bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_PowerSavingMode__PSM_AGC1;
/* PSM_Bandsplit_Filter bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_PowerSavingMode__PSM_Bandsplit_Filter;
/* PSM_RFpoly bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_PowerSavingMode__PSM_RFpoly;
/* PSM_Mixer bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_PowerSavingMode__PSM_Mixer;
/* PSM_Ifpoly bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_PowerSavingMode__PSM_Ifpoly;
/* PSM_Lodriver bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_PowerSavingMode__PSM_Lodriver;


/* TDA18273 Register Power_Level_byte_2 0x1E */
extern const TDA18273_BitField_t gTDA18273_Reg_Power_Level_byte_2;
/* PD_PLD_read bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_Power_Level_byte_2__PD_PLD_read;
/* IR_Target bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_Power_Level_byte_2__PLD_Temp_Slope;
/* IR_GStep bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_Power_Level_byte_2__PLD_Gain_Corr;


/* TDA18273 Register Adapt_Top_byte 0x1F */
extern const TDA18273_BitField_t gTDA18273_Reg_Adapt_Top_byte;
/* Fast_Mode_AGC bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_Adapt_Top_byte__Fast_Mode_AGC;
/* Range_LNA_Adapt bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_Adapt_Top_byte__Range_LNA_Adapt;
/* Index_K_LNA_Adapt bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_Adapt_Top_byte__Index_K_LNA_Adapt;
/* Index_K_Top_Adapt bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_Adapt_Top_byte__Index_K_Top_Adapt;
/* Ovld_Udld_FastUp bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_Adapt_Top_byte__Ovld_Udld_FastUp;


/* TDA18273 Register Vsync_byte 0x20 */
extern const TDA18273_BitField_t gTDA18273_Reg_Vsync_byte;
/* Neg_modulation bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_Vsync_byte__Neg_modulation;
/* Tracer_Step bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_Vsync_byte__Tracer_Step;
/* Vsync_int bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_Vsync_byte__Vsync_int;
/* Vsync_Thresh bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_Vsync_byte__Vsync_Thresh;
/* Vsync_Len bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_Vsync_byte__Vsync_Len;


/* TDA18273 Register Vsync_Mgt_byte 0x21 */
extern const TDA18273_BitField_t gTDA18273_Reg_Vsync_Mgt_byte;
/* PD_Vsync_Mgt bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_Vsync_Mgt_byte__PD_Vsync_Mgt;
/* PD_Ovld bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_Vsync_Mgt_byte__PD_Ovld;
/* PD_Ovld_RF bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_Vsync_Mgt_byte__PD_Ovld_RF;
/* AGC_Ovld_TOP bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_Vsync_Mgt_byte__AGC_Ovld_TOP;
/* Up_Step_Ovld bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_Vsync_Mgt_byte__Up_Step_Ovld;
/* AGC_Ovld_Timer bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_Vsync_Mgt_byte__AGC_Ovld_Timer;


/* TDA18273 Register IR_Mixer_byte_2 0x22 */
extern const TDA18273_BitField_t gTDA18273_Reg_IR_Mixer_byte_2;
/* IR_Mixer_loop_off bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_IR_Mixer_byte_2__IR_Mixer_loop_off;
/* IR_Mixer_Do_step bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_IR_Mixer_byte_2__IR_Mixer_Do_step;
/* Hi_Pass bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_IR_Mixer_byte_2__Hi_Pass;
/* IF_Notch bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_IR_Mixer_byte_2__IF_Notch;


/* TDA18273 Register AGC1_byte_3 0x23 */
extern const TDA18273_BitField_t gTDA18273_Reg_AGC1_byte_3;
/* AGC1_loop_off bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_AGC1_byte_3__AGC1_loop_off;
/* AGC1_Do_step bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_AGC1_byte_3__AGC1_Do_step;
/* Force_AGC1_gain bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_AGC1_byte_3__Force_AGC1_gain;
/* AGC1_Gain bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_AGC1_byte_3__AGC1_Gain;


/* TDA18273 Register RFAGCs_Gain_byte_1 0x24 */
extern const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_1;
/* PLD_DAC_Scale bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_1__PLD_DAC_Scale;
/* PLD_CC_Enable bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_1__PLD_CC_Enable;
/* PLD_Temp_Enable bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_1__PLD_Temp_Enable;
/* TH_AGC_Adapt34 bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_1__TH_AGC_Adapt34;
/* RFAGC_Sense_Enable bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_1__RFAGC_Sense_Enable;
/* RFAGC_K_Bypass bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_1__RFAGC_K_Bypass;
/* RFAGC_K_8 bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_1__RFAGC_K_8;


/* TDA18273 Register RFAGCs_Gain_byte_2 0x25 */
extern const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_2;
/* RFAGC_K bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_2__RFAGC_K;


/* TDA18273 Register AGC5_byte_2 0x26 */
extern const TDA18273_BitField_t gTDA18273_Reg_AGC5_byte_2;
/* AGC5_loop_off bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_AGC5_byte_2__AGC5_loop_off;
/* AGC5_Do_step bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_AGC5_byte_2__AGC5_Do_step;
/* Force_AGC5_gain bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_AGC5_byte_2__Force_AGC5_gain;
/* AGC5_Gain bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_AGC5_byte_2__AGC5_Gain;


/* TDA18273 Register RF_Cal_byte_1 0x27 */
extern const TDA18273_BitField_t gTDA18273_Reg_RF_Cal_byte_1;
/* RFCAL_Offset_Cprog0 bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RF_Cal_byte_1__RFCAL_Offset_Cprog0;
/* RFCAL_Offset_Cprog1 bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RF_Cal_byte_1__RFCAL_Offset_Cprog1;
/* RFCAL_Offset_Cprog2 bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RF_Cal_byte_1__RFCAL_Offset_Cprog2;
/* RFCAL_Offset_Cprog3 bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RF_Cal_byte_1__RFCAL_Offset_Cprog3;

/* TDA18273 Register RF_Cal_byte_2 0x28 */
extern const TDA18273_BitField_t gTDA18273_Reg_RF_Cal_byte_2;
/* RFCAL_Offset_Cprog4 bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RF_Cal_byte_2__RFCAL_Offset_Cprog4;
/* RFCAL_Offset_Cprog5 bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RF_Cal_byte_2__RFCAL_Offset_Cprog5;
/* RFCAL_Offset_Cprog6 bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RF_Cal_byte_2__RFCAL_Offset_Cprog6;
/* RFCAL_Offset_Cprog7 bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RF_Cal_byte_2__RFCAL_Offset_Cprog7;


/* TDA18273 Register RF_Cal_byte_3 0x29 */
extern const TDA18273_BitField_t gTDA18273_Reg_RF_Cal_byte_3;
/* RFCAL_Offset_Cprog8 bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RF_Cal_byte_3__RFCAL_Offset_Cprog8;
/* RFCAL_Offset_Cprog9 bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RF_Cal_byte_3__RFCAL_Offset_Cprog9;
/* RFCAL_Offset_Cprog10 bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RF_Cal_byte_3__RFCAL_Offset_Cprog10;
/* RFCAL_Offset_Cprog11 bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RF_Cal_byte_3__RFCAL_Offset_Cprog11;


/* TDA18273 Register Bandsplit_Filter_byte 0x2A */
extern const TDA18273_BitField_t gTDA18273_Reg_Bandsplit_Filter_byte;
/* Bandsplit_Filter_SubBand bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_Bandsplit_Filter_byte__Bandsplit_Filter_SubBand;


/* TDA18273 Register RF_Filters_byte_1 0x2B */
extern const TDA18273_BitField_t gTDA18273_Reg_RF_Filters_byte_1;
/* RF_Filter_Bypass bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RF_Filters_byte_1__RF_Filter_Bypass;
/* AGC2_loop_off bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RF_Filters_byte_1__AGC2_loop_off;
/* Force_AGC2_gain bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RF_Filters_byte_1__Force_AGC2_gain;
/* RF_Filter_Gv bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RF_Filters_byte_1__RF_Filter_Gv;
/* RF_Filter_Band bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RF_Filters_byte_1__RF_Filter_Band;


/* TDA18273 Register RF_Filters_byte_2 0x2C */
extern const TDA18273_BitField_t gTDA18273_Reg_RF_Filters_byte_2;
/* RF_Filter_Cap bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RF_Filters_byte_2__RF_Filter_Cap;


/* TDA18273 Register RF_Filters_byte_3 0x2D */
extern const TDA18273_BitField_t gTDA18273_Reg_RF_Filters_byte_3;
/* AGC2_Do_step bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RF_Filters_byte_3__AGC2_Do_step;
/* Gain_Taper bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RF_Filters_byte_3__Gain_Taper;


/* TDA18273 Register RF_Band_Pass_Filter_byte 0x2E */
extern const TDA18273_BitField_t gTDA18273_Reg_RF_Band_Pass_Filter_byte;
/* RF_BPF_Bypass bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RF_Band_Pass_Filter_byte__RF_BPF_Bypass;
/* RF_BPF bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RF_Band_Pass_Filter_byte__RF_BPF;


/* TDA18273 Register CP_Current_byte 0x2F */
extern const TDA18273_BitField_t gTDA18273_Reg_CP_Current_byte;
/* LO_CP_Current bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_CP_Current_byte__LO_CP_Current;
/* N_CP_Current bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_CP_Current_byte__N_CP_Current;


/* TDA18273 Register AGCs_DetOut_byte 0x30 */
extern const TDA18273_BitField_t gTDA18273_Reg_AGCs_DetOut_byte;
/* Up_AGC5 bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_AGCs_DetOut_byte__Up_AGC5;
/* Do_AGC5 bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_AGCs_DetOut_byte__Do_AGC5;
/* Up_AGC4 bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_AGCs_DetOut_byte__Up_AGC4;
/* Do_AGC4 bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_AGCs_DetOut_byte__Do_AGC4;
/* Up_AGC2 bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_AGCs_DetOut_byte__Up_AGC2;
/* Do_AGC2 bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_AGCs_DetOut_byte__Do_AGC2;
/* Up_AGC1 bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_AGCs_DetOut_byte__Up_AGC1;
/* Do_AGC1 bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_AGCs_DetOut_byte__Do_AGC1;


/* TDA18273 Register RFAGCs_Gain_byte_3 0x31 */
extern const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_3;
/* AGC2_Gain_Read bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_3__AGC2_Gain_Read;
/* AGC1_Gain_Read bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_3__AGC1_Gain_Read;


/* TDA18273 Register RFAGCs_Gain_byte_4 0x32 */
extern const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_4;
/* Cprog_Read bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_4__Cprog_Read;


/* TDA18273 Register RFAGCs_Gain_byte_5 0x33 */
extern const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_5;
/* RFAGC_Read_K_8 bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_5__RFAGC_Read_K_8;
/* Do_AGC1bis bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_5__Do_AGC1bis;
/* AGC1_Top_Adapt_Low bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_5__AGC1_Top_Adapt_Low;
/* Up_LNA_Adapt bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_5__Up_LNA_Adapt;
/* Do_LNA_Adapt bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_5__Do_LNA_Adapt;
/* TOP_AGC3_Read bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_5__TOP_AGC3_Read;


/* TDA18273 Register RFAGCs_Gain_byte_6 0x34 */
extern const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_6;
/* RFAGC_Read_K bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RFAGCs_Gain_byte_6__RFAGC_Read_K;


/* TDA18273 Register IFAGCs_Gain_byte 0x35 */
extern const TDA18273_BitField_t gTDA18273_Reg_IFAGCs_Gain_byte;
/* AGC5_Gain_Read bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_IFAGCs_Gain_byte__AGC5_Gain_Read;
/* AGC4_Gain_Read bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_IFAGCs_Gain_byte__AGC4_Gain_Read;


/* TDA18273 Register RSSI_byte_1 0x36 */
extern const TDA18273_BitField_t gTDA18273_Reg_RSSI_byte_1;
/* RSSI bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RSSI_byte_1__RSSI;


/* TDA18273 Register RSSI_byte_2 0x37 */
extern const TDA18273_BitField_t gTDA18273_Reg_RSSI_byte_2;
/* RSSI_AV bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RSSI_byte_2__RSSI_AV;
/* RSSI_Cap_Reset_En bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RSSI_byte_2__RSSI_Cap_Reset_En;
/* RSSI_Cap_Val bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RSSI_byte_2__RSSI_Cap_Val;
/* RSSI_Ck_Speed bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RSSI_byte_2__RSSI_Ck_Speed;
/* RSSI_Dicho_not bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_RSSI_byte_2__RSSI_Dicho_not;


/* TDA18273 Register Misc_byte 0x38 */
extern const TDA18273_BitField_t gTDA18273_Reg_Misc_byte;
/* RFCALPOR_I2C bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_Misc_byte__RFCALPOR_I2C;
/* PD_Underload bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_Misc_byte__PD_Underload;
/* DDS_Polarity bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_Misc_byte__DDS_Polarity;
/* IRQ_Mode bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_Misc_byte__IRQ_Mode;
/* IRQ_Polarity bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_Misc_byte__IRQ_Polarity;


/* TDA18273 Register rfcal_log_0 0x39 */
extern const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_0;
/* rfcal_log_0 bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_0__rfcal_log_0;


/* TDA18273 Register rfcal_log_1 0x3A */
extern const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_1;
/* rfcal_log_1 bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_1__rfcal_log_1;


/* TDA18273 Register rfcal_log_2 0x3B */
extern const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_2;
/* rfcal_log_2 bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_2__rfcal_log_2;


/* TDA18273 Register rfcal_log_3 0x3C */
extern const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_3;
/* rfcal_log_3 bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_3__rfcal_log_3;


/* TDA18273 Register rfcal_log_4 0x3D */
extern const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_4;
/* rfcal_log_4 bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_4__rfcal_log_4;


/* TDA18273 Register rfcal_log_5 0x3E */
extern const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_5;
/* rfcal_log_5 bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_5__rfcal_log_5;


/* TDA18273 Register rfcal_log_6 0x3F */
extern const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_6;
/* rfcal_log_6 bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_6__rfcal_log_6;


/* TDA18273 Register rfcal_log_7 0x40 */
extern const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_7;
/* rfcal_log_7 bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_7__rfcal_log_7;


/* TDA18273 Register rfcal_log_8 0x41 */
extern const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_8;
/* rfcal_log_8 bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_8__rfcal_log_8;


/* TDA18273 Register rfcal_log_9 0x42 */
extern const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_9;
/* rfcal_log_9 bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_9__rfcal_log_9;


/* TDA18273 Register rfcal_log_10 0x43 */
extern const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_10;
/* rfcal_log_10 bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_10__rfcal_log_10;


/* TDA18273 Register rfcal_log_11 0x44 */
extern const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_11;
/* rfcal_log_11 bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_rfcal_log_11__rfcal_log_11;



/* TDA18273 Register Main_Post_Divider_byte 0x51 */
extern const TDA18273_BitField_t gTDA18273_Reg_Main_Post_Divider_byte;
/* LOPostDiv bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_Main_Post_Divider_byte__LOPostDiv;
/* LOPresc bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_Main_Post_Divider_byte__LOPresc;


/* TDA18273 Register Sigma_delta_byte_1 0x52 */
extern const TDA18273_BitField_t gTDA18273_Reg_Sigma_delta_byte_1;
/* LO_Int bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_Sigma_delta_byte_1__LO_Int;


/* TDA18273 Register Sigma_delta_byte_2 0x53 */
extern const TDA18273_BitField_t gTDA18273_Reg_Sigma_delta_byte_2;
/* LO_Frac_2 bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_Sigma_delta_byte_2__LO_Frac_2;


/* TDA18273 Register Sigma_delta_byte_3 0x54 */
extern const TDA18273_BitField_t gTDA18273_Reg_Sigma_delta_byte_3;
/* LO_Frac_1 bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_Sigma_delta_byte_3__LO_Frac_1;


/* TDA18273 Register Sigma_delta_byte_4 0x55 */
extern const TDA18273_BitField_t gTDA18273_Reg_Sigma_delta_byte_4;
/* LO_Frac_0 bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_Sigma_delta_byte_4__LO_Frac_0;


/* TDA18273 Register Sigma_delta_byte_5 0x56 */
extern const TDA18273_BitField_t gTDA18273_Reg_Sigma_delta_byte_5;
/* N_K_correct_manual bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_Sigma_delta_byte_5__N_K_correct_manual;
/* LO_Calc_Disable bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_Sigma_delta_byte_5__LO_Calc_Disable;


/* TDA18273 Register Regulators_byte 0x58 */
extern const TDA18273_BitField_t gTDA18273_Reg_Regulators_byte;
/* RF_Reg bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_Regulators_byte__RF_Reg;


/* TDA18273 Register IR_Cal_byte_5 0x5B */
extern const TDA18273_BitField_t gTDA18273_Reg_IR_Cal_byte_5;
/* Mixer_Gain_Bypass bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_IR_Cal_byte_5__Mixer_Gain_Bypass;
/* IR_Mixer_Gain bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_IR_Cal_byte_5__IR_Mixer_Gain;


/* TDA18273 Register Power_Down_byte_2 0x5F */
extern const TDA18273_BitField_t gTDA18273_Reg_Power_Down_byte_2;
/* PD_LNA bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_Power_Down_byte_2__PD_LNA;
/* PD_Det4 bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_Power_Down_byte_2__PD_Det4;
/* PD_Det3 bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_Power_Down_byte_2__PD_Det3;
/* PD_Det1 bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_Power_Down_byte_2__PD_Det1;

/* TDA18273 Register Power_Down_byte_3 0x60 */
extern const TDA18273_BitField_t gTDA18273_Reg_Power_Down_byte_3;
/* Force_Soft_Reset bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_Power_Down_byte_3__Force_Soft_Reset;
/* Soft_Reset bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_Power_Down_byte_3__Soft_Reset;

/* TDA18273 Register Charge_pump_byte 0x64 */
extern const TDA18273_BitField_t gTDA18273_Reg_Charge_pump_byte;
/* ICP_Bypass bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_Charge_pump_byte__ICP_Bypass;
/* ICP bit(s):  */
extern const TDA18273_BitField_t gTDA18273_Reg_Charge_pump_byte__ICP;



#if 0

/* "C" Bit-fields definitions only for debug */

/* TDA18273 Register ID_byte_1 0x00 */
typedef union
{
    UInt8 ID_byte_1;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 MS : 1;
        UInt8 Ident_1 : 7;
#else
        UInt8 Ident_1 : 7;
        UInt8 MS : 1;
#endif
    }bF;
}TDA18273_Reg_ID_byte_1;
#define TDA18273_REG_ADD_ID_byte_1 (0x00)
#define TDA18273_REG_SZ_ID_byte_1  (0x01)

/* TDA18273 Register ID_byte_2 0x01 */
typedef union
{
    UInt8 ID_byte_2;
    struct
    {
        UInt8 Ident_2 : 8;
    }bF;
}TDA18273_Reg_ID_byte_2;
#define TDA18273_REG_ADD_ID_byte_2 (0x01)
#define TDA18273_REG_SZ_ID_byte_2  (0x01)

/* TDA18273 Register ID_byte_3 0x02 */
typedef union
{
    UInt8 ID_byte_3;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 Major_rev : 4;
        UInt8 Minor_rev : 4;
#else
        UInt8 Minor_rev : 4;
        UInt8 Major_rev : 4;
#endif
    }bF;
}TDA18273_Reg_ID_byte_3;
#define TDA18273_REG_ADD_ID_byte_3 (0x02)
#define TDA18273_REG_SZ_ID_byte_3  (0x01)

/* TDA18273 Register Thermo_byte_1 0x03 */
typedef union
{
    UInt8 Thermo_byte_1;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 UNUSED_D7 : 1;
        UInt8 TM_D : 7;
#else
        UInt8 TM_D : 7;
        UInt8 UNUSED_D7 : 1;
#endif
    }bF;
}TDA18273_Reg_Thermo_byte_1;
#define TDA18273_REG_ADD_Thermo_byte_1 (0x03)
#define TDA18273_REG_SZ_Thermo_byte_1  (0x01)

/* TDA18273 Register Thermo_byte_2 0x04 */
typedef union
{
    UInt8 Thermo_byte_2;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 UNUSED_D7_1 : 7;
        UInt8 TM_ON : 1;
#else
        UInt8 TM_ON : 1;
        UInt8 UNUSED_D7_1 : 7;
#endif
    }bF;
}TDA18273_Reg_Thermo_byte_2;
#define TDA18273_REG_ADD_Thermo_byte_2 (0x04)
#define TDA18273_REG_SZ_Thermo_byte_2  (0x01)

/* TDA18273 Register Power_state_byte_1 0x05 */
typedef union
{
    UInt8 Power_state_byte_1;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 POR : 1;
        UInt8 UNUSED_D6_D3 : 4;
        UInt8 AGCs_Lock : 1;
        UInt8 Vsync_Lock : 1;
        UInt8 LO_Lock : 1;

#else
        UInt8 LO_Lock : 1;
        UInt8 Vsync_Lock : 1;
        UInt8 AGCs_Lock : 1;
        UInt8 UNUSED_D6_D3 : 4;
        UInt8 POR : 1;
#endif
    }bF;
}TDA18273_Reg_Power_state_byte_1;
#define TDA18273_REG_ADD_Power_state_byte_1 (0x05)
#define TDA18273_REG_SZ_Power_state_byte_1  (0x01)

/* TDA18273 Register Power_state_byte_2 0x06 */
typedef union
{
    UInt8 Power_state_byte_2;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 UNUSED_D7_2 : 6;
        UInt8 SM : 1;
        UInt8 SM_XT : 1;
#else
        UInt8 SM_XT : 1;
        UInt8 SM : 1;
        UInt8 UNUSED_D7_2 : 6;
#endif
    }bF;
}TDA18273_Reg_Power_state_byte_2;
#define TDA18273_REG_ADD_Power_state_byte_2 (0x06)
#define TDA18273_REG_SZ_Power_state_byte_2  (0x01)

/* TDA18273 Register Input_Power_Level_byte 0x07 */
typedef union
{
    UInt8 Input_Power_Level_byte;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 Power_Level : 8;
#else
        UInt8 Power_Level : 8;
#endif
    }bF;
}TDA18273_Reg_Input_Power_Level_byte;
#define TDA18273_REG_ADD_Input_Power_Level_byte (0x07)
#define TDA18273_REG_SZ_Input_Power_Level_byte  (0x01)

/* TDA18273 Register IRQ_status 0x08 */
typedef union
{
    UInt8 IRQ_status;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 IRQ_status : 1;
        UInt8 UNUSED_D6 : 1;
        UInt8 MSM_XtalCal_End : 1;
        UInt8 MSM_RSSI_End : 1;
        UInt8 MSM_LOCalc_End : 1;
        UInt8 MSM_RFCal_End : 1;
        UInt8 MSM_IRCAL_End : 1;
        UInt8 MSM_RCCal_End : 1;
#else
        UInt8 MSM_RCCal_End : 1;
        UInt8 MSM_IRCAL_End : 1;
        UInt8 MSM_RFCal_End : 1;
        UInt8 MSM_LOCalc_End : 1;
        UInt8 MSM_RSSI_End : 1;
        UInt8 MSM_XtalCal_End : 1;
        UInt8 UNUSED_D6 : 1;
        UInt8 IRQ_status : 1;
#endif
    }bF;
}TDA18273_Reg_IRQ_status;
#define TDA18273_REG_ADD_IRQ_status (0x08)
#define TDA18273_REG_SZ_IRQ_status  (0x01)

/* TDA18273 Register IRQ_enable 0x09 */
typedef union
{
    UInt8 IRQ_enable;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 IRQ_Enable : 1;
        UInt8 UNUSED_D6 : 1;
        UInt8 XtalCal_Enable : 1;
        UInt8 MSM_RSSI_Enable : 1;
        UInt8 MSM_LOCalc_Enable : 1;
        UInt8 MSM_RFCAL_Enable : 1;
        UInt8 MSM_IRCAL_Enable : 1;
        UInt8 MSM_RCCal_Enable : 1;
#else
        UInt8 MSM_RCCal_Enable : 1;
        UInt8 MSM_IRCAL_Enable : 1;
        UInt8 MSM_RFCAL_Enable : 1;
        UInt8 MSM_LOCalc_Enable : 1;
        UInt8 MSM_RSSI_Enable : 1;
        UInt8 XtalCal_Enable : 1;
        UInt8 UNUSED_D6 : 1;
        UInt8 IRQ_Enable : 1;
#endif
    }bF;
}TDA18273_Reg_IRQ_enable;
#define TDA18273_REG_ADD_IRQ_enable (0x09)
#define TDA18273_REG_SZ_IRQ_enable  (0x01)

/* TDA18273 Register IRQ_clear 0x0A */
typedef union
{
    UInt8 IRQ_clear;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 IRQ_Clear : 1;
        UInt8 UNUSED_D6 : 1;
        UInt8 XtalCal_Clear : 1;
        UInt8 MSM_RSSI_Clear : 1;
        UInt8 MSM_LOCalc_Clear : 1;
        UInt8 MSM_RFCal_Clear : 1;
        UInt8 MSM_IRCAL_Clear : 1;
        UInt8 MSM_RCCal_Clear : 1;
#else
        UInt8 MSM_RCCal_Clear : 1;
        UInt8 MSM_IRCAL_Clear : 1;
        UInt8 MSM_RFCal_Clear : 1;
        UInt8 MSM_LOCalc_Clear : 1;
        UInt8 MSM_RSSI_Clear : 1;
        UInt8 XtalCal_Clear : 1;
        UInt8 UNUSED_D6 : 1;
        UInt8 IRQ_Clear : 1;
#endif
    }bF;
}TDA18273_Reg_IRQ_clear;
#define TDA18273_REG_ADD_IRQ_clear (0x0A)
#define TDA18273_REG_SZ_IRQ_clear  (0x01)

/* TDA18273 Register IRQ_set 0x0B */
typedef union
{
    UInt8 IRQ_set;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 IRQ_Set : 1;
        UInt8 UNUSED_D6 : 1;
        UInt8 XtalCal_Set : 1;
        UInt8 MSM_RSSI_Set : 1;
        UInt8 MSM_LOCalc_Set : 1;
        UInt8 MSM_RFCal_Set : 1;
        UInt8 MSM_IRCAL_Set : 1;
        UInt8 MSM_RCCal_Set : 1;
#else
        UInt8 MSM_RCCal_Set : 1;
        UInt8 MSM_IRCAL_Set : 1;
        UInt8 MSM_RFCal_Set : 1;
        UInt8 MSM_LOCalc_Set : 1;
        UInt8 MSM_RSSI_Set : 1;
        UInt8 XtalCal_Set : 1;
        UInt8 UNUSED_D6 : 1;
        UInt8 IRQ_Set : 1;
#endif
    }bF;
}TDA18273_Reg_IRQ_set;
#define TDA18273_REG_ADD_IRQ_set (0x0B)
#define TDA18273_REG_SZ_IRQ_set  (0x01)

/* TDA18273 Register AGC1_byte_1 0x0C */
typedef union
{
    UInt8 AGC1_byte_1;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 UNUSED_D7_4 : 4;
        UInt8 AGC1_TOP : 4;
#else
        UInt8 AGC1_TOP : 4;
        UInt8 UNUSED_D7_4 : 4;
#endif
    }bF;
}TDA18273_Reg_AGC1_byte_1;
#define TDA18273_REG_ADD_AGC1_byte_1 (0xOC)
#define TDA18273_REG_SZ_AGC1_byte_1  (0x01)

/* TDA18273 Register AGC1_byte_2 0x0D */
typedef union
{
    UInt8 AGC1_byte_2;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 UNUSED_D7_5 : 3;
        UInt8 AGC1_Top_Mode_Val : 2;
        UInt8 AGC1_Top_Mode : 3;
#else
        UInt8 AGC1_Top_Mode : 3;
        UInt8 AGC1_Top_Mode_Val : 2;
        UInt8 UNUSED_D7_5 : 3;
#endif
    }bF;
}TDA18273_Reg_AGC1_byte_2;
#define TDA18273_REG_ADD_AGC1_byte_2 (0xOD)
#define TDA18273_REG_SZ_AGC1_byte_2  (0x01)

/* TDA18273 Register AGC2_byte_1 0xOE */
typedef union
{
    UInt8 AGC2_byte_1;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 UNUSED_D7_5 : 3;
        UInt8 AGC2_TOP : 5;
#else
        UInt8 AGC2_TOP : 5;
        UInt8 UNUSED_D7_5 : 3;
#endif
    }bF;
}TDA18273_Reg_AGC2_byte_1;
#define TDA18273_REG_ADD_AGC2_byte_1 (0x0E)
#define TDA18273_REG_SZ_AGC2_byte_1  (0x01)

/* TDA18273 Register AGCK_byte_1 0x0F */
typedef union
{
    UInt8 AGCK_byte_1;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 AGCs_Up_Step_assym : 2;
        UInt8 UNUSED_D5 : 1;
        UInt8 Pulse_Shaper_Disable : 1;
        UInt8 AGCK_Step : 2;
        UInt8 AGCK_Mode : 2;
#else
        UInt8 AGCK_Mode : 2;
        UInt8 AGCK_Step : 2;
        UInt8 Pulse_Shaper_Disable : 1;
        UInt8 UNUSED_D5 : 1;
        UInt8 AGCs_Up_Step_assym : 2;
#endif
    }bF;
}TDA18273_Reg_AGCK_byte_1;
#define TDA18273_REG_ADD_AGCK_byte_1 (0x0F)
#define TDA18273_REG_SZ_AGCK_byte_1  (0x01)

/* TDA18273 Register RF_AGC_byte 0x10 */
typedef union
{
    UInt8 RF_AGC_byte;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 PD_AGC_Adapt3x : 2;
        UInt8 RFAGC_Adapt_TOP : 2;
        UInt8 RFAGC_Low_BW : 1;
        UInt8 RFAGC_Top : 3;
#else
        UInt8 RFAGC_Top : 3;
        UInt8 RFAGC_Low_BW : 1;
        UInt8 RFAGC_Adapt_TOP : 2;
        UInt8 PD_AGC_Adapt3x : 2;
#endif
    }bF;
}TDA18273_Reg_RF_AGC_byte;
#define TDA18273_REG_ADD_RF_AGC_byte (0x10)
#define TDA18273_REG_SZ_RF_AGC_byte  (0x01)

/* TDA18273 Register W_Filter_byte 0x11 */
typedef union
{
    UInt8 W_Filter_byte;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 VHF_III_mode : 1;
        UInt8 RF_Atten_3dB : 1;
        UInt8 W_Filter_Enable : 1;
        UInt8 W_Filter_Bypass : 1;
        UInt8 W_Filter : 2;
        UInt8 W_Filter_Offset : 2;
#else
        UInt8 W_Filter_Offset : 2;
        UInt8 W_Filter : 2;
        UInt8 W_Filter_Bypass : 1;
        UInt8 W_Filter_Enable : 1;
        UInt8 RF_Atten_3dB : 1;
        UInt8 VHF_III_mode : 1;
#endif
    }bF;
}TDA18273_Reg_W_Filter_byte;
#define TDA18273_REG_ADD_W_Filter_byte (0x11)
#define TDA18273_REG_SZ_W_Filter_byte  (0x01)

/* TDA18273 Register IR_Mixer_byte_1 0x12 */
typedef union
{
    UInt8 IR_Mixer_byte_1;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 UNUSED_D7_6 : 2;
        UInt8 S2D_Gain : 2;
        UInt8 IR_Mixer_Top : 4;
#else
        UInt8 IR_Mixer_Top : 4;
        UInt8 S2D_Gain : 2;
        UInt8 UNUSED_D7_6 : 2;
#endif
    }bF;
}TDA18273_Reg_IR_Mixer_byte_1;
#define TDA18273_REG_ADD_IR_Mixer_byte_1 (0x12)
#define TDA18273_REG_SZ_IR_Mixer_byte_1  (0x01)

/* TDA18273 Register AGC5_byte_1 0x13 */
typedef union
{
    UInt8 AGC5_byte_1;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 UNUSED_D7 : 1;
        UInt8 AGCs_Do_Step_assym : 2;
        UInt8 AGC5_Ana : 1;
        UInt8 AGC5_TOP : 4;
#else
        UInt8 AGC5_TOP : 4;
        UInt8 AGC5_Ana : 1;
        UInt8 AGCs_Do_Step_assym : 2;
        UInt8 UNUSED_D7 : 1;
#endif
    }bF;
}TDA18273_Reg_AGC5_byte_1;
#define TDA18273_REG_ADD_AGC5_byte_1 (0x13)
#define TDA18273_REG_SZ_AGC5_byte_1  (0x01)


/* TDA18273 Register IF_AGC_byte 0x14 */
typedef union
{
    UInt8 IF_AGC_byte;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 IFnotchToRSSI:1;
        UInt8 LPF_DCOffset_Corr:1;
        UInt8 UNUSED_D5_3 : 3;
        UInt8 IF_level : 3;
#else
        UInt8 IF_level : 3;
        UInt8 UNUSED_D5_3 : 3;
        UInt8 LPF_DCOffset_Corr : 1;
        UInt8 IFnotchToRSSI : 1;
#endif
    }bF;
}TDA18273_Reg_IF_AGC_byte;
#define TDA18273_REG_ADD_IF_AGC_byte (0x14)
#define TDA18273_REG_SZ_IF_AGC_byte  (0x01)

/* TDA18273 Register IF_Byte_1 0x15 */
typedef union
{
    UInt8 IF_Byte_1;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 IF_HP_Fc : 2;
        UInt8 IF_ATSC_Notch : 1;
        UInt8 LP_FC_Offset : 2;
        UInt8 LP_Fc : 3;
#else
        UInt8 LP_Fc : 3;
        UInt8 LP_FC_Offset : 2;
        UInt8 IF_ATSC_Notch : 1;
        UInt8 IF_HP_Fc : 2;
#endif
    }bF;
}TDA18273_Reg_IF_Byte_1;
#define TDA18273_REG_ADD_IF_Byte_1 (0x15)
#define TDA18273_REG_SZ_IF_Byte_1  (0x01)

/* TDA18273 Register Reference_Byte 0x16 */
typedef union
{
    UInt8 Reference_Byte;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 Digital_Clock_Mode : 2;
        UInt8 UNUSED_D5_2 : 4;
        UInt8 XTout : 2;
#else
        UInt8 XTout : 2;
        UInt8 UNUSED_D5_2 : 4;
        UInt8 Digital_Clock_Mode : 2;
#endif
    }bF;
}TDA18273_Reg_Reference_Byte;
#define TDA18273_REG_ADD_Reference_Byte (0x16)
#define TDA18273_REG_SZ_Reference_Byte  (0x01)

/* TDA18273 Register IF_Frequency_byte 0x17 */
typedef union
{
    UInt8 IF_Frequency_byte;
    struct
    {
        UInt8 IF_Freq : 8;
    }bF;
}TDA18273_Reg_IF_Frequency_byte;
#define TDA18273_REG_ADD_IF_Frequency_byte (0x17)
#define TDA18273_REG_SZ_IF_Frequency_byte  (0x01)


/* TDA18273 Register RF_Frequency_byte_1 0x18 */
typedef union
{
    UInt8 RF_Frequency_byte_1;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 UNUSED_D7_4 : 4;
        UInt8 RF_Freq_1 : 4;
#else
        UInt8 RF_Freq_1 : 4;
        UInt8 UNUSED_D7_4 : 4;
#endif
    }bF;
}TDA18273_Reg_RF_Frequency_byte_1;
#define TDA18273_REG_ADD_RF_Frequency_byte_1 (0x18)
#define TDA18273_REG_SZ_RF_Frequency_byte_1  (0x01)

/* TDA18273 Register RF_Frequency_byte_2 0x19 */
typedef union
{
    UInt8 RF_Frequency_byte_2;
    struct
    {
        UInt8 RF_Freq_2 : 8;
    }bF;
}TDA18273_Reg_RF_Frequency_byte_2;
#define TDA18273_REG_ADD_RF_Frequency_byte_2 (0x19)
#define TDA18273_REG_SZ_RF_Frequency_byte_2  (0x01)

/* TDA18273 Register RF_Frequency_byte_3 0x1A */
typedef union
{
    UInt8 RF_Frequency_byte_3;
    struct
    {
        UInt8 RF_Freq_3 : 8;
    }bF;
}TDA18273_Reg_RF_Frequency_byte_3;
#define TDA18273_REG_ADD_RF_Frequency_byte_3 (0x1A)
#define TDA18273_REG_SZ_RF_Frequency_byte_3  (0x01)

/* TDA18273 Register MSM_byte_1 0x1B */
typedef union
{
    UInt8 MSM_byte_1;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 RSSI_Meas : 1;
        UInt8 RF_CAL_AV : 1;
        UInt8 RF_CAL : 1;
        UInt8 IR_CAL_Loop : 1;
        UInt8 IR_Cal_Image : 1;
        UInt8 IR_CAL_Wanted : 1;
        UInt8 RC_Cal : 1;
        UInt8 Calc_PLL : 1;
#else
        UInt8 Calc_PLL : 1;
        UInt8 RC_Cal : 1;
        UInt8 IR_CAL_Wanted : 1;
        UInt8 IR_Cal_Image : 1;
        UInt8 IR_CAL_Loop : 1;
        UInt8 RF_CAL : 1;
        UInt8 RF_CAL_AV : 1;
        UInt8 RSSI_Meas : 1;
#endif
    }bF;
}TDA18273_Reg_MSM_byte_1;
#define TDA18273_REG_ADD_MSM_byte_1 (0x1B)
#define TDA18273_REG_SZ_MSM_byte_1  (0x01)

/* TDA18273 Register MSM_byte_2 0x1C */
typedef union
{
    UInt8 MSM_byte_2;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 UNUSED_D7_2 : 6;
        UInt8 XtalCal_Launch : 1;
        UInt8 MSM_Launch : 1;
#else
        UInt8 MSM_Launch : 1;
        UInt8 XtalCal_Launch : 1;
        UInt8 UNUSED_D7_2 : 6;
#endif
    }bF;
}TDA18273_Reg_MSM_byte_2;
#define TDA18273_REG_ADD_MSM_byte_2 (0x1C)
#define TDA18273_REG_SZ_MSM_byte_2  (0x01)

/* TDA18273 Register PowerSavingMode 0x1D */
typedef union
{
    UInt8 PowerSavingMode;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 PSM_AGC1 : 1;
        UInt8 PSM_Bandsplit_Filter : 2;
        UInt8 PSM_RFpoly : 1;
        UInt8 PSM_Mixer : 1;
        UInt8 PSM_Ifpoly : 1;
        UInt8 PSM_Lodriver : 2;
#else
        UInt8 PSM_Lodriver : 2;
        UInt8 PSM_Ifpoly : 1;
        UInt8 PSM_Mixer : 1;
        UInt8 PSM_RFpoly : 1;
        UInt8 PSM_Bandsplit_Filter : 2;
        UInt8 PSM_AGC1 : 1;
#endif
    }bF;
}TDA18273_Reg_PowerSavingMode;
#define TDA18273_REG_ADD_PowerSavingMode (0x1D)
#define TDA18273_REG_SZ_PowerSavingMode  (0x01)

/* TDA18273 Register Power_Level_byte_2 0x1E */
typedef union
{
    UInt8 Power_Level_byte_2;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 PD_PLD_read : 1;
        UInt8 PLD_Temp_Slope : 2;
        UInt8 PLD_Gain_Corr : 5;
#else
        UInt8 PLD_Gain_Corr : 5;
        UInt8 PLD_Temp_Slope : 2;
        UInt8 PD_PLD_read : 1;
#endif
    }bF;
}TDA18273_Reg_Power_Level_byte_2;
#define TDA18273_REG_ADD_Power_Level_byte_2 (0x1E)
#define TDA18273_REG_SZ_Power_Level_byte_2  (0x01)

/* TDA18273 Register Adapt_Top_byte 0x1F */
typedef union
{
    UInt8 Adapt_Top_byte;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 UNUSED_D7 : 1;
        UInt8 Fast_Mode_AGC : 1;
        UInt8 Range_LNA_Adapt : 1;
        UInt8 Index_K_LNA_Adapt : 2;
        UInt8 Index_K_Top_Adapt : 2;
        UInt8 Ovld_Udld_FastUp : 1;
#else
        UInt8 Ovld_Udld_FastUp : 1;
        UInt8 Index_K_Top_Adapt : 2;
        UInt8 Index_K_LNA_Adapt : 2;
        UInt8 Range_LNA_Adapt : 1;
        UInt8 Fast_Mode_AGC : 1;
        UInt8 UNUSED_D7 : 1;
#endif

    }bF;
}TDA18273_Reg_Adapt_Top_byte;
#define TDA18273_REG_ADD_Adapt_Top_byte (0x1F)
#define TDA18273_REG_SZ_Adapt_Top_byte  (0x01)

/* TDA18273 Register Vsync_byte 0x20 */
typedef union
{
    UInt8 Vsync_byte;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 Neg_modulation:1;
        UInt8 Tracer_Step:2;
        UInt8 Vsync_int:1;
        UInt8 Vsync_Thresh:2;
        UInt8 Vsync_Len:2;
#else
        UInt8 Vsync_Len:2;
        UInt8 Vsync_Thresh:2;
        UInt8 Vsync_int:1;
        UInt8 Tracer_Step:2;
        UInt8 Neg_modulation:1;
#endif
    }bF;
}TDA18273_Reg_Vsync_byte;
#define TDA18273_REG_ADD_Vsync_byte (0x20)
#define TDA18273_REG_SZ_Vsync_byte  (0x01)

/* TDA18273 Register Vsync_Mgt_byte 0x21 */
typedef union
{
    UInt8 Vsync_Mgt_byte;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 PD_Vsync_Mgt : 1;
        UInt8 PD_Ovld : 1;
        UInt8 PD_Ovld_RF : 1;
        UInt8 AGC_Ovld_TOP : 3;
        UInt8 Up_Step_Ovld : 1;
        UInt8 AGC_Ovld_Timer : 1;
#else
        UInt8 AGC_Ovld_Timer : 1;
        UInt8 Up_Step_Ovld : 1;
        UInt8 AGC_Ovld_TOP : 3;
        UInt8 PD_Ovld_RF : 1;
        UInt8 PD_Ovld : 1;
        UInt8 PD_Vsync_Mgt : 1;
#endif
    }bF;
}TDA18273_Reg_Vsync_Mgt_byte;
#define TDA18273_REG_ADD_Vsync_Mgt_byte (0x21)
#define TDA18273_REG_SZ_Vsync_Mgt_byte  (0x01)


/* TDA18273 Register IR_Mixer_byte_2 0x22 */
typedef union
{
    UInt8 IR_Mixer_byte_2;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 IR_Mixer_loop_off : 1;
        UInt8 IR_Mixer_Do_step : 2;
        UInt8 UNUSED_D4_2 : 3;
        UInt8 Hi_Pass : 1;
        UInt8 IF_Notch : 1;
#else
        UInt8 IF_Notch : 1;
        UInt8 Hi_Pass : 1;
        UInt8 UNUSED_D4_2 : 3;
        UInt8 IR_Mixer_Do_step : 2;
        UInt8 IR_Mixer_loop_off : 1;
#endif
    }bF;
}TDA18273_Reg_IR_Mixer_byte_2;
#define TDA18273_REG_ADD_IR_Mixer_byte_2 (0x22)
#define TDA18273_REG_SZ_IR_Mixer_byte_2  (0x01)

/* TDA18273 Register AGC1_byte_3 0x23 */
typedef union
{
    UInt8 AGC1_byte_3;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 AGC1_loop_off : 1;
        UInt8 AGC1_Do_step : 2;
        UInt8 Force_AGC1_gain : 1;
        UInt8 AGC1_Gain : 4;
#else
        UInt8 AGC1_Gain : 4;
        UInt8 Force_AGC1_gain : 1;
        UInt8 AGC1_Do_step : 2;
        UInt8 AGC1_loop_off : 1;
#endif
    }bF;
}TDA18273_Reg_AGC1_byte_3;
#define TDA18273_REG_ADD_AGC1_byte_3 (0x23)
#define TDA18273_REG_SZ_AGC1_byte_3  (0x01)

/* TDA18273 Register RFAGCs_Gain_byte_1 0x24 */
typedef union
{
    UInt8 RFAGCs_Gain_byte_1;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 PLD_DAC_Scale : 1;
        UInt8 PLD_CC_Enable : 1;
        UInt8 PLD_Temp_Enable : 1;
        UInt8 TH_AGC_Adapt34 : 1;
        UInt8 UNUSED_D3 : 1;
        UInt8 RFAGC_Sense_Enable : 1;
        UInt8 RFAGC_K_Bypass : 1;
        UInt8 RFAGC_K_8 : 1;
#else
        UInt8 RFAGC_K_8 : 1;
        UInt8 RFAGC_K_Bypass : 1;
        UInt8 RFAGC_Sense_Enable : 1;
        UInt8 UNUSED_D3 : 1;
        UInt8 TH_AGC_Adapt34 : 1;
        UInt8 PLD_Temp_Enable : 1;
        UInt8 PLD_CC_Enable : 1;
        UInt8 PLD_DAC_Scale : 1;
#endif
    }bF;
}TDA18273_Reg_RFAGCs_Gain_byte_1;
#define TDA18273_REG_ADD_RFAGCs_Gain_byte_1 (0x24)
#define TDA18273_REG_SZ_RFAGCs_Gain_byte_1  (0x01)

/* TDA18273 Register RFAGCs_Gain_byte_2 0x25 */
typedef union
{
    UInt8 RFAGCs_Gain_byte_2;
    struct
    {
        UInt8 RFAGC_K : 8;
    }bF;
}TDA18273_Reg_RFAGCs_Gain_byte_2;
#define TDA18273_REG_ADD_RFAGCs_Gain_byte_2 (0x25)
#define TDA18273_REG_SZ_RFAGCs_Gain_byte_2  (0x01)

/* TDA18273 Register AGC5_byte_2 0x26 */
typedef union
{
    UInt8 AGC5_byte_2;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 AGC5_loop_off : 1;
        UInt8 AGC5_Do_step : 2;
        UInt8 UNUSED_D4 : 1;
        UInt8 Force_AGC5_gain : 1;
        UInt8 AGC5_Gain : 3;
#else
        UInt8 AGC5_Gain : 3;
        UInt8 Force_AGC5_gain : 1;
        UInt8 UNUSED_D4 : 1;
        UInt8 AGC5_Do_step : 2;
        UInt8 AGC5_loop_off : 1;
#endif
    }bF;
}TDA18273_Reg_AGC5_byte_2;
#define TDA18273_REG_ADD_AGC5_byte_2 (0x26)
#define TDA18273_REG_SZ_AGC5_byte_2  (0x01)

/* TDA18273 Register RF_Cal_byte_1 0x27 */
typedef union
{
    UInt8 RF_Cal_byte_1;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 RFCAL_Offset_Cprog0 : 2;
        UInt8 RFCAL_Offset_Cprog1 : 2;
        UInt8 RFCAL_Offset_Cprog2 : 2;
        UInt8 RFCAL_Offset_Cprog3 : 2;
#else
        UInt8 RFCAL_Offset_Cprog3 : 2;
        UInt8 RFCAL_Offset_Cprog2 : 2;
        UInt8 RFCAL_Offset_Cprog1 : 2;
        UInt8 RFCAL_Offset_Cprog0 : 2;
#endif
    }bF;
}TDA18273_Reg_RF_Cal_byte_1;
#define TDA18273_REG_ADD_RF_Cal_byte_1 (0x27)
#define TDA18273_REG_SZ_RF_Cal_byte_1  (0x01)

/* TDA18273 Register RF_Cal_byte_2 0x28 */
typedef union
{
    UInt8 RF_Cal_byte_2;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 RFCAL_Offset_Cprog4 : 2;
        UInt8 RFCAL_Offset_Cprog5 : 2;
        UInt8 RFCAL_Offset_Cprog6 : 2;
        UInt8 RFCAL_Offset_Cprog7 : 2;
#else
        UInt8 RFCAL_Offset_Cprog7 : 2;
        UInt8 RFCAL_Offset_Cprog6 : 2;
        UInt8 RFCAL_Offset_Cprog5 : 2;
        UInt8 RFCAL_Offset_Cprog4 : 2;
#endif
    }bF;
}TDA18273_Reg_RF_Cal_byte_2;
#define TDA18273_REG_ADD_RF_Cal_byte_2 (0x28)
#define TDA18273_REG_SZ_RF_Cal_byte_2  (0x01)

/* TDA18273 Register RF_Cal_byte_3 0x29 */
typedef union
{
    UInt8 RF_Cal_byte_3;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 RFCAL_Offset_Cprog8 : 2;
        UInt8 RFCAL_Offset_Cprog9 : 2;
        UInt8 RFCAL_Offset_Cprog10 : 2;
        UInt8 RFCAL_Offset_Cprog11 : 2;
#else
        UInt8 RFCAL_Offset_Cprog11 : 2;
        UInt8 RFCAL_Offset_Cprog10 : 2;
        UInt8 RFCAL_Offset_Cprog9 : 2;
        UInt8 RFCAL_Offset_Cprog8 : 2;
#endif
    }bF;
}TDA18273_Reg_RF_Cal_byte_3;
#define TDA18273_REG_ADD_RF_Cal_byte_3 (0x29)
#define TDA18273_REG_SZ_RF_Cal_byte_3  (0x01)

/* TDA18273 Register Bandsplit_Filter_byte 0x2A */
typedef union
{
    UInt8 Bandsplit_Filter_byte;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 UNUSED_D7_2:6;
        UInt8 Bandsplit_Filter_SubBand:2;
#else
        UInt8 Bandsplit_Filter_SubBand:2;
        UInt8 UNUSED_D7_2:6;
#endif
    }bF;
}TDA18273_Reg_Bandsplit_Filter_byte;
#define TDA18273_REG_ADD_Bandsplit_Filter_byte (0x2A)
#define TDA18273_REG_SZ_Bandsplit_Filter_byte  (0x01)

/* TDA18273 Register RF_Filters_byte_1 0x2B */
typedef union
{
    UInt8 RF_Filters_byte_1;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 RF_Filter_Bypass : 1;
        UInt8 UNUSED_D6 : 1;
        UInt8 AGC2_loop_off : 1;
        UInt8 Force_AGC2_gain : 1;
        UInt8 RF_Filter_Gv : 2;
        UInt8 RF_Filter_Band : 2;
#else

        UInt8 RF_Filter_Band : 2;
        UInt8 RF_Filter_Gv : 2;
        UInt8 Force_AGC2_gain : 1;
        UInt8 AGC2_loop_off : 1;
        UInt8 UNUSED_D6 : 1;
        UInt8 RF_Filter_Bypass : 1;
#endif
    }bF;
}TDA18273_Reg_RF_Filters_byte_1;
#define TDA18273_REG_ADD_RF_Filters_byte_1 (0x2B)
#define TDA18273_REG_SZ_RF_Filters_byte_1  (0x01)

/* TDA18273 Register RF_Filters_byte_2 0x2C */
typedef union
{
    UInt8 RF_Filters_byte_2;
    struct
    {
        UInt8 RF_Filter_Cap : 8;
    }bF;
}TDA18273_Reg_RF_Filters_byte_2;
#define TDA18273_REG_ADD_RF_Filters_byte_2 (0x2C)
#define TDA18273_REG_SZ_RF_Filters_byte_2  (0x01)

/* TDA18273 Register RF_Filters_byte_3 0x2D */
typedef union
{
    UInt8 RF_Filters_byte_3;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 AGC2_Do_step : 2;
        UInt8 Gain_Taper : 6;
#else
        UInt8 Gain_Taper : 6;
        UInt8 AGC2_Do_step : 2;
#endif
    }bF;
}TDA18273_Reg_RF_Filters_byte_3;
#define TDA18273_REG_ADD_RF_Filters_byte_3 (0x2D)
#define TDA18273_REG_SZ_RF_Filters_byte_3  (0x01)

/* TDA18273 Register RF_Band_Pass_Filter_byte 0x2E */
typedef union
{
    UInt8 RF_Band_Pass_Filter_byte;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 RF_BPF_Bypass : 1;
        UInt8 UNUSED_D6_3 : 4;
        UInt8 RF_BPF : 3;
#else
        UInt8 RF_BPF : 3;
        UInt8 UNUSED_D6_3 : 4;
        UInt8 RF_BPF_Bypass : 1;
#endif
    }bF;
}TDA18273_Reg_RF_Band_Pass_Filter_byte;
#define TDA18273_REG_ADD_RF_Band_Pass_Filter_byte (0x2E)
#define TDA18273_REG_SZ_RF_Band_Pass_Filter_byte  (0x01)

/* TDA18273 Register CP_Current_byte 0x2F */
typedef union
{
    UInt8 CP_Current_byte;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 LO_CP_Current : 1;
        UInt8 N_CP_Current : 7;
#else
        UInt8 N_CP_Current : 7;
        UInt8 LO_CP_Current : 1;
#endif
    }bF;
}TDA18273_Reg_CP_Current_byte;
#define TDA18273_REG_ADD_CP_Current_byte (0x2F)
#define TDA18273_REG_SZ_CP_Current_byte  (0x01)

/* TDA18273 Register AGCs_DetOut_byte 0x30 */
typedef union
{
    UInt8 AGCs_DetOut_byte;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 Up_AGC5 : 1;
        UInt8 Do_AGC5 : 1;
        UInt8 Up_AGC4 : 1;
        UInt8 Do_AGC4 : 1;
        UInt8 Up_AGC2 : 1;
        UInt8 Do_AGC2 : 1;
        UInt8 Up_AGC1 : 1;
        UInt8 Do_AGC1 : 1;
#else
        UInt8 Do_AGC1 : 1;
        UInt8 Up_AGC1 : 1;
        UInt8 Do_AGC2 : 1;
        UInt8 Up_AGC2 : 1;
        UInt8 Do_AGC4 : 1;
        UInt8 Up_AGC4 : 1;
        UInt8 Do_AGC5 : 1;
        UInt8 Up_AGC5 : 1;
#endif
    }bF;
}TDA18273_Reg_AGCs_DetOut_byte;
#define TDA18273_REG_ADD_AGCs_DetOut_byte (0x30)
#define TDA18273_REG_SZ_AGCs_DetOut_byte  (0x01)

/* TDA18273 Register RFAGCs_Gain_byte_3 0x31 */
typedef union
{
    UInt8 RFAGCs_Gain_byte_3;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 UNUSED_D7_6 : 2;
        UInt8 AGC2_Gain_Read : 2;
        UInt8 AGC1_Gain_Read : 4;
#else
        UInt8 AGC1_Gain_Read : 4;
        UInt8 AGC2_Gain_Read : 2;
        UInt8 UNUSED_D7_6 : 2;
#endif
    }bF;
}TDA18273_Reg_RFAGCs_Gain_byte_3;
#define TDA18273_REG_ADD_RFAGCs_Gain_byte_3 (0x31)
#define TDA18273_REG_SZ_RFAGCs_Gain_byte_3  (0x01)

/* TDA18273 Register RFAGCs_Gain_byte_4 0x32 */
typedef union
{
    UInt8 RFAGCs_Gain_byte_4;
    struct
    {
        UInt8 Cprog_Read : 8;
    }bF;
}TDA18273_Reg_RFAGCs_Gain_byte_4;
#define TDA18273_REG_ADD_RFAGCs_Gain_byte_4 (0x32)
#define TDA18273_REG_SZ_RFAGCs_Gain_byte_4  (0x01)


/* TDA18273 Register RFAGCs_Gain_byte_5 0x33 */
typedef union
{
    UInt8 RFAGCs_Gain_byte_5;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 RFAGC_Read_K_8 : 1;
        UInt8 Do_AGC1bis : 1;
        UInt8 AGC1_Top_Adapt_Low : 1;
        UInt8 Up_LNA_Adapt : 1;
        UInt8 Do_LNA_Adapt : 1;
        UInt8 TOP_AGC3_Read : 3;
#else
        UInt8 TOP_AGC3_Read : 3;
        UInt8 Do_LNA_Adapt : 1;
        UInt8 Up_LNA_Adapt : 1;
        UInt8 AGC1_Top_Adapt_Low : 1;
        UInt8 Do_AGC1bis : 1;
        UInt8 RFAGC_Read_K_8 : 1;
#endif
    }bF;
}TDA18273_Reg_RFAGCs_Gain_byte_5;
#define TDA18273_REG_ADD_RFAGCs_Gain_byte_5 (0x33)
#define TDA18273_REG_SZ_RFAGCs_Gain_byte_5  (0x01)

/* TDA18273 Register RFAGCs_Gain_byte_6 0x34 */
typedef union
{
    UInt8 RFAGCs_Gain_byte_6;
    struct
    {
        UInt8 RFAGC_Read_K : 8;
    }bF;
}TDA18273_Reg_RFAGCs_Gain_byte_6;
#define TDA18273_REG_ADD_RFAGCs_Gain_byte_6 (0x34)
#define TDA18273_REG_SZ_RFAGCs_Gain_byte_6  (0x01)

/* TDA18273 Register IFAGCs_Gain_byte 0x35 */
typedef union
{
    UInt8 IFAGCs_Gain_byte;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 UNUSED_D7_6 : 2;
        UInt8 AGC5_Gain_Read : 3;
        UInt8 AGC4_Gain_Read : 3;
#else
        UInt8 AGC4_Gain_Read : 3;
        UInt8 AGC5_Gain_Read : 3;
        UInt8 UNUSED_D7_6 : 2;
#endif
    }bF;
}TDA18273_Reg_IFAGCs_Gain_byte;
#define TDA18273_REG_ADD_IFAGCs_Gain_byte (0x35)
#define TDA18273_REG_SZ_IFAGCs_Gain_byte  (0x01)

/* TDA18273 Register RSSI_byte_1 0x36 */
typedef union
{
    UInt8 RSSI_byte_1;
    struct
    {
        UInt8 RSSI : 8;
    }bF;
}TDA18273_Reg_RSSI_byte_1;
#define TDA18273_REG_ADD_RSSI_byte_1 (0x36)
#define TDA18273_REG_SZ_RSSI_byte_1  (0x01)

/* TDA18273 Register RSSI_byte_2 0x37 */
typedef union
{
    UInt8 RSSI_byte_2;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 UNUSED_D7_6 : 2;
        UInt8 RSSI_AV : 1;
        UInt8 UNUSED_D4 : 1;
        UInt8 RSSI_Cap_Reset_En : 1;
        UInt8 RSSI_Cap_Val : 1;
        UInt8 RSSI_Ck_Speed : 1;
        UInt8 RSSI_Dicho_not : 1;
#else
        UInt8 RSSI_Dicho_not : 1;
        UInt8 RSSI_Ck_Speed : 1;
        UInt8 RSSI_Cap_Val : 1;
        UInt8 RSSI_Cap_Reset_En : 1;
        UInt8 UNUSED_D4 : 1;
        UInt8 RSSI_AV : 1;
        UInt8 UNUSED_D7_6 : 2;
#endif
    }bF;
}TDA18273_Reg_RSSI_byte_2;
#define TDA18273_REG_ADD_RSSI_byte_2 (0x37)
#define TDA18273_REG_SZ_RSSI_byte_2  (0x01)

/* TDA18273 Register Misc_byte 0x38 */
typedef union
{
    UInt8 Misc_byte;
    struct
    {
#ifdef _TARGET_PLATFORM_MSB_FIRST
        UInt8 UNUSED_D7 : 1;
        UInt8 RFCALPOR_I2C : 1;
        UInt8 PD_Underload : 1;
        UInt8 DDS_Polarity : 1;
        UInt8 UNUSED_D2 : 2;
        UInt8 IRQ_Mode : 1;
        UInt8 IRQ_Polarity : 1;
#else
        UInt8 IRQ_Polarity : 1;
        UInt8 IRQ_Mode : 1;
        UInt8 UNUSED_D2 : 2;
        UInt8 DDS_Polarity : 1;
        UInt8 PD_Underload : 1;
        UInt8 RFCALPOR_I2C : 1;
        UInt8 UNUSED_D7 : 1;
#endif
    }bF;
}TDA18273_Reg_Misc_byte;
#define TDA18273_REG_ADD_Misc_byte (0x38)
#define TDA18273_REG_SZ_Misc_byte  (0x01)

/* TDA18273 Register rfcal_log_0 0x39 */
typedef union
{
    UInt8 rfcal_log_0;
    struct
    {
        UInt8 rfcal_log_0 : 8;
    }bF;
}TDA18273_Reg_rfcal_log_0;
#define TDA18273_REG_ADD_rfcal_log_0 (0x39)
#define TDA18273_REG_SZ_rfcal_log_0  (0x01)

/* TDA18273 Register rfcal_log_1 0x3A */
typedef union
{
    UInt8 rfcal_log_1;
    struct
    {
        UInt8 rfcal_log_1 : 8;
    }bF;
}TDA18273_Reg_rfcal_log_1;
#define TDA18273_REG_ADD_rfcal_log_1 (0x3A)
#define TDA18273_REG_SZ_rfcal_log_1  (0x01)

/* TDA18273 Register rfcal_log_2 0x3B */
typedef union
{
    UInt8 rfcal_log_2;
    struct
    {
        UInt8 rfcal_log_2 : 8;
    }bF;
}TDA18273_Reg_rfcal_log_2;
#define TDA18273_REG_ADD_rfcal_log_2 (0x3B)
#define TDA18273_REG_SZ_rfcal_log_2  (0x01)

/* TDA18273 Register rfcal_log_3 0x3C */
typedef union
{
    UInt8 rfcal_log_3;
    struct
    {
        UInt8 rfcal_log_3 : 8;
    }bF;
}TDA18273_Reg_rfcal_log_3;
#define TDA18273_REG_ADD_rfcal_log_3 (0x3C)
#define TDA18273_REG_SZ_rfcal_log_3  (0x01)

/* TDA18273 Register rfcal_log_4 0x3D */
typedef union
{
    UInt8 rfcal_log_4;
    struct
    {
        UInt8 rfcal_log_4 : 8;
    }bF;
}TDA18273_Reg_rfcal_log_4;
#define TDA18273_REG_ADD_rfcal_log_4 (0x3D)
#define TDA18273_REG_SZ_rfcal_log_4  (0x01)

/* TDA18273 Register rfcal_log_5 0x3E */
typedef union
{
    UInt8 rfcal_log_5;
    struct       
    {
        UInt8 rfcal_log_5 : 8;
    }bF;
}TDA18273_Reg_rfcal_log_5;
#define TDA18273_REG_ADD_rfcal_log_5 (0x3E)
#define TDA18273_REG_SZ_rfcal_log_5  (0x01)

/* TDA18273 Register rfcal_log_6 0x3F */
typedef union
{
    UInt8 rfcal_log_6;
    struct
    {
        UInt8 rfcal_log_6 : 8;
    }bF;
}TDA18273_Reg_rfcal_log_6;
#define TDA18273_REG_ADD_rfcal_log_6 (0x3F)
#define TDA18273_REG_SZ_rfcal_log_6  (0x01)

/* TDA18273 Register rfcal_log_7 0x40 */
typedef union
{
    UInt8 rfcal_log_7;
    struct
    {
        UInt8 rfcal_log_7 : 8;
    }bF;
}TDA18273_Reg_rfcal_log_7;
#define TDA18273_REG_ADD_rfcal_log_7 (0x40)
#define TDA18273_REG_SZ_rfcal_log_7  (0x01)

/* TDA18273 Register rfcal_log_8 0x41 */
typedef union
{
    UInt8 rfcal_log_8;
    struct
    {
        UInt8 rfcal_log_8 : 8;
    }bF;
}TDA18273_Reg_rfcal_log_8;
#define TDA18273_REG_ADD_rfcal_log_8 (0x41)
#define TDA18273_REG_SZ_rfcal_log_8  (0x01)

/* TDA18273 Register rfcal_log_9 0x42 */
typedef union
{
    UInt8 rfcal_log_9;
    struct
    {
        UInt8 rfcal_log_9 : 8;
    }bF;
}TDA18273_Reg_rfcal_log_9;
#define TDA18273_REG_ADD_rfcal_log_9 (0x42)
#define TDA18273_REG_SZ_rfcal_log_9  (0x01)

/* TDA18273 Register rfcal_log_10 0x43 */
typedef union
{
    UInt8 rfcal_log_10;
    struct
    {
        UInt8 rfcal_log_10 : 8;
    }bF;
}TDA18273_Reg_rfcal_log_10;
#define TDA18273_REG_ADD_rfcal_log_10 (0x43)
#define TDA18273_REG_SZ_rfcal_log_10  (0x01)

/* TDA18273 Register rfcal_log_11 0x44 */
typedef union
{
    UInt8 rfcal_log_11;
    struct
    {
        UInt8 rfcal_log_11 : 8;
    }bF;
}TDA18273_Reg_rfcal_log_11;
#define TDA18273_REG_ADD_rfcal_log_11 (0x44)
#define TDA18273_REG_SZ_rfcal_log_11  (0x01)


 /* TDA18273 Register Map */
 typedef struct _TDA18273_Reg_Map_t
 {
     TDA18273_Reg_ID_byte_1 Reg_ID_byte_1;
     TDA18273_Reg_ID_byte_2 Reg_ID_byte_2;
     TDA18273_Reg_ID_byte_3 Reg_ID_byte_3;
     TDA18273_Reg_Thermo_byte_1 Reg_Thermo_byte_1;
     TDA18273_Reg_Thermo_byte_2 Reg_Thermo_byte_2;
     TDA18273_Reg_Power_state_byte_1 Reg_Power_state_byte_1;
     TDA18273_Reg_Power_state_byte_2 Reg_Power_state_byte_2;
     TDA18273_Reg_Input_Power_Level_byte Reg_Input_Power_Level_byte;
     TDA18273_Reg_IRQ_status Reg_IRQ_status;
     TDA18273_Reg_IRQ_enable Reg_IRQ_enable;
     TDA18273_Reg_IRQ_clear Reg_IRQ_clear;
     TDA18273_Reg_IRQ_set Reg_IRQ_set;
     TDA18273_Reg_AGC1_byte_1 Reg_AGC1_byte_1;
     TDA18273_Reg_AGC1_byte_1 Reg_AGC1_byte_2;
     TDA18273_Reg_AGC2_byte_1 Reg_AGC2_byte_1;
     TDA18273_Reg_AGCK_byte_1 Reg_AGCK_byte_1;
     TDA18273_Reg_RF_AGC_byte Reg_RF_AGC_byte;
     TDA18273_Reg_W_Filter_byte Reg_W_Filter_byte;
     TDA18273_Reg_IR_Mixer_byte_1 Reg_IR_Mixer_byte_1;
     TDA18273_Reg_AGC5_byte_1 Reg_AGC5_byte_1;
     TDA18273_Reg_IF_AGC_byte Reg_IF_AGC_byte;
     TDA18273_Reg_IF_Byte_1 Reg_IF_Byte_1;
     TDA18273_Reg_Reference_Byte Reg_Reference_Byte;
     TDA18273_Reg_IF_Frequency_byte Reg_IF_Frequency_byte;
     TDA18273_Reg_RF_Frequency_byte_1 Reg_RF_Frequency_byte_1;
     TDA18273_Reg_RF_Frequency_byte_2 Reg_RF_Frequency_byte_2;
     TDA18273_Reg_RF_Frequency_byte_3 Reg_RF_Frequency_byte_3;
     TDA18273_Reg_MSM_byte_1 Reg_MSM_byte_1;
     TDA18273_Reg_MSM_byte_2 Reg_MSM_byte_2;
     TDA18273_Reg_PowerSavingMode Reg_PowerSavingMode;
     TDA18273_Reg_Power_Level_byte_2 Reg_Power_Level_byte_2;
     TDA18273_Reg_Adapt_Top_byte Reg_Adapt_Top_byte;
     TDA18273_Reg_Vsync_byte Reg_Vsync_byte;
     TDA18273_Reg_Vsync_Mgt_byte Reg_Vsync_Mgt_byte;
     TDA18273_Reg_IR_Mixer_byte_2 Reg_IR_Mixer_byte_2;
     TDA18273_Reg_AGC1_byte_3 Reg_AGC1_byte_3;
     TDA18273_Reg_RFAGCs_Gain_byte_1 Reg_RFAGCs_Gain_byte_1;
     TDA18273_Reg_RFAGCs_Gain_byte_2 Reg_RFAGCs_Gain_byte_2;
     TDA18273_Reg_AGC5_byte_2 Reg_AGC5_byte_2;
     TDA18273_Reg_RF_Cal_byte_1 Reg_RF_Cal_byte_1;
     TDA18273_Reg_RF_Cal_byte_2 Reg_RF_Cal_byte_2;
     TDA18273_Reg_RF_Cal_byte_3 Reg_RF_Cal_byte_3;
     TDA18273_Reg_Bandsplit_Filter_byte Reg_Bandsplit_Filter_byte;
     TDA18273_Reg_RF_Filters_byte_1 Reg_RF_Filters_byte_1;
     TDA18273_Reg_RF_Filters_byte_2 Reg_RF_Filters_byte_2;
     TDA18273_Reg_RF_Filters_byte_3 Reg_RF_Filters_byte_3;
     TDA18273_Reg_RF_Band_Pass_Filter_byte Reg_RF_Band_Pass_Filter_byte;
     TDA18273_Reg_CP_Current_byte Reg_CP_Current_byte;
     TDA18273_Reg_AGCs_DetOut_byte Reg_AGCs_DetOut_byte;
     TDA18273_Reg_RFAGCs_Gain_byte_3 Reg_RFAGCs_Gain_byte_3;
     TDA18273_Reg_RFAGCs_Gain_byte_4 Reg_RFAGCs_Gain_byte_4;
     TDA18273_Reg_RFAGCs_Gain_byte_5 Reg_RFAGCs_Gain_byte_5;
     TDA18273_Reg_RFAGCs_Gain_byte_6 Reg_RFAGCs_Gain_byte_6;
     TDA18273_Reg_IFAGCs_Gain_byte Reg_IFAGCs_Gain_byte;
     TDA18273_Reg_RSSI_byte_1 Reg_RSSI_byte_1;
     TDA18273_Reg_RSSI_byte_2 Reg_RSSI_byte_2;
     TDA18273_Reg_Misc_byte Reg_Misc_byte;
     TDA18273_Reg_rfcal_log_0 Reg_rfcal_log_0;
     TDA18273_Reg_rfcal_log_1 Reg_rfcal_log_1;
     TDA18273_Reg_rfcal_log_2 Reg_rfcal_log_2;
     TDA18273_Reg_rfcal_log_3 Reg_rfcal_log_3;
     TDA18273_Reg_rfcal_log_4 Reg_rfcal_log_4;
     TDA18273_Reg_rfcal_log_5 Reg_rfcal_log_5;
     TDA18273_Reg_rfcal_log_6 Reg_rfcal_log_6;
     TDA18273_Reg_rfcal_log_7 Reg_rfcal_log_7;
     TDA18273_Reg_rfcal_log_8 Reg_rfcal_log_8;
     TDA18273_Reg_rfcal_log_9 Reg_rfcal_log_9;
     TDA18273_Reg_rfcal_log_10 Reg_rfcal_log_10;
     TDA18273_Reg_rfcal_log_11 Reg_rfcal_log_11;
     UInt8 RegPadding[40];
 } TDA18273_Reg_Map_t, *pTDA18273_Reg_Map_t;

#endif

#ifdef __cplusplus
}
#endif

#endif /* _TMBSL_TDA18273_REGDEF_H */
