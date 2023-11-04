
/*
 *	avarice - The "avarice" program.
 *	Copyright (C) 2001 Scott Finneran
 *	Copyright (C) 2002, 2003, 2004 Intel Corporation
 *	Copyright (C) 2005,2006,2007 Joerg Wunsch
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License Version 2
 *	as published by the Free Software Foundation.
 *
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
 * This file contains the main() & support for avrjtagd.
 *
 * $Id$
 */


#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <termios.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <fcntl.h>

#include "avarice.h"
#include "remote.h"
#include "jtag.h"
#include "jtag1.h"
#include "jtag2.h"
#include "jtag3.h"
#include "gnu_getopt.h"

bool ignoreInterrupts;

static int makeSocket(struct sockaddr_in *name)
{
    int sock;
    int tmp;
    struct protoent *protoent;

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        throw jtag_exception();

    // Allow rapid reuse of this port.
    tmp = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&tmp, sizeof(tmp)) < 0)
        throw jtag_exception();

    // Enable TCP keep alive process.
    tmp = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char *)&tmp, sizeof(tmp)) < 0)
        throw jtag_exception();

    if (bind(sock, (struct sockaddr *)name, sizeof(*name)) < 0)
    {
	if (errno == EADDRINUSE)
	    fprintf(stderr, "bind() failed: another server is still running on this port\n");
        throw jtag_exception("bind() failed");
    }

    protoent = getprotobyname("tcp");
    if (protoent == NULL)
    {
        fprintf(stderr, "tcp protocol unknown (oops?)");
        throw jtag_exception();
    }

    tmp = 1;
    if (setsockopt(sock, protoent->p_proto, TCP_NODELAY,
                   (char *)&tmp, sizeof(tmp)) < 0)
        throw jtag_exception();

    return sock;
}


static void initSocketAddress(struct sockaddr_in *name,
			      const char *hostname, unsigned short int port)
{
    struct hostent *hostInfo;

    memset(name, 0, sizeof(*name));
    name->sin_family = AF_INET;
    name->sin_port = htons(port);
    // Try numeric interpretation (1.2.3.4) first, then
    // hostname resolution if that failed.
    if (inet_aton(hostname, &name->sin_addr) == 0)
    {
	hostInfo = gethostbyname(hostname);
	if (hostInfo == NULL)
	{
	    fprintf(stderr, "Unknown host %s", hostname);
	    throw jtag_exception();
	}
	name->sin_addr = *(struct in_addr *)hostInfo->h_addr;
    }
}

static unsigned long parseJtagBitrate(const char *val)
{
    char *endptr, c;
    unsigned long v;

    if (*val == '\0')
    {
        fprintf(stderr, "invalid number in JTAG bit rate");
        throw jtag_exception();
    }
    v = strtoul(val, &endptr, 10);
    if (*endptr == '\0')
	return v;
    while (isspace((unsigned char)(c = *endptr)))
	endptr++;
    switch (c)
    {
    case 'k':
    case 'K':
	v *= 1000UL;
	return v;
    case 'm':
    case 'M':
	v *= 1000000UL;
	return v;
    }
    if (strcmp(endptr, "Hz") == 0)
	return v;

    fprintf(stderr, "invalid number in JTAG bit rate");
    throw jtag_exception();
}

