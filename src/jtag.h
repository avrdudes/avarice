/*
 *	avarice - The "avarice" program.
 *	Copyright (C) 2001 Scott Finneran
 *	Copyright (C) 2002, 2003, 2004 Intel Corporation
 *	Copyright (C) 2005, 2006, 2007 Joerg Wunsch
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

#ifndef JTAG_H
#define JTAG_H

#include <sys/types.h>
#include <termios.h>

#include <exception>

#include "pragma.h"
#include "ioreg.h"

using namespace std;

/* The data in this structure will be sent directorly to the jtagice box. */

typedef struct {
    unsigned char cmd;                 // The jtag command to prefix the desc.

    /* The following arrays are bitmaps where each bit is a flag denoting
       wether the register can be read or written. Bit 0 of byte 0 represents
       the IO register at sram addres 0x20, while bit 7 of byte 7 represents
       the register at 0x5f. */

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
} jtag1_device_desc_type;

// In appnote AVR067, struct device_descriptor is written with
// int/long field types.  We cannot use them directly, as they were
// neither properly aligned for portability, nor did they care for
// endianess issues.  We thus use arrays of unsigned chars, plus
// conversion macros.

typedef struct {
    unsigned char cmd;                 // The jtag command to prefix the desc.

    unsigned char ucReadIO[8]; //LSB = IOloc 0, MSB = IOloc63
    unsigned char ucReadIOShadow[8]; //LSB = IOloc 0, MSB = IOloc63
    unsigned char ucWriteIO[8]; //LSB = IOloc 0, MSB = IOloc63
    unsigned char ucWriteIOShadow[8]; //LSB = IOloc 0, MSB = IOloc63
    unsigned char ucReadExtIO[52]; //LSB = IOloc 96, MSB = IOloc511
    unsigned char ucReadIOExtShadow[52]; //LSB = IOloc 96, MSB = IOloc511
    unsigned char ucWriteExtIO[52]; //LSB = IOloc 96, MSB = IOloc511
    unsigned char ucWriteIOExtShadow[52];//LSB = IOloc 96, MSB = IOloc511
    unsigned char ucIDRAddress; //IDR address
    unsigned char ucSPMCRAddress; //SPMCR Register address and dW BasePC
    unsigned char ucRAMPZAddress; //RAMPZ Register address in SRAM I/O
				  //space
    unsigned char uiFlashPageSize[2]; //Device Flash Page Size
    unsigned char ucEepromPageSize; //Device Eeprom Page Size in bytes
    unsigned char ulBootAddress[4]; //Device Boot Loader Start Address
    unsigned char uiUpperExtIOLoc[2]; //Topmost (last) extended I/O
				      //location, 0 if no external I/O
    unsigned char ulFlashSize[4]; //Device Flash Size
    unsigned char ucEepromInst[20]; //Instructions for W/R EEPROM
    unsigned char ucFlashInst[3]; //Instructions for W/R FLASH
    unsigned char ucSPHaddr;	// stack pointer high
    unsigned char ucSPLaddr;	// stack pointer low
    // new as of 16-02-2004
    unsigned char uiFlashpages[2]; // number of pages in flash
    unsigned char ucDWDRAddress; // DWDR register address
    unsigned char ucDWBasePC;	// base/mask value of the PC
    // new as of 30-04-2004
    unsigned char ucAllowFullPageBitstream; // FALSE on ALL new
					    //parts
    unsigned char uiStartSmallestBootLoaderSection[2]; //
    // new as of 18-10-2004
    unsigned char EnablePageProgramming; // For JTAG parts only,
					 // default TRUE
    unsigned char ucCacheType;	// CacheType_Normal 0x00,
				// CacheType_CAN 0x01,
				// CacheType_HEIMDALL 0x02
				// new as of 27-10-2004
    unsigned char uiSramStartAddr[2]; // Start of SRAM
    unsigned char ucResetType;	// Selects reset type. ResetNormal = 0x00
				// ResetAT76CXXX = 0x01
    unsigned char ucPCMaskExtended; // For parts with extended PC
    unsigned char ucPCMaskHigh; // PC high mask
    unsigned char ucEindAddress; // Selects reset type. [EIND address...]
    // new as of early 2005, firmware 4.x
    unsigned char EECRAddress[2]; // EECR IO address
} jtag2_device_desc_type;

