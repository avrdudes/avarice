/*
 *	avarice - The "avarice" program.
 *	Copyright (C) 2001 Scott Finneran
 *      Copyright (C) 2002 Intel Corporation
 *	Copyright (C) 2005 Joerg Wunsch
 *
 *      File format support using BFD contributed and copyright 2003
 *      Nils Kr. Strom
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
 * This file contains functions for interfacing with the JTAG box.
 *
 * $Id$
 */

#include <cstdio>

#include "jtag1.h"

void jtag1::enableProgramming() {
    programmingEnabled = true;
    if (!doSimpleJtagCommand(0xa3, 1)) {
        fprintf(stderr, "JTAG ICE: Failed to enable programming\n");
        throw jtag_exception();
    }
}

void jtag1::disableProgramming() {
    programmingEnabled = false;
    if (!doSimpleJtagCommand(0xa4, 1)) {
        fprintf(stderr, "JTAG ICE: Failed to disable programming\n");
        throw jtag_exception();
    }
}

// This is really a chip-erase which erases flash, lock-bits and eeprom
// (unless the save-eeprom fuse is set).
void jtag1::eraseProgramMemory() {
    if (!doSimpleJtagCommand(0xa5, 1)) {
        fprintf(stderr, "JTAG ICE: Failed to erase program memory\n");
        throw jtag_exception();
    }
}
