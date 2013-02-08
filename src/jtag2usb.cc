/*
 *	avarice - The "avarice" program.
 *	Copyright (C) 2005, 2007, 2012 Joerg Wunsch
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
 * mkII.  It is also used by the JTAGICE3.
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

#ifdef HAVE_LIBUSB_2_0
#  include <libusb20.h>
#  include <libusb20_desc.h>
#  include <poll.h>
#else
#  include <usb.h>
#endif

#include <pthread.h>

#ifdef __FreeBSD__
#  include <pthread_np.h>
#endif

#include "jtag.h"

#define USB_VENDOR_ATMEL 1003
#define USB_DEVICE_JTAGICEMKII 0x2103
#define USB_DEVICE_AVRDRAGON   0x2107
#define USB_DEVICE_JTAGICE3    0x2110

/* JTAGICEmkII */
#define USBDEV_BULK_EP_WRITE_MKII 0x02
#define USBDEV_BULK_EP_READ_MKII  0x82
#define USBDEV_MAX_XFER_MKII 64

/* JTAGICE3 */
#define USBDEV_BULK_EP_WRITE_3    0x01
#define USBDEV_BULK_EP_READ_3     0x82
#define USBDEV_EVT_EP_READ_3      0x83
#define USBDEV_MAX_XFER_3    512
#define USBDEV_MAX_EVT_3     64

#define MAX_MESSAGE      512

static int read_ep, write_ep, event_ep, max_xfer;
#ifdef HAVE_LIBUSB_2_0
typedef struct libusb20_device usb_dev_t;
#else
typedef usb_dev_handle usb_dev_t;
#endif

static usb_dev_t *udev = NULL;
static int pype[2];

#ifdef HAVE_LIBUSB_2_0
static pthread_t utid;
static struct libusb20_backend *be;
static struct libusb20_transfer *xfr_out;
static struct libusb20_transfer *xfr_in;
static struct libusb20_transfer *xfr_evt;
#else
static pthread_t rtid, wtid, etid;
#endif
static int usb_interface;

#ifdef HAVE_LIBUSB_2_0
/* convert LIBUSB20_ERROR into textual description */
static const char *
usb_error(int r)
{
  const char *msg = "UNKNOWN";

  switch ((enum libusb20_error)r)
    {
    case LIBUSB20_SUCCESS:
      msg = "success";
      break;

    case LIBUSB20_ERROR_IO:
      msg = "IO error";
      break;

    case LIBUSB20_ERROR_INVALID_PARAM:
      msg = "Invalid parameter";
      break;

    case LIBUSB20_ERROR_ACCESS:
      msg = "Access denied";
      break;

    case LIBUSB20_ERROR_NO_DEVICE:
      msg = "No such device";
      break;

    case LIBUSB20_ERROR_NOT_FOUND:
      msg = "Entity not found";
      break;

    case LIBUSB20_ERROR_BUSY:
      msg = "Resource busy";
      break;

    case LIBUSB20_ERROR_TIMEOUT:
      msg = "Operation timed out";
      break;

    case LIBUSB20_ERROR_OVERFLOW:
      msg = "Overflow";
      break;

    case LIBUSB20_ERROR_PIPE:
      msg = "Pipe error";
      break;

    case LIBUSB20_ERROR_INTERRUPTED:
      msg = "System call interrupted";
      break;

    case LIBUSB20_ERROR_NO_MEM:
      msg = "Insufficient memory";
      break;

    case LIBUSB20_ERROR_NOT_SUPPORTED:
      msg = "Operation not supported";
      break;

    case LIBUSB20_ERROR_OTHER:
      msg = "Other error";
      break;
    }

  return msg;
}

