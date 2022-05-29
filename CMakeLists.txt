cmake_minimum_required(VERSION 3.10)

project(avarice LANGUAGES C CXX VERSION 2.15)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# One source file per device in the devices directory
file(GLOB SUPPORTED_DEVICES src/devices/*.cpp)

add_executable(avarice
        src/main.cc
        src/remote.cc
        src/utils.cc
        src/crc16.c
        src/jtag1bp.cc
        src/jtag1io.cc
        src/jtag1misc.cc
        src/jtag1prog.cc
        src/jtag1run.cc
        src/jtag1rw.cc
        src/jtag2bp.cc
        src/jtag2io.cc
        src/jtag2misc.cc
        src/jtag2prog.cc
        src/jtag2run.cc
        src/jtag2rw.cc
        src/jtag2usb.cc
        src/jtag3bp.cc
        src/jtag3io.cc
        src/jtag3misc.cc
        src/jtag3prog.cc
        src/jtag3run.cc
        src/jtag3rw.cc
        src/jtag.cpp
        src/devdescr.cc
        ${SUPPORTED_DEVICES}
        )
target_compile_features(avarice PRIVATE cxx_std_17)
target_compile_options(avarice PRIVATE -Wall -Wextra)
target_compile_definitions(avarice PRIVATE PACKAGE_VERSION=\"${PROJECT_VERSION}\")

find_package(Boost COMPONENTS program_options REQUIRED)
find_package(LibUSB REQUIRED)
find_package(HIDAPI REQUIRED)
find_package(Threads REQUIRED)
target_link_libraries(avarice PRIVATE  ${Boost_LIBRARIES} ${LibUSB_LIBRARIES} HIDAPI::hidapi Threads::Threads)
target_include_directories(avarice PRIVATE ${Boost_INCLUDE_DIRS} ${LibUSB_INCLUDE_DIRS})
