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
 * This file contains functions for interfacing with the JTAG box.
 *
 * $Id$
 */


#include "avarice.h"
#include "jtag1.h"

void jtag1::setJtagParameter(uchar item, uchar newValue)
{
    const uchar command[] = {'B', item, newValue, JTAG_EOM };

    auto response = doJtagCommand(command, sizeof(command), 1);
    if (response[0] != JTAG_R_OK)
        throw jtag_exception("Unknown parameter");
}

uchar jtag1::getJtagParameter(uchar item)
{
    const uchar command[] = {'q', item, JTAG_EOM };

    auto response = doJtagCommand(command, sizeof(command), 2);
    if (response[1] != JTAG_R_OK)
        throw jtag_exception("Unknown parameter");

    return response[0];
}


