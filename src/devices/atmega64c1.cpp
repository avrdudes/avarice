#include "../jtag.h"

namespace {

[[maybe_unused]] const jtag_device_def_type atmega64c1{
    "atmega64c1",
    0x9686,
    256,
    256, // 65536 bytes flash
    8,
    256,    // 2048 bytes EEPROM
    31 * 4, // 31 interrupt vectors
    DEVFL_MKII_ONLY,
    nullptr, // registers not yet defined
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
        {0x53, 0xC2, 0xE0, 0xDF, 0xF7, 0x0F, 0xF7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x07,
         0x5F, 0x1D, 0xF0, 0xFF}, // ucReadExtIO
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // ucReadIOExtShadow
        {0x10, 0xC2, 0xE0, 0xD8, 0xF7, 0x0F, 0xF7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF7, 0x07,
         0x4D, 0x1C, 0xF0, 0xFF}, // ucWriteExtIO
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // ucWriteIOExtShadow
        0x00,                                                         // ucIDRAddress
        0x57,                                                         // ucSPMCRAddress
        0,                                                            // ucRAMPZAddress
        fill_b2(256),                                                 // uiFlashPageSize
        8,                                                            // ucEepromPageSize
        fill_b4(0x7E00),                                              // ulBootAddress
        fill_b2(0x00FA),                                              // uiUpperExtIOLoc
        fill_b4(65536),                                               // ulFlashSize
        {0xBD, 0xF2, 0xBD, 0xE1, 0xBB, 0xCF, 0xB4, 0x00, 0xBE, 0x01,
         0xB6, 0x01, 0xBC, 0x00, 0xBB, 0xBF, 0x99, 0xF9, 0xBB, 0xAF}, // ucEepromInst
        {0xB6, 0x01, 0x11},                                           // ucFlashInst
        0x3E,                                                         // ucSPHaddr
        0x3D,                                                         // ucSPLaddr
        fill_b2(65536 / 256),                                         // uiFlashpages
        0x31,                                                         // ucDWDRAddress
        0x00,                                                         // ucDWBasePC
        0x00,                                                         // ucAllowFullPageBitstream
        fill_b2(0x7E00), // uiStartSmallestBootLoaderSection
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

}
