/*
 * AMLOGIC Canvas management driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the named License,
 * or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
 *
 * Author:  Tim Yao <timyao@amlogic.com>
 *
 */

#ifndef CANVAS_H
#define CANVAS_H

#include <linux/types.h>
#include <linux/kobject.h>

#include <mach/cpu.h>

typedef struct {
    struct kobject kobj;
    ulong addr;
    u32 width;
    u32 height;
    u32 wrap;
    u32 blkmode;
} canvas_t;

#define OSD1_CANVAS_INDEX 0x40
#define OSD2_CANVAS_INDEX 0x43
#define OSD3_CANVAS_INDEX 0x41
#define OSD4_CANVAS_INDEX 0x42
#define ALLOC_CANVAS_INDEX  0x46

#define GE2D_MAX_CANVAS_INDEX   0x5f

#define DISPLAY_CANVAS_BASE_INDEX   0x60
#define DISPLAY_CANVAS_MAX_INDEX    0x65

#define DISPLAY2_CANVAS_BASE_INDEX   0x66
#define DISPLAY2_CANVAS_MAX_INDEX    0x6b

/*here ppmgr share the same canvas with deinterlace and mipi driver for m6*/
#define PPMGR_CANVAS_INDEX 0x70
#define PPMGR_DOUBLE_CANVAS_INDEX 0x74  //for double canvas use
#define PPMGR_DEINTERLACE_BUF_CANVAS 0x77   /*for progressive mjpeg use*/

#define FREESCALE_CANVAS_INDEX 0x50   //for osd&video scale use
#define MAX_FREESCALE_CANVAS_INDEX 0x5f

#define PPMGR_DEINTERLACE_BUF_NV21_CANVAS 0x7a   /*for progressive mjpeg (nv21 output)use*/

#define DI_USE_FIXED_CANVAS_IDX
#ifdef DI_USE_FIXED_CANVAS_IDX
#define DI_PRE_MEM_NR_CANVAS_IDX        0x70
#define DI_PRE_CHAN2_NR_CANVAS_IDX      0x71
#define DI_PRE_WR_NR_CANVAS_IDX         0x72
#define DI_PRE_WR_MTN_CANVAS_IDX        0x73
//NEW DI
#define DI_CONTPRD_CANVAS_IDX           0x74
#define DI_CONTP2RD_CANVAS_IDX           0x75
#define DI_CONTWR_CANVAS_IDX            0x76
//DI POST, share with DISPLAY
#define DI_POST_BUF0_CANVAS_IDX         0x60
#define DI_POST_BUF1_CANVAS_IDX         0x61
#define DI_POST_MTNCRD_CANVAS_IDX       0x62
#define DI_POST_MTNPRD_CANVAS_IDX       0x63
#else
#define DEINTERLACE_CANVAS_BASE_INDEX	0x70
#define DEINTERLACE_CANVAS_MAX_INDEX	0x7f
#endif

#define MIPI_CANVAS_INDEX 0x70
#define MIPI_CANVAS_MAX_INDEX 0x7f

#define VDIN1_CANVAS_INDEX 0xC0
#define VDIN1_CANVAS_MAX_INDEX 0xCF

#define AMLVIDEO2_RES_CANVAS 0xD0
#define AMLVIDEO2_MAX_RES_CANVAS 0xD7

#define AMVENC_CANVAS_INDEX 0xD8
#define AMVENC_CANVAS_MAX_INDEX 0xDD

extern void canvas_config(u32 index, ulong addr, u32 width,
                          u32 height, u32 wrap, u32 blkmode);

extern void canvas_read(u32 index, canvas_t *p);

extern void canvas_copy(unsigned src, unsigned dst);

extern void canvas_update_addr(u32 index, u32 addr);

extern unsigned int canvas_get_addr(u32 index);

#if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON6
// TODO: move to register headers
#define CANVAS_ADDR_NOWRAP      0x00
#define CANVAS_ADDR_WRAPX       0x01
#define CANVAS_ADDR_WRAPY       0x02
#define CANVAS_BLKMODE_MASK     3
#define CANVAS_BLKMODE_BIT      24
#define CANVAS_BLKMODE_LINEAR   0x00
#define CANVAS_BLKMODE_32X32    0x01
#define CANVAS_BLKMODE_64X32    0x02

#endif

#endif /* CANVAS_H */
