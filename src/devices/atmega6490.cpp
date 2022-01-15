#include "../jtag.h"

namespace {

constexpr gdb_io_reg_def_type atmega6490_io_registers[] = {
    {"PINA", 0x20, 0x00},    {"DDRA", 0x21, 0x00},       {"PORTA", 0x22, 0x00},
    {"PINB", 0x23, 0x00},    {"DDRB", 0x24, 0x00},       {"PORTB", 0x25, 0x00},
    {"PINC", 0x26, 0x00},    {"DDRC", 0x27, 0x00},       {"PORTC", 0x28, 0x00},
    {"PIND", 0x29, 0x00},    {"DDRD", 0x2a, 0x00},       {"PORTD", 0x2b, 0x00},
    {"PINE", 0x2c, 0x00},    {"DDRE", 0x2d, 0x00},       {"PORTE", 0x2e, 0x00},
    {"PINF", 0x2f, 0x00},    {"DDRF", 0x30, 0x00},       {"PORTF", 0x31, 0x00},
    {"PING", 0x32, 0x00},    {"DDRG", 0x33, 0x00},       {"PORTG", 0x34, 0x00},
    {"TIFR0", 0x35, 0x00},   {"TIFR1", 0x36, 0x00},      {"TIFR2", 0x37, 0x00},
    {"EIFR", 0x3c, 0x00},    {"EIMSK", 0x3d, 0x00},      {"GPIOR0", 0x3e, 0x00},
    {"EECR", 0x3f, 0x00},    {"EEDR", 0x40, 0x00},       {"EEARL", 0x41, 0x00},
    {"EEARH", 0x42, 0x00},   {"GTCCR", 0x43, 0x00},      {"TCCR0A", 0x44, 0x00},
    {"TCNT0", 0x46, 0x00},   {"OCR0A", 0x47, 0x00},      {"GPIOR1", 0x4a, 0x00},
    {"GPIOR2", 0x4b, 0x00},  {"SPCR", 0x4c, 0x00},       {"SPSR", 0x4d, 0x00},
    {"SPDR", 0x4e, 0x00},    {"ACSR", 0x50, 0x00},       {"OCDR", 0x51, 0x00},
    {"SMCR", 0x53, 0x00},    {"MCUSR", 0x54, 0x00},      {"MCUCR", 0x55, 0x00},
    {"SPMCSR", 0x57, 0x00},  {"SPL", 0x5d, 0x00},        {"SPH", 0x5e, 0x00},
    {"SREG", 0x5f, 0x00},    {"WDTCR", 0x60, 0x00},      {"CLKPR", 0x61, 0x00},
    {"PRR", 0x64, 0x00},     {"OSCCAL", 0x66, 0x00},     {"EICRA", 0x69, 0x00},
    {"PCMSK0", 0x6b, 0x00},  {"PCMSK1", 0x6c, 0x00},     {"PCMSK2", 0x6d, 0x00},
    {"TIMSK0", 0x6e, 0x00},  {"TIMSK1", 0x6f, 0x00},     {"TIMSK2", 0x70, 0x00},
    {"PCMSK3", 0x73, 0x00},  {"ADCL", 0x78, IO_REG_RSE}, {"ADCH", 0x79, IO_REG_RSE},
    {"ADCSRA", 0x7a, 0x00},  {"ADCSRB", 0x7b, 0x00},     {"ADMUX", 0x7c, 0x00},
    {"DIDR0", 0x7e, 0x00},   {"DIDR1", 0x7f, 0x00},      {"TCCR1A", 0x80, 0x00},
    {"TCCR1B", 0x81, 0x00},  {"TCCR1C", 0x82, 0x00},     {"TCNT1L", 0x84, 0x00},
    {"TCNT1H", 0x85, 0x00},  {"ICR1L", 0x86, 0x00},      {"ICR1H", 0x87, 0x00},
    {"OCR1AL", 0x88, 0x00},  {"OCR1AH", 0x89, 0x00},     {"OCR1BL", 0x8a, 0x00},
    {"OCR1BH", 0x8b, 0x00},  {"TCCR2A", 0xb0, 0x00},     {"TCNT2", 0xb2, 0x00},
    {"OCR2A", 0xb3, 0x00},   {"ASSR", 0xb6, 0x00},       {"USICR", 0xb8, 0x00},
    {"USISR", 0xb9, 0x00},   {"USIDR", 0xba, 0x00},      {"UCSR0A", 0xc0, 0x00},
    {"UCSR0B", 0xc1, 0x00},  {"UCSR0C", 0xc2, 0x00},     {"UBRR0L", 0xc4, 0x00},
    {"UBRR0H", 0xc5, 0x00},  {"UDR0", 0xc6, IO_REG_RSE}, {"PINH", 0xd8, 0x00},
    {"DDRH", 0xd9, 0x00},    {"PORTH", 0xda, 0x00},      {"PINJ", 0xdb, 0x00},
    {"DDRJ", 0xdc, 0x00},    {"PORTJ", 0xdd, 0x00},      {"LCDCRA", 0xe4, 0x00},
    {"LCDCRB", 0xe5, 0x00},  {"LCDFRR", 0xe6, 0x00},     {"LCDCCR", 0xe7, 0x00},
    {"LCDDR00", 0xec, 0x00}, {"LCDDR01", 0xed, 0x00},    {"LCDDR02", 0xee, 0x00},
    {"LCDDR03", 0xef, 0x00}, {"LCDDR04", 0xf0, 0x00},    {"LCDDR05", 0xf1, 0x00},
    {"LCDDR06", 0xf2, 0x00}, {"LCDDR07", 0xf3, 0x00},    {"LCDDR08", 0xf4, 0x00},
    {"LCDDR09", 0xf5, 0x00}, {"LCDDR10", 0xf6, 0x00},    {"LCDDR11", 0xf7, 0x00},
    {"LCDDR12", 0xf8, 0x00}, {"LCDDR13", 0xf9, 0x00},    {"LCDDR14", 0xfa, 0x00},
    {"LCDDR15", 0xfb, 0x00}, {"LCDDR16", 0xfc, 0x00},    {"LCDDR17", 0xfd, 0x00},
    {"LCDDR18", 0xfe, 0x00}, {"LCDDR19", 0xff, 0x00},    {nullptr, 0, 0}};

[[maybe_unused]] const jtag_device_def_type atmega6490{
    "atmega6490",
    0x9604,
    256,
    256, // 64K flash
    8,
    256,    // 2K bytes EEPROM
    25 * 4, // 25 interrupt vectors
    DEVFL_MKII_ONLY,
    atmega6490_io_registers,
    false,
    0x07,
    0x8000, // fuses
    0x66,   // osccal
    3,      // OCD revision
    {
        0 // no mkI support
    },
    {
        CMND_SET_DEVICE_DESCRIPTOR,
        {0xFF, 0xFF, 0xFF, 0xF0, 0xDF, 0x3C, 0xB9, 0xE0}, // ucReadIO
        {0},                                              // ucReadIOShadow
        {0xB6, 0x6D, 0x1B, 0xE0, 0xDF, 0x1C, 0xB8, 0xE0}, // ucWriteIO
        {0},                                              // ucWriteIOShadow
        {0x53, 0xFA, 0x09, 0xDF, 0xF7, 0x0F, 0x00, 0x00, 0x00, 0x00,
         0x4D, 0x07, 0x37, 0x00, 0x00, 0x3F, 0xF0, 0xF0, 0xFF, 0xFF}, // ucReadExtIO
        {0},                                                          // ucReadIOExtShadow
        {0x53, 0xFA, 0x09, 0xD8, 0xF7, 0x0F, 0x00, 0x00, 0x00, 0x00,
         0x4D, 0x05, 0x36, 0x00, 0x00, 0x3F, 0xE0, 0xF0, 0xFF, 0xFF}, // ucWriteExtIO
        {0},                                                          // ucWriteIOExtShadow
        0x31,                                                         // ucIDRAddress
        0x57,                                                         // ucSPMCRAddress
        0,                                                            // ucRAMPZAddress
        fill_b2(256),                                                 // uiFlashPageSize
        8,                                                            // ucEepromPageSize
        fill_b4(0x7E00),                                              // ulBootAddress
        fill_b2(0xFF),                                                // uiUpperExtIOLoc
        fill_b4(0x10000),                                             // ulFlashSize
        {0},                                                          // ucEepromInst
        {0},                                                          // ucFlashInst
        0x3e,                                                         // ucSPHaddr
        0x3d,                                                         // ucSPLaddr
        fill_b2(0x10000 / 256),                                       // uiFlashpages
        0,                                                            // ucDWDRAddress
        0x40,                                                         // ucDWBasePC
        0,                                                            // ucAllowFullPageBitstream
        fill_b2(0),     // uiStartSmallestBootLoaderSection
        1,              // EnablePageProgramming
        0,              // ucCacheType
        fill_b2(0x100), // uiSramStartAddr
        0,              // ucResetType
        0,              // ucPCMaskExtended
        0x0,            // ucPCMaskHigh
        0,              // ucEindAddress
        fill_b2(0x1f),  // EECRAddress
    },
    nullptr
};

} // namespace
