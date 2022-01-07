/*
 *	avarice - The "avarice" program.
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
 * This file contains miscellaneous routines for the mkII protocol.
 *
 * $Id$
 */


#include <cstdio>
#include <cstring>

#include "avarice.h"
#include "jtag2.h"

void jtag2::setJtagParameter(uchar item, uchar *newValue, int valSize)
{
    if (valSize > 4)
        throw jtag_exception("Parameter too large in setJtagParameter");

    /*
     * As the maximal parameter length is 4 bytes, we use a fixed-length
     * buffer, as opposed to malloc()ing it.
     */
    uchar buf[2 + 4] = {CMND_SET_PARAMETER, item, 0, 0, 0, 0};
    memcpy(buf + 2, newValue, valSize);

    uchar* resp;
    try
    {
        int respsize;
        doJtagCommand(buf, valSize + 2, resp, respsize);
    }
    catch (jtag_exception& e)
    {
        fprintf(stderr, "set parameter command failed: %s\n", e.what());
        throw;
    }

    delete [] resp;
}

/*
 * Get a JTAG ICE parameter.  Caller must delete [] the response.  Note
 * that the response still includes the response code at index 0 (to be
 * ignored).
 */
void jtag2::getJtagParameter(const uchar item, uchar *&resp, int &respSize)
{
    /*
     * As the maximal parameter length is 4 bytes, we use a fixed-length
     * buffer, as opposed to malloc()ing it.
     */
    const uchar buf[] = {CMND_GET_PARAMETER, item};
    try
    {
        doJtagCommand(buf, 2, resp, respSize);
    }
    catch (jtag_exception& e)
    {
        fprintf(stderr, "get parameter command failed: %s\n", e.what());
        throw;
    }
    if (resp[0] != RSP_PARAMETER || respSize <= 1)
        throw jtag_exception("unexpected response to get paremater command");
}


