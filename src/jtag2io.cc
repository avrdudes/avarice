/*
 *	avarice - The "avarice" program.
 *	Copyright (C) 2001 Scott Finneran
 *      Copyright (C) 2002, 2003, 2004 Intel Corporation
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
 * This file implements the basic IO handling for the mkII protocol.
 *
 * $Id$
 */


#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "avarice.h"
#include "crc16.h"
#include "jtag.h"
#include "jtag2.h"
#include "jtag2_defs.h"

jtag2::~jtag2(void)
{
    // Terminate connection to JTAG box.
    if (!signedIn)
	return;

    // Do not use doSimpleJtagCommand() here as it aborts avarice on
    // failure; in case CMND_RESTORE_TARGET fails, we'd like to try
    // the sign-off command anyway.

    uchar *response, rstcmd = CMND_RESTORE_TARGET;
    int responseSize;
    (void)doJtagCommand(&rstcmd, 1, response, responseSize);
    delete [] response;
    doSimpleJtagCommand(CMND_SIGN_OFF);
    signedIn = false;
}


/*
 * Send one frame.  Adds the required preamble and CRC, and ensures
 * the frame could be written correctly.
 */
void jtag2::sendFrame(uchar *command, int commandSize)
{
    unsigned char *buf = new unsigned char[commandSize + 10];
    check(buf != NULL, "Out of memory");

    buf[0] = MESSAGE_START;
    u16_to_b2(buf + 1, command_sequence);
    u32_to_b4(buf + 3, commandSize);
    buf[7] = TOKEN;
    memcpy(buf + 8, command, commandSize);

    crcappend(buf, commandSize + 8);

    int count = safewrite(buf, commandSize + 10);

    delete [] buf;

    if (count < 0)
      jtagCheck(count);
    else // this shouldn't happen
      check(count == commandSize + 10, JTAG_CAUSE);
}

/*
 * Receive one frame, return it in &msg.  Received sequence number is
 * returned in &seqno.  Any valid frame will be returned, regardless
 * whether it matches the expected sequence number, including event
 * notification frames (seqno == 0xffff).
 *
 * Caller must eventually free the buffer.
 */
int jtag2::recvFrame(unsigned char *&msg, unsigned short &seqno)
{
    enum states {
	sSTART, sSEQNUM1, sSEQNUM2, sSIZE1, sSIZE2, sSIZE3, sSIZE4,
	sTOKEN, sDATA, sCSUM1, sCSUM2, sDONE
    }  state = sSTART;
    int msglen = 0, l = 0;
    int headeridx = 0;
    bool ignorpkt = false;
    int rv;
    unsigned char c, *buf = NULL, header[8];
    unsigned short r_seqno = 0;
    unsigned short checksum = 0;

    msg = NULL;

    while (state != sDONE) {
	if (state == sDATA) {
	    debugOut("sDATA: reading %d bytes\n", msglen);
	    rv = 0;
	    if (ignorpkt) {
		/* skip packet's contents */
		for(l = 0; l < msglen; l++)
		    rv += timeout_read(&c, 1, JTAG_RESPONSE_TIMEOUT);
	    } else {
		rv += timeout_read(buf + 8, msglen, JTAG_RESPONSE_TIMEOUT);
	    }
	    if (rv == 0)
		/* timeout */
		break;
	} else {
	    rv = timeout_read(&c, 1, JTAG_RESPONSE_TIMEOUT);
	    debugOut("recv: 0x%02x\n", c);
	    if (rv == 0)
		/* timeout */
		break;
	}
	checksum ^= c;

	if (state < sDATA)
	    header[headeridx++] = c;

	switch (state) {
	case sSTART:
	    if (c == MESSAGE_START) {
		state = sSEQNUM1;
	    } else {
		headeridx = 0;
	    }
	    break;
	case sSEQNUM1:
	    r_seqno = c;
	    state = sSEQNUM2;
	    break;
	case sSEQNUM2:
	    r_seqno |= ((unsigned)c << 8);
	    state = sSIZE1;
	    break;
	case sSIZE1:
	    state = sSIZE2;
	    goto domsglen;
	case sSIZE2:
	    state = sSIZE3;
	    goto domsglen;
	case sSIZE3:
	    state = sSIZE4;
	    goto domsglen;
	case sSIZE4:
	    state = sTOKEN;
	  domsglen:
	    msglen >>= 8;
	    msglen |= ((unsigned)c << 24);
	    break;
	case sTOKEN:
	    if (c == TOKEN) {
		state = sDATA;
		if (msglen > MAX_MESSAGE) {
		    printf("msglen %lu exceeds max message size %u, ignoring message\n",
			   msglen, MAX_MESSAGE);
		    state = sSTART;
		    headeridx = 0;
		} else {
		    buf = new unsigned char[msglen + 10];
		    check(buf != NULL, "Out of memory");

		    memcpy(buf, header, 8);
		}
	    } else {
		state = sSTART;
		headeridx = 0;
	    }
	    break;
	case sDATA:
	    /* The entire payload has been read above. */
	    l = msglen + 8;
	    state = sCSUM1;
	    break;
	case sCSUM1:
	    buf[l++] = c;
	    state = sCSUM2;
	    break;
	case sCSUM2:
	    buf[l++] = c;
	    if (crcverify(buf, msglen + 10)) {
		debugOut("CRC OK");
		state = sDONE;
	    } else {
		debugOut("checksum error");
		delete [] buf;
		return -1;
	    }
	    break;
	default:
	    debugOut("unknown state");
	    delete [] buf;
	    return -1;
	}
    }

    seqno = r_seqno;
    msg = buf;

    return msglen;
}

