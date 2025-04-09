# Ugly hack to make find_package/find_library find static libs first
if(APPLE)
  set(CMAKE_FIND_LIBRARY_SUFFIXES .a .dylib)
  set(CMAKE_FIND_FRAMEWORK LAST)
  set(SHARED_LIB_EXT .dylib)
elseif(UNIX)
  set(CMAKE_FIND_LIBRARY_SUFFIXES .a .so)
  set(SHARED_LIB_EXT .so)
else()
  set(CMAKE_FIND_LIBRARY_SUFFIXES .lib .dll)
  set(SHARED_LIB_EXT .dll)
endif()

# SDL2/SDL2_ttf
if(UNIX)
  if(WANT_SDL2)
    set(SDL_SO_NAME "libSDL2-2.0.so.0")
	set(SDL_TTF_SO_NAME "")
	set(SDL_DYLIB_NAME "libSDL2-2.0.0.dylib")
	set(SDL_TTF_DYLIB_NAME "")
    find_package(SDL2)
    if(NOT SDL2_FOUND)
      find_path(SDL2_INCLUDE_DIRS SDL.h
	    REQUIRED
	    HINTS
	    PATH_SUFFIXES include include/SDL2
      )
      find_library(SDL2_LIBRARY
        REQUIRED
   	    NAMES SDL2
   	    HINTS
   	    PATH_SUFFIXES lib
      )
	  set(SDL_LIBRARIES ${SDL2_LIBRARY})
	else()
	  set(SDL_LIBRARIES ${SDL2_LIBRARIES})
    endif()
    message(STATUS "Found SDL headers: ${SDL2_INCLUDE_DIRS}")
    include_directories(${SDL2_INCLUDE_DIRS})
    find_path(SDL2_TTF_INCLUDE_DIR SDL_ttf.h
	  REQUIRED
	  HINTS
	  PATH_SUFFIXES include include/SDL2
    )
    find_library(SDL2_TTF_LIBRARY
      REQUIRED
   	  NAMES SDL2_ttf
   	  HINTS
   	  PATH_SUFFIXES lib
    )
    message(STATUS "Found SDL_TTF headers: ${SDL2_TTF_INCLUDE_DIR}")  
    include_directories(${SDL2_TTF_INCLUDE_DIR})
    find_library(FREETYPE_LIBRARY
      REQUIRED
   	  NAMES libfreetype freetype
   	  HINTS
   	  PATH_SUFFIXES lib
    )
    set(FREETYPE_LIBRARIES "${FREETYPE_LIBRARY}")
	set(SDL_TTF_LIBRARIES "${SDL2_TTF_LIBRARY};${FREETYPE_LIBRARIES}")
  else()
    set(SDL_SO_NAME "libSDL3.so.0")
	set(SDL_TTF_SO_NAME "")
	set(SDL_DYLIB_NAME "libSDL3.0.dylib")
	set(SDL_TTF_DYLIB_NAME "libSDL3_ttf.0.dylib")
    find_package(SDL3)
	set(SDL_LIBRARIES "${SDL3_LIBRARIES}")
    find_package(SDL3_ttf)
    set(SDL_TTF_LIBRARIES "SDL3_ttf::SDL3_ttf")
  endif()

  message(STATUS "Found SDL library: ${SDL_LIBRARIES}")
  message(STATUS "Found SDL_TTF library: ${SDL_TTF_LIBRARIES}")
endif()

# zlib
find_package(ZLIB REQUIRED)
message(STATUS "Found zlib headers: ${ZLIB_INCLUDE_DIR}")
message(STATUS "Found zlib library: ${ZLIB_LIBRARY}")
include_directories(${ZLIB_INCLUDE_DIR})

# libpng
find_package(PNG REQUIRED)
message(STATUS "Found libpng headers: ${PNG_INCLUDE_DIR}")
message(STATUS "Found libpng library: ${PNG_LIBRARY}")
include_directories(${PNG_INCLUDE_DIR})

# mpg123
find_path(MPG123_INCLUDE_DIRS mpg123.h
  REQUIRED
  HINTS
  PATH_SUFFIXES include
)
find_library(MPG123_LIBRARY_RELEASE
  REQUIRED
  NAMES mpg123
  HINTS
  PATH_SUFFIXES lib
)
find_library(MPG123_LIBRARY_DEBUG
  NAMES "mpg123${CMAKE_DEBUG_POSTFIX}"
  HINTS
  PATH_SUFFIXES lib
)
select_library_configurations(MPG123)
message(STATUS "Found libmpg123 headers: ${MPG123_INCLUDE_DIRS}")
message(STATUS "Found libmpg123 library: ${MPG123_LIBRARIES}")
include_directories(${MPG123_INCLUDE_DIRS})

