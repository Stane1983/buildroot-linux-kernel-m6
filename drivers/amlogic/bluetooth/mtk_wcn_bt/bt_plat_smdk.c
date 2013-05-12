/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 * 
 * MediaTek Inc. (C) 2010. All rights reserved.
 * 
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
//#include <linux/gpio.h>
#include <plat/platform.h>
#include <plat/plat_dev.h>
#include <plat/platform_data.h>
#include <plat/lm.h>
#include <plat/regops.h>

#include <mach/gpio_data.h>
#include <mach/pinmux.h>
#include <mach/gpio.h>
#include <mach/reg_addr.h>

#include "bt_hwctl.h"

/****************************************************************************
 *                           C O N S T A N T S                              *
*****************************************************************************/
#define MODULE_TAG         "[BT-PLAT] "

static int irq_num = INT_GPIO_5;
// to avoid irq enable and disable not match
static unsigned int irq_mask;

/****************************************************************************
 *                       I R Q   F U N C T I O N S                          *
*****************************************************************************/
static int mt_bt_request_irq(void)
{
    //return 0;
    int iRet = 0;
    irq_mask = 0;
    #if 1
    iRet = request_irq(irq_num, mt_bt_eirq_handler, 
        IRQF_TRIGGER_HIGH, "BT_INT_B", NULL);
    if (iRet){
        printk(KERN_ALERT MODULE_TAG "request_irq IRQ%d fails, errno %d\n", irq_num, iRet);
    }
    else{
        printk(KERN_INFO MODULE_TAG "request_irq IRQ%d success\n", irq_num);
        mt_bt_disable_irq();
        /* enable irq when driver init complete, at hci_uart_open */
    }
    #endif
    return iRet;
}

static void mt_bt_free_irq(void)
{
    //return;
    free_irq(irq_num, NULL);
    irq_mask = 0;
}

void mt_bt_enable_irq(void)
{
    //return;
    printk(KERN_INFO MODULE_TAG "enable_irq, mask %d\n", irq_mask);
    if (irq_mask){
        enable_irq(irq_num);
        irq_mask = 0;
    }
}
EXPORT_SYMBOL(mt_bt_enable_irq);

void mt_bt_disable_irq(void)
{
    //return;
    printk(KERN_INFO MODULE_TAG "disable_irq_nosync, mask %d\n", irq_mask);
    if (!irq_mask){
        disable_irq_nosync(irq_num);
        irq_mask = 1;
    }
}
EXPORT_SYMBOL(mt_bt_disable_irq);

/****************************************************************************
 *                      P O W E R   C O N T R O L                           *
*****************************************************************************/

extern void extern_bt_set_enable(int is_on);
int mt_bt_power_on(void)
{
    int error;
    printk(KERN_INFO MODULE_TAG "mt_bt_power_on ++\n");
    
/************************************************************
 *  Make sure BT_PWR_EN is default gpio output low when
 *  system boot up, otherwise MT6622 gets actived unexpectedly 
 ************************************************************/

    extern_bt_set_enable(1);
    
    msleep(15);
    /* RESET pin pull up */
    //gpio_out(PAD_GPIOX_16, 1);
    msleep(1000);
    error = mt_bt_request_irq();
    if (error){
        return error;
    }
    printk(KERN_INFO MODULE_TAG "mt_bt_power_on --\n");
    
    return 0;
}

EXPORT_SYMBOL(mt_bt_power_on);


void mt_bt_power_off(void)
{
    
    printk(KERN_INFO MODULE_TAG "mt_bt_power_off ++\n");

    extern_bt_set_enable(0);
    mt_bt_free_irq();

    printk(KERN_INFO MODULE_TAG "mt_bt_power_off --\n");
}

EXPORT_SYMBOL(mt_bt_power_off);