#include "../jtag.h"

namespace {

constexpr gdb_io_reg_def_type atmega406_io_registers[] =
    {
        { "PINA", 0x20, 0x00 },
        { "DDRA", 0x21, 0x00 },
        { "PORTA", 0x22, 0x00 },
        { "PINB", 0x23, 0x00 },
        { "DDRB", 0x24, 0x00 },
        { "PORTB", 0x25, 0x00 },
        { "PORTC", 0x28, 0x00 },
        { "PIND", 0x29, 0x00 },
        { "DDRD", 0x2a, 0x00 },
        { "PORTD", 0x2b, 0x00 },
        { "TIFR0", 0x35, 0x00 },
        { "TIFR1", 0x36, 0x00 },
        { "PCIFR", 0x3b, 0x00 },
        { "EIFR", 0x3c, 0x00 },
        { "EIMSK", 0x3d, 0x00 },
        { "GPIOR0", 0x3e, 0x00 },
        { "EECR", 0x3f, 0x00 },
        { "EEDR", 0x40, 0x00 },
        { "EEARL", 0x41, 0x00 },
        { "EEARH", 0x42, 0x00 },
        { "GTCCR", 0x43, 0x00 },
        { "TCCR0A", 0x44, 0x00 },
        { "TCCR0B", 0x45, 0x00 },
        { "TCNT0", 0x46, 0x00 },
        { "OCR0A", 0x47, 0x00 },
        { "OCR0B", 0x48, 0x00 },
        { "GPIOR1", 0x4a, 0x00 },
        { "GPIOR2", 0x4b, 0x00 },
        { "OCDR", 0x51, 0x00 },
        { "SMCR", 0x53, 0x00 },
        { "MCUSR", 0x54, 0x00 },
        { "MCUCR", 0x55, 0x00 },
        { "SPMCSR", 0x57, 0x00 },
        { "SPL", 0x5d, 0x00 },
        { "SPH", 0x5e, 0x00 },
        { "SREG", 0x5f, 0x00 },
        { "WDTCSR", 0x60, 0x00 },
        { "WUTCSR", 0x62, 0x00 },
        { "PRR0", 0x64, 0x00 },
        { "FOSCCAL", 0x66, 0x00 },
        { "PCICR", 0x68, 0x00 },
        { "EICRA", 0x69, 0x00 },
        { "PCMSK0", 0x6b, 0x00 },
        { "PCMSK1", 0x6c, 0x00 },
        { "TIMSK0", 0x6e, 0x00 },
        { "TIMSK1", 0x6f, 0x00 },
        { "VADCL", 0x78, 0x00 },
        { "VADCH", 0x79, 0x00 },
        { "VADCSR", 0x7a, 0x00 },
        { "VADMUX", 0x7c, 0x00 },
        { "DIDR0", 0x7e, 0x00 },
        { "TCCR1B", 0x81, 0x00 },
        { "TCNT1L", 0x84, 0x00 },
        { "TCNT1H", 0x85, 0x00 },
        { "OCR1AL", 0x88, 0x00 },
        { "OCR1AH", 0x89, 0x00 },
        { "TWBR", 0xb8, 0x00 },
        { "TWSR", 0xb9, 0x00 },
        { "TWAR", 0xba, 0x00 },
        { "TWDR", 0xbb, 0x00 },
        { "TWCR", 0xbc, 0x00 },
        { "TWAMR", 0xbd, 0x00 },
        { "TWBCSR", 0xbe, 0x00 },
        { "CCSR", 0xc0, 0x00 },
        { "BGCCR", 0xd0, 0x00 },
        { "BGCRR", 0xd1, 0x00 },
        { "CADAC0", 0xe0, 0x00 },
        { "CADAC1", 0xe1, 0x00 },
        { "CADAC2", 0xe2, 0x00 },
        { "CADAC3", 0xe3, 0x00 },
        { "CADCSRA", 0xe4, 0x00 },
        { "CADCSRB", 0xe5, 0x00 },
        { "CADRCC", 0xe6, 0x00 },
        { "CADRDC", 0xe7, 0x00 },
        { "CADICL", 0xe8, 0x00 },
        { "CADICH", 0xe9, 0x00 },
        { "FCSR", 0xf0, 0x00 },
        { "CBCR", 0xf1, 0x00 },
        { "BPIR", 0xf2, 0x00 },
        { "BPDUV", 0xf3, 0x00 },
        { "BPSCD", 0xf4, 0x00 },
        { "BPOCD", 0xf5, 0x00 },
        { "CBPTR", 0xf6, 0x00 },
        { "BPCR", 0xf7, 0x00 },
        { "BPPLR", 0xf8, 0x00 },
        { nullptr, 0, 0 }
};

[[maybe_unused]] const jtag_device_def_type device{
    "atmega406",
    0x9507,
    128,
    320, // 40960 bytes flash
    4,
    128,    // 512 bytes EEPROM
    23 * 4, // 23 interrupt vectors
    NO_TWEAKS,
    atmega406_io_registers,
    0x07,
    0x0200, // fuses
    0,      // osccal
    0,      // OCD revision
    nullptr,
    {
        CMND_SET_DEVICE_DESCRIPTOR,
        {0x3F, 0x0F, 0x60, 0xF8, 0xFF, 0x0D, 0xB8, 0xE0}, // ucReadIO
        {0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00}, // ucReadIOShadow
        {0x37, 0x0F, 0x00, 0xE0, 0xFF, 0x0D, 0xB8, 0xE0}, // ucWriteIO
        {0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00}, // ucWriteIOShadow
        {0x55, 0xDB, 0x00, 0x57, 0x32, 0x03, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x7F, 0x01, 0x00, 0x03, 0x00, 0xFF, 0x03, 0xFF, 0x01}, // ucReadExtIO
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // ucReadIOExtShadow
        {0x50, 0xDB, 0x00, 0x50, 0x32, 0x03, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x6D, 0x01, 0x00, 0x03, 0x00, 0xD0, 0x00, 0xFB, 0x00}, // ucWriteExtIO
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // ucWriteIOExtShadow
        0x31,                                                         // ucIDRAddress
        0X57,                                                         // ucSPMCRAddress
        0,                                                            // ucRAMPZAddress
        128,                                                 // uiFlashPageSize
        4,                                                            // ucEepromPageSize
        0x4F00,                                              // ulBootAddress
        0xF8,                                                // uiUpperExtIOLoc
        40960,                                               // ulFlashSize
        {0x00},                                                       // ucEepromInst
        {0x00},                                                       // ucFlashInst
        0x3E,                                                         // ucSPHaddr
        0x3D,                                                         // ucSPLaddr
        40960 / 128,                                         // uiFlashpages
        0x00,                                                         // ucDWDRAddress
        0x00,                                                         // ucDWBasePC
        0x00,                                                         // ucAllowFullPageBitstream
        0x00,  // uiStartSmallestBootLoaderSection
        1,              // EnablePageProgramming
        0x00,           // ucCacheType
        0x100, // uiSramStartAddr
        0,              // ucResetType
        0,              // ucPCMaskExtended
        0,              // ucPCMaskHigh
        0,              // ucEindAddress
        0x1F,  // EECRAddress
    },
    nullptr
};

}
