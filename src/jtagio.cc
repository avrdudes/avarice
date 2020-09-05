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
 * This file contains functions for interfacing with the JTAG box.
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
#include "jtag.h"
#include "jtag1.h"

/** Send a command to the jtag, and check result.

    Increase *tries, abort if reaches MAX_JTAG_COMM_ATTEMPS

    Reads first response byte. If no response is received within
    JTAG_RESPONSE_TIMEOUT, returns false. If response byte is 
    JTAG_R_RESP_OK returns true, otherwise returns false.
**/
 
SendResult jtag1::sendJtagCommand(uchar *command, int commandSize, int *tries)
{
    if ((*tries)++ >= MAX_JTAG_COMM_ATTEMPS)
        throw jtag_exception("JTAG communication failed");

    debugOut("\ncommand[%c, %d]: ", command[0], *tries);

    for (int i = 0; i < commandSize; i++)
	debugOut("%.2X ", command[i]);

    debugOut("\n");

    // before writing, clean up any "unfinished business".
    if (tcflush(jtagBox, TCIFLUSH) < 0)
        throw jtag_exception();

    int count = safewrite(command, commandSize);
    if (count < 0)
        throw jtag_exception();
    else if (count != commandSize)
        // this shouldn't happen
        throw jtag_exception();

    // And wait for all characters to go to the JTAG box.... can't hurt!
    if (tcdrain(jtagBox) < 0)
        throw jtag_exception();

    // We should get JTAG_R_OK, but we might get JTAG_R_INFO too (we just
    // ignore it)
    for (;;)
      {
	uchar ok;
	count = timeout_read(&ok, 1, JTAG_RESPONSE_TIMEOUT);
	if (count < 0)
            throw jtag_exception();

	// timed out
	if (count == 0)
	{
	    debugOut("Timed out.\n");
	    return send_failed;
	}

	switch (ok)
	{
	case JTAG_R_OK: return send_ok;
	case JTAG_R_INFO:
	    unsigned char infobuf[2];

	    /* An info ("IDR dirty") response. Ignore it. */
	    debugOut("Info response: ");
	    count = timeout_read(infobuf, 2, JTAG_RESPONSE_TIMEOUT);
	    for (int i = 0; i < count; i++)
	    {
		debugOut("%.2X ", infobuf[i]);
	    }
	    debugOut("\n");
	    if (count != 2 || infobuf[1] != JTAG_R_OK)
		return send_failed;
	    else
		return (SendResult)(mcu_data + infobuf[0]);
	    break;
	default:
	    debugOut("Out of sync, reponse was `%02x'\n", ok);
	    return send_failed;
	}
      }
}

/** Get a 'responseSize' byte response from the JTAG ICE

    Returns NULL if no bytes received for JTAG_COMM_TIMEOUT microseconds
    Returns a dynamically allocated buffer containing the reponse (caller
    must free) otherwise
**/
uchar *jtag1::getJtagResponse(int responseSize)
{
    uchar *response;
    int numCharsRead;

    // Increase by 1 because of the zero termination.
    //
    // note: IT IS THE CALLERS RESPONSIBILITY TO delete() THIS.
    response = new uchar[responseSize + 1];
    response[responseSize] = '\0';

    numCharsRead = timeout_read(response, responseSize,
                                JTAG_RESPONSE_TIMEOUT);
    if (numCharsRead < 0)
        throw jtag_exception();

    debugOut("response: ");
    for (int i = 0; i < numCharsRead; i++)
    {
	debugOut("%.2X ", response[i]);
    }
    debugOut("\n");

    if (numCharsRead < responseSize) // timeout problem
    {
	debugOut("Timed Out (partial response)\n");
	delete [] response;
	return NULL;
    }

    return response;
}

