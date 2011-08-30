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

void jtag::jtagCheck(int status)
{
    unixCheck(status, JTAG_CAUSE, NULL);
}

void jtag::restoreSerialPort()
{
  if (!is_usb && jtagBox >= 0 && oldtioValid)
      tcsetattr(jtagBox, TCSANOW, &oldtio);
}

jtag::jtag(void)
{
  jtagBox = 0;
  device_name = 0;
  oldtioValid = is_usb = false;
  ctrlPipe = -1;
}

jtag::jtag(const char *jtagDeviceName, char *name, emulator type)
{
    struct termios newtio;

    jtagBox = 0;
    oldtioValid = is_usb = false;
    ctrlPipe = -1;
    device_name = name;
    emu_type = type;
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
	unixCheck(jtagBox, "Failed to open %s", jtagDeviceName);

	// save current serial port settings and plan to restore them on exit
	jtagCheck(tcgetattr(jtagBox, &oldtio));
	oldtioValid = true;

	memset(&newtio, 0, sizeof(newtio));
	newtio.c_cflag = CS8 | CLOCAL | CREAD;

	// set baud rates in a platform-independent manner
	jtagCheck(cfsetospeed(&newtio, B19200));
	jtagCheck(cfsetispeed(&newtio, B19200));

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
	jtagCheck(tcflush(jtagBox, TCIFLUSH));
	jtagCheck(tcsetattr(jtagBox,TCSANOW,&newtio));
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
	//jtagCheck(selected);

	if (selected == 0)
	    return actual;

	ssize_t thisread = read(jtagBox, &buffer[actual], count - actual);
        if ((thisread < 0) && (errno == EAGAIN))
            continue;
	jtagCheck(thisread);

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

    jtagCheck(tcgetattr(jtagBox, &tio));

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
	exit(1);
    }

    cfsetospeed(&tio, newPortSpeed);
    cfsetispeed(&tio, newPortSpeed);

    jtagCheck(tcsetattr(jtagBox,TCSANOW,&tio));
    jtagCheck(tcflush(jtagBox, TCIFLUSH));
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
        exit(-1);
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

                check(jtagWrite(BFDmemorySpaceOffset[memtype] + addr,
                                page_size,
                                buf),
                      "Error writing to target");
                // No need for statusOut here, since jtagWrite does it.
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

        check(is_verified, "\nVerification failed!");
    }
}

void jtag::jtagWriteFuses(char *fuses)
{
    int temp[3];
    uchar fuseBits[3];
    uchar *readfuseBits;
    unsigned int c;

    check(fuses,
          "Error: No fuses string given");

    // Convert fuses to hex values (this avoids endianess issues)
    c = sscanf(fuses, "%02x%02x%02x", temp+2, temp+1, temp );
    check(c == 3,
          "Error: Fuses specified are not in hexidecimal");

    fuseBits[0] = (uchar)temp[0];
    fuseBits[1] = (uchar)temp[1];
    fuseBits[2] = (uchar)temp[2];

    statusOut("\nWriting Fuse Bytes:\n");
    jtagDisplayFuses(fuseBits);

    enableProgramming();

    check(jtagWrite(FUSE_SPACE_ADDR_OFFSET + 0, 3, fuseBits),
          "Error writing fuses");

    readfuseBits = jtagRead(FUSE_SPACE_ADDR_OFFSET + 0, 3);

    disableProgramming();

    check(memcmp(fuseBits, readfuseBits, 3) == 0,
          "Error verifying written fuses");

    delete [] readfuseBits;
}


void jtag::jtagReadFuses(void)
{
    uchar *fuseBits = 0;

    enableProgramming();
    statusOut("\nReading Fuse Bytes:\n");
    fuseBits = jtagRead(FUSE_SPACE_ADDR_OFFSET + 0, 3);
    disableProgramming();

    check(fuseBits, "Error reading fuses");

    jtagDisplayFuses(fuseBits);

    delete [] fuseBits;
}


void jtag::jtagDisplayFuses(uchar *fuseBits)
{
    statusOut("  Extended Fuse byte -> 0x%02x\n", fuseBits[2]);
    statusOut("      High Fuse byte -> 0x%02x\n", fuseBits[1]);
    statusOut("       Low Fuse byte -> 0x%02x\n", fuseBits[0]);
}


void jtag::jtagWriteLockBits(char *lock)
{
    int temp[1];
    uchar lockBits[1];
    uchar *readlockBits;
    unsigned int c;

    check(lock,
          "Error: No lock bit string given");

    check(strlen(lock) == 2,
          "Error: Fuses must be one byte exactly");

    // Convert lockbits to hex value
    c = sscanf(lock, "%02x", temp);
    check(c == 1,
          "Error: Fuses specified are not in hexidecimal");

    lockBits[0] = (uchar)temp[0];

    statusOut("\nWriting Lock Bits -> 0x%02x\n", lockBits[0]);

    enableProgramming();

    check(jtagWrite(LOCK_SPACE_ADDR_OFFSET + 0, 1, lockBits),
          "Error writing lockbits" );

    readlockBits = jtagRead(LOCK_SPACE_ADDR_OFFSET + 0, 1);

    disableProgramming();

    check(memcmp(lockBits, readlockBits, 1) == 0,
          "Error verifying written lock bits");

    delete [] readlockBits;
}


void jtag::jtagReadLockBits(void)
{
    uchar *lockBits = 0;

    enableProgramming();
    statusOut("\nReading Lock Bits:\n");
    lockBits = jtagRead(LOCK_SPACE_ADDR_OFFSET + 0, 1);
    disableProgramming();

    check(lockBits, "Error reading lock bits");

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
