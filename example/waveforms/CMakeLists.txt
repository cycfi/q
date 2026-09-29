###############################################################################
#  Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################

# Play the four bandwidth-limited oscillators (saw, square, pulse, triangle).
add_executable(example_waveforms waveforms.cpp)
target_link_libraries(example_waveforms libq libqio cycfi::infra)
