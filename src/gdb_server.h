/*
 *	avarice - The "avarice" program.
 *	Copyright (C) 2001 Scott Finneran
 *      Copyright (C) 2002 Intel Corporation
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
 * Interface definition for the include/remote.c file.
 */

#ifndef INCLUDE_REMOTE_H
#define INCLUDE_REMOTE_H

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include "jtag.h"

class GdbServer : public Server {
  public:
    /** true if interrupts should be stepped over when stepping */
    GdbServer(std::string_view const &listen, bool ignore_interrupts);

    void listen();

    void accept();

    /** GDB remote protocol interpreter */
    void handle();
    void waitForOutput();

    void waitForGdbInput();
    /** Return single char read from gdb. Abort in case of problem,
        exit cleanly if EOF detected on gdbFileDescriptor. **/
    unsigned char getDebugChar() override;

    void putDebugChar(char c);

    /** printf 'fmt, ...' to gdb **/
    void Out(const char *fmt, ...) override;
    void vOut(const char *fmt, va_list args);

    char *getpacket(int &len);
    void putpacket(const char *buffer);
    bool singleStep();

    bool handleInterrupt();

    int GetHandle() override { return m_fd; }

    int m_listen_port = 0;
    sockaddr_in m_name{};
    int m_sock;
    bool m_ignoreInterrupts;
    int m_fd;
};

#endif /* INCLUDE_REMOTE_H */
