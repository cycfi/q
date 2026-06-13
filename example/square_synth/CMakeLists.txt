###############################################################################
#  Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the Boost Software License, Version 1.0.
#  [ https://www.boost.org/LICENSE_1_0.txt ]
###############################################################################

# MIDI-controlled bandwidth-limited square wave synth with ADSR envelope.
add_executable(example_square_synth square_synth.cpp)
target_link_libraries(example_square_synth libq libqio cycfi::infra q_example_common)
