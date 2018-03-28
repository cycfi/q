###############################################################################
#  Copyright (c) 2016-2017 Joel de Guzman
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(q_aux)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"
      OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}-ftemplate-backtrace-limit=0")
endif()

add_executable(
   q_aux
   main.cpp
)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
   target_link_libraries(q_aux libq ${CMAKE_SOURCE_DIR}/aux/external/lib/osx/libsndfile.a)
endif()

target_include_directories(q_aux PRIVATE external/include/)


