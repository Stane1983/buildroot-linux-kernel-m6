/*
 * arch/arm/mach-meson2/include/mach/sleep.h
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

#ifndef __MACH_MESON2_SLEEP_H
#define __MACH_MESON2_SLEEP_H

#define ADJUST_CORE_VOLTAGE
#define TURN_OFF_DDR_PLL
//#define SAVE_DDR_REGS
//#define SYSTEM_16K

#ifdef SYSTEM_16K
#define VOLTAGE_DLY     0x400
#define MS_DLY          0x200
#else // 24M/128
#define VOLTAGE_DLY     0x4000
#define MS_DLY          0x2000
#endif

#endif /* __MACH_MESON2_SLEEP_H */
