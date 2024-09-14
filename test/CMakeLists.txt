###############################################################################
#  Copyright (c) 2014-2023 Joel de Guzman. All rights reserved.
#
#  Distributed under the Boost Software License, Version 1.0. (See accompanying
#  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
###############################################################################
cmake_minimum_required(VERSION 3.5.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

###############################################################################
project(q_test)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"
      OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}-ftemplate-backtrace-limit=0")
endif()

set(APP_SOURCES

   bitset.cpp
   decibel.cpp
   sin.cpp

   osc_basic_square.cpp
   osc_basic_saw.cpp
   osc_basic_pulse.cpp
   osc_basic_triangle.cpp
   osc_sin.cpp
   osc_square.cpp
   osc_pulse.cpp
   osc_saw.cpp
   osc_triangle.cpp

   gen_sin_cos.cpp
   gen_hamming.cpp
   gen_hann.cpp
   gen_blackman.cpp
   gen_exponential.cpp
   gen_linear.cpp
   gen_envelope.cpp
   gen_adsr_envelope.cpp
   gen_noise.cpp

   dynamics.cpp
   agc.cpp
   allpass.cpp
   biquad_lp.cpp
   envelope_follower.cpp
   rms_envelope_follower.cpp
   moving_average.cpp
   moving_average2.cpp
   moving_maximum.cpp
   moving_maximum2.cpp
   moving_sum.cpp
   comb.cpp
   compressor_expander.cpp
   compressor_expander2.cpp
   compressor_ff_fb.cpp
   peaks.cpp
   pitch_detector.cpp
   period_detector.cpp
   pitch_detector_ex.cpp
   fft.cpp
   signal_conditioner.cpp
   slope.cpp
   zero_crossing.cpp
   dynamic_smoother.cpp
   signal_slope.cpp
   pitch.cpp
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

include(CTest)
add_test(NAME test_fft COMMAND test_fft)
add_test(NAME test_bitset COMMAND test_bitset)
add_test(NAME test_decibel COMMAND test_decibel)
add_test(NAME test_moving_sum COMMAND test_moving_sum)
add_test(NAME test_pitch COMMAND test_pitch)
add_test(NAME test_period_detector COMMAND test_period_detector)
add_test(NAME test_pitch_detector COMMAND test_pitch_detector)
add_test(NAME test_pitch_detector_ex COMMAND test_pitch_detector_ex)
add_test(NAME test_sin COMMAND test_sin)
add_test(NAME test_gen_envelope COMMAND test_gen_envelope)
add_test(NAME test_gen_adsr_envelope COMMAND test_gen_adsr_envelope)
add_test(NAME test_dynamics COMMAND test_dynamics)
