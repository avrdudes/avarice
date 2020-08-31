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
 * This file implements target execution handling for the JTAGICE3 protocol.
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
#include "jtag3.h"
#include "remote.h"

unsigned long jtag3::getProgramCounter(void)
{
  if (cached_pc_is_valid)
    return cached_pc;

  uchar *resp;
  int respsize;
  uchar cmd[] = { SCOPE_AVR, CMD3_READ_PC, 0 };
  int cnt = 0;

  again:
  try
    {
      doJtagCommand(cmd, sizeof cmd, "read PC", resp, respsize);
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
      fprintf(stderr, "cannot read program counter: %s\n",
	      e.what());
      throw;
    }
  catch (jtag_exception& e)
    {
      fprintf(stderr, "cannot read program counter: %s\n",
	      e.what());
      throw;
    }

  unsigned long result = b4_to_u32(resp + 3);
  delete [] resp;

  // The JTAG box sees program memory as 16-bit wide locations. GDB
  // sees bytes. As such, double the PC value.
  result *= 2;

  cached_pc_is_valid = true;
  return cached_pc = result;
}

void jtag3::setProgramCounter(unsigned long pc)
{
  uchar *resp;
  int respsize;
  uchar cmd[7] = { SCOPE_AVR, CMD3_WRITE_PC };

  u32_to_b4(cmd + 3, pc / 2);

  try
    {
      doJtagCommand(cmd, sizeof cmd, "write PC", resp, respsize);
    }
  catch (jtag_exception& e)
    {
      fprintf(stderr, "cannot write program counter: %s\n",
	      e.what());
      throw;
    }

  delete [] resp;

  cached_pc_is_valid = false;
}

PRAGMA_DIAG_PUSH
PRAGMA_DIAG_IGNORED("-Wunused-parameter")

void jtag3::resetProgram(bool possible_nSRST_ignored)
{
  uchar cmd[] = { SCOPE_AVR, CMD3_RESET, 0, 0x01 };
  uchar *resp;
  int respsize;

  doJtagCommand(cmd, sizeof cmd, "reset", resp, respsize);
  delete [] resp;

  /* Await the BREAK event that is posted by the ICE. */
  bool bp, gdb;
  expectEvent(bp, gdb);

  /* The PC value in the event returned after a RESET is the
   * PC where the reset actually hit, so ignore it. */
  cached_pc_is_valid = false;
}

PRAGMA_DIAG_POP

void jtag3::interruptProgram(void)
{
  uchar cmd[] = { SCOPE_AVR, CMD3_STOP, 0, 0x01 };
  uchar *resp;
  int respsize;

  doJtagCommand(cmd, sizeof cmd, "stop", resp, respsize);
  delete [] resp;

  bool bp, gdb;
  expectEvent(bp, gdb);
}

void jtag3::resumeProgram(void)
{
  xmegaSendBPs();

  doSimpleJtagCommand(CMD3_CLEANUP, "cleanup");

  doSimpleJtagCommand(CMD3_GO, "go");

  cached_pc_is_valid = false;
}

