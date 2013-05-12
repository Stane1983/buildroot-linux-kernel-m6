/*
 * Video Enhancement
 *
 * Author: Lin Xu <lin.xu@amlogic.com>
 *         Bobby Yang <bo.yang@amlogic.com>
 *
 * Copyright (C) 2010 Amlogic Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/string.h>
#include <linux/spinlock.h>
#include <linux/module.h>

#include <linux/aml_common.h>
#include <mach/am_regs.h>

#include "linux/amports/vframe.h"
#include <linux/amports/amstream.h>
#include "linux/amports/ve.h"

#include "ve_regs.h"
#include "amve.h"

// 0: Invalid
// 1: Valid
// 2: Updated in 2D mode
// 3: Updated in 3D mode
static unsigned int ve_hsvs_flag;
unsigned long flags;
//#define NEW_DNLP_AFTER_PEAKING

static struct ve_hsvs_s ve_hsvs;

static unsigned char ve_dnlp_tgt[64];
bool ve_en;
unsigned int ve_dnlp_white_factor;
unsigned int ve_dnlp_rt;
unsigned int ve_dnlp_rl;
unsigned int ve_dnlp_black;
unsigned int ve_dnlp_white;
unsigned int ve_dnlp_luma_sum;
static ulong ve_dnlp_lpf[64], ve_dnlp_reg[16];

static bool frame_lock_nosm = 1;
module_param(frame_lock_nosm, bool, 0664);
MODULE_PARM_DESC(frame_lock_nosm, "frame_lock_nosm");

// ***************************************************************************
// *** VPP_FIQ-oriented functions *********************************************
// ***************************************************************************

static void ve_dnlp_calculate_tgt(vframe_t *vf)
{
    struct vframe_prop_s *p = &vf->prop;
    ulong data[5];
    ulong i = 0, j = 0, ave = 0, max = 0, div = 0;

    // old historic luma sum
    j = ve_dnlp_luma_sum;
    // new historic luma sum
    ve_dnlp_luma_sum = p->hist.luma_sum;

    // picture mode: freeze dnlp curve
    if (// new luma sum is 0, something is wrong, freeze dnlp curve
        (!ve_dnlp_luma_sum) ||
        // new luma sum is closed to old one, picture mode, freeze curve
        ((ve_dnlp_luma_sum < j + (j >> 5)) &&
         (ve_dnlp_luma_sum > j - (j >> 5))
        )
       )
        return;

    // get 5 regions
    for (i = 0; i < 5; i++)
    {
        j = 4 + 11 * i;
        data[i] = (ulong)p->hist.gamma[j     ] +
                  (ulong)p->hist.gamma[j +  1] +
                  (ulong)p->hist.gamma[j +  2] +
                  (ulong)p->hist.gamma[j +  3] +
                  (ulong)p->hist.gamma[j +  4] +
                  (ulong)p->hist.gamma[j +  5] +
                  (ulong)p->hist.gamma[j +  6] +
                  (ulong)p->hist.gamma[j +  7] +
                  (ulong)p->hist.gamma[j +  8] +
                  (ulong)p->hist.gamma[j +  9] +
                  (ulong)p->hist.gamma[j + 10];
    }

    // get max, ave, div
    for (i = 0; i < 5; i++)
    {
        if (max < data[i])
            max = data[i];
        ave += data[i];
        data[i] *= 5;
    }
    max *= 5;
    div = (max - ave > ave) ? max - ave : ave;

    // invalid histgram: freeze dnlp curve
    if (!max)
        return;

    // get 1st 4 points
    for (i = 0; i < 4; i++)
    {
        if (data[i] > ave)
            data[i] = 64 + (((data[i] - ave) << 1) + div) * ve_dnlp_rl / (div << 1);
        else if (data[i] < ave)
            data[i] = 64 - (((ave - data[i]) << 1) + div) * ve_dnlp_rl / (div << 1);
        else
            data[i] = 64;
        ve_dnlp_tgt[4 + 11 * (i + 1)] = ve_dnlp_tgt[4 + 11 * i] +
                                        ((44 * data[i] + 32) >> 6);
    }

    // fill in region 0 with black extension
    data[0] = ve_dnlp_black;
    if (data[0] > 16)
        data[0] = 16;
    data[0] = (ve_dnlp_tgt[15] - ve_dnlp_tgt[4]) * (16 - data[0]);
    for (j = 1; j <= 6; j++)
        ve_dnlp_tgt[4 + j] = ve_dnlp_tgt[4] + (data[0] * j + 88) / 176;
    data[0] = (ve_dnlp_tgt[15] - ve_dnlp_tgt[10]) << 1;
    for (j = 1; j <=4; j++)
        ve_dnlp_tgt[10 + j] = ve_dnlp_tgt[10] + (data[0] * j + 5) / 10;

    // fill in regions 1~3
    for (i = 1; i <= 3; i++)
    {
        data[i] = (ve_dnlp_tgt[11 * i + 15] - ve_dnlp_tgt[11 * i + 4]) << 1;
        for (j = 1; j <= 10; j++)
            ve_dnlp_tgt[11 * i + 4 + j] = ve_dnlp_tgt[11 * i + 4] + (data[i] * j + 11) / 22;
    }

    // fill in region 4 with white extension
    data[4] /= 20;
    data[4] = (ve_dnlp_white * ((ave << 4) - data[4] * ve_dnlp_white_factor)  + (ave << 3)) / (ave << 4);
    if (data[4] > 16)
        data[4] = 16;
    data[4] = (ve_dnlp_tgt[59] - ve_dnlp_tgt[48]) * (16 - data[4]);
    for (j = 1; j <= 6; j++)
        ve_dnlp_tgt[59 - j] = ve_dnlp_tgt[59] - (data[4] * j + 88) / 176;
    data[4] = (ve_dnlp_tgt[53] - ve_dnlp_tgt[48]) << 1;
    for (j = 1; j <= 4; j++)
        ve_dnlp_tgt[53 - j] = ve_dnlp_tgt[53] - (data[4] * j + 5) / 10;

}

static void ve_dnlp_calculate_lpf(void) // lpf[0] is always 0 & no need calculation
{
    ulong i = 0;

    for (i = 0; i < 64; i++) {
        ve_dnlp_lpf[i] = ve_dnlp_lpf[i] - (ve_dnlp_lpf[i] >> ve_dnlp_rt) + ve_dnlp_tgt[i];
    }
}

static void ve_dnlp_calculate_reg(void)
{
    ulong i = 0, j = 0, cur = 0, data = 0, offset = ve_dnlp_rt ? (1 << (ve_dnlp_rt - 1)) : 0;

    for (i = 0; i < 16; i++)
    {
        ve_dnlp_reg[i] = 0;
        cur = i << 2;
        for (j = 0; j < 4; j++)
        {
            data = (ve_dnlp_lpf[cur + j] + offset) >> ve_dnlp_rt;
            if (data > 255)
                data = 255;
            ve_dnlp_reg[i] |= data << (j << 3);
        }
    }
}

static void ve_dnlp_load_reg(void)
{
    WRITE_CBUS_REG(VPP_DNLP_CTRL_00, ve_dnlp_reg[0]);
    WRITE_CBUS_REG(VPP_DNLP_CTRL_01, ve_dnlp_reg[1]);
    WRITE_CBUS_REG(VPP_DNLP_CTRL_02, ve_dnlp_reg[2]);
    WRITE_CBUS_REG(VPP_DNLP_CTRL_03, ve_dnlp_reg[3]);
    WRITE_CBUS_REG(VPP_DNLP_CTRL_04, ve_dnlp_reg[4]);
    WRITE_CBUS_REG(VPP_DNLP_CTRL_05, ve_dnlp_reg[5]);
    WRITE_CBUS_REG(VPP_DNLP_CTRL_06, ve_dnlp_reg[6]);
    WRITE_CBUS_REG(VPP_DNLP_CTRL_07, ve_dnlp_reg[7]);
    WRITE_CBUS_REG(VPP_DNLP_CTRL_08, ve_dnlp_reg[8]);
    WRITE_CBUS_REG(VPP_DNLP_CTRL_09, ve_dnlp_reg[9]);
    WRITE_CBUS_REG(VPP_DNLP_CTRL_10, ve_dnlp_reg[10]);
    WRITE_CBUS_REG(VPP_DNLP_CTRL_11, ve_dnlp_reg[11]);
    WRITE_CBUS_REG(VPP_DNLP_CTRL_12, ve_dnlp_reg[12]);
    WRITE_CBUS_REG(VPP_DNLP_CTRL_13, ve_dnlp_reg[13]);
    WRITE_CBUS_REG(VPP_DNLP_CTRL_14, ve_dnlp_reg[14]);
    WRITE_CBUS_REG(VPP_DNLP_CTRL_15, ve_dnlp_reg[15]);
}
static unsigned int lock_range_50hz_fast =  7; // <= 14
static unsigned int lock_range_50hz_slow =  7; // <= 14
static unsigned int lock_range_60hz_fast =  5; // <=  4
static unsigned int lock_range_60hz_slow =  2; // <= 10
#define FLAG_LVDS_FREQ_SW1       (1 <<  6)

void ve_on_vs(vframe_t *vf)
{

    if (ve_en) {
        // calculate dnlp target data
        ve_dnlp_calculate_tgt(vf);
        // calculate dnlp low-pass-filter data
        ve_dnlp_calculate_lpf();
        // calculate dnlp reg data
        ve_dnlp_calculate_reg();
        // load dnlp reg data
        ve_dnlp_load_reg();
    }
    /* comment for duration algorithm is not based on panel vsync */
    if (vf->prop.meas.vs_cycle && !frame_lock_nosm)
    {
        if ((vecm_latch_flag & FLAG_LVDS_FREQ_SW1) &&
          (vf->duration >= 1920 - 19) &&
          (vf->duration <= 1920 + 19)
         )
            vpp_phase_lock_on_vs(vf->prop.meas.vs_cycle,
                                 vf->prop.meas.vs_stamp,
                                 true,
                                 lock_range_50hz_fast,
                                 lock_range_50hz_slow);
        if ((!(vecm_latch_flag & FLAG_LVDS_FREQ_SW1)) &&
          (vf->duration >= 1600 - 5) &&
          (vf->duration <= 1600 + 13)
         )
            vpp_phase_lock_on_vs(vf->prop.meas.vs_cycle,
                                 vf->prop.meas.vs_stamp,
                                 false,
                                 lock_range_60hz_fast,
                                 lock_range_60hz_slow);
    }

}
EXPORT_SYMBOL(ve_on_vs);

