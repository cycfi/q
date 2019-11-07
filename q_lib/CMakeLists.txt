###############################################################################
#  Copyright (c) 2016-2019 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################
cmake_minimum_required(VERSION 3.5.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(libq)
set(q_root ${CMAKE_CURRENT_SOURCE_DIR})

###############################################################################
# Get rid of these warnings
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"
      OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}-Wno-missing-braces]")
endif()

###############################################################################
# Sources

set(Q_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/include)

file(GLOB_RECURSE Q_HEADERS include/*.hpp)

source_group("Header Files\\q"
   FILES
   ${Q_HEADERS}
)

###############################################################################
# The Library

add_library(libq INTERFACE)

target_include_directories(libq INTERFACE include/)


