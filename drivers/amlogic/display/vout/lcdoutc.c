/*
 * AMLOGIC TCON controller driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the named License,
 * or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
 *
 * Author:  Tim Yao <timyao@amlogic.com>
 *
 */
#include <linux/init.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/string.h>
#include <linux/vout/lcdoutc.h>
#include <linux/vout/vinfo.h>
#include <linux/vout/vout_notify.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/logo/logo.h>
#include <mach/am_regs.h>
#include <mach/mlvds_regs.h>
#include <mach/clock.h>
#include <asm/fiq.h>
#include <linux/delay.h>
#include <plat/regops.h>
#include <mach/am_regs.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
extern unsigned int clk_util_clk_msr(unsigned int clk_mux);

//M6 PLL control value 
#define M6_PLL_CNTL_CST2 (0x814d3928)
#define M6_PLL_CNTL_CST3 (0x6b425012)
#define M6_PLL_CNTL_CST4 (0x110)

//VID PLL
#define M6_VID_PLL_CNTL_2 (M6_PLL_CNTL_CST2)
#define M6_VID_PLL_CNTL_3 (M6_PLL_CNTL_CST3)
#define M6_VID_PLL_CNTL_4 (M6_PLL_CNTL_CST4)

#define FIQ_VSYNC

#define BL_MAX_LEVEL 0x100
#define PANEL_NAME    "panel"

#define PRINT_DEBUG_INFO
#ifdef PRINT_DEBUG_INFO
#define PRINT_INFO(...)        printk(__VA_ARGS__)
#else
#define PRINT_INFO(...)    
#endif

#ifdef CONFIG_AM_TV_OUTPUT2
static unsigned int vpp2_sel = 0; /*0,vpp; 1, vpp2 */
#endif

typedef struct {
    Lcd_Config_t conf;
    vinfo_t lcd_info;
} lcd_dev_t;

static lcd_dev_t *pDev = NULL;

static void _lcd_init(Lcd_Config_t *pConf) ;

static void set_lcd_gamma_table_ttl(u16 *data, u32 rgb_mask)
{
    int i;

    while (!(aml_read_reg32(P_GAMMA_CNTL_PORT) & (0x1 << LCD_ADR_RDY)));
    aml_write_reg32(P_GAMMA_ADDR_PORT, (0x1 << LCD_H_AUTO_INC) |
                                    (0x1 << rgb_mask)   |
                                    (0x0 << LCD_HADR));
    for (i=0;i<256;i++) {
        while (!( aml_read_reg32(P_GAMMA_CNTL_PORT) & (0x1 << LCD_WR_RDY) )) ;
        aml_write_reg32(P_GAMMA_DATA_PORT, data[i]);
    }
    while (!(aml_read_reg32(P_GAMMA_CNTL_PORT) & (0x1 << LCD_ADR_RDY)));
    aml_write_reg32(P_GAMMA_ADDR_PORT, (0x1 << LCD_H_AUTO_INC) |
                                    (0x1 << rgb_mask)   |
                                    (0x23 << LCD_HADR));
}

void set_lcd_gamma_table_lvds(u16 *data, u32 rgb_mask)
{
    int i;

    while (!(aml_read_reg32(P_L_GAMMA_CNTL_PORT) & (0x1 << LCD_ADR_RDY)));
    aml_write_reg32(P_L_GAMMA_ADDR_PORT, (0x1 << LCD_H_AUTO_INC) |
                                    (0x1 << rgb_mask)   |
                                    (0x0 << LCD_HADR));
    for (i=0;i<256;i++) {
        while (!( aml_read_reg32(P_L_GAMMA_CNTL_PORT) & (0x1 << LCD_WR_RDY) )) ;
        aml_write_reg32(P_L_GAMMA_DATA_PORT, data[i]);
    }
    while (!(aml_read_reg32(P_L_GAMMA_CNTL_PORT) & (0x1 << LCD_ADR_RDY)));
    aml_write_reg32(P_L_GAMMA_ADDR_PORT, (0x1 << LCD_H_AUTO_INC) |
                                    (0x1 << rgb_mask)   |
                                    (0x23 << LCD_HADR));
}

static void write_tcon_double(Mlvds_Tcon_Config_t *mlvds_tcon)
{
    unsigned int tmp;
    int channel_num = mlvds_tcon->channel_num;
    int hv_sel = mlvds_tcon->hv_sel;
    int hstart_1 = mlvds_tcon->tcon_1st_hs_addr;
    int hend_1 = mlvds_tcon->tcon_1st_he_addr;
    int vstart_1 = mlvds_tcon->tcon_1st_vs_addr;
    int vend_1 = mlvds_tcon->tcon_1st_ve_addr;
    int hstart_2 = mlvds_tcon->tcon_2nd_hs_addr;
    int hend_2 = mlvds_tcon->tcon_2nd_he_addr;
    int vstart_2 = mlvds_tcon->tcon_2nd_vs_addr;
    int vend_2 = mlvds_tcon->tcon_2nd_ve_addr;

    tmp = aml_read_reg32(P_L_TCON_MISC_SEL_ADDR);
    switch(channel_num)
    {
        case 0 :
            aml_write_reg32(P_MTCON0_1ST_HS_ADDR, hstart_1);
            aml_write_reg32(P_MTCON0_1ST_HE_ADDR, hend_1);
            aml_write_reg32(P_MTCON0_1ST_VS_ADDR, vstart_1);
            aml_write_reg32(P_MTCON0_1ST_VE_ADDR, vend_1);
            aml_write_reg32(P_MTCON0_2ND_HS_ADDR, hstart_2);
            aml_write_reg32(P_MTCON0_2ND_HE_ADDR, hend_2);
            aml_write_reg32(P_MTCON0_2ND_VS_ADDR, vstart_2);
            aml_write_reg32(P_MTCON0_2ND_VE_ADDR, vend_2);
            tmp &= (~(1 << LCD_STH1_SEL)) | (hv_sel << LCD_STH1_SEL);
            aml_write_reg32(P_L_TCON_MISC_SEL_ADDR, tmp);
            break;
        case 1 :
            aml_write_reg32(P_MTCON1_1ST_HS_ADDR, hstart_1);
            aml_write_reg32(P_MTCON1_1ST_HE_ADDR, hend_1);
            aml_write_reg32(P_MTCON1_1ST_VS_ADDR, vstart_1);
            aml_write_reg32(P_MTCON1_1ST_VE_ADDR, vend_1);
            aml_write_reg32(P_MTCON1_2ND_HS_ADDR, hstart_2);
            aml_write_reg32(P_MTCON1_2ND_HE_ADDR, hend_2);
            aml_write_reg32(P_MTCON1_2ND_VS_ADDR, vstart_2);
            aml_write_reg32(P_MTCON1_2ND_VE_ADDR, vend_2);
            tmp &= (~(1 << LCD_CPV1_SEL)) | (hv_sel << LCD_CPV1_SEL);
            aml_write_reg32(P_L_TCON_MISC_SEL_ADDR, tmp);
            break;
        case 2 :
            aml_write_reg32(P_MTCON2_1ST_HS_ADDR, hstart_1);
            aml_write_reg32(P_MTCON2_1ST_HE_ADDR, hend_1);
            aml_write_reg32(P_MTCON2_1ST_VS_ADDR, vstart_1);
            aml_write_reg32(P_MTCON2_1ST_VE_ADDR, vend_1);
            aml_write_reg32(P_MTCON2_2ND_HS_ADDR, hstart_2);
            aml_write_reg32(P_MTCON2_2ND_HE_ADDR, hend_2);
            aml_write_reg32(P_MTCON2_2ND_VS_ADDR, vstart_2);
            aml_write_reg32(P_MTCON2_2ND_VE_ADDR, vend_2);
            tmp &= (~(1 << LCD_STV1_SEL)) | (hv_sel << LCD_STV1_SEL);
            aml_write_reg32(P_L_TCON_MISC_SEL_ADDR, tmp);
            break;
        case 3 :
            aml_write_reg32(P_MTCON3_1ST_HS_ADDR, hstart_1);
            aml_write_reg32(P_MTCON3_1ST_HE_ADDR, hend_1);
            aml_write_reg32(P_MTCON3_1ST_VS_ADDR, vstart_1);
            aml_write_reg32(P_MTCON3_1ST_VE_ADDR, vend_1);
            aml_write_reg32(P_MTCON3_2ND_HS_ADDR, hstart_2);
            aml_write_reg32(P_MTCON3_2ND_HE_ADDR, hend_2);
            aml_write_reg32(P_MTCON3_2ND_VS_ADDR, vstart_2);
            aml_write_reg32(P_MTCON3_2ND_VE_ADDR, vend_2);
            tmp &= (~(1 << LCD_OEV1_SEL)) | (hv_sel << LCD_OEV1_SEL);
            aml_write_reg32(P_L_TCON_MISC_SEL_ADDR, tmp);
            break;
        case 4 :
            aml_write_reg32(P_MTCON4_1ST_HS_ADDR, hstart_1);
            aml_write_reg32(P_MTCON4_1ST_HE_ADDR, hend_1);
            aml_write_reg32(P_MTCON4_1ST_VS_ADDR, vstart_1);
            aml_write_reg32(P_MTCON4_1ST_VE_ADDR, vend_1);
            tmp &= (~(1 << LCD_STH2_SEL)) | (hv_sel << LCD_STH2_SEL);
            aml_write_reg32(P_L_TCON_MISC_SEL_ADDR, tmp);
            break;
        case 5 :
            aml_write_reg32(P_MTCON5_1ST_HS_ADDR, hstart_1);
            aml_write_reg32(P_MTCON5_1ST_HE_ADDR, hend_1);
            aml_write_reg32(P_MTCON5_1ST_VS_ADDR, vstart_1);
            aml_write_reg32(P_MTCON5_1ST_VE_ADDR, vend_1);
            tmp &= (~(1 << LCD_CPV2_SEL)) | (hv_sel << LCD_CPV2_SEL);
            aml_write_reg32(P_L_TCON_MISC_SEL_ADDR, tmp);
            break;
        case 6 :
            aml_write_reg32(P_MTCON6_1ST_HS_ADDR, hstart_1);
            aml_write_reg32(P_MTCON6_1ST_HE_ADDR, hend_1);
            aml_write_reg32(P_MTCON6_1ST_VS_ADDR, vstart_1);
            aml_write_reg32(P_MTCON6_1ST_VE_ADDR, vend_1);
            tmp &= (~(1 << LCD_OEH_SEL)) | (hv_sel << LCD_OEH_SEL);
            aml_write_reg32(P_L_TCON_MISC_SEL_ADDR, tmp);
            break;
        case 7 :
            aml_write_reg32(P_MTCON7_1ST_HS_ADDR, hstart_1);
            aml_write_reg32(P_MTCON7_1ST_HE_ADDR, hend_1);
            aml_write_reg32(P_MTCON7_1ST_VS_ADDR, vstart_1);
            aml_write_reg32(P_MTCON7_1ST_VE_ADDR, vend_1);
            tmp &= (~(1 << LCD_OEV3_SEL)) | (hv_sel << LCD_OEV3_SEL);
            aml_write_reg32(P_L_TCON_MISC_SEL_ADDR, tmp);
            break;
        default:
            break;
    }
}

static void set_tcon_ttl(Lcd_Config_t *pConf)
{
    Lcd_Timing_t *tcon_adr = &(pConf->lcd_timing);
    
    set_lcd_gamma_table_ttl(pConf->lcd_effect.GammaTableR, LCD_H_SEL_R);
    set_lcd_gamma_table_ttl(pConf->lcd_effect.GammaTableG, LCD_H_SEL_G);
    set_lcd_gamma_table_ttl(pConf->lcd_effect.GammaTableB, LCD_H_SEL_B);

    aml_write_reg32(P_GAMMA_CNTL_PORT, pConf->lcd_effect.gamma_cntl_port);
    aml_write_reg32(P_GAMMA_VCOM_HSWITCH_ADDR, pConf->lcd_effect.gamma_vcom_hswitch_addr);

    aml_write_reg32(P_RGB_BASE_ADDR,   pConf->lcd_effect.rgb_base_addr);
    aml_write_reg32(P_RGB_COEFF_ADDR,  pConf->lcd_effect.rgb_coeff_addr);
    aml_write_reg32(P_POL_CNTL_ADDR,   pConf->lcd_timing.pol_cntl_addr);
    if(pConf->lcd_basic.lcd_bits == 8)
		aml_write_reg32(P_DITH_CNTL_ADDR,  0x400);
	else
		aml_write_reg32(P_DITH_CNTL_ADDR,  0x600);

    aml_write_reg32(P_STH1_HS_ADDR,    tcon_adr->sth1_hs_addr);
    aml_write_reg32(P_STH1_HE_ADDR,    tcon_adr->sth1_he_addr);
    aml_write_reg32(P_STH1_VS_ADDR,    tcon_adr->sth1_vs_addr);
    aml_write_reg32(P_STH1_VE_ADDR,    tcon_adr->sth1_ve_addr);

    aml_write_reg32(P_OEH_HS_ADDR,     tcon_adr->oeh_hs_addr);
    aml_write_reg32(P_OEH_HE_ADDR,     tcon_adr->oeh_he_addr);
    aml_write_reg32(P_OEH_VS_ADDR,     tcon_adr->oeh_vs_addr);
    aml_write_reg32(P_OEH_VE_ADDR,     tcon_adr->oeh_ve_addr);

    aml_write_reg32(P_VCOM_HSWITCH_ADDR, tcon_adr->vcom_hswitch_addr);
    aml_write_reg32(P_VCOM_VS_ADDR,    tcon_adr->vcom_vs_addr);
    aml_write_reg32(P_VCOM_VE_ADDR,    tcon_adr->vcom_ve_addr);

    aml_write_reg32(P_CPV1_HS_ADDR,    tcon_adr->cpv1_hs_addr);
    aml_write_reg32(P_CPV1_HE_ADDR,    tcon_adr->cpv1_he_addr);
    aml_write_reg32(P_CPV1_VS_ADDR,    tcon_adr->cpv1_vs_addr);
    aml_write_reg32(P_CPV1_VE_ADDR,    tcon_adr->cpv1_ve_addr);

    aml_write_reg32(P_STV1_HS_ADDR,    tcon_adr->stv1_hs_addr);
    aml_write_reg32(P_STV1_HE_ADDR,    tcon_adr->stv1_he_addr);
    aml_write_reg32(P_STV1_VS_ADDR,    tcon_adr->stv1_vs_addr);
    aml_write_reg32(P_STV1_VE_ADDR,    tcon_adr->stv1_ve_addr);

    aml_write_reg32(P_OEV1_HS_ADDR,    tcon_adr->oev1_hs_addr);
    aml_write_reg32(P_OEV1_HE_ADDR,    tcon_adr->oev1_he_addr);
    aml_write_reg32(P_OEV1_VS_ADDR,    tcon_adr->oev1_vs_addr);
    aml_write_reg32(P_OEV1_VE_ADDR,    tcon_adr->oev1_ve_addr);

    aml_write_reg32(P_INV_CNT_ADDR,    tcon_adr->inv_cnt_addr);
    aml_write_reg32(P_TCON_MISC_SEL_ADDR,     tcon_adr->tcon_misc_sel_addr);
    aml_write_reg32(P_DUAL_PORT_CNTL_ADDR, tcon_adr->dual_port_cntl_addr);

#ifdef CONFIG_AM_TV_OUTPUT2
    if(vpp2_sel){
        aml_write_reg32(P_VPP2_MISC, aml_read_reg32(P_VPP2_MISC) & ~(VPP_OUT_SATURATE));
    }
    else{
        aml_write_reg32(P_VPP_MISC, aml_read_reg32(P_VPP_MISC) & ~(VPP_OUT_SATURATE));
    }
#else
    aml_write_reg32(P_VPP_MISC, aml_read_reg32(P_VPP_MISC) & ~(VPP_OUT_SATURATE));
#endif    
}

