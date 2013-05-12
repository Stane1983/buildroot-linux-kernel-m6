#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/mm.h>
#include <linux/major.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/cdev.h>
//#include <linux/amports/canvas.h>
#include <asm/uaccess.h>
#include <mach/am_regs.h>

#include "hdmi_info_global.h"
#include "hdmi_tx_module.h"
#include "m1/hdmi_tx_reg.h"
#include "hdmi_tx_hdcp.h"
/*
    hdmi_tx_hdcp.c
    version 1.0
*/

// Notic: the HDCP key setting has been moved to uboot
// On MBX project, it is too late for HDCP get from 
// other devices

// buf: store buffer
// endian: 0: little endian  1: big endian
void hdmi_hdcp_get_aksv(char* buf, int endian)
{
    int i;
    if(endian ==1) {
        for(i = 0;i < 5; i++) {
            buf[i] = hdmi_rd_reg(TX_HDCP_SHW_AKSV_0 + i);
        }
    }
    else {
        for(i = 0;i < 5; i++) {
            buf[i] = hdmi_rd_reg(TX_HDCP_SHW_AKSV_0 + 4 - i);
        }
    }
}

void hdmi_hdcp_get_bksv(char* buf, int endian)
{
    int i;
    if(endian ==1){
        for(i = 0;i < 5; i++) {
            buf[i] = hdmi_rd_reg(TX_HDCP_SHW_BKSV_0 + i);
        }
    }
    else {
        for(i = 0;i < 5; i++) {
            buf[i] = hdmi_rd_reg(TX_HDCP_SHW_BKSV_0 + 4 - i);
        }
    }
}

/* verify ksv, 20 ones and 20 zeroes*/
int hdcp_ksv_valid(unsigned char * dat)
{
    int i, j, one_num = 0;
    for(i = 0; i < 5; i++){
        for(j=0;j<8;j++) {
            if((dat[i]>>j)&0x1) {
                one_num++;
            }
        }
    }
    if(one_num == 0)
        printk("HDMITX: no HDCP key available\n");
    return (one_num == 20);
}

