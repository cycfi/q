###############################################################################
#  Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################

# A whammy bar on the SOLA pitch shifter: a sample played against a MIDI
# pitch bend track, chosen from a menu or given as files, on the default
# audio device.
add_executable(example_whammy whammy.cpp)
target_link_libraries(example_whammy libq libqio cycfi::infra)
target_compile_definitions(example_whammy PRIVATE
   AUDIO_DIR="${CMAKE_CURRENT_SOURCE_DIR}/audio"
   MIDI_DIR="${CMAKE_CURRENT_SOURCE_DIR}/midi")
