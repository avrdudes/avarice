#include "../jtag.h"

namespace {

constexpr gdb_io_reg_def_type atmega64_io_registers[] = {{"PINF", 0x20, 0x00},
                                                         {"PINE", 0x21, 0x00},
                                                         {"DDRE", 0x22, 0x00},
                                                         {"PORTE", 0x23, 0x00},
                                                         {"ADCL", 0x24, IO_REG_RSE},
                                                         {"ADCH", 0x25, IO_REG_RSE},
                                                         {"ADCSR -- ADCSRA", 0x26, 0x00},
                                                         {"ADMUX", 0x27, 0x00},
                                                         {"ACSR", 0x28, 0x00},
                                                         {"UBRR0L", 0x29, 0x00},
                                                         {"UCSR0B", 0x2a, 0x00},
                                                         {"UCSR0A", 0x2b, 0x00},
                                                         {"UDR0", 0x2c, IO_REG_RSE},
                                                         {"SPCR", 0x2d, 0x00},
                                                         {"SPSR", 0x2e, 0x00},
                                                         {"SPDR", 0x2f, 0x00},
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
                                                         {"DDRA", 0x3a, 0x00},
                                                         {"PORTA", 0x3b, 0x00},
                                                         {"EECR", 0x3c, 0x00},
                                                         {"EEDR", 0x3d, 0x00},
                                                         {"EEARL", 0x3e, 0x00},
                                                         {"EEARH", 0x3f, 0x00},
                                                         {"SFIOR", 0x40, 0x00},
                                                         {"WDTCR", 0x41, 0x00},
                                                         {"OCDR", 0x42, 0x00},
                                                         {"OCR2", 0x43, 0x00},
                                                         {"TCNT2", 0x44, 0x00},
                                                         {"TCCR2", 0x45, 0x00},
                                                         {"ICR1L", 0x46, 0x00},
                                                         {"ICR1H", 0x47, 0x00},
                                                         {"OCR1BL", 0x48, 0x00},
                                                         {"OCR1BH", 0x49, 0x00},
                                                         {"OCR1AL", 0x4a, 0x00},
                                                         {"OCR1AH", 0x4b, 0x00},
                                                         {"TCNT1L", 0x4c, 0x00},
                                                         {"TCNT1H", 0x4d, 0x00},
                                                         {"TCCR1B", 0x4e, 0x00},
                                                         {"TCCR1A", 0x4f, 0x00},
                                                         {"ASSR", 0x50, 0x00},
                                                         {"OCR0", 0x51, 0x00},
                                                         {"TCNT0", 0x52, 0x00},
                                                         {"TCCR0", 0x53, 0x00},
                                                         {"MCUSR -- MCUCSR", 0x54, 0x00},
                                                         {"MCUCR", 0x55, 0x00},
                                                         {"TIFR", 0x56, 0x00},
                                                         {"TIMSK", 0x57, 0x00},
                                                         {"EIFR", 0x58, 0x00},
                                                         {"EIMSK", 0x59, 0x00},
                                                         {"EICRB", 0x5a, 0x00},
                                                         {"XDIV", 0x5c, 0x00},
                                                         {"SPL", 0x5d, 0x00},
                                                         {"SPH", 0x5e, 0x00},
                                                         {"SREG", 0x5f, 0x00},
                                                         {"DDRF", 0x61, 0x00},
                                                         {"PORTF", 0x62, 0x00},
                                                         {"PING", 0x63, 0x00},
                                                         {"DDRG", 0x64, 0x00},
                                                         {"PORTG", 0x65, 0x00},
                                                         {"SPMCR -- SPMCSR", 0x68, 0x00},
                                                         {"EICRA", 0x6a, 0x00},
                                                         {"XMCRB", 0x6c, 0x00},
                                                         {"XMCRA", 0x6d, 0x00},
                                                         {"OSCCAL", 0x6f, 0x00},
                                                         {"TWBR", 0x70, 0x00},
                                                         {"TWSR", 0x71, 0x00},
                                                         {"TWAR", 0x72, 0x00},
                                                         {"TWDR", 0x73, 0x00},
                                                         {"TWCR", 0x74, 0x00},
                                                         {"OCR1CL", 0x78, 0x00},
                                                         {"OCR1CH", 0x79, 0x00},
                                                         {"TCCR1C", 0x7a, 0x00},
                                                         {"ETIFR", 0x7c, 0x00},
                                                         {"ETIMSK", 0x7d, 0x00},
                                                         {"ICR3L", 0x80, 0x00},
                                                         {"ICR3H", 0x81, 0x00},
                                                         {"OCR3CL", 0x82, 0x00},
                                                         {"OCR3CH", 0x83, 0x00},
                                                         {"OCR3BL", 0x84, 0x00},
                                                         {"OCR3BH", 0x85, 0x00},
                                                         {"OCR3AL", 0x86, 0x00},
                                                         {"OCR3AH", 0x87, 0x00},
                                                         {"TCNT3L", 0x88, 0x00},
                                                         {"TCNT3H", 0x89, 0x00},
                                                         {"TCCR3B", 0x8a, 0x00},
                                                         {"TCCR3A", 0x8b, 0x00},
                                                         {"TCCR3C", 0x8c, 0x00},
                                                         {"ADCSRB", 0x8e, 0x00},
                                                         {"UBRR0H", 0x90, 0x00},
                                                         {"UCSR0C", 0x95, 0x00},
                                                         {"UBRR1H", 0x98, 0x00},
                                                         {"UBRR1L", 0x99, 0x00},
                                                         {"UCSR1B", 0x9a, 0x00},
                                                         {"UCSR1A", 0x9b, 0x00},
                                                         {"UDR1", 0x9c, IO_REG_RSE},
                                                         {"UCSR1C", 0x9d, 0x00},
                                                         {nullptr, 0, 0}};

constexpr jtag1_device_desc_type jtag1_device_desc{
    JTAG_C_SET_DEVICE_DESCRIPTOR,
    {0xCF, 0x2F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {0xCF, 0x27, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x3E, 0xB5, 0x1F, 0x37, 0xFF, 0x1F, 0x21, 0x2F, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x3E, 0xB5, 0x0F, 0x27, 0xFF, 0x1F, 0x21, 0x27, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    0x22,
    0x68,
    0x00,
    256,
    8,
    0x7E00,
    0x9D,
    {JTAG_EOM}};

[[maybe_unused]] const jtag_device_def_type atmega64{
    "atmega64",
    0x9602,
    256,
    256, // 64K flash
    8,
    256,  // 2K bytes EEPROM
    0x8c, // 35 interrupt vectors
    NO_TWEAKS,
    atmega64_io_registers,
    0x07,
    0x8000, // fuses
    0x6f,   // osccal
    2,      // OCD revision
    &jtag1_device_desc,
    {
        CMND_SET_DEVICE_DESCRIPTOR,
        {0xFF, 0x6F, 0xFF, 0xFF, 0xFB, 0xFF, 0xFF, 0xF7},            // ucReadIO
        {0},                                                         // ucReadIOShadow
        {0x8C, 0x26, 0xB6, 0xFD, 0xFB, 0xFF, 0xBF, 0xF6},            // ucWriteIO
        {0},                                                         // ucWriteIOShadow
        {0x3E, 0xB5, 0x1F, 0x37, 0xFF, 0x5F, 0x21, 0x2F, /* ... */}, // ucReadExtIO
        {0},                                                         // ucReadIOExtShadow
        {0x36, 0xB5, 0x0F, 0x27, 0xFF, 0x5F, 0x21, 0x27, /* ... */}, // ucWriteExtIO
        {0},                                                         // ucWriteIOExtShadow
        0x22,                                                        // ucIDRAddress
        0x68,                                                        // ucSPMCRAddress
        0,                                                           // ucRAMPZAddress
        256,                                                // uiFlashPageSize
        8,                                                           // ucEepromPageSize
        0x7E00,                                             // ulBootAddress
        0x9D,                                               // uiUpperExtIOLoc
        0x10000,                                            // ulFlashSize
        {0},                                                         // ucEepromInst
        {0},                                                         // ucFlashInst
        0x3e,                                                        // ucSPHaddr
        0x3d,                                                        // ucSPLaddr
        0x10000 / 256,                                      // uiFlashpages
        0,                                                           // ucDWDRAddress
        0,                                                           // ucDWBasePC
        1,                                                           // ucAllowFullPageBitstream
        0,     // uiStartSmallestBootLoaderSection
        1,              // EnablePageProgramming
        0,              // ucCacheType
        0x100, // uiSramStartAddr
        0,              // ucResetType
        0,              // ucPCMaskExtended
        0,              // ucPCMaskHigh
        0,              // ucEindAddress
        0x1c,  // EECRAddress
    },
    nullptr};

} // namespace
