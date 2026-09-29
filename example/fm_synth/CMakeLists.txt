###############################################################################
#  Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################

# Render the FM clips the documentation plays.
add_executable(example_fm_synth fm_synth.cpp)
target_link_libraries(example_fm_synth libq libqio cycfi::infra)
target_compile_definitions(example_fm_synth PRIVATE
   MIDI_DIR="${CMAKE_CURRENT_SOURCE_DIR}/midi")
