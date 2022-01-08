/*
 *	avarice - The "avarice" program.
 *	Copyright (C) 2012 Joerg Wunsch
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
 * This file contains miscellaneous routines for JTAGICE3 protocol.
 *
 * $Id$
 */

#include <cstdio>
#include <cstring>

#include "jtag3.h"

void jtag3::setJtagParameter(uchar scope, uchar section, uchar item, const uchar *newValue, int valSize) {
    uchar buf[6 + valSize]; // FIXME: variable length arrays not supported in C++
    buf[0] = scope;
    buf[1] = CMD3_SET_PARAMETER;
    buf[2] = 0;
    buf[3] = section;
    buf[4] = item;
    buf[5] = valSize;
    memcpy(buf + 6, newValue, valSize);

    uchar *resp;
    int respsize;
    try {
        doJtagCommand(buf, valSize + 6, "set parameter", resp, respsize);
    } catch (jtag_exception &e) {
        fprintf(stderr, "set parameter command failed: %s\n", e.what());
        throw;
    }

    delete[] resp;
}

/*
 * Get a JTAG ICE parameter.  Caller must delete [] the response.  Note
 * that the actual response data returned starts at offset 2.
 */
void jtag3::getJtagParameter(uchar scope, uchar section, uchar item, int length, uchar *&resp) {
    const uchar buf[6] = {scope, CMD3_GET_PARAMETER, 0, section, item, static_cast<uchar>(length)};

    int respsize;
    try {
        doJtagCommand(buf, 6, "get parameter", resp, respsize);
    } catch (jtag_exception &e) {
        fprintf(stderr, "get parameter command failed: %s\n", e.what());
        throw;
    }
    if (resp[1] != RSP3_DATA || respsize < 3 + length) {
        debugOut("unexpected response to get parameter command: 0x%02x\n", resp[1]);
        delete[] resp;
        throw jtag_exception("unexpected response to get parameter command");
    }
}
