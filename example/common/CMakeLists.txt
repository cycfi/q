###############################################################################
#  Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################

# Shared example helpers: device pickers (example.hpp) and the
# non-blocking keyboard utilities (keyboard.hpp).
add_library(q_example_common INTERFACE)
target_include_directories(q_example_common INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
