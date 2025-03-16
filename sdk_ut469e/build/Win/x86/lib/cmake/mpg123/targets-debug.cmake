#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "MPG123::libmpg123" for configuration "Debug"
set_property(TARGET MPG123::libmpg123 APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(MPG123::libmpg123 PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/lib/mpg123d.lib"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/bin/mpg123d.dll"
  )

list(APPEND _cmake_import_check_targets MPG123::libmpg123 )
list(APPEND _cmake_import_check_files_for_MPG123::libmpg123 "${_IMPORT_PREFIX}/lib/mpg123d.lib" "${_IMPORT_PREFIX}/bin/mpg123d.dll" )

# Import target "MPG123::libsyn123" for configuration "Debug"
set_property(TARGET MPG123::libsyn123 APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(MPG123::libsyn123 PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/lib/syn123d.lib"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/bin/syn123d.dll"
  )

list(APPEND _cmake_import_check_targets MPG123::libsyn123 )
list(APPEND _cmake_import_check_files_for_MPG123::libsyn123 "${_IMPORT_PREFIX}/lib/syn123d.lib" "${_IMPORT_PREFIX}/bin/syn123d.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
