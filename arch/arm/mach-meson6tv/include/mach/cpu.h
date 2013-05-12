/*
 * cpu.h
 *
 *  Created on: OCT 12, 2012
 */

#ifndef __MACH_MESON6TV_CPU_H_
#define __MACH_MESON6TV_CPU_H_

#include <plat/cpu.h>

extern int (*get_cpu_temperature_celius)(void);
int get_cpu_temperature(void);

#define MESON_CPU_TYPE	MESON_CPU_TYPE_MESON6TV

#endif /* __MACH_MESON6TV_CPU_H_ */
