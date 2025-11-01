
# to fix indentation: cmake-format --dangle-parens --dangle-align=prefix ../CMakeLists.txt > out.txt

set(OLDUNREAL_VERSION "469e")
cmake_minimum_required(VERSION 3.13) # we need add_link_options
set(OLDUNREAL_METAL_VERSION macos-metal2.0)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_PROJECT_VERSION_MAJOR "")
set(CMAKE_PROJECT_VERSION_MINOR "")
set(CMAKE_PROJECT_VERSION_PATCH "")
# uncomment this to see compiler/linker commands
# set(CMAKE_VERBOSE_MAKEFILE ON)
project(UT99)

include(CheckCCompilerFlag)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if (COMMAND CMAKE_POLICY)
  if (POLICY CMP0177)
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

check_cpu("defined(__arm64__) || defined(__aarch64__)" "arm64" OLDUNREAL_ARM64)

if (NOT OLDUNREAL_CHOSE_CPU)
  message(FATAL_ERROR "We don't support this architecture yet")
endif()

################################################################################
# Build options - Available in all builds
################################################################################

option(OLDUNREAL_BUILD_OPENGLDRV
  "Set to true to build UTGLR OpenGLDrv." TRUE
)
option(OLDUNREAL_USE_MIMALLOC
  "Set to true to use mimalloc as the memory allocator." TRUE
)
if(NOT WINDOWS)
  option(OLDUNREAL_USE_ASAN "True to enable address sanitizer" FALSE)
  option(OLDUNREAL_USE_TSAN "True to enable thread sanitizer" FALSE)
endif()

if (WINDOWS AND NOT "${CMAKE_GENERATOR_TOOLSET}" MATCHES "_xp")

  # We do not recommend building d3d10drv as it relies on a _LOT_ of
  #  deprecated DirectX functionality that is no longer available in
  #  the Windows SDK
  
  option(OLDUNREAL_BUILD_D3D10DRV
    "Set to true to build Marijn Kentie's D3D10Drv." FALSE
  )
  option(OLDUNREAL_BUILD_ICBINDX11DRV
    "Set to true to build Metallicafan212's ICBINDx11Drv." TRUE
  )
  option(OLDUNREAL_BUILD_D3D11DRV
    "Set to true to build dpJudas' D3D11Drv." TRUE
  )
  option(OLDUNREAL_BUILD_D3D12DRV
    "Set to true to build dpJudas' D3D12Drv." FALSE
  )
  option(OLDUNREAL_BUILD_VULKANDRV
    "Set to true to build dpJudas' VulkanDrv." TRUE
  )
endif()

# we could build this for Apple too...
if(LINUX)
  option(
    OLDUNREAL_BUILD_WX_LAUNCHER
    "Set to true to compile the wxWidgets-enabled launcher"
    FALSE
  )
  option(OLDUNREAL_BUILD_VULKANDRV
    "Set to true to build dpJudas' VulkanDrv." TRUE
  )
endif()

if(NOT WINDOWS)
  option(OLDUNREAL_BUILD_SDL2DRV
    "Set to true to build with the SDL2-based version of SDLDrv/SDLLaunch" TRUE
  )
endif()

################################################################################
# Build options - Not available for the SDK
################################################################################

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/UnObj.cpp)
  option(OLDUNREAL_BUILD_OLDOPENGLDRV
    "Set to true to build the original UTGLR OpenGLDrv (has no effect if OLDUNREAL_BUILD_OPENGLDRV is FALSE)." FALSE
  )
  option(OLDUNREAL_BUILD_OLDXOPENGLDRV
    "Set to true to build the old XOpenGLDrv." FALSE
  )
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

  set(WANT_GALAXY TRUE)

  if(NOT WIN32)
    add_compile_definitions(SDLBUILD=1)
    if(NOT OLDUNREAL_BUILD_SDL2DRV)
      set(WANT_SDL2 OFF)
  	  set(SDLDRV_DIR SDL3Drv)
  	  set(SDLLAUNCH_DIR SDL3Launch)
	  add_compile_definitions(SDL2BUILD=0)
	  add_compile_definitions(SDL3BUILD=1)
    else()
      set(WANT_SDL2 ON)
	  set(SDLDRV_DIR SDLDrv)
	  set(SDLLAUNCH_DIR SDLLaunch)
	  add_compile_definitions(SDL2BUILD=1)
	  add_compile_definitions(SDL3BUILD=0)
    endif()
  endif()
  
else()
  set(OLDUNREAL_BUILD_OLDOPENGLDRV FALSE)
  set(OLDUNREAL_BUILD_OLDXOPENGLDRV FALSE)
  set(OLDUNREAL_STATIC_LINK FALSE)
  set(OLDUNREAL_MAKE_BUNDLE FALSE)
  set(OLDUNREAL_BUILD_FRUCORE FALSE)
  set(WANT_GALAXY FALSE)
  
  if(APPLE)
    message(FATAL_ERROR "The OldUnreal SDK is not available on macOS. Sorry!")
  endif()
  
  if(EMSCRIPTEN)
    message(FATAL_ERROR "The OldUnreal SDK is not available for emscripten. Sorry!")
  endif()

  if(NOT OLDUNREAL_X86)
    message(FATAL_ERROR "The OldUnreal SDK is not available for 64-bit or ARM versions of Windows. Consider building with cmake -A Win32 to use our 32-bit SDK.")
  endif()
  
endif()

if((WIN32 AND "${CMAKE_GENERATOR_TOOLSET}" MATCHES "_xp") OR OLDUNREAL_USE_ASAN OR NOT WINDOWS)
  set(WANT_MIMALLOC FALSE)
else()
  set(WANT_MIMALLOC ${OLDUNREAL_USE_MIMALLOC})
endif()

################################################################################
# Figure out which executables to build + their names
################################################################################
if(WINDOWS)
  set(OLDUNREAL_EXE "UnrealTournament")
  set(OLDUNREAL_BUILD_SERVER TRUE)
  set(OLDUNREAL_SERVER_EXE "UCC")
  set(OLDUNREAL_BUILD_EDITOR TRUE)
  set(OLDUNREAL_EDITOR_EXE "UnrealEd")
  set(OLDUNREAL_BUILD_SETUP TRUE)
  set(OLDUNREAL_SETUP_EXE "Setup")

elseif(LINUX)
  set(OLDUNREAL_EXE "ut-bin-${OLDUNREAL_CPU}")
  set(OLDUNREAL_BUILD_SERVER TRUE)
  set(OLDUNREAL_SERVER_EXE "ucc-bin-${OLDUNREAL_CPU}")
  set(OLDUNREAL_BUILD_EDITOR FALSE)
  set(OLDUNREAL_BUILD_SETUP FALSE)
  set(OLDUNREAL_WX_LAUNCHER_EXE "wx-ut-bin-${OLDUNREAL_CPU}")

