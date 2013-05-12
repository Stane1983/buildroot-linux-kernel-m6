/*
 *
 * arch/arm/plat-meson/include/plat/codec.h
 *
 *  Copyright (C) 2010 AMLOGIC, INC.
 *
 * License terms: GNU General Public License (GPL) version 2
 * Basic platform init and mapping functions.
 */
#ifndef _PLAT_DEV_CODEC_H
#define _PLAT_DEV_CODEC_H

 
extern struct platform_device meson_device_codec ; 
extern uint32_t   setup_codec_resource(struct resource * res,char res_num);
#endif
