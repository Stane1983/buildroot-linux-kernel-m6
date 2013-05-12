/*
 * Silicon Image SiI1292 Device Driver
 *
 * Author: Amlogic, Inc.
 *
 * Copyright (C) 2012 Amlogic Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#include <linux/jiffies.h>

#include "../hal/si_hal.h"


bool_t HalTimerDelay(uint32_t baseTime, uint32_t delay)
{
	unsigned long timeout = baseTime + (delay*HZ)/1000;

	if ( time_before(jiffies, timeout) )
		return false;
	else
		return true;
}

