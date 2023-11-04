/*
 *	avarice - The "avarice" program.
 *	Copyright (C) 2012 Joerg Wunsch
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
 * This file implements the basic IO handling for the JTAGICE3 protocol.
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
#include "jtag.h"
#include "jtag3.h"

jtag3_io_exception::jtag3_io_exception(unsigned int code)
{
  static char buffer[50];
  response_code = code;

  switch (code)
    {
    case RSP3_FAIL_DEBUGWIRE:
      reason = "debugWIRE failed"; break;

    case RSP3_FAIL_PDI:
      reason = "PDI failed"; break;

    case RSP3_FAIL_NO_ANSWER:
      reason = "no answer from target"; break;

    case RSP3_FAIL_NO_TARGET_POWER:
      reason = "No target power present"; break;

    case RSP3_FAIL_WRONG_MODE:
      reason = "wrong mode"; break;

    case RSP3_FAIL_UNSUPP_MEMORY:
      reason = "unsupported memory type"; break;

    case RSP3_FAIL_WRONG_LENGTH:
      reason = "wrong length for memory access"; break;

    case RSP3_FAIL_NOT_UNDERSTOOD:
      reason = "command not understood"; break;

    default:
      snprintf(buffer, sizeof buffer, "Unknown response code 0x%0x", code);
      reason = buffer;
    }
}

jtag3::~jtag3(void)
{
  // Terminate connection to JTAG box.
  if (signedIn)
  {
    if (debug_active)
    {
      try
      {
	doSimpleJtagCommand(CMD3_STOP_DEBUG, "stop debugging");
      }
      catch (jtag_exception&)
      {
	// just proceed with the sign-off
      }

      try
      {
	doSimpleJtagCommand(CMD3_SIGN_OFF, "AVR sign-off");
      }
      catch (jtag_exception&)
      {
	// just proceed with the sign-off
      }
    }

    try
    {
      doSimpleJtagCommand(CMD3_SIGN_OFF, "sign-off", SCOPE_GENERAL);
    }
    catch (jtag_exception&)
    {
      // just proceed with the sign-off
    }
    signedIn = false;
  }
}


/*
 * Send one frame.  Adds the required preamble and CRC, and ensures
 * the frame could be written correctly.
 */
void jtag3::sendFrame(uchar *command, int commandSize)
{
    unsigned char *buf = new unsigned char[commandSize + 4];

    buf[0] = TOKEN;
    buf[1] = 0;
    u16_to_b2(buf + 2, command_sequence);
    memcpy(buf + 4, command, commandSize);

    for (int i = 0; i < commandSize + 4; i++)
	debugOut("%.2X ", buf[i]);
    debugOut("\n");

    int count = safewrite(buf, commandSize + 4);

    delete [] buf;

    if (count < 0)
        throw jtag_exception();
    else if (count != commandSize + 4)
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
int jtag3::recvFrame(unsigned char *&msg, unsigned short &seqno)
{
  uchar tempbuf[MAX_MESSAGE_SIZE_JTAGICE3];
  int rv, l;

  msg = NULL;

  int amnt;
  rv = timeout_read((void *)&amnt, sizeof amnt, JTAG_RESPONSE_TIMEOUT);
  if (rv == 0)
  {
    /* timeout */
    debugOut("read() timed out\n");

    return 0;
  }
  else if (rv < 0)
  {
    /* error */
    debugOut("read() error %d\n", rv);
    throw jtag_exception("read error");
  }
  if (amnt <= 0 || amnt > MAX_MESSAGE_SIZE_JTAGICE3)
  {
    debugOut("unexpected message size from pipe: %d\n", amnt);
    return 0;
  }

  rv = timeout_read(tempbuf, amnt, JTAG3_PIPE_TIMEOUT);
  if (rv > 0)
  {
    bool istoken = tempbuf[0] == TOKEN_EVT3;
    if (istoken)
      tempbuf[0] = TOKEN;

    debugOut("read: ");
    for (l = 0; l < rv; l++) {
      debugOut(" %02x", tempbuf[l]);
    }
    debugOut("\n");

    if (istoken)
    {
      unsigned int serial = tempbuf[2] + (tempbuf[3] << 8);
      debugOut("Event serial 0x%04x\n", serial);

      rv -= 4;
      msg = new unsigned char[rv];
      seqno = 0xffff;
      memcpy(msg, tempbuf + 4, rv);

      return rv;
    }
    else
    {
      seqno = tempbuf[1] + (tempbuf[2] << 8);
      rv -= 3;
      msg = new unsigned char[rv];
      memcpy(msg, tempbuf + 3, rv);

      return rv;
    }
  }
  else if (rv == 0)
  {
    /* timeout */
    debugOut("read() timed out\n");

    return 0;
  }
  else
  {
    /* error */
    debugOut("read() error %d\n", rv);
    throw jtag_exception("read error");
  }
}

/*
 * Try receiving frames, until we get the reply we are expecting.
 * Caller must delete[] the msg after processing it.
 */
int jtag3::recv(uchar *&msg)
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
	    return rv;
	}
	if (r_seqno == 0xffff) {
	    debugOut("\ngot asynchronous event: 0x%02x, 0x%02x\n",
		     msg[0], msg[1]);
	    if (cached_event == NULL)
	    {
	      cached_event = msg;
	      // do *not* delete[] it here
	      continue;
	    }
	} else {
	    debugOut("\ngot wrong sequence number, %u != %u\n",
		     r_seqno, command_sequence);
	}
	delete [] msg;
    }
}

