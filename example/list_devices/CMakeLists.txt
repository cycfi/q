###############################################################################
#  Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the Boost Software License, Version 1.0.
#  [ https://www.boost.org/LICENSE_1_0.txt ]
###############################################################################

# Enumerate the available audio and MIDI devices.
add_executable(example_list_devices list_devices.cpp)
target_link_libraries(example_list_devices libq libqio cycfi::infra)
