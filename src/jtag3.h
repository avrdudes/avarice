/*
 *	avarice - The "avarice" program.
 *	Copyright (C) 2012 Joerg Wunsch
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
 * This file extends the generic "jtag" class for the JTAGICE3 protocol.
 *
 * $Id$
 */

#ifndef JTAG3_H
#define JTAG3_H

#include "jtag.h"

enum jtag3consts
{
    SCOPE_INFO = 0x00,
    SCOPE_GENERAL = 0x01,
    SCOPE_AVR_ISP = 0x11,
    SCOPE_AVR = 0x12,

    /* Info scope */
    CMD3_GET_INFO = 0x00,

    /* byte after GET_INFO is always 0, next is: */
    CMD3_INFO_NAME = 0x80,      /* JTAGICE3 */
    CMD3_INFO_SERIAL = 0x81,    /* J3xxxxxxxxxx */

    /* Generic scope */
    CMD3_SET_PARAMETER = 0x01,
    CMD3_GET_PARAMETER = 0x02,
    CMD3_SIGN_ON = 0x10,
    CMD3_SIGN_OFF = 0x11,       /* takes one parameter? */

    /* AVR ISP scope: no commands of its own */

    /* AVR scope */
    //CMD3_SET_PARAMETER = 0x01,
    //CMD3_GET_PARAMETER = 0x02,
    //CMD3_SIGN_ON = 0x10, /* an additional signon/-off pair */
    //CMD3_SIGN_OFF = 0x11,
    CMD3_DEVICE_ID = 0x12,
    CMD3_START_DEBUG = 0x13,
    CMD3_STOP_DEBUG = 0x14,
    CMD3_ENTER_PROGMODE = 0x15,
    CMD3_LEAVE_PROGMODE = 0x16,
    CMD3_MONCON_DISABLE = 0x17,
    CMD3_ERASE_MEMORY = 0x20,
    CMD3_READ_MEMORY = 0x21,
    CMD3_WRITE_MEMORY = 0x23,
    CMD3_RESET = 0x30,
    CMD3_STOP = 0x31,
    CMD3_GO = 0x32,
    CMD3_RUN_TO = 0x33, /* followed by 0x00, word address to stop at*/
    CMD3_STEP = 0x34,
    CMD3_READ_PC = 0x35,
    CMD3_WRITE_PC = 0x36,
    CMD3_SET_BP = 0x40,
    CMD3_CLEAR_BP = 0x41,
    CMD3_SET_BP_XMEGA = 0x42,   /* very similar to mkII CMND_SET_BREAK_XMEGA */
    CMD3_SET_SOFT_BP = 0x43,
    CMD3_CLEAR_SOFT_BP = 0x44,
    CMD3_CLEANUP = 0x45,

    /* ICE responses */
    RSP3_OK = 0x80,
    RSP3_INFO = 0x81,
    RSP3_PC = 0x83,
    RSP3_DATA = 0x84,
    RSP3_FAILED = 0xA0,

    RSP3_STATUS_MASK = 0xE0,

    /* possible failure codes that could be appended to RSP3_FAILED: */
    RSP3_FAIL_DEBUGWIRE = 0x10,
    RSP3_FAIL_PDI = 0x1B,
    RSP3_FAIL_NO_ANSWER = 0x20,
    RSP3_FAIL_NO_TARGET_POWER = 0x22,
    RSP3_FAIL_WRONG_MODE = 0x32,    /* CPU running vs. stopped */
    RSP3_FAIL_UNSUPP_MEMORY = 0x34, /* unsupported memory type */
    RSP3_FAIL_WRONG_LENGTH = 0x35,  /* wrong lenth for mem access */
    RSP3_FAIL_NOT_UNDERSTOOD = 0x91,

    /* ICE events */
    EVT3_BREAK = 0x40,          /* AVR scope */
    EVT3_IDR = 0x41,            /* AVR scope */
    EVT3_SLEEP = 0x11,          /* General scope, also wakeup */
    EVT3_POWER = 0x10,          /* General scope */