/** Send a command to the jtag, and check result.

    Reads first response byte. If no response is received within
    JTAG_RESPONSE_TIMEOUT, returns false. If response is
    positive returns true, otherwise returns false.

    If response is positive, message (including response code) is
    returned in &msg, caller must delete [] it.  The message size is
    returned in &msgsize.
**/

bool jtag3::sendJtagCommand(uchar *command, int commandSize,
                            const char *name,
			    uchar *&msg, int &msgsize)
{
    debugOut("\ncommand \"%s\" [0x%02x, 0x%02x]\n",
             name, command[0], command[1]);

    sendFrame(command, commandSize);

    msgsize = recv(msg);
    if (msgsize < 1)
	return false;

    debugOut("response: ");
    for (int i = 0; i < msgsize; i++)
    {
	debugOut("%.2X ", msg[i]);
    }
    debugOut("\n");

    unsigned char c = msg[1];

    if (c >= RSP3_OK && c < RSP3_FAILED)
	return true;

    return false;
}


void jtag3::doJtagCommand(uchar *command, int  commandSize,
                          const char *name,
                          uchar *&response, int &responseSize)
{
    if (sendJtagCommand(command, commandSize, name, response, responseSize))
        return;

    if (responseSize == 0)
        throw jtag_timeout_exception();
    else
        throw jtag3_io_exception(response[3]);
}

void jtag3::doSimpleJtagCommand(uchar command, const char *name, uchar scope)
{
    int dummy;
    uchar *replydummy, cmd[3];
    int tries;

    cmd[0] = scope;
    cmd[1] = command;
    cmd[2] = 0;

    // Send command until we get an OK response
    for (tries = 0; tries < 10; tries++)
    {
	if (sendJtagCommand(cmd, 3, name, replydummy, dummy)) {
	    if (replydummy == NULL)
		throw jtag_io_exception();
	    if (dummy < 3)
		throw jtag_exception("Unexpected response size in doSimpleJtagCommand");
	    if (replydummy[1] != RSP3_OK)
	      {
		if (replydummy[1] < RSP3_FAILED)
		  throw jtag_exception("Unexpected positive reply in doSimpleJtagCommand");
		else
		  throw jtag_io_exception(dummy >= 4? replydummy[3]: 0);
	      }
	    delete [] replydummy;
	    return;
	}
    }
    throw jtag_exception("doSimpleJtagCommand(): too many failures");
}

void jtag3::changeBitRate(int newBitRate __attribute__((unused)))
{
  throw;
}

bool jtag3::synchroniseAt(int bitrate __attribute__((unused)))
{
  throw;
}

