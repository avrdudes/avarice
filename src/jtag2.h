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
 */

#ifndef JTAG2_H
#define JTAG2_H

#include "jtag.h"

/*
 * Communication with the JTAG ICE works in frames.
 *
 * Frame format:
 *
 *  +---------------------------------------------------------------+
 *  |   0   |  1  .  2  |  3 . 4 . 5 . 6  |   7   | ... | N-1 .  N  |
 *  |       |           |                 |       |     |           |
 *  | start | LSB   MSB | LSB ....... MSB | token | msg | LSB   MSB |
 *  | 0x1B  | sequence# | message size    | 0x0E  |     |   CRC16   |
 *  +---------------------------------------------------------------+
 *
 * Each request message will be returned by a response with a matching
 * sequence #.  Sequence # 0xffff is reserved for asynchronous event
 * notifications that will be sent by the ICE without a request
 * message (e.g. when the target hit a breakpoint).
 *
 * The message size excludes the framing overhead (10 bytes).
 *
 * The first byte of the message is always the request or response
 * code, which is roughly classified as:
 *
 * . Messages (commands) use 0x00 through 0x3f.  (The documentation
 *   claims that messages start at 0x01, but actually CMND_SIGN_OFF is
 *   0x00.)
 * . Internal commands use 0x40 through 0x7f (not documented).
 * . Success responses use 0x80 through 0x9f.
 * . Failure responses use 0xa0 through 0xbf.
 * . Events use 0xe0 through 0xff.
 */
#define MESSAGE_START 0x1b
#define TOKEN 0x0e

/* ICE command codes */
#define CMND_CHIP_ERASE 0x13
#define CMND_CLEAR_EVENTS 0x22
#define CMND_CLR_BREAK 0x1A
#define CMND_ENTER_PROGMODE 0x14
#define CMND_ERASEPAGE_SPM 0x0D
#define CMND_FORCED_STOP 0x0A
#define CMND_GET_BREAK 0x12
#define CMND_GET_PARAMETER 0x03
#define CMND_GET_SIGN_ON 0x01
#define CMND_GET_SYNC 0x0f
#define CMND_GO 0x08
#define CMND_LEAVE_PROGMODE 0x15
#define CMND_READ_MEMORY 0x05
#define CMND_READ_PC 0x07
#define CMND_RESET 0x0B
#define CMND_RESTORE_TARGET 0x23
#define CMND_RUN_TO_ADDR 0x1C
#define CMND_SELFTEST 0x10
#define CMND_SET_BREAK 0x11
#define CMND_SET_DEVICE_DESCRIPTOR 0x0C
#define CMND_SET_N_PARAMETERS 0x16
#define CMND_SET_PARAMETER 0x02
#define CMND_SIGN_OFF 0x00
#define CMND_SINGLE_STEP 0x09
#define CMND_SPI_CMD 0x1D
#define CMND_WRITE_MEMORY 0x04
#define CMND_WRITE_PC 0x06
#define CMND_XMEGA_ERASE 0x34

/* ICE responses */
#define RSP_DEBUGWIRE_SYNC_FAILED 0xAC
#define RSP_FAILED 0xA0
#define RSP_GET_BREAK 0x83
#define RSP_ILLEGAL_BREAKPOINT 0xA8
#define RSP_ILLEGAL_COMMAND 0xAA
#define RSP_ILLEGAL_EMULATOR_MODE 0xA4
#define RSP_ILLEGAL_JTAG_ID 0xA9
#define RSP_ILLEGAL_MCU_STATE 0xA5
#define RSP_ILLEGAL_MEMORY_TYPE 0xA2
#define RSP_ILLEGAL_MEMORY_RANGE 0xA3
#define RSP_ILLEGAL_PARAMETER 0xA1
#define RSP_ILLEGAL_POWER_STATE 0xAD
#define RSP_ILLEGAL_VALUE 0xA6
#define RSP_MEMORY 0x82
#define RSP_NO_TARGET_POWER 0xAB
#define RSP_OK 0x80
#define RSP_PARAMETER 0x81
#define RSP_PC 0x84
#define RSP_SELFTEST 0x85
#define RSP_SET_N_PARAMETERS 0xA7
#define RSP_SIGN_ON 0x86
#define RSP_SPI_DATA 0x88

