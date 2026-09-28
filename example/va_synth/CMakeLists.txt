###############################################################################
#  Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the Boost Software License, Version 1.0.
#  [ https://www.boost.org/LICENSE_1_0.txt ]
###############################################################################

# Render the virtual analog clips the documentation plays.
add_executable(example_va_synth va_synth.cpp)
target_link_libraries(example_va_synth libq libqio cycfi::infra)
target_compile_definitions(example_va_synth PRIVATE
   MIDI_DIR="${CMAKE_CURRENT_SOURCE_DIR}/midi")
