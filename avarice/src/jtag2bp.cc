/*
 *	avarice - The "avarice" program.
 *	Copyright (C) 2001 Scott Finneran
 *      Copyright (C) 2002 Intel Corporation
 *	Copyright (C) 2005,2006 Joerg Wunsch
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
 * This file contains the breakpoint handling for the mkII protocol.
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

#include "avarice.h"
#include "jtag.h"
#include "jtag2.h"

bool jtag2::codeBreakpointAt(unsigned int address)
{
  address /= 2;
  for (int i = 0; i < numBreakpointsCode; i++)
    if (bpCode[i].address == address)
      return true;
  return false;
}

bool jtag2::codeBreakpointBetween(unsigned int start, unsigned int end)
{
  start /= 2; end /= 2;
  for (int i = 0; i < numBreakpointsCode; i++)
    if (bpCode[i].address >= start && bpCode[i].address < end)
      return true;
  return false;
}

void jtag2::deleteAllBreakpoints(void)
{
    numBreakpointsData = numBreakpointsCode = 0;
}

bool jtag2::stopAt(unsigned int address)
{
    uchar one = 1;
    jtagWrite(BREAKPOINT_SPACE_ADDR_OFFSET + address / 2, 1, &one);
}

bool jtag2::addBreakpoint(unsigned int address, bpType type, unsigned int length)
{
    breakpoint2 *bp;

    debugOut("BP ADD type: %d  addr: 0x%x ", type, address);

    // Respect overall breakpoint limit
    if (numBreakpointsCode + numBreakpointsData == MAX_BREAKPOINTS2)
	// XXX fit software BPs here
	return false;

    // There's a spare breakpoint, is there one of the appropriate type
    // available?
    if (type == CODE)
    {
	if (numBreakpointsCode == MAX_BREAKPOINTS2_CODE)
	{
	    debugOut("FAILED\n");
	    return false;
	}

	// The JTAG box sees program memory as 16-bit wide locations. GDB
	// sees bytes. As such, halve the breakpoint address.
	address /= 2;

	bp = &bpCode[numBreakpointsCode++];
    }
    else // data breakpoint
    {
	// mask out the GCC offset
	(void)memorySpace((unsigned long &)address);

	if (numBreakpointsData == MAX_BREAKPOINTS2_DATA)
	{
	    debugOut("FAILED\n");
	    return false;
	}

	if (useDebugWire)
	{
	    debugOut("Data breakpoints not supported in debugWire\n");
	    return false;
	}

	// Range breakpoints allocate two slots (i.e. all available
	// data BP slots).
	if (length > 1 &&
	    (numBreakpointsData + 2 > MAX_BREAKPOINTS2_DATA ||
	     numBreakpointsCode + numBreakpointsData + 2 > MAX_BREAKPOINTS2))
	{
	    debugOut("FAILED (range BP needs 2 slots)\n");
	    return false;
	}
	// Range breakpoint needs to be aligned, and the length must
	// be representable as a bitmask.
	if (length > 1)
	{
	    int bitno = ffs((int)length);
	    unsigned int mask = 1 << (bitno - 1);
	    if (mask != length)
	    {
		debugOut("FAILED: length not power of 2 in range BP\n");
		return false;
	    }
	    mask--;
	    if ((address & mask) != 0)
	    {
		debugOut("FAILED: address in range BP is not base-aligned\n");
		return false;
	    }
	    mask = ~mask;

	    bp = &bpData[numBreakpointsData++];
	    bp->address = address;
	    bp->type = (bpType)(type | HAS_MASK);
	    bp = &bpData[numBreakpointsData++];
	    bp->address = mask;
	    bp->type = DATA_MASK;

	    debugOut("range BP ADDED: 0x%x/0x%x\n", address, mask);
	    return true;
	}

	bp = &bpData[numBreakpointsData++];
    }

    bp->address = address;
    bp->type = type;

    debugOut(" ADDED\n");
    return true;
}


bool jtag2::deleteBreakpoint(unsigned int address, bpType type, unsigned int length)
{
    breakpoint2 *bp;
    int *numBp;

    debugOut("BP DEL type: %d  addr: 0x%x ", type, address);

    if (type == CODE)
    {
	bp = bpCode;
	numBp = &numBreakpointsCode;
	// The JTAG box sees program memory as 16-bit wide locations. GDB
	// sees bytes. As such, halve the breakpoint address.
	address /= 2;
    }
    else // data
    {
	// mask out the GCC offset
	(void)memorySpace((unsigned long &)address);

	bp = bpData;
	numBp = &numBreakpointsData;
    }

    // Find and squash the removed breakpoint
    for (int i = 0; i < *numBp; i++)
    {
	if (bp[i].type == (type | HAS_MASK) && bp[i].address == address)
	{
	    debugOut("range BP %d REMOVED\n", i);
	    (*numBp) -= 2;
            /*
             * There is never something to move here.
             */
            /*
            if ((*numBp - i) >= 1)
                memmove(&bp[i], &bp[i + 2], (*numBp - i) * sizeof(breakpoint2));
            */
	    return true;
	}
	if (bp[i].type == type && bp[i].address == address)
	{
	    (*numBp)--;
	    debugOut("REMOVED %d, remaining: %d\n", i, *numBp);
            if ((*numBp - i) >= 1)
                memmove(&bp[i], &bp[i + 1], (*numBp - i) * sizeof(breakpoint2));
	    return true;
	}
    }
    debugOut("FAILED\n");
    return false;
}

