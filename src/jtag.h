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
 * This file contains functions for interfacing with the GDB remote protocol.
 */

#ifndef JTAG_H
#define JTAG_H

#include "ioreg.h"

/* The data in this structure will be sent directorly to the jtagice box. */

typedef struct {
    unsigned char cmd;                 // The jtag command to prefix the desc.

    /* The following arrays are bitmaps where each bit is a flag denoting
       wether the register can be read or written. Bit 0 of byte 0 represents
       the IO register at sram addres 0x20, while bit 7 of byte 7 is
       represents the register at 0x5f. */

    unsigned char rd[8];               // IO read access.
    unsigned char wr[8];               // IO write access.
    unsigned char sh_rd[8];            // IO shadow read access.
    unsigned char sh_wr[8];            // IO shadow write access.

    /* Same as above, except that first bit is register at sram address 0x60
       and last bit is register at 0xff. */

    unsigned char ext_rd[20];          // Extended IO read access.
    unsigned char ext_wr[20];          // Extended IO write access.
    unsigned char ext_sh_rd[20];       // Extended IO shadow read access.
    unsigned char ext_sh_wr[20];       // Extended IO shadow write access.

    /* Register locations. */

    unsigned char idr_addr;            // IDR address in IO space.
    unsigned char spmcr_addr;          // SPMCR address in SRAM space.
    unsigned char rampz_addr;          // RAMPZ address in IO space.

    /* Memory programming page sizes (in bytes). */

    unsigned char flash_pg_sz[2];      // [0]->little end; [1]->big end
    unsigned char eeprom_pg_sz;

    unsigned char boot_addr[4];        // Boot loader start address.
                                       // This is a WORD address.
                                       // [0]->little end; [3]->big end

    unsigned char last_ext_io_addr;    // Last extended IO location, 0 if no
                                       // extended IO.

    unsigned char eom[2];              // JTAG command terminator.
} jtag_device_desc_type;

typedef struct {
    const char* name;
    const unsigned int device_id;      // Part Number from JTAG Device 
                                       // Identification Register
    unsigned int flash_page_size;      // Flash memory page size in bytes
    unsigned int flash_page_count;     // Flash memory page count
    unsigned char eeprom_page_size;    // EEPROM page size in bytes
    unsigned int eeprom_page_count;    // EEPROM page count
    unsigned int vectors_end;	       // End of interrupt vector table

    gdb_io_reg_def_type *io_reg_defs;

    jtag_device_desc_type dev_desc;    // Device descriptor to download to
                                       // device
} jtag_device_def_type;

extern jtag_device_def_type *global_p_device_def;

// various enums
enum
{
    // Address space selector values
    ADDR_PROG_SPACE_PROG_ENABLED      = 0xB0,
    ADDR_PROG_SPACE_PROG_DISABLED     = 0xA0,
    ADDR_DATA_SPACE                   = 0x20,
    ADDR_EEPROM_SPACE                 = 0xB1,
    ADDR_FUSE_SPACE                   = 0xB2,
    ADDR_LOCK_SPACE                   = 0xB3,
    ADDR_SIG_SPACE                    = 0xB4,
    ADDR_BREAKPOINT_SPACE             = 0x60,

    // Address space offsets
    FLASH_SPACE_ADDR_OFFSET           = 0x000000,
    DATA_SPACE_ADDR_OFFSET            = 0x800000,

    EEPROM_SPACE_ADDR_OFFSET          = 0x810000,

    FUSE_SPACE_ADDR_OFFSET            = 0x820000,

    LOCK_SPACE_ADDR_OFFSET            = 0x830000,

    SIG_SPACE_ADDR_OFFSET             = 0x840000,

    BREAKPOINT_SPACE_ADDR_OFFSET      = 0x900000,

