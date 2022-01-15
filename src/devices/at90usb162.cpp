#include "../jtag.h"

namespace {

constexpr gdb_io_reg_def_type at90usb162_io_registers[] = {
    {"PINB", 0x23, 0x00},    {"DDRB", 0x24, 0x00},       {"PORTB", 0x25, 0x00},
    {"PINC", 0x26, 0x00},    {"DDRC", 0x27, 0x00},       {"PORTC", 0x28, 0x00},
    {"PIND", 0x29, 0x00},    {"DDRD", 0x2a, 0x00},       {"PORTD", 0x2b, 0x00},
    {"TIFR0", 0x35, 0x00},   {"TIFR1", 0x36, 0x00},      {"PCIFR", 0x3b, 0x00},
    {"EIFR", 0x3c, 0x00},    {"EIMSK", 0x3d, 0x00},      {"GPIOR0", 0x3e, 0x00},
    {"EECR", 0x3f, 0x00},    {"EEDR", 0x40, 0x00},       {"EEARL", 0x41, 0x00},
    {"EEARH", 0x42, 0x00},   {"GTCCR", 0x43, 0x00},      {"TCCR0A", 0x44, 0x00},
    {"TCCR0B", 0x45, 0x00},  {"TCNT0", 0x46, 0x00},      {"OCR0A", 0x47, 0x00},
    {"OCR0B", 0x48, 0x00},   {"PLLCSR", 0x49, 0x00},     {"GPIOR1", 0x4a, 0x00},
    {"GPIOR2", 0x4b, 0x00},  {"SPCR", 0x4c, 0x00},       {"SPSR", 0x4d, 0x00},
    {"SPDR", 0x4e, 0x00},    {"ACSR", 0x50, 0x00},       {"DWDR", 0x51, 0x00},
    {"SMCR", 0x53, 0x00},    {"MCUSR", 0x54, 0x00},      {"MCUCR", 0x55, 0x00},
    {"SPMCSR", 0x57, 0x00},  {"SPL", 0x5d, 0x00},        {"SPH", 0x5e, 0x00},
    {"SREG", 0x5f, 0x00},    {"WDTCSR", 0x60, 0x00},     {"CLKPR", 0x61, 0x00},
    {"WDTCKD", 0x62, 0x00},  {"REGCR", 0x63, 0x00},      {"PRR0", 0x64, 0x00},
    {"PRR1", 0x65, 0x00},    {"OSCCAL", 0x66, 0x00},     {"PCICR", 0x68, 0x00},
    {"EICRA", 0x69, 0x00},   {"EICRB", 0x6a, 0x00},      {"PCMSK0", 0x6b, 0x00},
    {"PCMSK1", 0x6c, 0x00},  {"TIMSK0", 0x6e, 0x00},     {"TIMSK1", 0x6f, 0x00},
    {"TCCR1A", 0x80, 0x00},  {"TCCR1B", 0x81, 0x00},     {"TCCR1C", 0x82, 0x00},
    {"TCNT1L", 0x84, 0x00},  {"TCNT1H", 0x85, 0x00},     {"ICR1L", 0x86, 0x00},
    {"ICR1H", 0x87, 0x00},   {"OCR1AL", 0x88, 0x00},     {"OCR1AH", 0x89, 0x00},
    {"OCR1BL", 0x8a, 0x00},  {"OCR1BH", 0x8b, 0x00},     {"OCR1CL", 0x8c, 0x00},
    {"OCR1CH", 0x8d, 0x00},  {"UCSR1A", 0xc8, 0x00},     {"UCSR1B", 0xc9, 0x00},
    {"UCSR1C", 0xca, 0x00},  {"UCSR1D", 0xcb, 0x00},     {"UBRR1L", 0xcc, 0x00},
    {"UBRR1H", 0xcd, 0x00},  {"UDR1", 0xce, IO_REG_RSE}, {"CKSEL0", 0xd0, 0x00},
    {"CKSEL1", 0xd1, 0x00},  {"CKSTA", 0xd2, 0x00},      {"USBCON", 0xd8, 0x00},
    {"UDPADDL", 0xdb, 0x00}, {"UDPADDH", 0xdc, 0x00},    {"UDCON", 0xe0, 0x00},
    {"UDINT", 0xe1, 0x00},   {"UDIEN", 0xe2, 0x00},      {"UDADDR", 0xe3, 0x00},
    {"UDFNUML", 0xe4, 0x00}, {"UDFNUMH", 0xe5, 0x00},    {"UDMFN", 0xe6, 0x00},
    {"UEINTX", 0xe8, 0x00},  {"UENUM", 0xe9, 0x00},      {"UERST", 0xea, 0x00},
    {"UECONX", 0xeb, 0x00},  {"UECFG0X", 0xec, 0x00},    {"UECFG1X", 0xed, 0x00},
    {"UESTA0X", 0xee, 0x00}, {"UESTA1X", 0xef, 0x00},    {"UEIENX", 0xf0, 0x00},
    {"UEDATX", 0xf1, 0x00},  {"UEBCLX", 0xf2, 0x00},     {"UEINT", 0xf4, 0x00},
    {"PS2CON", 0xfa, 0x00},  {"UPOE", 0xfb, 0x00},       {nullptr, 0, 0}};

[[maybe_unused]] const jtag_device_def_type at90usb162{
    "at90usb162",
    0x9482,
    128,
    128, // 16384 bytes flash
    4,
    128,    // 512 bytes EEPROM
    38 * 4, // 38 interrupt vectors
    DEVFL_MKII_ONLY,
    at90usb162_io_registers,
    0x07,
    0x0000, // fuses
    0x66,   // osccal
    1,      // OCD revision
    {
        0 // no mkI support
    },
    {
        CMND_SET_DEVICE_DESCRIPTOR,
        {0xF8, 0x0F, 0x60, 0xF8, 0xFF, 0x3F, 0xB9, 0xF0}, // ucReadIO
        {0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00}, // ucReadIOShadow
        {0xB0, 0x0D, 0x00, 0xE0, 0xFF, 0x1F, 0xB8, 0xF0}, // ucWriteIO
        {0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00}, // ucWriteIOShadow
        {0x7F, 0xDF, 0x00, 0x00, 0xF7, 0x3F, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x3F, 0x07, 0x01, 0x7F, 0xFF, 0x15, 0x0C}, // ucReadExtIO
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // ucReadIOExtShadow
        {0x34, 0xDF, 0x00, 0xC8, 0xF7, 0x3F, 0x40, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x7F, 0x03, 0x01, 0x0F, 0x7F, 0x11, 0x0C}, // ucWriteExtIO
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // ucWriteIOExtShadow
        0x00,                                                         // ucIDRAddress
        0x00,                                                         // ucSPMCRAddress
        0x3B,                                                         // ucRAMPZAddress
        fill_b2(128),                                                 // uiFlashPageSize
        4,                                                            // ucEepromPageSize
        fill_b4(0x1F00),                                              // ulBootAddress
        fill_b2(0xFB),                                                // uiUpperExtIOLoc
        fill_b4(16384),                                               // ulFlashSize
        {0xBD, 0xF2, 0xBD, 0xE1, 0xBB, 0xCF, 0xB4, 0x00, 0xBE, 0x01,
         0xB6, 0x01, 0xBC, 0x00, 0xBB, 0xBF, 0x99, 0xF9, 0xBB, 0xAF}, // ucEepromInst
        {0xB6, 0x01, 0x11},                                           // ucFlashInst
        0x3E,                                                         // ucSPHaddr
        0x3D,                                                         // ucSPLaddr
        fill_b2(16384 / 128),                                         // uiFlashpages
        0x31,                                                         // ucDWDRAddress
        0x00,                                                         // ucDWBasePC
        0x00,                                                         // ucAllowFullPageBitstream
        fill_b2(0x1F00), // uiStartSmallestBootLoaderSection
        1,               // EnablePageProgramming
        0,               // ucCacheType
        fill_b2(0x100),  // uiSramStartAddr
        0,               // ucResetType
        0,               // ucPCMaskExtended
        0,               // ucPCMaskHigh
        0x3C,            // ucEindAddress
        fill_b2(0x1F),   // EECRAddress
    },
    nullptr
};

} // namespace