// This has to be completely rewritten to generally allow for soft
// BPs.  By now, just throw in a special implementation for debugWire.
// That way, there's at least *some* BP handling for dW, even though
// we still artificially limit it to four BPs.
void jtag2::updateBreakpintsDW(void)
{
    debugOut("updateBreakpointsDW\n");

    // First, catch any breakpoints that do no longer persist, and
    // delete them from the ICE.  Those that are still the same
    // are marked in the scoreboard so we don't re-insert them.

    bool scoreboard[MAX_BREAKPOINTS2] = { false, false, false, false };
    breakpoint2 newcache[MAX_BREAKPOINTS2];
    int newcacheidx, i, slot;

    for (slot = newcacheidx = 0; slot < numBreakpointsCode; slot++)
    {
	for (i = 0; i < MAX_BREAKPOINTS2; i++)
	{
	    if (softBPcache[i].type == CODE &&
		softBPcache[i].address == bpCode[slot].address)
	    {
		// This one is still active.
		scoreboard[slot] = true;
		newcache[newcacheidx++] = bpCode[slot];

		// Delete it from the old cache.
		softBPcache[i].type = NONE;

		break;
	    }
	}
    }

    // At this point, the old cache consists solely of those BPs that
    // are to be deleted.
    for (i = 0; i < MAX_BREAKPOINTS2; i++)
    {
	if (softBPcache[i].type != CODE)
	    continue;

	uchar cmd[6] = { CMND_CLR_BREAK };
	cmd[1] = 0;
	u32_to_b4(cmd + 2, softBPcache[i].address);

	uchar *response;
	int responseSize;
	if(!doJtagCommand(cmd, 6, response, responseSize))
	    check(false, "Failed to clear breakpoint");

	delete [] response;
    }

    // Now go ahead, and insert all new BPs.
    for (slot = 0; slot < numBreakpointsCode; slot++)
    {
	if (scoreboard[slot])
	    // It's still there in the ICE.
	    continue;

	uchar cmd[8] = { CMND_SET_BREAK };
	cmd[1] = 0x01;
	cmd[2] = 0;
	u32_to_b4(cmd + 3, bpCode[slot].address);
	cmd[7] = 0x03;

	uchar *response;
	int responseSize;
	check(doJtagCommand(cmd, 8, response, responseSize),
	      "Failed to set breakpoint");
	delete [] response;

	newcache[newcacheidx++] = bpCode[slot];
    }

    // Finally, update the cache.
    while (newcacheidx < MAX_BREAKPOINTS2)
	newcache[newcacheidx++].type = NONE;

    for (i = 0; i < MAX_BREAKPOINTS2; i++)
	softBPcache[i] = newcache[i];
}


