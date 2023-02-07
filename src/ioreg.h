/*
 *	avarice - The "avarice" program.
 *	Copyright (C) 2003, 2004 James Harris
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License Version 2
 *      as published by the Free Software Foundation.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 *
 * This file contains IO Register definitions for use with avr-gdb's 'info
 * io_register' command.
 *
 * $Id$
 */

#ifndef INCLUDE_IOREG_H
#define INCLUDE_IOREG_H

#define IO_REG_RSE    0x01    // IO register has read side effect

typedef struct {
    const char* name;
    const unsigned int reg_addr;
    const unsigned char flags;
} gdb_io_reg_def_type;

extern gdb_io_reg_def_type atmega16_io_registers[];
extern gdb_io_reg_def_type atmega162_io_registers[];
extern gdb_io_reg_def_type atmega169_io_registers[];
extern gdb_io_reg_def_type atmega32_io_registers[];
extern gdb_io_reg_def_type atmega128_io_registers[];
extern gdb_io_reg_def_type atmega323_io_registers[];
extern gdb_io_reg_def_type atmega64_io_registers[];
extern gdb_io_reg_def_type at90can128_io_registers[];
extern gdb_io_reg_def_type atmega164p_io_registers[];
extern gdb_io_reg_def_type atmega324p_io_registers[];
extern gdb_io_reg_def_type atmega324pb_io_registers[];
extern gdb_io_reg_def_type atmega644_io_registers[];
extern gdb_io_reg_def_type atmega325_io_registers[];
extern gdb_io_reg_def_type atmega3250_io_registers[];
extern gdb_io_reg_def_type atmega645_io_registers[];
extern gdb_io_reg_def_type atmega6450_io_registers[];
extern gdb_io_reg_def_type atmega329_io_registers[];
extern gdb_io_reg_def_type atmega3290_io_registers[];
extern gdb_io_reg_def_type atmega649_io_registers[];
extern gdb_io_reg_def_type atmega6490_io_registers[];
extern gdb_io_reg_def_type atmega640_io_registers[];
extern gdb_io_reg_def_type atmega1280_io_registers[];
extern gdb_io_reg_def_type atmega1281_io_registers[];
extern gdb_io_reg_def_type atmega2560_io_registers[];
extern gdb_io_reg_def_type atmega2561_io_registers[];
extern gdb_io_reg_def_type atmega48_io_registers[];
extern gdb_io_reg_def_type atmega88_io_registers[];
extern gdb_io_reg_def_type atmega168_io_registers[];
extern gdb_io_reg_def_type attiny13_io_registers[];
extern gdb_io_reg_def_type attiny2313_io_registers[];
extern gdb_io_reg_def_type at90pwm2_io_registers[];
extern gdb_io_reg_def_type at90pwm3_io_registers[];
extern gdb_io_reg_def_type at90pwm2b_io_registers[];
extern gdb_io_reg_def_type at90pwm3b_io_registers[];
extern gdb_io_reg_def_type attiny24_io_registers[];
extern gdb_io_reg_def_type attiny44_io_registers[];
extern gdb_io_reg_def_type attiny84_io_registers[];
extern gdb_io_reg_def_type attiny25_io_registers[];
extern gdb_io_reg_def_type attiny45_io_registers[];
extern gdb_io_reg_def_type attiny85_io_registers[];
extern gdb_io_reg_def_type attiny261_io_registers[];
extern gdb_io_reg_def_type attiny402_io_registers[];
extern gdb_io_reg_def_type attiny412_io_registers[];
extern gdb_io_reg_def_type attiny461_io_registers[];
extern gdb_io_reg_def_type attiny814_io_registers[];
extern gdb_io_reg_def_type attiny861_io_registers[];
extern gdb_io_reg_def_type atmega32c1_io_registers[];
extern gdb_io_reg_def_type atmega32m1_io_registers[];
extern gdb_io_reg_def_type at90can32_io_registers[];
extern gdb_io_reg_def_type at90can64_io_registers[];
extern gdb_io_reg_def_type at90pwm216_io_registers[];
extern gdb_io_reg_def_type at90pwm316_io_registers[];
extern gdb_io_reg_def_type at90usb1287_io_registers[];
extern gdb_io_reg_def_type at90usb162_io_registers[];
extern gdb_io_reg_def_type at90usb646_io_registers[];
extern gdb_io_reg_def_type at90usb647_io_registers[];
extern gdb_io_reg_def_type atmega1284p_io_registers[];
extern gdb_io_reg_def_type atmega165_io_registers[];
extern gdb_io_reg_def_type atmega165p_io_registers[];
extern gdb_io_reg_def_type atmega168p_io_registers[];
extern gdb_io_reg_def_type atmega16hva_io_registers[];
extern gdb_io_reg_def_type atmega3250p_io_registers[];
extern gdb_io_reg_def_type atmega325p_io_registers[];
extern gdb_io_reg_def_type atmega328p_io_registers[];
extern gdb_io_reg_def_type atmega3290p_io_registers[];
extern gdb_io_reg_def_type atmega329p_io_registers[];
extern gdb_io_reg_def_type atmega32hvb_io_registers[];
extern gdb_io_reg_def_type atmega32u4_io_registers[];
extern gdb_io_reg_def_type atmega406_io_registers[];
extern gdb_io_reg_def_type atmega48p_io_registers[];
extern gdb_io_reg_def_type atmega3208_io_registers[];
extern gdb_io_reg_def_type atmega4808_io_registers[];
extern gdb_io_reg_def_type atmega4809_io_registers[];
extern gdb_io_reg_def_type atmega644p_io_registers[];
extern gdb_io_reg_def_type atmega88p_io_registers[];
extern gdb_io_reg_def_type attiny167_io_registers[];
extern gdb_io_reg_def_type attiny43u_io_registers[];
extern gdb_io_reg_def_type attiny48_io_registers[];
extern gdb_io_reg_def_type attiny88_io_registers[];
extern gdb_io_reg_def_type atmega128rfa1_io_registers[];
extern gdb_io_reg_def_type atmega256rfr2_io_registers[];
extern gdb_io_reg_def_type atxmega16a4u_io_registers[];

#endif /* INCLUDE_IOREG_H */
