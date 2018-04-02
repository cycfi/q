###############################################################################
#  Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(io)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"
      OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}-ftemplate-backtrace-limit=0")
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
   set(io_external_libs
      ${CMAKE_SOURCE_DIR}/io/external/lib/osx/libsndfile.a
   )
endif()

add_library(io src/audio_file.cpp)

target_link_libraries(io libq ${io_external_libs})

target_include_directories(io
   PUBLIC external/include/
   PUBLIC include/
)

target_include_directories(io PUBLIC include/)


