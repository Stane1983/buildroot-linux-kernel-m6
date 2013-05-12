#ifndef __HDMI_TX_HDCP_H
#define __HDMI_TX_HDCP_H
/*
    hdmi_tx_hdcp.c
    version 1.0
*/

// Notic: the HDCP key setting has been moved to uboot
// On MBX project, it is too late for HDCP get from 
// other devices

//int task_tx_key_setting(unsigned force_wrong);

// buf: store buffer
// endian: 0: little endian  1: big endian
void hdmi_hdcp_get_aksv(char* buf, int endian);

void hdmi_hdcp_get_bksv(char* buf, int endian);

int hdcp_ksv_valid(unsigned char * dat);

#endif