/* ICE events */
enum Event {
    EVT_NA = 0x00,
    EVT_BREAK = 0xE0,
    EVT_RUN = 0xE1,
    EVT_ERROR_PHY_FORCE_BREAK_TIMEOUT = 0xE2,
    EVT_ERROR_PHY_RELEASE_BREAK_TIMEOUT = 0xE3,
    EVT_TARGET_POWER_ON = 0xE4,
    EVT_TARGET_POWER_OFF = 0xE5,
    EVT_DEBUG = 0xE6,
    EVT_EXT_RESET = 0xE7,
    EVT_TARGET_SLEEP = 0xE8,
    EVT_TARGET_WAKEUP = 0xE9,
    EVT_ICE_POWER_ERROR_STATE = 0xEA,
    EVT_ICE_POWER_OK = 0xEB,
    EVT_IDR_DIRTY = 0xEC,
    EVT_ERROR_PHY_MAX_BIT_LENGTH_DIFF = 0xED,
    EVT_NONE = 0xEF,
    EVT_ERROR_PHY_SYNC_TIMEOUT = 0xF0,
    EVT_PROGRAM_BREAK = 0xF1,
    EVT_PDSB_BREAK = 0xF2,
    EVT_PDSMB_BREAK = 0xF3,
    EVT_ERROR_PHY_SYNC_TIMEOUT_BAUD = 0xF4,
    EVT_ERROR_PHY_SYNC_OUT_OF_RANGE = 0xF5,
    EVT_ERROR_PHY_SYNC_WAIT_TIMEOUT = 0xF6,
    EVT_ERROR_PHY_RECEIVE_TIMEOUT = 0xF7,
    EVT_ERROR_PHY_RECEIVED_BREAK = 0xF8,
    EVT_ERROR_PHY_OPT_RECEIVE_TIMEOUT = 0xF9,
    EVT_ERROR_PHY_OPT_RECEIVED_BREAK = 0xFA,
    EVT_RESULT_PHY_NO_ACTIVITY = 0xFB,
    EVT_MAX = 0xFF
};

/* memory types for CMND_{READ,WRITE}_MEMORY */
#define MTYPE_IO_SHADOW 0x30   /* cached IO registers? */
#define MTYPE_SRAM 0x20        /* target's SRAM or [ext.] IO registers */
#define MTYPE_EEPROM 0x22      /* EEPROM, what way? */
#define MTYPE_EVENT 0x60       /* ICE event memory */
#define MTYPE_SPM 0xA0         /* flash through LPM/SPM */
#define MTYPE_FLASH_PAGE 0xB0  /* flash in programming mode */
#define MTYPE_EEPROM_PAGE 0xB1 /* EEPROM in programming mode */
#define MTYPE_FUSE_BITS 0xB2   /* fuse bits in programming mode */
#define MTYPE_LOCK_BITS 0xB3   /* lock bits in programming mode */
#define MTYPE_SIGN_JTAG 0xB4   /* signature in programming mode */
#define MTYPE_OSCCAL_BYTE 0xB5 /* osccal cells in programming mode */
#define MTYPE_CAN 0xB6         /* CAN mailbox */

