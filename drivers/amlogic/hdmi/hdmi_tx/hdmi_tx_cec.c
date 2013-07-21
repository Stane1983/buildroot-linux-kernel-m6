/*
 * Amlogic M1 
 * HDMI CEC Driver-----------HDMI_TX
 * Copyright (C) 2011 Amlogic, Inc.
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

#include <linux/version.h>
#include <linux/module.h>
#include <linux/irq.h>
#include <linux/types.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/mm.h>
#include <linux/major.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/cdev.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/spinlock_types.h>
#include <linux/switch.h>

#include <asm/uaccess.h>
#include <asm/delay.h>
#include <mach/am_regs.h>
#include <mach/power_gate.h>
#include <linux/tvin/tvin.h>

#include <mach/gpio.h>

#ifdef CONFIG_ARCH_MESON
#include "m1/hdmi_tx_reg.h"
#endif
#ifdef CONFIG_ARCH_MESON3
#include "m3/hdmi_tx_reg.h"
#endif
#ifdef CONFIG_ARCH_MESON6
#include "m6/hdmi_tx_reg.h"
#endif
#include "hdmi_tx_module.h"
#include "hdmi_tx_cec.h"


//static void remote_cec_tasklet(unsigned long);
//static int REMOTE_CEC_IRQ = INT_REMOTE;
//DECLARE_TASKLET_DISABLED(tasklet_cec, remote_cec_tasklet, 0);
struct input_dev *remote_cec_dev;
DEFINE_SPINLOCK(cec_input_key);
DEFINE_SPINLOCK(cec_rx_lock);
DEFINE_SPINLOCK(cec_tx_lock);
DEFINE_SPINLOCK(cec_init_lock); 
static DECLARE_WAIT_QUEUE_HEAD(cec_key_poll);

//#define _RX_DATA_BUF_SIZE_ 6

/* global variables */
static	unsigned char    gbl_msg[MAX_MSG];
unsigned char hdmi_cec_func_config;
cec_global_info_t cec_global_info;
int cec_power_flag = 0;
EXPORT_SYMBOL(cec_power_flag);


struct switch_dev lang_dev = {	// android ics switch device
	.name = "lang_config",
	};	
EXPORT_SYMBOL(lang_dev);

static struct semaphore  tv_cec_sema;

static DEFINE_SPINLOCK(p_tx_list_lock);
//static DEFINE_SPINLOCK(cec_tx_lock);

static unsigned long cec_tx_list_flags;
//static unsigned long cec_tx_flags;
static unsigned int tx_msg_cnt = 0;

static struct list_head cec_tx_msg_phead = LIST_HEAD_INIT(cec_tx_msg_phead);

static tv_cec_pending_e cec_pending_flag = TV_CEC_PENDING_OFF;
//static tv_cec_polling_state_e cec_polling_state = TV_CEC_POLLING_OFF;

unsigned int menu_lang_array[] = {(((unsigned int)'c')<<16)|(((unsigned int)'h')<<8)|((unsigned int)'i'),
                                  (((unsigned int)'e')<<16)|(((unsigned int)'n')<<8)|((unsigned int)'g'),
                                  (((unsigned int)'j')<<16)|(((unsigned int)'p')<<8)|((unsigned int)'n'),
                                  (((unsigned int)'k')<<16)|(((unsigned int)'o')<<8)|((unsigned int)'r'),
                                  (((unsigned int)'f')<<16)|(((unsigned int)'r')<<8)|((unsigned int)'a'),
                                  (((unsigned int)'g')<<16)|(((unsigned int)'e')<<8)|((unsigned int)'r')
                                 };

static unsigned char * default_osd_name[16] = {
    "tv",
    "recording 1",
    "recording 2",
    "tuner 1",
    "PHILIPS HMP1",
    "audio system",
    "tuner 2",
    "tuner 3",
    "PHILIPS HMP2",
    "recording 3",
    "tunre 4",
    "PHILIPS HMP3",
    "reserved 1",
    "reserved 2",
    "free use",
    "unregistered"
};

cec_rx_msg_buf_t cec_rx_msg_buf;

//static unsigned char * osd_name_uninit = "\0\0\0\0\0\0\0\0";
static irqreturn_t cec_isr_handler(int irq, void *dev_instance);


//static unsigned char dev = 0;
static unsigned char cec_init_flag = 0;
static unsigned char cec_mutex_flag = 0;


//static unsigned int hdmi_rd_reg(unsigned long addr);
//static void hdmi_wr_reg(unsigned long addr, unsigned long data);

void cec_test_function(unsigned char* arg, unsigned char arg_cnt)
{
//    int i;
//    char buf[512];
//
//    switch (arg[0]) {
//    case 0x0:
//        cec_usrcmd_parse_all_dev_online();
//        break;
//    case 0x2:
//        cec_usrcmd_get_audio_status(arg[1]);
//        break;
//    case 0x3:
//        cec_usrcmd_get_deck_status(arg[1]);
//        break;
//    case 0x4:
//        cec_usrcmd_get_device_power_status(arg[1]);
//        break;
//    case 0x5:
//        cec_usrcmd_get_device_vendor_id(arg[1]);
//        break;
//    case 0x6:
//        cec_usrcmd_get_osd_name(arg[1]);
//        break;
//    case 0x7:
//        cec_usrcmd_get_physical_address(arg[1]);
//        break;
//    case 0x8:
//        cec_usrcmd_get_system_audio_mode_status(arg[1]);
//        break;
//    case 0x9:
//        cec_usrcmd_get_tuner_device_status(arg[1]);
//        break;
//    case 0xa:
//        cec_usrcmd_set_deck_cnt_mode(arg[1], arg[2]);
//        break;
//    case 0xc:
//        cec_usrcmd_set_imageview_on(arg[1]);
//        break;
//    case 0xd:
//        cec_usrcmd_set_play_mode(arg[1], arg[2]);
//        break;
//    case 0xe:
//        cec_usrcmd_get_menu_state(arg[1]);
//        break;
//    case 0xf:
//        cec_usrcmd_set_menu_state(arg[1], arg[2]);
//        break;
//    case 0x10:
//        cec_usrcmd_get_global_info(buf);
//        break;
//    case 0x11:
//        cec_usrcmd_get_menu_language(arg[1]);
//        break;
//    case 0x12:
//        cec_usrcmd_set_menu_language(arg[1], arg[2]);
//        break;
//    case 0x13:
//        cec_usrcmd_get_active_source();
//        break;
//    case 0x14:
//        cec_usrcmd_set_active_source();
//        break;
//    case 0x15:
//        cec_usrcmd_set_deactive_source(arg[1]);
//        break;
//    case 0x17:
//        cec_usrcmd_set_report_physical_address(arg[1], arg[2], arg[3], arg[4]);
//        break;
//    case 0x18:
//    	{int i = 0;
//    	cec_polling_online_dev(arg[1], &i);
//    	}
//    	break;
//    default:
//        break;
//    }
}


/***************************** cec low level code *****************************/
/*
static unsigned int cec_get_ms_tick(void)
{
    unsigned int ret = 0;
    struct timeval cec_tick;
    do_gettimeofday(&cec_tick);
    ret = cec_tick.tv_sec * 1000 + cec_tick.tv_usec / 1000;

    return ret;
}
*/
/*
static unsigned int cec_get_ms_tick_interval(unsigned int last_tick)
{
    unsigned int ret = 0;
    unsigned int tick = 0;
    struct timeval cec_tick;
    do_gettimeofday(&cec_tick);
    tick = cec_tick.tv_sec * 1000 + cec_tick.tv_usec / 1000;

    if (last_tick < tick) ret = tick - last_tick;
    else ret = ((unsigned int)(-1) - last_tick) + tick;
    return ret;
}
*/

int cec_ll_tx_irq(unsigned char *msg, unsigned char len)
{
    int i;
    int ret = 0;
    //unsigned long tx_flags;
    unsigned int n = 0;
    unsigned int repeat = 2;
    //unsigned long timeout =jiffies + (HZ);
    
//    spin_lock_irqsave(&cec_tx_lock, cec_tx_flags);
    do {
        printk("\nCEC repeat:%x\n", repeat);
        printk("\nCEC CEC_TX_MSG_STATUS:%lx\n", hdmi_rd_reg(CEC0_BASE_ADDR+CEC_TX_MSG_STATUS));
        printk("\nCEC CEC_RX_MSG_STATUS:%lx\n", hdmi_rd_reg(CEC0_BASE_ADDR+CEC_RX_MSG_STATUS));
        if(hdmi_rd_reg(CEC0_BASE_ADDR+CEC_TX_MSG_STATUS) == TX_DONE)
            break;
	    while ((hdmi_rd_reg(CEC0_BASE_ADDR+CEC_TX_MSG_STATUS) != TX_IDLE) || (hdmi_rd_reg(CEC0_BASE_ADDR+CEC_RX_MSG_STATUS) != RX_IDLE)){
	        //msleep(10);
	        mdelay(10);
	        n++;
	        if(n >= 100){
	            printk("\nCEC START:TX TIMEOUT!\n");
	            //cec_hw_reset();
	            break;
	        }
	    } 
	     for (i = 0; i < len; i++) {
	         hdmi_wr_reg(CEC0_BASE_ADDR+CEC_TX_MSG_0_HEADER + i, msg[i]);
	         hdmitx_cec_dbg_print("CEC: tx msg[%d]:0x%x\n",i,msg[i]);
	     }
	     
	     hdmi_wr_reg(CEC0_BASE_ADDR+CEC_TX_MSG_LENGTH, len-1);
	     hdmi_wr_reg(CEC0_BASE_ADDR+CEC_TX_MSG_CMD, TX_REQ_CURRENT);//TX_REQ_NEXT
	  
	//     hdmitx_cec_dbg_print("\n****************\n");
	//     hdmitx_cec_dbg_print("CEC:CEC_TX_MSG_0_HEADER):0x%lx\n",hdmi_rd_reg(CEC0_BASE_ADDR+CEC_TX_MSG_0_HEADER));
	//     hdmitx_cec_dbg_print("CEC:CEC_TX_MSG_1_OPCODE):0x%lx\n",hdmi_rd_reg(CEC0_BASE_ADDR+CEC_TX_MSG_1_OPCODE));
	//     hdmitx_cec_dbg_print("CEC:CEC_TX_NUM_MSG):0x%lx\n",hdmi_rd_reg(CEC0_BASE_ADDR+CEC_TX_NUM_MSG));
	//     hdmitx_cec_dbg_print("CEC:CEC_TX_MSG_LENGTH):0x%lx\n",hdmi_rd_reg(CEC0_BASE_ADDR+CEC_TX_MSG_LENGTH));
	//     hdmitx_cec_dbg_print("CEC:CEC_TX_MSG_STATUS):0x%lx\n",hdmi_rd_reg(CEC0_BASE_ADDR+CEC_TX_MSG_STATUS));      
	                        
	//     printk("CEC: follow interrupt?\n");
	//     if (stat_header == NULL) {        
	//         while (hdmi_rd_reg(CEC0_BASE_ADDR+CEC_TX_MSG_STATUS) == TX_BUSY) {
	//             msleep(50);
	//             if(time_after(jiffies,timeout)){
	//                 hdmirx_cec_dbg_print("CEC: tx time out!\n");                 
	//                 break;
	//             }    
	//         }
	//     } else if (*stat_header == 1) { // ping        
	//         while (hdmi_rd_reg(CEC0_BASE_ADDR+CEC_TX_MSG_STATUS) == TX_BUSY) {
	//             msleep(50);
	//             if(time_after(jiffies,timeout)){
	//                 hdmirx_cec_dbg_print("CEC: _tx time out!\n");                
	//                 break;
	//             }                 
	//         }       
	//         
	//     }
	    if(hdmi_rd_reg(CEC0_BASE_ADDR+CEC_TX_MSG_STATUS) != TX_DONE){
	        while (hdmi_rd_reg(CEC0_BASE_ADDR+CEC_TX_MSG_STATUS) != TX_DONE){
	            if(hdmi_rd_reg(CEC0_BASE_ADDR+CEC_TX_MSG_STATUS) == TX_ERROR){
	                //msleep(10);
	                ret = TX_ERROR;
	                hdmi_wr_reg(CEC0_BASE_ADDR+CEC_TX_MSG_CMD, TX_ABORT);
	                cec_hw_reset(); 
	                repeat --;
	                printk("\nCEC: TX_ERROR!\n"); 
	                break;            
	            }        
	            //msleep(50);
	            mdelay(50);
	            n++;
	            printk("CEC: TX transmit\n");
	            if(n >= 20){
	                ret = TX_BUSY;
	                hdmi_wr_reg(CEC0_BASE_ADDR+CEC_TX_MSG_CMD, TX_ABORT);
	                cec_hw_reset();
	                repeat--;
	                printk("\nCEC END: TX TIMEOUT!\n");
	                break;
	            }
	        }
	    }else{
	        ret = TX_DONE;
	        hdmi_wr_reg(CEC0_BASE_ADDR+CEC_TX_MSG_CMD, TX_NO_OP);
	        break;
	    }        
     } while(repeat);              
    hdmi_wr_reg(CEC0_BASE_ADDR+CEC_TX_CLEAR_BUF, 0x1);
    {//Delay some time
	    int i = 10;
	    while(i--);
    }
    hdmi_wr_reg(CEC0_BASE_ADDR+CEC_TX_CLEAR_BUF, 0x0);
    //spin_unlock_irqrestore(&cec_tx_lock,tx_flags);
    //ret = hdmi_rd_reg(CEC0_BASE_ADDR+CEC_TX_MSG_STATUS);
    hdmi_wr_reg(CEC0_BASE_ADDR+CEC_TX_MSG_CMD, TX_NO_OP);
    
    return ret;
}

