/*
 *	avarice - The "avarice" program.
 *	Copyright (C) 2001 Scott Finneran
 *      Copyright (C) 2002 Intel Corporation
 *	Copyright (C) 2005, 2007 Joerg Wunsch
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


#include <ctype.h>
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
    if (cached_pc_is_valid)
        return cached_pc;

    uchar *response;
    int responseSize;
    uchar command[] = { CMND_READ_PC };

    check(doJtagCommand(command, sizeof(command), response, responseSize, true),
	  "cannot read program counter");
    unsigned long result = b4_to_u32(response + 1);
    delete [] response;

    // The JTAG box sees program memory as 16-bit wide locations. GDB
    // sees bytes. As such, double the PC value.
    result *= 2;

    cached_pc_is_valid = true;
    return cached_pc = result;
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

    cached_pc_is_valid = false;

    return true;
}

PRAGMA_DIAG_PUSH
PRAGMA_DIAG_IGNORED("-Wunused-parameter")

bool jtag2::resetProgram(bool possible_nSRST_ignored)
{
    if (proto == PROTO_DW) {
	/* The JTAG ICE mkII and Dragon do not respond correctly to
	 * the CMND_RESET command while in debugWire mode. */
	return interruptProgram()
	    && setProgramCounter(0);
    } else {
	uchar cmd[2] = { CMND_RESET, 0x01 };
	uchar *resp;
	int respSize;

	bool rv = doJtagCommand(cmd, 2, resp, respSize);
	delete [] resp;

	/* Await the BREAK event that is posted by the ICE. */
	bool bp, gdb;
	expectEvent(bp, gdb);

	return rv;
    }
}

PRAGMA_DIAG_POP

bool jtag2::interruptProgram(void)
{
    uchar cmd[2] = { CMND_FORCED_STOP, 0x01 };
    uchar *resp;
    int respSize;

    bool rv = doJtagCommand(cmd, 2, resp, respSize);
    delete [] resp;

    bool bp, gdb;
    expectEvent(bp, gdb);

    return rv;
}

bool jtag2::resumeProgram(bool restoreTarget)
{
    xmegaSendBPs();

    if (restoreTarget)
	doSimpleJtagCommand(CMND_RESTORE_TARGET);
    doSimpleJtagCommand(CMND_GO);

    cached_pc_is_valid = false;

    return true;
}

void jtag2::expectEvent(bool &breakpoint, bool &gdbInterrupt)
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
	else if (!nonbreaking_events[evtbuf[8] - EVT_BREAK])
	{
	    switch (evtbuf[8])
	    {
		// Program stopped at some kind of breakpoint.
		case EVT_BREAK:
		    cached_pc = 2 * b4_to_u32(evtbuf + 9);
		    cached_pc_is_valid = true;
		    /* FALLTHROUGH */
		case EVT_EXT_RESET:
		case EVT_PDSB_BREAK:
		case EVT_PDSMB_BREAK:
		case EVT_PROGRAM_BREAK:
		    breakpoint = true;
		    break;

		case EVT_IDR_DIRTY:
		    // The program is still running at IDR dirty, so
		    // pretend a user break;
		    gdbInterrupt = true;
		    printf("\nIDR dirty: 0x%02x\n", evtbuf[9]);
		    break;

		    // Fatal debugWire errors, cannot continue
		case EVT_ERROR_PHY_FORCE_BREAK_TIMEOUT:
		case EVT_ERROR_PHY_MAX_BIT_LENGTH_DIFF:
		case EVT_ERROR_PHY_OPT_RECEIVE_TIMEOUT:
		case EVT_ERROR_PHY_OPT_RECEIVED_BREAK:
		case EVT_ERROR_PHY_RECEIVED_BREAK:
		case EVT_ERROR_PHY_RECEIVE_TIMEOUT:
		case EVT_ERROR_PHY_RELEASE_BREAK_TIMEOUT:
		case EVT_ERROR_PHY_SYNC_OUT_OF_RANGE:
		case EVT_ERROR_PHY_SYNC_TIMEOUT:
		case EVT_ERROR_PHY_SYNC_TIMEOUT_BAUD:
		case EVT_ERROR_PHY_SYNC_WAIT_TIMEOUT:
		    gdbInterrupt = true;
		    printf("\nFatal debugWIRE communication event: 0x%02x\n",
			   evtbuf[8]);
		    break;

		    // Other fatal errors, user could mask them off
		case EVT_ICE_POWER_ERROR_STATE:
		    gdbInterrupt = true;
		    printf("\nJTAG ICE mkII power failure\n");
		    break;

		case EVT_TARGET_POWER_OFF:
		    gdbInterrupt = true;
		    printf("\nTarget power turned off\n");
		    break;

		case EVT_TARGET_POWER_ON:
		    gdbInterrupt = true;
		    printf("\nTarget power returned\n");
		    break;

		case EVT_TARGET_SLEEP:
		    gdbInterrupt = true;
		    printf("\nTarget went to sleep\n");
		    break;

		case EVT_TARGET_WAKEUP:
		    gdbInterrupt = true;
		    printf("\nTarget went out of sleep\n");
		    break;

		    // Events where we want to continue
		case EVT_NONE:
		case EVT_RUN:
		    break;

		default:
		    gdbInterrupt = true;
		    printf("\nUnhandled JTAG ICE mkII event: 0x%0x2\n",
			   evtbuf[8]);
	    }
	}
	delete [] evtbuf;
    }
}