    /* memory types */
    /* not defined here, identical to generic definitions in jtag.h */
    // MTYPE_SRAM = 0x20,      /* target's SRAM or [ext.] IO registers */
    // MTYPE_EEPROM = 0x22,    /* EEPROM, what way? */
    // MTYPE_SPM = 0xA0,       /* flash through LPM/SPM */
    // MTYPE_FLASH_PAGE = 0xB0, /* flash in programming mode */
    // MTYPE_EEPROM_PAGE = 0xB1, /* EEPROM in programming mode */
    // MTYPE_FUSE_BITS = 0xB2, /* fuse bits in programming mode */
    // MTYPE_LOCK_BITS = 0xB3, /* lock bits in programming mode */
    // MTYPE_SIGN_JTAG = 0xB4, /* signature in programming mode */
    // MTYPE_OSCCAL_BYTE = 0xB5, /* osccal cells in programming mode */
    // MTYPE_FLASH = 0xc0, /* xmega (app.) flash - undocumented in AVR067 */
    // MTYPE_BOOT_FLASH = 0xc1, /* xmega boot flash - undocumented in AVR067 */
    // MTYPE_USERSIG = 0xc5, /* xmega user signature - undocumented in AVR067 */
    // MTYPE_PRODSIG = 0xc6, /* xmega production signature - undocumented in AVR067 */

    /*
     * Parameters are divided into sections, where the section number
     * precedes each parameter address.  There are distinct parameter
     * sets for generic and AVR scope.
     */
    PARM3_HW_VER = 0x00,        /* section 0, generic scope, 1 byte */
    PARM3_FW_MAJOR = 0x01,      /* section 0, generic scope, 1 byte */
    PARM3_FW_MINOR = 0x02,      /* section 0, generic scope, 1 byte */
    PARM3_FW_RELEASE = 0x03,    /* section 0, generic scope, 1 byte;
                                 * always asked for by Atmel Studio,
                                 * but never displayed there */
    PARM3_VTARGET = 0x00,       /* section 1, generic scope, 2 bytes,
                                 * in millivolts */
    PARM3_DEVICEDESC = 0x00,    /* section 2, memory etc. configuration,
                                 * 31 bytes for tiny/mega AVR, 47 bytes
                                 * for Xmega; is also used in command
                                 * 0x36 in JTAGICEmkII, starting with
                                 * firmware 7.x */

    PARM3_ARCH = 0x00,       /* section 0, AVR scope, 1 byte */
    PARM3_ARCH_TINY = 1,     /* also small megaAVR with ISP/DW only */
    PARM3_ARCH_MEGA = 2,
    PARM3_ARCH_XMEGA = 3,
    PARM3_ARCH_UPDI = 5,

    PARM3_SESS_PURPOSE = 0x01, /* section 0, AVR scope, 1 byte */
    PARM3_SESS_PROGRAMMING = 1,
    PARM3_SESS_DEBUGGING = 2,

    PARM3_CONNECTION = 0x00, /* section 1, AVR scope, 1 byte */
    PARM3_CONN_ISP = 1,
    PARM3_CONN_JTAG = 4,
    PARM3_CONN_DW = 5,
    PARM3_CONN_PDI = 6,
	  PARM3_CONN_UPDI = 8,

    PARM3_JTAGCHAIN = 0x01,     /* JTAG chain info, AVR scope (units
                                 * before/after, bits before/after),
                                 * 4 bytes */

    PARM3_CLK_MEGA_PROG = 0x20, /* section 1, AVR scope, 2 bytes (kHz) */
    PARM3_CLK_MEGA_DEBUG = 0x21, /* section 1, AVR scope, 2 bytes (kHz) */
    PARM3_CLK_XMEGA_JTAG = 0x30, /* section 1, AVR scope, 2 bytes (kHz) */
    PARM3_CLK_XMEGA_PDI = 0x31, /* section 1, AVR scope, 2 bytes (kHz) */