int cec_ll_tx(unsigned char *msg, unsigned char len, unsigned char *stat_header)
{
    int i;
    int ret = 0;
    //unsigned long tx_flags;
    unsigned int n = 0;
    unsigned int repeat = 2;
    //unsigned long timeout =jiffies + (HZ);
    
//    spin_lock_irqsave(&cec_tx_lock, cec_tx_flags);
    do {
        //printk("\nCEC repeat:%x\n", repeat);
        //printk("\nCEC CEC_TX_MSG_STATUS:%x\n", hdmi_rd_reg(CEC0_BASE_ADDR+CEC_TX_MSG_STATUS));
        //printk("\nCEC CEC_RX_MSG_STATUS:%x\n", hdmi_rd_reg(CEC0_BASE_ADDR+CEC_RX_MSG_STATUS));
        if(hdmi_rd_reg(CEC0_BASE_ADDR+CEC_TX_MSG_STATUS) == TX_DONE)
            break;
	    while ((hdmi_rd_reg(CEC0_BASE_ADDR+CEC_TX_MSG_STATUS) != TX_IDLE) || (hdmi_rd_reg(CEC0_BASE_ADDR+CEC_RX_MSG_STATUS) != RX_IDLE)){
	        msleep(10);
	        n++;
	        if(n >= 100){
	            printk("\nCEC START:TX TIMEOUT!\n");
	            //cec_hw_reset();
	            break;
	        }
	    } 
	     for (i = 0; i < len; i++) {
	         hdmi_wr_reg(CEC0_BASE_ADDR+CEC_TX_MSG_0_HEADER + i, msg[i]);
	         hdmitx_cec_dbg_print("CEC: tx msg[%d]:0x%x\n",i,msg[i]);
	     }
	     
	     hdmi_wr_reg(CEC0_BASE_ADDR+CEC_TX_MSG_LENGTH, len-1);
	     hdmi_wr_reg(CEC0_BASE_ADDR+CEC_TX_MSG_CMD, TX_REQ_CURRENT);//TX_REQ_NEXT
	  
	//     hdmitx_cec_dbg_print("\n****************\n");
	//     hdmitx_cec_dbg_print("CEC:CEC_TX_MSG_0_HEADER):0x%lx\n",hdmi_rd_reg(CEC0_BASE_ADDR+CEC_TX_MSG_0_HEADER));
	//     hdmitx_cec_dbg_print("CEC:CEC_TX_MSG_1_OPCODE):0x%lx\n",hdmi_rd_reg(CEC0_BASE_ADDR+CEC_TX_MSG_1_OPCODE));
	//     hdmitx_cec_dbg_print("CEC:CEC_TX_NUM_MSG):0x%lx\n",hdmi_rd_reg(CEC0_BASE_ADDR+CEC_TX_NUM_MSG));
	//     hdmitx_cec_dbg_print("CEC:CEC_TX_MSG_LENGTH):0x%lx\n",hdmi_rd_reg(CEC0_BASE_ADDR+CEC_TX_MSG_LENGTH));
	//     hdmitx_cec_dbg_print("CEC:CEC_TX_MSG_STATUS):0x%lx\n",hdmi_rd_reg(CEC0_BASE_ADDR+CEC_TX_MSG_STATUS));      
	                        
	//     printk("CEC: follow interrupt?\n");
	//     if (stat_header == NULL) {        
	//         while (hdmi_rd_reg(CEC0_BASE_ADDR+CEC_TX_MSG_STATUS) == TX_BUSY) {
	//             msleep(50);
	//             if(time_after(jiffies,timeout)){
	//                 hdmirx_cec_dbg_print("CEC: tx time out!\n");                 
	//                 break;
	//             }    
	//         }
	//     } else if (*stat_header == 1) { // ping        
	//         while (hdmi_rd_reg(CEC0_BASE_ADDR+CEC_TX_MSG_STATUS) == TX_BUSY) {
	//             msleep(50);
	//             if(time_after(jiffies,timeout)){
	//                 hdmirx_cec_dbg_print("CEC: _tx time out!\n");                
	//                 break;
	//             }                 
	//         }       
	//         
	//     }
	    if(hdmi_rd_reg(CEC0_BASE_ADDR+CEC_TX_MSG_STATUS) != TX_DONE){
	        while (hdmi_rd_reg(CEC0_BASE_ADDR+CEC_TX_MSG_STATUS) != TX_DONE){
	            if(hdmi_rd_reg(CEC0_BASE_ADDR+CEC_TX_MSG_STATUS) == TX_ERROR){
	                //msleep(10);
	                ret = TX_ERROR;
	                hdmi_wr_reg(CEC0_BASE_ADDR+CEC_TX_MSG_CMD, TX_ABORT);
	                cec_hw_reset(); 
	                repeat --;
	                printk("\nCEC: TX_ERROR!\n"); 
	                break;            
	            }        
	            msleep(50);
	            n++;
	            printk("CEC: TX transmit\n");
	            if(n >= 20){
	                ret = TX_BUSY;
	                hdmi_wr_reg(CEC0_BASE_ADDR+CEC_TX_MSG_CMD, TX_ABORT);
	                cec_hw_reset();
	                repeat--;
	                printk("\nCEC END: TX TIMEOUT!\n");
	                break;
	            }
	        }
	    }else{
	        ret = TX_DONE;
	        hdmi_wr_reg(CEC0_BASE_ADDR+CEC_TX_MSG_CMD, TX_NO_OP);
	        break;
	    }        
     } while(repeat);              
    hdmi_wr_reg(CEC0_BASE_ADDR+CEC_TX_CLEAR_BUF, 0x1);
    {//Delay some time
	    int i = 10;
	    while(i--);
    }
    hdmi_wr_reg(CEC0_BASE_ADDR+CEC_TX_CLEAR_BUF, 0x0);
    //spin_unlock_irqrestore(&cec_tx_lock,tx_flags);
    //ret = hdmi_rd_reg(CEC0_BASE_ADDR+CEC_TX_MSG_STATUS);
    hdmi_wr_reg(CEC0_BASE_ADDR+CEC_TX_MSG_CMD, TX_NO_OP);
    
    return ret;
}

int cec_ll_rx( unsigned char *msg, unsigned char *len)
{

    unsigned char i;
    unsigned char rx_status;
    unsigned char data;
    
    int rx_msg_length = hdmi_rd_reg(CEC0_BASE_ADDR + CEC_RX_MSG_LENGTH) + 1;
       
//    spin_lock(&cec_rx_lock);
//    hdmitx_cec_dbg_print("\n**************************************\n");
//    hdmitx_cec_dbg_print("CEC:CEC_RX_MSG_0_HEADER):0x%lx\n",hdmi_rd_reg(CEC0_BASE_ADDR+CEC_RX_MSG_0_HEADER));
//    hdmitx_cec_dbg_print("CEC:CEC_RX_MSG_1_OPCODE):0x%lx\n",hdmi_rd_reg(CEC0_BASE_ADDR+CEC_RX_MSG_1_OPCODE));
//    hdmitx_cec_dbg_print("CEC:CEC_RX_NUM_MSG):0x%lx\n",hdmi_rd_reg(CEC0_BASE_ADDR+CEC_RX_NUM_MSG));
//    hdmitx_cec_dbg_print("CEC:CEC_RX_MSG_STATUS):0x%lx\n",hdmi_rd_reg(CEC0_BASE_ADDR+CEC_RX_MSG_STATUS));
    hdmi_wr_reg(CEC0_BASE_ADDR + CEC_RX_MSG_CMD,  RX_ACK_CURRENT);
//    hdmitx_cec_dbg_print("\n****************\n");
    
    for (i = 0; i < rx_msg_length; i++) {
        data = hdmi_rd_reg(CEC0_BASE_ADDR + CEC_RX_MSG_0_HEADER +i);
        *msg = data;
        msg++;
        hdmitx_cec_dbg_print("CEC:rx data[%d]:0x%x\n",i,data);
    }
    *len = rx_msg_length;
    rx_status = hdmi_rd_reg(CEC0_BASE_ADDR+CEC_RX_MSG_STATUS);

    hdmi_wr_reg(CEC0_BASE_ADDR + CEC_RX_MSG_CMD, RX_DISABLE);

    hdmi_wr_reg(CEC0_BASE_ADDR + CEC_RX_MSG_CMD,  RX_NO_OP);
          
//    hdmi_wr_reg(CEC0_BASE_ADDR+CEC_RX_CLEAR_BUF, 0x1);
//    {//Delay some time
//	    int i = 10;
//	    while(i--);
//    }
//    hdmi_wr_reg(CEC0_BASE_ADDR+CEC_RX_CLEAR_BUF, 0x0);  
//    hdmitx_cec_dbg_print("\n****************\n");
//    hdmitx_cec_dbg_print("CEC:CEC_RX_MSG_0_HEADER):0x%lx\n",hdmi_rd_reg(CEC0_BASE_ADDR+CEC_RX_MSG_0_HEADER));
//    hdmitx_cec_dbg_print("CEC:CEC_RX_MSG_1_OPCODE):0x%lx\n",hdmi_rd_reg(CEC0_BASE_ADDR+CEC_RX_MSG_1_OPCODE));
//    hdmitx_cec_dbg_print("CEC:CEC_RX_NUM_MSG):0x%lx\n",hdmi_rd_reg(CEC0_BASE_ADDR+CEC_RX_NUM_MSG));
//    hdmitx_cec_dbg_print("CEC:CEC_RX_MSG_STATUS):0x%lx\n",hdmi_rd_reg(CEC0_BASE_ADDR+CEC_RX_MSG_STATUS));  

//    hdmitx_cec_dbg_print("\n**************************************\n");  
//    spin_unlock(&cec_rx_lock);

    return rx_status;
}

void cec_isr_post_process(void)
{
    /* isr post process */
    while(cec_rx_msg_buf.rx_read_pos != cec_rx_msg_buf.rx_write_pos) {
        cec_handle_message(&(cec_rx_msg_buf.cec_rx_message[cec_rx_msg_buf.rx_read_pos]));
        if (cec_rx_msg_buf.rx_read_pos == cec_rx_msg_buf.rx_buf_size - 1) {
            cec_rx_msg_buf.rx_read_pos = 0;
        } else {
            cec_rx_msg_buf.rx_read_pos++;
        }
    }

    //printk("[TV CEC RX]: rx_read_pos %x, rx_write_pos %x\n", cec_rx_msg_buf.rx_read_pos, cec_rx_msg_buf.rx_write_pos);
}

void cec_usr_cmd_post_process(void)
{
    cec_tx_message_list_t *p, *ptmp;
    /* usr command post process */
    //spin_lock_irqsave(&p_tx_list_lock, cec_tx_list_flags);

    list_for_each_entry_safe(p, ptmp, &cec_tx_msg_phead, list) {
        cec_ll_tx(p->msg, p->length, NULL);
        unregister_cec_tx_msg(p);
    }

    //spin_unlock_irqrestore(&p_tx_list_lock, cec_tx_list_flags);

    //printk("[TV CEC TX]: tx_msg_cnt = %x\n", tx_msg_cnt);
}

////void cec_timer_post_process(void)
////{
////    /* timer post process*/
////    if (cec_polling_state == TV_CEC_POLLING_ON) {
////        cec_tv_polling_online_dev();
////        cec_polling_state = TV_CEC_POLLING_OFF;
////    }
////}
void cec_node_init(hdmitx_dev_t* hdmitx_device)
{
	int i, bool = 0;
	//unsigned long cec_init_flags;
	
	enum _cec_log_dev_addr_e player_dev[3] = {   CEC_PLAYBACK_DEVICE_1_ADDR,
	    										 CEC_PLAYBACK_DEVICE_2_ADDR,
	    										 CEC_PLAYBACK_DEVICE_3_ADDR,
	    									  };
    if(!((hdmi_cec_func_config>>CEC_FUNC_MSAK) & 0x1))
        return ;
    printk("CEC node init\n");    
    // If VSDB is not valid, wait
    while(hdmitx_device->hdmi_info.vsdb_phy_addr.valid == 0)
        msleep(100);
        
    // Clear CEC Int. state and set CEC Int. mask
    WRITE_MPEG_REG(SYS_CPU_0_IRQ_IN1_INTR_STAT_CLR, READ_MPEG_REG(SYS_CPU_0_IRQ_IN1_INTR_STAT_CLR) | (1 << 23));    // Clear the interrupt
    WRITE_MPEG_REG(SYS_CPU_0_IRQ_IN1_INTR_MASK, READ_MPEG_REG(SYS_CPU_0_IRQ_IN1_INTR_MASK) | (1 << 23));            // Enable the hdmi cec interrupt

    
	for(i = 0; i < 3; i++){ 
//	    printk("CEC: start poll dev\n");  	
		cec_polling_online_dev(player_dev[i], &bool);
//		printk("CEC: end poll dev\n");
		if(bool == 0){  // 0 means that no any respond
            //cec_global_info.cec_node_info[player_dev[i]].power_status = TRANS_STANDBY_TO_ON;			
            cec_global_info.my_node_index = player_dev[i];
            cec_global_info.cec_node_info[player_dev[i]].log_addr = player_dev[i];
            // Set Physical address
            cec_global_info.cec_node_info[player_dev[i]].phy_addr.phy_addr_4 = ( ((hdmitx_device->hdmi_info.vsdb_phy_addr.a)<<12)
            	    												 +((hdmitx_device->hdmi_info.vsdb_phy_addr.b)<< 8)
            	    												 +((hdmitx_device->hdmi_info.vsdb_phy_addr.c)<< 4)
            	    												 +((hdmitx_device->hdmi_info.vsdb_phy_addr.d)    )
            	    												);
            	    												
            cec_global_info.cec_node_info[player_dev[i]].specific_info.audio.sys_audio_mode = OFF;
            cec_global_info.cec_node_info[player_dev[i]].specific_info.audio.audio_status.audio_mute_status = OFF; 
            cec_global_info.cec_node_info[player_dev[i]].specific_info.audio.audio_status.audio_volume_status = 0;         
                        	    												
            cec_global_info.cec_node_info[player_dev[i]].vendor_id = ('A'<<16)|('m'<<8)|'l';
            cec_global_info.cec_node_info[player_dev[i]].dev_type = cec_log_addr_to_dev_type(player_dev[i]);
            cec_global_info.cec_node_info[player_dev[i]].dev_type = cec_log_addr_to_dev_type(player_dev[i]);
            strcpy(cec_global_info.cec_node_info[player_dev[i]].osd_name, default_osd_name[player_dev[i]]); //Max num: 14Bytes
            //printk("CEC: Set logical address: %d\n", hdmi_rd_reg(CEC0_BASE_ADDR+CEC_LOGICAL_ADDR0));
            hdmi_wr_reg(CEC0_BASE_ADDR+CEC_LOGICAL_ADDR0, (0x1 << 4) | player_dev[i]);
		    
     		printk("CEC: Set logical address: %d\n", player_dev[i]);
            
            //cec_hw_reset();
            //spin_lock_irqsave(&cec_init_lock,cec_init_flags);
            printk("aml_read_reg32(P_AO_DEBUG_REG0):0x%x\n" ,aml_read_reg32(P_AO_DEBUG_REG0));
            //if((aml_read_reg32(P_AO_DEBUG_REG0) >> 4) & 0x1) {
		        cec_imageview_on_smp();
    		    cec_active_source_smp();
    		//}
		    //cec_usrcmd_set_report_physical_address();
		    cec_report_physical_address_smp();
		    
		    cec_get_menu_language_smp();
		    
		    //cec_device_vendor_id_smp();
		    
		    //cec_usrcmd_set_imageview_on( CEC_TV_ADDR );   // Wakeup TV

		    //msleep(200);
		    //cec_usrcmd_set_imageview_on( CEC_TV_ADDR );   // Wakeup TV again
		    //msleep(200);
     		//printk("CEC: Set physical address: %x\n", cec_global_info.cec_node_info[player_dev[i]].phy_addr.phy_addr_4);
     		
    		//cec_usrcmd_set_active_source(); 

    		//spin_unlock_irqrestore(&cec_init_lock,cec_init_flags);
    		//cec_active_source(&(cec_rx_msg_buf.cec_rx_message[cec_rx_msg_buf.rx_read_pos]));    
   		
    		cec_menu_status_smp();

     		  		
			cec_global_info.cec_node_info[cec_global_info.my_node_index].power_status = POWER_ON;
			cec_power_flag = 1;
    		break;
    		
		}
	}	
	if(bool == 1)
		printk("CEC: Can't get a valid logical address\n");
}

