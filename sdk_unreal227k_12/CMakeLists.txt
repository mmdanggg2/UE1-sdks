# to fix indentation: cmake-format --dangle-parens --dangle-align=prefix ../CMakeLists.txt > out.txt

################################################################################
# Project definitions
################################################################################

project(Unreal)
set(OLDUNREAL_VERSION "227k")
if(APPLE)
  set(SDLDRV_NAME "SDLDrv")
else()
  set(SDLDRV_NAME "SDL2Drv")
endif()
set(SDLLAUNCH_NAME "SDL2Launch")

################################################################################
# General CMake options
################################################################################

cmake_minimum_required(VERSION 3.13) # we need add_link_options
if(APPLE)
  set(CMAKE_CXX_STANDARD 17) # required by metal
else()
  set(CMAKE_CXX_STANDARD 14)
endif()
set(OLDUNREAL_METAL_VERSION macos-metal2.0)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_PROJECT_VERSION_MAJOR "")
set(CMAKE_PROJECT_VERSION_MINOR "")
set(CMAKE_PROJECT_VERSION_PATCH "")
# uncomment this to see compiler/linker commands
# set(CMAKE_VERBOSE_MAKEFILE ON)

include(CheckCCompilerFlag)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if(COMMAND CMAKE_POLICY)
  if(POLICY CMP0177)
	cmake_policy(SET CMP0177 OLD)
  endif()
endif()

################################################################################
# Figure out build type and target platform
################################################################################

# Sanity checks
if(WIN32 AND NOT WINDOWS)
  set(WINDOWS TRUE)
  set(PLATFORM_PREFIX "Win")
endif()
if(APPLE AND NOT MACOSX)
  set(MACOSX TRUE)
  set(PLATFORM_PREFIX "Mac")
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(LINUX TRUE)
  set(PLATFORM_PREFIX "Linux")
endif()

if(WINDOWS)
  set(LIBRARY_SUFFIX ".dll")
elseif(APPLE)
  set(LIBRARY_SUFFIX ".dylib")
else()
  set(LIBRARY_SUFFIX ".so")
endif()

# See what CPU we appear to be targeting...
macro(check_cpu _CDEFS _NAME _VALUE)
  check_c_source_compiles(
    "
        #if ( ${_CDEFS} )
        int main(int argc, char **argv) { int yup = 1; return 0; }
        #else
        #error Not targeting this CPU architecture.
        #endif
        "
    ${_VALUE}
  )

  if(${_VALUE})
    if(OLDUNREAL_CHOSE_CPU)
      message(STATUS "We appear to see two different CPU architectures!")
      message(STATUS "We saw '${OLDUNREAL_CPU}' and '${_NAME}'.")
      message(FATAL_ERROR "Please fix this before continuing.")
    endif()
    set(OLDUNREAL_CHOSE_CPU TRUE)
    set(OLDUNREAL_CPU ${_NAME})
  endif()
endmacro(check_cpu)

macro(get_library_path _LIBRARY _OUT)
  list(GET ${_LIBRARY} -1 _LIB)
  if(_LIB MATCHES ".*::.*")
    get_target_property(
	  ABSOLUTE_PATH
	  ${_LIB}
      IMPORTED_LOCATION
    )
	if(NOT ABSOLUTE_PATH)
	  get_target_property(
	    ABSOLUTE_PATH
		${_LIB}
		IMPORTED_LOCATION_RELEASE
	  )
    endif()	
  elseif(NOT _LIB STREQUAL "")
    get_filename_component(
      ABSOLUTE_PATH
  	  "${_LIB}"
	  REALPATH
	)
  endif()
  set(${_OUT} ${ABSOLUTE_PATH})
endmacro(get_library_path)

check_cpu(
  "defined(__i386__) || defined(__i686__) || defined(_M_IX86) || defined(i386)"
  "x86" OLDUNREAL_X86
)

check_cpu("defined(__x86_64__) || defined(_M_X64)" "amd64" OLDUNREAL_AMD64)

check_cpu("defined(__EMSCRIPTEN__)" "emscripten" OLDUNREAL_EMSCRIPTEN)

check_cpu("defined(__arm__)" "arm" OLDUNREAL_ARM)

check_cpu("defined(__aarch64__) || defined(__arm64__)" "arm64" OLDUNREAL_ARM64)

if (NOT OLDUNREAL_CHOSE_CPU)
  message(FATAL_ERROR "We don't support this architecture yet")
endif()

################################################################################
# Build options
################################################################################

if (WINDOWS)
  option(OLDUNREAL_BUILD_ICBINDX11DRV
    "Set to true to build Metallicafan212's ICBINDx11Drv." TRUE
  )
endif()

option(OLDUNREAL_BUILD_OPENGLDRV
  "Set to true to build UTGLR OpenGLDrv." TRUE
)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/UnObj.cpp)
  option(OLDUNREAL_BUILD_SWFMOD
    "Set to true to build SwFMOD." FALSE
  )
  option(OLDUNREAL_BUILD_GLIDEDRV
    "Set to true to build GlideDrv." FALSE
  )
  option(OLDUNREAL_BUILD_PHYSX
    "Set to true to build PhysX." TRUE
  )
  #Set to FALSE for building on RasPi

  option(OLDUNREAL_STATIC_LINK
    "Set to true to statically link everything, False to build DLLs." FALSE
  )
  
  if(APPLE)
    option(
      OLDUNREAL_MAKE_BUNDLE
      "Set to true to create an application bundle instead of standalone binaries (implies OLDUNREAL_STATIC_LINK)"
      FALSE
    )
    option(
      OLDUNREAL_BUILD_FRUCORE
      "Set to true to build the Apple Metal Renderer."
      TRUE
    )  
  endif()
else()
  set(OLDUNREAL_BUILD_SWFMOD FALSE)
  set(OLDUNREAL_BUILD_GLIDEDRV FALSE)
  set(OLDUNREAL_BUILD_PHYSX FALSE)
  set(OLDUNREAL_STATIC_LINK FALSE)
  set(OLDUNREAL_MAKE_BUNDLE FALSE)
  set(OLDUNREAL_BUILD_FRUCORE FALSE)

  if(APPLE)
	message(FATAL_ERROR "The OldUnreal SDK is not available on macOS. Sorry!")
  endif()
endif()

option(OLDUNREAL_USE_ASAN "True to enable address sanitizer" FALSE)

if(NOT WINDOWS)
  option(OLDUNREAL_USE_TSAN "True to enable thread sanitizer" FALSE)
endif()

################################################################################
# Figure out which executables to build + their names
################################################################################
if(WINDOWS)
  set(OLDUNREAL_EXE ${CMAKE_PROJECT_NAME})
  set(OLDUNREAL_BUILD_SERVER TRUE)
  set(OLDUNREAL_SERVER_EXE "UCC")
  set(OLDUNREAL_BUILD_EDITOR TRUE)
  set(OLDUNREAL_EDITOR_EXE "UnrealEd")
  set(OLDUNREAL_BUILD_SETUP TRUE)
  set(OLDUNREAL_SETUP_EXE "Setup")
  
  if (OLDUNREAL_BUILD_ICBINDX11DRV)
	set(ICBINDX11DRV_NAME "ICBINDx11Drv")
	set(ICBINDX11DRV_DIR "ICBINDx11Drv")
  endif()

elseif(LINUX)
  set(OLDUNREAL_EXE "UnrealLinux.bin")
  set(OLDUNREAL_BUILD_SERVER TRUE)
  set(OLDUNREAL_SERVER_EXE "UCCLinux.bin")
  set(OLDUNREAL_BUILD_EDITOR FALSE)
  set(OLDUNREAL_BUILD_SETUP FALSE)  

elseif(APPLE)
  if(OLDUNREAL_MAKE_BUNDLE)
    set(OLDUNREAL_EXE ${CMAKE_PROJECT_NAME})
    set(OLDUNREAL_BUILD_SERVER TRUE)
	set(OLDUNREAL_SERVER_EXE "UCC")
    set(OLDUNREAL_BUILD_EDITOR FALSE)
    set(OLDUNREAL_BUILD_SETUP FALSE)

  else()
    string(TOLOWER "macos-${CMAKE_PROJECT_NAME}-bin-${OLDUNREAL_CPU}" OLDUNREAL_EXE)
    set(OLDUNREAL_BUILD_SERVER TRUE)
    string(TOLOWER "macos-ucc-bin-${OLDUNREAL_CPU}" OLDUNREAL_SERVER_EXE)
    set(OLDUNREAL_BUILD_EDITOR FALSE)
    set(OLDUNREAL_BUILD_SETUP FALSE)

  endif()

endif()

################################################################################
# Figure out prefix/module path
################################################################################

if(NOT CMAKE_PREFIX_PATH)
  if(OLDUNREAL_DEPENDENCIES_PATH)
    set(CMAKE_PREFIX_PATH "${OLDUNREAL_DEPENDENCIES_PATH}")
  else()
    if(MSVC)
      set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build/${PLATFORM_PREFIX}/${OLDUNREAL_CPU}")
    else()
      set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build/${PLATFORM_PREFIX}/${OLDUNREAL_CPU}/${CMAKE_BUILD_TYPE}")
    endif()
  endif()