void jtag3::setDeviceDescriptor(jtag_device_def_type *dev)
{
  uchar *param, paramsize;
  jtag3_device_desc_type d3;

  if (proto == PROTO_UPDI)
  {
    param = (uchar *)&dev->dev_desc4;
	paramsize = sizeof dev->dev_desc4;
  }
  else if (is_xmega)
  {
    param = (uchar *)&dev->dev_desc3 + 4;
    paramsize = sizeof dev->dev_desc3 - 4;

    appsize = b4_to_u32(dev->dev_desc3.app_size);
  }
  else
  {
    param = (uchar *)&d3;
    paramsize = sizeof d3;
    memset(&d3, 0, sizeof d3);

    // Copy over all the data that can be derived from the existing
    // JTAG ICE mkII device descriptor.
    memcpy(d3.flash_page_size, dev->dev_desc2.uiFlashPageSize, 2);
    memcpy(d3.flash_size, dev->dev_desc2.ulFlashSize, 4);
    memcpy(d3.boot_address, dev->dev_desc2.ulBootAddress, 4);
    memcpy(d3.sram_offset, dev->dev_desc2.uiSramStartAddr, 2);

    unsigned int eesize = dev->eeprom_page_size * dev->eeprom_page_count;
    d3.eeprom_size[0] = eesize & 0xff;
    d3.eeprom_size[1] = (eesize >> 8) & 0xff;
    d3.eeprom_page_size = dev->eeprom_page_size;
    d3.always_one = 1;
    d3.allow_full_page_bitstream = dev->dev_desc2.ucAllowFullPageBitstream;
    d3.idr_address = dev->dev_desc2.ucIDRAddress;
    d3.eearh_address = dev->dev_desc2.EECRAddress[0] + 3;
    d3.eearl_address = dev->dev_desc2.EECRAddress[0] + 2;
    d3.eecr_address = dev->dev_desc2.EECRAddress[0];
    d3.eedr_address = dev->dev_desc2.EECRAddress[0] + 1;
    d3.spmcr_address = dev->dev_desc2.ucSPMCRAddress;

    // These data have to be recorded separately.
    d3.osccal_address = dev->osccal - 0x20;
    d3.ocd_revision = dev->ocdrev;
  }

  try
  {
    setJtagParameter(SCOPE_AVR, 2, PARM3_DEVICEDESC, param, paramsize);
  }
  catch (jtag_exception& e)
  {
    fprintf(stderr, "JTAG ICE: Failed to set device description: %s\n",
	    e.what());
    throw;
  }
}