/* same, for LIBUSB20_TRANSFER_XXX */
static const char *
usb_transfer_msg(uint8_t r)
{
  const char *msg = "UNKNOWN";

  switch ((enum libusb20_transfer_status)r)
    {
    case LIBUSB20_TRANSFER_COMPLETED:
      msg = "completed (no error)";
      break;

    case LIBUSB20_TRANSFER_START:
      msg = "start transfer";
      break;

    case LIBUSB20_TRANSFER_DRAINED:
      msg = "transfer drained";
      break;

    case LIBUSB20_TRANSFER_ERROR:
      msg = "transfer failed";
      break;

    case LIBUSB20_TRANSFER_TIMED_OUT:
      msg = "transfer timed out";
      break;

    case LIBUSB20_TRANSFER_CANCELLED:
      msg = "transfer cancelled";
      break;

    case LIBUSB20_TRANSFER_STALL:
      msg = "endpoint stalled, or control request not supported";
      break;

    case LIBUSB20_TRANSFER_NO_DEVICE:
      msg = "device disconnected";
      break;

    case LIBUSB20_TRANSFER_OVERFLOW:
      msg = "transfer overflow";
      break;
    }

  return msg;
}

static void usb20_cleanup(usb_dev_t *d)
{
  libusb20_dev_close(d);
  libusb20_be_free(be);
}
#endif


/*
 * Walk down all USB devices, and see whether we can find our emulator
 * device.
 */
static usb_dev_t *opendev(const char *jtagDeviceName, emulator emu_type,
			  int &usb_interface)
{
  char string[256];
#ifndef HAVE_LIBUSB_2_0
  struct usb_bus *bus;
  struct usb_device *dev;
#endif
  usb_dev_t *pdev;
  char *devnamecopy, *serno, *cp2;
  uint16_t pid;
  size_t x;

  switch (emu_type)
    {
    case EMULATOR_JTAGICE:
      pid = USB_DEVICE_JTAGICEMKII;
      read_ep = USBDEV_BULK_EP_READ_MKII;
      write_ep = USBDEV_BULK_EP_WRITE_MKII;
      event_ep = 0;
      max_xfer = USBDEV_MAX_XFER_MKII;
      break;

    case EMULATOR_DRAGON:
      pid = USB_DEVICE_AVRDRAGON;
      read_ep = USBDEV_BULK_EP_READ_MKII;
      write_ep = USBDEV_BULK_EP_WRITE_MKII;
      event_ep = 0;
      max_xfer = USBDEV_MAX_XFER_MKII;
      break;

    case EMULATOR_JTAGICE3:
      pid = USB_DEVICE_JTAGICE3;
      read_ep = USBDEV_BULK_EP_READ_3;
      write_ep = USBDEV_BULK_EP_WRITE_3;
      event_ep = USBDEV_EVT_EP_READ_3;
      max_xfer = USBDEV_MAX_XFER_3;
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
	  delete [] devnamecopy;
	  return NULL;
      }
    }

#ifdef HAVE_LIBUSB_2_0
  if ((be = libusb20_be_alloc_default()) == NULL)
    {
      perror("libusb20_be_alloc()");
      return NULL;
    }
#else
  usb_init();

  usb_find_busses();
  usb_find_devices();
#endif

  pdev = NULL;
  bool found = false;
#ifdef HAVE_LIBUSB_2_0
  while ((pdev = libusb20_be_device_foreach(be, pdev)) != NULL)
    {
      struct LIBUSB20_DEVICE_DESC_DECODED *ddp =
      libusb20_dev_get_device_desc(pdev);

      if (ddp->idVendor == USB_VENDOR_ATMEL &&
	  ddp->idProduct == pid)
	{
	  int rv = libusb20_dev_open(pdev, 3);
	  if (rv < 0)
	    {
	      fprintf(stderr, "cannot open device \"%s\"",
		      usb_error(rv));
	      libusb20_be_free(be);
	      return NULL;
	    }

	  /* yeah, we found something */
	  rv = libusb20_dev_req_string_simple_sync(pdev,
						   ddp->iSerialNumber,
						   string, sizeof(string));
	  if (rv < 0)
	    {
	      fprintf(stderr, "cannot read serial number \"%s\"",
		      usb_error(rv));
	      usb20_cleanup(pdev);
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
		  libusb20_dev_close(pdev);
		  continue;
		}
	    }

	  // we found what we want
	  found = true;
	  break;
	}
    }
