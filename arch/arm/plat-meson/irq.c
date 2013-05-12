/*
 *  arch/arm/plat-meson/core.c
 *
 *  Copyright (C) 2010 AMLOGIC, INC.
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

#include <mach/hardware.h>
#include <asm/memory.h>
#include <asm/mach/map.h>
#include <asm/mach/time.h>
#include <asm/mach/irq.h>
#include <mach/map.h>
#include <plat/io.h>



/***********************************************************************
 * IRQ
 **********************************************************************/

/* Enable interrupt */
int meson_unmask_irq_called = 0;

static void meson_unmask_irq(struct irq_data *d)
{
    unsigned int mask;
	unsigned int irq=d->irq;
    if (irq >= NR_IRQS) {
        return;
    }

    mask = 1 << IRQ_BIT(irq);
	meson_unmask_irq_called = 1;
	aml_set_reg32_mask(CBUS_REG_ADDR(IRQ_MASK_REG(irq)),mask);
    //SET_CBUS_REG_MASK(IRQ_MASK_REG(irq), mask);
	meson_unmask_irq_called = 2;
    dsb();
}

/* Disable interrupt */
static void meson_mask_irq(struct irq_data *d)
{
    unsigned int mask;
	unsigned int irq=d->irq;
	
    if (irq >= NR_IRQS) {
        return;
    }

    mask = 1 << IRQ_BIT(irq);
	aml_clr_reg32_mask(CBUS_REG_ADDR(IRQ_MASK_REG(irq)),mask);
    //CLEAR_CBUS_REG_MASK(IRQ_MASK_REG(irq), mask);

    dsb();
}

/* Clear interrupt */
static void meson_ack_irq(struct irq_data *d)
{
    unsigned int mask;
	unsigned int irq=d->irq;
    if (irq >= NR_IRQS) {
        return;
    }

    mask = 1 << IRQ_BIT(irq);

	aml_write_reg32(CBUS_REG_ADDR(IRQ_CLR_REG(irq)), mask);
    //WRITE_CBUS_REG(IRQ_CLR_REG(irq), mask);

    dsb();
}

static struct irq_chip meson_irq_chip = {
    .name   = "MESON-INTC",
    .irq_ack    = meson_ack_irq,
    .irq_mask   = meson_mask_irq,
    .irq_unmask = meson_unmask_irq,
};

int meson_init_irq_called = 0;
/* ARM Interrupt Controller Initialization */
void __init meson_init_irq(void)
{
    unsigned i;
	meson_init_irq_called = 1;
    /* Disable all interrupt requests */
    WRITE_CBUS_REG( CPU_0_IRQ_IN0_INTR_MASK, 0);
    WRITE_CBUS_REG( CPU_0_IRQ_IN1_INTR_MASK, 0);
    WRITE_CBUS_REG( CPU_0_IRQ_IN2_INTR_MASK, 0);
    WRITE_CBUS_REG( CPU_0_IRQ_IN3_INTR_MASK, 0);

    /* Clear all interrupts */
    WRITE_CBUS_REG( CPU_0_IRQ_IN0_INTR_STAT_CLR, ~0);
    WRITE_CBUS_REG( CPU_0_IRQ_IN1_INTR_STAT_CLR, ~0);
    WRITE_CBUS_REG( CPU_0_IRQ_IN2_INTR_STAT_CLR, ~0);
    WRITE_CBUS_REG( CPU_0_IRQ_IN3_INTR_STAT_CLR, ~0);

    /* Set all interrupts to IRQ */
    WRITE_CBUS_REG( CPU_0_IRQ_IN0_INTR_FIRQ_SEL, 0);
    WRITE_CBUS_REG( CPU_0_IRQ_IN1_INTR_FIRQ_SEL, 0);
    WRITE_CBUS_REG( CPU_0_IRQ_IN2_INTR_FIRQ_SEL, 0);
    WRITE_CBUS_REG( CPU_0_IRQ_IN3_INTR_FIRQ_SEL, 0);

    /* set up genirq dispatch */
    for (i = 0; i < NR_IRQS; i++) {
        irq_set_chip(i, &meson_irq_chip);
        irq_set_handler(i, handle_level_irq);
        set_irq_flags(i, IRQF_VALID);
    }
	meson_init_irq_called = 2;
}