void cec_node_uninit(hdmitx_dev_t* hdmitx_device)
{
    if(!((hdmi_cec_func_config>>CEC_FUNC_MSAK) & 0x1))        
       return ;
    cec_power_flag = 0;
    printk("CEC: cec node uninit!\n");
    WRITE_MPEG_REG(SYS_CPU_0_IRQ_IN1_INTR_MASK, READ_MPEG_REG(SYS_CPU_0_IRQ_IN1_INTR_MASK) & ~(1 << 23));            // Disable the hdmi cec interrupt
    //free_irq(INT_HDMI_CEC, (void *)hdmitx_device);
}

static int cec_task(void *data)
{
	extern void dump_hdmi_cec_reg(void);
    hdmitx_dev_t* hdmitx_device = (hdmitx_dev_t*) data;

//    printk("CEC: Physical Address [A]: %x\n",hdmitx_device->hdmi_info.vsdb_phy_addr.a);
//    printk("CEC: Physical Address [B]: %x\n",hdmitx_device->hdmi_info.vsdb_phy_addr.b);
//    printk("CEC: Physical Address [C]: %x\n",hdmitx_device->hdmi_info.vsdb_phy_addr.c);
//    printk("CEC: Physical Address [D]: %x\n",hdmitx_device->hdmi_info.vsdb_phy_addr.d);

    cec_init_flag = 1;
    
    cec_node_init(hdmitx_device);
    
//    dump_hdmi_cec_reg();
    
    // Get logical address

    printk("CEC: CEC task process\n");

    while (1) {
        //if((hdmi_cec_func_config>>CEC_FUNC_MSAK) & 0x1)
        //{
        //   cec_node_init(hdmitx_device);          
        //}
        //else
        //{
        //   cec_node_uninit(hdmitx_device);            
        //}
            
        if(down_interruptible(&tv_cec_sema))
           continue; 
                
        cec_isr_post_process();
        cec_usr_cmd_post_process();
        //\\cec_timer_post_process();
    }

    return 0;
}

/***************************** cec low level code end *****************************/


/***************************** cec middle level code *****************************/

void register_cec_rx_msg(unsigned char *msg, unsigned char len )
{
    unsigned long flags;
    //    hdmitx_cec_dbg_print("\nCEC:function:%s,file:%s,line:%d\n",__FUNCTION__,__FILE__,__LINE__);  
    memset((void*)(&(cec_rx_msg_buf.cec_rx_message[cec_rx_msg_buf.rx_write_pos])), 0, sizeof(cec_rx_message_t));
    memcpy(cec_rx_msg_buf.cec_rx_message[cec_rx_msg_buf.rx_write_pos].content.buffer, msg, len);

    cec_rx_msg_buf.cec_rx_message[cec_rx_msg_buf.rx_write_pos].operand_num = len >= 2 ? len - 2 : 0;
    cec_rx_msg_buf.cec_rx_message[cec_rx_msg_buf.rx_write_pos].msg_length = len;
    
    //spin_lock(&cec_input_key);
    spin_lock_irqsave(&cec_input_key,flags);
    cec_input_handle_message();
    spin_unlock_irqrestore(&cec_input_key,flags);
    //spin_unlock(&cec_input_key);    
    //wake_up_interruptible(&cec_key_poll);
    if (cec_rx_msg_buf.rx_write_pos == cec_rx_msg_buf.rx_buf_size - 1) {
        cec_rx_msg_buf.rx_write_pos = 0;
    } else {
        cec_rx_msg_buf.rx_write_pos++;
    }

    up(&tv_cec_sema);    
}

void register_cec_tx_msg(unsigned char *msg, unsigned char len )
{
    cec_tx_message_list_t* cec_usr_message_list = kmalloc(sizeof(cec_tx_message_list_t), GFP_ATOMIC);

    if (cec_usr_message_list != NULL) {
        memset(cec_usr_message_list, 0, sizeof(cec_tx_message_list_t));
        memcpy(cec_usr_message_list->msg, msg, len);
        cec_usr_message_list->length = len;

        spin_lock_irqsave(&p_tx_list_lock, cec_tx_list_flags);
        list_add_tail(&cec_usr_message_list->list, &cec_tx_msg_phead);
        spin_unlock_irqrestore(&p_tx_list_lock, cec_tx_list_flags);

        tx_msg_cnt++;
        up(&tv_cec_sema); 
    }
}
void cec_input_handle_message(void)
{
    unsigned char   opcode;
    //unsigned char   operand_num;
    //unsigned char   msg_length;
    
//    hdmitx_cec_dbg_print("\nCEC:function:%s,file:%s,line:%d\n",__FUNCTION__,__FILE__,__LINE__);  

    opcode = cec_rx_msg_buf.cec_rx_message[cec_rx_msg_buf.rx_write_pos].content.msg.opcode;   
    //operand_num = cec_rx_msg_buf.cec_rx_message[cec_rx_msg_buf.rx_write_pos].operand_num;
    //msg_length  = cec_rx_msg_buf.cec_rx_message[cec_rx_msg_buf.rx_write_pos].msg_length;

    /* process messages from tv polling and cec devices */
    printk("----OP code----: %x\n", opcode);
    if(((hdmi_cec_func_config>>CEC_FUNC_MSAK) & 0x1) && cec_power_flag)
    {
        switch (opcode) {
        /*case CEC_OC_ACTIVE_SOURCE:
            cec_active_source(pcec_message);
            break;
        case CEC_OC_INACTIVE_SOURCE:
            cec_deactive_source(pcec_message);
            break;
        case CEC_OC_CEC_VERSION:
            cec_report_version(pcec_message);
            break;
        case CEC_OC_DECK_STATUS:
            cec_deck_status(pcec_message);
            break;
        case CEC_OC_DEVICE_VENDOR_ID:
            cec_device_vendor_id(pcec_message);
            break;
        case CEC_OC_FEATURE_ABORT:
            cec_feature_abort(pcec_message);
            break;
        case CEC_OC_GET_CEC_VERSION:
            cec_get_version(pcec_message);
            break;
        case CEC_OC_GIVE_DECK_STATUS:
            cec_give_deck_status(pcec_message);
            break;
        case CEC_OC_MENU_STATUS:
            cec_menu_status(pcec_message);
            break;
        case CEC_OC_REPORT_PHYSICAL_ADDRESS:
            cec_report_phy_addr(pcec_message);
            break;
        case CEC_OC_REPORT_POWER_STATUS:
            cec_report_power_status(pcec_message);
            break;
        case CEC_OC_SET_OSD_NAME:
            cec_set_osd_name(pcec_message);
            break;
        case CEC_OC_VENDOR_COMMAND_WITH_ID:
            cec_vendor_cmd_with_id(pcec_message);
            break;
        case CEC_OC_SET_MENU_LANGUAGE:
            cec_set_menu_language(pcec_message);
            break;
        case CEC_OC_GIVE_PHYSICAL_ADDRESS:
            cec_give_physical_address(pcec_message);
            break;
        case CEC_OC_GIVE_DEVICE_VENDOR_ID:
            cec_give_device_vendor_id(pcec_message);
            break;
        case CEC_OC_GIVE_OSD_NAME:
            cec_give_osd_name(pcec_message);
            break;
        case CEC_OC_STANDBY:
              printk("----cec_standby-----");
            cec_standby(pcec_message);
            break;
        case CEC_OC_SET_STREAM_PATH:
            cec_set_stream_path(pcec_message);
            break;
        case CEC_OC_REQUEST_ACTIVE_SOURCE:
            cec_request_active_source(pcec_message);
            break;
        case CEC_OC_GIVE_DEVICE_POWER_STATUS:
            cec_give_device_power_status(pcec_message);
            break;
            */
         case CEC_OC_STANDBY:
         	cec_inactive_source(); 	  
            cec_standby_irq();
            break;       
        case CEC_OC_USER_CONTROL_PRESSED:
            cec_user_control_pressed_irq();
            break;
        case CEC_OC_USER_CONTROL_RELEASED:
            //cec_user_control_released_irq();
            break; 
        //case CEC_OC_IMAGE_VIEW_ON:      //not support in source
        //      cec_usrcmd_set_imageview_on( CEC_TV_ADDR );   // Wakeup TV
        //      break;  
        case CEC_OC_ROUTING_CHANGE: 
        case CEC_OC_VENDOR_REMOTE_BUTTON_DOWN:
        case CEC_OC_VENDOR_REMOTE_BUTTON_UP:
        case CEC_OC_CLEAR_ANALOGUE_TIMER:
        case CEC_OC_CLEAR_DIGITAL_TIMER:
        case CEC_OC_CLEAR_EXTERNAL_TIMER:
        case CEC_OC_DECK_CONTROL:
        case CEC_OC_GIVE_SYSTEM_AUDIO_MODE_STATUS:
        case CEC_OC_GIVE_TUNER_DEVICE_STATUS:
        case CEC_OC_MENU_REQUEST:
        case CEC_OC_SET_OSD_STRING:
        case CEC_OC_SET_SYSTEM_AUDIO_MODE:
        case CEC_OC_SET_TIMER_PROGRAM_TITLE:
        case CEC_OC_SYSTEM_AUDIO_MODE_REQUEST:
        case CEC_OC_SYSTEM_AUDIO_MODE_STATUS:
        case CEC_OC_TEXT_VIEW_ON:       //not support in source
        case CEC_OC_TIMER_CLEARED_STATUS:
        case CEC_OC_TIMER_STATUS:
        case CEC_OC_TUNER_DEVICE_STATUS:
        case CEC_OC_TUNER_STEP_DECREMENT:
        case CEC_OC_TUNER_STEP_INCREMENT:
        case CEC_OC_VENDOR_COMMAND:
        case CEC_OC_ROUTING_INFORMATION:
        case CEC_OC_SELECT_ANALOGUE_SERVICE:
        case CEC_OC_SELECT_DIGITAL_SERVICE:
        case CEC_OC_SET_ANALOGUE_TIMER :
        case CEC_OC_SET_AUDIO_RATE:
        case CEC_OC_SET_DIGITAL_TIMER:
        case CEC_OC_SET_EXTERNAL_TIMER:
        case CEC_OC_PLAY:
        case CEC_OC_RECORD_OFF:
        case CEC_OC_RECORD_ON:
        case CEC_OC_RECORD_STATUS:
        case CEC_OC_RECORD_TV_SCREEN:
        case CEC_OC_REPORT_AUDIO_STATUS:
        case CEC_OC_GET_MENU_LANGUAGE:
        case CEC_OC_GIVE_AUDIO_STATUS:
        case CEC_OC_ABORT_MESSAGE:
            printk("CEC: not support cmd: %x\n", opcode);
            break;
        default:
            break;
        }
    }
}

void unregister_cec_tx_msg(cec_tx_message_list_t* cec_tx_message_list)
{

    if (cec_tx_message_list != NULL) {
        list_del(&cec_tx_message_list->list);
        kfree(cec_tx_message_list);
        cec_tx_message_list = NULL;

        if (tx_msg_cnt > 0) tx_msg_cnt--;
    }
}

void cec_hw_reset(void)
{
    unsigned char index = cec_global_info.my_node_index;
#ifdef CONFIG_ARCH_MESON6
    aml_write_reg32(APB_REG_ADDR(HDMI_CNTL_PORT), aml_read_reg32(APB_REG_ADDR(HDMI_CNTL_PORT))|(1<<16));
#else 
    WRITE_APB_REG(HDMI_CNTL_PORT, READ_APB_REG(HDMI_CNTL_PORT)|(1<<16));
#endif
    hdmi_wr_reg(OTHER_BASE_ADDR+HDMI_OTHER_CTRL0, 0xc); //[3]cec_creg_sw_rst [2]cec_sys_sw_rst
    hdmi_wr_reg(CEC0_BASE_ADDR+CEC_TX_CLEAR_BUF, 0x1);
    hdmi_wr_reg(CEC0_BASE_ADDR+CEC_RX_CLEAR_BUF, 0x1);
    
    //mdelay(10);
    {//Delay some time
    	int i = 10;
    	while(i--);
    }
    hdmi_wr_reg(CEC0_BASE_ADDR+CEC_TX_CLEAR_BUF, 0x0);
    hdmi_wr_reg(CEC0_BASE_ADDR+CEC_RX_CLEAR_BUF, 0x0);
    hdmi_wr_reg(OTHER_BASE_ADDR+HDMI_OTHER_CTRL0, 0x0);
//    WRITE_APB_REG(HDMI_CNTL_PORT, READ_APB_REG(HDMI_CNTL_PORT)&(~(1<<16)));
#ifdef CONFIG_ARCH_MESON6
    aml_write_reg32(APB_REG_ADDR(HDMI_CNTL_PORT), aml_read_reg32(APB_REG_ADDR(HDMI_CNTL_PORT))&(~(1<<16)));
#else
    WRITE_APB_REG(HDMI_CNTL_PORT, READ_APB_REG(HDMI_CNTL_PORT)&(~(1<<16)));
#endif
    hdmi_wr_reg(CEC0_BASE_ADDR+CEC_CLOCK_DIV_H, 0x00 );
    hdmi_wr_reg(CEC0_BASE_ADDR+CEC_CLOCK_DIV_L, 0xf0 );

    hdmi_wr_reg(CEC0_BASE_ADDR+CEC_LOGICAL_ADDR0, (0x1 << 4) | cec_global_info.cec_node_info[index].log_addr);
    printk("CEC_LOGICAL_ADDR0:0x%lx\n",hdmi_rd_reg(CEC0_BASE_ADDR+CEC_LOGICAL_ADDR0));
}

