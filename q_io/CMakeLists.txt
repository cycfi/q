###############################################################################
#  Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################
cmake_minimum_required(VERSION 3.7.2)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(q_io)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"
      OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}-ftemplate-backtrace-limit=0")
endif()

option(PA_BUILD_SHARED "Build shared/dynamic library" OFF)
add_subdirectory(external/portaudio)

add_library(
   q_io
   src/audio_file.cpp
   src/audio_device.cpp
)

target_link_libraries(
   q_io
   libq
   infra
   portaudio_static
   ${io_external_libs}
)

target_include_directories(q_io
   PRIVATE external/include/
   PRIVATE external/portaudio/include/
   PUBLIC include/
)

target_include_directories(q_io PUBLIC include/)



