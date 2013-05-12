/*
 * arch/arm/mach-meson2/pinmux.c
 *
 * Copyright (C) 2012 Amlogic Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <linux/spinlock.h>

#include <mach/am_regs.h>
#include <mach/pinmux.h>
#include <mach/io.h>

#define PIN_MUX_REG_NUM   13
#define PIN_MUX_REG_0	  0x202c
#define PIN_MUX_REG_1	  0x202d
#define PIN_MUX_REG_2	  0x202e
#define PIN_MUX_REG_3	  0x202f
#define PIN_MUX_REG_4	  0x2030
#define PIN_MUX_REG_5	  0x2031
#define PIN_MUX_REG_6	  0x2032
#define PIN_MUX_REG_7	  0x2033
#define PIN_MUX_REG_8	  0x2034
#define PIN_MUX_REG_9	  0x2035
#define PIN_MUX_REG_10	  0x2036
#define PIN_MUX_REG_11	  0x2037
#define PIN_MUX_REG_12	  0x2038

static unsigned pin_mux_reg_addr[] = {
	CBUS_REG_ADDR(PIN_MUX_REG_0),
	CBUS_REG_ADDR(PIN_MUX_REG_1),
	CBUS_REG_ADDR(PIN_MUX_REG_2),
	CBUS_REG_ADDR(PIN_MUX_REG_3),
	CBUS_REG_ADDR(PIN_MUX_REG_4),
	CBUS_REG_ADDR(PIN_MUX_REG_5),
	CBUS_REG_ADDR(PIN_MUX_REG_6),
	CBUS_REG_ADDR(PIN_MUX_REG_7),
	CBUS_REG_ADDR(PIN_MUX_REG_8),
	CBUS_REG_ADDR(PIN_MUX_REG_9),
	CBUS_REG_ADDR(PIN_MUX_REG_10),
	CBUS_REG_ADDR(PIN_MUX_REG_11),
	CBUS_REG_ADDR(PIN_MUX_REG_12),
};

static DEFINE_SPINLOCK(lock);
static uint32_t pimux_locktable[PIN_MUX_REG_NUM];


pinmux_set_t *pinmux_cacl(char *str)
{
	/* @todo pinmux_cacl */
	return NULL;
}
EXPORT_SYMBOL(pinmux_cacl);

int32_t pinmux_set(pinmux_set_t * pinmux)
{
	uint32_t locallock[PIN_MUX_REG_NUM];
	uint32_t reg,value,conflict,dest_value;
	ulong flags;
	int i;

	if (pinmux==NULL)
		return -4;
	memset(locallock, 0, sizeof(locallock));

	/* check lock table */
	for (i=0; pinmux->pinmux[i].reg != PINMUX_END_VOID; i++)
	{
		reg = pinmux->pinmux[i].reg;
		locallock[reg] = pinmux->pinmux[i].clrmask | pinmux->pinmux[i].setmask;
		dest_value = pinmux->pinmux[i].setmask;

		conflict = locallock[reg] & pimux_locktable[reg];
		if (conflict)
		{
		value = readl(pin_mux_reg_addr[reg])&conflict;
		dest_value &= conflict;
			if (value != dest_value)
			{
				printk(KERN_ERR "set fail , detect locktable conflict\n");
				return -1;///lock fail some pin is locked by others
			}
		}
	}
	/**/
	if (pinmux->chip_select!=NULL )
	{
		if(pinmux->chip_select(true)==false){
			printk(KERN_ERR "failed to chip_select\n");
			return -3;///@select chip fail;
		}
	}

	spin_lock_irqsave(&lock, flags);
	for (i = 0; pinmux->pinmux[i].reg != PINMUX_END_VOID; i++)
	{
		printk(KERN_INFO "clrsetbits %08x %08x %08x \n",
		pin_mux_reg_addr[pinmux->pinmux[i].reg],
		pinmux->pinmux[i].clrmask,
		pinmux->pinmux[i].setmask);
		pimux_locktable[pinmux->pinmux[i].reg] |= locallock[pinmux->pinmux[i].reg];
		clrsetbits_le32(pin_mux_reg_addr[pinmux->pinmux[i].reg],
		pinmux->pinmux[i].clrmask,
		pinmux->pinmux[i].setmask);
	}
	spin_unlock_irqrestore(&lock, flags);
	return 0;
}
EXPORT_SYMBOL(pinmux_set);


int32_t pinmux_clr(pinmux_set_t * pinmux)
{
	ulong flags;
	int i;

	if (!pinmux)
		return -4;

	if (!pinmux->chip_select)
		return 0;
	pinmux->chip_select(false);
	printk(KERN_INFO "pinmux_clr : %p" ,pinmux->pinmux);

	spin_lock_irqsave(&lock, flags);
	for (i = 0; pinmux->pinmux[i].reg != PINMUX_END_VOID; i++)
	{
		pimux_locktable[pinmux->pinmux[i].reg] &= ~(pinmux->pinmux[i].clrmask | pinmux->pinmux[i].setmask);
	}

	for (i = 0; pinmux->pinmux[i].reg != PINMUX_END_VOID; i++)
	{
		printk(KERN_INFO "clrsetbits %x %x %x",
			pin_mux_reg_addr[pinmux->pinmux[i].reg],
			pinmux->pinmux[i].setmask | pinmux->pinmux[i].clrmask,
			pinmux->pinmux[i].clrmask);
		clrsetbits_le32(pin_mux_reg_addr[pinmux->pinmux[i].reg],
			pinmux->pinmux[i].setmask | pinmux->pinmux[i].clrmask,
			pinmux->pinmux[i].clrmask);
	}
	spin_unlock_irqrestore(&lock, flags);
	return 0;
}
EXPORT_SYMBOL(pinmux_clr);