unsigned char check_cec_msg_valid(const cec_rx_message_t* pcec_message)
{
    unsigned char rt = 0;
    unsigned char opcode;
    unsigned char opernum;
    if (!pcec_message)
        return rt;

    opcode = pcec_message->content.msg.opcode;
    opernum = pcec_message->operand_num;

    switch (opcode) {
        case CEC_OC_VENDOR_REMOTE_BUTTON_UP:
        case CEC_OC_STANDBY:
        case CEC_OC_RECORD_OFF:
        case CEC_OC_RECORD_TV_SCREEN:
        case CEC_OC_TUNER_STEP_DECREMENT:
        case CEC_OC_TUNER_STEP_INCREMENT:
        case CEC_OC_GIVE_AUDIO_STATUS:
        case CEC_OC_GIVE_SYSTEM_AUDIO_MODE_STATUS:
        case CEC_OC_USER_CONTROL_RELEASED:
        case CEC_OC_GIVE_OSD_NAME:
        case CEC_OC_GIVE_PHYSICAL_ADDRESS:
        case CEC_OC_GET_CEC_VERSION:
        case CEC_OC_GET_MENU_LANGUAGE:
        case CEC_OC_GIVE_DEVICE_VENDOR_ID:
        case CEC_OC_GIVE_DEVICE_POWER_STATUS:
        case CEC_OC_TEXT_VIEW_ON:
        case CEC_OC_IMAGE_VIEW_ON:
        case CEC_OC_ABORT_MESSAGE:
        case CEC_OC_REQUEST_ACTIVE_SOURCE:
            if ( opernum == 0)  rt = 1;
            break;
        case CEC_OC_SET_SYSTEM_AUDIO_MODE:
        case CEC_OC_RECORD_STATUS:
        case CEC_OC_DECK_CONTROL:
        case CEC_OC_DECK_STATUS:
        case CEC_OC_GIVE_DECK_STATUS:
        case CEC_OC_GIVE_TUNER_DEVICE_STATUS:
        case CEC_OC_PLAY:
        case CEC_OC_MENU_REQUEST:
        case CEC_OC_MENU_STATUS:
        case CEC_OC_REPORT_AUDIO_STATUS:
        case CEC_OC_TIMER_CLEARED_STATUS:
        case CEC_OC_SYSTEM_AUDIO_MODE_STATUS:
        case CEC_OC_USER_CONTROL_PRESSED:
        case CEC_OC_CEC_VERSION:
        case CEC_OC_REPORT_POWER_STATUS:
        case CEC_OC_SET_AUDIO_RATE:
            if ( opernum == 1)  rt = 1;
            break;
        case CEC_OC_INACTIVE_SOURCE:
        case CEC_OC_SYSTEM_AUDIO_MODE_REQUEST:
        case CEC_OC_FEATURE_ABORT:
        case CEC_OC_ACTIVE_SOURCE:
        case CEC_OC_ROUTING_INFORMATION:
        case CEC_OC_SET_STREAM_PATH:
            if (opernum == 2) rt = 1;
            break;
        case CEC_OC_REPORT_PHYSICAL_ADDRESS:
        case CEC_OC_SET_MENU_LANGUAGE:
        case CEC_OC_DEVICE_VENDOR_ID:
            if (opernum == 3) rt = 1;
            break;
        case CEC_OC_ROUTING_CHANGE:
        case CEC_OC_SELECT_ANALOGUE_SERVICE:
            if (opernum == 4) rt = 1;
            break;
        case CEC_OC_VENDOR_COMMAND_WITH_ID:
            if ((opernum > 3)&&(opernum < 15))  rt = 1;
            break;
        case CEC_OC_VENDOR_REMOTE_BUTTON_DOWN:
            if (opernum < 15)  rt = 1;
            break;
        case CEC_OC_SELECT_DIGITAL_SERVICE:
            if (opernum == 7) rt = 1;
            break;
        case CEC_OC_SET_ANALOGUE_TIMER:
        case CEC_OC_CLEAR_ANALOGUE_TIMER:
            if (opernum == 11) rt = 1;
            break;
        case CEC_OC_SET_DIGITAL_TIMER:
        case CEC_OC_CLEAR_DIGITAL_TIMER:
            if (opernum == 14) rt = 1;
            break;
        case CEC_OC_TIMER_STATUS:
            if ((opernum == 1 || opernum == 3)) rt = 1;
            break;
        case CEC_OC_TUNER_DEVICE_STATUS:
            if ((opernum == 5 || opernum == 8)) rt = 1;
            break;
        case CEC_OC_RECORD_ON:
            if (opernum > 0 && opernum < 9)  rt = 1;
            break;
        case CEC_OC_CLEAR_EXTERNAL_TIMER:
        case CEC_OC_SET_EXTERNAL_TIMER:
            if ((opernum == 9 || opernum == 10)) rt = 1;
            break;
        case CEC_OC_SET_TIMER_PROGRAM_TITLE:
        case CEC_OC_SET_OSD_NAME:
            if (opernum > 0 && opernum < 15) rt = 1;
            break;
        case CEC_OC_SET_OSD_STRING:
            if (opernum > 1 && opernum < 15) rt = 1;
            break;
        case CEC_OC_VENDOR_COMMAND:
            if (opernum < 15)   rt = 1;
            break;
        default:
            rt = 1;
            break;
    }

    if ((rt == 0) & (opcode != 0)){
        hdmirx_cec_dbg_print("CEC: opcode & opernum not match: %x, %x\n", opcode, opernum);
    }
    
    //?????rt = 1; // temporal
    return rt;
}
static char *rx_status[] = {
    "RX_IDLE ",
    "RX_BUSY ",
    "RX_DONE ",
    "RX_ERROR",
};

//static char *tx_status[] = {
//    "TX_IDLE ",
//    "TX_BUSY ",
//    "TX_DONE ",
//    "TX_ERROR",
//};

static irqreturn_t cec_isr_handler(int irq, void *dev_instance)
{
    unsigned int data_msg_num, data_msg_stat;
    unsigned int n;

    if (cec_pending_flag == TV_CEC_PENDING_ON) {
        // Clear the interrupt
        WRITE_MPEG_REG(SYS_CPU_0_IRQ_IN1_INTR_STAT_CLR, READ_MPEG_REG(SYS_CPU_0_IRQ_IN1_INTR_STAT_CLR) & ~(1 << 23));
        return IRQ_NONE;
    }
    data_msg_stat = hdmi_rd_reg(CEC0_BASE_ADDR+CEC_RX_MSG_STATUS);
    if (data_msg_stat) {
//        hdmirx_cec_dbg_print("CEC Irq Rx Status: %s\n", rx_status[data_msg_stat&3]);
        if ((data_msg_stat & 0x3) == RX_DONE) {
            data_msg_num = hdmi_rd_reg(CEC0_BASE_ADDR + CEC_RX_NUM_MSG);
            if (0x0 == data_msg_num){
                cec_hw_reset();
                //hdmi_wr_reg(CEC0_BASE_ADDR + CEC_RX_MSG_CMD,  RX_ACK_NEXT);
                printk("CEC: RX ERROR:data_msg_num=0,reset!\n");
            }
            for (n = 0; n < data_msg_num; n++) {                
                unsigned char rx_msg[MAX_MSG], rx_len;
                cec_ll_rx(rx_msg, &rx_len);
                register_cec_rx_msg(rx_msg, rx_len);                
                //hdmi_wr_reg(CEC0_BASE_ADDR + CEC_RX_CLEAR_BUF,  0x01);
                //hdmi_wr_reg(CEC0_BASE_ADDR + CEC_RX_CLEAR_BUF,  0x0);
            }
        } else {
            //hdmi_wr_reg(CEC0_BASE_ADDR + CEC_RX_CLEAR_BUF,  0x01);
            //hdmi_wr_reg(CEC0_BASE_ADDR + CEC_RX_CLEAR_BUF,  0x0);
            hdmi_wr_reg(CEC0_BASE_ADDR + CEC_RX_MSG_CMD,  RX_NO_OP);                
            cec_hw_reset();
            printk("CEC: recevie error[%s]\n", rx_status[data_msg_stat&3]);
        }
    }
    //tasklet_schedule(&tasklet_cec);  

    data_msg_stat = hdmi_rd_reg(CEC0_BASE_ADDR+CEC_TX_MSG_STATUS);
    if (data_msg_stat) {
//        hdmirx_cec_dbg_print("CEC Irq Tx Status: %s\n", tx_status[data_msg_stat&3]);
    }
    
    //cec_hw_reset();
    // Clear the interrupt
    WRITE_MPEG_REG(SYS_CPU_0_IRQ_IN1_INTR_STAT_CLR, READ_MPEG_REG(SYS_CPU_0_IRQ_IN1_INTR_STAT_CLR) & ~(1 << 23));

    return IRQ_HANDLED;
}

unsigned short cec_log_addr_to_dev_type(unsigned char log_addr)
{
    unsigned short us = CEC_UNREGISTERED_DEVICE_TYPE;
    if ((1 << log_addr) & CEC_DISPLAY_DEVICE) {
        us = CEC_DISPLAY_DEVICE_TYPE;
    } else if ((1 << log_addr) & CEC_RECORDING_DEVICE) {
        us = CEC_RECORDING_DEVICE_TYPE;
    } else if ((1 << log_addr) & CEC_PLAYBACK_DEVICE) {
        us = CEC_PLAYBACK_DEVICE_TYPE;
    } else if ((1 << log_addr) & CEC_TUNER_DEVICE) {
        us = CEC_TUNER_DEVICE_TYPE;
    } else if ((1 << log_addr) & CEC_AUDIO_SYSTEM_DEVICE) {
        us = CEC_AUDIO_SYSTEM_DEVICE_TYPE;
    }

    return us;
}
//
//static cec_hdmi_port_e cec_find_hdmi_port(unsigned char log_addr)
//{
//    cec_hdmi_port_e rt = CEC_HDMI_PORT_UKNOWN;
//
//    if ((cec_global_info.dev_mask & (1 << log_addr)) &&
//            (cec_global_info.cec_node_info[log_addr].phy_addr != 0) &&
//            (cec_global_info.cec_node_info[log_addr].hdmi_port == CEC_HDMI_PORT_UKNOWN)) {
//        if ((cec_global_info.cec_node_info[log_addr].phy_addr & 0xF000) == 0x1000) {
//            cec_global_info.cec_node_info[log_addr].hdmi_port = CEC_HDMI_PORT_1;
//        } else if ((cec_global_info.cec_node_info[log_addr].phy_addr & 0xF000) == 0x2000) {
//            cec_global_info.cec_node_info[log_addr].hdmi_port = CEC_HDMI_PORT_2;
//        } else if ((cec_global_info.cec_node_info[log_addr].phy_addr & 0xF000) == 0x3000) {
//            cec_global_info.cec_node_info[log_addr].hdmi_port = CEC_HDMI_PORT_3;
//        }
//    }
//
//    rt = cec_global_info.cec_node_info[log_addr].hdmi_port;
//
//    return rt;
//}

// -------------- command from cec devices ---------------------

void cec_polling_online_dev(int log_addr, int *bool)
{
    //int log_addr = 0;
    int r;
    unsigned short dev_mask_tmp = 0;
    unsigned char msg[1];
    unsigned char ping = 1;
    unsigned int n =0;

    //for (log_addr = 1; log_addr < CEC_UNREGISTERED_ADDR; log_addr++) {
        msg[0] = (log_addr<<4) | log_addr;
        hdmi_wr_reg(CEC0_BASE_ADDR+CEC_LOGICAL_ADDR0, (0x1 << 4) | log_addr);     
        r = cec_ll_tx(msg, 1, &ping);
        //r = TX_DONE;
        hdmitx_cec_dbg_print("\n --cec_polling--r:%d\n",r);

        while (r == TX_BUSY)
        {
            msleep(10);
            n++;
            if(n >= 100){
            	  printk("\nCEC POLLING TIMEOUT!\n");
                break;
            }
        }
            
        if (r == TX_DONE) {
            //dev_mask_tmp |= 1 << log_addr;
            //cec_global_info.cec_node_info[log_addr].log_addr = log_addr;
            //cec_global_info.cec_node_info[log_addr].dev_type = cec_log_addr_to_dev_type(log_addr);
            //cec_global_info.cec_node_info[log_addr].dev_type = CEC_PLAYBACK_DEVICE_TYPE;
//            cec_find_hdmi_port(log_addr);
            *bool = 1;
            //cec_hw_reset();
            //msleep(200);
        }else{
            dev_mask_tmp &= ~(1 << log_addr);
            memset(&(cec_global_info.cec_node_info[log_addr]), 0, sizeof(cec_node_info_t));
            //cec_global_info.cec_node_info[log_addr].log_addr = log_addr;
            //cec_global_info.cec_node_info[log_addr].dev_type = cec_log_addr_to_dev_type(log_addr);            
            cec_global_info.cec_node_info[log_addr].dev_type = cec_log_addr_to_dev_type(log_addr);
        	  *bool = 0;
        }
    //if (r != TX_DONE) {
    //    dev_mask_tmp &= ~(1 << log_addr);
    //    memset(&(cec_global_info.cec_node_info[log_addr]), 0, sizeof(cec_node_info_t));
    //    //cec_global_info.cec_node_info[log_addr].log_addr = log_addr;
    //    //cec_global_info.cec_node_info[log_addr].dev_type = cec_log_addr_to_dev_type(log_addr);            
    //    cec_global_info.cec_node_info[log_addr].dev_type = cec_log_addr_to_dev_type(log_addr);
    //    *bool = 0;
    //}
    //if (r == TX_DONE) {
    //    dev_mask_tmp |= 1 << log_addr;
    //    cec_global_info.cec_node_info[log_addr].log_addr = log_addr;
    //    cec_global_info.cec_node_info[log_addr].dev_type = cec_log_addr_to_dev_type(log_addr);
    //    //cec_global_info.cec_node_info[log_addr].dev_type = CEC_PLAYBACK_DEVICE_TYPE;
    //    //cec_find_hdmi_port(log_addr);
    //    *bool = 1;
    //    //cec_hw_reset();
    //    msleep(200);
    //}       
    //}
    printk("CEC: poll online logic device: 0x%x BOOL: %d\n", log_addr, *bool);

    if (cec_global_info.dev_mask != dev_mask_tmp) {
        cec_global_info.dev_mask = dev_mask_tmp;
    }

    //hdmirx_cec_dbg_print("cec log device exist: %x\n", dev_mask_tmp);
}

void cec_report_phy_addr(cec_rx_message_t* pcec_message)
{
    //unsigned char index = cec_global_info.my_node_index;
    ////unsigned char log_addr = pcec_message->content.msg.header >> 4;
    //cec_global_info.dev_mask |= 1 << index;
    //cec_global_info.cec_node_info[index].dev_type = cec_log_addr_to_dev_type(index);
    //cec_global_info.cec_node_info[index].real_info_mask |= INFO_MASK_DEVICE_TYPE;
    //memcpy(cec_global_info.cec_node_info[index].osd_name_def, default_osd_name[index], 16);
    //if ((cec_global_info.cec_node_info[index].real_info_mask & INFO_MASK_OSD_NAME) == 0) {
    //    memcpy(cec_global_info.cec_node_info[index].osd_name, osd_name_uninit, 16);
    //}
    //cec_global_info.cec_node_info[index].log_addr = index;
    //cec_global_info.cec_node_info[index].real_info_mask |= INFO_MASK_LOGIC_ADDRESS;
    //cec_global_info.cec_node_info[index].phy_addr.phy_addr_4 = (pcec_message->content.msg.operands[0] << 8) | pcec_message->content.msg.operands[1];
    //cec_global_info.cec_node_info[index].real_info_mask |= INFO_MASK_PHYSICAL_ADDRESS;
//

}