#else // !HAVE_LIBUSB_2_0
  for (bus = usb_get_busses(); !found && bus; bus = bus->next)
    {
      for (dev = bus->devices; !found && dev; dev = dev->next)
	{
	  pdev = usb_open(dev);
	  if (pdev)
	    {
	      if (dev->descriptor.idVendor == USB_VENDOR_ATMEL &&
		  dev->descriptor.idProduct == pid)
		{
		  /* yeah, we found something */
		  int rv = usb_get_string_simple(pdev,
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
			  usb_close(pdev);
			  continue;
			}
		    }

		  // we found what we want
		  found = true;
		  break;
		}
	      usb_close(pdev);
	    }
	}
    }
#endif // HAVE_LIBUSB_2_0

  delete [] devnamecopy;
  if (!found)
  {
    printf("did not find any%s USB device \"%s\"\n",
	   serno? " (matching)": "", jtagDeviceName);
    return NULL;
  }

#ifdef HAVE_LIBUSB_2_0
  int rv;

  if ((rv = libusb20_dev_set_config_index(pdev, 0)) != 0)
    {
      fprintf(stderr, "libusb20_dev_set_config_index: %s\n", usb_error(rv));
      usb20_cleanup(pdev);
      return NULL;
    }
  /*
   * Two transfers have been requested in libusb20_dev_open() above;
   * obtain the corresponding transfer struct pointers.
   */
  xfr_out = libusb20_tr_get_pointer(pdev, 0);
  xfr_in = libusb20_tr_get_pointer(pdev, 1);
  if (event_ep != 0)
    xfr_evt = libusb20_tr_get_pointer(pdev, 2);

  if (xfr_in == NULL || xfr_out == NULL)
    {
      fprintf(stderr, "libusb20_tr_get_pointer: %s\n", usb_error(rv));
      usb20_cleanup(pdev);
      return NULL;
    }

  /*
   * Open all transfers, the "out" one for the write endpoint, the
   * "in" one for the read endpoint (ep | 0x80), and the event EP
   * for the JTAGICE3 events.
   */
  if ((rv = libusb20_tr_open(xfr_out, 0, 1, write_ep)) != 0)
    {
      fprintf(stderr, "libusb20_tr_open: %s\n", usb_error(rv));
      usb20_cleanup(pdev);
      return NULL;
    }
  uint32_t max_packet_l;
  if ((max_packet_l = libusb20_tr_get_max_packet_length(xfr_out)) < max_xfer)
    {
      statusOut("downgrading max_xfer from %d to %d due to EP 0x%02x's wMaxPacketSize\n",
		max_xfer, max_packet_l, write_ep);
      max_xfer = max_packet_l;
    }
  if ((rv = libusb20_tr_open(xfr_in, 0, 1, read_ep)) != 0)
    {
      fprintf(stderr, "libusb20_tr_open: %s\n", usb_error(rv));
      usb20_cleanup(pdev);
      return NULL;
    }
  if ((max_packet_l = libusb20_tr_get_max_packet_length(xfr_in)) < max_xfer)
    {
      statusOut("downgrading max_xfer from %d to %d due to EP 0x%02x's wMaxPacketSize\n",
		max_xfer, max_packet_l, read_ep);
      max_xfer = max_packet_l;
    }
  if (event_ep != 0 &&
      (rv = libusb20_tr_open(xfr_evt, 0, 1, event_ep)) != 0)
    {
      fprintf(stderr, "libusb20_tr_open: %s\n", usb_error(rv));
      usb20_cleanup(pdev);
      return NULL;
    }