// New Xmega device descriptor, for firmware version 7 and above
typedef struct {
    unsigned char cmd;			// CMND_SET_XMEGA_PARAMS
    unsigned char whatever[2];		// cannot guess; must be 0x0002
    unsigned char datalen;		// length of the following data, = 47
    unsigned char nvm_app_offset[4];	// NVM offset for application flash
    unsigned char nvm_boot_offset[4];	// NVM offset for boot flash
    unsigned char nvm_eeprom_offset[4]; // NVM offset for EEPROM
    unsigned char nvm_fuse_offset[4];	// NVM offset for fuses
    unsigned char nvm_lock_offset[4];	// NVM offset for lock bits
    unsigned char nvm_user_sig_offset[4]; // NVM offset for user signature row
    unsigned char nvm_prod_sig_offset[4]; // NVM offset for production sign. row
    unsigned char nvm_data_offset[4];	// NVM offset for data memory (SRAM + IO)
    unsigned char app_size[4];		// size of application flash
    unsigned char boot_size[2];		// size of boot flash
    unsigned char flash_page_size[2];	// flash page size
    unsigned char eeprom_size[2];	// size of EEPROM
    unsigned char eeprom_page_size;	// EEPROM page size
    unsigned char nvm_base_addr[2];	// IO space base address of NVM controller
    unsigned char mcu_base_addr[2];	// IO space base address of MCU control
} xmega_device_desc_type;

// JTAGICE3 megaAVR parameter structure
typedef struct {
    unsigned char flash_page_size[2];   // in bytes
    unsigned char flash_size[4];        // in bytes
    unsigned char dummy1[4];            // always 0
    unsigned char boot_address[4];      // maximal (BOOTSZ = 3) bootloader
                                        // address, in 16-bit words (!)
    unsigned char sram_offset[2];       // pointing behind IO registers
    unsigned char eeprom_size[2];
    unsigned char eeprom_page_size;
    unsigned char ocd_revision;         // see XML; basically:
                                        // t13*, t2313*, t4313:        0
                                        // all other DW devices:       1
                                        // ATmega128(A):               1 (!)
                                        // ATmega16*,162,169*,32*,64*: 2
                                        // ATmega2560/2561:            4
                                        // all other megaAVR devices:  3
    unsigned char always_one;           // always = 1
    unsigned char allow_full_page_bitstream; // old AVRs, see XML
    unsigned char dummy2[2];            // always 0
                                        // all IO addresses below are given
                                        // in IO number space (without
                                        // offset 0x20), even though e.g.
                                        // OSCCAL always resides outside
    unsigned char idr_address;          // IDR, aka. OCDR
    unsigned char eearh_address;        // EEPROM access
    unsigned char eearl_address;
    unsigned char eecr_address;
    unsigned char eedr_address;
    unsigned char spmcr_address;
    unsigned char osccal_address;
} jtag3_device_desc_type;

/* UPDI device descriptor */
struct updi_device_desc {
    unsigned char prog_base[2];
    unsigned char flash_page_size;
    unsigned char eeprom_page_size;
    unsigned char nvm_base_addr[2];
    unsigned char ocd_base_addr[2];
};

#define fill_b4(u) \
{ ((u) & 0xffUL), (((u) & 0xff00UL) >> 8), \
  (((u) & 0xff0000UL) >> 16), (((u) & 0xff000000UL) >> 24) }
#define fill_b2(u) \
{ ((u) & 0xff), (((u) & 0xff00) >> 8) }

enum dev_flags {
	DEVFL_NONE         = 0x000000,
	DEVFL_NO_SOFTBP    = 0x000001, // Device cannot use software BPs (no BREAK insn)
	DEVFL_MKII_ONLY    = 0x000002, // Device is only supported in JTAG ICE mkII
};

typedef struct {
    const char* name;
    const unsigned int device_id;      // Part Number from JTAG Device 
                                       // Identification Register
    unsigned int flash_page_size;      // Flash memory page size in bytes
    unsigned int flash_page_count;     // Flash memory page count
    unsigned char eeprom_page_size;    // EEPROM page size in bytes
    unsigned int eeprom_page_count;    // EEPROM page count
    unsigned int vectors_end;	       // End of interrupt vector table
    enum dev_flags device_flags;       // See above.

    gdb_io_reg_def_type *io_reg_defs;

    bool is_xmega;                     // Device is an ATxmega part
    unsigned int fusemap;              // Bitmap of fuses supported (0b00000111 = 0x07 for
                                       // classic megaAVR devices, 0b00110111 = 0x37 for
                                       // Xmega devices
    unsigned int ocden_fuse;           // bitmask of the OCDEN fuse, bit [15:0] => high
                                       // fuse byte, so 0x00008000 for megaAVRs (Xmega
                                       // devices don't have OCDEN at all)

    unsigned char osccal;              // OSCCAL offset (XML param)
    unsigned char ocdrev;              // OCD revision (Studio 6 XML param)

    jtag1_device_desc_type dev_desc1;  // Device descriptor to download to
                                       // mkI device
    jtag2_device_desc_type dev_desc2;  // Device descriptor to download to
                                       // mkII device
    xmega_device_desc_type dev_desc3;  // Device descriptor to download for
                                       // Xmega devices in new (7+) firmware
                                       // JTAGICE mkII and AVR Dragon
	updi_device_desc dev_desc4;		   // Device descriptor to download to
									   // UPDI device
} jtag_device_def_type;

extern jtag_device_def_type deviceDefinitions[];

// various enums
enum
{
    // Constants common to both mkI and mkII

    // Address space offsets
    FLASH_SPACE_ADDR_OFFSET           = 0x000000,
    DATA_SPACE_ADDR_OFFSET            = 0x800000,

