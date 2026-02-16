/*
 *	avarice - The "avarice" program.
 *	Copyright (C) 2001 Scott Finneran
 *      Copyright (C) 2002, 2003, 2004 Intel Corporation
 *	Copyright (C) 2005, 2006, 2007 Joerg Wunsch
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
#include <signal.h>
#include <string.h>
#include <errno.h>

#include "avarice.h"
#include "crc16.h"
#include "jtag.h"
#include "jtag2.h"
#include "jtag2_defs.h"

jtag_io_exception::jtag_io_exception(unsigned int code)
{
    static char buffer[50];
    response_code = code;

    switch (code)
    {
        case RSP_DEBUGWIRE_SYNC_FAILED:
            reason = "DEBUGWIRE SYNC FAILED"; break;
        case RSP_FAILED:
            reason = "FAILED"; break;
        case RSP_GET_BREAK:
            reason = "GET BREAK"; break;
        case RSP_ILLEGAL_BREAKPOINT:
            reason = "ILLEGAL BREAKPOINT"; break;
        case RSP_ILLEGAL_COMMAND:
            reason = "ILLEGAL COMMAND"; break;
        case RSP_ILLEGAL_EMULATOR_MODE:
            reason = "ILLEGAL EMULATOR MODE"; break;
        case RSP_ILLEGAL_JTAG_ID:
            reason = "ILLEGAL JTAG ID"; break;
        case RSP_ILLEGAL_MCU_STATE:
            reason = "ILLEGAL MCU STATE"; break;
        case RSP_ILLEGAL_MEMORY_TYPE:
            reason = "ILLEGAL MEMORY TYPE"; break;
        case RSP_ILLEGAL_MEMORY_RANGE:
            reason = "ILLEGAL MEMORY RANGE"; break;
        case RSP_ILLEGAL_PARAMETER:
            reason = "ILLEGAL PARAMETER"; break;
        case RSP_ILLEGAL_POWER_STATE:
            reason = "ILLEGAL POWER STATE"; break;
        case RSP_ILLEGAL_VALUE:
            reason = "ILLEGAL VALUE"; break;
        case RSP_NO_TARGET_POWER:
            reason = "NO TARGET POWER"; break;
        case RSP_SET_N_PARAMETERS:
            reason = "SET N PARAMETERS"; break;
        default:
            snprintf(buffer, sizeof buffer, "Unknown response code 0x%0x", code);
            reason = buffer;
    }
}


jtag2::~jtag2(void)
{
    // Terminate connection to JTAG box.
    if (signedIn)
      {
	  try
	  {
	      if (debug_active)
		  doSimpleJtagCommand(CMND_RESTORE_TARGET);
	  }
	  catch (jtag_exception&)
	  {
	      // just proceed with the sign-off
	  }
	  doSimpleJtagCommand(CMND_SIGN_OFF);
	  signedIn = false;
      }
}


/*
 * Send one frame.  Adds the required preamble and CRC, and ensures
 * the frame could be written correctly.
 */
void jtag2::sendFrame(uchar *command, int commandSize)
{
    unsigned char *buf = new unsigned char[commandSize + 10];

    buf[0] = MESSAGE_START;
    u16_to_b2(buf + 1, command_sequence);
    u32_to_b4(buf + 3, commandSize);
    buf[7] = TOKEN;
    memcpy(buf + 8, command, commandSize);

    crcappend(buf, commandSize + 8);

    int count = safewrite(buf, commandSize + 10);

    delete [] buf;

    if (count < 0)
        throw jtag_exception();
    else if (count != commandSize + 10)
        // this shouldn't happen
        throw jtag_exception("Invalid write size");
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
    unsigned int msglen = 0, l = 0;
    unsigned int headeridx = 0;
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
		for(l = 0; l < msglen; l++) {
		    rv += timeout_read(&c, 1, JTAG_RESPONSE_TIMEOUT);
		    debugOut("ign: 0x%02x\n", c);
		}
	    } else {
		rv += timeout_read(buf + 8, msglen, JTAG_RESPONSE_TIMEOUT);
		debugOut("read: ");
		for (l = 0; l < msglen; l++) {
		    debugOut(" %02x", buf[l + 8]);
		}
		debugOut("\n");
	    }
	    if (rv == 0)
		/* timeout */
		break;
	} else {
	    rv = timeout_read(&c, 1, JTAG_RESPONSE_TIMEOUT);
	    if (rv == 0) {
		/* timeout */
		debugOut("recv: timeout\n");
		break;
	    }
	    debugOut("recv: 0x%02x\n", c);
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
		    printf("msglen %u exceeds max message size %u, ignoring message\n",
			   msglen, MAX_MESSAGE);
		    state = sSTART;
		    headeridx = 0;
		} else {
		    buf = new unsigned char[msglen + 10];
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

    return (int)msglen;
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
	    // For now, the only place that cares is jtagContinue
	    // and it just calls recvFrame and handles events directly. 
	} else {
	    debugOut("\ngot wrong sequence number, %u != %u\n",
		     r_seqno, command_sequence);
	}
	delete [] msg;
    }
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
    if (tries++ >= MAX_JTAG_COMM_ATTEMPS)
        throw jtag_exception("JTAG communication failed");

    debugOut("\ncommand[0x%02x, %d]: ", command[0], tries);

    for (int i = 0; i < commandSize; i++)
	debugOut("%.2X ", command[i]);

    debugOut("\n");

    sendFrame(command, commandSize);

    msgsize = recv(msg);
    if (verify && msgsize == 0)
        throw jtag_exception("no response received");
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