void jtag3::startJtagLink(void)
{
  doSimpleJtagCommand(CMD3_SIGN_ON, "sign-on", SCOPE_GENERAL);

  signedIn = true;

  uchar cmd[4], *resp;
  int respsize;

  cmd[0] = SCOPE_INFO;
  cmd[1] = CMD3_GET_INFO;
  cmd[2] = 0;
  cmd[3] = CMD3_INFO_SERIAL;

  doJtagCommand(cmd, 4, "get info (serial number)", resp, respsize);

  if (resp[1] != RSP3_INFO)
    debugOut("Unexpected positive response to get info: 0x%02x\n",
	     resp[1]);
  else if (respsize < 4)
    debugOut("Unexpected response size to get info: %d\n", respsize);
  else
  {
    memmove(resp, resp + 3, respsize - 3);
    resp[respsize - 3] = 0;
    statusOut("Found a device, serial number: %s\n", resp);
  }
  delete [] resp;

  getJtagParameter(SCOPE_GENERAL, 0, PARM3_HW_VER, 5, resp);

  debugOut("ICE hardware version: %d\n", resp[3]);
  debugOut("ICE firmware version: %d.%02d (rel. %d)\n",
	   resp[4], resp[5], (resp[6] | (resp[7] << 8)));

  delete[] resp;

  uchar paramdata[1];

  if (is_xmega)
    paramdata[0] = PARM3_ARCH_XMEGA;
  else if (proto == PROTO_DW)
    paramdata[0] = PARM3_ARCH_TINY;
  else if (proto == PROTO_UPDI)
	paramdata[0] = PARM3_ARCH_UPDI;
  else
    paramdata[0] = PARM3_ARCH_MEGA;
  setJtagParameter(SCOPE_AVR, 0, PARM3_ARCH, paramdata, 1);

  paramdata[0] = PARM3_SESS_DEBUGGING;
  setJtagParameter(SCOPE_AVR, 0, PARM3_SESS_PURPOSE, paramdata, 1);

  switch (proto)
  {
    case PROTO_JTAG:
      paramdata[0] = PARM3_CONN_JTAG;
      break;

    case PROTO_DW:
      paramdata[0] = PARM3_CONN_DW;
      softbp_only = true;
      break;

    case PROTO_PDI:
      paramdata[0] = PARM3_CONN_PDI;
      break;

    case PROTO_UPDI:
      paramdata[0] = PARM3_CONN_UPDI;
      break;
  }
  setJtagParameter(SCOPE_AVR, 1, PARM3_CONNECTION, paramdata, 1);

  if (proto == PROTO_JTAG)
    configDaisyChain();

  if (proto == PROTO_UPDI)
  {
	/*
	* Unfortunately, for Xmega devices on UPDI, it's necessary
	* to send the device descriptor before sign-on.  However,
	* in order to do this, the JTAGICE3 needs a valid device
	* descriptor already.
	*
	* Hopefully, the values below will remain constant for all
	* Xmega devices ...
	*/
	jtag_device_def_type desc = { "dummy", 0 };

	u16_to_b2(desc.dev_desc4.prog_base, 0x4000);
	desc.dev_desc4.flash_page_size = 128;
	desc.dev_desc4.eeprom_page_size = 64;
	u16_to_b2(desc.dev_desc4.nvm_base_addr, 0x1000);
	u16_to_b2(desc.dev_desc4.ocd_base_addr, 0x0F80);
	setDeviceDescriptor(&desc);
  }

  cmd[0] = SCOPE_AVR;
  cmd[1] = CMD3_SIGN_ON;
  cmd[2] = 0;
  cmd[3] = proto == PROTO_JTAG && apply_nSRST;

  doJtagCommand(cmd, 4, "AVR sign-on", resp, respsize);

  if (resp[1] == RSP3_DATA && respsize >= 6)
  {
    unsigned int did = resp[3] | (resp[4] << 8) | (resp[5] << 16) | resp[6] << 24;
    delete [] resp;

    if (proto == PROTO_JTAG)
    {
      debugOut("AVR sign-on responded with device ID = 0x%0X : Ver = 0x%0x : Device = 0x%0x : Manuf = 0x%0x\n",
	       did,
	       (did & 0xF0000000) >> 28,
	       (did & 0x0FFFF000) >> 12,
	       (did & 0x00000FFE) >> 1);

      device_id = (did & 0x0FFFF000) >> 12;
    }
	else if (proto == PROTO_UPDI)
	{
      debugOut("AVR sign-on responded with %c%c%c%c\n",
	    (did & 0x000000FF) >> 0,
	    (did & 0x0000FF00) >> 8,
	    (did & 0x00FF0000) >> 16,
	    (did & 0xFF000000) >> 24);
	  device_id = 0;
	}
    else // debugWIRE
    {
      debugOut("AVR sign-on responded with device ID = 0x%0X\n", did);
      device_id = did;
    }
  }
  else
  {
    delete [] resp;

    /* Read in the JTAG device ID to determine device */
    uchar cmd[] = { SCOPE_AVR, CMD3_DEVICE_ID, 0 };
    try
    {
      doJtagCommand(cmd, sizeof cmd, "device ID", resp, respsize);

      unsigned int did = resp[3] | (resp[4] << 8) | (resp[5] << 16) | resp[6] << 24;
      delete [] resp;

      debugOut("Device ID = 0x%0X : Ver = 0x%0x : Device = 0x%0x : Manuf = 0x%0x\n",
	       did,
	       (did & 0xF0000000) >> 28,
	       (did & 0x0FFFF000) >> 12,
	       (did & 0x00000FFE) >> 1);

      device_id = (did & 0x0FFFF000) >> 12;
    }
    catch (jtag_exception&)
    {
      // ignore
    }
  }

  debug_active = true;
}

