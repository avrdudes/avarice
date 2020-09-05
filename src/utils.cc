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

#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "avarice.h"
#include "remote.h"

bool debugMode = false;

void vdebugOut(const char *fmt, va_list args)
{
    if (debugMode)
    {
        (void)vfprintf(stderr, fmt, args);
    }
}

void debugOut(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vdebugOut(fmt, args);
    va_end(args);
}

void vstatusOut(const char *fmt, va_list args)
{
    vprintf(fmt, args);
}

void statusOut(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vstatusOut(fmt, args);
    va_end(args);
}

void statusFlush()
{
    fflush(stdout);
}

void unknownDevice(unsigned int devid, bool generic)
{
  fprintf(stderr,
          "Device ID 0x%04x is not known to AVaRICE.\n"
          "Please ask for it being added to the code",
          devid);
  if (generic)
      fprintf(stderr,
              ", or use\n"
              "-P <device> to override the automatic decision.\n");
  else
      fprintf(stderr, ".\n");
}