# sndfile
find_path(SNDFILE_INCLUDE_DIRS sndfile.h
  REQUIRED
  HINTS
  PATH_SUFFIXES include
)
find_library(SNDFILE_LIBRARY_RELEASE
  REQUIRED
  NAMES sndfile
  HINTS
  PATH_SUFFIXES lib
)
find_library(SNDFILE_LIBRARY_DEBUG
  NAMES "sndfile${CMAKE_DEBUG_POSTFIX}"
  HINTS
  PATH_SUFFIXES lib
)
select_library_configurations(SNDFILE)
message(STATUS "Found libsndfile headers: ${SNDFILE_INCLUDE_DIRS}")
message(STATUS "Found libsndfile library: ${SNDFILE_LIBRARIES}")
include_directories(${SNDFILE_INCLUDE_DIRS})

# libogg
find_package(Ogg REQUIRED)
message(STATUS "Found libogg headers: ${OGG_INCLUDE_DIRS}")
message(STATUS "Found libogg library: ${OGG_LIBRARIES}")
include_directories(${OGG_INCLUDE_DIRS})

# libvorbis
find_path(VORBIS_INCLUDE_DIR vorbisfile.h
  REQUIRED
  HINTS
  PATH_SUFFIXES include include/vorbis
)
find_library(VORBIS_LIBRARY_RELEASE
  REQUIRED
  NAMES vorbis
  HINTS
  PATH_SUFFIXES lib
)
find_library(VORBIS_LIBRARY_DEBUG
  NAMES "vorbis${CMAKE_DEBUG_POSTFIX}"
  HINTS
  PATH_SUFFIXES lib
)
select_library_configurations(VORBIS)

find_library(VORBISFILE_LIBRARY_RELEASE
  REQUIRED
  NAMES vorbisfile
  HINTS
  PATH_SUFFIXES lib
)
find_library(VORBISFILE_LIBRARY_DEBUG
  NAMES "vorbisfile${CMAKE_DEBUG_POSTFIX}"
  HINTS
  PATH_SUFFIXES lib
)
select_library_configurations(VORBISFILE)

find_library(VORBISENC_LIBRARY_RELEASE
  REQUIRED
  NAMES vorbisenc
  HINTS
  PATH_SUFFIXES lib
)
find_library(VORBISENC_LIBRARY_DEBUG
  NAMES "vorbisenc${CMAKE_DEBUG_POSTFIX}"
  HINTS
  PATH_SUFFIXES lib
)
select_library_configurations(VORBISENC)
message(STATUS "Found libvorbis headers: ${VORBIS_INCLUDE_DIR}")
message(STATUS "Found libvorbis libraries: ${VORBIS_LIBRARIES} ${VORBISFILE_LIBRARIES} ${VORBISENC_LIBRARIES}")
include_directories(${VORBIS_INCLUDE_DIR})

# opus - we don't use this directly but alure links to it
find_library(OPUS_LIBRARY_RELEASE
  REQUIRED
  NAMES opus
  HINTS
  PATH_SUFFIXES lib
)
find_library(OPUS_LIBRARY_DEBUG
  NAMES "opus${CMAKE_DEBUG_POSTFIX}"
  HINTS
  PATH_SUFFIXES lib
)
select_library_configurations(OPUS)
message(STATUS "Found opus library: ${OPUS_LIBRARY}")

# flac - we don't use this directly but alure links to it
find_library(FLAC_LIBRARY_RELEASE
  REQUIRED
  NAMES FLAC
  HINTS
  PATH_SUFFIXES lib
)
find_library(FLAC_LIBRARY_DEBUG
  NAMES "FLAC${CMAKE_DEBUG_POSTFIX}"
  HINTS
  PATH_SUFFIXES lib
)
select_library_configurations(FLAC)
message(STATUS "Found flac library: ${FLAC_LIBRARY}")

