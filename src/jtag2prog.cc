/*
 *	avarice - The "avarice" program.
 *	Copyright (C) 2001 Scott Finneran
 *      Copyright (C) 2002 Intel Corporation
 *	Copyright (C) 2005,2006 Joerg Wunsch
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
 * This file implements the target memory programming interface for
 * the mkII protocol.
 */

#include <cstdio>

#include "jtag2.h"

void Jtag2::enableProgramming() {
    if (proto != Debugproto::DW) {
        programmingEnabled = true;
        doSimpleJtagCommand(CMND_ENTER_PROGMODE);
    }
}

void Jtag2::disableProgramming() {
    if (proto != Debugproto::DW) {
        programmingEnabled = false;
        doSimpleJtagCommand(CMND_LEAVE_PROGMODE);
    }
}

// This is really a chip-erase which erases flash, lock-bits and eeprom
// (unless the save-eeprom fuse is set).
void Jtag2::eraseProgramMemory() {
    if (proto == Debugproto::DW)
        // debugWIRE auto-erases when programming
        return;

    if (is_xmega) {
        const uchar command[6] = {CMND_XMEGA_ERASE,
        0x00, // ERASE_MODE (erase chip)
        0x00,0x00,0x00,0x00 // ADDRESS
        };

        uchar *response;
        int respSize;
        try {
            doJtagCommand(command, sizeof(command), response, respSize);
        } catch (jtag_exception &e) {
            fprintf(stderr, "Failed to erase Xmega program memory: %s\n", e.what());
            throw;
        }
        delete[] response;
    } else {
        doSimpleJtagCommand(CMND_CHIP_ERASE);
    }
}