elseif(APPLE)
  if(OLDUNREAL_MAKE_BUNDLE)
    set(OLDUNREAL_EXE "UnrealTournament")
    set(OLDUNREAL_BUILD_SERVER TRUE)
	set(OLDUNREAL_SERVER_EXE "UCC")
    set(OLDUNREAL_BUILD_EDITOR FALSE)
    set(OLDUNREAL_BUILD_SETUP FALSE)

    # Sanity check
    if(OLDUNREAL_BUILD_WX_LAUNCHER)
      message(FATAL_ERROR "We cannot build the wxWidgets launcher for macOS yet")
    endif()

  else()
    set(OLDUNREAL_EXE "macos-ut-bin-${OLDUNREAL_CPU}")
    set(OLDUNREAL_BUILD_SERVER TRUE)
    set(OLDUNREAL_SERVER_EXE "macos-ucc-bin-${OLDUNREAL_CPU}")
    set(OLDUNREAL_BUILD_EDITOR FALSE)
    set(OLDUNREAL_BUILD_SETUP FALSE)
    set(OLDUNREAL_WX_LAUNCHER_EXE "macos-wx-ut-bin-${OLDUNREAL_CPU}")
	
  endif()

endif()

if (OLDUNREAL_BUILD_OLDOPENGLDRV)
  set(OPENGLDRV_DIR "FruitCompanyOpenGLDrv")
else ()
  set(OPENGLDRV_DIR "UTGLROpenGLDrv")
endif()

if (OLDUNREAL_BUILD_OLDXOPENGLDRV)
  set(XOPENGLDRV_DIR "XOpenGLDrv469a")
else ()
  set(XOPENGLDRV_DIR "XOpenGLDrv")
endif ()

if (OLDUNREAL_BUILD_ICBINDX11DRV)
  set(ICBINDX11DRV_NAME "ICBINDx11Drv")
  set(ICBINDX11DRV_DIR "ICBINDx11Drv")
endif()

if (OLDUNREAL_BUILD_D3D11DRV)
  set(D3D11DRV_NAME "D3D11Drv")
  set(D3D11DRV_DIR "UT99VulkanDrv/D3D11Drv")
endif()

if (OLDUNREAL_BUILD_D3D12DRV)
  set(D3D12DRV_NAME "D3D12Drv")
  set(D3D12DRV_DIR "UT99VulkanDrv/D3D12Drv")
endif()

if (OLDUNREAL_BUILD_VULKANDRV)
  set(VULKANDRV_NAME "VulkanDrv")
  set(VULKANDRV_DIR "UT99VulkanDrv/VulkanDrv")
endif()

if(OLDUNREAL_AMD64)
  set(SOFTDRV_DIR "SoftDrv64")
else()
  set(SOFTDRV_DIR "SoftDrv")
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

include(${CMAKE_CURRENT_SOURCE_DIR}/External/cmake/FindUE1Dependencies.cmake)

################################################################################
# Set compiler and linker flags
################################################################################

add_compile_definitions("$<$<CONFIG:Debug>:_DEBUG>")
add_compile_definitions("$<$<CONFIG:RelWithDebInfo>:_RELEASE>")
add_compile_definitions("$<$<CONFIG:RelWithDebInfo>:NDEBUG>")
add_compile_definitions("$<$<CONFIG:RelWithDebInfo>:RELEASE>")
add_compile_definitions("$<$<CONFIG:Release>:_RELEASE>")
add_compile_definitions("$<$<CONFIG:Release>:NDEBUG>")
add_compile_definitions("$<$<CONFIG:Release>:RELEASE>")

add_compile_definitions(UTGLR_UT_BUILD)
add_compile_definitions(UTGLR_USES_ALPHABLEND=1)
add_compile_definitions(ENGINE_ARCHITECTURE="${OLDUNREAL_CPU}")
if(WANT_MIMALLOC)
  add_compile_definitions("OLDUNREAL_USE_MIMALLOC=1")
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
  set(CMAKE_INSTALL_RPATH "$ORIGIN")
  add_compile_definitions(UNICODE=1)
  add_compile_definitions(_UNICODE=1)
  add_compile_definitions(_REENTRANT=1)
  add_compile_definitions(HAVE_PRAGMA_PACK=1)
  add_definitions(-fPIC)

  # disable A3D in Cluster
  add_compile_definitions(USE_A3D2=0)

  if(OLDUNREAL_ARM OR OLDUNREAL_ARM64)
    add_compile_definitions(__LINUX_ARM__=1)
    add_compile_definitions(SDL_DISABLE_MM3DNOW_H=1)
    add_compile_definitions(SDL_DISABLE_IMMINTRIN_H=1)
    add_compile_definitions(SDL_DISABLE_MMINTRIN_H=1)
    add_compile_definitions(SDL_DISABLE_XMMINTRIN_H=1)
    add_compile_definitions(SDL_DISABLE_EMMINTRIN_H=1)
    add_compile_definitions(SDL_DISABLE_PMMINTRIN_H=1)
    if (OLDUNREAL_ARM)
      add_definitions(-mfpu=neon-vfpv4)
      add_definitions(-mfloat-abi=hard)
    endif()
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
  # disable A3D in Cluster
  add_compile_definitions(USE_A3D2=0)

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
    # /Zc:wchar_t-
    # do not treat wchar_t as a built-in type. Doing so would break binary
    # compat with almost all native mods for earlier UT versions
    #
    # /arch:SSE2
    # assume we have at least a Pentium 4
    #
    # /LARGEADDRESSAWARE
    # we can handle more than 2Gb of RAM even in 32bit builds
    #
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:wchar_t- /arch:SSE2")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Zc:wchar_t- /arch:SSE2")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LARGEADDRESSAWARE")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /LARGEADDRESSAWARE")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /LARGEADDRESSAWARE")
    set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} /LARGEADDRESSAWARE")
	add_compile_definitions(WINDOWS_IGNORE_PACKING_MISMATCH=1)
  endif()
  #
  # /MP
  # enable parallel builds
  #
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")

  # the recursive garbage collector needs a lot of stack space
  set(CMAKE_EXE_LINKER_FLAGS
      "${CMAKE_EXE_LINKER_FLAGS} /STACK:16000000,1000000 /LTCG"
  )
	
endif()

if(CMAKE_COMPILER_IS_GNUCC OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pipe -fsigned-char -fno-strict-aliasing -Wno-inline-new-delete")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pipe -fsigned-char -fno-strict-aliasing")
endif()

if(OLDUNREAL_STATIC_LINK)
  add_compile_definitions(__STATIC_LINK=1)
  set(OLDUNREAL_PACKAGE_LINKAGE STATIC)
else()
  set(OLDUNREAL_PACKAGE_LINKAGE SHARED)
endif()

if(OLDUNREAL_USE_ASAN)
  add_compile_options("-fsanitize=address")
  link_libraries("-fsanitize=address")
  add_compile_definitions("OLDUNREAL_USE_ASAN=1")
endif()

