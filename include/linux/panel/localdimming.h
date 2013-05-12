/*
 * Local Dimming
 *
 * Author: Bobby Yang <bo.yang@amlogic.com>
 *
 * Copyright (C) 2012 Amlogic Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __AML_LOCAL_DIMMING_H
#define __AML_LOCAL_DIMMING_H

#include <linux/notifier.h>

#define LD_LUMA_INIT		0x0001
#define LD_LUMA_UPDATE		0x0010

extern int ld_register_notifier(struct notifier_block *nb);
extern int ld_unregister_notifier(struct notifier_block *nb);
extern int ld_notifier_call_chain(unsigned long val, void *v);


#endif /* __AML_LOCAL_DIMMING_H */
