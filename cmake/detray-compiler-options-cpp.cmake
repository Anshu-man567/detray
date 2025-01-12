# Detray library, part of the ACTS project (R&D line)
#
# (c) 2021-2022 CERN for the benefit of the ACTS project
#
# Mozilla Public License Version 2.0

# Include the helper function(s).
include( detray-functions )

# Turn on the correct setting for the __cplusplus macro with MSVC.
if( "${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC" )
   detray_add_flag( CMAKE_CXX_FLAGS "/Zc:__cplusplus" )
endif()

# Respect infinity expressions for IntelLLVM
if ( "${CMAKE_CXX_COMPILER_ID}" MATCHES "IntelLLVM")
   detray_add_flag( CMAKE_CXX_FLAGS "-fhonor-infinities")
endif()

# Turn on a number of warnings for the "known compilers".
if( ( "${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU" ) OR
    ( "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" ) OR 
    ( "${CMAKE_CXX_COMPILER_ID}" MATCHES "IntelLLVM") )

   # Basic flags for all build modes.
   detray_add_flag( CMAKE_CXX_FLAGS "-Wall" )
   detray_add_flag( CMAKE_CXX_FLAGS "-Wextra" )
   detray_add_flag( CMAKE_CXX_FLAGS "-Wshadow" )
   detray_add_flag( CMAKE_CXX_FLAGS "-Wunused-local-typedefs" )

   # More rigorous tests for the Debug builds.
   detray_add_flag( CMAKE_CXX_FLAGS_DEBUG "-Werror" )
   detray_add_flag( CMAKE_CXX_FLAGS_DEBUG "-pedantic" )

elseif( "${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC" )

   # Basic flags for all build modes.
   string( REGEX REPLACE "/W[0-9]" "" CMAKE_CXX_FLAGS
      "${CMAKE_CXX_FLAGS}" )
   detray_add_flag( CMAKE_CXX_FLAGS "/W4" )

   # More rigorous tests for the Debug builds.
   detray_add_flag( CMAKE_CXX_FLAGS_DEBUG "/WX" )

endif()