// ***************************************************************************
// *** IOCTL-oriented functions *********************************************
// ***************************************************************************

void vpp_enable_lcd_gamma_table(void)
{
    WRITE_MPEG_REG_BITS(L_GAMMA_CNTL_PORT, 1, GAMMA_EN, 1);
}

void vpp_disable_lcd_gamma_table(void)
{
    WRITE_MPEG_REG_BITS(L_GAMMA_CNTL_PORT, 0, GAMMA_EN, 1);
}

void vpp_set_lcd_gamma_table(u16 *data, u32 rgb_mask)
{
    int i;

    while (!(READ_MPEG_REG(L_GAMMA_CNTL_PORT) & (0x1 << ADR_RDY)));
    WRITE_MPEG_REG(L_GAMMA_ADDR_PORT, (0x1 << H_AUTO_INC) |
                                    (0x1 << rgb_mask)   |
                                    (0x0 << HADR));
    for (i=0;i<256;i++) {
        while (!( READ_MPEG_REG(L_GAMMA_CNTL_PORT) & (0x1 << WR_RDY) )) ;
        WRITE_MPEG_REG(L_GAMMA_DATA_PORT, data[i]);
    }
    while (!(READ_MPEG_REG(L_GAMMA_CNTL_PORT) & (0x1 << ADR_RDY)));
    WRITE_MPEG_REG(L_GAMMA_ADDR_PORT, (0x1 << H_AUTO_INC) |
                                    (0x1 << rgb_mask)   |
                                    (0x23 << HADR));
}