if(OLDUNREAL_USE_TSAN)
  add_compile_options("-fsanitize=thread")
  link_libraries("-fsanitize=thread")

  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie")
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
      PROPERTIES
	  COMPILE_DEFINITIONS "GPackage=LocalPackageName${_PACKAGE}"
      PREFIX ""	  
    )
  endif()

  # /Zp4
  # use 4-byte alignment for structs. This is necessary for binary compatibility
  #
  if(WINDOWS AND OLDUNREAL_X86)
	target_compile_options(${_PACKAGE} PRIVATE "$<IF:$<BOOL:$<TARGET_PROPERTY:needs_natural_alignment>>,,/Zp4>")
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
include_directories(Editor/Inc)
include_directories(Editor/Src)
include_directories(Fire/Inc)
include_directories(IpDrv/Inc)
include_directories(NullNetDriver/Inc)
include_directories(Audio/Inc)
include_directories(Render/Inc)
include_directories(${XOPENGLDRV_DIR}/Src)
include_directories(${XOPENGLDRV_DIR}/Inc)
include_directories(UWeb/Inc)
include_directories(Engine/Src)
include_directories(ALAudio/Inc)
include_directories(Cluster/Src)
include_directories(udemo/inc)
if(OLDUNREAL_BUILD_FRUCORE)
  include_directories(FruitCompanyRenderer/Inc)
endif()

if(WINDOWS)
  include_directories(Window/Inc)
  include_directories(WinDrv/Inc)
  include_directories(WinDrv/Src)
  include_directories(WinDrv/Src/Res)
  include_directories(D3DDrv/Src)
  include_directories(Launch/Inc)
  include_directories(Launch/Src/Res)
  include_directories(Setup/Inc)
  include_directories(Setup/Src/Res)
  include_directories(UnrealEd/Inc)
  include_directories(UnrealEd/Src/Res)

  if(NOT OLDUNREAL_AMD64)
    include_directories(Galaxy/Inc)
    include_directories(SoftDrv/Src)
  else()
    include_directories(SoftDrv64/Src)
  endif()
  
  if (OLDUNREAL_BUILD_ICBINDX11DRV)
    include_directories(${ICBINDX11DRV_DIR}/Inc)
  endif()
else()
  include_directories(${SDLLAUNCH_DIR}/Inc)
  include_directories(Window/Src/Res)
  include_directories(${SDLDRV_DIR}/Inc)
  include_directories(ALAudio/Inc)
endif()

################################################################################
# Target Definitions
################################################################################

