/*
 *	avarice - The "avarice" program.
 *	Copyright (C) 2001 Scott Finneran
 *      Copyright (C) 2002, 2003, 2004 Intel Corporation
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
 * This file implements those parts of class "jtag" that are common
 * for both, the mkI and mkII protocol.
 *
 * $Id$
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "avarice.h"
#include "jtag.h"

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

/*
 * Generic functions applicable to both, the mkI and mkII ICE.
 */

void jtag::restoreSerialPort()
{
  if (!is_usb && jtagBox >= 0 && oldtioValid)
      tcsetattr(jtagBox, TCSANOW, &oldtio);
}

jtag::jtag(void)
{
  jtagBox = 0;
  softbp_only = is_xmega = oldtioValid = is_usb = false;
}

jtag::jtag(const char *jtagDeviceName, char *name, emulator type)
{
    struct termios newtio;

    jtagBox = 0;
    oldtioValid = is_usb = false;
    device_name = name;
    emu_type = type;
    programmingEnabled = 0;
    deviceDef = NULL;
    if (strncmp(jtagDeviceName, "usb", 3) == 0)
      {
#ifdef HAVE_LIBUSB
	is_usb = true;
	openUSB(jtagDeviceName);
#else
	throw "avarice has not been compiled with libusb support\n";
#endif
      }
    else
      {
	// Open modem device for reading and writing and not as controlling
	// tty because we don't want to get killed if linenoise sends
	// CTRL-C.
	jtagBox = open(jtagDeviceName, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (jtagBox < 0)
	{
	    fprintf(stderr, "Failed to open %s", jtagDeviceName);
	    throw jtag_exception();
	}

	// save current serial port settings and plan to restore them on exit
	if (tcgetattr(jtagBox, &oldtio) < 0)
	    throw jtag_exception();
	oldtioValid = true;

	memset(&newtio, 0, sizeof(newtio));
	newtio.c_cflag = CS8 | CLOCAL | CREAD;

	// set baud rates in a platform-independent manner
	if (cfsetospeed(&newtio, B19200) < 0 ||
	    cfsetispeed(&newtio, B19200) < 0)
	    throw jtag_exception();

	// IGNPAR  : ignore bytes with parity errors
	//           otherwise make device raw (no other input processing)
	newtio.c_iflag = IGNPAR;

	// Raw output.
	newtio.c_oflag = 0;

	// Raw input.
	newtio.c_lflag = 0;

	// The following configuration should cause read to return if 2
	// characters are immediately avaible or if the period between
	// characters exceeds 5 * .1 seconds.
	newtio.c_cc[VTIME]    = 5;     // inter-character timer unused
	newtio.c_cc[VMIN]     = 255;   // blocking read until VMIN character
	// arrives

	// now clean the serial line and activate the settings for the port
	if (tcflush(jtagBox, TCIFLUSH) < 0 ||
	    tcsetattr(jtagBox, TCSANOW, &newtio) < 0)
	    throw jtag_exception();
      }
}

// NB: the destructor is virtual; class jtag2 extends it
jtag::~jtag(void)
{
  restoreSerialPort();
}


int jtag::timeout_read(void *buf, size_t count, unsigned long timeout)
{
    char *buffer = (char *)buf;
    size_t actual = 0;

    while (actual < count)
    {
	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(jtagBox, &readfds);

	struct timeval tmout;
	tmout.tv_sec = timeout / 1000000;
	tmout.tv_usec = timeout % 1000000;

	int selected = select(jtagBox + 1, &readfds, NULL, NULL, &tmout);
        /* Even though select() is not supposed to set errno to EAGAIN
           (according to the linux man page), it seems that errno can be set
           to EAGAIN on some cygwin systems. Thus, we need to catch that
           here. */
        if ((selected < 0) && (errno == EAGAIN || errno == EINTR))
            continue;

	if (selected == 0)
	    return actual;

	ssize_t thisread = read(jtagBox, &buffer[actual], count - actual);
        if ((thisread < 0) && (errno == EAGAIN))
            continue;
	if (thisread < 0)
            throw jtag_exception();

	actual += thisread;
    }

    return count;
}

int jtag::safewrite(const void *b, int count)
{
  char *buffer = (char *)b;
  int actual = 0;
  int flags = fcntl(jtagBox, F_GETFL);

  fcntl(jtagBox, F_SETFL, 0); // blocking mode
  while (count > 0)
    {
      int n = write(jtagBox, buffer, count);

      if (n == -1 && errno == EINTR)
	continue;
      if (n == -1)
	{
	  actual = -1;
	  break;
	}

      count -= n;
      actual += n;
      buffer += n;
    }
  fcntl(jtagBox, F_SETFL, flags); 
  return actual;
}

/** Change bitrate of PC's serial port as specified by BIT_RATE_xxx in
    'newBitRate' **/
void jtag::changeLocalBitRate(int newBitRate)
{
    if (is_usb)
        return;

    // Change the local port bitrate.
    struct termios tio;

    if (tcgetattr(jtagBox, &tio) < 0)
        throw jtag_exception();

    speed_t newPortSpeed = B19200;
    // Linux doesn't support 14400. Let's hope it doesn't end up there...
    switch(newBitRate)
    {
    case 9600:
	newPortSpeed = B9600;
	break;
    case 19200:
	newPortSpeed = B19200;
	break;
    case 38400:
	newPortSpeed = B38400;
	break;
    case 57600:
	newPortSpeed = B57600;
	break;
    case 115200:
	newPortSpeed = B115200;
	break;
    default:
	debugOut("unsupported bitrate: %d\n", newBitRate);
        throw jtag_exception("unsupported bitrate");
    }

    cfsetospeed(&tio, newPortSpeed);
    cfsetispeed(&tio, newPortSpeed);

    if (tcsetattr(jtagBox,TCSANOW,&tio) < 0 ||
        tcflush(jtagBox, TCIFLUSH) < 0)
        throw jtag_exception();
}

static bool pageIsEmpty(BFDimage *image, unsigned int addr, unsigned int size,
                        BFDmemoryType memtype)
{
    bool emptyPage = true;

    // Check if page is used
    for (unsigned int idx=addr; idx<addr+size; idx++)
    {
        if (idx >= image->last_address)
            break;

        // 1. If this address existed in input file, mark as ! empty.
        // 2. If we are programming FLASH, and contents == 0xff, we need
        //    not program (is 0xff after erase).
        if (image->image[idx].used)
        {
            if (!((memtype == MEM_FLASH) &&
                  (image->image[idx].val == 0xff)))
            {
                emptyPage = false;
                break;
            }
        }
    }
    return emptyPage;
}


unsigned int jtag::get_page_size(BFDmemoryType memtype)
{
    unsigned int page_size;
    switch( memtype ) {
    case MEM_FLASH:
        page_size = deviceDef->flash_page_size;
        break;
    case MEM_EEPROM:
        page_size = deviceDef->eeprom_page_size;
        break;
    default:
        page_size = 1;
        break;
    }
    return page_size;
}


void jtag::jtag_flash_image(BFDimage *image, BFDmemoryType memtype,
                             bool program, bool verify)
{
    unsigned int page_size = get_page_size(memtype);
    static uchar buf[MAX_IMAGE_SIZE];
    unsigned int i;
    uchar *response = NULL;
    unsigned int addr;

    if (! image->has_data)
    {
        fprintf(stderr, "File contains no data.\n");
        return;
    }


    if (program)
    {
        // First address must start on page boundary.
        addr = page_addr(image->first_address, memtype);

        statusOut("Downloading %s image to target.", image->name);
        statusFlush();

        while (addr < image->last_address)
        {
            if (!pageIsEmpty(image, addr, page_size, memtype))
            {
                // Must also convert address to gcc-hacked addr for jtagWrite
                debugOut("Writing page at addr 0x%.4lx size 0x%lx\n",
                         addr, page_size);

                // Create raw data buffer
                for (i=0; i<page_size; i++)
                    buf[i] = image->image[i+addr].val;

                try
                {
                    jtagWrite(BFDmemorySpaceOffset[memtype] + addr,
                              page_size,
                              buf);
                }
                catch (jtag_exception& e)
                {
                    fprintf(stderr, "Error writing to target: %s\n",
                            e.what());
                }
            }

            addr += page_size;

            statusOut(".");
            statusFlush();
        }

        statusOut("\n");
        statusFlush();
    }

    if (verify)
    {
        bool is_verified = true;

        // First address must start on page boundary.
        addr = page_addr(image->first_address, memtype);

        statusOut("\nVerifying %s", image->name);
        statusFlush();

        while (addr < image->last_address)
        {
            // Must also convert address to gcc-hacked addr for jtagWrite
            debugOut("Verifying page at addr 0x%.4lx size 0x%lx\n",
                     addr, page_size);

            response = jtagRead(BFDmemorySpaceOffset[memtype] + addr,
                                page_size);

            // Verify buffer, but only addresses in use.
            for (i=0; i < page_size; i++)
            {
                unsigned int c = i + addr;
                if (image->image[c].used )
                {
                    if (image->image[c].val != response[i])
                    {
                        statusOut("\nError verifying target addr %.4x. "
                                  "Expect [0x%02x] Got [0x%02x]",
                                  c, image->image[c].val, response[i]);
                        statusFlush();
                        is_verified = false;
                    }
                }
            }

            addr += page_size;

            statusOut(".");
            statusFlush();
        }
        delete [] response;

        statusOut("\n");
        statusFlush();

        if (!is_verified)
        {
            fprintf(stderr, "\nVerification failed!\n");
            throw jtag_exception();
        }
    }
}

void jtag::jtagWriteFuses(char *fuses)
{
    int temp[3];
    uchar fuseBits[3];
    uchar *readfuseBits;
    unsigned int c;

    if (deviceDef->fusemap > 0x07)
    {
        fprintf(stderr, "Fuse byte writing not supported on this device.\n");
        return;
    }

    if (fuses == 0)
    {
        fprintf(stderr, "Error: No fuses string given");
        return;
    }

    // Convert fuses to hex values (this avoids endianess issues)
    c = sscanf(fuses, "%02x%02x%02x", temp+2, temp+1, temp );
    if (c != 3)
    {
        fprintf(stderr, "Error: Fuses specified are not in hexidecimal");
        return;
    }

    fuseBits[0] = (uchar)temp[0];
    fuseBits[1] = (uchar)temp[1];
    fuseBits[2] = (uchar)temp[2];

    statusOut("\nWriting Fuse Bytes:\n");
    jtagDisplayFuses(fuseBits);

    try
    {
        jtagWrite(FUSE_SPACE_ADDR_OFFSET + 0, 3, fuseBits);
    }
    catch (jtag_exception& e)
    {
        fprintf(stderr, "Error writing fuses: %s\n",
                e.what());
    }

    readfuseBits = jtagRead(FUSE_SPACE_ADDR_OFFSET + 0, 3);

    if (memcmp(fuseBits, readfuseBits, 3) != 0)
    {
        fprintf(stderr, "Error verifying written fuses");
    }

    delete [] readfuseBits;
}


static unsigned int countFuses(unsigned int fusemap)
{
    unsigned int nfuses = 0;

    for (unsigned int i = 15, mask = 0x8000;
         mask != 0;
         i--, mask >>= 1)
    {
        if ((fusemap & mask) != 0)
        {
            nfuses = i + 1;
            break;
        }
    }
    if (nfuses == 0)
    {
        fprintf(stderr, "Device has no fuses?  Confused.");
        throw jtag_exception();
    }

    return nfuses;
}


void jtag::jtagReadFuses(void)
{
    uchar *fuseBits = 0;

    statusOut("\nReading Fuse Bytes:\n");
    fuseBits = jtagRead(FUSE_SPACE_ADDR_OFFSET + 0,
                        countFuses(deviceDef->fusemap));

    jtagDisplayFuses(fuseBits);

    delete [] fuseBits;
}


void jtag::jtagActivateOcdenFuse(void)
{
    if (deviceDef->ocden_fuse == 0)
        return;                 // device without an OCDEN fuse

    unsigned int nfuses = countFuses(deviceDef->fusemap);

    if (nfuses > 3)
        statusOut("jtag::jtagActivateOcdenFuse(): "
                  "Device has more than 3 fuses: %d, cannot handle\n",
                  nfuses);

    uchar *fuseBits = 0;
    fuseBits = jtagRead(FUSE_SPACE_ADDR_OFFSET + 0, 3);

    unsigned int fusevect = (fuseBits[2] << 16) |
        (fuseBits[1] << 8) |
        fuseBits[0];

    if ((fusevect & deviceDef->ocden_fuse) != 0)
    {
        statusOut("\nEnabling on-chip debugging:\n");

        fusevect &= ~deviceDef->ocden_fuse; // clear bit
        fuseBits[2] = fusevect >> 16;
        fuseBits[1] = fusevect >> 8;
        fuseBits[0] = fusevect;
        unsigned int offset;
        if (deviceDef->ocden_fuse > 0x8000)
            offset = 2;
        else if (deviceDef->ocden_fuse > 0x80)
            offset = 1;
        else
            offset = 0;
        jtagWrite(FUSE_SPACE_ADDR_OFFSET + offset, 1, &fuseBits[offset]);
    }
    delete [] fuseBits;
}

void jtag::jtagDisplayFuses(uchar *fuseBits)
{
    if (deviceDef->fusemap <= 0x07)
    {
        // tinyAVR/megaAVR: low/high/[extended] fuse
        const char *fusenames[3] = { "       Low",
                                     "      High",
                                     "  Extended" };
        for (unsigned int i = 2, mask = 0x04;
             mask != 0;
             i--, mask >>= 1)
        {
            if ((deviceDef->fusemap & mask) != 0)
                statusOut("%s Fuse byte -> 0x%02x\n",
                          fusenames[i], fuseBits[i]);
        }
    }
    else
    {
        // Xmega: fuse0 ... fuse15 (or just some of them)
        for (unsigned int i = 15, mask = 0x8000;
             mask != 0;
             i--, mask >>= 1)
        {
            if ((deviceDef->fusemap & mask) != 0)
                statusOut("  Fuse byte %d -> 0x%02x\n",
                          i, fuseBits[i]);
        }
    }
}


void jtag::jtagWriteLockBits(char *lock)
{
    int temp[1];
    uchar lockBits[1];
    uchar *readlockBits;
    unsigned int c;

    if (lock == 0)
    {
        fprintf(stderr, "Error: No lock bit string given");
        return;
    }

    if (strlen(lock) != 2)
    {
        fprintf(stderr, "Error: Fuses must be one byte exactly");
        return;
    }

    // Convert lockbits to hex value
    c = sscanf(lock, "%02x", temp);
    if (c != 1)
    {
        fprintf(stderr, "Error: Fuses specified are not in hexidecimal");
        return;
    }

    lockBits[0] = (uchar)temp[0];

    statusOut("\nWriting Lock Bits -> 0x%02x\n", lockBits[0]);

    enableProgramming();

    try
    {
        jtagWrite(LOCK_SPACE_ADDR_OFFSET + 0, 1, lockBits);
    }
    catch (jtag_exception& e)
    {
        fprintf(stderr, "Error writing lockbits: %s\n",
                e.what());
    }

    readlockBits = jtagRead(LOCK_SPACE_ADDR_OFFSET + 0, 1);

    disableProgramming();

    if (memcmp(lockBits, readlockBits, 1) != 0)
    {
        fprintf(stderr, "Error verifying written lock bits");
    }

    delete [] readlockBits;
}


void jtag::jtagReadLockBits(void)
{
    uchar *lockBits = 0;

    enableProgramming();
    statusOut("\nReading Lock Bits:\n");
    lockBits = jtagRead(LOCK_SPACE_ADDR_OFFSET + 0, 1);
    disableProgramming();

    jtagDisplayLockBits(lockBits);

    delete [] lockBits;
}


void jtag::jtagDisplayLockBits(uchar *lockBits)
{
    statusOut("Lock bits -> 0x%02x\n\n", lockBits[0]);

    statusOut("    Bit 7 [ Reserved ] -> %d\n", (lockBits[0] >> 7) & 1);
    statusOut("    Bit 6 [ Reserved ] -> %d\n", (lockBits[0] >> 6) & 1);
    statusOut("    Bit 5 [ BLB12    ] -> %d\n", (lockBits[0] >> 5) & 1);
    statusOut("    Bit 4 [ BLB11    ] -> %d\n", (lockBits[0] >> 4) & 1);
    statusOut("    Bit 3 [ BLB02    ] -> %d\n", (lockBits[0] >> 3) & 1);
    statusOut("    Bit 2 [ BLB01    ] -> %d\n", (lockBits[0] >> 2) & 1);
    statusOut("    Bit 1 [ LB2      ] -> %d\n", (lockBits[0] >> 1) & 1);
    statusOut("    Bit 0 [ LB1      ] -> %d\n", (lockBits[0] >> 0) & 1);
}

bool jtag::addBreakpoint(unsigned int address, bpType type, unsigned int length)
{
    int bp_i;

    debugOut("BP ADD type: %d  addr: 0x%x ", type, address);


    // Perhaps we have already set this breakpoint, and it is just
    // marked as disabled In that case we don't need to make a new
    // one, just flag this one as enabled again
    bp_i = 0;
    while (!bp[bp_i].last)
      {
	  if ((bp[bp_i].address == address) && (bp[bp_i].type == type))
	    {
		bp[bp_i].enabled = true;
		debugOut("ENABLED\n");
		break;
            }
	  bp_i++;
      }

    // Was what we just did not successful, if so...
    if (!bp[bp_i].enabled || (bp[bp_i].address != address) || (bp[bp_i].type == type))
      {

	  // Uhh... we are out of space. Try to find a disabled one and just
	  // write over it.
	  if ((bp_i + 1) == MAX_TOTAL_BREAKPOINTS2)
            {
		bp_i = 0;
		// We can't remove enabled breakpoints, or ones that
		// have JUST been disabled. The just disabled ones
		// because they have to sync to the ICE
		while (bp[bp_i].enabled || bp[bp_i].toremove)
		  {
		      bp_i++;
		  }
            }

	  // Sorry.. out of room :(
	  if ((bp_i + 1) == MAX_TOTAL_BREAKPOINTS2)
            {
		debugOut("FAILED\n");
		return false;
            }

        if (bp[bp_i].last)
	  {
	      //See if we need to set the new endpoint
	      bp[bp_i + 1].last = true;
	      bp[bp_i + 1].enabled = false;
	      bp[bp_i + 1].address = 0;
	      bp[bp_i + 1].type = NONE;
	  }

        // bp_i now has the new breakpoint we are going to use.
        bp[bp_i].last = false;
        bp[bp_i].enabled = true;
        bp[bp_i].address = address;
        bp[bp_i].type = type;

        // Is it a range breakpoint?
        // Range breakpoint needs to be aligned, and the length must
        // be representable as a bitmask.
        if ((length > 1) && ((type == READ_DATA) ||
                             (type == WRITE_DATA) ||
                             (type == ACCESS_DATA)))
	  {
	      int bitno = ffs((int)length);
	      unsigned int mask = 1 << (bitno - 1);
	      if (mask != length)
                {
		    debugOut("FAILED: length not power of 2 in range BP\n");
		    bp[bp_i].last = true;
		    bp[bp_i].enabled = false;
		    return false;
                }
	      mask--;
	      if ((address & mask) != 0)
                {
		    debugOut("FAILED: address in range BP is not base-aligned\n");
		    bp[bp_i].last = true;
		    bp[bp_i].enabled = false;
		    return false;
                }
	      mask = ~mask;

	      // add the breakpoint as a data mask.. only thing is we
	      // need to find it afterwards
	      if (!addBreakpoint(mask, DATA_MASK, 1))
                {
		    debugOut("FAILED\n");
		    bp[bp_i].last = true;
		    bp[bp_i].enabled = false;
		    bp[bp_i].has_mask = true;
		    return false;
                }

	      unsigned int j;
	      for(j = 0; !bp[j].last; j++)
                {
		    if ((bp[j].type == DATA_MASK) && (bp[bp_i].address == mask))
			break;
                }

	      bp[bp_i].mask_pointer = j;

	      debugOut("range BP ADDED: 0x%x/0x%x\n", address, mask);
	  }

      }

    // Is this breakpoint new?
    if (!bp[bp_i].icestatus)
      {
	  // Yup - flag it as something to download
	  bp[bp_i].toadd = true;
	  bp[bp_i].toremove = false;
      }
    else
      {
	  bp[bp_i].toadd = false;
	  bp[bp_i].toremove = false;
      }

    if (!layoutBreakpoints())
      {
	  debugOut("Not enough room in ICE for breakpoint. FAILED.\n");
	  bp[bp_i].enabled = false;
	  bp[bp_i].toadd = false;

	  if (bp[bp_i].has_mask)
              // these BP types have an associated mask
            {
		bp[bp[bp_i].mask_pointer].enabled = false;
		bp[bp[bp_i].mask_pointer].toadd = false;
            }

          return false;
      }

    return true;
}

PRAGMA_DIAG_PUSH
PRAGMA_DIAG_IGNORED("-Wunused-parameter")

bool jtag::deleteBreakpoint(unsigned int address, bpType type, unsigned int length)
{
    int bp_i;

    debugOut("BP DEL type: %d  addr: 0x%x ", type, address);

    bp_i = 0;
    while (!bp[bp_i].last)
      {
	  if ((bp[bp_i].address == address) && (bp[bp_i].type == type))
            {
		bp[bp_i].enabled = false;
		debugOut("DISABLED\n");
		break;
            }
	  bp_i++;
      }

    // If it somehow failed, got to tell..
    if (bp[bp_i].enabled || (bp[bp_i].address != address) || (bp[bp_i].type != type))
      {
	  debugOut("FAILED\n");
	  return false;
      }

    // Is this breakpoint actually enabled?
    if (bp[bp_i].icestatus)
      {
	  // Yup - flag it as something to delete
	  bp[bp_i].toadd = false;
	  bp[bp_i].toremove = true;
      }
    else
      {
	  bp[bp_i].toadd = false;
	  bp[bp_i].toremove = false;
      }

    return true;
}

PRAGMA_DIAG_POP

/*
 * This routine is where all the logic of what breakpoints go into the
 * ICE and what don't happens. When called it assumes all the "toadd"
 * breakpoints don't have valid "bpnum" attached to them. It then
 * attempts to fit all the needed breakpoints into the hardware
 * breakpoints first. If that won't fly it then adds software
 * breakpoints as needed... or just fails.
 *
 *
 * TODO: Some logic to decide which breakpoints change a lot and
 * should go in hardware, vs. which breakpoints are more fixed and
 * should just be done in code would be handy
 */
bool jtag::layoutBreakpoints(void)
{
    // remaining_bps is an array showing which breakpoints are still
    // available, starting at 0x00 Note 0x00 is software breakpoint
    // and will always be available in theory so just ignore the first
    // array element, it's meaningless...  FIXME: Slot 4 is set to
    // 'false', doesn't seem to work?
    bool remaining_bps[MAX_BREAKPOINTS2 + 2] = {false, true, true, true, false, false};
    int bp_i;
    uchar bpnum;
    bool softwarebps = true;
    bool hadroom = true;

    if (deviceDef->device_flags == DEVFL_NO_SOFTBP)
      {
	  softwarebps = false;
      }

    // Turn off everything but software breakpoints for DebugWire,
    // or for old firmware JTAGICEmkII with Xmega devices
    if (softbp_only)
      {
	  int k;
	  for (k = 1; k < MAX_BREAKPOINTS2 + 1; k++)
            {
		remaining_bps[k] = false;
            }
      }
    else if (is_xmega)
      {
	  // Xmega has only two hardware slots?
	  remaining_bps[BREAKPOINT2_XMEGA_UNAVAIL] = false;
      }

    bp_i = 0;
    while (!bp[bp_i].last)
      {
	  // check we have an enabled "stable" breakpoint that's not
	  // about to change
	  if (bp[bp_i].enabled && !bp[bp_i].toremove && bp[bp_i].icestatus)
            {
		remaining_bps[bp[bp_i].bpnum] = false;
            }

	  bp_i++;
      }

    // Do data watchpoints first
    for (bp_i = 0; !bp[bp_i].last; bp_i++)
      {
	  if (bp[bp_i].enabled && bp[bp_i].toadd &&
	      bp[bp_i].type == DATA_MASK)
	    {
		// Check if we have the mask slot available
		if (!remaining_bps[BREAKPOINT2_DATA_MASK])
		{
		    debugOut("Not enough room to store range breakpoint\n");
		    bp[bp[bp_i].mask_pointer].enabled = false;
		    bp[bp[bp_i].mask_pointer].toadd = false;
		    bp[bp_i].enabled = false;
		    bp[bp_i].toadd = false;
		    hadroom = false;
		    continue; // Skip this breakpoint
		}
		remaining_bps[BREAKPOINT2_DATA_MASK] = false;
		bp[bp_i].bpnum = BREAKPOINT2_DATA_MASK;
		continue;
	    }

        // Find the next data breakpoint that needs somewhere to live
	  if (bp[bp_i].enabled && bp[bp_i].toadd &&
	      ((bp[bp_i].type == READ_DATA) ||
	       (bp[bp_i].type == WRITE_DATA) ||
	       (bp[bp_i].type == ACCESS_DATA)))
	    {
		// Check if we have one of both slots available
		if (!remaining_bps[BREAKPOINT2_DATA_MASK] &&
		    !remaining_bps[BREAKPOINT2_FIRST_DATA])
		{
		    debugOut("Not enough room to store range breakpoint\n");
		    bp[bp_i].enabled = false;
		    bp[bp_i].toadd = false;
		    hadroom = false;
		    continue; // Skip this breakpoint
		}

		// Find next available slot
		bpnum = BREAKPOINT2_FIRST_DATA;
		while (!remaining_bps[bpnum] && (bpnum <= MAX_BREAKPOINTS2))
		  {
		      bpnum++;
		  }

		if (bpnum > MAX_BREAKPOINTS2)
		  {
		      debugOut("No more room for data breakpoints.\n");
		      hadroom = false;
		      break;
		  }

		bp[bp_i].bpnum = bpnum;
		remaining_bps[bpnum] = false;
	    }
      }

    // Do CODE breakpoints now

    bp_i = 0;
    while (!bp[bp_i].last)
      {
	  //Find the next spot to live in.
	  bpnum = 0x00;
	  while (!remaining_bps[bpnum] && (bpnum <= MAX_BREAKPOINTS2))
            {
		//debugOut("Slot %d full\n", bpnum);
		bpnum++;
            }

	  if (bpnum > MAX_BREAKPOINTS2)
            {
		if (softwarebps)
		  {
		      bpnum = 0x00;
		  }
		else
		  {
		      bpnum = 0xFF; // flag for NO MORE BREAKPOINTS
		  }
            }

	  // Find the next breakpoint that needs somewhere to live
	  if (bp[bp_i].enabled && bp[bp_i].toadd && (bp[bp_i].type == CODE))
            {
		if (bpnum == 0xFF)
		  {
		      debugOut("No more room for code breakpoints.\n");
		      hadroom = false;
		      break;
		  }
		bp[bp_i].bpnum = bpnum;
		remaining_bps[bpnum] = false;
            }

	  bp_i++;
      }

    return hadroom;
}

