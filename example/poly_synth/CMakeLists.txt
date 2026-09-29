###############################################################################
#  Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################

# MIDI-controlled polyphonic sawtooth synth (voice pool + ADSR-swept svf).
add_executable(example_poly_synth poly_synth.cpp)
target_link_libraries(example_poly_synth libq libqio cycfi::infra q_example_common)
