###############################################################################
#  Copyright (c) 2014-2020 Joel de Guzman. All rights reserved.
#
#  Distributed under the Boost Software License, Version 1.0. (See accompanying
#  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
###############################################################################
cmake_minimum_required(VERSION 3.5.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

###############################################################################
project(q_example)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"
      OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftemplate-backtrace-limit=0")
endif()

set(APP_SOURCES
   list_devices.cpp
   sin_synth.cpp
   square_synth.cpp
   delay.cpp
   io_delay.cpp
   midi_monitor.cpp
)

foreach(sourcefile ${APP_SOURCES})
   string(REPLACE ".cpp" "" name ${sourcefile})
   add_executable(example_${name} ${sourcefile})
   target_link_libraries(example_${name} libq libqio cycfi::infra)
endforeach(sourcefile ${APP_SOURCES})

# Copy test files to the binary dir
file(
  COPY audio_files
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})