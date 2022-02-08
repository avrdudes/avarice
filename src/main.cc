
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
 */

#include <arpa/inet.h>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <string>

#include "avarice.h"
#include "gnu_getopt.h"
#include "jtag.h"
#include "jtag1.h"
#include "jtag2.h"
#include "jtag3.h"
#include "remote.h"

bool ignoreInterrupts;

static int makeSocket(struct sockaddr_in *name) {
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

    if (bind(sock, (struct sockaddr *)name, sizeof(*name)) < 0) {
        if (errno == EADDRINUSE)
            debugOut( "bind() failed: another server is still running on this port\n");
        throw jtag_exception("bind() failed");
    }

    protoent = getprotobyname("tcp");
    if (protoent == nullptr) {
        debugOut( "tcp protocol unknown (oops?)");
        throw jtag_exception();
    }

    tmp = 1;
    if (setsockopt(sock, protoent->p_proto, TCP_NODELAY, (char *)&tmp, sizeof(tmp)) < 0)
        throw jtag_exception();

    return sock;
}

static void initSocketAddress(struct sockaddr_in *name, const char *hostname,
                              unsigned short int port) {
    struct hostent *hostInfo;

    memset(name, 0, sizeof(*name));
    name->sin_family = AF_INET;
    name->sin_port = htons(port);
    // Try numeric interpretation (1.2.3.4) first, then
    // hostname resolution if that failed.
    if (inet_aton(hostname, &name->sin_addr) == 0) {
        hostInfo = gethostbyname(hostname);
        if (hostInfo == nullptr) {
            debugOut( "Unknown host %s", hostname);
            throw jtag_exception();
        }
        name->sin_addr = *(struct in_addr *)hostInfo->h_addr;
    }
}

