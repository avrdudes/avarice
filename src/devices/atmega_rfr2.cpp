#include "../jtag.h"

namespace {

constexpr gdb_io_reg_def_type io_registers[] = {{"PINA", 0x20, 0x00},
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
                                                {"OCDR", 0x51, 0x00},
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
                                                {"PRR2", 0x63, 0x00},
                                                {"PRR0", 0x64, 0x00},
                                                {"PRR1", 0x65, 0x00},
                                                {"OSCCAL", 0x66, 0x00},
                                                {"BGCR", 0x67, 0x00},
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
                                                {"NEMCR", 0x75, 0x00},
                                                {"ADCSRC", 0x77, 0x00},
                                                {"ADCL -- ADCWL", 0x78, 0x00},
                                                {"ADCH -- ADCWH", 0x79, 0x00},
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
                                                {"IRQ_MASK1", 0xbe, 0x00},
                                                {"IRQ_STATUS1", 0xbf, 0x00},
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
                                                {"SCRSTRLL", 0xd7, 0x00},
                                                {"SCRSTRLH", 0xd8, 0x00},
                                                {"SCRSTRHL", 0xd9, 0x00},
                                                {"SCRSTRHH", 0xda, 0x00},
                                                {"SCCSR", 0xdb, 0x00},
                                                {"SCCR0", 0xdc, 0x00},
                                                {"SCCR1", 0xdd, 0x00},
                                                {"SCSR", 0xde, 0x00},
                                                {"SCIRQM", 0xdf, 0x00},
                                                {"SCIRQS", 0xe0, 0x00},
                                                {"SCCNTLL", 0xe1, 0x00},
                                                {"SCCNTLH", 0xe2, 0x00},
                                                {"SCCNTHL", 0xe3, 0x00},
                                                {"SCCNTHH", 0xe4, 0x00},
                                                {"SCBTSRLL", 0xe5, 0x00},
                                                {"SCBTSRLH", 0xe6, 0x00},
                                                {"SCBTSRHL", 0xe7, 0x00},
                                                {"SCBTSRHH", 0xe8, 0x00},
                                                {"SCTSRLL", 0xe9, 0x00},
                                                {"SCTSRLH", 0xea, 0x00},
                                                {"SCTSRHL", 0xeb, 0x00},
                                                {"SCTSRHH", 0xec, 0x00},
                                                {"SCOCR3LL", 0xed, 0x00},
                                                {"SCOCR3LH", 0xee, 0x00},
                                                {"SCOCR3HL", 0xef, 0x00},
                                                {"SCOCR3HH", 0xf0, 0x00},
                                                {"SCOCR2LL", 0xf1, 0x00},
                                                {"SCOCR2LH", 0xf2, 0x00},
                                                {"SCOCR2HL", 0xf3, 0x00},
                                                {"SCOCR2HH", 0xf4, 0x00},
                                                {"SCOCR1LL", 0xf5, 0x00},
                                                {"SCOCR1LH", 0xf6, 0x00},
                                                {"SCOCR1HL", 0xf7, 0x00},
                                                {"SCOCR1HH", 0xf8, 0x00},
                                                {"SCTSTRLL", 0xf9, 0x00},
                                                {"SCTSTRLH", 0xfa, 0x00},
                                                {"SCTSTRHL", 0xfb, 0x00},
                                                {"SCTSTRHH", 0xfc, 0x00},
                                                {"MAFCR0", 0x10c, 0x00},
                                                {"MAFCR1", 0x10d, 0x00},
                                                {"MAFSA0L", 0x10e, 0x00},
                                                {"MAFSA0H", 0x10f, 0x00},
                                                {"MAFPA0L", 0x110, 0x00},
                                                {"MAFPA0H", 0x111, 0x00},
                                                {"MAFSA1L", 0x112, 0x00},
                                                {"MAFSA1H", 0x113, 0x00},
                                                {"MAFPA1L", 0x114, 0x00},
                                                {"MAFPA1H", 0x115, 0x00},
                                                {"MAFSA2L", 0x116, 0x00},
                                                {"MAFSA2H", 0x117, 0x00},
                                                {"MAFPA2L", 0x118, 0x00},
                                                {"MAFPA2H", 0x119, 0x00},
                                                {"MAFSA3L", 0x11a, 0x00},
                                                {"MAFSA3H", 0x11b, 0x00},
                                                {"MAFPA3L", 0x11c, 0x00},
                                                {"MAFPA3H", 0x11d, 0x00},
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
                                                {"LLCR", 0x12f, 0x00},
                                                {"LLDRL", 0x130, 0x00},
                                                {"LLDRH", 0x131, 0x00},
                                                {"DRTRAM3", 0x132, 0x00},
                                                {"DRTRAM2", 0x133, 0x00},
                                                {"DRTRAM1", 0x134, 0x00},
                                                {"DRTRAM0", 0x135, 0x00},
                                                {"DPDS0", 0x136, 0x00},
                                                {"DPDS1", 0x137, 0x00},
                                                {"PARCR", 0x138, 0x00},
                                                {"TRXPR", 0x139, 0x00},
                                                {"AES_CTRL", 0x13c, 0x00},
                                                {"AES_STATUS", 0x13d, 0x00},
                                                {"AES_STATE", 0x13e, 0x00},
                                                {"AES_KEY", 0x13f, 0x00},
                                                {"TRX_STATUS", 0x141, 0x00},
                                                {"TRX_STATE", 0x142, 0x00},
                                                {"TRX_CTRL_0", 0x143, 0x00},
                                                {"TRX_CTRL_1", 0x144, 0x00},
                                                {"PHY_TX_PWR", 0x145, 0x00},
                                                {"PHY_RSSI", 0x146, 0x00},
                                                {"PHY_ED_LEVEL", 0x147, 0x00},
                                                {"PHY_CC_CCA", 0x148, 0x00},
                                                {"CCA_THRES", 0x149, 0x00},
                                                {"RX_CTRL", 0x14a, 0x00},
                                                {"SFD_VALUE", 0x14b, 0x00},
                                                {"TRX_CTRL_2", 0x14c, 0x00},
                                                {"ANT_DIV", 0x14d, 0x00},
                                                {"IRQ_MASK", 0x14e, 0x00},
                                                {"IRQ_STATUS", 0x14f, 0x00},
                                                {"VREG_CTRL", 0x150, 0x00},
                                                {"BATMON", 0x151, 0x00},
                                                {"XOSC_CTRL", 0x152, 0x00},
                                                {"CC_CTRL_0", 0x153, 0x00},
                                                {"CC_CTRL_1", 0x154, 0x00},
                                                {"RX_SYN", 0x155, 0x00},
                                                {"TRX_RPC", 0x156, 0x00},
                                                {"XAH_CTRL_1", 0x157, 0x00},
                                                {"FTN_CTRL", 0x158, 0x00},
                                                {"PLL_CF", 0x15a, 0x00},
                                                {"PLL_DCU", 0x15b, 0x00},
                                                {"PART_NUM", 0x15c, 0x00},
                                                {"VERSION_NUM", 0x15d, 0x00},
                                                {"MAN_ID_0", 0x15e, 0x00},
                                                {"MAN_ID_1", 0x15f, 0x00},
                                                {"SHORT_ADDR_0", 0x160, 0x00},
                                                {"SHORT_ADDR_1", 0x161, 0x00},
                                                {"PAN_ID_0", 0x162, 0x00},
                                                {"PAN_ID_1", 0x163, 0x00},
                                                {"IEEE_ADDR_0", 0x164, 0x00},
                                                {"IEEE_ADDR_1", 0x165, 0x00},
                                                {"IEEE_ADDR_2", 0x166, 0x00},
                                                {"IEEE_ADDR_3", 0x167, 0x00},
                                                {"IEEE_ADDR_4", 0x168, 0x00},
                                                {"IEEE_ADDR_5", 0x169, 0x00},
                                                {"IEEE_ADDR_6", 0x16a, 0x00},
                                                {"IEEE_ADDR_7", 0x16b, 0x00},
                                                {"XAH_CTRL_0", 0x16c, 0x00},
                                                {"CSMA_SEED_0", 0x16d, 0x00},
                                                {"CSMA_SEED_1", 0x16e, 0x00},
                                                {"CSMA_BE", 0x16f, 0x00},
                                                {"TST_CTRL_DIGI", 0x176, 0x00},
                                                {"TST_RX_LENGTH", 0x17b, 0x00},
                                                {"TST_AGC", 0x17c, 0x00},
                                                {"TST_SDM", 0x17d, 0x00},
                                                {"TRXFBST", 0x180, 0x00},
                                                {"TRXFBEND", 0x1ff, 0x00},
                                                {nullptr, 0, 0}};

[[maybe_unused]] const jtag_device_def_type atmega256rfr2{
    "atmega256rfr2",
    0xA802,
    256,
    1024, // 262144 bytes flash
    8,
    1024,   // 8192 bytes EEPROM
    77 * 4, // 77 interrupt vectors
    NO_TWEAKS,
    io_registers,
    0x07,
    0x8000, // fuses
    0x66,   // osccal
    4,      // OCD revision
    nullptr,
    {
        CMND_SET_DEVICE_DESCRIPTOR,
        {0xFF, 0xFF, 0xFF, 0xF9, 0xFF, 0x3D, 0xB9, 0xF8}, // ucReadIO
        {0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00}, // ucReadIOShadow
        {0xFF, 0xFF, 0x1F, 0xE0, 0xFF, 0x1D, 0xA9, 0xF8}, // ucWriteIO
        {0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00}, // ucWriteIOShadow
        {0xFB, 0xFF, 0xBF, 0xFF, 0xF7, 0x3F, 0xF7, 0x3F, 0xF7, 0x3F, 0x5F, 0x3F, 0x77, 0x77,
         0x03, 0xF0, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0xF7, 0xBF, 0xFF, 0xFA,
         0xFE, 0xFF, 0xA7, 0xFF, 0xFF, 0xFF, 0xEF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
         0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, // ucReadExtIO
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // ucReadIOExtShadow
        {0xFB, 0xFF, 0xBF, 0xFF, 0xB7, 0x3F, 0xB7, 0x3F, 0xB7, 0x3F, 0x5F, 0x3F, 0x77, 0x77,
         0x03, 0xB0, 0xFF, 0xE1, 0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0xB7, 0xBF, 0xFF, 0xDA,
         0x3C, 0xFF, 0xA7, 0x0F, 0xFF, 0xFF, 0xE8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
         0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, // ucWriteExtIO
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // ucWriteIOExtShadow
        0x31,                                                         // ucIDRAddress
        0x57,                                                         // ucSPMCRAddress
        0x3B,                                                         // ucRAMPZAddress
        256,                                                          // uiFlashPageSize
        8,                                                            // ucEepromPageSize
        0x1FE00,                                                      // ulBootAddress
        0x01FF,                                                       // uiUpperExtIOLoc
        262144,                                                       // ulFlashSize
        {0x00},                                                       // ucEepromInst
        {0x00},                                                       // ucFlashInst
        0x3E,                                                         // ucSPHaddr
        0x3D,                                                         // ucSPLaddr
        262144 / 256,                                                 // uiFlashpages
        0x00,                                                         // ucDWDRAddress
        0x00,                                                         // ucDWBasePC
        0x00,                                                         // ucAllowFullPageBitstream
        0x00,  // uiStartSmallestBootLoaderSection
        1,     // EnablePageProgramming
        0,     // ucCacheType
        0x200, // uiSramStartAddr
        0,     // ucResetType
        0,     // ucPCMaskExtended
        0,     // ucPCMaskHigh
        0x3C,  // ucEindAddress
        0x1F,  // EECRAddress
    },
    nullptr};

[[maybe_unused]] const jtag_device_def_type atmega64rfr2{
    "atmega64rfr2",
    0xA602,
    256,
    256, // 65536 bytes flash
    8,
    256,    // 2048 bytes EEPROM
    77 * 4, // 77 interrupt vectors
    NO_TWEAKS,
    io_registers,
    0x07,
    0x8000, // fuses
    0x66,   // osccal
    3,      // OCD revision
    nullptr,
    {
        CMND_SET_DEVICE_DESCRIPTOR,
        {0xFF, 0xFF, 0xFF, 0xF9, 0xFF, 0x3D, 0xB9, 0xF8}, // ucReadIO
        {0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00}, // ucReadIOShadow
        {0xFF, 0xFF, 0x1F, 0xE0, 0xFF, 0x1D, 0xA9, 0xF8}, // ucWriteIO
        {0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00}, // ucWriteIOShadow
        {0xFB, 0xFF, 0xBF, 0xFF, 0xF7, 0x3F, 0xF7, 0x3F, 0xF7, 0x3F, 0x5F, 0x3F, 0x77, 0x77,
         0x03, 0xF0, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0xF7, 0xBF, 0xFF, 0xFA,
         0xFE, 0xFF, 0xA7, 0xFF, 0xFF, 0xFF, 0xEF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
         0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, // ucReadExtIO
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // ucReadIOExtShadow
        {0xFB, 0xFF, 0xBF, 0xFF, 0xB7, 0x3F, 0xB7, 0x3F, 0xB7, 0x3F, 0x5F, 0x3F, 0x77, 0x77,
         0x03, 0xB0, 0xFF, 0xE1, 0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0xB7, 0xBF, 0xFF, 0xDA,
         0x3C, 0xFF, 0xA7, 0x0F, 0xFF, 0xFF, 0xE8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
         0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, // ucWriteExtIO
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // ucWriteIOExtShadow
        0x31,                                                         // ucIDRAddress
        0x57,                                                         // ucSPMCRAddress
        0x3B,                                                         // ucRAMPZAddress
        256,                                                          // uiFlashPageSize
        8,                                                            // ucEepromPageSize
        0x7E00,                                                       // ulBootAddress
        0x01FF,                                                       // uiUpperExtIOLoc
        65536,                                                        // ulFlashSize
        {0x00},                                                       // ucEepromInst
        {0x00},                                                       // ucFlashInst
        0x3E,                                                         // ucSPHaddr
        0x3D,                                                         // ucSPLaddr
        65536 / 256,                                                  // uiFlashpages
        0x00,                                                         // ucDWDRAddress
        0x00,                                                         // ucDWBasePC
        0x00,                                                         // ucAllowFullPageBitstream
        0x00,  // uiStartSmallestBootLoaderSection
        1,     // EnablePageProgramming
        0,     // ucCacheType
        0x200, // uiSramStartAddr
        0,     // ucResetType
        0,     // ucPCMaskExtended
        0,     // ucPCMaskHigh
        0x3C,  // ucEindAddress
        0x1F,  // EECRAddress
    },
    nullptr};

[[maybe_unused]] const jtag_device_def_type atmega644rfr2{
    "atmega644rfr2",
    0xA603,
    256,
    256, // 65536 bytes flash
    8,
    256,    // 2048 bytes EEPROM
    77 * 4, // 77 interrupt vectors
    NO_TWEAKS,
    io_registers,
    0x07,
    0x8000, // fuses
    0x66,   // osccal
    3,      // OCD revision
    nullptr,
    {
        CMND_SET_DEVICE_DESCRIPTOR,
        {0xFF, 0xFF, 0xFF, 0xF9, 0xFF, 0x3D, 0xB9, 0xF8}, // ucReadIO
        {0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00}, // ucReadIOShadow
        {0xFF, 0xFF, 0x1F, 0xE0, 0xFF, 0x1D, 0xA9, 0xF8}, // ucWriteIO
        {0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00}, // ucWriteIOShadow
        {0xFB, 0xFF, 0xBF, 0xFF, 0xF7, 0x3F, 0xF7, 0x3F, 0xF7, 0x3F, 0x5F, 0x3F, 0x77, 0x77,
         0x03, 0xF0, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0xF7, 0xBF, 0xFF, 0xFA,
         0xFE, 0xFF, 0xA7, 0xFF, 0xFF, 0xFF, 0xEF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
         0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, // ucReadExtIO
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // ucReadIOExtShadow
        {0xFB, 0xFF, 0xBF, 0xFF, 0xB7, 0x3F, 0xB7, 0x3F, 0xB7, 0x3F, 0x5F, 0x3F, 0x77, 0x77,
         0x03, 0xB0, 0xFF, 0xE1, 0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0xB7, 0xBF, 0xFF, 0xDA,
         0x3C, 0xFF, 0xA7, 0x0F, 0xFF, 0xFF, 0xE8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
         0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, // ucWriteExtIO
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // ucWriteIOExtShadow
        0x31,                                                         // ucIDRAddress
        0x57,                                                         // ucSPMCRAddress
        0x3B,                                                         // ucRAMPZAddress
        256,                                                          // uiFlashPageSize
        8,                                                            // ucEepromPageSize
        0x7E00,                                                       // ulBootAddress
        0x01FF,                                                       // uiUpperExtIOLoc
        65536,                                                        // ulFlashSize
        {0x00},                                                       // ucEepromInst
        {0x00},                                                       // ucFlashInst
        0x3E,                                                         // ucSPHaddr
        0x3D,                                                         // ucSPLaddr
        65536 / 256,                                                  // uiFlashpages
        0x00,                                                         // ucDWDRAddress
        0x00,                                                         // ucDWBasePC
        0x00,                                                         // ucAllowFullPageBitstream
        0x00,  // uiStartSmallestBootLoaderSection
        1,     // EnablePageProgramming
        0,     // ucCacheType
        0x200, // uiSramStartAddr
        0,     // ucResetType
        0,     // ucPCMaskExtended
        0,     // ucPCMaskHigh
        0x3C,  // ucEindAddress
        0x1F,  // EECRAddress
    },
    nullptr};

[[maybe_unused]] const jtag_device_def_type atmega128rfr2{
    "atmega128rfr2",
    0xA702,
    256,
    512, // 131072 bytes flash
    8,
    512,    // 4096 bytes EEPROM
    77 * 4, // 77 interrupt vectors
    NO_TWEAKS,
    io_registers,
    0x07,
    0x8000, // fuses
    0x66,   // osccal
    3,      // OCD revision
    nullptr,
    {
        CMND_SET_DEVICE_DESCRIPTOR,
        {0xFF, 0xFF, 0xFF, 0xF9, 0xFF, 0x3D, 0xB9, 0xF8}, // ucReadIO
        {0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00}, // ucReadIOShadow
        {0xFF, 0xFF, 0x1F, 0xE0, 0xFF, 0x1D, 0xA9, 0xF8}, // ucWriteIO
        {0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00}, // ucWriteIOShadow
        {0xFB, 0xFF, 0xBF, 0xFF, 0xF7, 0x3F, 0xF7, 0x3F, 0xF7, 0x3F, 0x5F, 0x3F, 0x77, 0x77,
         0x03, 0xF0, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0xF7, 0xBF, 0xFF, 0xFA,
         0xFE, 0xFF, 0xA7, 0xFF, 0xFF, 0xFF, 0xEF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
         0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, // ucReadExtIO
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // ucReadIOExtShadow
        {0xFB, 0xFF, 0xBF, 0xFF, 0xB7, 0x3F, 0xB7, 0x3F, 0xB7, 0x3F, 0x5F, 0x3F, 0x77, 0x77,
         0x03, 0xB0, 0xFF, 0xE1, 0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0xB7, 0xBF, 0xFF, 0xDA,
         0x3C, 0xFF, 0xA7, 0x0F, 0xFF, 0xFF, 0xE8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
         0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, // ucWriteExtIO
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // ucWriteIOExtShadow
        0x31,                                                         // ucIDRAddress
        0x57,                                                         // ucSPMCRAddress
        0x3B,                                                         // ucRAMPZAddress
        256,                                                          // uiFlashPageSize
        8,                                                            // ucEepromPageSize
        0xFE00,                                                       // ulBootAddress
        0x01FF,                                                       // uiUpperExtIOLoc
        131072,                                                       // ulFlashSize
        {0x00},                                                       // ucEepromInst
        {0x00},                                                       // ucFlashInst
        0x3E,                                                         // ucSPHaddr
        0x3D,                                                         // ucSPLaddr
        131072 / 256,                                                 // uiFlashpages
        0x00,                                                         // ucDWDRAddress
        0x00,                                                         // ucDWBasePC
        0x00,                                                         // ucAllowFullPageBitstream
        0x00,  // uiStartSmallestBootLoaderSection
        1,     // EnablePageProgramming
        0,     // ucCacheType
        0x200, // uiSramStartAddr
        0,     // ucResetType
        0,     // ucPCMaskExtended
        0,     // ucPCMaskHigh
        0x3C,  // ucEindAddress
        0x1F,  // EECRAddress
    },
    nullptr};

[[maybe_unused]] const jtag_device_def_type atmega1284rfr2{
    "atmega1284rfr2",
    0xA703,
    256,
    512, // 131072 bytes flash
    8,
    512,    // 4096 bytes EEPROM
    77 * 4, // 77 interrupt vectors
    NO_TWEAKS,
    io_registers,
    0x07,
    0x8000, // fuses
    0x66,   // osccal
    3,      // OCD revision
    nullptr,
    {
        CMND_SET_DEVICE_DESCRIPTOR,
        {0xFF, 0xFF, 0xFF, 0xF9, 0xFF, 0x3D, 0xB9, 0xF8}, // ucReadIO
        {0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00}, // ucReadIOShadow
        {0xFF, 0xFF, 0x1F, 0xE0, 0xFF, 0x1D, 0xA9, 0xF8}, // ucWriteIO
        {0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00}, // ucWriteIOShadow
        {0xFB, 0xFF, 0xBF, 0xFF, 0xF7, 0x3F, 0xF7, 0x3F, 0xF7, 0x3F, 0x5F, 0x3F, 0x77, 0x77,
         0x03, 0xF0, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0xF7, 0xBF, 0xFF, 0xFA,
         0xFE, 0xFF, 0xA7, 0xFF, 0xFF, 0xFF, 0xEF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
         0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, // ucReadExtIO
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // ucReadIOExtShadow
        {0xFB, 0xFF, 0xBF, 0xFF, 0xB7, 0x3F, 0xB7, 0x3F, 0xB7, 0x3F, 0x5F, 0x3F, 0x77, 0x77,
         0x03, 0xB0, 0xFF, 0xE1, 0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0xB7, 0xBF, 0xFF, 0xDA,
         0x3C, 0xFF, 0xA7, 0x0F, 0xFF, 0xFF, 0xE8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
         0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, // ucWriteExtIO
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // ucWriteIOExtShadow
        0x31,                                                         // ucIDRAddress
        0x57,                                                         // ucSPMCRAddress
        0x3B,                                                         // ucRAMPZAddress
        256,                                                          // uiFlashPageSize
        8,                                                            // ucEepromPageSize
        0xFE00,                                                       // ulBootAddress
        0x01FF,                                                       // uiUpperExtIOLoc
        131072,                                                       // ulFlashSize
        {0x00},                                                       // ucEepromInst
        {0x00},                                                       // ucFlashInst
        0x3E,                                                         // ucSPHaddr
        0x3D,                                                         // ucSPLaddr
        131072 / 256,                                                 // uiFlashpages
        0x00,                                                         // ucDWDRAddress
        0x00,                                                         // ucDWBasePC
        0x00,                                                         // ucAllowFullPageBitstream
        0x00,  // uiStartSmallestBootLoaderSection
        1,     // EnablePageProgramming
        0,     // ucCacheType
        0x200, // uiSramStartAddr
        0,     // ucResetType
        0,     // ucPCMaskExtended
        0,     // ucPCMaskHigh
        0x3C,  // ucEindAddress
        0x1F,  // EECRAddress
    },
    nullptr};

[[maybe_unused]] const jtag_device_def_type atmega2564rfr2{
    "atmega2564rfr2",
    0xA803,
    256,
    1024, // 262144 bytes flash
    8,
    1024,   // 8192 bytes EEPROM
    77 * 4, // 77 interrupt vectors
    NO_TWEAKS,
    io_registers,
    0x07,
    0x8000, // fuses
    0x66,   // osccal
    4,      // OCD revision
    nullptr,
    {
        CMND_SET_DEVICE_DESCRIPTOR,
        {0xFF, 0xFF, 0xFF, 0xF9, 0xFF, 0x3D, 0xB9, 0xF8}, // ucReadIO
        {0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00}, // ucReadIOShadow
        {0xFF, 0xFF, 0x1F, 0xE0, 0xFF, 0x1D, 0xA9, 0xF8}, // ucWriteIO
        {0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00}, // ucWriteIOShadow
        {0xFB, 0xFF, 0xBF, 0xFF, 0xF7, 0x3F, 0xF7, 0x3F, 0xF7, 0x3F, 0x5F, 0x3F, 0x77, 0x77,
         0x03, 0xF0, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0xF7, 0xBF, 0xFF, 0xFA,
         0xFE, 0xFF, 0xA7, 0xFF, 0xFF, 0xFF, 0xEF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
         0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, // ucReadExtIO
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // ucReadIOExtShadow
        {0xFB, 0xFF, 0xBF, 0xFF, 0xB7, 0x3F, 0xB7, 0x3F, 0xB7, 0x3F, 0x5F, 0x3F, 0x77, 0x77,
         0x03, 0xB0, 0xFF, 0xE1, 0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0xB7, 0xBF, 0xFF, 0xDA,
         0x3C, 0xFF, 0xA7, 0x0F, 0xFF, 0xFF, 0xE8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
         0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, // ucWriteExtIO
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // ucWriteIOExtShadow
        0x31,                                                         // ucIDRAddress
        0x57,                                                         // ucSPMCRAddress
        0x3B,                                                         // ucRAMPZAddress
        256,                                                          // uiFlashPageSize
        8,                                                            // ucEepromPageSize
        0x1FE00,                                                      // ulBootAddress
        0x01FF,                                                       // uiUpperExtIOLoc
        262144,                                                       // ulFlashSize
        {0x00},                                                       // ucEepromInst
        {0x00},                                                       // ucFlashInst
        0x3E,                                                         // ucSPHaddr
        0x3D,                                                         // ucSPLaddr
        262144 / 256,                                                 // uiFlashpages
        0x00,                                                         // ucDWDRAddress
        0x00,                                                         // ucDWBasePC
        0x00,                                                         // ucAllowFullPageBitstream
        0x00,  // uiStartSmallestBootLoaderSection
        1,     // EnablePageProgramming
        0,     // ucCacheType
        0x200, // uiSramStartAddr
        0,     // ucResetType
        0,     // ucPCMaskExtended
        0,     // ucPCMaskHigh
        0x3C,  // ucEindAddress
        0x1F,  // EECRAddress
    },
    nullptr};

} // namespace