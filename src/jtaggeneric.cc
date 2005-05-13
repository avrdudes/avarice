/*
 *	avarice - The "avarice" program.
 *	Copyright (C) 2001 Scott Finneran
 *      Copyright (C) 2002, 2003, 2004 Intel Corporation
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
#include "jtag1.h"

/*
 * Generic functions applicable to both, the mkI and mkII ICE.
 */

void jtag::jtagCheck(int status)
{
    unixCheck(status, JTAG_CAUSE, NULL);
}

void jtag::restoreSerialPort()
{
  if (jtagBox >= 0 && oldtioValid)
      tcsetattr(jtagBox, TCSANOW, &oldtio);
}

jtag::jtag(void)
{
  jtagBox = 0;
  oldtioValid = false;
}

jtag::jtag(const char *jtagDeviceName)
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
	jtagCheck(selected);

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
