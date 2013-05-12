/*
 * VDAC SWITCH definitions
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __AML_VDAC_SWITCH_H__
#define __AML_VDAC_SWITCH_H__

#include <linux/types.h>

enum aml_vdac_switch_type {
	VOUT_CVBS,
	VOUT_COMPONENT,
	VOUT_VGA,
	VOUT_MAX
};

struct aml_vdac_switch_platform_data {

	void (*vdac_switch_change_type_func)(unsigned type);
};

#endif
