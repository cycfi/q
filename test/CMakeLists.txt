###############################################################################
#  Copyright (c) 2014-2021 Joel de Guzman. All rights reserved.
#
#  Distributed under the Boost Software License, Version 1.0. (See accompanying
#  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
###############################################################################
cmake_minimum_required(VERSION 3.5.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

###############################################################################
project(q_test)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"
      OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftemplate-backtrace-limit=0")
endif()

set(APP_SOURCES

   bitset.cpp
   decibel.cpp

   gen_basic_square.cpp
   gen_basic_saw.cpp
   gen_basic_pulse.cpp
   gen_basic_triangle.cpp
   gen_sin.cpp
   gen_square.cpp
   gen_pulse.cpp
   gen_saw.cpp
   gen_triangle.cpp

   dynamics.cpp
   agc.cpp
   allpass.cpp
   biquad_lp.cpp
   envelope_follower.cpp
   rms_envelope_follower.cpp
   moving_average1.cpp
   moving_average2.cpp
   moving_maximum1.cpp
   moving_maximum2.cpp
   comb.cpp
   compressor_expander1.cpp
   compressor_expander2.cpp
   compressor_ff_fb.cpp
   peak_detector.cpp
   period_detector.cpp
   pitch_detector1.cpp
   pitch_detector2.cpp
   dual_pitch_detector.cpp
   fft.cpp
)

foreach(testsourcefile ${APP_SOURCES})
   string(REPLACE ".cpp" "" testname ${testsourcefile})
   add_executable(test_${testname} ${testsourcefile})
   target_link_libraries(test_${testname} libq libqio)
endforeach(testsourcefile ${APP_SOURCES})

# Copy test files to the binary dir
file(
  COPY audio_files
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Copy results folder to the binary dir
file(
  COPY results
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Copy golden folder to the binary dir
file(
  COPY golden
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})