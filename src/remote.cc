/*
 *	avarice - The "avarice" program.
 *	Copyright (C) 2001 Scott Finneran
 *      Copyright (C) 2002 Intel Corporation
 *	Copyright (C) 2005 Joerg Wunsch
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
 * This file contains functions for interfacing with the GDB remote protocol.
 *
 * $Id$
 */

#include <cctype>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include <string_view>
#include <string>

#include "avarice.h"
#include "jtag.h"
#include "remote.h"

enum {
    /** BUFMAX defines the maximum number of characters in
     * inbound/outbound buffers at least NUMREGBYTES*2 are needed for
     * register packets
     */
    BUFMAX = 400,
    NUMREGS = 32 /* + 1 + 1 + 1*/, /* SREG, FP, PC */
    SREG = 32,
    SP = 33,
    PC = 34,

    // Number of bytes of registers.  See GDB gdb/avr-tdep.c
    NUMREGBYTES = (NUMREGS + 1 + 2 + 4),

    // max number of bytes in a "monitor" command request
    MONMAX = 100,
};

static char remcomInBuffer[BUFMAX];
static char remcomOutBuffer[BUFMAX];

static void ok();
static void error(int n);

int gdbFileDescriptor = -1;

void setGdbFile(int fd) {
    gdbFileDescriptor = fd;
    int ret = fcntl(gdbFileDescriptor, F_SETFL, O_NONBLOCK);
    if (ret < 0)
        throw jtag_exception();
}

static void waitForGdbOutput() {
    fd_set writefds;

    FD_ZERO(&writefds);
    FD_SET(gdbFileDescriptor, &writefds);

    int numfds = select(gdbFileDescriptor + 1, nullptr, &writefds, nullptr, nullptr);
    if (numfds < 0)
        throw jtag_exception();
}

/** Send single char to gdb. Abort in case of problem. **/
static void putDebugChar(char c) {
    // This loop busy waits when it cannot write to gdb.
    // But that shouldn't happen
    for (;;) {
        const auto ret = write(gdbFileDescriptor, &c, 1);

        if (ret == 1)
            return;

        if (ret == 0) // this shouldn't happen?
            throw jtag_exception();

        if (errno != EAGAIN && ret < 0)
            throw jtag_exception();

        waitForGdbOutput();
    }
}

static void waitForGdbInput() {
    fd_set readfds;

    FD_ZERO(&readfds);
    FD_SET(gdbFileDescriptor, &readfds);

    int numfds = select(gdbFileDescriptor + 1, &readfds, nullptr, nullptr, nullptr);
    if (numfds < 0)
        throw jtag_exception();
}

/** Return single char read from gdb. Abort in case of problem,
    exit cleanly if EOF detected on gdbFileDescriptor. **/
unsigned char getDebugChar() {
    uchar c = 0;
    ssize_t result;

    do {
        waitForGdbInput();
        result = read(gdbFileDescriptor, &c, 1);
    } while (result < 0 && errno == EAGAIN);

    if (result < 0)
        throw jtag_exception();

    if (result == 0) // gdb exited
    {
        statusOut("gdb exited.\n");
        theJtagICE->resumeProgram();
        throw jtag_exception("gdb exited");
    }

    return c;
}

int checkForDebugChar() {
    uchar c = 0;

    const auto result = read(gdbFileDescriptor, &c, 1);

    if (result < 0 && errno == EAGAIN)
        return -1;

    if (result < 0)
        throw jtag_exception();

    if (result == 0) // gdb exited
    {
        statusOut("gdb exited.\n");
        theJtagICE->resumeProgram();
        throw jtag_exception("gdb exited");
    }

    return (int)c;
}

static const char hexchars[] = "0123456789abcdef";

static char *byteToHex(uchar x, char *buf) {
    *buf++ = hexchars[x >> 4];
    *buf++ = hexchars[x & 0xf];
    return buf;
}

static int hex(unsigned char ch) {
    if ((ch >= 'a') && (ch <= 'f')) {
        return (ch - 'a' + 10);
    }
    if ((ch >= '0') && (ch <= '9')) {
        return (ch - '0');
    }
    if ((ch >= 'A') && (ch <= 'F')) {
        return (ch - 'A' + 10);
    }
    return (-1);
}

