/*
 * arch/arm/mach-meson6tv/board-m6tv-ref.h
 *
 * Copyright (C) 2012 Amlogic, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef __MACH_MESON6TV_BOARD_M6TVREF_H
#define __MACH_MESON6TV_BOARD_M6TVREF_H

#include <asm/page.h>


/***********************************************************************
 * IO Mapping
 **********************************************************************/
#define PHYS_MEM_START      (0x80000000)
#define PHYS_MEM_SIZE       (512*SZ_1M)
#define PHYS_MEM_END        (PHYS_MEM_START + PHYS_MEM_SIZE -1 )

/******** Reserved memory setting ************************/
#define RESERVED_MEM_START  (0x80000000+64*SZ_1M)   /*start at the second 64M*/

/******** CODEC memory setting ************************/
//  Codec need 16M for 1080p decode
//  4M for sd decode;
#define ALIGN_MSK       ((SZ_1M)-1)
#define U_ALIGN(x)      ((x+ALIGN_MSK)&(~ALIGN_MSK))
#define D_ALIGN(x)      ((x)&(~ALIGN_MSK))

/******** AUDIODSP memory setting ************************/
#define AUDIODSP_ADDR_START     U_ALIGN(RESERVED_MEM_START) /*audiodsp memstart*/
#define AUDIODSP_ADDR_END   (AUDIODSP_ADDR_START+SZ_1M-1)   /*audiodsp memend*/

/******** Frame buffer memory configuration ***********/
#define OSD_480_PIX     (640*480)
#define OSD_576_PIX     (768*576)
#define OSD_720_PIX     (1280*720)
#define OSD_1080_PIX        (1920*1080)
#define OSD_PANEL_PIX       (800*480)
#define B16BpP          (2)
#define B32BpP          (4)
#define DOUBLE_BUFFER       (2)

#define OSD1_MAX_MEM        U_ALIGN(OSD_1080_PIX*B32BpP*DOUBLE_BUFFER)
#define OSD2_MAX_MEM        U_ALIGN(32*32*B32BpP)

/******** Reserved memory configuration ***************/
#define OSD1_ADDR_START     U_ALIGN(AUDIODSP_ADDR_END )
#define OSD1_ADDR_END       (OSD1_ADDR_START+OSD1_MAX_MEM - 1)
#define OSD2_ADDR_START     U_ALIGN(OSD1_ADDR_END)
#define OSD2_ADDR_END       (OSD2_ADDR_START +OSD2_MAX_MEM -1)

#if defined(CONFIG_AM_VDEC_H264)
#define CODEC_MEM_SIZE      U_ALIGN(32*SZ_1M)
#else
#define CODEC_MEM_SIZE      U_ALIGN(16*SZ_1M)
#endif
#define CODEC_ADDR_START    U_ALIGN(OSD2_ADDR_END)
#define CODEC_ADDR_END      (CODEC_ADDR_START+CODEC_MEM_SIZE-1)

/********VDIN Memory Configuration ***************/
#define TVIN_ADDR_START     (U_ALIGN(CODEC_ADDR_END))

#define VDIN_ADDR_START     (TVIN_ADDR_START)

#define VDIN0_MEM_SIZE      (U_ALIGN(36*SZ_1M))
#define VDIN0_ADDR_START    (U_ALIGN(VDIN_ADDR_START))
#define VDIN0_ADDR_END      (VDIN0_ADDR_START + VDIN0_MEM_SIZE - 1)

#define VDIN1_MEM_SIZE      (U_ALIGN(24*SZ_1M))
#define VDIN1_ADDR_START    (U_ALIGN(VDIN0_ADDR_END))
#define VDIN1_ADDR_END      (VDIN1_ADDR_START + VDIN1_MEM_SIZE - 1)

#define VDIN_ADDR_END       (VDIN1_ADDR_END)

#define TVAFE_MEM_SIZE      (U_ALIGN(5*SZ_1M))
#define TVAFE_ADDR_START    (U_ALIGN(VDIN_ADDR_END))
#define TVAFE_ADDR_END      (TVAFE_ADDR_START + TVAFE_MEM_SIZE - 1)

#define VDIN_MEM_SIZE       (VDIN0_MEM_SIZE + VDIN1_MEM_SIZE)
#define TVIN_ADDR_END       (TVAFE_ADDR_END)

#if defined(CONFIG_AMLOGIC_VIDEOIN_MANAGER)
#define VM_SIZE         (SZ_1M*16)
#else
#define VM_SIZE         (1)
#endif /* CONFIG_AMLOGIC_VIDEOIN_MANAGER  */

#define VM_ADDR_START       U_ALIGN(TVIN_ADDR_END)
#define VM_ADDR_END     (VM_SIZE + VM_ADDR_START - 1)

#if defined(CONFIG_AM_DEINTERLACE_SD_ONLY)
#define DI_MEM_SIZE     (SZ_1M*3)
#else
#define DI_MEM_SIZE     (SZ_1M*15)
#endif
#define DI_ADDR_START       U_ALIGN(VM_ADDR_END)
#define DI_ADDR_END     (DI_ADDR_START+DI_MEM_SIZE-1)

#ifdef CONFIG_POST_PROCESS_MANAGER
#ifdef CONFIG_POST_PROCESS_MANAGER_PPSCALER
#define PPMGR_MEM_SIZE      1920 * 1088 * 21
#else
#define PPMGR_MEM_SIZE      1920 * 1088 * 15
#endif
#else
#define PPMGR_MEM_SIZE      0
#endif /* CONFIG_POST_PROCESS_MANAGER */

#define PPMGR_ADDR_START    U_ALIGN(DI_ADDR_END)
#define PPMGR_ADDR_END      (PPMGR_ADDR_START+PPMGR_MEM_SIZE-1)

#define STREAMBUF_MEM_SIZE  (SZ_1M*7)
#define STREAMBUF_ADDR_START    U_ALIGN(PPMGR_ADDR_END)
#define STREAMBUF_ADDR_END  (STREAMBUF_ADDR_START+STREAMBUF_MEM_SIZE-1)

#ifdef CONFIG_D2D3_PROCESS
#define D2D3_MEM_SIZE                   (SZ_1M*2)
#else
#define D2D3_MEM_SIZE                   0
#endif
#define D2D3_ADDR_START                 U_ALIGN(STREAMBUF_ADDR_END)
#define D2D3_ADDR_END                   (D2D3_ADDR_START+D2D3_MEM_SIZE-1)
//64m for demod
#define DEMODBUF_MEM_SIZE	(SZ_1M*64)
#define DEMODBUF_ADDR_START	U_ALIGN(D2D3_ADDR_END)
#define DEMODBUF_ADDR_END	(DEMODBUF_ADDR_START+DEMODBUF_MEM_SIZE-1)

#define RESERVED_MEM_END    (DEMODBUF_ADDR_END)

int __init m6tvref_lcd_init(void);
int __init m6tvref_power_init(void);
int __init m6tvref_tvin_init(void);

#endif // __MACH_MESON6TV_BOARD_M6TVREF_H

