#include "../jtag.h"

namespace {

constexpr gdb_io_reg_def_type attiny13_io_registers[] = {
    {"ADCSRB", 0x23, 0x00}, {"ADCL", 0x24, IO_REG_RSE},    {"ADCH", 0x25, IO_REG_RSE},
    {"ADCSRA", 0x26, 0x00}, {"ADMUX", 0x27, 0x00},         {"ACSR", 0x28, 0x00},
    {"DIDR0", 0x34, 0x00},  {"PCMSK", 0x35, 0x00},         {"PINB", 0x36, 0x00},
    {"DDRB", 0x37, 0x00},   {"PORTB", 0x38, 0x00},         {"EECR", 0x3c, 0x00},
    {"EEDR", 0x3d, 0x00},   {"EEAR -- EEARL", 0x3e, 0x00}, {"WDTCR", 0x41, 0x00},
    {"CLKPR", 0x46, 0x00},  {"GTCCR", 0x48, 0x00},         {"OCR0B", 0x49, 0x00},
    {"DWDR", 0x4e, 0x00},   {"TCCR0A", 0x4f, 0x00},        {"OSCCAL", 0x51, 0x00},
    {"TCNT0", 0x52, 0x00},  {"TCCR0B", 0x53, 0x00},        {"MCUSR", 0x54, 0x00},
    {"MCUCR", 0x55, 0x00},  {"OCR0A", 0x56, 0x00},         {"SPMCSR", 0x57, 0x00},
    {"TIFR0", 0x58, 0x00},  {"TIMSK0", 0x59, 0x00},        {"GIFR", 0x5a, 0x00},
    {"GIMSK", 0x5b, 0x00},  {"SPL -- SP", 0x5d, 0x00},     {"SREG", 0x5f, 0x00},
    {nullptr, 0, 0}};

[[maybe_unused]] const jtag_device_def_type attiny13{
    "attiny13",
    0x9007,
    32,
    32, // 1024 bytes flash
    1,
    64,     // 64 bytes EEPROM
    10 * 2, // 10 interrupt vectors
    NO_TWEAKS,
    attiny13_io_registers,
    0x03,
    0x0000, // fuses
    0x51,   // osccal
    0,      // OCD revision
    nullptr,
    {
        CMND_SET_DEVICE_DESCRIPTOR,
        {0xF8, 0x01, 0xF0, 0x71, 0x42, 0x83, 0xFE, 0xAF}, // ucReadIO
        {0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00}, // ucReadIOShadow
        {0x88, 0x00, 0xB0, 0x71, 0x00, 0x83, 0x7C, 0xAA}, // ucWriteIO
        {0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00}, // ucWriteIOShadow
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // ucReadExtIO
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // ucReadIOExtShadow
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // ucWriteExtIO
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // ucWriteIOExtShadow
        0x00,                                                         // ucIDRAddress
        0X00,                                                         // ucSPMCRAddress
        0,                                                            // ucRAMPZAddress
        fill_b2(32),                                                  // uiFlashPageSize
        1,                                                            // ucEepromPageSize
        fill_b4(0x0000),                                              // ulBootAddress
        fill_b2(0x00),                                                // uiUpperExtIOLoc
        fill_b4(1024),                                                // ulFlashSize
        {0xBB, 0xFE, 0xBB, 0xEE, 0xBB, 0xCC, 0xB2, 0x0D, 0xBC, 0x0E,
         0xB4, 0x0E, 0xBA, 0x0D, 0xBB, 0xBC, 0x99, 0xE1, 0xBB, 0xAC}, // ucEepromInst
        {0xB4, 0x0E, 0x1E},                                           // ucFlashInst
        0,                                                            // ucSPHaddr
        0x3D,                                                         // ucSPLaddr
        fill_b2(1024 / 32),                                           // uiFlashpages
        0x2E,                                                         // ucDWDRAddress
        0x00,                                                         // ucDWBasePC
        0x00,                                                         // ucAllowFullPageBitstream
        fill_b2(0x00), // uiStartSmallestBootLoaderSection
        1,             // EnablePageProgramming
        0,             // ucCacheType
        fill_b2(0x60), // uiSramStartAddr
        0,             // ucResetType
        0,             // ucPCMaskExtended
        0,             // ucPCMaskHigh
        0,             // ucEindAddress
        fill_b2(0x1C), // EECRAddress
    },
    nullptr
};

} // namespace