/** Convert hex string at '*ptr' to an integer.
    Advances '*ptr' to 1st non-hex character found.
    Returns number of characters used in conversion.
 **/
static int hexToInt(char **ptr, int *intValue, int nMax = 0) {
    int numChars = 0;

    *intValue = 0;
    while (**ptr) {
        const int hexValue = hex(**ptr);
        if (hexValue >= 0) {
            *intValue = (*intValue << 4) | hexValue;
            numChars++;
        } else {
            break;
        }
        (*ptr)++;
        if (nMax != 0 && numChars >= nMax)
            return numChars;
    }
    return (numChars);
}

/** Convert the memory pointed to by mem into hex, placing result in buf.
    Return a pointer to the last char put in buf (null).
**/
static char *mem2hex(uchar *mem, char *buf, int count) {
    for (int i = 0; i < count; i++)
        buf = byteToHex(*mem++, buf);
    *buf = 0;
    return buf;
}

/** Convert the hex array pointed to by buf into binary to be placed in mem.
    Return a pointer to the character AFTER the last byte written.
**/
static uchar *hex2mem(char *buf, uchar *mem, int count) {
    for (int i = 0; i < count; i++) {
        unsigned char ch = hex(*buf++) << 4;
        ch += hex(*buf++);
        *mem++ = ch;
    }
    return mem;
}

static void putpacket(const char *buffer);

void vgdbOut(const char *fmt, va_list args) {
    // We protect against reentry because putpacket could try and report
    // an error which would lead to a call back to vgdbOut
    static bool reentered = false;

    if (gdbFileDescriptor >= 0 && !reentered) {
        char textbuf[BUFMAX], hexbuf[2 * BUFMAX], *textscan, *hexscan;

        reentered = true;

        vsnprintf(textbuf, BUFMAX, fmt, args);

        hexscan = hexbuf;
        *hexscan++ = 'O';
        for (textscan = textbuf; *textscan; textscan++)
            hexscan = byteToHex(*textscan, hexscan);
        *hexscan = '\0';
        putpacket(hexbuf);

        reentered = false;
    }
}

void gdbOut(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vgdbOut(fmt, args);
    va_end(args);
}

/** Fill 'remcomOutBuffer' with a status report for signal 'sigval'

    Reply with a packet of the form:

      "Tss20:rr;21:llhh;22:aabbccdd;"

    where (all values in hex):
      ss       = signal number (usually SIGTRAP)
      rr       = SREG value
      llhh     = SPL:SPH  (stack pointer)
      aabbccdd = PC (program counter)

    This actually saves having to read the 32 general registers when stepping
    over code since gdb won't send a 'g' packet until the PC it is hunting for
    is found.  */

static void reportStatusExtended(int sigval) {
    unsigned int pc = theJtagICE->getProgramCounter();

    // Read in SPL SPH SREG
    uchar *jtagBuffer = theJtagICE->jtagRead(theJtagICE->statusAreaAddress(), 0x03);

    if (jtagBuffer) {
        // We have SPL SPH SREG and need SREG SPL SPH

        snprintf(remcomOutBuffer, sizeof(remcomOutBuffer),
                 "T%02x"
                 "20:%02x;"
                 "21:%02x%02x;"
                 "22:%02x%02x%02x%02x;",
                 sigval & 0xff,
                 jtagBuffer[2], // SREG
                 jtagBuffer[0], // SPL
                 jtagBuffer[1], // SPH
                 pc & 0xff, (pc >> 8) & 0xff, (pc >> 16) & 0xff, (pc >> 24) & 0xff);

        delete[] jtagBuffer;
        jtagBuffer = nullptr;
    } else {
        error(1);
        return;
    }
}

