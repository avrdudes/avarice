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

bool jtag2::codeBreakpointBetween(unsigned int start, unsigned int end)
{
    int i;

    i = 0;
    while (!bp[i].last)
      {
	  if ((bp[i].address >= start && bp[i].address < end) &&
	      (bp[i].type == CODE) && bp[i].enabled)
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


bool jtag2::stopAt(unsigned int address)
{
    uchar one = 1;
    return jtagWrite(BREAKPOINT_SPACE_ADDR_OFFSET + address / 2, 1, &one);
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
    if ((global_p_device_def->device_flags & DEVFL_NO_SOFTBP) != 0)
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

bool jtag2::addBreakpoint(unsigned int address, bpType type, unsigned int length)
{
    int bp_i;

    debugOut("BP ADD type: %d  addr: 0x%x ", type, address);


    // Perhaps we have already set this breakpoint, and it is just
    // marked as disabled In that case we don't need to make a new
    // one, just flag this one as enabled again
    bp_i = 0;
    while (!bp[bp_i].last)
      {
	  if ((bp[bp_i].address == address) && (bp[bp_i].type == type))
	    {
		bp[bp_i].enabled = true;
		debugOut("ENABLED\n");
		break;
            }
	  bp_i++;
      }

    // Was what we just did not successful, if so...
    if (!bp[bp_i].enabled || (bp[bp_i].address != address) || (bp[bp_i].type == type))
      {

	  // Uhh... we are out of space. Try to find a disabled one and just
	  // write over it.
	  if ((bp_i + 1) == MAX_TOTAL_BREAKPOINTS2)
            {
		bp_i = 0;
		// We can't remove enabled breakpoints, or ones that
		// have JUST been disabled. The just disabled ones
		// because they have to sync to the ICE
		while (bp[bp_i].enabled || bp[bp_i].toremove)
		  {
		      bp_i++;
		  }
            }

	  // Sorry.. out of room :(
	  if ((bp_i + 1) == MAX_TOTAL_BREAKPOINTS2)
            {
		debugOut("FAILED\n");
		return false;
            }

        if (bp[bp_i].last)
	  {
	      //See if we need to set the new endpoint
	      bp[bp_i + 1].last = true;
	      bp[bp_i + 1].enabled = false;
	      bp[bp_i + 1].address = 0;
	      bp[bp_i + 1].type = NONE;
	  }

        // bp_i now has the new breakpoint we are going to use.
        bp[bp_i].last = false;
        bp[bp_i].enabled = true;
        bp[bp_i].address = address;
        bp[bp_i].type = type;

        // Is it a range breakpoint?
        // Range breakpoint needs to be aligned, and the length must
        // be representable as a bitmask.
        if ((length > 1) && ((type == READ_DATA) ||
                             (type == WRITE_DATA) ||
                             (type == ACCESS_DATA)))
	  {
	      int bitno = ffs((int)length);
	      unsigned int mask = 1 << (bitno - 1);
	      if (mask != length)
                {
		    debugOut("FAILED: length not power of 2 in range BP\n");
		    bp[bp_i].last = true;
		    bp[bp_i].enabled = false;
		    return false;
                }
	      mask--;
	      if ((address & mask) != 0)
                {
		    debugOut("FAILED: address in range BP is not base-aligned\n");
		    bp[bp_i].last = true;
		    bp[bp_i].enabled = false;
		    return false;
                }
	      mask = ~mask;

	      // add the breakpoint as a data mask.. only thing is we
	      // need to find it afterwards
	      if (!addBreakpoint(mask, DATA_MASK, 1))
                {
		    debugOut("FAILED\n");
		    bp[bp_i].last = true;
		    bp[bp_i].enabled = false;
		    return false;
                }

	      unsigned int j;
	      for(j = 0; !bp[j].last; j++)
                {
		    if ((bp[j].type == DATA_MASK) && (bp[bp_i].address == mask))
			break;
                }

	      bp[bp_i].mask_pointer = j;

	      debugOut("range BP ADDED: 0x%x/0x%x\n", address, mask);
	  }

      }

    // Is this breakpoint new?
    if (!bp[bp_i].icestatus)
      {
	  // Yup - flag it as something to download
	  bp[bp_i].toadd = true;
	  bp[bp_i].toremove = false;
      }
    else
      {
	  bp[bp_i].toadd = false;
	  bp[bp_i].toremove = false;
      }

    if (!layoutBreakpoints())
      {
	  debugOut("Not enough room in ICE for breakpoint. FAILED.\n");
	  bp[bp_i].enabled = false;
	  bp[bp_i].toadd = false;

	  if ((bp[bp_i].type == READ_DATA) ||
              (bp[bp_i].type == WRITE_DATA) ||
              (bp[bp_i].type == ACCESS_DATA))
              // these BP types have an associated mask
            {
		bp[bp[bp_i].mask_pointer].enabled = false;
		bp[bp[bp_i].mask_pointer].toadd = false;
            }

          return false;
      }

    return true;
}

PRAGMA_DIAG_PUSH
PRAGMA_DIAG_IGNORED("-Wunused-parameter")

bool jtag2::deleteBreakpoint(unsigned int address, bpType type, unsigned int length)
{
    int bp_i;

    debugOut("BP DEL type: %d  addr: 0x%x ", type, address);

    bp_i = 0;
    while (!bp[bp_i].last)
      {
	  if ((bp[bp_i].address == address) && (bp[bp_i].type == type))
            {
		bp[bp_i].enabled = false;
		debugOut("DISABLED\n");
		break;
            }
	  bp_i++;
      }

    // If it somehow failed, got to tell..
    if (bp[bp_i].enabled || (bp[bp_i].address != address) || (bp[bp_i].type != type))
      {
	  debugOut("FAILED\n");
	  return false;
      }

    // Is this breakpoint actually enabled?
    if (bp[bp_i].icestatus)
      {
	  // Yup - flag it as something to delete
	  bp[bp_i].toadd = false;
	  bp[bp_i].toremove = true;
      }
    else
      {
	  bp[bp_i].toadd = false;
	  bp[bp_i].toremove = false;
      }

    return true;
}

PRAGMA_DIAG_POP

/*
 * This routine is where all the logic of what breakpoints go into the
 * ICE and what don't happens. When called it assumes all the "toadd"
 * breakpoints don't have valid "bpnum" attached to them. It then
 * attempts to fit all the needed breakpoints into the hardware
 * breakpoints first. If that won't fly it then adds software
 * breakpoints as needed... or just fails.
 *
 *
 * TODO: Some logic to decide which breakpoints change a lot and
 * should go in hardware, vs. which breakpoints are more fixed and
 * should just be done in code would be handy
 */
bool jtag2::layoutBreakpoints(void)
{
    // remaining_bps is an array showing which breakpoints are still
    // available, starting at 0x00 Note 0x00 is software breakpoint
    // and will always be available in theory so just ignore the first
    // array element, it's meaningless...  FIXME: Slot 4 is set to
    // 'false', doesn't seem to work?
    bool remaining_bps[MAX_BREAKPOINTS2 + 2] = {false, true, true, true, false, false};
    int bp_i;
    uchar bpnum;
    bool softwarebps = true;
    bool hadroom = true;

    if (global_p_device_def->device_flags == DEVFL_NO_SOFTBP)
      {
	  softwarebps = false;
      }

    // Turn off everything but software breakpoints for DebugWire,
    // or for old firmware ICEs with Xmega devices
    if (useDebugWire ||
        (is_xmega && !has_full_xmega_support))
      {
	  int k;
	  for (k = 1; k < MAX_BREAKPOINTS2 + 1; k++)
            {
		remaining_bps[k] = false;
            }
      }
    else if (is_xmega)
      {
	  // Xmega has only two hardware slots?
	  remaining_bps[BREAKPOINT2_XMEGA_UNAVAIL] = false;
      }

    bp_i = 0;
    while (!bp[bp_i].last)
      {
	  // check we have an enabled "stable" breakpoint that's not
	  // about to change
	  if (bp[bp_i].enabled && !bp[bp_i].toremove && bp[bp_i].icestatus)
            {
		remaining_bps[bp[bp_i].bpnum] = false;
            }

	  bp_i++;
      }

    // Do data watchpoints first
    bp_i = 0;

    while (!bp[bp_i].last)
      {
        // Find the next data breakpoint that needs somewhere to live
	  if (bp[bp_i].enabled && bp[bp_i].toadd &&
	      ((bp[bp_i].type == READ_DATA) ||
	       (bp[bp_i].type == WRITE_DATA) ||
	       (bp[bp_i].type == ACCESS_DATA)))
	    {
		// Check if we have both slots available
		if (!remaining_bps[BREAKPOINT2_DATA_MASK] ||
		    !remaining_bps[BREAKPOINT2_FIRST_DATA])
		{
		    debugOut("Not enough room to store range breakpoint\n");
		    bp[bp[bp_i].mask_pointer].enabled = false;
		    bp[bp[bp_i].mask_pointer].toadd = false;
		    bp[bp_i].enabled = false;
		    bp[bp_i].toadd = false;
		    bp_i++;
		    hadroom = false;
		    continue; // Skip this breakpoint
		}
		else
		{
		    remaining_bps[BREAKPOINT2_DATA_MASK] = false;
		    bp[bp[bp_i].mask_pointer].bpnum = BREAKPOINT2_DATA_MASK;
		}

		// Find next available slot
		bpnum = BREAKPOINT2_FIRST_DATA;
		while (!remaining_bps[bpnum] && (bpnum <= MAX_BREAKPOINTS2))
		  {
		      bpnum++;
		  }

		if (bpnum > MAX_BREAKPOINTS2)
		  {
		      debugOut("No more room for data breakpoints.\n");
		      hadroom = false;
		      break;
		  }

		bp[bp_i].bpnum = bpnum;
		remaining_bps[bpnum] = false;
	    }

	  bp_i++;
      }

    // Do CODE breakpoints now

    bp_i = 0;
    while (!bp[bp_i].last)
      {
	  //Find the next spot to live in.
	  bpnum = 0x00;
	  while (!remaining_bps[bpnum] && (bpnum <= MAX_BREAKPOINTS2))
            {
		debugOut("Slot %d full\n", bpnum);
		bpnum++;
            }

	  if (bpnum > MAX_BREAKPOINTS2)
            {
		if (softwarebps)
		  {
		      bpnum = 0x00;
		  }
		else
		  {
		      bpnum = 0xFF; // flag for NO MORE BREAKPOINTS
		  }
            }

	  // Find the next breakpoint that needs somewhere to live
	  if (bp[bp_i].enabled && bp[bp_i].toadd && (bp[bp_i].type == CODE))
            {
		if (bpnum == 0xFF)
		  {
		      debugOut("No more room for code breakpoints.\n");
		      hadroom = false;
		      break;
		  }
		bp[bp_i].bpnum = bpnum;
		remaining_bps[bpnum] = false;
            }

	  bp_i++;
      }

    return hadroom;
}

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
		    if(!doJtagCommand(cmd, 6, response, responseSize))
			check(false, "Failed to clear breakpoint");

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
		    check(xmega_n_bps < 2, "Too many hard BPs for Xmega");
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
			    check(false, "Invalid bp mode (for data bp)");
			    break;
		    }

		    uchar *response;
		    int responseSize;
		    check(doJtagCommand(cmd, 8, response, responseSize),
			  "Failed to set breakpoint");
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
    check(doJtagCommand(cmdx, 14, response, responseSize),
	  "Failed to set Xmega breakpoints");
    delete [] response;

    xmega_n_bps = 0; // must be set again upon next run
}
