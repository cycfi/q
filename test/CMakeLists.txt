###############################################################################
#  Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the Boost Software License, Version 1.0.
#  [ https://www.boost.org/LICENSE_1_0.txt ]
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

   best_lag.cpp
   bitset.cpp
   clip.cpp
   decibel.cpp
   delay.cpp
   grain.cpp
   interpolation.cpp
   ring_buffer.cpp
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
   svf.cpp
   ladder.cpp
   chamberlin_filter.cpp
   envelope_follower.cpp
   rms_envelope_follower.cpp
   peak_envelope_follower.cpp
   ar_envelope_follower.cpp
   fast_envelope_follower.cpp
   fast_ave_envelope_follower.cpp
   fast_rms_envelope_follower.cpp
   true_rms_envelope_follower.cpp
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

# Micro-benchmarks (build-only, not registered with add_test).
add_subdirectory(benchmark)

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

# Create results/golden output dir at configure time so tests can write to it
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/results/golden)

include(CTest)
add_test(NAME test_fft COMMAND test_fft)
add_test(NAME test_bitset COMMAND test_bitset)
add_test(NAME test_decibel COMMAND test_decibel)
add_test(NAME test_interpolation COMMAND test_interpolation)
add_test(NAME test_ring_buffer COMMAND test_ring_buffer)
add_test(NAME test_delay COMMAND test_delay)
add_test(NAME test_grain COMMAND test_grain)
add_test(NAME test_best_lag COMMAND test_best_lag)
add_test(NAME test_moving_sum COMMAND test_moving_sum)
add_test(NAME test_pitch COMMAND test_pitch)
add_test(NAME test_period_detector COMMAND test_period_detector)
add_test(NAME test_pitch_detector COMMAND test_pitch_detector)
add_test(NAME test_pitch_detector_ex COMMAND test_pitch_detector_ex)
add_test(NAME test_sin COMMAND test_sin)
add_test(NAME test_gen_envelope COMMAND test_gen_envelope)
add_test(NAME test_gen_adsr_envelope COMMAND test_gen_adsr_envelope)
add_test(NAME test_dynamics COMMAND test_dynamics)
add_test(NAME test_peak_envelope_follower COMMAND test_peak_envelope_follower)
add_test(NAME test_ar_envelope_follower COMMAND test_ar_envelope_follower)
add_test(NAME test_fast_envelope_follower COMMAND test_fast_envelope_follower)
add_test(NAME test_fast_ave_envelope_follower COMMAND test_fast_ave_envelope_follower)
add_test(NAME test_fast_rms_envelope_follower COMMAND test_fast_rms_envelope_follower)
add_test(NAME test_true_rms_envelope_follower COMMAND test_true_rms_envelope_follower)
add_test(NAME test_agc COMMAND test_agc)
add_test(NAME test_biquad_lp COMMAND test_biquad_lp)
add_test(NAME test_svf COMMAND test_svf)
add_test(NAME test_ladder COMMAND test_ladder)
add_test(NAME test_chamberlin_filter COMMAND test_chamberlin_filter)
add_test(NAME test_comb COMMAND test_comb)
add_test(NAME test_compressor_expander COMMAND test_compressor_expander)
add_test(NAME test_compressor_expander2 COMMAND test_compressor_expander2)
add_test(NAME test_compressor_ff_fb COMMAND test_compressor_ff_fb)
add_test(NAME test_dynamic_smoother COMMAND test_dynamic_smoother)
add_test(NAME test_envelope_follower COMMAND test_envelope_follower)
add_test(NAME test_moving_average2 COMMAND test_moving_average2)
add_test(NAME test_moving_maximum2 COMMAND test_moving_maximum2)
add_test(NAME test_peaks COMMAND test_peaks)
add_test(NAME test_rms_envelope_follower COMMAND test_rms_envelope_follower)
add_test(NAME test_signal_conditioner COMMAND test_signal_conditioner)
add_test(NAME test_signal_slope COMMAND test_signal_slope)
add_test(NAME test_slope COMMAND test_slope)
add_test(NAME test_zero_crossing COMMAND test_zero_crossing)
