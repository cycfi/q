###############################################################################
#  Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################

# Monitor the incoming MIDI stream and print the events.
add_executable(example_midi_monitor midi_monitor.cpp)
target_link_libraries(example_midi_monitor libq libqio cycfi::infra q_example_common)
