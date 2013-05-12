/*
 * arch/arm/mach-meson2/irq.c
 *
 * Copyright (C) 2010 Amlogic, Inc.
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */


#include <linux/init.h>
#include <linux/irq.h>

#include <mach/am_regs.h>
#include <mach/irqs.h>


/* Enable interrupt */
static void m2_unmask_irq(struct irq_data *data)
{
	unsigned int mask;
	unsigned int irq;

	irq = data->irq;
	if (irq >= NR_IRQS)
		return;

	mask = 1 << IRQ_BIT(irq);

	SET_CBUS_REG_MASK(IRQ_MASK_REG(irq), mask);

	dsb();
}

/* Disable interrupt */
static void m2_mask_irq(struct irq_data *data)
{
	unsigned int mask;
	unsigned int irq;

	irq = data->irq;
	if (irq >= NR_IRQS)
		return;

	mask = 1 << IRQ_BIT(irq);

	CLEAR_CBUS_REG_MASK(IRQ_MASK_REG(irq), mask);

	dsb();
}

/* Clear interrupt */
static void m2_ack_irq(struct irq_data *data)
{
	unsigned int mask;
	unsigned int irq;

	irq = data->irq;
	if (irq >= NR_IRQS)
		return;

	mask = 1 << IRQ_BIT(irq);

	WRITE_CBUS_REG(IRQ_CLR_REG(irq), mask);

	dsb();
}

static struct irq_chip m2_irq_chip = {
	.name		= "MESON-INTC",
	.irq_ack	= m2_ack_irq,
	.irq_mask	= m2_mask_irq,
	.irq_unmask	= m2_unmask_irq,
};

/* ARM Interrupt Controller Initialization */
void __init m2_init_irq(void)
{
	unsigned int irq;

	/* Disable all interrupt requests */
	WRITE_CBUS_REG(A9_0_IRQ_IN0_INTR_MASK, 0);
	WRITE_CBUS_REG(A9_0_IRQ_IN1_INTR_MASK, 0);
	WRITE_CBUS_REG(A9_0_IRQ_IN2_INTR_MASK, 0);
	WRITE_CBUS_REG(A9_0_IRQ_IN3_INTR_MASK, 0);

	/* Clear all interrupts */
	WRITE_CBUS_REG(A9_0_IRQ_IN0_INTR_STAT_CLR, ~0);
	WRITE_CBUS_REG(A9_0_IRQ_IN1_INTR_STAT_CLR, ~0);
	WRITE_CBUS_REG(A9_0_IRQ_IN2_INTR_STAT_CLR, ~0);
	WRITE_CBUS_REG(A9_0_IRQ_IN3_INTR_STAT_CLR, ~0);

	/* Set all interrupts to IRQ */
	WRITE_CBUS_REG(A9_0_IRQ_IN0_INTR_FIRQ_SEL, 0);
	WRITE_CBUS_REG(A9_0_IRQ_IN1_INTR_FIRQ_SEL, 0);
	WRITE_CBUS_REG(A9_0_IRQ_IN2_INTR_FIRQ_SEL, 0);
	WRITE_CBUS_REG(A9_0_IRQ_IN3_INTR_FIRQ_SEL, 0);

	/* set up genirq dispatch */
	for (irq = 0; irq < NR_IRQS; irq++) {
		irq_set_chip(irq, &m2_irq_chip);
		irq_set_handler(irq, handle_level_irq);
		set_irq_flags(irq, IRQF_VALID);
	}
}