#else
  if (dev->config == NULL)
  {
      statusOut("USB device has no configuration\n");
    fail:
      usb_close(pdev);
      return NULL;
  }
  if (usb_set_configuration(pdev, dev->config[0].bConfigurationValue))
  {
      statusOut("error setting configuration %d: %s\n",
                dev->config[0].bConfigurationValue,
                usb_strerror());
      goto fail;
  }
  usb_interface = dev->config[0].interface[0].altsetting[0].bInterfaceNumber;
  if (usb_claim_interface(pdev, usb_interface))
  {
      statusOut("error claiming interface %d: %s\n",
                usb_interface, usb_strerror());
      goto fail;
  }
  struct usb_endpoint_descriptor *epp = dev->config[0].interface[0].altsetting[0].endpoint;
  for (int i = 0; i < dev->config[0].interface[0].altsetting[0].bNumEndpoints; i++)
  {
    if ((epp[i].bEndpointAddress == read_ep || epp[i].bEndpointAddress == write_ep) &&
	epp[i].wMaxPacketSize < max_xfer)
    {
      statusOut("downgrading max_xfer from %d to %d due to EP 0x%02x's wMaxPacketSize\n",
		max_xfer, epp[i].wMaxPacketSize, epp[i].bEndpointAddress);
      max_xfer = epp[i].wMaxPacketSize;
    }
  }
#endif

  /*
   * As of, at least, firmware 2.12, the JTAGICE3 does not handle
   * splitting large packets into smaller chunks correctly when being
   * operated on an USB 1.1 connection where wMaxPacketSize is
   * downgraded from 512 to 64 bytes.  The initial packets are sent
   * correctly, but subsequent packets contain wrong data.
   *
   * Performing the check below in a version-dependent manner is
   * possible (obtaining the firmware version only requires small
   * reply packets), but some means would have to be added to
   * communicate the information below back to the next higher layers.
   * As long as no fixed firmware is known, simply bail out here
   * instead.
   */
  if (event_ep != 0 &&
      max_xfer < USBDEV_MAX_XFER_3)
    {
      statusOut("Sorry, the JTAGICE3's firmware is broken on USB 1.1 connections\n");
#ifdef HAVE_LIBUSB_2_0
      usb20_cleanup(pdev);
#else
      usb_close(pdev);
#endif
      return NULL;
    }

  return pdev;
}