void cec_give_physical_address(cec_rx_message_t* pcec_message)
{
    unsigned char log_addr = pcec_message->content.msg.header >> 4;
    unsigned char index = cec_global_info.my_node_index;
    unsigned char phy_addr_ab = (aml_read_reg32(P_AO_DEBUG_REG1) >> 8) & 0xff;
    unsigned char phy_addr_cd = aml_read_reg32(P_AO_DEBUG_REG1) & 0xff;
    
    //if (cec_global_info.dev_mask & (1 << log_addr)) {
        unsigned char msg[5];
        msg[0] = ((index & 0xf) << 4) | log_addr;
        msg[1] = CEC_OC_REPORT_PHYSICAL_ADDRESS;
        msg[2] = phy_addr_ab;
        msg[3] = phy_addr_cd;
        msg[4] = cec_global_info.cec_node_info[index].log_addr;
        //msg[2] = cec_global_info.cec_node_info[index].phy_addr.phy_addr_2.ab;
        //msg[3] = cec_global_info.cec_node_info[index].phy_addr.phy_addr_2.cd;
        //msg[4] = cec_global_info.cec_node_info[index].log_addr;
        cec_ll_tx(msg, 5, NULL);
    //}
//    hdmirx_cec_dbg_print("cec_report_phy_addr: %x\n", cec_global_info.cec_node_info[index].log_addr);
}

//***************************************************************
void cec_device_vendor_id(cec_rx_message_t* pcec_message)
{
    unsigned char index = cec_global_info.my_node_index;
    unsigned char msg[9];
    
    msg[0] = ((index & 0xf) << 4) | CEC_BROADCAST_ADDR;
    msg[1] = CEC_OC_DEVICE_VENDOR_ID;
    memcpy(&msg[2], "PHILIPS", strlen("PHILIPS"));
    
    cec_ll_tx(msg, 9, NULL);
}

////////////////////////////////////////////////
void cec_give_osd_name(cec_rx_message_t* pcec_message)
{

    unsigned char log_addr = pcec_message->content.msg.header >> 4;
    unsigned char index = cec_global_info.my_node_index;
	unsigned char osd_len = strlen(cec_global_info.cec_node_info[index].osd_name);
    //if (cec_global_info.dev_mask & (1 << log_addr)) {
        unsigned char msg[16];
        msg[0] = ((index & 0xf) << 4) | log_addr;
        msg[1] = CEC_OC_SET_OSD_NAME;
        memcpy(&msg[2], cec_global_info.cec_node_info[index].osd_name, osd_len);
//        msg[2] = (cec_global_info.cec_node_info[index].vendor_id >> 16) & 0xff;
//        msg[3] = (cec_global_info.cec_node_info[index].vendor_id >> 8) & 0xff;
//        msg[4] = (cec_global_info.cec_node_info[index].vendor_id >> 0) & 0xff;
        cec_ll_tx(msg, 2 + osd_len, NULL);
    //}
//    hdmirx_cec_dbg_print("%s: %x\n", cec_global_info.cec_node_info[index].log_addr);
}


void cec_report_power_status(cec_rx_message_t* pcec_message)
{
    //unsigned char log_addr = pcec_message->content.msg.header >> 4;
    unsigned char index = cec_global_info.my_node_index;
    if (cec_global_info.dev_mask & (1 << index)) {
        cec_global_info.cec_node_info[index].power_status = pcec_message->content.msg.operands[0];
        cec_global_info.cec_node_info[index].real_info_mask |= INFO_MASK_POWER_STATUS;
        hdmirx_cec_dbg_print("cec_report_power_status: %x\n", cec_global_info.cec_node_info[index].power_status);
    }
}

void cec_feature_abort(cec_rx_message_t* pcec_message)
{
    unsigned char index = cec_global_info.my_node_index;
    unsigned char opcode = pcec_message->content.msg.opcode;
    
    if(opcode != 0xf){
        unsigned char msg[4];
        
        msg[0] = ((index & 0xf) << 4) | CEC_TV_ADDR;
        msg[1] = CEC_OC_FEATURE_ABORT;
        msg[2] = opcode;
        msg[3] = CEC_UNRECONIZED_OPCODE;
        
        cec_ll_tx(msg, 4, NULL);        
    }
    
    hdmirx_cec_dbg_print("cec_feature_abort: opcode %x\n", pcec_message->content.msg.opcode);
}

void cec_report_version(cec_rx_message_t* pcec_message)
{
    //unsigned char log_addr = pcec_message->content.msg.header >> 4;
    unsigned char index = cec_global_info.my_node_index;   
    if (cec_global_info.dev_mask & (1 << index)) {
        cec_global_info.cec_node_info[index].cec_version = pcec_message->content.msg.operands[0];
        cec_global_info.cec_node_info[index].real_info_mask |= INFO_MASK_CEC_VERSION;
        hdmirx_cec_dbg_print("cec_report_version: %x\n", cec_global_info.cec_node_info[index].cec_version);
    }
}


void cec_report_physical_address_smp(void)
{
    unsigned char msg[5]; 
    unsigned char index = cec_global_info.my_node_index;
    unsigned char phy_addr_ab = (aml_read_reg32(P_AO_DEBUG_REG1) >> 8) & 0xff;
    unsigned char phy_addr_cd = aml_read_reg32(P_AO_DEBUG_REG1) & 0xff;    
    //hdmitx_cec_dbg_print("\nCEC:function:%s,file:%s,line:%d\n",__FUNCTION__,__FILE__,__LINE__);     
    
    msg[0] = ((index & 0xf) << 4) | CEC_BROADCAST_ADDR;
    msg[1] = CEC_OC_REPORT_PHYSICAL_ADDRESS;
    msg[2] = phy_addr_ab;
    msg[3] = phy_addr_cd;
    //msg[2] = cec_global_info.cec_node_info[index].phy_addr.phy_addr_2.ab;
    //msg[3] = cec_global_info.cec_node_info[index].phy_addr.phy_addr_2.cd;
    msg[4] = cec_global_info.cec_node_info[index].dev_type;                        
    
    cec_ll_tx(msg, 5, NULL);
        
}

void cec_imageview_on_smp(void)
{
    unsigned char msg[2];
    unsigned char index = cec_global_info.my_node_index;

    //hdmitx_cec_dbg_print("\nCEC:function:%s,file:%s,line:%d\n",__FUNCTION__,__FILE__,__LINE__);   
    printk("CEC:hdmi_cec_func_config:0x%x\n",hdmi_cec_func_config);  
    if((hdmi_cec_func_config >> CEC_FUNC_MSAK) & 0x1){
        if((hdmi_cec_func_config >> ONE_TOUCH_PLAY_MASK) & 0x1)
        {
            msg[0] = ((index & 0xf) << 4) | CEC_TV_ADDR;
            msg[1] = CEC_OC_IMAGE_VIEW_ON;
            cec_ll_tx(msg, 2, NULL);
        }
    }  
}

void cec_get_menu_language_smp(void)
{
    unsigned char msg[2];
    unsigned char index = cec_global_info.my_node_index;
    
    //hdmitx_cec_dbg_print("\nCEC:function:%s,file:%s,line:%d\n",__FUNCTION__,__FILE__,__LINE__);    

    msg[0] = ((index & 0xf) << 4) | CEC_TV_ADDR;
    msg[1] = CEC_OC_GET_MENU_LANGUAGE;
    
    cec_ll_tx(msg, 2, NULL);
    
}

void cec_menu_status(cec_rx_message_t* pcec_message)
{
    unsigned char msg[3];
    unsigned char index = cec_global_info.my_node_index;
    
    //hdmitx_cec_dbg_print("\nCEC:function:%s,file:%s,line:%d\n",__FUNCTION__,__FILE__,__LINE__);    
    
    //if((2 == pcec_message->content.msg.operands[0]) || (0 == pcec_message->content.msg.operands[0]) ){    
    //    msg[0] = ((index & 0xf) << 4) | CEC_TV_ADDR;
    //    msg[1] = CEC_OC_MENU_STATUS;
    //    msg[2] = DEVICE_MENU_ACTIVE;
    //}else{
        msg[0] = ((index & 0xf) << 4) | CEC_TV_ADDR;
        msg[1] = CEC_OC_MENU_STATUS;
        msg[2] = DEVICE_MENU_INACTIVE;        
    //}
    cec_ll_tx(msg, 3, NULL);
}

void cec_menu_status_smp(void)
{
    unsigned char msg[3];
    unsigned char index = cec_global_info.my_node_index;
    
    //hdmitx_cec_dbg_print("\nCEC:function:%s,file:%s,line:%d\n",__FUNCTION__,__FILE__,__LINE__);    
    
    //if((2 == pcec_message->content.msg.operands[0]) || (0 == pcec_message->content.msg.operands[0]) ){    
        msg[0] = ((index & 0xf) << 4) | CEC_TV_ADDR;
        msg[1] = CEC_OC_MENU_STATUS;
        msg[2] = DEVICE_MENU_ACTIVE;
    //}else{
    //    msg[0] = ((index & 0xf) << 4) | CEC_TV_ADDR;
    //    msg[1] = CEC_OC_MENU_STATUS;
    //    msg[2] = DEVICE_MENU_INACTIVE;        
    //}
    cec_ll_tx(msg, 3, NULL);
    
    //MSG_P1( index, CEC_TV_ADDR,
    //        CEC_OC_MENU_STATUS, 
    //        DEVICE_MENU_ACTIVE
    //        );

    //register_cec_tx_msg(gbl_msg, 3); 

}

void cec_device_vendor_id_smp(void)
{
    unsigned char msg[3];
    unsigned char index = cec_global_info.my_node_index;
    
    //hdmitx_cec_dbg_print("\nCEC:function:%s,file:%s,line:%d\n",__FUNCTION__,__FILE__,__LINE__);    

    msg[0] = ((index & 0xf) << 4) | CEC_BROADCAST_ADDR;
    msg[1] = CEC_OC_DEVICE_VENDOR_ID;
    msg[2] = cec_global_info.cec_node_info[index].vendor_id;

    cec_ll_tx(msg, 3, NULL);

}

void cec_active_source_smp(void)
{
    unsigned char msg[4];
    unsigned char index = cec_global_info.my_node_index;
    unsigned char phy_addr_ab = (aml_read_reg32(P_AO_DEBUG_REG1) >> 8) & 0xff;
    unsigned char phy_addr_cd = aml_read_reg32(P_AO_DEBUG_REG1) & 0xff;      
    //hdmitx_cec_dbg_print("\nCEC:function:%s,file:%s,line:%d\n",__FUNCTION__,__FILE__,__LINE__);    

    if((hdmi_cec_func_config >> CEC_FUNC_MSAK) & 0x1){    
        if((hdmi_cec_func_config >> ONE_TOUCH_PLAY_MASK) & 0x1)
        {    
            msg[0] = ((index & 0xf) << 4) | CEC_BROADCAST_ADDR;
            msg[1] = CEC_OC_ACTIVE_SOURCE;
            msg[2] = phy_addr_ab;
            msg[3] = phy_addr_cd;
            //msg[2] = cec_global_info.cec_node_info[index].phy_addr.phy_addr_2.ab;
            //msg[3] = cec_global_info.cec_node_info[index].phy_addr.phy_addr_2.cd;
            cec_ll_tx(msg, 4, NULL);
        }
    }
}
void cec_active_source(cec_rx_message_t* pcec_message)
{
    unsigned char msg[4];
    //unsigned char log_addr = pcec_message->content.msg.header >> 4;
    unsigned char index = cec_global_info.my_node_index;
    //unsigned short phy_addr = (pcec_message->content.msg.operands[0] << 8) | pcec_message->content.msg.operands[1];
    unsigned char phy_addr_ab = (aml_read_reg32(P_AO_DEBUG_REG1) >> 8) & 0xff;
    unsigned char phy_addr_cd = aml_read_reg32(P_AO_DEBUG_REG1) & 0xff;    

    //if (cec_global_info.dev_mask & (1 << log_addr)) {
//    if (phy_addr == cec_global_info.cec_node_info[index].phy_addr.phy_addr_4) {

        msg[0] = ((index & 0xf) << 4) | CEC_BROADCAST_ADDR;
        msg[1] = CEC_OC_ACTIVE_SOURCE;
        msg[2] = phy_addr_ab;
        msg[3] = phy_addr_cd;
        //msg[2] = cec_global_info.cec_node_info[index].phy_addr.phy_addr_2.ab;
        //msg[3] = cec_global_info.cec_node_info[index].phy_addr.phy_addr_2.cd;
        cec_ll_tx(msg, 4, NULL);
        
//        MSG_P2( index, CEC_TV_ADDR, 
//                CEC_OC_ACTIVE_SOURCE, 
//                cec_global_info.cec_node_info[index].phy_addr.phy_addr_2.ab,
//                cec_global_info.cec_node_info[index].phy_addr.phy_addr_2.cd);
        
//        register_cec_tx_msg(gbl_msg, 4);         
//    }else{
//        cec_deactive_source(pcec_message);    	
//    }
}

//////////////////////////////////
void cec_set_stream_path(cec_rx_message_t* pcec_message)
{
//    unsigned char log_addr = pcec_message->content.msg.header >> 4;
    unsigned char index = cec_global_info.my_node_index;
    unsigned short phy_addr = (pcec_message->content.msg.operands[0] << 8) | pcec_message->content.msg.operands[1];

    //hdmitx_cec_dbg_print("\n-------function:%s,file:%s,line:%d--phy_addr:%u---\n",__FUNCTION__,__FILE__,__LINE__,phy_addr);
    //hdmitx_cec_dbg_print("\n-------function:%s,file:%s,line:%d--cec_global_info.cec_node_info[index].phy_addr.phy_addr_4:%u---\n",__FUNCTION__,__FILE__,__LINE__,cec_global_info.cec_node_info[index].phy_addr.phy_addr_4);
        
    //if (cec_global_info.dev_mask & (1 << log_addr)) {
    if((hdmi_cec_func_config >> CEC_FUNC_MSAK) & 0x1){    
        if((hdmi_cec_func_config >> AUTO_POWER_ON_MASK) & 0x1)
        {    
            if (phy_addr == cec_global_info.cec_node_info[index].phy_addr.phy_addr_4) {    
                unsigned char msg[4];
                msg[0] = ((index & 0xf) << 4) | CEC_BROADCAST_ADDR;
                msg[1] = CEC_OC_ACTIVE_SOURCE;
                msg[2] = cec_global_info.cec_node_info[index].phy_addr.phy_addr_2.ab;
                msg[3] = cec_global_info.cec_node_info[index].phy_addr.phy_addr_2.cd;
                cec_ll_tx(msg, 4, NULL);
            }
        }
    }
}
void cec_set_system_audio_mode(void)
{
    unsigned char index = cec_global_info.my_node_index;

    MSG_P1( index, CEC_TV_ADDR,
    //MSG_P1( index, CEC_BROADCAST_ADDR,  
            CEC_OC_SET_SYSTEM_AUDIO_MODE, 
            cec_global_info.cec_node_info[index].specific_info.audio.sys_audio_mode
            );
    
    register_cec_tx_msg(gbl_msg, 3);
    if(cec_global_info.cec_node_info[index].specific_info.audio.sys_audio_mode == ON)
        cec_global_info.cec_node_info[index].specific_info.audio.sys_audio_mode = OFF;
    else
        cec_global_info.cec_node_info[index].specific_info.audio.sys_audio_mode = ON;    	      
}

