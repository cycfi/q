###############################################################################
#  Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################

# Live feedback delay: audio interface input (channel 1) to the outputs.
add_executable(example_io_delay io_delay.cpp)
target_link_libraries(example_io_delay libq libqio cycfi::infra q_example_common)