#ifdef HAVE_LIBUSB_2_0
/* USB thread */
static void *usb_thread(void * data)
{
  struct pollfd fds[2];

  fds[0].fd = pype[0];
  fds[0].events = POLLIN | POLLRDNORM;
  fds[1].fd = libusb20_dev_get_fd(udev);
  // should we also poll for possible USB OUT transfers when splitting
  // one message into multiple packets?
  fds[1].events = POLLIN | POLLRDNORM;

  while (1)
    {
      char buf[MAX_MESSAGE];
      char rbuf[MAX_MESSAGE + sizeof(unsigned int)];
      char ebuf[USBDEV_MAX_EVT_3 + sizeof(unsigned int)];
      int rv;

      if (!libusb20_tr_pending(xfr_in))
	{
	  // setup and start new bulk IN transfer
	  libusb20_tr_setup_bulk(xfr_in, rbuf + sizeof(unsigned int),
				 max_xfer, 0);
	  libusb20_tr_start(xfr_in);
	}

      if (event_ep != 0 &&
	  !libusb20_tr_pending(xfr_evt))
	{
	  // setup and start new bulk IN transfer
	  libusb20_tr_setup_bulk(xfr_evt, ebuf + sizeof(unsigned int),
				 USBDEV_MAX_EVT_3, 0);
	  libusb20_tr_start(xfr_evt);
	}

      fds[0].revents = fds[1].revents = 0;
      rv = poll(fds, 2, INFTIM);
      if (rv < 0)
	{
	  if (errno != EINTR)
	    perror("poll()");
	  continue;
	}
      if (rv == 0)
	// why did poll() return?
	continue;

      if ((fds[0].revents & POLLERR) != 0 ||
	  (fds[1].revents & POLLERR) != 0)
	{
	  fprintf(stderr, "poll() returned POLLERR, why?\n");
	  fds[0].revents &= ~POLLERR;
	  fds[1].revents &= ~POLLERR;
	}
      if ((fds[0].revents & (POLLNVAL | POLLHUP)) != 0 ||
	  (fds[1].revents & (POLLNVAL | POLLHUP)) != 0)
	// fd is closed
	pthread_exit((void *)0);

      if (fds[0].revents != 0)
	{
	  // something is in the pipe there
	  if ((rv = read(pype[0], buf, MAX_MESSAGE)) > 0)
	    {
	      int offset = 0;

	      libusb20_tr_stop(xfr_in);

	      while (rv != 0)
		{
		  uint32_t amnt, result;

		  if (rv > max_xfer)
		    amnt = max_xfer;
 		  else
		    amnt = rv;
		  // right now, we run the bulk writes synchronously
		  uint8_t xfrstatus;

		  xfrstatus = libusb20_tr_bulk_intr_sync(xfr_out, buf + offset, amnt,
							 &result, 5000);
		  if (xfrstatus !=
		      (enum libusb20_transfer_status)LIBUSB20_TRANSFER_COMPLETED)
		    {
		      fprintf(stderr, "USB bulk write error: %s\n",
			      usb_transfer_msg(xfrstatus));
		      pthread_exit((void *)1);
		    }
		  if (result != amnt)
		    {
		      fprintf(stderr, "USB bulk short write: %u != %u\n",
			      result, amnt);
		      pthread_exit((void *)1);
		    }
		  if (rv == max_xfer)
		    {
		      /* send ZLP */
		      libusb20_tr_bulk_intr_sync(xfr_out, buf, 0,
						 &result, 5000);
		    }
		  rv -= amnt;
		  offset += amnt;
		}

	      libusb20_tr_setup_bulk(xfr_in, rbuf + sizeof(unsigned int),
				     max_xfer, 0);
	      libusb20_tr_start(xfr_in);
	    }
	  else if (errno != EINTR && errno != EAGAIN)
	    {
	      fprintf(stderr, "read error from AVaRICE: %s\n",
		      strerror(errno));
	      pthread_exit((void *)1);
	    }
	}

      if (fds[1].revents != 0)
	{
	  // something's available on USB
	  if ((rv = libusb20_dev_process(udev)) != 0)
	    // what's up?
	    continue;

	  if (!libusb20_tr_pending(xfr_in))
	    {

	      uint32_t result = libusb20_tr_get_actual_length(xfr_in);
	      uint8_t xfrstatus = libusb20_tr_get_status(xfr_in);

	      if (xfrstatus !=
		  (enum libusb20_transfer_status)LIBUSB20_TRANSFER_COMPLETED)
		{
		  fprintf(stderr, "USB bulk read error: %s\n",
			  usb_transfer_msg(xfrstatus));
		  pthread_exit((void *)1);
		}
	      /*
	       * We read a packet from USB.  If it's been a partial
	       * one (result matches the endpoint size), see to get
	       * more, until we have either a short read, or a ZLP.
	       *
	       * We do it synchronously right now.
	       */
	      unsigned int pkt_len = result;
	      bool needmore = result == max_xfer;

	      /* OK, if there is more to read, do so. */
	      while (needmore)
		{
		  int maxlen = MAX_MESSAGE - pkt_len;
		  if (maxlen > max_xfer)
		    maxlen = max_xfer;
		  xfrstatus = libusb20_tr_bulk_intr_sync(xfr_in,
							 rbuf + sizeof(unsigned int) + pkt_len,
							 maxlen, &result, 100);

		  if (xfrstatus !=
		      (enum libusb20_transfer_status)LIBUSB20_TRANSFER_COMPLETED)
		    {
		      fprintf(stderr, "USB bulk read error in continuation block: %s\n",
			      usb_transfer_msg(xfrstatus));
		      pthread_exit((void *)1);
		    }
		  if (result == 0)
		    {
		      /* Zero-length packet: we are done. */
		      break;
		    }

		  needmore = rv == max_xfer;
		  pkt_len += rv;
		  if (pkt_len == MAX_MESSAGE)
		    {
		      /* should not happen */
		      fprintf(stderr, "Message too big in USB receive.\n");
		      break;
		    }
		}

	      unsigned int writesize = pkt_len;
	      char *writep = rbuf + sizeof(unsigned int);
	      if (event_ep != 0)
		{
		  /*
		   * On the JTAGICE3, we prepend the length, so the
		   * parent knows how much data to expect from the
		   * pipe.
		   */
		  memcpy(rbuf, &pkt_len, sizeof(unsigned int));
		  writep -= sizeof(unsigned int);
		  writesize += sizeof(unsigned int);
		}

	      if (write(pype[0], writep, writesize) != writesize)
		{
		  fprintf(stderr, "short write to AVaRICE: %s\n",
			  strerror(errno));
		  pthread_exit((void *)1);
		}
	    }

	  if (event_ep != 0 &&
	      !libusb20_tr_pending(xfr_evt))
	    {

	      uint32_t result = libusb20_tr_get_actual_length(xfr_evt);
	      uint8_t xfrstatus = libusb20_tr_get_status(xfr_evt);

	      if (xfrstatus !=
		  (enum libusb20_transfer_status)LIBUSB20_TRANSFER_COMPLETED)
		{
		  fprintf(stderr, "USB bulk read error: %s\n",
			  usb_transfer_msg(xfrstatus));
		  pthread_exit((void *)1);
		}

	      if (result != 0)
		{
		  /* No partial packet handling needed on the event EP */

		  if (ebuf[0] != TOKEN)
		    {
		      fprintf(stderr,
			      "usb_daemon(): first byte of event message is not TOKEN but 0x%02x\n",
			      ebuf[0]);
		    }
		  else
		    {
		      unsigned int pkt_len = result;

		      /*
		       * On the JTAGICE3, we prepend the message length, so
		       * the parent knows how much data to expect from the
		       * pipe.
		       */
		      memcpy(ebuf, &pkt_len, sizeof(unsigned int));

		      ebuf[sizeof(unsigned int)] = TOKEN_EVT3;

		      if (write(pype[0], ebuf, pkt_len + sizeof(unsigned int))
			  != pkt_len + sizeof(unsigned int))
			{
			  fprintf(stderr, "short write to AVaRICE: %s\n",
				  strerror(errno));
			  pthread_exit((void *)1);
			}
		    }
		}
	    }

	}
    }
}