void cec_system_audio_mode_request(void)
{
    unsigned char index = cec_global_info.my_node_index;
    if(cec_global_info.cec_node_info[index].specific_info.audio.sys_audio_mode == OFF){
        MSG_P2( index, CEC_AUDIO_SYSTEM_ADDR,//CEC_TV_ADDR, 
                CEC_OC_SYSTEM_AUDIO_MODE_REQUEST, 
                cec_global_info.cec_node_info[index].phy_addr.phy_addr_2.ab,
                cec_global_info.cec_node_info[index].phy_addr.phy_addr_2.cd
                );
        register_cec_tx_msg(gbl_msg, 4);    	
        cec_global_info.cec_node_info[index].specific_info.audio.sys_audio_mode = ON;
    }
    else{        
        MSG_P0( index, CEC_AUDIO_SYSTEM_ADDR,//CEC_TV_ADDR, 
                CEC_OC_SYSTEM_AUDIO_MODE_REQUEST 
                ); 
        register_cec_tx_msg(gbl_msg, 2);    	
        cec_global_info.cec_node_info[index].specific_info.audio.sys_audio_mode = OFF; 
    }   	      
}

void cec_report_audio_status(void)
{
    unsigned char index = cec_global_info.my_node_index;

    MSG_P1( index, CEC_TV_ADDR,
    //MSG_P1( index, CEC_BROADCAST_ADDR,  
            CEC_OC_REPORT_AUDIO_STATUS, 
            cec_global_info.cec_node_info[index].specific_info.audio.audio_status.audio_mute_status | \
            cec_global_info.cec_node_info[index].specific_info.audio.audio_status.audio_volume_status
            );

    register_cec_tx_msg(gbl_msg, 3);   	      
}
void cec_request_active_source(cec_rx_message_t* pcec_message)
{
    cec_set_stream_path(pcec_message);
}

void cec_give_device_power_status(cec_rx_message_t* pcec_message)
{
    unsigned char log_addr = pcec_message->content.msg.header >> 4;
    unsigned char index = cec_global_info.my_node_index;

    //if (cec_global_info.dev_mask & (1 << log_addr)) {
        unsigned char msg[3];
        msg[0] = ((index & 0xf) << 4) | log_addr;
        msg[1] = CEC_OC_REPORT_POWER_STATUS;
        msg[2] = cec_global_info.cec_node_info[index].power_status;
        cec_ll_tx(msg, 3, NULL);
    //}
}

void cec_inactive_source(void)
{
    unsigned char index = cec_global_info.my_node_index;
    unsigned char msg[4];
    
    msg[0] = ((index & 0xf) << 4) | CEC_TV_ADDR;
    msg[1] = CEC_OC_INACTIVE_SOURCE;
	msg[2] = cec_global_info.cec_node_info[index].phy_addr.phy_addr_2.ab;
	msg[3] = cec_global_info.cec_node_info[index].phy_addr.phy_addr_2.cd;

    cec_ll_tx_irq(msg, 4);    
}
EXPORT_SYMBOL(cec_inactive_source);

void cec_deactive_source(cec_rx_message_t* pcec_message)
{
    //unsigned char log_addr = pcec_message->content.msg.header >> 4;
    unsigned char index = cec_global_info.my_node_index;    
    
    //if (cec_global_info.dev_mask & (1 << log_addr)) {
    //    if (cec_global_info.active_log_dev == log_addr) {
    //    cec_global_info.active_log_dev = 0;
    //    }
    //    hdmirx_cec_dbg_print("cec_deactive_source: %x\n", log_addr);
    //}
    
    MSG_P2( index, CEC_TV_ADDR, 
            CEC_OC_INACTIVE_SOURCE, 
            cec_global_info.cec_node_info[index].phy_addr.phy_addr_2.ab,
            cec_global_info.cec_node_info[index].phy_addr.phy_addr_2.cd);

    register_cec_tx_msg(gbl_msg, 4); 
}

void cec_get_version(cec_rx_message_t* pcec_message)
{
    unsigned char log_addr = pcec_message->content.msg.header >> 4;
    //if (cec_global_info.dev_mask & (1 << log_addr)) {
        unsigned char msg[3];
        msg[0] = log_addr;
        msg[1] = CEC_OC_CEC_VERSION;
        msg[2] = CEC_VERSION_13A;
        cec_ll_tx(msg, 3, NULL);
    //}
}

void cec_give_deck_status(cec_rx_message_t* pcec_message)
{
    unsigned char index = cec_global_info.my_node_index; 
    MSG_P1( index, CEC_TV_ADDR, 
            CEC_OC_DECK_STATUS, 
            0x1a);

    register_cec_tx_msg(gbl_msg, 3); 
}


void cec_deck_status(cec_rx_message_t* pcec_message)
{
//    unsigned char log_addr = pcec_message->content.msg.header >> 4;
    unsigned char index = cec_global_info.my_node_index; 
        
    if (cec_global_info.dev_mask & (1 << index)) {
        cec_global_info.cec_node_info[index].specific_info.playback.deck_info = pcec_message->content.msg.operands[0];
        cec_global_info.cec_node_info[index].real_info_mask |= INFO_MASK_DECK_INfO;
        hdmirx_cec_dbg_print("cec_deck_status: %x\n", cec_global_info.cec_node_info[index].specific_info.playback.deck_info);
    }
}
void cec_set_standby(void)
{
    unsigned char index = cec_global_info.my_node_index;
    unsigned char msg[9];

    msg[0] = ((index & 0xf) << 4) | CEC_BROADCAST_ADDR;
    msg[1] = CEC_OC_STANDBY;

    cec_ll_tx_irq(msg, 2);
}
EXPORT_SYMBOL(cec_set_standby);

void cec_set_osd_name(cec_rx_message_t* pcec_message)
{
    unsigned char index = cec_global_info.my_node_index;
    unsigned char msg[9];
    
    //"PHILIPS HMP8100"
    msg[0] = ((index & 0xf) << 4) | CEC_TV_ADDR;
    msg[1] = CEC_OC_SET_OSD_NAME;
    memcpy(&msg[2], "HMP8100", strlen("HMP8100"));

    cec_ll_tx(msg, 9, NULL);
}
void cec_vendor_cmd_with_id(cec_rx_message_t* pcec_message)
{
//    unsigned char log_addr = pcec_message->content.msg.header >> 4;
//    unsigned char index = cec_global_info.my_node_index;  
//    if (cec_global_info.dev_mask & (1 << index)) {
//        if (cec_global_info.cec_node_info[index].vendor_id.vendor_id_byte_num != 0) {
//            int i = cec_global_info.cec_node_info[index].vendor_id.vendor_id_byte_num;
//            int tmp = 0;
//            for ( ; i < pcec_message->operand_num; i++) {
//                tmp |= (pcec_message->content.msg.operands[i] << ((cec_global_info.cec_node_info[log_addr].vendor_id.vendor_id_byte_num - i - 1)*8));
//            }
//            hdmirx_cec_dbg_print("cec_vendor_cmd_with_id: %lx, %x\n", cec_global_info.cec_node_info[log_addr].vendor_id.vendor_id, tmp);
//        }
//    }
}


void cec_set_menu_language(cec_rx_message_t* pcec_message)
{
//    unsigned char log_addr = pcec_message->content.msg.header >> 4;
    unsigned char index = cec_global_info.my_node_index;
    //if (cec_global_info.dev_mask & (1 << index)) {
        //if (pcec_message->operand_num == 3) {
            int i;
            unsigned int tmp = ((pcec_message->content.msg.operands[0] << 16)  |
                                (pcec_message->content.msg.operands[1] << 8) |
                                (pcec_message->content.msg.operands[2]));

            hdmirx_cec_dbg_print("%c, %c, %c\n", pcec_message->content.msg.operands[0],
                                 pcec_message->content.msg.operands[1],
                                 pcec_message->content.msg.operands[2]);

            for (i = 0; i < (sizeof(menu_lang_array)/sizeof(menu_lang_array[0])); i++) {
                if (menu_lang_array[i] == tmp)
                    break;
            }

            cec_global_info.cec_node_info[index].menu_lang = i;
			switch_set_state(&lang_dev, cec_global_info.cec_node_info[index].menu_lang);
			
            cec_global_info.cec_node_info[index].real_info_mask |= INFO_MASK_MENU_LANGUAGE;

            hdmirx_cec_dbg_print("cec_set_menu_language: %x\n", cec_global_info.cec_node_info[index].menu_lang);
        //}
    //}
}

void cec_handle_message(cec_rx_message_t* pcec_message)
{
    unsigned char	brdcst, opcode;
    unsigned char	initiator, follower;
    unsigned char   operand_num;
    unsigned char   msg_length;

    /* parse message */
    if ((!pcec_message) || (check_cec_msg_valid(pcec_message) == 0)) return;

    initiator	= pcec_message->content.msg.header >> 4;
    follower	= pcec_message->content.msg.header & 0x0f;
    opcode		= pcec_message->content.msg.opcode;
    operand_num = pcec_message->operand_num;
    brdcst      = (follower == 0x0f);
    msg_length  = pcec_message->msg_length;

    
    /* process messages from tv polling and cec devices */
    printk("OP code: 0x%x\n", opcode);
    if(((hdmi_cec_func_config>>CEC_FUNC_MSAK) & 0x1) && cec_power_flag)
    {    
        switch (opcode) {
        case CEC_OC_ACTIVE_SOURCE:
            //cec_active_source(pcec_message);
            //cec_deactive_source(pcec_message);
            break;
        case CEC_OC_INACTIVE_SOURCE:
            //cec_deactive_source(pcec_message);
            break;
        case CEC_OC_CEC_VERSION:
            cec_report_version(pcec_message);
            break;
        case CEC_OC_DECK_STATUS:
            cec_deck_status(pcec_message);
            break;
        case CEC_OC_DEVICE_VENDOR_ID:
            //cec_device_vendor_id(pcec_message);
            break;
        case CEC_OC_FEATURE_ABORT:
            //cec_feature_abort(pcec_message);
            break;
        case CEC_OC_GET_CEC_VERSION:
            cec_get_version(pcec_message);
            break;
        case CEC_OC_GIVE_DECK_STATUS:
            cec_give_deck_status(pcec_message);
            break;
        case CEC_OC_MENU_STATUS:
            //cec_menu_status(pcec_message);
            break;
        case CEC_OC_REPORT_PHYSICAL_ADDRESS:
            cec_report_phy_addr(pcec_message);
            break;
        case CEC_OC_REPORT_POWER_STATUS:
            cec_report_power_status(pcec_message);
            break;
        case CEC_OC_SET_OSD_NAME:
            //cec_set_osd_name(pcec_message);
            break;
        case CEC_OC_VENDOR_COMMAND_WITH_ID:
            //cec_feature_abort(pcec_message);
            //cec_vendor_cmd_with_id(pcec_message);
            break;
        case CEC_OC_SET_MENU_LANGUAGE:
            cec_set_menu_language(pcec_message);
            break;
        case CEC_OC_GIVE_PHYSICAL_ADDRESS:
            //cec_report_phy_addr(pcec_message);//
            //cec_give_physical_address(pcec_message);
            cec_usrcmd_set_report_physical_address();
            break;
        case CEC_OC_GIVE_DEVICE_VENDOR_ID:
            cec_feature_abort(pcec_message);
            //cec_device_vendor_id(pcec_message);
            //cec_usrcmd_set_device_vendor_id();
            break;
        case CEC_OC_GIVE_OSD_NAME:
            cec_set_osd_name(pcec_message);
            //cec_give_osd_name(pcec_message);
            //cec_usrcmd_set_osd_name(pcec_message);
            break;
        case CEC_OC_STANDBY:
        	//printk("----cec_standby-----");
        	cec_menu_status(pcec_message);
        	cec_deactive_source(pcec_message);
            cec_standby(pcec_message);
            break;
        case CEC_OC_SET_STREAM_PATH:
            cec_set_stream_path(pcec_message);
            break;
        case CEC_OC_REQUEST_ACTIVE_SOURCE:
            //cec_request_active_source(pcec_message);
            cec_usrcmd_set_active_source();
            break;
        case CEC_OC_GIVE_DEVICE_POWER_STATUS:
            cec_give_device_power_status(pcec_message);
            break;
        case CEC_OC_USER_CONTROL_PRESSED:
            //printk("----cec_user_control_pressed-----");
            //cec_user_control_pressed(pcec_message);
            break;
        case CEC_OC_USER_CONTROL_RELEASED:
            //printk("----cec_user_control_released----");
            //cec_user_control_released(pcec_message);
            break; 
        case CEC_OC_IMAGE_VIEW_ON:      //not support in source
            cec_usrcmd_set_imageview_on( CEC_TV_ADDR );   // Wakeup TV
            break;  
        case CEC_OC_ROUTING_CHANGE:
        case CEC_OC_ROUTING_INFORMATION:    	
        	//cec_usrcmd_routing_information(pcec_message);	
        	break;
        case CEC_OC_GIVE_AUDIO_STATUS:   	  
        	cec_report_audio_status();
        	break;
        case CEC_OC_MENU_REQUEST:
            cec_menu_status_smp();
            break;
        case CEC_OC_PLAY:
            printk("CEC_OC_PLAY:0x%x\n",pcec_message->content.msg.operands[0]);        
            switch(pcec_message->content.msg.operands[0]){
                case 0x24:
                    input_event(remote_cec_dev, EV_KEY, KEY_PLAYPAUSE, 1);
                    input_sync(remote_cec_dev);	
                    input_event(remote_cec_dev, EV_KEY, KEY_PLAYPAUSE, 0);
                    input_sync(remote_cec_dev);
                    break;
                case 0x25:
                    input_event(remote_cec_dev, EV_KEY, KEY_PLAYPAUSE, 1);
                    input_sync(remote_cec_dev);	
                    input_event(remote_cec_dev, EV_KEY, KEY_PLAYPAUSE, 0);
                    input_sync(remote_cec_dev);
                    break;
                default:
                    break;                
            }
            break;
        case CEC_OC_DECK_CONTROL:
            printk("CEC_OC_DECK_CONTROL:0x%x\n",pcec_message->content.msg.operands[0]);        
            switch(pcec_message->content.msg.operands[0]){
                case 0x3:
                    input_event(remote_cec_dev, EV_KEY, KEY_STOP, 1);
                    input_sync(remote_cec_dev);	
                    input_event(remote_cec_dev, EV_KEY, KEY_STOP, 0);
                    input_sync(remote_cec_dev);
                    break;
                default:
                    break;                
            }
            break;
        case CEC_OC_GET_MENU_LANGUAGE:
            //cec_set_menu_language(pcec_message);
            //break;                 	  
        case CEC_OC_VENDOR_REMOTE_BUTTON_DOWN:
        case CEC_OC_VENDOR_REMOTE_BUTTON_UP:
        case CEC_OC_CLEAR_ANALOGUE_TIMER:
        case CEC_OC_CLEAR_DIGITAL_TIMER:
        case CEC_OC_CLEAR_EXTERNAL_TIMER:
        case CEC_OC_GIVE_SYSTEM_AUDIO_MODE_STATUS:
        case CEC_OC_GIVE_TUNER_DEVICE_STATUS:
        case CEC_OC_SET_OSD_STRING:
        case CEC_OC_SET_SYSTEM_AUDIO_MODE:
        case CEC_OC_SET_TIMER_PROGRAM_TITLE:
        case CEC_OC_SYSTEM_AUDIO_MODE_REQUEST:
        case CEC_OC_SYSTEM_AUDIO_MODE_STATUS:
        case CEC_OC_TEXT_VIEW_ON:       //not support in source
        case CEC_OC_TIMER_CLEARED_STATUS:
        case CEC_OC_TIMER_STATUS:
        case CEC_OC_TUNER_DEVICE_STATUS:
        case CEC_OC_TUNER_STEP_DECREMENT:
        case CEC_OC_TUNER_STEP_INCREMENT:
        case CEC_OC_VENDOR_COMMAND:
        case CEC_OC_SELECT_ANALOGUE_SERVICE:
        case CEC_OC_SELECT_DIGITAL_SERVICE:
        case CEC_OC_SET_ANALOGUE_TIMER :
        case CEC_OC_SET_AUDIO_RATE:
        case CEC_OC_SET_DIGITAL_TIMER:
        case CEC_OC_SET_EXTERNAL_TIMER:
        case CEC_OC_RECORD_OFF:
        case CEC_OC_RECORD_ON:
        case CEC_OC_RECORD_STATUS:
        case CEC_OC_RECORD_TV_SCREEN:
        case CEC_OC_REPORT_AUDIO_STATUS:
        case CEC_OC_ABORT_MESSAGE:
            cec_feature_abort(pcec_message);
            break;
        default:
            break;
        }
    }
}


