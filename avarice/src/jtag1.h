/*
 *	avarice - The "avarice" program.
 *	Copyright (C) 2001 Scott Finneran
 *	Copyright (C) 2002, 2003, 2004 Intel Corporation
 *	Copyright (C) 2005, 2007 Joerg Wunsch
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
 * This file extends the generic "jtag" class for the mkI protocol.
 *
 * $Id$
 */

#ifndef JTAG1_H
#define JTAG1_H

#include "jtag.h"

enum SendResult { send_failed, send_ok, mcu_data };

/*
   There are apparently a total of three hardware breakpoints 
   (the docs claim four, but documents 2 breakpoints accessible via 
   non-existent parameters). 

   In summary, there is one code-only breakpoint, and 2 breakpoints which
   can be:
   - 2 code breakpoints
   - 2 1-byte data breakpoints
   - 1 code and 1-byte data breakpoint
   - 1 ranged code breakpoint
   - 1 ranged data breakpoint

   Currently we're ignoring ranges, so we allow up to 3 breakpoints,
   of which a maximum of 2 are data breakpoints. This is easily handled
   by keeping each kind of breakpoint separate.

   In the future, we could support "software" breakpoints too (though it
   seems mildly painful)

   See avrIceProtocol.txt for full details.
*/

enum {
  // We distinguish the total possible breakpoints and those for each type
  // (code or data) - see above
  MAX_BREAKPOINTS_CODE = 4,
  MAX_BREAKPOINTS_DATA = 2,
  MAX_BREAKPOINTS = 4
};

struct breakpoint
{
    unsigned int address;
    bpType type;
};

class jtag1: public jtag
{
    /** Decode 3-byte big-endian address **/
    unsigned long decodeAddress(uchar *buf) {
	return buf[0] << 16 | buf[1] << 8 | buf[2];
    };

    /** Encode 3-byte big-endian address **/
    void encodeAddress(uchar *buffer, unsigned long x) {
	buffer[0] = x >> 16;
	buffer[1] = x >> 8;
	buffer[2] = x;
    };

    breakpoint bpCode[MAX_BREAKPOINTS_CODE], bpData[MAX_BREAKPOINTS_DATA];
    int numBreakpointsCode, numBreakpointsData;

  public:
    jtag1(const char *dev, char *name, bool nsrst = false):
      jtag(dev, name) {
	apply_nSRST = nsrst;
    };

    virtual void initJtagBox(void);
    virtual void initJtagOnChipDebugging(unsigned long bitrate);

    virtual void deleteAllBreakpoints(void);
    virtual bool deleteBreakpoint(unsigned int address, bpType type, unsigned int length);
    virtual bool addBreakpoint(unsigned int address, bpType type, unsigned int length);
    virtual void updateBreakpoints(void);
    virtual bool codeBreakpointAt(unsigned int address);
    virtual bool codeBreakpointBetween(unsigned int start, unsigned int end);
    virtual bool stopAt(unsigned int address);
    virtual void parseEvents(const char *);

    virtual void enableProgramming(void);
    virtual void disableProgramming(void);
    virtual void eraseProgramMemory(void);
    virtual void eraseProgramPage(unsigned long address);
    virtual void downloadToTarget(const char* filename, bool program, bool verify);

    virtual unsigned long getProgramCounter(void);
    virtual bool setProgramCounter(unsigned long pc);
    virtual bool resetProgram(bool possible_nSRST);
    virtual bool interruptProgram(void);
    virtual bool resumeProgram(bool restoreTarget);
    virtual bool jtagSingleStep(bool useHLL = false);
    virtual bool jtagContinue(void);

    virtual uchar *jtagRead(unsigned long addr, unsigned int numBytes);
    virtual bool jtagWrite(unsigned long addr, unsigned int numBytes, uchar buffer[]);
    virtual const unsigned int statusAreaAddress(void) {
        /* no Xmega handling in JTAG ICE mkI */
        return 0x5D + DATA_SPACE_ADDR_OFFSET;
    };
    virtual const unsigned int cpuRegisterAreaAddress(void) {
        /* no Xmega handling in JTAG ICE mkI */
        return DATA_SPACE_ADDR_OFFSET;
    }

  private:
    virtual void changeBitRate(int newBitRate);
    virtual void setDeviceDescriptor(jtag_device_def_type *dev);
    virtual bool synchroniseAt(int bitrate);
    virtual void startJtagLink(void);
    virtual void deviceAutoConfig(void);
    virtual void configDaisyChain(void);

    uchar *getJtagResponse(int responseSize);
    SendResult sendJtagCommand(uchar *command, int commandSize, int *tries);
    bool checkForEmulator(void);

    /** Send a command to the jtag, with retries, and return the 'responseSize'
	byte response. Aborts avarice in case of to many failed retries.

	Returns a dynamically allocated buffer containing the reponse (caller
	must free)
    **/
    uchar *doJtagCommand(uchar *command, int  commandSize, int responseSize);

    /** Simplified form of doJtagCommand:
	Send 1-byte command 'cmd' to JTAG ICE, with retries, expecting a
	'responseSize' byte reponse.

	Return true if responseSize is 0 or if last response byte is
	JTAG_R_OK
    **/
    bool doSimpleJtagCommand(uchar cmd, int responseSize);

    // Miscellaneous
    // -------------

    /** Set JTAG ICE parameter 'item' to 'newValue' **/
    void setJtagParameter(uchar item, uchar newValue);

    /** Return value of JTAG ICE parameter 'item' **/
    uchar getJtagParameter(uchar item);

};

#endif
