/*
 *	avarice - The "avarice" program.
 *	Copyright (C) 2005,2006,2007 Joerg Wunsch
 *	Copyright (C) 2007, Colin O'Flynn
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
 * This file extends the generic "jtag" class for the mkII protocol.
 *
 * $Id$
 */

#ifndef JTAG2_H
#define JTAG2_H

#include "jtag.h"

/*
 * JTAG ICE mkII breakpoints are quite tricky.
 *
 * There are four possible breakpoint slots in JTAG.  The first one is
 * always reserved for single-stepping, and cannot be accessed
 * directly.  The second slot (ID #1) can be freely used as a code
 * breakpoint (only).  The third and fourth slot can either be used as
 * a code breakpoint, as an independent (byte-wide) data breakpoint
 * (i.e. a watchpoint in GDB terms), or together as a data breakpoint
 * consisting of an address and a mask.  The latter does not match
 * directly to GDB watchpoints (of a certain lenght > 1) but imposes
 * the additional requirement that the base address be aligned
 * properly wrt. the mask.
 *
 * The single-step breakpoint can indirectly be used by filling the
 * respective high-level language information into the event memory,
 * and issuing a high-level "step over" single-step.  As GDB does not
 * install the breakpoints for "stepi"-style single-steps (which would
 * require the very same JTAG breakpoint register), this ought to
 * work.
 *
 * Finally, there are software breakpoints where the respective
 * instruction will be replaced by a BREAK instruction in flash ROM by
 * means of an SPM call.  Some devices do not allow for this (as they
 * are said to be broken), and in general as this method contributes
 * to flash wear, we rather do not uninstall and reinstall these
 * breakpoints each time GDB asks for it, but instead "cache" them
 * locally, and only delete and add them as they actually change.
 *
 * To add to the mess, GDB's remote protocol isn't very smart about
 * telling us whether the next list of breakpoints + resume are
 * actually meant to be a high-level language (HLL) single-step
 * command.  Thus, always assume the last breakpoint that comes in to
 * be a single-step breakpoint, and replace the "resume" operation by
 * a "step" one in that case.  At least, GDB issues the breakpoint
 * list in the order of breakpoint numbers, so any possible HLL
 * single-step breakoint must be the last one in the list.
 *
 * Finally, GDB has explicit commands for setting hardware-assisted
 * breakpoints, but the default "break" command uses a software
 * breakpoint.  We try to replace as many software breakpoints as
 * possible by hardware breakpoints though, for the sake of
 * efficiency, yet would want to respect the user's choice for setting
 * a hardware breakpoint ("hbreak" or "thbreak")...
 *
 * XXX This is not done yet.
 */

enum {
  // We distinguish the total possible breakpoints and those for each type
  // (code or data) - see above
  MAX_BREAKPOINTS2_CODE = 4,
  MAX_BREAKPOINTS2_DATA = 2,
  MAX_BREAKPOINTS2 = 4,
  // various slot #s
  BREAKPOINT2_FIRST_DATA = 2,
  BREAKPOINT2_DATA_MASK = 3,

  MAX_TOTAL_BREAKPOINTS2 = 255
};

struct breakpoint2
{
    // High-level information on breakpoint
    unsigned int address;
    unsigned int mask_pointer;
    bpType type;
    bool enabled;

    // Used to flag end of list
    bool last;

    // Low-level information on breakpoint
    bool icestatus; // Status of breakpoint in ICE itself: 'true'
                    // when is enabled in ACTUAL device
    bool toremove;  // Delete this guy in ICE
    bool toadd;     // Add this guy in ICE
    uchar bpnum;    // ICE's breakpoint number (0x00 for software)
};

const struct breakpoint2 default_bp =
{
    0,				/* address */
    0,				/* mask_pointer */
    NONE,			/* type */
    false,			/* enabled */
    true,			/* last */
    false,			/* icestatus */
    false,			/* toremove */
    false,			/* toadd */
    0,				/* bpnum*/
};

class jtag2: public jtag
{
  private:
    unsigned short command_sequence;
    int devdescrlen;
    bool signedIn;
    bool haveHiddenBreakpoint;
    bool useDebugWire;

    // Total breakpoints including software
    breakpoint2 bp[MAX_TOTAL_BREAKPOINTS2];

    unsigned char flashCache[MAX_FLASH_PAGE_SIZE];
    unsigned int flashCachePageAddr;
    unsigned char eepromCache[MAX_EEPROM_PAGE_SIZE];
    unsigned int eepromCachePageAddr;

