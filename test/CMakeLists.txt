###############################################################################
#  Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.
#
#  Distributed under the Boost Software License, Version 1.0. (See accompanying
#  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
###############################################################################

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

###############################################################################
# Boost

set(Boost_USE_STATIC_LIBS ON)
find_package(
  Boost 1.61 REQUIRED
  COMPONENTS filesystem system)

include_directories(${Boost_INCLUDE_DIRS})

add_definitions("-DBOOST_ALL_NO_LIB") # disable auto-linking

set(BOOST_CMAKE_ARGS )
if (DEFINED BOOST_ROOT)
   set(BOOST_CMAKE_ARGS ${BOOST_CMAKE_ARGS} "-DBOOST_ROOT=${BOOST_ROOT}")
endif()
if (DEFINED BOOST_INCLUDEDIR)
   set(BOOST_CMAKE_ARGS ${BOOST_CMAKE_ARGS}
      "-DBOOST_INCLUDEDIR=${BOOST_INCLUDEDIR}")
endif()
if (DEFINED BOOST_LIBRARYDIR)
   set(BOOST_CMAKE_ARGS ${BOOST_CMAKE_ARGS}
      "-DBOOST_LIBRARYDIR=${BOOST_LIBRARYDIR}")
endif()

###############################################################################
project(q_test)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"
      OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}-ftemplate-backtrace-limit=0")
endif()

set(APP_SOURCES
   gen_sin.cpp
   gen_harmonics.cpp
   zero_cross.cpp
   onset_detection.cpp
   peak_detection.cpp
   autocorrelation.cpp
   autocorrelation2.cpp
   pitch_detection.cpp
   pitch_detection2.cpp
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