/** Fill 'remcomOutBuffer' with a status report for signal 'sigval' **/
static void reportStatus(int sigval) {
    char *ptr = remcomOutBuffer;

    // We could use 'T'. But this requires reading SREG, SP, PC at least, so
    // won't be significantly faster than the 'g' operation that gdb will
    // request if we use 'S' here. [TRoth/2003-06-12: this is not necessarily
    // true. See above comment for reportStatusExtended().]

    *ptr++ = 'S'; // notify gdb with signo
    ptr = byteToHex(sigval, ptr);
    *ptr++ = 0;
}

// little-endian word read
static unsigned int readLWord(unsigned int address) {
    unsigned char *mem = theJtagICE->jtagRead(DATA_SPACE_ADDR_OFFSET + address, 2);

    if (!mem)
        return 0; // hmm

    unsigned int val = mem[0] | mem[1] << 8;
    delete[] mem;
    return val;
}

// big-endian word read
static unsigned int readBWord(unsigned int address) {
    unsigned char *mem = theJtagICE->jtagRead(DATA_SPACE_ADDR_OFFSET + address, 2);

    if (!mem)
        return 0; // hmm

    unsigned int val = mem[0] << 8 | mem[1];
    delete[] mem;
    return val;
}

static unsigned int readSP() { return readLWord(0x5d); }

static bool handleInterrupt() {
    bool result;

    // Set a breakpoint at return address
    debugOut("INTERRUPT\n");
    unsigned int intrSP = readSP();
    unsigned int retPC = readBWord(intrSP + 1) << 1;
    debugOut("INT SP = %x, retPC = %x\n", intrSP, retPC);

    bool needBP = !theJtagICE->codeBreakpointAt(retPC);

    for (;;) {
        if (needBP) {
            // If no breakpoint at return address (normal case),
            // add one.
            // Normally, this breakpoint add should succeed as gdb shouldn't
            // be using a momentary breakpoint when doing a step-through-range,
            // thus leaving is a free hw breakpoint. But if for some reason it
            // the add fails, interrupt the program at the interrupt handler
            // entry point
            if (!theJtagICE->addBreakpoint(retPC, BreakpointType::CODE, 0))
                return false;
        }
        result = theJtagICE->jtagContinue();
        if (needBP)
            theJtagICE->deleteBreakpoint(retPC, BreakpointType::CODE, 0);

        if (!result || !needBP) // user interrupt or hit user BP at retPC
            break;

        // We check that SP is > intrSP. If SP <= intrSP, this is just
        // an unrelated excursion to retPC
        if (readSP() > intrSP)
            break;
    }
    return result;
}

static bool singleStep() {
    try {
        theJtagICE->jtagSingleStep();
    } catch (jtag_exception &e) {
        gdbOut("Failed to single-step");
    }

    unsigned int newPC = theJtagICE->getProgramCounter();
    if (theJtagICE->codeBreakpointAt(newPC))
        return true;
    // assume interrupt when PC goes into interrupt table
    if (ignoreInterrupts && newPC < theJtagICE->deviceDef->vectors_end)
        return handleInterrupt();

    return true;
}

/** Read packet from gdb into remcomInBuffer, check checksum and confirm
    reception to gdb.
    Return pointer to null-terminated, actual packet data (without $, #,
    the checksum)
**/
static char *getpacket(int &len) {
    char *buffer = &remcomInBuffer[0];
    unsigned char checksum;
    unsigned char xmitcsum;
    int count;
    uchar ch;
    bool is_escaped;

    // scan for the sequence $<data>#<checksum>
    while (true) {
        // wait around for the start character, ignore all other characters
        while ((ch = getDebugChar()) != '$')
            ;

    retry:
        checksum = 0;
        count = 0;
        is_escaped = false;

        // now, read until a # or end of buffer is found
        while (count < BUFMAX - 1) {
            ch = getDebugChar();
            if (is_escaped) {
                checksum = checksum + ch;
                ch ^= 0x20;
                is_escaped = false;
                goto have_char;
            }
            if (ch == '$') {
                goto retry;
            }
            if (ch == '#') {
                break;
            }
            checksum = checksum + ch;
            if (ch == '}') {
                is_escaped = true;
                continue;
            }
        have_char:
            buffer[count] = ch;
            count = count + 1;
        }
        buffer[count] = 0;

        if (ch == '#') {
            ch = getDebugChar();
            xmitcsum = hex(ch) << 4;
            ch = getDebugChar();
            xmitcsum += hex(ch);

            if (checksum != xmitcsum) {
                char buf[16];

                mem2hex(&checksum, buf, 4);
                gdbOut("Bad checksum: my count = %s, ", buf);
                mem2hex(&xmitcsum, buf, 4);
                gdbOut("sent count = %s\n", buf);
                gdbOut(" -- Bad buffer: \"%s\"\n", buffer);

                putDebugChar('-'); // failed checksum
            } else {
                putDebugChar('+'); // successful transfer

                // if a sequence char is present, reply the sequence ID
                if (buffer[2] == ':') {
                    putDebugChar(buffer[0]);
                    putDebugChar(buffer[1]);

                    len = count - 3;

                    return &buffer[3];
                }

                len = count;
                return &buffer[0];
            }
        }
    }
}

