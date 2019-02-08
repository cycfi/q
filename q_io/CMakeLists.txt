###############################################################################
#  Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################
cmake_minimum_required(VERSION 3.5.1)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(libqio)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"
      OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}-ftemplate-backtrace-limit=0")
   set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-deprecated-declarations")
endif()

if (WIN32)
   option(PA_BUILD_SHARED "Build shared/dynamic library" OFF)
   add_subdirectory(external/portaudio)
   add_subdirectory(external/portmidi)
   set(PORTMIDI_INCLUDE_DIRS external/portmidi/pm_common/)
   set(PORTAUDIO_INCLUDE_DIRS external/portaudio/include/)
   set(PORTAUDIO_LIBRARY portaudio_static)
   set(PORTMIDI_LIBRARY portmidi)
else()
   set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
   find_package(portmidi REQUIRED)
   find_package(portaudio REQUIRED)
endif()

add_library(
   libqio
   src/audio_file.cpp
   src/audio_device.cpp
   src/audio_stream.cpp
   src/midi_device.cpp
   src/midi_stream.cpp
)

if (APPLE OR WIN32)
   target_link_libraries(
      libqio
      libq
      infra
      ${PORTMIDI_LIBRARY}
      ${PORTAUDIO_LIBRARY}
   )
else()
   target_link_libraries(
      libqio
      libq
      infra
      portmidi
      portaudio
   )
endif()

target_include_directories(libqio
   PRIVATE external/include/
   PRIVATE ${PORTAUDIO_INCLUDE_DIRS}
   PRIVATE ${PORTMIDI_INCLUDE_DIRS}
   PUBLIC include/
)


