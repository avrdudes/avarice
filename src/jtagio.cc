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
    check((*tries)++ < MAX_JTAG_COMM_ATTEMPS,
	      "JTAG ICE: Cannot synchronise");

    debugOut("\ncommand[%c, %d]: ", command[0], *tries);

    for (int i = 0; i < commandSize; i++)
	debugOut("%.2X ", command[i]);

    debugOut("\n");

    // before writing, clean up any "unfinished business".
    jtagCheck(tcflush(jtagBox, TCIFLUSH));

    int count = safewrite(command, commandSize);
    if (count < 0)
      jtagCheck(count);
    else // this shouldn't happen
      check(count == commandSize, JTAG_CAUSE);

    // And wait for all characters to go to the JTAG box.... can't hurt!
    jtagCheck(tcdrain(jtagBox));

    // We should get JTAG_R_OK, but we might get JTAG_R_INFO too (we just
    // ignore it)
    for (;;)
      {
	uchar ok;
	count = timeout_read(&ok, 1, JTAG_RESPONSE_TIMEOUT);
	jtagCheck(count);

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
    jtagCheck(numCharsRead);

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
	    check(response != NULL, JTAG_CAUSE);
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


/** Change bitrate of PC's serial port as specified by BIT_RATE_xxx in
    'newBitRate' **/
void jtag::changeLocalBitRate(uchar newBitRate)
{
    // Change the local port bitrate.
    struct termios tio;

    jtagCheck(tcgetattr(jtagBox, &tio)); 

    speed_t newPortSpeed = B19200;
    // Linux doesn't support 14400. Let's hope it doesn't end up there...
    switch(newBitRate)
    {
    case BIT_RATE_9600:
	newPortSpeed = B9600;
	break;
    case BIT_RATE_19200:
	newPortSpeed = B19200;
	break;
    case BIT_RATE_38400:
	newPortSpeed = B38400;
	break;
    case BIT_RATE_57600:
	newPortSpeed = B57600;
	break;
    case BIT_RATE_115200:
	newPortSpeed = B115200;
	break;
    default:
	debugOut("unsupported bitrate\n");
	exit(1);
    }

    cfsetospeed(&tio, newPortSpeed);
    cfsetispeed(&tio, newPortSpeed);

    jtagCheck(tcsetattr(jtagBox,TCSANOW,&tio));
    jtagCheck(tcflush(jtagBox, TCIFLUSH));
}

/** Set PC and JTAG ICE bitrate to BIT_RATE_xxx specified by 'newBitRate' **/
void jtag1::changeBitRate(uchar newBitRate)
{
    setJtagParameter(JTAG_P_BITRATE, newBitRate);
    changeLocalBitRate(newBitRate);
}

/** Set the JTAG ICE device descriptor data for specified device type **/
void jtag1::setDeviceDescriptor(jtag_device_def_type *dev)
{
    uchar *response = NULL;
    uchar *command = (uchar *)(&dev->dev_desc);

    response = doJtagCommand(command, sizeof dev->dev_desc, 1);
    check(response[0] == JTAG_R_OK,
	      "JTAG ICE: Failed to set device description");

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
bool jtag1::synchroniseAt(uchar bitrate)
{
    debugOut("Attempting synchronisation at bitrate %02x\n", bitrate);

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
	jtagCheck(tcflush(jtagBox, TCIFLUSH));
	if (checkForEmulator())
	    return true;
    }
    return false;
}

/** Attempt to synchronise with JTAG ICE at all possible bit rates **/
void jtag1::startJtagLink(void)
{
    static uchar bitrates[] =
    { BIT_RATE_115200, BIT_RATE_19200, BIT_RATE_57600, BIT_RATE_38400,
      BIT_RATE_9600 };

    for (unsigned int i = 0; i < sizeof bitrates / sizeof *bitrates; i++)
	if (synchroniseAt(bitrates[i]))
	    return;

    check(false, "Failed to synchronise with the JTAG ICE (is it connected and powered?)");
}

/** Device automatic configuration 
 Determines the device being controlled by the JTAG ICE and configures
 the system accordingly.

 May be overridden by command line parameter.

*/
void jtag1::deviceAutoConfig(void)
{
    unsigned int device_id;
    int i;
    jtag_device_def_type *pDevice = deviceDefinitions;

    // Auto config
    debugOut("Automatic device detection: ");
   
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


void jtag1::initJtagBox(void)
{
    statusOut("JTAG config starting.\n");

    startJtagLink();
    changeBitRate(BIT_RATE_115200);

    uchar hw_ver = getJtagParameter(JTAG_P_HW_VERSION);
    statusOut("Hardware Version: 0x%02x\n", hw_ver);
    //check(hw_ver == 0xc0, "JTAG ICE: Unknown hardware version");

    uchar sw_ver = getJtagParameter(JTAG_P_SW_VERSION);
    statusOut("Software Version: 0x%02x\n", sw_ver);

    interruptProgram();

    deviceAutoConfig();

    // Clear out the breakpoints.
    deleteAllBreakpoints();

    statusOut("JTAG config complete.\n");
}


void jtag1::initJtagOnChipDebugging(uchar bitrate)
{
    statusOut("Preparing the target device for On Chip Debugging.\n");

    // Set JTAG bitrate
    setJtagParameter(JTAG_P_CLOCK, bitrate);

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
    setJtagParameter(JTAG_P_TIMERS_RUNNING, 0x00);
    resetProgram();
}
