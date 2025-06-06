# CMakeLists.txt for ALURE
CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

IF(COMMAND CMAKE_POLICY)
  if(POLICY CMP0003)
    CMAKE_POLICY(SET CMP0003 NEW)
  endif()
  if(POLICY CMP0091)
    CMAKE_POLICY(SET CMP0091 NEW)
  endif()
ENDIF(COMMAND CMAKE_POLICY)

SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

INCLUDE(CheckCCompilerFlag)
INCLUDE(CheckCSourceCompiles)
INCLUDE(CheckFunctionExists)
INCLUDE(CheckIncludeFile)
INCLUDE(CheckIncludeFiles)
INCLUDE(CheckLibraryExists)
INCLUDE(CheckSharedFunctionExists)
INCLUDE(FindPkgConfig)
INCLUDE(UsePkgConfig)
INCLUDE(CheckFileOffsetBits)

PROJECT(ALURE)

IF(WIN32)
    SET(LIBNAME ALURE32)
ELSE(WIN32)
    SET(LIBNAME alure)
ENDIF(WIN32)

SET(LIB_MAJOR_VERSION "1")
SET(LIB_MINOR_VERSION "2")
SET(LIB_VERSION "${LIB_MAJOR_VERSION}.${LIB_MINOR_VERSION}")


INCLUDE_DIRECTORIES(include "${ALURE_BINARY_DIR}")


OPTION(BUILD_SHARED "Build the shared version of the library" ON)
OPTION(BUILD_STATIC "Build the static version of the library" ON)

IF(NOT BUILD_SHARED AND NOT BUILD_STATIC)
    MESSAGE(FATAL_ERROR "No libtype being built!")
ENDIF(NOT BUILD_SHARED AND NOT BUILD_STATIC)

IF(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
        "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)
IF(NOT DEFINED CMAKE_DEBUG_POSTFIX)
    SET(CMAKE_DEBUG_POSTFIX "" CACHE STRING
        "Library postfix for debug builds. Normally left blank."
        FORCE)
ENDIF(NOT DEFINED CMAKE_DEBUG_POSTFIX)


CHECK_LIBRARY_EXISTS(stdc++ memset "" HAS_STDCXX)
IF(HAS_STDCXX)
    SET(EXTRA_LIBS stdc++ ${EXTRA_LIBS})
ENDIF(HAS_STDCXX)

CHECK_LIBRARY_EXISTS(m pow "" HAS_LIBM)
IF(HAS_LIBM)
    SET(EXTRA_LIBS m ${EXTRA_LIBS})
ENDIF(HAS_LIBM)

CHECK_FILE_OFFSET_BITS()
IF(_FILE_OFFSET_BITS)
    ADD_DEFINITIONS(-D_FILE_OFFSET_BITS=${_FILE_OFFSET_BITS})
    SET(CMAKE_REQUIRED_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS} -D_FILE_OFFSET_BITS=${_FILE_OFFSET_BITS})
ENDIF(_FILE_OFFSET_BITS)

CHECK_FUNCTION_EXISTS(fseeko HAVE_FSEEKO)
IF(NOT HAVE_FSEEKO)
    CHECK_FUNCTION_EXISTS(_fseeki64 HAVE__FSEEKI64)
ENDIF(NOT HAVE_FSEEKO)

SET(STATIC_CFLAGS "${STATIC_CFLAGS} -DALURE_STATIC_LIBRARY")

IF(MSVC)
    # ???
