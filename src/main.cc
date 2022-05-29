
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

#include <iostream>
#include <string>

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
#include <unistd.h>

#include "avarice.h"
#include "jtag.h"
#include "jtag1.h"
#include "jtag2.h"
#include "jtag3.h"
#include "remote.h"

#include <boost/program_options.hpp>
namespace po = boost::program_options;

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
            fprintf(stderr, "bind() failed: another server is still running on this port\n");
        throw jtag_exception("bind() failed");
    }

    protoent = getprotobyname("tcp");
    if (protoent == nullptr) {
        fprintf(stderr, "tcp protocol unknown (oops?)");
        throw jtag_exception();
    }

    tmp = 1;
    if (setsockopt(sock, protoent->p_proto, TCP_NODELAY, (char *)&tmp, sizeof(tmp)) < 0)
        throw jtag_exception();

    return sock;
}

static void initSocketAddress(struct sockaddr_in *name, const char *hostname,
                              unsigned short int port) {
    memset(name, 0, sizeof(*name));
    name->sin_family = AF_INET;
    name->sin_port = htons(port);
    // Try numeric interpretation (1.2.3.4) first, then
    // hostname resolution if that failed.
    if (inet_aton(hostname, &name->sin_addr) == 0) {
        hostent *hostInfo = gethostbyname(hostname);
        if (hostInfo == nullptr) {
            fprintf(stderr, "Unknown host %s", hostname);
            throw jtag_exception();
        }
        name->sin_addr = *(struct in_addr *)hostInfo->h_addr;
    }
}

