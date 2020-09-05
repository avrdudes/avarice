/*
 *	avarice - The "avarice" program.
 *	Copyright (C) 2001 Scott Finneran
 *      Copyright (C) 2002 Intel Corporation
 *	Copyright (C) 2005 Joerg Wunsch
 *
 *      File format support using BFD contributed and copyright 2003
 *      Nils Kr. Strom
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


#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>

#if ENABLE_TARGET_PROGRAMMING
#  include "autoconf.h"
#  include <bfd.h>
#endif

#include "avarice.h"
#include "jtag.h"
#include "jtag1.h"

#if ENABLE_TARGET_PROGRAMMING
// The API changed for this in bfd.h. This is a work around.
#ifndef bfd_get_section_name
#  define bfd_get_section_name(bfd, ptr) bfd_section_name(ptr)
#endif
#ifndef bfd_get_section_size
#  define bfd_get_section_size bfd_section_size
#endif

static void initImage(BFDimage *image)
{
    unsigned int i;
    image->last_address = 0;
    image->first_address = 0;
    image->first_address_ok = false;
    image->has_data = false;
    for (i=0;i<MAX_IMAGE_SIZE;i++)
    {
        image->image[i].val  = 0x00;
        image->image[i].used = false;
    }
}


// Check if file format is supported.
// return nonzero on errors.
static int check_file_format(bfd *file)
{
    char **matching;
    int err = 1;

    // Check if archive, not plain file.
    if (bfd_check_format(file, bfd_archive) == true)
    {
        fprintf(stderr, "Input file is archive\n");
    }

    else if (bfd_check_format_matches (file, bfd_object, &matching))
        err = 0;

    else if (bfd_get_error () == bfd_error_file_ambiguously_recognized)
    {
        fprintf(stderr, "File format ambiguous: %s\n",
                bfd_errmsg(bfd_get_error()));
    }

    else if (bfd_get_error () != bfd_error_file_not_recognized)
    {
        fprintf(stderr, "File format not supported: %s\n",
                bfd_errmsg(bfd_get_error()));
    }

    else if (bfd_check_format_matches (file, bfd_core, &matching))
        err = 0;

    return err;
}


// Get address of section.
// We have two different scenarios (both with same result).
//   1. vma == lma : Normal section
//      Return real address (mask gcc-hacked MSB's away).
//
//   2. vma != lma : For sections to be relocated (e.g. .data)
//      lma is the address where the duplicate initialized data is stored.
//      vma is the destination address after relocation.
//      Return real address (mask gcc-hacked MSB's away).
//
//   3. Not correct memory type: return 0x800000.
//
static unsigned int get_section_addr(asection *section, BFDmemoryType memtype)
{
    BFDmemoryType sectmemtype;

    if ((section->flags & SEC_HAS_CONTENTS) &&
        ((section->flags & SEC_ALLOC) || (section->flags & SEC_LOAD)))
    {
        if (section->lma < DATA_SPACE_ADDR_OFFSET) // < 0x80...
            sectmemtype = MEM_FLASH;
        else if (section->lma < EEPROM_SPACE_ADDR_OFFSET) // < 0x81...
            sectmemtype = MEM_RAM;
        else if (section->lma < FUSE_SPACE_ADDR_OFFSET) // < 0x82...
            sectmemtype = MEM_EEPROM;
        else			// e.g. .fuses
	    return 0xffffff;

        if (memtype == sectmemtype) {
            if (sectmemtype == MEM_FLASH) {
                /* Don't mask the lma or you will not be able to handle more
                   than 64K of flash. */
                return (section->lma);
            }
            return (section->lma &~ ADDR_SPACE_MASK);
        }
        else
            return 0xffffff;
    }
    else
        return 0xffffff;
}



// Add section of memtype BFDmemoryType to image.
static void jtag_create_image(bfd *file, asection *section,
                              BFDimage *image,
                              BFDmemoryType memtype)
{
    const char *name;
    unsigned int addr;
    unsigned int size;
    static uchar buf[MAX_IMAGE_SIZE];
    unsigned int i;

    // If section is empty (although unexpected) return
    if (! section)
        return;

    // Get information about section
    name = bfd_get_section_name(file, section);
    size = bfd_get_section_size(section);

    if ((addr = get_section_addr(section, memtype)) != 0xffffff)
    {
        debugOut("Getting section contents, addr=0x%lx size=0x%lx\n",
                 addr, size);

        // Read entire section into buffer, at correct byte address.
        bfd_get_section_contents(file, section, buf, 0, size);

        // Copy section into memory struct. Mark as used.
        for (i=0; i<size; i++)
        {
            unsigned int c = i+addr;
            image->image[c].val = buf[i];
            image->image[c].used = true;
        }

        // Remember last address in image
        if (addr+size > image->last_address)
            image->last_address = addr+size;

        // Remember first address in image
        if ((! image->first_address_ok) || (addr < image->first_address))
        {
            image->first_address = addr;
            image->first_address_ok = true;
        }
        debugOut("%s Image create: Adding %s at addr 0x%lx size %d (0x%lx)\n",
                 BFDmemoryTypeString[memtype], name, addr, size, size);

        // Indicate image has data
        image->has_data = true;
    }
}
#endif	// ENABLE_TARGET_PROGRAMMING


