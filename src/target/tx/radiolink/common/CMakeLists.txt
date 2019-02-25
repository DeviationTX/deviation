cmake_minimum_required(VERSION 3.5)

add_definitions(-DSTM32F1)
add_definitions(-DSTM32F10X_HD)

add_definitions(-mcpu=cortex-m3)
add_definitions(-mthumb)
add_definitions(-mfix-cortex-m3-ldrd)

# add drivers
AUX_SOURCE_DIRECTORY(${CMAKE_SOURCE_DIR}/target/drivers/storage radiolink_sources)
AUX_SOURCE_DIRECTORY(${CMAKE_SOURCE_DIR}/target/drivers/mcu/stm32 radiolink_sources)
AUX_SOURCE_DIRECTORY(${CMAKE_SOURCE_DIR}/target/drivers/filesystems radiolink_sources)
AUX_SOURCE_DIRECTORY(${CMAKE_SOURCE_DIR}/target/drivers/filesystems/devofs radiolink_sources)
AUX_SOURCE_DIRECTORY(${CMAKE_SOURCE_DIR}/target/drivers/filesystems/petit_fat radiolink_sources)
AUX_SOURCE_DIRECTORY(${CMAKE_SOURCE_DIR}/target/drivers/serial/uart radiolink_sources)
AUX_SOURCE_DIRECTORY(${CMAKE_SOURCE_DIR}/target/drivers/backlight radiolink_sources)
AUX_SOURCE_DIRECTORY(${CMAKE_SOURCE_DIR}/target/drivers/input/analog radiolink_sources)
AUX_SOURCE_DIRECTORY(${CMAKE_SOURCE_DIR}/target/tx/devo/common/protocol radiolink_sources)

include_directories(${CMAKE_SOURCE_DIR}/target/drivers/filesystems)  


# include common
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/target/tx/radiolink/common/)
AUX_SOURCE_DIRECTORY(${CMAKE_SOURCE_DIR}/target/tx/radiolink/common radiolink_sources)


# build libopencm3
include_directories(${LIBOPENCM3_ROOT}/include)

link_directories(${LIBOPENCM3_ROOT}/lib)
add_custom_target(libopencm3 make WORKING_DIRECTORY ${LIBOPENCM3_ROOT})