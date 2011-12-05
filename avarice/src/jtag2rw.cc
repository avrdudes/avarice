/*
 *	avarice - The "avarice" program.
 *	Copyright (C) 2001 Scott Finneran
 *	Copyright (C) 2002, 2003, 2004 Intel Corporation
 *	Copyright (C) 2005,2006 Joerg Wunsch
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
	if (proto != PROTO_DW && programmingEnabled)
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
    case REGISTER_SPACE_ADDR_OFFSET:
	return MTYPE_XMEGA_REG;
    case DATA_SPACE_ADDR_OFFSET:
	return MTYPE_SRAM;
    default:
	if (proto == PROTO_DW || programmingEnabled)
	    return MTYPE_FLASH_PAGE;
	else if (is_xmega && has_full_xmega_support)
	    return MTYPE_XMEGA_APP_FLASH;
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
    bool needProgmode = whichSpace >= MTYPE_FLASH_PAGE &&
        whichSpace < MTYPE_XMEGA_REG;
    unsigned int pageSize = 0;
    unsigned int offset = 0;
    bool wasProgmode = programmingEnabled;
    if (needProgmode && !programmingEnabled)
       enableProgramming();

    unsigned char *cachePtr = NULL;
    unsigned int *cacheBaseAddr = NULL;

    switch (whichSpace)
    {
	// Pad to even byte count for flash memory.
	// Even MTYPE_SPM appears to cause a RSP_FAILED
	// otherwise.
    case MTYPE_SPM:
        offset = addr & 1;
        addr &= ~1;
	numBytes = (numBytes + 1) & ~1;
	break;

    case MTYPE_FLASH_PAGE:
	pageSize = deviceDef->flash_page_size;
	cachePtr = flashCache;
	cacheBaseAddr = &flashCachePageAddr;
	break;

    case MTYPE_EEPROM_PAGE:
	pageSize = deviceDef->eeprom_page_size;
	cachePtr = eepromCache;
	cacheBaseAddr = &eepromCachePageAddr;
	break;
    }

    uchar command[10] = { CMND_READ_MEMORY };
    command[1] = whichSpace;

    if (pageSize > 0) {
	u32_to_b4(command + 2, pageSize);
	response = new uchar[numBytes];

	unsigned int mask = pageSize - 1;
	unsigned int pageAddr = addr & ~mask;
	unsigned int chunksize = numBytes;
	unsigned int targetOffset = 0;

	if (addr + numBytes >= pageAddr + pageSize)
	    // Chunk would cross a page boundary, reduce it
	    // appropriately.
	    chunksize -= (addr + numBytes - (pageAddr + pageSize));
	offset = addr - pageAddr;

	while (numBytes > 0)
	{
	    uchar *resp;

	    if (pageAddr == *cacheBaseAddr)
	    {
		// quickly fetch from page cache
		memcpy(response + targetOffset,
		       cachePtr + offset,
		       chunksize);
	    }
	    else
	    {
		// read from device, cache result, and copy over our part
		u32_to_b4(command + 6, pageAddr);
		check(doJtagCommand(command, sizeof command, resp, responseSize, true),
		      "Failed to read target memory space");
		memcpy(cachePtr, resp + 1, pageSize);
		*cacheBaseAddr = pageAddr;
		memcpy(response + targetOffset,
		       cachePtr + offset,
		       chunksize);
		delete [] resp;
	    }

	    numBytes -= chunksize;
	    targetOffset += chunksize;

	    chunksize = numBytes > pageSize? pageSize: numBytes;
	    pageAddr += pageSize;
	}
    } else {
	u32_to_b4(command + 2, numBytes);
	u32_to_b4(command + 6, addr);

	check(doJtagCommand(command, sizeof command, response, responseSize, true),
	      "Failed to read target memory space");
	if (offset > 0)
	    memmove(response, response + 1 + offset, responseSize - 1 - offset);
	else
	    memmove(response, response + 1, responseSize - 1);
    }

    if (needProgmode && !wasProgmode)
       disableProgramming();

    return response;
}

bool jtag2::jtagWrite(unsigned long addr, unsigned int numBytes, uchar buffer[])
{
    if (numBytes == 0)
	return true;

    debugOut("jtagWrite ");
    uchar whichSpace = memorySpace(addr);

    // Hack to detect the start of a GDB "load" command.  Iff this
    // address is tied to flash ROM, and it is address 0, and the size
    // is larger than 4 bytes, assume it's the first block of a "load"
    // command.  If so, chip erase the device, and switch over to
    // programming mode to speed up things (drastically).

    if (whichSpace == MTYPE_SPM &&
	addr == 0 &&
	numBytes > 4)
    {
	debugOut("Detected GDB \"load\" command, erasing flash.\n");
	//whichSpace = MTYPE_FLASH_PAGE; // this will turn on progmode
	eraseProgramMemory();
    }

    bool needProgmode = whichSpace >= MTYPE_FLASH_PAGE &&
        whichSpace < MTYPE_XMEGA_REG;
    unsigned int pageSize = 0;
    bool wasProgmode = programmingEnabled;
    if (needProgmode && !programmingEnabled)
       enableProgramming();

    switch (whichSpace)
    {
    case MTYPE_FLASH_PAGE:
	pageSize = deviceDef->flash_page_size;
	break;

    case MTYPE_EEPROM_PAGE:
	pageSize = deviceDef->eeprom_page_size;
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