static void usage(const char *progname)
{
    fprintf(stderr,
	    "Usage: %s [OPTION]... [[HOST_NAME]:PORT]\n\n", progname);
    fprintf(stderr, "Options:\n");
    fprintf(stderr,
	    "  -h, --help                  Print this message.\n");
    fprintf(stderr,
	    "  -1, --mkI                   Connect to JTAG ICE mkI (default)\n");
    fprintf(stderr,
	    "  -2, --mkII                  Connect to JTAG ICE mkII\n");
    fprintf(stderr,
	    "  -3, --jtag3                 Connect to JTAGICE3 (Firmware 2.x)\n");
    fprintf(stderr,
            "  -4, --edbg                  Atmel-ICE, or JTAGICE3 (firmware 3.x), or EDBG Integrated Debugger\n");
    fprintf(stderr,
            "  -B, --jtag-bitrate <rate>   Set the bitrate that the JTAG box communicates\n"
            "                                with the avr target device. This must be less\n"
            "                                than 1/4 of the frequency of the target. Valid\n"
            "                                values are 1000/500/250/125 kHz (mkI),\n"
	    "                                or 22 through 6400 kHz (mkII).\n"
            "                                (default: 250 kHz)\n");
    fprintf(stderr,
	    "  -C, --capture               Capture running program.\n"
	    "                                Note: debugging must have been enabled prior\n"
            "                                to starting the program. (e.g., by running\n"
            "                                avarice earlier)\n");
    fprintf(stderr,
	    "  -c, --daisy-chain <ub,ua,bb,ba> Daisy chain settings:\n"
	    "                                <units before, units after,\n"
	    "                                bits before, bits after>\n");
    fprintf(stderr,
	    "  -D, --detach                Detach once synced with JTAG ICE\n");
    fprintf(stderr,
	    "  -d, --debug                 Enable printing of debug information.\n");
    fprintf(stderr,
            "  -e, --erase                 Erase target.\n");
    fprintf(stderr,
            "  -E, --event <eventlist>     List of events that do not interrupt.\n"
            "                                JTAG ICE mkII and AVR Dragon only.\n"
            "                                Default is \"none,run,target_power_on,target_sleep,target_wakeup\"\n");
#if ENABLE_TARGET_PROGRAMMING
    fprintf(stderr,
	    "  -f, --file <filename>       Specify a file for use with the --program and\n"
            "                                --verify options. If --file is passed and\n"
            "                                neither --program or --verify are given then\n"
            "                                --program is implied.\n");
#endif	// ENABLE_TARGET_PROGRAMMING
    fprintf(stderr,
	    "  -g, --dragon                Connect to an AVR Dragon rather than a JTAG ICE.\n"
	    "                                This implies --mkII, but might be required in\n"
	    "                                addition to --debugwire when debugWire is to\n"
	    "                                be used.\n");
    fprintf(stderr,
	    "  -I, --ignore-intr           Automatically step over interrupts.\n"
	    "                                Note: EXPERIMENTAL. Can not currently handle\n"
            "                                devices fused for compatibility.\n");
    fprintf(stderr,
	    "  -j, --jtag <devname>        Port attached to JTAG box (default: /dev/avrjtag).\n");
    fprintf(stderr,
	    "  -k, --known-devices         Print a list of known devices.\n");
    fprintf(stderr,
            "  -L, --write-lockbits <ll>   Write lock bits.\n");
    fprintf(stderr,
            "  -l, --read-lockbits         Read lock bits.\n");
    fprintf(stderr,
            "  -P, --part <name>           Target device name (e.g."
            " atmega16)\n\n");
#if ENABLE_TARGET_PROGRAMMING
    fprintf(stderr,
	    "  -p, --program               Program the target.\n"
	    "                                Binary filename must be specified with --file\n"
	    "                                option.\n");
#endif	// ENABLE_TARGET_PROGRAMMING
    fprintf(stderr,
            "  -r, --read-fuses            Read fuses bytes.\n");
    fprintf(stderr,
            "  -R, --reset-srst            External reset through nSRST signal.\n");
    fprintf(stderr,
            "  -u, --updi                  AVR part is an ATxmega device, using UPDI.\n");
    fprintf(stderr,
	    "  -V, --version               Print version information.\n");
#if ENABLE_TARGET_PROGRAMMING
    fprintf(stderr,
            "  -v, --verify                Verify program in device against file specified\n"
            "                                with --file option.\n");
#endif	// ENABLE_TARGET_PROGRAMMING
    fprintf(stderr,
	    "  -w, --debugwire             For the JTAG ICE mkII, connect to the target\n"
	    "                                using debugWire protocol rather than JTAG.\n");
    fprintf(stderr,
            "  -W, --write-fuses <eehhll>  Write fuses bytes.\n");
    fprintf(stderr,
            "  -x, --xmega                 AVR part is an ATxmega device, using JTAG.\n");
    fprintf(stderr,
            "  -X, --pdi                   AVR part is an ATxmega device, using PDI.\n");
    fprintf(stderr,
	    "HOST_NAME defaults to 0.0.0.0 (listen on any interface).\n"
	    "\":PORT\" is required to put avarice into gdb server mode.\n\n");
    fprintf(stderr,
            "Example usage:\n");
    fprintf(stderr,
            "\t%s --jtag /dev/ttyS0 :4242\n",
            progname);
    fprintf(stderr,
            "\t%s --dragon :4242\n",
            progname);
    fprintf(stderr, "\n");
    exit(0);
}