    EEPROM_SPACE_ADDR_OFFSET          = 0x810000,

    FUSE_SPACE_ADDR_OFFSET            = 0x820000,

    LOCK_SPACE_ADDR_OFFSET            = 0x830000,

    SIG_SPACE_ADDR_OFFSET             = 0x840000,

    REGISTER_SPACE_ADDR_OFFSET        = 0x850000, // Xmega only

    BREAKPOINT_SPACE_ADDR_OFFSET      = 0x900000,

    ADDR_SPACE_MASK = (DATA_SPACE_ADDR_OFFSET   |
                       EEPROM_SPACE_ADDR_OFFSET |
                       FUSE_SPACE_ADDR_OFFSET   |
                       LOCK_SPACE_ADDR_OFFSET   |
                       SIG_SPACE_ADDR_OFFSET    |
                       REGISTER_SPACE_ADDR_OFFSET |
                       BREAKPOINT_SPACE_ADDR_OFFSET),

    // JTAG communication timeouts, in microseconds
    // RESPONSE is for the first response byte
    // COMM is for subsequent response bytes
    MAX_JTAG_COMM_ATTEMPS	      = 10,
    MAX_JTAG_SYNC_ATTEMPS	      = 3,

    JTAG_RESPONSE_TIMEOUT	      = 1000000,
    JTAG_COMM_TIMEOUT		      = 100000,
    JTAG3_PIPE_TIMEOUT                = 100,

    MAX_FLASH_PAGE_SIZE               = 512,
    MAX_EEPROM_PAGE_SIZE              = 32,

    // JTAG ICE mkI protocol constants

    // Address space selector values
    ADDR_PROG_SPACE_PROG_ENABLED      = 0xB0,
    ADDR_PROG_SPACE_PROG_DISABLED     = 0xA0,
    ADDR_DATA_SPACE                   = 0x20,
    ADDR_EEPROM_SPACE                 = 0xB1,
    ADDR_FUSE_SPACE                   = 0xB2,
    ADDR_LOCK_SPACE                   = 0xB3,
    ADDR_SIG_SPACE                    = 0xB4,
    ADDR_BREAKPOINT_SPACE             = 0x60,

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

    // Set JTAG bitrate to 1MHz
    // ff: 1MHz, fe: 500kHz, fd: 250khz, fb: 125Khz
    // JTAG bitrates
    JTAG_BITRATE_1_MHz                = 0xff,
    JTAG_BITRATE_500_KHz              = 0xfe,
    JTAG_BITRATE_250_KHz              = 0xfd,
    JTAG_BITRATE_125_KHz              = 0xfb,

    // JTAG ICE mkII stuff goes here.  Most of this is straight from
    // AppNote AVR067.

    // Communication with the JTAG ICE works in frames.  The protocol
    // somewhat resembles the STK500v2 protocol, yet it is sufficiently
    // different to prevent a direct code reuse. :-(
    //
    // Frame format:
    //
    //  +---------------------------------------------------------------+
    //  |   0   |  1  .  2  |  3 . 4 . 5 . 6  |   7   | ... | N-1 .  N  |
    //  |       |           |                 |       |     |           |
    //  | start | LSB   MSB | LSB ....... MSB | token | msg | LSB   MSB |
    //  | 0x1B  | sequence# | message size    | 0x0E  |     |   CRC16   |
    //  +---------------------------------------------------------------+
    //
    // Each request message will be returned by a response with a matching
    // sequence #.  Sequence # 0xffff is reserved for asynchronous event
    // notifications that will be sent by the ICE without a request
    // message (e.g. when the target hit a breakpoint).
    //
    // The message size excludes the framing overhead (10 bytes).
    //
    // The first byte of the message is always the request or response
    // code, which is roughly classified as:
    //
    // . Messages (commands) use 0x00 through 0x3f.  (The documentation
    //   claims that messages start at 0x01, but actually CMND_SIGN_OFF is
    //   0x00.)
    // . Internal commands use 0x40 through 0x7f (not documented).
    // . Success responses use 0x80 through 0x9f.
    // . Failure responses use 0xa0 through 0xbf.
    // . Events use 0xe0 through 0xff.

    MESSAGE_START		= 0x1b,
    TOKEN			= 0x0e,

    // On the JTAGICE3, the USB reader framework modifies the token of
    // a received event into this so the upper layers can tell an event
    // message apart from a normal response message.
    TOKEN_EVT3			= 'T',

    // Max message size we are willing to accept.  Prevents us from trying
    // to allocate too much VM in case we received a nonsensical packet
    // length.  We have to allocate the buffer as soon as we've got the
    // length information (and thus have to trust that information by that
    // time at first), as the final CRC check can only be done once the
    // entire packet came it.
    MAX_MESSAGE			= 100000,

