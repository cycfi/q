###############################################################################
#  Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(q_test)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"
      OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}-ftemplate-backtrace-limit=0")
endif()

add_executable(
   q_test
   gen_sin.cpp
)

target_link_libraries(q_test libq io)



