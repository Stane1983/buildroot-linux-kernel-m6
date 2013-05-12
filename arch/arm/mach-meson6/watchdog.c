/*
 * Amlogic WATCH DOG
 *
 * Copyright (C) 2010 Amlogic Corporation
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
 */
#include <mach/am_regs.h>
#include <mach/watchdog.h>
#include <linux/spinlock.h>
#include <linux/printk.h>
#include <linux/module.h>
#include <linux/slab.h>

 
static DEFINE_SPINLOCK(aml_wd_lock);
static int g_time_out;
static int emergency_flag;
static int g_state = WATCHDOG_OFF;
static LIST_HEAD(watchdog_list);

static int get_total_state_locked(void)
{
	int total_state = WATCHDOG_OFF;
	aml_watchdog_t* watchdog;
	
	list_for_each_entry(watchdog, &watchdog_list, entry)  {
		pr_info("%s: %s, %dms\n", watchdog->name, 
			watchdog->state?"on":"off", watchdog->time_out);
		if (watchdog->state == WATCHDOG_ON) {
			total_state = WATCHDOG_ON;
			break;
		}
	}
	pr_info("total state: %s\n", total_state?"on":"off");
	pr_info("total time out: %d\n", g_time_out);
		
	return total_state;
}

static int get_total_time_out_locked(void)
{
	int total_time_out = 0;
	aml_watchdog_t* watchdog;
	
	list_for_each_entry(watchdog, &watchdog_list, entry) { 
		pr_info("%s: %s, %dms\n", watchdog->name, 
			watchdog->state?"on":"off", watchdog->time_out);
		if (watchdog->time_out > total_time_out 
				&& watchdog->state == WATCHDOG_ON)
			total_time_out = watchdog->time_out;
	}
	if (total_time_out > MAX_TIME_OUT)
		total_time_out = MAX_TIME_OUT;
	
	if (total_time_out < MIN_TIME_OUT)
		total_time_out = MIN_TIME_OUT;
	pr_info("total state: %s\n", g_state?"on":"off");
	pr_info("total time out: %d\n", total_time_out);
		
	return total_time_out;
}
 
int aml_disable_watchdog(aml_watchdog_t* watchdog)
{
	unsigned long flags;
	int total_time_out = 0;
	
	if (!watchdog || emergency_flag)
		return -1;
		
	pr_info("disable watchdog: %s\n", watchdog->name);
	
	spin_lock_irqsave(&aml_wd_lock, flags);
	watchdog->state = WATCHDOG_OFF;
	total_time_out = get_total_time_out_locked();
	if (g_time_out != total_time_out) {
		g_time_out = total_time_out;
		if (total_time_out == 0)
			total_time_out = DEFAULT_TIME_OUT;
		aml_set_reg32_bits(P_WATCHDOG_TC, 
				(total_time_out*100) & 0x3fffff, 0, 22);
	}
	if (get_total_state_locked() == WATCHDOG_OFF 
				&& g_state != WATCHDOG_OFF) {
		g_state = WATCHDOG_OFF;
		aml_write_reg32(P_WATCHDOG_RESET, 0);
		aml_clr_reg32_mask(P_WATCHDOG_TC, (1 << WATCHDOG_ENABLE_BIT));
	}
	spin_unlock_irqrestore(&aml_wd_lock, flags);
	return 0;
}
EXPORT_SYMBOL(aml_disable_watchdog);

int aml_enable_watchdog(aml_watchdog_t* watchdog)
{
	unsigned long flags;
	int total_time_out = 0;
	
	if (!watchdog || emergency_flag)
		return -1;
	pr_info("enable watchdog: %s\n", watchdog->name);
	
	spin_lock_irqsave(&aml_wd_lock, flags);
	watchdog->state = WATCHDOG_ON;
	total_time_out = get_total_time_out_locked();
	if (g_time_out != total_time_out) {
		g_time_out = total_time_out;
		if (total_time_out == 0)
			total_time_out = DEFAULT_TIME_OUT;
		aml_set_reg32_bits(P_WATCHDOG_TC, 
				(total_time_out*100) & 0x3fffff, 0, 22);
	}
	if (g_state == WATCHDOG_OFF) {
		g_state = WATCHDOG_ON;
		aml_write_reg32(P_WATCHDOG_RESET, 0);
		aml_set_reg32_mask(P_WATCHDOG_TC, 1 << WATCHDOG_ENABLE_BIT);
	}
	spin_unlock_irqrestore(&aml_wd_lock, flags);
	return 0;
}
EXPORT_SYMBOL(aml_enable_watchdog);

