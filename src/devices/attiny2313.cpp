#include "../jtag.h"

namespace {

constexpr gdb_io_reg_def_type attiny2313_io_registers[] = {{"DIDR", 0x21, 0x00},
                                                       {"UBRRH", 0x22, 0x00},
                                                       {"UCSRC", 0x23, 0x00},
                                                       {"ACSR", 0x28, 0x00},
                                                       {"UBRRL", 0x29, 0x00},
                                                       {"UCSRB", 0x2a, 0x00},
                                                       {"UCSRA", 0x2b, 0x00},
                                                       {"UDR -- RXB -- TXB", 0x2c, IO_REG_RSE},
                                                       {"USICR", 0x2d, 0x00},
                                                       {"USISR", 0x2e, 0x00},
                                                       {"USIDR", 0x2f, 0x00},
                                                       {"PIND", 0x30, 0x00},
                                                       {"DDRD", 0x31, 0x00},
                                                       {"PORTD", 0x32, 0x00},
                                                       {"GPIOR0", 0x33, 0x00},
                                                       {"GPIOR1", 0x34, 0x00},
                                                       {"GPIOR2", 0x35, 0x00},
                                                       {"PINB", 0x36, 0x00},
                                                       {"DDRB", 0x37, 0x00},
                                                       {"PORTB", 0x38, 0x00},
                                                       {"PINA", 0x39, 0x00},
                                                       {"DDRA", 0x3a, 0x00},
                                                       {"PORTA", 0x3b, 0x00},
                                                       {"EECR", 0x3c, 0x00},
                                                       {"EEDR", 0x3d, 0x00},
                                                       {"EEAR -- EEARL", 0x3e, 0x00},
                                                       {"PCMSK", 0x40, 0x00},
                                                       {"WDTCSR", 0x41, 0x00},
                                                       {"TCCR1C", 0x42, 0x00},
                                                       {"GTCCR", 0x43, 0x00},
                                                       {"ICR1L", 0x44, 0x00},
                                                       {"ICR1H", 0x45, 0x00},
                                                       {"CLKPR", 0x46, 0x00},
                                                       {"OCR1BL", 0x48, 0x00},
                                                       {"OCR1BH", 0x49, 0x00},
                                                       {"OCR1L -- OCR1AL", 0x4a, 0x00},
                                                       {"OCR1H -- OCR1AH", 0x4b, 0x00},
                                                       {"TCNT1L", 0x4c, 0x00},
                                                       {"TCNT1H", 0x4d, 0x00},
                                                       {"TCCR1B", 0x4e, 0x00},
                                                       {"TCCR1A", 0x4f, 0x00},
                                                       {"TCCR0A", 0x50, 0x00},
                                                       {"OSCCAL", 0x51, 0x00},
                                                       {"TCNT0", 0x52, 0x00},
                                                       {"TCCR0B", 0x53, 0x00},
                                                       {"MCUSR", 0x54, 0x00},
                                                       {"MCUCR", 0x55, 0x00},
                                                       {"OCR0A", 0x56, 0x00},
                                                       {"SPMCSR", 0x57, 0x00},
                                                       {"TIFR", 0x58, 0x00},
                                                       {"TIMSK", 0x59, 0x00},
                                                       {"EIFR", 0x5a, 0x00},
                                                       {"GIMSK", 0x5b, 0x00},
                                                       {"OCR0B", 0x5c, 0x00},
                                                       {"SPL -- SP", 0x5d, 0x00},
                                                       {"SREG", 0x5f, 0x00},
                                                       {nullptr, 0, 0}};

[[maybe_unused]] const jtag_device_def_type attiny2313{
    "attiny2313",
    0x910A,
    32,
    64, // 2048 bytes flash
    4,
    32,     // 128 bytes EEPROM
    19 * 2, // 19 interrupt vectors
    NO_TWEAKS,
    attiny2313_io_registers,
    0x07,
    0x0000, // fuses
    0x51,   // osccal
    0,      // OCD revision
    nullptr,
    {
        CMND_SET_DEVICE_DESCRIPTOR,
        {0x0E, 0xEF, 0xFF, 0x7F, 0x3F, 0xFF, 0x7F, 0xBF}, // ucReadIO
        {0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00}, // ucReadIOShadow
        {0x0E, 0xA6, 0xBE, 0x7D, 0x39, 0xFF, 0x7D, 0xBA}, // ucWriteIO
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
        32,                                                  // uiFlashPageSize
        4,                                                            // ucEepromPageSize
        0x0000,                                              // ulBootAddress
        0x00,                                                // uiUpperExtIOLoc
        2048,                                                // ulFlashSize
        {0xBB, 0xFE, 0xBB, 0xEE, 0xBB, 0xCC, 0xB2, 0x0D, 0xBA, 0x0F,
         0xB2, 0x0F, 0xBA, 0x0D, 0xBB, 0xBC, 0x99, 0xE1, 0xBB, 0xAC}, // ucEepromInst
        {0xB2, 0x0F, 0x1F},                                           // ucFlashInst
        0,                                                            // ucSPHaddr
        0x3D,                                                         // ucSPLaddr
        2048 / 32,                                           // uiFlashpages
        0x1F,                                                         // ucDWDRAddress
        0x00,                                                         // ucDWBasePC
        0x00,                                                         // ucAllowFullPageBitstream
        0x00, // uiStartSmallestBootLoaderSection
        1,             // EnablePageProgramming
        0,             // ucCacheType
        0x60, // uiSramStartAddr
        0,             // ucResetType
        0,             // ucPCMaskExtended
        0,             // ucPCMaskHigh
        0,             // ucEindAddress
        0x1C, // EECRAddress
    },
    nullptr
};

} // namespace
