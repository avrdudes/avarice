#include "../jtag.h"

namespace {

constexpr gdb_io_reg_def_type atmega325_io_registers[] = {{"PINA", 0x20, 0x00},
                                                      {"DDRA", 0x21, 0x00},
                                                      {"PORTA", 0x22, 0x00},
                                                      {"PINB", 0x23, 0x00},
                                                      {"DDRB", 0x24, 0x00},
                                                      {"PORTB", 0x25, 0x00},
                                                      {"PINC", 0x26, 0x00},
                                                      {"DDRC", 0x27, 0x00},
                                                      {"PORTC", 0x28, 0x00},
                                                      {"PIND", 0x29, 0x00},
                                                      {"DDRD", 0x2a, 0x00},
                                                      {"PORTD", 0x2b, 0x00},
                                                      {"PINE", 0x2c, 0x00},
                                                      {"DDRE", 0x2d, 0x00},
                                                      {"PORTE", 0x2e, 0x00},
                                                      {"PINF", 0x2f, 0x00},
                                                      {"DDRF", 0x30, 0x00},
                                                      {"PORTF", 0x31, 0x00},
                                                      {"PING", 0x32, 0x00},
                                                      {"DDRG", 0x33, 0x00},
                                                      {"PORTG", 0x34, 0x00},
                                                      {"TIFR0", 0x35, 0x00},
                                                      {"TIFR1", 0x36, 0x00},
                                                      {"TIFR2", 0x37, 0x00},
                                                      {"EIFR", 0x3c, 0x00},
                                                      {"EIMSK", 0x3d, 0x00},
                                                      {"GPIOR0", 0x3e, 0x00},
                                                      {"EECR", 0x3f, 0x00},
                                                      {"EEDR", 0x40, 0x00},
                                                      {"EEARL", 0x41, 0x00},
                                                      {"EEARH", 0x42, 0x00},
                                                      {"GTCCR", 0x43, 0x00},
                                                      {"TCCR0A", 0x44, 0x00},
                                                      {"TCNT0", 0x46, 0x00},
                                                      {"OCR0A", 0x47, 0x00},
                                                      {"GPIOR1", 0x4a, 0x00},
                                                      {"GPIOR2", 0x4b, 0x00},
                                                      {"SPCR", 0x4c, 0x00},
                                                      {"SPSR", 0x4d, 0x00},
                                                      {"SPDR", 0x4e, 0x00},
                                                      {"ACSR", 0x50, 0x00},
                                                      {"OCDR", 0x51, 0x00},
                                                      {"SMCR", 0x53, 0x00},
                                                      {"MCUSR", 0x54, 0x00},
                                                      {"MCUCR", 0x55, 0x00},
                                                      {"SPMCSR", 0x57, 0x00},
                                                      {"SPL", 0x5d, 0x00},
                                                      {"SPH", 0x5e, 0x00},
                                                      {"SREG", 0x5f, 0x00},
                                                      {"WDTCR", 0x60, 0x00},
                                                      {"CLKPR", 0x61, 0x00},
                                                      {"PRR", 0x64, 0x00},
                                                      {"OSCCAL", 0x66, 0x00},
                                                      {"EICRA", 0x69, 0x00},
                                                      {"PCMSK0", 0x6b, 0x00},
                                                      {"PCMSK1", 0x6c, 0x00},
                                                      {"TIMSK0", 0x6e, 0x00},
                                                      {"TIMSK1", 0x6f, 0x00},
                                                      {"TIMSK2", 0x70, 0x00},
                                                      {"ADCL", 0x78, IO_REG_RSE},
                                                      {"ADCH", 0x79, IO_REG_RSE},
                                                      {"ADCSRA", 0x7a, 0x00},
                                                      {"ADCSRB", 0x7b, 0x00},
                                                      {"ADMUX", 0x7c, 0x00},
                                                      {"DIDR0", 0x7e, 0x00},
                                                      {"DIDR1", 0x7f, 0x00},
                                                      {"TCCR1A", 0x80, 0x00},
                                                      {"TCCR1B", 0x81, 0x00},
                                                      {"TCCR1C", 0x82, 0x00},
                                                      {"TCNT1L", 0x84, 0x00},
                                                      {"TCNT1H", 0x85, 0x00},
                                                      {"ICR1L", 0x86, 0x00},
                                                      {"ICR1H", 0x87, 0x00},
                                                      {"OCR1AL", 0x88, 0x00},
                                                      {"OCR1AH", 0x89, 0x00},
                                                      {"OCR1BL", 0x8a, 0x00},
                                                      {"OCR1BH", 0x8b, 0x00},
                                                      {"TCCR2A", 0xb0, 0x00},
                                                      {"TCNT2", 0xb2, 0x00},
                                                      {"OCR2A", 0xb3, 0x00},
                                                      {"ASSR", 0xb6, 0x00},
                                                      {"USICR", 0xb8, 0x00},
                                                      {"USISR", 0xb9, 0x00},
                                                      {"USIDR", 0xba, 0x00},
                                                      {"UCSR0A", 0xc0, 0x00},
                                                      {"UCSR0B", 0xc1, 0x00},
                                                      {"UCSR0C", 0xc2, 0x00},
                                                      {"UBRR0L", 0xc4, 0x00},
                                                      {"UBRR0H", 0xc5, 0x00},
                                                      {"UDR0", 0xc6, IO_REG_RSE},
                                                      {nullptr, 0, 0}};

[[maybe_unused]] const jtag_device_def_type atmega325{
    "atmega325",
    0x9505,
    128,
    256, // 32768 bytes flash
    4,
    256,    // 1024 bytes EEPROM
    22 * 4, // 22 interrupt vectors
    DEVFL_MKII_ONLY,
    atmega325_io_registers,
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
        {0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00}, // ucReadIOShadow
        {0xB6, 0x6D, 0x1B, 0xE0, 0xDF, 0x1C, 0xB8, 0xE0}, // ucWriteIO
        {0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00}, // ucWriteIOShadow
        {0x53, 0xDA, 0x01, 0xDF, 0xF7, 0x0F, 0x00, 0x00, 0x00, 0x00,
         0x4D, 0x07, 0x37, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // ucReadExtIO
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // ucReadIOExtShadow
        {0x53, 0xDA, 0x01, 0xD8, 0xF7, 0x0F, 0x00, 0x00, 0x00, 0x00,
         0x4D, 0x05, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // ucWriteExtIO
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // ucWriteIOExtShadow
        0x31,                                                         // ucIDRAddress
        0x57,                                                         // ucSPMCRAddress
        0,                                                            // ucRAMPZAddress
        fill_b2(128),                                                 // uiFlashPageSize
        4,                                                            // ucEepromPageSize
        fill_b4(0x3F00),                                              // ulBootAddress
        fill_b2(0xC6),                                                // uiUpperExtIOLoc
        fill_b4(32768),                                               // ulFlashSize
        {0x00},                                                       // ucEepromInst
        {0x00},                                                       // ucFlashInst
        0x3E,                                                         // ucSPHaddr
        0x3D,                                                         // ucSPLaddr
        fill_b2(32768 / 128),                                         // uiFlashpages
        0x00,                                                         // ucDWDRAddress
        0x40,                                                         // ucDWBasePC
        0x00,                                                         // ucAllowFullPageBitstream
        fill_b2(0x00),  // uiStartSmallestBootLoaderSection
        1,              // EnablePageProgramming
        0,              // ucCacheType
        fill_b2(0x100), // uiSramStartAddr
        0,              // ucResetType
        0,              // ucPCMaskExtended
        0x40,           // ucPCMaskHigh
        0,              // ucEindAddress
        fill_b2(0x1F),  // EECRAddress
    },
    nullptr
};

} // namespace
