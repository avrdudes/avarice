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
 */

#include "avarice.h"

#ifdef HAVE_LIBUSB

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <usb.h>

#ifdef HAVE_LIBHIDAPI
#include <hidapi/hidapi.h>
#include <poll.h>
#endif

#include <thread>

#include "jtag.h"

#define USB_VENDOR_ATMEL 1003
#define USB_DEVICE_JTAGICEMKII 0x2103
#define USB_DEVICE_AVRDRAGON 0x2107
#define USB_DEVICE_JTAGICE3 0x2110
#define USB_DEVICE_EDBG 0x2111

/* JTAGICEmkII */
#define USBDEV_BULK_EP_WRITE_MKII 0x02
#define USBDEV_BULK_EP_READ_MKII 0x82
#define USBDEV_MAX_XFER_MKII 64

/* JTAGICE3 */
#define USBDEV_BULK_EP_WRITE_3 0x01
#define USBDEV_BULK_EP_READ_3 0x82
#define USBDEV_EVT_EP_READ_3 0x83
#define USBDEV_MAX_XFER_3 512
#define USBDEV_MAX_EVT_3 64

/* EDBG (JTAGICE3 3.x, Atmel-ICE, Integrated EDBG */
#define USBDEV_INTERRUPT_EP_WRITE_EDBG 0x01
#define USBDEV_INTERRUPT_EP_READ_EDBG 0X82
#define USBDEV_MAX_XFER_EDBG 512

#define MAX_MESSAGE 512

#ifdef HAVE_LIBHIDAPI
struct hid_thread_data {
    unsigned int max_pkt_size;
};
#endif

static int read_ep, write_ep, event_ep, max_xfer;
using usb_dev_t = usb_dev_handle;

static usb_dev_t *udev = nullptr;
#ifdef HAVE_LIBHIDAPI
static hid_device *hdev = nullptr;
static std::thread usb_hid_thread;
#endif
static int pype[2];

static std::thread usb_read_thread;
static std::thread usb_write_thread;
static std::thread usb_event_thread;
static int usb_interface;

/*
 * Walk down all USB devices, and see whether we can find our emulator
 * device.
 */
