/*
 * arch/arm/mach-meson2/time.c
 *
 * Copyright (C) 2010 AMLOGIC, INC.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/clockchips.h>
#include <linux/clocksource.h>
#include <asm/memory.h>
#include <asm/mach/map.h>
#include <asm/mach/time.h>
#include <asm/sched_clock.h>
#include <plat/regops.h>
#include <mach/map.h>
#include <mach/hardware.h>
#include <mach/reg_addr.h>
#include <mach/am_regs.h>

#include <linux/jiffies.h>


/* Enable interrupt */
static void m2_unmask_irq(unsigned int irq)
{
	unsigned int mask;

	if (irq >= NR_IRQS)
		return;

	mask = 1 << IRQ_BIT(irq);

	SET_CBUS_REG_MASK(IRQ_MASK_REG(irq), mask);

	dsb();
}

/* Disable interrupt */
static void m2_mask_irq(unsigned int irq)
{
	unsigned int mask;

	if (irq >= NR_IRQS)
		return;

	mask = 1 << IRQ_BIT(irq);

	CLEAR_CBUS_REG_MASK(IRQ_MASK_REG(irq), mask);

	dsb();
}

/* Clear interrupt */
static void m2_ack_irq(unsigned int irq)
{
	unsigned int mask;

	if (irq >= NR_IRQS)
		return;

	mask = 1 << IRQ_BIT(irq);

	WRITE_CBUS_REG(IRQ_CLR_REG(irq), mask);

	dsb();
}


/*
 * Clock Source Device, Timer-E
 */

#define TIMER_A_TICK	1000000		/* 1Mhz, 1us */
#define TIMER_A_BITS	16

#define TIMER_C_TICK	1000000		/* 1Mhz, 1us */
#define TIMER_C_BITS	16

#define TIMER_E_1MS_FREQ	1000	/* 1Khz, 1ms timebase resolution */
#define TIMER_E_BITS		24	/* Timer-E is 32-bit counter */
#define TIMER_E_1US_MULT	4194304000u
#define TIMER_E_1US_SHIFT	22
#define TIMER_E_1MS_MULT	4096000000u
#define TIMER_E_1MS_SHIFT	12


static cycle_t cycle_read_timer_e(struct clocksource *cs)
{
	/* return the count of timer-e */
	return (cycles_t) READ_CBUS_REG(ISA_TIMERE);
}

static struct clocksource clocksource_timer_e = {
	.name	= "Timer-E",
	.rating = 300,
	.read	= cycle_read_timer_e,
	.mask	= CLOCKSOURCE_MASK(TIMER_E_BITS),
	.flags	= CLOCK_SOURCE_IS_CONTINUOUS,
};

#ifdef CONFIG_HAVE_SCHED_CLOCK
static DEFINE_CLOCK_DATA(cd);
#endif

unsigned long long notrace sched_clock(void)
{
	struct clocksource *cs = &clocksource_timer_e;
	u32 cyc = cs->read(cs);
#ifdef CONFIG_HAVE_SCHED_CLOCK
	return cyc_to_fixed_sched_clock(&cd, cyc, (u32)~0,
		TIMER_E_1MS_MULT, TIMER_E_1MS_SHIFT);
	//return cyc_to_sched_clock(&cd, cyc, (u32)~0);
#else
	return clocksource_cyc2ns(cyc, cs->mult, cs->shift);
#endif
}

#ifdef CONFIG_HAVE_SCHED_CLOCK
static void notrace m2_update_sched_clock(void)
{
	struct clocksource *cs = &clocksource_timer_e;
	u32 cyc = cs->read(cs);
	update_sched_clock(&cd, cyc, (u32)~0);
}
#endif

