/*
 * Amlogic M1 
 * frame buffer driver-----------HDMI_TX
 * Copyright (C) 2010 Amlogic, Inc.
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
 */
#ifndef AVOS
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
#include <asm/uaccess.h>
#include <mach/am_regs.h>
#else
#include "ioapi.h"
#include <chipsupport/chipsupport.h>
#include <os/extend/interrupt.h>
#include <Drivers/include/peripheral_reg.h>
#include <Drivers/include/isa_reg.h>
#include <Drivers/include/mpeg_reg.h>
#include <interrupt.h>
#include "displaydev.h"
#include "policy.h"
#endif

#include "hdmi_tx_reg.h"

#ifdef AVOS
void WRITE_APB_REG(unsigned long addr, unsigned long data)
{
  *((volatile unsigned long *) (APB_BASE+addr)) = data; 
}

unsigned long READ_APB_REG(unsigned long addr)
{
    unsigned long data;
    data = *((volatile unsigned long *)(APB_BASE+addr)); 
    return data;
}        

#endif

// if the following bits are 0, then access HDMI IP Port will cause system hungup
#define GATE_NUM    2
Hdmi_Gate_s hdmi_gate[GATE_NUM] =   {   {HHI_HDMI_CLK_CNTL, 8},
                                        {HHI_GCLK_MPEG2   , 4},
                                    };

// In order to prevent system hangup, add check_cts_hdmi_sys_clk_status() to check 
static void check_cts_hdmi_sys_clk_status(void)
{
    int i;

    for(i = 0; i < GATE_NUM; i++){
        if(!(aml_read_reg32(CBUS_REG_ADDR(hdmi_gate[i].cbus_addr)) & (1<<hdmi_gate[i].gate_bit))){
            printk("HDMI Gate Clock is off, turn on now\n");
            aml_set_reg32_bits(CBUS_REG_ADDR(hdmi_gate[i].cbus_addr), 1, hdmi_gate[i].gate_bit, 1);
        }
    }
}

unsigned long hdmi_rd_reg(unsigned long addr)
{
    unsigned long data;
    check_cts_hdmi_sys_clk_status();
//    WRITE_APB_REG(HDMI_ADDR_PORT, addr);
//    WRITE_APB_REG(HDMI_ADDR_PORT, addr);
    aml_write_reg32(P_HDMI_ADDR_PORT, addr);
    aml_write_reg32(P_HDMI_ADDR_PORT, addr);
    
//    data = READ_APB_REG(HDMI_DATA_PORT);
    data = aml_read_reg32(P_HDMI_DATA_PORT);
    
    return (data);
}


void hdmi_wr_only_reg(unsigned long addr, unsigned long data)
{
    check_cts_hdmi_sys_clk_status();
//    WRITE_APB_REG(HDMI_ADDR_PORT, addr);
//    WRITE_APB_REG(HDMI_ADDR_PORT, addr);
//    
//    WRITE_APB_REG(HDMI_DATA_PORT, data);
    aml_write_reg32(P_HDMI_ADDR_PORT, addr);
    aml_write_reg32(P_HDMI_ADDR_PORT, addr);
    
    aml_write_reg32(P_HDMI_DATA_PORT, data);
}

void hdmi_wr_reg(unsigned long addr, unsigned long data)
{
    unsigned long rd_data;
    
    check_cts_hdmi_sys_clk_status();
//    WRITE_APB_REG(HDMI_ADDR_PORT, addr);
//    WRITE_APB_REG(HDMI_ADDR_PORT, addr);
//    
//    WRITE_APB_REG(HDMI_DATA_PORT, data);
    aml_write_reg32(P_HDMI_ADDR_PORT, addr);
    aml_write_reg32(P_HDMI_ADDR_PORT, addr);
    
    aml_write_reg32(P_HDMI_DATA_PORT, data);
    rd_data = hdmi_rd_reg (addr);
    if (rd_data != data) 
    {
//        //printk("hdmi_wr_reg(%x,%x) fails to write: %x\n",addr, data, rd_data);
//       //while(1){};      
    }
}


