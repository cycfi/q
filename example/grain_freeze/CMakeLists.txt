###############################################################################
#  Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################

# Granular freeze: sustain a frozen moment as a smooth, breathing texture.
add_executable(example_grain_freeze grain_freeze.cpp)
target_link_libraries(example_grain_freeze libq libqio cycfi::infra)
target_compile_definitions(example_grain_freeze PRIVATE
   AUDIO_DIR="${CMAKE_CURRENT_SOURCE_DIR}/audio")
