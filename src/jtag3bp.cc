/*
 *	avarice - The "avarice" program.
 *	Copyright (C) 2012 Joerg Wunsch
 *
 *	Heavily based on jtag2bp.cc, which is
 *	Copyright (C) 2007, Colin O'Flynn
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
 * This file contains the breakpoint handling for the JTAGICE3 protocol.
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
#include "jtag3.h"

bool jtag3::codeBreakpointAt(unsigned int address)
{
    int i;

    i = 0;
    while (!bp[i].last)
      {
	  if ((bp[i].address == address) && (bp[i].type == CODE) && bp[i].enabled)
	      return true;

	  i++;
      }

    return false;
}

void jtag3::deleteAllBreakpoints(void)
{
    int i = 0;

    while (!bp[i].last)
      {
	  if (bp[i].enabled && bp[i].icestatus)
	      bp[i].toremove = true;

	  bp[i].enabled = false;
	  i++;
      }
}


void jtag3::updateBreakpoints(void)
{
  int bp_i;

  layoutBreakpoints();

  // Delete all the breakpoints that were flagged first
  bp_i = 0;
  while (!bp[bp_i].last)
  {
    if (bp[bp_i].toremove)
    {
      debugOut("Breakpoint deleted in ICE. slot: %d  type: %d  addr: 0x%x\n",
	       bp[bp_i].bpnum, bp[bp_i].type, bp[bp_i].address);

      if (is_xmega &&
	  bp[bp_i].type == CODE && bp[bp_i].bpnum != 0x00)
      {
	// no action needed on this one, has been auto-removed
      }
      else
      {
	uchar cmd[7];
	int cmdlen;

	cmd[0] = SCOPE_AVR;
	cmd[2] = 0;

	if (bp[bp_i].bpnum == 0x00)
	{
	  // Software BP, address must be given
	  cmd[1] = CMD3_CLEAR_SOFT_BP;
	  u32_to_b4(cmd + 3, bp[bp_i].address );
	  cmdlen = 7;
	}
	else
	{
	  // Hardware BP, only BP number needed
	  cmd[1] = CMD3_CLEAR_BP;
	  cmd[3] = bp[bp_i].bpnum;
	  cmdlen = 4;
	}

	uchar *resp;
	int respsize;
	try
	{
	  doJtagCommand(cmd, cmdlen, "clear BP", resp, respsize);
	}
	catch (jtag_exception& e)
	{
	  fprintf(stderr, "Failed to clear breakpoint: %s\n",
		  e.what());
	  throw;
	}

	delete [] resp;
      }

      // rip breakpoint
      bp[bp_i].icestatus = false;
      bp[bp_i].toremove = false;
    }

    bp_i++;
  }

  // Add all the new breakpoints
  bp_i = 0;
  while (!bp[bp_i].last)
  {
    if (bp[bp_i].toadd && bp[bp_i].enabled)
    {
      debugOut("Breakpoint added in ICE. slot: %d  type: %d  addr: 0x%x\n",
	       bp[bp_i].bpnum, bp[bp_i].type, bp[bp_i].address);

      if (is_xmega &&
	  bp[bp_i].type == CODE && bp[bp_i].bpnum != 0x00)
      {
	if (xmega_n_bps >= 2)
	  throw jtag_exception("Too many hard BPs for Xmega");
	// Xmega code breakpoint
	xmega_bps[xmega_n_bps++] = bp[bp_i].address;
	// these breakpoints are auto-removed by the ICE
	bp[bp_i].toremove = true;
	bp[bp_i].icestatus = false;
      }
      else
      {
	uchar cmd[10];
	int cmdlen;

	cmd[0] = SCOPE_AVR;
	cmd[2] = 0;

	if (bp[bp_i].bpnum != 0)
	{
	  // Hardware BP
	  cmd[1] = CMD3_SET_BP;
	  cmd[4] = bp[bp_i].bpnum;
	  cmdlen = 10;

	  // JTAGICE3 handles all BP addresses (including CODE) as
	  // byte addresses
	  if (bp[bp_i].type == DATA_MASK ||
	      bp[bp_i].type == CODE)
	      u32_to_b4(cmd + 5, bp[bp_i].address);
	  else
	      u32_to_b4(cmd + 5, bp[bp_i].address & ~ADDR_SPACE_MASK);

	  // cmd[9] is the BP mode (memory read/write/read or write/code)
	  // cmd[3] is the BP type (program memory, data, data mask)
	  switch (bp[bp_i].type)
	  {
	    case READ_DATA:
	      cmd[9] = 0x00;
	      cmd[3] = 0x02;
	      break;
	    case WRITE_DATA:
	      cmd[9] = 0x01;
	      cmd[3] = 0x02;
	      break;
	    case ACCESS_DATA:
	      cmd[9] = 0x02;
	      cmd[3] = 0x02;
	      break;
	    case DATA_MASK:
	      cmd[9] = 0x00;
	      cmd[3] = 0x03;
	      break;
	    case CODE:
	      cmd[9] = 0x03;
	      cmd[3] = 0x01;
	      break;
	    default:
	      throw jtag_exception("Invalid bp mode (for data bp)");
	      break;
	  }
	}
	else
	{
	  // Software BP
	  cmd[1] = CMD3_SET_SOFT_BP;
	  cmdlen = 7;

	  u32_to_b4(cmd + 3, bp[bp_i].address & ~ADDR_SPACE_MASK);
	}

	uchar *resp;
	int respsize;
	try
	{
	  doJtagCommand(cmd, cmdlen, "set BP", resp, respsize);
	}
	catch (jtag_exception& e)
	{
	  fprintf(stderr, "Failed to set breakpoint: %s\n",
		  e.what());
	  throw;
	}
	delete [] resp;

	bp[bp_i].icestatus = true;
      }

      // It's a beautiful baby breakpoint
      bp[bp_i].toadd = false;
    }

    bp_i++;
  }
}

void jtag3::xmegaSendBPs(void)
{
  if (!is_xmega)
    return;

  uchar *resp;
  int respsize;
  uchar cmd[16] = { SCOPE_AVR, CMD3_SET_BP_XMEGA };
  if (xmega_n_bps > 0)
    u32_to_b4(cmd + 3, (xmega_bps[0] / 2));
  if (xmega_n_bps > 1)
    u32_to_b4(cmd + 7, (xmega_bps[1] / 2));
  cmd[14] = xmega_n_bps;

  /*
   * Apparently (tracing IAR), Xmega data breakpoints are supposed to
   * work like this:
   *   fill cmd[3:6] and cmd[7:10] with start and end address to
   *      watch for, respectively
   *   set cmd[14] to 2 (2 addresses present; only 1 for single-byte BP)
   *   set cmd[13] to 0xf2 for a range BP, 0xc2 if only single-byte BP
   */
  try
  {
      doJtagCommand(cmd, 16, "set BP Xmega", resp, respsize);
  }
  catch (jtag_exception& e)
  {
    fprintf(stderr, "Failed to set Xmega breakpoints: %s\n",
	    e.what());
    throw;
  }
  delete [] resp;

  xmega_n_bps = 0; // must be set again upon next run
}