static int comparenames(const void *a, const void *b)
{
    const jtag_device_def_type *ja = (const jtag_device_def_type *)a;
    const jtag_device_def_type *jb = (const jtag_device_def_type *)b;

    return strcmp(ja->name, jb->name);
}

static void knownParts()
{
    fprintf(stderr, "List of known AVR devices:\n\n");

    jtag_device_def_type* dev = deviceDefinitions;
    // Count the device descriptor records.
    size_t n = 0;
    while (dev->name != NULL)
      n++, dev++;
    // For historical reasons, device definitions are not
    // sorted.  Sort them here.
    qsort(deviceDefinitions, n, sizeof(jtag_device_def_type),
	  comparenames);
    fprintf(stderr,
	    "%-15s  %10s  %8s  %8s\n", "Device Name", "Device ID",
	    "Flash", "EEPROM");
    fprintf(stderr,
	    "%-15s  %10s  %8s  %8s\n", "---------------", "---------",
	    "-------", "-------");
    dev = deviceDefinitions;
    while (n-- != 0)
    {
        unsigned eesize = dev->eeprom_page_size * dev->eeprom_page_count;

	if (eesize != 0 && eesize < 1024)
	  fprintf(stderr, "%-15s      0x%04X  %4d KiB  %4.1f KiB\n",
		  dev->name,
		  dev->device_id,
		  dev->flash_page_size * dev->flash_page_count / 1024,
		  eesize / 1024.0);
	else
	  fprintf(stderr, "%-15s      0x%04X  %4d KiB  %4d KiB\n",
		  dev->name,
		  dev->device_id,
		  dev->flash_page_size * dev->flash_page_count / 1024,
		  eesize / 1024);
	dev++;
    }

    exit(1);
}

static struct option long_opts[] = {
    /* name,                 has_arg, flag,   val */
    { "mkI",                 0,       0,     '1' },
    { "mkII",                0,       0,     '2' },
    { "jtag3",               0,       0,     '3' },
    { "edbg",                0,       0,     '4' },
    { "jtag-bitrate",        1,       0,     'B' },
    { "capture",             0,       0,     'C' },
    { "daisy-chain",         1,       0,     'c' },
    { "detach",              0,       0,     'D' },
    { "debug",               0,       0,     'd' },
    { "erase",               0,       0,     'e' },
    { "event",               1,       0,     'E' },
    { "file",                1,       0,     'f' },
    { "dragon",              0,       0,     'g' },
    { "help",                0,       0,     'h' },
    { "ignore-intr",         0,       0,     'I' },
    { "jtag",                1,       0,     'j' },
    { "known-devices",       0,       0,     'k' },
    { "write-lockbits",      1,       0,     'L' },
    { "read-lockbits",       0,       0,     'l' },
    { "part",                1,       0,     'P' },
    { "program",             0,       0,     'p' },
    { "reset-srst",          0,       0,     'R' },
    { "read-fuses",          0,       0,     'r' },
    { "updi",                0,       0,     'u' },
    { "version",             0,       0,     'V' },
    { "verify",              0,       0,     'v' },
    { "debugwire",           0,       0,     'w' },
    { "write-fuses",         1,       0,     'W' },
    { "xmega",               0,       0,     'x' },
    { "pdi",                 0,       0,     'X' },
    { 0,                     0,       0,      0 }
};

jtag *theJtagICE;