    ADDR_SPACE_MASK = (DATA_SPACE_ADDR_OFFSET   |
                       EEPROM_SPACE_ADDR_OFFSET |
                       FUSE_SPACE_ADDR_OFFSET   |
                       LOCK_SPACE_ADDR_OFFSET   |
                       SIG_SPACE_ADDR_OFFSET    |
                       BREAKPOINT_SPACE_ADDR_OFFSET),

    // Lock Bit Values
    LOCK_BITS_ALL_UNLOCKED            = 0xff,

    // Fuse Bit Values
    FUSE_M103C                        = 0x02,
    FUSE_WDTON                        = 0x01,

    FUSE_OCDEN                        = 0x80,
    FUSE_JTAGEN                       = 0x40,
    FUSE_SPIEN                        = 0x20,
    FUSE_CKOPT                        = 0x10,
    FUSE_EESAVE                       = 0x08,
    FUSE_BOOTSZ1                      = 0x04,
    FUSE_BOOTSZ0                      = 0x02,
    FUSE_BOOTRST                      = 0x01,

    FUSE_BODLEVEL                     = 0x80,
    FUSE_BODEN                        = 0x40,
    FUSE_SUT1                         = 0x20,
    FUSE_SUT0                         = 0x10,
    FUSE_CKSEL3                       = 0x08,
    FUSE_CKSEL2                       = 0x04,
    FUSE_CKSEL1                       = 0x02,
    FUSE_CKSEL0                       = 0x01,

    // Comms link bit rates
    BIT_RATE_9600                     = 0xf4,
    BIT_RATE_14400                    = 0xf8,
    BIT_RATE_19200                    = 0xfa,
    BIT_RATE_38400                    = 0xfd,
    BIT_RATE_57600                    = 0xfe,
    BIT_RATE_115200                   = 0xff,

    // Breakpoints (match values returned by JTAG box).
    BREAKPOINT_NONE                   = 0x00,
    BREAKPOINT_X                      = 0x04,
    BREAKPOINT_Y                      = 0x08,
    BREAKPOINT_Z                      = 0x10,


    // Responses from JTAG ICE
    JTAG_R_OK			      = 'A',
    JTAG_R_BREAK		      = 'B',
    JTAG_R_INFO			      = 'G',
    JTAG_R_FAILED		      = 'F',
    JTAG_R_SYNC_ERROR		      = 'E',
    JTAG_R_SLEEP		      = 'H',
    JTAG_R_POWER		      = 'I',

    // JTAG parameters
    JTAG_P_BITRATE		      = 'b',
    JTAG_P_SW_VERSION		      = 0x7b,
    JTAG_P_HW_VERSION		      = 0x7a,
    JTAG_P_IREG_HIGH                  = 0x81,
    JTAG_P_IREG_LOW                   = 0x82,
    JTAG_P_OCD_VTARGET                = 0x84,
    JTAG_P_OCD_BREAK_CAUSE            = 0x85,
    JTAG_P_CLOCK		      = 0x86,
    JTAG_P_EXTERNAL_RESET             = 0x8b, /* W */
    JTAG_P_FLASH_PAGESIZE_LOW         = 0x88, /* W */
    JTAG_P_FLASH_PAGESIZE_HIGH        = 0x89, /* W */
    JTAG_P_EEPROM_PAGESIZE            = 0x8a, /* W */
    JTAG_P_TIMERS_RUNNING	      = 0xa0,
    JTAG_P_BP_FLOW		      = 0xa1,
    JTAG_P_BP_X_HIGH		      = 0xa2,
    JTAG_P_BP_X_LOW		      = 0xa3,
    JTAG_P_BP_Y_HIGH		      = 0xa4,
    JTAG_P_BP_Y_LOW		      = 0xa5,
    JTAG_P_BP_MODE		      = 0xa6,
    JTAG_P_JTAGID_BYTE0               = 0xa7, /* R */
    JTAG_P_JTAGID_BYTE1               = 0xa8, /* R */
    JTAG_P_JTAGID_BYTE2               = 0xa9, /* R */
    JTAG_P_JTAGID_BYTE3               = 0xaa, /* R */
    JTAG_P_UNITS_BEFORE               = 0xab, /* W */
    JTAG_P_UNITS_AFTER                = 0xac, /* W */
    JTAG_P_BIT_BEFORE                 = 0xad, /* W */
    JTAG_P_BIT_AFTER                  = 0xae, /* W */
    JTAG_P_PSB0_LOW                   = 0xaf, /* W */
    JTAG_P_PSBO_HIGH                  = 0xb0, /* W */
    JTAG_P_PSB1_LOW                   = 0xb1, /* W */
    JTAG_P_PSB1_HIGH                  = 0xb2, /* W */
    JTAG_P_MCU_MODE                   = 0xb3, /* R */

