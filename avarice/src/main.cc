/*
 *	avarice - The "avarice" program.
 *	Copyright (C) 2001 Scott Finneran
 *      Copyright (C) 2002 Intel Corporation
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License Version 2
 *      as published by the Free Software Foundation.
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
 */


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
#include "gnu_getopt.h"

bool ignoreInterrupts;

static int makeSocket(struct sockaddr_in *name, unsigned short int port)
{
    int sock;
    int tmp;
    struct protoent *protoent;

    sock = socket(PF_INET, SOCK_STREAM, 0);
    gdbCheck(sock);

    // Allow rapid reuse of this port.
    tmp = 1;
    gdbCheck(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&tmp, sizeof(tmp)));

    // Enable TCP keep alive process.
    tmp = 1;
    gdbCheck(setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char *)&tmp, sizeof(tmp)));

    gdbCheck(bind(sock, (struct sockaddr *)name, sizeof(*name)));

    protoent = getprotobyname("tcp");
    check(protoent != NULL, "tcp protocol unknown (oops?)");

    tmp = 1;
    gdbCheck(setsockopt(sock, protoent->p_proto, TCP_NODELAY,
			(char *)&tmp, sizeof(tmp)));

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
	check(hostInfo != NULL, "Unknown host %s", hostname);
	name->sin_addr = *(struct in_addr *)hostInfo->h_addr;
    }
}


static void usage(const char *progname)
{
    fprintf(stderr,
	    "Usage: %s [OPTION]... [<host-name>[:<port>]]\n\n", progname);
    fprintf(stderr, "Options:\n");
    fprintf(stderr,
	    "  -h, --help                  Print this message.\n");
    fprintf(stderr,
	    "  -d, --debug                 Enable printing of debug"
	    " information.\n");
    fprintf(stderr,
	    "  -D, --detach                Detach once synced with JTAG ICE\n");
    fprintf(stderr,
	    "  -C, --capture               Capture running program.\n"
	    "                                Note: debugging must have been enabled prior\n"
            "                                to starting the program. (e.g., by running\n"
            "                                avarice earlier)\n");
    fprintf(stderr,
	    "  -I, --ignore-intr           Automatically step over interrupts.\n"
	    "                                Note: EXPERIMENTAL. Can not currently handle\n"
            "                                devices fused for compatibility.\n");
    fprintf(stderr,
	    "  -f, --file <filename>       Specify a file for use with the --program and\n"
            "                                --verify options. If --file is passed and\n"
            "                                neither --program or --verify are given then\n"
            "                                --program is implied.\n");
    fprintf(stderr,
	    "  -j, --jtag <devname>        Port attached to JTAG box (default: /dev/avrjtag).\n");
    fprintf(stderr,
	    "  -p, --program               Erase and program target.\n"
	    "                                Binary filename must be specified with --file\n"
	    "                                option.\n");
    fprintf(stderr,
            "  -v, --verify                Verify program in device against file specified\n"
            "                                with --file option.\n");
    fprintf(stderr,
            "  -e, --erase                 Erase target.\n");
    fprintf(stderr,
            "  -r, --read-fuses            Read fuses bytes.\n");
    fprintf(stderr,
            "  -W, --write-fuses <eehhll>  Write fuses bytes.\n");
    fprintf(stderr,
            "  -L, --write-lockbits <ll>   Write lock bits.\n");
    fprintf(stderr,
            "  -P, --part <name>           Target device name (e.g."
            " atmega16)\n\n");
    fprintf(stderr,
	    "<host-name> defaults to 0.0.0.0 (listen on any interface).\n"
	    "\":<port>\" is required to put avarice into gdb server mode.\n\n");
    fprintf(stderr,
	    "e.g. %s --program --file test.bin --jtag /dev/ttyS0 :4242\n\n",
	    progname);
    exit(1);
}

static struct option long_opts[] = {
    /* name,                 has_arg, flag,   val */
    { "help",                0,       0,     'h' },
    { "debug",               0,       0,     'd' },
    { "version",             0,       0,     'V' },
    { "program",             0,       0,     'p' },
    { "verify",              0,       0,     'v' },
    { "erase",               0,       0,     'e' },
    { "read-fuses",          0,       0,     'r' },
    { "write-fuses",         1,       0,     'W' },
    { "write-lockbits",      1,       0,     'L' },
    { "part",                1,       0,     'P' },
    { "jtag",                1,       0,     'j' },
    { "ignore-intr",         0,       0,     'I' },
    { "detach",              0,       0,     'D' },
    { "capture",             0,       0,     'C' },
    { "file",                1,       0,     'f' },
    { 0,                     0,       0,      0 }
};

