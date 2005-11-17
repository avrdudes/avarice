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
 * This file implements target execution handling for the mkII protocol.
 *
 * $Id$
 */


#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>

#include "avarice.h"
#include "jtag.h"
#include "jtag2.h"
#include "remote.h"

unsigned long jtag2::getProgramCounter(void)
{
    uchar *response;
    int responseSize;
    uchar command[] = { CMND_READ_PC };

    check(doJtagCommand(command, sizeof(command), response, responseSize),
	  "cannot read program counter");
    unsigned long result = b4_to_u32(response + 1);
    delete [] response;

    // The JTAG box sees program memory as 16-bit wide locations. GDB
    // sees bytes. As such, double the PC value.
    result *= 2;

    return result;
}

bool jtag2::setProgramCounter(unsigned long pc)
{
    uchar *response;
    int responseSize;
    uchar command[5] = { CMND_WRITE_PC };

    u32_to_b4(command + 1, pc / 2);

    check(doJtagCommand(command, sizeof(command), response, responseSize),
	  "cannot write program counter");

    delete [] response;

    return true;
}

bool jtag2::resetProgram(void)
{
    uchar cmd[2] = { CMND_RESET, 0x01 };
    uchar *resp;
    int respSize;

    bool rv = doJtagCommand(cmd, 2, resp, respSize);
    delete [] resp;

    return rv;
}

bool jtag2::interruptProgram(void)
{
    uchar cmd[2] = { CMND_FORCED_STOP, 0x01 };
    uchar *resp;
    int respSize;

    bool rv = doJtagCommand(cmd, 2, resp, respSize);
    delete [] resp;

    return rv;
}

bool jtag2::resumeProgram(void)
{
    doSimpleJtagCommand(CMND_GO);

    return true;
}

bool jtag2::jtagSingleStep(bool useHLL)
{
    uchar cmd[3] = { CMND_SINGLE_STEP,
		     useHLL? 0x02: 0x01,
		     useHLL? 0x00: 0x01 };
    uchar *resp;
    int respSize, i = 2;
    bool rv;

    do
    {
	rv = doJtagCommand(cmd, 3, resp, respSize);
	uchar stat = resp[0];
	delete [] resp;

	if (rv)
	    break;

	if (stat != RSP_ILLEGAL_MCU_STATE)
	    break;
    }
    while (--i >= 0);

    return rv;
}

bool jtag2::jtagContinue(void)
{
    updateBreakpoints(); // download new bp configuration

    if (haveHiddenBreakpoint)
	// One of our breakpoints has been set as the high-level
	// language boundary address of our current statement, so
	// perform a high-level language single step.
	(void)jtagSingleStep(true);
    else
	doSimpleJtagCommand(CMND_GO);

    for (;;)
    {
	int maxfd;
	fd_set readfds;
	bool breakpoint = false, gdbInterrupt = false;

	// Now that we are "going", wait for either a response from the JTAG
	// box or a nudge from GDB.
	debugOut("Waiting for input.\n");

	// Check for input from JTAG ICE (breakpoint, sleep, info, power)
	// or gdb (user break)
	FD_ZERO (&readfds);
	FD_SET (gdbFileDescriptor, &readfds);
	FD_SET (jtagBox, &readfds);
	maxfd = jtagBox > gdbFileDescriptor ? jtagBox : gdbFileDescriptor;

	int numfds = select(maxfd + 1, &readfds, 0, 0, 0);
	unixCheck(numfds, "GDB/JTAG ICE communications failure");

	if (FD_ISSET(gdbFileDescriptor, &readfds))
	{
	    int c = getDebugChar();
	    if (c == 3) // interrupt
	    {
		debugOut("interrupted by GDB\n");
		gdbInterrupt = true;
	    }
	    else
		debugOut("Unexpected GDB input `%02x'\n", c);
	}

	if (FD_ISSET(jtagBox, &readfds))
	{
	    uchar *evtbuf;
	    int evtSize;
	    unsigned short seqno;
	    evtSize = recvFrame(evtbuf, seqno);
	    if (evtSize >= 0) {
		// XXX if not event, should push frame back into queue...
		// We really need a queue of received frames.
		if (seqno != 0xffff)
		    debugOut("Expected event packet, got other response");
		else if (evtbuf[8] == EVT_BREAK)
		    breakpoint = true;
		// Ignore other events.
		delete [] evtbuf;
	    }
	}

	// We give priority to user interrupts
	if (gdbInterrupt)
	    return false;
	if (breakpoint)
	    return true;
    }
}

