/*
 *	avarice - The "avarice" program.
 *	Copyright (C) 2001 Scott Finneran
 *      Copyright (C) 2002 Intel Corporation
 *	Copyright (C) 2005, 2007 Joerg Wunsch
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
 * This file contains functions for interfacing with the JTAG box.
 */

#include <cstdio>

#include "jtag1.h"
#include "remote.h"

unsigned long jtag1::getProgramCounter() {
    const uchar command[] = {'2', JTAG_EOM};

    auto response = doJtagCommand(command, sizeof(command), 4);

    if (Resp{response[3]} != Resp::OK)
        return PC_INVALID;
    else {
        auto result = decodeAddress(response.get());

        --result; // returned value is PC + 1 as far as GDB is concerned

        // The JTAG box sees program memory as 16-bit wide locations. GDB
        // sees bytes. As such, double the PC value.
        result *= 2;

        return result;
    }
}

void jtag1::setProgramCounter(unsigned long pc) {
    uchar command[] = {'3', 0, 0, 0, JTAG_EOM};

    // See decoding in getProgramCounter
    encodeAddress(&command[1], pc / 2 + 1);

    auto response = doJtagCommand(command, sizeof(command), 1);
    if (Resp{response[0]} != Resp::OK)
        throw jtag_exception();
}

void jtag1::resetProgram(bool possible_nSRST) {
    if (possible_nSRST && apply_nSRST) {
        setJtagParameter(JTAG_P_EXTERNAL_RESET, 0x01);
    }
    doSimpleJtagCommand('x', 1);
    if (possible_nSRST && apply_nSRST) {
        setJtagParameter(JTAG_P_EXTERNAL_RESET, 0x00);
    }
}

void jtag1::interruptProgram() {
    // Just ignore the returned PC. It appears to be wrong if the most
    // recent instruction was a branch.
    doSimpleJtagCommand('F', 4);
}

void jtag1::resumeProgram() { doSimpleJtagCommand('G', 0); }

void jtag1::jtagSingleStep() { doSimpleJtagCommand('1', 1); }

void jtag1::parseEvents(const char *) {
    // current no event name parsing in mkI
}

bool jtag1::jtagContinue() {
    updateBreakpoints(); // download new bp configuration

    if (!doSimpleJtagCommand('G', 0)) {
        gdbOut("Failed to continue\n");
        return true;
    }

    for (;;) {
        fd_set readfds;
        bool breakpoint = false;
        bool gdbInterrupt = false;

        // Now that we are "going", wait for either a response from the JTAG
        // box or a nudge from GDB.
        debugOut("Waiting for input.\n");

        // Check for input from JTAG ICE (breakpoint, sleep, info, power)
        // or gdb (user break)
        FD_ZERO(&readfds);
        FD_SET(gdbFileDescriptor, &readfds);
        FD_SET(jtagBox, &readfds);
        const int maxfd = jtagBox > gdbFileDescriptor ? jtagBox : gdbFileDescriptor;

        int numfds = select(maxfd + 1, &readfds, nullptr, nullptr, nullptr);
        if (numfds < 0) {
            debugOut("GDB/JTAG ICE communications failure");
            throw jtag_exception();
        }

        if (FD_ISSET(gdbFileDescriptor, &readfds)) {
            const auto c = getDebugChar();
            if (c == 3) // interrupt
            {
                debugOut("interrupted by GDB\n");
                gdbInterrupt = true;
            } else
                debugOut("Unexpected GDB input `%02x'\n", c);
        }

        // Read all extant responses (there's a small chance there could
        // be more than one)

        // Note: In case of a gdb interrupt, it's possible that we will
        // receive one of these responses before interruptProgram is
        // called. This should not cause a problem as the normal retry
        // mechanism should eat up this response.
        // It might be cleaner to handle JTAG_R_xxx in sendJtagCommand

        // Check the JTAG ICE's message. It can indicate a breakpoint
        // which causes us to return, a sleep status change (ignored),
        // power "event" -- whatever that is (ignored), or a byte of
        // info sent by the program (currently ignored, could be used
        // for something...)
        Resp response;

        // This read shouldn't need to be a timeout_read(), but some cygwin
        // systems don't seem to honor the O_NONBLOCK flag on file
        // descriptors.
        while (timeout_read(&response, 1, 1) == 1) {
            debugOut("JTAG box sent %c", response);
            switch (response) {
            case Resp::BREAK: {
                uchar buf[2];
                const auto count = timeout_read(buf, 2, JTAG_RESPONSE_TIMEOUT);
                if (count < 2)
                    throw jtag_exception();
                breakpoint = true;
                debugOut(": Break Status Register = 0x%02x%02x\n", buf[0], buf[1]);
                break;
            }
            case Resp::INFO:
            case Resp::SLEEP: {
                uchar buf[2];
                // we could do something here, esp. for info
                const auto count = timeout_read(buf, 2, JTAG_RESPONSE_TIMEOUT);
                if (count < 2)
                    throw jtag_exception();
                debugOut(": 0x%02, 0x%02\n", buf[0], buf[1]);
                break;
            }
            case Resp::POWER:
                // apparently no args?
                debugOut("\n");
                break;
            default:
                debugOut(": Unknown response\n");
                break;
            }
        }

        // We give priority to user interrupts
        if (gdbInterrupt)
            return false;
        if (breakpoint)
            return true;
    }
}