// --------------- cec command from user application --------------------

void cec_usrcmd_parse_all_dev_online(void)
{
    int i;
    unsigned short tmp_mask;

    hdmirx_cec_dbg_print("cec online: ###############################################\n");
    hdmirx_cec_dbg_print("active_log_dev %x\n", cec_global_info.active_log_dev);
    for (i = 0; i < MAX_NUM_OF_DEV; i++) {
        tmp_mask = 1 << i;
        if (tmp_mask & cec_global_info.dev_mask) {
            hdmirx_cec_dbg_print("cec online: -------------------------------------------\n");
            hdmirx_cec_dbg_print("hdmi_port:     %x\n", cec_global_info.cec_node_info[i].hdmi_port);
            hdmirx_cec_dbg_print("dev_type:      %x\n", cec_global_info.cec_node_info[i].dev_type);
            hdmirx_cec_dbg_print("power_status:  %x\n", cec_global_info.cec_node_info[i].power_status);
            hdmirx_cec_dbg_print("cec_version:   %x\n", cec_global_info.cec_node_info[i].cec_version);
            hdmirx_cec_dbg_print("vendor_id:     %x\n", cec_global_info.cec_node_info[i].vendor_id);
            hdmirx_cec_dbg_print("phy_addr:      %x\n", cec_global_info.cec_node_info[i].phy_addr.phy_addr_4);
            hdmirx_cec_dbg_print("log_addr:      %x\n", cec_global_info.cec_node_info[i].log_addr);
            hdmirx_cec_dbg_print("osd_name:      %s\n", cec_global_info.cec_node_info[i].osd_name);
            hdmirx_cec_dbg_print("osd_name_def:  %s\n", cec_global_info.cec_node_info[i].osd_name_def);
            hdmirx_cec_dbg_print("menu_state:    %x\n", cec_global_info.cec_node_info[i].menu_state);

            if (cec_global_info.cec_node_info[i].dev_type == CEC_PLAYBACK_DEVICE_TYPE) {
                hdmirx_cec_dbg_print("deck_cnt_mode: %x\n", cec_global_info.cec_node_info[i].specific_info.playback.deck_cnt_mode);
                hdmirx_cec_dbg_print("deck_info:     %x\n", cec_global_info.cec_node_info[i].specific_info.playback.deck_info);
                hdmirx_cec_dbg_print("play_mode:     %x\n", cec_global_info.cec_node_info[i].specific_info.playback.play_mode);
            }
        }
    }
    hdmirx_cec_dbg_print("##############################################################\n");
}

//////////////////////////////////////////////////
void cec_usrcmd_get_cec_version(unsigned char log_addr)
{
    MSG_P0(cec_global_info.my_node_index, log_addr, 
            CEC_OC_GET_CEC_VERSION);

    register_cec_tx_msg(gbl_msg, 2);
}

void cec_usrcmd_get_audio_status(unsigned char log_addr)
{
    MSG_P0(cec_global_info.my_node_index, log_addr, CEC_OC_GIVE_AUDIO_STATUS);

    register_cec_tx_msg(gbl_msg, 2);
}

void cec_usrcmd_get_deck_status(unsigned char log_addr)
{
    MSG_P1(cec_global_info.my_node_index, log_addr, CEC_OC_GIVE_DECK_STATUS, STATUS_REQ_ON);

    register_cec_tx_msg(gbl_msg, 3);
}

void cec_usrcmd_set_deck_cnt_mode(unsigned char log_addr, deck_cnt_mode_e deck_cnt_mode)
{
    MSG_P1(cec_global_info.my_node_index, log_addr, CEC_OC_DECK_CONTROL, deck_cnt_mode);

    register_cec_tx_msg(gbl_msg, 3);
}

void cec_usrcmd_get_device_power_status(unsigned char log_addr)
{
    MSG_P0(cec_global_info.my_node_index, log_addr, CEC_OC_GIVE_DEVICE_POWER_STATUS);

    register_cec_tx_msg(gbl_msg, 2);
}

void cec_usrcmd_get_device_vendor_id(unsigned char log_addr)
{
    MSG_P0(cec_global_info.my_node_index, log_addr, CEC_OC_GIVE_DEVICE_VENDOR_ID);

    register_cec_tx_msg(gbl_msg, 2);
}

void cec_usrcmd_get_osd_name(unsigned char log_addr)
{
    MSG_P0(cec_global_info.my_node_index, log_addr, CEC_OC_GIVE_OSD_NAME);

    register_cec_tx_msg(gbl_msg, 2);
}

void cec_usrcmd_get_physical_address(unsigned char log_addr)
{
    MSG_P0(cec_global_info.my_node_index, log_addr, CEC_OC_GIVE_PHYSICAL_ADDRESS);

    register_cec_tx_msg(gbl_msg, 2);
}

void cec_usrcmd_get_system_audio_mode_status(unsigned char log_addr)
{
    MSG_P0(cec_global_info.my_node_index, log_addr, CEC_OC_GIVE_SYSTEM_AUDIO_MODE_STATUS);

    register_cec_tx_msg(gbl_msg, 2);
}

void cec_usrcmd_set_standby(unsigned char log_addr)
{
    MSG_P0(cec_global_info.my_node_index, log_addr, CEC_OC_STANDBY);

    register_cec_tx_msg(gbl_msg, 2);
}

/////////////////////////
void cec_usrcmd_set_imageview_on(unsigned char log_addr)
{
    MSG_P0(cec_global_info.my_node_index, log_addr, 
            CEC_OC_IMAGE_VIEW_ON);

    register_cec_tx_msg(gbl_msg, 2);
}

void cec_usrcmd_text_view_on(unsigned char log_addr)
{
    MSG_P0(cec_global_info.my_node_index, log_addr, 
            CEC_OC_TEXT_VIEW_ON);

    register_cec_tx_msg(gbl_msg, 2);
}

void cec_usrcmd_get_tuner_device_status(unsigned char log_addr)
{
    MSG_P0(cec_global_info.my_node_index, log_addr, CEC_OC_GIVE_TUNER_DEVICE_STATUS);

    register_cec_tx_msg(gbl_msg, 2);
}

void cec_usrcmd_set_play_mode(unsigned char log_addr, play_mode_e play_mode)
{
    MSG_P1(cec_global_info.my_node_index, log_addr, CEC_OC_PLAY, play_mode);

    register_cec_tx_msg(gbl_msg, 3);
}

void cec_usrcmd_get_menu_state(unsigned char log_addr)
{
    MSG_P1(cec_global_info.my_node_index, log_addr, CEC_OC_MENU_REQUEST, MENU_REQ_QUERY);

    register_cec_tx_msg(gbl_msg, 3);
}

void cec_usrcmd_set_menu_state(unsigned char log_addr, menu_req_type_e menu_req_type)
{
    MSG_P1(cec_global_info.my_node_index, log_addr, CEC_OC_MENU_REQUEST, menu_req_type);

    register_cec_tx_msg(gbl_msg, 3);
}

void cec_usrcmd_get_menu_language(unsigned char log_addr)
{
    MSG_P0(cec_global_info.my_node_index, log_addr, CEC_OC_GET_MENU_LANGUAGE);

    register_cec_tx_msg(gbl_msg, 2);
}

void cec_usrcmd_set_menu_language(unsigned char log_addr, cec_menu_lang_e menu_lang)
{
    MSG_P3(cec_global_info.my_node_index, log_addr, CEC_OC_SET_MENU_LANGUAGE, (menu_lang_array[menu_lang]>>16)&0xFF,
           (menu_lang_array[menu_lang]>>8)&0xFF,
           (menu_lang_array[menu_lang])&0xFF);
    register_cec_tx_msg(gbl_msg, 5);
}

void cec_usrcmd_get_active_source(void)
{
    MSG_P0(cec_global_info.my_node_index, 0xF, CEC_OC_REQUEST_ACTIVE_SOURCE);
        
    register_cec_tx_msg(gbl_msg, 2);
}

void cec_usrcmd_set_active_source(void)
{
    unsigned char index = cec_global_info.my_node_index;
    unsigned char phy_addr_ab = cec_global_info.cec_node_info[index].phy_addr.phy_addr_2.ab;
    unsigned char phy_addr_cd = cec_global_info.cec_node_info[index].phy_addr.phy_addr_2.cd;
    
    printk("phy_addr_ab:%x\n", phy_addr_ab);
    printk("phy_addr_cd:%x\n", phy_addr_cd);
    printk("Physical address: 0x%x\n",aml_read_reg32(P_AO_DEBUG_REG1));
    
    phy_addr_ab = (aml_read_reg32(P_AO_DEBUG_REG1) >> 8) & 0xff;
    phy_addr_cd = aml_read_reg32(P_AO_DEBUG_REG1) & 0xff;

    MSG_P2(index, CEC_BROADCAST_ADDR, 
            CEC_OC_ACTIVE_SOURCE,
			phy_addr_ab,
			phy_addr_cd);

    register_cec_tx_msg(gbl_msg, 4);
}

void cec_usrcmd_set_deactive_source(unsigned char log_addr)
{
    unsigned char phy_addr_ab = (aml_read_reg32(P_AO_DEBUG_REG1) >> 8) & 0xff;
    unsigned char phy_addr_cd = aml_read_reg32(P_AO_DEBUG_REG1) & 0xff;
    
    MSG_P2(cec_global_info.my_node_index, log_addr, CEC_OC_INACTIVE_SOURCE,
           phy_addr_ab,
           phy_addr_cd);
          //cec_global_info.cec_node_info[log_addr].phy_addr.phy_addr_2.ab,
          //cec_global_info.cec_node_info[log_addr].phy_addr.phy_addr_2.cd);

    register_cec_tx_msg(gbl_msg, 4);
}

void cec_usrcmd_clear_node_dev_real_info_mask(unsigned char log_addr, cec_info_mask mask)
{
    cec_global_info.cec_node_info[log_addr].real_info_mask &= ~mask;
}

//void cec_usrcmd_set_stream_path(unsigned char log_addr)
//{
//    MSG_P2(cec_global_info.my_node_index, log_addr, CEC_OC_SET_STREAM_PATH, 
//                                                  cec_global_info.cec_node_info[log_addr].phy_addr.phy_addr_2.ab,
//                                                  cec_global_info.cec_node_info[log_addr].phy_addr.phy_addr_2.cd);
//
//    register_cec_tx_msg(gbl_msg, 4);
//}

void cec_usrcmd_set_osd_name(cec_rx_message_t* pcec_message)
{

    unsigned char log_addr = pcec_message->content.msg.header >> 4 ;  
    unsigned char index = cec_global_info.my_node_index;

    MSG_P14(index, log_addr, 
            CEC_OC_SET_OSD_NAME, 
            cec_global_info.cec_node_info[index].osd_name[0],
            cec_global_info.cec_node_info[index].osd_name[1],
            cec_global_info.cec_node_info[index].osd_name[2],
            cec_global_info.cec_node_info[index].osd_name[3],
            cec_global_info.cec_node_info[index].osd_name[4],
            cec_global_info.cec_node_info[index].osd_name[5],           
            cec_global_info.cec_node_info[index].osd_name[6],
            cec_global_info.cec_node_info[index].osd_name[7],
            cec_global_info.cec_node_info[index].osd_name[8],
            cec_global_info.cec_node_info[index].osd_name[9],
            cec_global_info.cec_node_info[index].osd_name[10],
            cec_global_info.cec_node_info[index].osd_name[11],  
            cec_global_info.cec_node_info[index].osd_name[12],
            cec_global_info.cec_node_info[index].osd_name[13]);

    register_cec_tx_msg(gbl_msg, 16);
}



void cec_usrcmd_set_device_vendor_id(void)
{
    unsigned char index = cec_global_info.my_node_index;

    MSG_P3(index, CEC_BROADCAST_ADDR, 
            CEC_OC_DEVICE_VENDOR_ID, 
            (cec_global_info.cec_node_info[index].vendor_id >> 16) & 0xff,
            (cec_global_info.cec_node_info[index].vendor_id >> 8) & 0xff,
            (cec_global_info.cec_node_info[index].vendor_id >> 0) & 0xff);

    register_cec_tx_msg(gbl_msg, 5);
}
void cec_usrcmd_set_report_physical_address(void)
{
    unsigned char index = cec_global_info.my_node_index;
    unsigned char phy_addr_ab = (aml_read_reg32(P_AO_DEBUG_REG1) >> 8) & 0xff;
    unsigned char phy_addr_cd = aml_read_reg32(P_AO_DEBUG_REG1) & 0xff;
    
    MSG_P3(index, CEC_BROADCAST_ADDR, 
           CEC_OC_REPORT_PHYSICAL_ADDRESS,
           phy_addr_ab,
           phy_addr_cd,
           CEC_PLAYBACK_DEVICE_TYPE);
			//cec_global_info.cec_node_info[index].phy_addr.phy_addr_2.ab,
			//cec_global_info.cec_node_info[index].phy_addr.phy_addr_2.cd,
			//cec_global_info.cec_node_info[index].dev_type);

    register_cec_tx_msg(gbl_msg, 5);
}