void jtag3::expectEvent(bool &breakpoint, bool &gdbInterrupt)
{
  uchar *evtbuf;
  int evtsize;
  unsigned short seqno;

  if (cached_event != NULL)
  {
      evtbuf = cached_event;
      cached_event = NULL;
  }
  else
  {
      evtsize = recvFrame(evtbuf, seqno);
      if (evtsize > 0) {
          // XXX if not event, should push frame back into queue...
          // We really need a queue of received frames.
          if (seqno != 0xffff)
          {
              debugOut("Expected event packet, got other response");
              return;
          }
      }
      else
      {
          debugOut("Timed out waiting for an event");
          return;
      }
  }

  breakpoint = gdbInterrupt = false;

  switch ((evtbuf[0] << 8) | evtbuf[1])
  {
      // Program stopped at some kind of breakpoint.
      // On Xmega, byte 7 denotes the reason:
      //   0x01 soft BP
      //   0x10 hard BP (byte 8 contains BP #, or 3 for data BP)
      //   0x20, 0x21 "run to address" or single-step
      //   0x40 reset, leave progmode etc.
      // On megaAVR, byte 6 , byte 7 are likely the "break status
      // register" (MSB, LSB), see here:
      // http://people.ece.cornell.edu/land/courses/ece4760/FinalProjects/s2009/jgs33_rrw32/Final%20Paper/index.html
      // The bits do not fully match that description but to
      // a good degree.
      case (SCOPE_AVR << 8) | EVT3_BREAK:
          if ((!is_xmega && evtbuf[7] != 0) ||
              (is_xmega && evtbuf[7] != 0x40))
          {
              // program breakpoint
              cached_pc = 2 * b4_to_u32(evtbuf + 2);
              cached_pc_is_valid = true;
              breakpoint = true;
              debugOut("caching PC: 0x%04x\n", cached_pc);
          }
          else
          {
              debugOut("ignoring break event\n");
          }
          break;

      case (SCOPE_AVR << 8) | EVT3_IDR:
          statusOut("IDR dirty: 0x%02x\n", evtbuf[3]);
          break;

      case (SCOPE_GENERAL << 8) | EVT3_POWER:
          if (evtbuf[3] == 0)
          {
              gdbInterrupt = true;
              statusOut("\nTarget power turned off\n");
          }
          else
          {
              statusOut("\nTarget power returned\n");
          }
          break;

      case (SCOPE_GENERAL << 8) | EVT3_SLEEP:
          if (evtbuf[3] == 0)
          {
              //gdbInterrupt = true;
              statusOut("\nTarget went to sleep\n");
          }
          else
          {
              //gdbInterrupt = true;
              statusOut("\nTarget went out of sleep\n");
          }
          break;

      default:
          gdbInterrupt = true;
          statusOut("\nUnhandled JTAGICE3 event: 0x%02x, 0x%02x\n",
                    evtbuf[0], evtbuf[1]);
  }

  delete [] evtbuf;
}

bool jtag3::eventLoop(void)
{
    int maxfd;
    fd_set readfds;
    bool breakpoint = false, gdbInterrupt = false;

    // Now that we are "going", wait for either a response from the JTAG
    // box or a nudge from GDB.

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
	  if (numfds < 0)
              throw jtag_exception("GDB/JTAG ICE communications failure");

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


void jtag3::jtagSingleStep(void)
{
  uchar cmd[] = { SCOPE_AVR, CMD3_STEP,
		  0, 0x01, 0x01 };
  uchar *resp;
  int respsize;

  xmegaSendBPs();

  cached_pc_is_valid = false;

  try
    {
      doJtagCommand(cmd, sizeof cmd, "single-step", resp, respsize);
    }
  catch (jtag_io_exception& e)
    {
      if (e.get_response() != RSP3_FAIL_WRONG_MODE)
	throw;
    }
  delete [] resp;

  bool bp, gdb;
  expectEvent(bp, gdb);
}

void jtag3::parseEvents(const char *evtlist)
{
#if 0
    memset(nonbreaking_events, 0, sizeof nonbreaking_events);

    const struct
    {
        uchar scope, num;
        const char *name;
    } evttable[] =
        {
            { SCOPE_AVR, EVT3_BREAK,			"break" },
            { SCOPE_GENERAL, EVT3_SLEEP,		"sleep" },
            { SCOPE_GENERAL, EVT3_POWER,                "power" },
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
#endif
}

bool jtag3::jtagContinue(void)
{
  updateBreakpoints(); // download new bp configuration

  xmegaSendBPs();

  if (cached_event != NULL)
  {
      delete [] cached_event;
      cached_event = NULL;
  }

  doSimpleJtagCommand(CMD3_GO, "go");

  return eventLoop();
}

