/*
 * arch/arm/mach-meson2/include/mach/map.h
 *
 * Copyright (C) 2012 Amlogic, Inc.
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

#ifndef __MACH_MESON2_MAP_H
#define __MACH_MESON2_MAP_H

#include <mach/regs.h>

#define CPU_0_IRQ_IN0_INTR_MASK SYS_CPU_0_IRQ_IN0_INTR_MASK
#define CPU_0_IRQ_IN1_INTR_MASK SYS_CPU_0_IRQ_IN1_INTR_MASK
#define CPU_0_IRQ_IN2_INTR_MASK SYS_CPU_0_IRQ_IN2_INTR_MASK
#define CPU_0_IRQ_IN3_INTR_MASK SYS_CPU_0_IRQ_IN3_INTR_MASK

#define CPU_0_IRQ_IN0_INTR_STAT_CLR SYS_CPU_0_IRQ_IN0_INTR_STAT_CLR
#define CPU_0_IRQ_IN1_INTR_STAT_CLR SYS_CPU_0_IRQ_IN1_INTR_STAT_CLR
#define CPU_0_IRQ_IN2_INTR_STAT_CLR SYS_CPU_0_IRQ_IN2_INTR_STAT_CLR
#define CPU_0_IRQ_IN3_INTR_STAT_CLR SYS_CPU_0_IRQ_IN3_INTR_STAT_CLR

#define CPU_0_IRQ_IN0_INTR_FIRQ_SEL SYS_CPU_0_IRQ_IN0_INTR_FIRQ_SEL
#define CPU_0_IRQ_IN1_INTR_FIRQ_SEL SYS_CPU_0_IRQ_IN1_INTR_FIRQ_SEL
#define CPU_0_IRQ_IN2_INTR_FIRQ_SEL SYS_CPU_0_IRQ_IN2_INTR_FIRQ_SEL
#define CPU_0_IRQ_IN3_INTR_FIRQ_SEL SYS_CPU_0_IRQ_IN3_INTR_FIRQ_SEL


#define SRAM_PHY_ADDR   IO_AHB_BUS_PHY_BASE

#endif /* __MACH_MESON2_MAP_H */
