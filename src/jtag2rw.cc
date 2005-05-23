/*
 *	avarice - The "avarice" program.
 *	Copyright (C) 2001 Scott Finneran
 *	Copyright (C) 2002, 2003, 2004 Intel Corporation
 *	Copyright (C) 2005 Joerg Wunsch
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License Version 2
 *	as published by the Free Software Foundation.
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
 * This file implements access to the target memory for the mkII protocol.
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
#include <assert.h>

#include "avarice.h"
#include "jtag.h"
#include "jtag2.h"
#include "remote.h"


/** Return the memory space code for the memory space indicated by the
    high-order bits of 'addr'. Also clear these high order bits in 'addr'
**/
uchar jtag2::memorySpace(unsigned long &addr)
{
    int mask;

    // We can't just mask the bits off, because 0x10000->0x1ffff are
    // valid code addresses
    if (addr & DATA_SPACE_ADDR_OFFSET)
    {
	mask = addr & ADDR_SPACE_MASK;
	addr &= ~ADDR_SPACE_MASK;
    }
    else
	mask = 0;

    switch (mask)
    {
    case EEPROM_SPACE_ADDR_OFFSET:
	if (programmingEnabled)
	    return MTYPE_EEPROM_PAGE;
	else
	    return MTYPE_EEPROM;
    case FUSE_SPACE_ADDR_OFFSET:
	return MTYPE_FUSE_BITS;
    case LOCK_SPACE_ADDR_OFFSET:
	return MTYPE_LOCK_BITS;
    case SIG_SPACE_ADDR_OFFSET:
	return MTYPE_SIGN_JTAG;
      // ... return MTYPE_OSCCAL_BYTE;
      // ... return MTYPE_CAN;
    case BREAKPOINT_SPACE_ADDR_OFFSET:
	return MTYPE_EVENT;
    case DATA_SPACE_ADDR_OFFSET:
	return MTYPE_SRAM;
    default:
	if (programmingEnabled)
	    return MTYPE_FLASH_PAGE;
	else
	    return MTYPE_SPM;
    }
}

uchar *jtag2::jtagRead(unsigned long addr, unsigned int numBytes)
{
    uchar *response;
    int responseSize;

    if (numBytes == 0)
    {
	response = new uchar[1];
	response[0] = '\0';
	return response;
    }

    debugOut("jtagRead ");
    uchar whichSpace = memorySpace(addr);
    bool needProgmode = whichSpace >= MTYPE_FLASH_PAGE;
    unsigned int pageSize = 0;
    bool wasProgmode = programmingEnabled;
    if (needProgmode && !programmingEnabled)
       enableProgramming();

    switch (whichSpace)
    {
	// Pad to even byte count for flash memory.
	// Even MTYPE_SPM appears to cause a RSP_FAILED
	// otherwise.
    case MTYPE_SPM:
	numBytes = (numBytes + 1) & ~1;
	break;

    case MTYPE_FLASH_PAGE:
	numBytes = (numBytes + 1) & ~1;
	pageSize = global_p_device_def->flash_page_size;
	break;

    case MTYPE_EEPROM_PAGE:
	pageSize = global_p_device_def->eeprom_page_size;
	break;
    }
    if (pageSize > 0) {
	unsigned int mask = pageSize - 1;
	addr &= ~mask;
	check(numBytes == pageSize,
	      "jtagRead(): numByte does not match page size");
    }
    uchar command[10] = { CMND_READ_MEMORY };
    command[1] = whichSpace;
    if (pageSize) {
	u32_to_b4(command + 2, pageSize);
	u32_to_b4(command + 6, addr);
    } else {
	u32_to_b4(command + 2, numBytes);
	u32_to_b4(command + 6, addr);
    }

    check(doJtagCommand(command, sizeof command, response, responseSize),
	  "Failed to read target memory space");

    if (needProgmode && !wasProgmode)
       disableProgramming();

    memmove(response, response + 1, responseSize - 1);
    return response;
}

bool jtag2::jtagWrite(unsigned long addr, unsigned int numBytes, uchar buffer[])
{
    if (numBytes == 0)
	return true;

    debugOut("jtagWrite ");
    uchar whichSpace = memorySpace(addr);
    bool needProgmode = whichSpace >= MTYPE_FLASH_PAGE;
    unsigned int pageSize = 0;
    bool wasProgmode = programmingEnabled;
    if (needProgmode && !programmingEnabled)
       enableProgramming();

    switch (whichSpace)
    {
    case MTYPE_FLASH_PAGE:
	pageSize = global_p_device_def->flash_page_size;
	break;

    case MTYPE_EEPROM_PAGE:
	pageSize = global_p_device_def->eeprom_page_size;
	break;
    }
    if (pageSize > 0) {
	unsigned int mask = pageSize - 1;
	addr &= ~mask;
	check(numBytes == pageSize,
	      "jtagWrite(): numByte does not match page size");
    }
    uchar *command = new uchar [10 + numBytes];
    command[0] = CMND_WRITE_MEMORY;
    command[1] = whichSpace;
    if (pageSize) {
	u32_to_b4(command + 2, pageSize);
	u32_to_b4(command + 6, addr);
    } else {
	u32_to_b4(command + 2, numBytes);
	u32_to_b4(command + 6, addr);
    }
    memcpy(command + 10, buffer, numBytes);

    uchar *response;
    int responseSize;

    check(doJtagCommand(command, 10 + numBytes, response, responseSize),
	  "Failed to write target memory space");
    delete [] command;
    delete [] response;

    if (needProgmode && !wasProgmode)
       disableProgramming();

    return true;
}
