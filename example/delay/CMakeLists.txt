###############################################################################
#  Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the Boost Software License, Version 1.0.
#  [ https://www.boost.org/LICENSE_1_0.txt ]
###############################################################################

# Load an audio file and process it with a feedback delay.
add_executable(example_delay delay.cpp)
target_link_libraries(example_delay libq libqio cycfi::infra)
target_compile_definitions(example_delay PRIVATE
   AUDIO_DIR="${CMAKE_CURRENT_SOURCE_DIR}/audio")