    breakpoint2 softBPcache[MAX_BREAKPOINTS2];

    bool nonbreaking_events[EVT_MAX - EVT_BREAK + 1];

  public:
    jtag2(const char *dev, char *name, bool useDW = false, bool is_dragon = false):
      jtag(dev, name, is_dragon? EMULATOR_DRAGON: EMULATOR_JTAGICE) {
	signedIn = haveHiddenBreakpoint = false;
	command_sequence = 0;
	devdescrlen = sizeof(jtag2_device_desc_type);
	useDebugWire = useDW;
	flashCachePageAddr = (unsigned long)-1;
	eepromCachePageAddr = (unsigned short)-1;
	for (int i = 0; i < MAX_BREAKPOINTS2; i++)
	  softBPcache[i].type = NONE;

	for (int j = 0; j < MAX_TOTAL_BREAKPOINTS2; j++)
	  bp[j] = default_bp;
    };
    virtual ~jtag2(void);

    virtual void initJtagBox(void);
    virtual void initJtagOnChipDebugging(unsigned long bitrate);

    virtual void deleteAllBreakpoints(void);
    virtual bool deleteBreakpoint(unsigned int address, bpType type, unsigned int length);
    virtual bool addBreakpoint(unsigned int address, bpType type, unsigned int length);
    virtual void updateBreakpoints(void);
    virtual bool layoutBreakpoints(void);
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
    virtual bool resetProgram(void);
    virtual bool interruptProgram(void);
    virtual bool resumeProgram(void);
    virtual bool jtagSingleStep(bool useHLL = false);
    virtual bool jtagContinue(void);

    virtual uchar *jtagRead(unsigned long addr, unsigned int numBytes);
    virtual bool jtagWrite(unsigned long addr, unsigned int numBytes, uchar buffer[]);

  private:
    virtual void changeBitRate(int newBitRate);
    virtual void setDeviceDescriptor(jtag_device_def_type *dev);
    virtual bool synchroniseAt(int bitrate);
    virtual void startJtagLink(void);
    virtual void deviceAutoConfig(void);
    virtual void configDaisyChain(void);

    void sendFrame(uchar *command, int commandSize);
    int recvFrame(unsigned char *&msg, unsigned short &seqno);
    int recv(unsigned char *&msg);

    unsigned long b4_to_u32(unsigned char *b) {
      unsigned long l;
      l = (unsigned)b[0];
      l += (unsigned)b[1] << 8;
      l += (unsigned)(unsigned)b[2] << 16;
      l += (unsigned)b[3] << 24;

      return l;
    };

    void u32_to_b4(unsigned char *b, unsigned long l) {
      b[0] = l & 0xff;
      b[1] = (l >> 8) & 0xff;
      b[2] = (l >> 16) & 0xff;
      b[3] = (l >> 24) & 0xff;
    };

    unsigned short b2_to_u16(unsigned char *b) {
      unsigned short l;
      l = (unsigned)b[0];
      l += (unsigned)b[1] << 8;

      return l;
    };

    void u16_to_b2(unsigned char *b, unsigned short l) {
      b[0] = l & 0xff;
      b[1] = (l >> 8) & 0xff;
    };


    bool sendJtagCommand(uchar *command, int commandSize, int &tries,
			 uchar *&msg, int &msgsize, bool verify = true);

    /** Send a command to the jtag, with retries, and return the
	'responseSize' byte &response, response size in
	&responseSize. If retryOnTimeout is true, retry the command
	if no (positive or negative) response arrived in time, abort
	after too many retries.

	If a negative response arrived, return false, otherwise true.

	Caller must delete [] the response.
    **/
    bool doJtagCommand(uchar *command, int  commandSize,
		       uchar *&response, int &responseSize,
		       bool retryOnTimeout = true);

    /** Simplified form of doJtagCommand:
	Send 1-byte command 'cmd' to JTAG ICE, with retries, expecting a
	response that consists only of the status byte which must be
	RSP_OK.
    **/
    void doSimpleJtagCommand(uchar cmd);

    // Miscellaneous
    // -------------

    /** Set JTAG ICE parameter 'item' to 'newValue' **/
    void setJtagParameter(uchar item, uchar *newValue, int valSize);

    /** Return value of JTAG ICE parameter 'item'; caller must delete
        [] resp
    **/
    void getJtagParameter(uchar item, uchar *&resp, int &respSize);

    uchar memorySpace(unsigned long &addr);

    /** debugWire version of the breakpoint updater.
     **/
    void updateBreakpintsDW(void);
};

#endif
