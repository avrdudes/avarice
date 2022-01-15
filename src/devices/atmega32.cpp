#include "../jtag.h"

namespace {

constexpr gdb_io_reg_def_type atmega32_io_registers[] = {
    {"TWBR", 0x20, 0x00},       {"TWSR", 0x21, 0x00},
    {"TWAR", 0x22, 0x00},       {"TWDR", 0x23, 0x00},
    {"ADCL", 0x24, IO_REG_RSE}, // Reading during a conversion corrupts
    {"ADCH", 0x25, IO_REG_RSE}, // ADC result
    {"ADCSRA", 0x26, 0x00},     {"ADMUX", 0x27, 0x00},
    {"ACSR", 0x28, 0x00},       {"UBRRL", 0x29, 0x00},
    {"UCSRB", 0x2A, 0x00},      {"UCSRA", 0x2B, 0x00},
    {"UDR", 0x2C, IO_REG_RSE}, // Reading this clears the UART RXC flag
    {"SPCR", 0x2D, 0x00},       {"SPSR", 0x2E, 0x00},
    {"SPDR", 0x2F, 0x00},       {"PIND", 0x30, 0x00},
    {"DDRD", 0x31, 0x00},       {"PORTD", 0x32, 0x00},
    {"PINC", 0x33, 0x00},       {"DDRC", 0x34, 0x00},
    {"PORTC", 0x35, 0x00},      {"PINB", 0x36, 0x00},
    {"DDRB", 0x37, 0x00},       {"PORTB", 0x38, 0x00},
    {"PINA", 0x39, 0x00},       {"DDRA", 0x3A, 0x00},
    {"PORTA", 0x3B, 0x00},      {"EECR", 0x3C, 0x00},
    {"EEDR", 0x3D, 0x00},       {"EEARL", 0x3E, 0x00},
    {"EEARH", 0x3F, 0x00},      {"UBRR0H -- UCSR0C", 0x40, 0x00},
    {"WDTCR", 0x41, 0x00},      {"ASSR", 0x42, 0x00},
    {"OCR2", 0x43, 0x00},       {"TCNT2", 0x44, 0x00},
    {"TCCR2", 0x45, 0x00},      {"ICR1L", 0x46, 0x00},
    {"ICR1H", 0x47, 0x00},      {"OCR1BL", 0x48, 0x00},
    {"OCR1BH", 0x49, 0x00},     {"OCR1AL", 0x4A, 0x00},
    {"OCR1AH", 0x4B, 0x00},     {"TCNT1L", 0x4C, 0x00},
    {"TCNT1H", 0x4D, 0x00},     {"TCCR1B", 0x4E, 0x00},
    {"TCCR1A", 0x4F, 0x00},     {"SFIOR", 0x50, 0x00},
    {"OSCCAL", 0x51, 0x00},     {"TCNT0", 0x52, 0x00},
    {"TCCR0", 0x53, 0x00},      {"MCUCSR", 0x54, 0x00},
    {"MCUCR", 0x55, 0x00},      {"TWCR", 0x56, 0x00},
    {"SPMCR", 0x57, 0x00},      {"TIFR", 0x58, 0x00},
    {"TIMSK", 0x59, 0x00},      {"GIFR", 0x5A, 0x00},
    {"GIMSK", 0x5B, 0x00},      {"OCR0", 0x5C, 0x00},
    {"SPL", 0x5D, 0x00},        {"SPH", 0x5E, 0x00},
    {"SREG", 0x5F, 0x00},       {nullptr, 0, 0}};

[[maybe_unused]] const jtag_device_def_type atmega32{
    "atmega32",
    0x9502,
    128,
    256, // 32K flash
    4,
    256,  // 1K EEPROM
    0x54, // 21 interrupt vectors
    DEVFL_NONE,
    atmega32_io_registers,
    0x03,
    0x8000, // fuses
    0x51,   // osccal
    2,      // OCD revision
    {JTAG_C_SET_DEVICE_DESCRIPTOR,
     {0xFF, 0x6F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
     {0xFF, 0x66, 0xFF, 0xFF, 0xFF, 0xFF, 0xBF, 0xFA},
     {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
     {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
     {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
     {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
     {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
     {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
     0x31,
     0x57,
     0x00,
     {128, 0},
     4,
     {0x00, 0x3F, 0x00, 0x00},
     0,
     {JTAG_EOM}},
    {
        CMND_SET_DEVICE_DESCRIPTOR,
        {0xFF, 0x6F, 0xFF, 0xFF, 0xFE, 0xFF, 0xFD, 0xFF}, // ucReadIO
        {0X00, 0X00, 0X00, 0X00, 0X01, 0X00, 0X00, 0X00}, // ucReadIOShadow
        {0x8F, 0x26, 0xB6, 0xFD, 0xFE, 0xFF, 0xBD, 0xFA}, // ucWriteIO
        {0X00, 0X00, 0X00, 0X00, 0X01, 0X00, 0X00, 0X00}, // ucWriteIOShadow
        {0},                                              // ucReadExtIO
        {0},                                              // ucReadIOExtShadow
        {0},                                              // ucWriteExtIO
        {0},                                              // ucWriteIOExtShadow
        0x31,                                             // ucIDRAddress
        0x57,                                             // ucSPMCRAddress
        0,                                                // ucRAMPZAddress
        fill_b2(128),                                     // uiFlashPageSize
        4,                                                // ucEepromPageSize
        fill_b4(0x3F00),                                  // ulBootAddress
        fill_b2(0),                                       // uiUpperExtIOLoc
        fill_b4(0x8000),                                  // ulFlashSize
        {0},                                              // ucEepromInst
        {0},                                              // ucFlashInst
        0x3e,                                             // ucSPHaddr
        0x3d,                                             // ucSPLaddr
        fill_b2(0x8000 / 128),                            // uiFlashpages
        0,                                                // ucDWDRAddress
        0,                                                // ucDWBasePC
        1,                                                // ucAllowFullPageBitstream
        fill_b2(0),                                       // uiStartSmallestBootLoaderSection
        1,                                                // EnablePageProgramming
        0,                                                // ucCacheType
        fill_b2(0x60),                                    // uiSramStartAddr
        0,                                                // ucResetType
        0,                                                // ucPCMaskExtended
        0,                                                // ucPCMaskHigh
        0,                                                // ucEindAddress
        fill_b2(0x1c),                                    // EECRAddress
    },
    nullptr
};

} // namespace