void jtag1::enableProgramming(void)
{
    programmingEnabled = true;
    if (!doSimpleJtagCommand(0xa3, 1))
    {
        fprintf(stderr, "JTAG ICE: Failed to enable programming\n");
        throw jtag_exception();
    }
}


void jtag1::disableProgramming(void)
{
    programmingEnabled = false;
    if (!doSimpleJtagCommand(0xa4, 1))
    {
        fprintf(stderr, "JTAG ICE: Failed to disable programming\n");
        throw jtag_exception();
    }
}


// This is really a chip-erase which erases flash, lock-bits and eeprom
// (unless the save-eeprom fuse is set).
void jtag1::eraseProgramMemory(void)
{
    if (!doSimpleJtagCommand(0xa5, 1))
    {
        fprintf(stderr, "JTAG ICE: Failed to erase program memory\n");
        throw jtag_exception();
    }
}

void jtag1::eraseProgramPage(unsigned long address)
{
    uchar *response = NULL;
    uchar command[] = { 0xa1, 0, 0, 0, JTAG_EOM };

    command[1] = address >> 8;
    command[2] = address;

    response = doJtagCommand(command, sizeof(command), 1);
    if (response[0] != JTAG_R_OK)
    {
        fprintf(stderr, "Page erase failed\n");
        throw jtag_exception();
    }

    delete [] response;
}


void jtag1::downloadToTarget(const char* filename, bool program, bool verify)
{
#if ENABLE_TARGET_PROGRAMMING
    // Basically, we just open the file and copy blocks over to the JTAG
    // box.
    struct stat ifstat;
    const char *target = NULL;
    const char *default_target = "binary";
    unsigned int page_size;
    bool done = 0;
    bfd *file;
    asection *p;

    static BFDimage flashimg, eepromimg;

    initImage(&flashimg);
    initImage(&eepromimg);

    flashimg.name = BFDmemoryTypeString[MEM_FLASH];
    eepromimg.name = BFDmemoryTypeString[MEM_EEPROM];

    if (stat(filename, &ifstat) < 0)
        throw jtag_exception("Can't stat() image file");

    // Open the input file.
    bfd_init();

    // Auto detect file format by a loop iterated at most two times.
    //   1. Auto-detect file format.
    //   2. If auto-detect failed, assume binary and iterate once more over
    //      loop.
    while (! done)
    {
        file = bfd_openr(filename, target);
        if (! file)
        {
            fprintf( stderr, "Could not open input file %s:%s\n", filename,
                     bfd_errmsg(bfd_get_error()) );
            return;
        }

        // Check if file format is supported. If not, go for binary mode.
        else if (check_file_format(file))
        {
            // File format detection failed. Assuming binary file
            // BFD section flags are CONTENTS,ALLOC,LOAD,DATA
            // We must force CODE in stead of DATA
            fprintf(stderr, "Warning: File format unknown, assuming "
                    "binary.\n");
            target = default_target;
        }

        else
            done = 1;
    }


    // Configure for JTAG download/programming

    // Set the flash page and eeprom page sizes (These are device dependent)
    page_size = get_page_size(MEM_FLASH);

    debugOut("Flash page size: 0x%0x\nEEPROM page size: 0x%0x\n",
             page_size, get_page_size(MEM_EEPROM));

    setJtagParameter(JTAG_P_FLASH_PAGESIZE_LOW, page_size & 0xff);
    setJtagParameter(JTAG_P_FLASH_PAGESIZE_HIGH, page_size >> 8);

    setJtagParameter(JTAG_P_EEPROM_PAGESIZE,
                     get_page_size(MEM_EEPROM));

    // Create RAM image by reading all sections in file
    p = file->sections;
    while (p)
    {
        jtag_create_image(file, p, &flashimg, MEM_FLASH);
        jtag_create_image(file, p, &eepromimg, MEM_EEPROM);
        p = p->next;
    }

    enableProgramming();

    // Write the complete FLASH/EEPROM images to the device.
    if (flashimg.has_data)
        jtag_flash_image(&flashimg, MEM_FLASH, program, verify);
    if (eepromimg.has_data)
        jtag_flash_image(&eepromimg, MEM_EEPROM, program, verify);

    disableProgramming();

    (void)(bfd_close(file));

    statusOut("\nDownload complete.\n");
#else  // !ENABLE_TARGET_PROGRAMMING
    (void)filename;
    (void)program;
    (void)verify;
    statusOut("\nDownload not done.\n");
    throw jtag_exception("AVaRICE was not configured for target programming");
#endif // ENABLE_TARGET_PROGRAMMING
}