/* (some) ICE parameters, for CMND_{GET,SET}_PARAMETER */
#define PAR_HW_VERSION 0x01
#define PAR_FW_VERSION 0x02
#define PAR_EMULATOR_MODE 0x03
#define EMULATOR_MODE_DEBUGWIRE 0x00
#define EMULATOR_MODE_JTAG 0x01
#define EMULATOR_MODE_UNKNOWN 0x02
#define EMULATOR_MODE_SPI 0x03
#define PAR_IREG 0x04
#define PAR_BAUD_RATE 0x05
#define PAR_BAUD_2400 0x01
#define PAR_BAUD_4800 0x02
#define PAR_BAUD_9600 0x03
#define PAR_BAUD_19200 0x04 /* default */
#define PAR_BAUD_38400 0x05
#define PAR_BAUD_57600 0x06
#define PAR_BAUD_115200 0x07
#define PAR_BAUD_14400 0x08
#define PAR_OCD_VTARGET 0x06
#define PAR_OCD_JTAG_CLK 0x07
#define PAR_OCD_BREAK_CAUSE 0x08
#define PAR_TIMERS_RUNNING 0x09
#define PAR_BREAK_ON_CHANGE_FLOW 0x0A
#define PAR_BREAK_ADDR1 0x0B
#define PAR_BREAK_ADDR2 0x0C
#define PAR_COMBBREAKCTRL 0x0D
#define PAR_JTAGID 0x0E
#define PAR_UNITS_BEFORE 0x0F
#define PAR_UNITS_AFTER 0x10
#define PAR_BIT_BEFORE 0x11
#define PAR_BIT_ATER 0x12
#define PAR_EXTERNAL_RESET 0x13
#define PAR_FLASH_PAGE_SIZE 0x14
#define PAR_EEPROM_PAGE_SIZE 0x15
#define PAR_UNUSED1 0x16
#define PAR_PSB0 0x17
#define PAR_PSB1 0x18
#define PAR_PROTOCOL_DEBUG_EVENT 0x19
#define PAR_MCU_STATE 0x1A
#define STOPPED 0x00
#define RUNNING 0x01
#define PROGRAMMING 0x02
#define PAR_DAISY_CHAIN_INFO 0x1B
#define PAR_BOOT_ADDRESS 0x1C
#define PAR_TARGET_SIGNATURE 0x1D
#define PAR_DEBUGWIRE_BAUDRATE 0x1E
#define PAR_PROGRAM_ENTRY_POINT 0x1F
#define PAR_PACKET_PARSING_ERRORS 0x40
#define PAR_VALID_PACKETS_RECEIVED 0x41
#define PAR_INTERCOMMUNICATION_TX_FAILURES 0x42
#define PAR_INTERCOMMUNICATION_RX_FAILURES 0x43
#define PAR_CRC_ERRORS 0x44
#define PAR_POWER_SOURCE 0x45
#define POWER_EXTERNAL 0x00
#define POWER_USB 0x01
#define PAR_CAN_FLAG 0x22
#define DONT_READ_CAN_MAILBOX 0x00
#define READ_CAN_MAILBOX 0x01
#define PAR_ENABLE_IDR_IN_RUN_MODE 0x23
#define ACCESS_OSCCAL 0x00
#define ACCESS_IDR 0x01
#define PAR_ALLOW_PAGEPROGRAMMING_IN_SCANCHAIN 0x24
#define PAGEPROG_NOT_ALLOWED 0x00
#define PAGEPROG_ALLOWED 0x01

class Jtag2 : public Jtag {
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

    void MarkNonBreaking(Event event) { nonbreaking_events[event - EVT_BREAK] = true; }

    [[nodiscard]] bool IsBreakingEvent(Event event) const {
        return !nonbreaking_events[event - EVT_BREAK];
    }

  public:
    /*
     * Max message size we are willing to accept.  Prevents us from trying
     * to allocate too much VM in case we received a nonsensical packet
     * length.  We have to allocate the buffer as soon as we've got the
     * length information (and thus have to trust that information by that
     * time at first), as the final CRC check can only be done once the
     * entire packet came it.
      */
    static inline constexpr size_t MAX_MESSAGE = 100000;

    Jtag2(Emulator emul, const char *dev, std::string_view expected_dev, Debugproto prot, bool nsrst, bool xmega)
        : Jtag(emul, dev, expected_dev, nsrst, xmega), proto(prot) {};
    ~Jtag2() override;

    void initJtagBox() override;
    void initJtagOnChipDebugging(unsigned long bitrate) override;

    void deleteAllBreakpoints() override;
    void updateBreakpoints() override;
    bool codeBreakpointAt(unsigned int address) override;
    void parseEvents(std::string_view const &) override;

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
        return (is_xmega ? 0x3D : 0x5D) + DATA_SPACE_ADDR_OFFSET;
    };
    [[nodiscard]] unsigned int cpuRegisterAreaAddress() const override {
        return is_xmega ? REGISTER_SPACE_ADDR_OFFSET : DATA_SPACE_ADDR_OFFSET;
    }

  private:
    void changeBitRate(int newBitRate);
    void setDeviceDescriptor(const jtag_device_def_type &dev) override;
    bool synchroniseAt(int bitrate) override;
    void startJtagLink() override;
    void deviceAutoConfig() override;
    void configDaisyChain();

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
    void setJtagParameter(uchar item, const uchar *newValue, int valSize);

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

class jtag2_io_exception : public jtag_exception {
  protected:
    unsigned int response_code = 0;

  public:
    jtag2_io_exception() : jtag_exception("Unknown JTAG response exception") {}
    explicit jtag2_io_exception(unsigned int code);

    [[nodiscard]] unsigned int get_response() const { return response_code; }
};

#endif
