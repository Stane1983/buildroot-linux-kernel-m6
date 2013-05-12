#ifndef _HDMI_CONFIG_H_
#define _HDMI_CONFIG_H_

struct hdmi_phy_set_data{
    unsigned long freq;
    unsigned long addr;
    unsigned long data;
};

struct hdmi_config_platform_data{
    void (*hdmi_5v_ctrl)(unsigned int pwr);
    void (*hdmi_3v3_ctrl)(unsigned int pwr);
    void (*hdmi_pll_vdd_ctrl)(unsigned int pwr);
    void (*hdmi_sspll_ctrl)(unsigned int level);    // SSPLL control level
    struct hdmi_phy_set_data *phy_data;             // For some boards, HDMI PHY setting may diff from ref board.
};

#endif

