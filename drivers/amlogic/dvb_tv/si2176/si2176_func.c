/*
 * Silicon labs si2176 Tuner Device Driver
 *
 * Author: Bobby Yang <bo.yang@amlogic.com>
 *
 *
 * Copyright (C) 2010 Amlogic Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


/* Standard Liniux Headers */
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/delay.h>

/* Local Headers */
#include "si2176_func.h"


#define I2C_TRY_MAX_CNT       3  //max try counter

static unsigned int si2176_version = 29; //the old available firmware is 42
static unsigned int delay_det=85;
static unsigned int cvbs_amp=110;
module_param(si2176_version, uint, 0664);
MODULE_PARM_DESC(si2176_version, "\n si2176_version \n");
module_param(delay_det, uint, 0644);
MODULE_PARM_DESC(delay_det, "delay set freq time\n");
module_param(cvbs_amp,uint,0644);
MODULE_PARM_DESC(cvbs_amp, "\n si2176_cvbs_amp_out \n");

static int __init si2176_tuner_parse_param(char *str)
{
        unsigned char *ptr = str;
        printk("[si2176..] %s bootargs is %s.\n",__func__,str);
        if(strstr(ptr,"b29"))
                si2176_version = 29;
        else if(strstr(ptr,"b30"))
                si2176_version = 30;
        else
                return -1;
        return 0;
}
__setup("tuner=",si2176_tuner_parse_param);
/* for lg tuner type */
//b29 is the 3.2B7,it  support the sound_level 
static unsigned char firmwaretable_b29[] = {
        0x04,0x01,0x01,0x00,0xEE,0xA4,0x94,0x17,
        0x07,0x05,0x99,0x31,0x5B,0xB5,0x2D,0x90,
        0x0F,0x76,0x41,0x39,0xE7,0x17,0xF3,0xA5,
        0x05,0xBC,0x8B,0x61,0x0A,0xF1,0x3F,0x10,
        0x22,0x52,0x13,0xFD,0xB0,0x7F,0x05,0x53,
        0x05,0x01,0x7A,0x86,0x58,0x91,0xD6,0xA4,
        0x22,0xC4,0xB5,0x5C,0xB8,0xD3,0x11,0xDE,
        0x05,0x20,0x20,0x2A,0x43,0x44,0x4F,0x28,
        0x22,0x87,0x6D,0x1E,0x90,0xE2,0xC6,0x02,
        0x05,0x13,0x29,0x80,0x96,0x0D,0x29,0x15,
        0x2A,0x7C,0x0C,0xA6,0x50,0xA0,0x62,0xD1,
        0x05,0xCA,0x09,0xC0,0xA4,0x9D,0x4C,0x68,
        0x2A,0xA6,0xC8,0x75,0x64,0x76,0x6D,0xC8,
        0x05,0x81,0x1D,0x07,0x71,0x53,0x14,0xAC,
        0x22,0x7B,0x4D,0x6D,0x86,0x15,0xEA,0xD0,
        0x05,0x05,0xC3,0x11,0xEF,0x16,0x70,0x74,
        0x2A,0x98,0xD8,0xBF,0xAF,0xC9,0xDA,0xE6,
        0x05,0x7D,0xE7,0xA5,0x3F,0xA6,0x52,0xBC,
        0x2F,0x1D,0x03,0x92,0x90,0x97,0xE5,0x39,
        0x29,0x85,0x25,0xD5,0xF4,0xE8,0xED,0x7F,
        0x0F,0xB0,0xB7,0xD4,0xEB,0x5B,0x69,0xD8,
        0x05,0xDD,0x8B,0xFB,0xD2,0xD6,0x6F,0x0D,
        0x22,0x4D,0xBE,0xBB,0xF9,0xAA,0x15,0x9B,
        0x0F,0xB4,0x87,0xC2,0xDB,0x5C,0x7D,0xDB,
        0x22,0xDD,0x93,0xED,0x25,0x7A,0xFC,0xB7,
        0x05,0xE5,0xB1,0x9C,0xA4,0xC5,0x3E,0x3A,
        0x27,0x6E,0x5F,0x8B,0xDF,0x77,0x16,0x1A,
        0x27,0x89,0x03,0xDC,0x00,0x16,0x61,0xA0,
        0x2F,0x05,0x92,0xA0,0x54,0xCC,0x06,0x96,
        0x27,0x3F,0x94,0x85,0x36,0x55,0x40,0x85,
        0x27,0xD2,0x2D,0x3E,0xAD,0x44,0x3D,0xFB,
        0x2F,0x5B,0x25,0xF1,0xC5,0xD8,0x8B,0x7D,
        0x27,0x55,0x85,0xAC,0x60,0x06,0xD4,0x22,
        0x2F,0xB2,0x15,0x29,0xE3,0x5D,0xEE,0x3F,
        0x2F,0x57,0xC6,0x33,0xA3,0xDA,0xED,0xF8,
        0x27,0x00,0x45,0x66,0x8C,0x0B,0xB2,0xC6,
        0x2F,0x66,0x5A,0xC7,0xEF,0x78,0x6B,0xB0,
        0x2F,0x88,0xB4,0x2B,0xD9,0x93,0x22,0xFD,
        0x27,0x86,0x29,0x2E,0xEA,0x16,0xB6,0x8D,
        0x2F,0x65,0x8F,0x33,0xCF,0xB7,0xEE,0x70,
        0x27,0xB2,0xD1,0xB5,0x3D,0xDE,0x07,0x4E,
        0x27,0xBB,0x13,0xAC,0xF8,0xBC,0xAD,0x02,
        0x2E,0x87,0x86,0xBD,0x81,0x07,0xB9,0xBD,
        0x07,0x6F,0x55,0x74,0x26,0x02,0x87,0xD3,
        0x2F,0xE4,0x16,0x64,0xC7,0xA0,0xCA,0x0E,
        0x27,0xF3,0x00,0xEC,0x26,0xD5,0xB0,0x52,
        0x2F,0x01,0xB2,0x7E,0xDF,0x2E,0x08,0x1F,
        0x0F,0xDD,0xF7,0x94,0x2F,0xAE,0xE3,0xBE,
        0x27,0x65,0xC4,0x5B,0x02,0xCB,0x22,0xE8,
        0x2F,0x60,0x59,0xA1,0x48,0xE8,0xAF,0xD8,
        0x2F,0xC3,0xA1,0x9F,0x82,0xD6,0x5F,0x30,
        0x27,0xCF,0x75,0x5F,0x3F,0x63,0xF2,0x84,
        0x2F,0x9E,0xEF,0xD3,0x4D,0x50,0xB2,0xC5,
        0x27,0x4D,0xAA,0x58,0x3C,0x0C,0x8C,0x1B,
        0x2F,0xE7,0x86,0xC6,0x35,0x71,0x8F,0x45,
        0x27,0x0C,0x6A,0xE7,0x36,0xEA,0xC6,0x1B,
        0x2F,0x18,0x86,0x76,0xD8,0x5E,0x90,0x29,
        0x21,0x30,0x30,0x4D,0xF8,0xA9,0x19,0x3A,
        0x0F,0x58,0xFF,0x4F,0x6A,0x07,0xE0,0x4D,
        0x27,0x14,0x14,0x07,0x3A,0x86,0x98,0xDD,
        0x2F,0x3C,0x67,0x4B,0x2A,0x4C,0x96,0xA2,
        0x2F,0xF4,0x54,0x6C,0xDD,0xF6,0x5A,0x37,
        0x27,0x80,0x77,0x8D,0x5C,0x8B,0xFF,0xAA,
        0x2F,0x6F,0x2C,0x2D,0x2D,0x6A,0x90,0xFB,
        0x2F,0x56,0x56,0x07,0x28,0xBB,0x3C,0xCF,
        0x27,0x10,0xEE,0x04,0x36,0x67,0x61,0x53,
        0x27,0xC1,0x8A,0x35,0xFB,0xB2,0xA6,0xF2,
        0x27,0x74,0xD0,0x1E,0x66,0x38,0xB6,0x6A,
        0x27,0x87,0x65,0xF9,0xF7,0x3C,0xC4,0x7B,
        0x27,0x6E,0xCA,0x12,0xD8,0x5C,0x41,0x23,
        0x2F,0x8D,0xA6,0x2E,0xAF,0xFF,0x18,0x0A,
        0x2F,0xB7,0xEC,0xE7,0xFD,0xCA,0x91,0x37,
        0x27,0x39,0xE0,0x6E,0x20,0x25,0xA3,0x77,
        0x2F,0x4E,0xDC,0x4E,0xB6,0x00,0x40,0x3E,
        0x22,0xC9,0xA3,0x33,0xB2,0x64,0xBE,0xAA,
        0x0F,0xBB,0xC1,0x23,0x71,0x27,0xDD,0xC6,
        0x0F,0xC4,0xEE,0xA9,0xBD,0x0C,0x8E,0x7F,
        0x07,0x0C,0xE5,0xC9,0xE5,0x44,0x76,0xA8,
        0x22,0x6C,0xB6,0x00,0x41,0xB8,0x7E,0x92,
        0x07,0x60,0x69,0xB8,0x5E,0x61,0xAE,0x69,
        0x0F,0xF2,0x06,0xEC,0xFC,0x2F,0xD4,0x37,
        0x07,0x22,0x79,0x99,0xEC,0x08,0xD4,0x59,
        0x07,0x1D,0xBF,0x96,0x14,0xB5,0x1C,0x15,
        0x0F,0x5C,0xA5,0xDE,0x6A,0x2E,0xC5,0xFE,
        0x0F,0x96,0x26,0x6D,0x1D,0x22,0xE7,0xE3,
        0x05,0xA7,0x01,0x13,0x2A,0x36,0xC3,0xF5,
        0x2A,0x5A,0x72,0x4B,0x87,0x71,0x59,0x87,
        0x05,0x93,0xE2,0x4A,0x49,0xD7,0x6E,0x69,
        0x23,0x1F,0x25,0x60,0x6E,0x1D,0x50,0x4A,
        0x07,0x86,0xB1,0x82,0x29,0x46,0xBF,0x3E,
        0x05,0x3E,0xD8,0x04,0x7D,0x4B,0xEB,0x68
};
static unsigned char firmwaretable_b2a[] = {
        0x04,0x01,0x80,0x00,0x4F,0x6C,0x6E,0x8D,
        0x05,0xE8,0x41,0xBC,0xAA,0xDE,0x9F,0xA8,
        0x07,0xDA,0x3C,0x94,0xFE,0x7E,0x04,0x5E,
        0x27,0xCE,0x85,0xDB,0x8E,0x2D,0x22,0x5D,
        0x2F,0x6A,0xC3,0x98,0x3A,0xAC,0x8D,0x1B,
        0x2F,0x00,0x1B,0x4B,0x28,0xB8,0xD5,0x08,
        0x23,0xAF,0x05,0x51,0xAA,0xAA,0x0F,0x91,
        0x07,0xF4,0x11,0xA5,0x63,0xCB,0x80,0x27,
        0x0F,0xCE,0x8D,0xA5,0x9A,0xD9,0x76,0x3B,
        0x07,0x60,0xAC,0x01,0x2E,0x9D,0x5D,0x67,
        0x2F,0x48,0x05,0xE7,0xC7,0xBC,0x62,0x5A,
        0x29,0x6E,0x34,0xF9,0xD2,0xB1,0x65,0x21,
        0x0F,0x69,0xEA,0x15,0x06,0xF9,0x08,0xB4,
        0x0F,0x32,0xD3,0x96,0x62,0x7E,0xCB,0xB9,
        0x07,0x73,0x83,0x92,0xD0,0x86,0xD5,0xEB,
        0x22,0x56,0xAA,0x71,0xBD,0xCD,0x94,0xC0,
        0x07,0x98,0x3D,0xCB,0x55,0xD4,0x0D,0x01,
        0x2A,0x19,0x2D,0xC5,0x75,0xE6,0x5B,0xF5,
        0x0F,0xA3,0xB6,0x16,0x66,0x2C,0x2D,0xE9,
        0x2D,0x15,0xAC,0x7D,0x40,0x0D,0xDD,0xE1,
        0x07,0xAC,0x05,0x46,0xA4,0x66,0xA5,0xF8,
        0x22,0x20,0x86,0xEA,0x6F,0x62,0x4E,0x40,
        0x07,0x53,0x03,0xD4,0x02,0xF4,0x7C,0x8D,
        0x22,0x6B,0x4E,0x4E,0x79,0xC8,0x98,0xB0,
        0x0F,0x9D,0x54,0xF1,0x14,0xFD,0xDB,0x1A,
        0x2A,0x48,0xCA,0xF0,0x1C,0xE3,0x9A,0xC2,
        0x07,0x19,0x41,0xC1,0x9A,0x65,0xAA,0x60,
        0x2A,0x68,0xAC,0xFA,0x4C,0x04,0x14,0x6A,
        0x07,0x53,0xAE,0x26,0x69,0x7D,0x4A,0x9B,
        0x22,0x53,0x0D,0x62,0x38,0x47,0x6C,0x58,
        0x0F,0x8B,0x8F,0x82,0x3B,0x31,0x71,0x06,
        0x2F,0xDD,0x6F,0x1D,0x37,0x07,0x1E,0x5A,
        0x27,0x9B,0x4D,0x23,0x6B,0xD3,0x14,0x64,
        0x27,0x28,0xB3,0xAE,0xCB,0x1D,0x86,0x93,
        0x2D,0x43,0xEE,0xAC,0xB1,0xB0,0xCC,0xB0,
        0x07,0x1B,0x29,0x37,0xA3,0xB1,0x0E,0x29,
        0x2D,0xA0,0x43,0x5D,0x91,0x34,0x56,0x06,
        0x0F,0xE9,0x13,0xCD,0x87,0xD6,0x8A,0x38,
        0x27,0x9C,0xC7,0x0B,0x51,0xE7,0x5C,0xFD,
        0x21,0x65,0xCA,0x28,0x91,0xD3,0x6D,0x98,
        0x0F,0x82,0xC4,0x8E,0xAF,0x6C,0x99,0x27,
        0x07,0x9E,0x4E,0xF4,0x07,0x79,0xF0,0xEC,
        0x07,0x54,0x8E,0x4F,0x66,0x7D,0xE7,0xCB,
        0x07,0x4B,0xEB,0xEA,0x08,0xDA,0xBC,0x04,
        0x07,0xA5,0xE0,0x2C,0xC0,0x9A,0x9E,0x97,
        0x27,0x0E,0x73,0x55,0xCA,0xB2,0x6A,0xCF,
        0x2F,0xE7,0xE6,0x8B,0xD3,0x62,0x10,0x9F,
        0x2B,0x53,0xFA,0x9A,0x56,0x20,0x6B,0xAD,
        0x07,0xF5,0xAD,0x5C,0xBE,0xC8,0x07,0xDB,
        0x2F,0x2B,0xA3,0x57,0xC7,0x62,0xC9,0x6D,
        0x27,0x7A,0xD2,0x79,0x52,0xFC,0xE6,0x8B,
        0x2F,0xED,0x27,0xD5,0x00,0xB5,0x3F,0xAB,
        0x22,0x11,0xB2,0x02,0xDA,0x87,0x44,0x27,
        0x07,0x98,0xAB,0x54,0xC2,0x87,0x50,0x45,
        0x2F,0xF1,0x35,0x7E,0x4D,0x04,0x75,0x76,
        0x24,0x4C,0x0E,0x5F,0xFB,0x11,0x68,0x3F,
        0x07,0x0B,0x38,0x8D,0x69,0x4B,0x9C,0x99,
        0x22,0x67,0x58,0x07,0xCC,0x6E,0xD3,0x3C,
        0x07,0x51,0xFB,0x8F,0x6A,0x47,0x8D,0x03,
        0x22,0x89,0xB0,0x19,0x79,0xF5,0xCE,0xED,
        0x07,0x68,0x38,0xDF,0x03,0xFB,0x8E,0x4B,
        0x0F,0x34,0xF9,0x4E,0xE6,0x17,0x38,0x0E,
        0x07,0x7D,0xFE,0x10,0x77,0x79,0xE4,0x52,
        0x2D,0xA0,0x54,0x49,0x83,0x4C,0xAB,0xA9,
        0x0F,0x21,0x21,0x25,0xDD,0x32,0xDE,0x5C,
        0x0F,0xC2,0x7F,0x5A,0xED,0xB7,0x1E,0x34,
        0x07,0x0A,0x03,0x79,0x52,0x8C,0xEB,0x31,
        0x22,0x83,0x1A,0x86,0x72,0xF5,0xBD,0xC0,
        0x0F,0x7E,0x4C,0xCA,0xC5,0x04,0x3F,0xCB,
        0x2A,0x05,0x81,0x9E,0x1F,0x7F,0x31,0x67,
        0x07,0xCD,0xEB,0x9C,0xF4,0x61,0x3D,0x68,
        0x2A,0x1C,0x3D,0x27,0x2E,0xDF,0x33,0x87,
        0x07,0x86,0x10,0xA3,0xBE,0x5C,0x9C,0x13,
        0x07,0xF2,0xAA,0x43,0xDE,0xB0,0x44,0xAC,
        0x0F,0xB6,0x4C,0xBA,0xEC,0xDD,0xC2,0x51,
        0x2A,0xE5,0x9C,0xF1,0x75,0x97,0xA2,0x3B,
        0x0F,0x2E,0x0E,0x26,0x9D,0xEB,0x50,0xBC,
        0x27,0xAA,0xE7,0xD6,0x3F,0x70,0x41,0x69,
        0x27,0xFF,0x00,0x3D,0x50,0x6E,0x68,0xF8,
        0x23,0x12,0x30,0xB0,0x34,0x89,0xD9,0x9C,
        0x0F,0x96,0xA7,0x13,0xCE,0x2A,0xB7,0x50,
        0x27,0x5E,0x3E,0x64,0xCE,0xE9,0x97,0x56,
        0x29,0xFB,0xE3,0xFC,0xBF,0xE5,0x32,0xEB,
        0x0F,0x2C,0xF4,0x8D,0x08,0x82,0x82,0x2A,
        0x2A,0x27,0xED,0xC0,0x18,0x45,0x94,0x74,
        0x07,0x51,0x91,0x34,0x60,0x01,0x7E,0x76,
        0x22,0x59,0x15,0xD6,0x96,0x41,0xA8,0x81,
        0x0F,0x97,0x64,0xBD,0x4D,0x0B,0xED,0x51,
        0x27,0x5B,0xED,0x76,0xD1,0xC9,0xAF,0x11,
        0x2C,0x04,0xDE,0x5E,0x3D,0xB9,0x1F,0x3B,
        0x07,0x16,0x63,0x97,0xEE,0x2B,0x0A,0x13,
        0x22,0xB7,0x0D,0xEC,0x30,0x83,0xEC,0xD1,
        0x07,0x84,0xDD,0x1D,0xBE,0xC9,0x2F,0x8C,
        0x2A,0xC6,0xF9,0x0E,0x27,0x86,0xE9,0x5F,
        0x07,0x87,0xED,0x70,0x92,0x00,0x7E,0x8F,
        0x0F,0xA0,0x0F,0xCD,0x31,0x14,0xE1,0x17,
        0x0F,0x96,0x59,0xAA,0x51,0x5D,0xEF,0x93,
        0x25,0x5E,0xB7,0xFA,0xC8,0xF9,0x5A,0x11,
        0x07,0x46,0x69,0x29,0xAD,0x1A,0xB4,0x86,
        0x2F,0xD8,0x24,0xE2,0xFF,0xB0,0xE2,0x83,
        0x21,0x39,0x12,0x74,0x77,0x0F,0x4B,0x81,
        0x07,0xFB,0x5E,0x43,0xBF,0xCE,0xCC,0x18,
        0x0F,0x0B,0x8D,0x81,0xD3,0x72,0x88,0xAB,
        0x0F,0xF6,0x60,0xEF,0x2B,0x9E,0x6F,0x1A,
        0x0F,0x9A,0x0F,0xCB,0xD3,0x14,0x62,0xFB,
        0x07,0x1F,0x4C,0xFF,0x52,0x98,0x8E,0x2D,
        0x0F,0xFD,0xEE,0xDC,0xAA,0x97,0x5B,0x51,
        0x07,0x50,0x77,0xA6,0x32,0xDF,0xEF,0x32,
        0x0F,0x2D,0x26,0x1C,0x4F,0xBF,0x1C,0xE2,
        0x07,0x0F,0x55,0x9C,0xDD,0x71,0xDE,0xE7,
        0x0F,0x08,0xF6,0xD9,0xD4,0xFA,0xCF,0x6F,
        0x0F,0x46,0xB6,0xC0,0x55,0x62,0xBC,0x82,
        0x0F,0xF2,0x04,0xF8,0x20,0x7E,0x75,0x18,
        0x07,0x73,0xF2,0x66,0x0E,0x79,0xEC,0xD9,
        0x27,0x07,0x77,0x6B,0x0F,0x81,0x08,0x59,
        0x27,0x14,0x97,0x56,0x7D,0x61,0xA8,0x85,
        0x2F,0x1D,0x9D,0xB9,0x9E,0x70,0x47,0x1D,
        0x2F,0x5E,0xC4,0xBC,0xA0,0x21,0x6E,0xCC,
        0x2F,0x5C,0x64,0xDF,0x1A,0x93,0x25,0x88,
        0x2F,0xA7,0xEC,0x6B,0x0B,0x07,0x0E,0xAB,
        0x27,0x65,0x0B,0x1A,0xC3,0x79,0x07,0x1F,
        0x21,0xD4,0xF1,0xF2,0xEC,0xDE,0xA9,0x6B,
        0x05,0x1A,0x1D,0x28,0x76,0xA0,0xF0,0x6C,
        0x2F,0x73,0x6A,0x87,0xF2,0xDA,0xAD,0x09,
        0x2F,0xCC,0x2A,0xFE,0xCD,0x92,0x13,0x15,
        0x2F,0xA3,0x2A,0x18,0xB2,0xF9,0x3C,0x84,
        0x27,0x2B,0x28,0x2C,0xBF,0x71,0x42,0x6A,
        0x27,0x07,0xF0,0xAF,0xC6,0x2A,0x13,0x3E,
        0x2F,0x08,0xC2,0xD6,0x1E,0x99,0xE7,0xA6,
        0x27,0xEE,0x35,0xB1,0x20,0xFD,0x45,0x87,
        0x27,0xE4,0xE4,0xEB,0xD8,0xC6,0xB6,0xB3,
        0x27,0xF8,0x2F,0xDE,0x1E,0x4F,0x60,0x43,
        0x27,0xA8,0xC6,0xE0,0x01,0x14,0x83,0x5E,
        0x27,0xD5,0x15,0x06,0xCD,0x10,0xEC,0x6B,
        0x2F,0x90,0x7B,0x98,0x36,0xB1,0xBD,0x90,
        0x2F,0xD2,0x76,0x50,0x1A,0xE6,0x0A,0xB8,
        0x2F,0xBB,0x78,0x91,0xAB,0xC5,0x98,0xEE,
        0x27,0x5E,0x44,0x77,0xBD,0xE4,0xE6,0x7B,
        0x2F,0x68,0x76,0x29,0x61,0x3C,0x26,0x4C,
        0x27,0x0D,0x32,0x4E,0xE5,0xBD,0xE2,0x53,
        0x29,0x49,0x1C,0xBC,0x35,0x03,0x3B,0xDF,
        0x07,0x21,0x4F,0xA5,0xD3,0x77,0x03,0x8F,
        0x2D,0x92,0x23,0xA6,0x6C,0xB5,0x5C,0x2B,
        0x07,0x8E,0x90,0x98,0x3E,0x41,0x32,0x3F,
        0x25,0xCE,0xF4,0x39,0x82,0x53,0xE1,0x4B,
        0x0F,0x39,0xB8,0x29,0xC2,0xB4,0x18,0x66,
        0x2F,0x01,0xAD,0x51,0xFA,0xAD,0xFF,0x34,
        0x27,0xC6,0xFD,0x7B,0x4B,0x49,0x43,0x5E,
        0x2F,0x1E,0x98,0xE8,0x0B,0xD5,0x1D,0xB9,
        0x2F,0x92,0x25,0xF6,0x89,0x29,0xBE,0x32,
        0x26,0x85,0xB7,0x59,0x20,0x0C,0x21,0xB2,
        0x07,0x93,0xC6,0xDA,0x6C,0xF1,0x28,0x76,
        0x27,0x85,0xB5,0x62,0x69,0x62,0x94,0x89,
        0x29,0x8C,0x4A,0x8A,0x64,0x69,0x2C,0xFB,
        0x07,0x75,0xF4,0x79,0xE2,0xA7,0xEA,0x8C,
        0x2C,0xB1,0xC2,0x7C,0xE3,0x5D,0x81,0x56,
        0x0F,0x25,0xFB,0xC2,0x92,0x5C,0x67,0xF1,
        0x27,0xD2,0xC8,0x18,0xF5,0x70,0xB3,0x91,
        0x2F,0x42,0xDE,0x50,0xF8,0x4D,0x89,0xAF,
        0x2F,0x20,0x7B,0x34,0x7A,0x2F,0x49,0x65,
        0x27,0xFA,0xC8,0x4A,0x06,0x91,0x47,0x17,
        0x27,0xBB,0x53,0xCE,0xE5,0x9D,0x4D,0xBB,
        0x2F,0x92,0x3D,0x53,0x82,0x1C,0xF0,0x96,
        0x21,0x71,0x87,0x0A,0x60,0xA7,0x9D,0xCB,
        0x07,0x7A,0x46,0x0C,0x2E,0x9A,0xC4,0x24,
        0x2A,0xE3,0xCE,0x5E,0x17,0x6D,0xAA,0x52,
        0x0F,0xDD,0xB5,0x7B,0xEE,0x5B,0xE1,0xF4,
        0x2F,0x69,0x2E,0x5D,0x7C,0x2D,0xCC,0xE3,
        0x21,0x18,0x09,0xEF,0x26,0x3C,0x72,0xDB,
        0x0F,0xFC,0x23,0x7E,0x61,0x8B,0xAD,0x1B,
        0x0F,0x40,0x67,0x73,0x1D,0xCE,0xD0,0x2C,
        0x0F,0x53,0x07,0x45,0x63,0x9A,0xA7,0x17,
        0x22,0x0F,0x79,0x41,0x58,0x43,0xF8,0x6B,
        0x07,0xD4,0x06,0x70,0x5A,0xC0,0x63,0x2A,
        0x0F,0x10,0x74,0xF0,0x80,0x6F,0x1F,0x2D,
        0x27,0xAA,0xBB,0x07,0xAE,0xD9,0x73,0xA5,
        0x2F,0x1B,0x21,0x9E,0xD9,0x19,0xDC,0x0A,
        0x2F,0x31,0xB5,0x3C,0xB5,0xE9,0xBE,0xDB,
        0x27,0xAB,0x63,0xE9,0xDF,0x2D,0xD5,0x37,
        0x27,0x1C,0xB4,0x06,0x31,0x0C,0xAF,0xF1,
        0x2F,0xAF,0x37,0x12,0xBE,0x56,0x3F,0x31,
        0x2F,0x79,0x92,0x0D,0x8A,0x06,0xB8,0xF2,
        0x27,0x55,0x16,0x59,0xBD,0xD8,0x9C,0xC8,
        0x2F,0x3D,0x4F,0x72,0x78,0xC0,0x6D,0xEA,
        0x2E,0xAD,0x2D,0xB3,0x3F,0x1E,0x39,0xA5,
        0x0F,0xD0,0x2C,0x21,0x64,0x01,0x0C,0x85,
        0x27,0x77,0x3A,0x75,0xAD,0x8F,0x26,0x79,
        0x2F,0xDE,0x65,0x36,0x09,0xEC,0xB1,0x04,
        0x2F,0xFA,0xEF,0x5A,0x19,0x67,0xE8,0x0E,
        0x27,0x3C,0x71,0x25,0x59,0x78,0x4B,0x41,
        0x0F,0xF3,0x13,0xD7,0xA2,0xDA,0x18,0x1D,
        0x27,0x36,0x9A,0x7B,0x8D,0xD0,0x3C,0x24,
        0x23,0x23,0xAB,0x1D,0x01,0xD9,0xA6,0xEB,
        0x07,0xB1,0xCF,0x2B,0x8B,0x99,0x4F,0x1F,
        0x27,0x95,0xF3,0x6C,0xF5,0x12,0xB3,0xE6,
        0x2F,0x64,0xB9,0xB4,0xA1,0x13,0x60,0x16,
        0x24,0xD3,0x65,0x46,0xF0,0xBC,0x11,0x1E,
        0x0F,0xB4,0xF9,0xCD,0x17,0x5E,0xC8,0x88,
        0x27,0xD6,0xD4,0x96,0xB3,0x96,0xD8,0x8F,
        0x27,0x6A,0xC7,0xAC,0x01,0x0A,0xF0,0xDF,
        0x0F,0xD4,0x23,0x5C,0x61,0xC1,0x24,0x73,
        0x27,0xF9,0x13,0xA6,0x86,0xB5,0x6B,0x65,
        0x2C,0xAD,0xA9,0x85,0xDF,0xA6,0xD3,0x48,
        0x0F,0x97,0x73,0x4C,0x38,0xCE,0xD6,0xC5,
        0x24,0x8C,0x19,0x31,0xE0,0xEB,0xD2,0xAE,
        0x07,0x73,0x8C,0x00,0x99,0xAF,0x7F,0xC6,
        0x0F,0x0F,0x66,0x4A,0xE4,0xB8,0x90,0x0F,
        0x25,0xEC,0xF2,0xA1,0xBC,0xB4,0x1F,0xC9,
        0x07,0x63,0x78,0xD1,0xFF,0x41,0xDF,0x40,
        0x07,0x65,0x61,0x3A,0x73,0x09,0x0D,0xDD,
        0x0F,0x9A,0xD8,0x72,0x19,0xDA,0x01,0x30,
        0x2F,0x6D,0x8C,0x69,0x43,0x2C,0xB0,0x16,
        0x24,0xE5,0x5E,0x9F,0x40,0xB0,0x4E,0x2C,
        0x0F,0xE6,0xCC,0x32,0xD7,0xD6,0x78,0x5D,
        0x2F,0x80,0x46,0xD4,0x71,0x87,0x51,0x87,
        0x21,0x77,0xAB,0x08,0x15,0x97,0xF0,0x9F,
        0x07,0xE0,0x28,0x57,0x92,0x35,0xDC,0x1F,
        0x27,0xE0,0xA2,0xDA,0xFB,0x29,0x97,0x4C,
        0x2F,0xF5,0xC2,0x21,0xBC,0xBD,0x2A,0x5A,
        0x27,0xE1,0xAE,0x22,0x7C,0xD9,0x45,0xEC,
        0x2F,0x8B,0xCB,0x74,0x7C,0x01,0x87,0x0D,
        0x27,0x55,0x76,0x2B,0x6C,0x32,0xA3,0xBC,
        0x2F,0x2A,0x87,0xD1,0xAE,0x0B,0xA5,0x96,
        0x27,0x66,0x57,0xB8,0x4B,0xBD,0x60,0xCC,
        0x2D,0x24,0xAC,0x84,0x32,0x8E,0xE6,0xA7,
        0x07,0x6F,0x67,0xBC,0x43,0x6A,0x95,0x10,
        0x22,0x3E,0x4C,0xCD,0x1D,0x31,0x88,0xF0,
        0x07,0x82,0x8F,0x95,0x61,0x44,0x98,0xFA,
        0x2E,0x2C,0x7A,0x23,0x24,0x97,0x3D,0x8E,
        0x07,0xF8,0xB8,0xD4,0xFF,0x33,0x06,0xA5,
        0x07,0xCA,0xEA,0xA7,0x38,0xC5,0x06,0x3C,
        0x0F,0xA9,0x1D,0x9A,0x1E,0x69,0xF8,0x30,
        0x0F,0xFD,0x6C,0xED,0xB2,0x92,0xDB,0x15,
        0x07,0x75,0x44,0xBD,0xC5,0x47,0x93,0x2C,
        0x07,0x53,0xD2,0x37,0xAF,0x7F,0x07,0xF9,
        0x2F,0xDD,0x6A,0xD2,0xBA,0xAD,0xD3,0xA0,
        0x21,0x03,0x81,0x14,0xE9,0x65,0x73,0xFD,
        0x07,0xAA,0xCD,0x54,0x0A,0x0F,0xE1,0xD5,
        0x0F,0x94,0xD3,0x81,0xD2,0x84,0xA3,0xCE,
        0x27,0xD1,0xB0,0x0B,0xAD,0xF0,0x7E,0xD3,
        0x2F,0x0C,0x4C,0x01,0x84,0x13,0x4A,0xED,
        0x27,0x23,0x80,0x0F,0x5B,0xE1,0xBE,0x78,
        0x2F,0x70,0xFF,0x5A,0xB7,0xD6,0x08,0xBE,
        0x27,0x01,0xF1,0x72,0x93,0x74,0x36,0xDB,
        0x23,0x63,0xB1,0xB4,0xD9,0x40,0x82,0xC8,
        0x07,0x04,0xF6,0xD8,0xDC,0x53,0xC3,0xF0,
        0x27,0x51,0x67,0x87,0x74,0x63,0xEC,0xAE,
        0x2A,0x09,0xCE,0x78,0xAC,0x58,0xAC,0x1D,
        0x0F,0xF3,0x3B,0xB3,0x69,0x11,0xA2,0x65,
        0x27,0x09,0x0E,0x70,0xEC,0x84,0x5C,0x04,
        0x27,0x1E,0x75,0x63,0xEE,0x39,0x6F,0xAA,
        0x24,0x16,0xCB,0xA5,0x4C,0xC5,0xAE,0x76,
        0x07,0x2D,0x27,0xDD,0xB7,0x12,0x88,0x03,
        0x26,0x52,0x85,0x4A,0xEB,0x7E,0x56,0x15,
        0x07,0x1D,0x4B,0x2B,0x8B,0x09,0x0A,0x8E,
        0x27,0x60,0x48,0xE7,0xE1,0xAD,0xF0,0xED,
        0x27,0xCD,0x98,0x39,0xB4,0x52,0xDA,0x77,
        0x27,0xA1,0x40,0x67,0xAC,0x95,0xBA,0x23,
        0x27,0xA6,0x26,0x36,0xDC,0x9F,0x16,0xA8,
        0x27,0xE9,0xD9,0xAB,0xCE,0xED,0x65,0x80,
        0x24,0x13,0x86,0x27,0xC1,0x78,0xD3,0xDF,
        0x07,0x1B,0x9A,0x4C,0x88,0x46,0xBE,0xCF,
        0x27,0x03,0xE5,0xBA,0x57,0x12,0x89,0x34,
        0x27,0xB5,0xD3,0xE6,0x89,0xAD,0xDD,0xF8,
        0x2F,0xA0,0x96,0xCC,0xE5,0xA9,0xA3,0x0F,
        0x27,0x09,0x02,0x9F,0x6C,0x1A,0x60,0x3A,
        0x2F,0x04,0x4F,0x94,0xE2,0xF3,0xAC,0x2D,
        0x23,0x6F,0x29,0x6B,0x63,0x52,0x57,0xB6,
        0x0F,0xC3,0xBC,0xAB,0x81,0xE4,0x23,0x33,
        0x0F,0x3B,0xC3,0x09,0xF6,0xB5,0x5D,0x3D,
        0x07,0x85,0x0B,0x9D,0xBB,0xAC,0x50,0x3B,
        0x2E,0x1E,0x79,0x38,0xEB,0x33,0x51,0xA5,
        0x07,0x4B,0x25,0xA2,0x90,0x14,0x92,0xE4,
        0x2F,0x66,0x44,0x37,0x8A,0x3E,0x5E,0x85,
        0x2F,0xD4,0x3C,0x1C,0x70,0xC0,0xCF,0x74,
        0x27,0x82,0xB1,0x2F,0x36,0x66,0xAA,0xDE,
        0x27,0x88,0x2C,0xC3,0x67,0x3B,0xE2,0xB5,
        0x2F,0x22,0x37,0x2A,0xD5,0x68,0x0E,0x12,
        0x22,0x8A,0x34,0xD1,0xDE,0x7F,0x62,0x2B,
        0x0F,0x8D,0x83,0xF6,0x50,0x68,0x93,0x24,
        0x2F,0x15,0x96,0x8A,0xB9,0x1D,0x6E,0xCC,
        0x27,0xE9,0x79,0x69,0x4E,0xBA,0x34,0x9E,
        0x27,0xB3,0x07,0x84,0xB9,0x53,0x44,0xCE,
        0x2F,0x6C,0x6A,0xC7,0x4B,0xA7,0x66,0x09,
        0x2F,0xDE,0xDC,0xB3,0x06,0x65,0xD3,0xFC,
        0x2A,0x45,0x0D,0xB4,0x32,0x38,0x47,0xF4,
        0x07,0x89,0xCE,0x76,0x31,0xE7,0xB1,0xBA,
        0x27,0xBF,0x76,0x59,0x16,0xA1,0x32,0x86,
        0x27,0xE8,0x59,0x45,0x54,0x74,0xF3,0x01,
        0x2F,0x8B,0x2D,0x52,0xA7,0xD7,0xF9,0x77,
        0x2A,0xC3,0x00,0x65,0xD0,0xDB,0xCA,0x29,
        0x07,0xD7,0x5D,0x9E,0x0D,0x54,0x20,0x6D,
        0x27,0xC0,0xE7,0x52,0xCD,0x36,0x8A,0x8A,
        0x21,0x77,0x8A,0x8E,0x8A,0xE3,0xDF,0x92,
        0x07,0xDE,0x17,0x4F,0x35,0xBB,0x03,0x81,
        0x25,0xF2,0x86,0x11,0x3C,0x71,0xDD,0x32,
        0x07,0xD5,0xF5,0x0D,0xEE,0x87,0x67,0x81,
        0x23,0x04,0xD1,0x8B,0x6D,0x6B,0xE1,0x23,
        0x07,0x1A,0x50,0x4D,0x28,0x7C,0x33,0xC7,
        0x27,0x22,0x03,0x29,0x3E,0x77,0x20,0x24,
        0x27,0x3F,0xE6,0x75,0x32,0x13,0x7D,0x65,
        0x24,0x34,0xB7,0xCE,0xE1,0x93,0x10,0xC9,
        0x07,0xF8,0xF7,0xCF,0x00,0xCD,0x84,0xD3,
        0x0F,0x43,0x4B,0x43,0x1A,0x07,0x1B,0x24,
        0x27,0x37,0x57,0x3D,0x2D,0xDF,0x12,0x4D,
        0x27,0x7D,0x40,0x45,0x47,0xC9,0x32,0x9E,
        0x26,0x70,0xA4,0x84,0x6B,0x5D,0xC7,0xE3,
        0x07,0xF6,0xC6,0xF5,0xE0,0x72,0x95,0x59,
        0x2F,0x4F,0x2F,0xB1,0x75,0x5B,0x7F,0x45,
        0x27,0xF3,0xC1,0xDE,0x19,0xF3,0xE1,0x99,
        0x22,0x1A,0x3E,0x65,0x58,0xD0,0xA3,0x45,
        0x07,0xFB,0x0A,0x95,0x96,0x8A,0x73,0x1B,
        0x27,0xAD,0x70,0xA8,0xA4,0xBE,0x99,0x7A,
        0x27,0x50,0x10,0x42,0x15,0xE9,0x94,0xDB,
        0x2F,0x91,0xCD,0xB6,0x22,0x02,0x58,0x8D,
        0x27,0xBA,0x29,0xB3,0x6D,0x3D,0x26,0x54,
        0x2F,0xFC,0x7D,0xDE,0x88,0xE8,0xCE,0x49,
        0x2F,0x40,0xFA,0x39,0x62,0x98,0x57,0x83,
        0x27,0x1E,0xFE,0xCE,0x1E,0xB2,0xF1,0x5A,
        0x27,0x2E,0x58,0x51,0x63,0x3E,0xD8,0x93,
        0x27,0x99,0x8F,0x41,0x53,0xCE,0x2C,0x0E,
        0x27,0x6E,0x10,0x81,0x47,0xC6,0x1D,0xCB,
        0x27,0x3C,0x14,0x02,0x50,0x47,0x15,0xED,
        0x2A,0x64,0x79,0x96,0x67,0x4E,0xCF,0x3D,
        0x0F,0xE1,0x2D,0x5D,0x8E,0xCB,0x92,0xF1,
        0x2F,0x35,0xD9,0xF1,0x6A,0x47,0xA7,0x0F,
        0x27,0x71,0xB5,0x04,0xD5,0x31,0xAA,0x03,
        0x27,0x7A,0x14,0x7B,0xE9,0x80,0xBA,0x2A,
        0x25,0xAF,0xD9,0x39,0x18,0xF6,0x5E,0x41,
        0x07,0x00,0x64,0xF6,0xD7,0x41,0xC2,0x33,
        0x2F,0xD2,0xCA,0x8F,0xC5,0xFA,0x3B,0x13,
        0x2F,0x3E,0x8B,0x3B,0x24,0x72,0xF9,0x06,
        0x27,0x9D,0x50,0x4E,0xBA,0xC7,0x3D,0x3E,
        0x23,0xF0,0x63,0xAD,0x06,0x77,0x3B,0x81,
        0x07,0x96,0x0E,0xAB,0x19,0xA5,0x65,0xA2,
        0x0F,0x9F,0x10,0x33,0x9E,0x9B,0xC2,0xF6,
        0x07,0x89,0xB4,0x94,0xDD,0xD8,0x80,0xB4,
        0x27,0x74,0xD6,0x7E,0x77,0x02,0x50,0xDB,
        0x27,0x5F,0x5A,0x68,0xC0,0x17,0xB2,0xFB,
        0x27,0xDF,0x4A,0xCD,0xEB,0x17,0x41,0x2E,
        0x2F,0x60,0x09,0xDE,0xDB,0xD3,0x37,0xE7,
        0x25,0x5B,0xB8,0xBE,0x72,0xC1,0xF7,0xC3,
        0x07,0xBC,0xB5,0x39,0xB1,0x9D,0xF8,0xE9,
        0x2F,0x3A,0x0D,0x45,0xAE,0xFF,0x11,0x79,
        0x29,0xEB,0x8E,0x38,0x91,0xD9,0x80,0x95,
        0x0F,0x14,0x37,0xE5,0x2F,0x50,0xE3,0xCE,
        0x2F,0xC4,0xEB,0x6E,0xD4,0xE8,0x31,0x24,
        0x2F,0x78,0xB7,0xB8,0x2A,0xB1,0xAC,0xA8,
        0x23,0x41,0xBA,0x85,0x50,0x52,0x5E,0xDE,
        0x0F,0x86,0x86,0x0D,0xDF,0x67,0x5A,0xE9,
        0x2F,0x27,0xA2,0x53,0xB2,0xCF,0x54,0x33,
        0x27,0x03,0x05,0x32,0x4A,0xA6,0xC0,0x1B,
        0x27,0xDC,0xA1,0xC7,0xA4,0xF0,0xFD,0x6D,
        0x23,0xEF,0x74,0x8A,0xAA,0x2F,0x93,0x18,
        0x0F,0xAF,0x86,0x23,0xAF,0xB9,0x06,0xCC,
        0x0F,0x7D,0xCF,0xA6,0x5A,0x5E,0xFC,0x4E,
        0x0F,0x8C,0x48,0xD3,0xCE,0x97,0x9F,0xFC,
        0x07,0x06,0x01,0x82,0xDB,0x73,0x18,0x7F,
        0x07,0x1C,0xE4,0x91,0x17,0xB9,0x94,0x53,
        0x2F,0x53,0xD3,0x45,0xC5,0x5D,0x27,0xD7,
        0x27,0x99,0xEC,0x71,0xF0,0x2D,0x8B,0x5A,
        0x2F,0x45,0x35,0xEC,0xDC,0x3B,0xED,0x68,
        0x2F,0x96,0x1D,0x38,0x2A,0x5F,0x25,0xB0,
        0x2F,0xBC,0x5B,0x09,0x62,0xC3,0x70,0x5D,
        0x2F,0x5E,0x89,0xF8,0xF7,0x1B,0xB6,0xF0,
        0x2F,0x8C,0x85,0x0A,0xF0,0x45,0x0E,0x1C,
        0x2F,0x61,0x41,0x4A,0x5F,0x52,0x0E,0xE3,
        0x2F,0xBE,0x57,0x5E,0x59,0xC3,0x62,0x29,
        0x27,0xBD,0x67,0x4F,0x7D,0xC7,0xE3,0xB8,
        0x2F,0xCD,0x73,0xFD,0x9B,0xD4,0x48,0x49,
        0x29,0xE7,0x3F,0x70,0x88,0xA2,0x7C,0xAD,
        0x0F,0x69,0x90,0x85,0x19,0x26,0xFE,0x1E,
        0x27,0xE7,0x4C,0x20,0x0E,0x78,0x71,0xD0,
        0x27,0xD3,0xEC,0x39,0xA7,0x49,0xDC,0x0A,
        0x2F,0xFE,0x60,0x0C,0x94,0x7A,0x5E,0xAC,
        0x27,0xE6,0xB2,0xDE,0x2E,0xA7,0x6A,0x82,
        0x27,0x99,0x07,0x28,0xA8,0xC9,0xD0,0x7D,
        0x2F,0xFB,0xF1,0x4A,0x82,0x1D,0x3B,0x3C,
        0x2F,0xAA,0x7E,0x91,0x08,0x19,0x40,0x45,
        0x2F,0x9A,0x14,0x9E,0xA1,0xD0,0xD9,0x89,
        0x27,0xE9,0x9A,0x5A,0x1A,0x2A,0x73,0x10,
        0x27,0x0C,0xB6,0x1F,0x37,0xD6,0x6E,0x4A,
        0x27,0x7B,0x91,0x11,0x13,0x42,0xBA,0x77,
        0x27,0xC0,0xA7,0xF5,0x56,0xC1,0x48,0xDD,
        0x27,0xCE,0xF9,0xFE,0x31,0x70,0x5D,0x77,
        0x2F,0x6E,0xE9,0x4E,0x65,0x7F,0x39,0xEB,
        0x07,0x7D,0x9F,0xF3,0x29,0xE4,0x44,0x7D,
        0x07,0x72,0xCF,0xB3,0xC1,0xA4,0x41,0x9E,
        0x0F,0xAF,0xD4,0x2A,0x79,0x05,0x56,0x17,
        0x2F,0x3A,0xCE,0x89,0x31,0xCB,0x4F,0x29,
        0x2D,0xA0,0xDC,0x67,0x3A,0xA2,0x81,0xF6,
        0x0F,0xE3,0x9D,0xAF,0xF7,0x53,0x5B,0xBB,
        0x07,0x40,0xD5,0xCD,0x53,0xDD,0x36,0xF5,
        0x07,0xE3,0xA8,0xA6,0x8B,0xA7,0xFF,0xBD,
        0x27,0xEF,0xBB,0x40,0xBC,0x48,0xF6,0x17,
        0x25,0x78,0xE9,0x5B,0x1C,0xF1,0xBD,0xBA,
        0x07,0xAD,0x46,0x1A,0x9C,0xAA,0x09,0x56,
        0x07,0x8C,0xD4,0xDD,0xE2,0x48,0xF9,0x08,
        0x0F,0x2C,0x9F,0x2B,0x98,0x79,0x60,0xF2,
        0x27,0xE5,0xE8,0xB1,0x60,0xC3,0xF9,0xCC,
        0x2D,0x2D,0x6E,0x7F,0x94,0x00,0x25,0x92,
        0x0F,0x35,0xD0,0x2B,0xC5,0xAD,0x91,0xE5,
        0x07,0xBC,0x7C,0xFB,0xE7,0xE4,0x5C,0x9C,
        0x0F,0xC0,0x76,0x99,0x7E,0x78,0x5F,0xE2,
        0x2F,0x4B,0xD0,0x82,0xC2,0x13,0x32,0xB2,
        0x27,0x2A,0x34,0xDA,0x57,0x44,0x20,0x2A,
        0x27,0x0B,0xC2,0x4E,0xCD,0x77,0xB0,0x0A,
        0x21,0x95,0x92,0x63,0x00,0x2E,0x07,0x48,
        0x0F,0x5C,0x08,0x45,0x47,0x05,0xAD,0xBB,
        0x27,0x2C,0x38,0x2A,0xEB,0xC7,0x83,0x11,
        0x2C,0xE7,0x6F,0xEE,0xB5,0xBB,0xC3,0x1F,
        0x0F,0x03,0xCA,0x2E,0x58,0xAC,0x43,0xAF,
        0x27,0xFC,0xF8,0x7F,0xEE,0x74,0x59,0x1A,
        0x2A,0xA6,0xB9,0x96,0xCA,0x75,0x5B,0xDD,
        0x07,0x43,0x5D,0x2E,0x90,0x49,0x4B,0x1F,
        0x2F,0x28,0xE8,0xE1,0x68,0x98,0x62,0x9E,
        0x2B,0xCC,0x18,0xBF,0xB6,0x30,0x45,0x0B,
        0x0F,0xA5,0x26,0x5F,0xF2,0x4E,0x8D,0x4C,
        0x2F,0x1D,0x2E,0x2B,0xBD,0x51,0xE0,0x8B,
        0x22,0xC9,0x0F,0x84,0x7B,0x1E,0xD3,0x3B,
        0x0F,0xC5,0x59,0x5B,0xEF,0x74,0xAE,0x9D,
        0x0F,0x9C,0x3C,0x2E,0xBD,0xE7,0xCF,0x33,
        0x0F,0xF2,0xD0,0x95,0xFD,0x16,0x91,0xEC,
        0x27,0xD1,0x25,0x1E,0x42,0xE4,0x76,0xA5,
        0x27,0xB2,0x51,0xF4,0x47,0x86,0x96,0x3C,
        0x2F,0x86,0xD4,0xC3,0xEB,0x0F,0x61,0xC5,
        0x2F,0x02,0xC7,0xD2,0xF3,0x08,0x37,0x11,
        0x2F,0x6E,0x76,0xB8,0x3C,0x8D,0xB2,0x3C,
        0x27,0x9A,0x63,0x1C,0xE7,0x0F,0x30,0xA9,
        0x2F,0x99,0x35,0x0B,0x4F,0x30,0xD8,0x40,
        0x2F,0x05,0xB5,0x9B,0x25,0xDC,0xAA,0x04,
        0x2F,0x7E,0xC8,0xA0,0x9A,0xCF,0x5B,0x02,
        0x2F,0xF1,0xAE,0x6A,0xC0,0xF2,0x7F,0x26,
        0x27,0xD0,0xB5,0x4F,0xF2,0x49,0xE3,0x6B,
        0x2F,0x05,0x93,0x36,0x06,0x87,0x4F,0xCC,
        0x2F,0x23,0xEC,0x7C,0x4C,0x72,0x9A,0x3C,
        0x2F,0x6B,0x54,0xF8,0x3B,0x61,0x5E,0x2C,
        0x2F,0x64,0x81,0x71,0x57,0xAB,0x7E,0x00,
        0x27,0x1B,0xDF,0x60,0x87,0xEC,0x09,0xCE,
        0x27,0xD9,0x0F,0x88,0x85,0x66,0x30,0x75,
        0x2B,0x4A,0xDA,0x8E,0x70,0x95,0xE1,0x03,
        0x07,0x7F,0xB4,0x6C,0x2A,0x75,0x26,0xB4,
        0x27,0x0F,0x2B,0x5C,0x41,0x5E,0x1B,0x17,
        0x27,0xC1,0xC3,0xFD,0x47,0x80,0xF3,0x0C,
        0x2D,0xA0,0xD6,0x52,0xD4,0x7D,0x36,0x56,
        0x0F,0x33,0x9A,0x66,0x38,0x78,0x41,0xDB,
        0x27,0x5F,0xA3,0x2B,0x2E,0x2C,0x10,0x4B,
        0x2F,0xB9,0xE4,0x5A,0xB5,0x4F,0x77,0xC5,
        0x27,0x71,0x81,0x09,0xCB,0x75,0x8B,0xBE,
        0x2F,0x51,0xF0,0xBF,0xCF,0x7E,0x86,0x1F,
        0x0F,0x99,0x38,0x76,0x15,0x09,0xE3,0xD4,
        0x2F,0xEC,0xF4,0x74,0x81,0x20,0xBD,0x72,
        0x2F,0xD7,0x2F,0x9B,0xCC,0x51,0x20,0xFB,
        0x2F,0xF6,0x33,0x61,0xDA,0x43,0xDF,0x18,
        0x0F,0xD2,0x46,0x42,0x53,0xFB,0x33,0x85,
        0x0F,0xC2,0xE3,0xFA,0x62,0x71,0x50,0x3F,
        0x2F,0xE0,0xD4,0xB9,0xEB,0xBB,0x2F,0xEA,
        0x2F,0xB1,0x81,0x68,0x7C,0xAE,0x15,0x12,
        0x2F,0xAD,0x25,0xB8,0xC9,0xD4,0xDE,0x0F,
        0x2F,0x8E,0xF4,0xBB,0x6E,0xDE,0x58,0x90,
        0x2F,0xAF,0x29,0xFD,0x4D,0xF4,0x6F,0xCD,
        0x2F,0x43,0x04,0x98,0x64,0x8A,0x05,0x8C,
        0x27,0x63,0xE2,0x80,0x63,0xE3,0x79,0x4E,
        0x27,0xE8,0x83,0x73,0x9F,0x52,0x3E,0x0B,
        0x2F,0xF4,0x4C,0xF1,0x20,0xE1,0x61,0x87,
        0x27,0x2F,0x13,0x94,0x95,0x0A,0x95,0x5E,
        0x2F,0x8B,0xB7,0x5A,0x4E,0x68,0x74,0x38,
        0x27,0xF0,0xB0,0x06,0x0C,0x5E,0xFC,0x83,
        0x22,0x58,0x90,0x75,0xD6,0x11,0xD7,0x23,
        0x0F,0xD1,0x1D,0x83,0xFB,0x05,0xC1,0xB5,
        0x27,0xC0,0x9C,0xCD,0x2E,0x86,0x59,0x3F,
        0x27,0xC5,0x8F,0x00,0xCD,0x54,0xBF,0x63,
        0x27,0x0B,0x2A,0xF0,0xD5,0xE0,0xE7,0x7D,
        0x27,0x79,0x4B,0xA8,0x6A,0x63,0x4F,0x6E,
        0x27,0x22,0xF9,0xE0,0xA4,0x15,0xD1,0xE9,
        0x2F,0xEE,0xB9,0xEC,0x42,0x47,0x31,0xD9,
        0x2F,0xCE,0x73,0x1C,0x27,0xF5,0x83,0xF0,
        0x2B,0xC8,0xB2,0x41,0x37,0xEC,0x07,0x65,
        0x0F,0x1E,0x28,0x57,0x8B,0x7F,0xBD,0x0E,
        0x07,0xFE,0x98,0x3E,0xCA,0xCA,0xE7,0xB8,
        0x07,0x9D,0x32,0x82,0x84,0x7F,0x6B,0x05,
        0x2F,0x8D,0x92,0xFC,0x4A,0x2B,0xDA,0xEC,
        0x27,0xD1,0x0E,0x34,0x61,0xB0,0x50,0x3A,
        0x27,0x0F,0x34,0xCC,0xF6,0xB6,0xF7,0x35,
        0x2C,0x22,0x0C,0xDA,0xCA,0xBF,0x60,0xFB,
        0x0F,0xA9,0x87,0xC4,0x65,0x52,0xA1,0xDE,
        0x27,0x63,0x6C,0xAE,0xED,0x2E,0x5A,0xE9,
        0x21,0x05,0xD7,0x22,0x50,0x14,0xD8,0x13,
        0x07,0x7F,0xDA,0xCE,0xAA,0xA4,0x09,0x6D,
        0x2F,0xBF,0x0C,0xF7,0xFD,0xE0,0x62,0xB4,
        0x2A,0x4C,0x6E,0x4B,0xC9,0x21,0xFC,0x7F,
        0x07,0x84,0xB9,0x4B,0x0D,0x2F,0x95,0x06,
        0x0F,0x42,0xB8,0x0B,0xF1,0xAD,0xAB,0x6E,
        0x0F,0xFC,0x3B,0x3C,0xD0,0x2F,0x84,0x53,
        0x2F,0xA1,0x07,0x6A,0xF0,0x3F,0xB5,0x9E,
        0x29,0x2F,0xA6,0x58,0x9B,0x4D,0xE4,0x8B,
        0x07,0xF5,0xD7,0x4F,0x34,0xE6,0x71,0x48,
        0x2F,0x26,0xA3,0x2E,0x0F,0x2E,0xA8,0x08,
        0x27,0xB7,0xCA,0x34,0x7B,0x52,0xF1,0xE2,
        0x27,0xB3,0x6B,0x90,0x56,0xC4,0x79,0x31,
        0x27,0xE8,0x83,0x3B,0xB5,0x0D,0x87,0xB7,
        0x2F,0x8D,0x54,0xF7,0x05,0x9E,0xB4,0x3D,
        0x2F,0xB9,0x79,0x24,0x1E,0x21,0x53,0x15,
        0x27,0x3A,0x0D,0xC7,0x85,0xE0,0x03,0xCC,
        0x2B,0x7B,0x51,0x51,0xF1,0x90,0x53,0x4B,
        0x0F,0xA1,0x95,0xD0,0x39,0x2F,0x9B,0x52,
        0x27,0x97,0xF8,0x1A,0x24,0x30,0xE6,0x97,
        0x27,0xFB,0xBA,0x50,0x03,0xDE,0x9A,0x09,
        0x2F,0x21,0xCF,0xDA,0x40,0xC6,0x03,0x2D,
        0x27,0x74,0x62,0xCF,0x6D,0x4C,0x86,0x43,
        0x2B,0x5C,0x13,0x29,0xC6,0xFD,0x0E,0x22,
        0x07,0x21,0xAC,0x96,0x7B,0xFB,0xD5,0xF6,
        0x2F,0x20,0x74,0x6D,0xBD,0x29,0x03,0xE3,
        0x27,0x7C,0xFD,0x09,0x9D,0xC5,0x4B,0xFD,
        0x2F,0xBD,0x3C,0xA2,0x37,0xFC,0x6B,0x28,
        0x27,0xEE,0x95,0x11,0x10,0x61,0xCE,0x72,
        0x27,0xEF,0x0C,0x70,0x0B,0x3D,0x56,0xA3,
        0x27,0x50,0x90,0xF9,0x36,0x07,0x11,0x1D,
        0x2C,0x65,0x31,0x07,0xB1,0x1E,0x03,0xE3,
        0x07,0x6E,0x29,0x99,0x36,0xB7,0x11,0xB0,
        0x27,0x92,0x21,0xC3,0xDC,0x3E,0x91,0x1F,
        0x2F,0xA2,0xEE,0x78,0x4D,0xEB,0x8B,0xA6,
        0x2F,0x56,0x48,0xDA,0x15,0x60,0x5F,0x4C,
        0x07,0xF4,0x25,0xAE,0x8C,0xC5,0x2C,0x0D,
        0x27,0xCE,0xB1,0x4C,0x31,0x14,0x5C,0xDF,
        0x2F,0x74,0x51,0xE0,0x62,0x9A,0x25,0x4B,
        0x27,0xB0,0xCD,0xAF,0x31,0xB8,0x8C,0x81,
        0x27,0xA6,0x7A,0xF0,0x8B,0xE6,0x17,0xED,
        0x2F,0xCA,0xC1,0x4B,0xBD,0x28,0x73,0x77,
        0x2C,0x22,0x87,0x83,0x30,0xF1,0x56,0x72,
        0x07,0x21,0x06,0x4D,0xA3,0xAD,0xD5,0xE8,
        0x2F,0xFA,0x37,0xAC,0xA6,0x58,0x25,0xF5,
        0x2F,0x00,0x54,0xA1,0xC2,0x9D,0x4F,0xE9,
        0x2F,0x7E,0xDA,0x01,0xF0,0x47,0x84,0x8C,
        0x27,0x8C,0xF1,0x40,0xE7,0x17,0x05,0xED,
        0x0F,0x94,0xE2,0x9A,0x69,0x4A,0xC8,0xE7,
        0x0F,0xC5,0xFE,0xF2,0x7D,0xC7,0x1A,0x65,
        0x0F,0xE4,0x06,0x6C,0xA8,0x7E,0xA3,0x9F,
        0x2B,0xBE,0x61,0x2F,0x40,0xC1,0xF8,0xFB,
        0x07,0x1B,0xC1,0x5E,0x3B,0x7C,0x15,0x1F,
        0x27,0x0E,0x3A,0xD4,0xB3,0xE6,0x27,0xA7,
        0x27,0x40,0x00,0x5E,0x99,0x6D,0x1E,0x8C,
        0x2F,0x44,0xE4,0x28,0x60,0x9D,0x42,0x26,
        0x27,0x54,0xBA,0xA5,0xFA,0x96,0x59,0x12,
        0x26,0x61,0x6C,0x3B,0xE3,0x15,0x58,0x11,
        0x0F,0x11,0x25,0x40,0x8F,0x24,0xEC,0x56,
        0x27,0x30,0xD4,0xD1,0xEE,0xD3,0x2E,0x84,
        0x2F,0x34,0x99,0xBB,0x5E,0x31,0x6B,0xD3,
        0x27,0xE1,0x00,0xF0,0x77,0x25,0x32,0x21,
        0x2F,0x40,0x78,0xB8,0x4D,0x20,0x89,0x19,
        0x2B,0x83,0x7D,0xF7,0xF1,0xA1,0x39,0xCC,
        0x0F,0x01,0x88,0x06,0xBE,0xDA,0x16,0x54,
        0x2F,0x8C,0xC7,0x46,0xBA,0x97,0x3B,0xA0,
        0x27,0x11,0x2D,0x48,0x50,0x3D,0x43,0x13,
        0x27,0x26,0x09,0x6A,0x28,0x31,0xE9,0x4F,
        0x2F,0x27,0x21,0xCD,0x1F,0x0F,0x14,0xFD,
        0x27,0x08,0x17,0xAC,0x19,0xE1,0xEE,0x81,
        0x27,0x06,0x46,0x31,0xD0,0xD9,0x8C,0x3A,
        0x2F,0x6C,0xC9,0x22,0x76,0xBF,0x2D,0xC0,
        0x27,0x59,0x94,0x9C,0x6F,0x00,0x99,0xD8,
        0x27,0x6B,0x80,0xAF,0x62,0xDA,0x3B,0xB0,
        0x2F,0x1E,0x9E,0x0B,0x1C,0x96,0xCC,0xB8,
        0x2F,0x9E,0x6C,0x3B,0x90,0x8B,0x99,0x63,
        0x2B,0xF1,0x2A,0x8E,0x19,0x08,0x92,0x5C,
        0x07,0x64,0x29,0x69,0x21,0x1F,0x74,0x04,
        0x2F,0x5F,0xDF,0x2B,0x94,0xEA,0x11,0xDC,
        0x27,0x03,0x8C,0x02,0x94,0x97,0xE9,0xFA,
        0x2B,0x98,0x04,0xE2,0xB9,0xD3,0xCA,0x1D,
        0x07,0xB6,0x5F,0xF8,0x49,0x32,0x5D,0xE1,
        0x2E,0xDE,0xA9,0xDA,0x52,0x08,0x1F,0x4F,
        0x0F,0x43,0xBA,0x71,0xD3,0x15,0x83,0xDE,
        0x0F,0x1D,0x57,0x70,0x35,0x6F,0xBB,0xE2,
        0x27,0x2B,0xAD,0xF0,0x15,0x9C,0xF8,0x85,
        0x29,0xAE,0x9B,0xCD,0x37,0xC8,0x20,0xA3,
        0x0F,0x73,0xC8,0x53,0xE0,0x6F,0x5C,0x13,
        0x0F,0x4A,0x21,0xC0,0xE2,0xD5,0xDC,0x57,
        0x0F,0xB7,0xED,0x6D,0xA0,0x9E,0xDA,0xA3,
        0x07,0x70,0x2A,0xF1,0xAA,0x56,0x45,0x1F,
        0x07,0xF4,0x7D,0x57,0x9F,0x15,0x92,0x34,
        0x07,0x1A,0x7B,0x3F,0x7E,0xE2,0x5E,0xD6,
        0x27,0x5E,0xC5,0x15,0xA1,0x73,0x87,0x4E,
        0x24,0x85,0xDA,0x47,0x8A,0xDD,0xA4,0xF1,
        0x0F,0xE9,0x59,0xD0,0xA7,0x01,0xE8,0xC7,
        0x27,0x66,0x37,0x72,0xAC,0x79,0xAD,0x5E,
        0x27,0x9E,0xBB,0xDB,0x4F,0xF7,0x0D,0xAD,
        0x2F,0xAE,0x85,0xB2,0xF8,0x01,0x6F,0x02,
        0x27,0x80,0xB8,0xC7,0x95,0xD7,0x64,0xCA,
        0x2A,0x24,0xE2,0x6C,0xA9,0xAA,0xF3,0xAB,
        0x07,0x06,0x04,0x0B,0xB4,0x8B,0x5F,0xA3,
        0x0F,0x81,0x66,0x1F,0xDC,0x76,0xB6,0x7B,
        0x2B,0x80,0x14,0x7C,0xBD,0xF3,0xA9,0xEF,
        0x0F,0x69,0x85,0xDF,0xAC,0x12,0x4F,0x1B,
        0x23,0xC0,0x55,0x05,0xE5,0x77,0xA8,0xD1,
        0x07,0xED,0xE9,0x86,0x13,0xDB,0xB6,0x2A,
        0x27,0x6A,0xF7,0x23,0x6F,0x81,0xDD,0xE0,
        0x27,0x55,0x4E,0xFC,0x58,0xD9,0xE5,0x63,
        0x2F,0x86,0xC8,0xCB,0xB6,0x60,0x13,0x2A,
        0x2F,0x6F,0x28,0x43,0x39,0x1C,0x28,0x3B,
        0x2F,0x2B,0xD4,0x95,0x09,0xC8,0x23,0xC9,
        0x27,0x9C,0x94,0xF8,0x88,0x7F,0xAB,0xED,
        0x2F,0x65,0x8D,0xF2,0x0E,0xBF,0xB5,0x15,
        0x27,0x64,0xA0,0x68,0x5C,0xC1,0x47,0x31,
        0x27,0x39,0x2D,0x05,0x01,0x16,0xD5,0x42,
        0x27,0xC4,0x6F,0xF1,0x9E,0x63,0xF8,0x95,
        0x2F,0xAC,0xD6,0xDB,0x24,0x2D,0x25,0xC2,
        0x27,0xB5,0xA5,0xA8,0x84,0x44,0xB5,0x73,
        0x2F,0x74,0x54,0xAE,0x96,0xDC,0xCB,0xCE,
        0x2F,0x46,0x51,0xE5,0x84,0x60,0xF8,0x10,
        0x22,0x61,0x94,0x75,0xEB,0x96,0x9B,0x90,
        0x07,0xB2,0xC3,0x35,0x94,0x82,0x68,0x74,
        0x23,0x04,0x44,0x4F,0x51,0xE3,0x1E,0xE0,
        0x0F,0x81,0x60,0x60,0x4C,0x82,0xEE,0x24,
        0x2F,0xE5,0x31,0xD5,0xBA,0xE6,0x9A,0xBC,
        0x2F,0x44,0x5C,0xF5,0x10,0x72,0x51,0x77,
        0x29,0xA4,0xF9,0x14,0xC3,0x67,0x94,0x41,
        0x07,0xF1,0xF6,0x20,0xC8,0xFA,0xF8,0x36,
        0x2F,0x53,0x2F,0x01,0x4C,0xE3,0x51,0x22,
        0x22,0x4C,0x54,0x37,0xC6,0x80,0x78,0x80,
        0x07,0x80,0xF4,0x6E,0xF5,0xD3,0xEB,0x32,
        0x2F,0x60,0xFC,0x4A,0x8E,0xFC,0x54,0x59,
        0x2F,0x16,0xF6,0xF2,0xCB,0x5D,0x7E,0x4F,
        0x24,0xE6,0x76,0xD5,0x20,0x47,0x28,0x95,
        0x0F,0x71,0x59,0xB2,0x69,0x5B,0x4A,0x54,
        0x25,0xA7,0xF5,0x4F,0xC6,0x83,0xB1,0xF8,
        0x07,0x64,0x1A,0x30,0x1B,0xA7,0x0A,0x96,
        0x07,0x2E,0x9C,0x49,0xBF,0x84,0x8F,0xB8,
        0x2F,0x70,0x58,0xBA,0x0F,0x52,0x22,0xEE,
        0x27,0x56,0x82,0x2C,0x69,0x74,0xDD,0x8C,
        0x27,0xF0,0x22,0x67,0xA8,0x27,0xE5,0x49,
        0x2B,0x24,0xA7,0xF6,0xB3,0x00,0xEB,0xF8,
        0x0F,0x80,0xB4,0x7B,0x2D,0x4F,0xFC,0x5D,
        0x2F,0xE2,0x9F,0x91,0x35,0x20,0x92,0xC3,
        0x2F,0x2E,0x6C,0xC9,0xBF,0x87,0xAF,0x15,
        0x27,0xBD,0x6D,0x88,0xA0,0xA9,0x12,0x81,
        0x27,0xD1,0x53,0xE6,0xAD,0x29,0xC5,0x88,
        0x2C,0xA4,0xE1,0x71,0x02,0x16,0x2F,0x40,
        0x0F,0x4C,0x89,0xBF,0xAC,0xA0,0xD1,0xC5,
        0x07,0x93,0x6A,0x4A,0xAE,0x49,0x9D,0x08,
        0x07,0xF5,0x0A,0x81,0x60,0x86,0xBD,0xC0,
        0x07,0x01,0xEA,0xBF,0x17,0x71,0xFD,0xC6,
        0x27,0xED,0x1C,0xDA,0xCF,0x1B,0x15,0xC3,
        0x24,0x42,0x35,0x96,0x40,0xF4,0x4B,0x66,
        0x0F,0x7E,0xED,0x10,0x9E,0x4A,0xB9,0x77,
        0x27,0x48,0x57,0xA5,0xC0,0xBA,0x45,0x3F,
        0x27,0x5D,0x5F,0x72,0xA6,0xF5,0x2F,0x05,
        0x2A,0xA2,0xFD,0x30,0x29,0x92,0xF6,0xDB,
        0x07,0xEF,0xD0,0x9E,0x85,0x66,0x66,0xA5,
        0x2F,0x7D,0x2B,0x8E,0x17,0x1D,0x62,0x5F,
        0x27,0xAA,0xFC,0xBB,0x33,0xD6,0x23,0xAE,
        0x07,0x8A,0xEA,0xED,0x27,0x94,0xA1,0x17,
        0x22,0xA4,0xF5,0xEC,0x4E,0x2B,0xC1,0x80,
        0x07,0xA2,0x9A,0xE7,0x65,0x02,0x67,0xCD,
        0x2F,0xEE,0x15,0x9E,0x44,0x05,0xD8,0xC1,
        0x07,0x4B,0x06,0x93,0xBE,0x69,0xF6,0x19,
        0x27,0x21,0x58,0x39,0x12,0xAE,0x93,0x88,
        0x27,0x50,0x6B,0xD6,0xE2,0xDD,0x7D,0xE7,
        0x27,0xA0,0xBF,0x06,0xA7,0x1E,0x1B,0x78,
        0x27,0xBB,0xDF,0x6C,0xBD,0xBF,0xF4,0xEA,
        0x2F,0xFB,0x3F,0x87,0x48,0x49,0x31,0xD5,
        0x2F,0x58,0x72,0x1E,0xC4,0x11,0x59,0x1A,
        0x2F,0x47,0x41,0xF9,0x52,0x81,0x24,0xE8,
        0x2F,0x65,0xF2,0x73,0xE3,0x11,0x13,0x3A,
        0x2F,0x10,0x8E,0xFD,0x53,0x64,0xC3,0x7D,
        0x2F,0xCD,0x92,0xAF,0x58,0x06,0x6D,0x90,
        0x2F,0x55,0xD5,0x96,0x9D,0x87,0x96,0xB7,
        0x27,0xA0,0x52,0x87,0x25,0x22,0x4C,0x8B,
        0x2F,0xD0,0x5E,0x58,0x13,0x6B,0x2C,0xD7,
        0x2E,0x69,0xAB,0x78,0x86,0x9B,0x7F,0x73,
        0x07,0x28,0xCB,0x9B,0xEF,0xAB,0xE2,0x4E,
        0x2F,0x6C,0xBB,0x96,0x6B,0xB9,0x80,0x18,
        0x21,0x8D,0xCD,0xFB,0xB0,0xC6,0xB3,0xF8,
        0x0F,0x88,0x39,0x17,0x78,0xD6,0x80,0x92,
        0x2F,0x46,0x17,0x34,0x41,0x56,0xFE,0x4D,
        0x2F,0xFB,0x4B,0xF8,0x94,0x54,0x83,0xB1,
        0x27,0xE1,0xAF,0x2F,0xDB,0xB9,0x41,0x08,
        0x27,0xEE,0x84,0xEB,0x50,0x8A,0xA4,0xF0,
        0x2F,0xD9,0x63,0x31,0x38,0xC3,0x3C,0xAA,
        0x27,0x06,0x04,0x9E,0x31,0x9D,0xC2,0xD6,
        0x2F,0xCC,0x64,0x4F,0xA2,0x65,0x79,0x56,
        0x2F,0x56,0x92,0xBB,0xF0,0xC0,0x5F,0x95,
        0x2F,0x2A,0x3D,0x86,0x04,0x22,0xDA,0xC5,
        0x27,0xFC,0x46,0x35,0x09,0xE0,0xC7,0x09,
        0x2F,0xC3,0x69,0x84,0x16,0x5F,0x1C,0xD1,
        0x27,0xC2,0x1B,0xB0,0xF6,0x03,0x45,0xD7,
        0x2F,0x05,0x16,0x68,0x82,0x28,0xE2,0xDB,
        0x29,0xCE,0x7C,0xEA,0xD0,0x15,0xF4,0x0A,
        0x0F,0xBF,0x3E,0xB1,0x21,0xFB,0xC6,0x39,
        0x2F,0xA0,0x28,0xF4,0x4C,0xBA,0x3E,0x07,
        0x2F,0x73,0x9E,0xD5,0x13,0x5D,0xCA,0xFF,
        0x29,0x21,0x59,0xE7,0x8A,0x78,0xEB,0x2B,
        0x0F,0xD6,0x24,0xEE,0x0D,0x6F,0xBF,0x6F,
        0x2F,0x2A,0xA2,0xE0,0xA2,0xD5,0x3C,0x87,
        0x27,0x70,0x63,0xCE,0x27,0x49,0x65,0x4A,
        0x2F,0xD8,0x06,0xAE,0x56,0xDD,0x73,0x85,
        0x2F,0x63,0x66,0x81,0x07,0xA7,0x7F,0x8E,
        0x27,0xE8,0x1F,0x8E,0xD2,0x81,0x09,0xE8,
        0x27,0x86,0xB6,0xAD,0x85,0x72,0x2F,0x56,
        0x27,0xD2,0xF4,0x24,0x60,0xF5,0x18,0x59,
        0x27,0x94,0x9D,0x73,0x47,0x55,0xEC,0xA1,
        0x2F,0x89,0xDC,0x83,0x74,0x88,0x20,0xD1,
        0x2F,0x45,0x75,0xC2,0x09,0xE7,0x60,0xA8,
        0x2F,0x90,0xEB,0xAF,0x72,0x7D,0xA4,0xF4,
        0x2F,0xFE,0x86,0x6F,0xA7,0xBA,0xB3,0x76,
        0x27,0xD5,0x1F,0x8B,0x9E,0xEB,0xED,0x0D,
        0x2F,0xA9,0x4F,0x0D,0x90,0xB4,0x98,0xAC,
        0x27,0xC6,0x43,0x32,0x34,0xD8,0xCC,0x66,
        0x2F,0xA3,0x3E,0x9F,0x17,0xF5,0x06,0x90,
        0x2F,0x4B,0xCF,0x0A,0xED,0xD6,0xCA,0x83,
        0x2F,0x62,0x12,0x24,0x94,0x13,0x50,0x67,
        0x2F,0xC9,0xC5,0xFE,0xA2,0x88,0x30,0xD7,
        0x27,0xC2,0x94,0x79,0x39,0x4F,0x2E,0x00,
        0x2F,0xAD,0xA3,0x04,0x78,0x5A,0xA8,0xCA,
        0x27,0xD6,0x34,0x5C,0x27,0x6F,0x70,0xAD,
        0x27,0xBB,0xDA,0x43,0x74,0x06,0xCD,0x42,
        0x2F,0x9F,0xDF,0x82,0x6C,0x52,0x65,0x30,
        0x2F,0xBE,0x2B,0xAA,0x2D,0x35,0xF4,0xD1,
        0x27,0x5B,0x02,0xC2,0x90,0x5B,0xC7,0x52,
        0x27,0x46,0xB7,0xF9,0x5C,0x82,0x45,0x7A,
        0x26,0xFF,0x46,0x8D,0xFB,0xCF,0xBD,0x71,
        0x07,0x89,0x70,0x42,0x03,0x70,0x81,0xDB,
        0x07,0xD4,0x4A,0xE5,0x4D,0x01,0x26,0x4D,
        0x07,0xFF,0x2D,0xBE,0x4A,0x6C,0xA9,0x2E,
        0x0F,0xAA,0xD1,0xC2,0x8E,0x98,0x34,0x22,
        0x0F,0x31,0x46,0x10,0xCF,0x3A,0x07,0xC1,
        0x0F,0x27,0x5C,0x08,0xAC,0xE5,0x07,0x9D,
        0x07,0xD3,0x91,0x7F,0xBF,0xFE,0xA8,0x8C,
        0x27,0x5F,0x01,0xD0,0x2D,0x01,0xC7,0x6A,
        0x2F,0xBD,0x14,0xB5,0xA7,0xC1,0x9F,0x50,
        0x27,0xC0,0x74,0x1F,0x26,0x5C,0x7E,0x8D,
        0x24,0x20,0x13,0x93,0x42,0x0D,0xF2,0x02,
        0x07,0x62,0xF6,0x4F,0x85,0x2A,0x2F,0x38,
        0x27,0x45,0x42,0xBD,0x73,0xC7,0xFC,0xAA,
        0x2F,0x0B,0xEA,0xCA,0xE2,0xAE,0x22,0xC2,
        0x27,0x35,0x7A,0x98,0xE3,0x3F,0x07,0x47,
        0x2F,0x7F,0x4E,0xE7,0x75,0x73,0xBD,0x31,
        0x27,0xDC,0x8D,0x57,0xDC,0x61,0x42,0xF1,
        0x27,0x3C,0x2B,0x37,0x03,0x97,0xC9,0xC6,
        0x2B,0x12,0x49,0x41,0xB1,0x45,0x40,0x81,
        0x07,0x28,0x29,0x7B,0x8A,0xFE,0x52,0xA2,
        0x07,0xBE,0xD2,0x5A,0x7E,0xF1,0x26,0xFD,
        0x07,0xAB,0x88,0x4A,0x29,0xA5,0xFE,0x4F,
        0x27,0xB9,0x3E,0xA1,0xB9,0x31,0xDC,0x97,
        0x2B,0x01,0x66,0xC1,0xA9,0x20,0x05,0x76,
        0x0F,0x69,0x67,0x6D,0x1D,0x79,0xEB,0x90,
        0x22,0x3F,0xDC,0x3E,0x30,0xE7,0x6F,0x82,
        0x07,0x21,0xA6,0x3D,0xC3,0x40,0x84,0x5D,
        0x27,0xFF,0x7A,0x8F,0xBC,0xC1,0x12,0xA1,
        0x27,0xBE,0xBC,0x58,0xF7,0x4E,0x1B,0x17,
        0x27,0x10,0x26,0x85,0x68,0x1B,0x03,0x40,
        0x2F,0x55,0x87,0xAE,0x18,0xE9,0x61,0xAC,
        0x2F,0x83,0x88,0xE3,0x44,0x36,0xB5,0x1F,
        0x2F,0x07,0xAC,0xA3,0xB6,0xB3,0xB3,0x7B,
        0x27,0xD3,0x95,0x7C,0x56,0xA0,0xCF,0xF9,
        0x2A,0x18,0x9F,0xA9,0x88,0xEB,0x3C,0xAD,
        0x07,0x2A,0xFA,0x95,0x46,0x70,0x26,0x7A,
        0x27,0x0F,0xBA,0xAD,0xC4,0xC3,0x72,0x60,
        0x2D,0x1E,0xC1,0xB9,0xB3,0x06,0x8E,0x39,
        0x07,0xB7,0xAA,0xA0,0x5F,0xE5,0xE5,0xE4,
        0x27,0xE6,0xA7,0xDB,0x69,0x37,0x87,0xB8,
        0x26,0xCF,0x19,0xE8,0x63,0xB4,0xD0,0xAB,
        0x07,0xF4,0x4B,0x11,0xF2,0x26,0x59,0x19,
        0x0F,0x90,0x3D,0xE6,0x4D,0x77,0xFD,0x23,
        0x07,0xFA,0x0C,0xB5,0x67,0xED,0xFE,0x01,
        0x0F,0x56,0xE8,0x48,0x16,0xF4,0xAB,0x07,
        0x2F,0xF0,0xB7,0xFD,0xB8,0x8F,0x11,0xF9,
        0x27,0xD5,0x61,0x33,0xA8,0x05,0x50,0x27,
        0x07,0xA5,0x18,0xF5,0x1D,0x0C,0xA7,0x27,
        0x2D,0x29,0xE3,0xC7,0x5A,0xAC,0x69,0x99,
        0x07,0x2C,0xB5,0xEA,0x3B,0x66,0x25,0x55,
        0x27,0x9E,0xCB,0xE3,0xC5,0x14,0x0B,0xC0,
        0x2F,0xB2,0x5A,0xDB,0x09,0x66,0x3B,0x5E,
        0x27,0x08,0x5F,0x5D,0x24,0x63,0xDD,0x16,
        0x2F,0x35,0x60,0xE0,0xD0,0x77,0xAB,0xD0,
        0x2F,0x55,0x65,0x7E,0x13,0xE4,0xF8,0x01,
        0x27,0x86,0xE3,0x69,0xE8,0x77,0x20,0xAE,
        0x27,0xFB,0x42,0x8D,0x13,0x16,0x75,0x6C,
        0x27,0x90,0xF7,0x07,0xEF,0x66,0xF5,0x71,
        0x27,0xCD,0x03,0xA6,0xEC,0xA4,0x60,0xC8,
        0x2F,0xFD,0xB9,0x79,0xCE,0xDA,0x23,0x31,
        0x27,0xC0,0x7B,0x98,0x7E,0x17,0x35,0x10,
        0x27,0x9B,0xEB,0x75,0x40,0x67,0xE4,0x39,
        0x27,0xB3,0xBD,0xB0,0x56,0x43,0x63,0xF7,
        0x27,0x86,0xD7,0xD0,0x62,0x4E,0x39,0xD5,
        0x2F,0xDE,0x85,0x5F,0x5A,0x3D,0xDF,0x91,
        0x2F,0x48,0xA6,0xC9,0x9D,0x1A,0x1A,0x90,
        0x2F,0x19,0xEB,0xEF,0x76,0x03,0x20,0x48,
        0x2F,0xF9,0x8D,0xD8,0x9D,0xFE,0x6D,0x2B,
        0x2B,0x03,0x30,0x76,0x42,0xB2,0x8D,0x35,
        0x07,0x6A,0x2B,0x8F,0xEC,0x5F,0xCF,0x38,
        0x2E,0x21,0x99,0x24,0xE4,0x18,0xFC,0xD7,
        0x07,0x94,0xFB,0xC8,0x02,0x46,0xD4,0x7E,
        0x2F,0xF6,0x07,0x2A,0x23,0x43,0xB2,0x7E,
        0x27,0x4B,0xDC,0xE1,0xCD,0x6B,0x50,0x3D,
        0x27,0x33,0x95,0x08,0xDF,0x44,0xC5,0xEA,
        0x2F,0x21,0x26,0xDE,0xE2,0x1C,0x1A,0x4E,
        0x27,0x7E,0x3C,0x8B,0xDC,0x3F,0xBA,0xCE,
        0x27,0x62,0xCE,0xA5,0x8B,0x9C,0x06,0xA2,
        0x27,0xDF,0xB8,0xDC,0x04,0x1C,0x3E,0xC2,
        0x2F,0x1E,0xA2,0x3D,0xF7,0xD1,0x02,0xF2,
        0x2F,0x9D,0x36,0x19,0xC0,0x91,0x17,0xAB,
        0x07,0xCA,0x1D,0x37,0x12,0xCB,0x0B,0x55,
        0x27,0x7C,0xB0,0x44,0xD4,0xA5,0xF5,0x30,
        0x27,0x62,0x1A,0x97,0xEF,0x50,0x4C,0xC9,
        0x2D,0x95,0x03,0x5F,0x1A,0x25,0x1E,0xA5,
        0x0F,0x80,0xC1,0x2F,0xF2,0x4A,0x10,0x3D,
        0x27,0xBC,0x35,0x42,0xFA,0x1C,0x45,0x37,
        0x2F,0xAA,0x94,0xEF,0xF4,0x5B,0x5B,0x61,
        0x2F,0x1E,0x9E,0xF6,0xDA,0x5A,0x57,0x7A,
        0x27,0xA5,0x0F,0xD4,0x16,0x96,0xEF,0xE6,
        0x27,0x58,0x34,0x82,0x19,0x99,0xB0,0x9B,
        0x21,0x2A,0x53,0x79,0x44,0x00,0xB7,0xAF,
        0x07,0x56,0x2F,0xEB,0xBD,0x76,0x41,0x2C,
        0x2F,0x43,0xDD,0x24,0xFB,0x3C,0x14,0xE8,
        0x27,0x8B,0x36,0x54,0xD1,0x1F,0x6C,0xDC,
        0x27,0x1A,0x7F,0x43,0x5B,0xBB,0xC6,0xCC,
        0x2F,0x48,0xFB,0xE3,0x11,0xF4,0xA3,0x66,
        0x27,0x75,0x97,0x10,0x0C,0xC3,0x0C,0x71,
        0x2F,0x8B,0x53,0x99,0xAB,0x47,0xF1,0xE1,
        0x2F,0xC2,0x2F,0x65,0x45,0x15,0x57,0x7E,
        0x2F,0x83,0x03,0xF5,0xC8,0xF0,0xDA,0xBB,
        0x27,0x59,0x70,0x80,0x92,0xC1,0x60,0x0E,
        0x27,0xF4,0x2E,0x33,0xD4,0xA8,0xB8,0x25,
        0x2F,0xD4,0x4C,0xEB,0xC4,0x22,0x8A,0x10,
        0x27,0xC7,0x75,0x6F,0x0E,0x9C,0x44,0x84,
        0x29,0x16,0xCE,0x7A,0xB2,0x87,0x83,0x31,
        0x07,0x4E,0x73,0xF1,0x5E,0xE9,0x1A,0x3D,
        0x2F,0xE8,0x1F,0x74,0x59,0x17,0x01,0xB8,
        0x27,0x06,0x85,0x3C,0xE9,0x5D,0x39,0xAC,
        0x29,0x1E,0xDC,0x2F,0x0E,0xCD,0xB5,0xEC,
        0x07,0x5C,0x20,0x8E,0x2F,0x98,0xAD,0x49,
        0x2F,0xC5,0x88,0x9B,0x27,0x58,0xB0,0x1E,
        0x2F,0xCD,0xA0,0x0D,0xA1,0x66,0xFC,0x9F,
        0x2F,0xE2,0xE2,0x15,0xC4,0x0C,0x70,0x58,
        0x27,0xC3,0x09,0xC5,0xD6,0xC9,0x4E,0x9E,
        0x27,0xB6,0xE5,0x47,0xFD,0x57,0x34,0x0D,
        0x27,0xDF,0xD3,0xA3,0xB1,0x5F,0x7E,0xC0,
        0x2F,0xA3,0x21,0xC0,0x1B,0x95,0x79,0x98,
        0x2F,0x86,0x84,0xBB,0xB6,0xE7,0x88,0xF1,
        0x0F,0xCA,0x77,0x1E,0x0B,0xA5,0x37,0x13,
        0x07,0x50,0xF8,0x58,0xCF,0xE1,0x58,0x22,
        0x2F,0xE5,0xE6,0xFB,0x18,0x64,0x19,0xBE,
        0x2D,0x36,0x61,0x4E,0xB4,0x78,0xB4,0xA5,
        0x0F,0x0A,0x18,0xDA,0x40,0x68,0xC0,0x04,
        0x0F,0x51,0x25,0xD4,0x50,0xAB,0xF9,0x32,
        0x0F,0x4E,0x52,0xAA,0xC2,0x37,0x44,0x85,
        0x27,0xB8,0x0F,0x4F,0x00,0xC7,0xED,0xA3,
        0x21,0xBE,0x47,0x90,0x17,0x3D,0x90,0x3F,
        0x0F,0x07,0x53,0x5A,0xDB,0x70,0x54,0xBC,
        0x27,0x4D,0x0F,0xC0,0xF8,0xC6,0xDF,0xC8,
        0x07,0x49,0x0F,0x96,0xCF,0x39,0x5E,0x40,
        0x2F,0x51,0x56,0xAC,0xA3,0xEA,0xD8,0x41,
        0x27,0xAB,0x88,0x73,0x49,0x29,0x41,0x5E,
        0x27,0xA4,0x99,0x6A,0x62,0x58,0x90,0xBB,
        0x22,0x69,0x4D,0x66,0x01,0x42,0x2B,0xBE,
        0x07,0x34,0x3F,0x8B,0xA0,0x8E,0x9E,0x1C,
        0x0F,0xF9,0x97,0x90,0x1D,0x32,0xC8,0xEA,
        0x0F,0x5F,0xA0,0x3D,0x8C,0x76,0x71,0x60,
        0x2E,0xEC,0x3A,0x43,0xF2,0x29,0xBF,0x42,
        0x0F,0xD0,0x0B,0xBB,0x6F,0x4B,0xD2,0xDD,
        0x07,0x74,0x5C,0x6A,0xEB,0xC6,0x84,0x6F,
        0x07,0xA6,0x3B,0x51,0xE6,0x41,0x09,0x4E,
        0x2F,0xB4,0xC5,0xB5,0xBF,0x46,0x9E,0x88,
        0x2B,0x8C,0x54,0x27,0x92,0x47,0x0A,0x3B,
        0x07,0x0C,0x46,0x8B,0x54,0x02,0x2F,0x27,
        0x2C,0x60,0xB3,0xB8,0xA8,0xEF,0xFB,0x9F,
        0x07,0xFD,0x9C,0x61,0xD8,0x05,0x0C,0x8B,
        0x07,0xA9,0x42,0xE0,0x86,0x17,0xE7,0x61,
        0x07,0x49,0xE0,0xD4,0x06,0xC6,0xF9,0x96,
        0x27,0x0A,0x8C,0xA7,0x17,0x92,0x6C,0x3E,
        0x21,0x82,0x2A,0xB1,0x69,0x4F,0xB4,0xCF,
        0x07,0x79,0x59,0x4D,0x76,0xAD,0x9D,0x4D,
        0x0F,0x06,0x01,0xE6,0x4E,0xCE,0x24,0x9D,
        0x0F,0x0B,0xEC,0xA2,0xE5,0xB1,0x6D,0x04,
        0x2F,0x99,0x4E,0xD2,0x95,0x7D,0xEE,0x3B,
        0x2B,0xF0,0x5E,0x3C,0xAA,0x8B,0x7D,0x29,
        0x07,0x55,0x45,0x57,0x62,0x6E,0x3D,0xD5,
        0x07,0xAC,0x90,0x56,0x1F,0xC0,0x38,0xDD,
        0x07,0x8F,0xBA,0x99,0x0A,0x7D,0x17,0xA2,
        0x2F,0x03,0x66,0x01,0xBA,0xB9,0x3B,0x0C,
        0x23,0x75,0xE3,0x4E,0x7C,0x02,0x2F,0xA7,
        0x07,0xEC,0x07,0xDD,0x14,0xB4,0xBB,0x6E,
        0x07,0x28,0x42,0xC9,0xD9,0xF3,0x72,0x3D,
        0x27,0xE4,0x25,0x0F,0x4D,0xB5,0x36,0x5C,
        0x2F,0x6D,0x45,0xB1,0xC9,0x49,0x53,0x4D,
        0x2F,0x76,0x4E,0xD2,0xC9,0xFB,0xFF,0x17,
        0x22,0x92,0x7B,0x16,0xBC,0x60,0x15,0x88,
        0x07,0xD9,0xB5,0xAE,0x12,0x37,0x58,0xEE,
        0x27,0x57,0x32,0x63,0xD1,0x3B,0xA3,0xC1,
        0x27,0x21,0xDD,0xC2,0x1A,0x71,0xE4,0xD0,
        0x27,0x77,0x8E,0x8F,0x8E,0x75,0xC3,0xA9,
        0x27,0x90,0xF1,0x15,0x65,0x94,0xFD,0xC7,
        0x23,0xBD,0x0C,0xC0,0x0D,0xA5,0xE8,0x09,
        0x0F,0x82,0x83,0x29,0xFD,0x19,0xAB,0xBB,
        0x27,0xEB,0x27,0x17,0xF7,0x0E,0x80,0xA8,
        0x2F,0xC0,0xC8,0x99,0x86,0xB7,0xFC,0x76,
        0x29,0x5B,0x35,0x60,0xBA,0x3C,0xF4,0x67,
        0x07,0xB9,0x68,0xC2,0xC3,0x13,0x2F,0xE4,
        0x27,0x7B,0xEB,0x83,0x88,0xB4,0x67,0x34,
        0x2B,0xF8,0x44,0x85,0x99,0x6E,0x4F,0x8C,
        0x07,0x06,0x8F,0x33,0xC7,0xA3,0xE1,0x3E,
        0x07,0x05,0x03,0x94,0x18,0x20,0x41,0x67,
        0x07,0xFB,0x2D,0x7F,0x6C,0x1E,0x8D,0x53,
        0x05,0x68,0x9A,0x84,0x63,0x09,0x21,0x7F,
        0x25,0xE2,0x76,0x81,0x23,0x4D,0x12,0x42,
        0x07,0xD5,0x01,0x64,0x2E,0x98,0x7B,0x7E,
        0x07,0x68,0xF3,0x24,0x93,0x10,0x6B,0x5E,
        0x07,0xDC,0xF6,0x6F,0x0D,0x6B,0xF0,0x60,
        0x0F,0x14,0x1F,0x6C,0xFD,0x1D,0x91,0x9B,
        0x0F,0xF3,0x71,0xFB,0x14,0xAE,0x6D,0xE3,
        0x0F,0x7A,0x50,0x65,0xB6,0x2F,0x9E,0x84,
        0x0F,0x03,0xFD,0x59,0x4F,0xBE,0xF5,0xED,
        0x0F,0x1A,0xF6,0xE5,0x5B,0xFE,0xF6,0x15,
        0x0F,0x32,0x3E,0x12,0x12,0xC4,0x8E,0x05,
        0x07,0x25,0xD1,0xDD,0xD7,0x18,0x3F,0xFB,
        0x0F,0x4C,0xBB,0x27,0xDB,0x1C,0xF4,0x6F,
        0x0F,0x05,0x37,0xE8,0xA3,0xEC,0x4A,0x46,
        0x07,0xD5,0x15,0x31,0xFA,0x01,0x12,0x00,
        0x07,0xA9,0xEF,0x47,0xE3,0x57,0x98,0x67,
        0x2B,0xC5,0xB2,0xD6,0x3E,0x87,0xB6,0xA8,
        0x0F,0xD9,0xE8,0x88,0xE9,0x66,0xEA,0xDB,
        0x0F,0xF6,0x57,0x2C,0xC3,0x43,0xF4,0x2E,
        0x0F,0x9B,0xD5,0x74,0xA2,0x42,0x5F,0xFC,
        0x07,0x2C,0x8D,0x4A,0x2E,0x36,0xE8,0x23,
        0x0F,0x6A,0x0C,0xAD,0xDD,0x50,0xD7,0xAE,
        0x2F,0xAE,0x85,0x7F,0x2F,0xE5,0x02,0xA0,
        0x24,0xCA,0xDE,0xF5,0xE9,0x32,0x37,0xF7,
        0x0F,0xE5,0x17,0xFE,0x5D,0x97,0x4E,0x73,
        0x22,0x80,0x1C,0xF3,0xBD,0x8B,0x99,0x6C,
        0x07,0x62,0x50,0x0A,0x95,0x32,0xBE,0x86,
        0x0F,0x3D,0x2F,0xEA,0x81,0x8F,0x0B,0xC7,
        0x07,0x7E,0x0F,0xE4,0x1F,0xD5,0x97,0x55,
        0x0F,0xED,0x79,0x75,0xD2,0xF5,0x3F,0xF0,
        0x0F,0xEE,0x80,0x5D,0xF2,0xAF,0xCD,0xB9,
        0x05,0xC3,0x4F,0x1C,0xB2,0x9A,0xEA,0xE3,
        0x27,0x87,0xB1,0xC0,0x06,0x26,0xAB,0x90,
        0x0F,0xC2,0xE2,0x5C,0x3D,0xBA,0xF1,0xE8,
        0x07,0x37,0x30,0x60,0x40,0x11,0x43,0xA6,
        0x0F,0x99,0x26,0x3F,0x88,0x13,0xAB,0x8B,
        0x2F,0xFA,0xC3,0xDB,0xBF,0x0B,0x1E,0xA2,
        0x29,0xCA,0x2C,0xE2,0xC9,0xFB,0x05,0x10,
        0x0F,0x73,0x2B,0xA3,0xE1,0x6A,0x93,0xFA,
        0x05,0xE7,0x2E,0xFA,0x40,0xC6,0x41,0xF2,
        0x2D,0x4E,0x2F,0x31,0xF4,0xD3,0xB9,0xBB,
        0x05,0xF2,0x8E,0x48,0xDE,0x91,0x5B,0x6A
};

