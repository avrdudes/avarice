/*
 *	avarice - The "avarice" program.
 *	Copyright (C) 2003 James Harris
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
 * io_regsiter' command.
 */

#ifndef INCLUDE_IOREG_H
#define INCLUDE_IOREG_H

#define IO_REG_RSE    0x01    // IO register has read side effect

typedef struct {
    const char* name;
    const unsigned char reg_addr;
    const unsigned char flags;
} gdb_io_reg_def_type;

extern gdb_io_reg_def_type atmega16_io_registers[];

#endif /* INCLUDE_IOREG_H */