/*
 * Try receiving frames, until we get the reply we are expecting.
 * Caller must delete[] the msg after processing it.
 */
int jtag2::recv(uchar *&msg)
{
    unsigned short r_seqno;
    int rv;

    for (;;) {
	if ((rv = recvFrame(msg, r_seqno)) <= 0)
	    return rv;
	debugOut("\nGot message seqno %d (command_sequence == %d)\n",
		 r_seqno, command_sequence);
	if (r_seqno == command_sequence) {
	    if (++command_sequence == 0xffff)
		command_sequence = 0;
	    /*
	     * We move the payload to the beginning of the buffer, to make
	     * the job easier for the caller.  We have to return the
	     * original pointer though, as the caller must free() it.
	     */
	    memmove(msg, msg + 8, rv);
	    return rv;
	}
	if (r_seqno == 0xffff) {
	    debugOut("\ngot asynchronous event: 0x%02x\n",
		     msg[8]);
	    // XXX should we queue that event up somewhere?
	    // How to process it?  Register event handlers
	    // for interesting events?
	    if (msg[8] == EVT_BREAK)
	      breakpointHit = true;
	} else {
	    debugOut("\ngot wrong sequence number, %u != %u\n",
		     r_seqno, command_sequence);
	}
	delete [] msg;
    }
    return 0;
}

/** Send a command to the jtag, and check result.

    Increase *tries, abort if reaches MAX_JTAG_COMM_ATTEMPS

    Reads first response byte. If no response is received within
    JTAG_RESPONSE_TIMEOUT, returns false. If response is
    positive returns true, otherwise returns false.

    If response is positive, message (including response code) is
    returned in &msg, caller must delete [] it.  The message size is
    returned in &msgsize.
**/

bool jtag2::sendJtagCommand(uchar *command, int commandSize, int &tries,
			    uchar *&msg, int &msgsize, bool verify)
{
    check(tries++ < MAX_JTAG_COMM_ATTEMPS,
	      "JTAG ICE: Cannot synchronise");

    debugOut("\ncommand[0x%02x, %d]: ", command[0], tries);

    for (int i = 0; i < commandSize; i++)
	debugOut("%.2X ", command[i]);

    debugOut("\n");

    sendFrame(command, commandSize);

    msgsize = recv(msg);
    if (verify)
	jtagCheck(msgsize - 1);
    else if (msgsize < 1)
	return false;

    debugOut("response: ");
    for (int i = 0; i < msgsize; i++)
    {
	debugOut("%.2X ", msg[i]);
    }
    debugOut("\n");

    unsigned char c = msg[0];

    if (c >= RSP_OK && c < RSP_FAILED)
	return true;

    return false;
}


bool jtag2::doJtagCommand(uchar *command, int  commandSize,
			  uchar *&response, int  &responseSize,
			  bool retryOnTimeout)
{
    int tryCount = 0;

    // Send command until we get an OK response
    for (;;)
    {
	if (sendJtagCommand(command, commandSize, tryCount, response, responseSize, false))
	    return true;

	if (responseSize > 0 || !retryOnTimeout)
	    // Got a negative response in time; there is no point
	    // in retrying the command.
	    return false;
    }
}

void jtag2::doSimpleJtagCommand(uchar command)
{
    int tryCount = 0, dummy;
    uchar *replydummy;

    // Send command until we get an OK response
    for (;;)
    {
	if (sendJtagCommand(&command, 1, tryCount, replydummy, dummy)) {
	    check(replydummy != NULL, JTAG_CAUSE);
	    check(dummy == 1 && replydummy[0] == RSP_OK,
		  "Unexpected response in doSimpleJtagCommand");
	    delete [] replydummy;
	    return;
	}
	// See whether it timed out only.  If so, retry.
	jtagCheck(dummy <= 0);
    }
}