static unsigned char firmwaretable_b30[] = {
        0x04,0x01,0x01,0x00,0xEE,0xA4,0x94,0x17,
        0x07,0x05,0x99,0x31,0x5B,0xB5,0x2D,0x90,
        0x0F,0x76,0x41,0x39,0xE7,0x17,0xF3,0xA5,
        0x05,0xBC,0x8B,0x61,0x0A,0xF1,0x3F,0x10,
        0x22,0x52,0x13,0xFD,0xB0,0x7F,0x05,0x53,
        0x05,0x01,0x7A,0x86,0x58,0x91,0xD6,0xA4,
        0x22,0xC4,0xB5,0x5C,0xB8,0xD3,0x11,0xDE,
        0x05,0x20,0x20,0x2A,0x43,0x44,0x4F,0x28,
        0x22,0x87,0x6D,0x1E,0x90,0xE2,0xC6,0x02,
        0x05,0x13,0x29,0x80,0x96,0x0D,0x29,0x15,
        0x2A,0x7C,0x0C,0xA6,0x50,0xA0,0x62,0xD1,
        0x05,0xCA,0x09,0xC0,0xA4,0x9D,0x4C,0x68,
        0x2A,0xA6,0xC8,0x75,0x64,0x76,0x6D,0xC8,
        0x05,0x81,0x1D,0x07,0x71,0x53,0x14,0xAC,
        0x22,0x7B,0x4D,0x6D,0x86,0x15,0xEA,0xD0,
        0x05,0x05,0xC3,0x11,0xEF,0x16,0x70,0x74,
        0x2A,0x98,0xD8,0xBF,0xAF,0xC9,0xDA,0xE6,
        0x05,0x7D,0xE7,0xA5,0x3F,0xA6,0x52,0xBC,
        0x2F,0x1D,0x03,0x92,0x90,0x97,0xE5,0x39,
        0x29,0x85,0x25,0xD5,0xF4,0xE8,0xED,0x7F,
        0x0F,0xB0,0xB7,0xD4,0xEB,0x5B,0x69,0xD8,
        0x05,0xDD,0x8B,0xFB,0xD2,0xD6,0x6F,0x0D,
        0x22,0x4D,0xBE,0xBB,0xF9,0xAA,0x15,0x9B,
        0x0F,0xB4,0x87,0xC2,0xDB,0x5C,0x7D,0xDB,
        0x22,0xDD,0x93,0xED,0x25,0x7A,0xFC,0xB7,
        0x05,0xE5,0xB1,0x9C,0xA4,0xC5,0x3E,0x3A,
        0x27,0x6E,0x5F,0x8B,0xDF,0x77,0x16,0x1A,
        0x27,0x89,0x03,0xDC,0x00,0x16,0x61,0xA0,
        0x2F,0x05,0x92,0xA0,0x54,0xCC,0x06,0x96,
        0x27,0x3F,0x94,0x85,0x36,0x55,0x40,0x85,
        0x27,0xD2,0x2D,0x3E,0xAD,0x44,0x3D,0xFB,
        0x2F,0x5B,0x25,0xF1,0xC5,0xD8,0x8B,0x7D,
        0x27,0x55,0x85,0xAC,0x60,0x06,0xD4,0x22,
        0x2F,0xB2,0x15,0x29,0xE3,0x5D,0xEE,0x3F,
        0x2F,0x57,0xC6,0x33,0xA3,0xDA,0xED,0xF8,
        0x27,0x00,0x45,0x66,0x8C,0x0B,0xB2,0xC6,
        0x2F,0x66,0x5A,0xC7,0xEF,0x78,0x6B,0xB0,
        0x2F,0x88,0xB4,0x2B,0xD9,0x93,0x22,0xFD,
        0x27,0x86,0x29,0x2E,0xEA,0x16,0xB6,0x8D,
        0x2F,0x65,0x8F,0x33,0xCF,0xB7,0xEE,0x70,
        0x27,0xB2,0xD1,0xB5,0x3D,0xDE,0x07,0x4E,
        0x27,0xBB,0x13,0xAC,0xF8,0xBC,0xAD,0x02,
        0x2E,0x87,0x86,0xBD,0x81,0x07,0xB9,0xBD,
        0x07,0x6F,0x55,0x74,0x26,0x02,0x87,0xD3,
        0x2F,0xE4,0x16,0x64,0xC7,0xA0,0xCA,0x0E,
        0x27,0xF3,0x00,0xEC,0x26,0xD5,0xB0,0x52,
        0x2F,0x01,0xB2,0x7E,0xDF,0x2E,0x08,0x1F,
        0x0F,0xDD,0xF7,0x94,0x2F,0xAE,0xE3,0xBE,
        0x27,0x65,0xC4,0x5B,0x02,0xCB,0x22,0xE8,
        0x2F,0x60,0x59,0xA1,0x48,0xE8,0xAF,0xD8,
        0x2F,0xC3,0xA1,0x9F,0x82,0xD6,0x5F,0x30,
        0x27,0xCF,0x75,0x5F,0x3F,0x63,0xF2,0x84,
        0x2F,0x9E,0xEF,0xD3,0x4D,0x50,0xB2,0xC5,
        0x27,0x4D,0xAA,0x58,0x3C,0x0C,0x8C,0x1B,
        0x2F,0xE7,0x86,0xC6,0x35,0x71,0x8F,0x45,
        0x27,0x0C,0x6A,0xE7,0x36,0xEA,0xC6,0x1B,
        0x2F,0x18,0x86,0x76,0xD8,0x5E,0x90,0x29,
        0x21,0x30,0x30,0x4D,0xF8,0xA9,0x19,0x3A,
        0x0F,0x58,0xFF,0x4F,0x6A,0x07,0xE0,0x4D,
        0x27,0x14,0x14,0x07,0x3A,0x86,0x98,0xDD,
        0x2F,0x3C,0x67,0x4B,0x2A,0x4C,0x96,0xA2,
        0x2F,0xF4,0x54,0x6C,0xDD,0xF6,0x5A,0x37,
        0x27,0x80,0x77,0x8D,0x5C,0x8B,0xFF,0xAA,
        0x2F,0x6F,0x2C,0x2D,0x2D,0x6A,0x90,0xFB,
        0x2F,0x56,0x56,0x07,0x28,0xBB,0x3C,0xCF,
        0x27,0x10,0xEE,0x04,0x36,0x67,0x61,0x53,
        0x27,0xC1,0x8A,0x35,0xFB,0xB2,0xA6,0xF2,
        0x27,0x74,0xD0,0x1E,0x66,0x38,0xB6,0x6A,
        0x27,0x87,0x65,0xF9,0xF7,0x3C,0xC4,0x7B,
        0x27,0x6E,0xCA,0x12,0xD8,0x5C,0x41,0x23,
        0x2F,0x8D,0xA6,0x2E,0xAF,0xFF,0x18,0x0A,
        0x2F,0xB7,0xEC,0xE7,0xFD,0xCA,0x91,0x37,
        0x27,0x39,0xE0,0x6E,0x20,0x25,0xA3,0x77,
        0x2F,0x4E,0xDC,0x4E,0xB6,0x00,0x40,0x3E,
        0x22,0xC9,0xA3,0x33,0xB2,0x64,0xBE,0xAA,
        0x0F,0xBB,0xC1,0x23,0x71,0x27,0xDD,0xC6,
        0x0F,0xC4,0xEE,0xA9,0xBD,0x0C,0x8E,0x7F,
        0x07,0x0C,0xE5,0xC9,0xE5,0x44,0x76,0xA8,
        0x22,0x6C,0xB6,0x00,0x41,0xB8,0x7E,0x92,
        0x07,0x60,0x69,0xB8,0x5E,0x61,0xAE,0x69,
        0x0F,0xF2,0x06,0xEC,0xFC,0x2F,0xD4,0x37,
        0x07,0x22,0x79,0x99,0xEC,0x08,0xD4,0x59,
        0x07,0x1D,0xBF,0x96,0x14,0xB5,0x1C,0x15,
        0x0F,0x5C,0xA5,0xDE,0x6A,0x2E,0xC5,0xFE,
        0x0F,0x96,0x26,0x6D,0x1D,0x22,0xE7,0xE3,
        0x05,0xA7,0x01,0x13,0x2A,0x36,0xC3,0xF5,
        0x2A,0x5A,0x72,0x4B,0x87,0x71,0x59,0x87,
        0x05,0x93,0xE2,0x4A,0x49,0xD7,0x6E,0x69,
        0x23,0x1F,0x25,0x60,0x6E,0x1D,0x50,0x4A,
        0x07,0x86,0xB1,0x82,0x29,0x46,0xBF,0x3E,
        0x05,0x3E,0xD8,0x04,0x7D,0x4B,0xEB,0x68
};


