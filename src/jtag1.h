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

class jtag1 : public jtag {
  public:
    // Responses from JTAG ICE
    enum class Resp : uchar {
        OK = 'A',
        BREAK = 'B',
        INFO = 'G',
        FAILED = 'F',
        SYNC_ERROR = 'E',
        SLEEP = 'H',
        POWER = 'I',
    };

  private:
    /** Decode 3-byte big-endian address **/
    static unsigned long decodeAddress(const uchar *buf) { return buf[0] << 16 | buf[1] << 8 | buf[2]; };

    /** Encode 3-byte big-endian address **/
    static void encodeAddress(uchar *buffer, unsigned long x) {
        buffer[0] = x >> 16;
        buffer[1] = x >> 8;
        buffer[2] = x;
    };

    struct breakpoint {
        unsigned int address = 0;
        BreakpointType type = BreakpointType::NONE;
    };

    breakpoint bpCode[MAX_BREAKPOINTS_CODE];
    breakpoint bpData[MAX_BREAKPOINTS_DATA];
    int numBreakpointsCode = 0;
    int numBreakpointsData = 0;

  public:
    jtag1(Emulator emul, const char *dev, const char *name, bool nsrst) : jtag(emul, dev, name, nsrst){};

    void initJtagBox() override;
    void initJtagOnChipDebugging(unsigned long bitrate) override;

    void deleteAllBreakpoints() override;
    bool deleteBreakpoint(unsigned int address, BreakpointType type, unsigned int length) override;
    bool addBreakpoint(unsigned int address, BreakpointType type, unsigned int length) override;
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
        /* no Xmega handling in JTAG ICE mkI */
        return 0x5D + DATA_SPACE_ADDR_OFFSET;
    };
    unsigned int cpuRegisterAreaAddress() const override {
        /* no Xmega handling in JTAG ICE mkI */
        return DATA_SPACE_ADDR_OFFSET;
    }

  private:
    void changeBitRate(int newBitRate) override;
    void setDeviceDescriptor(const jtag_device_def_type &dev) override;
    bool synchroniseAt(int bitrate) override;
    void startJtagLink() override;
    void deviceAutoConfig() override;
    virtual void configDaisyChain();

    std::unique_ptr<uchar[]> getJtagResponse(int responseSize);

    enum SendResult { send_failed, send_ok, mcu_data };

    SendResult sendJtagCommand(const uchar *command, int commandSize, int &tries);
    bool checkForEmulator();

    /** Send a command to the jtag, with retries, and return the 'responseSize'
        byte response. Aborts avarice in case of to many failed retries.

        Returns a dynamically allocated buffer containing the reponse (caller
        must free)
    **/
    std::unique_ptr<uchar[]> doJtagCommand(const uchar *command, int commandSize, int responseSize);

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