endif()

message(STATUS "Using dependencies path: ${CMAKE_PREFIX_PATH}")

# if(NOT EXISTS "${CMAKE_PREFIX_PATH}")
#   message(FATAL_ERROR "Invalid dependencies path: ${CMAKE_PREFIX_PATH} (does this folder exist?)")
# endif()

################################################################################
# Locate all 3rd party headers and libs
################################################################################

if(OLDUNREAL_BUILD_PHYSX)
  set(WANT_PHYSX ON)
endif()
if(OLDUNREAL_BUILD_GLIDEDRV)
  set(WANT_OPENGLIDE ON)
endif()
if(WINDOWS AND NOT OLDUNREAL_AMD64 AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Galaxy/src/UnGalaxy.cpp)
  set(WANT_GALAXY ON)
endif()
include(${CMAKE_CURRENT_SOURCE_DIR}/External/cmake/FindUE1Dependencies.cmake)

################################################################################
# Set compiler and linker flags
################################################################################

if(NOT WINDOWS AND NOT CMAKE_BUILD_TYPE)
  message(WARNING "CMAKE_BUILD_TYPE not set!")
  add_compile_definitions(_RELEASE)
  add_compile_definitions(NDEBUG)
  add_compile_definitions(RELEASE)
endif()

add_compile_definitions("$<$<CONFIG:Debug>:_DEBUG>")
add_compile_definitions("$<$<CONFIG:Release>:_RELEASE>")
add_compile_definitions("$<$<CONFIG:Release>:NDEBUG>")
add_compile_definitions("$<$<CONFIG:Release>:RELEASE>")

add_compile_definitions(UTGLR_UNREAL_227_BUILD)
add_compile_definitions(UTGLR_USES_ALPHABLEND=1)
add_compile_definitions(ENGINE_ARCHITECTURE="${OLDUNREAL_CPU}")

if(OLDUNREAL_ARM OR OLDUNREAL_ARM64)
  add_compile_definitions(USES_SSE_INTRINSICS=0)
else()
  add_compile_definitions(USES_SSE_INTRINSICS=1)
endif()

if(OLDUNREAL_BUILD_PHYSX)
  add_compile_definitions(PX_PHYSX_STATIC_LIB)
endif()

if(APPLE AND OLDUNREAL_MAKE_BUNDLE)
  set(OLDUNREAL_STATIC_LINK TRUE)
endif()

if(NOT KTEXCOMP_LIBRARY)
  message(STATUS "KTexComp not found. Disabling texture compression support")
  add_compile_definitions(OLDUNREAL_TEXTURE_COMPRESSION=0)
else()
  add_compile_definitions(OLDUNREAL_TEXTURE_COMPRESSION=1)
endif()

if(LINUX)
  # Don't use -rpath (at least, not the silly one CMake forces).
  set(CMAKE_SKIP_RPATH
    ON
      CACHE BOOL "Skip RPATH" FORCE
  )
  add_compile_definitions(UNICODE=1)
  add_compile_definitions(_UNICODE=1)
  add_compile_definitions(_REENTRANT=1)
  add_compile_definitions(HAVE_PRAGMA_PACK=1)
  add_compile_definitions(SDL2BUILD)
  add_definitions(-fPIC)

  if(OLDUNREAL_ARM)
    add_compile_definitions(__LINUX_ARM__=1)
    add_compile_definitions(SDL_DISABLE_MM3DNOW_H=1)
    add_compile_definitions(SDL_DISABLE_IMMINTRIN_H=1)
    add_compile_definitions(SDL_DISABLE_MMINTRIN_H=1)
    add_compile_definitions(SDL_DISABLE_XMMINTRIN_H=1)
    add_compile_definitions(SDL_DISABLE_EMMINTRIN_H=1)
    add_compile_definitions(SDL_DISABLE_PMMINTRIN_H=1)
    add_definitions(-mfpu=neon-vfpv4)
    add_definitions(-mfloat-abi=hard)
  elseif (OLDUNREAL_ARM64)
    add_compile_definitions(__LINUX_ARM__=1)
    add_compile_definitions(SDL_DISABLE_MM3DNOW_H=1)
    add_compile_definitions(SDL_DISABLE_IMMINTRIN_H=1)
    add_compile_definitions(SDL_DISABLE_MMINTRIN_H=1)
    add_compile_definitions(SDL_DISABLE_XMMINTRIN_H=1)
    add_compile_definitions(SDL_DISABLE_EMMINTRIN_H=1)
    add_compile_definitions(SDL_DISABLE_PMMINTRIN_H=1)
    add_definitions(-march=armv8-a)
    add_definitions(-mtune=cortex-a72)
  else()
    add_compile_definitions(__LINUX_X86__=1)

	# stijn: statically link libstdc++ and libgcc as permitted by the GPL
	# runtime exception

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse -msse2")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse -msse2")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}  -static-libgcc -static-libstdc++ -fuse-ld=gold -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/Version.script")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}  -static-libgcc -static-libstdc++ -fuse-ld=gold")

	# strip symbols in release builds
	add_link_options("$<$<CONFIG:Release>:-Wl,--strip-all>")
  endif()

elseif(APPLE)
  add_compile_definitions(MACOSX=1)
  add_compile_definitions(UNICODE=1)
  add_compile_definitions(_UNICODE=1)
  add_compile_definitions(_REENTRANT=1)
  add_compile_definitions(HAVE_PRAGMA_PACK=1)
  add_compile_definitions(SDL2BUILD)

  if (OLDUNREAL_ARM64)
    add_compile_definitions(__APPLE_ARM64__=1)
  else()
    add_definitions(-mmacosx-version-min=10.9)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -mmacosx-version-min=10.9")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mmacosx-version-min=10.9")
  endif()

  set(CMAKE_INSTALL_RPATH "@executable_path")

else()
  # use multi-threaded version of C runtime lib and link it statically
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS=1)
  add_compile_definitions(_CRT_NO_VA_START_VALIDATION)
  add_compile_definitions(UNICODE=1)
  add_compile_definitions(_UNICODE=1)
  add_compile_definitions(WIN32=1)
  if(OLDUNREAL_AMD64)
    add_compile_definitions(WIN64=1)
  else()
	#
	# /arch:SSE2
    # assume we have at least a Pentium 4
	#
	# /LARGEADDRESSAWARE
    # we can handle more than 2Gb of RAM even in 32bit builds
	#
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE2")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /arch:SSE2")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LARGEADDRESSAWARE")
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /LARGEADDRESSAWARE")
	set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /LARGEADDRESSAWARE")
	set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} /LARGEADDRESSAWARE")
  endif()
  #
  # /MP
  # Parallel builds
  #
  string(APPEND CMAKE_C_FLAGS " /MP")
  string(APPEND CMAKE_CXX_FLAGS " /MP")

  # the recursive garbage collector needs a lot of stack space
  set(CMAKE_EXE_LINKER_FLAGS
      "${CMAKE_EXE_LINKER_FLAGS} /STACK:16000000,1000000 /LTCG"
  )

endif()

if(CMAKE_COMPILER_IS_GNUCC OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pipe -fsigned-char -Wno-inline-new-delete")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pipe -fsigned-char")
endif()

if(OLDUNREAL_STATIC_LINK)
  add_compile_definitions(__STATIC_LINK=1)
  set(OLDUNREAL_PACKAGE_LINKAGE STATIC)
else()
  set(OLDUNREAL_PACKAGE_LINKAGE SHARED)
endif()

if(OLDUNREAL_USE_ASAN)
  add_compile_definitions(OLDUNREAL_ASAN=1)
  add_compile_options("-fsanitize=address")
  link_libraries("-fsanitize=address")
elseif(OLDUNREAL_USE_TSAN)
  add_compile_options("-fsanitize=thread")
  link_libraries("-fsanitize=thread")
endif()

################################################################################
# Linker macros
################################################################################

macro(oldunreal_set_package_properties _PACKAGE)
  string(TOUPPER "${_PACKAGE}" _UPACKAGE)
  if(WINDOWS AND NOT OLDUNREAL_STATIC_LINK)
    # stijn: the GPackage symbol has local (intra-module) visibility on Windows
    # platforms so there's no need to give it a unique name here
    set_target_properties(
      ${_PACKAGE} PROPERTIES COMPILE_DEFINITIONS
                             "${_UPACKAGE}_API=__declspec(dllexport)" PREFIX ""
    )
    set_target_properties(${_PACKAGE} PROPERTIES DEBUG_POSTFIX "")
  elseif(WINDOWS AND OLDUNREAL_STATIC_LINK)
    set_target_properties(
      ${_PACKAGE} PROPERTIES COMPILE_DEFINITIONS
                             "GPackage=LocalPackageName${_PACKAGE}" PREFIX ""
    )
  else()
    # stijn: for WIN32 static and for NIX, we need to make sure that the various
    # GPackage symbols don't clash so we rename them using a preprocessor
    # definition
    set_target_properties(
      ${_PACKAGE}
      PROPERTIES COMPILE_DEFINITIONS "GPackage=LocalPackageName${_PACKAGE}"
                 PREFIX "" LINK_FLAGS "-Wl,-rpath,$ORIGIN"
    )
  endif()

  # No need to depend on other packages when static linking, so we can build all
  # in parallel.
  if(NOT OLDUNREAL_STATIC_LINK)
    if(NOT ${_PACKAGE} STREQUAL "Core")
      target_link_libraries(${_PACKAGE} Core)
      if(NOT ${_PACKAGE} STREQUAL "Engine")
        target_link_libraries(${_PACKAGE} Engine)
      endif()
    endif()
  endif()