void vpp_set_rgb_ogo(struct tcon_rgb_ogo_s *p)
{

    // write to registers
    WRITE_CBUS_REG(VPP_GAINOFF_CTRL0, ((p->en            << 31) & 0x80000000) |
                                      ((p->r_gain        << 16) & 0x07ff0000) |
                                      ((p->g_gain        <<  0) & 0x000007ff));
    WRITE_CBUS_REG(VPP_GAINOFF_CTRL1, ((p->b_gain        << 16) & 0x07ff0000) |
                                      ((p->r_post_offset <<  0) & 0x000007ff));
    WRITE_CBUS_REG(VPP_GAINOFF_CTRL2, ((p->g_post_offset << 16) & 0x07ff0000) |
                                      ((p->b_post_offset <<  0) & 0x000007ff));
    WRITE_CBUS_REG(VPP_GAINOFF_CTRL3, ((p->r_pre_offset  << 16) & 0x07ff0000) |
                                      ((p->g_pre_offset  <<  0) & 0x000007ff));
    WRITE_CBUS_REG(VPP_GAINOFF_CTRL4, ((p->b_pre_offset  <<  0) & 0x000007ff));
    
}


void ve_set_dnlp(struct ve_dnlp_s *p)
{
    ulong i = 0;

    // get command parameters
    ve_en                = p->en;
    ve_dnlp_white_factor = (p->rt >> 4) & 0xf;
    ve_dnlp_rt           = p->rt & 0xf;
    ve_dnlp_rl           = p->rl;
    ve_dnlp_black        = p->black;
    ve_dnlp_white        = p->white;

    if (ve_en)
    {
        // clear historic luma sum
        ve_dnlp_luma_sum = 0;
        // init tgt & lpf
        for (i = 0; i < 64; i++) {
            ve_dnlp_tgt[i] = i << 2;
            ve_dnlp_lpf[i] = ve_dnlp_tgt[i] << ve_dnlp_rt;
        }
        // calculate dnlp reg data
        ve_dnlp_calculate_reg();
        // load dnlp reg data
        ve_dnlp_load_reg();
#ifdef NEW_DNLP_AFTER_PEAKING
        // enable dnlp
        WRITE_CBUS_REG_BITS(VPP_PEAKING_DNLP, 1, PEAKING_DNLP_EN_BIT, PEAKING_DNLP_EN_WID);
    }
    else
    {
        // disable dnlp
        WRITE_CBUS_REG_BITS(VPP_PEAKING_DNLP, 0, PEAKING_DNLP_EN_BIT, PEAKING_DNLP_EN_WID);
    }
#else
        // enable dnlp
        WRITE_CBUS_REG_BITS(VPP_VE_ENABLE_CTRL, 1, DNLP_EN_BIT, DNLP_EN_WID);
    }
    else
    {
        // disable dnlp
        WRITE_CBUS_REG_BITS(VPP_VE_ENABLE_CTRL, 0, DNLP_EN_BIT, DNLP_EN_WID);
    }