uchar *jtag1::doJtagCommand(uchar *command, int  commandSize, int  responseSize)
{
    int tryCount = 0;

    // Send command until we get RESP_OK
    for (;;)
    {
	uchar *response;
	static uchar sync[] = { ' ' };
	static uchar stop[] = { 'S', JTAG_EOM };

	switch (sendJtagCommand(command, commandSize, &tryCount))
	{
	case send_ok:
	    response = getJtagResponse(responseSize);
	    if (response == NULL)
                throw jtag_exception();
	    return response;
	case send_failed:
	    // We're out of sync. Attempt to resync.
	    while (sendJtagCommand(sync, sizeof sync, &tryCount) != send_ok) 
		;
	    break;
	default:
	    /* "IDR dirty", aka I/O debug register dirty, aka we got some
	       data from the target processor. This seems to be
	       indefinitely repeated if we don't do anything.  Asking for
	       the sign on seems to shut it up, so we do that.  (another
	       option is to do a 'forced stop' ('F' command), but that is a
	       bit more intrusive --- it should be ok, as we currently only
	       send commands to stopped targets, but...) */
	    if (sendJtagCommand(stop, sizeof stop, &tryCount) == send_ok)
		getJtagResponse(8);
	    break;
	}
    }

}

bool jtag1::doSimpleJtagCommand(unsigned char cmd, int responseSize)
{
    uchar *response;
    uchar command[] = { cmd, JTAG_EOM };
    bool result;

    response = doJtagCommand(command, sizeof command, responseSize);
    result = responseSize == 0 || response[responseSize - 1] == JTAG_R_OK;
    
    delete [] response;

    return result;
}


/** Set PC and JTAG ICE bitrate to BIT_RATE_xxx specified by 'newBitRate' **/
void jtag1::changeBitRate(int newBitRate)
{
    uchar jtagrate;

    switch (newBitRate) {
    case 9600:
	jtagrate = BIT_RATE_9600;
	break;
    case 19200:
	jtagrate = BIT_RATE_19200;
	break;
    case 38400:
	jtagrate = BIT_RATE_38400;
	break;
    case 57600:
	jtagrate = BIT_RATE_57600;
	break;
    case 115200:
	jtagrate = BIT_RATE_115200;
	break;
    default:
        return;
    }
    setJtagParameter(JTAG_P_BITRATE, jtagrate);
    changeLocalBitRate(newBitRate);
}

/** Set the JTAG ICE device descriptor data for specified device type **/
void jtag1::setDeviceDescriptor(jtag_device_def_type *dev)
{
    uchar *response = NULL;
    uchar *command = (uchar *)(&dev->dev_desc1);

    response = doJtagCommand(command, sizeof dev->dev_desc1, 1);
    if (response[0] != JTAG_R_OK)
        throw jtag_exception ("JTAG ICE: Failed to set device description");

    delete [] response;
}

/** Check for emulator using the 'S' command *without* retries
    (used at startup to check sync only) **/
bool jtag1::checkForEmulator(void)
{
    uchar *response;
    uchar command[] = { 'S', JTAG_EOM };
    bool result;
    int tries = 0;

    if (!sendJtagCommand(command, sizeof command, &tries))
      return false;

    response = getJtagResponse(8);
    result = response && !strcmp((char *)response, "AVRNOCDA");
    
    delete [] response;

    return result;
}

/** Attempt to synchronise with JTAG at specified bitrate **/
bool jtag1::synchroniseAt(int bitrate)
{
    debugOut("Attempting synchronisation at bitrate %d\n", bitrate);

    changeLocalBitRate(bitrate);

    int tries = 0;
    while (tries < MAX_JTAG_SYNC_ATTEMPS)
    {
	// 'S  ' works if this is the first string sent after power up.
	// But if another char is sent, the JTAG seems to go in to some 
	// internal mode. 'SE  ' takes it out of there, apparently (sometimes
	// 'E  ' is enough, but not always...)
	sendJtagCommand((uchar *)"SE  ", 4, &tries);
	usleep(2 * JTAG_COMM_TIMEOUT); // let rest of response come before we ignore it
	if (tcflush(jtagBox, TCIFLUSH) < 0)
            throw jtag_exception();
	if (checkForEmulator())
	    return true;
    }
    return false;
}

