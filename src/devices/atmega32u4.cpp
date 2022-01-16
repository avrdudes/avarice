#include "../jtag.h"

namespace {

constexpr gdb_io_reg_def_type atmega32u4_io_registers[] = {{"PINB", 0x23, 0x00},
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
                                                           {"PLLCSR", 0x49, 0x00},
                                                           {"GPIOR1", 0x4a, 0x00},
                                                           {"GPIOR2", 0x4b, 0x00},
                                                           {"SPCR", 0x4c, 0x00},
                                                           {"SPSR", 0x4d, 0x00},
                                                           {"SPDR", 0x4e, 0x00},
                                                           {"ACSR", 0x50, 0x00},
                                                           {"OCDR", 0x51, 0x00},
                                                           {"PLLFRQ", 0x52, 0x00},
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
                                                           {"RCCTRL", 0x67, 0x00},
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
                                                           {"UHCON", 0x9e, 0x00},
                                                           {"UHINT", 0x9f, 0x00},
                                                           {"UHIEN", 0xa0, 0x00},
                                                           {"UHADDR", 0xa1, 0x00},
                                                           {"UHFNUML", 0xa2, 0x00},
                                                           {"UHFNUMH", 0xa3, 0x00},
                                                           {"UHFLEN", 0xa4, 0x00},
                                                           {"UPINRQX", 0xa5, 0x00},
                                                           {"UPINTX", 0xa6, 0x00},
                                                           {"UPNUM", 0xa7, 0x00},
                                                           {"UPRST", 0xa8, 0x00},
                                                           {"UPCONX", 0xa9, 0x00},
                                                           {"UPCFG0X", 0xaa, 0x00},
                                                           {"UPCFG1X", 0xab, 0x00},
                                                           {"UPSTAX", 0xac, 0x00},
                                                           {"UPCFG2X", 0xad, 0x00},
                                                           {"UPIENX", 0xae, 0x00},
                                                           {"UPDATX", 0xaf, 0x00},
                                                           {"TCCR2A", 0xb0, 0x00},
                                                           {"TCCR2B", 0xb1, 0x00},
                                                           {"TCNT2", 0xb2, 0x00},
                                                           {"OCR2A", 0xb3, 0x00},
                                                           {"OCR2B", 0xb4, 0x00},
                                                           {"TWBR", 0xb8, 0x00},
                                                           {"TWSR", 0xb9, 0x00},
                                                           {"TWAR", 0xba, 0x00},
                                                           {"TWDR", 0xbb, 0x00},
                                                           {"TWCR", 0xbc, 0x00},
                                                           {"TWAMR", 0xbd, 0x00},
                                                           {"TCNT4L", 0xbe, 0x00},
                                                           {"TCNT4H -- TC4H", 0xbf, 0x00},
                                                           {"TCCR4A", 0xc0, 0x00},
                                                           {"TCCR4B", 0xc1, 0x00},
                                                           {"TCCR4C", 0xc2, 0x00},
                                                           {"TCCR4D", 0xc3, 0x00},
                                                           {"TCCR4E", 0xc4, 0x00},
                                                           {"CLKSEL0", 0xc5, 0x00},
                                                           {"CLKSEL1", 0xc6, 0x00},
                                                           {"CLKSTA", 0xc7, 0x00},
                                                           {"UCSR1A", 0xc8, 0x00},
                                                           {"UCSR1B", 0xc9, 0x00},
                                                           {"UCSR1C", 0xca, 0x00},
                                                           {"UBRR1L", 0xcc, 0x00},
                                                           {"UBRR1H", 0xcd, 0x00},
                                                           {"UDR1", 0xce, IO_REG_RSE},
                                                           {"OCR4A", 0xcf, 0x00},
                                                           {"OCR4B", 0xd0, 0x00},
                                                           {"OCR4C", 0xd1, 0x00},
                                                           {"OCR4D", 0xd2, 0x00},
                                                           {"DT4", 0xd4, 0x00},
                                                           {"UHWCON", 0xd7, 0x00},
                                                           {"USBCON", 0xd8, 0x00},
                                                           {"USBSTA", 0xd9, 0x00},
                                                           {"USBINT", 0xda, 0x00},
                                                           {"OTGCON", 0xdd, 0x00},
                                                           {"OTGIEN", 0xde, 0x00},
                                                           {"OTGINT", 0xdf, 0x00},
                                                           {"UDCON", 0xe0, 0x00},
                                                           {"UDINT", 0xe1, 0x00},
                                                           {"UDIEN", 0xe2, 0x00},
                                                           {"UDADDR", 0xe3, 0x00},
                                                           {"UDFNUML", 0xe4, 0x00},
                                                           {"UDFNUMH", 0xe5, 0x00},
                                                           {"UDMFN", 0xe6, 0x00},
                                                           {"UDTST", 0xe7, 0x00},
                                                           {"UEINTX", 0xe8, 0x00},
                                                           {"UENUM", 0xe9, 0x00},
                                                           {"UERST", 0xea, 0x00},
                                                           {"UECONX", 0xeb, 0x00},
                                                           {"UECFG0X", 0xec, 0x00},
                                                           {"UECFG1X", 0xed, 0x00},
                                                           {"UESTA0X", 0xee, 0x00},
                                                           {"UESTA1X", 0xef, 0x00},
                                                           {"UEIENX", 0xf0, 0x00},
                                                           {"UEDATX", 0xf1, 0x00},
                                                           {"UEBCLX", 0xf2, 0x00},
                                                           {"UEBCHX", 0xf3, 0x00},
                                                           {"UEINT", 0xf4, 0x00},
                                                           {"UPERRX", 0xf5, 0x00},
                                                           {"UPBCLX", 0xf6, 0x00},
                                                           {"UPBCHX", 0xf7, 0x00},
                                                           {"UPINT", 0xf8, 0x00},
                                                           {"OTGTCON", 0xf9, 0x00},
                                                           {nullptr, 0, 0}};

[[maybe_unused]] const jtag_device_def_type device{
    "atmega32u4",
    0x9587,
    128,
    256, // 32768 bytes flash
    4,
    256,    // 1024 bytes EEPROM
    43 * 4, // 43 interrupt vectors
    NO_TWEAKS,
    atmega32u4_io_registers,
    0x07,
    0x8000, // fuses
    0x66,   // osccal
    3,      // OCD revision
    nullptr,
    {
        CMND_SET_DEVICE_DESCRIPTOR,
        {0xF8, 0xFF, 0xE3, 0xFB, 0xFF, 0x3F, 0xBD, 0xF8}, // ucReadIO
        {0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00}, // ucReadIOShadow
        {0xB0, 0x6D, 0x03, 0xE0, 0xFF, 0x1F, 0xBC, 0xF8}, // ucWriteIO
        {0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00}, // ucWriteIOShadow
        {0xF3, 0xCF, 0x07, 0xDF, 0xF7, 0x3F, 0xF7, 0x3F, 0x00, 0x00, 0x1F, 0xFF, 0xFF, 0xB7, 0x97,
         0x07, 0x7F, 0xFF, 0x1D}, // ucReadExtIO
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // ucReadIOExtShadow
        {0xF3, 0xCF, 0x07, 0xD8, 0xF7, 0x3F, 0xF7, 0x3F, 0x00, 0x00, 0x1F, 0xEF, 0x7F, 0xB6, 0x97,
         0x05, 0x0F, 0x7F, 0x11}, // ucWriteExtIO
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // ucWriteIOExtShadow
        0x31,                                                         // ucIDRAddress
        0x57,                                                         // ucSPMCRAddress
        0x3B,                                                         // ucRAMPZAddress
        128,                                                 // uiFlashPageSize
        4,                                                            // ucEepromPageSize
        0x3F00,                                              // ulBootAddress
        0x00FF,                                              // uiUpperExtIOLoc
        32768,                                               // ulFlashSize
        {0x00},                                                       // ucEepromInst
        {0x00},                                                       // ucFlashInst
        0x3E,                                                         // ucSPHaddr
        0x3D,                                                         // ucSPLaddr
        32768 / 128,                                         // uiFlashpages
        0x00,                                                         // ucDWDRAddress
        0x00,                                                         // ucDWBasePC
        0x00,                                                         // ucAllowFullPageBitstream
        0x00,  // uiStartSmallestBootLoaderSection
        1,              // EnablePageProgramming
        0,              // ucCacheType
        0x100, // uiSramStartAddr
        0,              // ucResetType
        0,              // ucPCMaskExtended
        0,              // ucPCMaskHigh
        0x3C,           // ucEindAddress
        0x1F,  // EECRAddress
    },
    nullptr
};

} // namespace
