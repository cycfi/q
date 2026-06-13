###############################################################################
#  Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the Boost Software License, Version 1.0.
#  [ https://www.boost.org/LICENSE_1_0.txt ]
###############################################################################

# Granular freeze: sustain a frozen moment as a smooth, breathing texture.
add_executable(example_grain_freeze grain_freeze.cpp)
target_link_libraries(example_grain_freeze libq libqio cycfi::infra)
target_compile_definitions(example_grain_freeze PRIVATE
   AUDIO_DIR="${CMAKE_CURRENT_SOURCE_DIR}/audio")