int main(int argc, char **argv)
{
    int sock;
    struct sockaddr_in clientname;
    struct sockaddr_in name;
    char *inFileName = 0;
    char *jtagDeviceName = "/dev/avrjtag";
    const char *hostName = "0.0.0.0";	/* INADDR_ANY */
    int  hostPortNumber;
    bool erase = false;
    bool program = false;
    bool readFuses = false;
    bool writeFuses = false;
    char *fuses;
    bool writeLockBits = false;
    bool gdbServerMode = false;
    char *lockBits;
    bool detach = false;
    bool capture = false;
    bool verify = false;
    char *progname = argv[0];
    int  option_index;

    statusOut("AVaRICE version %s, %s %s\n\n",
	      PACKAGE_VERSION, __DATE__, __TIME__);

    device_name = 0;

    opterr = 0;                 /* disable default error message */
    
    while (1)
    {
        int c = getopt_long (argc, argv, "VhdDCIf:j:pverW:L:P:",
                             long_opts, &option_index);
        if (c == -1)
            break;              /* no more options */

        switch (c) {
            case 'h':
            case '?':
                usage(progname);
            case 'd':
                debugMode = true;
                break;
            case 'D':
                detach = true;
                break;
            case 'C':
                capture = true;
                break;
            case 'I':
                ignoreInterrupts = true;
                break;
            case 'f':
                inFileName = optarg;
                break;
            case 'j':
                jtagDeviceName = optarg;
                break;
            case 'p':
                program = true;
                break;
            case 'v':
                verify = true;
                break;
            case 'e':
                erase = true;
                break;
            case 'r':
                readFuses = true;
                break;
            case 'W':
                fuses = optarg;
                writeFuses = true;
                break;
            case 'L':
                lockBits = optarg;
                writeLockBits = true;
                break;
            case 'P':
                device_name = optarg;
                break;
	    case 'V':
		exit(0);
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

    // And say hello to the JTAG box
    initJtagPort(jtagDeviceName);

    initJtagBox(capture);

    if (erase)
    {
        enableProgramming();
	statusOut("Erasing program memory.\n");
	eraseProgramMemory();
	statusOut("Erase complete.\n");
        disableProgramming();
    }

    if (writeFuses)
        jtagWriteFuses(fuses);

    if (writeLockBits)
        jtagWriteLockBits(lockBits);

    if (inFileName != (char *)0)
    {
        if (program || verify) {
            downloadToTarget(inFileName, program, verify);
        }
        else {
            /* If --file is given and neither --program or --verify, then we
               program, but not verify so as to be backward compatible with
               the old meaning of --file from before the addition of --program
               and --verify. */

            downloadToTarget(inFileName, true, false);
        }
        resetProgram();
    }
    else
    {
	check( (!program) && (!verify),
	      "\nERROR: Filename not specified."
	      " Use the --file option.\n");
    }

    // Quit for operations that don't interact with gdb.
    if (gdbServerMode)
    {
        initSocketAddress(&name, hostName, hostPortNumber);
        sock = makeSocket(&name, hostPortNumber);
        statusOut("Waiting for connection on port %hu.\n", hostPortNumber);
        gdbCheck(listen(sock, 1));

        if (detach)
        {
            int child = fork();

            unixCheck(child, "Failed to fork");
            if (child != 0)
                _exit(0);
            else
                unixCheck(setsid(), "setsid failed - weird bug");
        }

        // Connection request on original socket.
        socklen_t size = (socklen_t)sizeof(clientname);
        int gfd = accept(sock, (struct sockaddr *)&clientname, &size);
        gdbCheck(gfd);
        statusOut("Connection opened by host %s, port %hu.\n",
                  inet_ntoa(clientname.sin_addr), ntohs(clientname.sin_port));

        setGdbFile(gfd);

        // Now do the actual processing of GDB messages
        // We stay here until exiting because of error of EOF on the
        // gdb connection
        for (;;)
            talkToGdb();
    }

    return 0;
}