//#define NB_LINES (sizeof(firmwaretable)/(8*sizeof(unsigned char)))

#ifdef USING_ATV_FILTER
static unsigned char dlif_vidfilt_table[] = {0};
#define DLIF_VIDFILT_LINES (sizeof(dlif_vidfilt_table)/(8*sizeof(unsigned char)))
#endif

static si2176_common_reply_struct  reply;
/************************************************************************************************************************
NAME:		   si2176_readcommandbytes function
DESCRIPTION:Read inbbytes from the i2c device into pucdatabuffer, return number of bytes read
Parameter: iI2CIndex, the index of the first byte to read.
Parameter: inbbytes, the number of bytes to read.
Parameter: *pucdatabuffer, a pointer to a buffer used to store the bytes
Porting:    Replace with embedded system I2C read function
Returns:    Actual number of bytes read.
 ************************************************************************************************************************/
static int si2176_readcommandbytes(struct i2c_client *si2176, int inbbytes, unsigned char *pucdatabuffer)
{
        int i2c_flag = 0;
        int i = 0;
        unsigned int i2c_try_cnt = I2C_TRY_MAX_CNT;

        struct i2c_msg msg[] = {
                {
                        .addr  = si2176->addr,
                        .flags  = I2C_M_RD,
                        .len     = inbbytes,
                        .buf     = pucdatabuffer,
                },
        };

repeat:
        i2c_flag = i2c_transfer(si2176->adapter, msg, 1);
        if (i2c_flag < 0) {
                pr_err("%s: error in read sli2176, %d byte(s) should be read,. \n", __func__, inbbytes);
                if (i++ < i2c_try_cnt) {
                        pr_err("%s: error in read sli2176, try again!!!\n", __func__);
                        goto repeat;
                }
                else
                        return -EIO;
        }
        else {
                //pr_info("%s: read %d bytes\n", __func__, inbbytes);
                return inbbytes;
        }
}

