#ifndef __USB_CLK_HEADER_
#define __USB_CLK_HEADER_

typedef union usb_peri_reg{
    /** raw register data */
    uint32_t d32;
    /** register bits */
    struct {
        unsigned a_por:1;
        unsigned b_por:1;
        unsigned unused:3;
        unsigned clk_sel:3;
        unsigned clk_en:1;
        unsigned ss_scale_down:2;
        unsigned b_reset:1;
        unsigned b_prst_n:1;
        unsigned b_pll_dect_rst:1;
        unsigned unused1:1;
        unsigned ss_scale_down_mode:2;
        unsigned a_reset:1;
        unsigned a_prst_n:1;
        unsigned a_pll_dect_rst:1;
        unsigned a_drvvbus:1;
        unsigned b_drvvbus:1;
        unsigned b_clk_dtd:1;
        unsigned unused2:1;
        unsigned clk_div:7;
        unsigned a_clk_dtd:1;
    } b;
} usb_peri_reg_t;

typedef union usb_peri_misc_reg{
    /** raw register data */
    uint32_t d32;
    /** register bits */
    struct {
        unsigned chrgsel:1;
        unsigned vdatdetenb:1;
        unsigned vdatsrcenb:1;
        unsigned uartdata:1;
        unsigned uartenb:1;
        unsigned sleepm:1;
        unsigned txhsvtune:2;
        unsigned vregtune:1;
        unsigned unsued1:1;
        unsigned thread_id:6;
        unsigned burst_num:6;
        unsigned utmi0_iddig_val:1;
        unsigned utmi0_iddig_overrid:1;
        unsigned unused2:4;
        unsigned utmi_vbusvalid_a:1;	/* Only in USB_REG3 */
        unsigned utmi_vbusvalid_b:1;	/* Only in USB_REG3 */
        unsigned utmi_iddig_a:1;		/* Only in USB_REG3 */
        unsigned utmi_iddig_b:1;		/* Only in USB_REG3 */
    } b;
} usb_peri_misc_reg_t;
/*
 * Clock source index must sync with chip's spec
 * M1/M2/M3/M6 are different!
 */ 
#define USB_PHY_CLK_SEL_XTAL	0
#define USB_PHY_CLK_SEL_XTAL_DIV_2	1
#define USB_PHY_CLK_SEL_SYS_PLL	2
#define USB_PHY_CLK_SEL_MISC_PLL	3
#define USB_PHY_CLK_SEL_DDR_PLL	4
#define USB_PHY_CLK_SEL_AUD_PLL	5
#define USB_PHY_CLK_SEL_VID_PLL	6
#define USB_PHY_CLK_SEL_VID_PLL_DIV_2	7

extern int set_usb_phy_clk(struct lm_device * plmdev,int is_enable);
extern int set_usb_phy_id_mode(struct lm_device * plmdev,unsigned int phy_id_mode);
#endif
