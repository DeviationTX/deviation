cmake_minimum_required(VERSION 3.5)

include(${CMAKE_MODULE_PATH}/arm-gcc-toolchain.cmake)
include(${CMAKE_SOURCE_DIR}/target/tx/radiolink/common/CMakeLists.txt)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(at10 channels.c 
	${radiolink_sources}
	${common_sources}
	${gui_monochrome_sources})

include_directories(${CMAKE_SOURCE_DIR}/gui/320x240x16/)

firmware_size(at10)