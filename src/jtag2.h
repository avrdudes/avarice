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

class jtag2 : public jtag {
  private:
    unsigned short command_sequence = 0;
    int devdescrlen = sizeof(jtag2_device_desc_type);
    bool signedIn = false;
    bool debug_active = false;
    Debugproto proto;
    bool has_full_xmega_support; // Firmware revision of JTAGICE mkII or AVR Dragon
                                 // allows for full Xmega support (>= 7.x)
    unsigned long cached_pc;
    bool cached_pc_is_valid = false;

    unsigned char flashCache[MAX_FLASH_PAGE_SIZE];
    unsigned int flashCachePageAddr = (unsigned int)-1;
    unsigned char eepromCache[MAX_EEPROM_PAGE_SIZE];
    unsigned int eepromCachePageAddr = (unsigned short)-1;

    bool nonbreaking_events[EVT_MAX - EVT_BREAK + 1];

  public:
    jtag2(const char *dev, const char *name, Debugproto prot = Debugproto::JTAG,
          bool is_dragon = false, bool nsrst = false, bool xmega = false)
        : jtag(dev, name, nsrst, is_dragon ? Emulator::DRAGON : Emulator::JTAGICE), proto(prot) {
        is_xmega = xmega;
    };
    ~jtag2() override;

    void initJtagBox() override;
    void initJtagOnChipDebugging(unsigned long bitrate) override;

    void deleteAllBreakpoints() override;
    void updateBreakpoints() override;
    bool codeBreakpointAt(unsigned int address) override;
    void parseEvents(const char *) override;

    void enableProgramming() override;
    void disableProgramming() override;
    void eraseProgramMemory() override;
    void eraseProgramPage(unsigned long address) override;

    unsigned long getProgramCounter() override;
    void setProgramCounter(unsigned long pc) override;
    void resetProgram(bool possible_nSRST) override;
    void interruptProgram() override;
    void resumeProgram() override;
    void jtagSingleStep() override;
    bool jtagContinue() override;

    uchar *jtagRead(unsigned long addr, unsigned int numBytes) override;
    void jtagWrite(unsigned long addr, unsigned int numBytes, uchar buffer[]) override;
    unsigned int statusAreaAddress() const override {
        return (is_xmega ? 0x3D : 0x5D) + DATA_SPACE_ADDR_OFFSET;
    };
    unsigned int cpuRegisterAreaAddress() const override {
        return is_xmega ? REGISTER_SPACE_ADDR_OFFSET : DATA_SPACE_ADDR_OFFSET;
    }

  private:
    void changeBitRate(int newBitRate) override;
    void setDeviceDescriptor(const jtag_device_def_type &dev) override;
    bool synchroniseAt(int bitrate) override;
    void startJtagLink() override;
    void deviceAutoConfig() override;
    virtual void configDaisyChain();

    void sendFrame(const uchar *command, int commandSize);
    int recvFrame(unsigned char *&msg, unsigned short &seqno);
    int recv(unsigned char *&msg);

    bool sendJtagCommand(const uchar *command, int commandSize, int &tries, uchar *&msg,
                         int &msgsize, bool verify = true);

    /** Send a command to the jtag, with retries, and return the
        'responseSize' byte &response, response size in
        &responseSize. If retryOnTimeout is true, retry the command
        if no (positive or negative) response arrived in time, abort
        after too many retries.

        If a negative response arrived, throw an exception.

        Caller must delete [] the response.
    **/
    void doJtagCommand(const uchar *command, int commandSize, uchar *&response, int &responseSize,
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

    /** Wait until either the ICE or GDB issued an event.  As this is
        the heart of jtagContinue for the mkII, it returns true when a
        breakpoint was reached, and false for GDB input.
     **/
    bool eventLoop();

    /** Expect an ICE event in the input stream.
     **/
    void expectEvent(bool &breakpoint, bool &gdbInterrupt);

    /** Update Xmega breakpoints on target
     **/
    void xmegaSendBPs();
};

#endif