void cec_usrcmd_routing_change(cec_rx_message_t* pcec_message)
{
    //unsigned char index = cec_global_info.my_node_index;
    //unsigned char log_addr = pcec_message->content.msg.header >> 4 ;
    //cec_global_info.cec_node_info[index].log_addr = index;
    //cec_global_info.cec_node_info[index].real_info_mask |= INFO_MASK_LOGIC_ADDRESS;
    //cec_global_info.cec_node_info[index].phy_addr.phy_addr_4 = (pcec_message->content.msg.operands[2] << 8) | pcec_message->content.msg.operands[3];
    //cec_global_info.cec_node_info[index].real_info_mask |= INFO_MASK_PHYSICAL_ADDRESS;    
    //MSG_P4(index, CEC_BROADCAST_ADDR, 
    //        CEC_OC_ROUTING_CHANGE, 
        //  cec_global_info.cec_node_info[original_index].phy_addr.phy_addr_2.ab,
        //  cec_global_info.cec_node_info[original_index].phy_addr.phy_addr_2.cd,
        //  cec_global_info.cec_node_info[new_index].phy_addr.phy_addr_2.ab,
        //  cec_global_info.cec_node_info[new_index].phy_addr.phy_addr_2.cd,
        //  );

    //register_cec_tx_msg(gbl_msg, 6);
}

void cec_usrcmd_routing_information(cec_rx_message_t* pcec_message)
{
    unsigned char msg[4];
    unsigned char index = cec_global_info.my_node_index;
    //unsigned char log_addr = pcec_message->content.msg.header >> 4 ;
    
    //unsigned char phy_addr_ab = pcec_message->content.msg.operands[2];
    //unsigned char phy_addr_cd = pcec_message->content.msg.operands[3];
    unsigned char phy_addr_ab = cec_global_info.cec_node_info[index].phy_addr.phy_addr_2.ab;
    unsigned char phy_addr_cd = cec_global_info.cec_node_info[index].phy_addr.phy_addr_2.cd;
    printk("phy_addr_ab:%x\n", phy_addr_ab);
    printk("phy_addr_cd:%x\n", phy_addr_cd);
    printk("Physical address: 0x%x\n",aml_read_reg32(P_AO_DEBUG_REG1));
    //MSG_P2( index, CEC_BROADCAST_ADDR, 
    //        CEC_OC_ROUTING_INFORMATION, 
    //                    phy_addr_ab,
    //                    phy_addr_cd );
    //
    //register_cec_tx_msg(gbl_msg, 4);


    msg[0] = ((index & 0xf) << 4) | CEC_BROADCAST_ADDR;
    msg[1] = CEC_OC_ROUTING_INFORMATION;
    msg[2] = phy_addr_ab;
    msg[2] = phy_addr_cd;
    cec_ll_tx(msg, 4, NULL);

}
/***************************** cec middle level code end *****************************/


/***************************** cec high level code *****************************/

void cec_init(hdmitx_dev_t* hdmitx_device)
{
    int i;    
    if(!((hdmi_cec_func_config>>CEC_FUNC_MSAK) & 0x1)){
        printk("CEC not init\n");
        return ;
    }
    printk("CEC init\n");    
    hdmi_wr_reg(CEC0_BASE_ADDR+CEC_CLOCK_DIV_H, 0x00 );
    hdmi_wr_reg(CEC0_BASE_ADDR+CEC_CLOCK_DIV_L, 0xf0 );


// ?????
//    if (cec_init_flag == 1) return;

    cec_rx_msg_buf.rx_write_pos = 0;
    cec_rx_msg_buf.rx_read_pos = 0;
    cec_rx_msg_buf.rx_buf_size = sizeof(cec_rx_msg_buf.cec_rx_message)/sizeof(cec_rx_msg_buf.cec_rx_message[0]);
    memset(cec_rx_msg_buf.cec_rx_message, 0, sizeof(cec_rx_msg_buf.cec_rx_message));

    memset(&cec_global_info, 0, sizeof(cec_global_info_t));
    //cec_global_info.my_node_index = CEC0_LOG_ADDR;

    if (cec_mutex_flag == 0) {
        //init_MUTEX(&tv_cec_sema);
        sema_init(&tv_cec_sema,1);
        cec_mutex_flag = 1;
    }
    
    kthread_run(cec_task, (void*)hdmitx_device, "kthread_cec");
    if(request_irq(INT_HDMI_CEC, &cec_isr_handler,
                IRQF_SHARED, "amhdmitx-cec",
                (void *)hdmitx_device)){
        printk("HDMI CEC:Can't register IRQ %d\n",INT_HDMI_CEC);
        return;               
    }

    remote_cec_dev = input_allocate_device();   
    if (!remote_cec_dev)                          
    {  
        printk(KERN_ERR "remote_cec.c: Not enough memory\n");   
    }
    remote_cec_dev->name = "cec_input";
   
    //printk("\n--111--function:%s,line:%d,count:%d\n",__FUNCTION__,__LINE__,tasklet_cec.count);
   // tasklet_enable(&tasklet_cec);
    //printk("\n--222--function:%s,line:%d,count:%d\n",__FUNCTION__,__LINE__,tasklet_cec.count);
      //tasklet_cec.data = (unsigned long)remote_cec;
                                           
    remote_cec_dev->evbit[0] = BIT_MASK(EV_KEY);      
    remote_cec_dev->keybit[BIT_WORD(BTN_0)] = BIT_MASK(BTN_0); 
    for (i = 0; i<KEY_MAX; i++){
          set_bit( i, remote_cec_dev->keybit);
      }
                   
    if (input_register_device(remote_cec_dev))  
    {  
        printk(KERN_ERR "remote_cec.c: Failed to register device\n");  
        input_free_device(remote_cec_dev);   
    }  
    return;
}

void cec_uninit(hdmitx_dev_t* hdmitx_device)
{
    if(!((hdmi_cec_func_config>>CEC_FUNC_MSAK) & 0x1)){
        return ;
    }
    printk("CEC: cec uninit!\n");
    if (cec_init_flag == 1) {
        WRITE_MPEG_REG(SYS_CPU_0_IRQ_IN1_INTR_MASK, READ_MPEG_REG(SYS_CPU_0_IRQ_IN1_INTR_MASK) & ~(1 << 23));            // Disable the hdmi cec interrupt
        free_irq(INT_HDMI_CEC, (void *)hdmitx_device);
        cec_init_flag = 0;
    }
    input_unregister_device(remote_cec_dev);    
}

void cec_set_pending(tv_cec_pending_e on_off)
{
    cec_pending_flag = on_off;
}

size_t cec_usrcmd_get_global_info(char * buf)
{
    int i = 0;
    int dev_num = 0;

    cec_node_info_t * buf_node_addr = (cec_node_info_t *)(buf + (unsigned int)(((cec_global_info_to_usr_t*)0)->cec_node_info_online));

    for (i = 0; i < MAX_NUM_OF_DEV; i++) {
        if (cec_global_info.dev_mask & (1 << i)) {
            memcpy(&(buf_node_addr[dev_num]), &(cec_global_info.cec_node_info[i]), sizeof(cec_node_info_t));
            dev_num++;
        }
    }

    buf[0] = dev_num;
    buf[1] = cec_global_info.active_log_dev;
#if 0
    printk("\n");
    printk("%x\n",(unsigned int)(((cec_global_info_to_usr_t*)0)->cec_node_info_online));
    printk("%x\n", ((cec_global_info_to_usr_t*)buf)->dev_number);
    printk("%x\n", ((cec_global_info_to_usr_t*)buf)->active_log_dev);
    printk("%x\n", ((cec_global_info_to_usr_t*)buf)->cec_node_info_online[0].hdmi_port);
    for (i=0; i < (sizeof(cec_node_info_t) * dev_num) + 2; i++) {
        printk("%x,",buf[i]);
    }
    printk("\n");
#endif
    return (sizeof(cec_node_info_t) * dev_num) + (unsigned int)(((cec_global_info_to_usr_t*)0)->cec_node_info_online);
}

void cec_usrcmd_set_lang_config(const char * buf, size_t count)
{
    char tmpbuf[128];
    int i=0;

    while((buf[i])&&(buf[i]!=',')&&(buf[i]!=' ')){
        tmpbuf[i]=buf[i];
        i++;    
    }

    cec_global_info.cec_node_info[cec_global_info.my_node_index].menu_lang = simple_strtoul(tmpbuf, NULL, 16);

}
void cec_usrcmd_set_config(const char * buf, size_t count)
{
    int i = 0;
    int j = 0;
//    int bool = 0;
    char param[16] = {0};

    if(count > 32){
        printk("CEC: too many args\n");
    }
    for(i = 0; i < count; i++){
        if ( (buf[i] >= '0') && (buf[i] <= 'f') ){
            param[j] = simple_strtoul(&buf[i], NULL, 16);
            j ++;
        }
        while ( buf[i] != ' ' )
            i ++;
    }
   
    switch (param[0]) {
    case CEC_FUNC_MSAK:   
        if(param[1]){
            hdmi_cec_func_config |= (1 << CEC_FUNC_MSAK);
            aml_write_reg32(P_AO_DEBUG_REG0, aml_read_reg32(P_AO_DEBUG_REG0) | (hdmi_cec_func_config & 0xf));
        }else{
            hdmi_cec_func_config &= ~(1 << CEC_FUNC_MSAK);
            aml_write_reg32(P_AO_DEBUG_REG0, aml_read_reg32(P_AO_DEBUG_REG0) & (~(hdmi_cec_func_config & 0xf)));
            break;
        }
    case ONE_TOUCH_PLAY_MASK:
        if(param[1]){
            hdmi_cec_func_config |= (1 << ONE_TOUCH_PLAY_MASK);
            aml_write_reg32(P_AO_DEBUG_REG0, aml_read_reg32(P_AO_DEBUG_REG0) | (hdmi_cec_func_config & 0xf));
        }else{
            hdmi_cec_func_config &= ~(1 << ONE_TOUCH_PLAY_MASK);
            aml_write_reg32(P_AO_DEBUG_REG0, aml_read_reg32(P_AO_DEBUG_REG0) & (~(hdmi_cec_func_config & 0xf)));            
            break;
        }
    case ONE_TOUCH_STANDBY_MASK:
        if(param[1]){
            hdmi_cec_func_config |= (1 << ONE_TOUCH_STANDBY_MASK);
            aml_write_reg32(P_AO_DEBUG_REG0, aml_read_reg32(P_AO_DEBUG_REG0) | (hdmi_cec_func_config & 0xf));
        }else{
            hdmi_cec_func_config &= ~(1 << ONE_TOUCH_STANDBY_MASK);
            aml_write_reg32(P_AO_DEBUG_REG0, aml_read_reg32(P_AO_DEBUG_REG0) & (~(hdmi_cec_func_config & 0xf)));            
            break;
        }
    case AUTO_POWER_ON_MASK:  
        if(param[1]){
            hdmi_cec_func_config |= (1 << AUTO_POWER_ON_MASK);
            aml_write_reg32(P_AO_DEBUG_REG0, aml_read_reg32(P_AO_DEBUG_REG0) | (hdmi_cec_func_config & 0xf));
        }else{
            hdmi_cec_func_config &= ~(1 << AUTO_POWER_ON_MASK);
            aml_write_reg32(P_AO_DEBUG_REG0, aml_read_reg32(P_AO_DEBUG_REG0) & (~(hdmi_cec_func_config & 0xf)));            
            break;
        }
    default:
        break;
    }
    hdmirx_cec_dbg_print("hdmi_cec_func_config:0x%x \n",hdmi_cec_func_config);    
}


void cec_usrcmd_set_dispatch(const char * buf, size_t count)
{
    int i = 0;
    int j = 0;
    int bool = 0;
    char param[16] = {0};

    if(count > 32){
        printk("CEC: too many args\n");
    }
    for(i = 0; i < count; i++){
        if ( (buf[i] >= '0') && (buf[i] <= 'f') ){
            param[j] = simple_strtoul(&buf[i], NULL, 16);
            j ++;
        }
        while ( buf[i] != ' ' )
            i ++;
    }
   
    hdmirx_cec_dbg_print("cec_usrcmd_set_dispatch: \n");

    switch (param[0]) {
    case GET_CEC_VERSION:   //0 LA
        cec_usrcmd_get_cec_version(param[1]);
        break;
    case GET_DEV_POWER_STATUS:
        cec_usrcmd_get_device_power_status(param[1]);
        break;
    case GET_DEV_VENDOR_ID:
        cec_usrcmd_get_device_vendor_id(param[1]);
        break;
    case GET_OSD_NAME:
        cec_usrcmd_get_osd_name(param[1]);
        break;
    case GET_PHYSICAL_ADDR:
        cec_usrcmd_get_physical_address(param[1]);
        break;
    case SET_STANDBY:       //d LA
        cec_usrcmd_set_standby(param[1]);
        break;
    case SET_IMAGEVIEW_ON:  //e LA
        cec_usrcmd_set_imageview_on(param[1]);
        break;
    case GIVE_DECK_STATUS:
        cec_usrcmd_get_deck_status(param[1]);
        break;
    case SET_DECK_CONTROL_MODE:
        cec_usrcmd_set_deck_cnt_mode(param[1], param[2]);
        break;
    case SET_PLAY_MODE:
        cec_usrcmd_set_play_mode(param[1], param[2]);
        break;
    case GET_SYSTEM_AUDIO_MODE:
        cec_usrcmd_get_system_audio_mode_status(param[1]);
        break;
    case GET_TUNER_DEV_STATUS:
        cec_usrcmd_get_tuner_device_status(param[1]);
        break;
    case GET_AUDIO_STATUS:
        cec_usrcmd_get_audio_status(param[1]);
        break;
    case GET_OSD_STRING:
        break;
    case GET_MENU_STATE:
        cec_usrcmd_get_menu_state(param[1]);
        break;
    case SET_MENU_STATE:
        cec_usrcmd_set_menu_state(param[1], param[2]);
        break;
    case SET_MENU_LANGAGE:
        cec_usrcmd_set_menu_language(param[1], param[2]);
        break;
    case GET_MENU_LANGUAGE:
        cec_usrcmd_get_menu_language(param[1]);
        break;
    case GET_ACTIVE_SOURCE:     //13???????
        cec_usrcmd_get_active_source();
        break;
    case SET_ACTIVE_SOURCE:
        cec_usrcmd_set_active_source();
        break;
    case SET_DEACTIVE_SOURCE:
        cec_usrcmd_set_deactive_source(param[1]);
        break;
//    case CLR_NODE_DEV_REAL_INFO_MASK:
//        cec_usrcmd_clear_node_dev_real_info_mask(param[1], (((cec_info_mask)param[2]) << 24) |
//                                                         (((cec_info_mask)param[3]) << 16) |
//                                                         (((cec_info_mask)param[4]) << 8)  |
//                                                         ((cec_info_mask)param[5]));
//        break;
    case REPORT_PHYSICAL_ADDRESS:    //17 
    	cec_usrcmd_set_report_physical_address();
    	break;
    case SET_TEXT_VIEW_ON:          //18 LA
    	cec_usrcmd_text_view_on(param[1]);
        break;
    case POLLING_ONLINE_DEV:    //19 LA 
        hdmitx_cec_dbg_print("\n-----POLLING_ONLINE_DEV------\n");
        cec_polling_online_dev(param[1], &bool);
        break;
    default:
        break;
    }
}

/***************************** cec high level code end *****************************/

