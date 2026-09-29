###############################################################################
#  Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################

# Synthesize a 440 Hz sine wave to the default audio output.
add_executable(example_sin_osc sin_osc.cpp)
target_link_libraries(example_sin_osc libq libqio cycfi::infra)