#else

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

	    if (rv > max_xfer)
	      amnt = max_xfer;
	    else
	      amnt = rv;
	    result = usb_bulk_write(udev, write_ep,
				    buf + offset, amnt, 5000);
	    if (result != amnt)
	    {
	      fprintf(stderr, "USB bulk write error: %s\n",
		      usb_strerror());
	      pthread_exit((void *)1);
	    }
	    if (rv == max_xfer)
	    {
	      /* send ZLP */
	      usb_bulk_write(udev, write_ep, buf, 0, 5000);
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

/* USB event reader thread (JTAGICE3 only) */
static void *usb_thread_read(void *data)
{
  while (1)
    {
      char buf[MAX_MESSAGE + sizeof(unsigned int)];
      int rv;

      rv = usb_bulk_read(udev, read_ep, buf + sizeof(unsigned int),
			 max_xfer, 0);
      if (rv == 0 || rv == -EINTR || rv == -EAGAIN || rv == -ETIMEDOUT)
      {
	/* OK, try again */
      }
      else if (rv < 0)
      {
	fprintf(stderr, "USB bulk read error: %s (%d)\n",
		usb_strerror(), rv);
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
	bool needmore = rv == max_xfer;

	/* OK, if there is more to read, do so. */
	while (needmore)
	{
	  int maxlen = MAX_MESSAGE - pkt_len;
	  if (maxlen > max_xfer)
	    maxlen = max_xfer;
	  rv = usb_bulk_read(udev, read_ep, buf + sizeof(unsigned int) + pkt_len,
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

	  needmore = rv == max_xfer;
	  pkt_len += rv;
	  if (pkt_len == MAX_MESSAGE)
	  {
	    /* should not happen */
	    fprintf(stderr, "Message too big in USB receive.\n");
	    break;
	  }
	}

	unsigned int writesize = pkt_len;
	char *writep = buf + sizeof(unsigned int);
	if (event_ep != 0)
	  {
	    /*
	     * On the JTAGICE3, we prepend the message length, so
	     * the parent knows how much data to expect from the
	     * pipe.
	     */
	    memcpy(buf, &pkt_len, sizeof(unsigned int));
	    writep -= sizeof(unsigned int);
	    writesize += sizeof(unsigned int);
	  }

	if (write(pype[0], writep, writesize) != writesize)
	{
	  fprintf(stderr, "short write to AVaRICE: %s\n",
		  strerror(errno));
	  pthread_exit((void *)1);
	}
      }
    }
}

/* USB reader thread */
static void *usb_thread_event(void *data)
{
  while (1)
    {
      /*
       * Events are shorter than regular data packets, so no
       * reassembly and ZLP handling is needed here.
       */
      char buf[USBDEV_MAX_EVT_3 + sizeof(unsigned int)];
      int rv;

      rv = usb_bulk_read(udev, event_ep, buf + sizeof(unsigned int),
			 USBDEV_MAX_EVT_3, 0);
      if (rv == 0 || rv == -EINTR || rv == -EAGAIN || rv == -ETIMEDOUT)
      {
	/* OK, try again */
      }
      else if (rv < 0)
      {
	fprintf(stderr, "USB event read error: %s (%d)\n",
		usb_strerror(), rv);
	pthread_exit((void *)1);
      }
      else
      {
	if (buf[sizeof(unsigned int)] != TOKEN)
	{
	  fprintf(stderr,
		  "usb_daemon(): first byte of event message is not TOKEN but 0x%02x\n",
		  buf[sizeof(unsigned int)]);
	}
	else
	{
	  unsigned int pkt_len = rv;

	  /*
	   * On the JTAGICE3, we prepend the message length first,
	   * the parent knows how much data to expect from the
	   * pipe.
	   */
	  memcpy(buf, &pkt_len, sizeof(unsigned int));

	  buf[sizeof(unsigned int)] = TOKEN_EVT3;
	  if (write(pype[0], buf, pkt_len + sizeof(unsigned int)) !=
	      pkt_len + sizeof(unsigned int))
	  {
	    fprintf(stderr, "short write to AVaRICE: %s\n",
		    strerror(errno));
	    pthread_exit((void *)1);
	  }
	}
      }
    }
}
#endif

void jtag::resetUSB(void)
{
#ifndef HAVE_LIBUSB_2_0
  if (udev)
    {
      usb_resetep(udev, read_ep);
      usb_resetep(udev, write_ep);
      if (event_ep != 0)
	usb_resetep(udev, event_ep);
    }
#endif
}

static void
cleanup_usb(void)
{
#ifdef HAVE_LIBUSB_2_0
  usb20_cleanup(udev);
#else
  usb_release_interface(udev, usb_interface);
  usb_close(udev);
#endif
}


void jtag::openUSB(const char *jtagDeviceName)
{
  if (socketpair(AF_UNIX, SOCK_STREAM, PF_UNSPEC, pype) < 0)
      throw jtag_exception("cannot create pipe");

  udev = opendev(jtagDeviceName, emu_type, usb_interface);
  if (udev == NULL)
    throw jtag_exception("cannot open USB device");

#ifdef HAVE_LIBUSB_2_0
  pthread_create(&utid, NULL, usb_thread, NULL);
#  ifdef __FreeBSD__
  pthread_set_name_np(utid, "USB thread");
#  endif
#else
  pthread_create(&rtid, NULL, usb_thread_read, NULL);
  pthread_create(&wtid, NULL, usb_thread_write, NULL);
  if (event_ep != 0)
    pthread_create(&etid, NULL, usb_thread_event, NULL);
#  ifdef __FreeBSD__
  pthread_set_name_np(rtid, "USB reader thread");
  pthread_set_name_np(wtid, "USB writer thread");
  if (event_ep != 0)
    pthread_set_name_np(etid, "USB event thread");
#  endif
#endif

  atexit(cleanup_usb);

  jtagBox = pype[1];
}

#endif /* HAVE_LIBUSB */