    // ICE command codes
    CMND_CHIP_ERASE		= 0x13,
    CMND_CLEAR_EVENTS		= 0x22,
    CMND_CLR_BREAK		= 0x1A,
    CMND_ENTER_PROGMODE		= 0x14,
    CMND_ERASEPAGE_SPM		= 0x0D,
    CMND_FORCED_STOP		= 0x0A,
    CMND_GET_BREAK		= 0x12,
    CMND_GET_PARAMETER		= 0x03,
    CMND_GET_SIGN_ON		= 0x01,
    CMND_GET_SYNC		= 0x0f,
    CMND_GO			= 0x08,
    CMND_LEAVE_PROGMODE		= 0x15,
    CMND_READ_MEMORY		= 0x05,
    CMND_READ_PC		= 0x07,
    CMND_RESET			= 0x0B,
    CMND_RESTORE_TARGET		= 0x23,
    CMND_RUN_TO_ADDR		= 0x1C,
    CMND_SELFTEST		= 0x10,
    CMND_SET_BREAK		= 0x11,
    CMND_SET_BREAK_XMEGA	= 0x37, // undocumented in AVR067
    CMND_SET_DEVICE_DESCRIPTOR	= 0x0C,
    CMND_SET_N_PARAMETERS	= 0x16,
    CMND_SET_XMEGA_PARAMS	= 0x36, // undocumented in AVR067
    CMND_SET_PARAMETER		= 0x02,
    CMND_SIGN_OFF		= 0x00,
    CMND_SINGLE_STEP		= 0x09,
    CMND_SPI_CMD		= 0x1D,
    CMND_WRITE_MEMORY		= 0x04,
    CMND_WRITE_PC		= 0x06,
    CMND_XMEGA_ERASE		= 0x34,

    // ICE responses
    RSP_DEBUGWIRE_SYNC_FAILED	= 0xAC,
    RSP_FAILED			= 0xA0,
    RSP_GET_BREAK		= 0x83,
    RSP_ILLEGAL_BREAKPOINT	= 0xA8,
    RSP_ILLEGAL_COMMAND		= 0xAA,
    RSP_ILLEGAL_EMULATOR_MODE	= 0xA4,
    RSP_ILLEGAL_JTAG_ID		= 0xA9,
    RSP_ILLEGAL_MCU_STATE	= 0xA5,
    RSP_ILLEGAL_MEMORY_TYPE	= 0xA2,
    RSP_ILLEGAL_MEMORY_RANGE	= 0xA3,
    RSP_ILLEGAL_PARAMETER	= 0xA1,
    RSP_ILLEGAL_POWER_STATE	= 0xAD,
    RSP_ILLEGAL_VALUE		= 0xA6,
    RSP_MEMORY			= 0x82,
    RSP_NO_TARGET_POWER		= 0xAB,
    RSP_OK			= 0x80,
    RSP_PARAMETER		= 0x81,
    RSP_PC			= 0x84,
    RSP_SELFTEST		= 0x85,
    RSP_SET_N_PARAMETERS	= 0xA7,
    RSP_SIGN_ON			= 0x86,
    RSP_SPI_DATA		= 0x88,

    // ICE events
    EVT_BREAK				= 0xE0,
    EVT_DEBUG				= 0xE6,
    EVT_ERROR_PHY_FORCE_BREAK_TIMEOUT	= 0xE2,
    EVT_ERROR_PHY_MAX_BIT_LENGTH_DIFF	= 0xED,
    EVT_ERROR_PHY_OPT_RECEIVE_TIMEOUT	= 0xF9,
    EVT_ERROR_PHY_OPT_RECEIVED_BREAK	= 0xFA,
    EVT_ERROR_PHY_RECEIVED_BREAK	= 0xF8,
    EVT_ERROR_PHY_RECEIVE_TIMEOUT	= 0xF7,
    EVT_ERROR_PHY_RELEASE_BREAK_TIMEOUT	= 0xE3,
    EVT_ERROR_PHY_SYNC_OUT_OF_RANGE	= 0xF5,
    EVT_ERROR_PHY_SYNC_TIMEOUT		= 0xF0,
    EVT_ERROR_PHY_SYNC_TIMEOUT_BAUD	= 0xF4,
    EVT_ERROR_PHY_SYNC_WAIT_TIMEOUT	= 0xF6,
    EVT_RESULT_PHY_NO_ACTIVITY		= 0xFB,
    EVT_EXT_RESET			= 0xE7,
    EVT_ICE_POWER_ERROR_STATE		= 0xEA,
    EVT_ICE_POWER_OK			= 0xEB,
    EVT_IDR_DIRTY			= 0xEC,
    EVT_NONE				= 0xEF,
    EVT_PDSB_BREAK			= 0xF2,
    EVT_PDSMB_BREAK			= 0xF3,
    EVT_PROGRAM_BREAK			= 0xF1,
    EVT_RUN				= 0xE1,
    EVT_TARGET_POWER_OFF		= 0xE5,
    EVT_TARGET_POWER_ON			= 0xE4,
    EVT_TARGET_SLEEP			= 0xE8,
    EVT_TARGET_WAKEUP			= 0xE9,
    // trailer
    EVT_MAX                             = 0xFF,