file(GLOB CORE_HEADERS Core/Inc/*.h)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/UnObj.cpp)
  set(CORE_SOURCES
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
    Core/Src/UnCompat.cpp
    Core/Src/UnCorSc.cpp
    Core/Src/UnCoreNative.cpp
    Core/Src/UnCoreNet.cpp
    Core/Src/UnGUID.cpp
    Core/Src/UnLinker.cpp
    Core/Src/UnLocale.cpp
    Core/Src/UnMath.cpp
    Core/Src/UnMem.cpp
    Core/Src/UnMD5.cpp
    Core/Src/UnMisc.cpp
    Core/Src/UnName.cpp
    Core/Src/UnProp.cpp
    Core/Src/UnUnix.cpp
    Core/Src/UnVcWin32.cpp
    Core/Src/UnThreadingWindows.cpp
    Core/Src/UnThreadingUnix.cpp
    Core/Src/UnixWPrintf.c
    Core/Src/UnX86.cpp
    Core/Src/divsufsort.cpp
  )
  if(WANT_MIMALLOC)
    list(APPEND CORE_SOURCES External/mimalloc/src/static.c)
  endif()
  add_library(
    Core
    ${OLDUNREAL_PACKAGE_LINKAGE}
    ${CORE_SOURCES}
    ${CORE_HEADERS}
  )
  oldunreal_set_package_properties(Core)
  if(WANT_MIMALLOC)
    target_include_directories(Core PRIVATE External/mimalloc/include)
  endif()
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

file(GLOB IPDRV_HEADERS IpDrv/Inc/*.h)
file(GLOB IPDRV_SOURCES IpDrv/Src/*.cpp)
if(IPDRV_SOURCES)
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

file(GLOB NULLNETDRIVER_HEADERS NullNetDriver/Inc/*.h)
file(GLOB NULLNETDRIVER_SOURCES NullNetDriver/Src/*.cpp)
if(NULLNETDRIVER_SOURCES)
  add_library(
    NullNetDriver 
    ${OLDUNREAL_PACKAGE_LINKAGE} 
    ${NULLNETDRIVER_SOURCES}
    ${NULLNETDRIVER_HEADERS}
  )
  oldunreal_set_package_properties(NullNetDriver)
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

file(GLOB UWEB_HEADERS UWeb/Inc/*.h)
file(GLOB UWEB_SOURCES UWeb/Src/*.cpp)
if(UWEB_SOURCES)
  add_library(
    UWeb 
    ${OLDUNREAL_PACKAGE_LINKAGE} 
    ${UWEB_SOURCES}
    ${UWEB_HEADERS}
  )
  oldunreal_set_package_properties(UWeb)
else()
  oldunreal_import_core_library(UWeb)
endif()

if (OLDUNREAL_BUILD_OPENGLDRV)
  file(GLOB OPENGLDRV_HEADERS ${OPENGLDRV_DIR}/Inc/*.h)
  file(GLOB OPENGLDRV_SOURCES ${OPENGLDRV_DIR}/Src/*.cpp )
  
  add_library(
    OpenGLDrv
    ${OLDUNREAL_PACKAGE_LINKAGE}
    ${OPENGLDRV_SOURCES}
    ${OPENGLDRV_HEADERS}
  )
  oldunreal_set_package_properties(OpenGLDrv)
  target_include_directories(OpenGLDrv PRIVATE ${OPENGLDRV_DIR}/Inc)
endif()

file(GLOB XOPENGLDRV_HEADERS ${XOPENGLDRV_DIR}/Inc/*.h)
file(GLOB XOPENGLDRV_SOURCES ${XOPENGLDRV_DIR}/Src/*.cpp)
add_library(
  XOpenGLDrv 
  ${OLDUNREAL_PACKAGE_LINKAGE}
  ${XOPENGLDRV_SOURCES}
  ${XOPENGLDRV_HEADERS}
)
oldunreal_set_package_properties(XOpenGLDrv)
target_include_directories(XOpenGLDrv PRIVATE ${XOPENGLDRV_DIR}/Inc/)

file(GLOB ALAUDIO_HEADERS ALAudio/Inc/*.h)
file(GLOB_RECURSE ALAUDIO_SOURCES ALAudio/Src/*.cpp)
add_library(
  ALAudio
  ${OLDUNREAL_PACKAGE_LINKAGE}
  ${ALAUDIO_SOURCES}
  ${ALAUDIO_HEADERS}
)
oldunreal_set_package_properties(ALAudio)

file(GLOB CLUSTER_HEADERS Cluster/Inc/*.h)
file(GLOB CLUSTER_SOURCES Cluster/Src/*.cpp)
if(CLUSTER_SOURCES)
  add_library(
    Cluster
    ${OLDUNREAL_PACKAGE_LINKAGE}
    ${CLUSTER_SOURCES}
    ${CLUSTER_HEADERS}
  )
  oldunreal_set_package_properties(Cluster)
  target_include_directories(Cluster PRIVATE External/fmod/inc)
endif()

file(GLOB UDEMO_HEADERS udemo/inc/*.h)
file(GLOB UDEMO_SOURCES udemo/src/*.cpp)
add_library(
  udemo
  ${OLDUNREAL_PACKAGE_LINKAGE}
  ${UDEMO_SOURCES}
  ${UDEMO_HEADERS}
)
oldunreal_set_package_properties(udemo)

if(OLDUNREAL_BUILD_FRUCORE)
  file(GLOB FRUCORE_HEADERS FruitCompanyRenderer/Inc/*.h)
  file(GLOB FRUCORE_SOURCES FruitCompanyRenderer/Src/*.cpp FruitCompanyRenderer/Src/*.m)
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
    set_target_properties(Frucore PROPERTIES CXX_STANDARD 17)
    
  else()
    foreach(FRUCORE_SHADER ${FRUCORE_SHADERS})
      get_filename_component(TMP ${FRUCORE_SHADER} NAME)
      string(REPLACE ".metal" ".air" FRUCORE_SHADER_AIR ${TMP})
      list(APPEND FRUCORE_SHADER_LIB_OBJECTS ${FRUCORE_SHADER_AIR})
      add_custom_command(
	    DEPENDS ${FRUCORE_SHADER} ${FRUCORE_HEADERS}
        OUTPUT ${FRUCORE_SHADER_AIR}
        COMMAND xcrun -sdk macosx metal -std=${OLDUNREAL_METAL_VERSION} -mmacos-version-min=10.9 -I ${CMAKE_CURRENT_SOURCE_DIR}/FruitCompanyRenderDrv/Inc/ -frecord-sources -c ${FRUCORE_SHADER} -o ${FRUCORE_SHADER_AIR}
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
    set_target_properties(Frucore PROPERTIES CXX_STANDARD 17)

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

    file(GLOB D3DDRV_HEADERS D3DDrv/Inc/*.h)
    file(GLOB D3DDRV_SOURCES D3DDrv/Src/*.cpp)
    add_library(
      D3DDrv 
      ${OLDUNREAL_PACKAGE_LINKAGE}
      ${D3DDRV_SOURCES}
      ${D3DDRV_HEADERS}
    )
    oldunreal_set_package_properties(D3DDrv)
    target_include_directories(D3DDrv PRIVATE External/dxsdk/Include)
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
  target_include_directories(D3D9Drv PRIVATE D3D9Drv/Inc)

  if(OLDUNREAL_BUILD_D3D10DRV)
    file(GLOB D3D10DRV_HEADERS D3D10Drv/d3d10drv/*.h)
    file(GLOB D3D10DRV_SOURCES D3D10Drv/d3d10drv/*.cpp)
    add_library(
      D3D10Drv
      ${OLDUNREAL_PACKAGE_LINKAGE}
      ${D3D10DRV_SOURCES}
      ${D3D10DRV_HEADERS}
    )
    oldunreal_set_package_properties(D3D10Drv)
  endif()

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

  if(OLDUNREAL_BUILD_D3D11DRV)
    file(GLOB D3D11DRV_HEADERS ${D3D11DRV_DIR}/*.h)
    file(GLOB D3D11DRV_SOURCES ${D3D11DRV_DIR}/*.cpp)
    add_library(
      ${D3D11DRV_NAME}
      ${OLDUNREAL_PACKAGE_LINKAGE}
      ${D3D11DRV_SOURCES}
      ${D3D11DRV_HEADERS}
    )
    oldunreal_set_package_properties(${D3D11DRV_NAME})
    target_include_directories(${D3D11DRV_NAME} PRIVATE ${D3D11DRV_DIR})
  endif()

  if(OLDUNREAL_BUILD_D3D12DRV)
    file(GLOB D3D12DRV_HEADERS ${D3D12DRV_DIR}/*.h)
    file(GLOB D3D12DRV_SOURCES ${D3D12DRV_DIR}/*.cpp)
    add_library(
      ${D3D12DRV_NAME}
      ${OLDUNREAL_PACKAGE_LINKAGE}
      ${D3D12DRV_SOURCES}
      ${D3D12DRV_HEADERS}
    )
    oldunreal_set_package_properties(${D3D12DRV_NAME})
    target_include_directories(${D3D12DRV_NAME} PRIVATE ${D3D12DRV_DIR})
  endif()

  if(OLDUNREAL_BUILD_VULKANDRV)
    file(GLOB VULKANDRV_HEADERS
      ${VULKANDRV_DIR}/*.h
    )
    file(GLOB VULKANDRV_SOURCES
      ${VULKANDRV_DIR}/*.cpp
      ${VULKANDRV_DIR}/../ZVulkan/src/*.cpp 
      ${VULKANDRV_DIR}/../ZVulkan/src/vk_mem_alloc/*.cpp 
      ${VULKANDRV_DIR}/../ZVulkan/src/volk/*.c 
      ${VULKANDRV_DIR}/../ZVulkan/src/glslang/glslang/MachineIndependent/*.cpp 
      ${VULKANDRV_DIR}/../ZVulkan/src/glslang/glslang/MachineIndependent/preprocessor/*.cpp 
      ${VULKANDRV_DIR}/../ZVulkan/src/glslang/glslang/GenericCodeGen/*.cpp
      ${VULKANDRV_DIR}/../ZVulkan/src/glslang/glslang/ResourceLimits/*.cpp
      ${VULKANDRV_DIR}/../ZVulkan/src/glslang/spirv/*.cpp 
      ${VULKANDRV_DIR}/../ZVulkan/src/glslang/OGLCompilersDLL/*.cpp 
      ${VULKANDRV_DIR}/../ZVulkan/src/glslang/glslang/OSDependent/Windows/ossource.cpp 
    )

    add_library(
      ${VULKANDRV_NAME}
      ${OLDUNREAL_PACKAGE_LINKAGE}
      ${VULKANDRV_SOURCES}
      ${VULKANDRV_HEADERS}
    )
    set_target_properties(
	  ${VULKANDRV_NAME}
	  PROPERTIES
	  needs_natural_alignment 1
    )
    oldunreal_set_package_properties(${VULKANDRV_NAME})
    target_include_directories(${VULKANDRV_NAME} PRIVATE
      ${VULKANDRV_DIR}
      ${VULKANDRV_DIR}/../ZVulkan/src
      ${VULKANDRV_DIR}/../ZVulkan/include
      ${VULKANDRV_DIR}/../ZVulkan/include/zvulkan
    )
    set_target_properties(${VULKANDRV_NAME} PROPERTIES CXX_STANDARD 17)
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
  file(GLOB WINDOW_SOURCES Window/Src/*.cpp Window/Src/Res/*.rc)
  add_library(
    Window
    ${OLDUNREAL_PACKAGE_LINKAGE}
    ${WINDOW_SOURCES}
    ${WINDOW_HEADERS}
  )
  oldunreal_set_package_properties(Window)

  file(GLOB WINDRV_HEADERS WinDrv/Inc/*.h)
  file(GLOB WINDRV_SOURCES WinDrv/Src/*.cpp WinDrv/Src/Res/*.rc )
  add_library(
    WinDrv
    ${OLDUNREAL_PACKAGE_LINKAGE}
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
    file(GLOB UNREALED_SOURCES UnrealEd/Src/*.cpp UnrealEd/Src/Res/*.rc)
    add_executable(
      ${OLDUNREAL_EDITOR_EXE}
      WIN32
      ${UNREALED_SOURCES}
      ${UNREALED_ICONS}
      hidpi.manifest
      ${UNREALED_HEADERS}
    )
    set_target_properties(${OLDUNREAL_EDITOR_EXE} PROPERTIES DEBUG_POSTFIX "")  
  endif()

  if(OLDUNREAL_BUILD_SETUP)
    file(GLOB SETUP_HEADERS Setup/Inc/*.h Setup/Src/Res/*.h)
    file(GLOB SETUP_SOURCES Setup/Src/*.cpp Setup/Src/Res/*.rc)
    add_executable(
      ${OLDUNREAL_SETUP_EXE}
      WIN32
      ${SETUP_SOURCES}
      hidpi.manifest
      ${SETUP_HEADERS}
    )
    set_target_properties(${OLDUNREAL_SETUP_EXE} PROPERTIES DEBUG_POSTFIX "")    
  endif()

else() # Non-Windows targets

  file(GLOB SDLDRV_HEADERS ${SDLDRV_DIR}/Inc/*.h)
  file(GLOB SDLDRV_SOURCES ${SDLDRV_DIR}/Src/*.cpp)
  add_library(
    SDLDrv
    ${OLDUNREAL_PACKAGE_LINKAGE}
    ${SDLDRV_SOURCES}
    ${SDLDRV_HEADERS}
  )
  oldunreal_set_package_properties(SDLDrv)

  if(NOT OLDUNREAL_MAKE_BUNDLE)
    add_executable(
      ${OLDUNREAL_EXE}
      ${SDLLAUNCH_DIR}/Src/SDLLaunch.cpp
    )
    set_target_properties(${OLDUNREAL_EXE} PROPERTIES CXX_STANDARD 17)
	target_include_directories(${OLDUNREAL_EXE} PRIVATE External/fmod/inc)
  else()
    add_executable(
      ${OLDUNREAL_EXE}
      MACOSX_BUNDLE
      ${SDLLAUNCH_DIR}/Src/SDLLaunch.cpp
      ${SDLLAUNCH_DIR}/Src/Unreal2.icns
    )
    set_target_properties(
      ${OLDUNREAL_EXE}
      PROPERTIES
      MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/${SDLLAUNCH_DIR}/Src/Unreal.plist.in
	  CXX_STANDARD 17
    )
	target_include_directories(${OLDUNREAL_EXE} PRIVATE External/fmod/inc)
  endif()
	
  if(OLDUNREAL_BUILD_SERVER)
    add_executable(
      ${OLDUNREAL_SERVER_EXE}
      UCC/Src/UCC.cpp
    )
  endif()

  if (OLDUNREAL_BUILD_WX_LAUNCHER)
    add_executable(
    ${OLDUNREAL_WX_LAUNCHER_EXE}
    wxUnrealTournament/SDLLaunch/Src/SDLLaunch.cpp
  )
  target_compile_definitions(${OLDUNREAL_WX_LAUNCHER_EXE} PRIVATE WX)
  endif()
  

  if(OLDUNREAL_BUILD_VULKANDRV)
    file(GLOB VULKANDRV_HEADERS
      ${VULKANDRV_DIR}/*.h
    )
    file(GLOB VULKANDRV_SOURCES
      ${VULKANDRV_DIR}/*.cpp 
      ${VULKANDRV_DIR}/../ZVulkan/src/*.cpp 
      ${VULKANDRV_DIR}/../ZVulkan/src/vk_mem_alloc/*.cpp 
      ${VULKANDRV_DIR}/../ZVulkan/src/volk/*.c 
      ${VULKANDRV_DIR}/../ZVulkan/src/glslang/glslang/MachineIndependent/*.cpp 
      ${VULKANDRV_DIR}/../ZVulkan/src/glslang/glslang/MachineIndependent/preprocessor/*.cpp 
      ${VULKANDRV_DIR}/../ZVulkan/src/glslang/glslang/GenericCodeGen/*.cpp
      ${VULKANDRV_DIR}/../ZVulkan/src/glslang/glslang/ResourceLimits/*.cpp	  
      ${VULKANDRV_DIR}/../ZVulkan/src/glslang/spirv/*.cpp 
      ${VULKANDRV_DIR}/../ZVulkan/src/glslang/OGLCompilersDLL/*.cpp 
      ${VULKANDRV_DIR}/../ZVulkan/src/glslang/glslang/OSDependent/Unix/*.cpp 
    )

    add_library(
      ${VULKANDRV_NAME}
      ${OLDUNREAL_PACKAGE_LINKAGE}
      ${VULKANDRV_SOURCES}
      ${VULKANDRV_HEADERS}
    )
    oldunreal_set_package_properties(${VULKANDRV_NAME})
    target_include_directories(${VULKANDRV_NAME} PRIVATE
      ${VULKANDRV_DIR}
      ${VULKANDRV_DIR}/../ZVulkan/src
      ${VULKANDRV_DIR}/../ZVulkan/include
      ${VULKANDRV_DIR}/../ZVulkan/include/zvulkan
    )
    set_target_properties(${VULKANDRV_NAME} PROPERTIES CXX_STANDARD 17)
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
get_target_property(UWEB_IMPORTED UWeb IMPORTED)

if(NOT ENGINE_IMPORTED)
  target_link_libraries(
    Engine
    ${PNG_LIBRARY}
    ${ZLIB_LIBRARY}
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

target_link_libraries(
  ALAudio
  ${SDL_LIBRARIES}
  ${OPENAL_LIBRARY}
  ${XMP_LIBRARY}
  ${VORBISFILE_LIBRARY}
  ${VORBIS_LIBRARY}
  ${OGG_LIBRARY}
  ${FLAC_LIBRARY}
  ${MPG123_LIBRARY}
  ${OPUS_LIBRARY}
  ${SNDFILE_LIBRARY}
)

if(TARGET Cluster)
  target_link_libraries(
    Cluster
    ${FMOD_LINK_LIBRARY}
  )
endif()

if(OLDUNREAL_BUILD_VULKANDRV)
  target_link_libraries(VulkanDrv Render)
endif()

if(OLDUNREAL_STATIC_LINK)
  target_link_libraries(
    ${OLDUNREAL_EXE}
    Editor
    IpDrv
    Fire
    Render
    UWeb
    XOpenGLDrv
    ALAudio
    Cluster
    udemo
    ${VORBISFILE_LIBRARY}
    ${XMP_LIBRARY}
    ${OPENAL_LIBRARY}
  )
  target_include_directories(${OLDUNREAL_EXE} PRIVATE External/fmod/inc)

  if (OLDUNREAL_BUILD_OPENGLDRV)
    target_link_libraries(${OLDUNREAL_EXE} OpenGLDrv)
  endif()

  if(OLDUNREAL_BUILD_WX_LAUNCHER)
    target_link_libraries(
      ${OLDUNREAL_WX_LAUNCHER_EXE}
      Editor
      IpDrv
      Fire
      Render
      UWeb
      XOpenGLDrv
      ALAudio
      Cluster
      udemo
      ${VORBISFILE_LIBRARY}
      ${XMP_LIBRARY}
      ${OPENAL_LIBRARY}
    )

    if (OLDUNREAL_BUILD_OPENGLDRV)
      target_link_libraries(${OLDUNREAL_WX_LAUNCHER_EXE} OpenGLDrv)
    endif()
  endif()

  if(OLDUNREAL_BUILD_SERVER)
    target_link_libraries(
      ${OLDUNREAL_SERVER_EXE}
      Editor
      Fire
      UWeb
      IpDrv
      NullNetDriver
      Render
    )
	if(OLDUNREAL_STATIC_LINK)
	  target_link_libraries(
	    ${OLDUNREAL_SERVER_EXE}
		udemo
	  )
	endif()
  endif()

  if(OLDUNREAL_BUILD_EDITOR)
    target_link_libraries(
      ${OLDUNREAL_EDITOR_EXE}
      IpDrv
      Fire
      Render
      UWeb
      OpenGLDrv
      XOpenGLDrv
      ALAudio
      Cluster
      udemo
  )
  endif()
      
endif()

target_link_libraries(
  ${OLDUNREAL_EXE}
  Engine
  Core
)

if (OLDUNREAL_BUILD_WX_LAUNCHER)
  target_link_libraries(
    ${OLDUNREAL_WX_LAUNCHER_EXE}
    Engine
    Core
    ${wxWidgets_LIBRARIES}
  )
endif()

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
  # We don't have RDTSC on ARM so we use SDL to get timestamps
  if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/UnObj.cpp AND (OLDUNREAL_ARM OR OLDUNREAL_ARM64))
    target_link_libraries(Core ${SDL_LIBRARIES})
  endif()

  target_link_libraries(
    SDLDrv
    ${SDL_LIBRARIES}
    ${SDL_TTF_LIBRARIES}
	${PNG_LIBRARY}
    ${ZLIB_LIBRARY}
  )

  target_link_libraries(
    ${OLDUNREAL_EXE}
    SDLDrv
    ${SDL_LIBRARIES}
    ${SDL_TTF_LIBRARIES}
  )
  
  if(OLDUNREAL_BUILD_WX_LAUNCHER)
    target_link_libraries(
      ${OLDUNREAL_WX_LAUNCHER_EXE}
      SDLDrv
      ${SDL_LIBRARIES}
      ${SDL_TTF_LIBRARIES}
    )
  endif()

  if ((OLDUNREAL_ARM OR OLDUNREAL_ARM64) AND OLDUNREAL_BUILD_SERVER)
    target_link_libraries(${OLDUNREAL_SERVER_EXE} ${SDL_LIBRARIES})
  endif()
  
endif()


################################################################################
# Platform-specific Linking Rules
################################################################################

if(EMSCRIPTEN)
  set(SDLDIR "${CMAKE_CURRENT_SOURCE_DIR}/${SDL_NAME}")

  macro(emscripten_build _UT99EXE _GLDRV _DEFXOPENGL _LINKFLAGS)
    add_executable(${_UT99EXE} ${SDLLAUNCH_DIR}/Src/SDLLaunch.cpp)
    target_link_libraries(
      ${_UT99EXE}
      Editor
      IpDrv
      Fire
      Render
      UWeb
      SDLDrv
      ${_GLDRV}
      ALAudio
      Engine
      Core
    )
    set_target_properties(
      ${_UT99EXE} PROPERTIES COMPILE_DEFINITIONS
                             FORCE_XOPENGLDRV=${_DEFXOPENGL}
    )
    target_link_libraries(${_UT99EXE} "${SDLDIR}/lib/emscripten/lib${SDL_NAME}.a")
    if(${_DEFXOPENGL} STREQUAL "0")
      target_link_libraries(
        ${_UT99EXE}
        "${CMAKE_CURRENT_SOURCE_DIR}/regal/lib/emscripten/libRegallib.a"
      )
      target_link_libraries(
        ${_UT99EXE}
        "${CMAKE_CURRENT_SOURCE_DIR}/regal/lib/emscripten/libglslopt.a"
      )
    endif()
    set_target_properties(
      ${_UT99EXE}
      PROPERTIES LINK_FLAGS
                 "${_LINKFLAGS} -s TOTAL_MEMORY=134217728 -s ASSERTIONS=0"
    )
  endmacro()

  emscripten_build(
    "index-wasm-opengldrv" "OpenGLDrv" "0"
    "-s WASM=1 -s LEGACY_GL_EMULATION=1 -s GL_UNSAFE_OPTS=0"
  )
  emscripten_build(
    "index-wasm-xopengldrv" "XOpenGLDrv" "1"
    "-s WASM=1 -s FULL_ES3=1 -s USE_WEBGL2=1"
  )
  emscripten_build(
    "index-asmjs-opengldrv" "OpenGLDrv" "0"
    "-s LEGACY_GL_EMULATION=1 -s GL_UNSAFE_OPTS=0"
  )
  emscripten_build(
    "index-asmjs-xopengldrv" "XOpenGLDrv" "1" "-s FULL_ES3=1 -s USE_WEBGL2=1"
  )
  
elseif(WINDOWS)
  if(NOT ENGINE_IMPORTED)
    target_link_libraries(Engine ws2_32)
  endif()
  if(NOT CORE_IMPORTED)
    target_link_libraries(Core PRIVATE winmm)
  endif()
  if(NOT IPDRV_IMPORTED)
    target_link_libraries(IpDrv ws2_32)
  endif()
  target_link_libraries(Window comctl32)
  if(NOT EDITOR_IMPORTED)
    target_link_libraries(Editor Window)
  endif()
  if(TARGET SoftDrv)
    target_link_libraries(SoftDrv Render)
  endif()
  target_link_libraries(D3D9Drv
    ${CMAKE_CURRENT_SOURCE_DIR}/External/dxsdk/Lib/${OLDUNREAL_CPU}/d3dx9.lib
	winmm
  )
  if(OLDUNREAL_BUILD_D3D10DRV)
    target_link_libraries(D3D10Drv
      ${CMAKE_CURRENT_SOURCE_DIR}/External/dxsdk/Lib/${OLDUNREAL_CPU}/d3d10.lib
      ${CMAKE_CURRENT_SOURCE_DIR}/External/dxsdk/Lib/${OLDUNREAL_CPU}/d3dx10.lib
      ${CMAKE_CURRENT_SOURCE_DIR}/External/dxsdk/Lib/${OLDUNREAL_CPU}/dxgi.lib      
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
  if(OLDUNREAL_BUILD_D3D11DRV)
    target_link_libraries(${D3D11DRV_NAME}
      Render
      d3d11.lib
      d3dcompiler.lib
    )  
  endif()
  target_link_libraries(WinDrv
	Window
	${CMAKE_CURRENT_SOURCE_DIR}/External/dxsdk/Lib/${OLDUNREAL_CPU}/dxguid.lib
	imm32
	dinput8
	winmm
  )

  if(NOT OLDUNREAL_AMD64)
    target_link_libraries(D3DDrv
	  ${CMAKE_CURRENT_SOURCE_DIR}/External/dxsdk/Lib/${OLDUNREAL_CPU}/dxguid7.lib
    )
	
	if(TARGET Galaxy)
      target_link_libraries(
        Galaxy
        ${CMAKE_CURRENT_SOURCE_DIR}/External/dxsdk/Lib/${OLDUNREAL_CPU}/dxguid.lib
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
  
  if(TARGET Cluster)
    target_link_libraries(
      Cluster
      ${CMAKE_CURRENT_SOURCE_DIR}/External/dxsdk/Lib/${OLDUNREAL_CPU}/dxguid.lib
      winmm
    )
  endif()
  target_link_libraries(
    ALAudio
    winmm
  )
  
  if (OLDUNREAL_BUILD_OPENGLDRV)
    target_link_libraries(OpenGLDrv opengl32)
  endif()

  target_link_libraries(XOpenGLDrv opengl32)

  if(OLDUNREAL_STATIC_LINK)
    target_link_libraries(
      ${OLDUNREAL_EXE}
      WinDrv
      SoftDrv
      D3D9Drv
    )

    if(OLDUNREAL_BUILD_D3D10DRV)
      target_link_libraries(
       ${OLDUNREAL_EXE}
       D3D10Drv
      )
    endif()

    if(OLDUNREAL_BUILD_ICBINDX11DRV)
      target_link_libraries(
       ${OLDUNREAL_EXE}
       ${ICBINDX11DRV_NAME}
      )
    endif()

    if(OLDUNREAL_BUILD_D3D11DRV)
	  target_link_libraries(
		${OLDUNREAL_EXE}
		${D3D11DRV_NAME}
	  )
    endif()

    if(OLDUNREAL_BUILD_D3D12DRV)
	  target_link_libraries(
		${OLDUNREAL_EXE}
		${D3D12DRV_NAME}
	  )
    endif()

    if(OLDUNREAL_BUILD_VULKANDRV)
      target_link_libraries(
        ${OLDUNREAL_EXE}
        ${VULKANDRV_NAME}
      )
    endif()


    if(NOT OLDUNREAL_AMD64)
      target_link_libraries(${OLDUNREAL_EXE} Galaxy D3DDrv)
    endif()

    if(OLDUNREAL_BUILD_EDITOR)
      target_link_libraries(
        ${OLDUNREAL_EDITOR_EXE}
        WinDrv
        SoftDrv
        D3D9Drv
      )

      if(OLDUNREAL_BUILD_D3D10DRV)
        target_link_libraries(
         ${OLDUNREAL_EDITOR_EXE}
         D3D10Drv
        )
      endif()

      if(OLDUNREAL_BUILD_ICBINDX11DRV)
        target_link_libraries(
         ${OLDUNREAL_EDITOR_EXE}
         ${ICBINDX11DRV_NAME}
        )
      endif()

	  if(OLDUNREAL_BUILD_D3D11DRV)
        target_link_libraries(
		  ${OLDUNREAL_EDITOR_EXE}
		  ${D3D11DRV_NAME}
	    )
      endif()

	  if(OLDUNREAL_BUILD_D3D12DRV)
        target_link_libraries(
		  ${OLDUNREAL_EDITOR_EXE}
		  ${D3D12DRV_NAME}
	    )
      endif()

      if(OLDUNREAL_BUILD_VULKANDRV)
        target_link_libraries(
          ${OLDUNREAL_EDITOR_EXE}
          ${OLDUNREAL_VULKANDRV_NAME}
        )
      endif()

      if(NOT OLDUNREAL_AMD64)
        target_link_libraries(${OLDUNREAL_EDITOR_EXE} Galaxy D3DDrv)
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

  if(OLDUNREAL_BUILD_WX_LAUNCHER)
    set_target_properties(
      ${OLDUNREAL_WX_LAUNCHER_EXE}
      PROPERTIES
      LINK_FLAGS "-Wl,-rpath,$ORIGIN"
    )
    target_link_libraries(
      ${OLDUNREAL_WX_LAUNCHER_EXE}
      dl
      pthread
    )
  endif()

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
    ${SDL_LIBRARIES}
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
    udemo
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

  if(NOT UWEB_IMPORTED)
    install(TARGETS UWeb DESTINATION ./)
  endif()  
  
  if(TARGET Cluster)
    install(TARGETS Cluster DESTINATION ./)
  endif()

  if (OLDUNREAL_BUILD_FRUCORE)
    install(TARGETS Frucore DESTINATION ./)
  endif()
	
  if (OLDUNREAL_BUILD_OPENGLDRV)
    install(TARGETS OpenGLDrv DESTINATION ./)
  endif()
  

  if (OLDUNREAL_BUILD_VULKANDRV)
    install(TARGETS VulkanDrv DESTINATION ./)
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

if (OLDUNREAL_BUILD_WX_LAUNCHER)
  install(
    TARGETS
    ${OLDUNREAL_WX_LAUNCHER_EXE}
    DESTINATION ./
  )
endif()


################################################################################
# Platform-specific Installation Rules
################################################################################

if(WINDOWS)
  if(NOT OLDUNREAL_STATIC_LINK AND NOT OLDUNREAL_AMD64 AND TARGET Galaxy)
    install(TARGETS Galaxy D3DDrv DESTINATION ./)
  endif()
  
  if(TARGET SoftDrv)
    install(TARGETS SoftDrv DESTINATION ./)
  endif()

  install(
    TARGETS
    Window
    WinDrv    
    D3D9Drv
    DESTINATION ./
  )

  if(OLDUNREAL_BUILD_D3D10DRV)
    install(
      TARGETS
      D3D10Drv
      DESTINATION ./
    )

    file(GLOB D3D10DRV_SHADERS D3D10Drv/d3d10drv/*.fx D3D10Drv/d3d10drv/*.fxh)
    install(
      FILES
      ${D3D10DRV_SHADERS}
      DESTINATION ./d3d10drv/
    )      
  endif()

  if(OLDUNREAL_BUILD_ICBINDX11DRV)
    install(
      TARGETS
      ${ICBINDX11DRV_NAME}
      DESTINATION ./
    )
  endif()

  if(OLDUNREAL_BUILD_D3D11DRV)
    install(
      TARGETS
      ${D3D11DRV_NAME}
      DESTINATION ./
    )
  endif()

  if(OLDUNREAL_BUILD_D3D12DRV)
    install(
      TARGETS
      ${D3D12DRV_NAME}
      DESTINATION ./
    )
  endif()

  if(OLDUNREAL_BUILD_VULKANDRV)
    install(
      TARGETS
      ${VULKANDRV_NAME}
      DESTINATION ./
    )
  endif()

  
  foreach(LIB
    ${XMP_LIBRARY_RELEASE}
    ${OPENAL_LIBRARY_RELEASE}
    ${MPG123_LIBRARY_RELEASE}
    ${SNDFILE_LIBRARY_RELEASE}
  )    	
    string(REPLACE ".lib" ".dll" TMP ${LIB})
    string(REPLACE "lib/" "bin/" OUT ${TMP})
    install(FILES ${OUT} DESTINATION ./)
  endforeach()
  
  install(
    FILES
    ${OPENAL_INSTALL_LIBRARY}
    ${AKELEDIT_LIBRARY}
	${BRIGHT_INSTALL_BINARY}
	${MAGICK_INSTALL_BINARY}
	${MAGICK_INSTALL_NOTICE}
    DESTINATION ./
  )
  
elseif(LINUX)
  if(NOT OLDUNREAL_STATIC_LINK)
    install(
      TARGETS
      SDLDrv
      DESTINATION ./
    )
  endif()

  foreach(LIB
  	"${XMP_LIBRARY};libxmp.so.4"
	"${OPENAL_LIBRARY};libopenal.so.1"
#	"${SDL_LIBRARIES};${SDL_SO_NAME}"
#   "${SDL_TTF_LIBRARIES};${SDL_TTF_SO_NAME}"
	"${MPG123_LIBRARY};libmpg123.so"
	"${SNDFILE_LIBRARY};libsndfile.so.1"
	"${FMOD_LINK_LIBRARY};libfmod.so.13"
  )
    list(GET LIB -2 ORIG_NAME)
    list(GET LIB -1 NEW_NAME)

    get_library_path(ORIG_NAME ABSOLUTE_PATH)

	if(EXISTS ${ABSOLUTE_PATH})
      install(
        FILES ${ABSOLUTE_PATH}
	    RENAME ${NEW_NAME}
        DESTINATION ./
	  )
    endif()
  endforeach()
  
elseif(APPLE AND OLDUNREAL_MAKE_BUNDLE)

  set(MACOSX_BUNDLE_ICON_FILE Unreal2.icns)
  set(MACOSX_BUNDLE_BUNDLE_VERSION ${OLDUNREAL_VERSION})
  set(MACOSX_BUNDLE_SHORT_VERSION_STRING ${OLDUNREAL_VERSION})
  set(MACOSX_BUNDLE_LONG_VERSION_STRING ${OLDUNREAL_VERSION})  
  set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.oldunreal.unrealtournament")
  set(MACOSX_BUNDLE_INFO_STRING "Unreal Tournament - version ${OLDUNREAL_VERSION}")
  set(MACOSX_BUNDLE_BUNDLE_NAME "Unreal Tournament")
  set(MACOSX_BUNDLE_COPYRIGHT "Epic Games")

  # Copies the latest .u and .int files into the app bundle template
  add_custom_command(TARGET ${OLDUNREAL_EXE}
    PRE_BUILD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ./scripts/mac-update-template.sh
  )

  set_source_files_properties(
    ${SDLLAUNCH_DIR}/Src/Unreal2.icns
    PROPERTIES
    MACOSX_PACKAGE_LOCATION
    Resources
  )
	  
  set(APPS "${CMAKE_INSTALL_PREFIX}/${OLDUNREAL_EXE}.app")
  set(DIRS "${CMAKE_INSTALL_PREFIX}/")
  set(LIBS
    "${CMAKE_INSTALL_PREFIX}/${OLDUNREAL_EXE}.app/Contents/Frameworks/libopenal.1.dylib"
    "${CMAKE_INSTALL_PREFIX}/${OLDUNREAL_EXE}.app/Contents/Frameworks/libxmp.4.dylib"
    "${CMAKE_INSTALL_PREFIX}/${OLDUNREAL_EXE}.app/Contents/Frameworks/libfmod.dylib"
    "${CMAKE_INSTALL_PREFIX}/${OLDUNREAL_EXE}.app/Contents/Frameworks/libmpg123.dylib"
    "${CMAKE_INSTALL_PREFIX}/${OLDUNREAL_EXE}.app/Contents/Frameworks/libsndfile.1.dylib"
    "${CMAKE_INSTALL_PREFIX}/${OLDUNREAL_EXE}.app/Contents/Frameworks/${SDL_DYLIB_NAME}"
  )

  if(NOT "${SDL_TTF_DYLIB_NAME}" STREQUAL "")
    list(APPEND LIBS "${CMAKE_INSTALL_PREFIX}/${OLDUNREAL_EXE}.app/Contents/Frameworks/${SDL_TTF_DYLIB_NAME}")
  endif()
  
  install(
    TARGETS ${OLDUNREAL_EXE}
    BUNDLE DESTINATION . COMPONENT Runtime
    RUNTIME DESTINATION bin COMPONENT Runtime
  )

  # Copy UCC into the bundle
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
    "Web"
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
    "${SDL_LIBRARIES};${SDL_DYLIB_NAME}"
	"${SDL_TTF_LIBRARIES};${SDL_TTF_DYLIB_NAME}"
    "${FMOD_INSTALL_LIBRARY};libfmod.dylib"
  )
    # Ugh. SDL has two libs in the list
    list(GET LIB -2 ORIG_NAME)
    list(GET LIB -1 NEW_NAME)

#    if(NOT "${ORIG_NAME}" STREQUAL "" AND NOT "${NEW_NAME}" STREQUAL "")
      get_library_path(ORIG_NAME ABSOLUTE_PATH)

  	  message(STATUS "Copying lib ${ABSOLUTE_PATH} ${NEW_NAME}")
      install(
        FILES ${ABSOLUTE_PATH}
        RENAME ${NEW_NAME}
        DESTINATION ${OLDUNREAL_EXE}.app/Contents/Frameworks
      )
#	endif()
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
      SDLDrv
      DESTINATION ./
    )
  endif()

  get_library_path(SDL_LIBRARIES SDL_INSTALL_LIBRARY)
  get_library_path(SDL_TTF_LIBRARIES SDL_TTF_INSTALL_LIBRARY)
  
  install(
    FILES
    ${XMP_LIBRARY}
    ${OPENAL_LIBRARY}
    ${FMOD_INSTALL_LIBRARY}	
    ${SDL_INSTALL_LIBRARY}
	${SDL_TTF_INSTALL_LIBRARY}
    ${MPG123_LIBRARY}
    ${SNDFILE_LIBRARY}
    DESTINATION ./
  )

endif()

# end of CMakeLists.txt ...
