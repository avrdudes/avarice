
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

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

#include "avarice.h"
#include "gdb_server.h"
#include "jtag.h"
#include "jtag1.h"
#include "jtag2.h"
#include "jtag3.h"

#include <boost/program_options.hpp>
namespace po = boost::program_options;

static unsigned long parseJtagBitrate(std::string_view const &val) {
    if (val.empty()) {
        fprintf(stderr, "invalid number in JTAG bit rate");
        throw jtag_exception();
    }

    char *endptr, c;
    const char* val_ptr = val.data();
    unsigned long v = strtoul(val_ptr, &endptr, 10);
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

        po::options_description desc("avarice [Options] [[HOST_NAME]:PORT]\n\nOptions");
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
                " using debugWire protocol rather than JTAG.")
            ( "read-lockbits,l", "Read lock bits")
            ( "reset-srst,R", "External reset through nSRST signal.")
            ("jtag,j", po::value<std::string>(), "Port attached to JTAG box")
            ( "part,P", po::value<std::string>(), "Target device name (e.g. atmega16)")
            ("jtag-bitrate,B", po::value<std::string>()->default_value("250 kHz"),
                "Set the bitrate that the JTAG box communicates with the avr target device."
                " This must be less than 1/4 of the frequency of the target. Valid"
                " values are 1000/500/250/125 kHz (mkI), or 22 through 6400 kHz (mkII).")
            ("daisy-chain,c", po::value<std::string>(), "<ub,ua,bb,ba> Daisy chain settings:"
                " <units before, units after,bits before, bits after>")
            ("write-lockbits,L", po::value<std::string>(), "<ll> Write lock bits.")
            ("write-fuses,W", po::value<std::string>(), "<eehhll>  Write fuses bytes.")
            ("event,E", po::value<std::string>()->default_value("none,run,target_power_on,target_sleep,target_wakeup"),
                "<eventlist>  List of events that do not interrupt. JTAG ICE mkII and AVR Dragon only.")
            ("server", po::value<std::string>(), "[[HOST_NAME]:PORT]\n"
                "HOST_NAME defaults to 0.0.0.0 (listen on any interface).\n"
                ":PORT is required to put avarice into gdb server mode.");

        po::positional_options_description p;
        p.add("server", 1);
        // clang-format on

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv). options(desc).positional(p).run(), vm);
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
            exit(0);
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
            debugMode = true;
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

        const bool gdbServerMode = vm.count("server");

        // Init JTAG debugger for initial use.
        //   - If we're attaching to a running target, we cannot do this.
        //   - If we're running as a standalone programmer, we don't want
        //     this.
        if (gdbServerMode && !vm.count("capture")) {
            const auto jtagBitrate = parseJtagBitrate(vm["jtag-bitrate"].as<std::string>());
            theJtagICE->initJtagOnChipDebugging(jtagBitrate);
        }

        // Write fuses after all programming parts have completed.
        if( vm.count("write-lockbits")) {
            const char* lockBits = vm["write-lockbits"].as<std::string>().c_str();
            theJtagICE->jtagWriteLockBits(lockBits);
        }

        // Quit & resume mote for operations that don't interact with gdb.
        if (gdbServerMode) {
            const bool ignore_interrupts = vm.count("ignore-intr");
            GdbServer server(vm["server"].as<std::string>(), ignore_interrupts);
            server.listen();

            if (vm.count("detach")) {
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

            server.accept(); // blocks until a client connects

            // Now do the actual processing of GDB messages
            // We stay here until exiting because of error of EOF on the
            // gdb connection
            for (;;)
                server.handle();
        } else {
            theJtagICE->resumeProgram();
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
