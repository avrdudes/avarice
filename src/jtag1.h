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
    // JTAG parameters
    JTAG_P_BITRATE = 'b',
    JTAG_P_SW_VERSION = 0x7b,
    JTAG_P_HW_VERSION = 0x7a,
    JTAG_P_IREG_HIGH = 0x81,
    JTAG_P_IREG_LOW = 0x82,
    JTAG_P_OCD_VTARGET = 0x84,
    JTAG_P_OCD_BREAK_CAUSE = 0x85,
    JTAG_P_CLOCK = 0x86,
    JTAG_P_EXTERNAL_RESET = 0x8b,      /* W */
    JTAG_P_FLASH_PAGESIZE_LOW = 0x88,  /* W */
    JTAG_P_FLASH_PAGESIZE_HIGH = 0x89, /* W */
    JTAG_P_EEPROM_PAGESIZE = 0x8a,     /* W */
    JTAG_P_TIMERS_RUNNING = 0xa0,
    JTAG_P_BP_FLOW = 0xa1,
    JTAG_P_BP_X_HIGH = 0xa2,
    JTAG_P_BP_X_LOW = 0xa3,
    JTAG_P_BP_Y_HIGH = 0xa4,
    JTAG_P_BP_Y_LOW = 0xa5,
    JTAG_P_BP_MODE = 0xa6,
    JTAG_P_JTAGID_BYTE0 = 0xa7, /* R */
    JTAG_P_JTAGID_BYTE1 = 0xa8, /* R */
    JTAG_P_JTAGID_BYTE2 = 0xa9, /* R */
    JTAG_P_JTAGID_BYTE3 = 0xaa, /* R */
    JTAG_P_UNITS_BEFORE = 0xab, /* W */
    JTAG_P_UNITS_AFTER = 0xac,  /* W */
    JTAG_P_BIT_BEFORE = 0xad,   /* W */
    JTAG_P_BIT_AFTER = 0xae,    /* W */
    JTAG_P_PSB0_LOW = 0xaf,     /* W */
    JTAG_P_PSBO_HIGH = 0xb0,    /* W */
    JTAG_P_PSB1_LOW = 0xb1,     /* W */
    JTAG_P_PSB1_HIGH = 0xb2,    /* W */
    JTAG_P_MCU_MODE = 0xb3,     /* R */

    // Set JTAG bitrate to 1MHz
    // ff: 1MHz, fe: 500kHz, fd: 250khz, fb: 125Khz
    // JTAG bitrates
    JTAG_BITRATE_1_MHz = 0xff,
    JTAG_BITRATE_500_KHz = 0xfe,
    JTAG_BITRATE_250_KHz = 0xfd,
    JTAG_BITRATE_125_KHz = 0xfb,

    // We distinguish the total possible breakpoints and those for each type
    // (code or data) - see above
    MAX_BREAKPOINTS_CODE = 4,
    MAX_BREAKPOINTS_DATA = 2,
    MAX_BREAKPOINTS = 4
};

class jtag1 : public Jtag {
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

    /*
     * Max message size we are willing to accept.  Prevents us from trying
     * to allocate too much VM in case we received a nonsensical packet
     * length.  We have to allocate the buffer as soon as we've got the
     * length information (and thus have to trust that information by that
     * time at first), as the final CRC check can only be done once the
     * entire packet came it.
     */
    static inline constexpr size_t MAX_MESSAGE = 100000;

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
    jtag1(Emulator emul, const char *dev, std::string_view expected_dev, bool nsrst)
        : Jtag(emul, dev, expected_dev, nsrst){};

    void initJtagBox() override;
    void initJtagOnChipDebugging(unsigned long bitrate) override;

    void deleteAllBreakpoints() override;
    bool deleteBreakpoint(unsigned int address, BreakpointType type, unsigned int length) override;
    bool addBreakpoint(unsigned int address, BreakpointType type, unsigned int length) override;
    void updateBreakpoints() override;
    bool codeBreakpointAt(unsigned int address) override;

    void enableProgramming() override;
    void disableProgramming() override;
    void eraseProgramMemory() override;

    unsigned long getProgramCounter() override;
    void setProgramCounter(unsigned long pc) override;
    void resetProgram(bool possible_nSRST) override;
    void interruptProgram() override;
    void resumeProgram() override;
    void jtagSingleStep() override;
    bool jtagContinue() override;

    uchar *jtagRead(unsigned long addr, unsigned int numBytes) override;

    void jtagWrite(unsigned long addr, unsigned int numBytes, uchar buffer[]) override;

    [[nodiscard]] unsigned int statusAreaAddress() const override {
        /* no Xmega handling in JTAG ICE mkI */
        return 0x5D + DATA_SPACE_ADDR_OFFSET;
    };
    [[nodiscard]] unsigned int cpuRegisterAreaAddress() const override {
        /* no Xmega handling in JTAG ICE mkI */
        return DATA_SPACE_ADDR_OFFSET;
    }

  private:
    void changeBitRate(int newBitRate);
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
