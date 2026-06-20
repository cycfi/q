###############################################################################
#  Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the Boost Software License, Version 1.0.
#  [ https://www.boost.org/LICENSE_1_0.txt ]
###############################################################################

# Play the four bandwidth-limited oscillators (saw, square, pulse, triangle).
add_executable(example_waveforms waveforms.cpp)
target_link_libraries(example_waveforms libq libqio cycfi::infra)
