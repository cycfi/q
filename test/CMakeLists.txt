###############################################################################
#  Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(q_test)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"
      OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}-ftemplate-backtrace-limit=0")
endif()

set(APP_SOURCES
   gen_sin.cpp
   zero_cross.cpp
   onset_detection.cpp
   peak_detection.cpp
   autocorrelation.cpp
)

foreach(testsourcefile ${APP_SOURCES})
   string(REPLACE ".cpp" "" testname ${testsourcefile})
    add_executable(${testname} ${testsourcefile})
    target_link_libraries(${testname} libq io)
endforeach(testsourcefile ${APP_SOURCES})

# Copy test files to the binary dir
file(
  COPY audio_files
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Copy results folder to the binary dir
file(
  COPY results
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})