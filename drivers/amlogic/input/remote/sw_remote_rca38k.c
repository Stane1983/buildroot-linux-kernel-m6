/*
 * linux/drivers/input/irremote/sw_remote_rca38k.c
 *
 * Keypad Driver
 *
 * Copyright (C) 2013 Amlogic Corporation
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * author :   platform-beijing
 */
/*
 * !!caution: if you use remote ,you should disable card1 used for  ata_enable pin.
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/types.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/errno.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <mach/am_regs.h>
#include "am_remote.h"

extern char *remote_log_buf;
static int dbg_printk(const char *fmt, ...)
{
	char buf[100];
	va_list args;

	va_start(args, fmt);
	vscnprintf(buf, 100, fmt, args);
	if (strlen(remote_log_buf) + (strlen(buf) + 64) > REMOTE_LOG_BUF_LEN) {
		remote_log_buf[0] = '\0';
	}
	strcat(remote_log_buf, buf);
	va_end(args);
	return 0;
}


static int get_pulse_width(unsigned long data)
{
	struct remote *remote_data = (struct remote *)data;
	int pulse_width;
	const char *state;

	pulse_width = (am_remote_read_reg(AM_IR_DEC_REG1) & 0x1FFF0000) >> 16;
	state = remote_data->step == REMOTE_STATUS_WAIT ? "wait" :
		remote_data->step == REMOTE_STATUS_LEADER ? "leader" :
		remote_data->step == REMOTE_STATUS_DATA ? "data" :
		remote_data->step == REMOTE_STATUS_SYNC ? "sync" : NULL;
	dbg_printk("%02d:pulse_wdith:%d==>%s\r\n",
			remote_data->bit_count - remote_data->bit_num, pulse_width, state);
	//sometimes we found remote  pulse width==0.        in order to sync machine state we modify it .
	if (pulse_width == 0) {
		switch (remote_data->step) {
			case REMOTE_STATUS_LEADER:
				pulse_width = remote_data->time_window[0] + 1;
				break;
			case REMOTE_STATUS_DATA:
				pulse_width = remote_data->time_window[2] + 1;
				break;
		}
	}
	return pulse_width;
}

static inline void rca_software_mode_remote_wait(unsigned long data)
{
	struct remote *remote_data = (struct remote *)data;
	remote_data->step = REMOTE_STATUS_LEADER;
	remote_data->cur_keycode = 0;
	remote_data->bit_num = remote_data->bit_count;
}

static inline void rca_software_mode_remote_leader(unsigned long data)
{
	unsigned short pulse_width;
	struct remote *remote_data = (struct remote *)data;
	pulse_width = get_pulse_width(data);
	if ((pulse_width > remote_data->time_window[0])
			&& (pulse_width < remote_data->time_window[1])) {
		remote_data->step = REMOTE_STATUS_DATA;
	} else {
		remote_data->step = REMOTE_STATUS_WAIT;
	}
	remote_data->cur_keycode = 0;
	remote_data->bit_num = remote_data->bit_count;
}

static inline void rca_software_mode_remote_send_key(unsigned long data)
{
	struct remote *remote_data = (struct remote *)data;
//	unsigned int report_key_code = remote_data->cur_keycode >> 16 & 0xffff;
 	unsigned int scancode;
	int i,want_repeat_enable;
	remote_data->step = REMOTE_STATUS_SYNC;
	if (remote_data->repeate_flag) {
		if ((remote_data->custom_code[0] & 0xf) != (remote_data->cur_keycode & 0xf)){
			return;
		}
		if (remote_data->repeat_tick < jiffies) {
			remote_send_key(remote_data->input,
					~(remote_data->cur_keycode >> 16) & 0xff, 2);
			remote_data->repeat_tick +=
				msecs_to_jiffies(remote_data->input->rep[REP_PERIOD]);
		}
	} else {
		if ((remote_data->custom_code[0] & 0xf) != (remote_data->cur_keycode & 0xf)){
			input_dbg("Wrong custom code is 0x%08x\n",
					remote_data->cur_keycode);
			return;
		}
		remote_send_key(remote_data->input,~(remote_data->cur_keycode >> 16)&0xff, 1);	
		scancode = ~(remote_data->cur_keycode >> 16)&0xff;
		for (i = 0; i < ARRAY_SIZE(remote_data->key_repeat_map); i++)
		if (remote_data->key_repeat_map[i] == scancode) {
			want_repeat_enable = 1;
		}
		else{
			want_repeat_enable = 0;
		}
		if (remote_data->repeat_enable || want_repeat_enable)
			remote_data->repeat_tick =
				jiffies +
				msecs_to_jiffies(remote_data->input->rep[REP_DELAY]);
	}
}

static inline void rca_software_mode_remote_data(unsigned long data)
{
	unsigned short pulse_width;
	struct remote *remote_data = (struct remote *)data;

	pulse_width = get_pulse_width(data);
	remote_data->step = REMOTE_STATUS_DATA;
	if ((pulse_width > remote_data->time_window[2])
			&& (pulse_width < remote_data->time_window[3])) {
		remote_data->bit_num--;
	} else if ((pulse_width > remote_data->time_window[4])
			&& (pulse_width < remote_data->time_window[5])) {
		remote_data->cur_keycode |=
			1 << (remote_data->bit_count - remote_data->bit_num);
		remote_data->bit_num--;
	} else {
		remote_data->step = REMOTE_STATUS_WAIT;
	}
	if (remote_data->bit_num == 0) {
		remote_data->repeate_flag = 0;
		remote_data->send_data = 1;
		fiq_bridge_pulse_trigger(&remote_data->fiq_handle_item);
	}
}
static inline void rca_software_mode_remote_sync(unsigned long data)
{
	unsigned short pulse_width;
	struct remote *remote_data = (struct remote *)data;

	pulse_width = get_pulse_width(data);
	if ((pulse_width > remote_data->time_window[6])
	    && (pulse_width < remote_data->time_window[7])) {
		remote_data->repeate_flag = 1;
		if (remote_data->repeat_enable) {
			remote_data->send_data = 1;
		} else {
			remote_data->step = REMOTE_STATUS_SYNC;
			return;
		}
	}
	remote_data->step = REMOTE_STATUS_SYNC;
	fiq_bridge_pulse_trigger(&remote_data->fiq_handle_item);
}

void remote_rca_reprot_key(unsigned long data)
{
	struct remote *remote_data = (struct remote *)data;
	int current_jiffies = jiffies;

	if (((current_jiffies - remote_data->last_jiffies) > 20)
			&& (remote_data->step <= REMOTE_STATUS_SYNC)) {
		remote_data->step = REMOTE_STATUS_WAIT;
	}
	remote_data->last_jiffies = current_jiffies;	//ignore a little msecs
	switch (remote_data->step) {
		case REMOTE_STATUS_WAIT:
			rca_software_mode_remote_wait(data);
			break;
		case REMOTE_STATUS_LEADER:
			rca_software_mode_remote_leader(data);
			break;
		case REMOTE_STATUS_DATA:
			rca_software_mode_remote_data(data);
			break;
		case REMOTE_STATUS_SYNC:
			rca_software_mode_remote_sync(data);
			break;
		default:
			break;
	}
}

irqreturn_t remote_rca_bridge_isr(int irq, void *dev_id)
{
	struct remote *remote_data = (struct remote *)dev_id;

	if (remote_data->send_data) {	//report key
		rca_software_mode_remote_send_key((unsigned long)remote_data);
		remote_data->send_data = 0;
	}
	remote_data->timer.data = (unsigned long)remote_data;
	mod_timer(&remote_data->timer,
			jiffies + msecs_to_jiffies(remote_data->release_delay));
	return IRQ_HANDLED;
}
