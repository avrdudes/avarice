/*
 *	avarice - The "avarice" program.
 *	Copyright (C) 2012 Joerg Wunsch
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
 * This file implements access to the target memory for the JTAGICE3 protocol.
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
#include "jtag3.h"
#include "remote.h"


/** Return the memory space code for the memory space indicated by the
    high-order bits of 'addr'. Also clear these high order bits in 'addr'
**/
uchar jtag3::memorySpace(unsigned long &addr)
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
	if (is_xmega || proto == PROTO_UPDI)
	    return MTYPE_XMEGA_APP_FLASH;
	else if (proto == PROTO_DW || programmingEnabled)
	    return MTYPE_FLASH_PAGE;
	else
	    return MTYPE_SPM;
    }
}

uchar *jtag3::jtagRead(unsigned long addr, unsigned int numBytes)
{
    uchar *response;
    int responsesize;

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
	unsigned int alignSize = 0;
    bool wasProgmode = programmingEnabled;
    if (needProgmode && !programmingEnabled)
       enableProgramming();

    unsigned char *cachePtr = NULL;
    unsigned int *cacheBaseAddr = NULL;

    switch (whichSpace)
    {
	case MTYPE_SIGN_JTAG:
	// The signature needs to be read from its exact address,
	// not just the signature space with address 0.
		if (proto == PROTO_UPDI)
			addr = 0x1100;
	break;
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

	case MTYPE_XMEGA_APP_FLASH:
		// Flash for UPDI devices can only be accessed in word (16bit) chunks
		if (proto == PROTO_UPDI)
			alignSize = 2;
	break;
    }

    uchar cmd[12];

    cmd[0] = SCOPE_AVR;
    cmd[1] = CMD3_READ_MEMORY;
    cmd[2] = 0;
    cmd[3] = whichSpace;

    if (pageSize > 0) {
	u32_to_b4(cmd + 8, pageSize);
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
		u32_to_b4(cmd + 4, pageAddr);
                try
                {
                    doJtagCommand(cmd, sizeof cmd, "read memory", resp, responsesize);
                }
                catch (jtag_exception& e)
                {
                    fprintf(stderr, "Failed to read target memory space: %s\n",
                            e.what());
                    delete [] response;
                    throw;
                }
		memcpy(cachePtr, resp + 3, pageSize);
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
		unsigned int addrAdj = addr;
		unsigned int numBytesAdj = numBytes;
		// Access to some memories may only be allowed in chunks aligned
		// to a certain size. If that is the case, properly align what is
		// read from the device (thus reading more than asked), then only
		// return to GDB the requested data.
		if (alignSize != 0)
		{
			// Get address adjustment by rounding down to alignment size
			// e.g. For alignmentSize of 10
			// 39 becomes 30 => adjustment of 9
			addrAdj = (addr / alignSize) * alignSize;
			// Get length adjustment by rounding up to number of bytes
			// e.g. For alignmentSize of 10
			// 39 becomes 40 => adjustment of 1
			numBytesAdj = ((numBytes + alignSize - 1) / alignSize) * alignSize;

			if (addr + numBytes > addrAdj + numBytesAdj)
			{
				// If both address and length are not aligned in request, our
				// alignment may make us read less than requested range.
				// Add alignmentSize to correct for this.
				// e.g. For alignmentSize of 10
				// 39 bytes from address 39 => bytes #39-77 requested
				// but 40 bytes from address 30 => bytes #30-69 read.
				// Adding 10 => bytes #30-79 read which covers expected range
				numBytesAdj += alignSize;
			}
		}
	u32_to_b4(cmd + 8, numBytesAdj);
	u32_to_b4(cmd + 4, addrAdj);

        int cnt = 0;
      again:
	try
        {
            doJtagCommand(cmd, sizeof cmd, "read memory", response, responsesize);
        }
        catch (jtag_io_exception& e)
        {
            cnt++;
            if (e.get_response() == RSP3_FAIL_WRONG_MODE &&
                cnt < 2)
            {
                interruptProgram();
                goto again;
            }
            fprintf(stderr, "Failed to read target memory space: %s\n",
                    e.what());
            throw;
        }
        catch (jtag_exception& e)
        {
            fprintf(stderr, "Failed to read target memory space: %s\n",
                    e.what());
            throw;
        }
	responsesize -= (addr - addrAdj) + (numBytesAdj - numBytes);
	offset = (addr - addrAdj);
	if (offset > 0)
	    memmove(response, response + 3 + offset, responsesize - 1 - offset);
	else
	    memmove(response, response + 3, responsesize - 1);
    }

    if (needProgmode && !wasProgmode)
       disableProgramming();

    return response;
}

void jtag3::jtagWrite(unsigned long addr, unsigned int numBytes, uchar buffer[])
{
    if (numBytes == 0)
	return;

    debugOut("jtagWrite ");
    uchar whichSpace = memorySpace(addr);

    // Hack to detect the start of a GDB "load" command.  Iff this
    // address is tied to flash ROM, and it is address 0, and the size
    // is larger than 4 bytes, assume it's the first block of a "load"
    // command.  If so, chip erase the device, and switch over to
    // programming mode to speed up things (drastically).

    if (proto != PROTO_DW &&
	whichSpace == MTYPE_SPM &&
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
	if (numBytes != pageSize)
	    throw ("jtagWrite(): numByte does not match page size");
    }
    uchar cmd[14 + numBytes];

    cmd[0] = SCOPE_AVR;
    cmd[1] = CMD3_WRITE_MEMORY;
    cmd[2] = 0;
    cmd[3] = whichSpace;
    if (pageSize) {
	u32_to_b4(cmd + 8, pageSize);
	u32_to_b4(cmd + 4, addr);
    } else {
	u32_to_b4(cmd + 8, numBytes);
	u32_to_b4(cmd + 4, addr);
    }
    cmd[12] = 0;
    memcpy(cmd + 13, buffer, numBytes);

    uchar *response;
    int responsesize;

    try
    {
        doJtagCommand(cmd, 14 + numBytes, "write memory", response, responsesize);
    }
    catch (jtag_exception& e)
    {
        fprintf(stderr, "Failed to write target memory space: %s\n",
                e.what());
        throw;
    }
    delete [] response;

    if (needProgmode && !wasProgmode)
       disableProgramming();
}
