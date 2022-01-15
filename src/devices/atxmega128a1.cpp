#include "../jtag.h"

namespace {

constexpr jtag2_device_desc_type jtag2_device_desc{
    CMND_SET_DEVICE_DESCRIPTOR,
    {0xFF, 0xFF, 0xFF, 0xF9, 0xFF, 0x3D, 0xB9, 0xF8}, // ucReadIO
    {0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00}, // ucReadIOShadow
    {0xFF, 0xFF, 0x1F, 0xE0, 0xFF, 0x1D, 0xA9, 0xF8}, // ucWriteIO
    {0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00}, // ucWriteIOShadow
    {0x73, 0xFF, 0x3F, 0xFF, 0xF7, 0x3F, 0xF7, 0x3F, 0xF7, 0x3F, 0x5F, 0x3F, 0x37, 0x37, 0x36,
     0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x0F, 0x00, 0x00, 0xF7, 0x3F, 0x36, 0x00}, // ucReadExtIO
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // ucReadIOExtShadow
    {0x73, 0xFF, 0x3F, 0xF8, 0xF7, 0x3F, 0xF7, 0x3F, 0xF7, 0x3F, 0x5F, 0x2F, 0x36, 0x36, 0x36,
     0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x0F, 0x00, 0x00, 0xF7, 0x3F, 0x36, 0x00}, // ucWriteExtIO
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // ucWriteIOExtShadow
    0x31,                                                         // ucIDRAddress
    0x57,                                                         // ucSPMCRAddress
    0,                                                            // ucRAMPZAddress
    fill_b2(512),                                                 // uiFlashPageSize
    32,                                                           // ucEepromPageSize
    fill_b4(0x20000),                                             // ulBootAddress
    fill_b2(0x136),                                               // uiUpperExtIOLoc
    fill_b4(139264),                                              // ulFlashSize
    {0x00},                                                       // ucEepromInst
    {0x00},                                                       // ucFlashInst
    0x3E,                                                         // ucSPHaddr
    0x3D,                                                         // ucSPLaddr
    fill_b2(139264 / 512),                                        // uiFlashpages
    0x00,                                                         // ucDWDRAddress
    0x00,                                                         // ucDWBasePC
    0x00,                                                         // ucAllowFullPageBitstream
    fill_b2(0x00), // uiStartSmallestBootLoaderSection
    1,             // EnablePageProgramming
    0x02,          // ucCacheType
    fill_b2(8192), // uiSramStartAddr
    0,             // ucResetType
    0,             // ucPCMaskExtended
    0,             // ucPCMaskHigh
    0,             // ucEindAddress
    fill_b2(0),    // EECRAddress
};

constexpr xmega_device_desc_type xmega_device_desc{
    CMND_SET_XMEGA_PARAMS, // cmd
    fill_b2(2),            // whatever
    47,                    // length of following data
    fill_b4(0x800000),     // NVM offset for application flash
    fill_b4(0x820000),     // NVM offset for boot flash
    fill_b4(0x8c0000),     // NVM offset for EEPROM
    fill_b4(0x8f0020),     // NVM offset for fuses
    fill_b4(0x8f0027),     // NVM offset for lock bits
    fill_b4(0x8e0400),     // NVM offset for user signature row
    fill_b4(0x8e0200),     // NVM offset for production sig. row
    fill_b4(0x1000000),    // NVM offset for data memory
    fill_b4(131072),       // size of application flash
    fill_b2(8192),         // size of boot flash
    fill_b2(512),          // flash page size
    fill_b2(2048),         // size of EEPROM
    32,                    // EEPROM page size
    fill_b2(0x1c0),        // IO space base address of NVM controller
    fill_b2(0x90),         // IO space address of MCU control
};

[[maybe_unused]] const jtag_device_def_type device_rev_d{"atxmega128a1revd",
                                                         0x9741,
                                                         512,
                                                         272, // 139264 bytes flash
                                                         32,
                                                         64,      // 2048 bytes EEPROM
                                                         125 * 4, // 125 interrupt vectors
                                                         DEVFL_MKII_ONLY,
                                                         nullptr, // registers not yet defined
                                                         true,
                                                         0x37,
                                                         0x0000, // fuses
                                                         0,      // osccal
                                                         0,      // OCD revision
                                                         {
                                                             0 // no mkI support
                                                         },
                                                         jtag2_device_desc,
                                                         xmega_device_desc};

// DEV_ATXMEGA128A1 revision G (and newer)
[[maybe_unused]] const jtag_device_def_type device_rev_g{"atxmega128a1",
                                                         0x974c,
                                                         512,
                                                         272, // 139264 bytes flash
                                                         32,
                                                         64,      // 2048 bytes EEPROM
                                                         125 * 4, // 125 interrupt vectors
                                                         DEVFL_MKII_ONLY,
                                                         nullptr, // registers not yet defined
                                                         true,
                                                         0x37,
                                                         0x0000, // fuses
                                                         0,      // osccal
                                                         0,      // OCD revision
                                                         {
                                                             0 // no mkI support
                                                         },
                                                         jtag2_device_desc,
                                                         xmega_device_desc};

} // namespace