/** Send packet 'buffer' to gdb. Adds $, # and checksum wrappers. **/
static void putpacket(const char *buffer) {
    char ch;

    //  $<packet info>#<checksum>.
    do {
        putDebugChar('$');
        uchar checksum = 0;
        int count = 0;

        while ((ch = buffer[count])) {
            putDebugChar(ch);
            checksum += ch;
            ++count;
        }
        putDebugChar('#');
        putDebugChar(hexchars[checksum >> 4]);
        putDebugChar(hexchars[checksum % 16]);
    } while (getDebugChar() != '+'); // wait for the ACK
}

/** Set remcomOutBuffer to "ok" response */
static void ok() { strcpy(remcomOutBuffer, "OK"); }

/** Set remcomOutBuffer to error 'n' response */
static void error(int n) {
    char *ptr = remcomOutBuffer;

    *ptr++ = 'E';
    ptr = byteToHex(n, ptr);
    *ptr = '\0';
}

/** reply to a "monitor" command */
static bool monitor(const std::string_view cmd) {

    // put "s" into remcomOutBuffer, obeying the packet size limit
    auto replyString = [](const char *s) {
        unsigned int i = 0;
        char c;

        char *ptr = remcomOutBuffer;
        while ((c = *s++) != 0 && i < BUFMAX - 1) {
            ptr = byteToHex((int)c, ptr);
            i += 2;
        }
        *ptr = '\0';
    };

    if ((cmd == "help") || (cmd == "?")) {
        replyString("AVaRICE commands:\n"
                    "help, ?:   get help\n"
                    "version:   ask AVaRICE version\n"
                    "reset:     reset target\n");
        return true;
    } else if (cmd == "version") {
        char reply[80];
        sprintf(reply, "AVaRICE version %s, %s %s\n", PACKAGE_VERSION, __DATE__, __TIME__);
        replyString(reply);
        return true;
    } else if (cmd == "reset") {
        try {
            theJtagICE->resetProgram(false);
            replyString("Resetting MCU...\n");
        } catch (jtag_exception &e) {
            char reply[80];
            sprintf(reply, "Failed to reset MCU: %s\n", e.what());
            replyString(reply);
        }
        return true;
    } else {
        return false;
    }
}

static void repStatus(bool breaktime) {
    if (breaktime)
        reportStatusExtended(SIGTRAP);
    else {
        // A breakpoint did not occur. Assume that GDB sent a break.
        // Halt the target.
        theJtagICE->interruptProgram();
        // Report this as a user interrupt
        reportStatusExtended(SIGINT);
    }
}

static std::string makeSafeString(const char *s, int inLength) {
    std::string r;
    int j = 0;
    while (j++ < inLength) {
        const char c = *s++;
        if (isprint(c)) {
            r += c;
        } else {
            char tmp_hex_buf[5]; // "\xFF"
            sprintf(tmp_hex_buf, "\\x%02x", (unsigned)c & 0xff);
            r += tmp_hex_buf;
        }
    }
    return r;
}

