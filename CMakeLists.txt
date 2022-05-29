cmake_minimum_required(VERSION 3.10)

project(avarice LANGUAGES C CXX VERSION 2.15)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# One source file per device in the devices directory
file(GLOB SUPPORTED_DEVICES src/devices/*.cpp)

add_executable(avarice
        src/main.cpp
        src/gdb_server.cpp
        src/utils.cpp
        src/crc16.cpp
        src/jtag1bp.cpp
        src/jtag1io.cpp
        src/jtag1misc.cpp
        src/jtag1prog.cpp
        src/jtag1run.cpp
        src/jtag1rw.cpp
        src/jtag2bp.cpp
        src/jtag2io.cpp
        src/jtag2misc.cpp
        src/jtag2prog.cpp
        src/jtag2run.cpp
        src/jtag2rw.cpp
        src/jtag2usb.cpp
        src/jtag3bp.cpp
        src/jtag3io.cpp
        src/jtag3misc.cpp
        src/jtag3prog.cpp
        src/jtag3run.cpp
        src/jtag3rw.cpp
        src/jtag.cpp
        src/devdescr.cpp
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
