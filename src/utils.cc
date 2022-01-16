/*
 *	avarice - The "avarice" program.
 *	Copyright (C) 2001 Scott Finneran
 *      Copyright (C) 2002 Intel Corporation
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
 */

#include <cstdarg>
#include <cstdio>
#include <inttypes.h>

#include "avarice.h"

bool debugMode = false;

void vdebugOut(const char *fmt, va_list args) {
    if (!debugMode) return;
    (void)vfprintf(stderr, fmt, args);
}

void debugOut(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vdebugOut(fmt, args);
    va_end(args);
}

void debugOutBufHex(const char* prefix, const void* data, size_t data_size) {
    if (!debugMode) return;

    const auto* data_bytes = static_cast<const uint8_t*>(data);
    debugOut(prefix);
    for (size_t i = 0; i < data_size; ++i)
        debugOut("%.2X ", data_bytes[i]);
    debugOut("\n");
}

void vstatusOut(const char *fmt, va_list args) { vprintf(fmt, args); }

void statusOut(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vstatusOut(fmt, args);
    va_end(args);
}

void statusFlush() { fflush(stdout); }
