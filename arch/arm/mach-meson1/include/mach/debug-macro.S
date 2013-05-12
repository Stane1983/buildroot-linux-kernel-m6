/* arch/arm/mach-meson/include/mach/debug-macro.S
 *
 * Copyright (C) 2011 Amlogic, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */



#include <mach/hardware.h>
#include <mach/am_regs.h>

	.macro	addruart, rp, rv
	ldr	\rp, =MESON_PERIPHS1_PHYS_BASE
	ldr	\rv, =MESON_PERIPHS1_VIRT_BASE
	.endm

	.macro	senduart, rd, rx
	teq	\rx, #0
	strne	\rd, [\rx]
	.endm

	.macro	waituart, rd, rx
	@ wait for TX_READY
1001:	ldr	\rd, [\rx, #0x0C]
	tst	\rd, #1 << 21  @ WFIFO FULL
	bne	1001b
	.endm

	.macro	busyuart, rd, rx
2001:	ldr	\rd, [\rx, #0x0C]
	tst	\rd, #1 << 23  @ CTS
	bne	2001b
	.endm