/** Set PC and JTAG ICE bitrate to BIT_RATE_xxx specified by 'newBitRate' **/
void jtag2::changeBitRate(int newBitRate)
{
    uchar jtagrate;

    switch (newBitRate) {
    case 9600:
	jtagrate = PAR_BAUD_9600;
	break;
    case 19200:
	jtagrate = PAR_BAUD_19200;
	break;
    case 38400:
	jtagrate = PAR_BAUD_38400;
	break;
    case 57600:
	jtagrate = PAR_BAUD_57600;
	break;
    case 115200:
	jtagrate = PAR_BAUD_115200;
	break;
    }
    setJtagParameter(PAR_BAUD_RATE, &jtagrate, 1);
    changeLocalBitRate(newBitRate);
}

/** Set the JTAG ICE device descriptor data for specified device type **/
void jtag2::setDeviceDescriptor(jtag_device_def_type *dev)
{
    uchar *response;
    uchar *command = (uchar *)(&dev->dev_desc2);
    int respSize;

    check(doJtagCommand(command, devdescrlen, response, respSize),
	  "JTAG ICE: Failed to set device description");

    delete [] response;
}

/** Attempt to synchronise with JTAG at specified bitrate **/
bool jtag2::synchroniseAt(int bitrate)
{
    debugOut("Attempting synchronisation at bitrate %d\n", bitrate);

    changeLocalBitRate(bitrate);

    int tries = 0;
    uchar *signonmsg, signoncmd = CMND_GET_SIGN_ON;
    int msgsize;

    while (tries < MAX_JTAG_SYNC_ATTEMPS)
    {
	if (sendJtagCommand(&signoncmd, 1, tries, signonmsg, msgsize, false)) {
	    check(signonmsg[0] == RSP_SIGN_ON && msgsize > 17,
		  "Unexpected response to sign-on command");
	    signonmsg[msgsize - 1] = '\0';
	    statusOut("Found a device: %s\n", signonmsg + 16);
	    statusOut("Serial number:  %02x:%02x:%02x:%02x:%02x:%02x\n",
		   signonmsg[10], signonmsg[11], signonmsg[12],
		   signonmsg[13], signonmsg[14], signonmsg[15]);
	    debugOut("JTAG ICE mkII sign-on message:\n");
	    debugOut("Communications protocol version: %u\n",
		     (unsigned)signonmsg[1]);
	    debugOut("M_MCU:\n");
	    debugOut("  boot-loader FW version:        %u\n",
		     (unsigned)signonmsg[2]);
	    debugOut("  firmware version:              %u.%02u\n",
		     (unsigned)signonmsg[4], (unsigned)signonmsg[3]);
	    debugOut("  hardware version:              %u\n",
		     (unsigned)signonmsg[5]);
	    debugOut("S_MCU:\n");
	    debugOut("  boot-loader FW version:        %u\n",
		     (unsigned)signonmsg[6]);
	    debugOut("  firmware version:              %u.%02u\n",
		     (unsigned)signonmsg[8], (unsigned)signonmsg[7]);
	    debugOut("  hardware version:              %u\n",
		     (unsigned)signonmsg[9]);

	    unsigned short fwver = ((unsigned)signonmsg[8] << 8) | (unsigned)signonmsg[7];

	    // Check the S_MCU firmware version to know which format
	    // of the device descriptor to send.
#define FWVER(maj, min) ((maj << 8) | (min))
	    if (fwver < FWVER(3, 16)) {
		devdescrlen -= 2;
		fprintf(stderr,
			"Warning: S_MCU firmware version might be "
			"too old to work correctly\n ");
	    } else if (fwver < FWVER(4, 0)) {
		devdescrlen -= 2;
	    }
#undef FWVER
	    delete [] signonmsg;
	    return true;
	}
    }
    return false;
}

/** Attempt to synchronise with JTAG ICE at all possible bit rates **/
void jtag2::startJtagLink(void)
{
    static int bitrates[] =
    { 19200, 115200, 57600, 38400, 9600 };

    for (unsigned int i = 0; i < sizeof bitrates / sizeof *bitrates; i++)
	if (synchroniseAt(bitrates[i])) {
	    uchar val = EMULATOR_MODE_JTAG;
	    setJtagParameter(PAR_EMULATOR_MODE, &val, 1);
	    signedIn = true;

	    return;
	}

    check(false, "Failed to synchronise with the JTAG ICE (is it connected and powered?)");
}