static void set_tcon_lvds(Lcd_Config_t *pConf)
{
    Lcd_Timing_t *tcon_adr = &(pConf->lcd_timing);
    
    set_lcd_gamma_table_lvds(pConf->lcd_effect.GammaTableR, LCD_H_SEL_R);
    set_lcd_gamma_table_lvds(pConf->lcd_effect.GammaTableG, LCD_H_SEL_G);
    set_lcd_gamma_table_lvds(pConf->lcd_effect.GammaTableB, LCD_H_SEL_B);

    aml_write_reg32(P_L_GAMMA_CNTL_PORT, pConf->lcd_effect.gamma_cntl_port);
    aml_write_reg32(P_L_GAMMA_VCOM_HSWITCH_ADDR, pConf->lcd_effect.gamma_vcom_hswitch_addr);

    aml_write_reg32(P_L_RGB_BASE_ADDR,   pConf->lcd_effect.rgb_base_addr);
    aml_write_reg32(P_L_RGB_COEFF_ADDR,  pConf->lcd_effect.rgb_coeff_addr);
    aml_write_reg32(P_L_POL_CNTL_ADDR,   pConf->lcd_timing.pol_cntl_addr);    
	if(pConf->lcd_basic.lcd_bits == 8)
		aml_write_reg32(P_L_DITH_CNTL_ADDR,  0x400);
	else
		aml_write_reg32(P_L_DITH_CNTL_ADDR,  0x600);

    aml_write_reg32(P_L_INV_CNT_ADDR,    tcon_adr->inv_cnt_addr);
    aml_write_reg32(P_L_TCON_MISC_SEL_ADDR,     tcon_adr->tcon_misc_sel_addr);
    aml_write_reg32(P_L_DUAL_PORT_CNTL_ADDR, tcon_adr->dual_port_cntl_addr);

#ifdef CONFIG_AM_TV_OUTPUT2
    if(vpp2_sel){
        aml_write_reg32(P_VPP2_MISC, aml_read_reg32(P_VPP2_MISC) & ~(VPP_OUT_SATURATE));
    }
    else{
    aml_write_reg32(P_VPP_MISC, aml_read_reg32(P_VPP_MISC) & ~(VPP_OUT_SATURATE));
}
#else
    aml_write_reg32(P_VPP_MISC, aml_read_reg32(P_VPP_MISC) & ~(VPP_OUT_SATURATE));
#endif
}

// Set the mlvds TCON
// this function should support dual gate or singal gate TCON setting.
// singal gate TCON, Scan Function TO DO.
// scan_function   // 0 - Z1, 1 - Z2, 2- Gong
static void set_tcon_mlvds(Lcd_Config_t *pConf)
{
    Mlvds_Tcon_Config_t *mlvds_tconfig_l = pConf->lvds_mlvds_config.mlvds_tcon_config;
    int dual_gate = pConf->lvds_mlvds_config.mlvds_config->test_dual_gate;
    int bit_num = pConf->lcd_basic.lcd_bits;
    int pair_num = pConf->lvds_mlvds_config.mlvds_config->test_pair_num;

    unsigned int data32;

    int pclk_div;
    int ext_pixel = dual_gate ? pConf->lvds_mlvds_config.mlvds_config->total_line_clk : 0;
    int dual_wr_rd_start;
    int i = 0;

//    PRINT_INFO(" Notice: Setting VENC_DVI_SETTING[0x%4x] and GAMMA_CNTL_PORT[0x%4x].LCD_GAMMA_EN as 0 temporary\n", VENC_DVI_SETTING, GAMMA_CNTL_PORT);
//    PRINT_INFO(" Otherwise, the panel will display color abnormal.\n");
//    aml_write_reg32(P_VENC_DVI_SETTING, 0);

    set_lcd_gamma_table_lvds(pConf->lcd_effect.GammaTableR, LCD_H_SEL_R);
    set_lcd_gamma_table_lvds(pConf->lcd_effect.GammaTableG, LCD_H_SEL_G);
    set_lcd_gamma_table_lvds(pConf->lcd_effect.GammaTableB, LCD_H_SEL_B);

    aml_write_reg32(P_L_GAMMA_CNTL_PORT, pConf->lcd_effect.gamma_cntl_port);
    aml_write_reg32(P_L_GAMMA_VCOM_HSWITCH_ADDR, pConf->lcd_effect.gamma_vcom_hswitch_addr);

    aml_write_reg32(P_L_RGB_BASE_ADDR, pConf->lcd_effect.rgb_base_addr);
    aml_write_reg32(P_L_RGB_COEFF_ADDR, pConf->lcd_effect.rgb_coeff_addr);
    //aml_write_reg32(P_L_POL_CNTL_ADDR, pConf->pol_cntl_addr);
	if(pConf->lcd_basic.lcd_bits == 8)
		aml_write_reg32(P_L_DITH_CNTL_ADDR,  0x400);
	else
		aml_write_reg32(P_L_DITH_CNTL_ADDR,  0x600);

//    aml_write_reg32(P_L_INV_CNT_ADDR, pConf->inv_cnt_addr);
//    aml_write_reg32(P_L_TCON_MISC_SEL_ADDR, pConf->tcon_misc_sel_addr);
//    aml_write_reg32(P_L_DUAL_PORT_CNTL_ADDR, pConf->dual_port_cntl_addr);
//
/*
#ifdef CONFIG_AM_TV_OUTPUT2
    if(vpp2_sel){
        CLEAR_MPEG_REG_MASK(VPP2_MISC, VPP_OUT_SATURATE);
    }
    else{
        CLEAR_MPEG_REG_MASK(VPP_MISC, VPP_OUT_SATURATE);
    }
#else
    CLEAR_MPEG_REG_MASK(VPP_MISC, VPP_OUT_SATURATE);
#endif
*/
    data32 = (0x9867 << tcon_pattern_loop_data) |
             (1 << tcon_pattern_loop_start) |
             (4 << tcon_pattern_loop_end) |
             (1 << ((mlvds_tconfig_l[6].channel_num)+tcon_pattern_enable)); // POL_CHANNEL use pattern generate

    aml_write_reg32(P_L_TCON_PATTERN_HI,  (data32 >> 16));
    aml_write_reg32(P_L_TCON_PATTERN_LO, (data32 & 0xffff));

    pclk_div = (bit_num == 8) ? 3 : // phy_clk / 8
                                2 ; // phy_clk / 6
   data32 = (1 << ((mlvds_tconfig_l[7].channel_num)-2+tcon_pclk_enable)) |  // enable PCLK_CHANNEL
            (pclk_div << tcon_pclk_div) |
            (
              (pair_num == 6) ?
              (
              ((bit_num == 8) & dual_gate) ?
              (
                (0 << (tcon_delay + 0*3)) |
                (0 << (tcon_delay + 1*3)) |
                (0 << (tcon_delay + 2*3)) |
                (0 << (tcon_delay + 3*3)) |
                (0 << (tcon_delay + 4*3)) |
                (0 << (tcon_delay + 5*3)) |
                (0 << (tcon_delay + 6*3)) |
                (0 << (tcon_delay + 7*3))
              ) :
              (
                (0 << (tcon_delay + 0*3)) |
                (0 << (tcon_delay + 1*3)) |
                (0 << (tcon_delay + 2*3)) |
                (0 << (tcon_delay + 3*3)) |
                (0 << (tcon_delay + 4*3)) |
                (0 << (tcon_delay + 5*3)) |
                (0 << (tcon_delay + 6*3)) |
                (0 << (tcon_delay + 7*3))
              )
              ) :
              (
              ((bit_num == 8) & dual_gate) ?
              (
                (0 << (tcon_delay + 0*3)) |
                (0 << (tcon_delay + 1*3)) |
                (0 << (tcon_delay + 2*3)) |
                (0 << (tcon_delay + 3*3)) |
                (0 << (tcon_delay + 4*3)) |
                (0 << (tcon_delay + 5*3)) |
                (0 << (tcon_delay + 6*3)) |
                (0 << (tcon_delay + 7*3))
              ) :
              (bit_num == 8) ?
              (
                (0 << (tcon_delay + 0*3)) |
                (0 << (tcon_delay + 1*3)) |
                (0 << (tcon_delay + 2*3)) |
                (0 << (tcon_delay + 3*3)) |
                (0 << (tcon_delay + 4*3)) |
                (0 << (tcon_delay + 5*3)) |
                (0 << (tcon_delay + 6*3)) |
                (0 << (tcon_delay + 7*3))
              ) :
              (
                (0 << (tcon_delay + 0*3)) |
                (0 << (tcon_delay + 1*3)) |
                (0 << (tcon_delay + 2*3)) |
                (0 << (tcon_delay + 3*3)) |
                (0 << (tcon_delay + 4*3)) |
                (0 << (tcon_delay + 5*3)) |
                (0 << (tcon_delay + 6*3)) |
                (0 << (tcon_delay + 7*3))
              )
              )
            );

    aml_write_reg32(P_TCON_CONTROL_HI,  (data32 >> 16));
    aml_write_reg32(P_TCON_CONTROL_LO, (data32 & 0xffff));


    aml_write_reg32(P_L_TCON_DOUBLE_CTL,
                   (1<<(mlvds_tconfig_l[3].channel_num))   // invert CPV
                  );
				  
	// for channel 4-7, set second setting same as first
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6	
    aml_write_reg32(P_L_DE_HS_ADDR, (0x3 << 14) | ext_pixel);   // 0x3 -- enable double_tcon fir channel7:6
    aml_write_reg32(P_L_DE_HE_ADDR, (0x3 << 14) | ext_pixel);   // 0x3 -- enable double_tcon fir channel5:4
    aml_write_reg32(P_L_DE_VS_ADDR, (0x3 << 14) | 0);	// 0x3 -- enable double_tcon fir channel3:2
    aml_write_reg32(P_L_DE_VE_ADDR, (0x3 << 14) | 0);	// 0x3 -- enable double_tcon fir channel1:0
#else	
    aml_write_reg32(P_L_DE_HS_ADDR, (0xf << 12) | ext_pixel);   // 0xf -- enable double_tcon fir channel7:4
    aml_write_reg32(P_L_DE_HE_ADDR, (0xf << 12) | ext_pixel);   // 0xf -- enable double_tcon fir channel3:0
    aml_write_reg32(P_L_DE_VS_ADDR, 0);
    aml_write_reg32(P_L_DE_VE_ADDR, 0);
#endif	

    dual_wr_rd_start = 0x5d;
    aml_write_reg32(P_MLVDS_DUAL_GATE_WR_START, dual_wr_rd_start);
    aml_write_reg32(P_MLVDS_DUAL_GATE_WR_END, dual_wr_rd_start + 1280);
    aml_write_reg32(P_MLVDS_DUAL_GATE_RD_START, dual_wr_rd_start + ext_pixel - 2);
    aml_write_reg32(P_MLVDS_DUAL_GATE_RD_END, dual_wr_rd_start + 1280 + ext_pixel - 2);

    aml_write_reg32(P_MLVDS_SECOND_RESET_CTL, (pConf->lvds_mlvds_config.mlvds_config->mlvds_insert_start + ext_pixel));

    data32 = (0 << ((mlvds_tconfig_l[5].channel_num)+mlvds_tcon_field_en)) |  // enable EVEN_F on TCON channel 6
             ( (0x0 << mlvds_scan_mode_odd) | (0x0 << mlvds_scan_mode_even)
             ) | (0 << mlvds_scan_mode_start_line);

    aml_write_reg32(P_MLVDS_DUAL_GATE_CTL_HI,  (data32 >> 16));
    aml_write_reg32(P_MLVDS_DUAL_GATE_CTL_LO, (data32 & 0xffff));

    PRINT_INFO("write minilvds tcon 0~7.\n");
    for(i = 0; i < 8; i++)
    {
		write_tcon_double(&mlvds_tconfig_l[i]);
    }
}

static void set_video_spread_spectrum(int video_pll_sel, int video_ss_level)
{ 
    if (video_pll_sel){
		switch (video_ss_level)
		{
			case 0:  // disable ss
				aml_write_reg32(P_HHI_VIID_PLL_CNTL2, 0x814d3928 );
				aml_write_reg32(P_HHI_VIID_PLL_CNTL3, 0x6b425012 );
				aml_write_reg32(P_HHI_VIID_PLL_CNTL4, 0x110 );
				break;
			case 1:  //about 1%
				aml_write_reg32(P_HHI_VIID_PLL_CNTL2, 0x16110696);
				aml_write_reg32(P_HHI_VIID_PLL_CNTL3, 0x4d625012);
				aml_write_reg32(P_HHI_VIID_PLL_CNTL4, 0x130);
				break;
			case 2:  //about 2%
				aml_write_reg32(P_HHI_VIID_PLL_CNTL2, 0x16110696);
				aml_write_reg32(P_HHI_VIID_PLL_CNTL3, 0x2d425012);
				aml_write_reg32(P_HHI_VIID_PLL_CNTL4, 0x130);
				break;
			case 3:  //about 3%
				aml_write_reg32(P_HHI_VIID_PLL_CNTL2, 0x16110696);
				aml_write_reg32(P_HHI_VIID_PLL_CNTL3, 0x1d425012);
				aml_write_reg32(P_HHI_VIID_PLL_CNTL4, 0x130);
				break;		
			case 4:  //about 4%
				aml_write_reg32(P_HHI_VIID_PLL_CNTL2, 0x16110696);
				aml_write_reg32(P_HHI_VIID_PLL_CNTL3, 0x0d125012);
				aml_write_reg32(P_HHI_VIID_PLL_CNTL4, 0x130);
				break;
			case 5:  //about 5%
				aml_write_reg32(P_HHI_VIID_PLL_CNTL2, 0x16110696);
				aml_write_reg32(P_HHI_VIID_PLL_CNTL3, 0x0e425012);
				aml_write_reg32(P_HHI_VIID_PLL_CNTL4, 0x130);
				break;	
			default:  //disable ss
				aml_write_reg32(P_HHI_VIID_PLL_CNTL2, 0x814d3928);
				aml_write_reg32(P_HHI_VIID_PLL_CNTL3, 0x6b425012);
				aml_write_reg32(P_HHI_VIID_PLL_CNTL4, 0x110);		
		}
	}
	else{
		switch (video_ss_level)
		{
			case 0:  // disable ss
				aml_write_reg32(P_HHI_VID_PLL_CNTL2, 0x814d3928 );
				aml_write_reg32(P_HHI_VID_PLL_CNTL3, 0x6b425012 );
				aml_write_reg32(P_HHI_VID_PLL_CNTL4, 0x110 );
				break;
			case 1:  //about 1%
				aml_write_reg32(P_HHI_VID_PLL_CNTL2, 0x16110696);
				aml_write_reg32(P_HHI_VID_PLL_CNTL3, 0x4d625012);
				aml_write_reg32(P_HHI_VID_PLL_CNTL4, 0x130);
				break;
			case 2:  //about 2%
				aml_write_reg32(P_HHI_VID_PLL_CNTL2, 0x16110696);
				aml_write_reg32(P_HHI_VID_PLL_CNTL3, 0x2d425012);
				aml_write_reg32(P_HHI_VID_PLL_CNTL4, 0x130);
				break;
			case 3:  //about 3%
				aml_write_reg32(P_HHI_VID_PLL_CNTL2, 0x16110696);
				aml_write_reg32(P_HHI_VID_PLL_CNTL3, 0x1d425012);
				aml_write_reg32(P_HHI_VID_PLL_CNTL4, 0x130);
				break;		
			case 4:  //about 4%
				aml_write_reg32(P_HHI_VID_PLL_CNTL2, 0x16110696);
				aml_write_reg32(P_HHI_VID_PLL_CNTL3, 0x0d125012);
				aml_write_reg32(P_HHI_VID_PLL_CNTL4, 0x130);
				break;
			case 5:  //about 5%
				aml_write_reg32(P_HHI_VID_PLL_CNTL2, 0x16110696);
				aml_write_reg32(P_HHI_VID_PLL_CNTL3, 0x0e425012);
				aml_write_reg32(P_HHI_VID_PLL_CNTL4, 0x130);
				break;	
			default:  //disable ss
				aml_write_reg32(P_HHI_VID_PLL_CNTL2, 0x814d3928);
				aml_write_reg32(P_HHI_VID_PLL_CNTL3, 0x6b425012);
				aml_write_reg32(P_HHI_VID_PLL_CNTL4, 0x110);		
		}	
	}
	//PRINT_INFO("set video spread spectrum %d%%.\n", video_ss_level);	
}

