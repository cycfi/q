###############################################################################
#  Copyright (c) 2016-2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################
cmake_minimum_required(VERSION 3.16.0)

option(Q_BUILD_PLUG "build Q plugin library (fetches its dependencies)" OFF)

# q_plug's macOS deployment target, which must be set before project().
# Artist uses std::filesystem, unavailable in Apple's libc++ before 10.15;
# 11.0 is the floor Artist's own prebuilt bundles target.
if (Q_BUILD_PLUG AND NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
   set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0"
      CACHE STRING "Minimum macOS version")
endif()

project(q)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

option(Q_BUILD_EXAMPLES "build Q library examples" ON)
option(Q_BUILD_TEST "build Q library tests" ON)
option(Q_BUILD_IO "build Q IO library" ON)

###############################################################################
# Cycfi Infra
#
# infra is Cycfi-owned, so it ships as a git submodule: clone with
# --recurse-submodules and you can hack on infra in place. If the submodule is
# not checked out (e.g. a plain `git clone`), fall back to fetching it, so the
# build still works out of the box.

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/infra/CMakeLists.txt")
   add_subdirectory(infra)
else()
   FetchContent_Declare(
     cycfi_infra
     GIT_REPOSITORY https://github.com/cycfi/infra.git
     GIT_TAG 2024_MAY
     GIT_SUBMODULES_RECURSE ON
   )
   FetchContent_MakeAvailable(cycfi_infra)
endif()

###############################################################################
# q_lib

add_subdirectory(q_lib)

###############################################################################
# q_io

if (Q_BUILD_IO)
   add_subdirectory(q_io)
endif()

###############################################################################
# q_plug

if (Q_BUILD_PLUG)
   add_subdirectory(q_plug)
endif()

###############################################################################
# Examples and Tests

if (Q_BUILD_EXAMPLES)
   add_subdirectory(example)
endif()

if (Q_BUILD_TEST)
   add_subdirectory(test)
   enable_testing()
endif()

