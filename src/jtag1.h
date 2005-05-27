/*
 *	avarice - The "avarice" program.
 *	Copyright (C) 2001 Scott Finneran
 *	Copyright (C) 2002, 2003, 2004 Intel Corporation
 *	Copyright (C) 2005 Joerg Wunsch
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

  public:
    jtag1(const char *dev): jtag(dev) {};

    virtual void initJtagBox(void);
    virtual void initJtagOnChipDebugging(unsigned long bitrate);

    virtual void deleteAllBreakpoints(void);
    virtual bool deleteBreakpoint(unsigned int address, bpType type, unsigned int length);
    virtual bool addBreakpoint(unsigned int address, bpType type, unsigned int length);
    virtual void updateBreakpoints(bool setCodeBreakpoints);
    virtual bool codeBreakpointAt(unsigned int address);
    virtual bool codeBreakpointBetween(unsigned int start, unsigned int end);
    virtual bool stopAt(unsigned int address);
    virtual void breakOnChangeFlow(void);

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
    virtual bool jtagSingleStep(bool hll = false);
    virtual bool jtagContinue(bool setCodeBreakpoints);

    virtual uchar *jtagRead(unsigned long addr, unsigned int numBytes);
    virtual bool jtagWrite(unsigned long addr, unsigned int numBytes, uchar buffer[]);

  private:
    virtual void changeBitRate(int newBitRate);
    virtual void setDeviceDescriptor(jtag_device_def_type *dev);
    virtual bool synchroniseAt(int bitrate);
    virtual void startJtagLink(void);
    virtual void deviceAutoConfig(void);

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
