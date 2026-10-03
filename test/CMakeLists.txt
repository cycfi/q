###############################################################################
#  Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################
cmake_minimum_required(VERSION 3.16.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

###############################################################################
project(q_test)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"
      OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftemplate-backtrace-limit=0")
endif()

set(APP_SOURCES

   utility/best_lag.cpp
   utility/bitset.cpp
   fx/clip.cpp
   support/decibel.cpp
   fx/delay.cpp
   fx/delta_gate.cpp
   fx/spectral_flux.cpp
   fx/fast_downsample.cpp
   synth/grain.cpp
   utility/interpolation.cpp
   midi/midi_processor.cpp
   midi/midi_parameters.cpp
   midi/midi_controllers.cpp
   midi/midi_byte_reader.cpp
   midi/midi_modes.cpp
   midi/midi_mpe.cpp
   midi/midi_mpe_spec.cpp
   midi/midi2_packet.cpp
   midi/midi2_messages.cpp
   midi/midi2_builders.cpp
   midi/midi2_scaling.cpp
   midi/midi2_translate.cpp
   midi/midi2_sysex.cpp
   midi/midi2_utility.cpp
   midi/midi2_flex.cpp
   midi/midi2_per_note.cpp
   midi/midi2_stream.cpp
   midi/midi_ci.cpp
   midi/midi_ci_initiator.cpp
   midi/midi_ci_profile_initiator.cpp
   midi/midi_ci_property_initiator.cpp
   midi/midi_ci_process.cpp
   midi/midi_ci_profiles.cpp
   midi/midi_ci_property.cpp
   midi/midi2_endpoint.cpp
   midi/midi2_inquiry.cpp
   midi/midi2_sysex_writer.cpp
   midi/midi2_sysex8_writer.cpp
   midi/midi2_endpoint_loopback.cpp
   q_io/midi_event_queue.cpp
   q_io/midi_convert.cpp
   q_io/midi_loopback.cpp
   q_io/midi2_loopback.cpp
   q_io/audio_file.cpp
   q_io/audio_stream.cpp
   utility/ring_buffer.cpp
   support/phase_iterator.cpp
   detail/sin.cpp

   synth/va/osc_basic_square.cpp
   synth/va/osc_basic_saw.cpp
   synth/va/osc_basic_pulse.cpp
   synth/va/osc_basic_triangle.cpp
   synth/osc_sin.cpp
   synth/va/osc_square.cpp
   synth/va/osc_pulse.cpp
   synth/va/osc_saw.cpp
   synth/va/osc_triangle.cpp
   synth/osc_sample_hold.cpp

   synth/gen/gen_sin_cos.cpp
   synth/gen/gen_hamming.cpp
   synth/gen/gen_hann.cpp
   synth/gen/gen_blackman.cpp
   synth/gen/gen_exponential.cpp
   synth/gen/gen_linear.cpp
   synth/gen/gen_envelope.cpp
   synth/gen/gen_adsr_envelope.cpp
   synth/gen/envelope_reconfig.cpp
   synth/gen/envelope_sustain.cpp
   synth/gen/envelope_copy.cpp
   midi/midi_file_reader.cpp
   synth/gen/envelope.cpp
   synth/gen/gen_noise.cpp

   synth/fm/fm_operator.cpp
   synth/fm/fm_routing.cpp
   synth/fm/dx_envelope_gen.cpp
   synth/gen/gen_lfo.cpp
   synth/fm/fm_algorithm.cpp
   synth/fm/fm_voice.cpp
   synth/fm/dx_patcher_voice.cpp
   synth/fm/dx_patcher.cpp
   synth/fm/dx_sysex.cpp

   fx/dynamics.cpp
   fx/agc.cpp
   fx/allpass.cpp
   fx/biquad_lp.cpp
   fx/svf.cpp
   fx/ladder.cpp
   detail/fast_math.cpp
   fx/ota_ladder.cpp
   synth/va/analog_osc.cpp
   synth/va/triangle_osc.cpp
   synth/va/sync.cpp
   fx/chamberlin_filter.cpp
   fx/lowpass.cpp
   fx/envelope_follower.cpp
   fx/rms_envelope_follower.cpp
   fx/peak_envelope_follower.cpp
   fx/ar_envelope_follower.cpp
   fx/fast_envelope_follower.cpp
   fx/fast_ave_envelope_follower.cpp
   fx/fast_rms_envelope_follower.cpp
   fx/true_rms_envelope_follower.cpp
   fx/moving_average.cpp
   fx/moving_average2.cpp
   fx/moving_average_filters.cpp
   fx/moving_maximum.cpp
   fx/moving_maximum2.cpp
   fx/moving_sum.cpp
   fx/moving_sum_ref.cpp
   fx/comb.cpp
   fx/compressor_expander.cpp
   fx/compressor_expander2.cpp
   fx/compressor_ff_fb.cpp
   fx/peaks.cpp
   fx/peak_picker.cpp
   pitch/pitch_detector.cpp
   pitch/period_detector.cpp
   pitch/pitch_detector_ex.cpp
   fft/fft.cpp
   support/bypassable.cpp
   fx/signal_conditioner.cpp
   fx/signal_conditioner_bypass.cpp
   fx/slope.cpp
   fx/zero_crossing.cpp
   fx/dynamic_smoother.cpp
   fx/signal_slope.cpp
   support/pitch.cpp
)

# The tests follow the include tree: midi/midi_ci.cpp tests q/midi/ci.hpp.
# Each is named for its file alone, test_midi_ci, and all of them build and
# run here, so the golden and result paths are the same for every test.
foreach(testsourcefile ${APP_SOURCES})
   get_filename_component(testname ${testsourcefile} NAME_WE)
   add_executable(test_${testname} ${testsourcefile})
   target_include_directories(test_${testname} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
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
add_test(NAME test_phase_iterator COMMAND test_phase_iterator)
add_test(NAME test_delay COMMAND test_delay)
add_test(NAME test_delta_gate COMMAND test_delta_gate)
add_test(NAME test_spectral_flux COMMAND test_spectral_flux)
add_test(NAME test_fast_downsample COMMAND test_fast_downsample)
add_test(NAME test_grain COMMAND test_grain)
add_test(NAME test_midi_processor COMMAND test_midi_processor)
add_test(NAME test_midi_parameters COMMAND test_midi_parameters)
add_test(NAME test_midi_controllers COMMAND test_midi_controllers)
add_test(NAME test_midi_byte_reader COMMAND test_midi_byte_reader)
add_test(NAME test_midi_modes COMMAND test_midi_modes)
add_test(NAME test_midi_mpe COMMAND test_midi_mpe)
add_test(NAME test_midi_mpe_spec COMMAND test_midi_mpe_spec)
add_test(NAME test_midi2_packet COMMAND test_midi2_packet)
add_test(NAME test_midi2_messages COMMAND test_midi2_messages)
add_test(NAME test_midi2_builders COMMAND test_midi2_builders)
add_test(NAME test_midi2_scaling COMMAND test_midi2_scaling)
add_test(NAME test_midi2_translate COMMAND test_midi2_translate)
add_test(NAME test_midi2_sysex COMMAND test_midi2_sysex)
add_test(NAME test_midi2_utility COMMAND test_midi2_utility)
add_test(NAME test_midi2_flex COMMAND test_midi2_flex)
add_test(NAME test_midi2_per_note COMMAND test_midi2_per_note)
add_test(NAME test_midi2_stream COMMAND test_midi2_stream)
add_test(NAME test_midi_ci COMMAND test_midi_ci)
add_test(NAME test_midi_ci_initiator COMMAND test_midi_ci_initiator)
add_test(NAME test_midi_ci_profile_initiator COMMAND test_midi_ci_profile_initiator)
add_test(NAME test_midi_ci_property_initiator COMMAND test_midi_ci_property_initiator)
add_test(NAME test_midi_ci_process COMMAND test_midi_ci_process)
add_test(NAME test_midi_ci_profiles COMMAND test_midi_ci_profiles)
add_test(NAME test_midi_ci_property COMMAND test_midi_ci_property)
add_test(NAME test_midi2_endpoint COMMAND test_midi2_endpoint)
add_test(NAME test_midi2_inquiry COMMAND test_midi2_inquiry)
add_test(NAME test_midi2_sysex_writer COMMAND test_midi2_sysex_writer)
add_test(NAME test_midi2_sysex8_writer COMMAND test_midi2_sysex8_writer)
add_test(NAME test_midi2_endpoint_loopback COMMAND test_midi2_endpoint_loopback)
add_test(NAME test_midi_event_queue COMMAND test_midi_event_queue)
add_test(NAME test_midi_convert COMMAND test_midi_convert)
add_test(NAME test_midi_loopback COMMAND test_midi_loopback)
add_test(NAME test_midi2_loopback COMMAND test_midi2_loopback)
add_test(NAME test_audio_file COMMAND test_audio_file)
add_test(NAME test_audio_stream COMMAND test_audio_stream)
add_test(NAME test_best_lag COMMAND test_best_lag)
add_test(NAME test_moving_sum COMMAND test_moving_sum)
add_test(NAME test_moving_sum_ref COMMAND test_moving_sum_ref)
add_test(NAME test_pitch COMMAND test_pitch)
add_test(NAME test_period_detector COMMAND test_period_detector)
add_test(NAME test_pitch_detector COMMAND test_pitch_detector)
add_test(NAME test_pitch_detector_ex COMMAND test_pitch_detector_ex)
add_test(NAME test_sin COMMAND test_sin)
add_test(NAME test_gen_envelope COMMAND test_gen_envelope)
add_test(NAME test_gen_adsr_envelope COMMAND test_gen_adsr_envelope)
add_test(NAME test_envelope_reconfig COMMAND test_envelope_reconfig)
add_test(NAME test_envelope_sustain COMMAND test_envelope_sustain)
add_test(NAME test_envelope_copy COMMAND test_envelope_copy)
add_test(NAME test_midi_file_reader COMMAND test_midi_file_reader)
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
add_test(NAME test_fast_math COMMAND test_fast_math)
add_test(NAME test_ota_ladder COMMAND test_ota_ladder)
add_test(NAME test_analog_osc COMMAND test_analog_osc)
add_test(NAME test_triangle_osc COMMAND test_triangle_osc)
add_test(NAME test_sync COMMAND test_sync)
add_test(NAME test_chamberlin_filter COMMAND test_chamberlin_filter)
add_test(NAME test_lowpass COMMAND test_lowpass)
add_test(NAME test_comb COMMAND test_comb)
add_test(NAME test_compressor_expander COMMAND test_compressor_expander)
add_test(NAME test_compressor_expander2 COMMAND test_compressor_expander2)
add_test(NAME test_compressor_ff_fb COMMAND test_compressor_ff_fb)
add_test(NAME test_dynamic_smoother COMMAND test_dynamic_smoother)
add_test(NAME test_envelope COMMAND test_envelope)
add_test(NAME test_envelope_follower COMMAND test_envelope_follower)
add_test(NAME test_moving_average2 COMMAND test_moving_average2)
add_test(NAME test_moving_average_filters COMMAND test_moving_average_filters)
add_test(NAME test_moving_maximum2 COMMAND test_moving_maximum2)
add_test(NAME test_peaks COMMAND test_peaks)
add_test(NAME test_peak_picker COMMAND test_peak_picker)
add_test(NAME test_rms_envelope_follower COMMAND test_rms_envelope_follower)
add_test(NAME test_bypassable COMMAND test_bypassable)
add_test(NAME test_signal_conditioner COMMAND test_signal_conditioner)
add_test(NAME test_signal_conditioner_bypass COMMAND test_signal_conditioner_bypass)
add_test(NAME test_signal_slope COMMAND test_signal_slope)
add_test(NAME test_slope COMMAND test_slope)
add_test(NAME test_zero_crossing COMMAND test_zero_crossing)
add_test(NAME test_fm_operator COMMAND test_fm_operator)
add_test(NAME test_fm_routing COMMAND test_fm_routing)
add_test(NAME test_dx_envelope_gen COMMAND test_dx_envelope_gen)
add_test(NAME test_gen_lfo COMMAND test_gen_lfo)
add_test(NAME test_fm_algorithm COMMAND test_fm_algorithm)
add_test(NAME test_fm_voice COMMAND test_fm_voice)
add_test(NAME test_dx_patcher_voice COMMAND test_dx_patcher_voice)
add_test(NAME test_dx_patcher COMMAND test_dx_patcher)
add_test(NAME test_dx_sysex COMMAND test_dx_sysex)
add_test(NAME test_osc_sample_hold COMMAND test_osc_sample_hold)
