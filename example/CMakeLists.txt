###############################################################################
#  Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the Boost Software License, Version 1.0.
#  [ https://www.boost.org/LICENSE_1_0.txt ]
###############################################################################
cmake_minimum_required(VERSION 3.5.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

###############################################################################
project(q_example)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"
      OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftemplate-backtrace-limit=0")
endif()

# Each example is self-contained: one directory, one target, and (when the
# example reads audio) its own audio/ directory, baked in as AUDIO_DIR so
# the example runs from anywhere.

add_subdirectory(common)
add_subdirectory(list_devices)
add_subdirectory(sin_osc)
add_subdirectory(waveforms)
add_subdirectory(square_synth)
add_subdirectory(delay)
add_subdirectory(grain_freeze)
add_subdirectory(io_delay)
add_subdirectory(midi_monitor)
add_subdirectory(sustain_hold)
