###############################################################################
#  Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################
cmake_minimum_required(VERSION 3.16.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(libqio)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"
      OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftemplate-backtrace-limit=0")
   set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-deprecated-declarations")
endif()

option(BUILD_SHARED_LIBS "Build shared/dynamic library" OFF)

###############################################################################
# rtaudio
#
# Replaces PortAudio, whose CoreAudio backend could deadlock in stop(): its
# IsRunning listener takes the AudioUnit lock while the caller holds it.
# RtAudio drives the HAL IOProc directly, so there is no such listener.

set(RTAUDIO_BUILD_TESTING OFF CACHE BOOL "" FORCE)
if (WIN32)
   # ASIO beside WASAPI; the SDK ships inside RtAudio.
   set(RTAUDIO_API_ASIO ON CACHE BOOL "" FORCE)
endif()

FetchContent_Declare(
  rtaudio
  GIT_REPOSITORY https://github.com/thestk/rtaudio.git
  GIT_TAG 6.0.1
)

FetchContent_MakeAvailable(rtaudio)

###############################################################################
# libremidi
#
# Replaces PortMidi, which is MIDI 1.0 only: its event is a packed three byte
# message and there is no packet in the API to extend. libremidi speaks both
# versions over the same backends, and hands over Universal MIDI Packets where
# the platform provides them.

FetchContent_Declare(
  libremidi
  GIT_REPOSITORY https://github.com/celtera/libremidi.git
  GIT_TAG 9d69bfb   # v5.4.3 plus the stream and flex data packet fix
)

FetchContent_MakeAvailable(libremidi)

###############################################################################
# libqio

add_library(libqio STATIC
   src/audio_file.cpp
   src/audio_device.cpp
   src/audio_stream.cpp
   src/midi_device.cpp
   src/midi_stream.cpp
   src/midi2_stream.cpp
)

target_link_libraries(libqio
   libq
   cycfi::infra
   libremidi
   rtaudio
)

if (APPLE)
   set(CMAKE_MACOSX_RPATH 1)
   find_library(COREAUDIO_LIBRARY CoreAudio REQUIRED)
   find_library(AUDIOTOOLBOX_LIBRARY AudioToolbox REQUIRED)
   find_library(AUDIOUNIT_LIBRARY AudioUnit REQUIRED)
   find_library(CARBON_LIBRARY Carbon REQUIRED)
   mark_as_advanced(COREAUDIO_LIBRARY AUDIOTOOLBOX_LIBRARY AUDIOUNIT_LIBRARY CARBON_LIBRARY)
endif()

target_include_directories(libqio
   PRIVATE external/include/
   PUBLIC include/
)