void jtag2::doJtagCommand(uchar *command, int  commandSize,
			  uchar *&response, int  &responseSize,
			  bool retryOnTimeout)
{
    int sizeseen = 0;
    uchar code = 0;

    for (int tryCount = 0; tryCount < 8; tryCount++)
    {
	if (sendJtagCommand(command, commandSize, tryCount, response, responseSize, false))
	    return;

	if (!retryOnTimeout)
	{
	    if (responseSize == 0)
		throw jtag_timeout_exception();
	    else
		throw jtag_io_exception(response[0]);
	}

	if (responseSize > 0 && response[0] > RSP_FAILED)
	    // no point in retrying failures other than FAILED
	    throw jtag_io_exception(response[0]);

	if (responseSize > 0)
	{
	    sizeseen = responseSize;
	    code = response[0];
	}

#ifdef HAVE_LIBUSB
	if (tryCount == 4 && responseSize == 0 && is_usb)
	  {
	    /* signal the USB daemon to reset the EPs */
	    debugOut("Resetting EPs...\n");
	    resetUSB();
	  }
#endif
    }
    if (sizeseen > 0)
	throw jtag_io_exception(code);
    else
	throw jtag_timeout_exception();
}

void jtag2::doSimpleJtagCommand(uchar command)
{
    int tryCount = 0, dummy;
    uchar *replydummy;

    // Send command until we get an OK response
    for (;;)
    {
	if (sendJtagCommand(&command, 1, tryCount, replydummy, dummy, false)) {
	    if (replydummy == NULL)
		throw jtag_io_exception();
	    if (dummy != 1)
		throw jtag_exception("Unexpected response size in doSimpleJtagCommand");
	    if (replydummy[0] != RSP_OK)
		throw jtag_io_exception(replydummy[0]);
	    delete [] replydummy;
	    return;
	}
    }
}

