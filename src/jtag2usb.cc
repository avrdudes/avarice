/*
 *	avarice - The "avarice" program.
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
 * This file implements the libusb-based USB connection to a JTAG ICE
 * mkII.
 *
 * $Id$
 */


#include "avarice.h"

#ifdef HAVE_LIBUSB

#include <stdarg.h>
#include <stdint.h>
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
#include <pthread.h>

#include "jtag.h"

#define USB_VENDOR_ATMEL 1003
#define USB_DEVICE_JTAGICEMKII 0x2103
#define USB_DEVICE_AVRDRAGON   0x2107
/*
 * Should we query the endpoint number and max transfer size from USB?
 * After all, the JTAG ICE mkII docs document these values.
 */
#define JTAGICE_BULK_EP_WRITE 0x02
#define JTAGICE_BULK_EP_READ  0x82
#define JTAGICE_MAX_XFER 64

#define MAX_MESSAGE      512

static usb_dev_handle *udev = NULL;
static int pype[2];
static pthread_t rtid, wtid;
static int usb_interface;

/*
 * Walk down all USB devices, and see whether we can find our emulator
 * device.
 */
static usb_dev_handle *opendev(const char *jtagDeviceName, emulator emu_type,
			       int &usb_interface)
{
  char string[256];
  struct usb_bus *bus;
  struct usb_device *dev;
  usb_dev_handle *udev;
  char *devnamecopy, *serno, *cp2;
  uint16_t pid;
  size_t x;

  switch (emu_type)
    {
    case EMULATOR_JTAGICE:
      pid = USB_DEVICE_JTAGICEMKII;
      break;

    case EMULATOR_DRAGON:
      pid = USB_DEVICE_AVRDRAGON;
      break;

    default:
      // should not happen
      return NULL;
    }

  devnamecopy = new char[x = strlen(jtagDeviceName) + 1];
  memcpy(devnamecopy, jtagDeviceName, x);

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
  if ((serno = strchr(devnamecopy, ':')) != NULL)
    {
      /* first, drop all colons there if any */
      cp2 = ++serno;

      while ((cp2 = strchr(cp2, ':')) != NULL)
	{
	  x = strlen(cp2) - 1;
	  memmove(cp2, cp2 + 1, x);
	  cp2[x] = '\0';
	}

      if (strlen(serno) > 12)
      {
	  fprintf(stderr, "invalid serial number \"%s\"", serno);
	  return NULL;
      }
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
		  dev->descriptor.idProduct == pid)
		{
		  /* yeah, we found something */
		  int rv = usb_get_string_simple(udev,
						 dev->descriptor.iSerialNumber,
						 string, sizeof(string));
		  if (rv < 0)
		  {
		      fprintf(stderr, "cannot read serial number \"%s\"",
			      usb_strerror());
		      return NULL;
		  }

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

  delete devnamecopy;
  if (!found)
  {
    printf("did not find any%s USB device \"%s\"\n",
	   serno? " (matching)": "", jtagDeviceName);
    return NULL;
  }
  if (dev->config == NULL)
  {
      statusOut("USB device has no configuration\n");
    fail:
      usb_close(udev);
      return NULL;
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

  return udev;
}

/* USB writer thread */
static void *usb_thread_write(void * data)
{
  while (1)
    {
      char buf[MAX_MESSAGE];
      int rv;

      if ((rv = read(pype[0], buf, MAX_MESSAGE)) > 0)
        {
	  int offset = 0;

	  while (rv != 0)
	  {
	    int amnt, result;

	    if (rv > JTAGICE_MAX_XFER)
	      amnt = JTAGICE_MAX_XFER;
	    else
	      amnt = rv;
	    result = usb_bulk_write(udev, JTAGICE_BULK_EP_WRITE,
				    buf + offset, amnt, 5000);
	    if (result != amnt)
	    {
	      fprintf(stderr, "USB bulk write error: %s\n",
		      usb_strerror());
	      pthread_exit((void *)1);
	    }
	    if (rv == JTAGICE_MAX_XFER)
	    {
	      /* send ZLP */
	      usb_bulk_write(udev, JTAGICE_BULK_EP_WRITE, buf,
			     0, 5000);
	    }
	    rv -= amnt;
	    offset += amnt;
	  }
          continue;
        }
      else if (errno != EINTR && errno != EAGAIN)
        {
          fprintf(stderr, "read error from AVaRICE: %s\n",
                  strerror(errno));
          pthread_exit((void *)1);
        }
    }
}

/* USB reader thread */
static void *usb_thread_read(void *data)
{
  while (1)
    {
      char buf[MAX_MESSAGE];
      int rv;

      rv = usb_bulk_read(udev, JTAGICE_BULK_EP_READ, buf,
			 JTAGICE_MAX_XFER, 0);
      if (rv == 0 || rv == -EINTR || rv == -EAGAIN || rv == -ETIMEDOUT)
      {
	/* OK, try again */
      }
      else if (rv < 0)
      {
	fprintf(stderr, "USB bulk read error: %s\n",
		usb_strerror());
	pthread_exit((void *)1);
      }
      else
      {
	/*
	 * We read a packet from USB.  If it's been a partial
	 * one (result matches the endpoint size), see to get
	 * more, until we have either a short read, or a ZLP.
	 */
	unsigned int pkt_len = rv;
	bool needmore = rv == JTAGICE_MAX_XFER;

	/* OK, if there is more to read, do so. */
	while (needmore)
	{
	  int maxlen = MAX_MESSAGE - pkt_len;
	  if (maxlen > JTAGICE_MAX_XFER)
	    maxlen = JTAGICE_MAX_XFER;
	  rv = usb_bulk_read(udev, JTAGICE_BULK_EP_READ, buf + pkt_len,
			     maxlen, 100);

	  if (rv == -EINTR || rv == -EAGAIN || rv == -ETIMEDOUT)
	  {
	    continue;
	  }
	  if (rv == 0)
	  {
	    /* Zero-length packet: we are done. */
	    break;
	  }
	  if (rv < 0)
	  {
	    fprintf(stderr,
		    "USB bulk read error in continuation block: %s\n",
		      usb_strerror());
	    pthread_exit((void *)1);
	  }

	  needmore = rv == JTAGICE_MAX_XFER;
	  pkt_len += rv;
	  if (pkt_len == MAX_MESSAGE)
	  {
	    /* should not happen */
	    fprintf(stderr, "Message too big in USB receive.\n");
	    break;
	  }
	}

	if (write(pype[0], buf, pkt_len) != pkt_len)
	{
	  fprintf(stderr, "short write to AVaRICE: %s\n",
		  strerror(errno));
	  pthread_exit((void *)1);
	}
      }
    }
}

void jtag::resetUSB(void)
{
  if (udev)
    {
      usb_resetep(udev, JTAGICE_BULK_EP_READ);
      usb_resetep(udev, JTAGICE_BULK_EP_WRITE);
    }
}

static void
cleanup_usb(void)
{
  usb_release_interface(udev, usb_interface);
  usb_close(udev);
}


void jtag::openUSB(const char *jtagDeviceName)
{
  if (socketpair(AF_UNIX, SOCK_STREAM, PF_UNSPEC, pype) < 0)
      throw jtag_exception("cannot create pipe");

  udev = opendev(jtagDeviceName, emu_type, usb_interface);
  if (udev == NULL)
    throw jtag_exception("cannot open USB device");

  pthread_create(&rtid, NULL, usb_thread_read, NULL);
  pthread_create(&wtid, NULL, usb_thread_write, NULL);

  atexit(cleanup_usb);

  jtagBox = pype[1];
}

#endif /* HAVE_LIBUSB */