/************************************************************************************************************************
NAME:  si2176_writecommandbytes
DESCRIPTION:  Write inbbytes from pucdatabuffer to the i2c device, return number of bytes written
Porting:    Replace with inbbytes system I2C write function
Returns:    Number of bytes written
 ************************************************************************************************************************/
static int si2176_writecommandbytes(struct i2c_client *si2176, int inbbytes, unsigned char *pucdatabuffer)
{
        int i2c_flag = 0;
        int i = 0;
        unsigned int i2c_try_cnt = I2C_TRY_MAX_CNT;

        struct i2c_msg msg[] = {
                {
                        .addr	= si2176->addr,
                        .flags	= 0,    //|I2C_M_TEN,
                        .len	= inbbytes,
                        .buf	= pucdatabuffer,
                }

        };

repeat:
        i2c_flag = i2c_transfer(si2176->adapter, msg, 1);
        if (i2c_flag < 0) {
                pr_err("%s: error in write sli2176, %d byte(s) should be read,. \n", __func__, inbbytes);
                if (i++ < i2c_try_cnt) {
                        pr_err("%s: error in wirte sli2176, try again!!!\n", __func__);
                        goto repeat;
                }
                else
                        return -EIO;
        }
        else {
                //pr_info("%s: write %d bytes\n", __func__, inbbytes);
                return inbbytes;
        }
}

/***********************************************************************************************************************
  sli2176_pollforcts function
Use:        CTS checking function
Used to check the CTS bit until it is set before sending the next command
Comments:   The status byte definition being identical for all commands,
using this function to fill the status structure hels reducing the code size
Comments:   waitForCTS = 1 => I2C polling
waitForCTS = 2 => INTB followed by a read (reading a HW byte using the cypress chip)
max timeout = 100 ms

Porting:    If reading INTB is not possible, the waitForCTS = 2 case can be removed

Parameter: waitForCTS          a flag indicating if waiting for CTS is required
Returns:   1 if the CTS bit is set, 0 otherwise
 ***********************************************************************************************************************/
static unsigned char si2176_pollforcts(struct i2c_client *si2176)
{
        unsigned char error_code = 0;
        unsigned char loop_count = 0;
        unsigned char rspbytebuffer[1];

        for (loop_count=0; loop_count<50; loop_count++) { /* wait a maximum of 50*25ms = 1.25s  */
                if (si2176_readcommandbytes(si2176, 1, rspbytebuffer) != 1)
                        error_code = ERROR_SI2176_POLLING_CTS;
                else
                        error_code = NO_SI2176_ERROR;
                if (error_code || (rspbytebuffer[0] & 0x80))
                        goto exit;
                mdelay(2); /* CTS not set, wait 2ms and retry */
        }
        error_code = ERROR_SI2176_CTS_TIMEOUT;

exit:
        if (error_code)
                pr_info("%s: poll cts function error:%d!!!...............\n", __func__, error_code);

        return error_code;
}

/***********************************************************************************************************************
  SI2176_CurrentResponseStatus function
Use:        status checking function
Used to fill the SI2176_COMMON_REPLY_struct members with the ptDataBuffer byte's bits
Comments:   The status byte definition being identical for all commands,
using this function to fill the status structure hels reducing the code size

Parameter: *ret          the SI2176_COMMON_REPLY_struct
Parameter: ptDataBuffer  a single byte received when reading a command's response (the first byte)
Returns:   0 if the err bit (bit 6) is unset, 1 otherwise
 ***********************************************************************************************************************/
static unsigned char si2176_currentresponsestatus(si2176_common_reply_struct *common_reply, unsigned char ptdatabuffer)
{
        /* _status_code_insertion_start */
        common_reply->tunint = ((ptdatabuffer >> 0 ) & 0x01);
        common_reply->atvint = ((ptdatabuffer >> 1 ) & 0x01);
        common_reply->dtvint = ((ptdatabuffer >> 2 ) & 0x01);
        common_reply->err    = ((ptdatabuffer >> 6 ) & 0x01);
        common_reply->cts    = ((ptdatabuffer >> 7 ) & 0x01);
        /* _status_code_insertion_point */
        return (common_reply->err ? ERROR_SI2176_ERR : NO_SI2176_ERROR);
}

/***********************************************************************************************************************
  si2176_pollforresponse function
Use:        command response retrieval function
Used to retrieve the command response in the provided buffer,
poll for response either by I2C polling or wait for INTB
Comments:   The status byte definition being identical for all commands,
using this function to fill the status structure hels reducing the code size
Comments:   waitForCTS = 1 => I2C polling
waitForCTS = 2 => INTB followed by a read (reading a HW byte using the cypress chip)
max timeout = 100 ms

Porting:    If reading INTB is not possible, the waitForCTS = 2 case can be removed

Parameter:  waitforresponse  a flag indicating if waiting for the response is required
Parameter:  nbbytes          the number of response bytes to read
Parameter:  pbytebuffer      a buffer into which bytes will be stored
Returns:    0 if no error, an error code otherwise
 ***********************************************************************************************************************/
static unsigned char si2176_pollforresponse(struct i2c_client *si2176, unsigned char waitforresponse, unsigned int nbbytes, unsigned char *pbytebuffer, si2176_common_reply_struct *common_reply)
{
        unsigned char error_code;
        unsigned char loop_count;

        for (loop_count=0; loop_count<50; loop_count++) { /* wait a maximum of 50*2ms = 100ms                        */
                switch (waitforresponse) { /* type of response polling?                                */
                        case 0 : /* no polling? valid option, but shouldn't have been called */
                                error_code = NO_SI2176_ERROR; /* return no error                                          */
                                goto exit;

                        case 1 : /* I2C polling status?                                      */
                                if (si2176_readcommandbytes(si2176, nbbytes, pbytebuffer) != nbbytes)
                                        error_code = ERROR_SI2176_POLLING_RESPONSE;
                                else
                                        error_code = NO_SI2176_ERROR;
                                if (error_code)
                                        goto exit;	/* if error, exit with error code */
                                if (pbytebuffer[0] & 0x80)	  /* CTS set? */
                                {
                                        error_code = si2176_currentresponsestatus(common_reply, pbytebuffer[0]);
                                        goto exit; /* exit whether ERR set or not   */
                                }
                                break;

                        default :
                                error_code = ERROR_SI2176_PARAMETER_OUT_OF_RANGE; /* support debug of invalid CTS poll parameter   */
                                goto exit;
                }
                mdelay(2); /* CTS not set, wait 2ms and retry                         */
        }
        error_code = ERROR_SI2176_CTS_TIMEOUT;

exit:
        return error_code;
}

/************************************************************************************************************************
NAME: CheckStatus
DESCRIPTION:     Read Si2170 STATUS byte and return decoded status
Parameter:  Si2170 Context (I2C address)
Parameter:  Status byte (TUNINT, ATVINT, DTVINT, ERR, CTS, CHLINT, and CHL flags).
Returns:    Si2170/I2C transaction error code
 ************************************************************************************************************************/
#if 0
static int si2176_check_status(struct i2c_client *si2176, si2176_common_reply_struct *common_reply)
{
        unsigned char buffer[1];
        /* read STATUS byte */
        if (si2176_pollforresponse(si2176, 1, 1, buffer, common_reply) != 0)
        {
                return ERROR_SI2176_POLLING_RESPONSE;
        }

        return 0;
}
#endif
/***********************************************************************************************************************
NAME: Si2170_L1_API_Patch
DESCRIPTION: Patch information function
Used to send a number of bytes to the Si2170. Useful to download the firmware.
Parameter:   *api    a pointer to the api context to initialize
Parameter:  waitForCTS flag for CTS checking prior to sending a Si2170 API Command
Parameter:  waitForResponse flag for CTS checking and Response readback after sending Si2170 API Command
Parameter:  number of bytes to transmit
Parameter:  Databuffer containing the bytes to transfer in an unsigned char array.
Returns:   0 if no error, else a nonzero int representing an error
 ***********************************************************************************************************************/
static unsigned char si2176_api_patch(struct i2c_client *si2176, int inbbytes, unsigned char *pucdatabuffer, si2176_common_reply_struct *common_reply)
{
        unsigned char res = 0;
        unsigned char error_code = 0;
        unsigned char rspbytebuffer[1];

        res = si2176_pollforcts(si2176);
        if (res != NO_SI2176_ERROR)
        {
                pr_info("%s: poll cts error:%d!!!!\n", __func__, res);
                return res;
        }

        res = si2176_writecommandbytes(si2176, inbbytes, pucdatabuffer);
        if (res!=inbbytes)
        {
                pr_info("%s: poll cts error:%d!!!!\n", __func__, ERROR_SI2176_SENDING_COMMAND);
                return ERROR_SI2176_SENDING_COMMAND;
        }

        error_code = si2176_pollforresponse(si2176, 1, 1, rspbytebuffer, common_reply);
        if (error_code)
                pr_info("%s: poll response error:%d!!!!\n", __func__, error_code);

        return error_code;
}


/* _commands_insertion_start */
#ifdef SI2176_AGC_OVERRIDE_CMD
/*---------------------------------------------------*/
/* SI2176_AGC_OVERRIDE COMMAND                     */
/*---------------------------------------------------*/
static unsigned char si2176_agc_override(struct i2c_client *si2176,
                unsigned char   force_max_gain,
                unsigned char   force_top_gain,
                si2176_cmdreplyobj_t *rsp)
{
        unsigned char error_code = 0;
        unsigned char cmdbytebuffer[2];
        unsigned char rspbytebuffer[1];

#ifdef DEBUG_RANGE_CHECK
        if ( (force_max_gain > SI2176_AGC_OVERRIDE_CMD_FORCE_MAX_GAIN_MAX)
                        || (force_top_gain > SI2176_AGC_OVERRIDE_CMD_FORCE_TOP_GAIN_MAX) )
                return ERROR_SI2176_PARAMETER_OUT_OF_RANGE;
#endif /* DEBUG_RANGE_CHECK */

        error_code = si2176_pollforcts(si2176);
        if (error_code)
        {
                pr_info("%s: poll cts error:%d!!!!\n", __func__, error_code);
                goto exit;
        }

        cmdbytebuffer[0] = SI2176_AGC_OVERRIDE_CMD;
        cmdbytebuffer[1] = (unsigned char) ( ( force_max_gain & SI2176_AGC_OVERRIDE_CMD_FORCE_MAX_GAIN_MASK ) << SI2176_AGC_OVERRIDE_CMD_FORCE_MAX_GAIN_LSB|
                        ( force_top_gain & SI2176_AGC_OVERRIDE_CMD_FORCE_TOP_GAIN_MASK ) << SI2176_AGC_OVERRIDE_CMD_FORCE_TOP_GAIN_LSB);

        if (si2176_writecommandbytes(si2176, 2, cmdbytebuffer) != 2)
        {
                error_code = ERROR_SI2176_SENDING_COMMAND;
                pr_info("%s: write command byte error:%d!!!!\n", __func__, error_code);
        }

        if (!error_code)
        {
                error_code = si2176_pollforresponse(si2176, 1, 1, rspbytebuffer, &reply);
                if (error_code)
                        pr_info("%s: poll response error:%d!!!!\n", __func__, error_code);
                rsp->agc_override.status = &reply;
        }
exit:
        return error_code;
}
#endif /* SI2176_AGC_OVERRIDE_CMD */
#ifdef SI2176_ATV_CW_TEST_CMD
/*---------------------------------------------------*/
/* SI2176_ATV_CW_TEST COMMAND                      */
/*---------------------------------------------------*/
static unsigned char si2176_atv_cw_test(struct i2c_client *si2176,
                unsigned char   pc_lock,
                si2176_cmdreplyobj_t *rsp)
{
        unsigned char error_code = 0;
        unsigned char cmdbytebuffer[2];
        unsigned char rspbytebuffer[1];

#ifdef DEBUG_RANGE_CHECK
        if ( (pc_lock > SI2176_ATV_CW_TEST_CMD_PC_LOCK_MAX) )
                return ERROR_SI2176_PARAMETER_OUT_OF_RANGE;
#endif /* DEBUG_RANGE_CHECK */

        error_code = si2176_pollforcts(si2176);
        if (error_code) goto exit;

        cmdbytebuffer[0] = SI2176_ATV_CW_TEST_CMD;
        cmdbytebuffer[1] = (unsigned char) ( ( pc_lock & SI2176_ATV_CW_TEST_CMD_PC_LOCK_MASK ) << SI2176_ATV_CW_TEST_CMD_PC_LOCK_LSB);

        if (si2176_writecommandbytes(si2176, 2, cmdbytebuffer) != 2) error_code = ERROR_SI2176_SENDING_COMMAND;

        if (!error_code)
        {
                error_code = si2176_pollforresponse(si2176, 1, 1, rspbytebuffer, &reply);
                if (error_code)
                        pr_info("%s: poll response error:%d!!!!\n", __func__, error_code);
                rsp->atv_cw_test.status = &reply;
        }
exit:
        return error_code;
}
#endif /* SI2176_ATV_CW_TEST_CMD */
#ifdef SI2176_ATV_RESTART_CMD
/*---------------------------------------------------*/
/* SI2176_ATV_RESTART COMMAND                      */
/*---------------------------------------------------*/
unsigned char si2176_atv_restart(struct i2c_client *si2176,
                unsigned char   mode,
                si2176_cmdreplyobj_t *rsp)
{
        unsigned char error_code = 0;
        unsigned char cmdbytebuffer[2];
        unsigned char rspbytebuffer[1];

#ifdef DEBUG_RANGE_CHECK
        if ( (mode > SI2176_ATV_RESTART_CMD_MODE_MAX) )
                return ERROR_SI2176_PARAMETER_OUT_OF_RANGE;
#endif /* DEBUG_RANGE_CHECK */

        error_code = si2176_pollforcts(si2176);
        if (error_code)
        {
                pr_info("%s: poll cts error:%d!!!!\n", __func__, error_code);
                goto exit;
        }

        cmdbytebuffer[0] = SI2176_ATV_RESTART_CMD;
        cmdbytebuffer[1] = (unsigned char) ( ( mode & SI2176_ATV_RESTART_CMD_MODE_MASK ) << SI2176_ATV_RESTART_CMD_MODE_LSB);

        if (si2176_writecommandbytes(si2176, 2, cmdbytebuffer) != 2)
        {
                error_code = ERROR_SI2176_SENDING_COMMAND;
                pr_info("%s: write command byte error:%d!!!!\n", __func__, error_code);
        }

        if (!error_code)
        {
                error_code = si2176_pollforresponse(si2176, 1, 1, rspbytebuffer, &reply);
                if (error_code)
                        pr_info("%s: poll response error:%d!!!!\n", __func__, error_code);
                rsp->atv_restart.status = &reply;
        }
exit:
        return error_code;
}
#endif /* SI2176_ATV_RESTART_CMD */
#ifdef SI2176_DTV_RESTART_CMD
/*---------------------------------------------------*/
/* SI2176_DTV_RESTART COMMAND                      */
/*---------------------------------------------------*/
unsigned char si2176_dtv_restart(struct i2c_client *si2176, si2176_cmdreplyobj_t *rsp)
{
        unsigned char error_code = 0;
        unsigned char cmdbytebuffer[1];
        unsigned char rspbytebuffer[1];


        error_code = si2176_pollforcts(si2176);
        if (error_code)
        {
                pr_info("%s: poll cts error:%d!!!!\n", __func__, error_code);
                goto exit;
        }

        cmdbytebuffer[0] = SI2176_DTV_RESTART_CMD;
        if (si2176_writecommandbytes(si2176, 1, cmdbytebuffer) != 1)
        {
                error_code = ERROR_SI2176_SENDING_COMMAND;
                pr_info("%s: write command byte error:%d!!!!\n", __func__, error_code);
        }

        if (!error_code)
        {
                error_code = si2176_pollforresponse(si2176, 1, 1, rspbytebuffer, &reply);
                if (error_code)
                        pr_info("%s: poll response error:%d!!!!\n", __func__, error_code);
                rsp->atv_restart.status = &reply;
        }
exit:
        return error_code;
}
#endif /* SI2176_ATV_RESTART_CMD */

