/*
 * arch/arm/mach-meson2/board-m2-skt.c
 *
 * Copyright (C) 2010 Amlogic, Inc.
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

#ifndef __BOARD_M2_REF_H
#define __BOARD_M2_REF_H

#include <asm/page.h>

#define PHYS_MEM_START          (0x80000000)
#define PHYS_MEM_SIZE           (512*SZ_1M)
#define PHYS_MEM_END            (PHYS_MEM_START + PHYS_MEM_SIZE -1 )

/*** Reserved memory setting ***/
#define RESERVED_MEM_START      (PHYS_MEM_START+64*SZ_1M)   /*start at the second 64M*/

#define ALIGN_MSK               ((SZ_1M)-1)
#define U_ALIGN(x)              ((x+ALIGN_MSK)&(~ALIGN_MSK))
#define D_ALIGN(x)              ((x)&(~ALIGN_MSK))

/*** AUDIODSP memory setting ***/
#define AUDIODSP_ADDR_START     U_ALIGN(RESERVED_MEM_START)
#define AUDIODSP_ADDR_SIZE      (1*SZ_1M)
#define AUDIODSP_ADDR_END       (AUDIODSP_ADDR_START+AUDIODSP_ADDR_SIZE-1)

/*** Frame buffer memory configuration ***/
#define OSD_480_PIX             (640*480)
#define OSD_576_PIX             (768*576)
#define OSD_720_PIX             (1280*720)
#define OSD_1080_PIX            (1920*1080)
#define OSD_PANEL_PIX           (OSD_1080_PIX)
#define B16BpP                  (2)
#define B32BpP                  (4)
#define DOUBLE_BUFFER           (2)
#define TRIPLE_BUFFER           (3)
#define OSD1_MAX_MEM            U_ALIGN(OSD_PANEL_PIX*B32BpP*DOUBLE_BUFFER)
#define OSD2_MAX_MEM            U_ALIGN(32*32*B16BpP)

#define OSD1_ADDR_START         U_ALIGN(AUDIODSP_ADDR_END )
#define OSD1_ADDR_END           (OSD1_ADDR_START + OSD1_MAX_MEM - 1)
#define OSD2_ADDR_START         U_ALIGN(OSD1_ADDR_END)
#define OSD2_ADDR_END           (OSD2_ADDR_START + OSD2_MAX_MEM - 1)

/*** Stream buffer ***/
#define STREAMBUF_MEM_SIZE      (SZ_1M*10)
#define STREAMBUF_ADDR_START    U_ALIGN(OSD2_ADDR_END)
#define STREAMBUF_ADDR_END      (STREAMBUF_ADDR_START+STREAMBUF_MEM_SIZE-1)

/*** TVIN memory ***/
#define VDIN_ADDR_START         U_ALIGN(STREAMBUF_ADDR_END)
#define VDIN0_MEM_SIZE          U_ALIGN(27*SZ_1M)
#define VDIN1_MEM_SIZE          (1)

#define VDIN0_ADDR_START        U_ALIGN(VDIN_ADDR_START)
#define VDIN0_ADDR_END          (VDIN0_ADDR_START + VDIN0_MEM_SIZE - 1)
#define VDIN1_ADDR_START        U_ALIGN(VDIN0_ADDR_END)
#define VDIN1_ADDR_END          (VDIN1_ADDR_START + VDIN1_MEM_SIZE - 1)

#define VDIN_ADDR_END           (VDIN1_ADDR_END)
#define VDIN_MEM_SIZE           (VDIN_ADDR_END - VDIN_ADDR_START + 1)

/*** For video to keep buffer ***/
#define VIDEO_ADDR_START        U_ALIGN(VDIN_ADDR_END)

#define VIDEO_Y_ADDR_SIZE       (SZ_1M*8)
#define VIDEO_Y_ADDR_START      U_ALIGN(VIDEO_ADDR_START)
#define VIDEO_Y_ADDR_END        (VIDEO_Y_ADDR_START + VIDEO_Y_ADDR_SIZE - 1)

#define VIDEO_U_ADDR_SIZE       (SZ_512K*1)
#define VIDEO_U_ADDR_START      U_ALIGN(VIDEO_Y_ADDR_END)
#define VIDEO_U_ADDR_END        (VIDEO_U_ADDR_START + VIDEO_U_ADDR_SIZE - 1)

#define VIDEO_V_ADDR_SIZE       (SZ_512K*1)
#define VIDEO_V_ADDR_START      U_ALIGN(VIDEO_U_ADDR_END)
#define VIDEO_V_ADDR_END        (VIDEO_V_ADDR_START + VIDEO_V_ADDR_SIZE - 1)

#define VIDEO_ADDR_END          (VIDEO_V_ADDR_END)
#define VIDEO_MEM_SIZE          (VIDEO_ADDR_END - VIDEO_ADDR_START + 1)

/*** for AFE ***/
#define TVAFE_MEM_SIZE          U_ALIGN(5*SZ_1M)
#define TVAFE_ADDR_START        U_ALIGN(VIDEO_ADDR_END)
#define TVAFE_ADDR_END          (TVAFE_ADDR_START + TVAFE_MEM_SIZE - 1)

/*** CODEC memory setting ***/
#define CODEC_ADDR_START        U_ALIGN(STREAMBUF_ADDR_END)
#if defined(CONFIG_AM_VDEC_H264)
#define CODEC_MEM_SIZE          U_ALIGN(64*SZ_1M)
#else
#define CODEC_MEM_SIZE          U_ALIGN(16*SZ_1M)
#endif
// shared with the total of vdin0+vdin1+yuv+tvafe
#if (CODEC_MEM_SIZE > (VDIN_MEM_SIZE + VIDEO_MEM_SIZE + TVAFE_MEM_SIZE))
#define CODEC_ADDR_END          (CODEC_ADDR_START + CODEC_MEM_SIZE - 1)
#else
#define CODEC_ADDR_END          (CODEC_ADDR_START + VDIN_MEM_SIZE + VIDEO_MEM_SIZE + TVAFE_MEM_SIZE - 1)
#endif


/*** for PPMGR ***/
#define PPMGR_MEM_SIZE          U_ALIGN(26*SZ_1M)
#define PPMGR_ADDR_START        U_ALIGN(CODEC_ADDR_END)
#define PPMGR_ADDR_END          (PPMGR_MEM_SIZE+PPMGR_ADDR_START-1)

/*** for FREESCALE ***/
// shared with ppmgr memory
#define FREESCALE_MEM_SIZE      (PPMGR_MEM_SIZE)
#define FREESCALE_ADDR_START    (PPMGR_ADDR_START)
#define FREESCALE_ADDR_END      (PPMGR_ADDR_END)

/*** for DI ***/
// shared with ppmgr memory
#define DI_MEM_SIZE             (FREESCALE_MEM_SIZE)
#define DI_ADDR_START           (FREESCALE_ADDR_START)
#define DI_ADDR_END             (FREESCALE_ADDR_END)

#define RESERVED_MEM_END        (FREESCALE_ADDR_END)

#endif //__BOARD_M2_REF_H