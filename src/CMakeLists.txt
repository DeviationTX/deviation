cmake_minimum_required(VERSION 3.7)

set (SDIR "${CMAKE_SOURCE_DIR}")

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

SET(CMAKE_BUILD_TYPE "dev" CACHE STRING
	"Choose the type of build, options are: default, dev, stack, perf")

set(LIBOPENCM3_ROOT ${CMAKE_SOURCE_DIR}/libopencm3)

include(${CMAKE_MODULE_PATH}/utils.cmake)

include_directories(${CMAKE_SOURCE_DIR})

# include all the targets CMakeLists.txt
subdirlist(families ${CMAKE_SOURCE_DIR}/target/tx)
FOREACH(family ${families})
	subdirlist(targets ${family})
	# each family
	FOREACH(target ${targets})
		IF(${target} MATCHES "/common")
			CONTINUE()
		ENDIF()
		IF(EXISTS ${target}/CMakeLists.txt)
			ADD_SUBDIRECTORY(${target})
		ENDIF()
	ENDFOREACH()
ENDFOREACH()

include_directories(${CMAKE_SOURCE_DIR})

AUX_SOURCE_DIRECTORY(${CMAKE_SOURCE_DIR} common_sources)
AUX_SOURCE_DIRECTORY(${CMAKE_SOURCE_DIR}/misc common_sources)
AUX_SOURCE_DIRECTORY(${CMAKE_SOURCE_DIR}/screen common_sources)
AUX_SOURCE_DIRECTORY(${CMAKE_SOURCE_DIR}/gui common_sources)
AUX_SOURCE_DIRECTORY(${CMAKE_SOURCE_DIR}/config common_sources)
AUX_SOURCE_DIRECTORY(${CMAKE_SOURCE_DIR} common_sources)

AUX_SOURCE_DIRECTORY(${CMAKE_SOURCE_DIR}/screen/text/ gui_text_sources)
AUX_SOURCE_DIRECTORY(${CMAKE_SOURCE_DIR}/pages/text/ gui_text_sources)
AUX_SOURCE_DIRECTORY(${CMAKE_SOURCE_DIR}/pages/text/standard gui_text_sources)
AUX_SOURCE_DIRECTORY(${CMAKE_SOURCE_DIR}/pages/text/advanced gui_text_sources)

AUX_SOURCE_DIRECTORY(${CMAKE_SOURCE_DIR}/screen/320x240x16/ gui_color_sources)
AUX_SOURCE_DIRECTORY(${CMAKE_SOURCE_DIR}/pages/320x240x16/ gui_color_sources)
AUX_SOURCE_DIRECTORY(${CMAKE_SOURCE_DIR}/pages/320x240x16/standard gui_color_sources)
AUX_SOURCE_DIRECTORY(${CMAKE_SOURCE_DIR}/pages/320x240x16/advanced gui_color_sources)

AUX_SOURCE_DIRECTORY(${CMAKE_SOURCE_DIR}/screen/128x64x1/ gui_monochrome_sources)
AUX_SOURCE_DIRECTORY(${CMAKE_SOURCE_DIR}/pages/128x64x1/ gui_monochrome_sources)
AUX_SOURCE_DIRECTORY(${CMAKE_SOURCE_DIR}/pages/128x64x1/standard gui_monochrome_sources)
AUX_SOURCE_DIRECTORY(${CMAKE_SOURCE_DIR}/pages/128x64x1/advanced gui_monochrome_sources)