static void vclk_set_lcd(int lcd_type, int pll_sel, int pll_div_sel, int vclk_sel, unsigned long pll_reg, unsigned long vid_div_reg, unsigned int xd)
{
    PRINT_INFO("setup lcd clk.\n");
    vid_div_reg |= (1 << 16) ; // turn clock gate on
    vid_div_reg |= (pll_sel << 15); // vid_div_clk_sel
   
    if(vclk_sel) {
      aml_write_reg32(P_HHI_VIID_CLK_CNTL, aml_read_reg32(P_HHI_VIID_CLK_CNTL) & ~(1 << 19) );     //disable clk_div0 
    }
    else {
      aml_write_reg32(P_HHI_VID_CLK_CNTL, aml_read_reg32(P_HHI_VID_CLK_CNTL) & ~(1 << 19) );     //disable clk_div0 
      aml_write_reg32(P_HHI_VID_CLK_CNTL, aml_read_reg32(P_HHI_VID_CLK_CNTL) & ~(1 << 20) );     //disable clk_div1 
    } 

    // delay 2uS to allow the sync mux to switch over
    //aml_write_reg32(P_ISA_TIMERE, 0); while( aml_read_reg32(P_ISA_TIMERE) < 2 ) {}    
    udelay(2);

    if(pll_sel){
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6
		M6_PLL_RESET(P_HHI_VIID_PLL_CNTL);
		aml_write_reg32(P_HHI_VIID_PLL_CNTL, pll_reg|(1<<29) );            
		aml_write_reg32(P_HHI_VIID_PLL_CNTL2, 0x814d3928 );
		aml_write_reg32(P_HHI_VIID_PLL_CNTL3, 0x6b425012 );
		aml_write_reg32(P_HHI_VIID_PLL_CNTL4, 0x110 );
		aml_write_reg32(P_HHI_VIID_PLL_CNTL, pll_reg );
		M6_PLL_WAIT_FOR_LOCK(P_HHI_VIID_PLL_CNTL);
#else
		aml_write_reg32(P_HHI_VIID_PLL_CNTL, pll_reg|(1<<30) );
		aml_write_reg32(P_HHI_VIID_PLL_CNTL2, 0x65e31ff );
		aml_write_reg32(P_HHI_VIID_PLL_CNTL3, 0x9649a941 );
		aml_write_reg32(P_HHI_VIID_PLL_CNTL, pll_reg );
#endif
    }    
    else{
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6
		M6_PLL_RESET(P_HHI_VID_PLL_CNTL);
		aml_write_reg32(P_HHI_VID_PLL_CNTL, pll_reg|(1<<29) );
		aml_write_reg32(P_HHI_VID_PLL_CNTL2, M6_VID_PLL_CNTL_2 );
		aml_write_reg32(P_HHI_VID_PLL_CNTL3, M6_VID_PLL_CNTL_3 );
		aml_write_reg32(P_HHI_VID_PLL_CNTL4, M6_VID_PLL_CNTL_4 );
		aml_write_reg32(P_HHI_VID_PLL_CNTL, pll_reg );
		M6_PLL_WAIT_FOR_LOCK(P_HHI_VID_PLL_CNTL);
#else
		aml_write_reg32(P_HHI_VID_PLL_CNTL, pll_reg|(1<<30) );
		aml_write_reg32(P_HHI_VID_PLL_CNTL2, 0x65e31ff );
		aml_write_reg32(P_HHI_VID_PLL_CNTL3, 0x9649a941 );
		aml_write_reg32(P_HHI_VID_PLL_CNTL, pll_reg );
#endif
    }
    udelay(10);
	
    if(pll_div_sel ){
        aml_write_reg32(P_HHI_VIID_DIVIDER_CNTL,   vid_div_reg);
		//reset HHI_VIID_DIVIDER_CNTL
		aml_write_reg32(P_HHI_VIID_DIVIDER_CNTL, aml_read_reg32(P_HHI_VIID_DIVIDER_CNTL)|(1<<7));    //0x104c[7]:SOFT_RESET_POST
        aml_write_reg32(P_HHI_VIID_DIVIDER_CNTL, aml_read_reg32(P_HHI_VIID_DIVIDER_CNTL)|(1<<3));    //0x104c[3]:SOFT_RESET_PRE
        aml_write_reg32(P_HHI_VIID_DIVIDER_CNTL, aml_read_reg32(P_HHI_VIID_DIVIDER_CNTL)&(~(1<<1)));    //0x104c[1]:RESET_N_POST
        aml_write_reg32(P_HHI_VIID_DIVIDER_CNTL, aml_read_reg32(P_HHI_VIID_DIVIDER_CNTL)&(~(1<<0)));    //0x104c[0]:RESET_N_PRE
        msleep(2);
        aml_write_reg32(P_HHI_VIID_DIVIDER_CNTL, aml_read_reg32(P_HHI_VIID_DIVIDER_CNTL)&(~((1<<7)|(1<<3))));
        aml_write_reg32(P_HHI_VIID_DIVIDER_CNTL, aml_read_reg32(P_HHI_VIID_DIVIDER_CNTL)|((1<<1)|(1<<0)));
    }
    else{
        aml_write_reg32(P_HHI_VID_DIVIDER_CNTL,   vid_div_reg);
        //reset HHI_VID_DIVIDER_CNTL
        aml_write_reg32(P_HHI_VID_DIVIDER_CNTL, aml_read_reg32(P_HHI_VID_DIVIDER_CNTL)|(1<<7));    //0x1066[7]:SOFT_RESET_POST
        aml_write_reg32(P_HHI_VID_DIVIDER_CNTL, aml_read_reg32(P_HHI_VID_DIVIDER_CNTL)|(1<<3));    //0x1066[3]:SOFT_RESET_PRE
        aml_write_reg32(P_HHI_VID_DIVIDER_CNTL, aml_read_reg32(P_HHI_VID_DIVIDER_CNTL)&(~(1<<1)));    //0x1066[1]:RESET_N_POST
        aml_write_reg32(P_HHI_VID_DIVIDER_CNTL, aml_read_reg32(P_HHI_VID_DIVIDER_CNTL)&(~(1<<0)));    //0x1066[0]:RESET_N_PRE
        msleep(2);
        aml_write_reg32(P_HHI_VID_DIVIDER_CNTL, aml_read_reg32(P_HHI_VID_DIVIDER_CNTL)&(~((1<<7)|(1<<3))));
        aml_write_reg32(P_HHI_VID_DIVIDER_CNTL, aml_read_reg32(P_HHI_VID_DIVIDER_CNTL)|((1<<1)|(1<<0)));
    }

    if(vclk_sel)
        aml_write_reg32(P_HHI_VIID_CLK_DIV, (aml_read_reg32(P_HHI_VIID_CLK_DIV) & ~(0xFF << 0)) | (xd-1) );   // setup the XD divider value
    else 
        aml_write_reg32(P_HHI_VID_CLK_DIV, (aml_read_reg32(P_HHI_VID_CLK_DIV) & ~(0xFF << 0)) | (xd-1) );   // setup the XD divider value

    // delay 5uS
    //aml_write_reg32(P_ISA_TIMERE, 0); while( aml_read_reg32(P_ISA_TIMERE) < 5 ) {}    
    udelay(5);
       
	if(vclk_sel) {
		if(pll_div_sel) aml_set_reg32_bits (P_HHI_VIID_CLK_CNTL, 4, 16, 3);  // Bit[18:16] - v2_cntl_clk_in_sel
		else aml_set_reg32_bits (P_HHI_VIID_CLK_CNTL, 0, 16, 3);  // Bit[18:16] - cntl_clk_in_sel
		//aml_write_reg32(P_HHI_VIID_CLK_CNTL, aml_read_reg32(P_HHI_VIID_CLK_CNTL) |  (1 << 19) );     //enable clk_div0 
		aml_set_reg32_mask(P_HHI_VIID_CLK_CNTL, (1 << 19) );     //enable clk_div0
	}
	else {
		if(pll_div_sel) aml_set_reg32_bits (P_HHI_VID_CLK_CNTL, 4, 16, 3);  // Bit[18:16] - v2_cntl_clk_in_sel
		else aml_set_reg32_bits (P_HHI_VID_CLK_CNTL, 0, 16, 3);  // Bit[18:16] - cntl_clk_in_sel
		//aml_write_reg32(P_HHI_VID_CLK_CNTL, aml_read_reg32(P_HHI_VID_CLK_CNTL) |  (1 << 19) );     //enable clk_div0 
		aml_set_reg32_mask(P_HHI_VID_CLK_CNTL, (1 << 19) );     //enable clk_div0 
		//aml_write_reg32(P_HHI_VID_CLK_CNTL, aml_read_reg32(P_HHI_VID_CLK_CNTL) |  (1 << 20) );     //enable clk_div1 
		aml_set_reg32_mask(P_HHI_VID_CLK_CNTL, (1 << 20) );     //enable clk_div1 
	}
    
    // delay 2uS
    //aml_write_reg32(P_ISA_TIMERE, 0); while( aml_read_reg32(P_ISA_TIMERE) < 2 ) {}    
    udelay(2);
    
    // set tcon_clko setting
    aml_set_reg32_bits (P_HHI_VID_CLK_CNTL, 
                    (
                    (0 << 11) |     //clk_div1_sel
                    (1 << 10) |     //clk_inv
                    (0 << 9)  |     //neg_edge_sel
                    (0 << 5)  |     //tcon high_thresh
                    (0 << 1)  |     //tcon low_thresh
                    (1 << 0)        //cntl_clk_en1
                    ), 
                    20, 12);

    if(lcd_type == LCD_DIGITAL_TTL){
		if(vclk_sel)
		{
			aml_set_reg32_bits (P_HHI_VID_CLK_DIV, 8, 20, 4); // [23:20] enct_clk_sel, select v2_clk_div1
		}
		else
		{
			aml_set_reg32_bits (P_HHI_VID_CLK_DIV, 0, 20, 4); // [23:20] enct_clk_sel, select v1_clk_div1
		}
		
	}
	else {
		if(vclk_sel)
		{
			aml_set_reg32_bits (P_HHI_VIID_CLK_DIV, 8, 12, 4); // [23:20] encl_clk_sel, select v2_clk_div1
		}
		else
		{
			aml_set_reg32_bits (P_HHI_VIID_CLK_DIV, 0, 12, 4); // [23:20] encl_clk_sel, select v1_clk_div1
		}		
	}
	
    if(vclk_sel) {
      aml_set_reg32_bits (P_HHI_VIID_CLK_CNTL, 
                   (1<<0),  // Enable cntl_div1_en
                   0, 1    // cntl_div1_en
                   );
      aml_set_reg32_bits (P_HHI_VIID_CLK_CNTL, 1, 15, 1);  //soft reset
      aml_set_reg32_bits (P_HHI_VIID_CLK_CNTL, 0, 15, 1);  //release soft reset
    }
    else {
      aml_set_reg32_bits (P_HHI_VID_CLK_CNTL, 
                   (1<<0),  // Enable cntl_div1_en
                   0, 1    // cntl_div1_en
                   );
      aml_set_reg32_bits (P_HHI_VID_CLK_CNTL, 1, 15, 1);  //soft reset
      aml_set_reg32_bits (P_HHI_VID_CLK_CNTL, 0, 15, 1);  //release soft reset
    }    

    //PRINT_INFO("video pl1 clk = %d\n", clk_util_clk_msr(VID_PLL_CLK));
    //PRINT_INFO("video pll2 clk = %d\n", clk_util_clk_msr(VID2_PLL_CLK));
    //PRINT_INFO("cts_enct clk = %d\n", clk_util_clk_msr(CTS_ENCT_CLK));
	//PRINT_INFO("cts_encl clk = %d\n", clk_util_clk_msr(CTS_ENCL_CLK));
}

static void set_pll_ttl(Lcd_Config_t *pConf)
{            
    unsigned pll_reg, div_reg, xd;    
    int pll_sel, pll_div_sel, vclk_sel;
	int lcd_type, ss_level;
    
    pll_reg = pConf->lcd_timing.pll_ctrl;
    div_reg = pConf->lcd_timing.div_ctrl | 0x3; 
	ss_level = ((pConf->lcd_timing.clk_ctrl) >>16) & 0xf;      
    pll_sel = ((pConf->lcd_timing.clk_ctrl) >>12) & 0x1;
    pll_div_sel = ((pConf->lcd_timing.clk_ctrl) >>8) & 0x1;
    vclk_sel = ((pConf->lcd_timing.clk_ctrl) >>4) & 0x1;        
	xd = pConf->lcd_timing.clk_ctrl & 0xf;
	
	lcd_type = pConf->lcd_basic.lcd_type;
    
    printk("ss_level=%d, pll_sel=%d, pll_div_sel=%d, vclk_sel=%d, pll_reg=0x%x, div_reg=0x%x, xd=%d.\n", ss_level, pll_sel, pll_div_sel, vclk_sel, pll_reg, div_reg, xd);
    vclk_set_lcd(lcd_type, pll_sel, pll_div_sel, vclk_sel, pll_reg, div_reg, xd); 
	set_video_spread_spectrum(pll_sel, ss_level);
}

static void clk_util_lvds_set_clk_div(  unsigned long   divn_sel,
                                    unsigned long   divn_tcnt,
                                    unsigned long   div2_en  )
{
    // assign          lvds_div_phy_clk_en     = tst_lvds_tmode ? 1'b1         : phy_clk_cntl[10];
    // assign          lvds_div_div2_sel       = tst_lvds_tmode ? atest_i[5]   : phy_clk_cntl[9];
    // assign          lvds_div_sel            = tst_lvds_tmode ? atest_i[7:6] : phy_clk_cntl[8:7];
    // assign          lvds_div_tcnt           = tst_lvds_tmode ? 3'd6         : phy_clk_cntl[6:4];
    // If dividing by 1, just select the divide by 1 path
    if( divn_tcnt == 1 ) {
        divn_sel = 0;
    }
    aml_write_reg32(P_LVDS_PHY_CLK_CNTL, ((aml_read_reg32(P_LVDS_PHY_CLK_CNTL) & ~((0x3 << 7) | (1 << 9) | (0x7 << 4))) | ((1 << 10) | (divn_sel << 7) | (div2_en << 9) | (((divn_tcnt-1)&0x7) << 4))) );
}

static void set_pll_lvds(Lcd_Config_t *pConf)
{
    PRINT_INFO("%s\n", __FUNCTION__);

    int pll_div_post;
    int phy_clk_div2;
    int FIFO_CLK_SEL;
    unsigned long rd_data;

    unsigned pll_reg, div_reg, xd;
    int pll_sel, pll_div_sel, vclk_sel;
	int lcd_type, ss_level;
	
    pll_reg = pConf->lcd_timing.pll_ctrl;
    div_reg = pConf->lcd_timing.div_ctrl | 0x3;    
	ss_level = ((pConf->lcd_timing.clk_ctrl) >>16) & 0xf;
    pll_sel = ((pConf->lcd_timing.clk_ctrl) >>12) & 0x1;
    //pll_div_sel = ((pConf->lcd_timing.clk_ctrl) >>8) & 0x1;
	pll_div_sel = 1;
    vclk_sel = ((pConf->lcd_timing.clk_ctrl) >>4) & 0x1;
	//xd = pConf->lcd_timing.clk_ctrl & 0xf;
	xd = 1;
	
	lcd_type = pConf->lcd_basic.lcd_type;
	
    pll_div_post = 7;

    phy_clk_div2 = 0;
	
	div_reg = (div_reg | (1 << 8) | (1 << 11) | ((pll_div_post-1) << 12) | (phy_clk_div2 << 10));
	printk("ss_level=%d, pll_sel=%d, pll_div_sel=%d, vclk_sel=%d, pll_reg=0x%x, div_reg=0x%x, xd=%d.\n", ss_level, pll_sel, pll_div_sel, vclk_sel, pll_reg, div_reg, xd);
    vclk_set_lcd(lcd_type, pll_sel, pll_div_sel, vclk_sel, pll_reg, div_reg, xd);					 
	set_video_spread_spectrum(pll_sel, ss_level);
	
	clk_util_lvds_set_clk_div(1, pll_div_post, phy_clk_div2);
	
    aml_write_reg32(P_LVDS_PHY_CNTL0, 0xffff );

    //    lvds_gen_cntl       <= {10'h0,      // [15:4] unused
    //                            2'h1,       // [5:4] divide by 7 in the PHY
    //                            1'b0,       // [3] fifo_en
    //                            1'b0,       // [2] wr_bist_gate
    //                            2'b00};     // [1:0] fifo_wr mode
    FIFO_CLK_SEL = 1; // div7
    rd_data = aml_read_reg32(P_LVDS_GEN_CNTL);
    rd_data &= 0xffcf | (FIFO_CLK_SEL<< 4);
    aml_write_reg32(P_LVDS_GEN_CNTL, rd_data);

    // Set Hard Reset lvds_phy_ser_top
    aml_write_reg32(P_LVDS_PHY_CLK_CNTL, aml_read_reg32(P_LVDS_PHY_CLK_CNTL) & 0x7fff);
    // Release Hard Reset lvds_phy_ser_top
    aml_write_reg32(P_LVDS_PHY_CLK_CNTL, aml_read_reg32(P_LVDS_PHY_CLK_CNTL) | 0x8000);	
}

