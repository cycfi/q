###############################################################################
#  Copyright (c) 2016-2023 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################
cmake_minimum_required(VERSION 3.16.0)

project(q)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

option(Q_BUILD_EXAMPLES "build Q library examples" ON)
option(Q_BUILD_TEST "build Q library tests" ON)
option(Q_BUILD_IO "build Q IO library" ON)

###############################################################################
# Cycfi Infra

FetchContent_Declare(
  cycfi_infra
  GIT_REPOSITORY https://github.com/cycfi/infra.git
  GIT_TAG 2024_MAY
  GIT_SUBMODULES_RECURSE ON
)

FetchContent_MakeAvailable(cycfi_infra)

###############################################################################
# q_lib

add_subdirectory(q_lib)

###############################################################################
# q_io

if (Q_BUILD_IO)
   add_subdirectory(q_io)
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

