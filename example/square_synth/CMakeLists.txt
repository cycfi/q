###############################################################################
#  Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################

# MIDI-controlled bandwidth-limited square wave synth with ADSR envelope.
add_executable(example_square_synth square_synth.cpp)
target_link_libraries(example_square_synth libq libqio cycfi::infra q_example_common)