#endif
}
unsigned int ve_get_vs_cnt(void)
{
    return (READ_CBUS_REG(VPP_VDO_MEAS_VS_COUNT_LO));
}

unsigned int vpp_log[128][10];

void vpp_phase_lock_on_vs(unsigned int cycle,
                          unsigned int stamp,
                          bool         lock50,
                          unsigned int range_fast,
                          unsigned int range_slow)
{
    unsigned int vtotal_ori = READ_CBUS_REG(ENCL_VIDEO_MAX_LNCNT);
    unsigned int vtotal     = lock50 ? 1349 : 1124;
	unsigned int stamp_in   = READ_CBUS_REG(VDIN_MEAS_VS_COUNT_LO);
    unsigned int stamp_out  = ve_get_vs_cnt();
    unsigned int phase      = 0;
	unsigned int cnt = READ_CBUS_REG(ASSIST_SPARE8_REG1);
	int step = 0, i = 0;

    // get phase
    if (stamp_out < stamp)
        phase = 0xffffffff - stamp + stamp_out + 1;
    else
        phase = stamp_out - stamp;
    while (phase >= cycle)
        phase -= cycle;
    // 225~315 degree => tune fast panel output
    if ((phase > ((cycle * 5) >> 3)) &&
        (phase < ((cycle * 7) >> 3))
       )
    {
        vtotal -= range_slow;
		step = 1;
    }
    // 45~135 degree => tune slow panel output
    else if ((phase > ( cycle      >> 3)) &&
             (phase < ((cycle * 3) >> 3))
            )
    {
        vtotal += range_slow;
		step = -1;
    }
    // 315~360 degree => tune fast panel output
    else if (phase >= ((cycle * 7) >> 3))
    {
        vtotal -= range_fast;
		step = +2;
    }
    // 0~45 degree => tune slow panel output
    else if (phase <= (cycle >> 3))
    {
        vtotal += range_fast;
		step = -2;
    }
    // 135~225 degree => keep still
    else
    {
        vtotal = vtotal_ori;
		step = 0;
    }
    if (vtotal != vtotal_ori)
        WRITE_CBUS_REG(ENCL_VIDEO_MAX_LNCNT, vtotal);
    if (cnt)
    {
        cnt--;
        WRITE_CBUS_REG(ASSIST_SPARE8_REG1, cnt);
        if (cnt)
        {
            vpp_log[cnt][0] = stamp;
            vpp_log[cnt][1] = stamp_in;
            vpp_log[cnt][2] = stamp_out;
            vpp_log[cnt][3] = cycle;
            vpp_log[cnt][4] = phase;
            vpp_log[cnt][5] = vtotal;
            vpp_log[cnt][6] = step;
        }
        else
        {
            for (i = 127; i > 0; i--)
            {
                printk("Ti=%10u Tio=%10u To=%10u CY=%6u PH =%10u Vt=%4u S=%2d\n",
                       vpp_log[i][0],
                       vpp_log[i][1],
                       vpp_log[i][2],
                       vpp_log[i][3],
                       vpp_log[i][4],
                       vpp_log[i][5],
                       vpp_log[i][6]
                       );
            }
            }
        }

}

