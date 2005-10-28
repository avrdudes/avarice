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
 * This file implements the libusb-based USB connection to a JTAG ICE
 * mkII.
 *
 * $Id$
 */


#include "avarice.h"

#ifdef HAVE_LIBUSB

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <termios.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

#include <usb.h>

#include "jtag.h"

#define USB_VENDOR_ATMEL 1003
#define USB_DEVICE_JTAGICEMKII 0x2103
/*
 * Should we query the endpoint number and max transfer size from USB?
 * After all, the JTAG ICE mkII docs document these values.
 */
#define JTAGICE_BULK_EP_WRITE 0x02
#define JTAGICE_BULK_EP_READ  0x82
#define JTAGICE_MAX_XFER 64

static int signalled, exiting;
static pid_t usb_kid;

/*
 * Various signal handlers for the USB daemon child.
 */
static void sigtermhandler(int signo)
{
  // give the pipes some time to flush before exiting
  exiting++;
  alarm(3);
}

static void alarmhandler(int signo)
{
  signalled++;
}

static void dummyhandler(int signo)
{
  /* nothing to do, just abort the current read()/select() */
}

/*
 * atexit() handler
 */
static void kill_daemon(void)
{
  kill(usb_kid, SIGTERM);
}

/*
 * Signal handler for the parent (i.e. for AVaRICE).
 */
static void inthandler(int signo)
{
  int status;

  kill(usb_kid, SIGTERM);
  (void)wait(&status);
  signal(signo, SIG_DFL);
  kill(getpid(), signo);
}

static void childhandler(int signo)
{
  int status;
  char b[500];

  (void)wait(&status);

#define PRINTERR(msg) write(fileno(stderr), msg, strlen(msg))
  PRINTERR("USB daemon died\n");
  _exit(1);
}

/*
 * The USB daemon itself.  Polls the USB device for data as long as
 * there is room in the AVaRICE pipe.  Polls the AVaRICE descriptor
 * for data, and sends them to the USB device.
 */
static void usb_daemon(usb_dev_handle *udev, int fd, int usb_interface)
{
  int ioflags;

  signal(SIGALRM, alarmhandler);
  signal(SIGTERM, sigtermhandler);
  signal(SIGINT, sigtermhandler);
  if (fcntl(fd, F_GETFL, &ioflags) != -1)
    {
      ioflags |= O_ASYNC;
      if (fcntl(fd, F_SETFL, &ioflags) != -1)
	signal(SIGIO, dummyhandler);
    }

  for (; !signalled;)
    {
      fd_set r, w;
      struct timeval tv;

      /*
       * See if our parent has something to tell us, or room in the
       * pipe to send something to.
       */
      FD_ZERO(&r);
      FD_ZERO(&w);
      FD_SET(fd, &r);
      FD_SET(fd, &w);
      tv.tv_sec = 0;
      tv.tv_usec = 0;
      if (select(fd + 1, &r, &w, NULL, &tv) > 0)
	{
	  if (FD_ISSET(fd, &r))
	    {
	      char buf[JTAGICE_MAX_XFER];
	      ssize_t rv;

	      if ((rv = read(fd, buf, JTAGICE_MAX_XFER)) > 0)
		{
		  if (usb_bulk_write(udev, JTAGICE_BULK_EP_WRITE, buf,
				     rv, 5000) !=
		      rv)
		    {
		      fprintf(stderr, "USB bulk write error: %s\n",
			      usb_strerror());
		      exit(1);
		    }
		  continue;
		}
	      if (rv < 0 && errno != EINTR && errno != EAGAIN)
		{
		  fprintf(stderr, "read error from AVaRICE: %s\n",
			  strerror(errno));
		  exit(1);
		}
	    }
	  if (FD_ISSET(fd, &w))
	    {
	      char buf[JTAGICE_MAX_XFER];
	      int rv;
	      rv = usb_bulk_read(udev, JTAGICE_BULK_EP_READ, buf,
				 JTAGICE_MAX_XFER, 100);
	      if (rv == 0 || rv == -EINTR || rv == -EAGAIN || rv == -ETIMEDOUT)
		continue;
	      if (rv < 0)
		{
		  if (!exiting)
		    fprintf(stderr, "USB bulk read error: %s\n",
			    usb_strerror());
		  exit(1);
		}
	      if (write(fd, buf, rv) != rv)
		{
		  fprintf(stderr, "short write to AVaRICE: %s\n",
			  strerror(errno));
		  exit(1);
		}
	    }
	}
    }
  (void)usb_release_interface(udev, usb_interface);
  usb_close(udev);
  exit(0);
}