static void set_pll_mlvds(Lcd_Config_t *pConf)
{
    PRINT_INFO("%s\n", __FUNCTION__);

    int test_bit_num = pConf->lcd_basic.lcd_bits;
    int test_dual_gate = pConf->lvds_mlvds_config.mlvds_config->test_dual_gate;
    int test_pair_num= pConf->lvds_mlvds_config.mlvds_config->test_pair_num;
    int pll_div_post;
    int phy_clk_div2;
    int FIFO_CLK_SEL;
    int MPCLK_DELAY;
    int MCLK_half;
    int MCLK_half_delay;
    unsigned int data32;
    unsigned long mclk_pattern_dual_6_6;
    int test_high_phase = (test_bit_num != 8) | test_dual_gate;
    unsigned long rd_data;

    unsigned pll_reg, div_reg, xd;
    int pll_sel, pll_div_sel, vclk_sel;
	int lcd_type, ss_level;
	
    pll_reg = pConf->lcd_timing.pll_ctrl;
    div_reg = pConf->lcd_timing.div_ctrl | 0x3;    
	ss_level = ((pConf->lcd_timing.clk_ctrl) >>16) & 0xf;
    pll_sel = ((pConf->lcd_timing.clk_ctrl) >>12) & 0x1;
    //pll_div_sel = ((pConf->lcd_timing.clk_ctrl) >>8) & 0x1;
	pll_div_sel = 1;
    vclk_sel = ((pConf->lcd_timing.clk_ctrl) >>4) & 0x1;
	//xd = pConf->lcd_timing.clk_ctrl & 0xf;
	xd = 1;
	
	lcd_type = pConf->lcd_basic.lcd_type;

    switch(pConf->lvds_mlvds_config.mlvds_config->TL080_phase)
    {
      case 0 :
        mclk_pattern_dual_6_6 = 0xc3c3c3;
        MCLK_half = 1;
        break;
      case 1 :
        mclk_pattern_dual_6_6 = 0xc3c3c3;
        MCLK_half = 0;
        break;
      case 2 :
        mclk_pattern_dual_6_6 = 0x878787;
        MCLK_half = 1;
        break;
      case 3 :
        mclk_pattern_dual_6_6 = 0x878787;
        MCLK_half = 0;
        break;
      case 4 :
        mclk_pattern_dual_6_6 = 0x3c3c3c;
        MCLK_half = 1;
        break;
       case 5 :
        mclk_pattern_dual_6_6 = 0x3c3c3c;
        MCLK_half = 0;
        break;
       case 6 :
        mclk_pattern_dual_6_6 = 0x787878;
        MCLK_half = 1;
        break;
      default : // case 7
        mclk_pattern_dual_6_6 = 0x787878;
        MCLK_half = 0;
        break;
    }

    pll_div_post = (test_bit_num == 8) ?
                      (
                        test_dual_gate ? 4 :
                                         8
                      ) :
                      (
                        test_dual_gate ? 3 :
                                         6
                      ) ;

    phy_clk_div2 = (test_pair_num != 3);
	
	div_reg = (div_reg | (1 << 8) | (1 << 11) | ((pll_div_post-1) << 12) | (phy_clk_div2 << 10));
	printk("ss_level=%d, pll_sel=%d, pll_div_sel=%d, vclk_sel=%d, pll_reg=0x%x, div_reg=0x%x, xd=%d.\n", ss_level, pll_sel, pll_div_sel, vclk_sel, pll_reg, div_reg, xd);
    vclk_set_lcd(lcd_type, pll_sel, pll_div_sel, vclk_sel, pll_reg, div_reg, xd);	  					 
	set_video_spread_spectrum(pll_sel, ss_level);
	
	clk_util_lvds_set_clk_div(1, pll_div_post, phy_clk_div2);	
	
	//enable v2_clk div
    // aml_write_reg32(P_ HHI_VIID_CLK_CNTL, aml_read_reg32(P_HHI_VIID_CLK_CNTL) | (0xF << 0) );
    // aml_write_reg32(P_ HHI_VID_CLK_CNTL, aml_read_reg32(P_HHI_VID_CLK_CNTL) | (0xF << 0) );

    aml_write_reg32(P_LVDS_PHY_CNTL0, 0xffff );

    //    lvds_gen_cntl       <= {10'h0,      // [15:4] unused
    //                            2'h1,       // [5:4] divide by 7 in the PHY
    //                            1'b0,       // [3] fifo_en
    //                            1'b0,       // [2] wr_bist_gate
    //                            2'b00};     // [1:0] fifo_wr mode

    FIFO_CLK_SEL = (test_bit_num == 8) ? 2 : // div8
                                    0 ; // div6
    rd_data = aml_read_reg32(P_LVDS_GEN_CNTL);
    rd_data = (rd_data & 0xffcf) | (FIFO_CLK_SEL<< 4);
    aml_write_reg32(P_LVDS_GEN_CNTL, rd_data);

    MPCLK_DELAY = (test_pair_num == 6) ?
                  ((test_bit_num == 8) ? (test_dual_gate ? 5 : 3) : 2) :
                  ((test_bit_num == 8) ? 3 : 3) ;

    MCLK_half_delay = pConf->lvds_mlvds_config.mlvds_config->phase_select ? MCLK_half :
                      (
                      test_dual_gate &
                      (test_bit_num == 8) &
                      (test_pair_num != 6)
                      );

    if(test_high_phase)
    {
        if(test_dual_gate)
        data32 = (MPCLK_DELAY << mpclk_dly) |
                 (((test_bit_num == 8) ? 3 : 2) << mpclk_div) |
                 (1 << use_mpclk) |
                 (MCLK_half_delay << mlvds_clk_half_delay) |
                 (((test_bit_num == 8) ? (
                                           (test_pair_num == 6) ? 0x999999 : // DIV4
                                                                  0x555555   // DIV2
                                         ) :
                                         (
                                           (test_pair_num == 6) ? mclk_pattern_dual_6_6 : // DIV8
                                                                  0x999999   // DIV4
                                         )
                                         ) << mlvds_clk_pattern);      // DIV 8
        else if(test_bit_num == 8)
            data32 = (MPCLK_DELAY << mpclk_dly) |
                     (((test_bit_num == 8) ? 3 : 2) << mpclk_div) |
                     (1 << use_mpclk) |
                     (0 << mlvds_clk_half_delay) |
                     (0xc3c3c3 << mlvds_clk_pattern);      // DIV 8
        else
            data32 = (MPCLK_DELAY << mpclk_dly) |
                     (((test_bit_num == 8) ? 3 : 2) << mpclk_div) |
                     (1 << use_mpclk) |
                     (0 << mlvds_clk_half_delay) |
                     (
                       (
                         (test_pair_num == 6) ? 0xc3c3c3 : // DIV8
                                                0x999999   // DIV4
                       ) << mlvds_clk_pattern
                     );
    }
    else
    {
        if(test_pair_num == 6)
        {
            data32 = (MPCLK_DELAY << mpclk_dly) |
                     (((test_bit_num == 8) ? 3 : 2) << mpclk_div) |
                     (1 << use_mpclk) |
                     (0 << mlvds_clk_half_delay) |
                     (
                       (
                         (test_pair_num == 6) ? 0x999999 : // DIV4
                                                0x555555   // DIV2
                       ) << mlvds_clk_pattern
                     );
        }
        else
        {
            data32 = (1 << mlvds_clk_half_delay) |
                   (0x555555 << mlvds_clk_pattern);      // DIV 2
        }
    }

    aml_write_reg32(P_MLVDS_CLK_CTL_HI,  (data32 >> 16));
    aml_write_reg32(P_MLVDS_CLK_CTL_LO, (data32 & 0xffff));

	//pll_div_sel
    if(1){
		// Set Soft Reset vid_pll_div_pre
		aml_write_reg32(P_HHI_VIID_DIVIDER_CNTL, aml_read_reg32(P_HHI_VIID_DIVIDER_CNTL) | 0x00008);
		// Set Hard Reset vid_pll_div_post
		aml_write_reg32(P_HHI_VIID_DIVIDER_CNTL, aml_read_reg32(P_HHI_VIID_DIVIDER_CNTL) & 0x1fffd);
		// Set Hard Reset lvds_phy_ser_top
		aml_write_reg32(P_LVDS_PHY_CLK_CNTL, aml_read_reg32(P_LVDS_PHY_CLK_CNTL) & 0x7fff);
		// Release Hard Reset lvds_phy_ser_top
		aml_write_reg32(P_LVDS_PHY_CLK_CNTL, aml_read_reg32(P_LVDS_PHY_CLK_CNTL) | 0x8000);
		// Release Hard Reset vid_pll_div_post
		aml_write_reg32(P_HHI_VIID_DIVIDER_CNTL, aml_read_reg32(P_HHI_VIID_DIVIDER_CNTL) | 0x00002);
		// Release Soft Reset vid_pll_div_pre
		aml_write_reg32(P_HHI_VIID_DIVIDER_CNTL, aml_read_reg32(P_HHI_VIID_DIVIDER_CNTL) & 0x1fff7);
	}
	else{
		// Set Soft Reset vid_pll_div_pre
		aml_write_reg32(P_HHI_VID_DIVIDER_CNTL, aml_read_reg32(P_HHI_VID_DIVIDER_CNTL) | 0x00008);
		// Set Hard Reset vid_pll_div_post
		aml_write_reg32(P_HHI_VID_DIVIDER_CNTL, aml_read_reg32(P_HHI_VID_DIVIDER_CNTL) & 0x1fffd);
		// Set Hard Reset lvds_phy_ser_top
		aml_write_reg32(P_LVDS_PHY_CLK_CNTL, aml_read_reg32(P_LVDS_PHY_CLK_CNTL) & 0x7fff);
		// Release Hard Reset lvds_phy_ser_top
		aml_write_reg32(P_LVDS_PHY_CLK_CNTL, aml_read_reg32(P_LVDS_PHY_CLK_CNTL) | 0x8000);
		// Release Hard Reset vid_pll_div_post
		aml_write_reg32(P_HHI_VID_DIVIDER_CNTL, aml_read_reg32(P_HHI_VID_DIVIDER_CNTL) | 0x00002);
		// Release Soft Reset vid_pll_div_pre
		aml_write_reg32(P_HHI_VID_DIVIDER_CNTL, aml_read_reg32(P_HHI_VID_DIVIDER_CNTL) & 0x1fff7);
    }	
}

static void venc_set_ttl(Lcd_Config_t *pConf)
{
    PRINT_INFO("%s\n", __FUNCTION__);
	aml_write_reg32(P_ENCT_VIDEO_EN,           0);
#ifdef CONFIG_AM_TV_OUTPUT2
    if(vpp2_sel){
        aml_write_reg32(P_VPU_VIU_VENC_MUX_CTRL, (aml_read_reg32(P_VPU_VIU_VENC_MUX_CTRL)&(~(0x3<<2)))|(0x3<<2)); //viu2 select enct
    }
    else{
        aml_write_reg32(P_VPU_VIU_VENC_MUX_CTRL, (aml_read_reg32(P_VPU_VIU_VENC_MUX_CTRL)&(~0xff3))|0x443); //viu1 select enct,Select encT clock to VDIN, Enable VIU of ENC_T domain to VDIN
    }
#else
    aml_write_reg32(P_VPU_VIU_VENC_MUX_CTRL,
       (3<<0) |    // viu1 select enct
       (3<<2)      // viu2 select enct
    );
#endif    
    aml_write_reg32(P_ENCT_VIDEO_MODE,        0);
    aml_write_reg32(P_ENCT_VIDEO_MODE_ADV,    0x0418);  
	  
	// bypass filter
    aml_write_reg32(P_ENCT_VIDEO_FILT_CTRL,    0x1000);
    aml_write_reg32(P_VENC_DVI_SETTING,        0x11);
    aml_write_reg32(P_VENC_VIDEO_PROG_MODE,    0x100);

    aml_write_reg32(P_ENCT_VIDEO_MAX_PXCNT,    pConf->lcd_basic.h_period - 1);
    aml_write_reg32(P_ENCT_VIDEO_MAX_LNCNT,    pConf->lcd_basic.v_period - 1);

    aml_write_reg32(P_ENCT_VIDEO_HAVON_BEGIN,  pConf->lcd_timing.video_on_pixel);
    aml_write_reg32(P_ENCT_VIDEO_HAVON_END,    pConf->lcd_basic.h_active - 1 + pConf->lcd_timing.video_on_pixel);
    aml_write_reg32(P_ENCT_VIDEO_VAVON_BLINE,  pConf->lcd_timing.video_on_line);
    aml_write_reg32(P_ENCT_VIDEO_VAVON_ELINE,  pConf->lcd_basic.v_active + 3  + pConf->lcd_timing.video_on_line);

    aml_write_reg32(P_ENCT_VIDEO_HSO_BEGIN,    15);
    aml_write_reg32(P_ENCT_VIDEO_HSO_END,      31);
    aml_write_reg32(P_ENCT_VIDEO_VSO_BEGIN,    15);
    aml_write_reg32(P_ENCT_VIDEO_VSO_END,      31);
    aml_write_reg32(P_ENCT_VIDEO_VSO_BLINE,    0);
    aml_write_reg32(P_ENCT_VIDEO_VSO_ELINE,    2);    
    
    // enable enct
    aml_write_reg32(P_ENCT_VIDEO_EN,           1);
}

static void venc_set_lvds(Lcd_Config_t *pConf)
{
    PRINT_INFO("%s\n", __FUNCTION__);
    
	aml_write_reg32(P_ENCL_VIDEO_EN,           0);
	//int havon_begin = 80;
#ifdef CONFIG_AM_TV_OUTPUT2
    if(vpp2_sel){
        aml_write_reg32(P_VPU_VIU_VENC_MUX_CTRL, (aml_read_reg32(P_VPU_VIU_VENC_MUX_CTRL)&(~(0x3<<2)))|(0x0<<2)); //viu2 select encl
    }
    else{
        aml_write_reg32(P_VPU_VIU_VENC_MUX_CTRL, (aml_read_reg32(P_VPU_VIU_VENC_MUX_CTRL)&(~0xff3))|0x880); //viu1 select encl,Select encL clock to VDIN, Enable VIU of ENC_L domain to VDIN
    }
#else
    aml_write_reg32(P_VPU_VIU_VENC_MUX_CTRL,
       (0<<0) |    // viu1 select encl
       (0<<2)      // viu2 select encl
       );
#endif	
	aml_write_reg32(P_ENCL_VIDEO_MODE,         0x0040 | (1<<14)); // Enable Hsync and equalization pulse switch in center; bit[14] cfg_de_v = 1
	aml_write_reg32(P_ENCL_VIDEO_MODE_ADV,     0x0008); // Sampling rate: 1

	// bypass filter
 	aml_write_reg32(P_ENCL_VIDEO_FILT_CTRL	,0x1000);
	
	aml_write_reg32(P_ENCL_VIDEO_MAX_PXCNT,	pConf->lcd_basic.h_period - 1);
	aml_write_reg32(P_ENCL_VIDEO_MAX_LNCNT,	pConf->lcd_basic.v_period - 1);

	aml_write_reg32(P_ENCL_VIDEO_HAVON_BEGIN,	pConf->lcd_timing.video_on_pixel);
	aml_write_reg32(P_ENCL_VIDEO_HAVON_END,		pConf->lcd_basic.h_active - 1 + pConf->lcd_timing.video_on_pixel);
	aml_write_reg32(P_ENCL_VIDEO_VAVON_BLINE,	pConf->lcd_timing.video_on_line);
	aml_write_reg32(P_ENCL_VIDEO_VAVON_ELINE,	pConf->lcd_basic.v_active + 1  + pConf->lcd_timing.video_on_line);

	aml_write_reg32(P_ENCL_VIDEO_HSO_BEGIN,	pConf->lcd_timing.sth1_hs_addr);//10);
	aml_write_reg32(P_ENCL_VIDEO_HSO_END,	pConf->lcd_timing.sth1_he_addr);//20);
	aml_write_reg32(P_ENCL_VIDEO_VSO_BEGIN,	pConf->lcd_timing.stv1_hs_addr);//10);
	aml_write_reg32(P_ENCL_VIDEO_VSO_END,	pConf->lcd_timing.stv1_he_addr);//20);
	aml_write_reg32(P_ENCL_VIDEO_VSO_BLINE,	pConf->lcd_timing.stv1_vs_addr);//2);
	aml_write_reg32(P_ENCL_VIDEO_VSO_ELINE,	pConf->lcd_timing.stv1_ve_addr);//4);
		
	aml_write_reg32(P_ENCL_VIDEO_RGBIN_CTRL, 	0);
    	
	// enable encl
    aml_write_reg32(P_ENCL_VIDEO_EN,           1);
}