/** Attempt to synchronise with JTAG ICE at all possible bit rates **/
void jtag1::startJtagLink(void)
{
    static int bitrates[] =
    { 115200, 19200, 57600, 38400, 9600 };

    for (unsigned int i = 0; i < sizeof bitrates / sizeof *bitrates; i++)
	if (synchroniseAt(bitrates[i]))
	    return;

    throw jtag_exception("Failed to synchronise with the JTAG ICE (is it connected and powered?)");
}

/** Device automatic configuration 
 Determines the device being controlled by the JTAG ICE and configures
 the system accordingly.

 May be overridden by command line parameter.

*/
void jtag1::deviceAutoConfig(void)
{
    unsigned int device_id;
    jtag_device_def_type *pDevice = deviceDefinitions;

    // Auto config
    debugOut("Automatic device detection: ");

    /* Set daisy chain information */
    configDaisyChain();

    /* Read in the JTAG device ID to determine device */
    device_id = getJtagParameter(JTAG_P_JTAGID_BYTE0);
    /* Reading the first byte resumes the program (weird, no?) */
    interruptProgram();
    device_id += (getJtagParameter(JTAG_P_JTAGID_BYTE1) <<  8) +
      (getJtagParameter(JTAG_P_JTAGID_BYTE2) << 16) + 
      (getJtagParameter(JTAG_P_JTAGID_BYTE3) << 24);

   
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
	if ((pDevice->device_flags & DEVFL_MKII_ONLY) != 0)
        {
            fprintf(stderr, "Device is not supported by JTAG ICE mkI");
            throw jtag_exception();
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
	if ((pDevice->device_flags & DEVFL_MKII_ONLY) != 0)
        {
            fprintf(stderr, "Device is not supported by JTAG ICE mkI");
            throw jtag_exception();
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


void jtag1::initJtagBox(void)
{
    statusOut("JTAG config starting.\n");

    startJtagLink();
    changeBitRate(115200);

    uchar hw_ver = getJtagParameter(JTAG_P_HW_VERSION);
    statusOut("Hardware Version: 0x%02x\n", hw_ver);

    uchar sw_ver = getJtagParameter(JTAG_P_SW_VERSION);
    statusOut("Software Version: 0x%02x\n", sw_ver);

    interruptProgram();

    deviceAutoConfig();

    // Clear out the breakpoints.
    deleteAllBreakpoints();

    statusOut("JTAG config complete.\n");
}


void jtag1::initJtagOnChipDebugging(unsigned long bitrate)
{
    statusOut("Preparing the target device for On Chip Debugging.\n");

    uchar br;
    if (bitrate >= 1000000UL)
	br = JTAG_BITRATE_1_MHz;
    else if (bitrate >= 500000)
	br = JTAG_BITRATE_500_KHz;
    else if (bitrate >= 250000)
	br = JTAG_BITRATE_250_KHz;
    else
	br = JTAG_BITRATE_125_KHz;
    // Set JTAG bitrate
    setJtagParameter(JTAG_P_CLOCK, br);

    // Ensure on-chip debug enable fuse is enabled ie '0'
    jtagActivateOcdenFuse();

    resetProgram(false);
    setJtagParameter(JTAG_P_TIMERS_RUNNING, 0x00);
    resetProgram(true);
}

void jtag1::configDaisyChain(void)
{
    /* Set daisy chain information (if needed) */
    if (dchain.units_before > 0)
	setJtagParameter(JTAG_P_UNITS_BEFORE,dchain.units_before);
    if (dchain.units_after > 0)
	setJtagParameter(JTAG_P_UNITS_AFTER,dchain.units_after);
    if (dchain.bits_before > 0)
	setJtagParameter(JTAG_P_BIT_BEFORE,dchain.bits_before);
    if (dchain.bits_after > 0)
	setJtagParameter(JTAG_P_BIT_AFTER,dchain.bits_after);
}

