/*
 * drivers/amlogic/input/adc_keypad/adc_holdkey.c
 *
 * ADC HoldKey Driver
 *
 * Copyright (C) 2012 Amlogic, Inc.
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * author :   Alex Deng
 */


struct call_key_platform_data
{
	int hp_in_value;
	int hp_out_value;
	int (*get_hp_status)(void);
    int (*get_call_key_value)(void);
    int (*get_phone_ring_value)(void);
};