static void venc_set_mlvds(Lcd_Config_t *pConf)
{
    PRINT_INFO("%s\n", __FUNCTION__);

    aml_write_reg32(P_ENCL_VIDEO_EN,           0);

#ifdef CONFIG_AM_TV_OUTPUT2
    if(vpp2_sel){
        aml_write_reg32(P_VPU_VIU_VENC_MUX_CTRL, (aml_read_reg32(P_VPU_VIU_VENC_MUX_CTRL)&(~(0x3<<2)))|(0x0<<2)); //viu2 select encl
    }
    else{
        aml_write_reg32(P_VPU_VIU_VENC_MUX_CTRL, (aml_read_reg32(P_VPU_VIU_VENC_MUX_CTRL)&(~(0x3<<0)))|(0x0<<0)); //viu1 select encl
    }
#else
    aml_write_reg32(P_VPU_VIU_VENC_MUX_CTRL,
       (0<<0) |    // viu1 select encl
       (0<<2)      // viu2 select encl
       );
#endif
	int ext_pixel = pConf->lvds_mlvds_config.mlvds_config->test_dual_gate ? pConf->lvds_mlvds_config.mlvds_config->total_line_clk : 0;
	int active_h_start = pConf->lcd_timing.video_on_pixel;
	int active_v_start = pConf->lcd_timing.video_on_line;
	int width = pConf->lcd_basic.h_active;
	int height = pConf->lcd_basic.v_active;
	int max_height = pConf->lcd_basic.v_period;

	aml_write_reg32(P_ENCL_VIDEO_MODE,             0x0040 | (1<<14)); // Enable Hsync and equalization pulse switch in center; bit[14] cfg_de_v = 1
	aml_write_reg32(P_ENCL_VIDEO_MODE_ADV,         0x0008); // Sampling rate: 1
	
	// bypass filter
 	aml_write_reg32(P_ENCL_VIDEO_FILT_CTRL,	0x1000);
	
	aml_write_reg32(P_ENCL_VIDEO_YFP1_HTIME,       active_h_start);
	aml_write_reg32(P_ENCL_VIDEO_YFP2_HTIME,       active_h_start + width);

	aml_write_reg32(P_ENCL_VIDEO_MAX_PXCNT,        pConf->lvds_mlvds_config.mlvds_config->total_line_clk - 1 + ext_pixel);
	aml_write_reg32(P_ENCL_VIDEO_MAX_LNCNT,        max_height - 1);

	aml_write_reg32(P_ENCL_VIDEO_HAVON_BEGIN,      active_h_start);
	aml_write_reg32(P_ENCL_VIDEO_HAVON_END,        active_h_start + width - 1);  // for dual_gate mode still read 1408 pixel at first half of line
	aml_write_reg32(P_ENCL_VIDEO_VAVON_BLINE,      active_v_start);
	aml_write_reg32(P_ENCL_VIDEO_VAVON_ELINE,      active_v_start + height -1);  //15+768-1);

	aml_write_reg32(P_ENCL_VIDEO_HSO_BEGIN,        24);
	aml_write_reg32(P_ENCL_VIDEO_HSO_END,          1420 + ext_pixel);
	aml_write_reg32(P_ENCL_VIDEO_VSO_BEGIN,        1400 + ext_pixel);
	aml_write_reg32(P_ENCL_VIDEO_VSO_END,          1410 + ext_pixel);
	aml_write_reg32(P_ENCL_VIDEO_VSO_BLINE,        1);
	aml_write_reg32(P_ENCL_VIDEO_VSO_ELINE,        3);

	aml_write_reg32(P_ENCL_VIDEO_RGBIN_CTRL, 	0); 	

	// enable encl
    aml_write_reg32(P_ENCL_VIDEO_EN,           1);
}

static void set_control_lvds(Lcd_Config_t *pConf)
{
    PRINT_INFO("%s\n", __FUNCTION__);
	
	int lvds_repack, pn_swap, bit_num;
	unsigned long data32;    

    data32 = (0x00 << LVDS_blank_data_r) |
             (0x00 << LVDS_blank_data_g) |
             (0x00 << LVDS_blank_data_b) ; 
    aml_write_reg32(P_LVDS_BLANK_DATA_HI,  (data32 >> 16));
    aml_write_reg32(P_LVDS_BLANK_DATA_LO, (data32 & 0xffff));
	
	aml_write_reg32(P_LVDS_PHY_CNTL0, 0xffff );
	aml_write_reg32(P_LVDS_PHY_CNTL1, 0xff00 );
	
	//aml_write_reg32(P_ENCL_VIDEO_EN,           1);	
	lvds_repack = (pConf->lvds_mlvds_config.lvds_config->lvds_repack) & 0x1;
	pn_swap = (pConf->lvds_mlvds_config.lvds_config->pn_swap) & 0x1;
	switch(pConf->lcd_basic.lcd_bits)
	{
		case 10:
			bit_num=0;
			break;
		case 8:
			bit_num=1;
			break;
		case 6:
			bit_num=2;
			break;
		case 4:
			bit_num=3;
			break;
		default:
			bit_num=1;
			break;
	}
	
	aml_write_reg32(P_MLVDS_CONTROL,  (aml_read_reg32(P_MLVDS_CONTROL) & ~(1 << 0)));  //disable mlvds	
    
	aml_write_reg32(P_LVDS_PACK_CNTL_ADDR, 
					( lvds_repack<<0 ) | // repack
					( 0<<2 ) | // odd_even
					( 0<<3 ) | // reserve
					( 0<<4 ) | // lsb first
					( pn_swap<<5 ) | // pn swap
					( 0<<6 ) | // dual port
					( 0<<7 ) | // use tcon control
					( bit_num<<8 ) | // 0:10bits, 1:8bits, 2:6bits, 3:4bits.
					( 0<<10 ) | //r_select  //0:R, 1:G, 2:B, 3:0
					( 1<<12 ) | //g_select  //0:R, 1:G, 2:B, 3:0
					( 2<<14 ));  //b_select  //0:R, 1:G, 2:B, 3:0; 
				   
    aml_write_reg32(P_LVDS_GEN_CNTL, (aml_read_reg32(P_LVDS_GEN_CNTL) | (1 << 0)) );  //fifo enable  
    //aml_write_reg32(P_LVDS_GEN_CNTL, (aml_read_reg32(P_LVDS_GEN_CNTL) | (1 << 3))); // enable fifo
	
	//PRINT_INFO("lvds fifo clk = %d.\n", clk_util_clk_msr(LVDS_FIFO_CLK));	
}

static void set_control_mlvds(Lcd_Config_t *pConf)
{
    PRINT_INFO("%s\n", __FUNCTION__);
	
	int test_bit_num = pConf->lcd_basic.lcd_bits;
    int test_pair_num = pConf->lvds_mlvds_config.mlvds_config->test_pair_num;
    int test_dual_gate = pConf->lvds_mlvds_config.mlvds_config->test_dual_gate;
    int scan_function = pConf->lvds_mlvds_config.mlvds_config->scan_function;     //0:U->D,L->R  //1:D->U,R->L
    int mlvds_insert_start;
    unsigned int reset_offset;
    unsigned int reset_length;

    unsigned long data32;
    
    mlvds_insert_start = test_dual_gate ?
                           ((test_bit_num == 8) ? ((test_pair_num == 6) ? 0x9f : 0xa9) :
                                                  ((test_pair_num == 6) ? pConf->lvds_mlvds_config.mlvds_config->mlvds_insert_start : 0xa7)
                           ) :
                           (
                             (test_pair_num == 6) ? ((test_bit_num == 8) ? 0xa9 : 0xa7) :
                                                    ((test_bit_num == 8) ? 0xae : 0xad)
                           );

    // Enable the LVDS PHY (power down bits)
    aml_write_reg32(P_LVDS_PHY_CNTL1,aml_read_reg32(P_LVDS_PHY_CNTL1) | (0x7F << 8) );

    data32 = (0x00 << LVDS_blank_data_r) |
             (0x00 << LVDS_blank_data_g) |
             (0x00 << LVDS_blank_data_b) ;
    aml_write_reg32(P_LVDS_BLANK_DATA_HI,  (data32 >> 16));
    aml_write_reg32(P_LVDS_BLANK_DATA_LO, (data32 & 0xffff));

    data32 = 0x7fffffff; //  '0'x1 + '1'x32 + '0'x2
    aml_write_reg32(P_MLVDS_RESET_PATTERN_HI,  (data32 >> 16));
    aml_write_reg32(P_MLVDS_RESET_PATTERN_LO, (data32 & 0xffff));
    data32 = 0x8000; // '0'x1 + '1'x32 + '0'x2
    aml_write_reg32(P_MLVDS_RESET_PATTERN_EXT,  (data32 & 0xffff));

    reset_length = 1+32+2;
    reset_offset = test_bit_num - (reset_length%test_bit_num);

    data32 = (reset_offset << mLVDS_reset_offset) |
             (reset_length << mLVDS_reset_length) |
             ((test_pair_num == 6) << mLVDS_data_write_toggle) |
             ((test_pair_num != 6) << mLVDS_data_write_ini) |
             ((test_pair_num == 6) << mLVDS_data_latch_1_toggle) |
             (0 << mLVDS_data_latch_1_ini) |
             ((test_pair_num == 6) << mLVDS_data_latch_0_toggle) |
             (1 << mLVDS_data_latch_0_ini) |
             ((test_pair_num == 6) << mLVDS_reset_1_select) |
             (mlvds_insert_start << mLVDS_reset_start);
    aml_write_reg32(P_MLVDS_CONFIG_HI,  (data32 >> 16));
    aml_write_reg32(P_MLVDS_CONFIG_LO, (data32 & 0xffff));

    data32 = (1 << mLVDS_double_pattern) |  //POL double pattern
			 (0x3f << mLVDS_ins_reset) |
             (test_dual_gate << mLVDS_dual_gate) |
             ((test_bit_num == 8) << mLVDS_bit_num) |
             ((test_pair_num == 6) << mLVDS_pair_num) |
             (0 << mLVDS_msb_first) |
             (0 << mLVDS_PORT_SWAP) |
             ((scan_function==1 ? 1:0) << mLVDS_MLSB_SWAP) |
             (0 << mLVDS_PN_SWAP) |
             (1 << mLVDS_en);
    aml_write_reg32(P_MLVDS_CONTROL,  (data32 & 0xffff));

    aml_write_reg32(P_LVDS_PACK_CNTL_ADDR,
                   ( 0 ) | // repack
                   ( 0<<2 ) | // odd_even
                   ( 0<<3 ) | // reserve
                   ( 0<<4 ) | // lsb first
                   ( 0<<5 ) | // pn swap
                   ( 0<<6 ) | // dual port
                   ( 0<<7 ) | // use tcon control
                   ( 1<<8 ) | // 0:10bits, 1:8bits, 2:6bits, 3:4bits.
                   ( (scan_function==1 ? 2:0)<<10 ) |  //r_select // 0:R, 1:G, 2:B, 3:0
                   ( 1<<12 ) |                        //g_select
                   ( (scan_function==1 ? 0:2)<<14 ));  //b_select

    aml_write_reg32(P_L_POL_CNTL_ADDR,  (1 << LCD_DCLK_SEL) |
       //(0x1 << LCD_HS_POL) |
       (0x1 << LCD_VS_POL)
    );

    //aml_write_reg32(P_LVDS_GEN_CNTL, (aml_read_reg32(P_LVDS_GEN_CNTL) | (1 << 3))); // enable fifo
}

static void init_lvds_phy(Lcd_Config_t *pConf)
{
    PRINT_INFO("%s\n", __FUNCTION__);
	
	unsigned tmp_add_data;
	
    aml_write_reg32(P_LVDS_PHY_CNTL3, 0x0ee0);      //0x0ee1
    aml_write_reg32(P_LVDS_PHY_CNTL4 ,0);
	
    tmp_add_data  = 0;
    tmp_add_data |= (pConf->lvds_mlvds_config.lvds_phy_control->lvds_prem_ctl & 0xf) << 0; //LVDS_PREM_CTL<3:0>=<1111>
    tmp_add_data |= (pConf->lvds_mlvds_config.lvds_phy_control->lvds_swing_ctl & 0xf) << 4; //LVDS_SWING_CTL<3:0>=<0011>    
    tmp_add_data |= (pConf->lvds_mlvds_config.lvds_phy_control->lvds_vcm_ctl & 0x7) << 8; //LVDS_VCM_CTL<2:0>=<001>
	tmp_add_data |= (pConf->lvds_mlvds_config.lvds_phy_control->lvds_ref_ctl & 0x1f) << 11; //LVDS_REFCTL<4:0>=<01111> 
	aml_write_reg32(P_LVDS_PHY_CNTL5, tmp_add_data);

	aml_write_reg32(P_LVDS_PHY_CNTL0,0x001f);
	aml_write_reg32(P_LVDS_PHY_CNTL1,0xffff);	

    aml_write_reg32(P_LVDS_PHY_CNTL6,0xcccc);
    aml_write_reg32(P_LVDS_PHY_CNTL7,0xcccc);
    aml_write_reg32(P_LVDS_PHY_CNTL8,0xcccc);
	
    //aml_write_reg32(P_LVDS_PHY_CNTL4, aml_read_reg32(P_LVDS_PHY_CNTL4) | (0x7f<<0));  //enable LVDS phy port..	
}

#include <mach/mod_gate.h>

static void switch_lcd_gates(Lcd_Type_t lcd_type)
{
    switch(lcd_type){
        case LCD_DIGITAL_TTL:
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6
            switch_mod_gate_by_name("tcon", 1);
            switch_mod_gate_by_name("lvds", 0);
#endif
            break;
        case LCD_DIGITAL_LVDS:
        case LCD_DIGITAL_MINILVDS:
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6
            switch_mod_gate_by_name("lvds", 1);
            switch_mod_gate_by_name("tcon", 0);
#endif
            break;
        default:
            break;
    }
}

static inline void _init_display_driver(Lcd_Config_t *pConf)
{ 
    int lcd_type;
	const char* lcd_type_table[]={
		"NULL",
		"TTL",
		"LVDS",
		"miniLVDS",
		"invalid",
	}; 
	
	lcd_type = pDev->conf.lcd_basic.lcd_type;
	printk("\nInit LCD type: %s.\n", lcd_type_table[lcd_type]);
	printk("lcd frame rate=%d/%d.\n", pDev->conf.lcd_timing.sync_duration_num, pDev->conf.lcd_timing.sync_duration_den);
	
    switch_lcd_gates(lcd_type);

	switch(lcd_type){
        case LCD_DIGITAL_TTL:
            set_pll_ttl(pConf);
            venc_set_ttl(pConf);
			set_tcon_ttl(pConf);    
            break;
        case LCD_DIGITAL_LVDS:
        	set_pll_lvds(pConf);        	
            venc_set_lvds(pConf);        	
        	set_control_lvds(pConf);        	
        	init_lvds_phy(pConf);
			set_tcon_lvds(pConf);  	
            break;
        case LCD_DIGITAL_MINILVDS:
			set_pll_mlvds(pConf);
			venc_set_mlvds(pConf);
			set_control_mlvds(pConf);
			init_lvds_phy(pConf);
			set_tcon_mlvds(pConf);  	
            break;
        default:
            printk("Invalid LCD type.\n");
			break;
    }
}

