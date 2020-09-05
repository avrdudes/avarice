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


#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>

#include "avarice.h"
#include "jtag.h"
#include "jtag3.h"


void jtag3::enableProgramming(void)
{
    if (proto != PROTO_DW)
    {
	programmingEnabled = true;
	doSimpleJtagCommand(CMD3_ENTER_PROGMODE, "enter progmode");
    }
}


void jtag3::disableProgramming(void)
{
    if (proto != PROTO_DW)
    {
	programmingEnabled = false;
	doSimpleJtagCommand(CMD3_LEAVE_PROGMODE, "leave progmode");
    }
}


// This is really a chip-erase which erases flash, lock-bits and eeprom
// (unless the save-eeprom fuse is set).
void jtag3::eraseProgramMemory(void)
{
    if (proto == PROTO_DW)
        // debugWIRE auto-erases when programming
        return;

    uchar *resp;
    int respsize;
    uchar buf[8];

    buf[0] = SCOPE_AVR;
    buf[1] = CMD3_ERASE_MEMORY;
    buf[2] = 0;
    buf[3] = XMEGA_ERASE_CHIP;
    buf[4] = buf[5] = buf[6] = buf[7] = 0; /* page address */

    doJtagCommand(buf, 8, "chip erase", resp, respsize);

    delete [] resp;
}

void jtag3::eraseProgramPage(unsigned long address)
{
    uchar *resp;
    int respsize;
    uchar buf[8];

    buf[0] = SCOPE_AVR;
    buf[1] = CMD3_ERASE_MEMORY;
    buf[2] = 0;
    if (is_xmega && address >= appsize)
    {
        buf[3] = XMEGA_ERASE_BOOT_PAGE;
        address -= appsize;
    }
    else
    {
        buf[3] = XMEGA_ERASE_APP_PAGE;
    }
    u32_to_b4(buf + 4, address);

    doJtagCommand(buf, 8, "page erase", resp, respsize);

    delete [] resp;
}


void jtag3::downloadToTarget(const char* filename __unused, bool program __unused, bool verify __unused)
{
    statusOut("\nDownload not done.\n");
    throw jtag_exception("Target programming not implemented for JTAGICE3");
}