    PARM3_TIMERS_RUNNING = 0x00, /* section 3, AVR scope, 1 byte */


    /* Xmega erase memory types, for CMND_XMEGA_ERASE */
    XMEGA_ERASE_CHIP = 0x00,
    XMEGA_ERASE_APP = 0x01,
    XMEGA_ERASE_BOOT = 0x02,
    XMEGA_ERASE_EEPROM = 0x03,
    XMEGA_ERASE_APP_PAGE = 0x04,
    XMEGA_ERASE_BOOT_PAGE = 0x05,
    XMEGA_ERASE_EEPROM_PAGE = 0x06,
    XMEGA_ERASE_USERSIG = 0x07,


    MAX_MESSAGE_SIZE_JTAGICE3 = 512,
};


class jtag3: public jtag
{
  private:
    unsigned short command_sequence;
    bool signedIn;
    bool debug_active;
    enum debugproto proto;
    unsigned long cached_pc;
    bool cached_pc_is_valid;
    bool is_edbg;

    unsigned char flashCache[MAX_FLASH_PAGE_SIZE];
    unsigned int flashCachePageAddr;
    unsigned char eepromCache[MAX_EEPROM_PAGE_SIZE];
    unsigned int eepromCachePageAddr;

    unsigned long appsize;
    unsigned int device_id;

    unsigned char *cached_event;

  public:
    jtag3(const char *dev, char *name, enum debugproto prot = PROTO_JTAG,
	  bool nsrst = false,
          bool xmega = false,
          bool edbg = false):
        jtag(dev, name, edbg? EMULATOR_EDBG: EMULATOR_JTAGICE3) {
	signedIn = debug_active = false;
	command_sequence = 0;
	proto = prot;
	apply_nSRST = nsrst;
        is_xmega = xmega;
	xmega_n_bps = 0;
	flashCachePageAddr = (unsigned int)-1;
	eepromCachePageAddr = (unsigned short)-1;

	for (int j = 0; j < MAX_TOTAL_BREAKPOINTS2; j++)
	  bp[j] = default_bp;
        cached_pc_is_valid = false;
        appsize = 0;
        device_id = 0;
        cached_event = NULL;
        is_edbg = edbg;
    };
    virtual ~jtag3(void);

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
    virtual bool synchroniseAt(int bitrate);
    virtual void setDeviceDescriptor(jtag_device_def_type *dev);
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


    bool sendJtagCommand(uchar *command, int commandSize,
                         const char *name,
			 uchar *&msg, int &msgsize);

    /** Send a command to the jtag, and return the
	'responseSize' byte &response, response size in
	&responseSize.

	If a negative response arrived, throw an exception.

	Caller must delete [] the response.
    **/
    void doJtagCommand(uchar *command, int  commandSize,
                       const char *name,
		       uchar *&response, int &responseSize);

    /** Simplified form of doJtagCommand:
	Send 1-byte command 'cmd' to JTAG ICE, expecting a
	response that consists only of the status byte which must be
	RSP_OK.
    **/
    void doSimpleJtagCommand(uchar cmd, const char *name,
                             uchar scope = SCOPE_AVR);

    // Miscellaneous
    // -------------

    /** Set JTAG ICE parameter 'item' to 'newValue' **/
    void setJtagParameter(uchar scope, uchar section, uchar item,
                          uchar *newValue, int valSize);

    /** Return value of JTAG ICE parameter 'item'; caller must delete
        [] resp
    **/
    void getJtagParameter(uchar scope, uchar section, uchar item, int length,
                          uchar *&resp);

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

class jtag3_io_exception: public jtag_io_exception
{
  public:
    jtag3_io_exception(): jtag_io_exception(0) {}
    jtag3_io_exception(unsigned int code);

    unsigned int get_response(void) { return response_code; }
};

#endif
