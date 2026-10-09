###############################################################################
#  Copyright (c) 2019-2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################
cmake_minimum_required(VERSION 3.21)

# The macOS deployment target is set by q's top-level CMakeLists, before
# project(), when Q_BUILD_PLUG is on.

project(q_plug VERSION 0.1.1 LANGUAGES C CXX)

# C++20, PIC, the static MSVC runtime and Objective-C, set before any
# dependency is added so they reach every target. See q_plug_settings.cmake.
include(cmake/q_plug_settings.cmake)
q_plug_settings()

# What a plugin's CMakeLists names goes in the cache, so a plugin built
# outside Q, in a directory of its own, sees it too.
set(Q_PLUG_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" CACHE INTERNAL "")
set(q_plug_VERSION "${q_plug_VERSION}" CACHE INTERNAL "")

# The formats a plugin is built in. AUv2 is macOS only; CLAP and VST3 are
# everywhere. Examples pass this to make_clapfirst_plugins, and ask it for
# a standalone app as well, which hosts the CLAP with its own audio and
# MIDI devices through RtAudio and RtMidi, fetched at configure time.
set(formats CLAP VST3)
if(APPLE)
   list(APPEND formats AUV2)
endif()
set(Q_PLUG_PLUGIN_FORMATS "${formats}" CACHE INTERNAL "")

###############################################################################
# Dependencies, fetched at configure time and pinned. CLAP is a Cycfi fork
# that carries bug fixes only, each one a pull request upstream; features
# never go in there. Elements brings Artist as its one submodule; Artist
# fetches the rest of its own when it configures.

include(FetchContent)

FetchContent_Declare(
   clap
   GIT_REPOSITORY https://github.com/cycfi/clap.git
   GIT_TAG ca1b43ee2b4f381e94e4170e622d60b92108b128
   SOURCE_SUBDIR do-not-add          # clap-wrapper adds it
)
# Cycfi's fork, branch q-pin: upstream 8fb879c (free-audio next) and a
# standalone that reports a plugin it cannot create instead of crashing.
# Back to free-audio once that lands.
FetchContent_Declare(
   clap_wrapper
   GIT_REPOSITORY https://github.com/cycfi/clap-wrapper.git
   GIT_TAG af23b0fa058b96430354d23b023db9d143467852
   SOURCE_SUBDIR do-not-add          # added below, EXCLUDE_FROM_ALL
)
FetchContent_Declare(
   cycfi_elements
   GIT_REPOSITORY https://github.com/cycfi/elements.git
   GIT_TAG c32b2668f9bb57070742fbf817f9f778a38f8f33
   GIT_SUBMODULES lib/artist
   GIT_SUBMODULES_RECURSE OFF
   SOURCE_SUBDIR do-not-add          # added below, after its settings
)
FetchContent_Declare(
   nlohmann_json
   GIT_REPOSITORY https://github.com/nlohmann/json.git
   GIT_TAG v3.12.0
   GIT_SHALLOW ON
)
set(JSON_BuildTests OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(clap clap_wrapper cycfi_elements
   nlohmann_json)

###############################################################################
# clap-wrapper: projects the native CLAP to VST3 and AUv2. The VST3 and
# AudioUnit SDKs are downloaded at configure time.

set(CLAP_WRAPPER_DOWNLOAD_DEPENDENCIES TRUE CACHE BOOL "" FORCE)
set(CLAP_WRAPPER_BUILD_TESTS           FALSE CACHE BOOL "" FORCE)
set(CLAP_SDK_ROOT "${clap_SOURCE_DIR}" CACHE PATH "" FORCE)

add_subdirectory(${clap_wrapper_SOURCE_DIR} ${clap_wrapper_BINARY_DIR}
   EXCLUDE_FROM_ALL)

# The standalone app plays through RtAudio. q_io already brings the same
# release clap-wrapper would download, and the two would clash, so the
# wrapper is handed q_io's.
if(TARGET rtaudio)
   add_library(base-sdk-rtaudio INTERFACE)
   target_link_libraries(base-sdk-rtaudio INTERFACE rtaudio)
endif()

###############################################################################
# q_plug: the CLAP adapter and public headers. The one translation unit that
# knows CLAP is compiled into every plugin that links this target.

add_library(q_plug OBJECT
   src/clap/base_plugin.cpp
   src/controller.cpp
   src/presenter.cpp
   src/header.cpp
)
# State and presets are JSON.
target_include_directories(q_plug PUBLIC include)
target_link_libraries(q_plug PUBLIC libq clap nlohmann_json::nlohmann_json)

###############################################################################
# GUI: Elements on Artist, native Quartz2D on macOS, Direct2D on Windows and
# Cairo on Linux. Elements uses q's infra when it finds the cycfi::infra
# target, so there is a single infra. Direct2D draws with
# system libraries only, so Windows needs neither vcpkg nor pkg-config.

if(NOT WIN32)
   find_package(PkgConfig)
   if(NOT PkgConfig_FOUND)
      message(FATAL_ERROR
         "q_plug: Artist needs pkg-config. On macOS: brew install pkg-config.")
   endif()
endif()

# A Linux plugin draws inside the host's X11 window, which Elements' GTK host
# cannot do, so its native X11 host is chosen before Elements defaults to GTK.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
   set(ELEMENTS_HOST_UI_LIBRARY "x11" CACHE STRING "gtk, cocoa or win32")
endif()

list(APPEND CMAKE_MODULE_PATH "${cycfi_elements_SOURCE_DIR}/cmake")
include(ElementsConfigCommon)
set(ELEMENTS_BUILD_EXAMPLES OFF)

# Each plugin compiles the host layer itself, so its view class carries the
# plugin's own name. See q_plug_add_host.
set(ELEMENTS_HOST_IN_CONSUMER ON)
# The drawing backend is Elements' choice: the platform's own unless one of
# ELEMENTS_SKIA, ELEMENTS_CAIRO, ELEMENTS_DIRECT2D or ELEMENTS_QUARTZ_2D is
# set.
add_subdirectory(${cycfi_elements_SOURCE_DIR} ${cycfi_elements_BINARY_DIR})

# Kept where q_plug_add_host and q_plug_add_resources find them from any
# directory, a plugin's outside Q included.
set_property(GLOBAL PROPERTY
   Q_PLUG_ELEMENTS_HOST_SOURCES "${ELEMENTS_HOST_SOURCES}")
set_property(GLOBAL PROPERTY
   Q_PLUG_ELEMENTS_SOURCE_DIR "${cycfi_elements_SOURCE_DIR}")

target_link_libraries(q_plug PUBLIC elements)
if(WIN32)
   target_sources(q_plug PRIVATE src/windows/host_view.cpp)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
   target_sources(q_plug PRIVATE src/linux/host_view.cpp)
endif()
if(APPLE)
   target_sources(q_plug PRIVATE src/macos/host_view.mm)
   # Elements and Artist leave the Apple frameworks to the application
   # (ElementsConfigApp.cmake); here the plugin modules are the application.
   target_link_libraries(q_plug PUBLIC
      "-framework AppKit" "-framework Foundation" "-framework QuartzCore"
      "-framework CoreGraphics" "-framework CoreText"
      "-framework CoreFoundation")
endif()

###############################################################################
# Tests and examples

include(cmake/q_plug_auv2_plist.cmake)
include(cmake/q_plug_formats.cmake)
include(cmake/q_plug_host.cmake)
include(cmake/q_plug_resources.cmake)
include(cmake/q_plug_tests.cmake)

# Examples and tests live beside q's own, in example/q_plug and
# test/q_plug, but are added from here so that the settings above reach
# them.

if(Q_BUILD_TEST)
   include(cmake/q_plug_validators.cmake)
   enable_testing()
endif()

if(Q_BUILD_EXAMPLES)
   add_subdirectory(${q_SOURCE_DIR}/example/q_plug
      ${CMAKE_CURRENT_BINARY_DIR}/example)
endif()

# After the examples: a plugin test links the example it drives.
if(Q_BUILD_TEST)
   add_subdirectory(${q_SOURCE_DIR}/test/q_plug
      ${CMAKE_CURRENT_BINARY_DIR}/test)
endif()

# At the end of the top-level directory, after every plugin, including one
# built outside Q whose own directory comes after this one.
cmake_language(DEFER DIRECTORY ${CMAKE_SOURCE_DIR}
   CALL q_plug_finalize_auv2_plists)
