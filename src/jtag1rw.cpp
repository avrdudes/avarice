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
 * This file contains functions for interfacing with the JTAG box.
 */

#include <cassert>
#include <cstring>

#include "jtag1.h"

/** Return the memory space code for the memory space indicated by the
    high-order bits of '*addr'. Also clear these high order bits in '*addr'
**/
static uchar memorySpace(unsigned long *addr) {
    int mask;

    // We can't just mask the bits off, because 0x10000->0x1ffff are
    // valid code addresses
    if (*addr & DATA_SPACE_ADDR_OFFSET) {
        mask = *addr & ADDR_SPACE_MASK;
        *addr &= ~ADDR_SPACE_MASK;
    } else
        mask = 0;

    switch (mask) {
    case EEPROM_SPACE_ADDR_OFFSET:
        return ADDR_EEPROM_SPACE;
    case FUSE_SPACE_ADDR_OFFSET:
        return ADDR_FUSE_SPACE;
    case LOCK_SPACE_ADDR_OFFSET:
        return ADDR_LOCK_SPACE;
    case SIG_SPACE_ADDR_OFFSET:
        return ADDR_SIG_SPACE;
    case BREAKPOINT_SPACE_ADDR_OFFSET:
        return ADDR_BREAKPOINT_SPACE;
    case DATA_SPACE_ADDR_OFFSET:
        return ADDR_DATA_SPACE;
    default:
        return 0; // program memory, handled specially
    }
}

static void swapBytes(uchar *buffer, unsigned int count) {
    assert(!(count & 1));
    for (unsigned int i = 0; i < count; i += 2) {
        uchar temp = buffer[i];
        buffer[i] = buffer[i + 1];
        buffer[i + 1] = temp;
    }
}

uchar *jtag1::jtagRead(unsigned long addr, unsigned int numBytes) {
    if (numBytes == 0) {
        auto response = std::make_unique<uchar[]>(1);
        response[0] = '\0';
        return response.release(); // TODO: be compatible with other jtagRead()
    }

    BOOST_LOG_TRIVIAL(debug) << "jtagRead";
    uchar command[] = {'R', 0, 0, 0, 0, 0, JTAG_EOM};

    int whichSpace = memorySpace(&addr);
    if (whichSpace) {
        command[1] = whichSpace;
        if (numBytes > 256)
            return nullptr;
        command[2] = numBytes - 1;
        encodeAddress(&command[3], addr);

        // Response will be the number of data bytes with an 'A' at the
        // start and end. As such, the response size will be number of bytes
        // + 2. Then add an additional byte for the trailing zero (see
        // protocol document).

        auto response = doJtagCommand(command, sizeof(command), numBytes + 2);
        if (Resp{response[numBytes + 1]} == Resp::OK)
            return response.release(); // TODO: be compatible with other jtagRead()

        throw jtag_exception();
    } else {
        // Reading program memory
        whichSpace =
            programmingEnabled ? ADDR_PROG_SPACE_PROG_ENABLED : ADDR_PROG_SPACE_PROG_DISABLED;

        // Program space is 16 bits wide, with word reads
        const auto numLocations = (addr & 1)?(numBytes + 2) / 2:(numBytes + 1) / 2;
        if (numLocations > (UINT8_MAX+1))
            throw jtag_exception();

        command[1] = whichSpace;
        command[2] = numLocations - 1;
        encodeAddress(&command[3], addr / 2);

        auto response = doJtagCommand(command, sizeof(command), numLocations * 2 + 2);
        if (Resp{response[numLocations * 2 + 1]} == Resp::OK) {
            // Programming mode and regular mode are byte-swapped...
            if (!programmingEnabled)
                swapBytes(response.get(), numLocations * 2);

            if (addr & 1)
                // we read one byte early. move stuff down.
                memmove(response.get(), response.get() + 1, numBytes);

            return response.release(); // TODO: be compatible with other jtagRead()
        }

        throw jtag_exception();
    }
}

void jtag1::jtagWrite(unsigned long addr, unsigned int numBytes, uchar buffer[]) {
    if (numBytes == 0)
        return;

    BOOST_LOG_TRIVIAL(debug) << "jtagWrite";
    unsigned int numLocations;
    int whichSpace = memorySpace(&addr);
    if (whichSpace)
        numLocations = numBytes;
    else {
        // Writing program memory, which is word (16-bit) addressed

        // We don't handle odd lengths or start addresses
        if ((addr & 1)) {
            throw jtag_exception("Odd program memory write operation");
        }

        // Odd length: Write one more byte.
        if ((numBytes & 1)) {
            BOOST_LOG_TRIVIAL(warning) << "Odd pgm wr length";
            numBytes += 1;
        }

        addr /= 2;
        numLocations = numBytes / 2;

        if (programmingEnabled)
            whichSpace = ADDR_PROG_SPACE_PROG_ENABLED;
        else {
            whichSpace = ADDR_PROG_SPACE_PROG_DISABLED;
            swapBytes(buffer, numBytes);
        }
    }

    // This is the maximum write size
    if (numLocations > (UINT8_MAX+1))
        throw jtag_exception("Attempt to write more than 256 bytes");

    // Writing is a two part process

    // Part 1: send the address
    {
        uchar command[] = {
            'W',     static_cast<uchar>(whichSpace), static_cast<uchar>(numLocations - 1), 0, 0, 0,
            JTAG_EOM};
        encodeAddress(&command[3], addr);

        auto response_part1 = doJtagCommand(command, sizeof(command), 0);
        if (!response_part1)
            throw jtag_exception();
    }

    // Part 2: send the data in the following form:
    // h [data byte]...[data byte] __

    // Before we begin, a little note on the endianness.
    // Firstly, data space is 8 bits wide and the only data written by
    // this routine will be byte-wide, so endianness does not matter.

    // For code space, the data is 16 bit wide. The data appears to be
    // formatted big endian in the processor memory. The JTAG box
    // expects little endian data. The object files output from GCC are
    // already word-wise little endian.

    // As this data is already formatted, and currently the only writes
    // to program space are for code download, it is simpler at this
    // stage to simply pass the data straight through. This may need to
    // change in the future.
    auto txBuffer = std::make_unique<uchar[]>(numBytes + 3); // allow for header and trailer
    txBuffer[0] = 'h';
    memcpy(&txBuffer[1], buffer, numBytes);
    txBuffer[numBytes + 1] = ' ';
    txBuffer[numBytes + 2] = ' ';

    auto response_part2 = doJtagCommand(txBuffer.get(), numBytes + 3, 1);
    if (!response_part2)
        throw jtag_exception();
}