int main(int argc, char **argv)
{
    int sock;
    struct sockaddr_in clientname;
    struct sockaddr_in name;
    char *inFileName = 0;
    const char *jtagDeviceName = NULL;
    char *device_name = 0;
    const char *eventlist = "none,run,target_power_on,target_sleep,target_wakeup";
    unsigned long jtagBitrate = 0;
    const char *hostName = "0.0.0.0";	/* INADDR_ANY */
    int  hostPortNumber = 0;
    bool erase = false;
    bool program = false;
    bool readFuses = false;
    bool writeFuses = false;
    char *fuses = NULL;
    bool readLockBits = false;
    bool writeLockBits = false;
    bool gdbServerMode = false;
    char *lockBits = NULL;
    bool detach = false;
    bool capture = false;
    bool verify = false;
    bool apply_nsrst = false;
    bool is_xmega = false;
    char *progname = argv[0];
    enum {
	MKI, MKII, DRAGON, JTAG3, EDBG
    } devicetype = MKI;		// default to mkI devicetype
    enum debugproto proto = PROTO_JTAG;
    int  option_index;
    unsigned int units_before = 0;
    unsigned int units_after = 0;
    unsigned int bits_before = 0;
    unsigned int bits_after = 0;

    statusOut("AVaRICE version %s, %s %s\n\n",
	      PACKAGE_VERSION, __DATE__, __TIME__);

    device_name = 0;

    opterr = 0;                 /* disable default error message */

    while (1)
    {
        int c = getopt_long (argc, argv, "1234B:Cc:DdeE:f:ghIj:kL:lP:pRruVvwW:xX",
                             long_opts, &option_index);
        if (c == -1)
            break;              /* no more options */

        switch (c) {
            case 'h':
            case '?':
                usage(progname);
            case 'k':
                knownParts();
	    case '1':
		devicetype = MKI;
		break;
	    case '2':
		devicetype = MKII;
		break;
	    case '3':
		devicetype = JTAG3;
		break;
            case '4':
                devicetype = EDBG;
                break;
            case 'B':
		jtagBitrate = parseJtagBitrate(optarg);
                break;
            case 'C':
                capture = true;
                break;
            case 'c':
                if (sscanf(optarg,"%u,%u,%u,%u",
                           &units_before, &units_after,
                           &bits_before, &bits_after) != 4)
                    usage(progname);
		if (units_before > bits_before || units_after > bits_after ||
		    bits_before > 32 || bits_after > 32) {
		    fprintf(stderr,
			    "%s: daisy-chain parameters out of range"
			    " (max. 32 bits before/after)\n",
			    progname);
		    exit(1);
		}
                break;
            case 'D':
                detach = true;
                break;
            case 'd':
                debugMode = true;
                break;
            case 'e':
                erase = true;
                break;
            case 'E':
                eventlist = optarg;
                break;
            case 'f':
                inFileName = optarg;
                break;
	    case 'g':
		devicetype = DRAGON;
		break;
            case 'I':
                ignoreInterrupts = true;
                break;
            case 'j':
                jtagDeviceName = optarg;
                break;
            case 'L':
                lockBits = optarg;
                writeLockBits = true;
                break;
            case 'l':
                readLockBits = true;
                break;
            case 'P':
                device_name = optarg;
                break;
            case 'p':
                program = true;
                break;
	    case 'R':
	        apply_nsrst = true;
		break;
            case 'r':
                readFuses = true;
                break;
            case 'u':
		proto = PROTO_UPDI;
		break;
            case 'V':
                exit(0);
            case 'v':
                verify = true;
                break;
            case 'w':
		proto = PROTO_DW;
		break;
            case 'W':
                fuses = optarg;
                writeFuses = true;
                break;
            case 'x':
                is_xmega = true;
                break;
            case 'X':
                is_xmega = true;
		proto = PROTO_PDI;
                break;
            default:
                fprintf (stderr, "getop() did something screwey");
                exit (1);
        }
    }

    if ((optind+1) == argc) {
        /* Looks like user has given [[host]:port], so parse out the host and
           port number then enable gdb server mode. */

        int i;
        char *arg = argv[optind];
        int len = strlen (arg);
        char *host = new char[len+1];
        memset (host, '\0', len+1);

        for (i=0; i<len; i++) {
            if ((arg[i] == '\0') || (arg[i] == ':'))
                break;

            host[i] = arg[i];
        }

        if (strlen (host)) {
            hostName = host;
        }

        if (arg[i] == ':') {
            i++;
        }

        if (i >= len) {
            /* No port was given. */
            fprintf (stderr, "avarice: %s is not a valid host:port value.\n",
                     arg);
            exit (1);
        }

        char *endptr;
        hostPortNumber = (int)strtol(arg+i, &endptr, 0);
        if (endptr == arg+i) {
            /* Invalid convertion. */
            fprintf (stderr, "avarice: failed to convert port number: %s\n",
                     arg+i);
            exit (1);
        }

        /* Make sure the the port value is not a priviledged port and is not
           greater than max port value. */

        if ((hostPortNumber < 1024) || (hostPortNumber > 0xffff)) {
            fprintf (stderr, "avarice: invalid port number: %d (must be >= %d"
                     " and <= %d)\n", hostPortNumber, 1024, 0xffff);
            exit (1);
        }

        gdbServerMode = true;
    }
    else if (optind != argc) {
        usage (progname);
    }

    if (jtagBitrate == 0 && (proto == PROTO_JTAG))
    {
        fprintf (stdout,
                 "Defaulting JTAG bitrate to 250 kHz.\n\n");

        jtagBitrate = 250000;
    }

    // Use a default device name to connect to if not specified on the
    // command-line.  If the JTAG_DEV environment variable is set, use
    // the name given there.  As the AVR Dragon can only be talked to
    // through USB, default it to USB, but use a generic name else.
    if (jtagDeviceName == NULL)
    {
      char *cp = getenv ("JTAG_DEV");

      if (cp != NULL)
	jtagDeviceName = cp;
      else if (devicetype == DRAGON || devicetype == JTAG3 || devicetype == EDBG)
	jtagDeviceName = "usb";
      else
	jtagDeviceName = "/dev/avrjtag";
    }

    if (debugMode)
      setvbuf(stderr, NULL, _IOLBF, 0);

    int rv = 0;			// return value from main()

    try {
	// And say hello to the JTAG box
	switch (devicetype) {
	case MKI:
	    theJtagICE = new jtag1(jtagDeviceName, device_name, apply_nsrst);
	    break;

	case MKII:
	case DRAGON:
	    theJtagICE = new jtag2(jtagDeviceName, device_name, proto,
				   devicetype == DRAGON, apply_nsrst, is_xmega);
	    break;

	case JTAG3:
	    theJtagICE = new jtag3(jtagDeviceName, device_name, proto,
				   apply_nsrst, is_xmega);
	    break;

	case EDBG:
	    theJtagICE = new jtag3(jtagDeviceName, device_name, proto,
				   apply_nsrst, is_xmega, true);
	    break;
	}

	// Set Daisy-chain variables
	theJtagICE->dchain.units_before = (unsigned char) units_before;
	theJtagICE->dchain.units_after = (unsigned char) units_after;
	theJtagICE->dchain.bits_before = (unsigned char) bits_before;
	theJtagICE->dchain.bits_after = (unsigned char) bits_after;

        // Tell which events to ignore.
        theJtagICE->parseEvents(eventlist);

	// Init JTAG box.
	theJtagICE->initJtagBox();

        if (erase)
        {
            if (proto == PROTO_DW)
            {
                statusOut("WARNING: Chip erase not possible in debugWire mode; ignored\n");
            }
            else
            {
                theJtagICE->enableProgramming();
                statusOut("Erasing program memory.\n");
                theJtagICE->eraseProgramMemory();
                statusOut("Erase complete.\n");
                theJtagICE->disableProgramming();
            }
        }

        if (readFuses)
        {
            theJtagICE->jtagReadFuses();
        }

        if (readLockBits)
        {
            theJtagICE->jtagReadLockBits();
        }

        if (writeFuses)
            theJtagICE->jtagWriteFuses(fuses);

        // Init JTAG debugger for initial use.
        //   - If we're attaching to a running target, we cannot do this.
        //   - If we're running as a standalone programmer, we don't want
        //     this.
        if( gdbServerMode && ( ! capture ) )
            theJtagICE->initJtagOnChipDebugging(jtagBitrate);

        if (inFileName != (char *)0)
        {
#if ENABLE_TARGET_PROGRAMMING
            if ((program == false) && (verify == false)) {
                /* If --file is given and neither --program or --verify, then we
                   program, but not verify so as to be backward compatible with
                   the old meaning of --file from before the addition of --program
                   and --verify. */

                program = true;
            }

            if ((erase == false) && (program == true)) {
                statusOut("WARNING: The default behaviour has changed.\n"
                          "Programming no longer erases by default. If you want to"
                          " erase and program\nin a single step, use the --erase "
                          "in addition to --program. The reason for\nthis change "
                          "is to allow programming multiple sections (e.g. "
                          "application and\nbootloader) in multiple passes.\n\n");
            }

            theJtagICE->downloadToTarget(inFileName, program, verify);
            theJtagICE->resetProgram(false);
#else  // !ENABLE_TARGET_PROGRAMMING
	    statusOut("\n\n"
		      "AVaRICE has not been configured for target programming\n"
		      "through the --program option.  Target programming in\n"
		      "AVaRICE is a deprecated feature; use AVRDUDE instead.\n");
	    rv = 1;
#endif // ENABLE_TARGET_PROGRAMMING
        }
        else
        {
            if( (program) || (verify))
            {
#if ENABLE_TARGET_PROGRAMMING
                fprintf(stderr, 
                   "\nERROR: Filename not specified."
                   " Use the --file option.\n");
                throw jtag_exception();
#else  // !ENABLE_TARGET_PROGRAMMING
		statusOut("\n\n"
			  "AVaRICE has not been configured for target programming\n"
			  "through the --program option.  Target programming in\n"
			  "AVaRICE is a deprecated feature; use AVRDUDE instead.\n");
		rv = 1;
#endif // ENABLE_TARGET_PROGRAMMING
            }
        }

        // Write fuses after all programming parts have completed.
        if (writeLockBits)
            theJtagICE->jtagWriteLockBits(lockBits);

        // Quit & resume mote for operations that don't interact with gdb.
        if (!gdbServerMode)
            theJtagICE->resumeProgram();
        else
        {
            initSocketAddress(&name, hostName, hostPortNumber);
            sock = makeSocket(&name);
            statusOut("Waiting for connection on port %hu.\n", hostPortNumber);
            if (listen(sock, 1) < 0)
                throw jtag_exception();

            if (detach)
            {
                int child = fork();

                if (child < 0)
                {
                    fprintf(stderr, "Failed to fork");
                    throw jtag_exception();
                }
                if (child != 0)
                    _exit(0);
                else
                    if (setsid() < 0)
                    {
                        fprintf(stderr, "setsid failed - weird bug");
                        throw jtag_exception();
                    }
            }

            // Connection request on original socket.
            socklen_t size = (socklen_t)sizeof(clientname);
            int gfd = accept(sock, (struct sockaddr *)&clientname, &size);
            if (gfd < 0)
                throw jtag_exception();
            statusOut("Connection opened by host %s, port %hu.\n",
                      inet_ntoa(clientname.sin_addr), ntohs(clientname.sin_port));

            setGdbFile(gfd);

            // Now do the actual processing of GDB messages
            // We stay here until exiting because of error of EOF on the
            // gdb connection
            for (;;)
                talkToGdb();
        }
    }
    catch (const char *msg)
      {
	fprintf(stderr, "%s\n", msg);
	return 1;
      }
    catch (jtag_exception&)
    {
        // ignored; guarantee theJtagICE object will be deleted
        // correctly, as this says "good-bye" to the JTAG ICE mkII
        rv = 1;
    }

    delete theJtagICE;

    return rv;
}
