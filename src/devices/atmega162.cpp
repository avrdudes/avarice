#include "../jtag.h"

namespace {

constexpr gdb_io_reg_def_type atmega162_io_registers[] = {
    {"UBRR1L", 0x20, 0x00},
    {"UCSR1B", 0x21, 0x00},
    {"UCSR1A", 0x22, 0x00},
    {"UDR1", 0x23, IO_REG_RSE}, // Reading this clears the UART RXC flag
    {"OCDR -- OSCCAL", 0x24, 0x00},
    {"PINE", 0x25, 0x00},
    {"DDRE", 0x26, 0x00},
    {"PORTE", 0x27, 0x00},
    {"ACSR", 0x28, 0x00},
    {"UBRR0L", 0x29, 0x00},
    {"UCSR0B", 0x2A, 0x00},
    {"UCSR0A", 0x2B, 0x00},
    {"UDR0", 0x2C, IO_REG_RSE}, // Reading this clears the UART RXC flag
    {"SPCR", 0x2D, 0x00},
    {"SPSR", 0x2E, 0x00},
    {"SPDR", 0x2F, 0x00},
    {"PIND", 0x30, 0x00},
    {"DDRD", 0x31, 0x00},
    {"PORTD", 0x32, 0x00},
    {"PINC", 0x33, 0x00},
    {"DDRC", 0x34, 0x00},
    {"PORTC", 0x35, 0x00},
    {"PINB", 0x36, 0x00},
    {"DDRB", 0x37, 0x00},
    {"PORTB", 0x38, 0x00},
    {"PINA", 0x39, 0x00},
    {"DDRA", 0x3A, 0x00},
    {"PORTA", 0x3B, 0x00},
    {"EECR", 0x3C, 0x00},
    {"EEDR", 0x3D, 0x00},
    {"EEARL", 0x3E, 0x00},
    {"EEARH", 0x3F, 0x00},
    {"UBRR0H -- UCSR0C", 0x40, 0x00},
    {"WDTCR", 0x41, 0x00},
    {"OCR2", 0x42, 0x00},
    {"TCNT2", 0x43, 0x00},
    {"ICR1L", 0x44, 0x00},
    {"ICR1H", 0x45, 0x00},
    {"ASSR", 0x46, 0x00},
    {"TCCR2", 0x47, 0x00},
    {"OCR1BL", 0x48, 0x00},
    {"OCR1BH", 0x49, 0x00},
    {"OCR1AL", 0x4A, 0x00},
    {"OCR1AH", 0x4B, 0x00},
    {"TCNT1L", 0x4C, 0x00},
    {"TCNT1H", 0x4D, 0x00},
    {"TCCR1B", 0x4E, 0x00},
    {"TCCR1A", 0x4F, 0x00},
    {"SFIOR", 0x50, 0x00},
    {"OCR0", 0x51, 0x00},
    {"TCNT0", 0x52, 0x00},
    {"TCCR0", 0x53, 0x00},
    {"MCUCSR", 0x54, 0x00},
    {"MCUCR", 0x55, 0x00},
    {"EMCUCR", 0x56, 0x00},
    {"SPMCR", 0x57, 0x00},
    {"TIFR", 0x58, 0x00},
    {"TIMSK", 0x59, 0x00},
    {"GIFR", 0x5A, 0x00},
    {"GICR", 0x5B, 0x00},
    {"UCSR1C -- UBRR1H", 0x5C, 0x00},
    {"SPL", 0x5D, 0x00},
    {"SPH", 0x5E, 0x00},
    {"SREG", 0x5F, 0x00},
    {"CLKPR", 0x61, 0x00},
    {"PCMSK0", 0x6B, 0x00},
    {"PCMSK1", 0x6C, 0x00},
    {"ETIFR", 0x7C, 0x00},
    {"ETIMSK", 0x7D, 0x00},
    {"ICR3L", 0x80, 0x00},
    {"ICR3H", 0x81, 0x00},
    {"OCR3BL", 0x84, 0x00},
    {"OCR3BH", 0x85, 0x00},
    {"OCR3AL", 0x86, 0x00},
    {"OCR3AH", 0x87, 0x00},
    {"TCNT3L", 0x88, 0x00},
    {"TCNT3H", 0x89, 0x00},
    {"TCCR3B", 0x8A, 0x00},
    {"TCCR3A", 0x8B, 0x00},
    {nullptr, 0, 0}};

[[maybe_unused]] const jtag_device_def_type atmega162{
    "atmega162",
    0x9404,
    128,
    128, // 16K flash
    4,
    128,  // 512 bytes EEPROM
    0x70, // 28 interrupt vectors
    DEVFL_NONE,
    atmega162_io_registers,
    false,
    0x07,
    0x8000, // fuses
    0x24,   // osccal
    2,      // OCD revision
    {JTAG_C_SET_DEVICE_DESCRIPTOR,
     {0xF7, 0x6F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
     {0xF3, 0x66, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFA},
     {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
     {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
     {0x02, 0x18, 0x00, 0x30, 0xF3, 0x0F, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
     {0x02, 0x18, 0x00, 0x20, 0xF3, 0x0F, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
     {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
     {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
     0x04,
     0x57,
     0x00,
     {128, 0},
     4,
     {0x80, 0x1F, 0x00, 0x00},
     0x8B,
     {JTAG_EOM}},
    {
        CMND_SET_DEVICE_DESCRIPTOR,
        {0xE7, 0x6F, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xEF}, // ucReadIO
        {0xC3, 0x26, 0xB6, 0xFD, 0xFE, 0xFF, 0xFF, 0xEA}, // ucReadIOShadow
        {0X00, 0X00, 0X00, 0X00, 0X01, 0X00, 0X00, 0X10}, // ucWriteIO
        {0X00, 0X00, 0X00, 0X00, 0X01, 0X00, 0X00, 0X10}, // ucWriteIOShadow
        {0},                                              // ucReadExtIO
        {0},                                              // ucReadIOExtShadow
        {0},                                              // ucWriteExtIO
        {0},                                              // ucWriteIOExtShadow
        0x04,                                             // ucIDRAddress
        0x57,                                             // ucSPMCRAddress
        0,                                                // ucRAMPZAddress
        fill_b2(128),                                     // uiFlashPageSize
        4,                                                // ucEepromPageSize
        fill_b4(0x1F80),                                  // ulBootAddress
        fill_b2(0xBB),                                    // uiUpperExtIOLoc
        fill_b4(0x4000),                                  // ulFlashSize
        {0},                                              // ucEepromInst
        {0},                                              // ucFlashInst
        0x3e,                                             // ucSPHaddr
        0x3d,                                             // ucSPLaddr
        fill_b2(0x4000 / 128),                            // uiFlashpages
        0,                                                // ucDWDRAddress
        0,                                                // ucDWBasePC
        1,                                                // ucAllowFullPageBitstream
        fill_b2(0),                                       // uiStartSmallestBootLoaderSection
        1,                                                // EnablePageProgramming
        0,                                                // ucCacheType
        fill_b2(0x100),                                   // uiSramStartAddr
        0,                                                // ucResetType
        0,                                                // ucPCMaskExtended
        0,                                                // ucPCMaskHigh
        0,                                                // ucEindAddress
        fill_b2(0x1c),                                    // EECRAddress
    },
    nullptr
};

} // namespace
