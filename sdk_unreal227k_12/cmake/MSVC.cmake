include_guard(GLOBAL)

include(CMakeInitializeConfigs)

if (COMMAND CMAKE_POLICY)
  if (POLICY CMP0054)
	cmake_policy(SET CMP0054 NEW)
  endif()
endif()

# We use this to override default compiler settings
function(cmake_initialize_per_config_variable _PREFIX _DOCSTRING)
  if (_PREFIX MATCHES "CMAKE_(C|CXX)_FLAGS")
    foreach (config
      ${_PREFIX}_DEBUG_INIT
      ${_PREFIX}_RELEASE_INIT
      ${_PREFIX}_RELWITHDEBINFO_INIT
      ${_PREFIX}_MINSIZEREL_INIT
	  ${_PREFIX}_INIT
	)
      # make sure we statically link the C runtime
      string(REPLACE "/MDd" "/MTd" ${config} "${${config}}")
      string(REPLACE "/MD" "/MT" ${config} "${${config}}")
  	  if("${config}" MATCHES "DEBUG" AND NOT ${config} MATCHES "/MTd")
		string(APPEND ${config} " /MTd")
	  endif()
	  if (NOT "${config}" MATCHES "DEBUG" AND NOT "${config}" MATCHES "${_PREFIX}_INIT" AND NOT ${config} MATCHES "/MT")
		string(APPEND ${config} " /MT")
	  endif()

	  # use async exceptions
	  string(REPLACE "/EHsc" "/EHa" ${config} "${${config}}")
	  string(REPLACE "/EHs" "/EHa" ${config} "${${config}}")
	  if(NOT ${config} MATCHES "/EHa")
        string(APPEND ${config} " /EHa")
	  endif()

  message(STATUS "${config}: ${${config}}")

    endforeach()
  endif()
    
  _cmake_initialize_per_config_variable(${ARGV})
endfunction()

# add postfixes to filenames for non-release configurations
set(CMAKE_DEBUG_POSTFIX "d")

# ensures we find the debug version of a target when building - Do we still need this?
set(CMAKE_MAP_IMPORTED_CONFIG_DEBUG DEBUG)
set(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO RELEASE)

# sub-projects that set CMP0091 to NEW will ignore /MT and /MTd and use the value set in CMAKE_MSVC_RUNTIME_LIBRARY instead
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