    // memory types for CMND_{READ,WRITE}_MEMORY
    MTYPE_IO_SHADOW	= 0x30,	// cached IO registers?
    MTYPE_SRAM		= 0x20,	// target's SRAM or [ext.] IO registers
    MTYPE_EEPROM	= 0x22,	// EEPROM, what way?
    MTYPE_EVENT		= 0x60,	// ICE event memory
    MTYPE_EVENT_COMPRESSED = 0x61, // ICE event memory, bit-mapped
    MTYPE_SPM		= 0xA0,	// flash through LPM/SPM
    MTYPE_FLASH_PAGE	= 0xB0,	// flash in programming mode
    MTYPE_EEPROM_PAGE	= 0xB1,	// EEPROM in programming mode
    MTYPE_FUSE_BITS	= 0xB2,	// fuse bits in programming mode
    MTYPE_LOCK_BITS	= 0xB3,	// lock bits in programming mode
    MTYPE_SIGN_JTAG	= 0xB4,	// signature in programming mode
    MTYPE_OSCCAL_BYTE	= 0xB5,	// osccal cells in programming mode
    MTYPE_CAN		= 0xB6,	// CAN mailbox
    MTYPE_XMEGA_REG	= 0xB8,	// Xmega CPU registers
    MTYPE_XMEGA_APP_FLASH = 0xC0, // Xmega application flash

    // (some) ICE parameters, for CMND_{GET,SET}_PARAMETER
    PAR_HW_VERSION		= 0x01,
    PAR_FW_VERSION		= 0x02,

    PAR_EMULATOR_MODE		= 0x03,
    EMULATOR_MODE_DEBUGWIRE	= 0x00,
    EMULATOR_MODE_JTAG		= 0x01,
    EMULATOR_MODE_UNKNOWN	= 0x02,
    EMULATOR_MODE_SPI		= 0x03,
    EMULATOR_MODE_JTAG_XMEGA	= 0x05,
    EMULATOR_MODE_PDI		= 0x06,

    PAR_IREG			= 0x04,

    PAR_BAUD_RATE		= 0x05,
    PAR_BAUD_2400		= 0x01,
    PAR_BAUD_4800		= 0x02,
    PAR_BAUD_9600		= 0x03,
    PAR_BAUD_19200		= 0x04,	// default
    PAR_BAUD_38400		= 0x05,
    PAR_BAUD_57600		= 0x06,
    PAR_BAUD_115200		= 0x07,
    PAR_BAUD_14400		= 0x08,

    PAR_OCD_VTARGET		= 0x06,
    PAR_OCD_JTAG_CLK		= 0x07,
    PAR_OCD_BREAK_CAUSE		= 0x08,
    PAR_TIMERS_RUNNING		= 0x09,
    PAR_BREAK_ON_CHANGE_FLOW	= 0x0A,
    PAR_BREAK_ADDR1		= 0x0B,
    PAR_BREAK_ADDR2		= 0x0C,
    PAR_COMBBREAKCTRL		= 0x0D,
    PAR_JTAGID			= 0x0E,
    PAR_UNITS_BEFORE		= 0x0F,
    PAR_UNITS_AFTER		= 0x10,
    PAR_BIT_BEFORE		= 0x11,
    PAR_BIT_ATER		= 0x12,
    PAR_EXTERNAL_RESET		= 0x13,
    PAR_FLASH_PAGE_SIZE		= 0x14,
    PAR_EEPROM_PAGE_SIZE	= 0x15,
    PAR_UNUSED1			= 0x16,
    PAR_PSB0			= 0x17,
    PAR_PSB1			= 0x18,
    PAR_PROTOCOL_DEBUG_EVENT	= 0x19,

    PAR_MCU_STATE		= 0x1A,
    STOPPED			= 0x00,
    RUNNING			= 0x01,
    PROGRAMMING			= 0x02,

    PAR_DAISY_CHAIN_INFO	= 0x1B,
    PAR_BOOT_ADDRESS		= 0x1C,
    PAR_TARGET_SIGNATURE	= 0x1D,
    PAR_DEBUGWIRE_BAUDRATE	= 0x1E,
    PAR_PROGRAM_ENTRY_POINT	= 0x1F,
    PAR_PACKET_PARSING_ERRORS	= 0x40,
    PAR_VALID_PACKETS_RECEIVED	= 0x41,
    PAR_INTERCOMMUNICATION_TX_FAILURES = 0x42,
    PAR_INTERCOMMUNICATION_RX_FAILURES = 0x43,
    PAR_CRC_ERRORS		= 0x44,

    PAR_POWER_SOURCE		= 0x45,
    POWER_EXTERNAL		= 0x00,
    POWER_USB			= 0x01,

    PAR_CAN_FLAG		= 0x22,
    DONT_READ_CAN_MAILBOX	= 0x00,
    READ_CAN_MAILBOX		= 0x01,