endmacro()

macro(oldunreal_import_core_library _LIBRARY)
	add_library(${_LIBRARY} SHARED IMPORTED)
	set_target_properties(${_LIBRARY} PROPERTIES
		IMPORTED_LOCATION_DEBUG           ${CMAKE_INSTALL_PREFIX}/${_LIBRARY}${LIBRARY_SUFFIX}
		IMPORTED_LOCATION_RELWITHDEBINFO  ${CMAKE_INSTALL_PREFIX}/${_LIBRARY}${LIBRARY_SUFFIX}
		IMPORTED_LOCATION_RELEASE         ${CMAKE_INSTALL_PREFIX}/${_LIBRARY}${LIBRARY_SUFFIX}
		IMPORTED_LOCATION_MINSIZEREL      ${CMAKE_INSTALL_PREFIX}/${_LIBRARY}${LIBRARY_SUFFIX}
		IMPORTED_IMPLIB_DEBUG             ${CMAKE_CURRENT_SOURCE_DIR}/${_LIBRARY}/Lib/${_LIBRARY}.lib
		IMPORTED_IMPLIB_RELEASE           ${CMAKE_CURRENT_SOURCE_DIR}/${_LIBRARY}/Lib/${_LIBRARY}.lib
		IMPORTED_IMPLIB_RELWITHDEBINFO    ${CMAKE_CURRENT_SOURCE_DIR}/${_LIBRARY}/Lib/${_LIBRARY}.lib
		IMPORTED_IMPLIB_MINSIZEREL        ${CMAKE_CURRENT_SOURCE_DIR}/${_LIBRARY}/Lib/${_LIBRARY}.lib		
	)
endmacro()

################################################################################
# Include Directories
################################################################################

include_directories(Core/Inc)
include_directories(Engine/Inc)
include_directories(ALAudio/Inc)
include_directories(Audio/Inc)
include_directories(Editor/Inc)
include_directories(Editor/Src)
include_directories(Emitter/Inc)
include_directories(Emitter/Src)
include_directories(Engine/Src)
include_directories(Fire/Inc)
if(OLDUNREAL_BUILD_GLIDEDRV)
  include_directories(GlideDrv/Inc)
endif()
if (OLDUNREAL_BUILD_ICBINDX11DRV)
  include_directories(${ICBINDX11DRV_DIR}/Inc)
endif()
include_directories(IpDrv/Inc)
if(OLDUNREAL_BUILD_OPENGLDRV)
  include_directories(OpenGLDrv/Inc)
  include_directories(OpenGLDrv/Src)
endif()
include_directories(PathLogic/Inc)
include_directories(PathLogic/Src)
include_directories(USQLite/Inc)
include_directories(USQLite/Src)
include_directories(Render/Inc)
include_directories(UPak/Inc)
include_directories(UWebAdmin/Inc)
include_directories(UWebAdmin/Src)
include_directories(XOpenGLDrv/Src)
include_directories(XOpenGLDrv/Inc)

if(OLDUNREAL_BUILD_FRUCORE)
  include_directories(FruitCompanyRenderer/Inc)
endif()

if(OLDUNREAL_BUILD_PHYSX)
  include_directories(PhysXPhysics/Inc)
endif()

if(WINDOWS)
  include_directories(Window/Inc)
  include_directories(WinDrv/Inc)
  include_directories(WinDrv/Src)
  include_directories(WinDrv/Src/Res)
  include_directories(D3DDrv/Src)

  include_directories(D3D9Drv/Inc)
  include_directories(Launch/Inc)
  include_directories(Launch/Src/Res)
  include_directories(Setup/Inc)
  include_directories(Setup/Src/Res)
  include_directories(UnrealEd/Inc)
  include_directories(UnrealEd/Src/Res)

  if(NOT OLDUNREAL_AMD64)
    include_directories(Galaxy/Inc)
    include_directories(SoftDrv/Src)
	include_directories(D3D8drv/Inc)
  else()
    include_directories(SoftDrv64/Src)
  endif()
else()
  include_directories(${SDLLAUNCH_NAME}/Inc)
  include_directories(Window/Src/Res)
  include_directories(${SDLDRV_NAME}/Inc)
  include_directories(SDLSoftDrv/Src)
  include_directories(ALAudio/Inc)
endif()

################################################################################
# Target Definitions
################################################################################

