###############################################################################
#  Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.
#
#  Distributed under the Boost Software License, Version 1.0. (See accompanying
#  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
###############################################################################
cmake_minimum_required(VERSION 3.7.2)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

###############################################################################
project(q_example)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"
      OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}-ftemplate-backtrace-limit=0")
endif()

set(APP_SOURCES
   list_devices.cpp
)

foreach(sourcefile ${APP_SOURCES})
   string(REPLACE ".cpp" "" name ${sourcefile})
   add_executable(${name} ${sourcefile})
   target_link_libraries(${name} libq q_io infra)
endforeach(sourcefile ${APP_SOURCES})

