#include "../jtag.h"

namespace {

constexpr xmega_device_desc_type xmega_device_desc{
    CMND_SET_XMEGA_PARAMS, // cmd
    2,                     // whatever
    47,                    // length of following data
    0x800000,              // NVM offset for application flash
    0x810000,              // NVM offset for boot flash
    0x8c0000,              // NVM offset for EEPROM
    0x8f0020,              // NVM offset for fuses
    0x8f0027,              // NVM offset for lock bits
    0x8e0400,              // NVM offset for user signature row
    0x8e0200,              // NVM offset for production sig. row
    0x1000000,             // NVM offset for data memory
    65536,                 // size of application flash
    4096,                  // size of boot flash
    256,                   // flash page size
    2048,                  // size of EEPROM
    32,                    // EEPROM page size
    0x1c0,                 // IO space base address of NVM controller
    0x90,                  // IO space address of MCU control
};

[[maybe_unused]] const jtag_device_def_type
    device{"atxmega64b1",
           0x9652,
           256,
           272, // 69,632 bytes flash (page size. # pages)
           32,
           64,     // 2048 bytes EEPROM
           81 * 4, // 81 interrupt vectors
           NO_TWEAKS,
           nullptr, // registers not yet defined
           0x37,
           0x0000, // fuses
           0,      // osccal
           0,      // OCD revision
           nullptr,
           {
               CMND_SET_DEVICE_DESCRIPTOR,
               {0xFF, 0xFF, 0xFF, 0xF9, 0xFF, 0x3D, 0xB9, 0xF8}, // ucReadIO
               {0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00}, // ucReadIOShadow
               {0xFF, 0xFF, 0x1F, 0xE0, 0xFF, 0x1D, 0xA9, 0xF8}, // ucWriteIO
               {0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00}, // ucWriteIOShadow
               {0x73, 0xFF, 0x3F, 0xFF, 0xF7, 0x3F, 0xF7, 0x3F, 0xF7, 0x3F,
                0x5F, 0x3F, 0x37, 0x37, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00,
                0xFF, 0x0F, 0x00, 0x00, 0xF7, 0x3F, 0x36, 0x00}, // ucReadExtIO
               {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // ucReadIOExtShadow
               {0x73, 0xFF, 0x3F, 0xF8, 0xF7, 0x3F, 0xF7, 0x3F, 0xF7, 0x3F,
                0x5F, 0x2F, 0x36, 0x36, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00,
                0xFF, 0x0F, 0x00, 0x00, 0xF7, 0x3F, 0x36, 0x00}, // ucWriteExtIO
               {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // ucWriteIOExtShadow
               0x31,                                                         // ucIDRAddress
               0x57,                                                         // ucSPMCRAddress
               0,                                                            // ucRAMPZAddress
               256,                                                          // uiFlashPageSize
               32,                                                           // ucEepromPageSize
               0x10000,                                                      // ulBootAddress
               0x136,                                                        // uiUpperExtIOLoc
               69632,                                                        // ulFlashSize
               {0x00},                                                       // ucEepromInst
               {0x00},                                                       // ucFlashInst
               0x3E,                                                         // ucSPHaddr
               0x3D,                                                         // ucSPLaddr
               69632 / 256,                                                  // uiFlashpages
               0x00,                                                         // ucDWDRAddress
               0x00,                                                         // ucDWBasePC
               0x00, // ucAllowFullPageBitstream
               0x00, // uiStartSmallestBootLoaderSection
               1,    // EnablePageProgramming
               0x02, // ucCacheType
               8192, // uiSramStartAddr
               0,    // ucResetType
               0,    // ucPCMaskExtended
               0,    // ucPCMaskHigh
               0,    // ucEindAddress
               0,    // EECRAddress
           },
           &xmega_device_desc};

} // namespace