static inline void _disable_display_driver(Lcd_Config_t *pConf)
{	
    int pll_sel, vclk_sel;	    
    
    pll_sel = ((pConf->lcd_timing.clk_ctrl) >>12) & 0x1;
    vclk_sel = ((pConf->lcd_timing.clk_ctrl) >>4) & 0x1;	
	
	aml_set_reg32_bits(P_HHI_VIID_DIVIDER_CNTL, 0, 11, 1);	//close lvds phy clk gate: 0x104c[11]
	
	aml_write_reg32(P_ENCT_VIDEO_EN, 0);	//disable enct
	aml_write_reg32(P_ENCL_VIDEO_EN, 0);	//disable encl
	
	if (vclk_sel)
		aml_set_reg32_bits(P_HHI_VIID_CLK_CNTL, 0, 0, 5);		//close vclk2 gate: 0x104b[4:0]
	else
		aml_set_reg32_bits(P_HHI_VID_CLK_CNTL, 0, 0, 5);		//close vclk1 gate: 0x105f[4:0]
	
	if (pll_sel){	
		aml_set_reg32_bits(P_HHI_VIID_DIVIDER_CNTL, 0, 16, 1);	//close vid2_pll gate: 0x104c[16]
		aml_set_reg32_bits(P_HHI_VIID_PLL_CNTL, 1, 30, 1);		//power down vid2_pll: 0x1047[30]
	}
	else{
		aml_set_reg32_bits(P_HHI_VID_DIVIDER_CNTL, 0, 16, 1);	//close vid1_pll gate: 0x1066[16]
		aml_set_reg32_bits(P_HHI_VID_PLL_CNTL, 1, 30, 1);		//power down vid1_pll: 0x105c[30]
	}
	printk("disable lcd display driver.\n");
}

static inline void _enable_vsync_interrupt(void)
{
    if ((aml_read_reg32(P_ENCT_VIDEO_EN) & 1) || (aml_read_reg32(P_ENCL_VIDEO_EN) & 1)) {
        aml_write_reg32(P_VENC_INTCTRL, 0x200);
#if 0
        while ((aml_read_reg32(P_VENC_INTFLAG) & 0x200) == 0) {
            u32 line1, line2;

            line1 = line2 = aml_read_reg32(P_VENC_ENCP_LINE);

            while (line1 >= line2) {
                line2 = line1;
                line1 = aml_read_reg32(P_VENC_ENCP_LINE);
            }

            aml_read_reg32(P_VENC_INTFLAG);
            aml_read_reg32(P_VENC_INTFLAG);
            aml_read_reg32(P_VENC_INTFLAG);
            aml_read_reg32(P_VENC_INTFLAG);
            if (aml_read_reg32(P_VENC_INTFLAG) & 0x200) {
                break;
            }

            aml_write_reg32(P_ENCP_VIDEO_EN, 0);
            aml_read_reg32(P_VENC_INTFLAG);
            aml_read_reg32(P_VENC_INTFLAG);
            aml_read_reg32(P_VENC_INTFLAG);
            aml_read_reg32(P_VENC_INTFLAG);
            aml_read_reg32(P_VENC_INTFLAG);

            aml_write_reg32(P_ENCP_VIDEO_EN, 1);
            aml_read_reg32(P_VENC_INTFLAG);
            aml_read_reg32(P_VENC_INTFLAG);
            aml_read_reg32(P_VENC_INTFLAG);
            aml_read_reg32(P_VENC_INTFLAG);
            aml_read_reg32(P_VENC_INTFLAG);
        }
#endif
    }
    else{
        aml_write_reg32(P_VENC_INTCTRL, 0x2);
    }
}
static void _enable_backlight(u32 brightness_level)
{
    pDev->conf.lcd_power_ctrl.backlight_ctrl?pDev->conf.lcd_power_ctrl.backlight_ctrl(ON):0;
}
void _disable_backlight(void)
{
    pDev->conf.lcd_power_ctrl.backlight_ctrl?pDev->conf.lcd_power_ctrl.backlight_ctrl(OFF):0;
}
void _set_backlight_level(int level)
{
    pDev->conf.lcd_power_ctrl.set_bl_level?pDev->conf.lcd_power_ctrl.set_bl_level(level):0;
}
static void _lcd_module_enable(void)
{
    BUG_ON(pDev==NULL);    

	_init_display_driver(&pDev->conf);
	pDev->conf.lcd_power_ctrl.power_ctrl?pDev->conf.lcd_power_ctrl.power_ctrl(ON):0;
    
    _enable_vsync_interrupt();
}

static const vinfo_t *lcd_get_current_info(void)
{
    return &pDev->lcd_info;
}

static int lcd_set_current_vmode(vmode_t mode)
{
    if (mode != VMODE_LCD)
        return -EINVAL;
#ifdef CONFIG_AM_TV_OUTPUT2
    vpp2_sel = 0;
#endif    
    aml_write_reg32(P_VPP_POSTBLEND_H_SIZE, pDev->lcd_info.width);
    _lcd_module_enable();
    if (VMODE_INIT_NULL == pDev->lcd_info.mode)
        pDev->lcd_info.mode = VMODE_LCD;
    _enable_backlight(BL_MAX_LEVEL);
    return 0;
}

#ifdef CONFIG_AM_TV_OUTPUT2
static int lcd_set_current_vmode2(vmode_t mode)
{
    if (mode != VMODE_LCD)
        return -EINVAL;
    vpp2_sel = 1;
    aml_write_reg32(P_VPP2_POSTBLEND_H_SIZE, pDev->lcd_info.width);
    _lcd_module_enable();
    if (VMODE_INIT_NULL == pDev->lcd_info.mode)
        pDev->lcd_info.mode = VMODE_LCD;
    _enable_backlight(BL_MAX_LEVEL);
    return 0;
}
#endif
static vmode_t lcd_validate_vmode(char *mode)
{
    if ((strncmp(mode, PANEL_NAME, strlen(PANEL_NAME))) == 0)
        return VMODE_LCD;
    
    return VMODE_MAX;
}
static int lcd_vmode_is_supported(vmode_t mode)
{
    mode&=VMODE_MODE_BIT_MASK;
    if(mode == VMODE_LCD )
    return true;
    return false;
}
static int lcd_module_disable(vmode_t cur_vmod)
{
    BUG_ON(pDev==NULL);
    _disable_backlight();
    pDev->conf.lcd_power_ctrl.power_ctrl?pDev->conf.lcd_power_ctrl.power_ctrl(OFF):0;
    _disable_display_driver(&pDev->conf);
#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6
    switch_mod_gate_by_name("tcon", 0);
    switch_mod_gate_by_name("lvds", 0);
#endif

    return 0;
}
#ifdef  CONFIG_PM
static int lcd_suspend(void)
{
    BUG_ON(pDev==NULL);
    PRINT_INFO("lcd_suspend \n");
    _disable_backlight();
    pDev->conf.lcd_power_ctrl.power_ctrl?pDev->conf.lcd_power_ctrl.power_ctrl(OFF):0;
	_disable_display_driver(&pDev->conf);	
    return 0;
}
static int lcd_resume(void)
{
    PRINT_INFO("lcd_resume\n");
    _lcd_module_enable();
    _enable_backlight(BL_MAX_LEVEL);
    return 0;
}
#endif
static vout_server_t lcd_vout_server={
    .name = "lcd_vout_server",
    .op = {    
        .get_vinfo = lcd_get_current_info,
        .set_vmode = lcd_set_current_vmode,
        .validate_vmode = lcd_validate_vmode,
        .vmode_is_supported=lcd_vmode_is_supported,
        .disable=lcd_module_disable,
#ifdef  CONFIG_PM  
        .vout_suspend=lcd_suspend,
        .vout_resume=lcd_resume,
#endif
    },
};

#ifdef CONFIG_AM_TV_OUTPUT2
static vout_server_t lcd_vout2_server={
    .name = "lcd_vout2_server",
    .op = {    
        .get_vinfo = lcd_get_current_info,
        .set_vmode = lcd_set_current_vmode2,
        .validate_vmode = lcd_validate_vmode,
        .vmode_is_supported=lcd_vmode_is_supported,
        .disable=lcd_module_disable,
#ifdef  CONFIG_PM  
        .vout_suspend=lcd_suspend,
        .vout_resume=lcd_resume,
#endif
    },
};
#endif
static void _init_vout(lcd_dev_t *pDev)
{	
    pDev->lcd_info.name = PANEL_NAME;
    pDev->lcd_info.mode = VMODE_LCD;
    pDev->lcd_info.width = pDev->conf.lcd_basic.h_active;
    pDev->lcd_info.height = pDev->conf.lcd_basic.v_active;
    pDev->lcd_info.field_height = pDev->conf.lcd_basic.v_active;
    pDev->lcd_info.aspect_ratio_num = pDev->conf.lcd_basic.screen_ratio_width;
    pDev->lcd_info.aspect_ratio_den = pDev->conf.lcd_basic.screen_ratio_height;
    pDev->lcd_info.screen_real_width= pDev->conf.lcd_basic.screen_actual_width;
    pDev->lcd_info.screen_real_height= pDev->conf.lcd_basic.screen_actual_height;
    pDev->lcd_info.sync_duration_num = pDev->conf.lcd_timing.sync_duration_num;
    pDev->lcd_info.sync_duration_den = pDev->conf.lcd_timing.sync_duration_den;
       
    //add lcd actual active area size
    printk("lcd actual active area size: %d %d (mm).\n", pDev->conf.lcd_basic.screen_actual_width, pDev->conf.lcd_basic.screen_actual_height);
    vout_register_server(&lcd_vout_server);
#ifdef CONFIG_AM_TV_OUTPUT2
   vout2_register_server(&lcd_vout2_server);
#endif   
}

static void _lcd_init(Lcd_Config_t *pConf)
{
    //logo_object_t  *init_logo_obj=NULL;
    
    _init_vout(pDev);
    //init_logo_obj = get_current_logo_obj();    
    //if(NULL==init_logo_obj ||!init_logo_obj->para.loaded)
        //_lcd_module_enable();
}

#include <mach/lcd_aml.h>

static int lcd_reboot_notifier(struct notifier_block *nb, unsigned long state, void *cmd)
 {
    if (state == SYS_RESTART)
	{	
		printk("shut down lcd...\n");
		_disable_backlight();
		pDev->conf.lcd_power_ctrl.power_ctrl?pDev->conf.lcd_power_ctrl.power_ctrl(OFF):0;
	}
    return NOTIFY_DONE;
}

//****************************
//gamma debug
//****************************
#ifdef CONFIG_AML_GAMMA_DEBUG
static unsigned short gamma_adjust_r[256];
static unsigned short gamma_adjust_g[256];
static unsigned short gamma_adjust_b[256];

static void read_original_gamma_table(void)
{
    unsigned i;

    printk("read original gamma table R:\n");
    for (i=0; i<256; i++)
    {
        printk("%u,", gamma_adjust_r[i]);
    }
    printk("\n\nread original gamma table G:\n");
    for (i=0; i<256; i++)
    {
        printk("%u,", gamma_adjust_g[i]);
    }
    printk("\n\nread original gamma table B:\n");
    for (i=0; i<256; i++)
    {
        printk("%u,", gamma_adjust_b[i]);
    }
    printk("\n");
}

static void read_current_gamma_table(void)
{
	unsigned i;

	printk("read current gamma table R:\n");
    for (i=0; i<256; i++)
    {
        printk("%d ", pDev->conf.lcd_effect.GammaTableR[i]);
    }
    printk("\n\nread current gamma table G:\n");
    for (i=0; i<256; i++)
    {
        printk("%d ", pDev->conf.lcd_effect.GammaTableG[i]);
    }
    printk("\n\nread current gamma table B:\n");
    for (i=0; i<256; i++)
    {
        printk("%d ", pDev->conf.lcd_effect.GammaTableB[i]);
    }
    printk("\n");
}

static void write_gamma_table(void)
{
    if (pDev->conf.lcd_basic.lcd_type == LCD_DIGITAL_TTL)
    {
        aml_write_reg32(P_GAMMA_CNTL_PORT, aml_read_reg32(P_GAMMA_CNTL_PORT) & ~(1<<0));
        set_lcd_gamma_table_ttl(pDev->conf.lcd_effect.GammaTableR, LCD_H_SEL_R);
        set_lcd_gamma_table_ttl(pDev->conf.lcd_effect.GammaTableG, LCD_H_SEL_G);
        set_lcd_gamma_table_ttl(pDev->conf.lcd_effect.GammaTableB, LCD_H_SEL_B);
        aml_write_reg32(P_GAMMA_CNTL_PORT, aml_read_reg32(P_GAMMA_CNTL_PORT) | (1<<0));
        printk("write ttl gamma table ");
    }
    else
    {
        aml_write_reg32(P_L_GAMMA_CNTL_PORT, aml_read_reg32(P_L_GAMMA_CNTL_PORT) & ~(1<<0));
        set_lcd_gamma_table_lvds(pDev->conf.lcd_effect.GammaTableR, LCD_H_SEL_R);
        set_lcd_gamma_table_lvds(pDev->conf.lcd_effect.GammaTableG, LCD_H_SEL_G);
        set_lcd_gamma_table_lvds(pDev->conf.lcd_effect.GammaTableB, LCD_H_SEL_B);
        aml_write_reg32(P_L_GAMMA_CNTL_PORT, aml_read_reg32(P_L_GAMMA_CNTL_PORT) | (1<<0));
        printk("write lvds/mlvds gamma table ");
    }
}

static void set_gamma_coeff(unsigned r_coeff, unsigned g_coeff, unsigned b_coeff)
{
	int i;

    for (i=0; i<256; i++) {
        pDev->conf.lcd_effect.GammaTableR[i] = (unsigned short)(gamma_adjust_r[i] * r_coeff / 100);
        pDev->conf.lcd_effect.GammaTableG[i] = (unsigned short)(gamma_adjust_g[i] * g_coeff / 100);
        pDev->conf.lcd_effect.GammaTableB[i] = (unsigned short)(gamma_adjust_b[i] * b_coeff / 100);
    }

	write_gamma_table();
	printk("with scale factor R:%u\%, G:%u\%, B:%u\%.\n", r_coeff, g_coeff, b_coeff);
}

static const char * usage_str =
{"Usage:\n"
"    echo coeff <R_coeff> <G_coeff> <B_coeff> > write ; set R,G,B gamma scale factor, base on the original gamma table\n"
"data format:\n"
"    <R/G/B_coeff>  : a number in Dec(0~100), means a percent value\n"
"\n"
"    echo [r|g|b] <step> <value> <value> <value> <value> <value> <value> <value> <value> > write ; input R/G/B gamma table\n"
"    echo w [0 | 8 | 10] > write ; apply the original/8bit/10bit gamma table\n"
"data format:\n"
"    <step>  : 0xX, 4bit in Hex, there are 8 steps(0~7, 8bit gamma) or 16 steps(0~f, 10bit gamma) for a single cycle\n"
"    <value> : 0xXXXXXXXX, 32bit in Hex, 2 or 4 gamma table values (8 or 10bit gamma) combia in one <value>\n"
"\n"
"    echo f[r | g | b | w] <level_value> > write ; write R/G/B/white gamma level with fixed level_value\n"
"data format:\n"
"    <level_value>  : a number in Dec(0~255)\n"
"\n"
"    echo [0 | 1] > read ; readback original/current gamma table\n"
};

static ssize_t gamma_help(struct class *class, struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n",usage_str);
}

static ssize_t aml_lcd_gamma_read(struct class *class, 
			struct class_attribute *attr,	const char *buf, size_t count)
{
	if (buf[0] == '0')
		read_original_gamma_table();
	else
		read_current_gamma_table();

