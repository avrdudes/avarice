#include "../jtag.h"

namespace {

[[maybe_unused]] const jtag_device_def_type attiny4313{
    "attiny4313",
    0x920D,
    64,
    64, // 4096 bytes flash
    4,
    64,     // 256 bytes EEPROM
    21 * 2, // 21 interrupt vectors
    DEVFL_MKII_ONLY,
    nullptr, // registers not yet defined
    false,
    0x07,
    0x0000, // fuses
    0x51,   // osccal
    0,      // OCD revision
    {
        0 // no mkI support
    },
    {
        CMND_SET_DEVICE_DESCRIPTOR,
        {0x0E, 0xEF, 0xFF, 0x7F, 0x3F, 0xFF, 0x7F, 0xFF}, // ucReadIO
        {0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00}, // ucReadIOShadow
        {0x0E, 0xA6, 0xBE, 0x7D, 0x39, 0xFF, 0x7D, 0xFA}, // ucWriteIO
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
        fill_b2(64),                                                  // uiFlashPageSize
        4,                                                            // ucEepromPageSize
        fill_b4(0x0000),                                              // ulBootAddress
        fill_b2(0x00),                                                // uiUpperExtIOLoc
        fill_b4(4096),                                                // ulFlashSize
        {0xBB, 0xFF, 0xBB, 0xEE, 0xBB, 0xCC, 0xB2, 0x0D, 0xBC, 0x07,
         0xB4, 0x07, 0xBA, 0x0D, 0xBB, 0xBC, 0x99, 0xE1, 0xBB, 0xAC}, // ucEepromInst
        {0xB4, 0x07, 0x17},                                           // ucFlashInst
        0x3E,                                                         // ucSPHaddr
        0x3D,                                                         // ucSPLaddr
        fill_b2(4096 / 64),                                           // uiFlashpages
        0x27,                                                         // ucDWDRAddress
        0x00,                                                         // ucDWBasePC
        0x00,                                                         // ucAllowFullPageBitstream
        fill_b2(0x00), // uiStartSmallestBootLoaderSection
        1,             // EnablePageProgramming
        0,             // ucCacheType
        fill_b2(0x60), // uiSramStartAddr
        0,             // ucResetType
        0,             // ucPCMaskExtended
        0,             // ucPCMaskHigh
        0,             // ucEindAddress
        fill_b2(0x1C), // EECRAddress
    },
    {0}
};

}
