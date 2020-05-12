###############################################################################
#  Copyright (c) 2016-2020 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################
cmake_minimum_required(VERSION 3.5.1)

project(cycfi_q)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(Q_BUILD_EXAMPLES "build Q library examples" ON)
option(Q_BUILD_TEST "build Q library examples" ON)

add_subdirectory(q_lib)
add_subdirectory(q_io)

if (Q_BUILD_EXAMPLES)
   add_subdirectory(example)
endif()

if (Q_BUILD_TEST)
   add_subdirectory(test)
endif()

