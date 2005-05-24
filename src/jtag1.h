/*
 *	avarice - The "avarice" program.
 *	Copyright (C) 2001 Scott Finneran
 *	Copyright (C) 2002, 2003, 2004 Intel Corporation
 *	Copyright (C) 2005 Joerg Wunsch
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License Version 2
 *	as published by the Free Software Foundation.
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
 * This file contains functions for interfacing with the GDB remote protocol.
 *
 * $Id$
 */

#ifndef JTAG1_H
#define JTAG1_H

#include "jtag.h"

class jtag1: public jtag
{
  public:
  jtag1(const char *dev): jtag(dev) {};

  virtual void initJtagBox(void);
  virtual void initJtagOnChipDebugging(uchar bitrate);

  virtual void deleteAllBreakpoints(void);
  virtual bool deleteBreakpoint(unsigned int address, bpType type, unsigned int length);
  virtual bool addBreakpoint(unsigned int address, bpType type, unsigned int length);
  virtual void updateBreakpoints(bool setCodeBreakpoints);
  virtual bool codeBreakpointAt(unsigned int address);
  virtual bool codeBreakpointBetween(unsigned int start, unsigned int end);
  virtual bool stopAt(unsigned int address);
  virtual void breakOnChangeFlow(void);

  virtual void enableProgramming(void);
  virtual void disableProgramming(void);
  virtual void eraseProgramMemory(void);
  virtual void eraseProgramPage(unsigned long address);
  virtual void downloadToTarget(const char* filename, bool program, bool verify);

  virtual unsigned long getProgramCounter(void);
  virtual bool setProgramCounter(unsigned long pc);
  virtual bool resetProgram(void);
  virtual bool interruptProgram(void);
  virtual bool resumeProgram(void);
  virtual bool jtagSingleStep(void);
  virtual bool jtagContinue(bool setCodeBreakpoints);

  virtual uchar *jtagRead(unsigned long addr, unsigned int numBytes);
  virtual bool jtagWrite(unsigned long addr, unsigned int numBytes, uchar buffer[]);
  virtual void jtagWriteFuses(char *fuses);
  virtual void jtagReadFuses(void);
  virtual void jtagDisplayFuses(uchar *fuseBits);
  virtual void jtagWriteLockBits(char *lock);
  virtual void jtagReadLockBits(void);
  virtual void jtagDisplayLockBits(uchar *lockBits);

  private:
  virtual SendResult sendJtagCommand(uchar *command, int commandSize, int *tries);
  virtual uchar *getJtagResponse(int responseSize);
  virtual void changeBitRate(uchar newBitRate);
  virtual void setDeviceDescriptor(jtag_device_def_type *dev);
  virtual bool checkForEmulator(void);
  virtual bool synchroniseAt(uchar bitrate);
  virtual void startJtagLink(void);
  virtual void deviceAutoConfig(void);
  virtual uchar *doJtagCommand(uchar *command, int  commandSize, int  responseSize);
  virtual bool doSimpleJtagCommand(unsigned char cmd, int responseSize);
  virtual void setJtagParameter(uchar item, uchar newValue);
  virtual uchar getJtagParameter(uchar item);
  virtual void jtag_flash_image(BFDimage *image, BFDmemoryType memtype,
				bool program, bool verify);
};

#endif
