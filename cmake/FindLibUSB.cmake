# - Try to find LibUSB-1.0
# Once done this will define
#
#  LibUSB_FOUND - System has libusb
#  LibUSB_INCLUDE_DIRS - The libusb include directories
#  LibUSB_LIBRARIES - The libraries needed to use libusb
#  LibUSB_DEFINITIONS - Compiler switches required for using libusb
#  LibUSB_VERSION - the libusb version
#

find_package(PkgConfig)
pkg_check_modules(PC_LibUSB libusb)
set(LibUSB_DEFINITIONS ${PC_LibUSB_CFLAGS_OTHER})

find_path(LibUSB_INCLUDE_DIR NAMES usb.h
          HINTS ${PC_LibUSB_INCLUDE_DIRS}
          #PATH_SUFFIXES libusb-0.1
          PATHS
          /usr/include
          /usr/local/include )

set(libusb0_library_names libusb-0.1)

#libusb-1.0 compatible library on freebsd
if((CMAKE_SYSTEM_NAME STREQUAL "FreeBSD") OR (CMAKE_SYSTEM_NAME STREQUAL "kFreeBSD"))
    list(APPEND libusb0_library_names usb)
endif()

#libusb-1.0 name on Windows (from PothosSDR distribution)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    list(APPEND libusb0_library_names libusb-0.1)
endif()

find_library(LibUSB_LIBRARY
             NAMES ${libusb0_library_names}
             HINTS ${PC_LibUSB_LIBRARY_DIRS}
             PATHS
             /usr/lib
             /usr/local/lib )
set(LibUSB_LIBRARY /usr/lib/libusb.so)

message(${LibUSB_LIBRARY} ${libusb0_library_names})
set(LibUSB_VERSION ${PC_LibUSB_VERSION})

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LibUSB_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(LibUSB
                                  REQUIRED_VARS LibUSB_LIBRARY LibUSB_INCLUDE_DIR
                                  VERSION_VAR LibUSB_VERSION)

mark_as_advanced(LibUSB_LIBRARY LibUSB_INCLUDE_DIR LibUSB_VERSION)

set(LibUSB_LIBRARIES ${LibUSB_LIBRARY} )
set(LibUSB_INCLUDE_DIRS ${LibUSB_INCLUDE_DIR} )