    // JTAG commands
    JTAG_C_SET_DEVICE_DESCRIPTOR      = 0xA0,

    MAX_JTAG_COMM_ATTEMPS	      = 10,
    MAX_JTAG_SYNC_ATTEMPS	      = 3,

    // JTAG communication timeouts, in microseconds
    // RESPONSE is for the first response byte
    // COMM is for subsequent response bytes
    JTAG_RESPONSE_TIMEOUT	      = 1000000,
    JTAG_COMM_TIMEOUT		      = 100000,

    // Set JTAG bitrate to 1MHz
    // ff: 1MHz, fe: 500kHz, fd: 250khz, fb: 125Khz
    // JTAG bitrates
    JTAG_BITRATE_1_MHz                = 0xff,
    JTAG_BITRATE_500_KHz              = 0xfe,
    JTAG_BITRATE_250_KHz              = 0xfd,
    JTAG_BITRATE_125_KHz              = 0xfb
};

enum {
    PC_INVALID			      = 0xffffffff
};

// The Sync_CRC/EOP message terminator (no real CRC in sight...)
#define JTAG_EOM 0x20, 0x20

// A generic error message when nothing good comes to mind
#define JTAG_CAUSE "JTAG ICE communication failed"

// The file descriptor used while talking to the JTAG ICE
extern int jtagBox;

// Whether we are in "programming mode" (changes how program memory
// is written, apparently)
extern bool programmingEnabled;

// Name of the device controlled by the JTAG ICE
extern char *device_name;

// Basic JTAG I/O
// -------------

/** If status < 0: Report JTAG ICE communication error & exit **/
void jtagCheck(int status);

/** Initialise the serial port specified by jtagDeviceName for JTAG ICE access
 **/
void initJtagPort(char *jtagDeviceName);

/** A timed-out read from file descriptor 'fd'.

    'timeout' is in microseconds, it is the maximum interval within which
    the read must make progress (i.e., it's a per-byte timeout)

    Returns the number of bytes read or -1 for errors other than timeout.

    Note: EOF and timeout cannot be distinguished
**/
int timeout_read(int fd, void *buf, size_t count, unsigned long timeout);

/** Decode 3-byte big-endian address **/
unsigned long decodeAddress(uchar *buffer);

/** Encode 3-byte big-endian address **/
void encodeAddress(uchar *buffer, unsigned long x);

/** Send a command to the jtag, with retries, and return the 'responseSize' 
    byte response. Aborts avarice in case of to many failed retries.

    Returns a dynamically allocated buffer containing the reponse (caller
    must free)
**/
uchar *doJtagCommand(uchar *command, int  commandSize, int responseSize);

/** Simplified form of doJtagCommand:
    Send 1-byte command 'cmd' to JTAG ICE, with retries, expecting a 
    'responseSize' byte reponse.

    Return true if responseSize is 0 or if last response byte is
    JTAG_R_OK
**/
bool doSimpleJtagCommand(uchar cmd, int responseSize);

/** Send initial configuration to setup the JTAG box itself. 
    If attach is true, the currently running program is attached
    (Note: when attaching, fuse bits cannot be set so debugging must
    have been enabled earlier)

    The bitrate sets the JTAG bitrate. The bitrate must be less than 1/4 that
    of the target avr frequency or the jtagice will have problems reading from
    the target. The problems are usually manifested as failed calls to
    jtagRead().
 **/