	return count;
	//return 0;
}

static unsigned gamma_adjust_r_temp[128];
static unsigned gamma_adjust_g_temp[128];
static unsigned gamma_adjust_b_temp[128];
static ssize_t aml_lcd_gamma_debug(struct class *class, struct class_attribute *attr, const char *buf, size_t count)
{
	unsigned int ret;
	unsigned int i, j;
	unsigned t[8];
	unsigned short t_bit;

    switch (buf[0])
    {
    case 'c':
        t[0] = 100;
        t[1] = 100;
        t[2] = 100;
        ret = sscanf(buf, "coeff %u %u %u", &t[0], &t[1], &t[2]);
        set_gamma_coeff(t[0], t[1], t[2]);
        break;
    case 'r':
        ret = sscanf(buf, "r %x %x %x %x %x %x %x %x %x", &i, &t[0], &t[1], &t[2], &t[3], &t[4], &t[5], &t[6], &t[7]);
        if (i<16)
        {
            i =  i * 8;
            for (j=0; j<8; j++)
            {
                gamma_adjust_r_temp[i+j] = t[j];
            }
            printk("write R table: step %u.\n", i/8);
        }
        break;
    case 'g':
        ret = sscanf(buf, "g %x %x %x %x %x %x %x %x %x", &i, &t[0], &t[1], &t[2], &t[3], &t[4], &t[5], &t[6], &t[7]);
        if (i<16)
        {
            i =  i * 8;
            for (j=0; j<8; j++)
            {
                gamma_adjust_g_temp[i+j] = t[j];
            }
            printk("write G table: step %u.\n", i/8);
        }
        break;
    case 'b':
        ret = sscanf(buf, "b %x %x %x %x %x %x %x %x %x", &i, &t[0], &t[1], &t[2], &t[3], &t[4], &t[5], &t[6], &t[7]);
        if (i<16)
        {
            i =  i * 8;
            for (j=0; j<8; j++)
            {
                gamma_adjust_b_temp[i+j] = t[j];
            }
            printk("write B table: step %u.\n", i/8);
        }
        break;
    case 'w':
        t_bit = 0;
        ret = sscanf(buf, "w %d", &t_bit);
        if (t_bit == 8)
        {
            for (i=0; i<64; i++) {
                for (j=0; j<4; j++){
                    pDev->conf.lcd_effect.GammaTableR[i*4+j] = (unsigned short)(((gamma_adjust_r_temp[i] >> (24-j*8)) & 0xff) << 2);
                    pDev->conf.lcd_effect.GammaTableG[i*4+j] = (unsigned short)(((gamma_adjust_g_temp[i] >> (24-j*8)) & 0xff) << 2);
                    pDev->conf.lcd_effect.GammaTableB[i*4+j] = (unsigned short)(((gamma_adjust_b_temp[i] >> (24-j*8)) & 0xff) << 2);					
                }
            }
            write_gamma_table();
            printk("8bit finished.\n");
        }
        else if (t_bit == 10)
        {
            for (i=0; i<128; i++) {
                for (j=0; j<2; j++){
                    pDev->conf.lcd_effect.GammaTableR[i*2+j] = (unsigned short)((gamma_adjust_r_temp[i] >> (16-j*16)) & 0xffff);
                    pDev->conf.lcd_effect.GammaTableG[i*2+j] = (unsigned short)((gamma_adjust_g_temp[i] >> (16-j*16)) & 0xffff);
                    pDev->conf.lcd_effect.GammaTableB[i*2+j] = (unsigned short)((gamma_adjust_b_temp[i] >> (16-j*16)) & 0xffff);
                }
            }
            write_gamma_table();
            printk("10bit finished.\n");
        }
        else
        {
            for (i=0; i<256; i++) {
                pDev->conf.lcd_effect.GammaTableR[i] = gamma_adjust_r[i];
                pDev->conf.lcd_effect.GammaTableG[i] = gamma_adjust_g[i];
                pDev->conf.lcd_effect.GammaTableB[i] = gamma_adjust_b[i];
            }
            write_gamma_table();
            printk("to original.\n");
        }
        break;
    case 'f':
        t_bit=255;
        if (buf[1] == 'r')
        {
            ret = sscanf(buf, "fr %u", &t_bit);
            t_bit &= 0xff;
            for (i=0; i<256; i++) {
                pDev->conf.lcd_effect.GammaTableR[i] = t_bit<<2;
            }
            write_gamma_table();
            printk("with R fixed value %u finished.\n", t_bit);
        }
        else if (buf[1] == 'g')
        {
            ret = sscanf(buf, "fg %u", &t_bit);
            t_bit &= 0xff; 
            for (i=0; i<256; i++) {
                pDev->conf.lcd_effect.GammaTableG[i] = t_bit<<2;
            }
            write_gamma_table();
            printk("with G fixed value %u finished.\n", t_bit);
        }
        else if (buf[1] == 'b')
        {
            ret = sscanf(buf, "fb %u", &t_bit);
            t_bit &= 0xff;
            for (i=0; i<256; i++) {
                pDev->conf.lcd_effect.GammaTableB[i] = t_bit<<2;
            }
            write_gamma_table();
            printk("with B fixed value %u finished.\n", t_bit);
        }
        else
        {
            ret = sscanf(buf, "fw %u", &t_bit);
            t_bit &= 0xff;
            for (i=0; i<256; i++) {
                pDev->conf.lcd_effect.GammaTableR[i] = t_bit<<2;
                pDev->conf.lcd_effect.GammaTableG[i] = t_bit<<2;
                pDev->conf.lcd_effect.GammaTableB[i] = t_bit<<2;
            }
            write_gamma_table();
            printk("with fixed value %u finished.\n", t_bit);
        }
        break;
        default:
            printk("wrong format of gamma table writing.\n");
    }

	if (ret != 1 || ret !=2)
		return -EINVAL;

	return count;
	//return 0;
}

static struct class_attribute aml_lcd_class_attrs[] = {
	__ATTR(write,  S_IRUGO | S_IWUSR, gamma_help, aml_lcd_gamma_debug),
	__ATTR(read,  S_IRUGO | S_IWUSR, gamma_help, aml_lcd_gamma_read),
	__ATTR(help,  S_IRUGO | S_IWUSR, gamma_help, NULL),
    __ATTR_NULL
};

static struct class aml_gamma_class = {
    .name = "gamma",
    .class_attrs = aml_lcd_class_attrs,
};
#endif
//****************************

//****************************
//LCD debug
//****************************
static Lcd_Config_t lcd_config_temp;
static int lvds_repack_temp, pn_swap_temp;

static Lvds_Phy_Control_t lcd_lvds_phy_control = 
{
    .lvds_prem_ctl = 0x0,		
    .lvds_swing_ctl = 0x4,	    
    .lvds_vcm_ctl = 0x7,
    .lvds_ref_ctl = 0x15, 
};

static Lvds_Config_t lcd_lvds_config=
{
    .lvds_repack=0,   //data mapping  //0:JEIDA mode, 1:VESA mode
	.pn_swap=0,		  //0:normal, 1:swap
};

static const char * lcd_usage_str =
{"Usage:\n"
"    echo basic <h_active> <v_active> <h_period> <v_period> <lcd_type> <lcd_bits> > debug ; write lcd basic config\n"
"    echo timing <pll_ctrl> <div_ctrl> <clk_ctrl> <hs_rising> <hs_failing> <vs_rising> <vs_failing> > debug ; write lcd timing\n"
"data format:\n"
"    <lcd_type>	: 1 for TTL, 2 for LVDS\n"
"    <pll_ctrl>, <div_ctrl>, <clk_ctrl>	: lcd clock parameters in Hex\n"
"    all the other data above are decimal numbers\n"
"\n"
"    echo ttl <clk_pol> <rb_swap> <bit_swap> > debug ; write ttl config\n"
"    echo lvds <lvds_repack> <pn_swap> > debug ; write lvds config\n"
"data format:\n"
"    <clk_pol>  : 0 for failing edge, 1 for rising edge\n"
"    <rb_swap>  : 0 for normal, 1 for swap r/b\n"
"    <bit_swap> : 0 for normal, 1 for swap msb/lsb\n"
"    <lvds_repack>  : 0 for JEIDA mode, 1 for VESA mode\n"
"    <pn_swap>  	: 0 for normal, 1 for swap lvds p/n channels\n"
"\n"
"    echo write > debug ; update lcd display config\n"
"    echo reset > debug ; reset lcd config\n"
"    echo read > debug ; read current lcd config\n"
"\n"
"    echo disable > debug ; power off lcd \n"
"    echo enable > debug ; power on lcd \n"
};

static ssize_t lcd_debug_help(struct class *class, struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n",lcd_usage_str);
}

static void read_current_lcd_config(Lcd_Config_t *pConf)
{	
	int sync_duration;
	
	printk("Current lcd config:\n");
	printk("h_active	%d,\nv_active	%d,\nh_period	%d,\nv_period	%d,\nlcd_bits	%d,\n", pConf->lcd_basic.h_active, pConf->lcd_basic.v_active, pConf->lcd_basic.h_period, pConf->lcd_basic.v_period, pConf->lcd_basic.lcd_bits);
	printk("pll_ctrl	0x%x,\ndiv_ctrl	0x%x,\nclk_ctrl	0x%x,\nhs_rising	%d,\nhs_failing	%d,\nvs_rising	%d,\nvs_failing	%d,\n", pConf->lcd_timing.pll_ctrl, pConf->lcd_timing.div_ctrl, pConf->lcd_timing.clk_ctrl, pConf->lcd_timing.sth1_hs_addr, pConf->lcd_timing.sth1_he_addr, pConf->lcd_timing.stv1_vs_addr, pConf->lcd_timing.stv1_ve_addr);
	if (pConf->lcd_basic.lcd_type == LCD_DIGITAL_TTL)
		printk("clk_pol	%d,\nrb_swap	%d,\nbit_swap	%d,\n", (pConf->lcd_timing.pol_cntl_addr >> LCD_CPH1_POL) & 0x1, (pConf->lcd_timing.dual_port_cntl_addr >> LCD_RGB_SWP) & 0x1, (pConf->lcd_timing.dual_port_cntl_addr >> LCD_BIT_SWP) & 0x1);
	else
		printk("lvds_repack	%d,\npn_swap	%d,\n", pConf->lvds_mlvds_config.lvds_config->lvds_repack, pConf->lvds_mlvds_config.lvds_config->pn_swap);
	
	printk("\nLCD type: %s.\n", ((pConf->lcd_basic.lcd_type == LCD_DIGITAL_TTL) ? "TTL" : "LVDS"));
	sync_duration = pConf->lcd_timing.sync_duration_num / 10;
	printk("Frame rate: %d.%dHz.\n\n", sync_duration, pConf->lcd_timing.sync_duration_num - sync_duration * 10);
}

static pinmux_item_t lcd_ttl_pinmux[] = {    
    {
        .reg = PINMUX_REG(0),	
        .setmask = (1 << 0) | (1 << 2) | (1 << 4),	//6bit RGB	
		.clrmask = (1<<22) | (1<<23) | (1<<24) | (1<<27),	//clera tcon pinmux
    },
	{
        .reg = PINMUX_REG(1),
        .setmask = (1<<14) | (1<<17) | (1<<18) | (1<<19),	//cph1, oeh, stv1, sth1 
    },
	{
        .reg = PINMUX_REG(5),	
        .clrmask = (0x3 << 15) | (0x1f << 19),	//clera 6bit RGB pinmux
    },
    PINMUX_END_ITEM
};

static pinmux_set_t lcd_ttl_pinmux_set = {
    .chip_select = NULL,
    .pinmux = &lcd_ttl_pinmux[0],
};

static void enalbe_lcd_ports(Lcd_Config_t *pConf)
{
	int lcd_type, lcd_bits;
	
	lcd_bits = pConf->lcd_basic.lcd_bits;
	lcd_type = pConf->lcd_basic.lcd_type;
	switch(lcd_type){
        case LCD_DIGITAL_TTL:
			if (lcd_bits == 8)
			{	
				lcd_ttl_pinmux[0].setmask = (3 << 0) | (3 << 2) | (3 << 4);	//8bit RGB
				lcd_ttl_pinmux[2].clrmask = (0x1ff << 15);	//clera 8bit RGB pinmux
			}
            pinmux_set(&lcd_ttl_pinmux_set);
            break;
        case LCD_DIGITAL_LVDS:
        	aml_set_reg32_bits(P_LVDS_PHY_CNTL3, 1, 0, 1);
			aml_set_reg32_bits(P_LVDS_GEN_CNTL, 1, 3, 1);
			if (lcd_bits == 6)
				aml_set_reg32_bits(P_LVDS_PHY_CNTL4, 0x27, 0, 7);
			else
				aml_set_reg32_bits(P_LVDS_PHY_CNTL4, 0x2f, 0, 7);		
            break;
        case LCD_DIGITAL_MINILVDS:
			//to do
            break;
        default:
            printk("Invalid LCD type.\n");
			break;
    }	
}

static void scale_framebuffer(void)
{		
	if ((pDev->conf.lcd_basic.h_active != lcd_config_temp.lcd_basic.h_active) || (pDev->conf.lcd_basic.v_active != lcd_config_temp.lcd_basic.v_active)) 
	{
		printk("\nPlease input below commands:\n");
		printk("echo 0 0 %d %d > /sys/class/video/axis\n", pDev->conf.lcd_basic.h_active, pDev->conf.lcd_basic.v_active);
		printk("echo %d > /sys/class/graphics/fb0/scale_width\n", lcd_config_temp.lcd_basic.h_active);
		printk("echo %d > /sys/class/graphics/fb0/scale_height\n", lcd_config_temp.lcd_basic.v_active);
		printk("echo 1 > /sys/class/graphics/fb0/free_scale\n\n");
	}
}

static void lcd_tcon_config(Lcd_Config_t *pConf)
{
	if (pConf->lcd_basic.lcd_type == LCD_DIGITAL_TTL)
	{
		pConf->lcd_timing.sth1_vs_addr = 0;
		pConf->lcd_timing.sth1_ve_addr = pConf->lcd_basic.v_period - 1;
		pConf->lcd_timing.stv1_hs_addr = 0;
		pConf->lcd_timing.stv1_he_addr = pConf->lcd_basic.h_period - 1;
	}
	else if (pConf->lcd_basic.lcd_type == LCD_DIGITAL_LVDS)
	{
		pConf->lcd_timing.sth1_vs_addr = 0;
		pConf->lcd_timing.sth1_ve_addr = pConf->lcd_basic.v_period - 1;
		pConf->lcd_timing.stv1_hs_addr = 10;
		pConf->lcd_timing.stv1_he_addr = 20;
	}	
}

static void lcd_sync_duration(Lcd_Config_t *pConf)
{
	unsigned m, n, od, div, xd, pre_div;
	unsigned h_period, v_period, sync_duration;	

	m = ((pConf->lcd_timing.pll_ctrl) >> 0) & 0x1ff;
	n = ((pConf->lcd_timing.pll_ctrl) >> 9) & 0x1f;
	od = ((pConf->lcd_timing.pll_ctrl) >> 16) & 0x3;
	div = ((pConf->lcd_timing.div_ctrl) >> 4) & 0x7;
	h_period = pConf->lcd_basic.h_period;
	v_period = pConf->lcd_basic.v_period;
	
	od = (od == 0) ? 1:((od == 1) ? 2:4);
	switch(pConf->lcd_basic.lcd_type)
	{
		case LCD_DIGITAL_TTL:
			xd = ((pConf->lcd_timing.clk_ctrl) >> 0) & 0xf;
			pre_div = 1;
			break;
		case LCD_DIGITAL_LVDS:
			xd = 1;
			pre_div = 7;
			break;
		case LCD_DIGITAL_MINILVDS:
			xd = 1;
			pre_div = 6;
			break;	
		default:
			pre_div = 1;
			break;
	}
	
	sync_duration = m*24*1000/(n*od*(div+1)*xd*pre_div);		
	sync_duration = ((sync_duration * 10000 / h_period) * 10) / v_period;
	sync_duration = (sync_duration + 5) / 10;	
	
	pConf->lcd_timing.sync_duration_num = sync_duration;
	pConf->lcd_timing.sync_duration_den = 10;
}

