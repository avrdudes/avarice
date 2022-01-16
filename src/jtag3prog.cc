/*
 *	avarice - The "avarice" program.
 *	Copyright (C) 2012 Joerg Wunsch
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
 * the JTAGICE3 protocol.
 *
 * $Id$
 */

#include "jtag3.h"

void Jtag3::enableProgramming() {
    if (proto == Debugproto::DW)  return;

    programmingEnabled = true;
    doSimpleJtagCommand(CMD3_ENTER_PROGMODE, "enter progmode");
}

void Jtag3::disableProgramming() {
    if (proto == Debugproto::DW) return;

    programmingEnabled = false;
    doSimpleJtagCommand(CMD3_LEAVE_PROGMODE, "leave progmode");
}

// This is really a chip-erase which erases flash, lock-bits and eeprom
// (unless the save-eeprom fuse is set).
void Jtag3::eraseProgramMemory() {
    if (proto == Debugproto::DW)
        // debugWIRE auto-erases when programming
        return;

    const uchar buf[8] = {SCOPE_AVR, CMD3_ERASE_MEMORY, 0, XMEGA_ERASE_CHIP,
                          0, 0, 0, 0}; /* page address */

    uchar *resp;
    int respsize;

    doJtagCommand(buf, sizeof(buf), "chip erase", resp, respsize);

    delete[] resp;
}

void Jtag3::eraseProgramPage(const unsigned long address) {
    uchar buf[8];
    buf[0] = SCOPE_AVR;
    buf[1] = CMD3_ERASE_MEMORY;
    buf[2] = 0;
    if (is_xmega && address >= appsize) {
        buf[3] = XMEGA_ERASE_BOOT_PAGE;
        u32_to_b4(buf + 4, address - appsize);
    } else {
        buf[3] = XMEGA_ERASE_APP_PAGE;
        u32_to_b4(buf + 4, address);
    }

    uchar *resp;
    int respsize;

    doJtagCommand(buf, sizeof(buf), "page erase", resp, respsize);

    delete[] resp;
}