/** Device automatic configuration
 Determines the device being controlled by the JTAG ICE and configures
 the system accordingly.

 May be overridden by command line parameter.

*/
void jtag3::deviceAutoConfig(void)
{
    uchar *resp;
    jtag_device_def_type *pDevice = deviceDefinitions;

    // Auto config
    debugOut("Automatic device detection: ");

    if (device_id == 0)
    {
      /* Read in the JTAG device ID to determine device */

      if (is_xmega)
	{
	  /*
	   * Unfortunately, for Xmega devices, there's a
	   * chicken-and-egg problem here.  Xmega devices connected
	   * through JTAG should already have responded with a device
	   * ID above, but those connected through PDI didn't, so the
	   * last resort is to read the respective signature memory
	   * area.  However, in order to do this, the JTAGICE3 needs a
	   * valid device descriptor already. :-(
	   *
	   * Hopefully, the values below will remain constant for all
	   * Xmega devices ...
	   */
	  jtag_device_def_type desc = { "dummy", 0,
				     0, 0, 0, 0, 0, DEVFL_NONE,
				     nullptr, true, 0, 0, 0, 0,
				     {}, {}, {} };

	  u32_to_b4(desc.dev_desc3.nvm_data_offset, 0x1000000);
	  u16_to_b2(desc.dev_desc3.mcu_base_addr, 0x90);
	  setDeviceDescriptor(&desc);
	}

      resp = jtagRead(SIG_SPACE_ADDR_OFFSET, 3);
      device_id = resp[2] | (resp[1] << 8);
      delete [] resp;
    }

    statusOut("Reported device ID: 0x%0X\n", device_id);

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


void jtag3::initJtagBox(void)
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

    try
    {
      startJtagLink();

      //interruptProgram();

      deviceAutoConfig();

      // Clear out the breakpoints.
      deleteAllBreakpoints();

      statusOut("JTAG config complete.\n");
    }
    catch (jtag_exception& e)
    {
      fprintf(stderr, "initJtagBox() failed: %s\n",
	    e.what());
      throw;
    }
}


void jtag3::initJtagOnChipDebugging(unsigned long bitrate)
{
    statusOut("Preparing the target device for On Chip Debugging.\n");

    bitrate /= 1000;		// JTAGICE3 always uses kHz

    uchar value[2];
    value[0] = bitrate & 0xff;
    value[1] = (bitrate >> 8) & 0xff;

    uchar param = 0;
    if (proto == PROTO_JTAG)
    {
      if (is_xmega)
	param = PARM3_CLK_XMEGA_JTAG;
      else
	param = PARM3_CLK_MEGA_DEBUG;
    }
    else if (proto == PROTO_PDI)
    {
      // trying to set the PDI clock doesn't work here
      // param = PARM3_CLK_XMEGA_PDI;
    }
    if (param != 0)
    {
      setJtagParameter(SCOPE_AVR, 1, param, value, 2);
    }

    // Ensure on-chip debug enable fuse is enabled ie '0'
    jtagActivateOcdenFuse();

    uchar timers = 0;		// stopped
    setJtagParameter(SCOPE_AVR, 3, PARM3_TIMERS_RUNNING, &timers, 1);

    if (proto == PROTO_DW || is_xmega)
    {
        uchar cmd[] = { SCOPE_AVR, CMD3_START_DEBUG, 0, 1 };
        uchar *resp;
        int respsize;

        doJtagCommand(cmd, sizeof cmd, "start debugging", resp, respsize);
        delete [] resp;
    }

    // Sometimes (like, after just enabling the OCDEN fuse), the first
    // resetProgram() runs into an error code 0x32.  Just retry it once.
    try
      {
	resetProgram();
      }
    catch (jtag_exception &e)
      {
	debugOut("retrying reset ...\n");
	resetProgram();
      }

    cached_pc_is_valid = false;
}

void jtag3::configDaisyChain(void)
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
	setJtagParameter(SCOPE_AVR, 1, PARM3_JTAGCHAIN, buf, 4);
    }
}