#ifdef SI2176_ATV_STATUS_CMD
/*---------------------------------------------------*/
/* SI2176_ATV_STATUS COMMAND                       */
/*---------------------------------------------------*/
unsigned char si2176_atv_status(struct i2c_client *si2176,
                unsigned char intack,
                si2176_cmdreplyobj_t *rsp)
{
        unsigned char error_code = 0;
        unsigned char cmdbytebuffer[2];
        unsigned char rspbytebuffer[12];

#ifdef DEBUG_RANGE_CHECK
        if ( (intack > SI2176_ATV_STATUS_CMD_INTACK_MAX) )
                return ERROR_SI2176_PARAMETER_OUT_OF_RANGE;
#endif /* DEBUG_RANGE_CHECK */

        error_code = si2176_pollforcts(si2176);
        if (error_code)
        {
                pr_info("%s: poll cts error:%d!!!!\n", __func__, error_code);
                goto exit;
        }

        cmdbytebuffer[0] = SI2176_ATV_STATUS_CMD;
        cmdbytebuffer[1] = (unsigned char) ( ( intack & SI2176_ATV_STATUS_CMD_INTACK_MASK ) << SI2176_ATV_STATUS_CMD_INTACK_LSB);

        if (si2176_writecommandbytes(si2176, 2, cmdbytebuffer) != 2)
        {
                error_code = ERROR_SI2176_SENDING_COMMAND;
                pr_info("%s: write command byte error:%d!!!!\n", __func__, error_code);
        }

        if (!error_code)
        {
                error_code = si2176_pollforresponse(si2176, 1, 12, rspbytebuffer, &reply);
                if (error_code)
                        pr_info("%s: poll response error:%d!!!!\n", __func__, error_code);
                rsp->atv_status.status = &reply;
                if (!error_code)
                {
                        rsp->atv_status.chlint           =   (( ( (rspbytebuffer[1]  )) >> SI2176_ATV_STATUS_RESPONSE_CHLINT_LSB           ) & SI2176_ATV_STATUS_RESPONSE_CHLINT_MASK           );
                        rsp->atv_status.pclint           =   (( ( (rspbytebuffer[1]  )) >> SI2176_ATV_STATUS_RESPONSE_PCLINT_LSB           ) & SI2176_ATV_STATUS_RESPONSE_PCLINT_MASK           );
                        rsp->atv_status.dlint             =   (( ( (rspbytebuffer[1]  )) >> SI2176_ATV_STATUS_RESPONSE_DLINT_LSB            ) & SI2176_ATV_STATUS_RESPONSE_DLINT_MASK            );
                        rsp->atv_status.snrlint          =   (( ( (rspbytebuffer[1]  )) >> SI2176_ATV_STATUS_RESPONSE_SNRLINT_LSB          ) & SI2176_ATV_STATUS_RESPONSE_SNRLINT_MASK          );
                        rsp->atv_status.snrhint         =   (( ( (rspbytebuffer[1]  )) >> SI2176_ATV_STATUS_RESPONSE_SNRHINT_LSB          ) & SI2176_ATV_STATUS_RESPONSE_SNRHINT_MASK          );
                        rsp->atv_status.chl                =   (( ( (rspbytebuffer[2]  )) >> SI2176_ATV_STATUS_RESPONSE_CHL_LSB              ) & SI2176_ATV_STATUS_RESPONSE_CHL_MASK              );
                        rsp->atv_status.pcl               =   (( ( (rspbytebuffer[2]  )) >> SI2176_ATV_STATUS_RESPONSE_PCL_LSB              ) & SI2176_ATV_STATUS_RESPONSE_PCL_MASK              );
                        rsp->atv_status.dl                 =   (( ( (rspbytebuffer[2]  )) >> SI2176_ATV_STATUS_RESPONSE_DL_LSB               ) & SI2176_ATV_STATUS_RESPONSE_DL_MASK               );
                        rsp->atv_status.snrl              =   (( ( (rspbytebuffer[2]  )) >> SI2176_ATV_STATUS_RESPONSE_SNRL_LSB             ) & SI2176_ATV_STATUS_RESPONSE_SNRL_MASK             );
                        rsp->atv_status.snrh             =   (( ( (rspbytebuffer[2]  )) >> SI2176_ATV_STATUS_RESPONSE_SNRH_LSB             ) & SI2176_ATV_STATUS_RESPONSE_SNRH_MASK             );
                        rsp->atv_status.video_snr     =   (( ( (rspbytebuffer[3]  )) >> SI2176_ATV_STATUS_RESPONSE_VIDEO_SNR_LSB        ) & SI2176_ATV_STATUS_RESPONSE_VIDEO_SNR_MASK        );
                        rsp->atv_status.afc_freq       = (((( ( (rspbytebuffer[4]  ) | (rspbytebuffer[5]  << 8 )) >> SI2176_ATV_STATUS_RESPONSE_AFC_FREQ_LSB         ) & SI2176_ATV_STATUS_RESPONSE_AFC_FREQ_MASK) <<SI2176_ATV_STATUS_RESPONSE_AFC_FREQ_SHIFT ) >>SI2176_ATV_STATUS_RESPONSE_AFC_FREQ_SHIFT         );
                        rsp->atv_status.video_sc_spacing = (((( ( (rspbytebuffer[6]  ) | (rspbytebuffer[7]  << 8 )) >> SI2176_ATV_STATUS_RESPONSE_VIDEO_SC_SPACING_LSB ) & SI2176_ATV_STATUS_RESPONSE_VIDEO_SC_SPACING_MASK) <<SI2176_ATV_STATUS_RESPONSE_VIDEO_SC_SPACING_SHIFT ) >>SI2176_ATV_STATUS_RESPONSE_VIDEO_SC_SPACING_SHIFT );
                        rsp->atv_status.video_sys    =   (( ( (rspbytebuffer[8]  )) >> SI2176_ATV_STATUS_RESPONSE_VIDEO_SYS_LSB        ) & SI2176_ATV_STATUS_RESPONSE_VIDEO_SYS_MASK        );
                        rsp->atv_status.color            =   (( ( (rspbytebuffer[8]  )) >> SI2176_ATV_STATUS_RESPONSE_COLOR_LSB            ) & SI2176_ATV_STATUS_RESPONSE_COLOR_MASK            );
                        rsp->atv_status.trans            =   (( ( (rspbytebuffer[8]  )) >> SI2176_ATV_STATUS_RESPONSE_TRANS_LSB            ) & SI2176_ATV_STATUS_RESPONSE_TRANS_MASK            );
                        rsp->atv_status.audio_sys    =   (( ( (rspbytebuffer[9]  )) >> SI2176_ATV_STATUS_RESPONSE_AUDIO_SYS_LSB        ) & SI2176_ATV_STATUS_RESPONSE_AUDIO_SYS_MASK        );
                        rsp->atv_status.audio_demod_mode =   (( ( (rspbytebuffer[9]  )) >> SI2176_ATV_STATUS_RESPONSE_AUDIO_DEMOD_MODE_LSB ) & SI2176_ATV_STATUS_RESPONSE_AUDIO_DEMOD_MODE_MASK );
                        rsp->atv_status.audio_chan_bw    =   (( ( (rspbytebuffer[10] )) >> SI2176_ATV_STATUS_RESPONSE_AUDIO_CHAN_BW_LSB    ) & SI2176_ATV_STATUS_RESPONSE_AUDIO_CHAN_BW_MASK    );
                        rsp->atv_status.sound_level = rspbytebuffer[11];
                }
        }
exit:
        return error_code;
}
#endif /* SI2176_ATV_STATUS_CMD */
#ifdef SI2176_CONFIG_PINS_CMD
/*---------------------------------------------------*/
/* SI2176_CONFIG_PINS COMMAND                      */
/*---------------------------------------------------*/
static unsigned char si2176_config_pins(struct i2c_client *si2176,
                unsigned char   gpio1_mode,
                unsigned char   gpio1_read,
                unsigned char   gpio2_mode,
                unsigned char   gpio2_read,
                unsigned char   gpio3_mode,
                unsigned char   gpio3_read,
                unsigned char   bclk1_mode,
                unsigned char   bclk1_read,
                unsigned char   xout_mode,
                si2176_cmdreplyobj_t *rsp)
{
        unsigned char error_code = 0;
        unsigned char cmdbytebuffer[6];
        unsigned char rspbytebuffer[6];

#ifdef DEBUG_RANGE_CHECK
        if ( (gpio1_mode > SI2176_CONFIG_PINS_CMD_GPIO1_MODE_MAX)
                        || (gpio1_read > SI2176_CONFIG_PINS_CMD_GPIO1_READ_MAX)
                        || (gpio2_mode > SI2176_CONFIG_PINS_CMD_GPIO2_MODE_MAX)
                        || (gpio2_read > SI2176_CONFIG_PINS_CMD_GPIO2_READ_MAX)
                        || (gpio3_mode > SI2176_CONFIG_PINS_CMD_GPIO3_MODE_MAX)
                        || (gpio3_read > SI2176_CONFIG_PINS_CMD_GPIO3_READ_MAX)
                        || (bclk1_mode > SI2176_CONFIG_PINS_CMD_BCLK1_MODE_MAX)
                        || (bclk1_read > SI2176_CONFIG_PINS_CMD_BCLK1_READ_MAX)
                        || (xout_mode  > SI2176_CONFIG_PINS_CMD_XOUT_MODE_MAX ) )
                return ERROR_SI2176_PARAMETER_OUT_OF_RANGE;
#endif /* DEBUG_RANGE_CHECK */

        error_code = si2176_pollforcts(si2176);
        if (error_code)
        {
                pr_info("%s: poll cts error:%d!!!!\n", __func__, error_code);
                goto exit;
        }

        cmdbytebuffer[0] = SI2176_CONFIG_PINS_CMD;
        cmdbytebuffer[1] = (unsigned char) ( ( gpio1_mode & SI2176_CONFIG_PINS_CMD_GPIO1_MODE_MASK ) << SI2176_CONFIG_PINS_CMD_GPIO1_MODE_LSB|
                        ( gpio1_read & SI2176_CONFIG_PINS_CMD_GPIO1_READ_MASK ) << SI2176_CONFIG_PINS_CMD_GPIO1_READ_LSB);
        cmdbytebuffer[2] = (unsigned char) ( ( gpio2_mode & SI2176_CONFIG_PINS_CMD_GPIO2_MODE_MASK ) << SI2176_CONFIG_PINS_CMD_GPIO2_MODE_LSB|
                        ( gpio2_read & SI2176_CONFIG_PINS_CMD_GPIO2_READ_MASK ) << SI2176_CONFIG_PINS_CMD_GPIO2_READ_LSB);
        cmdbytebuffer[3] = (unsigned char) ( ( gpio3_mode & SI2176_CONFIG_PINS_CMD_GPIO3_MODE_MASK ) << SI2176_CONFIG_PINS_CMD_GPIO3_MODE_LSB|
                        ( gpio3_read & SI2176_CONFIG_PINS_CMD_GPIO3_READ_MASK ) << SI2176_CONFIG_PINS_CMD_GPIO3_READ_LSB);
        cmdbytebuffer[4] = (unsigned char) ( ( bclk1_mode & SI2176_CONFIG_PINS_CMD_BCLK1_MODE_MASK ) << SI2176_CONFIG_PINS_CMD_BCLK1_MODE_LSB|
                        ( bclk1_read & SI2176_CONFIG_PINS_CMD_BCLK1_READ_MASK ) << SI2176_CONFIG_PINS_CMD_BCLK1_READ_LSB);
        cmdbytebuffer[5] = (unsigned char) ( ( xout_mode  & SI2176_CONFIG_PINS_CMD_XOUT_MODE_MASK  ) << SI2176_CONFIG_PINS_CMD_XOUT_MODE_LSB );

        if (si2176_writecommandbytes(si2176, 6, cmdbytebuffer) != 6)
        {
                error_code = ERROR_SI2176_SENDING_COMMAND;
                pr_info("%s: write command byte error:%d!!!!\n", __func__, error_code);
        }

        if (!error_code)
        {
                error_code = si2176_pollforresponse(si2176, 1, 6, rspbytebuffer, &reply);
                if (error_code)
                        pr_info("%s: poll response error:%d!!!!\n", __func__, error_code);
                rsp->config_pins.status = &reply;
                if (!error_code)
                {
                        rsp->config_pins.gpio1_mode  =   (( ( (rspbytebuffer[1]  )) >> SI2176_CONFIG_PINS_RESPONSE_GPIO1_MODE_LSB  ) & SI2176_CONFIG_PINS_RESPONSE_GPIO1_MODE_MASK  );
                        rsp->config_pins.gpio1_state =   (( ( (rspbytebuffer[1]  )) >> SI2176_CONFIG_PINS_RESPONSE_GPIO1_STATE_LSB ) & SI2176_CONFIG_PINS_RESPONSE_GPIO1_STATE_MASK );
                        rsp->config_pins.gpio2_mode  =   (( ( (rspbytebuffer[2]  )) >> SI2176_CONFIG_PINS_RESPONSE_GPIO2_MODE_LSB  ) & SI2176_CONFIG_PINS_RESPONSE_GPIO2_MODE_MASK  );
                        rsp->config_pins.gpio2_state =   (( ( (rspbytebuffer[2]  )) >> SI2176_CONFIG_PINS_RESPONSE_GPIO2_STATE_LSB ) & SI2176_CONFIG_PINS_RESPONSE_GPIO2_STATE_MASK );
                        rsp->config_pins.gpio3_mode  =   (( ( (rspbytebuffer[3]  )) >> SI2176_CONFIG_PINS_RESPONSE_GPIO3_MODE_LSB  ) & SI2176_CONFIG_PINS_RESPONSE_GPIO3_MODE_MASK  );
                        rsp->config_pins.gpio3_state =   (( ( (rspbytebuffer[3]  )) >> SI2176_CONFIG_PINS_RESPONSE_GPIO3_STATE_LSB ) & SI2176_CONFIG_PINS_RESPONSE_GPIO3_STATE_MASK );
                        rsp->config_pins.bclk1_mode  =   (( ( (rspbytebuffer[4]  )) >> SI2176_CONFIG_PINS_RESPONSE_BCLK1_MODE_LSB  ) & SI2176_CONFIG_PINS_RESPONSE_BCLK1_MODE_MASK  );
                        rsp->config_pins.bclk1_state =   (( ( (rspbytebuffer[4]  )) >> SI2176_CONFIG_PINS_RESPONSE_BCLK1_STATE_LSB ) & SI2176_CONFIG_PINS_RESPONSE_BCLK1_STATE_MASK );
                        rsp->config_pins.xout_mode   =   (( ( (rspbytebuffer[5]  )) >> SI2176_CONFIG_PINS_RESPONSE_XOUT_MODE_LSB   ) & SI2176_CONFIG_PINS_RESPONSE_XOUT_MODE_MASK   );
                }
        }
exit:
        return error_code;
}
#endif /* SI2176_CONFIG_PINS_CMD */
#ifdef SI2176_EXIT_BOOTLOADER_CMD
/*---------------------------------------------------*/
/* SI2176_EXIT_BOOTLOADER COMMAND                  */
/*---------------------------------------------------*/
static unsigned char si2176_exit_bootloader(struct i2c_client *si2176,
                unsigned char   func,
                unsigned char   ctsien,
                si2176_cmdreplyobj_t *rsp)
{
        unsigned char error_code = 0;
        unsigned char cmdbytebuffer[2];
        unsigned char rspbytebuffer[1];

#ifdef DEBUG_RANGE_CHECK
        if ( (func   > SI2176_EXIT_BOOTLOADER_CMD_FUNC_MAX  )
                        || (ctsien > SI2176_EXIT_BOOTLOADER_CMD_CTSIEN_MAX) )
                return ERROR_SI2176_PARAMETER_OUT_OF_RANGE;
#endif /* DEBUG_RANGE_CHECK */

        error_code = si2176_pollforcts(si2176);
        if (error_code) goto exit;

        cmdbytebuffer[0] = SI2176_EXIT_BOOTLOADER_CMD;
        cmdbytebuffer[1] = (unsigned char) ( ( func   & SI2176_EXIT_BOOTLOADER_CMD_FUNC_MASK   ) << SI2176_EXIT_BOOTLOADER_CMD_FUNC_LSB  |
                        ( ctsien & SI2176_EXIT_BOOTLOADER_CMD_CTSIEN_MASK ) << SI2176_EXIT_BOOTLOADER_CMD_CTSIEN_LSB);

        if (si2176_writecommandbytes(si2176, 2, cmdbytebuffer) != 2) error_code = ERROR_SI2176_SENDING_COMMAND;

        if (!error_code)
        {
                error_code = si2176_pollforresponse(si2176, 1, 1, rspbytebuffer, &reply);
                if (error_code)
                        pr_info("%s: poll response error:%d!!!!\n", __func__, error_code);
                rsp->exit_bootloader.status = &reply;
        }
exit:
        return error_code;
}
#endif /* SI2176_EXIT_BOOTLOADER_CMD */
#ifdef SI2176_FINE_TUNE_CMD
/*---------------------------------------------------*/
/* SI2176_FINE_TUNE COMMAND                        */
/*---------------------------------------------------*/
unsigned char si2176_fine_tune(struct i2c_client *si2176,
                unsigned char   reserved,
                int   offset_500hz,
                si2176_cmdreplyobj_t *rsp)
{
        unsigned char error_code = 0;
        unsigned char cmdbytebuffer[4];
        unsigned char rspbytebuffer[1];

#ifdef DEBUG_RANGE_CHECK
        if ( (reserved     > SI2176_FINE_TUNE_CMD_RESERVED_MAX    )
                        || (offset_500hz > SI2176_FINE_TUNE_CMD_OFFSET_500HZ_MAX)  || (offset_500hz < SI2176_FINE_TUNE_CMD_OFFSET_500HZ_MIN) )
                return ERROR_SI2176_PARAMETER_OUT_OF_RANGE;
#endif /* DEBUG_RANGE_CHECK */

        error_code = si2176_pollforcts(si2176);
        if (error_code)
        {
                pr_info("%s: poll cts error:%d!!!!\n", __func__, error_code);
                goto exit;
        }

        cmdbytebuffer[0] = SI2176_FINE_TUNE_CMD;
        cmdbytebuffer[1] = (unsigned char) ( ( reserved     & SI2176_FINE_TUNE_CMD_RESERVED_MASK     ) << SI2176_FINE_TUNE_CMD_RESERVED_LSB    );
        cmdbytebuffer[2] = (unsigned char) ( ( offset_500hz & SI2176_FINE_TUNE_CMD_OFFSET_500HZ_MASK ) << SI2176_FINE_TUNE_CMD_OFFSET_500HZ_LSB);
        cmdbytebuffer[3] = (unsigned char) ((( offset_500hz & SI2176_FINE_TUNE_CMD_OFFSET_500HZ_MASK ) << SI2176_FINE_TUNE_CMD_OFFSET_500HZ_LSB)>>8);

        if (si2176_writecommandbytes(si2176, 4, cmdbytebuffer) != 4)
        {
                error_code = ERROR_SI2176_SENDING_COMMAND;
                pr_info("%s: write command byte error:%d!!!!\n", __func__, error_code);
        }

        if (!error_code)
        {
                error_code = si2176_pollforresponse(si2176, 1, 1, rspbytebuffer, &reply);
                if (error_code)
                        pr_info("%s: poll response error:%d!!!!\n", __func__, error_code);
                rsp->fine_tune.status = &reply;
        }
exit:
        return error_code;
}
#endif /* SI2176_FINE_TUNE_CMD */
#ifdef SI2176_GET_PROPERTY_CMD
/*---------------------------------------------------*/
/* SI2176_GET_PROPERTY COMMAND                     */
/*---------------------------------------------------*/
unsigned char si2176_get_property(struct i2c_client *si2176,
                unsigned char   reserved,
                unsigned int    prop,
                si2176_cmdreplyobj_t *rsp)
{
        unsigned char error_code = 0;
        unsigned char cmdbytebuffer[4];
        unsigned char rspbytebuffer[4];

#ifdef DEBUG_RANGE_CHECK
        if ( (reserved > SI2176_GET_PROPERTY_CMD_RESERVED_MAX) )
                return ERROR_SI2176_PARAMETER_OUT_OF_RANGE;
#endif /* DEBUG_RANGE_CHECK */

        error_code = si2176_pollforcts(si2176);
        if (error_code)
        {
                pr_info("%s: poll cts error:%d!!!!\n", __func__, error_code);
                goto exit;
        }

        cmdbytebuffer[0] = SI2176_GET_PROPERTY_CMD;
        cmdbytebuffer[1] = (unsigned char) ( ( reserved & SI2176_GET_PROPERTY_CMD_RESERVED_MASK ) << SI2176_GET_PROPERTY_CMD_RESERVED_LSB);
        cmdbytebuffer[2] = (unsigned char) ( ( prop     & SI2176_GET_PROPERTY_CMD_PROP_MASK     ) << SI2176_GET_PROPERTY_CMD_PROP_LSB    );
        cmdbytebuffer[3] = (unsigned char) ((( prop     & SI2176_GET_PROPERTY_CMD_PROP_MASK     ) << SI2176_GET_PROPERTY_CMD_PROP_LSB    )>>8);

        if (si2176_writecommandbytes(si2176, 4, cmdbytebuffer) != 4)
        {
                error_code = ERROR_SI2176_SENDING_COMMAND;
                pr_info("%s: write command byte error:%d!!!!\n", __func__, error_code);
        }

        if (!error_code)
        {
                error_code = si2176_pollforresponse(si2176, 1, 4, rspbytebuffer, &reply);
                if (error_code)
                        pr_info("%s: poll response error:%d!!!!\n", __func__, error_code);
                rsp->get_property.status = &reply;
                if (!error_code)
                {
                        rsp->get_property.reserved =   (( ( (rspbytebuffer[1]  )) >> SI2176_GET_PROPERTY_RESPONSE_RESERVED_LSB ) & SI2176_GET_PROPERTY_RESPONSE_RESERVED_MASK );
                        rsp->get_property.data     =   (( ( (rspbytebuffer[2]  ) | (rspbytebuffer[3]  << 8 )) >> SI2176_GET_PROPERTY_RESPONSE_DATA_LSB     ) & SI2176_GET_PROPERTY_RESPONSE_DATA_MASK     );
                }
        }
exit:
        return error_code;
}
#endif /* SI2176_GET_PROPERTY_CMD */
#ifdef SI2176_GET_REV_CMD
/*---------------------------------------------------*/
/* SI2176_GET_REV COMMAND                          */
/*---------------------------------------------------*/
static unsigned char si2176_get_rev(struct i2c_client *si2176,
                si2176_cmdreplyobj_t *rsp)
{
        unsigned char error_code = 0;
        unsigned char cmdbytebuffer[1];
        unsigned char rspbytebuffer[10];

        error_code = si2176_pollforcts(si2176);
        if (error_code) goto exit;

        cmdbytebuffer[0] = SI2176_GET_REV_CMD;

        if (si2176_writecommandbytes(si2176, 1, cmdbytebuffer) != 1) error_code = ERROR_SI2176_SENDING_COMMAND;

        if (!error_code)
        {
                error_code = si2176_pollforresponse(si2176, 1, 10, rspbytebuffer, &reply);
                if (error_code)
                        pr_info("%s: poll response error:%d!!!!\n", __func__, error_code);
                rsp->get_rev.status = &reply;
                if (!error_code)
                {
                        rsp->get_rev.pn       =   (( ( (rspbytebuffer[1]  )) >> SI2176_GET_REV_RESPONSE_PN_LSB       ) & SI2176_GET_REV_RESPONSE_PN_MASK       );
                        rsp->get_rev.fwmajor  =   (( ( (rspbytebuffer[2]  )) >> SI2176_GET_REV_RESPONSE_FWMAJOR_LSB  ) & SI2176_GET_REV_RESPONSE_FWMAJOR_MASK  );
                        rsp->get_rev.fwminor  =   (( ( (rspbytebuffer[3]  )) >> SI2176_GET_REV_RESPONSE_FWMINOR_LSB  ) & SI2176_GET_REV_RESPONSE_FWMINOR_MASK  );
                        rsp->get_rev.patch    =   (( ( (rspbytebuffer[4]  ) | (rspbytebuffer[5]  << 8 )) >> SI2176_GET_REV_RESPONSE_PATCH_LSB    ) & SI2176_GET_REV_RESPONSE_PATCH_MASK    );
                        rsp->get_rev.cmpmajor =   (( ( (rspbytebuffer[6]  )) >> SI2176_GET_REV_RESPONSE_CMPMAJOR_LSB ) & SI2176_GET_REV_RESPONSE_CMPMAJOR_MASK );
                        rsp->get_rev.cmpminor =   (( ( (rspbytebuffer[7]  )) >> SI2176_GET_REV_RESPONSE_CMPMINOR_LSB ) & SI2176_GET_REV_RESPONSE_CMPMINOR_MASK );
                        rsp->get_rev.cmpbuild =   (( ( (rspbytebuffer[8]  )) >> SI2176_GET_REV_RESPONSE_CMPBUILD_LSB ) & SI2176_GET_REV_RESPONSE_CMPBUILD_MASK );
                        rsp->get_rev.chiprev  =   (( ( (rspbytebuffer[9]  )) >> SI2176_GET_REV_RESPONSE_CHIPREV_LSB  ) & SI2176_GET_REV_RESPONSE_CHIPREV_MASK  );
                }
        }
exit:
        return error_code;
}
#endif /* SI2176_GET_REV_CMD */
#ifdef SI2176_PART_INFO_CMD
/*---------------------------------------------------*/
/* SI2176_PART_INFO COMMAND                        */
/*---------------------------------------------------*/
static unsigned char si2176_part_info(struct i2c_client *si2176,
                si2176_cmdreplyobj_t *rsp)
{
        unsigned char error_code = 0;
        unsigned char cmdbytebuffer[1];
        unsigned char rspbytebuffer[13];

        error_code = si2176_pollforcts(si2176);
        if (error_code) goto exit;

        cmdbytebuffer[0] = SI2176_PART_INFO_CMD;

        if (si2176_writecommandbytes(si2176, 1, cmdbytebuffer) != 1) error_code = ERROR_SI2176_SENDING_COMMAND;

        if (!error_code)
        {
                error_code = si2176_pollforresponse(si2176, 1, 13, rspbytebuffer, &reply);
                if (error_code)
                        pr_info("%s: poll response error:%d!!!!\n", __func__, error_code);
                rsp->part_info.status = &reply;
                if (!error_code)
                {
                        rsp->part_info.chiprev  =   (( ( (rspbytebuffer[1]  )) >> SI2176_PART_INFO_RESPONSE_CHIPREV_LSB  ) & SI2176_PART_INFO_RESPONSE_CHIPREV_MASK  );
                        rsp->part_info.part     =   (( ( (rspbytebuffer[2]  )) >> SI2176_PART_INFO_RESPONSE_PART_LSB     ) & SI2176_PART_INFO_RESPONSE_PART_MASK     );
                        rsp->part_info.pmajor   =   (( ( (rspbytebuffer[3]  )) >> SI2176_PART_INFO_RESPONSE_PMAJOR_LSB   ) & SI2176_PART_INFO_RESPONSE_PMAJOR_MASK   );
                        rsp->part_info.pminor   =   (( ( (rspbytebuffer[4]  )) >> SI2176_PART_INFO_RESPONSE_PMINOR_LSB   ) & SI2176_PART_INFO_RESPONSE_PMINOR_MASK   );
                        rsp->part_info.pbuild   =   (( ( (rspbytebuffer[5]  )) >> SI2176_PART_INFO_RESPONSE_PBUILD_LSB   ) & SI2176_PART_INFO_RESPONSE_PBUILD_MASK   );
                        rsp->part_info.reserved =   (( ( (rspbytebuffer[6]  ) | (rspbytebuffer[7]  << 8 )) >> SI2176_PART_INFO_RESPONSE_RESERVED_LSB ) & SI2176_PART_INFO_RESPONSE_RESERVED_MASK );
                        rsp->part_info.serial   =   (( ( (rspbytebuffer[8]  ) | (rspbytebuffer[9]  << 8 ) | (rspbytebuffer[10] << 16 ) | (rspbytebuffer[11] << 24 )) >> SI2176_PART_INFO_RESPONSE_SERIAL_LSB   ) & SI2176_PART_INFO_RESPONSE_SERIAL_MASK   );
                        rsp->part_info.romid    =   (( ( (rspbytebuffer[12] )) >> SI2176_PART_INFO_RESPONSE_ROMID_LSB    ) & SI2176_PART_INFO_RESPONSE_ROMID_MASK    );
                }
        }
exit:
        return error_code;
}
#endif /* SI2176_PART_INFO_CMD */
#ifdef SI2176_POWER_DOWN_CMD
/*---------------------------------------------------*/
/* SI2176_POWER_DOWN COMMAND                       */
/*---------------------------------------------------*/
unsigned char si2176_power_down(struct i2c_client *si2176,
                si2176_cmdreplyobj_t *rsp)
{
        unsigned char error_code = 0;
        unsigned char cmdbytebuffer[1];
        unsigned char rspbytebuffer[1];

        error_code = si2176_pollforcts(si2176);
        if (error_code)
        {
                pr_info("%s: poll cts error:%d!!!!\n", __func__, error_code);
                goto exit;
        }

        cmdbytebuffer[0] = SI2176_POWER_DOWN_CMD;

        if (si2176_writecommandbytes(si2176, 1, cmdbytebuffer) != 1)
        {
                error_code = ERROR_SI2176_SENDING_COMMAND;
                pr_info("%s: write command byte error:%d!!!!\n", __func__, error_code);
        }

        if (!error_code)
        {
                error_code = si2176_pollforresponse(si2176, 1, 1, rspbytebuffer, &reply);
                if (error_code)
                        pr_info("%s: poll response error:%d!!!!\n", __func__, error_code);
                rsp->power_down.status = &reply;
        }
exit:
        return error_code;
}
#endif /* SI2176_POWER_DOWN_CMD */
#ifdef SI2176_POWER_UP_CMD
/*---------------------------------------------------*/
/* SI2176_POWER_UP COMMAND                         */
/*---------------------------------------------------*/
static unsigned char si2176_power_up(struct i2c_client *si2176,
                unsigned char   subcode,
                unsigned char   reserved1,
                unsigned char   reserved2,
                unsigned char   reserved3,
                unsigned char   clock_mode,
                unsigned char   clock_freq,
                unsigned char   addr_mode,
                unsigned char   func,
                unsigned char   ctsien,
                unsigned char   wake_up,
                si2176_cmdreplyobj_t *rsp)
{
        unsigned char error_code = 0;
        unsigned char cmdbytebuffer[9];
        unsigned char rspbytebuffer[1];

#ifdef DEBUG_RANGE_CHECK
        if ( (subcode    > SI2176_POWER_UP_CMD_SUBCODE_MAX   )  || (subcode    < SI2176_POWER_UP_CMD_SUBCODE_MIN   )
                        || (reserved1  > SI2176_POWER_UP_CMD_RESERVED1_MAX )  || (reserved1  < SI2176_POWER_UP_CMD_RESERVED1_MIN )
                        || (reserved2  > SI2176_POWER_UP_CMD_RESERVED2_MAX )
                        || (reserved3  > SI2176_POWER_UP_CMD_RESERVED3_MAX )
                        || (clock_mode > SI2176_POWER_UP_CMD_CLOCK_MODE_MAX)  || (clock_mode < SI2176_POWER_UP_CMD_CLOCK_MODE_MIN)
                        || (clock_freq > SI2176_POWER_UP_CMD_CLOCK_FREQ_MAX)
                        || (addr_mode  > SI2176_POWER_UP_CMD_ADDR_MODE_MAX )
                        || (func       > SI2176_POWER_UP_CMD_FUNC_MAX      )
                        || (ctsien     > SI2176_POWER_UP_CMD_CTSIEN_MAX    )
                        || (wake_up    > SI2176_POWER_UP_CMD_WAKE_UP_MAX   )  || (wake_up    < SI2176_POWER_UP_CMD_WAKE_UP_MIN   ) )

        {
                if(SI2176_DEBUG)
                        pr_info("%s: DEBUG_RANGE_CHECK!!!!\n", __func__);
                return ERROR_SI2176_PARAMETER_OUT_OF_RANGE;
        }
#endif /* DEBUG_RANGE_CHECK */

        error_code = si2176_pollforcts(si2176);
        if (error_code)
        {
                pr_info("%s: poll cts error:%d!!!!\n", __func__, error_code);
                goto exit;
        }

        cmdbytebuffer[0] = SI2176_POWER_UP_CMD;
        cmdbytebuffer[1] = (unsigned char) ( ( subcode    & SI2176_POWER_UP_CMD_SUBCODE_MASK    ) << SI2176_POWER_UP_CMD_SUBCODE_LSB   );
        cmdbytebuffer[2] = (unsigned char) ( ( reserved1  & SI2176_POWER_UP_CMD_RESERVED1_MASK  ) << SI2176_POWER_UP_CMD_RESERVED1_LSB );
        cmdbytebuffer[3] = (unsigned char) ( ( reserved2  & SI2176_POWER_UP_CMD_RESERVED2_MASK  ) << SI2176_POWER_UP_CMD_RESERVED2_LSB );
        cmdbytebuffer[4] = (unsigned char) ( ( reserved3  & SI2176_POWER_UP_CMD_RESERVED3_MASK  ) << SI2176_POWER_UP_CMD_RESERVED3_LSB );
        cmdbytebuffer[5] = (unsigned char) ( ( clock_mode & SI2176_POWER_UP_CMD_CLOCK_MODE_MASK ) << SI2176_POWER_UP_CMD_CLOCK_MODE_LSB|
                        ( clock_freq & SI2176_POWER_UP_CMD_CLOCK_FREQ_MASK ) << SI2176_POWER_UP_CMD_CLOCK_FREQ_LSB);
        cmdbytebuffer[6] = (unsigned char) ( ( addr_mode  & SI2176_POWER_UP_CMD_ADDR_MODE_MASK  ) << SI2176_POWER_UP_CMD_ADDR_MODE_LSB );
        cmdbytebuffer[7] = (unsigned char) ( ( func       & SI2176_POWER_UP_CMD_FUNC_MASK       ) << SI2176_POWER_UP_CMD_FUNC_LSB      |
                        ( ctsien     & SI2176_POWER_UP_CMD_CTSIEN_MASK     ) << SI2176_POWER_UP_CMD_CTSIEN_LSB    );
        cmdbytebuffer[8] = (unsigned char) ( ( wake_up    & SI2176_POWER_UP_CMD_WAKE_UP_MASK    ) << SI2176_POWER_UP_CMD_WAKE_UP_LSB   );

        if (si2176_writecommandbytes(si2176, 9, cmdbytebuffer) != 9)
        {
                error_code = ERROR_SI2176_SENDING_COMMAND;
                if (error_code)
                        pr_info("%s: si2176_writecommandbytes!!!!\n", __func__);
        }

        if (!error_code)
        {
                error_code = si2176_pollforresponse(si2176, 1, 1, rspbytebuffer, &reply);
                if (error_code)
                        pr_info("%s: poll response error:%d!!!!\n", __func__, error_code);

                rsp->power_up.status = &reply;
        }
exit:
        return error_code;
}
#endif /* SI2176_POWER_UP_CMD */
#ifdef SI2176_SET_PROPERTY_CMD
/*---------------------------------------------------*/
/* SI2176_SET_PROPERTY COMMAND                     */
/*---------------------------------------------------*/
unsigned char si2176_set_property(struct i2c_client *si2176,
                unsigned char   reserved,
                unsigned int    prop,
                unsigned int    data,
                si2176_cmdreplyobj_t *rsp)
{
        unsigned char error_code = 0;
        unsigned char cmdbytebuffer[6];
        unsigned char rspbytebuffer[4];

        error_code = si2176_pollforcts(si2176);
        if (error_code)
        {
                pr_info("%s: poll cts error:%d!!!!\n", __func__, error_code);
                goto exit;
        }

        cmdbytebuffer[0] = SI2176_SET_PROPERTY_CMD;
        cmdbytebuffer[1] = (unsigned char) ( ( reserved & SI2176_SET_PROPERTY_CMD_RESERVED_MASK ) << SI2176_SET_PROPERTY_CMD_RESERVED_LSB);
        cmdbytebuffer[2] = (unsigned char) ( ( prop     & SI2176_SET_PROPERTY_CMD_PROP_MASK     ) << SI2176_SET_PROPERTY_CMD_PROP_LSB    );
        cmdbytebuffer[3] = (unsigned char) ((( prop     & SI2176_SET_PROPERTY_CMD_PROP_MASK     ) << SI2176_SET_PROPERTY_CMD_PROP_LSB    )>>8);
        cmdbytebuffer[4] = (unsigned char) ( ( data     & SI2176_SET_PROPERTY_CMD_DATA_MASK     ) << SI2176_SET_PROPERTY_CMD_DATA_LSB    );
        cmdbytebuffer[5] = (unsigned char) ((( data     & SI2176_SET_PROPERTY_CMD_DATA_MASK     ) << SI2176_SET_PROPERTY_CMD_DATA_LSB    )>>8);

        if (si2176_writecommandbytes(si2176, 6, cmdbytebuffer) != 6)
        {
                error_code = ERROR_SI2176_SENDING_COMMAND;
                pr_info("%s: write command byte error:%d!!!!\n", __func__, error_code);
        }

        if (!error_code)
        {
                error_code = si2176_pollforresponse(si2176, 1, 4, rspbytebuffer, &reply);
                if (error_code)
                        pr_info("%s: poll response error:%d!!!!\n", __func__, error_code);
                rsp->set_property.status = &reply;
                if (!error_code)
                {
                        rsp->set_property.reserved =   (( ( (rspbytebuffer[1]  )) >> SI2176_SET_PROPERTY_RESPONSE_RESERVED_LSB ) & SI2176_SET_PROPERTY_RESPONSE_RESERVED_MASK );
                        rsp->set_property.data     =   (( ( (rspbytebuffer[2]  ) | (rspbytebuffer[3]  << 8 )) >> SI2176_SET_PROPERTY_RESPONSE_DATA_LSB     ) & SI2176_SET_PROPERTY_RESPONSE_DATA_MASK     );
                }
        }
exit:
        return error_code;
}
#endif /* SI2176_SET_PROPERTY_CMD */
#ifdef SI2176_STANDBY_CMD
/*---------------------------------------------------*/
/* SI2176_STANDBY COMMAND                          */
/*---------------------------------------------------*/
static unsigned char si2176_standby(struct i2c_client *si2176,
                si2176_cmdreplyobj_t *rsp)
{
        unsigned char error_code = 0;
        unsigned char cmdbytebuffer[1];
        unsigned char rspbytebuffer[1];

        error_code = si2176_pollforcts(si2176);
        if (error_code)
        {
                pr_info("%s: poll cts error:%d!!!!\n", __func__, error_code);
                goto exit;
        }

        cmdbytebuffer[0] = SI2176_STANDBY_CMD;

        if (si2176_writecommandbytes(si2176, 1, cmdbytebuffer) != 1)
        {
                error_code = ERROR_SI2176_SENDING_COMMAND;
                pr_info("%s: write command byte error:%d!!!!\n", __func__, error_code);
        }

        if (!error_code)
        {
                error_code = si2176_pollforresponse(si2176, 1, 1, rspbytebuffer, &reply);
                if (error_code)
                        pr_info("%s: poll response error:%d!!!!\n", __func__, error_code);
                rsp->standby.status = &reply;
        }
exit:
        return error_code;
}
#endif /* SI2176_STANDBY_CMD */
#ifdef SI2176_TUNER_STATUS_CMD
/*---------------------------------------------------*/
/* SI2176_TUNER_STATUS COMMAND                     */
/*---------------------------------------------------*/
unsigned char si2176_tuner_status(struct i2c_client *si2176,
                unsigned char   intack,
                si2176_cmdreplyobj_t *rsp)
{
        unsigned char error_code = 0;
        unsigned char cmdbytebuffer[2];
        unsigned char rspbytebuffer[12];

#ifdef DEBUG_RANGE_CHECK
        if ( (intack > SI2176_TUNER_STATUS_CMD_INTACK_MAX) )
                return ERROR_SI2176_PARAMETER_OUT_OF_RANGE;
#endif /* DEBUG_RANGE_CHECK */

        error_code = si2176_pollforcts(si2176);
        if (error_code)
        {
                pr_info("%s: poll cts error:%d!!!!\n", __func__, error_code);
                goto exit;
        }

        cmdbytebuffer[0] = SI2176_TUNER_STATUS_CMD;
        cmdbytebuffer[1] = (unsigned char) ( ( intack & SI2176_TUNER_STATUS_CMD_INTACK_MASK ) << SI2176_TUNER_STATUS_CMD_INTACK_LSB);

        if (si2176_writecommandbytes(si2176, 2, cmdbytebuffer) != 2)
        {
                error_code = ERROR_SI2176_SENDING_COMMAND;
                pr_info("%s: write command byte error:%d!!!!\n", __func__, error_code);
        }

        if (!error_code)
        {
                error_code = si2176_pollforresponse(si2176, 1, 12, rspbytebuffer, &reply);
                if (error_code)
                        pr_info("%s: poll response error:%d!!!!\n", __func__, error_code);
                rsp->tuner_status.status = &reply;
                if (!error_code)
                {
                        rsp->tuner_status.tcint          = (( ( (rspbytebuffer[1]  )) >> SI2176_TUNER_STATUS_RESPONSE_TCINT_LSB    ) & SI2176_TUNER_STATUS_RESPONSE_TCINT_MASK    );
                        rsp->tuner_status.rssilint      = (( ( (rspbytebuffer[1]  )) >> SI2176_TUNER_STATUS_RESPONSE_RSSILINT_LSB ) & SI2176_TUNER_STATUS_RESPONSE_RSSILINT_MASK );
                        rsp->tuner_status.rssihint     = (( ( (rspbytebuffer[1]  )) >> SI2176_TUNER_STATUS_RESPONSE_RSSIHINT_LSB ) & SI2176_TUNER_STATUS_RESPONSE_RSSIHINT_MASK );
                        rsp->tuner_status.tc              = (( ( (rspbytebuffer[2]  )) >> SI2176_TUNER_STATUS_RESPONSE_TC_LSB       ) & SI2176_TUNER_STATUS_RESPONSE_TC_MASK       );
                        rsp->tuner_status.rssil          = (( ( (rspbytebuffer[2]  )) >> SI2176_TUNER_STATUS_RESPONSE_RSSIL_LSB    ) & SI2176_TUNER_STATUS_RESPONSE_RSSIL_MASK    );
                        rsp->tuner_status.rssih         = (( ( (rspbytebuffer[2]  )) >> SI2176_TUNER_STATUS_RESPONSE_RSSIH_LSB    ) & SI2176_TUNER_STATUS_RESPONSE_RSSIH_MASK    );
                        rsp->tuner_status.rssi           = (((( ( (rspbytebuffer[3]  )) >> SI2176_TUNER_STATUS_RESPONSE_RSSI_LSB     ) & SI2176_TUNER_STATUS_RESPONSE_RSSI_MASK) <<SI2176_TUNER_STATUS_RESPONSE_RSSI_SHIFT ) >>SI2176_TUNER_STATUS_RESPONSE_RSSI_SHIFT     );
                        rsp->tuner_status.freq          = (( ( (rspbytebuffer[4]  ) | (rspbytebuffer[5]  << 8 ) | (rspbytebuffer[6]  << 16 ) | (rspbytebuffer[7]  << 24 )) >> SI2176_TUNER_STATUS_RESPONSE_FREQ_LSB     ) & SI2176_TUNER_STATUS_RESPONSE_FREQ_MASK     );
                        rsp->tuner_status.mode        = (( ( (rspbytebuffer[8]  )) >> SI2176_TUNER_STATUS_RESPONSE_MODE_LSB     ) & SI2176_TUNER_STATUS_RESPONSE_MODE_MASK     );
                        rsp->tuner_status.vco_code = (((( ( (rspbytebuffer[10] ) | (rspbytebuffer[11] << 8 )) >> SI2176_TUNER_STATUS_RESPONSE_VCO_CODE_LSB ) & SI2176_TUNER_STATUS_RESPONSE_VCO_CODE_MASK) <<SI2176_TUNER_STATUS_RESPONSE_VCO_CODE_SHIFT ) >>SI2176_TUNER_STATUS_RESPONSE_VCO_CODE_SHIFT );
                }
        }
exit:
        return error_code;
}
#endif /* SI2176_TUNER_STATUS_CMD */
#ifdef SI2176_TUNER_TUNE_FREQ_CMD
/*---------------------------------------------------*/
/* SI2176_TUNER_TUNE_FREQ COMMAND                  */
/*---------------------------------------------------*/
unsigned char si2176_tuner_tune_freq(struct i2c_client *si2176,
                unsigned char   mode,
                unsigned long   freq,
                si2176_cmdreplyobj_t *rsp)
{
        unsigned char error_code = 0;
        unsigned char cmdbytebuffer[8];
        unsigned char rspbytebuffer[1];

#ifdef DEBUG_RANGE_CHECK
        if ( (mode > SI2176_TUNER_TUNE_FREQ_CMD_MODE_MAX)
                        || (freq > SI2176_TUNER_TUNE_FREQ_CMD_FREQ_MAX)  || (freq < SI2176_TUNER_TUNE_FREQ_CMD_FREQ_MIN) )
                return ERROR_SI2176_PARAMETER_OUT_OF_RANGE;
#endif /* DEBUG_RANGE_CHECK */

        error_code = si2176_pollforcts(si2176);
        if (error_code)
        {
                pr_info("%s: poll cts error:%d!!!!\n", __func__, error_code);
                goto exit;
        }

        cmdbytebuffer[0] = SI2176_TUNER_TUNE_FREQ_CMD;
        cmdbytebuffer[1] = (unsigned char) ( ( mode & SI2176_TUNER_TUNE_FREQ_CMD_MODE_MASK ) << SI2176_TUNER_TUNE_FREQ_CMD_MODE_LSB);
        cmdbytebuffer[2] = (unsigned char)0x00;
        cmdbytebuffer[3] = (unsigned char)0x00;
        cmdbytebuffer[4] = (unsigned char) ( ( freq & SI2176_TUNER_TUNE_FREQ_CMD_FREQ_MASK ) << SI2176_TUNER_TUNE_FREQ_CMD_FREQ_LSB);
        cmdbytebuffer[5] = (unsigned char) ((( freq & SI2176_TUNER_TUNE_FREQ_CMD_FREQ_MASK ) << SI2176_TUNER_TUNE_FREQ_CMD_FREQ_LSB)>>8);
        cmdbytebuffer[6] = (unsigned char) ((( freq & SI2176_TUNER_TUNE_FREQ_CMD_FREQ_MASK ) << SI2176_TUNER_TUNE_FREQ_CMD_FREQ_LSB)>>16);
        cmdbytebuffer[7] = (unsigned char) ((( freq & SI2176_TUNER_TUNE_FREQ_CMD_FREQ_MASK ) << SI2176_TUNER_TUNE_FREQ_CMD_FREQ_LSB)>>24);

        if (si2176_writecommandbytes(si2176, 8, cmdbytebuffer) != 8)
        {
                error_code = ERROR_SI2176_SENDING_COMMAND;
                pr_info("%s: write command byte error:%d!!!!\n", __func__, error_code);
        }

        if (!error_code)
        {
                error_code = si2176_pollforresponse(si2176, 1, 1, rspbytebuffer, &reply);
                if (error_code)
                        pr_info("%s: poll response error:%d!!!!\n", __func__, error_code);
                rsp->tuner_tune_freq.status = &reply;
        }
exit:
        return error_code;
}
#endif /* SI2176_TUNER_TUNE_FREQ_CMD */
/* _commands_insertion_point */

