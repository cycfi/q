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
project(q_test)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"
      OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}-ftemplate-backtrace-limit=0")
endif()

set(APP_SOURCES
   gen_basic_square.cpp
   gen_basic_saw.cpp
   gen_basic_pulse.cpp
   gen_basic_triangle.cpp
   gen_sin.cpp
   gen_square.cpp
   gen_pulse.cpp
   gen_saw.cpp
   gen_triangle.cpp

   zero_cross.cpp
   envelope_follower.cpp
   moving_average.cpp
   compressor_expander.cpp
   compressor_expander2.cpp
   onset_detector.cpp
   peak_detection.cpp
   pitch_detection.cpp
   pitch_detection2.cpp
   pitch_follower.cpp
)

foreach(testsourcefile ${APP_SOURCES})
   string(REPLACE ".cpp" "" testname ${testsourcefile})
   add_executable(test_${testname} ${testsourcefile})
   target_link_libraries(test_${testname} libq q_io infra)
endforeach(testsourcefile ${APP_SOURCES})

# Copy test files to the binary dir
file(
  COPY audio_files
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Copy results folder to the binary dir
file(
  COPY results
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})