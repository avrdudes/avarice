/*
 *	avarice - The "avarice" program.
 *	Copyright (C) 2001 Scott Finneran
 *      Copyright (C) 2002, 2003, 2004 Intel Corporation
 *	Copyright (C) 2005-2008 Joerg Wunsch
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
 * This file contains the JTAG ICE device descriptors of all supported
 * MCU types for both, the mkI and mkII protocol.
 */

#include "jtag.h"

#include <algorithm>
#include <cstring>
#include <cstdio>

const jtag_device_def_type &jtag_device_def_type::Find(const unsigned int id, std::string_view name) {

    // So we can do a case insensitive search in our database
    std::string lowercase_name;
    std::transform(name.begin(), name.end(), lowercase_name.begin(), ::tolower);

    const jtag_device_def_type *found_id = nullptr;
    if (id) {
        statusOut("Reported device ID: 0x%0X\n", id);
        found_id = *std::find_if(jtag_device_def_type::devices.cbegin(),
                                jtag_device_def_type::devices.cend(),
                                [&](const auto *dev) { return dev->device_id == id; });
        if( found_id == *jtag_device_def_type::devices.cend() )
            found_id = nullptr;
    }

    const jtag_device_def_type *found_name = nullptr;
    if (!lowercase_name.empty()) {
        debugOut("Looking for device: %s\n", lowercase_name.data());
        found_name = *std::find_if(jtag_device_def_type::devices.cbegin(),
                                  jtag_device_def_type::devices.cend(),
                                  [&](const auto *dev) { return dev->name == lowercase_name; });
        if( found_name == *jtag_device_def_type::devices.cend() )
            found_name = nullptr;
    }

    if( !found_id && !found_name ) {
        fprintf(stderr, "Device not found in internal database: id=0x%0X or name='%s'\n", id, name.data());
        throw jtag_exception();
    } else if( found_id && found_name ) {
        if( found_name->device_id != found_id->device_id ) {
            statusOut("Detected device ID: 0x%0X %s -- FORCED with %s\n", found_id->device_id,
                      found_id->name, name.data());
            return *found_name;
        } else {
            return *found_id;
        }
    } else if( found_id ) {
        return *found_id;
    } else if( found_name ) {
        return *found_name;
    } else {
        // All cases were covered, silence the compiler
        throw jtag_exception();
    }
}

void jtag_device_def_type::DumpAll() {
    fprintf(stderr, "%-17s  %10s  %8s  %8s\n", "Device Name", "Device ID", "Flash", "EEPROM");
    fprintf(stderr, "%-17s  %10s  %8s  %8s\n", "---------------", "---------", "-------",
            "-------");
    for( const auto* dev : jtag_device_def_type::devices ) {
        const unsigned eesize = dev->eeprom_page_size * dev->eeprom_page_count;

        if (eesize != 0 && eesize < 1024)
            fprintf(stderr, "%-17s      0x%04X  %4d KiB  %4.1f KiB\n", dev->name, dev->device_id,
                    dev->flash_page_size * dev->flash_page_count / 1024, eesize / 1024.0);
        else
            fprintf(stderr, "%-17s      0x%04X  %4d KiB  %4d KiB\n", dev->name, dev->device_id,
                    dev->flash_page_size * dev->flash_page_count / 1024, eesize / 1024);
    }
}