void jtag2::updateBreakpoints(void)
{
    int slot;
    int bpD = numBreakpointsData;
    int bpC = numBreakpointsCode;

    if (useDebugWire)
    {
	updateBreakpintsDW();
	return;
    }

    debugOut("updateBreakpoints\n");
    haveHiddenBreakpoint = false;

    for (slot = MAX_BREAKPOINTS2 - 1; slot >= 0; slot--)
    {
        if (bpD > 0)
        {
            // There are data breakpoints left, handle them.
            --bpD;
            breakpoint2 *bp = bpData + bpD;
            unsigned char bpMode, bp_Type;
            bool has_mask = (bpData[bpD].type & HAS_MASK) != 0;

            switch (bpData[bpD].type)
            {
            case READ_DATA:
            case READ_DATA | HAS_MASK:
                bpMode = 0x00;
                bp_Type = 0x02;
                break;
            case WRITE_DATA:
            case WRITE_DATA | HAS_MASK:
                bpMode = 0x01;
                bp_Type = 0x02;
                break;
            case ACCESS_DATA:
            case ACCESS_DATA | HAS_MASK:
                bpMode = 0x02;
                bp_Type = 0x02;
                break;
            case DATA_MASK:
                check(slot == BREAKPOINT2_DATA_MASK,
                      "Invalid BP slot for data mask");
                bpMode = 0x00;
                bp_Type = 0x03;
                break;
            case NONE:
            default:
                check(false, "Invalid bp mode (for data bp)");
                bpMode = 0;		// supress warning
                bp_Type = 0;
                break;
            }

            uchar cmd[8] = { CMND_SET_BREAK };
            cmd[1] = bp_Type;
            cmd[2] = slot;
            u32_to_b4(cmd + 3, bpData[bpD].address);
            cmd[7] = bpMode;

            uchar *response;
            int responseSize;
            check(doJtagCommand(cmd, 8, response, responseSize),
                  "Failed to set breakpoint");
            delete [] response;

            continue;
        }

        if (bpC > 0)
        {
            // Handle code breakpoints
            --bpC;
            breakpoint2 *bp = bpCode + bpC;
            unsigned char bpMode, bp_Type;

            switch (bp->type)
            {
            case CODE:
                bpMode = 0x03;
                bp_Type = 0x01;
                break;
            case NONE:
            default:
                check(false, "Invalid bp mode (for code bp)");
                bpMode = 0;		// supress warning
                bp_Type = 0;
                break;
            }

            if (slot == 0)
            {
                haveHiddenBreakpoint = true;

                doSimpleJtagCommand(CMND_CLEAR_EVENTS);

                unsigned int off = bp->address / 8;
                uchar *command = new uchar [10 + off + 1];
                command[0] = CMND_WRITE_MEMORY;
                command[1] = MTYPE_EVENT_COMPRESSED;
                u32_to_b4(command + 2, off + 1);
                u32_to_b4(command + 6, 0);
                memset(command + 10, 0, off);
                command[10 + off] = 1 << (bp->address % 8);

                uchar *response;
                int responseSize;

                check(doJtagCommand(command, 10 + off + 1,
                                    response, responseSize),
                      "Failed to write target memory space");
                delete [] command;
                delete [] response;
            }
            else
            {
                uchar cmd[8] = { CMND_SET_BREAK };
                cmd[1] = bp_Type;
                cmd[2] = slot;
                u32_to_b4(cmd + 3, bp->address);
                cmd[7] = bpMode;

                uchar *response;
                int responseSize;
                check(doJtagCommand(cmd, 8, response, responseSize),
                      "Failed to set breakpoint");
                delete [] response;
            }
            continue;
        }

        // If there is anything left, clear out the BP.
        if (slot > 0)
        {
            uchar cmd[6] = { CMND_CLR_BREAK };
            cmd[1] = slot;
            u32_to_b4(cmd + 2, /* address? */ 0);

            uchar *response;
            int responseSize;
            if(!doJtagCommand(cmd, 6, response, responseSize))
                check(false, "Failed to clear breakpoint");

            delete [] response;
        }
    }
}
