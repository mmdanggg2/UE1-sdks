cmake_minimum_required(VERSION 3.0.0)
project(AkelEdit)

if("${CMAKE_GENERATOR_PLATFORM}" STREQUAL "")
  set(TARGET_ARCH ${CMAKE_HOST_SYSTEM_PROCESSOR})
else()
  set(TARGET_ARCH ${CMAKE_GENERATOR_PLATFORM})
endif()

if("${TARGET_ARCH}" STREQUAL "x86" OR "${TARGET_ARCH}" STREQUAL "Win32")
  set(BIT "32")
elseif("${TARGET_ARCH}" STREQUAL "x64" OR "${TARGET_ARCH}" STREQUAL "Win64" OR "${TARGET_ARCH}" STREQUAL "AMD64")
  set(BIT "64")
else()
  message(FATAL_ERROR "Unsupported target architecture: ${TARGET_ARCH}")
endif()

add_compile_definitions(RC_AKELEDITBIT=${BIT})

set(SOURCES
  "AkelEdit.c"
  "Resources/AkelEdit.rc"
)

set(HEADERS
  "AkelBuild.h"
  "AkelEdit.h"
  "RegExpFunc.h"
  "StackFunc.h"
  "StrFunc.h"
  "x64Func.h"
)

add_library(AkelEdit
  SHARED
  ${SOURCES}
)

set_target_properties(AkelEdit
  PROPERTIES
  PUBLIC_HEADER "${HEADERS}"
)

target_link_libraries(AkelEdit
  uuid
  imm32
  ole32
  gdi32
)

install(TARGETS AkelEdit
  LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/lib"
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}  
)
