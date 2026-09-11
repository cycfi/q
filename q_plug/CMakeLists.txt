###############################################################################
#  Copyright (c) 2019-2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################
cmake_minimum_required(VERSION 3.21)

# macOS deployment target must be set before project(). Artist uses
# std::filesystem, unavailable in Apple's libc++ before 10.15; 11.0 is the
# floor Artist's own prebuilt bundles target.
if(NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0"
        CACHE STRING "Minimum macOS version")
endif()

project(qplug VERSION 0.1.1 LANGUAGES C CXX)

if(APPLE)
    enable_language(OBJC)
    enable_language(OBJCXX)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# The MSVC runtime, chosen here for the whole build because every
# dependency picks one otherwise: Elements, Artist and libunibreak set the
# static runtime on their targets, and clap-wrapper falls back to it when
# nobody has chosen. Mixing the two does not link. Static also suits a
# plugin, which is loaded into a host that owes it no redistributable.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

option(QPLUG_BUILD_EXAMPLES "build qplug examples" ON)
option(QPLUG_BUILD_TEST "build qplug validation tests" ON)

set(QPLUG_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

# The formats a plugin is built in. AUv2 is macOS only; CLAP and VST3 are
# everywhere. Examples pass this to make_clapfirst_plugins.
set(QPLUG_PLUGIN_FORMATS CLAP VST3)
if(APPLE)
   list(APPEND QPLUG_PLUGIN_FORMATS AUV2)
endif()

###############################################################################
# Submodules: fetch any that are missing, so a clone without --recursive
# still configures. Same device as Artist's CMakeLists.

if(EXISTS "${QPLUG_ROOT}/.git")
   find_package(Git REQUIRED)
   function(qplug_submodule_check dir)
      file(GLOB present "${QPLUG_ROOT}/${dir}/*")
      if(NOT present)
         message(STATUS "qplug: fetching submodule ${dir}")
         execute_process(
            COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
               -- ${dir}
            WORKING_DIRECTORY ${QPLUG_ROOT}
            COMMAND_ERROR_IS_FATAL ANY
         )
      endif()
   endfunction()
   foreach(m lib/clap lib/clap-wrapper lib/q lib/infra lib/elements)
      qplug_submodule_check(${m})
   endforeach()
endif()

###############################################################################
# clap-wrapper: projects the native CLAP to VST3 and AUv2. The VST3 and
# AudioUnit SDKs are downloaded at configure time. CLAP and the wrapper are
# submodules on Cycfi forks that carry bug fixes only, each one a pull
# request upstream; features never go in there.

set(CLAP_WRAPPER_DOWNLOAD_DEPENDENCIES TRUE CACHE BOOL "" FORCE)
set(CLAP_WRAPPER_BUILD_TESTS           FALSE CACHE BOOL "" FORCE)
set(CLAP_SDK_ROOT "${QPLUG_ROOT}/lib/clap" CACHE PATH "" FORCE)

add_subdirectory(lib/clap-wrapper EXCLUDE_FROM_ALL)

###############################################################################
# qplug: the CLAP adapter and public headers. The one translation unit that
# knows CLAP is compiled into every plugin that links this target.

add_library(qplug OBJECT
   lib/src/clap/base_plugin.cpp
   lib/src/controller.cpp
   lib/src/presenter.cpp
   lib/src/header.cpp
)
# lib/json is nlohmann's single header, vendored as released; state and
# presets are JSON.
target_include_directories(qplug PUBLIC
   lib/include
   lib/q/q_lib/include
   lib/json/include
)
target_link_libraries(qplug PUBLIC clap)

###############################################################################
# GUI: Elements on Artist, native Quartz2D on macOS and Cairo elsewhere.
# Elements brings its own copy of infra, so lib/infra is not on the include
# path here, to keep a single infra. Artist asks for pkg-config at configure
# time on every platform, and on Windows pkgconf comes from vcpkg with the
# Cairo port.

find_package(PkgConfig)
if(NOT PkgConfig_FOUND)
   message(FATAL_ERROR
      "qplug: Artist needs pkg-config. On macOS: brew install pkg-config.")
endif()

list(APPEND CMAKE_MODULE_PATH "${QPLUG_ROOT}/lib/elements/cmake")
include(ElementsConfigCommon)
set(ELEMENTS_BUILD_EXAMPLES OFF)

# Each plugin compiles the host layer itself, so its view class carries the
# plugin's own name. See qplug_add_host.
set(ELEMENTS_HOST_IN_CONSUMER ON)
if(NOT APPLE)
   set(ELEMENTS_CAIRO ON)
endif()
add_subdirectory(lib/elements)

target_link_libraries(qplug PUBLIC elements)
if(WIN32)
   target_sources(qplug PRIVATE lib/src/windows/host_view.cpp)
endif()
if(APPLE)
   target_sources(qplug PRIVATE lib/src/macos/host_view.mm)
   # Elements and Artist leave the Apple frameworks to the application
   # (ElementsConfigApp.cmake); here the plugin modules are the application.
   target_link_libraries(qplug PUBLIC
      "-framework AppKit" "-framework Foundation" "-framework QuartzCore"
      "-framework CoreGraphics" "-framework CoreText"
      "-framework CoreFoundation")
endif()

###############################################################################
# Tests and examples

include(cmake/qplug_auv2_plist.cmake)
include(cmake/qplug_host.cmake)
include(cmake/qplug_resources.cmake)
include(cmake/qplug_tests.cmake)

if(QPLUG_BUILD_TEST)
   include(cmake/qplug_validators.cmake)
   enable_testing()
endif()

if(QPLUG_BUILD_EXAMPLES)
   add_subdirectory(examples)
endif()

# After the examples: a plugin test links the example it drives.
if(QPLUG_BUILD_TEST)
   add_subdirectory(test)
endif()

qplug_finalize_auv2_plists()