# openal-soft
find_path(OPENAL_INCLUDE_DIRS al.h
  REQUIRED
  HINTS
  PATH_SUFFIXES include include/AL
)
find_library(OPENAL_LIBRARY_RELEASE
  REQUIRED
  NAMES OpenAL32 openal
  HINTS
  PATH_SUFFIXES lib
)
find_library(OPENAL_LIBRARY_DEBUG
  NAMES "OpenAL32${CMAKE_DEBUG_POSTFIX}" "openal${CMAKE_DEBUG_POSTFIX}"
  HINTS
  PATH_SUFFIXES lib
)
select_library_configurations(OPENAL)
message(STATUS "Found openal headers: ${OPENAL_INCLUDE_DIRS}")
message(STATUS "Found openal library: ${OPENAL_LIBRARIES}")
include_directories(${OPENAL_INCLUDE_DIRS})

if(WANT_MIMALLOC)
  find_package(mimalloc REQUIRED)
  message(STATUS "Found mimalloc headers: ${MIMALLOC_INCLUDE_DIR}")
  include_directories(${MIMALLOC_INCLUDE_DIR})
endif()

# galaxy
if(WINDOWS)
  if(NOT OLDUNREAL_AMD64 AND WANT_GALAXY)
    find_path(GALAXY_INCLUDE_DIR Galaxy.h
  	  REQUIRED
  	  HINTS
	  PATH_SUFFIXES include
    )
    find_library(GALAXY_LIBRARY_RELEASE
	  REQUIRED
	  NAMES GalaxyLib
	  HINTS
	  PATH_SUFFIXES lib
    )
    find_library(GALAXY_LIBRARY_DEBUG
	  NAMES "GalaxyLib${CMAKE_DEBUG_POSTFIX}"
	  HINTS
	  PATH_SUFFIXES lib
    )
    select_library_configurations(GALAXY)  
    message(STATUS "Found Galaxy header: ${GALAXY_INCLUDE_DIR}")
    message(STATUS "Found Galaxy library: ${GALAXY_LIBRARY}")
    include_directories(${GALAXY_INCLUDE_DIR})
  endif()

  find_path(AKELEDIT_INCLUDE_DIR AkelEdit.h
	REQUIRED
	HINTS
	PATH_SUFFIXES include
  )
  find_file(AKELEDIT_LIBRARY AkelEdit.dll
	REQUIRED
	NAMES AkelEdit
	HINTS
	PATH_SUFFIXES bin
  )
  message(STATUS "Found AkelEdit header: ${AKELEDIT_INCLUDE_DIR}")
  message(STATUS "Found AkelEdit library: ${AKELEDIT_LIBRARY}")
  include_directories(${AKELEDIT_INCLUDE_DIR})
endif()

# alure
find_path(ALURE_INCLUDE_DIR alure.h
  REQUIRED
  HINTS
  PATH_SUFFIXES include include/AL include/OpenAL
)
find_library(ALURE_LIBRARY_RELEASE
  REQUIRED
  NAMES alure-static ALURE32-STATIC
  HINTS
  PATH_SUFFIXES lib
)
find_library(ALURE_LIBRARY_DEBUG
  NAMES "alure-static${CMAKE_DEBUG_POSTFIX}" "ALURE32-STATIC${CMAKE_DEBUG_POSTFIX}"
  HINTS
  PATH_SUFFIXES lib
)
select_library_configurations(ALURE)
message(STATUS "Found alure headers: ${ALURE_INCLUDE_DIR}")
message(STATUS "Found alure library: ${ALURE_LIBRARIES}")
include_directories(${ALURE_INCLUDE_DIR})

# libxmp
find_path(XMP_INCLUDE_DIR xmp.h
  REQUIRED
  HINTS
  PATH_SUFFIXES include
)
find_library(XMP_LIBRARY_RELEASE
  REQUIRED
  NAMES xmp libxmp
  HINTS
  PATH_SUFFIXES lib
)
find_library(XMP_LIBRARY_DEBUG
  NAMES "xmp${CMAKE_DEBUG_POSTFIX}" "libxmp${CMAKE_DEBUG_POSTFIX}"
  HINTS
  PATH_SUFFIXES lib
)
select_library_configurations(XMP)
message(STATUS "Found xmp headers: ${XMP_INCLUDE_DIR}")
message(STATUS "Found xmp library: ${XMP_LIBRARIES}")
include_directories(${XMP_INCLUDE_DIR})