/** Set PC and JTAG ICE bitrate to BIT_RATE_xxx specified by 'newBitRate' **/
void jtag2::changeBitRate(int newBitRate)
{
    // Don't try to change the speed of an USB connection.
    // For the AVR Dragon, that would even result in the parameter
    // change below being rejected.
    if (is_usb)
        return;

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
    uchar *command;
    int respSize;

    if (is_xmega && has_full_xmega_support)
	command = (uchar *)&dev->dev_desc3;
    else
	command = (uchar *)&dev->dev_desc2;

    try
    {
        doJtagCommand(command, devdescrlen, response, respSize);
    }
    catch (jtag_exception& e)
    {
        fprintf(stderr, "JTAG ICE: Failed to set device description: %s\n",
                e.what());
        throw;
    }
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
            if (signonmsg[0] != RSP_SIGN_ON || msgsize <= 17)
                throw jtag_exception("Unexpected response to sign-on command");
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

	    // The AVR Dragon always uses the full device descriptor.
	    if (emu_type == EMULATOR_JTAGICE)
	    {
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
	    }

	    has_full_xmega_support = (unsigned)signonmsg[8] >= 7;
	    if (is_xmega)
	    {
		if (has_full_xmega_support)
		{
		    devdescrlen = sizeof(xmega_device_desc_type);
		}
		else
		{
		    fprintf(stderr,
			    "Warning, S_MCU firmware version (%u.%02u) too old to work "
			    "correctly for Xmega devices, >= 7.x required\n",
			    (unsigned)signonmsg[8], (unsigned)signonmsg[7]);
                    softbp_only = true;
		}
	    }

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
	    uchar val;

	    signedIn = true;

	    if (proto == PROTO_JTAG && apply_nSRST) {
		val = 0x01;
		setJtagParameter(PAR_EXTERNAL_RESET, &val, 1);
	    }

	    const char *protoName = "unknown";
	    switch (proto)
	    {
		case PROTO_JTAG:
		    if (is_xmega)
			val = EMULATOR_MODE_JTAG_XMEGA;
		    else
			val = EMULATOR_MODE_JTAG;
		    protoName = "JTAG";
		    break;

		case PROTO_DW:
		    val = EMULATOR_MODE_DEBUGWIRE;
		    protoName = "debugWIRE";
                    softbp_only = true;
		    break;

		case PROTO_PDI:
		    val = EMULATOR_MODE_PDI;
		    protoName = "PDI";
		    break;
		default:
			break;
	    }
	    try
	    {
		setJtagParameter(PAR_EMULATOR_MODE, &val, 1);
		debug_active = true;
	    }
	    catch (jtag_io_exception&)
	    {
		fprintf(stderr,
			"Failed to activate %s debugging protocol\n",
			protoName);
		throw;
	    }

	    return;
	}

    throw jtag_exception("Failed to synchronise with the JTAG ICE (is it connected and powered?)");
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
    jtag_device_def_type *pDevice = deviceDefinitions;

    // Auto config
    debugOut("Automatic device detection: ");

    /* Set daisy chain information */
    configDaisyChain();

    /* Read in the JTAG device ID to determine device */
    if (proto == PROTO_DW)
    {
	getJtagParameter(PAR_TARGET_SIGNATURE, resp, respSize);
	if (respSize < 3)
            throw jtag_exception("Invalid response size to PAR_TARGET_SIGNATURE");
	device_id = resp[1] | (resp[2] << 8);
	delete [] resp;

	statusOut("Reported debugWire device ID: 0x%0X\n", device_id);
    }
    else if (proto == PROTO_PDI)
    {
	resp = jtagRead(SIG_SPACE_ADDR_OFFSET, 3);
	device_id = resp[2] | (resp[1] << 8);
	delete [] resp;

	statusOut("Reported PDI device ID: 0x%0X\n", device_id);
    }
    else
    {
	getJtagParameter(PAR_JTAGID, resp, respSize);
	if (respSize < 5)
            throw jtag_exception("Invalid response size to PAR_TARGET_SIGNATURE");
	device_id = resp[1] | (resp[2] << 8) | (resp[3] << 16) | resp[4] << 24;
	delete [] resp;

	debugOut("JTAG id = 0x%0X : Ver = 0x%0x : Device = 0x%0x : Manuf = 0x%0x\n",
		 device_id,
		 (device_id & 0xF0000000) >> 28,
		 (device_id & 0x0FFFF000) >> 12,
		 (device_id & 0x00000FFE) >> 1);

	device_id = (device_id & 0x0FFFF000) >> 12;
	statusOut("Reported JTAG device ID: 0x%0X\n", device_id);
    }

    if (device_name == 0)
    {
        while (pDevice->name)
        {
            if (pDevice->device_id == device_id)
                break;

            pDevice++;
        }
        if (pDevice->name == 0)
        {
            unknownDevice(device_id);
            throw jtag_exception();
        }
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
        if (pDevice->name == 0)
        {
            unknownDevice(device_id, false);
            throw jtag_exception();
        }
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

    deviceDef = pDevice;

    setDeviceDescriptor(pDevice);
}


void jtag2::initJtagBox(void)
{
    statusOut("JTAG config starting.\n");

    if (device_name != 0)
    {
        jtag_device_def_type *pDevice = deviceDefinitions;

        while (pDevice->name)
        {
            if (strcasecmp(pDevice->name, device_name) == 0)
                break;

            pDevice++;
        }

        if (pDevice->name != 0)
        {
            // If a device name has been specified on the command-line,
            // this overrides the is_xmega setting.
            is_xmega = pDevice->is_xmega;
        }
    }

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

    if (proto == PROTO_JTAG)
    {
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
    }

    // Ensure on-chip debug enable fuse is enabled ie '0'

    // The enableProgramming()/disableProgramming() pair might seem to
    // be not needed (as the fuse read/write operations would enforce
    // going to programming mode anyway), but for devices that don't
    // feature an OCDEN fuse (i.e., Xmega devices),
    // jtagActivateOcdenFuse() bails out immediately.  At least with
    // firmware 7.13, the ICE seems to become totally upset then when
    // debugging an Xmega device without having went through a
    // programming-mode cycle before.  Upon a reset command, it
    // confirms the reset, but the target happily proceeds in RUNNING
    // state.
    enableProgramming();
    jtagActivateOcdenFuse();
    disableProgramming();

    resetProgram();
    uchar timers = 0;		// stopped
    if (!is_xmega)
        setJtagParameter(PAR_TIMERS_RUNNING, &timers, 1);
}

void jtag2::configDaisyChain(void)
{
    unsigned char buf[4];

    if ((dchain.units_before > 0) ||
	(dchain.units_after > 0) ||
	(dchain.bits_before > 0) ||
	(dchain.bits_after > 0) ){
	buf[0] = dchain.units_before;
	buf[1] = dchain.units_after;
	buf[2] = dchain.bits_before;
	buf[3] = dchain.bits_after;
	setJtagParameter(PAR_DAISY_CHAIN_INFO, buf, 4);
    }
}
