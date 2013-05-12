/*
 *
 * arch/arm/plat-meson/include/plat/fb.h
 *
 *  Copyright (C) 2010 AMLOGIC, INC.
 *
 * License terms: GNU General Public License (GPL) version 2
 * Basic platform init and mapping functions.
 */
#ifndef _PLAT_DEV_FB_EXT_H
#define _PLAT_DEV_FB_EXT_H

extern struct platform_device meson_device_fb_ext;
extern uint32_t setup_fb_ext_resource(struct resource *res, char res_num);
#endif
