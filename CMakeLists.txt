###############################################################################
#  Copyright (c) 2016-2018 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################
cmake_minimum_required(VERSION 3.7.2)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT DEFINED INTEGRE_COMMON_ROOT)
  set(INTEGRE_COMMON_ROOT ../infra)
endif()

add_subdirectory(${INTEGRE_COMMON_ROOT} "${CMAKE_CURRENT_BINARY_DIR}/infra")

add_subdirectory(q_lib)
add_subdirectory(io)
add_subdirectory(test)