/** Device automatic configuration
 Determines the device being controlled by the JTAG ICE and configures
 the system accordingly.

 May be overridden by command line parameter.

*/
void jtag2::deviceAutoConfig(void)
{
    unsigned int device_id;
    uchar *resp;
    int respSize;
    int i;
    jtag_device_def_type *pDevice = deviceDefinitions;

    // Auto config
    debugOut("Automatic device detection: ");

    /* Read in the JTAG device ID to determine device */
    getJtagParameter(PAR_JTAGID, resp, respSize);
    jtagCheck(respSize == 4);
    device_id = resp[1] | (resp[2] << 8) | (resp[3] << 16) | resp[4] << 24;
    delete [] resp;

    debugOut("JTAG id = 0x%0X : Ver = 0x%0x : Device = 0x%0x : Manuf = 0x%0x\n",
             device_id,
             (device_id & 0xF0000000) >> 28,
             (device_id & 0x0FFFF000) >> 12,
             (device_id & 0x00000FFE) >> 1);

    device_id = (device_id & 0x0FFFF000) >> 12;
    statusOut("Reported JTAG device ID: 0x%0X\n", device_id);

    if (device_name == 0)
    {
        while (pDevice->name)
        {
            if (pDevice->device_id == device_id)
                break;

            pDevice++;
        }
        check(pDevice->name,
              "No configuration available for device ID: %0x\n",
              device_id);
    }
    else
    {
        debugOut("Looking for device: %s\n", device_name);

        while (pDevice->name)
        {
            if (strcasecmp(pDevice->name, device_name) == 0)
                break;

            pDevice++;
        }
        check(pDevice->name,
              "No configuration available for Device: %s\n",
              device_name);
    }

    if (device_name)
    {
        if (device_id != pDevice->device_id)
        {
            statusOut("Configured for device ID: 0x%0X %s -- FORCED with %s\n",
                      pDevice->device_id, pDevice->name, device_name);
        }
        else
        {
            statusOut("Configured for device ID: 0x%0X %s -- Matched with "
                      "%s\n", pDevice->device_id, pDevice->name, device_name);
        }
    }
    else
    {
        statusOut("Configured for device ID: 0x%0X %s\n",
                  pDevice->device_id, pDevice->name);
    }

    device_name = (char*)pDevice->name;

    global_p_device_def = pDevice;

    setDeviceDescriptor(pDevice);
}


void jtag2::initJtagBox(void)
{
    statusOut("JTAG config starting.\n");

    startJtagLink();
    changeBitRate(115200);

    interruptProgram();

    deviceAutoConfig();

    // Clear out the breakpoints.
    deleteAllBreakpoints();

    statusOut("JTAG config complete.\n");
}


void jtag2::initJtagOnChipDebugging(unsigned long bitrate)
{
    statusOut("Preparing the target device for On Chip Debugging.\n");

    uchar br;
    if (bitrate >= 6400000)
      br = 0;
    else if (bitrate >= 2800000)
      br = 1;
    else if (bitrate >= 20900)
      br = (unsigned char)(5.35e6 / (double)bitrate);
    else
      br = 255;
    // Set JTAG bitrate
    setJtagParameter(PAR_OCD_JTAG_CLK, &br, 1);

    // When attaching we can't change fuse bits, etc, as
    // enabling+disabling programming resets the processor
    enableProgramming();

    // Ensure that all lock bits are "unlocked" ie all 1's
    uchar *lockBits = 0;
    lockBits = jtagRead(LOCK_SPACE_ADDR_OFFSET + 0, 1);

    if (*lockBits != LOCK_BITS_ALL_UNLOCKED)
    {
        lockBits[0] = LOCK_BITS_ALL_UNLOCKED;
        jtagWrite(LOCK_SPACE_ADDR_OFFSET + 0, 1, lockBits);
    }

    statusOut("\nDisabling lock bits:\n");
    statusOut("  LockBits -> 0x%02x\n", *lockBits);

    if (lockBits)
    {
        delete [] lockBits;
        lockBits = 0;
    }

    // Ensure on-chip debug enable fuse is enabled ie '0'
    uchar *fuseBits = 0;
    statusOut("\nEnabling on-chip debugging:\n");
    fuseBits = jtagRead(FUSE_SPACE_ADDR_OFFSET + 0, 3);

    if ((fuseBits[1] & FUSE_OCDEN) == FUSE_OCDEN)
    {
        fuseBits[1] = fuseBits[1] & ~FUSE_OCDEN; // clear bit
        jtagWrite(FUSE_SPACE_ADDR_OFFSET + 1, 1, &fuseBits[1]);
    }

    jtagDisplayFuses(fuseBits);

    if (fuseBits)
    {
        delete [] fuseBits;
        fuseBits = 0;
    }

    disableProgramming();

    resetProgram();
    uchar timers = 0;		// stopped
    setJtagParameter(PAR_TIMERS_RUNNING, &timers, 1);
    resetProgram();
}
