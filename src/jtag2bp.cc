/*
 *	avarice - The "avarice" program.
 *	Copyright (C) 2001 Scott Finneran
 *      Copyright (C) 2002 Intel Corporation
 *	Copyright (C) 2005,2006 Joerg Wunsch
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
 * This file contains the breakpoint handling for the mkII
 * protocol. It has been re-written by Colin O'Flynn.
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

void jtag2::deleteAllBreakpoints(void)
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


#ifdef notyet
bool jtag2::BreakpointRoom(bpType type, unsigned int length)
{
    int numdata = 0;
    int numcode = 0;
    int bp_i = 0;

    //code
    if (type == CODE)
      {
	  numcode++;
      }

    //data
    else if ((bp[bp_i].type == READ_DATA) ||
	     (bp[bp_i].type == WRITE_DATA) ||
	     (bp[bp_i].type == ACCESS_DATA))
      {
	  // range BPs use two slots
	  if (length > 1)
	    {
		numdata += 2;
	    }
	  else
	    {
		numdata++;
	    }
      }

    // First let's check debugWire which doesn't let you have data breakpoints
    if (useDebugWire)
      {
	  if (numdata > 0)
            {
		debugOut("DebugWire doesn't support data breakpoints.\n");
		return false;
            }
      }

    // Count current breakpoints
    while (!bp[bp_i].last)
      {
        // how many breakpoints are we predicting to be enabled?
        if (bp[bp_i].toadd || bp[bp_i].enabled)
            {
             if ((bp[bp_i].type == READ_DATA) ||
		 (bp[bp_i].type == WRITE_DATA) ||
		 (bp[bp_i].type == ACCESS_DATA) ||
		 (bp[bp_i].type == DATA_MASK))
	       {
		   numdata++;
	       }
	     else if (bp[bp_i].type == CODE)
	       {
		   numcode++;
	       }
            }
        bp_i++;
      }

    // Some devices don't support software breakpoints.  Also there
    // might be an upper limit on software breakpoints, do we care?
    if ((deviceDef->device_flags & DEVFL_NO_SOFTBP) != 0)
      {
	  // There are only 3 hardware BPs available, the fourth one
	  // is reserved by the ICE for single-stepping.
	  if ((numcode > (MAX_BREAKPOINTS2_CODE - 1)) ||
	      ((numcode + numdata) > (MAX_BREAKPOINTS2 - 1)))
	    {
                return false;
	    }
      }

    if (numdata > MAX_BREAKPOINTS2_DATA)
      {
	  return false;
      }

    // Your room is ready
    return true;
}
#endif // notyet

void jtag2::updateBreakpoints(void)
{
    int bp_i;

    layoutBreakpoints();

    // Delete all the breakpoints that were flagged first
    bp_i = 0;
    while (!bp[bp_i].last)
      {
	  uchar cmd[6] = { CMND_CLR_BREAK };

	  if (bp[bp_i].toremove)
            {
		debugOut("Breakpoint deleted in ICE. slot: %d  type: %d  addr: 0x%x\n",
			 bp[bp_i].bpnum, bp[bp_i].type, bp[bp_i].address);

		if (is_xmega && has_full_xmega_support &&
		    bp[bp_i].type == CODE && bp[bp_i].bpnum != 0x00)
		{
		    // no action needed on this one, has been auto-removed
		}
		else
		{
		    cmd[1] = bp[bp_i].bpnum;

		    // Software breakpoints need the address!
		    if (bp[bp_i].bpnum == 0x00)
			u32_to_b4(cmd + 2, (bp[bp_i].address / 2));
		    else
			u32_to_b4(cmd + 2, 0);

		    uchar *response;
		    int responseSize;
                    try
                    {
                        doJtagCommand(cmd, 6, response, responseSize);
                    }
                    catch (jtag_exception& e)
                    {
                        fprintf(stderr, "Failed to clear breakpoint: %s\n",
                                e.what());
                        throw;
                    }

		    delete [] response;
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
	  uchar cmd[8] = { CMND_SET_BREAK };

	  if (bp[bp_i].toadd && bp[bp_i].enabled)
            {
		debugOut("Breakpoint added in ICE. slot: %d  type: %d  addr: 0x%x\n",
			 bp[bp_i].bpnum, bp[bp_i].type, bp[bp_i].address);

		if (is_xmega && has_full_xmega_support &&
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
		    cmd[2] = bp[bp_i].bpnum;

		    if (bp[bp_i].type == CODE)
		    {
			// The JTAG box sees program memory as 16-bit
			// wide locations. GDB sees bytes. As such,
			// halve the breakpoint address.
			u32_to_b4(cmd + 3, (bp[bp_i].address / 2));
		    }
		    else
		    {
			u32_to_b4(cmd + 3, bp[bp_i].address & ~ADDR_SPACE_MASK);
		    }

		    // cmd[7] is the BP mode (memory read/write/read or write/code)
		    // cmd[1] is the BP type (program memory, data, data mask)
		    switch (bp[bp_i].type)
		    {
			case READ_DATA:
			    cmd[7] = 0x00;
			    cmd[1] = 0x02;
			    break;
			case WRITE_DATA:
			    cmd[7] = 0x01;
			    cmd[1] = 0x02;
			    break;
			case ACCESS_DATA:
			    cmd[7] = 0x02;
			    cmd[1] = 0x02;
			    break;
			case DATA_MASK:
			    cmd[7] = 0x00;
			    cmd[1] = 0x03;
			    break;
			case CODE:
			    cmd[7] = 0x03;
			    cmd[1] = 0x01;
			    break;
			default:
			    throw jtag_exception("Invalid bp mode (for data bp)");
			    break;
		    }

		    uchar *response;
		    int responseSize;
                    try
                    {
                        doJtagCommand(cmd, 8, response, responseSize);
                    }
                    catch (jtag_exception& e)
                    {
                        fprintf(stderr, "Failed to set breakpoint: %s\n",
                                e.what());
                        throw;
                    }
		    delete [] response;

		    bp[bp_i].icestatus = true;
		}

		// It's a beautiful baby breakpoint
		bp[bp_i].toadd = false;
	    }

	  bp_i++;
      }
}

void jtag2::xmegaSendBPs(void)
{
    if (!(is_xmega && has_full_xmega_support))
	return;

    uchar *response;
    int responseSize;
    uchar cmdx[14] = { CMND_SET_BREAK_XMEGA };
    if (xmega_n_bps > 0)
	u32_to_b4(cmdx + 1, (xmega_bps[0] / 2));
    if (xmega_n_bps > 1)
	u32_to_b4(cmdx + 5, (xmega_bps[1] / 2));
    cmdx[12] = xmega_n_bps;
    try
    {
        doJtagCommand(cmdx, 14, response, responseSize);
    }
    catch (jtag_exception& e)
    {
        fprintf(stderr, "Failed to set Xmega breakpoints: %s\n",
                e.what());
        throw;
    }
    delete [] response;

    xmega_n_bps = 0; // must be set again upon next run
}
