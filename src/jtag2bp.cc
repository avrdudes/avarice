/*
 *	avarice - The "avarice" program.
 *	Copyright (C) 2001 Scott Finneran
 *      Copyright (C) 2002 Intel Corporation
 *	Copyright (C) 2005 Joerg Wunsch
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
    breakpoint *bp;

    debugOut("BP ADD type: %d  addr: 0x%x ", type, address);

    // Respect overall breakpoint limit
    if (numBreakpointsCode + numBreakpointsData == MAX_BREAKPOINTS)
      return false;

    // There's a spare breakpoint, is there one of the appropriate type
    // available?
    if (type == CODE)
    {
	if (numBreakpointsCode == MAX_BREAKPOINTS_CODE)
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
	if (numBreakpointsData == MAX_BREAKPOINTS_DATA)
	{
	    debugOut("FAILED\n");
	    return false;
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
    breakpoint *bp;
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
	bp = bpData;
	numBp = &numBreakpointsData;
    }

    // Find and squash the removed breakpoint
    for (int i = 0; i < *numBp; i++)
    {
	if (bp[i].type == type && bp[i].address == address)
	{
	    debugOut("REMOVED %d\n", i);
	    (*numBp)--;
	    memmove(&bp[i], &bp[i + 1], (*numBp - i) * sizeof(breakpoint));
	    return true;
	}
    }
    debugOut("FAILED\n");
    return false;
}


void jtag2::updateBreakpoints(bool setCodeBreakpoints)
{
    unsigned char bpMode;
    int bpC = 0, bpD = 0;
    breakpoint *bp;

    debugOut("updateBreakpoints\n");

    for (bpC = bpD = 0; bpC < numBreakpointsCode + numBreakpointsData; bpC++)
    {
	if (bpC < numBreakpointsCode) {
	    if (!setCodeBreakpoints)
		continue;
	    bp = bpCode + bpC;
	} else {
	    bp = bpData + bpD;
	    bpD++;
	}

	uchar bpType = 0x02;
	switch (bp->type)
	{
	case READ_DATA:
	    bpMode = 0x00;
	    break;
	case WRITE_DATA:
	    bpMode = 0x01;
	    break;
	case ACCESS_DATA:
	    bpMode = 0x02;
	    break;
	case CODE:
	    bpMode = 0x03;
	    bpType = 0x01;
	    break;
        case NONE:
        default:
	    check(false, "Invalid bp mode");
	    bpMode = 0;		// supress warning
            break;
	}

	uchar cmd[8] = { CMND_SET_BREAK };
	cmd[1] = bpType;
	cmd[2] = bpC + 1;
	u32_to_b4(cmd + 3, bp->address);
	cmd[7] = bpMode;

	uchar *response;
	int responseSize;
	check(doJtagCommand(cmd, 8, response, responseSize),
	      "Failed to set breakpoint");
	delete [] response;
    }
    // clear remaining slots
    for (; bpC < MAX_BREAKPOINTS; bpC++)
    {
	uchar cmd[6] = { CMND_CLR_BREAK };
	cmd[1] = bpC + 1;
	u32_to_b4(cmd + 2, /* bp->address? */ 0);

	uchar *response;
	int responseSize;
	if(!doJtagCommand(cmd, 6, response, responseSize))
	{
	    // Getting an illegal breakpoint response is OK
	    // to us; it means that bp had never been set.
	    check(response[0] == RSP_ILLEGAL_BREAKPOINT,
		  "Failed to clear breakpoint");
	}
	delete [] response;
    }
}

void jtag2::breakOnChangeFlow(void)
{
    uchar one;

    setJtagParameter(PAR_BREAK_ON_CHANGE_FLOW, &one, 1);
}