static unsigned long parseJtagBitrate(const char *val) {
    char *endptr, c;

    if (*val == '\0') {
        debugOut( "invalid number in JTAG bit rate");
        throw jtag_exception();
    }
    unsigned long v = strtoul(val, &endptr, 10);
    if (*endptr == '\0')
        return v;
    while (isspace((unsigned char)(c = *endptr)))
        endptr++;
    switch (c) {
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

    debugOut( "invalid number in JTAG bit rate");
    throw jtag_exception();
}

static void usage(const char *progname) {
    debugOut( "Usage: %s [OPTION]... [[HOST_NAME]:PORT]\n\n", progname);
    debugOut( "Options:\n");
    debugOut( "  -h, --help                  Print this message.\n");
    debugOut( "  -1, --mkI                   Connect to JTAG ICE mkI (default)\n");
    debugOut( "  -2, --mkII                  Connect to JTAG ICE mkII\n");
    debugOut( "  -3, --jtag3                 Connect to JTAGICE3 (Firmware 2.x)\n");
    debugOut( "  -4, --edbg                  Atmel-ICE, or JTAGICE3 (firmware 3.x), or EDBG "
                    "Integrated Debugger\n");
    debugOut(
            "  -B, --jtag-bitrate <rate>   Set the bitrate that the JTAG box communicates\n"
            "                                with the avr target device. This must be less\n"
            "                                than 1/4 of the frequency of the target. Valid\n"
            "                                values are 1000/500/250/125 kHz (mkI),\n"
            "                                or 22 through 6400 kHz (mkII).\n"
            "                                (default: 250 kHz)\n");
    debugOut( "  -C, --capture               Capture running program.\n"
                    "                                Note: debugging must have been enabled prior\n"
                    "                                to starting the program. (e.g., by running\n"
                    "                                avarice earlier)\n");
    debugOut( "  -c, --daisy-chain <ub,ua,bb,ba> Daisy chain settings:\n"
                    "                                <units before, units after,\n"
                    "                                bits before, bits after>\n");
    debugOut( "  -D, --detach                Detach once synced with JTAG ICE\n");
    debugOut( "  -d, --debug                 Enable printing of debug information.\n");
    debugOut( "  -e, --erase                 Erase target.\n");
    debugOut( "  -E, --event <eventlist>     List of events that do not interrupt.\n"
                    "                                JTAG ICE mkII and AVR Dragon only.\n"
                    "                                Default is "
                    "\"none,run,target_power_on,target_sleep,target_wakeup\"\n");
    debugOut(
            "  -g, --dragon                Connect to an AVR Dragon rather than a JTAG ICE.\n"
            "                                This implies --mkII, but might be required in\n"
            "                                addition to --debugwire when debugWire is to\n"
            "                                be used.\n");
    debugOut( "  -I, --ignore-intr           Automatically step over interrupts.\n"
                    "                                Note: EXPERIMENTAL. Can not currently handle\n"
                    "                                devices fused for compatibility.\n");
    debugOut(
            "  -j, --jtag <devname>        Port attached to JTAG box (default: /dev/avrjtag).\n");
    debugOut( "  -k, --known-devices         Print a list of known devices.\n");
    debugOut( "  -L, --write-lockbits <ll>   Write lock bits.\n");
    debugOut( "  -l, --read-lockbits         Read lock bits.\n");
    debugOut( "  -P, --part <name>           Target device name (e.g."
                    " atmega16)\n\n");
    debugOut( "  -r, --read-fuses            Read fuses bytes.\n");
    debugOut( "  -R, --reset-srst            External reset through nSRST signal.\n");
    debugOut( "  -V, --version               Print version information.\n");
    debugOut( "  -w, --debugwire             For the JTAG ICE mkII, connect to the target\n"
                    "                                using debugWire protocol rather than JTAG.\n");
    debugOut( "  -W, --write-fuses <eehhll>  Write fuses bytes.\n");
    debugOut( "  -x, --xmega                 AVR part is an ATxmega device, using JTAG.\n");
    debugOut( "  -X, --pdi                   AVR part is an ATxmega device, using PDI.\n");
    debugOut( "HOST_NAME defaults to 0.0.0.0 (listen on any interface).\n"
                    "\":PORT\" is required to put avarice into gdb server mode.\n\n");
    debugOut( "Example usage:\n");
    debugOut( "\t%s --jtag /dev/ttyS0 :4242\n", progname);
    debugOut( "\t%s --dragon :4242\n", progname);
    debugOut( "\n");
}

static void knownParts() {
    debugOut( "List of known AVR devices:\n\n");
    jtag_device_def_type::DumpAll();
}

static struct option long_opts[] = {
    /* name,                 has_arg, flag,   val */
    {"mkI", 0, nullptr, '1'},           {"mkII", 0, nullptr, '2'},
    {"jtag3", 0, nullptr, '3'},         {"edbg", 0, nullptr, '4'},
    {"jtag-bitrate", 1, nullptr, 'B'},  {"capture", 0, nullptr, 'C'},
    {"daisy-chain", 1, nullptr, 'c'},   {"detach", 0, nullptr, 'D'},
    {"debug", 0, nullptr, 'd'},         {"erase", 0, nullptr, 'e'},
    {"event", 1, nullptr, 'E'},         {"file", 1, nullptr, 'f'},
    {"dragon", 0, nullptr, 'g'},        {"help", 0, nullptr, 'h'},
    {"ignore-intr", 0, nullptr, 'I'},   {"jtag", 1, nullptr, 'j'},
    {"known-devices", 0, nullptr, 'k'}, {"write-lockbits", 1, nullptr, 'L'},
    {"read-lockbits", 0, nullptr, 'l'}, {"part", 1, nullptr, 'P'},
    {"program", 0, nullptr, 'p'},       {"reset-srst", 0, nullptr, 'R'},
    {"read-fuses", 0, nullptr, 'r'},    {"version", 0, nullptr, 'V'},
    {"verify", 0, nullptr, 'v'},        {"debugwire", 0, nullptr, 'w'},
    {"write-fuses", 1, nullptr, 'W'},   {"xmega", 0, nullptr, 'x'},
    {"pdi", 0, nullptr, 'X'},           {nullptr, 0, nullptr, 0}};

std::unique_ptr<Jtag> theJtagICE;

int main(int argc, char **argv) {
    int sock;
    sockaddr_in clientname;
    sockaddr_in name;
    char *inFileName = nullptr;
    const char *jtagDeviceName = nullptr;
    const char *eventlist = "none,run,target_power_on,target_sleep,target_wakeup";
    unsigned long jtagBitrate = 0;
    const char *hostName = "0.0.0.0"; /* INADDR_ANY */
    int hostPortNumber = 0;
    bool erase = false;
    bool program = false;
    bool readFuses = false;
    bool writeFuses = false;
    const char *fuses = nullptr;
    bool readLockBits = false;
    bool writeLockBits = false;
    bool gdbServerMode = false;
    char *lockBits = nullptr;
    bool detach = false;
    bool capture = false;
    bool verify = false;
    bool apply_nsrst = false;
    bool is_xmega = false;
    const char *progname = argv[0];
    Emulator devicetype = Emulator::JTAGICE; // default to mkI devicetype
    Debugproto proto = Debugproto::JTAG;
    int option_index;
    DaisyChainInfo daisy_chain_info{};

    statusOut("AVaRICE version %s, %s %s\n\n", PACKAGE_VERSION, __DATE__, __TIME__);

    std::string_view device_name;

    opterr = 0; /* disable default error message */

    int rv = 0; // return value from main()

    try {
        while (true) {
            int c = getopt_long(argc, argv, "1234B:Cc:DdeE:f:ghIj:kL:lP:pRrVvwW:xX", long_opts,
                                &option_index);
            if (c == -1)
                break; /* no more options */

            switch (c) {
            case 'h':
            case '?':
                usage(progname);
                exit(0);
                break;
            case 'k':
                knownParts();
                exit(1);
                break;
            case '1':
                devicetype = Emulator::JTAGICE;
                break;
            case '2':
                devicetype = Emulator::JTAGICE2;
                break;
            case '3':
                devicetype = Emulator::JTAGICE3;
                break;
            case '4':
                devicetype = Emulator::EDBG;
                break;
            case 'g':
                devicetype = Emulator::DRAGON;
                break;
            case 'B':
                jtagBitrate = parseJtagBitrate(optarg);
                break;
            case 'C':
                capture = true;
                break;
            case 'c': {
                unsigned int units_before = 0;
                unsigned int units_after = 0;
                unsigned int bits_before = 0;
                unsigned int bits_after = 0;

                if (sscanf(optarg, "%u,%u,%u,%u", &units_before, &units_after, &bits_before,
                           &bits_after) != 4)
                    usage(progname);

                daisy_chain_info = DaisyChainInfo{
                    static_cast<uchar>(units_before), static_cast<uchar>(units_after),
                    static_cast<uchar>(bits_before), static_cast<uchar>(bits_after)};
                if (!daisy_chain_info.IsValid()) {
                    debugOut(
                            "%s: daisy-chain parameters out of range"
                            " (max. 32 bits before/after)\n",
                            progname);
                    exit(1);
                }
                break;
            }
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
            case 'V':
                exit(0);
            case 'v':
                verify = true;
                break;
            case 'w':
                proto = Debugproto::DW;
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
                proto = Debugproto::PDI;
                break;
            default:
                debugOut( "getop() did something screwey");
                exit(1);
            }
        }

        if ((optind + 1) == argc) {
            /* Looks like user has given [[host]:port], so parse out the host and
               port number then enable gdb server mode. */

            size_t i;
            const char *arg = argv[optind];
            const auto len = strlen(arg);
            char *host = new char[len + 1];
            memset(host, '\0', len + 1);

            for (i = 0; i < len; i++) {
                if ((arg[i] == '\0') || (arg[i] == ':'))
                    break;

                host[i] = arg[i];
            }

            if (strlen(host)) {
                hostName = host;
            }

            if (arg[i] == ':') {
                i++;
            }

            if (i >= len) {
                /* No port was given. */
                debugOut( "avarice: %s is not a valid host:port value.\n", arg);
                exit(1);
            }

            char *endptr;
            hostPortNumber = (int)strtol(arg + i, &endptr, 0);
            if (endptr == arg + i) {
                /* Invalid convertion. */
                debugOut( "avarice: failed to convert port number: %s\n", arg + i);
                exit(1);
            }

            /* Make sure the the port value is not a priviledged port and is not
               greater than max port value. */

            if ((hostPortNumber < 1024) || (hostPortNumber > 0xffff)) {
                debugOut(
                        "avarice: invalid port number: %d (must be >= %d"
                        " and <= %d)\n",
                        hostPortNumber, 1024, 0xffff);
                exit(1);
            }

            gdbServerMode = true;
        } else if (optind != argc) {
            usage(progname);
        }

        if (jtagBitrate == 0 && (proto == Debugproto::JTAG)) {
            fprintf(stdout, "Defaulting JTAG bitrate to 250 kHz.\n\n");

            jtagBitrate = 250000;
        }

        // Use a default device name to connect to if not specified on the
        // command-line.  If the JTAG_DEV environment variable is set, use
        // the name given there.  As the AVR Dragon can only be talked to
        // through USB, default it to USB, but use a generic name else.
        if (jtagDeviceName == nullptr) {
            char *cp = getenv("JTAG_DEV");

            if (cp != nullptr)
                jtagDeviceName = cp;
            else if (devicetype == Emulator::DRAGON || devicetype == Emulator::JTAGICE3 ||
                     devicetype == Emulator::EDBG || devicetype == Emulator::JTAGICE2)
                jtagDeviceName = "usb";
            else
                jtagDeviceName = "/dev/avrjtag";
        }

        if (debugMode)
            setvbuf(stderr, nullptr, _IOLBF, 0);

        // And say hello to the JTAG box
        switch (devicetype) {
        case Emulator::JTAGICE:
            theJtagICE =
                std::make_unique<jtag1>(devicetype, jtagDeviceName, device_name, apply_nsrst);
            break;

        case Emulator::JTAGICE2:
        case Emulator::DRAGON:
            theJtagICE = std::make_unique<Jtag2>(devicetype, jtagDeviceName, device_name, proto,
                                                 apply_nsrst, is_xmega);
            break;

        case Emulator::JTAGICE3:
        case Emulator::EDBG:
            theJtagICE = std::make_unique<Jtag3>(devicetype, jtagDeviceName, device_name,
                                                 apply_nsrst, is_xmega, proto);
            break;
        }

        // Set Daisy-chain variables
        theJtagICE->dchain = daisy_chain_info;

        // Tell which events to ignore.
        theJtagICE->parseEvents(eventlist);

        // Init JTAG box.
        theJtagICE->initJtagBox();

        if (erase) {
            if (proto == Debugproto::DW) {
                statusOut("WARNING: Chip erase not possible in debugWire mode; ignored\n");
            } else {
                theJtagICE->enableProgramming();
                statusOut("Erasing program memory.\n");
                theJtagICE->eraseProgramMemory();
                statusOut("Erase complete.\n");
                theJtagICE->disableProgramming();
            }
        }

        if (readFuses) {
            theJtagICE->jtagReadFuses();
        }

        if (readLockBits) {
            theJtagICE->jtagReadLockBits();
        }

        if (writeFuses)
            theJtagICE->jtagWriteFuses(fuses);

        // Init JTAG debugger for initial use.
        //   - If we're attaching to a running target, we cannot do this.
        //   - If we're running as a standalone programmer, we don't want
        //     this.
        if (gdbServerMode && (!capture))
            theJtagICE->initJtagOnChipDebugging(jtagBitrate);

        if (inFileName != (char *)nullptr) {
            statusOut("\n\n"
                      "AVaRICE has not been configured for target programming\n"
                      "through the --program option.  Target programming in\n"
                      "AVaRICE is a deprecated feature; use AVRDUDE instead.\n");
            rv = 1;
        } else {
            if ((program) || (verify)) {
                statusOut("\n\n"
                          "AVaRICE has not been configured for target programming\n"
                          "through the --program option.  Target programming in\n"
                          "AVaRICE is a deprecated feature; use AVRDUDE instead.\n");
                rv = 1;
            }
        }

        // Write fuses after all programming parts have completed.
        if (writeLockBits)
            theJtagICE->jtagWriteLockBits(lockBits);

        // Quit & resume mote for operations that don't interact with gdb.
        if (!gdbServerMode)
            theJtagICE->resumeProgram();
        else {
            initSocketAddress(&name, hostName, hostPortNumber);
            sock = makeSocket(&name);
            statusOut("Waiting for connection on port %hu.\n", hostPortNumber);
            if (listen(sock, 1) < 0)
                throw jtag_exception();

            if (detach) {
                int child = fork();

                if (child < 0) {
                    debugOut( "Failed to fork");
                    throw jtag_exception();
                }
                if (child != 0)
                    _exit(0);
                else if (setsid() < 0) {
                    debugOut( "setsid failed - weird bug");
                    throw jtag_exception();
                }
            }

            // Connection request on original socket.
            auto size = static_cast<socklen_t>(sizeof(clientname));
            int gfd = accept(sock, (struct sockaddr *)&clientname, &size);
            if (gfd < 0)
                throw jtag_exception();
            statusOut("Connection opened by host %s, port %hu.\n", inet_ntoa(clientname.sin_addr),
                      ntohs(clientname.sin_port));

            setGdbFile(gfd);

            // Now do the actual processing of GDB messages
            // We stay here until exiting because of error of EOF on the
            // gdb connection
            for (;;)
                talkToGdb();
        }
    } catch (jtag_exception &) {
        // ignored; guarantee theJtagICE object will be deleted
        // correctly, as this says "good-bye" to the JTAG ICE mkII
        rv = 1;
    } catch (...) {
        // Fatal error?
        rv = 2;
    }

    return rv;
}