    PAR_ENABLE_IDR_IN_RUN_MODE	= 0x23,
    ACCESS_OSCCAL		= 0x00,
    ACCESS_IDR			= 0x01,

    PAR_ALLOW_PAGEPROGRAMMING_IN_SCANCHAIN = 0x24,
    PAGEPROG_NOT_ALLOWED	= 0x00,
    PAGEPROG_ALLOWED		= 0x01,

    /* EDBG related constants */
    EDBG_VENDOR_AVR_CMD		= 0x80,
    EDBG_VENDOR_AVR_RSP		= 0x81,
    EDBG_VENDOR_AVR_EVT		= 0x82,

};

enum {
    PC_INVALID			      = 0xffffffff
};

/*
 * JTAG ICE mkII breakpoints are quite tricky.
 *
 * There are four possible breakpoint slots in JTAG.  The first one is
 * always reserved for single-stepping, and cannot be accessed
 * directly.  The second slot (ID #1) can be freely used as a code
 * breakpoint (only).  The third and fourth slot can either be used as
 * a code breakpoint, as an independent (byte-wide) data breakpoint
 * (i.e. a watchpoint in GDB terms), or together as a data breakpoint
 * consisting of an address and a mask.  The latter does not match
 * directly to GDB watchpoints (of a certain lenght > 1) but imposes
 * the additional requirement that the base address be aligned
 * properly wrt. the mask.
 *
 * The single-step breakpoint can indirectly be used by filling the
 * respective high-level language information into the event memory,
 * and issuing a high-level "step over" single-step.  As GDB does not
 * install the breakpoints for "stepi"-style single-steps (which would
 * require the very same JTAG breakpoint register), this ought to
 * work.
 *
 * Finally, there are software breakpoints where the respective
 * instruction will be replaced by a BREAK instruction in flash ROM by
 * means of an SPM call.  Some devices do not allow for this (as they
 * are said to be broken), and in general as this method contributes
 * to flash wear, we rather do not uninstall and reinstall these
 * breakpoints each time GDB asks for it, but instead "cache" them
 * locally, and only delete and add them as they actually change.
 *
 * To add to the mess, GDB's remote protocol isn't very smart about
 * telling us whether the next list of breakpoints + resume are
 * actually meant to be a high-level language (HLL) single-step
 * command.  Thus, always assume the last breakpoint that comes in to
 * be a single-step breakpoint, and replace the "resume" operation by
 * a "step" one in that case.  At least, GDB issues the breakpoint
 * list in the order of breakpoint numbers, so any possible HLL
 * single-step breakoint must be the last one in the list.
 *
 * Finally, GDB has explicit commands for setting hardware-assisted
 * breakpoints, but the default "break" command uses a software
 * breakpoint.  We try to replace as many software breakpoints as
 * possible by hardware breakpoints though, for the sake of
 * efficiency, yet would want to respect the user's choice for setting
 * a hardware breakpoint ("hbreak" or "thbreak")...
 *
 * XXX This is not done yet.
 */

enum bpType
{
    NONE,           // disabled.
    CODE,           // normal code space breakpoint.
    WRITE_DATA,     // write data space breakpoint (ie "watch").
    READ_DATA,      // read data space breakpoint (ie "watch").
    ACCESS_DATA,    // read/write data space breakpoint (ie "watch").
    DATA_MASK,      // mask for data space breakpoint.
};

enum {
  // We distinguish the total possible breakpoints and those for each type
  // (code or data) - see above
  MAX_BREAKPOINTS2_CODE = 4,
  MAX_BREAKPOINTS2_DATA = 2,
  MAX_BREAKPOINTS2 = 4,
  // various slot #s
  BREAKPOINT2_XMEGA_UNAVAIL = 1,
  BREAKPOINT2_FIRST_DATA = 2,
  BREAKPOINT2_DATA_MASK = 3,

  MAX_TOTAL_BREAKPOINTS2 = 255
};

struct breakpoint2
{
    // High-level information on breakpoint
    unsigned int address;
    unsigned int mask_pointer;
    bpType type;
    bool enabled;

    // Used to flag end of list
    bool last;

    // Low-level information on breakpoint
    bool icestatus; // Status of breakpoint in ICE itself: 'true'
                    // when is enabled in ACTUAL device
    bool toremove;  // Delete this guy in ICE
    bool toadd;     // Add this guy in ICE
    uchar bpnum;    // ICE's breakpoint number (0x00 for software)
    bool has_mask;  // data watchpoint has a mask associated
};

const struct breakpoint2 default_bp =
{
    0,				/* address */
    0,				/* mask_pointer */
    NONE,			/* type */
    false,			/* enabled */
    true,			/* last */
    false,			/* icestatus */
    false,			/* toremove */
    false,			/* toadd */
    0,				/* bpnum*/
    false,			/* has_mask */
};

// Enumerations for target memory type.
typedef enum {
    MEM_FLASH = 0,
    MEM_EEPROM = 1,
    MEM_RAM = 2,
} BFDmemoryType;

