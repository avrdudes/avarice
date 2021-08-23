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

class jtag2: public jtag
{
  private:
    unsigned short command_sequence;
    int devdescrlen;
    bool signedIn;
    bool debug_active;
    enum debugproto proto;
    bool has_full_xmega_support;       // Firmware revision of JTAGICE mkII or AVR Dragon
                                       // allows for full Xmega support (>= 7.x)
    unsigned long cached_pc;
    bool cached_pc_is_valid;

    unsigned char flashCache[MAX_FLASH_PAGE_SIZE];
    unsigned int flashCachePageAddr;
    unsigned char eepromCache[MAX_EEPROM_PAGE_SIZE];
    unsigned int eepromCachePageAddr;

    bool nonbreaking_events[EVT_MAX - EVT_BREAK + 1];

  public:
    jtag2(const char *dev, char *name, enum debugproto prot = PROTO_JTAG,
	  bool is_dragon = false, bool nsrst = false,
          bool xmega = false):
      jtag(dev, name, is_dragon? EMULATOR_DRAGON: EMULATOR_JTAGICE) {
	signedIn = debug_active = false;
	command_sequence = 0;
	devdescrlen = sizeof(jtag2_device_desc_type);
	proto = prot;
	apply_nSRST = nsrst;
        is_xmega = xmega;
	xmega_n_bps = 0;
	flashCachePageAddr = (unsigned int)-1;
	eepromCachePageAddr = (unsigned short)-1;

	for (int j = 0; j < MAX_TOTAL_BREAKPOINTS2; j++)
	  bp[j] = default_bp;
        cached_pc_is_valid = false;
    };
    virtual ~jtag2(void);

    virtual void initJtagBox(void);
    virtual void initJtagOnChipDebugging(unsigned long bitrate);

    virtual void deleteAllBreakpoints(void);
    virtual void updateBreakpoints(void);
    virtual bool codeBreakpointAt(unsigned int address);
    virtual void parseEvents(const char *);

    virtual void enableProgramming(void);
    virtual void disableProgramming(void);
    virtual void eraseProgramMemory(void);
    virtual void eraseProgramPage(unsigned long address);
    virtual void downloadToTarget(const char* filename, bool program, bool verify);

    virtual unsigned long getProgramCounter(void);
    virtual void setProgramCounter(unsigned long pc);
    virtual void resetProgram(bool ignored = false);
    virtual void interruptProgram(void);
    virtual void resumeProgram(void);
    virtual void jtagSingleStep(void);
    virtual bool jtagContinue(void);

    virtual uchar *jtagRead(unsigned long addr, unsigned int numBytes);
    virtual void jtagWrite(unsigned long addr, unsigned int numBytes, uchar buffer[]);
    virtual unsigned int statusAreaAddress(void) const {
        return (is_xmega? 0x3D: 0x5D) + DATA_SPACE_ADDR_OFFSET;
    };
    virtual unsigned int cpuRegisterAreaAddress(void) const {
        return is_xmega? REGISTER_SPACE_ADDR_OFFSET: DATA_SPACE_ADDR_OFFSET;
    }

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

	If a negative response arrived, throw an exception.

	Caller must delete [] the response.
    **/
    void doJtagCommand(uchar *command, int  commandSize,
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

    /** Wait until either the ICE or GDB issued an event.  As this is
	the heart of jtagContinue for the mkII, it returns true when a
	breakpoint was reached, and false for GDB input.
     **/
    bool eventLoop(void);

    /** Expect an ICE event in the input stream.
     **/
    void expectEvent(bool &breakpoint, bool &gdbInterrupt);

    /** Update Xmega breakpoints on target
     **/
    void xmegaSendBPs(void);
};

#endif
