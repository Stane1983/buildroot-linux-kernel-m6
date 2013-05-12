/*
 *
 * arch/arm/plat-meson/include/plat/plat_pinmux.h
 *
 *  Copyright (C) 2010 AMLOGIC, INC.
 *
 * License terms: GNU General Public License (GPL) version 2
 * Basic platform init and mapping functions.
 */
 #ifndef  PLAT_PINMUX_H
 #define PLAT_PINMUX_H
 #include <mach/pinmux.h>
 #include <plat/platform_data.h>
 static plat_data_public_t  *cur_platdata;
static  int32_t  plat_pinmux_setup(void)
{
    return pinmux_set(&cur_platdata->pinmux_set);
}
static int32_t   plat_pinmux_clear(void)
{
    return pinmux_clr(&cur_platdata->pinmux_set);
}
static inline void plat_set_default_pinmux(plat_data_public_t *npd)
{
    if(NULL==npd) return ;
     cur_platdata=npd;
     if((NULL==cur_platdata->pinmux_cfg.setup)||(NULL==cur_platdata->pinmux_cfg.clear))
     {
            cur_platdata->pinmux_cfg.setup=plat_pinmux_setup;
            cur_platdata->pinmux_cfg.clear=plat_pinmux_clear;
     }
}
#endif 