extern const char *BFDmemoryTypeString[];
extern const int BFDmemorySpaceOffset[];

enum emulator {
  EMULATOR_JTAGICE,
  EMULATOR_DRAGON,
  EMULATOR_JTAGICE3,
  EMULATOR_EDBG
};


// Allocate 1 meg for image buffer. This is where the file data is
// stored before writing occurs.
#define MAX_IMAGE_SIZE 1000000


typedef struct {
    uchar val;
    bool  used;
} AVRMemoryByte;


// Struct that holds the memory image. We read from file using BFD
// into this struct, then pass the entire struct to the target writer.
typedef struct {
    AVRMemoryByte image[MAX_IMAGE_SIZE];
    unsigned int last_address;
    unsigned int first_address;
    bool first_address_ok;
    bool has_data;
    const char *name;
} BFDimage;


// The Sync_CRC/EOP message terminator (no real CRC in sight...)
#define JTAG_EOM 0x20, 0x20

enum debugproto {
    PROTO_JTAG, PROTO_DW, PROTO_PDI, PROTO_UPDI
};

class jtag
{
  protected:
  // The initial serial port parameters. We restore them on exit.
  struct termios oldtio;
  bool oldtioValid;

  // The file descriptor used while talking to the JTAG ICE
  int jtagBox;

  // For the mkII device, is the box attached via USB?
  bool is_usb;

  // The type of our emulator: JTAG ICE, or AVR Dragon.
  emulator emu_type;

  // Whether nSRST is to be applied when connecting (override JTD bit).
  bool apply_nSRST;

  // Total breakpoints including software
  breakpoint2 bp[MAX_TOTAL_BREAKPOINTS2];

  // Xmega hard breakpoing break handling
  unsigned int xmega_n_bps;
  unsigned long xmega_bps[2];

  // This device or connection cannot handle hard BPs
  bool softbp_only;

  // Target device is an ATxmega one
  bool is_xmega;

  public:
  // Whether we are in "programming mode" (changes how program memory
  // is written, apparently)
  bool programmingEnabled;

  // Name of the device controlled by the JTAG ICE
  char *device_name;

  // Pointer to device definition
  jtag_device_def_type *deviceDef;

  // Daisy chain info
  struct {
    unsigned char units_before;
    unsigned char units_after;
    unsigned char bits_before;
    unsigned char bits_after;
  } dchain;

  protected:
  void openUSB(const char *jtagDeviceName);
  void resetUSB(void);
  int safewrite(const void *b, int count);
  void changeLocalBitRate(int newBitRate);
  void restoreSerialPort(void);

  virtual void changeBitRate(int newBitRate) = 0;
  virtual void setDeviceDescriptor(jtag_device_def_type *dev) = 0;
  virtual bool synchroniseAt(int bitrate) = 0;
  virtual void startJtagLink(void) = 0;
  virtual void deviceAutoConfig(void) = 0;
  void jtag_flash_image(BFDimage *image, BFDmemoryType memtype,
			bool program, bool verify);
  // Return page address of
  unsigned int page_addr(unsigned int addr, BFDmemoryType memtype)
  {
    unsigned int page_size = get_page_size( memtype );
    return (unsigned int)(addr & (~(page_size - 1)));
  };

  unsigned int get_page_size(BFDmemoryType memtype);

  public:
  jtag(void);
  jtag(const char *dev, char *name, emulator type = EMULATOR_JTAGICE);
  virtual ~jtag(void);

  // Basic JTAG I/O
  // -------------

  /** Send initial configuration to setup the JTAG box itself. 
   **/
  virtual void initJtagBox(void) = 0;

  /**
     Send initial configuration to the JTAG box when starting a new
     debug session.  (Note: when attaching to a running target, fuse
     bits cannot be set so debugging must have been enabled earlier)

     The bitrate sets the JTAG bitrate. The bitrate must be less than
     1/4 that of the target avr frequency or the jtagice will have
     problems reading from the target. The problems are usually
     manifested as failed calls to jtagRead().
  **/
  virtual void initJtagOnChipDebugging(unsigned long bitrate) = 0;


  /** A timed-out read from file descriptor 'fd'.

  'timeout' is in microseconds, it is the maximum interval within which
  the read must make progress (i.e., it's a per-byte timeout)

  Returns the number of bytes read or -1 for errors other than timeout.

  Note: EOF and timeout cannot be distinguished
  **/
  int timeout_read(void *buf, size_t count, unsigned long timeout);

  // Breakpoints
  // -----------

  /** Clear out the breakpoints. */
  virtual void deleteAllBreakpoints(void) = 0;

  /** Delete breakpoint at the specified address. */
  virtual bool deleteBreakpoint(unsigned int address, bpType type, unsigned int length);

  /** Add a code breakpoint at the specified address. */
  virtual bool addBreakpoint(unsigned int address, bpType type, unsigned int length);

  bool layoutBreakpoints(void);

  /** Send the breakpoint details down to the JTAG box. */
  virtual void updateBreakpoints(void) = 0;