/* _send_command2_insertion_start */

/* --------------------------------------------*/
/* SEND_COMMAND2 FUNCTION                      */
/* --------------------------------------------*/
unsigned char si2176_sendcommand(struct i2c_client *si2176, int cmd, si2176_cmdobj_t *c, si2176_cmdreplyobj_t *rsp)
{
        switch (cmd)
        {
#ifdef SI2176_AGC_OVERRIDE_CMD
                case SI2176_AGC_OVERRIDE_CMD:
                        return si2176_agc_override(si2176, c->agc_override.force_max_gain, c->agc_override.force_top_gain, rsp);
                        break;
#endif /*     SI2176_AGC_OVERRIDE_CMD */
#ifdef SI2176_ATV_CW_TEST_CMD
                case SI2176_ATV_CW_TEST_CMD:
                        return si2176_atv_cw_test(si2176, c->atv_cw_test.pc_lock, rsp);
                        break;
#endif /*     SI2176_ATV_CW_TEST_CMD */
#ifdef SI2176_ATV_RESTART_CMD
                case SI2176_ATV_RESTART_CMD:
                        return si2176_atv_restart(si2176, c->atv_restart.mode, rsp);
                        break;
#endif /*     SI2176_ATV_RESTART_CMD */
#ifdef SI2176_ATV_STATUS_CMD
                case SI2176_ATV_STATUS_CMD:
                        return si2176_atv_status(si2176, c->atv_status.intack, rsp);
                        break;
#endif /*     SI2176_ATV_STATUS_CMD */
#ifdef SI2176_CONFIG_PINS_CMD
                case SI2176_CONFIG_PINS_CMD:
                        return si2176_config_pins(si2176, c->config_pins.gpio1_mode, c->config_pins.gpio1_read, c->config_pins.gpio2_mode, c->config_pins.gpio2_read, c->config_pins.gpio3_mode, c->config_pins.gpio3_read, c->config_pins.bclk1_mode, c->config_pins.bclk1_read, c->config_pins.xout_mode, rsp);
                        break;
#endif /*     SI2176_CONFIG_PINS_CMD */
#ifdef SI2176_EXIT_BOOTLOADER_CMD
                case SI2176_EXIT_BOOTLOADER_CMD:
                        return si2176_exit_bootloader(si2176, c->exit_bootloader.func, c->exit_bootloader.ctsien, rsp);
                        break;
#endif /*     SI2176_EXIT_BOOTLOADER_CMD */
#ifdef SI2176_FINE_TUNE_CMD
                case SI2176_FINE_TUNE_CMD:
                        return si2176_fine_tune(si2176, c->fine_tune.reserved, c->fine_tune.offset_500hz, rsp);
                        break;
#endif /*     SI2176_FINE_TUNE_CMD */
#ifdef SI2176_GET_PROPERTY_CMD
                case SI2176_GET_PROPERTY_CMD:
                        return si2176_get_property(si2176, c->get_property.reserved, c->get_property.prop, rsp);
                        break;
#endif /*     SI2176_GET_PROPERTY_CMD */
#ifdef SI2176_GET_REV_CMD
                case SI2176_GET_REV_CMD:
                        return si2176_get_rev(si2176, rsp);
                        break;
#endif /*     SI2176_GET_REV_CMD */
#ifdef SI2176_PART_INFO_CMD
                case SI2176_PART_INFO_CMD:
                        return si2176_part_info(si2176, rsp);
                        break;
#endif /*     SI2176_PART_INFO_CMD */
#ifdef SI2176_POWER_DOWN_CMD
                case SI2176_POWER_DOWN_CMD:
                        return si2176_power_down(si2176, rsp);
                        break;
#endif /*     SI2176_POWER_DOWN_CMD */
#ifdef SI2176_POWER_UP_CMD
                case SI2176_POWER_UP_CMD:
                        return si2176_power_up(si2176, c->power_up.subcode, c->power_up.reserved1, c->power_up.reserved2, c->power_up.reserved3, c->power_up.clock_mode, c->power_up.clock_freq, c->power_up.addr_mode, c->power_up.func, c->power_up.ctsien, c->power_up.wake_up, rsp);
                        break;
#endif /*     SI2176_POWER_UP_CMD */
#ifdef SI2176_SET_PROPERTY_CMD
                case SI2176_SET_PROPERTY_CMD:
                        return si2176_set_property(si2176, c->set_property.reserved, c->set_property.prop, c->set_property.data, rsp);
                        break;
#endif /*     SI2176_SET_PROPERTY_CMD */
#ifdef SI2176_STANDBY_CMD
                case SI2176_STANDBY_CMD:
                        return si2176_standby(si2176, rsp);
                        break;
#endif /*     SI2176_STANDBY_CMD */
#ifdef SI2176_TUNER_STATUS_CMD
                case SI2176_TUNER_STATUS_CMD:
                        return si2176_tuner_status(si2176, c->tuner_status.intack, rsp);
                        break;
#endif /*     SI2176_TUNER_STATUS_CMD */
#ifdef SI2176_TUNER_TUNE_FREQ_CMD
                case SI2176_TUNER_TUNE_FREQ_CMD:
                        return si2176_tuner_tune_freq(si2176, c->tuner_tune_freq.mode, c->tuner_tune_freq.freq, rsp);
                        break;
#endif /*     SI2176_TUNER_TUNE_FREQ_CMD */
                default :
                        break;
        }
        return 0;
}
/* _send_command2_insertion_point */

/***********************************************************************************************************************
  si2176_setproperty function
Use:        property set function
Used to call L1_SET_PROPERTY with the property Id and data provided.
Comments:   This is a way to make sure CTS is polled when setting a property
Parameter: *api     the SI2176 context
Parameter: waitforcts flag to wait for a CTS before issuing the property command
Parameter: waitforresponse flag to wait for a CTS after issuing the property command
Parameter: prop     the property Id
Parameter: data     the property bytes
Returns:    0 if no error, an error code otherwise
 ***********************************************************************************************************************/
static unsigned char si2176_setproperty(struct i2c_client *si2176, unsigned int prop, int  data, si2176_cmdreplyobj_t *rsp)
{
        unsigned char  reserved          = 0;
        return si2176_set_property(si2176, reserved, prop, data, rsp);
}

/***********************************************************************************************************************
  si2176_getproperty function
Use:        property get function
Used to call L1_GET_PROPERTY with the property Id provided.
Comments:   This is a way to make sure CTS is polled when retrieving a property
Parameter: *api     the SI2176 context
Parameter: waitforcts flag to wait for a CTS before issuing the property command
Parameter: waitforresponse flag to wait for a CTS after issuing the property command
Parameter: prop     the property Id
Parameter: *data    a buffer to store the property bytes into
Returns:    0 if no error, an error code otherwise
 ***********************************************************************************************************************/
static unsigned char si2176_getproperty(struct i2c_client *si2176, unsigned int prop, int *data, si2176_cmdreplyobj_t *rsp)
{
        unsigned char reserved = 0;
        unsigned char res;
        res = si2176_get_property(si2176, reserved, prop, rsp);
        *data = rsp->get_property.data;
        return res;
}

/* _set_property2_insertion_start */

/* --------------------------------------------*/
/* SET_PROPERTY2 FUNCTION                      */
/* --------------------------------------------*/
unsigned char si2176_sendproperty(struct i2c_client *si2176, unsigned int prop, si2176_propobj_t *p, si2176_cmdreplyobj_t *rsp)
{
        int data = 0;
        switch (prop)
        {
#ifdef SI2176_ATV_AFC_RANGE_PROP
                case SI2176_ATV_AFC_RANGE_PROP:
                        data = (p->atv_afc_range.range_khz & SI2176_ATV_AFC_RANGE_PROP_RANGE_KHZ_MASK) << SI2176_ATV_AFC_RANGE_PROP_RANGE_KHZ_LSB ;
                        break;
#endif /*     SI2176_ATV_AFC_RANGE_PROP */
#ifdef SI2176_ATV_AF_OUT_PROP
                case SI2176_ATV_AF_OUT_PROP:
                        data = (p->atv_af_out.volume & SI2176_ATV_AF_OUT_PROP_VOLUME_MASK) << SI2176_ATV_AF_OUT_PROP_VOLUME_LSB ;
                        break;
#endif /*     SI2176_ATV_AF_OUT_PROP */
#ifdef SI2176_ATV_AGC_SPEED_PROP
                case SI2176_ATV_AGC_SPEED_PROP:
                        data = (p->atv_agc_speed.if_agc_speed & SI2176_ATV_AGC_SPEED_PROP_IF_AGC_SPEED_MASK) << SI2176_ATV_AGC_SPEED_PROP_IF_AGC_SPEED_LSB ;
                        break;
#endif /*     SI2176_ATV_AGC_SPEED_PROP */
#ifdef SI2176_ATV_AUDIO_MODE_PROP
                case SI2176_ATV_AUDIO_MODE_PROP:
                        data = (p->atv_audio_mode.audio_sys  & SI2176_ATV_AUDIO_MODE_PROP_AUDIO_SYS_MASK ) << SI2176_ATV_AUDIO_MODE_PROP_AUDIO_SYS_LSB  |
                                (p->atv_audio_mode.demod_mode & SI2176_ATV_AUDIO_MODE_PROP_DEMOD_MODE_MASK) << SI2176_ATV_AUDIO_MODE_PROP_DEMOD_MODE_LSB  |
                                (p->atv_audio_mode.chan_bw    & SI2176_ATV_AUDIO_MODE_PROP_CHAN_BW_MASK   ) << SI2176_ATV_AUDIO_MODE_PROP_CHAN_BW_LSB ;
                        break;
#endif /*     SI2176_ATV_AUDIO_MODE_PROP */
#ifdef SI2176_ATV_CVBS_OUT_PROP
                case SI2176_ATV_CVBS_OUT_PROP:
                        data = (p->atv_cvbs_out.offset & SI2176_ATV_CVBS_OUT_PROP_OFFSET_MASK) << SI2176_ATV_CVBS_OUT_PROP_OFFSET_LSB  |
                                (p->atv_cvbs_out.amp    & SI2176_ATV_CVBS_OUT_PROP_AMP_MASK   ) << SI2176_ATV_CVBS_OUT_PROP_AMP_LSB ;
                        break;
#endif /*     SI2176_ATV_CVBS_OUT_PROP */
#ifdef SI2176_ATV_CVBS_OUT_FINE_PROP
                case SI2176_ATV_CVBS_OUT_FINE_PROP:
                        data = (p->atv_cvbs_out_fine.offset & SI2176_ATV_CVBS_OUT_FINE_PROP_OFFSET_MASK) << SI2176_ATV_CVBS_OUT_FINE_PROP_OFFSET_LSB  |
                                (p->atv_cvbs_out_fine.amp    & SI2176_ATV_CVBS_OUT_FINE_PROP_AMP_MASK   ) << SI2176_ATV_CVBS_OUT_FINE_PROP_AMP_LSB ;
                        break;
#endif /*     SI2176_ATV_CVBS_OUT_FINE_PROP */
#ifdef SI2176_ATV_IEN_PROP
                case SI2176_ATV_IEN_PROP:
                        data = (p->atv_ien.chlien  & SI2176_ATV_IEN_PROP_CHLIEN_MASK ) << SI2176_ATV_IEN_PROP_CHLIEN_LSB  |
                                (p->atv_ien.pclien  & SI2176_ATV_IEN_PROP_PCLIEN_MASK ) << SI2176_ATV_IEN_PROP_PCLIEN_LSB  |
                                (p->atv_ien.dlien   & SI2176_ATV_IEN_PROP_DLIEN_MASK  ) << SI2176_ATV_IEN_PROP_DLIEN_LSB  |
                                (p->atv_ien.snrlien & SI2176_ATV_IEN_PROP_SNRLIEN_MASK) << SI2176_ATV_IEN_PROP_SNRLIEN_LSB  |
                                (p->atv_ien.snrhien & SI2176_ATV_IEN_PROP_SNRHIEN_MASK) << SI2176_ATV_IEN_PROP_SNRHIEN_LSB ;
                        break;
#endif /*     SI2176_ATV_IEN_PROP */
#ifdef SI2176_ATV_INT_SENSE_PROP
                case SI2176_ATV_INT_SENSE_PROP:
                        data = (p->atv_int_sense.chlnegen  & SI2176_ATV_INT_SENSE_PROP_CHLNEGEN_MASK ) << SI2176_ATV_INT_SENSE_PROP_CHLNEGEN_LSB  |
                                (p->atv_int_sense.pclnegen  & SI2176_ATV_INT_SENSE_PROP_PCLNEGEN_MASK ) << SI2176_ATV_INT_SENSE_PROP_PCLNEGEN_LSB  |
                                (p->atv_int_sense.dlnegen   & SI2176_ATV_INT_SENSE_PROP_DLNEGEN_MASK  ) << SI2176_ATV_INT_SENSE_PROP_DLNEGEN_LSB  |
                                (p->atv_int_sense.snrlnegen & SI2176_ATV_INT_SENSE_PROP_SNRLNEGEN_MASK) << SI2176_ATV_INT_SENSE_PROP_SNRLNEGEN_LSB  |
                                (p->atv_int_sense.snrhnegen & SI2176_ATV_INT_SENSE_PROP_SNRHNEGEN_MASK) << SI2176_ATV_INT_SENSE_PROP_SNRHNEGEN_LSB  |
                                (p->atv_int_sense.chlposen  & SI2176_ATV_INT_SENSE_PROP_CHLPOSEN_MASK ) << SI2176_ATV_INT_SENSE_PROP_CHLPOSEN_LSB  |
                                (p->atv_int_sense.pclposen  & SI2176_ATV_INT_SENSE_PROP_PCLPOSEN_MASK ) << SI2176_ATV_INT_SENSE_PROP_PCLPOSEN_LSB  |
                                (p->atv_int_sense.dlposen   & SI2176_ATV_INT_SENSE_PROP_DLPOSEN_MASK  ) << SI2176_ATV_INT_SENSE_PROP_DLPOSEN_LSB  |
                                (p->atv_int_sense.snrlposen & SI2176_ATV_INT_SENSE_PROP_SNRLPOSEN_MASK) << SI2176_ATV_INT_SENSE_PROP_SNRLPOSEN_LSB  |
                                (p->atv_int_sense.snrhposen & SI2176_ATV_INT_SENSE_PROP_SNRHPOSEN_MASK) << SI2176_ATV_INT_SENSE_PROP_SNRHPOSEN_LSB ;
                        break;
#endif /*     SI2176_ATV_INT_SENSE_PROP */
#ifdef SI2176_ATV_MIN_LVL_LOCK_PROP
                case SI2176_ATV_MIN_LVL_LOCK_PROP:
                        data = (p->atv_min_lvl_lock.thrs & SI2176_ATV_MIN_LVL_LOCK_PROP_THRS_MASK) << SI2176_ATV_MIN_LVL_LOCK_PROP_THRS_LSB ;
                        break;
#endif /*     SI2176_ATV_MIN_LVL_LOCK_PROP */
#ifdef SI2176_ATV_RF_TOP_PROP
                case SI2176_ATV_RF_TOP_PROP:
                        data = (p->atv_rf_top.atv_rf_top & SI2176_ATV_RF_TOP_PROP_ATV_RF_TOP_MASK) << SI2176_ATV_RF_TOP_PROP_ATV_RF_TOP_LSB ;
                        break;
#endif /*     SI2176_ATV_RF_TOP_PROP */
#ifdef SI2176_ATV_RSQ_RSSI_THRESHOLD_PROP
                case SI2176_ATV_RSQ_RSSI_THRESHOLD_PROP:
                        data = (p->atv_rsq_rssi_threshold.lo & SI2176_ATV_RSQ_RSSI_THRESHOLD_PROP_LO_MASK) << SI2176_ATV_RSQ_RSSI_THRESHOLD_PROP_LO_LSB  |
                                (p->atv_rsq_rssi_threshold.hi & SI2176_ATV_RSQ_RSSI_THRESHOLD_PROP_HI_MASK) << SI2176_ATV_RSQ_RSSI_THRESHOLD_PROP_HI_LSB ;
                        break;
#endif /*     SI2176_ATV_RSQ_RSSI_THRESHOLD_PROP */
#ifdef SI2176_ATV_RSQ_SNR_THRESHOLD_PROP
                case SI2176_ATV_RSQ_SNR_THRESHOLD_PROP:
                        data = (p->atv_rsq_snr_threshold.lo & SI2176_ATV_RSQ_SNR_THRESHOLD_PROP_LO_MASK) << SI2176_ATV_RSQ_SNR_THRESHOLD_PROP_LO_LSB  |
                                (p->atv_rsq_snr_threshold.hi & SI2176_ATV_RSQ_SNR_THRESHOLD_PROP_HI_MASK) << SI2176_ATV_RSQ_SNR_THRESHOLD_PROP_HI_LSB ;
                        break;
#endif /*     SI2176_ATV_RSQ_SNR_THRESHOLD_PROP */
#ifdef Si2176_ATV_SOUND_AGC_SPEED_PROP
                case Si2176_ATV_SOUND_AGC_SPEED_PROP:
                        data = (p->atv_sound_agc.other_systems & Si2176_ATV_SOUND_AGC_SPEED_PROP_OTHER_SYSTEMS_MASK) << Si2176_ATV_SOUND_AGC_SPEED_PROP_OTHER_SYSTEMS_LSB |
                                (p->atv_sound_agc.system_l & Si2176_ATV_SOUND_AGC_SPEED_PROP_SYSTEM_L_MASK) << Si2176_ATV_SOUND_AGC_SPEED_PROP_SYSTEM_L_LSB;
                        break;
#endif
#ifdef SI2176_ATV_SIF_OUT_PROP
                case SI2176_ATV_SIF_OUT_PROP:
                        data = (p->atv_sif_out.offset & SI2176_ATV_SIF_OUT_PROP_OFFSET_MASK) << SI2176_ATV_SIF_OUT_PROP_OFFSET_LSB  |
                                (p->atv_sif_out.amp    & SI2176_ATV_SIF_OUT_PROP_AMP_MASK   ) << SI2176_ATV_SIF_OUT_PROP_AMP_LSB ;
                        break;
#endif /*     SI2176_ATV_SIF_OUT_PROP */
#ifdef SI2176_ATV_SOUND_AGC_LIMIT_PROP
                case SI2176_ATV_SOUND_AGC_LIMIT_PROP:
                        data = (p->atv_sound_agc_limit.max_gain & SI2176_ATV_SOUND_AGC_LIMIT_PROP_MAX_GAIN_MASK) << SI2176_ATV_SOUND_AGC_LIMIT_PROP_MAX_GAIN_LSB  |
                                (p->atv_sound_agc_limit.min_gain & SI2176_ATV_SOUND_AGC_LIMIT_PROP_MIN_GAIN_MASK) << SI2176_ATV_SOUND_AGC_LIMIT_PROP_MIN_GAIN_LSB ;
                        break;
#endif /*     SI2176_ATV_SOUND_AGC_LIMIT_PROP */
#ifdef SI2176_ATV_VIDEO_EQUALIZER_PROP
                case SI2176_ATV_VIDEO_EQUALIZER_PROP:
                        data = (p->atv_video_equalizer.slope & SI2176_ATV_VIDEO_EQUALIZER_PROP_SLOPE_MASK) << SI2176_ATV_VIDEO_EQUALIZER_PROP_SLOPE_LSB ;
                        break;
#endif /*     SI2176_ATV_VIDEO_EQUALIZER_PROP */
#ifdef SI2176_ATV_VIDEO_MODE_PROP
                case SI2176_ATV_VIDEO_MODE_PROP:
                        data = (p->atv_video_mode.video_sys       & SI2176_ATV_VIDEO_MODE_PROP_VIDEO_SYS_MASK      ) << SI2176_ATV_VIDEO_MODE_PROP_VIDEO_SYS_LSB  |
                                (p->atv_video_mode.color           & SI2176_ATV_VIDEO_MODE_PROP_COLOR_MASK          ) << SI2176_ATV_VIDEO_MODE_PROP_COLOR_LSB  |
                                (p->atv_video_mode.trans           & SI2176_ATV_VIDEO_MODE_PROP_TRANS_MASK          ) << SI2176_ATV_VIDEO_MODE_PROP_TRANS_LSB  |
                                (p->atv_video_mode.invert_signal   & SI2176_ATV_VIDEO_MODE_PROP_INVERT_SIGNAL_MASK  ) << SI2176_ATV_VIDEO_MODE_PROP_INVERT_SIGNAL_LSB ;
                        break;
#endif /*     SI2176_ATV_VIDEO_MODE_PROP */
#ifdef SI2176_ATV_VSNR_CAP_PROP
                case SI2176_ATV_VSNR_CAP_PROP:
                        data = (p->atv_vsnr_cap.atv_vsnr_cap & SI2176_ATV_VSNR_CAP_PROP_ATV_VSNR_CAP_MASK) << SI2176_ATV_VSNR_CAP_PROP_ATV_VSNR_CAP_LSB ;
                        break;
#endif /*     SI2176_ATV_VSNR_CAP_PROP */
#ifdef SI2176_ATV_VSYNC_TRACKING_PROP
                case SI2176_ATV_VSYNC_TRACKING_PROP:
                        data = (p->atv_vsync_tracking.min_pulses_to_lock   & SI2176_ATV_VSYNC_TRACKING_PROP_MIN_PULSES_TO_LOCK_MASK  ) << SI2176_ATV_VSYNC_TRACKING_PROP_MIN_PULSES_TO_LOCK_LSB  |
                                (p->atv_vsync_tracking.min_fields_to_unlock & SI2176_ATV_VSYNC_TRACKING_PROP_MIN_FIELDS_TO_UNLOCK_MASK) << SI2176_ATV_VSYNC_TRACKING_PROP_MIN_FIELDS_TO_UNLOCK_LSB ;
                        break;
#endif /*     SI2176_ATV_VSYNC_TRACKING_PROP */
#ifdef SI2176_CRYSTAL_TRIM_PROP
                case SI2176_CRYSTAL_TRIM_PROP:
                        data = (p->crystal_trim.xo_cap & SI2176_CRYSTAL_TRIM_PROP_XO_CAP_MASK) << SI2176_CRYSTAL_TRIM_PROP_XO_CAP_LSB ;
                        break;
#endif /*     SI2176_CRYSTAL_TRIM_PROP */
#ifdef SI2176_DTV_LIF_FREQ_PROP
                case SI2176_DTV_LIF_FREQ_PROP:
                        data = (p->dtv_lif_freq.offset & SI2176_DTV_LIF_FREQ_PROP_OFFSET_MASK) << SI2176_DTV_LIF_FREQ_PROP_OFFSET_LSB ;
                        break;
#endif /*     SI2176_DTV_LIF_FREQ_PROP */
#ifdef SI2176_DTV_LIF_OUT_PROP
                case SI2176_DTV_LIF_OUT_PROP:
                        data = (p->dtv_lif_out.offset & SI2176_DTV_LIF_OUT_PROP_OFFSET_MASK) << SI2176_DTV_LIF_OUT_PROP_OFFSET_LSB  |
                                (p->dtv_lif_out.amp    & SI2176_DTV_LIF_OUT_PROP_AMP_MASK   ) << SI2176_DTV_LIF_OUT_PROP_AMP_LSB ;
                        break;
#endif /*     SI2176_DTV_LIF_OUT_PROP */
#ifdef SI2176_DTV_MODE_PROP
                case SI2176_DTV_MODE_PROP:
                        data = (p->dtv_mode.bw              & SI2176_DTV_MODE_PROP_BW_MASK             ) << SI2176_DTV_MODE_PROP_BW_LSB  |
                                (p->dtv_mode.modulation      & SI2176_DTV_MODE_PROP_MODULATION_MASK     ) << SI2176_DTV_MODE_PROP_MODULATION_LSB  |
                                (p->dtv_mode.invert_spectrum & SI2176_DTV_MODE_PROP_INVERT_SPECTRUM_MASK) << SI2176_DTV_MODE_PROP_INVERT_SPECTRUM_LSB ;
                        break;
#endif /*     SI2176_DTV_MODE_PROP */
#ifdef SI2176_DTV_RF_TOP_PROP
                case SI2176_DTV_RF_TOP_PROP:
                        data = (p->dtv_rf_top.dtv_rf_top & SI2176_DTV_RF_TOP_PROP_DTV_RF_TOP_MASK) << SI2176_DTV_RF_TOP_PROP_DTV_RF_TOP_LSB ;
                        break;
#endif /*     SI2176_DTV_RF_TOP_PROP */
#ifdef SI2176_DTV_RSQ_RSSI_THRESHOLD_PROP
                case SI2176_DTV_RSQ_RSSI_THRESHOLD_PROP:
                        data = (p->dtv_rsq_rssi_threshold.lo & SI2176_DTV_RSQ_RSSI_THRESHOLD_PROP_LO_MASK) << SI2176_DTV_RSQ_RSSI_THRESHOLD_PROP_LO_LSB  |
                                (p->dtv_rsq_rssi_threshold.hi & SI2176_DTV_RSQ_RSSI_THRESHOLD_PROP_HI_MASK) << SI2176_DTV_RSQ_RSSI_THRESHOLD_PROP_HI_LSB ;
                        break;
#endif /*     SI2176_DTV_RSQ_RSSI_THRESHOLD_PROP */
#ifdef SI2176_MASTER_IEN_PROP
                case SI2176_MASTER_IEN_PROP:
                        data = (p->master_ien.tunien & SI2176_MASTER_IEN_PROP_TUNIEN_MASK) << SI2176_MASTER_IEN_PROP_TUNIEN_LSB  |
                                (p->master_ien.atvien & SI2176_MASTER_IEN_PROP_ATVIEN_MASK) << SI2176_MASTER_IEN_PROP_ATVIEN_LSB  |
                                (p->master_ien.dtvien & SI2176_MASTER_IEN_PROP_DTVIEN_MASK) << SI2176_MASTER_IEN_PROP_DTVIEN_LSB  |
                                (p->master_ien.errien & SI2176_MASTER_IEN_PROP_ERRIEN_MASK) << SI2176_MASTER_IEN_PROP_ERRIEN_LSB  |
                                (p->master_ien.ctsien & SI2176_MASTER_IEN_PROP_CTSIEN_MASK) << SI2176_MASTER_IEN_PROP_CTSIEN_LSB ;
                        break;
#endif /*     SI2176_MASTER_IEN_PROP */
#ifdef SI2176_TUNER_BLOCKED_VCO_PROP
                case SI2176_TUNER_BLOCKED_VCO_PROP:
                        data = (p->tuner_blocked_vco.vco_code & SI2176_TUNER_BLOCKED_VCO_PROP_VCO_CODE_MASK) << SI2176_TUNER_BLOCKED_VCO_PROP_VCO_CODE_LSB ;
                        break;
#endif /*     SI2176_TUNER_BLOCKED_VCO_PROP */
#ifdef SI2176_TUNER_IEN_PROP
                case SI2176_TUNER_IEN_PROP:
                        data = (p->tuner_ien.tcien    & SI2176_TUNER_IEN_PROP_TCIEN_MASK   ) << SI2176_TUNER_IEN_PROP_TCIEN_LSB  |
                                (p->tuner_ien.rssilien & SI2176_TUNER_IEN_PROP_RSSILIEN_MASK) << SI2176_TUNER_IEN_PROP_RSSILIEN_LSB  |
                                (p->tuner_ien.rssihien & SI2176_TUNER_IEN_PROP_RSSIHIEN_MASK) << SI2176_TUNER_IEN_PROP_RSSIHIEN_LSB ;
                        break;
#endif /*     SI2176_TUNER_IEN_PROP */
#ifdef SI2176_TUNER_INT_SENSE_PROP
                case SI2176_TUNER_INT_SENSE_PROP:
                        data = (p->tuner_int_sense.tcnegen    & SI2176_TUNER_INT_SENSE_PROP_TCNEGEN_MASK   ) << SI2176_TUNER_INT_SENSE_PROP_TCNEGEN_LSB  |
                                (p->tuner_int_sense.rssilnegen & SI2176_TUNER_INT_SENSE_PROP_RSSILNEGEN_MASK) << SI2176_TUNER_INT_SENSE_PROP_RSSILNEGEN_LSB  |
                                (p->tuner_int_sense.rssihnegen & SI2176_TUNER_INT_SENSE_PROP_RSSIHNEGEN_MASK) << SI2176_TUNER_INT_SENSE_PROP_RSSIHNEGEN_LSB  |
                                (p->tuner_int_sense.tcposen    & SI2176_TUNER_INT_SENSE_PROP_TCPOSEN_MASK   ) << SI2176_TUNER_INT_SENSE_PROP_TCPOSEN_LSB  |
                                (p->tuner_int_sense.rssilposen & SI2176_TUNER_INT_SENSE_PROP_RSSILPOSEN_MASK) << SI2176_TUNER_INT_SENSE_PROP_RSSILPOSEN_LSB  |
                                (p->tuner_int_sense.rssihposen & SI2176_TUNER_INT_SENSE_PROP_RSSIHPOSEN_MASK) << SI2176_TUNER_INT_SENSE_PROP_RSSIHPOSEN_LSB ;
                        break;
#endif /*     SI2176_TUNER_INT_SENSE_PROP */
#ifdef SI2176_TUNER_LO_INJECTION_PROP
                case SI2176_TUNER_LO_INJECTION_PROP:
                        data = (p->tuner_lo_injection.band_1 & SI2176_TUNER_LO_INJECTION_PROP_BAND_1_MASK) << SI2176_TUNER_LO_INJECTION_PROP_BAND_1_LSB  |
                                (p->tuner_lo_injection.band_2 & SI2176_TUNER_LO_INJECTION_PROP_BAND_2_MASK) << SI2176_TUNER_LO_INJECTION_PROP_BAND_2_LSB  |
                                (p->tuner_lo_injection.band_3 & SI2176_TUNER_LO_INJECTION_PROP_BAND_3_MASK) << SI2176_TUNER_LO_INJECTION_PROP_BAND_3_LSB  |
                                (p->tuner_lo_injection.band_4 & SI2176_TUNER_LO_INJECTION_PROP_BAND_4_MASK) << SI2176_TUNER_LO_INJECTION_PROP_BAND_4_LSB  |
                                (p->tuner_lo_injection.band_5 & SI2176_TUNER_LO_INJECTION_PROP_BAND_5_MASK) << SI2176_TUNER_LO_INJECTION_PROP_BAND_5_LSB ;
                        break;
#endif /*     SI2176_TUNER_LO_INJECTION_PROP */
                default :
                        break;
        }
        return si2176_setproperty(si2176, prop , data, rsp);
}
/* _set_property2_insertion_point */

