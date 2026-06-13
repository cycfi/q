###############################################################################
#  Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the Boost Software License, Version 1.0.
#  [ https://www.boost.org/LICENSE_1_0.txt ]
###############################################################################

# Monitor the incoming MIDI stream and print the events.
add_executable(example_midi_monitor midi_monitor.cpp)
target_link_libraries(example_midi_monitor libq libqio cycfi::infra q_example_common)