  /** True if there is a breakpoint at address */
  virtual bool codeBreakpointAt(unsigned int address) = 0;

  /** Parse a list of event names to *not* cause a break. */
  virtual void parseEvents(const char *) = 0;

  // Writing to program memory
  // -------------------------

  /** Switch to faster programming mode, allows chip erase */
  virtual void enableProgramming(void) = 0;

  /** Switch back to normal programming mode **/
  virtual void disableProgramming(void) = 0;

  /** Erase all chip memory **/
  virtual void eraseProgramMemory(void) = 0;

  virtual void eraseProgramPage(unsigned long address) = 0;

  /** Download an image contained in the specified file. */
  virtual void downloadToTarget(const char* filename, bool program, bool verify) = 0;

  // Running, single stepping, etc
  // -----------------------------

  /** Retrieve the current Program Counter value, or PC_INVALID if fails */
  virtual unsigned long getProgramCounter(void) = 0;

  /** Set program counter to 'pc' **/
  virtual void setProgramCounter(unsigned long pc) = 0;

  /** Reset AVR. **/
  virtual void resetProgram(bool possible_nSRST) = 0;

  /** Interrupt AVR. **/
  virtual void interruptProgram(void) = 0;

  /** Resume program execution.
      Note: the gdb 'continue' command is handled by jtagContinue,
      this is just the low level command to resume after interruptProgram
  **/
  virtual void resumeProgram(void) = 0;

  /** Issue a "single step" command to the JTAG box.
  **/
  virtual void jtagSingleStep(void) = 0;

  /** Send the program on it's merry way, and wait for a breakpoint or
      input from gdb.
      Return true for a breakpoint, false for gdb input. **/
  virtual bool jtagContinue(void) = 0;

  // R/W memory
  // ----------

  /** Read 'numBytes' from target memory address 'addr'.

    The memory space is selected by the high order bits of 'addr' (see
    above).

    Returns a dynamically allocated buffer with the requested bytes if
    successful (caller must free), or NULL if the read failed.
  **/
  virtual uchar *jtagRead(unsigned long addr, unsigned int numBytes) = 0;

  /** Write 'numBytes' bytes from 'buffer' to target memory address 'addr'

    The memory space is selected by the high order bits of 'addr' (see
    above).

    Returns true for a successful write, false for a failed one.

    Note: The current behaviour for program-space writes is pretty
    weird (does not match jtagRead). See comments in jtagrw.cc.
  **/
  virtual void jtagWrite(unsigned long addr, unsigned int numBytes, uchar buffer[]) = 0;


  /** Write fuses to target.

    The input parameter is a string from command-line, as produced by
    printf("%x", 0xaabbcc );
  **/
  void jtagWriteFuses(char *fuses);


  /** Read fuses from target.

    Shows extended, high and low fuse byte.
  */
  void jtagReadFuses(void);


  /** Display fuses.

    Shows extended, high and low fuse byte.
  */
  void jtagDisplayFuses(uchar *fuseBits);


  /** Activate OCDEN.
   */
  void jtagActivateOcdenFuse(void);


  /** Write lockbits to target.

    The input parameter is a string from command-line, as produced by
    printf("%x", 0xaa);
  **/
  void jtagWriteLockBits(char *lock);


  /** Read the lock bits from the target and display them.
   **/
  void jtagReadLockBits(void);


  /** Display lockbits.

    Shows raw value and individual bits.
  **/
  void jtagDisplayLockBits(uchar *lockBits);

  /** Return read address for CPU status area.

    Returns the address to read the CPU status area (SPL, SPH, SREG)
    from.  0x5D + DATA_SPACE_ADDR_OFFSET for megaAVR, 0x3D +
    DATA_SPACE_ADDR_OFFSET for Xmega devices.
  **/
  virtual unsigned int statusAreaAddress(void) const = 0;

  /** Return read address for CPU registers.

    Returns the address to read the CPU registers (r0 through r31).
    DATA_SPACE_ADDR_OFFSET for megaAVR, REGISTER_SPACE_ADDR_OFFSET
    for Xmega devices.

  **/
  virtual unsigned int cpuRegisterAreaAddress(void) const = 0;

};

class jtag_exception: public exception
{
  protected:
    const char *reason;
  public:
    jtag_exception()
    {
        reason =  "JTAG exception";
    }
    jtag_exception(const char *r)
    {
        reason = r;
    }
    virtual const char * what() const throw()
    {
        return reason;
    }
};

class jtag_timeout_exception: public jtag_exception
{
  public:
    jtag_timeout_exception(): jtag_exception("JTAG ICE timeout exception") {}
};

class jtag_io_exception: public jtag_exception
{
  protected:
    unsigned int response_code;

  public:
    jtag_io_exception(): jtag_exception("Unknown JTAG response exception")
    {
        response_code = 0;
    }
    jtag_io_exception(unsigned int code);

    unsigned int get_response(void) { return response_code; }
};


extern class jtag *theJtagICE;

#endif
