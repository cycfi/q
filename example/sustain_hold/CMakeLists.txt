###############################################################################
#  Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the Boost Software License, Version 1.0.
#  [ https://www.boost.org/LICENSE_1_0.txt ]
###############################################################################

# Interactive monophonic infinite sustain (SPACE: hold/release, q: quit).
add_executable(example_sustain_hold sustain_hold.cpp)
target_link_libraries(example_sustain_hold libq libqio cycfi::infra q_example_common)
target_compile_definitions(example_sustain_hold PRIVATE
   AUDIO_DIR="${CMAKE_CURRENT_SOURCE_DIR}/audio")