static usb_dev_t *opendev(const char *jtagDeviceName, Emulator emu_type) {
    char string[256];
    struct usb_bus *bus;
    struct usb_device *dev;
    usb_dev_t *pdev;
    char *serno, *cp2;
    uint16_t pid;
    size_t x;

    switch (emu_type) {
    case Emulator::JTAGICE2:
        pid = USB_DEVICE_JTAGICEMKII;
        read_ep = USBDEV_BULK_EP_READ_MKII;
        write_ep = USBDEV_BULK_EP_WRITE_MKII;
        event_ep = 0;
        max_xfer = USBDEV_MAX_XFER_MKII;
        break;

    case Emulator::DRAGON:
        pid = USB_DEVICE_AVRDRAGON;
        read_ep = USBDEV_BULK_EP_READ_MKII;
        write_ep = USBDEV_BULK_EP_WRITE_MKII;
        event_ep = 0;
        max_xfer = USBDEV_MAX_XFER_MKII;
        break;

    case Emulator::JTAGICE3:
        pid = USB_DEVICE_JTAGICE3;
        read_ep = USBDEV_BULK_EP_READ_3;
        write_ep = USBDEV_BULK_EP_WRITE_3;
        event_ep = USBDEV_EVT_EP_READ_3;
        max_xfer = USBDEV_MAX_XFER_3;
        break;

    default:
        // should not happen
        return nullptr;
    }

    char* devnamecopy = new char[x = strlen(jtagDeviceName) + 1];
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
    if ((serno = strchr(devnamecopy, ':')) != nullptr) {
        /* first, drop all colons there if any */
        cp2 = ++serno;

        while ((cp2 = strchr(cp2, ':')) != nullptr) {
            x = strlen(cp2) - 1;
            memmove(cp2, cp2 + 1, x);
            cp2[x] = '\0';
        }

        if (strlen(serno) > 12) {
            debugOut( "invalid serial number \"%s\"", serno);
            delete[] devnamecopy;
            return nullptr;
        }
    }

    usb_init();

    usb_find_busses();
    usb_find_devices();

    pdev = nullptr;
    bool found = false;
    for (bus = usb_get_busses(); !found && bus; bus = bus->next) {
        for (dev = bus->devices; !found && dev; dev = dev->next) {
            pdev = usb_open(dev);
            if (pdev) {
                if (dev->descriptor.idVendor == USB_VENDOR_ATMEL &&
                    dev->descriptor.idProduct == pid) {
                    /* yeah, we found something */
                    int rv = usb_get_string_simple(pdev, dev->descriptor.iSerialNumber, string,
                                                   sizeof(string));
                    if (rv < 0) {
                        debugOut( "cannot read serial number \"%s\"", usb_strerror());
                        return nullptr;
                    }

                    debugOut("Found JTAG ICE, serno: %s\n", string);
                    if (serno != nullptr) {
                        /*
                         * See if the serial number requested by the
                         * user matches what we found, matching
                         * right-to-left.
                         */
                        x = strlen(string) - strlen(serno);
                        if (strcasecmp(string + x, serno) != 0) {
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

    delete[] devnamecopy;
    if (!found) {
        printf("did not find any%s USB device \"%s\"\n", serno ? " (matching)" : "",
               jtagDeviceName);
        return nullptr;
    }

    if (dev->config == nullptr) {
        statusOut("USB device has no configuration\n");
    fail:
        usb_close(pdev);
        return nullptr;
    }
    if (usb_set_configuration(pdev, dev->config[0].bConfigurationValue)) {
        statusOut("error setting configuration %d: %s\n", dev->config[0].bConfigurationValue,
                  usb_strerror());
        goto fail;
    }
    usb_interface = dev->config[0].interface[0].altsetting[0].bInterfaceNumber;
    if (usb_claim_interface(pdev, usb_interface)) {
        statusOut("error claiming interface %d: %s\n", usb_interface, usb_strerror());
        goto fail;
    }
    struct usb_endpoint_descriptor *epp = dev->config[0].interface[0].altsetting[0].endpoint;
    for (int i = 0; i < dev->config[0].interface[0].altsetting[0].bNumEndpoints; i++) {
        if ((epp[i].bEndpointAddress == read_ep || epp[i].bEndpointAddress == write_ep) &&
            epp[i].wMaxPacketSize < max_xfer) {
            statusOut("downgrading max_xfer from %d to %d due to EP 0x%02x's wMaxPacketSize\n",
                      max_xfer, epp[i].wMaxPacketSize, epp[i].bEndpointAddress);
            max_xfer = epp[i].wMaxPacketSize;
        }
    }

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
    if (event_ep != 0 && max_xfer < USBDEV_MAX_XFER_3) {
        statusOut("Sorry, the JTAGICE3's firmware is broken on USB 1.1 connections\n");
        usb_close(pdev);
        return nullptr;
    }

    return pdev;
}

#ifdef HAVE_LIBHIDAPI
/*
 * Open HID, used for CMSIS-DAP (EDBG) devices
 */
static hid_device *openhid(const char *jtagDeviceName, unsigned int &max_pkt_size) {
    size_t x;
    wchar_t wserno[15];
    size_t serlen = 0;

    char *devnamecopy = new char[x = strlen(jtagDeviceName) + 1];
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
    char *serno = strchr(devnamecopy, ':');
    if (serno != nullptr) {
        /* first, drop all colons there if any */
        char *cp2 = ++serno;

        while ((cp2 = strchr(cp2, ':')) != nullptr) {
            x = strlen(cp2) - 1;
            memmove(cp2, cp2 + 1, x);
            cp2[x] = '\0';
        }

        serlen = strlen(serno);
        if (serlen > 12) {
            debugOut( "invalid serial number \"%s\"", serno);
            delete[] devnamecopy;
            return nullptr;
        }
        mbstowcs(wserno, serno, 15);
    }
    delete[] devnamecopy;

    /*
     * Find any Atmel device which is a HID.  Then, look at the product
     * string whether it contains the mandatory substring "CMSIS-DAP".
     * (This distinguishes the ICEs from e.g. a keyboard or mouse demo
     * using the Atmel VID.)
     *
     * If a serial number has been asked for, try to match it as well.
     */
    auto list = hid_enumerate(USB_VENDOR_ATMEL, 0);
    if (!list)
        return nullptr;

    auto walk = list;
    while (walk) {
        if (wcsstr(walk->product_string, L"CMSIS-DAP") != nullptr) {
            debugOut("Found HID PID:VID 0x%04x:0x%04x, serno %ls\n", walk->vendor_id,
                     walk->product_id, walk->serial_number);
            // Atmel CMSID-DAP device found
            // If no serial number requested, we are done.
            if (serlen == 0)
                break;
            // Otherwise, match the serial number
            size_t slen = wcslen(walk->serial_number);
            if (slen >= serlen) {
                if (wcscmp(walk->serial_number + slen - serlen, wserno) == 0) {
                    // found matching serial number
                    debugOut("...matched\n");
                    break;
                }
            }
            // else: proceed to next device
        }
        walk = walk->next;
    }
    if (walk == nullptr) {
        debugOut( "No (matching) HID found\n");
        hid_free_enumeration(list);
        return nullptr;
    }

    hid_device *pdev = hid_open_path(walk->path);
    hid_free_enumeration(list);
    if (pdev == nullptr)
        // can't happen?
        return nullptr;

    hid_set_nonblocking(pdev, 1);

    /*
     * Probe for the endpoint size.  Atmel tools are very picky about
     * always being talked to with full-sized packets.  Alas, libhidapi
     * has no API function to obtain the endpoint size, so we first send
     * an inquiry in 64 bytes (for mEDBG), and if we don't get a timely
     * response, provide another 448 bytes to complete the 512-byte
     * packet (JTAGICE3, Atmel-ICE, full EDBG).
     */
    debugOut("Probing for HID max. packet size\n");
    max_pkt_size = 64; // first guess
    unsigned char probebuf[512 + 1] = {
        0,    // no HID report number used
        0,    // DAP_info command
        0xFF, // get max. packet size
    };
    hid_write(pdev, probebuf, 64 + 1);
    int res = hid_read_timeout(pdev, probebuf, 10 /* bytes */, 50 /* milliseconds */);
    if (res == 0) {
        // no timely response, assume 512 byte size
        hid_write(pdev, probebuf, (512 - 64) + 1);
        max_pkt_size = 512;
        res = hid_read_timeout(pdev, probebuf, 10, 50);
    }
    if (res <= 0) {
        debugOut( "openhid(): device not responding to DAP_Info\n");
        hid_close(pdev);
        return nullptr;
    }
    if (probebuf[0] != 0 || probebuf[1] != 2) {
        debugOut("Unexpected DAP_Info response 0x%02x 0x%02x\n", probebuf[0], probebuf[1]);
    } else {
        unsigned int probesize = probebuf[2] + (probebuf[3] << 8);
        if (probesize != 64 && probesize != 512) {
            debugOut("Unexpected max. packet size %u, proceeding with %u\n", probesize,
                     max_pkt_size);
        } else {
            debugOut("Setting max. packet size to %u from DAP_Info\n", probesize);
            max_pkt_size = probesize;
        }
    }

    return pdev;
}
#endif // HAVE_LIBHIDAPI


/* USB writer thread */
static void *usb_write_handler(void *) {
    while (true) {
        char buf[MAX_MESSAGE];
        auto rv = read(pype[0], buf, MAX_MESSAGE);
        if (rv > 0) {
            int offset = 0;

            while (rv != 0) {
                int amnt, result;

                if (rv > max_xfer)
                    amnt = max_xfer;
                else
                    amnt = rv;
                result = usb_bulk_write(udev, write_ep, buf + offset, amnt, 5000);
                if (result != amnt) {
                    debugOut( "USB bulk write error: %s\n", usb_strerror());
                    pthread_exit((void *)1);
                }
                if (rv == max_xfer) {
                    /* send ZLP */
                    usb_bulk_write(udev, write_ep, buf, 0, 5000);
                }
                rv -= amnt;
                offset += amnt;
            }
            continue;
        } else if (errno != EINTR && errno != EAGAIN) {
            debugOut( "read error from AVaRICE: %s\n", strerror(errno));
            pthread_exit((void *)1);
        }
    }
}

/* USB event reader thread (JTAGICE3 only) */
static void *usb_read_handler(void *) {
    while (true) {
        char buf[MAX_MESSAGE + sizeof(unsigned int)];
        int rv = usb_bulk_read(udev, read_ep, buf + sizeof(unsigned int), max_xfer, 0);
        if (rv == 0 || rv == -EINTR || rv == -EAGAIN || rv == -ETIMEDOUT) {
            /* OK, try again */
        } else if (rv < 0) {
            debugOut( "USB bulk read error: %s (%d)\n", usb_strerror(), rv);
            pthread_exit((void *)1);
        } else {
            /*
             * We read a packet from USB.  If it's been a partial
             * one (result matches the endpoint size), see to get
             * more, until we have either a short read, or a ZLP.
             */
            unsigned int pkt_len = rv;
            bool needmore = rv == max_xfer;

            /* OK, if there is more to read, do so. */
            while (needmore) {
                int maxlen = MAX_MESSAGE - pkt_len;
                if (maxlen > max_xfer)
                    maxlen = max_xfer;
                rv =
                    usb_bulk_read(udev, read_ep, buf + sizeof(unsigned int) + pkt_len, maxlen, 100);

                if (rv == -EINTR || rv == -EAGAIN || rv == -ETIMEDOUT) {
                    continue;
                }
                if (rv == 0) {
                    /* Zero-length packet: we are done. */
                    break;
                }
                if (rv < 0) {
                    debugOut( "USB bulk read error in continuation block: %s\n",
                            usb_strerror());
                    pthread_exit((void *)1);
                }

                needmore = rv == max_xfer;
                pkt_len += rv;
                if (pkt_len == MAX_MESSAGE) {
                    /* should not happen */
                    debugOut( "Message too big in USB receive.\n");
                    break;
                }
            }

            unsigned int writesize = pkt_len;
            char *writep = buf + sizeof(unsigned int);
            if (event_ep != 0) {
                /*
                 * On the JTAGICE3, we prepend the message length, so
                 * the parent knows how much data to expect from the
                 * pipe.
                 */
                memcpy(buf, &pkt_len, sizeof(unsigned int));
                writep -= sizeof(unsigned int);
                writesize += sizeof(unsigned int);
            }

            if (write(pype[0], writep, writesize) != writesize) {
                debugOut( "short write to AVaRICE: %s\n", strerror(errno));
                pthread_exit((void *)1);
            }
        }
    }
}

/* USB reader thread */
static void *usb_event_handler(void *) {
    while (true) {
        /*
         * Events are shorter than regular data packets, so no
         * reassembly and ZLP handling is needed here.
         */
        char buf[USBDEV_MAX_EVT_3 + sizeof(unsigned int)];

        const auto rv = usb_bulk_read(udev, event_ep, buf + sizeof(unsigned int), USBDEV_MAX_EVT_3, 0);
        if (rv == 0 || rv == -EINTR || rv == -EAGAIN || rv == -ETIMEDOUT) {
            /* OK, try again */
        } else if (rv < 0) {
            debugOut( "USB event read error: %s (%d)\n", usb_strerror(), rv);
            pthread_exit((void *)1);
        } else {
            if (buf[sizeof(unsigned int)] != TOKEN) {
                debugOut(
                        "usb_daemon(): first byte of event message is not TOKEN but 0x%02x\n",
                        buf[sizeof(unsigned int)]);
            } else {
                unsigned int pkt_len = rv;

                /*
                 * On the JTAGICE3, we prepend the message length first,
                 * the parent knows how much data to expect from the
                 * pipe.
                 */
                memcpy(buf, &pkt_len, sizeof(unsigned int));

                buf[sizeof(unsigned int)] = TOKEN_EVT3;
                if (write(pype[0], buf, pkt_len + sizeof(unsigned int)) !=
                    pkt_len + sizeof(unsigned int)) {
                    debugOut( "short write to AVaRICE: %s\n", strerror(errno));
                    pthread_exit((void *)1);
                }
            }
        }
    }
}

void Jtag::resetUSB() {
    if (udev) {
        usb_resetep(udev, read_ep);
        usb_resetep(udev, write_ep);
        if (event_ep != 0)
            usb_resetep(udev, event_ep);
    }
}

#ifdef HAVE_LIBHIDAPI
static void *hid_handler(void *data) {
    auto hdata = static_cast<struct hid_thread_data *>(data);

    pollfd fds[1];
    fds[0].fd = pype[0];
    fds[0].events = POLLIN | POLLRDNORM;

    debugOut("HID thread started\n");

    while (true) {
        // One additional byte is for libhidapi to tell we don't use HID
        // report numbers.  Four bytes are wrapping overhead.
        unsigned char buf[MAX_MESSAGE + 5];

        // Poll for data from main thread.
        // Wait for at most 50 ms, so we can regularly
        // ping for events even if no upstream activity
        // is present.
        fds[0].revents = 0;
        int rv = poll(fds, 1, 50);
        if (rv < 0) {
            if (errno != EINTR)
                perror("poll()");
            continue;
        }
        if (rv == 0) {
            // timed out, so just ping for event
            buf[0] = 0;
            buf[1] = EDBG_VENDOR_AVR_EVT;
            rv = hid_write(hdev, buf, hdata->max_pkt_size + 1);
            if (rv < 0)
                throw jtag_exception("Querying for event: hid_write() failed");

            rv = hid_read_timeout(hdev, buf, hdata->max_pkt_size + 1, 200);
            if (rv <= 0) {
                debugOut("Querying for event: hid_read() failed (%d)\n", rv);
                continue;
            }
            // Now examine whether the reply actually contained an event.
            if (buf[0] != EDBG_VENDOR_AVR_EVT) {
                debugOut("Querying for event: unexpected response (0x%02x)\n", buf[0]);
                continue;
            }
            if (buf[1] == 0 && buf[2] == 0)
                // nothing returned
                continue;
            unsigned int len = buf[1] * 256 + buf[2];
            if (len > MAX_MESSAGE - 10) {
                debugOut("Querying for event: insane event size %u\n", len);
                continue;
            }
            // tag this as an event packet
            buf[3] = TOKEN_EVT3;
            // make room to prepend packet length
            memmove(buf + sizeof(unsigned int), buf + 3, len);
            memcpy(buf, &len, sizeof(unsigned int));
            // pass event upstream
            write(pype[0], buf, len + sizeof(unsigned int));
            continue;
        }

        if ((fds[0].revents & POLLERR) != 0) {
            debugOut( "poll() returned POLLERR, why?\n");
            fds[0].revents &= ~POLLERR;
        }
        if ((fds[0].revents & (POLLNVAL | POLLHUP)) != 0)
            // fd is closed
            pthread_exit(nullptr);

        if (fds[0].revents != 0) {
            // something is in the pipe there, presumably a command
            // read to offset 5 to leave room for the wrapper
            if ((rv = read(pype[0], buf + 5, MAX_MESSAGE)) > 0) {
                if (rv < 6) {
                    debugOut("Reading command from AVaRICE failed\n");
                    continue;
                }

                // used in both, request and reply data
                unsigned int npackets = (rv + hdata->max_pkt_size - 1) / hdata->max_pkt_size;
                unsigned int thispacket = 1;
                unsigned int len = rv;
                // used in reassembling reply data
                size_t offset = sizeof(unsigned int);
                unsigned int totlength = 0;

                while (thispacket <= npackets) {
                    if (thispacket != 1)
                        memmove(buf + 5, buf + (hdata->max_pkt_size - 4) + 5, len);

                    buf[0] = 0; // libhidapi: no report ID
                    buf[1] = EDBG_VENDOR_AVR_CMD;
                    buf[2] = (thispacket << 4) | npackets;
                    unsigned int cursize =
                        (len > hdata->max_pkt_size - 4) ? hdata->max_pkt_size - 4 : len;
                    buf[3] = cursize >> 8;
                    buf[4] = cursize;
                    rv = hid_write(hdev, buf, hdata->max_pkt_size + 1);
                    if ((unsigned)rv != hdata->max_pkt_size + 1) {
                        debugOut("hid_write: short write, %u vs. %d\n", hdata->max_pkt_size + 1,
                                 rv);
                        goto done;
                    }

                    rv = hid_read_timeout(hdev, buf, hdata->max_pkt_size + 1, 200);
                    if (rv < 0)
                        throw jtag_exception("Error reading HID");

                    thispacket++;
                    len -= cursize;
                }

                // Query response
                for (npackets = 0, thispacket = 0; thispacket <= npackets; thispacket++) {
                    buf[offset] = 0;
                    buf[offset + 1] = EDBG_VENDOR_AVR_RSP;
                    rv = hid_write(hdev, buf + offset, hdata->max_pkt_size + 1);
                    if (rv < 0)
                        throw jtag_exception("Querying for response: hid_write() failed");

                    rv = hid_read_timeout(hdev, buf + offset, hdata->max_pkt_size + 1, 500);
                    if (rv <= 0) {
                        debugOut("Querying for response: hid_read() failed (%d)\n", rv);
                        goto done;
                    }
                    debugOut("Received 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", buf[offset],
                             buf[offset + 1], buf[offset + 2], buf[offset + 3], buf[offset + 4],
                             buf[offset + 5]);
                    // Now examine whether the reply actually contained a response.
                    if (buf[offset] != EDBG_VENDOR_AVR_RSP) {
                        debugOut("Querying for response: unexpected response (0x%02x)\n", buf[0]);
                        goto done;
                    }
                    if (npackets == 0) {
                        // first fragment arriving
                        npackets = buf[offset + 1] & 0x0F;
                        thispacket = 1;
                    }
                    if ((buf[offset + 1] >> 4) != thispacket) {
                        debugOut("Wrong fragment: got %d, expected %d\n", (buf[offset + 1] >> 4),
                                 thispacket);
                        goto done;
                    }
                    unsigned int packet_len = buf[offset + 2] * 256 + buf[offset + 3];
                    if (packet_len < 5 || packet_len > hdata->max_pkt_size) {
                        debugOut("Querying for response: insane event size %u\n", packet_len);
                        goto done;
                    }
                    totlength += packet_len;
                    if (totlength > MAX_MESSAGE) {
                        debugOut("reply size too large: %u\n", totlength);
                        goto done;
                    }
                    // Update length field to pass (later) upstream
                    memcpy(buf, &totlength, sizeof(unsigned int));
                    // skip wrapper data in payload
                    memmove(buf + offset, buf + offset + 4, packet_len);
                    offset += packet_len;
                }
                // pass reply upstream
                write(pype[0], buf, totlength + sizeof(unsigned int));
            done:;
            } else if (errno != EINTR && errno != EAGAIN) {
                debugOut( "read error from AVaRICE: %s\n", strerror(errno));
                pthread_exit((void *)1);
            }
        }
    }
}

static void cleanup_hid() {
    hid_close(hdev);
    hdev = nullptr;
}
#endif

static void cleanup_usb() {
    usb_release_interface(udev, usb_interface);
    usb_close(udev);
}

void Jtag::openUSB(const char *jtagDeviceName) {
    if (socketpair(AF_UNIX, SOCK_STREAM, PF_UNSPEC, pype) < 0)
        throw jtag_exception("cannot create pipe");

    if (emu_type == Emulator::EDBG) {
#ifdef HAVE_LIBHIDAPI
        static struct hid_thread_data hdata;
        hdev = openhid(jtagDeviceName, hdata.max_pkt_size = 512);
        if (hdev == nullptr)
            throw jtag_exception("cannot open HID");

        usb_hid_thread = std::thread(hid_handler, &hdata);
        atexit(cleanup_hid);
#else // !HAVE_LIBHIDAPI
        throw jtag_exception("EDBG/CMSIS-DAP devices require libhidapi support");
#endif
    } else {
        udev = opendev(jtagDeviceName, emu_type);
        if (udev == nullptr)
            throw jtag_exception("cannot open USB device");

        usb_read_thread = std::thread(usb_read_handler, nullptr);
        usb_write_thread = std::thread( usb_write_handler, nullptr);
        if (event_ep != 0)
            usb_event_thread = std::thread( usb_event_handler, nullptr);

        atexit(cleanup_usb);
    }

    jtagBox = pype[1];
}

#endif /* HAVE_LIBUSB */