pid_t jtag::openUSB(const char *jtagDeviceName)
{
  char string[256];
  struct usb_bus *bus;
  struct usb_device *dev;
  usb_dev_handle *udev;
  char *serno, *cp2;
  int usb_interface;
  size_t x;

  /*
   * The syntax for usb devices is defined as:
   *
   * -P usb[:serialnumber]
   *
   * See if we've got a serial number passed here.  The serial number
   * might contain colons which we remove below, and we compare it
   * right-to-left, so only the least significant nibbles need to be
   * specified.
   */
  if ((serno = strchr(jtagDeviceName, ':')) != NULL)
    {
      /* first, drop all colons there if any */
      cp2 = ++serno;

      while ((cp2 = strchr(cp2, ':')) != NULL)
	{
	  x = strlen(cp2) - 1;
	  memmove(cp2, cp2 + 1, x);
	  cp2[x] = '\0';
	}

      unixCheck(strlen(serno) <= 12, "invalid serial number \"%s\"", serno);
    }

  usb_init();

  usb_find_busses();
  usb_find_devices();

  udev = NULL;
  bool found = false;
  for (bus = usb_get_busses(); !found && bus; bus = bus->next)
    {
      for (dev = bus->devices; !found && dev; dev = dev->next)
	{
	  udev = usb_open(dev);
	  if (udev)
	    {
	      if (dev->descriptor.idVendor == USB_VENDOR_ATMEL &&
		  dev->descriptor.idProduct == USB_DEVICE_JTAGICEMKII)
		{
		  /* yeah, we found something */
		  int rv = usb_get_string_simple(udev,
						 dev->descriptor.iSerialNumber,
						 string, sizeof(string));
		  unixCheck(rv >= 0, "cannot read serial number \"%s\"",
			    usb_strerror());

		  debugOut("Found JTAG ICE, serno: %s\n", string);
		  if (serno != NULL)
		    {
		      /*
		       * See if the serial number requested by the
		       * user matches what we found, matching
		       * right-to-left.
		       */
		      x = strlen(string) - strlen(serno);
		      if (strcasecmp(string + x, serno) != 0)
			{
			  debugOut("serial number doesn't match\n");
			  usb_close(udev);
			  continue;
			}
		    }

		  // we found what we want
		  found = true;
		  break;
		}
	      usb_close(udev);
	    }
	}
    }
  unixCheck(found? 0: -1,
	    "did not find any%s USB device \"%s\"",
	    serno? " (matching)": "", jtagDeviceName);

  if (dev->config == NULL)
  {
      statusOut("USB device has no configuration\n");
    fail:
      usb_close(udev);
      exit(EXIT_FAILURE);
  }
  if (usb_set_configuration(udev, dev->config[0].bConfigurationValue))
  {
      statusOut("error setting configuration %d: %s\n",
                dev->config[0].bConfigurationValue,
                usb_strerror());
      goto fail;
  }
  usb_interface = dev->config[0].interface[0].altsetting[0].bInterfaceNumber;
  if (usb_claim_interface(udev, usb_interface))
  {
      statusOut("error claiming interface %d: %s\n",
                usb_interface, usb_strerror());
      goto fail;
  }

  pid_t p;
  int pype[2];
  unixCheck(socketpair(AF_UNIX, SOCK_STREAM, PF_UNSPEC, pype) == 0,
	    "cannot create pipe");

  switch ((p = fork()))
    {
    case 0:
      close(pype[0]);
      usb_daemon(udev, pype[1], usb_interface);
      break;
    case -1:
      unixCheck(0, "Cannot fork");
      break;
    default:
      close(pype[1]);
      jtagBox = pype[0];
      usb_kid = p;
    }
  atexit(kill_daemon);
  signal(SIGTERM, inthandler);
  signal(SIGINT, inthandler);
  signal(SIGQUIT, inthandler);
  signal(SIGCHLD, childhandler);
}

#endif /* HAVE_LIBUSB */