void talkToGdb() {
    bool dontSendReply = false;
    static char last_cmd = 0;

    int plen;
    char *ptr = getpacket(plen);

    if (debugMode) {
        const auto s = makeSafeString(ptr, plen);
        debugOut("GDB: <%s>\n", s.c_str());
    }

    // default empty response
    remcomOutBuffer[0] = 0;

    const char cmd = *ptr++;
    switch (cmd) {
    case 'k': // kill the program
        dontSendReply = true;
        break;

    case 'R':
        try {
            theJtagICE->resetProgram(false);
        } catch (jtag_exception &e) {
            gdbOut("reset failed\n");
        }
        dontSendReply = true;
        break;

    case '!':
        ok();
        break;

    case 'M': {
        int lead = 0;
        static bool last_orphan_pending = false;
        static uchar last_orphan = 0xff;

        // MAA..AA,LLLL: Write LLLL bytes at address AA.AA return OK
        // TRY TO READ '%x,%x:'.  IF SUCCEED, SET PTR = 0

        error(1); // default is error
        int length;
        int addr;
        if ((hexToInt(&ptr, &addr)) && (*(ptr++) == ',') && (hexToInt(&ptr, &length)) &&
            (*(ptr++) == ':') && (length > 0)) {
            debugOut("\nGDB: Write %d bytes to 0x%X\n", length, addr);

            // There is no gaurantee that gdb will send a word aligned stream
            // of bytes, so we need to try and catch that here. This ugly, but
            // it would more difficult to change in gdb and probably affect
            // more than avarice users. This hack will make the gdb 'load'
            // command less prone to failure.

            if (addr & 1) {
                // odd addr means there may be a byte from last 'M' to write
                if ((last_cmd == 'M') && (last_orphan_pending)) {
                    length++;
                    addr--;
                    lead = 1;
                }
            }

            last_orphan_pending = false;

            auto *jtagBuffer = new uchar[length];
            hex2mem(ptr, jtagBuffer + lead, length);
            if (lead)
                jtagBuffer[0] = last_orphan;

            if ((addr < DATA_SPACE_ADDR_OFFSET) && (length & 1)) {
                // An odd length means we will have%02x an orphan this round but
                // only if we are writing to PROG space.
                last_orphan_pending = true;
                last_orphan = jtagBuffer[length];
                length--;
            }

            try {
                theJtagICE->jtagWrite(addr, length, jtagBuffer);
            } catch (jtag_exception &) {
                delete[] jtagBuffer;
                break;
            }
            ok();
            delete[] jtagBuffer;
        }
        break;
    }
    case 'm': { // mAA..AA,LLLL  Read LLLL bytes at address AA..AA
        int length;
        int addr;
        if ((hexToInt(&ptr, &addr)) && (*(ptr++) == ',') && (hexToInt(&ptr, &length))) {
            debugOut("\nGDB: Read %d bytes from 0x%X\n", length, addr);
            try {
                uchar *jtagBuffer = theJtagICE->jtagRead(addr, length);
                mem2hex(jtagBuffer, remcomOutBuffer, length);
                delete[] jtagBuffer;
            } catch (jtag_exception &) {
                error(1);
            }
        }
        break;
    }
    case '?':
        // Report status. We don't actually know, so always report breakpoint
        reportStatus(SIGTRAP);
        break;

    case 'g': // return the value of the CPU registers
    {
        uchar regBuffer[40];
        memset(regBuffer, 0, sizeof(regBuffer));

        // Read the registers directly from memory
        // R0..R31 are at locations 0..31
        // SP is at 0x5D & 0x5E
        // SREG is at 0x5F
        debugOut("\nGDB: (Registers)Read %d bytes from 0x%X\n", 0x20,
                 theJtagICE->cpuRegisterAreaAddress());
        uchar *jtagBuffer = theJtagICE->jtagRead(theJtagICE->cpuRegisterAreaAddress(), 0x20);
        if (jtagBuffer) {
            // Put GPRs into the first 32 locations
            memcpy(regBuffer, jtagBuffer, 0x20);

            delete[] jtagBuffer;
            jtagBuffer = nullptr;
        } else {
            error(1);
            break;
        }

        // Read in SPL SPH SREG
        jtagBuffer = theJtagICE->jtagRead(theJtagICE->statusAreaAddress(), 0x03);

        if (jtagBuffer) {
            // We have SPL SPH SREG and need SREG SPL SPH

            // SREG
            regBuffer[0x20] = jtagBuffer[0x02];

            // NOTE: Little endian, so SPL comes first.

            // SPL
            regBuffer[0x21] = jtagBuffer[0x00];

            // SPH
            regBuffer[0x22] = jtagBuffer[0x01];

            delete[] jtagBuffer;
            jtagBuffer = nullptr;
        } else {
            error(1);
            break;
        }

        // PC
        const auto newPC = theJtagICE->getProgramCounter();
        regBuffer[35] = (unsigned char)(newPC & 0xff);
        regBuffer[36] = (unsigned char)((newPC & 0xff00) >> 8);
        regBuffer[37] = (unsigned char)((newPC & 0xff0000) >> 16);
        regBuffer[38] = (unsigned char)((newPC & 0xff000000) >> 24);
        debugOut("PC = %x\n", newPC);

        if (newPC == PC_INVALID)
            error(1);
        else
            mem2hex(regBuffer, remcomOutBuffer, 32 + 1 + 2 + 4);

        break;
    }

    case 'q': // general query
    {
        if (strncmp(ptr, "Ravr.io_reg", strlen("Ravr.io_reg")) == 0) {
            int i, j, regcount;

            debugOut("\nGDB: (io registers) Read %d bytes from 0x%X\n", 0x40, 0x20);

            /* If there is an io_reg_defs for this device then respond */

            gdb_io_reg_def_type *io_reg_defs = theJtagICE->deviceDef->io_reg_defs;
            if (io_reg_defs) {
                // count the number of defined registers
                regcount = 0;
                while (io_reg_defs[regcount].name) {
                    regcount++;
                }

                ptr += strlen("Ravr.io_reg");
                if (*ptr == '\0') {
                    sprintf(remcomOutBuffer, "%02x", regcount);
                } else if (*ptr == ':') {
                    // Request for a sequence of io registers
                    int offset;
                    i = 0;
                    j = 0;
                    int count;

                    // Find the first register
                    ptr++;
                    hexToInt(&ptr, &i);

                    // Confirm presence of ','
                    if (*ptr++ == ',') {
                        hexToInt(&ptr, &j);
                    }

                    // i is the first register to read
                    // j is the number of registers to read
                    while ((j > 0) && (i < regcount)) {
                        count = 0;

                        if ((io_reg_defs[i].name != nullptr) && (io_reg_defs[i].flags != 0x00)) {
                            // Register with special flags set
                            offset = strlen(remcomOutBuffer);
                            sprintf(&remcomOutBuffer[offset], "[-- %s --],00;",
                                    io_reg_defs[i].name);
                            i++;
                            j--;
                        } else {
                            // Get the address of the first io_register to be
                            // read
                            unsigned int addr = io_reg_defs[i].reg_addr;

                            // Count the number of consecutive address,
                            // no-side effects, valid registers

                            for (count = 0; count < j; count++) {
                                if ((io_reg_defs[i + count].name == nullptr) ||
                                    (io_reg_defs[i + count].flags != 0x00) ||
                                    (io_reg_defs[i + count].reg_addr != addr)) {
                                    break;
                                }
                                addr++;
                            }

                            if (count) {
                                // Read consecutive address io_registers
                                uchar *jtagBuffer = theJtagICE->jtagRead(
                                    DATA_SPACE_ADDR_OFFSET + io_reg_defs[i].reg_addr, count);

                                if (jtagBuffer) {
                                    int k = 0;
                                    // successfully read
                                    while (count--) {
                                        offset = strlen(remcomOutBuffer);
                                        sprintf(&remcomOutBuffer[offset], "%s,%02x;",
                                                io_reg_defs[i].name, jtagBuffer[k++]);
                                        i++;
                                        j--;
                                    }

                                    delete[] jtagBuffer;
                                    jtagBuffer = nullptr;
                                }
                            }
                        }
                    }
                }
            }
        } else if (strncmp(ptr, "Supported:", 10) == 0) {
            strcpy(remcomOutBuffer, "qXfer:memory-map:read+");
        } else if (strncmp(ptr, "Xfer:memory-map:read::", 22) == 0) {
            sprintf(remcomOutBuffer,
                    "l<memory-map>\n"
                    /* The RAM size indicated includes the possible
                     * EEPROM range, so GDB will treat EEPROM uploads
                     * just like simple SRAM load operations.  AVaRICE
                     * will disambiguate them based on their virtual
                     * offset. */
                    "  <memory type=\"ram\" start=\"0x800000\" length=\"0x20000\" />\n"
                    "  <memory type=\"flash\" start=\"0\" length=\"0x%x\">\n"
                    "     <property name=\"blocksize\">0x%0x</property>\n"
                    "  </memory>\n"
                    "</memory-map>\n",
                    theJtagICE->deviceDef->flash_page_size *
                        theJtagICE->deviceDef->flash_page_count,
                    theJtagICE->deviceDef->flash_page_size);
        } else if (strncmp(ptr, "Rcmd,", 4) == 0) {
            char cmdbuf[MONMAX];
            memset(cmdbuf, 0, sizeof(cmdbuf));
            ptr += 5;
            int length = strlen(ptr);
            for (int i = 0; i < MONMAX && length; i++) {
                int c;
                length -= hexToInt(&ptr, &c, 2);
                cmdbuf[i] = (char)c;
            }
            debugOut("\nGDB: (monitor) %s\n", cmdbuf);

            // when creating a response, minde the BUFMAX bytes per
            // packet limit above
            if (!monitor(cmdbuf))
                remcomOutBuffer[0] = '\0'; // not recognized
        }

        break;
    }

    case 'P':     // set the value of a single CPU register - return OK
        error(1); // error by default
        int regno;
        if (hexToInt(&ptr, &regno) && *ptr++ == '=') {
            uchar reg[4];

            try {
                if (regno >= 0 && regno < NUMREGS) {
                    hex2mem(ptr, reg, 1);
                    theJtagICE->jtagWrite(theJtagICE->cpuRegisterAreaAddress() + regno, 1, reg);
                    ok();
                    break;
                } else if (regno == SREG) {
                    hex2mem(ptr, reg, 1);
                    theJtagICE->jtagWrite(theJtagICE->statusAreaAddress() + 2, 1, reg);
                } else if (regno == SP) {
                    hex2mem(ptr, reg, 2);
                    theJtagICE->jtagWrite(theJtagICE->statusAreaAddress(), 2, reg);
                    ok();
                } else if (regno == PC) {
                    hex2mem(ptr, reg, 4);
                    theJtagICE->setProgramCounter(b4_to_u32(reg));
                    ok();
                }
            } catch (jtag_exception &e) {
                // ignore, error state already set above
            }
        }
        break;

    case 'G': // set the value of the CPU registers
        // It would appear that we don't need to support this as
        // we have 'P'. Report an error (rather than fail silently,
        // this will make errors in this comment more apparent...)
        error(1);
        break;

    case 's': // sAA..AA    Step one instruction from AA..AA(optional)
        // try to read optional parameter, pc unchanged if no parm
        int addr;
        if (hexToInt(&ptr, &addr)) {
            try {
                theJtagICE->setProgramCounter(addr);
            } catch (jtag_exception &) {
                gdbOut("Failed to set PC");
            }
        }
        repStatus(singleStep());
        break;

    case 'C': {
        /* Continue with signal command format:
           "C<sig>;<addr>" or "S<sig>;<addr>"

           "<sig>" should always be 2 hex digits, possibly zero padded.
           ";<addr>" part is optional.

           If addr is given, resume at that address, otherwise, resume at
           current address. */

        int sig;
        if (hexToInt(&ptr, &sig)) {
            if (sig == SIGHUP) {
                try {
                    theJtagICE->resetProgram(false);
                    reportStatusExtended(SIGTRAP);
                } catch (jtag_exception &e) {
                    fprintf(stderr, "Failed to reset MCU: %s\n", e.what());
                }
            }
        }
        break;
    }

    case 'c': // cAA..AA    Continue from address AA..AA(optional)
        // try to read optional parameter, pc unchanged if no parm
        if (hexToInt(&ptr, &addr)) {
            try {
                theJtagICE->setProgramCounter(addr);
            } catch (jtag_exception &) {
                gdbOut("Failed to set PC");
            }
        }
        repStatus(theJtagICE->jtagContinue());
        break;

    case 'D':
        // Detach, resumes target. Can get control back with step
        // or continue
        try {
            theJtagICE->resumeProgram();
        } catch (jtag_exception &) {
            error(1);
            break;
        }
        ok();
        break;

    case 'v': {
        static unsigned char *flashbuf;
        static unsigned int maxaddr;
        if (strncmp(ptr, "FlashErase:", 11) == 0) {
            ptr += 11;
            int offset, length;
            hexToInt(&ptr, &offset);
            ++ptr;
            hexToInt(&ptr, &length);
            statusOut("erasing %d bytes @ 0x%0x\n", length, offset);
            theJtagICE->enableProgramming();
            theJtagICE->eraseProgramMemory();
            flashbuf = new unsigned char[theJtagICE->deviceDef->flash_page_size *
                                         theJtagICE->deviceDef->flash_page_count];
            maxaddr = 0;
            memset(flashbuf, 0xff,
                   theJtagICE->deviceDef->flash_page_size *
                       theJtagICE->deviceDef->flash_page_count);
            ok();
        } else if (strncmp(ptr, "FlashWrite:", 11) == 0) {
            char *optr = ptr;
            ptr += 11;
            int offset;
            hexToInt(&ptr, &offset);
            ++ptr; // past ":"
            const unsigned int amount = plen - 1 - (ptr - optr);
            statusOut("buffering data, %d bytes @ 0x%x\n", amount, offset);
            if (offset + amount > maxaddr)
                maxaddr = offset + amount;
            memcpy(flashbuf + offset, ptr, amount);
            ok();
        } else if (strncmp(ptr, "FlashDone", 9) == 0) {
            statusOut("committing to flash\n");
            const auto pagesize = theJtagICE->deviceDef->flash_page_size;
            for (unsigned int offset = 0; offset < maxaddr; offset += pagesize) {
                theJtagICE->jtagWrite(offset, pagesize, flashbuf + offset);
            }
            theJtagICE->disableProgramming();
            delete[] flashbuf;
            ok();
        }
        break;
    }

    case 'Z':
    case 'z': {
        error(1); // assume the worst.

        int bp_type;
        int length;
        // Add a Breakpoint. note: length specifier is ignored for now.
        if (hexToInt(&ptr, &bp_type) && (*ptr++ == ',') && hexToInt(&ptr, &addr) &&
            (*ptr++ == ',') && hexToInt(&ptr, &length)) {

            BreakpointType mode = BreakpointType::NONE;
            switch (bp_type) {
            case 0:
            case 1:
                mode = BreakpointType::CODE;
                break;
            case 2:
                mode = BreakpointType::WRITE_DATA;
                break;
            case 3:
                mode = BreakpointType::READ_DATA;
                break;
            case 4:
                mode = BreakpointType::ACCESS_DATA;
                break;
            default:
                debugOut("Unknown breakpoint type from GDB.\n");
                throw jtag_exception();
            }

            try {
                if (cmd == 'Z') { // Adding
                    theJtagICE->addBreakpoint(addr, mode, length);
                } else {
                    theJtagICE->deleteBreakpoint(addr, mode, length);
                }
            } catch (jtag_exception &) {
                break;
            }
            ok();
        }
        break;
    }
    default: // Unknown code.  Return an empty reply message.
        break;
    } // switch

    last_cmd = cmd;

    // reply to the request
    if (!dontSendReply) {
        debugOut("->GDB: %s\n", remcomOutBuffer);
        putpacket(remcomOutBuffer);
    }
}