static void save_lcd_config(Lcd_Config_t *pConf)
{
	lcd_config_temp.lcd_basic.h_active = pConf->lcd_basic.h_active;
	lcd_config_temp.lcd_basic.v_active = pConf->lcd_basic.v_active;
	lcd_config_temp.lcd_basic.h_period = pConf->lcd_basic.h_period;
	lcd_config_temp.lcd_basic.v_period = pConf->lcd_basic.v_period;
	lcd_config_temp.lcd_basic.lcd_type = pConf->lcd_basic.lcd_type;
	lcd_config_temp.lcd_basic.lcd_bits = pConf->lcd_basic.lcd_bits;
			
	lcd_config_temp.lcd_timing.pll_ctrl = pConf->lcd_timing.pll_ctrl;
	lcd_config_temp.lcd_timing.div_ctrl = pConf->lcd_timing.div_ctrl;
	lcd_config_temp.lcd_timing.clk_ctrl = pConf->lcd_timing.clk_ctrl;
	lcd_config_temp.lcd_timing.sth1_hs_addr = pConf->lcd_timing.sth1_hs_addr;
	lcd_config_temp.lcd_timing.sth1_he_addr = pConf->lcd_timing.sth1_he_addr;
	lcd_config_temp.lcd_timing.sth1_vs_addr = pConf->lcd_timing.sth1_vs_addr;
	lcd_config_temp.lcd_timing.sth1_ve_addr = pConf->lcd_timing.sth1_ve_addr;
	lcd_config_temp.lcd_timing.stv1_hs_addr = pConf->lcd_timing.stv1_hs_addr;
	lcd_config_temp.lcd_timing.stv1_he_addr = pConf->lcd_timing.stv1_he_addr;
	lcd_config_temp.lcd_timing.stv1_vs_addr = pConf->lcd_timing.stv1_vs_addr;
	lcd_config_temp.lcd_timing.stv1_ve_addr = pConf->lcd_timing.stv1_ve_addr;
	lcd_config_temp.lcd_timing.oeh_hs_addr = pConf->lcd_timing.oeh_hs_addr;
	lcd_config_temp.lcd_timing.oeh_he_addr = pConf->lcd_timing.oeh_he_addr;
	lcd_config_temp.lcd_timing.oeh_vs_addr = pConf->lcd_timing.oeh_vs_addr;
	lcd_config_temp.lcd_timing.oeh_ve_addr = pConf->lcd_timing.oeh_ve_addr;

	lcd_config_temp.lcd_timing.pol_cntl_addr = pConf->lcd_timing.pol_cntl_addr;
	lcd_config_temp.lcd_timing.dual_port_cntl_addr = pConf->lcd_timing.dual_port_cntl_addr;
	
	if (pConf->lcd_basic.lcd_type == LCD_DIGITAL_LVDS)
	{
		lvds_repack_temp = pConf->lvds_mlvds_config.lvds_config->lvds_repack;
		pn_swap_temp = pConf->lvds_mlvds_config.lvds_config->pn_swap;
	}
}

static void reset_lcd_config(Lcd_Config_t *pConf)
{
	int res = 0;
	
	printk("reset lcd config.\n");
	if ((pConf->lcd_basic.h_active != lcd_config_temp.lcd_basic.h_active) || (pConf->lcd_basic.v_active != lcd_config_temp.lcd_basic.v_active))
		res = 1;
	
	pConf->lcd_basic.h_active = lcd_config_temp.lcd_basic.h_active;
	pConf->lcd_basic.v_active = lcd_config_temp.lcd_basic.v_active;
	pConf->lcd_basic.h_period = lcd_config_temp.lcd_basic.h_period;
	pConf->lcd_basic.v_period = lcd_config_temp.lcd_basic.v_period;
	pConf->lcd_basic.lcd_type = lcd_config_temp.lcd_basic.lcd_type;
	pConf->lcd_basic.lcd_bits = lcd_config_temp.lcd_basic.lcd_bits;
			
	pConf->lcd_timing.pll_ctrl = lcd_config_temp.lcd_timing.pll_ctrl;
	pConf->lcd_timing.div_ctrl = lcd_config_temp.lcd_timing.div_ctrl;
	pConf->lcd_timing.clk_ctrl = lcd_config_temp.lcd_timing.clk_ctrl;
	pConf->lcd_timing.sth1_hs_addr = lcd_config_temp.lcd_timing.sth1_hs_addr;
	pConf->lcd_timing.sth1_he_addr = lcd_config_temp.lcd_timing.sth1_he_addr;
	pConf->lcd_timing.sth1_vs_addr = lcd_config_temp.lcd_timing.sth1_vs_addr;
	pConf->lcd_timing.sth1_ve_addr = lcd_config_temp.lcd_timing.sth1_ve_addr;
	pConf->lcd_timing.stv1_hs_addr = lcd_config_temp.lcd_timing.stv1_hs_addr;
	pConf->lcd_timing.stv1_he_addr = lcd_config_temp.lcd_timing.stv1_he_addr;
	pConf->lcd_timing.stv1_vs_addr = lcd_config_temp.lcd_timing.stv1_vs_addr;
	pConf->lcd_timing.stv1_ve_addr = lcd_config_temp.lcd_timing.stv1_ve_addr;
	pConf->lcd_timing.oeh_hs_addr = lcd_config_temp.lcd_timing.oeh_hs_addr;
	pConf->lcd_timing.oeh_he_addr = lcd_config_temp.lcd_timing.oeh_he_addr;
	pConf->lcd_timing.oeh_vs_addr = lcd_config_temp.lcd_timing.oeh_vs_addr;
	pConf->lcd_timing.oeh_ve_addr = lcd_config_temp.lcd_timing.oeh_ve_addr;

	pConf->lcd_timing.pol_cntl_addr = lcd_config_temp.lcd_timing.pol_cntl_addr;
	pConf->lcd_timing.dual_port_cntl_addr = lcd_config_temp.lcd_timing.dual_port_cntl_addr;
	
	if (lcd_config_temp.lcd_basic.lcd_type == LCD_DIGITAL_LVDS)
	{
		lcd_lvds_config.lvds_repack = lvds_repack_temp;
		lcd_lvds_config.pn_swap = pn_swap_temp;
	}
	
	lcd_sync_duration(&pDev->conf);
	_init_display_driver(&pDev->conf);
	enalbe_lcd_ports(&pDev->conf);
	if (res)
	{
		printk("\nPlease input below commands:\n");		
		printk("echo 0 > /sys/class/graphics/fb0/free_scale\n\n");
	}
}

static ssize_t lcd_debug(struct class *class, struct class_attribute *attr, const char *buf, size_t count)
{
	unsigned int ret;
	unsigned t[8];
	
	switch (buf[0])
	{
		case 'r':	
			if (buf[2] == 'a')	//read lcd config
			{
				read_current_lcd_config(&pDev->conf);
			}
			else if (buf[2] == 's')	//reset lcd config
			{			
				reset_lcd_config(&pDev->conf);				
			}
			break;
		case 'b':	//write basic config
			t[0] = 1024;
			t[1] = 768;
			t[2] = 1344;
			t[3] = 806;
			t[4] = 2;
			t[5] = 8;
			ret = sscanf(buf, "basic %d %d %d %d %d %d", &t[0], &t[1], &t[2], &t[3], &t[4], &t[5]);
			pDev->conf.lcd_basic.h_active = t[0];
			pDev->conf.lcd_basic.v_active = t[1];
			pDev->conf.lcd_basic.h_period = t[2];
			pDev->conf.lcd_basic.v_period = t[3];
			pDev->conf.lcd_basic.lcd_type = t[4];
			pDev->conf.lcd_basic.lcd_bits = t[5];
			printk("write lcd basic config:\n");
			printk("h_active=%d, v_active=%d, h_period=%d, v_period=%d, lcd_type: %s, lcd_bits: %d\n", t[0], t[1], t[2], t[3], ((t[4] == LCD_DIGITAL_TTL) ? "TTL" : ((t[4] == LCD_DIGITAL_LVDS) ? "LVDS" : "miniLVDS")), t[5]);
			break;
		case 't':	
			if (buf[1] == 'i') //write display timing
			{
				t[0] = 0x10220;
				t[1] = 0x18803;
				t[2] = 0x1111;
				t[3] = 10;
				t[4] = 20;
				t[5] = 2;
				t[6] = 4;
				ret = sscanf(buf, "timing %x %x %x %d %d %d %d", &t[0], &t[1], &t[2], &t[3], &t[4], &t[5], &t[6]);
				pDev->conf.lcd_timing.pll_ctrl = t[0];
				pDev->conf.lcd_timing.div_ctrl = t[1];
				pDev->conf.lcd_timing.clk_ctrl = t[2];
				pDev->conf.lcd_timing.sth1_hs_addr = t[3];
				pDev->conf.lcd_timing.sth1_he_addr = t[4];
				pDev->conf.lcd_timing.stv1_vs_addr = t[5];
				pDev->conf.lcd_timing.stv1_ve_addr = t[6];
				pDev->conf.lcd_timing.oeh_hs_addr = pDev->conf.lcd_timing.video_on_pixel+ 19;
				pDev->conf.lcd_timing.oeh_he_addr = pDev->conf.lcd_timing.video_on_pixel + 19 + pDev->conf.lcd_basic.h_active;
				pDev->conf.lcd_timing.oeh_vs_addr = pDev->conf.lcd_timing.video_on_line;
				pDev->conf.lcd_timing.oeh_ve_addr = pDev->conf.lcd_timing.video_on_line + pDev->conf.lcd_basic.v_active - 1;
				printk("write lcd timing config:\n");
				printk("pll_ctrl=0x%x, div_ctrl=0x%x, clk_ctrl=0x%x, hs_rising=%d, hs_falling=%d, vs_rising=%d, vs_falling=%d\n", t[0], t[1], t[2], t[3], t[4], t[5], t[6]);
			}
			else if (buf[1] == 't')	//write ttl config	//clk_pol, rb_swap, bit_swap
			{
				t[0] = 1;
				t[1] = 0;
				t[2] = 0;
				ret = sscanf(buf, "ttl %d %d %d", &t[0], &t[1], &t[2]);
				pDev->conf.lcd_timing.pol_cntl_addr = (t[0] << LCD_CPH1_POL) |(0x1 << LCD_HS_POL) | (0x1 << LCD_VS_POL);
				pDev->conf.lcd_timing.dual_port_cntl_addr = (1<<LCD_TTL_SEL) | (1<<LCD_ANALOG_SEL_CPH3) | (1<<LCD_ANALOG_3PHI_CLK_SEL) | (t[1]<<LCD_RGB_SWP) | (t[2]<<LCD_BIT_SWP);
				printk("write ttl config:\n");
				printk("clk_pol: %s, rb_swap: %s, bit_swap: %s\n", ((t[0] == 1) ? "positive" : "negative"), ((t[1] == 1) ? "enable" : "disable"), ((t[2] == 1) ? "enable" : "disable"));
			}
			break;		
		case 'l':	//write lvds config		//lvds_repack, pn_swap
			t[0] = 1;
			t[1] = 0;
			ret = sscanf(buf, "lvds %d %d", &t[0], &t[1]);
			lcd_lvds_config.lvds_repack = t[0];
			lcd_lvds_config.pn_swap = t[1];
			pDev->conf.lvds_mlvds_config.lvds_config = &lcd_lvds_config;
			pDev->conf.lvds_mlvds_config.lvds_phy_control = &lcd_lvds_phy_control;
			printk("write lvds config:\n");
			printk("lvds_repack: %s, rb_swap: %s\n", ((t[0] == 1) ? "VESA mode" : "JEIDA mode"), ((t[1] == 1) ? "enable" : "disable"));
			break;
		case 'm':	//write mlvds config
			//to do
			break;
		case 'w':	//update display config
			if (pDev->conf.lcd_basic.lcd_type == LCD_DIGITAL_MINILVDS)
			{
				printk("Don't support miniLVDS yet. Will reset to original lcd config.\n");
				reset_lcd_config(&pDev->conf);
			}
			else
			{
				lcd_tcon_config(&pDev->conf);
				lcd_sync_duration(&pDev->conf);
				_init_display_driver(&pDev->conf);
				enalbe_lcd_ports(&pDev->conf);
				scale_framebuffer();
			}
			break;
		case 'd':
			printk("power off lcd.\n");
			_disable_backlight();
			pDev->conf.lcd_power_ctrl.power_ctrl?pDev->conf.lcd_power_ctrl.power_ctrl(OFF):0;
			//_disable_display_driver(&pDev->conf);
			break;
		case 'e':
			printk("power on lcd.\n");
			//_lcd_module_enable();			
			if (pDev->conf.lcd_basic.lcd_type != LCD_DIGITAL_TTL)
			{	
				init_lvds_phy(&pDev->conf);	
			}
			pDev->conf.lcd_power_ctrl.power_ctrl?pDev->conf.lcd_power_ctrl.power_ctrl(ON):0;							
			_enable_backlight(BL_MAX_LEVEL);
			break;
		default:
			printk("wrong format of lcd debug command.\n");			
	}	
	
	if (ret != 1 || ret !=2)
		return -EINVAL;
	
	return count;
	//return 0;
}

static struct class_attribute lcd_debug_class_attrs[] = {   
	__ATTR(debug,  S_IRUGO | S_IWUSR, lcd_debug_help, lcd_debug),	
	__ATTR(help,  S_IRUGO | S_IWUSR, lcd_debug_help, NULL),
    __ATTR_NULL
};

static struct class aml_lcd_debug_class = {
    .name = "lcd",
    .class_attrs = lcd_debug_class_attrs,
};
//****************************

static struct notifier_block lcd_reboot_nb;
static int lcd_probe(struct platform_device *pdev)
{
    struct aml_lcd_platform *pdata;    

    pdata = pdev->dev.platform_data;
    pDev = (lcd_dev_t *)kmalloc(sizeof(lcd_dev_t), GFP_KERNEL);    

    if (!pDev) {
        PRINT_INFO("[tcon]: Not enough memory.\n");
        return -ENOMEM;
    }    

//    extern Lcd_Config_t m6skt_lcd_config;
//    pDev->conf = m6skt_lcd_config;
    pDev->conf = *(Lcd_Config_t *)(pdata->lcd_conf);        //*(Lcd_Config_t *)(s->start);    

    printk("LCD probe ok\n");    

    _lcd_init(&pDev->conf);    
	
	int err;
	lcd_reboot_nb.notifier_call = lcd_reboot_notifier;
    err = register_reboot_notifier(&lcd_reboot_nb);
	if (err)
	{
		printk("notifier register lcd_reboot_notifier fail!\n");
	}
	
	int ret;
	
	save_lcd_config(&pDev->conf);
	ret = class_register(&aml_lcd_debug_class);
	if(ret){
		printk("class register aml_lcd_debug_class fail!\n");
	}
#ifdef CONFIG_AML_GAMMA_DEBUG	
	int i;
	for (i=0; i<256; i++) {
        gamma_adjust_r[i] = pDev->conf.lcd_effect.GammaTableR[i];
        gamma_adjust_g[i] = pDev->conf.lcd_effect.GammaTableG[i];
		gamma_adjust_b[i] = pDev->conf.lcd_effect.GammaTableB[i];
    }
	
	ret = class_register(&aml_gamma_class);
	if(ret){
		printk("class register aml_gamma_class fail!\n");
	}
#endif

    return 0;
}

static int lcd_remove(struct platform_device *pdev)
{
    unregister_reboot_notifier(&lcd_reboot_nb);
    kfree(pDev);
    
    return 0;
}

static struct platform_driver lcd_driver = {
    .probe      = lcd_probe,
    .remove     = lcd_remove,
    .driver     = {
        .name   = "mesonlcd",    // removed "tcon-dev"
    }
};

static int __init lcd_init(void)
{
    printk("LCD driver init\n");
    if (platform_driver_register(&lcd_driver)) {
        PRINT_INFO("failed to register tcon driver module\n");
        return -ENODEV;
    }

    return 0;
}

static void __exit lcd_exit(void)
{
    platform_driver_unregister(&lcd_driver);
}

subsys_initcall(lcd_init);
module_exit(lcd_exit);

MODULE_DESCRIPTION("Meson LCD Panel Driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Amlogic, Inc.");