/* _get_property2_insertion_start */

/* --------------------------------------------*/
/* GET_PROPERTY2 FUNCTION                       */
/* --------------------------------------------*/
unsigned char si2176_receiveproperty(struct i2c_client *si2176, unsigned int prop, si2176_propobj_t *p, si2176_cmdreplyobj_t *rsp)
{
        int data, res;

        res = si2176_getproperty(si2176, prop, &data, rsp);

        if (res!=NO_SI2176_ERROR)
                return res;
        switch (prop)
        {
#ifdef SI2176_ATV_AFC_RANGE_PROP
                case SI2176_ATV_AFC_RANGE_PROP:
                        p->atv_afc_range.range_khz = (data >> SI2176_ATV_AFC_RANGE_PROP_RANGE_KHZ_LSB) & SI2176_ATV_AFC_RANGE_PROP_RANGE_KHZ_MASK;
                        break;
#endif /*     SI2176_ATV_AFC_RANGE_PROP */
#ifdef SI2176_ATV_AF_OUT_PROP
                case SI2176_ATV_AF_OUT_PROP:
                        p->atv_af_out.volume = (data >> SI2176_ATV_AF_OUT_PROP_VOLUME_LSB) & SI2176_ATV_AF_OUT_PROP_VOLUME_MASK;
                        break;
#endif /*     SI2176_ATV_AF_OUT_PROP */
#ifdef SI2176_ATV_AGC_SPEED_PROP
                case SI2176_ATV_AGC_SPEED_PROP:
                        p->atv_agc_speed.if_agc_speed = (data >> SI2176_ATV_AGC_SPEED_PROP_IF_AGC_SPEED_LSB) & SI2176_ATV_AGC_SPEED_PROP_IF_AGC_SPEED_MASK;
                        break;
#endif /*     SI2176_ATV_AGC_SPEED_PROP */
#ifdef SI2176_ATV_AUDIO_MODE_PROP
                case SI2176_ATV_AUDIO_MODE_PROP:
                        p->atv_audio_mode.audio_sys      = (data >> SI2176_ATV_AUDIO_MODE_PROP_AUDIO_SYS_LSB ) & SI2176_ATV_AUDIO_MODE_PROP_AUDIO_SYS_MASK;
                        p->atv_audio_mode.demod_mode = (data >> SI2176_ATV_AUDIO_MODE_PROP_DEMOD_MODE_LSB) & SI2176_ATV_AUDIO_MODE_PROP_DEMOD_MODE_MASK;
                        p->atv_audio_mode.chan_bw        = (data >> SI2176_ATV_AUDIO_MODE_PROP_CHAN_BW_LSB   ) & SI2176_ATV_AUDIO_MODE_PROP_CHAN_BW_MASK;
                        break;
#endif /*     SI2176_ATV_AUDIO_MODE_PROP */
#ifdef SI2176_ATV_CVBS_OUT_PROP
                case SI2176_ATV_CVBS_OUT_PROP:
                        p->atv_cvbs_out.offset = (data >> SI2176_ATV_CVBS_OUT_PROP_OFFSET_LSB) & SI2176_ATV_CVBS_OUT_PROP_OFFSET_MASK;
                        p->atv_cvbs_out.amp   = (data >> SI2176_ATV_CVBS_OUT_PROP_AMP_LSB   ) & SI2176_ATV_CVBS_OUT_PROP_AMP_MASK;
                        break;
#endif /*     SI2176_ATV_CVBS_OUT_PROP */
#ifdef SI2176_ATV_CVBS_OUT_FINE_PROP
                case SI2176_ATV_CVBS_OUT_FINE_PROP:
                        p->atv_cvbs_out_fine.offset = (data >> SI2176_ATV_CVBS_OUT_FINE_PROP_OFFSET_LSB) & SI2176_ATV_CVBS_OUT_FINE_PROP_OFFSET_MASK;
                        p->atv_cvbs_out_fine.amp   = (data >> SI2176_ATV_CVBS_OUT_FINE_PROP_AMP_LSB   ) & SI2176_ATV_CVBS_OUT_FINE_PROP_AMP_MASK;
                        break;
#endif /*     SI2176_ATV_CVBS_OUT_FINE_PROP */
#ifdef SI2176_ATV_IEN_PROP
                case SI2176_ATV_IEN_PROP:
                        p->atv_ien.chlien   = (data >> SI2176_ATV_IEN_PROP_CHLIEN_LSB ) & SI2176_ATV_IEN_PROP_CHLIEN_MASK;
                        p->atv_ien.pclien   = (data >> SI2176_ATV_IEN_PROP_PCLIEN_LSB ) & SI2176_ATV_IEN_PROP_PCLIEN_MASK;
                        p->atv_ien.dlien     = (data >> SI2176_ATV_IEN_PROP_DLIEN_LSB  ) & SI2176_ATV_IEN_PROP_DLIEN_MASK;
                        p->atv_ien.snrlien  = (data >> SI2176_ATV_IEN_PROP_SNRLIEN_LSB) & SI2176_ATV_IEN_PROP_SNRLIEN_MASK;
                        p->atv_ien.snrhien = (data >> SI2176_ATV_IEN_PROP_SNRHIEN_LSB) & SI2176_ATV_IEN_PROP_SNRHIEN_MASK;
                        break;
#endif /*     SI2176_ATV_IEN_PROP */
#ifdef SI2176_ATV_INT_SENSE_PROP
                case SI2176_ATV_INT_SENSE_PROP:
                        p->atv_int_sense.chlnegen  = (data >> SI2176_ATV_INT_SENSE_PROP_CHLNEGEN_LSB ) & SI2176_ATV_INT_SENSE_PROP_CHLNEGEN_MASK;
                        p->atv_int_sense.pclnegen  = (data >> SI2176_ATV_INT_SENSE_PROP_PCLNEGEN_LSB ) & SI2176_ATV_INT_SENSE_PROP_PCLNEGEN_MASK;
                        p->atv_int_sense.dlnegen    = (data >> SI2176_ATV_INT_SENSE_PROP_DLNEGEN_LSB  ) & SI2176_ATV_INT_SENSE_PROP_DLNEGEN_MASK;
                        p->atv_int_sense.snrlnegen = (data >> SI2176_ATV_INT_SENSE_PROP_SNRLNEGEN_LSB) & SI2176_ATV_INT_SENSE_PROP_SNRLNEGEN_MASK;
                        p->atv_int_sense.snrhnegen = (data >> SI2176_ATV_INT_SENSE_PROP_SNRHNEGEN_LSB) & SI2176_ATV_INT_SENSE_PROP_SNRHNEGEN_MASK;
                        p->atv_int_sense.chlposen   = (data >> SI2176_ATV_INT_SENSE_PROP_CHLPOSEN_LSB ) & SI2176_ATV_INT_SENSE_PROP_CHLPOSEN_MASK;
                        p->atv_int_sense.pclposen   = (data >> SI2176_ATV_INT_SENSE_PROP_PCLPOSEN_LSB ) & SI2176_ATV_INT_SENSE_PROP_PCLPOSEN_MASK;
                        p->atv_int_sense.dlposen     = (data >> SI2176_ATV_INT_SENSE_PROP_DLPOSEN_LSB  ) & SI2176_ATV_INT_SENSE_PROP_DLPOSEN_MASK;
                        p->atv_int_sense.snrlposen  = (data >> SI2176_ATV_INT_SENSE_PROP_SNRLPOSEN_LSB) & SI2176_ATV_INT_SENSE_PROP_SNRLPOSEN_MASK;
                        p->atv_int_sense.snrhposen = (data >> SI2176_ATV_INT_SENSE_PROP_SNRHPOSEN_LSB) & SI2176_ATV_INT_SENSE_PROP_SNRHPOSEN_MASK;
                        break;
#endif /*     SI2176_ATV_INT_SENSE_PROP */
#ifdef SI2176_ATV_MIN_LVL_LOCK_PROP
                case SI2176_ATV_MIN_LVL_LOCK_PROP:
                        p->atv_min_lvl_lock.thrs = (data >> SI2176_ATV_MIN_LVL_LOCK_PROP_THRS_LSB) & SI2176_ATV_MIN_LVL_LOCK_PROP_THRS_MASK;
                        break;
#endif /*     SI2176_ATV_MIN_LVL_LOCK_PROP */
#ifdef SI2176_ATV_RF_TOP_PROP
                case SI2176_ATV_RF_TOP_PROP:
                        p->atv_rf_top.atv_rf_top = (data >> SI2176_ATV_RF_TOP_PROP_ATV_RF_TOP_LSB) & SI2176_ATV_RF_TOP_PROP_ATV_RF_TOP_MASK;
                        break;
#endif /*     SI2176_ATV_RF_TOP_PROP */
#ifdef SI2176_ATV_RSQ_RSSI_THRESHOLD_PROP
                case SI2176_ATV_RSQ_RSSI_THRESHOLD_PROP:
                        p->atv_rsq_rssi_threshold.lo = (data >> SI2176_ATV_RSQ_RSSI_THRESHOLD_PROP_LO_LSB) & SI2176_ATV_RSQ_RSSI_THRESHOLD_PROP_LO_MASK;
                        p->atv_rsq_rssi_threshold.hi = (data >> SI2176_ATV_RSQ_RSSI_THRESHOLD_PROP_HI_LSB) & SI2176_ATV_RSQ_RSSI_THRESHOLD_PROP_HI_MASK;
                        break;
#endif /*     SI2176_ATV_RSQ_RSSI_THRESHOLD_PROP */
#ifdef SI2176_ATV_RSQ_SNR_THRESHOLD_PROP
                case SI2176_ATV_RSQ_SNR_THRESHOLD_PROP:
                        p->atv_rsq_snr_threshold.lo = (data >> SI2176_ATV_RSQ_SNR_THRESHOLD_PROP_LO_LSB) & SI2176_ATV_RSQ_SNR_THRESHOLD_PROP_LO_MASK;
                        p->atv_rsq_snr_threshold.hi = (data >> SI2176_ATV_RSQ_SNR_THRESHOLD_PROP_HI_LSB) & SI2176_ATV_RSQ_SNR_THRESHOLD_PROP_HI_MASK;
                        break;
#endif /*     SI2176_ATV_RSQ_SNR_THRESHOLD_PROP */
#ifdef SI2176_ATV_SIF_OUT_PROP
                case SI2176_ATV_SIF_OUT_PROP:
                        p->atv_sif_out.offset  = (data >> SI2176_ATV_SIF_OUT_PROP_OFFSET_LSB) & SI2176_ATV_SIF_OUT_PROP_OFFSET_MASK;
                        p->atv_sif_out.amp    = (data >> SI2176_ATV_SIF_OUT_PROP_AMP_LSB   ) & SI2176_ATV_SIF_OUT_PROP_AMP_MASK;
                        break;
#endif /*     SI2176_ATV_SIF_OUT_PROP */
#ifdef SI2176_ATV_SOUND_AGC_LIMIT_PROP
                case SI2176_ATV_SOUND_AGC_LIMIT_PROP:
                        p->atv_sound_agc_limit.max_gain = (data >> SI2176_ATV_SOUND_AGC_LIMIT_PROP_MAX_GAIN_LSB) & SI2176_ATV_SOUND_AGC_LIMIT_PROP_MAX_GAIN_MASK;
                        p->atv_sound_agc_limit.min_gain  = (data >> SI2176_ATV_SOUND_AGC_LIMIT_PROP_MIN_GAIN_LSB) & SI2176_ATV_SOUND_AGC_LIMIT_PROP_MIN_GAIN_MASK;
                        break;
#endif /*     SI2176_ATV_SOUND_AGC_LIMIT_PROP */
#ifdef SI2176_ATV_VIDEO_EQUALIZER_PROP
                case SI2176_ATV_VIDEO_EQUALIZER_PROP:
                        p->atv_video_equalizer.slope = (data >> SI2176_ATV_VIDEO_EQUALIZER_PROP_SLOPE_LSB) & SI2176_ATV_VIDEO_EQUALIZER_PROP_SLOPE_MASK;
                        break;
#endif /*     SI2176_ATV_VIDEO_EQUALIZER_PROP */
#ifdef SI2176_ATV_VIDEO_MODE_PROP
                case SI2176_ATV_VIDEO_MODE_PROP:
                        p->atv_video_mode.video_sys     = (data >> SI2176_ATV_VIDEO_MODE_PROP_VIDEO_SYS_LSB      ) & SI2176_ATV_VIDEO_MODE_PROP_VIDEO_SYS_MASK;
                        p->atv_video_mode.color             = (data >> SI2176_ATV_VIDEO_MODE_PROP_COLOR_LSB          ) & SI2176_ATV_VIDEO_MODE_PROP_COLOR_MASK;
                        p->atv_video_mode.trans             = (data >> SI2176_ATV_VIDEO_MODE_PROP_TRANS_LSB          ) & SI2176_ATV_VIDEO_MODE_PROP_TRANS_MASK;
                        p->atv_video_mode.invert_signal = (data >> SI2176_ATV_VIDEO_MODE_PROP_INVERT_SIGNAL_LSB  ) & SI2176_ATV_VIDEO_MODE_PROP_INVERT_SIGNAL_MASK;
                        break;
#endif /*     SI2176_ATV_VIDEO_MODE_PROP */
#ifdef SI2176_ATV_VSNR_CAP_PROP
                case SI2176_ATV_VSNR_CAP_PROP:
                        p->atv_vsnr_cap.atv_vsnr_cap = (data >> SI2176_ATV_VSNR_CAP_PROP_ATV_VSNR_CAP_LSB) & SI2176_ATV_VSNR_CAP_PROP_ATV_VSNR_CAP_MASK;
                        break;
#endif /*     SI2176_ATV_VSNR_CAP_PROP */
#ifdef SI2176_ATV_VSYNC_TRACKING_PROP
                case SI2176_ATV_VSYNC_TRACKING_PROP:
                        p->atv_vsync_tracking.min_pulses_to_lock   = (data >> SI2176_ATV_VSYNC_TRACKING_PROP_MIN_PULSES_TO_LOCK_LSB  ) & SI2176_ATV_VSYNC_TRACKING_PROP_MIN_PULSES_TO_LOCK_MASK;
                        p->atv_vsync_tracking.min_fields_to_unlock = (data >> SI2176_ATV_VSYNC_TRACKING_PROP_MIN_FIELDS_TO_UNLOCK_LSB) & SI2176_ATV_VSYNC_TRACKING_PROP_MIN_FIELDS_TO_UNLOCK_MASK;
                        break;
#endif /*     SI2176_ATV_VSYNC_TRACKING_PROP */
#ifdef SI2176_CRYSTAL_TRIM_PROP
                case SI2176_CRYSTAL_TRIM_PROP:
                        p->crystal_trim.xo_cap = (data >> SI2176_CRYSTAL_TRIM_PROP_XO_CAP_LSB) & SI2176_CRYSTAL_TRIM_PROP_XO_CAP_MASK;
                        break;
#endif /*     SI2176_CRYSTAL_TRIM_PROP */
#ifdef SI2176_MASTER_IEN_PROP
                case SI2176_MASTER_IEN_PROP:
                        p->master_ien.tunien = (data >> SI2176_MASTER_IEN_PROP_TUNIEN_LSB) & SI2176_MASTER_IEN_PROP_TUNIEN_MASK;
                        p->master_ien.atvien = (data >> SI2176_MASTER_IEN_PROP_ATVIEN_LSB) & SI2176_MASTER_IEN_PROP_ATVIEN_MASK;
                        p->master_ien.dtvien = (data >> SI2176_MASTER_IEN_PROP_DTVIEN_LSB) & SI2176_MASTER_IEN_PROP_DTVIEN_MASK;
                        p->master_ien.errien = (data >> SI2176_MASTER_IEN_PROP_ERRIEN_LSB) & SI2176_MASTER_IEN_PROP_ERRIEN_MASK;
                        p->master_ien.ctsien = (data >> SI2176_MASTER_IEN_PROP_CTSIEN_LSB) & SI2176_MASTER_IEN_PROP_CTSIEN_MASK;
                        break;
#endif /*     SI2176_MASTER_IEN_PROP */
#ifdef SI2176_TUNER_BLOCKED_VCO_PROP
                case SI2176_TUNER_BLOCKED_VCO_PROP:
                        p->tuner_blocked_vco.vco_code = (data >> SI2176_TUNER_BLOCKED_VCO_PROP_VCO_CODE_LSB) & SI2176_TUNER_BLOCKED_VCO_PROP_VCO_CODE_MASK;
                        break;
#endif /*     SI2176_TUNER_BLOCKED_VCO_PROP */
#ifdef SI2176_TUNER_IEN_PROP
                case SI2176_TUNER_IEN_PROP:
                        p->tuner_ien.tcien      = (data >> SI2176_TUNER_IEN_PROP_TCIEN_LSB   ) & SI2176_TUNER_IEN_PROP_TCIEN_MASK;
                        p->tuner_ien.rssilien  = (data >> SI2176_TUNER_IEN_PROP_RSSILIEN_LSB) & SI2176_TUNER_IEN_PROP_RSSILIEN_MASK;
                        p->tuner_ien.rssihien = (data >> SI2176_TUNER_IEN_PROP_RSSIHIEN_LSB) & SI2176_TUNER_IEN_PROP_RSSIHIEN_MASK;
                        break;
#endif /*     SI2176_TUNER_IEN_PROP */
#ifdef SI2176_TUNER_INT_SENSE_PROP
                case SI2176_TUNER_INT_SENSE_PROP:
                        p->tuner_int_sense.tcnegen      = (data >> SI2176_TUNER_INT_SENSE_PROP_TCNEGEN_LSB   ) & SI2176_TUNER_INT_SENSE_PROP_TCNEGEN_MASK;
                        p->tuner_int_sense.rssilnegen  = (data >> SI2176_TUNER_INT_SENSE_PROP_RSSILNEGEN_LSB) & SI2176_TUNER_INT_SENSE_PROP_RSSILNEGEN_MASK;
                        p->tuner_int_sense.rssihnegen = (data >> SI2176_TUNER_INT_SENSE_PROP_RSSIHNEGEN_LSB) & SI2176_TUNER_INT_SENSE_PROP_RSSIHNEGEN_MASK;
                        p->tuner_int_sense.tcposen      = (data >> SI2176_TUNER_INT_SENSE_PROP_TCPOSEN_LSB   ) & SI2176_TUNER_INT_SENSE_PROP_TCPOSEN_MASK;
                        p->tuner_int_sense.rssilposen  = (data >> SI2176_TUNER_INT_SENSE_PROP_RSSILPOSEN_LSB) & SI2176_TUNER_INT_SENSE_PROP_RSSILPOSEN_MASK;
                        p->tuner_int_sense.rssihposen = (data >> SI2176_TUNER_INT_SENSE_PROP_RSSIHPOSEN_LSB) & SI2176_TUNER_INT_SENSE_PROP_RSSIHPOSEN_MASK;
                        break;
#endif /*     SI2176_TUNER_INT_SENSE_PROP */
#ifdef SI2176_TUNER_LO_INJECTION_PROP
                case SI2176_TUNER_LO_INJECTION_PROP:
                        p->tuner_lo_injection.band_1 = (data >> SI2176_TUNER_LO_INJECTION_PROP_BAND_1_LSB) & SI2176_TUNER_LO_INJECTION_PROP_BAND_1_MASK;
                        p->tuner_lo_injection.band_2 = (data >> SI2176_TUNER_LO_INJECTION_PROP_BAND_2_LSB) & SI2176_TUNER_LO_INJECTION_PROP_BAND_2_MASK;
                        p->tuner_lo_injection.band_3 = (data >> SI2176_TUNER_LO_INJECTION_PROP_BAND_3_LSB) & SI2176_TUNER_LO_INJECTION_PROP_BAND_3_MASK;
                        p->tuner_lo_injection.band_4 = (data >> SI2176_TUNER_LO_INJECTION_PROP_BAND_4_LSB) & SI2176_TUNER_LO_INJECTION_PROP_BAND_4_MASK;
                        p->tuner_lo_injection.band_5 = (data >> SI2176_TUNER_LO_INJECTION_PROP_BAND_5_LSB) & SI2176_TUNER_LO_INJECTION_PROP_BAND_5_MASK;
                        break;
#endif /*     SI2176_TUNER_LO_INJECTION_PROP */
                default :
                        break;
        }
        return res;
}

/************************************************************************************************************************
NAME: si2176_SetupATVDefaults
DESCRIPTION: Setup si2176 ATV startup configuration
This is a list of all the ATV configuration properties.   Depending on your application, only a subset may be required.
The properties are stored in the global property structure 'prop'.  The function ATVConfig(..) must be called
after any properties are modified.
Parameter:  none
Returns:    0 if successful
Programming Guide Reference:    Flowchart A.5 (ATV Setup flowchart)

 ************************************************************************************************************************/
static int si2176_setupatvdefaults(si2176_propobj_t *prop)
{
        prop->atv_cvbs_out.amp                                     = cvbs_amp;
        prop->atv_cvbs_out.offset                                   = 25;
        prop->atv_sif_out.amp                                         = 60;
        prop->atv_sif_out.offset                                       = 135;
        prop->atv_sound_agc.other_systems                  = 4;
        prop->atv_sound_agc.system_l                           = 5;
        prop->atv_sound_agc_limit.min_gain                   = -84;
        prop->atv_sound_agc_limit.max_gain                  = 72;
        prop->atv_afc_range.range_khz                          = 2000;
        prop->atv_rf_top.atv_rf_top                     = SI2176_ATV_RF_TOP_PROP_ATV_RF_TOP_AUTO;
        prop->atv_vsync_tracking.min_pulses_to_lock    = 2;
        prop->atv_vsync_tracking.min_fields_to_unlock  = 16;
        prop->atv_vsnr_cap.atv_vsnr_cap                       = 0;
        prop->atv_cvbs_out_fine.amp                              = 100;
        prop->atv_cvbs_out_fine.offset                            = 4;
#ifdef SI2176_B30
        prop->atv_agc_speed.if_agc_speed                    = 178;//patch for skyworth
#elif defined(SI2176_B2A)
        prop->atv_agc_speed.if_agc_speed                = SI2176_ATV_AGC_SPEED_PROP_IF_AGC_SPEED_AUTO;
#endif
        prop->atv_min_lvl_lock.thrs                                 = 34;
        prop->atv_af_out.volume                                     = 53;
        prop->atv_rsq_rssi_threshold.hi                          = 0;
        prop->atv_rsq_rssi_threshold.lo                          = -80;
        prop->atv_rsq_snr_threshold.hi                          = 60;//40;
        prop->atv_rsq_snr_threshold.lo                          = 10;//25;
        prop->atv_video_equalizer.slope                        = 0;
        prop->atv_int_sense.chlnegen                   = SI2176_ATV_INT_SENSE_PROP_CHLNEGEN_DISABLE;
        prop->atv_int_sense.chlposen                   = SI2176_ATV_INT_SENSE_PROP_CHLPOSEN_ENABLE;
        prop->atv_int_sense.dlnegen                     = SI2176_ATV_INT_SENSE_PROP_DLNEGEN_DISABLE;
        prop->atv_int_sense.dlposen                     = SI2176_ATV_INT_SENSE_PROP_DLPOSEN_ENABLE;
        prop->atv_int_sense.pclnegen                   = SI2176_ATV_INT_SENSE_PROP_PCLNEGEN_DISABLE;
        prop->atv_int_sense.pclposen                   = SI2176_ATV_INT_SENSE_PROP_PCLPOSEN_ENABLE;
        prop->atv_int_sense.snrhnegen                = SI2176_ATV_INT_SENSE_PROP_SNRHNEGEN_DISABLE;
        prop->atv_int_sense.snrhposen                = SI2176_ATV_INT_SENSE_PROP_SNRHPOSEN_ENABLE;
        prop->atv_int_sense.snrlnegen                 = SI2176_ATV_INT_SENSE_PROP_SNRLNEGEN_DISABLE;
        prop->atv_int_sense.snrlposen                 = SI2176_ATV_INT_SENSE_PROP_SNRLPOSEN_ENABLE;
        prop->atv_ien.chlien                                 = SI2176_ATV_IEN_PROP_CHLIEN_ENABLE;     /* enable only CHL to drive ATVINT */
        prop->atv_ien.dlien                                   = SI2176_ATV_IEN_PROP_DLIEN_ENABLE;
        prop->atv_ien.pclien                                 = SI2176_ATV_IEN_PROP_PCLIEN_ENABLE;
        prop->atv_ien.snrhien                               = SI2176_ATV_IEN_PROP_SNRHIEN_ENABLE;
        prop->atv_ien.snrlien                                = SI2176_ATV_IEN_PROP_SNRLIEN_ENABLE;
        prop->atv_audio_mode.audio_sys            = SI2176_ATV_AUDIO_MODE_PROP_AUDIO_SYS_MONO;
        prop->atv_audio_mode.chan_bw              = SI2176_ATV_AUDIO_MODE_PROP_CHAN_BW_DEFAULT;
        prop->atv_audio_mode.demod_mode       = SI2176_ATV_AUDIO_MODE_PROP_DEMOD_MODE_FM1;
        prop->atv_video_mode.video_sys             = SI2176_ATV_VIDEO_MODE_PROP_VIDEO_SYS_DK;
        prop->atv_video_mode.trans                     = SI2176_ATV_VIDEO_MODE_PROP_TRANS_CABLE;//SI2176_ATV_VIDEO_MODE_PROP_TRANS_TERRESTRIAL;
        prop->atv_video_mode.color                     = SI2176_ATV_VIDEO_MODE_PROP_COLOR_PAL_NTSC;

        return 0;
}
/************************************************************************************************************************
NAME: si2176_SetupCommonDefaults
DESCRIPTION: Setup si2176 Common startup configuration
This is a list of all the common configuration properties.   Depending on your application, only a subset may be required.
The properties are stored in the global property structure 'prop'.  The function CommonConfig(..) must be called
after any of these properties are modified.
Parameter:  none
Returns:    0 if successful
Programming Guide Reference:    Flowchart A.6a (Common setup flowchart)
 ************************************************************************************************************************/
static int si2176_setupcommondefaults(si2176_propobj_t *prop)
{
        /**** Enable Interrupt Sources *******/
        prop->master_ien.atvien = SI2176_MASTER_IEN_PROP_ATVIEN_ON;
        prop->master_ien.ctsien = SI2176_MASTER_IEN_PROP_CTSIEN_ON;
        prop->master_ien.dtvien = SI2176_MASTER_IEN_PROP_DTVIEN_OFF;//SI2176_MASTER_IEN_PROP_DTVIEN_OFF;
        prop->master_ien.errien = SI2176_MASTER_IEN_PROP_ERRIEN_OFF;
        prop->master_ien.tunien = SI2176_MASTER_IEN_PROP_TUNIEN_ON;
        /* Crystal Trim adjustment */
        prop->crystal_trim.xo_cap = 8;
        return 0;
}
/************************************************************************************************************************
NAME: si2176_SetupTunerDefaults
DESCRIPTION: Setup si2176 Tuner startup configuration
This is a list of all the Tuner configuration properties.   Depending on your application, only a subset may be required.
The properties are stored in the global property structure 'prop'.  The function TunerConfig(..) must be called
after any of these properties are modified.
Parameter:  none
Returns:    0 if successful
Programming Guide Reference:    Flowchart A.6a (Tuner setup flowchart)
 ************************************************************************************************************************/
static int si2176_setuptunerdefaults(si2176_propobj_t *prop)
{

        /* Setting si2176_TUNER_IEN_PROP property */
        prop->tuner_ien.tcien               = SI2176_TUNER_IEN_PROP_TCIEN_ENABLE; /* enable only TC to drive TUNINT */
        prop->tuner_ien.rssilien           = SI2176_TUNER_IEN_PROP_RSSILIEN_ENABLE;
        prop->tuner_ien.rssihien          = SI2176_TUNER_IEN_PROP_RSSIHIEN_DISABLE;

        /* Setting si2176_TUNER_BLOCK_VCO_PROP property */
        prop->tuner_blocked_vco.vco_code   = 0x8000;

        /* Setting si2176_TUNER_INT_SENSE_PROP property */
        prop->tuner_int_sense.rssihnegen  = SI2176_TUNER_INT_SENSE_PROP_RSSIHNEGEN_DISABLE;
        prop->tuner_int_sense.rssihposen  = SI2176_TUNER_INT_SENSE_PROP_RSSIHPOSEN_ENABLE;
        prop->tuner_int_sense.rssilnegen   = SI2176_TUNER_INT_SENSE_PROP_RSSILNEGEN_DISABLE;
        prop->tuner_int_sense.rssilposen   = SI2176_TUNER_INT_SENSE_PROP_RSSILPOSEN_ENABLE;
        prop->tuner_int_sense.tcnegen      = SI2176_TUNER_INT_SENSE_PROP_RSSIHNEGEN_DISABLE;
        prop->tuner_int_sense.tcposen      = SI2176_TUNER_INT_SENSE_PROP_TCPOSEN_ENABLE;


        /* Setting si2176_TUNER_LO_INJECTION_PROP property */
        prop->tuner_lo_injection.band_1     = SI2176_TUNER_LO_INJECTION_PROP_BAND_1_HIGH_SIDE;
        prop->tuner_lo_injection.band_2     = SI2176_TUNER_LO_INJECTION_PROP_BAND_2_LOW_SIDE;
        prop->tuner_lo_injection.band_3     = SI2176_TUNER_LO_INJECTION_PROP_BAND_3_LOW_SIDE;
        prop->tuner_lo_injection.band_4     = SI2176_TUNER_LO_INJECTION_PROP_BAND_4_LOW_SIDE;
        prop->tuner_lo_injection.band_5     = SI2176_TUNER_LO_INJECTION_PROP_BAND_5_LOW_SIDE;

        return 0;
}

/************************************************************************************************************************
NAME: si2176_ATVConfig
DESCRIPTION: Setup si2176 ATV properties configuration
This function will download all the ATV configuration properties stored in the global structure 'prop.
Depending on your application, only a subset may be required to be modified.
The function si2176_SetupATVDefaults() should be called before the first call to this function. Afterwards
to change a property change the appropriate element in the property structure 'prop' and call this routine.
Use the comments above the SetProperty2 calls as a guide to the parameters which are changed.
Parameter:  Pointer to si2176 Context (I2C address)
Returns:    I2C transaction error code, 0 if successful
Programming Guide Reference:    Flowchart A.5 (ATV setup flowchart)
 ************************************************************************************************************************/
