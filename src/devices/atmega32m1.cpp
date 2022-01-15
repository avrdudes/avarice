#include "../jtag.h"

namespace {

constexpr gdb_io_reg_def_type atmega32m1_io_registers[] = {{"PINB", 0x23, 0x00},
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
                                                       {"TIFR0", 0x35, 0x00},
                                                       {"TIFR1", 0x36, 0x00},
                                                       {"GPIOR1", 0x39, 0x00},
                                                       {"GPIOR2", 0x3a, 0x00},
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
                                                       {"SPCR", 0x4c, 0x00},
                                                       {"SPSR", 0x4d, 0x00},
                                                       {"SPDR", 0x4e, 0x00},
                                                       {"ACSR", 0x50, 0x00},
                                                       {"DWDR", 0x51, 0x00},
                                                       {"SMCR", 0x53, 0x00},
                                                       {"MCUSR", 0x54, 0x00},
                                                       {"MCUCR", 0x55, 0x00},
                                                       {"SPMCSR", 0x57, 0x00},
                                                       {"SPL", 0x5d, 0x00},
                                                       {"SPH", 0x5e, 0x00},
                                                       {"SREG", 0x5f, 0x00},
                                                       {"WDTCSR", 0x60, 0x00},
                                                       {"CLKPR", 0x61, 0x00},
                                                       {"PRR", 0x64, 0x00},
                                                       {"OSCCAL", 0x66, 0x00},
                                                       {"PCICR", 0x68, 0x00},
                                                       {"EICRA", 0x69, 0x00},
                                                       {"PCMSK0", 0x6a, 0x00},
                                                       {"PCMSK1", 0x6b, 0x00},
                                                       {"PCMSK2", 0x6c, 0x00},
                                                       {"PCMSK3", 0x6d, 0x00},
                                                       {"TIMSK0", 0x6e, 0x00},
                                                       {"TIMSK1", 0x6f, 0x00},
                                                       {"AMP0CSR", 0x75, 0x00},
                                                       {"AMP1CSR", 0x76, 0x00},
                                                       {"AMP2CSR", 0x77, 0x00},
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
                                                       {"DACON", 0x90, 0x00},
                                                       {"DACL", 0x91, 0x00},
                                                       {"DACH", 0x92, 0x00},
                                                       {"AC0CON", 0x94, 0x00},
                                                       {"AC1CON", 0x95, 0x00},
                                                       {"AC2CON", 0x96, 0x00},
                                                       {"AC3CON", 0x97, 0x00},
                                                       {"POCR0SAL", 0xa0, 0x00},
                                                       {"POCR0SAH", 0xa1, 0x00},
                                                       {"POCR0RAL", 0xa2, 0x00},
                                                       {"POCR0RAH", 0xa3, 0x00},
                                                       {"POCR0SBL", 0xa4, 0x00},
                                                       {"POCR0SBH", 0xa5, 0x00},
                                                       {"POCR1SAL", 0xa6, 0x00},
                                                       {"POCR1SAH", 0xa7, 0x00},
                                                       {"POCR1RAL", 0xa8, 0x00},
                                                       {"POCR1RAH", 0xa9, 0x00},
                                                       {"POCR1SBL", 0xaa, 0x00},
                                                       {"POCR1SBH", 0xab, 0x00},
                                                       {"POCR2SAL", 0xac, 0x00},
                                                       {"POCR2SAH", 0xad, 0x00},
                                                       {"POCR2RAL", 0xae, 0x00},
                                                       {"POCR2RAH", 0xaf, 0x00},
                                                       {"POCR2SBL", 0xb0, 0x00},
                                                       {"POCR2SBH", 0xb1, 0x00},
                                                       {"POCRxRBL -- POCR_RBL", 0xb2, 0x00},
                                                       {"POCRxRBH -- POCR_RBH", 0xb3, 0x00},
                                                       {"PSYNC", 0xb4, 0x00},
                                                       {"PCNF", 0xb5, 0x00},
                                                       {"POC", 0xb6, 0x00},
                                                       {"PCTL", 0xb7, 0x00},
                                                       {"PMIC0", 0xb8, 0x00},
                                                       {"PMIC1", 0xb9, 0x00},
                                                       {"PMIC2", 0xba, 0x00},
                                                       {"PIM", 0xbb, 0x00},
                                                       {"PIFR", 0xbc, 0x00},
                                                       {"LINCR", 0xc8, 0x00},
                                                       {"LINSIR", 0xc9, 0x00},
                                                       {"LINENIR", 0xca, 0x00},
                                                       {"LINERR", 0xcb, 0x00},
                                                       {"LINBTR", 0xcc, 0x00},
                                                       {"LINBRRL", 0xcd, 0x00},
                                                       {"LINBRRH", 0xce, 0x00},
                                                       {"LINDLR", 0xcf, 0x00},
                                                       {"LINIDR", 0xd0, 0x00},
                                                       {"LINSEL", 0xd1, 0x00},
                                                       {"LINDAT", 0xd2, 0x00},
                                                       {"CANGCON", 0xd8, 0x00},
                                                       {"CANGSTA", 0xd9, 0x00},
                                                       {"CANGIT", 0xda, 0x00},
                                                       {"CANGIE", 0xdb, 0x00},
                                                       {"CANEN2", 0xdc, 0x00},
                                                       {"CANEN1", 0xdd, 0x00},
                                                       {"CANIE2", 0xde, 0x00},
                                                       {"CANIE1", 0xdf, 0x00},
                                                       {"CANSIT2", 0xe0, 0x00},
                                                       {"CANSIT1", 0xe1, 0x00},
                                                       {"CANBT1", 0xe2, 0x00},
                                                       {"CANBT2", 0xe3, 0x00},
                                                       {"CANBT3", 0xe4, 0x00},
                                                       {"CANTCON", 0xe5, 0x00},
                                                       {"CANTIML", 0xe6, 0x00},
                                                       {"CANTIMH", 0xe7, 0x00},
                                                       {"CANTTCL", 0xe8, 0x00},
                                                       {"CANTTCH", 0xe9, 0x00},
                                                       {"CANTEC", 0xea, 0x00},
                                                       {"CANREC", 0xeb, 0x00},
                                                       {"CANHPMOB", 0xec, 0x00},
                                                       {"CANPAGE", 0xed, 0x00},
                                                       {"CANSTMOB", 0xee, 0x00},
                                                       {"CANCDMOB", 0xef, 0x00},
                                                       {"CANIDT4", 0xf0, 0x00},
                                                       {"CANIDT3", 0xf1, 0x00},
                                                       {"CANIDT2", 0xf2, 0x00},
                                                       {"CANIDT1", 0xf3, 0x00},
                                                       {"CANIDM4", 0xf4, 0x00},
                                                       {"CANIDM3", 0xf5, 0x00},
                                                       {"CANIDM2", 0xf6, 0x00},
                                                       {"CANIDM1", 0xf7, 0x00},
                                                       {"CANSTML", 0xf8, 0x00},
                                                       {"CANSTMH", 0xf9, 0x00},
                                                       {"CANMSG", 0xfa, 0x00},
                                                       {nullptr, 0, 0}};

[[maybe_unused]] const jtag_device_def_type atmega16m1{
    "atmega32m1",
    0x9584,
    128,
    256, // 32768 bytes flash
    4,
    256,    // 1024 bytes EEPROM
    31 * 4, // 31 interrupt vectors
    DEVFL_MKII_ONLY,
    atmega32m1_io_registers,
    0x00,
    0x0000, // fuses
    0x66,   // osccal
    1,      // OCD revision
    {
        0 // no mkI support
    },
    {
        CMND_SET_DEVICE_DESCRIPTOR,
        {0xF8, 0x7F, 0x60, 0xF6, 0xFF, 0x33, 0xB9, 0xE0}, // ucReadIO
        {0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00}, // ucReadIOShadow
        {0xB0, 0x6D, 0x00, 0xE6, 0xFF, 0x13, 0xB8, 0xE0}, // ucWriteIO
        {0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00}, // ucWriteIOShadow
        {0x53, 0xC2, 0xE0, 0xDF, 0xF7, 0x0F, 0xF7, 0x00, 0xFF, 0xFF, 0xFF, 0x1F, 0x00, 0xFF, 0x07,
         0x5F, 0x1D, 0xF0, 0xFF}, // ucReadExtIO
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // ucReadIOExtShadow
        {0x10, 0xC2, 0xE0, 0xD8, 0xF7, 0x0F, 0xF7, 0x00, 0xFF, 0xFF, 0xFF, 0x1F, 0x00, 0xF7, 0x07,
         0x4D, 0x1C, 0xF0, 0xFF}, // ucWriteExtIO
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // ucWriteIOExtShadow
        0x00,                                                         // ucIDRAddress
        0x57,                                                         // ucSPMCRAddress
        0,                                                            // ucRAMPZAddress
        fill_b2(128),                                                 // uiFlashPageSize
        4,                                                            // ucEepromPageSize
        fill_b4(0x3F00),                                              // ulBootAddress
        fill_b2(0x00FA),                                              // uiUpperExtIOLoc
        fill_b4(32768),                                               // ulFlashSize
        {0xBD, 0xF2, 0xBD, 0xE1, 0xBB, 0xCF, 0xB4, 0x00, 0xBE, 0x01,
         0xB6, 0x01, 0xBC, 0x00, 0xBB, 0xBF, 0x99, 0xF9, 0xBB, 0xAF}, // ucEepromInst
        {0xB6, 0x01, 0x11},                                           // ucFlashInst
        0x3E,                                                         // ucSPHaddr
        0x3D,                                                         // ucSPLaddr
        fill_b2(32768 / 128),                                         // uiFlashpages
        0x31,                                                         // ucDWDRAddress
        0x00,                                                         // ucDWBasePC
        0x00,                                                         // ucAllowFullPageBitstream
        fill_b2(0x3F00), // uiStartSmallestBootLoaderSection
        1,               // EnablePageProgramming
        0,               // ucCacheType
        fill_b2(0x0100), // uiSramStartAddr
        0,               // ucResetType
        0,               // ucPCMaskExtended
        0,               // ucPCMaskHigh
        0,               // ucEindAddress
        fill_b2(0x1F),   // EECRAddress
    },
    nullptr
};

} // namespace
