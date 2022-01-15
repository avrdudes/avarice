#include "../jtag.h"

namespace {

constexpr gdb_io_reg_def_type atmega640_io_registers[] = {{"PINA", 0x20, 0x00},
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
                                                      {"TIFR3", 0x38, 0x00},
                                                      {"TIFR4", 0x39, 0x00},
                                                      {"TIFR5", 0x3a, 0x00},
                                                      {"PCIFR", 0x3b, 0x00},
                                                      {"EIFR", 0x3c, 0x00},
                                                      {"EIMSK", 0x3d, 0x00},
                                                      {"GPIOR0", 0x3e, 0x00},
                                                      {"EECR", 0x3f, 0x00},
                                                      {"EEDR", 0x40, 0x00},
                                                      {"EEARL", 0x41, 0x00},
                                                      {"EEARH", 0x42, 0x00},
                                                      {"GTCCR", 0x43, 0x00},
                                                      {"TCCR0A", 0x44, 0x00},
                                                      {"TCCR0B", 0x45, 0x00},
                                                      {"TCNT0", 0x46, 0x00},
                                                      {"OCR0A", 0x47, 0x00},
                                                      {"OCR0B", 0x48, 0x00},
                                                      {"GPIOR1", 0x4a, 0x00},
                                                      {"GPIOR2", 0x4b, 0x00},
                                                      {"SPCR", 0x4c, 0x00},
                                                      {"SPSR", 0x4d, 0x00},
                                                      {"SPDR", 0x4e, 0x00},
                                                      {"ACSR", 0x50, 0x00},
                                                      {"MONDR -- OCDR", 0x51, 0x00},
                                                      {"SMCR", 0x53, 0x00},
                                                      {"MCUSR", 0x54, 0x00},
                                                      {"MCUCR", 0x55, 0x00},
                                                      {"SPMCSR", 0x57, 0x00},
                                                      {"RAMPZ", 0x5b, 0x00},
                                                      {"EIND", 0x5c, 0x00},
                                                      {"SPL", 0x5d, 0x00},
                                                      {"SPH", 0x5e, 0x00},
                                                      {"SREG", 0x5f, 0x00},
                                                      {"WDTCSR", 0x60, 0x00},
                                                      {"CLKPR", 0x61, 0x00},
                                                      {"PRR0", 0x64, 0x00},
                                                      {"PRR1", 0x65, 0x00},
                                                      {"OSCCAL", 0x66, 0x00},
                                                      {"PCICR", 0x68, 0x00},
                                                      {"EICRA", 0x69, 0x00},
                                                      {"EICRB", 0x6a, 0x00},
                                                      {"PCMSK0", 0x6b, 0x00},
                                                      {"PCMSK1", 0x6c, 0x00},
                                                      {"PCMSK2", 0x6d, 0x00},
                                                      {"TIMSK0", 0x6e, 0x00},
                                                      {"TIMSK1", 0x6f, 0x00},
                                                      {"TIMSK2", 0x70, 0x00},
                                                      {"TIMSK3", 0x71, 0x00},
                                                      {"TIMSK4", 0x72, 0x00},
                                                      {"TIMSK5", 0x73, 0x00},
                                                      {"XMCRA", 0x74, 0x00},
                                                      {"XMCRB", 0x75, 0x00},
                                                      {"ADCL", 0x78, IO_REG_RSE},
                                                      {"ADCH", 0x79, IO_REG_RSE},
                                                      {"ADCSRA", 0x7a, 0x00},
                                                      {"ADCSRB", 0x7b, 0x00},
                                                      {"ADMUX", 0x7c, 0x00},
                                                      {"DIDR2", 0x7d, 0x00},
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
                                                      {"OCR1CL", 0x8c, 0x00},
                                                      {"OCR1CH", 0x8d, 0x00},
                                                      {"TCCR3A", 0x90, 0x00},
                                                      {"TCCR3B", 0x91, 0x00},
                                                      {"TCCR3C", 0x92, 0x00},
                                                      {"TCNT3L", 0x94, 0x00},
                                                      {"TCNT3H", 0x95, 0x00},
                                                      {"ICR3L", 0x96, 0x00},
                                                      {"ICR3H", 0x97, 0x00},
                                                      {"OCR3AL", 0x98, 0x00},
                                                      {"OCR3AH", 0x99, 0x00},
                                                      {"OCR3BL", 0x9a, 0x00},
                                                      {"OCR3BH", 0x9b, 0x00},
                                                      {"OCR3CL", 0x9c, 0x00},
                                                      {"OCR3CH", 0x9d, 0x00},
                                                      {"TCCR4A", 0xa0, 0x00},
                                                      {"TCCR4B", 0xa1, 0x00},
                                                      {"TCCR4C", 0xa2, 0x00},
                                                      {"TCNT4L", 0xa4, 0x00},
                                                      {"TCNT4H", 0xa5, 0x00},
                                                      {"ICR4L", 0xa6, 0x00},
                                                      {"ICR4H", 0xa7, 0x00},
                                                      {"OCR4AL", 0xa8, 0x00},
                                                      {"OCR4AH", 0xa9, 0x00},
                                                      {"OCR4BL", 0xaa, 0x00},
                                                      {"OCR4BH", 0xab, 0x00},
                                                      {"OCR4CL", 0xac, 0x00},
                                                      {"OCR4CH", 0xad, 0x00},
                                                      {"TCCR2A", 0xb0, 0x00},
                                                      {"TCCR2B", 0xb1, 0x00},
                                                      {"TCNT2", 0xb2, 0x00},
                                                      {"OCR2A", 0xb3, 0x00},
                                                      {"OCR2B", 0xb4, 0x00},
                                                      {"ASSR", 0xb6, 0x00},
                                                      {"TWBR", 0xb8, 0x00},
                                                      {"TWSR", 0xb9, 0x00},
                                                      {"TWAR", 0xba, 0x00},
                                                      {"TWDR", 0xbb, 0x00},
                                                      {"TWCR", 0xbc, 0x00},
                                                      {"TWAMR", 0xbd, 0x00},
                                                      {"UCSR0A", 0xc0, 0x00},
                                                      {"UCSR0B", 0xc1, 0x00},
                                                      {"UCSR0C", 0xc2, 0x00},
                                                      {"UBRR0L", 0xc4, 0x00},
                                                      {"UBRR0H", 0xc5, 0x00},
                                                      {"UDR0", 0xc6, IO_REG_RSE},
                                                      {"UCSR1A", 0xc8, 0x00},
                                                      {"UCSR1B", 0xc9, 0x00},
                                                      {"UCSR1C", 0xca, 0x00},
                                                      {"UBRR1L", 0xcc, 0x00},
                                                      {"UBRR1H", 0xcd, 0x00},
                                                      {"UDR1", 0xce, IO_REG_RSE},
                                                      {"UCSR2A", 0xd0, 0x00},
                                                      {"UCSR2B", 0xd1, 0x00},
                                                      {"UCSR2C", 0xd2, 0x00},
                                                      {"UBRR2L", 0xd4, 0x00},
                                                      {"UBRR2H", 0xd5, 0x00},
                                                      {"UDR2", 0xd6, IO_REG_RSE},
                                                      {"PINH", 0x100, 0x00},
                                                      {"DDRH", 0x101, 0x00},
                                                      {"PORTH", 0x102, 0x00},
                                                      {"PINJ", 0x103, 0x00},
                                                      {"DDRJ", 0x104, 0x00},
                                                      {"PORTJ", 0x105, 0x00},
                                                      {"PINK", 0x106, 0x00},
                                                      {"DDRK", 0x107, 0x00},
                                                      {"PORTK", 0x108, 0x00},
                                                      {"PINL", 0x109, 0x00},
                                                      {"DDRL", 0x10a, 0x00},
                                                      {"PORTL", 0x10b, 0x00},
                                                      {"TCCR5A", 0x120, 0x00},
                                                      {"TCCR5B", 0x121, 0x00},
                                                      {"TCCR5C", 0x122, 0x00},
                                                      {"TCNT5L", 0x124, 0x00},
                                                      {"TCNT5H", 0x125, 0x00},
                                                      {"ICR5L", 0x126, 0x00},
                                                      {"ICR5H", 0x127, 0x00},
                                                      {"OCR5AL", 0x128, 0x00},
                                                      {"OCR5AH", 0x129, 0x00},
                                                      {"OCR5BL", 0x12a, 0x00},
                                                      {"OCR5BH", 0x12b, 0x00},
                                                      {"OCR5CL", 0x12c, 0x00},
                                                      {"OCR5CH", 0x12d, 0x00},
                                                      {"UCSR3A", 0x130, 0x00},
                                                      {"UCSR3B", 0x131, 0x00},
                                                      {"UCSR3C", 0x132, 0x00},
                                                      {"UBRR3L", 0x134, 0x00},
                                                      {"UBRR3H", 0x135, 0x00},
                                                      {"UDR3", 0x136, IO_REG_RSE},
                                                      {nullptr, 0, 0}};

[[maybe_unused]] const jtag_device_def_type atmega640{
    "atmega640",
    0x9608,
    256,
    256, // 64K flash
    8,
    512,  // 4K bytes EEPROM
    0xe4, // 57 interrupt vectors
    DEVFL_MKII_ONLY,
    atmega640_io_registers,
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
        {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x3D, 0xB9, 0xF8}, // ucReadIO
        {0},                                              // ucReadIOShadow
        {0xFF, 0xFF, 0x1F, 0xE0, 0xFF, 0x1D, 0xA9, 0xF8}, // ucWriteIO
        {0},                                              // ucWriteIOShadow
        {
            0x73, 0xFF, 0x3F, 0xFF, 0xF7, 0x3F, 0xF7, 0x3F, 0xF7, 0x3F,
            0x5F, 0x3F, 0x37, 0x37, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00,
            0xFF, 0x0F, 0x00, 0x00, 0xF7, 0x3F, 0x36, 0x00}, // ucReadExtIO
        {0},                                                 // ucReadIOExtShadow
        {
            0x73, 0xFF, 0x3F, 0xF8, 0xF7, 0x3F, 0xF7, 0x3F, 0xF7, 0x3F,
            0x5F, 0x2F, 0x36, 0x36, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00,
            0xFF, 0x0F, 0x00, 0x00, 0xF7, 0x3F, 0x36, 0x00}, // ucWriteExtIO
        {0},                                                 // ucWriteIOExtShadow
        0x31,                                                // ucIDRAddress
        0x57,                                                // ucSPMCRAddress
        0x3B,                                                // ucRAMPZAddress
        fill_b2(256),                                        // uiFlashPageSize
        8,                                                   // ucEepromPageSize
        fill_b4(0x7E00),                                     // ulBootAddress
        fill_b2(0x0136),                                     // uiUpperExtIOLoc
        fill_b4(0x20000),                                    // ulFlashSize
        {0},                                                 // ucEepromInst
        {0},                                                 // ucFlashInst
        0x3e,                                                // ucSPHaddr
        0x3d,                                                // ucSPLaddr
        fill_b2(0x10000 / 256),                              // uiFlashpages
        0,                                                   // ucDWDRAddress
        0,                                                   // ucDWBasePC
        0,                                                   // ucAllowFullPageBitstream
        fill_b2(0),                                          // uiStartSmallestBootLoaderSection
        1,                                                   // EnablePageProgramming
        0,                                                   // ucCacheType
        fill_b2(0x200),                                      // uiSramStartAddr
        0,                                                   // ucResetType
        0,                                                   // ucPCMaskExtended
        0,                                                   // ucPCMaskHigh
        0x3c,                                                // ucEindAddress
        fill_b2(0x1F),                                       // EECRAddress
    },
    {0} // Xmega device descr.
};

} // namespace
