###############################################################################
#  Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################
cmake_minimum_required(VERSION 3.7.2)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(libqio)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"
      OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}-ftemplate-backtrace-limit=0")
   set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-deprecated-declarations")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_subdirectory(external/portmidi)
add_subdirectory(external/portaudio)

add_library(
   libqio
   src/audio_file.cpp
   src/audio_device.cpp
   src/audio_stream.cpp
   src/midi_device.cpp
   src/midi_stream.cpp
)

target_link_libraries(
   libqio
   libq
   infra
   portaudio
   portmidi
   ${io_external_libs}
)

target_include_directories(libqio
   PRIVATE external/include/
   PUBLIC include/
)