int si2176_atvconfig(struct i2c_client *si2176, si2176_propobj_t *prop, si2176_cmdreplyobj_t *rsp)
{

        /* Set ATV_CVBS_OUT property */
        //prop.atv_cvbs_out.amp,
        //prop.atv_cvbs_out.offset
        if (si2176_sendproperty(si2176, SI2176_ATV_CVBS_OUT_PROP, prop, rsp) != 0)
        {
                return ERROR_SI2176_SENDING_COMMAND;
        }

        /* Set the ATV_SIF_OUT property */
        //prop.atv_sif_out.amp,
        //prop.atv_sif_out.offset
        if (si2176_sendproperty(si2176, SI2176_ATV_SIF_OUT_PROP, prop, rsp) != 0)
        {
                return ERROR_SI2176_SENDING_COMMAND;
        }
        if(si2176_sendproperty(si2176, Si2176_ATV_SOUND_AGC_SPEED_PROP, prop, rsp) !=0)
        {
                return ERROR_SI2176_SENDING_COMMAND;
        }
        /* Set the si2176_ATV_SOUND_AGC_LIMIT property */
        //prop.atv_sif_out.min_gain,
        //prop.atv_sif_out.max_gain
        if (si2176_sendproperty(si2176, SI2176_ATV_SOUND_AGC_LIMIT_PROP, prop, rsp) != 0)
        {
                return ERROR_SI2176_SENDING_COMMAND;
        }
        /* Set the ATV_AFC_RANGE property */
        //prop.atv_afc_range.range_khz
        if (si2176_sendproperty(si2176, SI2176_ATV_AFC_RANGE_PROP, prop, rsp) != 0)
        {
                return ERROR_SI2176_SENDING_COMMAND;
        }

        /* Set the ATV_RF_TOP property */
        //prop.atv_rf_top.atv_rf_top
        if (si2176_sendproperty(si2176, SI2176_ATV_RF_TOP_PROP, prop, rsp) != 0)
        {
                return ERROR_SI2176_SENDING_COMMAND;
        }

        /* Set the ATV_VSYNC_TRACKING property */
        //prop.atv_vsync_tracking.min_pulses_to_lock,
        //prop.atv_vsync_tracking.min_fields_to_unlock
        if (si2176_sendproperty(si2176, SI2176_ATV_VSYNC_TRACKING_PROP, prop, rsp) != 0)
        {
                return ERROR_SI2176_SENDING_COMMAND;
        }

        /* Set the ATV_VSNR_CAP property */
        //prop.atv_vsnr_cap.atv_vsnr_cap
        if (si2176_sendproperty(si2176, SI2176_ATV_VSNR_CAP_PROP, prop, rsp) != 0)
        {
                return ERROR_SI2176_SENDING_COMMAND;
        }

        /* Set the ATV_CVBS_OUT_FINE property */
        //prop.atv_cvbs_out_fine.amp,
        //prop.atv_cvbs_out_fine.offset
        if (si2176_sendproperty(si2176, SI2176_ATV_CVBS_OUT_FINE_PROP, prop, rsp) != 0)
        {
                return ERROR_SI2176_SENDING_COMMAND;
        }

        /* Set the ATV_AGC_SPEED property */
        //prop.atv_agc_speed.if_agc_speed
        if (si2176_sendproperty(si2176, SI2176_ATV_AGC_SPEED_PROP, prop, rsp) != 0)
        {
                return ERROR_SI2176_SENDING_COMMAND;
        }

        /* Set the ATV_MIN_LVL_LOCK property */
        //prop.atv_min_lvl_lock.thrs
        //prop.atv_min_lvl_lock.tol
        if (si2176_sendproperty(si2176, SI2176_ATV_MIN_LVL_LOCK_PROP, prop, rsp) != 0)
        {
                return ERROR_SI2176_SENDING_COMMAND;
        }

        /* Set the ATV_AF_OUT property */
        //prop.atv_af_out.volume
        if (si2176_sendproperty(si2176, SI2176_ATV_AF_OUT_PROP, prop, rsp) != 0)
        {
                return ERROR_SI2176_SENDING_COMMAND;
        }

        /*Set the ATV_RSSI_RSQ_THRESHOLD property */
        //prop.atv_rsq_rssi_threshold.hi,
        //prop.atv_rsq_rssi_threshold.lo
        if (si2176_sendproperty(si2176, SI2176_ATV_RSQ_RSSI_THRESHOLD_PROP, prop, rsp) != 0)
        {
                return ERROR_SI2176_SENDING_COMMAND;
        }

        /*Set the ATV_SNR_RSQ_THRESHOLD property */
        //prop.atv_rsq_snr_threshold.hi,
        //prop.atv_rsq_snr_threshold.lo
        if (si2176_sendproperty(si2176, SI2176_ATV_RSQ_SNR_THRESHOLD_PROP, prop, rsp) != 0)
        {
                return ERROR_SI2176_SENDING_COMMAND;
        }

        /*Set the ATV_VIDEO_EQUALIZER property */
        //prop.atv_video_equalizer.slope
        if (si2176_sendproperty(si2176, SI2176_ATV_VIDEO_EQUALIZER_PROP, prop, rsp) != 0)
        {
                return ERROR_SI2176_SENDING_COMMAND;
        }

        /*Set the ATV_INT_SENSE property */
        //prop.atv_int_sense.chlnegen,
        //prop.atv_int_sense.chlposen,
        //prop.atv_int_sense.dlnegen,
        //prop.atv_int_sense.dlposen,
        //prop.atv_int_sense.pclnegen,
        //prop.atv_int_sense.pclposen,
        //prop.atv_int_sense.snrhnegen,
        //prop.atv_int_sense.snrhposen,
        //prop.atv_int_sense.snrlnegen,
        //prop.atv_int_sense.snrlposen
        if (si2176_sendproperty(si2176, SI2176_ATV_INT_SENSE_PROP, prop, rsp) != 0)
        {
                return ERROR_SI2176_SENDING_COMMAND;
        }
        /* setup ATV_IEN_PROP  IEN properties to enable ATVINT on CHL  */

        /* prop.atv_ien.chlien,
           prop.atv_ien.pclien,
           prop.atv_ien.dlien ,
           prop.atv_ien.snrlien,
           prop.atv_ien.snrhien */
        if (si2176_sendproperty(si2176, SI2176_ATV_IEN_PROP, prop, rsp) != 0)
        {
                return ERROR_SI2176_SENDING_COMMAND;
        }

        /* Set the ATV_AUDIO_MODE property */
        /*rop.atv_audio_mode.audio_sys,
          prop.atv_audio_mode.chan_bw,
          prop.atv_audio_mode.demod_mode  */
        if (si2176_sendproperty(si2176, SI2176_ATV_AUDIO_MODE_PROP, prop, rsp) != 0)
        {
                return ERROR_SI2176_SENDING_COMMAND;
        }

        /* Set the ATV_VIDEO_MODE property */
        /*prop.atv_video_mode.video_sys,
          prop.atv_video_mode.trans,
          prop.atv_video_mode.color  */
        if (si2176_sendproperty(si2176, SI2176_ATV_VIDEO_MODE_PROP, prop, rsp) != 0)
        {
                return ERROR_SI2176_SENDING_COMMAND;
        }

        return 0;
}

/************************************************************************************************************************
NAME: si2176_CommonConfig
DESCRIPTION: Setup si2176 Common properties configuration
This function will download all the DTV configuration properties stored in the global structure 'prop.
Depending on your application, only a subset may be required to be modified.
The function si2176_SetupCommonDefaults() should be called before the first call to this function. Afterwards
to change a property change the appropriate element in the property structure 'prop' and call this routine.
Use the comments above the si2176_sendproperty calls as a guide to the parameters which are changed.
Parameter:  Pointer to si2176 Context (I2C address)
Returns:    I2C transaction error code, 0 if successful
Programming Guide Reference:    Flowchart A.6a (Common setup flowchart)
 ************************************************************************************************************************/
int si2176_commonconfig(struct i2c_client *si2176, si2176_propobj_t *prop, si2176_cmdreplyobj_t *rsp)
{

        /* Setting si2176_MASTER_IEN_PROP property */
        /***
          prop.master_ien.atvien,
          prop.master_ien.ctsien,
          prop.master_ien.dtvien,
          prop.master_ien.errien,
          prop.master_ien.tunien  */

        if (si2176_sendproperty(si2176, SI2176_MASTER_IEN_PROP, prop, rsp) != 0)
        {
                return ERROR_SI2176_SENDING_COMMAND;
        }

        /* Setting si2176_CRYSTAL_TRIM_PROP */
        //rop.crystal_trim.xo_cap
        if (si2176_sendproperty(si2176, SI2176_CRYSTAL_TRIM_PROP, prop, rsp) != 0)
        {
                return ERROR_SI2176_SENDING_COMMAND;
        }

        return 0;
}
/************************************************************************************************************************
NAME: si2176_TunerConfig
DESCRIPTION: Setup si2176 Tuner (RF to IF analog path) properties configuration
This function will download all the DTV configuration properties stored in the global structure 'prop.
Depending on your application, only a subset may be required to be modified.
The function si2176_SetupTunerDefaults() should be called before the first call to this function. Afterwards
to change a property change the appropriate element in the property structure 'prop' and call this routine.
Use the comments above the si2176_sendproperty calls as a guide to the parameters which are changed.
Parameter:  Pointer to si2176 Context (I2C address)
Returns:    I2C transaction error code, 0 if successful
Programming Guide Reference:    Flowchart A.6a (Tuner setup flowchart)
 ************************************************************************************************************************/
int si2176_tunerconfig(struct i2c_client *si2176, si2176_propobj_t *prop, si2176_cmdreplyobj_t *rsp)
{
        /* Set TUNER_IEN property */
        /*rop.tuner_ien.tcien,
          prop.tuner_ien.rssilien,
          prop.tuner_ien.rssihien */
        if (si2176_sendproperty(si2176, SI2176_TUNER_IEN_PROP, prop, rsp) != 0)
        {
                return ERROR_SI2176_SENDING_COMMAND;
        }

        /* Setting TUNER_BLOCKED_VCO property */
        /*rop.tuner_blocked_vco.vco_code, */
        if (si2176_sendproperty(si2176, SI2176_TUNER_BLOCKED_VCO_PROP, prop, rsp) != 0)
        {
                return ERROR_SI2176_SENDING_COMMAND;
        }
        /* Setting si2176_TUNER_INT_SENSE_PROP property */
        /*    rop.tuner_int_sense.rssihnegen,
              prop.tuner_int_sense.rssihposen,
              prop.tuner_int_sense.rssilnegen,
              prop.tuner_int_sense.rssilposen,
              prop.tuner_int_sense.tcnegen,
              prop.tuner_int_sense.tcposen */

        if (si2176_sendproperty(si2176, SI2176_TUNER_INT_SENSE_PROP, prop, rsp) != 0)
        {
                return ERROR_SI2176_SENDING_COMMAND;
        }

        /* Setting si2176_TUNER_LO_INJECTION_PROP property */
        /*rop.tuner_lo_injection.band_1,
          prop.tuner_lo_injection.band_2,
          prop.tuner_lo_injection.band_3,
          prop.tuner_lo_injection.band_4,
          prop.tuner_lo_injection.band_5 */
        if (si2176_sendproperty(si2176, SI2176_TUNER_LO_INJECTION_PROP, prop, rsp) != 0)
        {
                return ERROR_SI2176_SENDING_COMMAND;
        }


        return 0;
}
/************************************************************************************************************************
NAME: Si2170_SetupDTVDefaults
DESCRIPTION: Setup Si2170 DTV startup configuration
This is a list of all the DTV configuration properties.   Depending on your application, only a subset may be required.
The properties are stored in the global property structure 'prop'.  The function DTVConfig(..) must be called
after any properties are modified.
Parameter:  none
Returns:    0 if successful
Programming Guide Reference:    Flowchart A.6 (DTV setup flowchart)

 ************************************************************************************************************************/
static int si2176_setupdtvdefaults(si2176_propobj_t *prop, int ch_mode)
{
        prop->dtv_config_if_port.dtv_out_type   = SI2176_DTV_CONFIG_IF_PORT_PROP_DTV_OUT_TYPE_LIF_IF1;
        prop->dtv_config_if_port.dtv_agc_source = SI2176_DTV_CONFIG_IF_PORT_PROP_DTV_AGC_SOURCE_INTERNAL;
        prop->dtv_lif_freq.offset = 5000;
        prop->dtv_mode.bw = SI2176_DTV_MODE_PROP_BW_BW_8MHZ;
        prop->dtv_mode.invert_spectrum = SI2176_DTV_MODE_PROP_INVERT_SPECTRUM_NORMAL;
        //set in the get tuner ops
        prop->dtv_mode.modulation = SI2176_DTV_MODE_PROP_MODULATION_DVBC;
        prop->dtv_rsq_rssi_threshold.hi=0;
        prop->dtv_rsq_rssi_threshold.lo=-80;
        prop->dtv_ext_agc.max_10mv=250;
        prop->dtv_ext_agc.min_10mv=50;
        if(ch_mode == 4)
                prop->dtv_lif_out.amp=25;//22;//27;//23;
        else
                prop->dtv_lif_out.amp=25;//27;

        prop->dtv_lif_out.offset=148;
        prop->dtv_agc_speed.agc_decim=SI2176_DTV_AGC_SPEED_PROP_AGC_DECIM_OFF;
        prop->dtv_agc_speed.if_agc_speed=SI2176_DTV_AGC_SPEED_PROP_IF_AGC_SPEED_AUTO;
        prop->dtv_rf_top.dtv_rf_top=SI2176_DTV_RF_TOP_PROP_DTV_RF_TOP_AUTO;
        prop->dtv_int_sense.chlnegen=SI2176_DTV_INT_SENSE_PROP_CHLNEGEN_DISABLE;
        prop->dtv_int_sense.chlposen=SI2176_DTV_INT_SENSE_PROP_CHLPOSEN_ENABLE;
        prop->dtv_ien.chlien  = SI2176_DTV_IEN_PROP_CHLIEN_ENABLE;      /* enable only CHL to drive DTVINT */
        return 0;
}
/************************************************************************************************************************
NAME: Si2170_DTVConfig
DESCRIPTION: Setup Si2170 DTV properties configuration
This function will download all the DTV configuration properties stored in the global structure 'prop.
Depending on your application, only a subset may be required to be modified.
The function Si2170_SetupDTVDefaults() should be called before the first call to this function. Afterwards
to change a property change the appropriate element in the property structure 'prop' and call this routine.
Use the comments above the Si2170_L1_SetProperty2 calls as a guide to the parameters which are changed.
Parameter:  Pointer to Si2170 Context (I2C address)
Returns:    I2C transaction error code, 0 if successful
Programming Guide Reference:    Flowchart A.6 (DTV setup flowchart)
 ************************************************************************************************************************/
int si2176_dtvconfig(struct i2c_client *si2176, si2176_propobj_t *prop, si2176_cmdreplyobj_t *rsp)
{

        /* Setting DTV_CONFIG_IF_PORT_PROP property */
        /*	prop.dtv_config_if_port.dtv_out_type,
                prop.dtv_config_if_port.dtv_agc_source */
        if (si2176_sendproperty(si2176, SI2176_DTV_CONFIG_IF_PORT_PROP, prop, rsp) != 0)
        {
                return ERROR_SI2176_SENDING_COMMAND;
        }

        /* Setting DTV_LIF_FREQ_PROP */
        //	prop.dtv_lif_freq.offset
        if (si2176_sendproperty(si2176, SI2176_DTV_LIF_FREQ_PROP, prop, rsp) != 0)
        {
                return ERROR_SI2176_SENDING_COMMAND;
        }

        /* Setting DTV_MODE_PROP property */
        /*	prop.dtv_mode.bw,
                prop.dtv_mode.invert_spectrum,
                prop.dtv_mode.modulation*/
        if (si2176_sendproperty(si2176, SI2176_DTV_MODE_PROP, prop, rsp) != 0)
        {
                return ERROR_SI2176_SENDING_COMMAND;
        }

        /* Setting DTV_RSQ_RSSI_THRESHOLD property */
        //	prop.dtv_rsq_rssi_threshold.hi,
        //	prop.dtv_rsq_rssi_threshold.lo
        if (si2176_sendproperty(si2176, SI2176_DTV_RSQ_RSSI_THRESHOLD_PROP, prop, rsp) != 0)
        {
                return ERROR_SI2176_SENDING_COMMAND;
        }

        /* Setting DTV_EXT_AGC property */
        //	prop.dtv_ext_agc.max_10mv,
        //	prop.dtv_ext_agc.min_10mv
        if (si2176_sendproperty(si2176, SI2176_DTV_EXT_AGC_PROP, prop, rsp) != 0)
        {
                return ERROR_SI2176_SENDING_COMMAND;
        }

        /* Setting DTV_LIF_OUT property */
        //	prop.dtv_lif_out.amp,
        //	prop.dtv_lif_out.offset
        if (si2176_sendproperty(si2176, SI2176_DTV_LIF_OUT_PROP, prop, rsp) != 0)
        {
                return ERROR_SI2176_SENDING_COMMAND;
        }

        /* Setting DTV_AGC_SPEED property */
        //	prop.dtv_agc_speed.agc_decim,
        //	prop.dtv_agc_speed.if_agc_speed
        if (si2176_sendproperty(si2176, SI2176_DTV_AGC_SPEED_PROP, prop, rsp) != 0)
        {
                return ERROR_SI2176_SENDING_COMMAND;
        }

        /* Setting DTV_RF_TOP property */
        //	prop.dtv_rf_top.dtv_rf_top
        if (si2176_sendproperty(si2176, SI2176_DTV_RF_TOP_PROP, prop, rsp) != 0)
        {
                return ERROR_SI2176_SENDING_COMMAND;
        }

        /* Setting DTV_INT_SENSE property */
        //	prop.dtv_int_sense.chlnegen,
        //	prop.dtv_int_sense.chlposen
        if (si2176_sendproperty(si2176, SI2176_DTV_INT_SENSE_PROP, prop, rsp) != 0)
        {
                return ERROR_SI2176_SENDING_COMMAND;
        }

        /* Set DTV_IEN property */
        /*	prop.dtv_ien.chlien */
        if (si2176_sendproperty(si2176, SI2176_DTV_IEN_PROP, prop, rsp) != 0)
        {
                return ERROR_SI2176_SENDING_COMMAND;
        }

        return 0;
}


/************************************************************************************************************************
NAME: si2176_Tune
DESCRIPTIION: Tune si2176 in specified mode (ATV/DTV) at center frequency, wait for TUNINT and xTVINT with timeout

Parameter:  Pointer to si2176 Context (I2C address)
Parameter:  Mode (ATV or DTV) use si2176_TUNER_TUNE_FREQ_CMD_MODE_ATV or si2176_TUNER_TUNE_FREQ_CMD_MODE_DTV constants
Parameter:  frequency (Hz) as a unsigned long integer
Parameter:  rsp - commandResp structure to returns tune status info.
Returns:    0 if channel found.  A nonzero value means either an error occurred or channel not locked.
Programming Guide Reference:    Flowchart A.7 (Tune flowchart)
 ************************************************************************************************************************/
int si2176_tune(struct i2c_client *si2176, unsigned char mode, unsigned long freq, si2176_cmdreplyobj_t *rsp, si2176_common_reply_struct *common_reply)
{
        int return_code = 0,count = 0;

        if (si2176_tuner_tune_freq(si2176, mode, freq, rsp) != 0)
        {
                pr_info("%s: tuner tune freq error:%d!!!!\n", __func__, ERROR_SI2176_SENDING_COMMAND);
                return ERROR_SI2176_SENDING_COMMAND;
        }
        mdelay(5);
        for (count=50; count ;count--)
        {
                return_code = si2176_pollforcts(si2176);
                if (!return_code)
                        break;
                mdelay(2);
        }
        if (!count)
        {
                pr_info("%s: poll cts error:%d!!!!\n", __func__, return_code);
                return return_code;
        }
        mdelay(delay_det);
#if 0
        /* wait for TUNINT, timeout is 150ms */
        for (count=75; count ;count--)
        {
                if ((return_code = si2176_check_status(si2176, common_reply)) != 0)
                        return return_code;
                if (common_reply->tunint)
                        break;
                mdelay(2);
        }
        if (!count)
        {
                pr_info("%s: ERROR_SI2176_TUNINT_TIMEOUT error:%d!!!!\n", __func__, ERROR_SI2176_TUNINT_TIMEOUT);
                return ERROR_SI2176_TUNINT_TIMEOUT;
        }
        /* wait for xTVINT, timeout is 350ms for ATVINT and 6 ms for DTVINT */
        count = ((mode==SI2176_TUNER_TUNE_FREQ_CMD_MODE_ATV) ? 300 : 3);
        for (;count ;count--)
        {
                if ((return_code = si2176_check_status(si2176, common_reply)) != 0)
                        return return_code;
                if ((mode==SI2176_TUNER_TUNE_FREQ_CMD_MODE_ATV) ? common_reply->atvint : common_reply->dtvint)
                        break;
                mdelay(2);
        }
        if (!count)
        {
                pr_info("%s: ERROR_SI2176_XTVINT_TIMEOUT error:%d!!!!\n", __func__, ERROR_SI2176_XTVINT_TIMEOUT);
                return ERROR_SI2176_XTVINT_TIMEOUT;
        }
#endif
        return return_code;
}

/************************************************************************************************************************
NAME: si2176_LoadVideofilter
DESCRIPTION:        Load video filters from vidfiltTable in si2176_write_xTV_video_coeffs.h file into si2176
Programming Guide Reference:    Flowchart A.4 (Download Video Filters flowchart)

Parameter:  si2176 Context (I2C address)
Parameter:  pointer to video filter table array
Parameter:  number of lines in video filter table array (size in bytes / BYTES_PER_LINE)
Returns:    si2176/I2C transaction error code, 0 if successful
 ************************************************************************************************************************/
int si2176_loadvideofilter(struct i2c_client *si2176, unsigned char* vidfilttable, int lines, si2176_common_reply_struct *common_reply)
{
#define BYTES_PER_LINE 8
        int line;

        /* for each 8 bytes in VIDFILT_TABLE */
        for (line = 0; line < lines; line++)
        {
                /* send that 8 byte I2C command to si2176 */
                if (si2176_api_patch(si2176, BYTES_PER_LINE, vidfilttable+BYTES_PER_LINE*line, common_reply) != 0)
                {
                        return ERROR_SI2176_SENDING_COMMAND;
                }
        }
        return 0;
}
/************************************************************************************************************************
NAME: si2176_Configure
DESCRIPTION: Setup si2176 video filters, GPIOs/clocks, Common Properties startup, Tuner startup, ATV startup, and DTV startup.
Parameter:  Pointer to si2176 Context (I2C address)
Parameter:  rsp - commandResp structure buffer.
Returns:    I2C transaction error code, 0 if successful
 ************************************************************************************************************************/
int si2176_configure(struct i2c_client *si2176, si2176_propobj_t *prop, si2176_cmdreplyobj_t *rsp, si2176_common_reply_struct *common_reply)
{
        int return_code = 0;

        /* load ATV video filter file */
#ifdef USING_ATV_FILTER
        if ((return_code = si2176_loadvideofilter(si2176, dlif_vidfilt_table, DLIF_VIDFILT_LINES, common_reply) != 0)
                        return return_code;
#endif

                        /* Set the GPIO Pins using the CONFIG_PINS command*/
                        if (si2176_config_pins(si2176,      /* turn off BCLK1 and XOUT */
                                        SI2176_CONFIG_PINS_CMD_GPIO1_MODE_NO_CHANGE,
                                        SI2176_CONFIG_PINS_CMD_GPIO1_READ_DO_NOT_READ,
                                        SI2176_CONFIG_PINS_CMD_GPIO2_MODE_NO_CHANGE,
                                        SI2176_CONFIG_PINS_CMD_GPIO2_READ_DO_NOT_READ,
                                        SI2176_CONFIG_PINS_CMD_GPIO3_MODE_NO_CHANGE,
                                        SI2176_CONFIG_PINS_CMD_GPIO3_READ_DO_NOT_READ,
                                        SI2176_CONFIG_PINS_CMD_BCLK1_MODE_DISABLE,
                                        SI2176_CONFIG_PINS_CMD_BCLK1_READ_DO_NOT_READ,
                                        SI2176_CONFIG_PINS_CMD_XOUT_MODE_DISABLE,
                                        rsp) !=0)
                        {
                        pr_info("%s: config pins error:%d!!!!\n", __func__, ERROR_SI2176_SENDING_COMMAND);
                        return ERROR_SI2176_SENDING_COMMAND;
                        }
#if  1
                        /* Set Common Properties startup configuration */
                        si2176_setupcommondefaults(prop);
                        if ((return_code = si2176_commonconfig(si2176, prop, rsp)) != 0)
                        {
                                pr_info("%s: setup command defaults error:%d!!!!\n", __func__, return_code);
                                return return_code;
                        }

                        /* Set Tuner Properties startup configuration */
                        si2176_setuptunerdefaults(prop);
                        if ((return_code = si2176_tunerconfig(si2176, prop, rsp)) != 0)
                        {
                                pr_info("%s: setup tuner defaults error:%d!!!!\n", __func__, return_code);
                                return return_code;
                        }
                        /* Set ATV startup configuration */
                        si2176_setupatvdefaults(prop);
                        if ((return_code = si2176_atvconfig(si2176, prop, rsp)) != 0)
                        {
                                pr_info("%s: setup atv defaults error:%d!!!!\n", __func__, return_code);
                                return return_code;
                        }
                        /* Set DTV startup configuration */
                        si2176_setupdtvdefaults(prop, 4);
                        if ((return_code = si2176_dtvconfig(si2176, prop, rsp)) != 0)
                        {
                                pr_info("%s: setup dtv defaults error:%d!!!!\n", __func__, return_code);
                                return return_code;
                        }
#endif

                        return return_code;
}
/************************************************************************************************************************
NAME: si2176_LoadFirmware
DESCRIPTON: Load firmware from FIRMWARE_TABLE array in si2176_Firmware_x_y_build_z.h file into si2176
Requires si2176 to be in bootloader mode after PowerUp
Programming Guide Reference:    Flowchart A.3 (Download FW PATCH flowchart)

Parameter:  si2176 Context (I2C address)
Parameter:  pointer to firmware table array
Parameter:  number of lines in firmware table array (size in bytes / BYTES_PER_LINE)
Returns:    si2176/I2C transaction error code, 0 if successful
 ************************************************************************************************************************/
int si2176_loadfirmware(struct i2c_client *si2176, unsigned char* firmwaretable, int lines, si2176_common_reply_struct *common_reply)
{
        int return_code = 0;
        int line;

        /* for each 8 bytes in FIRMWARE_TABLE */
        for (line = 0; line < lines; line++)
        {
                /* send that 8 byte I2C command to si2176 */
                if (si2176_api_patch(si2176, 8, firmwaretable+8*line, common_reply) != 0)
                {
                        return ERROR_SI2176_LOADING_FIRMWARE;
                }
        }
        return return_code;
}

/************************************************************************************************************************
NAME: si2176_StartFirmware
DESCRIPTION: Start si2176 firmware (put the si2176 into run mode)
Parameter:   si2176 Context (I2C address)
Parameter (passed by Reference): 	ExitBootloadeer Response Status byte : tunint, atvint, dtvint, err, cts
Returns:     I2C transaction error code, 0 if successful
 ************************************************************************************************************************/
int si2176_startfirmware(struct i2c_client *si2176, si2176_cmdreplyobj_t *rsp)
{

        if (si2176_exit_bootloader(si2176, SI2176_EXIT_BOOTLOADER_CMD_FUNC_TUNER, SI2176_EXIT_BOOTLOADER_CMD_CTSIEN_OFF, rsp) != 0)
        {
                return ERROR_SI2176_STARTING_FIRMWARE;
        }

        return 0;
}
/************************************************************************************************************************
NAME: si2176_PowerUpWithPatch
DESCRIPTION: Send si2176 API PowerUp Command with PowerUp to bootloader,
Check the Chip rev and part, and ROMID are compared to expected values.
Load the Firmware Patch then Start the Firmware.
Programming Guide Reference:    Flowchart A.2a (POWER_UP with patch flowchart)

Parameter:  si2176 Context (I2C address)
Returns:    si2176/I2C transaction error code, 0 if successful
 ************************************************************************************************************************/
int si2176_powerupwithpatch(struct i2c_client *si2176, si2176_cmdreplyobj_t *rsp, si2176_common_reply_struct *common_reply)
{
        int return_code = 0, num = 0;

        return_code = si2176_power_up(si2176,          /* always wait for CTS prior to POWER_UP command */
                        SI2176_POWER_UP_CMD_SUBCODE_CODE,
                        SI2176_POWER_UP_CMD_RESERVED1_RESERVED,
                        SI2176_POWER_UP_CMD_RESERVED2_RESERVED,
                        SI2176_POWER_UP_CMD_RESERVED3_RESERVED,
                        SI2176_POWER_UP_CMD_CLOCK_MODE_XTAL,
                        SI2176_POWER_UP_CMD_CLOCK_FREQ_CLK_24MHZ,
                        SI2176_POWER_UP_CMD_ADDR_MODE_CURRENT,
                        SI2176_POWER_UP_CMD_FUNC_BOOTLOADER,        /* start in bootloader mode */
                        SI2176_POWER_UP_CMD_CTSIEN_DISABLE,
                        SI2176_POWER_UP_CMD_WAKE_UP_WAKE_UP,
                        rsp);
        if (return_code)
                pr_info("%s: si2176_power_up error:%d!!!\n", __func__, return_code);

#if 0
        /* Get the Part Info from the chip.   This command is only valid in Bootloader mode */
        if (si2176_part_info(si2176, &rsp) != 0)
                return ERROR_SI2176_SENDING_COMMAND;
        /* Check the Chip revision, part and ROMID against expected values and report an error if incompatible */
        if (rsp.part_info.chiprev != chiprev)
                return ERROR_SI2176_INCOMPATIBLE_PART;
        /* Part Number is represented as the last 2 digits */
        if (rsp.part_info.part != part)
                return ERROR_SI2176_INCOMPATIBLE_PART;
        /* Part Major Version in ASCII */
        if (rsp.part_info.pmajor != partmajorversion)
                return ERROR_SI2176_INCOMPATIBLE_PART;

#endif
        mdelay(25);
        /* Load the Firmware */
        if (si2176_version == 30)  // for version B30
        {
                num = sizeof(firmwaretable_b30)/(8*sizeof(unsigned char));
                return_code = si2176_loadfirmware(si2176, firmwaretable_b30, num, common_reply);
        }
        else if (si2176_version == 29)  // for version B29,just is 3.2B7
        {
                num = sizeof(firmwaretable_b29)/(8*sizeof(unsigned char));
                return_code = si2176_loadfirmware(si2176, firmwaretable_b29, num, common_reply);
        }
        else if (si2176_version == 42)  // for version B2A  
        {
                printk("si2176 load firmware b2a.\n");
                num = sizeof(firmwaretable_b2a)/(8*sizeof(unsigned char));
                return_code = si2176_loadfirmware(si2176, firmwaretable_b2a, num, common_reply);
        }
        else
        {
                num = sizeof(firmwaretable_b30)/(8*sizeof(unsigned char));
                return_code = si2176_loadfirmware(si2176, firmwaretable_b30, num, common_reply);
        }

        if (return_code != 0)
        {
                pr_info("%s: si2176_loadfirmware error:%d!!!\n", __func__, return_code);
                return return_code;
        }

        /*Start the Firmware */
        if ((return_code = si2176_startfirmware(si2176, rsp)) != 0) /* Start firmware */
        {
                pr_info("%s: si2176_startfirmware error:%d!!!\n", __func__, return_code);
                return return_code;
        }
        mdelay(50);
        if (si2176_get_rev(si2176, rsp) != 0)
        {
                pr_info("%s: si2176_get_rev error:%d!!!\n", __func__, ERROR_SI2176_SENDING_COMMAND);
                return ERROR_SI2176_SENDING_COMMAND;
        }
        else
        {
                pr_info("%s: rsp.get_rev.pn :      %d \n", __func__, rsp->get_rev.pn);
                pr_info("%s: rsp.get_rev.fwmajor : %d \n", __func__, rsp->get_rev.fwmajor);
                pr_info("%s: rsp.get_rev.fwminor : %d \n", __func__, rsp->get_rev.fwminor);
                pr_info("%s: rsp.get_rev.patch :   %d \n", __func__, rsp->get_rev.patch);
                pr_info("%s: rsp.get_rev.cmpmajor :%d \n", __func__, rsp->get_rev.cmpmajor);
                pr_info("%s: rsp.get_rev.cmpminor :%d \n", __func__, rsp->get_rev.cmpminor);
                pr_info("%s: rsp.get_rev.cmpbuild :%d \n", __func__, rsp->get_rev.cmpbuild);
                pr_info("%s: rsp.get_rev.chiprev : %d \n", __func__, (u32)rsp->get_rev.chiprev);
        }

        return return_code;
}


/************************************************************************************************************************
NAME: si2176_Init
DESCRIPTION:Reset and Initialize si2176
Parameter:  si2176 Context (I2C address)
Returns:    I2C transaction error code, 0 if successful
 ************************************************************************************************************************/
int si2176_init(struct i2c_client *si2176, si2176_cmdreplyobj_t *rsp, si2176_common_reply_struct *common_reply)
{
        int return_code = 0;

        /* Reset si2176 */
        /* TODO: SendRSTb requires porting to toggle the RSTb line low -> high */
        //sendrstb();

        return_code = si2176_powerupwithpatch(si2176, rsp, common_reply);
        if (return_code)		/* PowerUp into bootloader */
        {
                pr_info("%s: init si2176 error!!!\n", __func__);
        }

        /* At this point, FW is loaded and started.  Return 0*/
        return return_code;
}