ELSE(MSVC)
    IF(NOT WIN32)
        CHECK_C_SOURCE_COMPILES("int foo() __attribute__((constructor));
                                 int main() {return 0;}" HAVE_GCC_CONSTRUCTOR)

        # Force -fPIC for the static lib, because it causes problems when the
        # static version is linked to a dynamic lib
        SET(STATIC_CFLAGS "${STATIC_CFLAGS} -fPIC")
    ENDIF(NOT WIN32)

    CHECK_C_COMPILER_FLAG(-Wextra HAVE_WEXTRA)
    IF(HAVE_WEXTRA)
        ADD_DEFINITIONS(-Wextra)
    ENDIF(HAVE_WEXTRA)
    ADD_DEFINITIONS(-Wall)

    CHECK_C_COMPILER_FLAG(-funswitch-loops HAVE_FUNSWITCH_LOOPS)
    IF(HAVE_FUNSWITCH_LOOPS)
        ADD_DEFINITIONS(-funswitch-loops)
    ENDIF(HAVE_FUNSWITCH_LOOPS)
ENDIF(MSVC)

# Set visibility/export options if available
SET(EXPORT_DECL "")
IF(WIN32)
    SET(EXPORT_DECL "__declspec(dllexport)")
ELSE(WIN32)
    CHECK_C_COMPILER_FLAG(-fvisibility=hidden HAVE_VISIBILITY_SWITCH)
    CHECK_C_SOURCE_COMPILES("int foo() __attribute__((visibility(\"protected\")));
                             int main() {return 0;}" HAVE_GCC_VISIBILITY)
    IF(HAVE_VISIBILITY_SWITCH AND HAVE_GCC_VISIBILITY)
        ADD_DEFINITIONS(-fvisibility=hidden)
        SET(EXPORT_DECL "__attribute__((visibility(\"protected\")))")
    ENDIF(HAVE_VISIBILITY_SWITCH AND HAVE_GCC_VISIBILITY)
ENDIF(WIN32)


CHECK_INCLUDE_FILE(windows.h HAVE_WINDOWS_H -D_WIN32_WINNT=0x0500)
IF(HAVE_WINDOWS_H)
    CHECK_INCLUDE_FILE(ddk/ntddcdrm.h HAVE_DDK_NTDDCDRM_H)
ELSE(HAVE_WINDOWS_H)
    CHECK_C_COMPILER_FLAG(-pthread HAVE_PTHREAD)
    IF(HAVE_PTHREAD)
        ADD_DEFINITIONS(-pthread)
        SET(EXTRA_LIBS ${EXTRA_LIBS} -pthread)
    ENDIF(HAVE_PTHREAD)

    # We need pthreads outside of Windows
    CHECK_INCLUDE_FILE(pthread.h HAVE_PTHREAD_H)
    IF(NOT HAVE_PTHREAD_H)
        MESSAGE(FATAL_ERROR "PThreads is required for non-Windows builds!")
    ENDIF()
    # Some systems need pthread_np.h to get recursive mutexes
    CHECK_INCLUDE_FILES("pthread.h;pthread_np.h" HAVE_PTHREAD_NP_H)

    # _GNU_SOURCE is needed on some systems for extra attributes, and
    # _REENTRANT is needed for libc thread-safety
    ADD_DEFINITIONS(-D_GNU_SOURCE=1)
    CHECK_FUNCTION_EXISTS(pthread_create HAS_PTHREAD_CREATE)
    IF(NOT HAS_PTHREAD_CREATE)
        CHECK_SHARED_FUNCTION_EXISTS(pthread_create "pthread.h" pthread "" HAVE_LIBPTHREAD)
        IF(NOT HAVE_LIBPTHREAD)
            MESSAGE(FATAL_ERROR "pthread_create not found on non-Windows target!")
        ENDIF(NOT HAVE_LIBPTHREAD)
        SET(EXTRA_LIBS pthread ${EXTRA_LIBS})
    ENDIF(NOT HAS_PTHREAD_CREATE)

    CHECK_FUNCTION_EXISTS(nanosleep HAVE_NANOSLEEP)
    IF(NOT HAVE_NANOSLEEP)
        MESSAGE(FATAL_ERROR "No sleep function found!")
    ENDIF(NOT HAVE_NANOSLEEP)
ENDIF(HAVE_WINDOWS_H)

CHECK_INCLUDE_FILE(sys/types.h HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILE(sys/wait.h HAVE_SYS_WAIT_H)
CHECK_INCLUDE_FILE(signal.h HAVE_SIGNAL_H)
CHECK_INCLUDE_FILE(dlfcn.h HAVE_DLFCN_H)

IF(HAVE_DLFCN_H)
    CHECK_SHARED_FUNCTION_EXISTS(dlopen "dlfcn.h" dl "" HAVE_LIBDL)
    IF(HAVE_LIBDL)
        SET(EXTRA_LIBS dl ${EXTRA_LIBS})
    ENDIF(HAVE_LIBDL)
ENDIF(HAVE_DLFCN_H)

OPTION(DYNLOAD    "Dynamically load support libs at run-time" ON)
OPTION(SNDFILE    "SoundFile support (for various formats)" ON)
OPTION(VORBIS     "VorbisFile support (for Ogg Vorbis)"  ON)
OPTION(FLAC       "FLAC support (for FLAC and Ogg FLAC)" ON)
OPTION(MPG123     "MPG123 support (for MP1/MP2/MP3)" ON)
OPTION(DUMB       "DUMB support (for IT/XM/S3M/MOD)" ON)
OPTION(MODPLUG    "ModPlug support (for IT/XM/S3M/MOD)" OFF)
OPTION(FLUIDSYNTH "FluidSynth support (for MID)" ON)

IF(WIN32)
    ADD_DEFINITIONS(-D_WIN32)
ELSE(WIN32)
    IF(NOT HAVE_DLFCN_H OR NOT HAVE_LIBDL)
        SET(DYNLOAD OFF)
    ENDIF(NOT HAVE_DLFCN_H OR NOT HAVE_LIBDL)
    PKG_CHECK_MODULES(OPENAL openal)
ENDIF(WIN32)

IF(NOT OPENAL_FOUND)
    FIND_PACKAGE(OpenAL)
    IF(NOT OPENAL_FOUND)
        MESSAGE(FATAL_ERROR "OpenAL not found!\nPlease try setting the OPENALDIR environment variable and try again.")
    ENDIF(NOT OPENAL_FOUND)

    MESSAGE(STATUS "OpenAL include: ${OPENAL_INCLUDE_DIR}")
    MESSAGE(STATUS "OpenAL lib: ${OPENAL_LIBRARY}")

    INCLUDE_DIRECTORIES(${OPENAL_INCLUDE_DIR})
    SET(OPENAL_LIBRARIES ${OPENAL_LIBRARY})
ELSE(NOT OPENAL_FOUND)
    INCLUDE_DIRECTORIES(${OPENAL_INCLUDE_DIRS})
    LINK_DIRECTORIES(${OPENAL_LIBRARY_DIRS})
ENDIF(NOT OPENAL_FOUND)

# Base source files
SET(ALURE_OBJS  src/alure.cpp
                src/buffer.cpp
                src/istream.cpp
                src/stream.cpp
                src/streamdec.cpp
                src/streamplay.cpp
                src/codec_wav.cpp
                src/codec_aiff.cpp
)

# SndFile support
IF(SNDFILE)
    PKG_CHECK_MODULES(SNDFILE sndfile)
    IF(SNDFILE_FOUND)
        SET(HAS_SNDFILE 1)
        LINK_DIRECTORIES(${SNDFILE_LIBRARY_DIRS})
        SET_SOURCE_FILES_PROPERTIES(src/codec_sndfile.cpp PROPERTIES
                                    COMPILE_FLAGS "${SNDFILE_CFLAGS}")
    ELSE(SNDFILE_FOUND)
        FIND_PACKAGE(SndFile)
        IF(SNDFILE_FOUND)
            SET(HAS_SNDFILE 1)
            INCLUDE_DIRECTORIES(${SNDFILE_INCLUDE_DIRS})
        ENDIF(SNDFILE_FOUND)
    ENDIF(SNDFILE_FOUND)
    IF(HAS_SNDFILE)
        SET(ALURE_OBJS ${ALURE_OBJS} src/codec_sndfile.cpp)
    ENDIF(HAS_SNDFILE)
ELSE(SNDFILE)
    SET(SNDFILE_LIBRARIES "")
ENDIF(SNDFILE)

# Vorbis support
IF(VORBIS)
    PKG_CHECK_MODULES(VORBISFILE vorbisfile)
    IF(VORBISFILE_FOUND)
        SET(HAS_VORBISFILE 1)
        LINK_DIRECTORIES(${VORBISFILE_LIBRARY_DIRS})
        SET_SOURCE_FILES_PROPERTIES(src/codec_vorbisfile.cpp PROPERTIES
                                    COMPILE_FLAGS "${VORBISFILE_CFLAGS}")
    ELSE(VORBISFILE_FOUND)
        FIND_PACKAGE(OggVorbis)
        IF(OGGVORBIS_FOUND)
            SET(HAS_VORBISFILE 1)
            INCLUDE_DIRECTORIES(${OGGVORBIS_INCLUDE_DIRS})
            SET(VORBISFILE_LIBRARIES ${OGGVORBIS_LIBRARIES})
        ENDIF(OGGVORBIS_FOUND)
    ENDIF(VORBISFILE_FOUND)
    IF(NOT HAS_VORBISFILE)
        PKG_CHECK_MODULES(VORBISIDEC vorbisidec)
        IF(NOT VORBISIDEC_FOUND)
            CHECK_INCLUDE_FILE(tremor/ivorbisfile.h HAVE_TREMOR_IVORBISFILE_H)
            IF(HAVE_TREMOR_IVORBISFILE_H)
                CHECK_SHARED_FUNCTION_EXISTS(ov_open "tremor/ivorbisfile.h" vorbisidec "" HAVE_LIBVORBISIDEC)
                IF(DYNLOAD OR HAVE_LIBVORBISIDEC)
                    SET(HAS_VORBISFILE 1)
                    IF(HAVE_LIBVORBISIDEC)
                        SET(VORBISFILE_LIBRARIES "vorbisidec")
                    ENDIF(HAVE_LIBVORBISIDEC)
                    SET_SOURCE_FILES_PROPERTIES(src/codec_vorbisfile.cpp PROPERTIES
                                                COMPILE_FLAGS "-DHAS_VORBISIDEC")
                ENDIF(DYNLOAD OR HAVE_LIBVORBISIDEC)
            ENDIF(HAVE_TREMOR_IVORBISFILE_H)
        ELSE(NOT VORBISIDEC_FOUND)
            SET(HAS_VORBISFILE 1)
            LINK_DIRECTORIES(${VORBISIDEC_LIBRARY_DIRS})
            SET_SOURCE_FILES_PROPERTIES(src/codec_vorbisfile.cpp PROPERTIES
                                        COMPILE_FLAGS "${VORBISIDEC_CFLAGS} -DHAS_VORBISIDEC")
        ENDIF(NOT VORBISIDEC_FOUND)
    ENDIF(NOT HAS_VORBISFILE)
    IF(HAS_VORBISFILE)
        SET(ALURE_OBJS ${ALURE_OBJS} src/codec_vorbisfile.cpp)
    ENDIF(HAS_VORBISFILE)
ELSE(VORBIS)
    SET(VORBISFILE_LIBRARIES "")
ENDIF(VORBIS)

# FLAC support
IF(FLAC)
    FIND_PACKAGE(FLAC)
    IF(FLAC_FOUND)
        INCLUDE_DIRECTORIES(${FLAC_INCLUDE_DIR})
        SET(HAS_FLAC 1)
    ENDIF(FLAC_FOUND)
    IF(HAS_FLAC)
        SET(ALURE_OBJS ${ALURE_OBJS} src/codec_flac.cpp)
    ENDIF(HAS_FLAC)
ELSE(FLAC)
    SET(LIBFLAC_LIBRARIES "")
ENDIF(FLAC)

# MPG123 support
IF(MPG123)
    FIND_PACKAGE(MPG123)
    IF(NOT MPG123_FOUND)
        CHECK_INCLUDE_FILE(mpg123.h HAVE_MPG123_H)
        IF(HAVE_MPG123_H)
            CHECK_SHARED_FUNCTION_EXISTS(mpg123_init "mpg123.h" mpg123 "" HAVE_LIBMPG123)
            IF(DYNLOAD OR HAVE_LIBMPG123)
                SET(HAS_MPG123 1)
                IF(HAVE_LIBMPG123)
                    SET(MPG123_LIBRARIES "mpg123")
                ENDIF(HAVE_LIBMPG123)
            ENDIF(DYNLOAD OR HAVE_LIBMPG123)
        ENDIF(HAVE_MPG123_H)
    ELSE(NOT MPG123_FOUND)
        SET(HAS_MPG123 1)
        LINK_DIRECTORIES(${MPG123_LIBRARY_DIRS})
        SET_SOURCE_FILES_PROPERTIES(src/codec_mpg123.cpp PROPERTIES
                                    COMPILE_FLAGS "${MPG123_CFLAGS}")
    ENDIF(NOT MPG123_FOUND)
    IF(HAS_MPG123)
        SET(ALURE_OBJS ${ALURE_OBJS} src/codec_mpg123.cpp)
    ENDIF(HAS_MPG123)
ELSE(MPG123)
    SET(MPG123_LIBRARIES "")
ENDIF(MPG123)

# DUMB support
IF(DUMB)
    CHECK_INCLUDE_FILE(dumb.h HAVE_DUMB_H)
    IF(HAVE_DUMB_H)
        IF(HAS_LIBM)
            CHECK_LIBRARY_EXISTS("dumb;m" dumbfile_open_ex "" HAVE_LIBDUMB)
        ELSE(HAS_LIBM)
            CHECK_LIBRARY_EXISTS(dumb dumbfile_open_ex "" HAVE_LIBDUMB)
        ENDIF(HAS_LIBM)
        IF(DYNLOAD OR HAVE_LIBDUMB)
            SET(HAS_DUMB 1)
            IF(HAVE_LIBDUMB)
                SET(DUMB_LIBRARIES dumb)
            ENDIF(HAVE_LIBDUMB)
        ENDIF(DYNLOAD OR HAVE_LIBDUMB)
    ENDIF(HAVE_DUMB_H)
    IF(HAS_DUMB)
        SET(ALURE_OBJS ${ALURE_OBJS} src/codec_dumb.cpp)
    ENDIF(HAS_DUMB)
ELSE(DUMB)
    SET(DUMB_LIBRARIES "")
ENDIF(DUMB)

# ModPlug support
IF(MODPLUG)
    CHECK_INCLUDE_FILE(libmodplug/modplug.h HAVE_LIBMODPLUG_MODPLUG_H)
    IF(HAVE_LIBMODPLUG_MODPLUG_H)
        CHECK_SHARED_FUNCTION_EXISTS(ModPlug_Load "libmodplug/modplug.h" modplug "" HAVE_LIBMODPLUG)
        IF(DYNLOAD OR HAVE_LIBMODPLUG)
            SET(HAS_MODPLUG 1)
            IF(HAVE_LIBMODPLUG)
                SET(MODPLUG_LIBRARIES "modplug")
            ENDIF(HAVE_LIBMODPLUG)
        ENDIF(DYNLOAD OR HAVE_LIBMODPLUG)
    ENDIF(HAVE_LIBMODPLUG_MODPLUG_H)
    IF(HAS_MODPLUG)
        SET(ALURE_OBJS ${ALURE_OBJS} src/codec_modplug.cpp)
    ENDIF(HAS_MODPLUG)
ELSE(MODPLUG)
    SET(MODPLUG_LIBRARIES "")
ENDIF(MODPLUG)

# FluidSynth support
IF(FLUIDSYNTH)
    PKG_CHECK_MODULES(FLUIDSYNTH fluidsynth>=1.1.1)
    IF(NOT FLUIDSYNTH_FOUND)
        CHECK_INCLUDE_FILE(fluidsynth.h HAVE_FLUIDSYNTH_H)
        IF(HAVE_FLUIDSYNTH_H)
            CHECK_SHARED_FUNCTION_EXISTS(new_fluid_synth "fluidsynth.h" fluidsynth "" HAVE_LIBFLUIDSYNTH)
            IF(DYNLOAD OR HAVE_LIBFLUIDSYNTH)
                SET(HAS_FLUIDSYNTH 1)
                IF(HAVE_LIBFLUIDSYNTH)
                    SET(FLUIDSYNTH_LIBRARIES "fluidsynth")
                ENDIF(HAVE_LIBFLUIDSYNTH)
            ENDIF(DYNLOAD OR HAVE_LIBFLUIDSYNTH)
        ENDIF(HAVE_FLUIDSYNTH_H)
    ELSE(NOT FLUIDSYNTH_FOUND)
        SET(HAS_FLUIDSYNTH 1)
        LINK_DIRECTORIES(${FLUIDSYNTH_LIBRARY_DIRS})
        SET_SOURCE_FILES_PROPERTIES(src/codec_fluidsynth.cpp PROPERTIES
                                    COMPILE_FLAGS "${FLUIDSYNTH_CFLAGS}")
    ENDIF(NOT FLUIDSYNTH_FOUND)
    IF(HAS_FLUIDSYNTH)
        SET(ALURE_OBJS ${ALURE_OBJS} src/codec_fluidsynth.cpp)
    ENDIF(HAS_FLUIDSYNTH)
ELSE(FLUIDSYNTH)
    SET(FLUIDSYNTH_LIBRARIES "")
ENDIF(FLUIDSYNTH)


IF(NOT DYNLOAD)
    SET(EXTRA_LIBS ${SNDFILE_LIBRARIES} ${VORBISFILE_LIBRARIES} ${LIBFLAC_LIBRARIES} ${MPG123_LIBRARIES} ${DUMB_LIBRARIES} ${MODPLUG_LIBRARIES} ${FLUIDSYNTH_LIBRARIES} ${EXTRA_LIBS})
ELSE(NOT DYNLOAD)
    ADD_DEFINITIONS(-DDYNLOAD=1)
ENDIF(NOT DYNLOAD)

# For alure.pc.in
SET(prefix ${CMAKE_INSTALL_PREFIX})
SET(exec_prefix "\${prefix}")
SET(libdir "\${exec_prefix}/lib${LIB_SUFFIX}")
SET(bindir "\${exec_prefix}/bin")
SET(includedir "\${prefix}/include")
SET(PKG_CONFIG_REQUIRES openal)
SET(PACKAGE_VERSION "${LIB_VERSION}")

# End configuration
CONFIGURE_FILE(
    "${ALURE_SOURCE_DIR}/config.h.in"
    "${ALURE_BINARY_DIR}/config.h")
IF(BUILD_SHARED)
    CONFIGURE_FILE(
        "${ALURE_SOURCE_DIR}/alure.pc.in"
        "${ALURE_BINARY_DIR}/alure.pc"
        @ONLY)
ENDIF(BUILD_SHARED)
IF(BUILD_STATIC)
    CONFIGURE_FILE(
        "${ALURE_SOURCE_DIR}/alure-static.pc.in"
        "${ALURE_BINARY_DIR}/alure-static.pc"
        @ONLY)
ENDIF(BUILD_STATIC)

ADD_DEFINITIONS(-DHAVE_CONFIG_H)


IF(BUILD_SHARED)
    #build a shared library
    ADD_LIBRARY(${LIBNAME} SHARED ${ALURE_OBJS})
    SET_TARGET_PROPERTIES(${LIBNAME} PROPERTIES DEFINE_SYMBOL ALURE_BUILD_LIBRARY
                                                VERSION ${LIB_VERSION}.0
                                                SOVERSION ${LIB_MAJOR_VERSION})
    IF(WIN32)
        SET_TARGET_PROPERTIES(${LIBNAME} PROPERTIES PREFIX "")
    ELSEIF(NOT APPLE)
        SET_TARGET_PROPERTIES(${LIBNAME} PROPERTIES LINK_FLAGS "-Wl,--version-script=${ALURE_SOURCE_DIR}/libalure.map")
    ENDIF(WIN32)
    TARGET_LINK_LIBRARIES(${LIBNAME} ${OPENAL_LIBRARIES} ${EXTRA_LIBS})

    INSTALL(TARGETS ${LIBNAME}
            RUNTIME DESTINATION bin
            LIBRARY DESTINATION lib${LIB_SUFFIX}
            ARCHIVE DESTINATION lib${LIB_SUFFIX}
)
ENDIF(BUILD_SHARED)
IF(BUILD_STATIC)
    #build a static library
    ADD_LIBRARY(${LIBNAME}-static STATIC ${ALURE_OBJS})
    SET_TARGET_PROPERTIES(${LIBNAME}-static PROPERTIES DEFINE_SYMBOL ALURE_BUILD_LIBRARY
                                                       COMPILE_FLAGS ${STATIC_CFLAGS}
                                                       VERSION ${LIB_VERSION}.0)
    INSTALL(TARGETS ${LIBNAME}-static
            RUNTIME DESTINATION bin
            LIBRARY DESTINATION lib${LIB_SUFFIX}
            ARCHIVE DESTINATION lib${LIB_SUFFIX}
)
ENDIF(BUILD_STATIC)


OPTION(BUILD_EXAMPLES   "Build example programs"   ON)
OPTION(INSTALL_EXAMPLES "Install example programs" ON)

IF(BUILD_EXAMPLES)
    IF(HAVE_DDK_NTDDCDRM_H)
        SET_SOURCE_FILES_PROPERTIES(examples/alurecdplay.c PROPERTIES
                                    COMPILE_FLAGS "-DHAVE_DDK_NTDDCDRM_H")
    ENDIF(HAVE_DDK_NTDDCDRM_H)
    ADD_EXECUTABLE(alureplay examples/alureplay.c)
    ADD_EXECUTABLE(alurecdplay examples/alurecdplay.c)
    ADD_EXECUTABLE(alurestream examples/alurestream.c)
    IF(BUILD_SHARED)
        TARGET_LINK_LIBRARIES(alureplay ${LIBNAME} ${OPENAL_LIBRARIES} ${EXTRA_LIBS})
        TARGET_LINK_LIBRARIES(alurecdplay ${LIBNAME} ${OPENAL_LIBRARIES} ${EXTRA_LIBS})
        TARGET_LINK_LIBRARIES(alurestream ${LIBNAME} ${OPENAL_LIBRARIES} ${EXTRA_LIBS})
    ELSE(BUILD_SHARED)
        SET_TARGET_PROPERTIES(alureplay PROPERTIES COMPILE_FLAGS -DALURE_STATIC_LIBRARY)
        SET_TARGET_PROPERTIES(alurecdplay PROPERTIES COMPILE_FLAGS -DALURE_STATIC_LIBRARY)
        SET_TARGET_PROPERTIES(alurestream PROPERTIES COMPILE_FLAGS -DALURE_STATIC_LIBRARY)
        TARGET_LINK_LIBRARIES(alureplay ${LIBNAME}-static ${OPENAL_LIBRARIES} ${EXTRA_LIBS})
        TARGET_LINK_LIBRARIES(alurecdplay ${LIBNAME}-static ${OPENAL_LIBRARIES} ${EXTRA_LIBS})
        TARGET_LINK_LIBRARIES(alurestream ${LIBNAME}-static ${OPENAL_LIBRARIES} ${EXTRA_LIBS})
    ENDIF(BUILD_SHARED)
ENDIF(BUILD_EXAMPLES)


FIND_PROGRAM(NATDOCS_BIN NaturalDocs)
IF(NATDOCS_BIN)
    ADD_CUSTOM_TARGET(docs
                      "${NATDOCS_BIN}" -i "${ALURE_SOURCE_DIR}/src" -o HTML "${ALURE_SOURCE_DIR}/docs/html" -p "${ALURE_SOURCE_DIR}/docs/naturaldocs" -s Default CustomStyle
                      COMMENT "Building NaturalDocs documentation..." VERBATIM)
ELSE(NATDOCS_BIN)
    MESSAGE(STATUS "\nNaturalDocs not found; building documentation is disabled")
ENDIF(NATDOCS_BIN)


#add an install target here
IF(APPLE)
    SET(INCPATH OpenAL)
ELSE(APPLE)
    SET(INCPATH AL)
ENDIF(APPLE)

INSTALL(DIRECTORY "${ALURE_SOURCE_DIR}/docs/html"
        DESTINATION share/doc/alure
)
INSTALL(FILES include/AL/alure.h
        DESTINATION "include/${INCPATH}"
)
IF(BUILD_SHARED)
    INSTALL(FILES "${ALURE_BINARY_DIR}/alure.pc"
            DESTINATION "lib${LIB_SUFFIX}/pkgconfig"
    )
ENDIF(BUILD_SHARED)
IF(BUILD_STATIC)
    INSTALL(FILES "${ALURE_BINARY_DIR}/alure-static.pc"
            DESTINATION "lib${LIB_SUFFIX}/pkgconfig"
    )
ENDIF(BUILD_STATIC)
IF(BUILD_EXAMPLES AND INSTALL_EXAMPLES)
    INSTALL(TARGETS alureplay alurestream alurecdplay
            RUNTIME DESTINATION bin)
ENDIF(BUILD_EXAMPLES AND INSTALL_EXAMPLES)


MESSAGE(STATUS "\n********************** Configured options **********************")
IF(BUILD_SHARED AND BUILD_STATIC)
    MESSAGE(STATUS "Building shared, static library")
ELSEIF(BUILD_SHARED)
    MESSAGE(STATUS "Building shared library")
ELSE(BUILD_SHARED AND BUILD_STATIC)
    MESSAGE(STATUS "Building static library")
ENDIF(BUILD_SHARED AND BUILD_STATIC)
IF(BUILD_EXAMPLES AND INSTALL_EXAMPLES)
    MESSAGE(STATUS "Building and installing examples")
ELSEIF(BUILD_EXAMPLES)
    MESSAGE(STATUS "Building (not installing) examples")
ELSE(BUILD_EXAMPLES AND INSTALL_EXAMPLES)
    MESSAGE(STATUS "Not building examples")
ENDIF(BUILD_EXAMPLES AND INSTALL_EXAMPLES)
MESSAGE(STATUS "")
IF(HAS_SNDFILE)
    MESSAGE(STATUS "SndFile support: enabled")
ELSE(HAS_SNDFILE)
    MESSAGE(STATUS "SndFile support: disabled")
ENDIF(HAS_SNDFILE)
IF(HAS_VORBISFILE)
    MESSAGE(STATUS "VorbisFile support: enabled")
ELSE(HAS_VORBISFILE)
    MESSAGE(STATUS "VorbisFile support: disabled")
ENDIF(HAS_VORBISFILE)
IF(HAS_FLAC)
    MESSAGE(STATUS "FLAC support: enabled")
ELSE(HAS_FLAC)
    MESSAGE(STATUS "FLAC support: disabled")
ENDIF(HAS_FLAC)
IF(HAS_MPG123)
    MESSAGE(STATUS "MPG123 support: enabled")
ELSE(HAS_MPG123)
    MESSAGE(STATUS "MPG123 support: disabled")
ENDIF(HAS_MPG123)
IF(HAS_DUMB)
    MESSAGE(STATUS "DUMB support: enabled")
ELSE(HAS_DUMB)
    MESSAGE(STATUS "DUMB support: disabled")
ENDIF(HAS_DUMB)
IF(HAS_MODPLUG)
    MESSAGE(STATUS "ModPlug support: enabled")
ELSE(HAS_MODPLUG)
    MESSAGE(STATUS "ModPlug support: disabled")
ENDIF(HAS_MODPLUG)
IF(HAS_FLUIDSYNTH)
    MESSAGE(STATUS "FluidSynth support: enabled")
ELSE(HAS_FLUIDSYNTH)
    MESSAGE(STATUS "FluidSynth support: disabled")
ENDIF(HAS_FLUIDSYNTH)
MESSAGE(STATUS "")
