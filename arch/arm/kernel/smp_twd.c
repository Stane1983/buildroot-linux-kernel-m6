/*
 *  linux/arch/arm/kernel/smp_twd.c
 *
 *  Copyright (C) 2002 ARM Ltd.
 *  All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/cpufreq.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/smp.h>
#include <linux/jiffies.h>
#include <linux/clockchips.h>
#include <linux/irq.h>
#include <linux/io.h>

#include <asm/smp_twd.h>
#include <asm/localtimer.h>
#include <asm/hardware/gic.h>

/* set up by the platform code */
void __iomem *twd_base;

static struct clk *twd_clk;
static unsigned long twd_timer_rate;

static struct clock_event_device __percpu **twd_evt;

static void twd_set_mode(enum clock_event_mode mode,
			struct clock_event_device *clk)
{
	unsigned long ctrl;

	switch (mode) {
	case CLOCK_EVT_MODE_PERIODIC:
		/* timer load already set up */
		ctrl = TWD_TIMER_CONTROL_ENABLE | TWD_TIMER_CONTROL_IT_ENABLE
			| TWD_TIMER_CONTROL_PERIODIC;
		__raw_writel(twd_timer_rate / HZ, twd_base + TWD_TIMER_LOAD);
		break;
	case CLOCK_EVT_MODE_ONESHOT:
		/* period set, and timer enabled in 'next_event' hook */
		ctrl = TWD_TIMER_CONTROL_IT_ENABLE | TWD_TIMER_CONTROL_ONESHOT;
		break;
	case CLOCK_EVT_MODE_UNUSED:
	case CLOCK_EVT_MODE_SHUTDOWN:
	default:
		ctrl = 0;
		break;
	}

	__raw_writel(ctrl, twd_base + TWD_TIMER_CONTROL);
}

static int twd_set_next_event(unsigned long evt,
			struct clock_event_device *unused)
{
	unsigned long ctrl = __raw_readl(twd_base + TWD_TIMER_CONTROL);

	ctrl |= TWD_TIMER_CONTROL_ENABLE;

	__raw_writel(evt, twd_base + TWD_TIMER_COUNTER);
	__raw_writel(ctrl, twd_base + TWD_TIMER_CONTROL);

	return 0;
}

/*
 * local_timer_ack: checks for a local timer interrupt.
 *
 * If a local timer interrupt has occurred, acknowledge and return 1.
 * Otherwise, return 0.
 */
int twd_timer_ack(void)
{
	if (__raw_readl(twd_base + TWD_TIMER_INTSTAT)) {
		__raw_writel(1, twd_base + TWD_TIMER_INTSTAT);
		return 1;
	}

	return 0;
}

static unsigned long  twd_calibrate_rate(void)
{
    unsigned long count,rate;
    u64 waitjiffies;


    /*
     * If this is the first time round, we need to work out how fast
     * the timer ticks
     */

    printk(KERN_INFO "Calibrating local timer... ");

    /* Wait for a tick to start */
    waitjiffies = get_jiffies_64() + 1;

    while (get_jiffies_64() < waitjiffies)
        udelay(10);

    /* OK, now the tick has started, let's get the timer going */
    waitjiffies += 5;

    /* enable, no interrupt or reload */
    __raw_writel(0x1, twd_base + TWD_TIMER_CONTROL);

    /* maximum value */
    __raw_writel(0xFFFFFFFFU, twd_base + TWD_TIMER_COUNTER);

    while (get_jiffies_64() < waitjiffies)
        udelay(10);

    count = __raw_readl(twd_base + TWD_TIMER_COUNTER);

    rate = (0xFFFFFFFFU - count) * (HZ / 5);

    printk("%lu.%02luMHz.\n", rate / 1000000, (rate / 10000) % 100);
    return rate;
}

/*
 * Setup the local clock events for a CPU.
 */
static struct clk *twd_get_clock(void)
{
    struct clk *clk;
    int err;

