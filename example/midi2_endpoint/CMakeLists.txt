###############################################################################
#  Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the Boost Software License, Version 1.0.
#  [ https://www.boost.org/LICENSE_1_0.txt ]
###############################################################################
add_executable(example_midi2_endpoint midi2_endpoint.cpp)
target_link_libraries(example_midi2_endpoint
   libq libqio cycfi::infra q_example_common libremidi)
