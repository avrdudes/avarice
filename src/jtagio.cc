/*
 *	avarice - The "avarice" program.
 *	Copyright (C) 2001 Scott Finneran
 *      Copyright (C) 2002, 2003, 2004 Intel Corporation
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
#include <sys/time.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "avarice.h"
#include "jtag.h"
#include "ioreg.h"

jtag_device_def_type *global_p_device_def;

// Name of device controlled by JTAG ICE
char* device_name;

/* Device descriptor magic from Atmel's documents. Let's hope it's more
   accurate than the rest of that text... */
jtag_device_def_type deviceDefinitions[] = {
    {
        "atmega16",
        0x9403,
        128, 128,    // 16K flash 
        4,   128,    // 512 bytes EEPROM
        0x54,        // 21 interrupt vectors
        atmega16_io_registers,
        {
            JTAG_C_SET_DEVICE_DESCRIPTOR,
            { 0xCF, 0xAF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFF },
            { 0x87, 0x26, 0xFF, 0xEF, 0xFE, 0xFF, 0x3F, 0xFA },
            { 0x00, 0x00, 0x00, 0x00, 0x00, 0x2F, 0x00, 0x00 },
            { 0x00, 0x00, 0x00, 0x00, 0x00, 0x2F, 0x00, 0x00 },
            { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00 },
            { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00 },
            { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00 },
            { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00 },
            0x31,
            0x57,
            0x00,
            { 128, 0 },
            0,
            { 0x80, 0x1F, 0x00, 0x00 },
            0,
            { JTAG_EOM }
        }
    },
    // DEV_ATMEGA_162
    {
        "atmega162",
        0x9404,
        128, 128,    // 16K flash 
        4,   128,    // 512 bytes EEPROM
        0x70,        // 28 interrupt vectors
        atmega162_io_registers,
        {
            JTAG_C_SET_DEVICE_DESCRIPTOR,
            { 0xF7, 0x6F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
            { 0xF3, 0x66, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFA }, 
            { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 
            { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 
            { 0x02, 0x18, 0x00, 0x30, 0xF3, 0x0F, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
              0x00, 0x00, 0x00, 0x00 },
            { 0x02, 0x18, 0x00, 0x20, 0xF3, 0x0F, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00 },
            { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
              0x00, 0x00, 0x00, 0x00 },
            { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00 },
            0x04,
            0x57,
            0x00,
            { 128, 0 },
            4,
            { 0x80, 0x1F, 0x00, 0x00 },
            0x8B,
            { JTAG_EOM }
        }
    },
    // DEV_ATMEGA_169
    {
        "atmega169",
        0x9405,
        128, 128,    // 16K flash 
        4,   128,    // 512 bytes EEPROM
        0x5c,        // 23 interrupt vectors
        atmega169_io_registers,
        {
            JTAG_C_SET_DEVICE_DESCRIPTOR,
            { 0xFF, 0xFF, 0xFF, 0xF0, 0xDF, 0x3C, 0xBB, 0xE0 }, 
            { 0xB6, 0x6D, 0x1B, 0xE0, 0xDF, 0x3C, 0xBA, 0xE0 }, 
            { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 
            { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
            { 0x43, 0xDA, 0x00, 0xFF, 0xF7, 0x0F, 0x00, 0x00,
              0x00, 0x00, 0x4D, 0x07, 0x37, 0x00, 0x00, 0x00, 
              0xF0, 0xF0, 0xDE, 0x7B },
            { 0x43, 0xDA, 0x00, 0xFF, 0xF7, 0x0F, 0x00, 0x00,
              0x00, 0x00, 0x4D, 0x05, 0x36, 0x00, 0x00, 0x00,
              0xE0, 0xF0, 0xDE, 0x7B },
            { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
              0x00, 0x00, 0x00, 0x00 },
            { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00 }, 
            0x31,
            0x57,
            0x00,
            { 128, 0 },
            4,
            { 0x80, 0x1F, 0x00, 0x00 },
            0xFE,
            { JTAG_EOM }
        }
    },
    // DEV_ATMEGA_323
    {
        "atmega323",
        0x9501,
        128, 256,    // 32K flash 
        4,   256,    // 1K EEPROM
        0x50,        // 20 interrupt vectors
        NULL,        // io reg defs not defined yet
        {
            JTAG_C_SET_DEVICE_DESCRIPTOR,
            { 0xCF, 0xAF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFF }, 
            { 0x87, 0x26, 0xFF, 0xEF, 0xFE, 0xFF, 0x3F, 0xFA }, 
            { 0x00, 0x00, 0x00, 0x00, 0x00, 0x2F, 0x00, 0x00 }, 
            { 0x00, 0x00, 0x00, 0x00, 0x00, 0x2F, 0x00, 0x00 }, 
            { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
              0x00, 0x00, 0x00, 0x00 },
            { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00 },
            { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
              0x00, 0x00, 0x00, 0x00 },
            { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00 },
            0x31,
            0x57,
            0x00,
            { 128, 0 },
            0,
            { 0x00, 0x3F, 0x00, 0x00 },
            0,
            { JTAG_EOM }
        }
    },
    // DEV_ATMEGA_32
    {
        "atmega32",
        0x9502,
        128, 256,    // 32K flash 
        4,   256,    // 1K EEPROM
        0x54,        // 21 interrupt vectors
        atmega32_io_registers,
        {
            JTAG_C_SET_DEVICE_DESCRIPTOR,
            { 0xFF, 0x6F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, 
            { 0xFF, 0x66, 0xFF, 0xFF, 0xFF, 0xFF, 0xBF, 0xFA }, 
            { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 
            { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 
            { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
              0x00, 0x00, 0x00, 0x00 },
            { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00 },
            { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
              0x00, 0x00, 0x00, 0x00 },
            { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00 },
            0x31,
            0x57,
            0x00,
            { 128, 0 },
            4,
            { 0x00, 0x3F, 0x00, 0x00 },
            0,
            { JTAG_EOM }
        }
    },

    // DEV_ATMEGA_64
    {
        "atmega64",
        0x9602,
        256, 256,    // 64K flash 
        8,   256,    // 2K bytes EEPROM
        0x8c,        // 35 interrupt vectors
        NULL,        // io reg defs not defined yet
        {
            JTAG_C_SET_DEVICE_DESCRIPTOR,
            { 0xCF, 0x2F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
            { 0xCF, 0x27, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE },
            { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
            { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
            { 0x3E, 0xB5, 0x1F, 0x37, 0xFF, 0x1F, 0x21, 0x2F,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00 },
            { 0x3E, 0xB5, 0x0F, 0x27, 0xFF, 0x1F, 0x21, 0x27,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00 },
            { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00 },
            { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00 },
            0x22,
            0x68,
            0x00,
            { 0, 1 },
            8,
            { 0x00, 0x7E, 0x00, 0x00 },
            0x9D,
            { JTAG_EOM }
        }
    },
    // DEV_ATMEGA_128
    {
        "atmega128",
        0x9702,
        256, 512,    // 128K flash 
        8,   512,    // 4K bytes EEPROM
        0x8c,        // 35 interrupt vectors
        NULL,        // io reg defs not defined yet
        {
            JTAG_C_SET_DEVICE_DESCRIPTOR,
            { 0xCF, 0x2F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, 
            { 0xCF, 0x27, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE }, 
            { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 
            { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 
            { 0x3E, 0xB5, 0x1F, 0x37, 0xFF, 0x1F, 0x21, 0x2F,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
              0x00, 0x00, 0x00, 0x00 },
            { 0x3E, 0xB5, 0x0F, 0x27, 0xFF, 0x1F, 0x21, 0x27,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00 },
            { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
              0x00, 0x00, 0x00, 0x00 },
            { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00 }, 
            0x22,
            0x68,
            0x3B,
            { 0, 1 },
            8,
            { 0x00, 0xFE, 0x00, 0x00 },
            0x9D,
            { JTAG_EOM }
        }
    },
#if 0
    // DEV_UNKNOWN - what avarice came with (looks like a garbled 128)
    {
        "unknown",
        0x0,
        256, 512,    // 128K flash 
        8,   512,    // 4K EEPROM
        0x00,        // interrupt vectors
        NULL,        // io reg defs not defined yet
        {
            JTAG_C_SET_DEVICE_DESCRIPTOR,
            { 0xCF, 0x27, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
            { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
            { 0xCF, 0x27, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE },
            { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
            { 0x3E, 0xB5, 0x1F, 0x37, 0xFF, 0x1F, 0x21, 0x3F,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00 },
            { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00 },
            { 0x3E, 0xB5, 0x1F, 0x37, 0xFF, 0x1F, 0x21, 0x3F,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00 },
            { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00 }, 
            0x22,
            0x68,
            0x3B,
            { 0, 1 },
            8,
            { 0x00, 0xFE, 0x00, 0x00 },
            0x9D,
            { JTAG_EOM }
        }
    },
#endif
    // Termination record.
    { 
        NULL,                   // name
        0,                      // id
        0, 0,                   // flash
        0, 0,                   // eeprom
        0,                      // interrupt vectors
        NULL,                   // io reg defs
        { 0 }                   // device descriptor information
    }
};

// The file descriptor used while talking to the JTAG ICE
int jtagBox = 0;

// The initial serial port parameters. We restore them on exit.
static struct termios oldtio;
static bool oldtioValid = false;

void jtagCheck(int status)
{
    unixCheck(status, JTAG_CAUSE, NULL);
}

static void restoreSerialPort()
{
  if (jtagBox >= 0 && oldtioValid)
      tcsetattr(jtagBox, TCSANOW, &oldtio);
}

void initJtagPort(char *jtagDeviceName)
{
    struct termios newtio;

    // Open modem device for reading and writing and not as controlling
    // tty because we don't want to get killed if linenoise sends
    // CTRL-C.
    jtagBox = open(jtagDeviceName, O_RDWR | O_NOCTTY | O_NONBLOCK);
    unixCheck(jtagBox, "Failed to open %s", jtagDeviceName);

    // save current serial port settings and plan to restore them on exit
    jtagCheck(tcgetattr(jtagBox, &oldtio));
    oldtioValid = true;
    atexit(restoreSerialPort);

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

int timeout_read(int fd, void *buf, size_t count, unsigned long timeout)
{
    char *buffer = (char *)buf;
    size_t actual = 0;

    while (actual < count)
    {
	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(fd, &readfds);

	struct timeval tmout;
	tmout.tv_sec = timeout / 1000000;
	tmout.tv_usec = timeout % 1000000;

	int selected = select(fd + 1, &readfds, NULL, NULL, &tmout);
        /* Even though select() is not supposed to set errno to EAGAIN
           (according to the linux man page), it seems that errno can be set
           to EAGAIN on some cygwin systems. Thus, we need to catch that
           here. */
        if ((selected < 0) && (errno == EAGAIN || errno == EINTR))
            continue;
	jtagCheck(selected);

	if (selected == 0)
	    return actual;

	ssize_t thisread = read(fd, &buffer[actual], count - actual);
        if ((thisread < 0) && (errno == EAGAIN))
            continue;
	jtagCheck(thisread);

	actual += thisread;
    }

    return count;
}

static int safewrite(int fd, const void *b, int count)
{
  char *buffer = (char *)b;
  int actual = 0;
  int flags = fcntl(fd, F_GETFL);

  fcntl(fd, F_SETFL, 0); // blocking mode
  while (count > 0)
    {
      int n = write(fd, buffer, count);

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
  fcntl(fd, F_SETFL, flags); 
  return actual;
}

unsigned long decodeAddress(uchar *buf)
{
    return buf[0] << 16 | buf[1] << 8 | buf[2];
}

void encodeAddress(uchar *buffer, unsigned long x)
{
    buffer[0] = x >> 16;
    buffer[1] = x >> 8;
    buffer[2] = x;
}
    
/** Send a command to the jtag, and check result.

    Increase *tries, abort if reaches MAX_JTAG_COMM_ATTEMPS

    Reads first response byte. If no response is received within
    JTAG_RESPONSE_TIMEOUT, returns false. If response byte is 
    JTAG_R_RESP_OK returns true, otherwise returns false.
**/
static bool sendJtagCommand(uchar *command, int commandSize, int *tries)
{
    check((*tries)++ < MAX_JTAG_COMM_ATTEMPS,
	      "JTAG ICE: Cannot synchronise");

    debugOut("\ncommand[%c, %d]: ", command[0], *tries);

    for (int i = 0; i < commandSize; i++)
	debugOut("%.2X ", command[i]);

    debugOut("\n");

    // before writing, clean up any "unfinished business".
    jtagCheck(tcflush(jtagBox, TCIFLUSH));

    int count = safewrite(jtagBox, command, commandSize);
    if (count < 0)
      jtagCheck(count);
    else // this shouldn't happen
      check(count == commandSize, JTAG_CAUSE);

    // And wait for all characters to go to the JTAG box.... can't hurt!
    jtagCheck(tcdrain(jtagBox));

    // We should get RESP_OK. 
    uchar ok;
    count = timeout_read(jtagBox, &ok, 1, JTAG_RESPONSE_TIMEOUT);
    jtagCheck(count);

    // timed out
    if (count == 0)
    {
	debugOut("Timed out.\n");
	return false;
    }

    if (ok == JTAG_R_OK)
	return true;

    debugOut("Out of sync, reponse was `%02x'\n", ok);

    return false;
}

/** Get a 'responseSize' byte response from the JTAG ICE

    Returns NULL if no bytes received for JTAG_COMM_TIMEOUT microseconds
    Returns a dynamically allocated buffer containing the reponse (caller
    must free) otherwise
**/
static uchar *getJtagResponse(int responseSize)
{
    uchar *response;
    int numCharsRead;

    // Increase by 1 because of the zero termination.
    //
    // note: IT IS THE CALLERS RESPONSIBILITY TO delete() THIS.
    response = new uchar[responseSize + 1];
    response[responseSize] = '\0';

    numCharsRead = timeout_read(jtagBox, response, responseSize,
                                JTAG_RESPONSE_TIMEOUT);
    jtagCheck(numCharsRead);

    debugOut("response: ");
    for (int i = 0; i < numCharsRead; i++)
    {
	debugOut("%.2X ", response[i]);
    }
    debugOut("\n");

    if (numCharsRead < responseSize) // timeout problem
    {
	debugOut("Timed Out (partial response)\n");
	delete [] response;
	return NULL;
    }

    return response;
}

uchar *doJtagCommand(uchar *command, int  commandSize, int  responseSize)
{
    int tryCount = 0;

    // Send command until we get RESP_OK
    for (;;)
    {
	if (sendJtagCommand(command, commandSize, &tryCount))
	    break;

	// We're out of sync. Attempt to resync.
	uchar sync[] = { ' ' };
	while (!sendJtagCommand(sync, 1, &tryCount)) 
	    ;
    }

    uchar *response = getJtagResponse(responseSize);
    check(response != NULL, JTAG_CAUSE);

    return response;
}

bool doSimpleJtagCommand(unsigned char cmd, int responseSize)
{
    uchar *response;
    uchar command[] = { cmd, JTAG_EOM };
    bool result;

    response = doJtagCommand(command, sizeof command, responseSize);
    result = responseSize == 0 || response[responseSize - 1] == JTAG_R_OK;
    
    delete [] response;

    return result;
}


/** Change bitrate of PC's serial port as specified by BIT_RATE_xxx in
    'newBitRate' **/
static void changeLocalBitRate(uchar newBitRate)
{
    // Change the local port bitrate.
    struct termios tio;

    jtagCheck(tcgetattr(jtagBox, &tio)); 

    speed_t newPortSpeed = B19200;
    // Linux doesn't support 14400. Let's hope it doesn't end up there...
    switch(newBitRate)
    {
    case BIT_RATE_9600:
	newPortSpeed = B9600;
	break;
    case BIT_RATE_19200:
	newPortSpeed = B19200;
	break;
    case BIT_RATE_38400:
	newPortSpeed = B38400;
	break;
    case BIT_RATE_57600:
	newPortSpeed = B57600;
	break;
    case BIT_RATE_115200:
	newPortSpeed = B115200;
	break;
    default:
	debugOut("unsupported bitrate\n");
	exit(1);
    }

    cfsetospeed(&tio, newPortSpeed);
    cfsetispeed(&tio, newPortSpeed);

    jtagCheck(tcsetattr(jtagBox,TCSANOW,&tio));
    jtagCheck(tcflush(jtagBox, TCIFLUSH));
}

/** Set PC and JTAG ICE bitrate to BIT_RATE_xxx specified by 'newBitRate' **/
static void changeBitRate(uchar newBitRate)
{
    setJtagParameter(JTAG_P_BITRATE, newBitRate);
    changeLocalBitRate(newBitRate);
}

/** Set the JTAG ICE device descriptor data for specified device type **/
static void setDeviceDescriptor(jtag_device_def_type *dev)
{
    uchar *response = NULL;
    uchar *command = (uchar *)(&dev->dev_desc);

    response = doJtagCommand(command, sizeof dev->dev_desc, 1);
    check(response[0] == JTAG_R_OK,
	      "JTAG ICE: Failed to set device description");

    delete [] response;
}

/** Check for emulator using the 'S' command *without* retries
    (used at startup to check sync only) **/
static bool checkForEmulator(void)
{
    uchar *response;
    uchar command[] = { 'S', JTAG_EOM };
    bool result;
    int tries = 0;

    if (!sendJtagCommand(command, sizeof command, &tries))
      return false;

    response = getJtagResponse(8);
    result = response && !strcmp((char *)response, "AVRNOCDA");
    
    delete [] response;

    return result;
}

/** Attempt to synchronise with JTAG at specified bitrate **/
static bool synchroniseAt(uchar bitrate)
{
    debugOut("Attempting synchronisation at bitrate %02x\n", bitrate);

    changeLocalBitRate(bitrate);

    int tries = 0;
    while (tries < MAX_JTAG_SYNC_ATTEMPS)
    {
	// 'S  ' works if this is the first string sent after power up.
	// But if another char is sent, the JTAG seems to go in to some 
	// internal mode. 'SE  ' takes it out of there, apparently (sometimes
	// 'E  ' is enough, but not always...)
	sendJtagCommand((uchar *)"SE  ", 4, &tries);
	usleep(2 * JTAG_COMM_TIMEOUT); // let rest of response come before we ignore it
	jtagCheck(tcflush(jtagBox, TCIFLUSH));
	if (checkForEmulator())
	    return true;
    }
    return false;
}

/** Attempt to synchronise with JTAG ICE at all possible bit rates **/
static void startJtagLink(void)
{
    static uchar bitrates[] =
    { BIT_RATE_115200, BIT_RATE_19200, BIT_RATE_57600, BIT_RATE_38400,
      BIT_RATE_9600 };

    for (unsigned int i = 0; i < sizeof bitrates / sizeof *bitrates; i++)
	if (synchroniseAt(bitrates[i]))
	    return;

    check(false, "Failed to synchronise with the JTAG ICE (is it connected and powered?)");
}

/** Device automatic configuration 
 Determines the device being controlled by the JTAG ICE and configures
 the system accordingly.

 May be overridden by command line parameter.

*/
static void deviceAutoConfig(void)
{
    unsigned int device_id;
    int i;
    jtag_device_def_type *pDevice = deviceDefinitions;

    // Auto config
    debugOut("Automatic device detection: ");
   
    /* Read in the JTAG device ID to determine device */
    device_id = getJtagParameter(JTAG_P_JTAGID_BYTE0);
    /* Reading the first byte resumes the program (weird, no?) */
    interruptProgram();
    device_id += (getJtagParameter(JTAG_P_JTAGID_BYTE1) <<  8) +
      (getJtagParameter(JTAG_P_JTAGID_BYTE2) << 16) + 
      (getJtagParameter(JTAG_P_JTAGID_BYTE3) << 24);

   
    debugOut("JTAG id = 0x%0X : Ver = 0x%0x : Device = 0x%0x : Manuf = 0x%0x\n", 
             device_id,
             (device_id & 0xF0000000) >> 28,
             (device_id & 0x0FFFF000) >> 12,
             (device_id & 0x00000FFE) >> 1);
   
    device_id = (device_id & 0x0FFFF000) >> 12;
    statusOut("Reported JTAG device ID: 0x%0X\n", device_id);
    
    if (device_name == 0)
    {
        while (pDevice->name)
        {
            if (pDevice->device_id == device_id)
                break;

            pDevice++;
        }
        check(pDevice->name,
              "No configuration available for device ID: %0x\n",
              device_id); 
    }
    else
    {
        debugOut("Looking for device: %s\n", device_name);

        while (pDevice->name)
        {
            if (strcasecmp(pDevice->name, device_name) == 0)
                break;

            pDevice++;
        }
        check(pDevice->name,
              "No configuration available for Device: %s\n",
              device_name);
    }

    if (device_name)
    {
        if (device_id != pDevice->device_id)
        {
            statusOut("Configured for device ID: 0x%0X %s -- FORCED with %s\n",
                      pDevice->device_id, pDevice->name, device_name);
        }
        else
        {
            statusOut("Configured for device ID: 0x%0X %s -- Matched with "
                      "%s\n", pDevice->device_id, pDevice->name, device_name);
        }
    }
    else
    {
        statusOut("Configured for device ID: 0x%0X %s\n",
                  pDevice->device_id, pDevice->name);
    }

    device_name = (char*)pDevice->name;

    global_p_device_def = pDevice;

    setDeviceDescriptor(pDevice);
}


void initJtagBox(void)
{
    statusOut("JTAG config starting.\n");

    startJtagLink();
    changeBitRate(BIT_RATE_115200);

    uchar hw_ver = getJtagParameter(JTAG_P_HW_VERSION);
    statusOut("Hardware Version: 0x%02x\n", hw_ver);
    //check(hw_ver == 0xc0, "JTAG ICE: Unknown hardware version");

    uchar sw_ver = getJtagParameter(JTAG_P_SW_VERSION);
    statusOut("Software Version: 0x%02x\n", sw_ver);

    interruptProgram();

    deviceAutoConfig();

    // Clear out the breakpoints.
    deleteAllBreakpoints();

    statusOut("JTAG config complete.\n");
}


void initJtagOnChipDebugging(uchar bitrate)
{
    statusOut("Preparing the target device for On Chip Debugging.\n");

    // Set JTAG bitrate
    setJtagParameter(JTAG_P_CLOCK, bitrate);

    // When attaching we can't change fuse bits, etc, as 
    // enabling+disabling programming resets the processor
    enableProgramming();

    // Ensure that all lock bits are "unlocked" ie all 1's
    uchar *lockBits = 0;
    lockBits = jtagRead(LOCK_SPACE_ADDR_OFFSET + 0, 1);

    if (*lockBits != LOCK_BITS_ALL_UNLOCKED)
    {
        lockBits[0] = LOCK_BITS_ALL_UNLOCKED;
        jtagWrite(LOCK_SPACE_ADDR_OFFSET + 0, 1, lockBits);
    }

    statusOut("\nDisabling lock bits:\n");
    statusOut("  LockBits -> 0x%02x\n", *lockBits);

    if (lockBits)
    {
        delete [] lockBits;
        lockBits = 0;
    }

    // Ensure on-chip debug enable fuse is enabled ie '0'
    uchar *fuseBits = 0;
    statusOut("\nEnabling on-chip debugging:\n");
    fuseBits = jtagRead(FUSE_SPACE_ADDR_OFFSET + 0, 3);

    if ((fuseBits[1] & FUSE_OCDEN) == FUSE_OCDEN)
    {
        fuseBits[1] = fuseBits[1] & ~FUSE_OCDEN; // clear bit
        jtagWrite(FUSE_SPACE_ADDR_OFFSET + 1, 1, &fuseBits[1]);
    }

    jtagDisplayFuses(fuseBits);

    if (fuseBits)
    {
        delete [] fuseBits;
        fuseBits = 0;
    }

    disableProgramming();

    resetProgram();
    setJtagParameter(JTAG_P_TIMERS_RUNNING, 0x00);
    resetProgram();
}