int aml_emergency_restart(int time_out)
{
	emergency_flag = 1;
	pr_info("aml_emergency_restart\n"); 
	aml_set_reg32_bits(P_WATCHDOG_TC, 
				(time_out*100) & 0x3fffff, 0, 22);
	aml_write_reg32(P_WATCHDOG_RESET, 0);
	aml_set_reg32_mask(P_WATCHDOG_TC, 1 << WATCHDOG_ENABLE_BIT);
	return 0;
}
EXPORT_SYMBOL(aml_emergency_restart);
	
int aml_set_watchdog_timeout_ms(aml_watchdog_t* watchdog, int time_out)
{
	unsigned long flags;
	int total_time_out = 0;

	if (!watchdog || emergency_flag)
		return -1;
		
	pr_info("set watchdog time out: %s\n", watchdog->name);
	
	spin_lock_irqsave(&aml_wd_lock, flags);
	watchdog->time_out =  time_out;
	total_time_out = get_total_time_out_locked();
	if (g_time_out != total_time_out) {
		g_time_out = total_time_out;
		if (total_time_out == 0)
			total_time_out = DEFAULT_TIME_OUT;
		aml_set_reg32_bits(P_WATCHDOG_TC, 
				(total_time_out*100) & 0x3fffff, 0, 22);
	}
	spin_unlock_irqrestore(&aml_wd_lock, flags);
	return 0;
}
EXPORT_SYMBOL(aml_set_watchdog_timeout_ms);

int aml_reset_watchdog(aml_watchdog_t* watchdog)
{
	unsigned long flags;
	if (!watchdog || emergency_flag)
		return -1;
	
	spin_lock_irqsave(&aml_wd_lock, flags);
	pr_info("reset watchdog: %s\n", watchdog->name);
	aml_write_reg32(P_WATCHDOG_RESET, 0);	
	spin_unlock_irqrestore(&aml_wd_lock, flags);
	return 0;
}
EXPORT_SYMBOL(aml_reset_watchdog);

aml_watchdog_t* aml_get_watchdog(const char* name)
{
	int str_len = strlen(name);
	unsigned long flags;
	aml_watchdog_t* watchdog;
	if (str_len > NAME_MAX_LEN)
		str_len = NAME_MAX_LEN;
	printk("aml_get_watchdog\n");
	spin_lock_irqsave(&aml_wd_lock, flags);
	list_for_each_entry(watchdog, &watchdog_list, entry) 
		if (!strncmp(watchdog->name, name, str_len)) {
			spin_unlock_irqrestore(&aml_wd_lock, flags);
			return watchdog;
		}
	spin_unlock_irqrestore(&aml_wd_lock, flags);
	printk("aml_get_watchdog NULL\n");
	return NULL;
}
EXPORT_SYMBOL(aml_get_watchdog);

aml_watchdog_t* aml_create_watchdog(const char* name)
{
	int str_len = strlen(name);
	unsigned long flags;
	aml_watchdog_t* watchdog = NULL;
	if (str_len > NAME_MAX_LEN)
		str_len = NAME_MAX_LEN;
	
	spin_lock_irqsave(&aml_wd_lock, flags);
	list_for_each_entry(watchdog, &watchdog_list, entry) 
		if (!strncmp(watchdog->name, name, str_len)) {
			pr_info("watch dog: %s have been exist\n", name);
			spin_unlock_irqrestore(&aml_wd_lock, flags);
			return NULL;
		}
	
	watchdog = kzalloc(sizeof(aml_watchdog_t), GFP_KERNEL);
	if (watchdog) {
		strncpy(watchdog->name, name, str_len);
		list_add(&watchdog->entry, &watchdog_list);
	}
	spin_unlock_irqrestore(&aml_wd_lock, flags);
	return watchdog;
}
EXPORT_SYMBOL(aml_create_watchdog);

int aml_destroy_watchdog(aml_watchdog_t* watchdog_p)
{
	unsigned long flags;
	int ret = -1;
	int str_len = strlen(watchdog_p->name);
	aml_watchdog_t* watchdog;
	
	spin_lock_irqsave(&aml_wd_lock, flags);
	list_for_each_entry(watchdog, &watchdog_list, entry) 
		if (!strncmp(watchdog->name, watchdog_p->name, str_len)) {
			list_del(&watchdog->entry);
			kfree(watchdog);
			ret = 0;
			break;
		}
	spin_unlock_irqrestore(&aml_wd_lock, flags);
	return ret;
}
EXPORT_SYMBOL(aml_destroy_watchdog);

static int __init aml_watchdog_init(void)
{
	int ret = -1;
	aml_watchdog_t* watchdog;
	
	watchdog  = aml_create_watchdog(SUSPEND_WATCHDOG);
	if (watchdog)
		ret = 0;
	return ret;
}
late_initcall(aml_watchdog_init);