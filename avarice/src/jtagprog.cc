/*
 *	avarice - The "avarice" program.
 *	Copyright (C) 2001 Scott Finneran
 *      Copyright (C) 2002 Intel Corporation
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
#include <bfd.h>

#include "avarice.h"
#include "jtag.h"

// Allocate 1 meg for image buffer. This is where the file data is
// stored before writing occurs.
#define MAX_IMAGE_SIZE 1000000


// This flag is used to select the address space values for the
// jtagRead() and jtagWrite() commands.
bool programmingEnabled = false;


// Enumerations for target memory type.
typedef enum {
    MEM_FLASH = 0,
    MEM_EEPROM = 1,
    MEM_RAM = 2,
} BFDmemoryType;

const char *BFDmemoryTypeString[] = {
    "FLASH", 
    "EEPROM",
    "RAM",
};

const int BFDmemorySpaceOffset[] = {
    FLASH_SPACE_ADDR_OFFSET,
    EEPROM_SPACE_ADDR_OFFSET,
    DATA_SPACE_ADDR_OFFSET,
};


// Struct that holds the memory image. We read from file using BFD
// into this struct, then pass the entire struct to the target writer.
typedef struct {
    uchar image[MAX_IMAGE_SIZE];
    int last_address;
    int first_address;
    bool first_address_ok;
    bool has_data;
} BFDimage;


void enableProgramming(void)
{
    programmingEnabled = true;
    check(doSimpleJtagCommand(0xa3, 1), 
	      "JTAG ICE: Failed to enable programming");
}


void disableProgramming(void)
{
    programmingEnabled = false;
    check(doSimpleJtagCommand(0xa4, 1), 
	  "JTAG ICE: Failed to disable programming");
}


void eraseProgramMemory(void)
{
    check(doSimpleJtagCommand(0xa5, 1), 
	  "JTAG ICE: Failed to erase program memory");
}

void eraseProgramPage(unsigned long address)
{
    uchar *response = NULL;
    uchar command[] = { 0xa1, 0, 0, 0, JTAG_EOM };

    command[1] = address >> 8;
    command[2] = address;

    response = doJtagCommand(command, sizeof(command), 1);
    check(response[0] == JTAG_R_OK, "Page erase failed\n");

    delete [] response;
}


// Check if file format is supported.
// return nonzero on errors.
static int check_file_format(bfd *file)
{
    char **matching;
    int done = 0;
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
        fprintf(stderr, "File format ambigious: %s\n",
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


unsigned int get_page_size(BFDmemoryType memtype)
{
    unsigned int page_size;
    switch( memtype ) {
    case MEM_FLASH:
        page_size = global_p_device_def->flash_page_size;
        break;
    case MEM_EEPROM:
        page_size = global_p_device_def->eeprom_page_size;
        break;
    default:
        page_size = 1;
        break;
    }
    return page_size;
}


// Return page address of 
static inline unsigned int page_addr(unsigned int addr, BFDmemoryType memtype)
{
    unsigned int page_size = get_page_size( memtype );
    return (unsigned int)(addr & (~(page_size - 1)));
}


// Return section flag string for bfd section. Useful for debugging.
char *get_section_flags(asection *section)
{
    char *comma = "";
    static char flagstr[255];
    char *p = flagstr;
#define PF(x, y) \
  if (section->flags & x) { p += sprintf( p, "%s%s", comma, y); comma = ", "; }

    PF (SEC_HAS_CONTENTS, "CONTENTS");
    PF (SEC_ALLOC, "ALLOC");
    PF (SEC_CONSTRUCTOR, "CONSTRUCTOR");
    PF (SEC_LOAD, "LOAD");
    PF (SEC_RELOC, "RELOC");
    PF (SEC_READONLY, "READONLY");
    PF (SEC_CODE, "CODE");
    PF (SEC_DATA, "DATA");
    PF (SEC_ROM, "ROM");
    PF (SEC_DEBUGGING, "DEBUGGING");
    PF (SEC_NEVER_LOAD, "NEVER_LOAD");
    PF (SEC_EXCLUDE, "EXCLUDE");
    PF (SEC_SORT_ENTRIES, "SORT_ENTRIES");
    PF (SEC_BLOCK, "BLOCK");
    PF (SEC_CLINK, "CLINK");
    PF (SEC_SMALL_DATA, "SMALL_DATA");
    PF (SEC_SHARED, "SHARED");
    PF (SEC_ARCH_BIT_0, "ARCH_BIT_0");
    PF (SEC_THREAD_LOCAL, "THREAD_LOCAL");
    return flagstr;
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
    unsigned int addr = section->lma;

    if ((section->flags & SEC_HAS_CONTENTS) && 
        ((section->flags & SEC_ALLOC) || (section->flags & SEC_LOAD)))
    {
        if (section->lma < DATA_SPACE_ADDR_OFFSET) // < 0x80...
            sectmemtype = MEM_FLASH;
        else if (section->lma < EEPROM_SPACE_ADDR_OFFSET) // < 0x81...
            sectmemtype = MEM_RAM;
        else if (section->lma < FUSE_SPACE_ADDR_OFFSET) // < 0x82...
            sectmemtype = MEM_EEPROM;
        
        if (memtype == sectmemtype)
            return (section->lma &~ ADDR_SPACE_MASK);
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

    // If section is empty (although unexpected) return
    if (! section)
        return;

    // Get information about section
    name = bfd_get_section_name(file, section);
    size = bfd_get_section_size_before_reloc(section);
  
    if ((addr = get_section_addr(section, memtype)) != 0xffffff)
    {
        debugOut("Getting section contents, addr=0x%lx size=0x%lx\n",
                 addr, size);
        // Read entire section into image buffer, at correct byte address.
        bfd_get_section_contents(file, section, &image->image[addr], 0, size);

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
    else
    {
        debugOut("%s Image create: No data for section %s (vma=0x%lx, "
                 "lma=0x%lx , size=0x%lx, flags=%s).\n", 
                 BFDmemoryTypeString[memtype], name, section->vma,
                 section->lma, size, get_section_flags(section));
    }
}


static void jtag_flash_image(BFDimage *image, BFDmemoryType memtype)
{
    unsigned int page_size = get_page_size(memtype);

    // It is possible to write EEPROM using three pages in one chunk.
    if (memtype == MEM_EEPROM)
            page_size =0x0f;//*= 4;

    // First address must start on page boundary.
    unsigned int addr = page_addr(image->first_address, memtype);

    if (! image->has_data)
    {
        fprintf(stderr, "File contains no data.\n");
        exit(-1);
    }

    while (addr <= image->last_address-1)
    {
        // Must also convert address to gcc-hacked addr for jtagWrite
        debugOut("Writing page at addr 0x%.4lx size 0x%lx\n", 
                 addr, page_size);
        check(jtagWrite(BFDmemorySpaceOffset[memtype] + addr, 
                        page_size,
                        &image->image[addr]),
              "Error writing to target");
        addr += page_size;
    }

    statusOut("\n");
    statusFlush();
}


void downloadToTarget(const char* filename)
{
    // Basically, we just open the file and copy blocks over to the JTAG
    // box.
    struct stat ifstat;
    char *target = NULL;
    char *default_target = "binary";
    unsigned int page_size;
    bool done = 0;
    bfd *file;
    asection *p;

    BFDimage flashimg, eepromimg;

    flashimg.last_address = 0;
    flashimg.first_address = 0;
    flashimg.first_address_ok = false;
    flashimg.has_data = false;
    memset(flashimg.image, 0x00, MAX_IMAGE_SIZE);

    eepromimg.last_address = 0;
    eepromimg.first_address = 0;
    eepromimg.first_address_ok = false;
    eepromimg.has_data = false;
    memset(eepromimg.image, 0x00, MAX_IMAGE_SIZE);
    

    unixCheck(stat(filename, &ifstat), "Can't stat() file %s", filename);

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
            exit(-1);
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

    // First erase the flash
    enableProgramming();
    eraseProgramMemory();

    // And then send down the new image.

    statusOut("Downloading to target.");
    statusFlush();
    
    // Create RAM image by reading all sections in file
    p = file->sections;
    while( p )
    {
        jtag_create_image(file, p, &flashimg, MEM_FLASH);
        jtag_create_image(file, p, &eepromimg, MEM_EEPROM);
        p = p->next;
    }

#ifdef DEBUG_IMAGE
    debugOut("DEBUG_IMAGE\n");
    // Debug: Create file with raw dump of image
    FILE *fh = fopen( "image.txt", "w");
    int c = 0;
    int d=0;
    fprintf(fh, "%.4lx: ", c);
    while (c<eepromimg.last_address)
    {
        fprintf(fh, "%.2x ", eepromimg.image[c++]);
        if (d++==20)
        {
            fprintf(fh, "\n");
            fprintf(fh, "%.4lx: ", c);
            d=0;
        }
    }
    fclose(fh);
#endif

    // Write the complete RAM image to the device.
    if (flashimg.has_data)
        jtag_flash_image(&flashimg, MEM_FLASH);
    if (eepromimg.has_data)
        jtag_flash_image(&eepromimg, MEM_EEPROM);
    
    disableProgramming();

    unixCheck(bfd_close(file), "Error closing %s", filename);

    statusOut("\nDownload complete.\n");
}