file(GLOB CORE_HEADERS Core/Inc/*.h)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/UnObj.cpp)
  add_library(
    Core
    ${OLDUNREAL_PACKAGE_LINKAGE}
    # stijn: all of the important global objects live in UnObj.cpp now
    # these globals need to be constructed first and destructed last
    Core/Src/UnObj.cpp
    Core/Src/Core.cpp
    Core/Src/UExporter.cpp
    Core/Src/UFactory.cpp
    Core/Src/UnAnsi.cpp
    Core/Src/UnBits.cpp
    Core/Src/UnCache.cpp
    Core/Src/UnClass.cpp
    Core/Src/UnCorSc.cpp
    Core/Src/UnCoreNative.cpp
    Core/Src/UnCoreNet.cpp
    Core/Src/UnDebugCPUClock.cpp
    Core/Src/UnDebugLog.cpp
    Core/Src/UnGarbage.cpp
    Core/Src/UnGUID.cpp
    Core/Src/UnLinker.cpp
    Core/Src/UnLocale.cpp
    Core/Src/UnMath.cpp
    Core/Src/UnMem.cpp
    Core/Src/UnMD5.cpp
    Core/Src/UnMisc.cpp
    Core/Src/UnName.cpp
    Core/Src/UnProp.cpp
    Core/Src/UnStats.cpp
    Core/Src/UnThread.cpp
    Core/Src/UnUnix.cpp
    Core/Src/UnVcWin32.cpp
    Core/Src/UnixWPrintf.c
    Core/Src/ZLibCompress.cpp
    Core/Src/fcaseopen.c
    ${CORE_HEADERS}
  )
  oldunreal_set_package_properties(Core)
else()
  oldunreal_import_core_library(Core)
endif()

file(GLOB ENGINE_HEADERS Engine/Inc/*.h)
file(GLOB ENGINE_SOURCES Engine/Src/*.cpp)
if(ENGINE_SOURCES)
  add_library(
    Engine
    ${OLDUNREAL_PACKAGE_LINKAGE}
    ${ENGINE_SOURCES}
    ${ENGINE_HEADERS}
  )
  # Our DOREP macros get a massive perf boost if we disable thread-safe statics
  if(WINDOWS)
    set_source_files_properties(Engine/Src/UnActor.cpp PROPERTIES COMPILE_FLAGS /Zc:threadSafeInit-)
  else()
    set_source_files_properties(Engine/Src/UnActor.cpp PROPERTIES COMPILE_FLAGS -fno-threadsafe-statics)
  endif()
  oldunreal_set_package_properties(Engine)
else()
  oldunreal_import_core_library(Engine)
endif()

file(GLOB EDITOR_HEADERS Editor/Inc/*.h)
file(GLOB EDITOR_SOURCES Editor/Src/*.cpp)
if(EDITOR_SOURCES)
  add_library(
    Editor
    ${OLDUNREAL_PACKAGE_LINKAGE}
    ${EDITOR_SOURCES}
    ${EDITOR_HEADERS}
  )
  oldunreal_set_package_properties(Editor)
else()
  oldunreal_import_core_library(Editor)
endif()

file(GLOB EMITTER_HEADERS Emitter/Inc/*.h)
file(GLOB EMITTER_SOURCES Emitter/Src/*.cpp)
if(EMITTER_SOURCES)
  add_library(Emitter
    ${OLDUNREAL_PACKAGE_LINKAGE}
    ${EMITTER_SOURCES}
    ${EMITTER_HEADERS}
  )
  oldunreal_set_package_properties(Emitter)
else()
  oldunreal_import_core_library(Emitter)
endif()

file(GLOB FIRE_HEADERS Fire/Inc/*.h)
file(GLOB FIRE_SOURCES Fire/Src/*.cpp)
if(FIRE_SOURCES)
  add_library(
    Fire
    ${OLDUNREAL_PACKAGE_LINKAGE}
    ${FIRE_SOURCES}
    ${FIRE_HEADERS}
  )
  oldunreal_set_package_properties(Fire)
else()
  oldunreal_import_core_library(Fire)
endif()

if(OLDUNREAL_BUILD_GLIDEDRV)
  file(GLOB GLIDEDRV_HEADERS GlideDrv/Inc/*.h)
  file(GLOB GLIDEDRV_SOURCES GlideDrv/Src/*.cpp)
  if(GLIDEDRV_SOURCES)
    add_library(GlideDrv
      ${OLDUNREAL_PACKAGE_LINKAGE}
      ${GLIDEDRV_SOURCES}
      ${GLIDEDRV_HEADERS}
    )
    oldunreal_set_package_properties(GlideDrv)
  endif()
endif()

file(GLOB IPDRV_HEADERS IpDrv/Inc/*.h)
file(GLOB IPDRV_SOURCES IpDrv/Src/*.cpp)
if(FIRE_SOURCES)
  add_library(
    IpDrv
    ${OLDUNREAL_PACKAGE_LINKAGE}
    ${IPDRV_SOURCES}
    ${IPDRV_HEADERS}
  )
  oldunreal_set_package_properties(IpDrv)
else()
  oldunreal_import_core_library(IpDrv)
endif()

file(GLOB RENDER_HEADERS Render/Inc/*.h)
file(GLOB RENDER_SOURCES Render/Src/*.cpp)
if(RENDER_SOURCES)
  add_library(
    Render
    ${OLDUNREAL_PACKAGE_LINKAGE}
    ${RENDER_SOURCES}
    ${RENDER_HEADERS}
  )
  oldunreal_set_package_properties(Render)
else()
  oldunreal_import_core_library(Render)
endif()

if(OLDUNREAL_BUILD_SWFMOD)
  file(GLOB SWFMOD_HEADERS SwFMOD/Inc/*.h)
  file(GLOB SWFMOD_SOURCES SwFMOD/Src/*.cpp)
  if(SWFMOD_SOURCES)
    add_library(SwFMOD
      ${OLDUNREAL_PACKAGE_LINKAGE}
      ${SWFMOD_SOURCES}
      ${SWFMOD_HEADERS}
    )
    oldunreal_set_package_properties(SwFMOD)
    target_include_directories(SwFMOD PRIVATE SwFMOD/inc External/fmodex/inc)
  endif()
endif()

file(GLOB UPAK_HEADERS UPak/Inc/*.h)
file(GLOB UPAK_SOURCES UPak/Src/*.cpp)
if(UPAK_SOURCES)
  add_library(UPak
    ${OLDUNREAL_PACKAGE_LINKAGE}
    ${UPAK_SOURCES}
    ${UPAK_HEADERS}
  )
  oldunreal_set_package_properties(UPak)
else()
  oldunreal_import_core_library(UPak)
endif()

file(GLOB PATHLOGIC_HEADERS PathLogic/Inc/*.h)
file(GLOB PATHLOGIC_SOURCES PathLogic/Src/*.cpp)
if(PATHLOGIC_SOURCES)
  add_library(PathLogic
    ${OLDUNREAL_PACKAGE_LINKAGE}
    ${PATHLOGIC_SOURCES}
    ${PATHLOGIC_HEADERS}
  )
  oldunreal_set_package_properties(PathLogic)
else()
  oldunreal_import_core_library(PathLogic)
endif()

file(GLOB UENGINEPPC_HEADERS UEnginePPC/Inc/*.h)
file(GLOB UENGINEPPC_SOURCES UEnginePPC/Src/*.cpp UEnginePPC/Src/*.c)
if(UENGINEPPC_SOURCES)
  add_library(UEnginePPC
    ${OLDUNREAL_PACKAGE_LINKAGE}
    ${UENGINEPPC_SOURCES}
    ${UENGINEPPC_HEADERS}
  )
  oldunreal_set_package_properties(UEnginePPC)
  target_include_directories(UEnginePPC PRIVATE UEnginePPC/Inc)
else()
  oldunreal_import_core_library(UEnginePPC)
endif()

file(GLOB USQLITE_HEADERS USQLite/Inc/*.h)
file(GLOB USQLITE_SOURCES USQLite/Src/*.cpp USQLite/Src/*.c)
if(USQLITE_SOURCES)
  add_library(USQLite
    ${OLDUNREAL_PACKAGE_LINKAGE}
    ${USQLITE_SOURCES}
    ${USQLITE_HEADERS}
  )
  oldunreal_set_package_properties(USQLite)
else()
  oldunreal_import_core_library(USQLite)
endif()

file(GLOB UWEBADMIN_HEADERS UWebAdmin/Inc/*.h)
file(GLOB UWEBADMIN_SOURCES UWebAdmin/Src/*.cpp)
if(UWEBADMIN_SOURCES)
  add_library(UWebAdmin
    ${OLDUNREAL_PACKAGE_LINKAGE}
    ${UWEBADMIN_SOURCES}
    ${UWEBADMIN_HEADERS}
  )
  oldunreal_set_package_properties(UWebAdmin)
else()
  oldunreal_import_core_library(UWebAdmin)
endif()

if (OLDUNREAL_BUILD_OPENGLDRV)
  file(GLOB OPENGLDRV_HEADERS OpenGLDrv/Inc/*.h)
  file(GLOB OPENGLDRV_SOURCES OpenGLDrv/Src/*.cpp)
  add_library(
    OpenGLDrv
    ${OLDUNREAL_PACKAGE_LINKAGE}
	${OPENGLDRV_SOURCES}
    ${OPENGLDRV_HEADERS}
  )
  oldunreal_set_package_properties(OpenGLDrv)
endif()

file(GLOB XOPENGLDRV_HEADERS XOpenGLDrv/Inc/*.h)
file(GLOB XOPENGLDRV_SOURCES XOpenGLDrv/Src/*.cpp)
list(FILTER XOPENGLDRV_SOURCES EXCLUDE REGEX ".*glad_es.cpp" )
add_library(
  XOpenGLDrv
  ${OLDUNREAL_PACKAGE_LINKAGE}
  ${XOPENGLDRV_SOURCES}
  ${XOPENGLDRV_HEADERS}
)
oldunreal_set_package_properties(XOpenGLDrv)

#if (NOT OLDUNREAL_USE_ASAN)
  file(GLOB ALAUDIO_HEADERS ALAudio/Inc/*.h)
  file(GLOB_RECURSE ALAUDIO_SOURCES ALAudio/Src/*.cpp)
  add_library(
    ALAudio
    ${OLDUNREAL_PACKAGE_LINKAGE}
    ${ALAUDIO_SOURCES}
    ${ALAUDIO_HEADERS}
  )
  oldunreal_set_package_properties(ALAudio)
#endif()

if(OLDUNREAL_BUILD_PHYSX)
  file(GLOB PHYSXPHYSICS_HEADERS PhysXPhysics/Inc/*.h)
  file(GLOB_RECURSE PHYSXPHYSICS_SOURCES PhysXPhysics/Src/*.cpp)
  if(PHYSXPHYSICS_SOURCES)
    add_library(
      PhysXPhysics
      ${OLDUNREAL_PACKAGE_LINKAGE}
      ${PHYSXPHYSICS_SOURCES}
      ${PHYSXPHYSICS_HEADERS}
    )
    oldunreal_set_package_properties(PhysXPhysics)
  endif()
endif()

if(OLDUNREAL_BUILD_FRUCORE)
  file(GLOB FRUCORE_HEADERS FruitCompanyRenderer/Inc/*.h)
  file(GLOB FRUCORE_SOURCES FruitCompanyRenderer/Src/*.cpp)
  file(GLOB FRUCORE_SHADERS FruitCompanyRenderer/Shaders/*.metal)

  if(CMAKE_GENERATOR STREQUAL Xcode)
    set_source_files_properties(${FRUCORE_SHADERS} PROPERTIES LANGUAGE METAL)
    add_library(
      Frucore 
      ${OLDUNREAL_PACKAGE_LINKAGE}
	  ${FRUCORE_SOURCES}
	  ${FRUCORE_HEADERS}
	  ${FRUCORE_SHADERS}
    )
    oldunreal_set_package_properties(Frucore)
    
  else()
    foreach(FRUCORE_SHADER ${FRUCORE_SHADERS})
      get_filename_component(TMP ${FRUCORE_SHADER} NAME)
      string(REPLACE ".metal" ".air" FRUCORE_SHADER_AIR ${TMP})
      list(APPEND FRUCORE_SHADER_LIB_OBJECTS ${FRUCORE_SHADER_AIR})
      add_custom_command(
	    DEPENDS ${FRUCORE_SHADER} ${FRUCORE_HEADERS}
        OUTPUT ${FRUCORE_SHADER_AIR}
        COMMAND xcrun -sdk macosx metal  -std=${OLDUNREAL_METAL_VERSION} -mmacos-version-min=10.9 -I ${CMAKE_CURRENT_SOURCE_DIR}/FruitCompanyRenderDrv/Inc/ -frecord-sources -c ${FRUCORE_SHADER} -o ${FRUCORE_SHADER_AIR}
	    COMMENT "Creating ${FRUCORE_SHADER_AIR}"
	    VERBATIM
      )
    endforeach()

    add_custom_command(
      OUTPUT default.metallib
	  DEPENDS ${FRUCORE_SHADER_LIB_OBJECTS} ${FRUCORE_HEADERS}
	  COMMAND xcrun -sdk macosx metallib ${FRUCORE_SHADER_LIB_OBJECTS} -o default.metallib
    )
    add_custom_target(FrucoreShaders DEPENDS default.metallib)

    add_library(
      Frucore
      ${OLDUNREAL_PACKAGE_LINKAGE}
	  ${FRUCORE_SOURCES}
	  ${FRUCORE_HEADERS}
    )
    oldunreal_set_package_properties(Frucore)

    add_dependencies(Frucore FrucoreShaders)
  endif()

endif()

if(WINDOWS)
  if(NOT OLDUNREAL_AMD64)
    file(GLOB GALAXY_HEADERS Galaxy/Inc/*.h)
	file(GLOB GALAXY_SOURCES Galaxy/Src/*.cpp)
	if(GALAXY_SOURCES)
      add_library(
        Galaxy
        ${OLDUNREAL_PACKAGE_LINKAGE}
  	    ${GALAXY_SOURCES}
        ${GALAXY_HEADERS}
      )
      oldunreal_set_package_properties(Galaxy)
	endif()
  endif()

  file(GLOB D3DDRV_HEADERS D3DDrv/Inc/*.h)
  file(GLOB D3DDRV_SOURCES D3DDrv/Src/*.cpp)
  add_library(
    D3DDrv
    ${OLDUNREAL_PACKAGE_LINKAGE}
	${D3DDRV_SOURCES}
    ${D3DDRV_HEADERS}
  )
  oldunreal_set_package_properties(D3DDrv)

  if(NOT OLDUNREAL_AMD64)
    file(GLOB D3D8DRV_HEADERS D3D8Drv/Inc/*.h)
    file(GLOB D3D8DRV_SOURCES D3D8Drv/Src/*.cpp)
    add_library(
      D3D8Drv
      ${OLDUNREAL_PACKAGE_LINKAGE}
      ${D3D8DRV_SOURCES}
      ${D3D8DRV_HEADERS}
      )
    target_include_directories(D3D8Drv PRIVATE External/dxsdk/Include)
    oldunreal_set_package_properties(D3D8Drv)
  endif()

  file(GLOB D3D9DRV_HEADERS D3D9Drv/Inc/*.h)
  file(GLOB D3D9DRV_SOURCES D3D9Drv/Src/*.cpp)
  add_library(
    D3D9Drv
    ${OLDUNREAL_PACKAGE_LINKAGE}
    ${D3D9DRV_SOURCES}
    ${D3D9DRV_HEADERS}
  )
  oldunreal_set_package_properties(D3D9Drv)
  
  if(OLDUNREAL_BUILD_ICBINDX11DRV)
    file(GLOB ICBINDX11DRV_HEADERS ${ICBINDX11DRV_DIR}/Inc/*.h)
    file(GLOB ICBINDX11DRV_SOURCES ${ICBINDX11DRV_DIR}/Src/*.cpp)
	list(FILTER ICBINDX11DRV_SOURCES EXCLUDE REGEX ".+UnHardcodedShaders.cpp")
	file(GLOB ICBINDX11DRV_SHADERS ${ICBINDX11DRV_DIR}/Shaders/*.hlsl ${ICBINDX11DRV_DIR}/Shaders/*.h)

    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${ICBINDX11DRV_DIR}/Src/UnHardcodedShaders.cpp
	  DEPENDS ${ICBINDX11DRV_SHADERS}
	  COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/${ICBINDX11DRV_DIR}/PrecompileShader.py
	  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${ICBINDX11DRV_DIR}
    )
	
    add_library(
      ${ICBINDX11DRV_NAME}
      ${OLDUNREAL_PACKAGE_LINKAGE}
      ${ICBINDX11DRV_SOURCES}
	  ${CMAKE_CURRENT_SOURCE_DIR}/${ICBINDX11DRV_DIR}/Src/UnHardcodedShaders.cpp
      ${ICBINDX11DRV_HEADERS}
    )
    oldunreal_set_package_properties(${ICBINDX11DRV_NAME})
    target_compile_definitions(${ICBINDX11DRV_NAME} PRIVATE SHADER_FOLDER="./${ICBINDX11DRV_NAME}/")
	set_target_properties(${ICBINDX11DRV_NAME} PROPERTIES CXX_STANDARD 17)
  endif()

  file(GLOB SCRIPTEDAIED_HEADERS ScriptedAIEd/Inc/*.h)
  file(GLOB SCRIPTEDAIED_SOURCES ScriptedAIEd/Src/*.cpp)
  if(SCRIPTEDAIED_SOURCES)
    add_library(
      ScriptedAIEd
      ${OLDUNREAL_PACKAGE_LINKAGE}
      ${SCRIPTEDAIED_SOURCES}
      ${SCRIPTEDAIED_HEADERS}
    )
    oldunreal_set_package_properties(ScriptedAIEd)
    target_include_directories(ScriptedAIEd PRIVATE ScriptedAIEd/Inc)
  endif()

  if(OLDUNREAL_AMD64)
	set(SOFTDRV_DIR "SoftDrv")
  else()
	set(SOFTDRV_DIR "SoftDrv")
  endif()
  file(GLOB SOFTDRV_HEADERS ${SOFTDRV_DIR}/Inc/*.h)
  file(GLOB SOFTDRV_SOURCES ${SOFTDRV_DIR}/Src/*.cpp)
  if(SOFTDRV_SOURCES)
    add_library(
      SoftDrv
      ${OLDUNREAL_PACKAGE_LINKAGE}
      ${SOFTDRV_SOURCES}
      ${SOFTDRV_HEADERS}
    )
    oldunreal_set_package_properties(SoftDrv)
  endif()

  file(GLOB WINDOW_HEADERS Window/Inc/*.h)
  file(GLOB WINDOW_SOURCES Window/Src/*.cpp)
  add_library(
    Window
	${OLDUNREAL_PACKAGE_LINKAGE}
	Window/Src/Res/WindowRes.rc
	${WINDOW_SOURCES}
	${WINDOW_HEADERS}
  )
  oldunreal_set_package_properties(Window)

  file(GLOB WINDRV_HEADERS WinDrv/Inc/*.h)
  file(GLOB WINDRV_SOURCES WinDrv/Src/*.cpp)
  add_library(
    WinDrv
	${OLDUNREAL_PACKAGE_LINKAGE}
	WinDrv/Src/Res/WinDrvRes.rc
	${WINDRV_SOURCES}
	${WINDRV_HEADERS}
  )
  oldunreal_set_package_properties(WinDrv)

  add_executable(
    ${OLDUNREAL_EXE}
	WIN32
    Launch/Src/Launch.cpp
    Launch/Src/LaunchPrivate.h
    Launch/Src/Res/LaunchRes.h
    Launch/Src/Res/LaunchRes.rc
    Launch/Src/Res/Logo.bmp
    hidpi.manifest
  )
  set_target_properties(${OLDUNREAL_EXE} PROPERTIES DEBUG_POSTFIX "")

  if(OLDUNREAL_BUILD_SERVER)
    add_executable(
      ${OLDUNREAL_SERVER_EXE}
	  UCC/Src/UCC.cpp
	  hidpi.manifest
    )
    set_target_properties(${OLDUNREAL_SERVER_EXE} PROPERTIES DEBUG_POSTFIX "")
  endif()

  if(OLDUNREAL_BUILD_EDITOR)
    file(GLOB UNREALED_HEADERS UnrealEd/Inc/*.h UnrealEd/Res/*.h)
    file(GLOB UNREALED_ICONS UnrealEd/Src/Res/*.bmp)
	file(GLOB UNREALED_SOURCES UnrealEd/Src/*.cpp)
    add_executable(
      ${OLDUNREAL_EDITOR_EXE}
	  WIN32
	  ${UNREALED_SOURCES}
      UnrealEd/Src/Res/UnrealEd.rc
	  ${UNREALED_ICONS}
      hidpi.manifest
      ${UNREALED_HEADERS}
    )
    set_target_properties(${OLDUNREAL_EDITOR_EXE} PROPERTIES DEBUG_POSTFIX "")
  endif()

  if(OLDUNREAL_BUILD_SETUP)
    file(GLOB SETUP_HEADERS Setup/Inc/*.h Setup/Src/Res/*.h)
	file(GLOB SETUP_SOURCES Setup/Src/*.cpp)
    add_executable(
      ${OLDUNREAL_SETUP_EXE}
	  WIN32
	  ${SETUP_SOURCES}
      Setup/Src/SetupPrivate.h
      Setup/Src/Res/SetupRes.rc
      hidpi.manifest
      ${SETUP_HEADERS}
    )
    set_target_properties(${OLDUNREAL_SETUP_EXE} PROPERTIES DEBUG_POSTFIX "")
  endif()

else() # Non-Windows targets

  file(GLOB SDLDRV_HEADERS ${SDLDRV_NAME}/Inc/*.h)
  file(GLOB SDLDRV_SOURCES ${SDLDRV_NAME}/Src/*.cpp)
  add_library(
    ${SDLDRV_NAME}
	${OLDUNREAL_PACKAGE_LINKAGE}
	${SDLDRV_SOURCES}
	${SDLDRV_HEADERS}
  )
  oldunreal_set_package_properties(${SDLDRV_NAME})

  file(GLOB SDLLAUNCH_SOURCES ${SDLLAUNCH_NAME}/Src/*.cpp)
  if(NOT OLDUNREAL_MAKE_BUNDLE)
    add_executable(
	  ${OLDUNREAL_EXE}
	  ${SDLLAUNCH_SOURCES}
    )
  else()
    add_executable(
      ${OLDUNREAL_EXE}
	  MACOSX_BUNDLE
      ${SDLLAUNCH_SOURCES}
      ${SDLLAUNCH_NAME}/Src/Unreal2.icns
    )
	set_target_properties(
	  ${OLDUNREAL_EXE}
	  PROPERTIES
	  MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/${SDLLAUNCH_NAME}/Src/Unreal.plist.in
	)
  endif()

  if(OLDUNREAL_BUILD_SERVER)
    add_executable(
	  ${OLDUNREAL_SERVER_EXE}
	  UCC/Src/UCC.cpp
	)
  endif()

endif()

################################################################################
# Common Linking Rules
################################################################################

get_target_property(CORE_IMPORTED Core IMPORTED)
get_target_property(ENGINE_IMPORTED Engine IMPORTED)
get_target_property(EDITOR_IMPORTED Editor IMPORTED)
get_target_property(FIRE_IMPORTED Fire IMPORTED)
get_target_property(IPDRV_IMPORTED IpDrv IMPORTED)
get_target_property(RENDER_IMPORTED Render IMPORTED)
get_target_property(UWEBADMIN_IMPORTED UWebAdmin IMPORTED)
get_target_property(UPAK_IMPORTED UPak IMPORTED)
get_target_property(PATHLOGIC_IMPORTED PathLogic IMPORTED)
get_target_property(USQLITE_IMPORTED USQLite IMPORTED)
get_target_property(UENGINEPPC_IMPORTED UEnginePPC IMPORTED)
get_target_property(EMITTER_IMPORTED Emitter IMPORTED)

if(NOT CORE_IMPORTED)
  target_link_libraries(
    Core
    ${ZLIB_LIBRARY}
    ${SDL2_LIBRARIES}
  )
endif()

if(NOT ENGINE_IMPORTED)
  target_link_libraries(
    Engine
    ${PNG_LIBRARY}
    ${ZLIB_LIBRARY}
    ${VORBISFILE_LIBRARY}
    ${VORBISENC_LIBRARY}
    ${VORBIS_LIBRARY}
    ${OGG_LIBRARY}
  )
endif()

if(NOT ENGINE_IMPORTED AND KTEXCOMP_LIBRARY)
  target_link_libraries(
	Engine
	${KTEXCOMP_LIBRARY}
  )
endif()

if(NOT EDITOR_IMPORTED)
  target_link_libraries(
    Editor
    Render
  )
endif()

#if (NOT OLDUNREAL_USE_ASAN)
target_link_libraries(
  ALAudio
  ${SDL2_LIBRARIES}
  ${OPENAL_LIBRARY}
  ${ALURE_LIBRARY}
  ${XMP_LIBRARY}
  ${VORBISFILE_LIBRARY}
  ${VORBIS_LIBRARY}
  ${OGG_LIBRARY}
  ${FLAC_LIBRARY}
  ${MPG123_LIBRARY}
  ${OPUS_LIBRARY}
  ${SNDFILE_LIBRARY}
)
#endif()

if(OLDUNREAL_BUILD_PHYSX AND TARGET PhysXPhysics)
target_link_libraries(
  PhysXPhysics
  PhysX::PhysX
  PhysX::PhysXCooking
  PhysX::PhysXFoundation
  PhysX::PhysXCommon
  PhysX::PhysXExtensions
)
endif()


if(OLDUNREAL_BUILD_GLIDEDRV AND TARGET GlideDrv)
  target_link_libraries(
    GlideDrv
	Render
	${OPENGLIDE_LIBRARY}
  )
endif()

if(OLDUNREAL_BUILD_ICBINDX11DRV)
  target_link_libraries(${ICBINDX11DRV_NAME}
    Render
    d3d11.lib
    d3dcompiler.lib
    DWrite.lib
    D2d1.lib
	ntdll.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/External/dxsdk/Lib/${OLDUNREAL_CPU}/dxguid.lib
  )  
endif()

if(OLDUNREAL_STATIC_LINK)
  target_link_libraries(
    ${OLDUNREAL_EXE}
	Emitter
    Editor
    IpDrv
    Fire
    Render
    XOpenGLDrv
	PhysXPhysics
	UPak
	PathLogic
	USQLite
	UEnginePPC
	UWebAdmin
	${VORBISFILE_LIBRARY}
    ${XMP_LIBRARY}
    ${ALURE_LIBRARY}
    ${OPENAL_LIBRARY}
  )

#  if(NOT OLDUNREAL_USE_ASAN)
    target_link_libraries(
	  ${OLDUNREAL_EXE}
	  ALAudio
    )
#  endif()

  if(OLDUNREAL_BUILD_SWFMOD)
    target_link_libraries(${OLDUNREAL_EXE} SwFMOD)
  endif()

  if(OLDUNREAL_BUILD_GLIDEDRV)
	target_link_libraries(${OLDUNREAL_EXE} GlideDrv)
  endif()
  
  if(OLDUNREAL_BUILD_ICBINDX11DRV)
    target_link_libraries(
    ${OLDUNREAL_EXE}
    ${ICBINDX11DRV_NAME}
    )
  endif()

  if (OLDUNREAL_BUILD_OPENGLDRV)
    target_link_libraries(${OLDUNREAL_EXE} OpenGLDrv)
  endif()

  if(OLDUNREAL_BUILD_SERVER)
    target_link_libraries(
      ${OLDUNREAL_SERVER_EXE}
	  Emitter
      Editor
      Fire
      IpDrv
	  Render
	  UPak
	  PathLogic
	  USQLite
	  UEnginePPC
	  UWebAdmin
	  PhysXPhysics
    )
  endif()

  if(OLDUNREAL_BUILD_EDITOR)
    target_link_libraries(
	  ${OLDUNREAL_EDITOR_EXE}
	  Emitter
      IpDrv
      Fire
      Render
	  XOpenGLDrv
	  PhysXPhysics
	  UPak
	  PathLogic
	  USQLite
	  UEnginePPC
	  UWebAdmin
	)

#    if(NOT OLDUNREAL_USE_ASAN)
	  target_link_libraries(
		${OLDUNREAL_EDITOR_EXE}
		ALAudio
      )
#	endif()

    if(OLDUNREAL_BUILD_SWFMOD)
	  target_link_libraries(${OLDUNREAL_EDITOR_EXE} SwFMOD)
	endif()
	
	if(OLDUNREAL_BUILD_ICBINDX11DRV)
        target_link_libraries(
         ${OLDUNREAL_EDITOR_EXE}
         ${ICBINDX11DRV_NAME}
        )
    endif()

	if(OLDUNREAL_BUILD_GLIDEDRV)
	  target_link_libraries(${OLDUNREAL_EDITOR_EXE} GlideDrv)
    endif()

    if(OLDUNREAL_BUILD_OPENGLDRV)
	  target_link_libraries(${OLDUNREAL_EDITOR_EXE} OpenGLDrv)
	endif()

  endif()

endif()

target_link_libraries(
  ${OLDUNREAL_EXE}
  Engine
  Core
)

if (OLDUNREAL_BUILD_SERVER)
  target_link_libraries(
    ${OLDUNREAL_SERVER_EXE}
	Engine
	Core
  )
endif()

if (OLDUNREAL_BUILD_SETUP)
  target_link_libraries(
    ${OLDUNREAL_SETUP_EXE}
	Core
  )
endif()

if (OLDUNREAL_BUILD_EDITOR)
  target_link_libraries(
    ${OLDUNREAL_EDITOR_EXE}
	Editor
	Engine
	Core
  )
endif()

################################################################################
# Common Linker Rules for Linux and Mac
################################################################################
if(UNIX)
  # We don't have RDTSC on ARM so we use SDL2 to get timestamps
  if (OLDUNREAL_ARM AND NOT CORE_IMPORTED)
    target_link_libraries(Core ${SDL2_LIBRARIES})
  endif()

  target_link_libraries(
    ${SDLDRV_NAME}
	${SDL2_LIBRARIES}
	${SDL2_TTF_LIBRARY}
	${FREETYPE_LIBRARIES}
  )

  target_link_libraries(
    ${OLDUNREAL_EXE}
	${SDLDRV_NAME}
    ${SDL2_LIBRARIES}
	${SDL2_TTF_LIBRARY}
  )

  if (OLDUNREAL_BUILD_SERVER)
    target_link_libraries(${OLDUNREAL_SERVER_EXE} ${SDL2_LIBRARIES})
  endif()

endif()


################################################################################
# Platform-specific Linking Rules
################################################################################

if(WINDOWS)
  if(NOT ENGINE_IMPORTED)
    target_link_libraries(Engine ws2_32)
  endif()
  if(NOT CORE_IMPORTED)
    target_link_libraries(Core winmm IpHlpApi)
  endif()
  if(NOT IPDRV_IMPORTED)
    target_link_libraries(IpDrv ws2_32)
  endif()
  target_link_libraries(Window winmm comctl32)
  if(NOT EDITOR_IMPORTED)
    target_link_libraries(Editor Window)
  endif()
  if(TARGET SoftDrv)
    target_link_libraries(SoftDrv Render)
  endif()
  target_link_libraries(D3DDrv winmm dxguid)
  target_link_libraries(D3D9Drv winmm
	${CMAKE_CURRENT_SOURCE_DIR}/External/dxsdk/Lib/${OLDUNREAL_CPU}/d3dx9.lib
  )
  target_link_libraries(ScriptedAIEd Window)
  
  if(OLDUNREAL_BUILD_ICBINDX11DRV)
    install(
      TARGETS
      ${ICBINDX11DRV_NAME}
      DESTINATION ./
    )
  endif()
  
  target_link_libraries(WinDrv Window dxguid imm32 dinput8)

  if(NOT OLDUNREAL_AMD64)
	target_link_libraries(D3D8Drv
	  ${CMAKE_CURRENT_SOURCE_DIR}/External/dxsdk/Lib/${OLDUNREAL_CPU}/d3dx8.lib
    )

	if(TARGET Galaxy)
      target_link_libraries(
        Galaxy
  	    dxguid
  	    winmm
	    ${GALAXY_LIBRARY}
	    ${OGG_LIBRARY}
	    ${VORBIS_LIBRARY}
	    ${VORBISFILE_LIBRARY}
	    ${VORBISENC_LIBRARY}
      )

	  # GalaxyLib doesn't support SafeSEH because of its assembly code. We should rewrite it in C some time...
	  target_link_options(Galaxy
	    PRIVATE
	    /SAFESEH:NO
	  )
	endif()

    if(OLDUNREAL_STATIC_LINK)
	  target_link_options(
		${OLDUNREAL_EXE}
	    PRIVATE
	    /SAFESEH:NO
	  )

	  if(OLDUNREAL_BUILD_EDITOR)
	    target_link_options(
		  ${OLDUNREAL_EDITOR_EXE}
	      PRIVATE
	      /SAFESEH:NO
	    )
	  endif()
	endif()
  endif()

  if(OLDUNREAL_BUILD_SWFMOD)
    target_link_libraries(
      SwFMOD
	  ${FMODEX_LINK_LIBRARY}
    )
  endif()

#  if(NOT OLDUNREAL_USE_ASAN)
    target_link_libraries(
      ALAudio
      winmm
    )
#  endif()

  if (OLDUNREAL_BUILD_OPENGLDRV)
    target_link_libraries(OpenGLDrv winmm opengl32)
  endif()

  target_link_libraries(XOpenGLDrv opengl32)

  if(OLDUNREAL_STATIC_LINK)
    target_link_libraries(
      ${OLDUNREAL_EXE}
      WinDrv
      SoftDrv
      D3DDrv
      D3D9Drv
    )

    if(NOT OLDUNREAL_AMD64)
      target_link_libraries(${OLDUNREAL_EXE} Galaxy D3D8Drv)
    endif()

    if(OLDUNREAL_BUILD_EDITOR)
      target_link_libraries(
        ${OLDUNREAL_EDITOR_EXE}
        WinDrv
        SoftDrv
        D3DDrv
        D3D9Drv
		ScriptedAIEd
      )
      if(NOT OLDUNREAL_AMD64)
	    target_link_libraries(${OLDUNREAL_EDITOR_EXE} Galaxy D3D8Drv)
	  endif()
    endif()

  endif()

  target_link_libraries(${OLDUNREAL_EXE} Window)
  if(OLDUNREAL_BUILD_EDITOR)
    target_link_libraries(${OLDUNREAL_EDITOR_EXE} Window comctl32)
  endif()
  if(OLDUNREAL_BUILD_SETUP)
    target_link_libraries(${OLDUNREAL_SETUP_EXE} Window)
  endif()

elseif(LINUX)
  if(NOT CORE_IMPORTED)
    target_link_libraries(
      Core
  	  dl
    )
  endif()

  set_target_properties(
    ${OLDUNREAL_EXE}
	PROPERTIES
	LINK_FLAGS "-Wl,-rpath,$ORIGIN"
  )
  target_link_libraries(
    ${OLDUNREAL_EXE}
    dl
    pthread
  )

  if(OLDUNREAL_BUILD_SERVER)
    set_target_properties(
      ${OLDUNREAL_SERVER_EXE}
	  PROPERTIES
	  LINK_FLAGS "-Wl,-rpath,$ORIGIN"
    )
    target_link_libraries(
	  ${OLDUNREAL_SERVER_EXE}
	  dl
	  pthread
	)
  endif()

elseif(APPLE)
  target_link_libraries(
    Core
	${COCOA_FRAMEWORK}
	iconv
	${SDL2_LIBRARIES}
  )

  if(OLDUNREAL_BUILD_FRUCORE)
    if(OLDUNREAL_STATIC_LINK)
      target_link_libraries(
        ${OLDUNREAL_EXE}
        Frucore
        ${METAL_FRAMEWORK}
        ${FOUNDATION_FRAMEWORK}
        ${COCOA_FRAMEWORK}
        ${COREGRAPHICS_FRAMEWORK}
        ${METALKIT_FRAMEWORK}
      )
    else()
      target_link_libraries(
        Frucore
        ${METAL_FRAMEWORK}
        ${FOUNDATION_FRAMEWORK}
        ${COCOA_FRAMEWORK}
        ${COREGRAPHICS_FRAMEWORK}
        ${METALKIT_FRAMEWORK}		
      )
    endif()
  endif()

  target_link_libraries(
    ${OLDUNREAL_EXE}
	${CARBON_FRAMEWORK}
	${COCOA_FRAMEWORK}
  )

endif()

################################################################################
# Common Installation Rules
################################################################################

if(NOT OLDUNREAL_STATIC_LINK)
  install(
    TARGETS
	XOpenGLDrv
	ALAudio
    DESTINATION ./
  )

  if(NOT CORE_IMPORTED)
	install(TARGETS Core DESTINATION ./)
  endif()

  if(NOT ENGINE_IMPORTED)
	install(TARGETS Engine DESTINATION ./)
  endif()

  if(NOT EDITOR_IMPORTED)
	install(TARGETS Editor DESTINATION ./)
  endif()

  if(NOT FIRE_IMPORTED)
	install(TARGETS Fire DESTINATION ./)
  endif()

  if(NOT IPDRV_IMPORTED)
	install(TARGETS IpDrv DESTINATION ./)
  endif()

  if(NOT RENDER_IMPORTED)
	install(TARGETS Render DESTINATION ./)
  endif()

  if(NOT UWEBADMIN_IMPORTED)
	install(TARGETS UWebAdmin DESTINATION ./)
  endif()

  if(NOT UPAK_IMPORTED)
	install(TARGETS UPak DESTINATION ./)
  endif()

  if(NOT PATHLOGIC_IMPORTED)
	install(TARGETS PathLogic DESTINATION ./)
  endif()

  if(NOT USQLITE_IMPORTED)
	install(TARGETS USQLite DESTINATION ./)
  endif()

  if(NOT UENGINEPPC_IMPORTED)
	install(TARGETS UEnginePPC DESTINATION ./)
  endif()

  if(NOT EMITTER_IMPORTED)
	install(TARGETS Emitter DESTINATION ./)
  endif()
 
  if(OLDUNREAL_BUILD_PHYSX AND TARGET PhysXPhysics)
    install(TARGETS PhysXPhysics DESTINATION ./)
  endif()
  
  if(OLDUNREAL_BUILD_SWFMOD AND TARGET SwFMOD)
    install(TARGETS SwFMOD DESTINATION ./)
  endif()

  if(OLDUNREAL_BUILD_GLIDEDRV AND TARGET GlideDrv)
	install(TARGETS GlideDrv DESTINATION ./)
  endif()

  if(OLDUNREAL_BUILD_OPENGLDRV)
    install(TARGETS OpenGLDrv DESTINATION ./)
  endif()

  if (OLDUNREAL_BUILD_FRUCORE)
    install(TARGETS Frucore DESTINATION ./)
  endif()

endif()

if(OLDUNREAL_BUILD_FRUCORE)
  if(NOT CMAKE_GENERATOR STREQUAL Xcode)
    if(NOT OLDUNREAL_MAKE_BUNDLE)
      install(FILES ${CMAKE_CURRENT_BINARY_DIR}/default.metallib DESTINATION ./)
	else()
  	  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/default.metallib DESTINATION ${OLDUNREAL_EXE}.app/Contents/Resources/)
	endif()
  endif()
endif()


if (NOT OLDUNREAL_MAKE_BUNDLE)
  install(
	TARGETS
	${OLDUNREAL_EXE}
	DESTINATION ./
  )
endif()

if (OLDUNREAL_BUILD_SERVER AND NOT OLDUNREAL_MAKE_BUNDLE)
  install(
	TARGETS
	${OLDUNREAL_SERVER_EXE}
	DESTINATION ./
  )
endif()

if (OLDUNREAL_BUILD_EDITOR)
  install(
	TARGETS
	${OLDUNREAL_EDITOR_EXE}
	DESTINATION ./
  )
endif()

if (OLDUNREAL_BUILD_SETUP)
  install(
	TARGETS
	${OLDUNREAL_SETUP_EXE}
    DESTINATION ./
  )
endif()

################################################################################
# Platform-specific Installation Rules
################################################################################

if(WINDOWS)
  if (NOT OLDUNREAL_STATIC_LINK AND NOT OLDUNREAL_AMD64 AND TARGET Galaxy)
    install(TARGETS Galaxy D3D8Drv DESTINATION ./)
  endif()

  install(
    TARGETS
	Window
	WinDrv
	D3DDrv
	D3D9Drv
	ScriptedAIEd
	DESTINATION ./
  )

  if(TARGET SoftDrv)
	install(
	  TARGETS
	  SoftDrv
	  DESTINATION ./
    )
  endif()

  foreach(LIB
      ${XMP_LIBRARY_RELEASE}
	  ${OPENAL_LIBRARY_RELEASE}
	  ${MPG123_LIBRARY_RELEASE}
      ${OPENGLIDE_LIBRARY_RELEASE}
	  ${SNDFILE_LIBRARY_RELEASE}
  )
    string(REPLACE ".lib" ".dll" TMP ${LIB})
    string(REPLACE "lib/" "bin/" OUT ${TMP})
	if(NOT OUT STREQUAL "")
	  install(FILES ${OUT} DESTINATION ./)
    endif()
  endforeach()

  install(
    FILES
	${OPENAL_INSTALL_LIBRARY}
	${AKELEDIT_LIBRARY}
	DESTINATION ./
  )

  if(OLDUNREAL_BUILD_SWFMOD)
	install(
	  FILES
	  ${FMODEX_INSTALL_LIBRARY}
	  DESTINATION ./
	)
  endif()
  
  if(OLDUNREAL_BUILD_ICBINDX11DRV)
	install(
	  DIRECTORY
	  ${ICBINDX11DRV_DIR}/Shaders/
	  DESTINATION ./${ICBINDX11DRV_NAME}
	)
  endif()

elseif(LINUX)
  if(NOT OLDUNREAL_STATIC_LINK)
    install(
      TARGETS
	  ${SDLDRV_NAME}
      DESTINATION ./
    )
  endif()

  foreach(LIB
  	"${XMP_LIBRARY};libxmp.so.4"
	"${OPENAL_LIBRARY};libopenal.so.1"
	"${SDL2_LIBRARIES};libSDL2-2.0.so.0"
	"${MPG123_LIBRARY};libmpg123.so.0"
	"${MPG123_LIBRARY};libmpg123.so"
	"${SNDFILE_LIBRARY};libsndfile.so.1"
  )
    list(GET LIB -2 ORIG_NAME)
	list(GET LIB -1 NEW_NAME)

    get_library_path(ORIG_NAME ABSOLUTE_PATH)

	install(
	  FILES ${ABSOLUTE_PATH}
	  RENAME ${NEW_NAME}
	  DESTINATION ./
	)
  endforeach()

  if(OLDUNREAL_BUILD_GLIDEDRV)
    get_library_path(OPENGLIDE_LIBRARY OPENGLIDE_INSTALL_LIBRARY)
	install(
	  FILES OPENGLIDE_INSTALL_LIBRARY
	  RENAME libglide2x.so
	  DESTINATION ./
    )
  endif()

elseif(APPLE AND OLDUNREAL_MAKE_BUNDLE)

  set(MACOSX_BUNDLE_ICON_FILE Unreal2.icns)
  set(MACOSX_BUNDLE_BUNDLE_VERSION ${OLDUNREAL_VERSION})
  set(MACOSX_BUNDLE_SHORT_VERSION_STRING ${OLDUNREAL_VERSION})
  set(MACOSX_BUNDLE_LONG_VERSION_STRING ${OLDUNREAL_VERSION})
  string(TOLOWER "com.oldunreal.${CMAKE_PROJECT_NAME}" MACOSX_BUNDLE_GUI_IDENTIFIER)
  set(MACOSX_BUNDLE_INFO_STRING "${CMAKE_PROJECT_NAME} - version ${OLDUNREAL_VERSION}")
  set(MACOSX_BUNDLE_BUNDLE_NAME "${CMAKE_PROJECT_NAME}")
  set(MACOSX_BUNDLE_COPYRIGHT "Epic Games")

  # Copies the latest .u and .int files into the app bundle template
  add_custom_command(TARGET ${OLDUNREAL_EXE}
    PRE_BUILD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ./scripts/mac-update-template.sh
  )

  set_source_files_properties(
    ${SDLLAUNCH_NAME}/Src/Unreal2.icns
	PROPERTIES
	MACOSX_PACKAGE_LOCATION
	Resources
  )

  set(APPS "${CMAKE_INSTALL_PREFIX}/${OLDUNREAL_EXE}.app")
  set(DIRS "${CMAKE_INSTALL_PREFIX}/")
  set(LIBS
    "${CMAKE_INSTALL_PREFIX}/${OLDUNREAL_EXE}.app/Contents/Frameworks/libopenal.1.dylib"
    "${CMAKE_INSTALL_PREFIX}/${OLDUNREAL_EXE}.app/Contents/Frameworks/libxmp.4.dylib"
    "${CMAKE_INSTALL_PREFIX}/${OLDUNREAL_EXE}.app/Contents/Frameworks/libmpg123.dylib"
    "${CMAKE_INSTALL_PREFIX}/${OLDUNREAL_EXE}.app/Contents/Frameworks/libsndfile.1.dylib"	
    "${CMAKE_INSTALL_PREFIX}/${OLDUNREAL_EXE}.app/Contents/Frameworks/libSDL2-2.0.0.dylib"
  )

  install(
    TARGETS ${OLDUNREAL_EXE}
    BUNDLE DESTINATION . COMPONENT Runtime
    RUNTIME DESTINATION bin COMPONENT Runtime
  )

  if(OLDUNREAL_BUILD_SERVER)
    install(
	  TARGETS ${OLDUNREAL_SERVER_EXE}
	  DESTINATION ${OLDUNREAL_EXE}.app/Contents/MacOS/
	)
  endif()

  # Copy data from the template into the bundle
  foreach(
	FOLDER
	"Textures"
	"Maps"
	"Sounds"
	"Music"
	"System"
  )
    install(
      DIRECTORY MacTemplate/${FOLDER}Clean/
      DESTINATION ${OLDUNREAL_EXE}.app/Contents/MacOS/${FOLDER}
    )
  endforeach()

  # Copy data from the root folder into the bundle
  foreach(
	FOLDER
	"Help"
	"WebServer"
	"SystemLocalized"
  )
    install(
	  DIRECTORY ${FOLDER}
      DESTINATION ${OLDUNREAL_EXE}.app/Contents/MacOS/
    )
  endforeach()

  # Copy libraries into the bundle
  foreach(LIB
	"${OPENAL_LIBRARY};libopenal.1.dylib"
	"${XMP_LIBRARY};libxmp.4.dylib"
	"${MPG123_LIBRARY};libmpg123.dylib"
    "${SNDFILE_LIBRARY};libsndfile.1.dylib"	
	"${SDL2_LIBRARIES};libSDL2-2.0.0.dylib"
  )
    # Ugh. SDL2 has two libs in the list
    list(GET LIB -2 ORIG_NAME)
    list(GET LIB -1 NEW_NAME)

    get_library_path(ORIG_NAME ABSOLUTE_PATH)
    
    install(
      FILES ${ABSOLUTE_PATH}
      RENAME ${NEW_NAME}
      DESTINATION ${OLDUNREAL_EXE}.app/Contents/Frameworks
    )
  endforeach()

  install(
    CODE "
	  include(BundleUtilities)
	  fixup_bundle(\"${APPS}\"   \"${LIBS}\"   \"${DIRS}\")
	"
    COMPONENT Runtime
  )

elseif(APPLE)
  if(NOT OLDUNREAL_STATIC_LINK)
    install(
      TARGETS
	  ${SDLDRV_NAME}
      DESTINATION ./
    )
  endif()

  get_library_path(SDL2_LIBRARIES SDL2_INSTALL_LIBRARY)

  install(
    FILES
	${XMP_LIBRARY}
	${OPENAL_LIBRARY}
	${SDL2_INSTALL_LIBRARY}
	${MPG123_LIBRARY}
	${SNDFILE_LIBRARY}
	DESTINATION ./
  )
endif()

# end of CMakeLists.txt ...