void initJtagBox(bool attach, uchar bitrate);


// Breakpoints
// -----------

enum bpType
{
    NONE,           // disabled.
    CODE,           // normal code space breakpoint.
    WRITE_DATA,     // write data space breakpoint (ie "watch").
    READ_DATA,      // read data space breakpoint (ie "watch").
    ACCESS_DATA,    // read/write data space breakpoint (ie "watch").
};

/** Clear out the breakpoints. */
void deleteAllBreakpoints(void);

/** Delete breakpoint at the specified address. */
bool deleteBreakpoint(unsigned int address, bpType type, unsigned int length);

/** Add a code breakpoint at the specified address. */
bool addBreakpoint(unsigned int address, bpType type, unsigned int length);

/** Send the breakpoint details down to the JTAG box. */
void updateBreakpoints(bool setCodeBreakpoints);

/** True if there is a breakpoint at address */
bool codeBreakpointAt(unsigned int address);

/** True if there is a breakpoint between start (inclusive) and 
    end (exclusive) */
bool codeBreakpointBetween(unsigned int start, unsigned int end);

bool stopAt(unsigned int address);

// Miscellaneous
// -------------

/** Set JTAG ICE parameter 'item' to 'newValue' **/
void setJtagParameter(uchar item, uchar newValue);

/** Return value of JTAG ICE parameter 'item' **/
uchar getJtagParameter(uchar item);

// Writing to program memory
// -------------------------

/** Switch to faster programming mode, allows chip erase */
void enableProgramming(void);

/** Switch back to normal programming mode **/
void disableProgramming(void);

/** Erase all chip memory **/
void eraseProgramMemory(void);

/** Download an image contained in the specified file. */
void downloadToTarget(const char* filename, bool program, bool verify);

// Running, single stepping, etc
// -----------------------------

/** Retrieve the current Program Counter value, or PC_INVALID if fails */
unsigned long getProgramCounter(void);

/** Set program counter to 'pc'. Return true iff successful **/
bool setProgramCounter(unsigned long pc);

/** Reset AVR. Return true iff successful **/
bool resetProgram(void);

/** Interrupt AVR. Return true iff successful **/
bool interruptProgram(void);

/** Resume program execution. Return true iff successful.
    Note: the gdb 'continue' command is handled by jtagContinue,
    this is just the low level command to resume after interruptProgram
**/
bool resumeProgram(void);

/** Issue a "single step" command to the JTAG box. 
    Return true iff successful **/
bool jtagSingleStep(void);

/** Send the program on it's merry way, and wait for a breakpoint or
    input from gdb.
    Return true for a breakpoint, false for gdb input. **/
bool jtagContinue(bool setCodeBreakpoints);

// R/W memory
// ----------

/** Read 'numBytes' from target memory address 'addr'.

    The memory space is selected by the high order bits of 'addr' (see
    above).

    Returns a dynamically allocated buffer with the requested bytes if
    successful (caller must free), or NULL if the read failed.
 **/
uchar *jtagRead(unsigned long addr, unsigned int numBytes);

/** Write 'numBytes' bytes from 'buffer' to target memory address 'addr'

    The memory space is selected by the high order bits of 'addr' (see
    above).

    Returns true for a successful write, false for a failed one.

    Note: The current behaviour for program-space writes is pretty
    weird (does not match jtagRead). See comments in jtagrw.cc.
**/
bool jtagWrite(unsigned long addr, unsigned int numBytes, uchar buffer[]);


/** Write fuses to target.

    The input parameter is a string from command-line, as produced by
    printf("%x", 0xaabbcc );
**/
void jtagWriteFuses(char *fuses);


/** Write lockbits to target.

    The input parameter is a string from command-line, as produced by
    printf("%x", 0xaa);
**/
void jtagWriteLockBits(char *lock);

#endif