    clk = clk_get_sys("smp_twd", NULL);
    if (IS_ERR(clk)) {
        pr_err("smp_twd: clock not found: %d\n", (int)PTR_ERR(clk));
        return clk;
    }
#if 0
    err = clk_prepare(clk);
    if (err) {
        pr_err("smp_twd: clock failed to prepare: %d\n", err);
        clk_put(clk);
        return ERR_PTR(err);
    }

    err = clk_enable(clk);
    if (err) {
        pr_err("smp_twd: clock failed to enable: %d\n", err);
        clk_unprepare(clk);
        clk_put(clk);
        return ERR_PTR(err);
    }
#endif
    return clk;
}
#ifdef CONFIG_CPU_FREQ
static unsigned long get_smp_twd_rate(void)
{
    if(IS_ERR_OR_NULL(twd_clk))
        twd_clk=twd_get_clock();
    BUG_ON(IS_ERR_OR_NULL(twd_clk));
    return clk_get_rate(twd_clk);
}
/*
 * Updates clockevent frequency when the cpu frequency changes.
 * Called on the cpu that is changing frequency with interrupts disabled.
 */
static void twd_update_frequency(void *data)
{
	twd_timer_rate = get_smp_twd_rate();

	clockevents_update_freq(*__this_cpu_ptr(twd_evt), twd_timer_rate);
}

static int twd_cpufreq_transition(struct notifier_block *nb,
    unsigned long state, void *data)
{
    struct cpufreq_freqs *freqs = data;

    /*
     * The twd clock events must be reprogrammed to account for the new
     * frequency.  The timer is local to a cpu, so cross-call to the
     * changing cpu.
     */
    if (state == CPUFREQ_POSTCHANGE || state == CPUFREQ_RESUMECHANGE)
    {
        on_each_cpu( twd_update_frequency,NULL, 1);
        printk("new rate %u\n",twd_timer_rate);
    }
    printk("%s %d %u\n",__func__,state,twd_timer_rate);
    return NOTIFY_OK;
}

static struct notifier_block twd_cpufreq_nb = {
    .notifier_call = twd_cpufreq_transition,
};

static int twd_cpufreq_init(void)
{
    if (twd_evt && *__this_cpu_ptr(twd_evt) )
        return cpufreq_register_notifier(&twd_cpufreq_nb,
            CPUFREQ_TRANSITION_NOTIFIER);

    return 0;
}
core_initcall(twd_cpufreq_init);

#endif

void __cpuinit twd_timer_setup(struct clock_event_device *clk)
{
	struct clock_event_device **this_cpu_clk;

	if (!twd_evt) {
		int err;

		twd_evt = alloc_percpu(struct clock_event_device *);
		if (!twd_evt) {
			pr_err("twd: can't allocate memory\n");
			return;
		}


	}

	if (!twd_clk)
		twd_clk = twd_get_clock();

	if (!IS_ERR_OR_NULL(twd_clk))
		twd_timer_rate = clk_get_rate(twd_clk);
	else
		twd_timer_rate=twd_timer_rate?twd_timer_rate:twd_calibrate_rate();

	__raw_writel(0, twd_base + TWD_TIMER_CONTROL);

	clk->name = "local_timer";
	clk->features = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT |
			CLOCK_EVT_FEAT_C3STOP;
	clk->rating = 350;
	clk->set_mode = twd_set_mode;
	clk->set_next_event = twd_set_next_event;
	

	this_cpu_clk = __this_cpu_ptr(twd_evt);
	*this_cpu_clk = clk;
	/*
	clk->shift = 20;
	clk->mult = div_sc(twd_timer_rate, NSEC_PER_SEC, clk->shift);
	clk->max_delta_ns = clockevent_delta2ns(0xffffffff, clk);
	clk->min_delta_ns = clockevent_delta2ns(0xf, clk);
    */
	clockevents_config_and_register(clk, twd_timer_rate,
					0xf, 0xffffffff);
	/* Make sure our local interrupt controller has this enabled */
	gic_enable_ppi(clk->irq);
}