bool jtag2::eventLoop(void)
{
    int maxfd;
    fd_set readfds;
    bool breakpoint = false, gdbInterrupt = false;

    // Now that we are "going", wait for either a response from the JTAG
    // box or a nudge from GDB.

    if (ctrlPipe != -1)
      {
	  /* signal the USB daemon to start polling. */
	  char cmd[1] = { 'p' };
	  (void)(write(ctrlPipe, cmd, 1) != 0);
      }

    for (;;)
      {
	  debugOut("Waiting for input.\n");

	  // Check for input from JTAG ICE (breakpoint, sleep, info, power)
	  // or gdb (user break)
	  FD_ZERO (&readfds);
	  if (gdbFileDescriptor != -1)
	    FD_SET (gdbFileDescriptor, &readfds);
	  FD_SET (jtagBox, &readfds);
	  if (gdbFileDescriptor != -1)
	    maxfd = jtagBox > gdbFileDescriptor ? jtagBox : gdbFileDescriptor;
	  else
	    maxfd = jtagBox;

	  int numfds = select(maxfd + 1, &readfds, 0, 0, 0);
	  unixCheck(numfds, "GDB/JTAG ICE communications failure");

	  if (gdbFileDescriptor != -1 && FD_ISSET(gdbFileDescriptor, &readfds))
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
		expectEvent(breakpoint, gdbInterrupt);
	    }

	  // We give priority to user interrupts
	  if (gdbInterrupt)
	      return false;
	  if (breakpoint)
	      return true;
      }
}


bool jtag2::jtagSingleStep(bool useHLL)
{
    uchar cmd[3] = { CMND_SINGLE_STEP,
		     0x01,
		     useHLL? 0x00: 0x01 };
    uchar *resp;
    int respSize, i = 2;
    bool rv;

    xmegaSendBPs();

    cached_pc_is_valid = false;

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

    if (!rv)
        return false;

    bool bp, gdb;
    expectEvent(bp, gdb);

    return true;
}

void jtag2::parseEvents(const char *evtlist)
{
    memset(nonbreaking_events, 0, sizeof nonbreaking_events);

    const struct
    {
        uchar num;
        const char *name;
    } evttable[] =
        {
            { EVT_BREAK,				"break" },
            { EVT_DEBUG,				"debug" },
            { EVT_ERROR_PHY_FORCE_BREAK_TIMEOUT,	"error_phy_force_break_timeout" },
            { EVT_ERROR_PHY_MAX_BIT_LENGTH_DIFF,	"error_phy_max_bit_length_diff" },
            { EVT_ERROR_PHY_OPT_RECEIVE_TIMEOUT,	"error_phy_opt_receive_timeout" },
            { EVT_ERROR_PHY_OPT_RECEIVED_BREAK,		"error_phy_opt_received_break" },
            { EVT_ERROR_PHY_RECEIVED_BREAK,		"error_phy_received_break" },
            { EVT_ERROR_PHY_RECEIVE_TIMEOUT,		"error_phy_receive_timeout" },
            { EVT_ERROR_PHY_RELEASE_BREAK_TIMEOUT,	"error_phy_release_break_timeout" },
            { EVT_ERROR_PHY_SYNC_OUT_OF_RANGE,		"error_phy_sync_out_of_range" },
            { EVT_ERROR_PHY_SYNC_TIMEOUT,		"error_phy_sync_timeout" },
            { EVT_ERROR_PHY_SYNC_TIMEOUT_BAUD,		"error_phy_sync_timeout_baud" },
            { EVT_ERROR_PHY_SYNC_WAIT_TIMEOUT,		"error_phy_sync_wait_timeout" },
            { EVT_RESULT_PHY_NO_ACTIVITY,		"result_phy_no_activity" },
            { EVT_EXT_RESET,				"ext_reset" },
            { EVT_ICE_POWER_ERROR_STATE,		"ice_power_error_state" },
            { EVT_ICE_POWER_OK,				"ice_power_ok" },
            { EVT_IDR_DIRTY,				"idr_dirty" },
            { EVT_NONE,					"none" },
            { EVT_PDSB_BREAK,				"pdsb_break" },
            { EVT_PDSMB_BREAK,				"pdsmb_break" },
            { EVT_PROGRAM_BREAK,			"program_break" },
            { EVT_RUN,					"run" },
            { EVT_TARGET_POWER_OFF,			"target_power_off" },
            { EVT_TARGET_POWER_ON,			"target_power_on" },
            { EVT_TARGET_SLEEP,				"target_sleep" },
            { EVT_TARGET_WAKEUP,			"target_wakeup" },
        };

    // parse the given comma-separated string
    const char *cp1, *cp2;
    cp1 = evtlist;
    while (*cp1 != '\0')
    {
        while (isspace(*cp1) || *cp1 == ',')
            cp1++;
        cp2 = cp1;
        while (*cp2 != '\0' && *cp2 != ',')
            cp2++;
        size_t l = cp2 - cp1;
        uchar evtval = 0;

        // Now, cp1 points to the name to parse, of length l
        for (unsigned int i = 0; i < sizeof evttable / sizeof evttable[0]; i++)
        {
            if (strncmp(evttable[i].name, cp1, l) == 0)
            {
                evtval = evttable[i].num;
                break;
            }
        }
        if (evtval == 0)
        {
            fprintf(stderr, "Warning: event name %.*s not matched\n",
                    (int)l, cp1);
        }
        else
        {
            nonbreaking_events[evtval - EVT_BREAK] = true;
        }

        cp1 = cp2;
    }
}

bool jtag2::jtagContinue(void)
{
    updateBreakpoints(); // download new bp configuration

    if (haveHiddenBreakpoint)
    {
	// One of our breakpoints has been set as the high-level
	// language boundary address of our current statement, so
	// perform a high-level language single step.
	(void)jtagSingleStep(true);
    }
    else
    {
	xmegaSendBPs();

	doSimpleJtagCommand(CMND_GO);
    }

    return eventLoop();
}