static unsigned long parseJtagBitrate(const char *val) {
    char *endptr, c;

    if (*val == '\0') {
        fprintf(stderr, "invalid number in JTAG bit rate");
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

    fprintf(stderr, "invalid number in JTAG bit rate");
    throw jtag_exception();
}

std::unique_ptr<Jtag> theJtagICE;

int main(int argc, char **argv) {
    statusOut("AVaRICE version %s, %s %s\n\n", PACKAGE_VERSION, __DATE__, __TIME__);

    int rv = 0; // return value from main()

    try {
        // printf( "Example usage:\n");
        // printf( "\t%s --jtag /dev/ttyS0 :4242\n", progname);
        // printf( "\t%s --dragon :4242\n", progname);

        // TODO: add '?'

        po::options_description desc("Allowed options");
        // clang-format off
        desc.add_options()
            ("help,h", "produce help message")
            ("version,V", "Print version information.")
            ("mkI,1", "Connect to JTAG ICE mkI (default)")
            ("mkII,2", "Connect to JTAG ICE mkII")
            ("jtag3,3", "Connect to JTAGICE3 (Firmware 2.x)")
            ("edbg,4", "Atmel-ICE, or JTAGICE3 (firmware 3.x),"
                                         "or EDBG Integrated Debugger")
            ("dragon,g", "Connect to an AVR Dragon rather than a JTAG ICE."
                "This implies --mkII, but might be required in"
                "addition to --debugwire when debugWire is to be used.")
            ("known-devices,k", "Print a list of known devices")
            ("xmega,x", "AVR part is an ATxmega device, using JTAG")
            ("pdi,X", "AVR part is an ATxmega device, using PDI.")
            ("debug,d", "Enable printing of debug information.")
            ("erase,e", "Erase target")
            ("detach,D", "Detach once synced with JTAG ICE")
            ("ignore-intr,I", "Automatically step over interrupts."
                "Note: EXPERIMENTAL. Can not currently handle devices fused for compatibility.")
            ("read-fuses,r", "Read fuses bytes.")
            ("capture,C", "Capture running program."
                "Note: debugging must have been enabled prior"
                "to starting the program. (e.g., by running avarice earlier)")
            ("debugwire,w", "For the JTAG ICE mkII, connect to the target"
                "using debugWire protocol rather than JTAG.")
            ( "read-lockbits,l", "Read lock bits")
            ( "reset-srst,R", "External reset through nSRST signal.")
            ("jtag,j", po::value<std::string>(), "Port attached to JTAG box")
            ( "part,P", po::value<std::string>(), "Target device name (e.g. atmega16)")
            ("jtag-bitrate,B", po::value<std::string>()->default_value("250 kHz"),
                "Set the bitrate that the JTAG box communicates with the avr target device."
                "This must be less than 1/4 of the frequency of the target. Valid"
                "values are 1000/500/250/125 kHz (mkI), or 22 through 6400 kHz (mkII).")
            ("daisy-chain,c", po::value<std::string>(), "<ub,ua,bb,ba> Daisy chain settings:"
                "<units before, units after,bits before, bits after>")
            ("write-lockbits,L", po::value<std::string>(), "<ll> Write lock bits.")
            ("write-fuses,W", po::value<std::string>(), "<eehhll>  Write fuses bytes.")
            ("event,E", po::value<std::string>()->default_value("none,run,target_power_on,target_sleep,target_wakeup"),
                "<eventlist>  List of events that do not interrupt. JTAG ICE mkII and AVR Dragon only.")
            ("server", po::value<std::string>(), "[[HOST_NAME]:PORT]\n"
                "HOST_NAME defaults to 0.0.0.0 (listen on any interface).\n"
                ":PORT is required to put avarice into gdb server mode.");
        // clang-format on

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::cout << desc << "\n";
            exit(0);
        }
        if (vm.count("version")) {
            // We already print the version at the start of the program
            exit(0);
        }
        if (vm.count("known-devices")) {
            std::cout << "List of known AVR devices:\n\n";
            jtag_device_def_type::DumpAll();
            exit(1);
        }

        Debugproto proto = Debugproto::JTAG;
        bool is_xmega = false;
        if (vm.count("xmega")) {
            is_xmega = true;
        }
        if (vm.count("pdi")) {
            is_xmega = true;
            proto = Debugproto::PDI;
        }
        if( vm.count("debugwire")) {
            proto = Debugproto::DW;
        }

        const Emulator devicetype = [&vm] {
            if (vm.count("mkII")) {
                return Emulator::JTAGICE2;
            } else if( vm.count("jtag3")) {
                return Emulator::JTAGICE3;
            } else if( vm.count("edbg")) {
                return Emulator::EDBG;
            } else if( vm.count("dragon")) {
                return Emulator::DRAGON;
            } else {
                return Emulator::JTAGICE; // default to mkI devicetype
            }
        }();

        if( vm.count("ignore-intr")) {
            ignoreInterrupts = true;
        }

        const char *hostName = "0.0.0.0"; /* INADDR_ANY */
        int hostPortNumber = 0;
        bool gdbServerMode = false;
        if( vm.count("server")) {
            /* Looks like user has given [[host]:port], so parse out the host and
               port number then enable gdb server mode. */

            size_t i;
            const char* arg = vm["server"].as<std::string>().c_str();
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
                fprintf(stderr, "avarice: %s is not a valid host:port value.\n", arg);
                exit(1);
            }

            char *endptr;
            hostPortNumber = (int)strtol(arg + i, &endptr, 0);
            if (endptr == arg + i) {
                /* Invalid convertion. */
                fprintf(stderr, "avarice: failed to convert port number: %s\n", arg + i);
                exit(1);
            }

            /* Make sure the the port value is not a priviledged port and is not
               greater than max port value. */

            if ((hostPortNumber < 1024) || (hostPortNumber > 0xffff)) {
                fprintf(stderr,
                        "avarice: invalid port number: %d (must be >= %d"
                        " and <= %d)\n",
                        hostPortNumber, 1024, 0xffff);
                exit(1);
            }

            gdbServerMode = true;
        }

        // Use a default device name to connect to if not specified on the
        // command-line.  If the JTAG_DEV environment variable is set, use
        // the name given there.  As the AVR Dragon can only be talked to
        // through USB, default it to USB, but use a generic name else.
        const std::string jtagDeviceName = [&] {
            if (vm.count("jtag")) {
                return vm["jtag"].as<std::string>();
            } else {
                const char *cp = getenv("JTAG_DEV");
                if (cp != nullptr)
                    return std::string{cp};
                else if (devicetype == Emulator::DRAGON || devicetype == Emulator::JTAGICE3 ||
                         devicetype == Emulator::EDBG || devicetype == Emulator::JTAGICE2)
                    return std::string{"usb"};
                else
                    return std::string{"/dev/avrjtag"};
            }
        }();

        if (vm.count("debug")) {
            setvbuf(stderr, nullptr, _IOLBF, 0);
        }

        const bool apply_nsrst = vm.count("reset-srst");

        const std::string device_name = vm["part"].as<std::string>();
        // And say hello to the JTAG box
        switch (devicetype) {
        case Emulator::JTAGICE:
            theJtagICE =
                std::make_unique<jtag1>(devicetype, jtagDeviceName.c_str(), device_name, apply_nsrst);
            break;

        case Emulator::JTAGICE2:
        case Emulator::DRAGON:
            theJtagICE = std::make_unique<Jtag2>(devicetype, jtagDeviceName.c_str(), device_name, proto,
                                                 apply_nsrst, is_xmega);
            break;

        case Emulator::JTAGICE3:
        case Emulator::EDBG:
            theJtagICE = std::make_unique<Jtag3>(devicetype, jtagDeviceName.c_str(), device_name,
                                                 apply_nsrst, is_xmega, proto);
            break;
        }

        // Set Daisy-chain variables
        theJtagICE->dchain = [&] {
            DaisyChainInfo daisy_chain_info{};
            if (vm.count("daisy-chain")) {
                unsigned int units_before = 0;
                unsigned int units_after = 0;
                unsigned int bits_before = 0;
                unsigned int bits_after = 0;

                const char *daisy_chain = vm["daisy_chain"].as<std::string>().c_str();
                if (sscanf(daisy_chain, "%u,%u,%u,%u", &units_before, &units_after, &bits_before,
                           &bits_after) != 4)
                    exit(1);

                daisy_chain_info = DaisyChainInfo{
                    static_cast<uchar>(units_before), static_cast<uchar>(units_after),
                    static_cast<uchar>(bits_before), static_cast<uchar>(bits_after)};
                if (!daisy_chain_info.IsValid()) {
                    fprintf(stderr, "daisy-chain parameters out of range"
                                    " (max. 32 bits before/after)\n");
                    exit(1);
                }
            }
            return daisy_chain_info;
        }();

        // Tell which events to ignore.
        theJtagICE->parseEvents(vm["event"].as<std::string>());

        // Init JTAG box.
        theJtagICE->initJtagBox();

        if( vm.count("erase")) {
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

        if( vm.count("read-fuses")) {
            theJtagICE->jtagReadFuses();
        }

        if( vm.count("read-lockbits")) {
            theJtagICE->jtagReadLockBits();
        }

        if( vm.count("write-fuses")) {
            const char* fuses = vm["write-fuses"].as<std::string>().c_str();
            theJtagICE->jtagWriteFuses(fuses);
        }

        // Init JTAG debugger for initial use.
        //   - If we're attaching to a running target, we cannot do this.
        //   - If we're running as a standalone programmer, we don't want
        //     this.
        if (gdbServerMode && !vm.count("capture")) {
            const unsigned long jtagBitrate = parseJtagBitrate(vm["jtag-bitrate"].as<std::string>().c_str());
            theJtagICE->initJtagOnChipDebugging(jtagBitrate);
        }

        // Write fuses after all programming parts have completed.
        if( vm.count("write-lockbits")) {
            const char* lockBits = vm["write-lockbits"].as<std::string>().c_str();
            theJtagICE->jtagWriteLockBits(lockBits);
        }

        // Quit & resume mote for operations that don't interact with gdb.
        if (!gdbServerMode)
            theJtagICE->resumeProgram();
        else {
            sockaddr_in name {};
            initSocketAddress(&name, hostName, hostPortNumber);
            int sock = makeSocket(&name);
            statusOut("Waiting for connection on port %hu.\n", hostPortNumber);
            if (listen(sock, 1) < 0)
                throw jtag_exception();

            if( vm.count("detach")) {
                int child = fork();

                if (child < 0) {
                    fprintf(stderr, "Failed to fork");
                    throw jtag_exception();
                }
                if (child != 0)
                    _exit(0);
                else if (setsid() < 0) {
                    fprintf(stderr, "setsid failed - weird bug");
                    throw jtag_exception();
                }
            }

            // Connection request on original socket.
            sockaddr_in clientname {};
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
