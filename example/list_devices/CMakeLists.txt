###############################################################################
#  Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################

# Enumerate the available audio and MIDI devices.
add_executable(example_list_devices list_devices.cpp)
target_link_libraries(example_list_devices libq libqio cycfi::infra)