# ktexcomp
find_path(KTEXCOMP_INCLUDE_DIR BC.h
  HINTS
  PATH_SUFFIXES include
)
find_library(KTEXCOMP_LIBRARY_RELEASE
  NAMES KTexComp
  HINTS
  CONFIG
  PATH_SUFFIXES lib
)
find_library(KTEXCOMP_LIBRARY_DEBUG
  NAMES "KTexComp${CMAKE_DEBUG_POSTFIX}"
  HINTS
  CONFIG
  PATH_SUFFIXES lib
)
if(KTEXCOMP_LIBRARY_RELEASE)
  select_library_configurations(KTEXCOMP)
  message(STATUS "Found KTexComp headers: ${KTEXCOMP_INCLUDE_DIR}")
  message(STATUS "Found KTexComp library: ${KTEXCOMP_LIBRARIES}")
  include_directories(${KTEXCOMP_INCLUDE_DIR})
endif()

if(WINDOWS)
  set(FMOD_INSTALL_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/External/fmod/lib/${OLDUNREAL_CPU}/fmod.dll)
  set(FMOD_LINK_LIBRARY
    ${CMAKE_CURRENT_SOURCE_DIR}/External/fmod/lib/${OLDUNREAL_CPU}/fmod_vc.lib)
  if (OLDUNREAL_AMD64)
	set(FMODEX_INSTALL_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/External/fmodex/lib/${OLDUNREAL_CPU}/fmodex64.dll)
  else()
    set(FMODEX_INSTALL_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/External/fmodex/lib/${OLDUNREAL_CPU}/fmodex.dll)
  endif()
  set(FMODEX_LINK_LIBRARY
    ${CMAKE_CURRENT_SOURCE_DIR}/External/fmodex/lib/${OLDUNREAL_CPU}/fmodex_vc.lib)  
else()
  set(FMOD_INSTALL_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/External/fmod/lib/${OLDUNREAL_CPU}/libfmod${SHARED_LIB_EXT})
  set(FMOD_LINK_LIBRARY ${FMOD_INSTALL_LIBRARY})
  set(FMODEX_INSTALL_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/External/fmodex/lib/${OLDUNREAL_CPU}/libfmod${SHARED_LIB_EXT})
  set(FMODEX_LINK_LIBRARY ${FMODEX_INSTALL_LIBRARY})  
endif()

if(WINDOWS)
  if("${CMAKE_GENERATOR_TOOLSET}" MATCHES "_xp")
    set(MAGICK_INSTALL_BINARY ${CMAKE_CURRENT_SOURCE_DIR}/External/magickxp/magick.exe)
    set(MAGICK_INSTALL_NOTICE ${CMAKE_CURRENT_SOURCE_DIR}/External/magickxp/magick-NOTICE.txt)
  else()
	set(MAGICK_INSTALL_BINARY ${CMAKE_CURRENT_SOURCE_DIR}/External/magick/magick.exe)
	set(MAGICK_INSTALL_NOTICE ${CMAKE_CURRENT_SOURCE_DIR}/External/magick/magick-NOTICE.txt)
  endif()

  find_program(BRIGHT_INSTALL_BINARY Bright)
endif()

# Header-only or precompiled stuff
include_directories(External/glm)
include_directories(External/curl/include)
if(APPLE)
  include_directories(External/metal-cpp)
endif()

# Platform/distro packages and libs
if(LINUX)
  find_package(Threads REQUIRED)
  if (OLDUNREAL_BUILD_WX_LAUNCHER)
    set(wxWidgets_CONFIGURATION mswu)
    find_package(wxWidgets COMPONENTS core base REQUIRED)
    include(${wxWidgets_USE_FILE})
  endif()
elseif(WINDOWS)
  find_package(Threads REQUIRED)
elseif(APPLE)
  find_package(Threads REQUIRED)
  find_library(COCOA_FRAMEWORK Cocoa)
  find_library(METAL_FRAMEWORK Metal)
  find_library(FOUNDATION_FRAMEWORK Foundation)
  find_library(COREGRAPHICS_FRAMEWORK CoreGraphics)
  find_library(METALKIT_FRAMEWORK MetalKit)
endif()

if(WANT_PHYSX)
  set(PhysX_DIR "${CMAKE_PREFIX_PATH}/PhysX/bin/cmake/physx/")
  find_package(PhysX REQUIRED)
endif()

if(WANT_OPENGLIDE)
  find_library(OPENGLIDE_LIBRARY_RELEASE
    REQUIRED
    NAMES glide2x
    HINTS
    PATH_SUFFIXES lib 
  )
  find_library(OPENGLIDE_LIBRARY_DEBUG
    NAMES "glide2x${CMAKE_DEBUG_POSTFIX}"
    HINTS
    PATH_SUFFIXES lib
  )
  select_library_configurations(OPENGLIDE)
  message(STATUS "Found openglide libraries: ${OPENGLIDE_LIBRARIES}")

endif()