static void __init m2_clocksource_init(void)
{
	struct clocksource *cs = &clocksource_timer_e;

	/* init sched clock */
#ifdef CONFIG_HAVE_SCHED_CLOCK
	init_fixed_sched_clock(&cd, m2_update_sched_clock, TIMER_E_BITS,
		TIMER_E_1MS_FREQ, TIMER_E_1MS_MULT, TIMER_E_1MS_SHIFT);
	//init_sched_clock(&cd, m2_update_sched_clock, TIMER_E_BITS, TIMER_E_FREQ);
#endif

	/* select input clock 1us */
	CLEAR_CBUS_REG_MASK(ISA_TIMER_MUX, TIMER_E_INPUT_MASK);
	SET_CBUS_REG_MASK(ISA_TIMER_MUX, TIMERE_UNIT_1ms << TIMER_E_INPUT_BIT);
	WRITE_CBUS_REG(ISA_TIMERE, 0);

	clocksource_register_hz(cs, TIMER_E_1MS_FREQ);
	printk("Timer-E shift:%u mult:%u\n", cs->shift, cs->mult);
}


/*
 * Clock Event Device
 */

static void m2_clkevt_set_mode(enum clock_event_mode mode,
	struct clock_event_device *evt)
{
	switch (mode) {
	case CLOCK_EVT_MODE_RESUME:
		break;

	case CLOCK_EVT_MODE_PERIODIC:
		m2_mask_irq(INT_TIMER_C);
		m2_unmask_irq(INT_TIMER_A);
		break;

	case CLOCK_EVT_MODE_ONESHOT:
		m2_mask_irq(INT_TIMER_A);
		break;

	case CLOCK_EVT_MODE_SHUTDOWN:
	case CLOCK_EVT_MODE_UNUSED:
		m2_mask_irq(INT_TIMER_A);
		m2_mask_irq(INT_TIMER_C);
		break;
	}
}

static int m2_set_next_event(unsigned long evt, struct clock_event_device *dev)
{
	m2_mask_irq(INT_TIMER_C);

	/* use a big number to clear previous trigger cleanly */
	SET_CBUS_REG_MASK(ISA_TIMERC, evt & 0xffff);
	m2_ack_irq(INT_TIMER_C);

	/* then set next event */
	WRITE_CBUS_REG_BITS(ISA_TIMERC, evt, 0, 16);
	m2_unmask_irq(INT_TIMER_C);

	return 0;
}

static struct clock_event_device clockevent_m2 = {
	.name           = "TIMER-AC",
	.rating         = 300,
	.features       = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT,
	.shift          = 20,
	.set_next_event = m2_set_next_event,
	.set_mode       = m2_clkevt_set_mode,
};


/*
 * Clock event timer A interrupt handler
 */
static irqreturn_t m2_timer_interrupt(int irq, void *dev_id)
{
	struct clock_event_device *evt = dev_id;
	m2_ack_irq(irq);
	evt->event_handler(evt);
	return IRQ_HANDLED;
}

static struct irqaction m2_timer_irq = {
	.name           = "M2 Timer Tick",
	.flags          = IRQF_DISABLED | IRQF_TIMER | IRQF_IRQPOLL,
	.handler        = m2_timer_interrupt,
	.dev_id		= &clockevent_m2,
};


static void __init m2_clockevent_init(void)
{
	CLEAR_CBUS_REG_MASK(ISA_TIMER_MUX, TIMER_A_INPUT_MASK | TIMER_C_INPUT_MASK);
	/* select input timebase 1us */
	SET_CBUS_REG_MASK(ISA_TIMER_MUX,
		(TIMER_UNIT_1us << TIMER_A_INPUT_BIT) |
		(TIMER_UNIT_1us << TIMER_C_INPUT_BIT));
	WRITE_CBUS_REG(ISA_TIMERA, 9999);

	/* Register clock event device */
	clockevent_m2.mult = div_sc(1000000, NSEC_PER_SEC, clockevent_m2.shift);
	clockevent_m2.max_delta_ns = clockevent_delta2ns(0xfffe, &clockevent_m2);
	clockevent_m2.min_delta_ns = clockevent_delta2ns(1, &clockevent_m2);
	clockevent_m2.cpumask = cpumask_of(0);
	clockevents_register_device(&clockevent_m2);

	/* Set up the IRQ handler */
	setup_irq(INT_TIMER_A, &m2_timer_irq);
	setup_irq(INT_TIMER_C, &m2_timer_irq);
}

/*
 * This sets up the system timers, clock source and clock event.
 */
static void __init m2_timer_init(void)
{
	m2_clocksource_init();
	m2_clockevent_init();
}

struct sys_timer m2_sys_timer = {
	.init = m2_timer_init,
};

