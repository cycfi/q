###############################################################################
#  Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################
#
#  q_testdata(<test> <set>...)
#
#  Gives a registered test the data sets it needs. Each set, pinned in
#  sets.txt, gets one fixture test, testdata_<set>, that fetches it into
#  the per-user cache the first time a test needing it runs (fetch.cmake).
#  The test is told where the set is by the environment variable
#  Q_TESTDATA_<SET>, and exits with 77 to skip when the set is missing.
#
#  The cache is $CYCFI_TESTDATA, else ~/.cache/cycfi/testdata (on Windows,
#  %LOCALAPPDATA%/cycfi/testdata), shared by every build and every clone.
#
###############################################################################

set(Q_TESTDATA_SETS "${CMAKE_CURRENT_LIST_DIR}/sets.txt")
set(Q_TESTDATA_FETCH "${CMAKE_CURRENT_LIST_DIR}/fetch.cmake")

if (DEFINED ENV{CYCFI_TESTDATA})
   set(Q_TESTDATA_CACHE "$ENV{CYCFI_TESTDATA}")
elseif (WIN32)
   file(TO_CMAKE_PATH "$ENV{LOCALAPPDATA}/cycfi/testdata" Q_TESTDATA_CACHE)
else()
   set(Q_TESTDATA_CACHE "$ENV{HOME}/.cache/cycfi/testdata")
endif()

file(STRINGS "${Q_TESTDATA_SETS}" _q_testdata_lines REGEX "^[a-z0-9_]+ ")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
   "${Q_TESTDATA_SETS}")

function(_q_testdata_fixture set)
   foreach(line ${_q_testdata_lines})
      string(REPLACE " " ";" fields "${line}")
      list(GET fields 0 name)
      if (name STREQUAL set)
         list(GET fields 1 version)
         list(GET fields 2 access)
         list(GET fields 3 sha256)
         if (NOT TEST testdata_${set})
            add_test(NAME testdata_${set}
               COMMAND ${CMAKE_COMMAND}
                  -DNAME=${set} -DVERSION=${version} -DACCESS=${access}
                  -DSHA256=${sha256} -DCACHE=${Q_TESTDATA_CACHE}
                  -P ${Q_TESTDATA_FETCH})
            set_tests_properties(testdata_${set} PROPERTIES
               FIXTURES_SETUP testdata_${set}
               SKIP_REGULAR_EXPRESSION "testdata unavailable"
               TIMEOUT 900)
         endif()
         set(_q_testdata_dir
            "${Q_TESTDATA_CACHE}/${set}_v${version}" PARENT_SCOPE)
         return()
      endif()
   endforeach()
   message(FATAL_ERROR "q_testdata: no set '${set}' in ${Q_TESTDATA_SETS}")
endfunction()

function(q_testdata test)
   foreach(set ${ARGN})
      _q_testdata_fixture(${set})
      string(TOUPPER "${set}" var)
      set_property(TEST ${test} APPEND PROPERTY
         FIXTURES_REQUIRED testdata_${set})
      set_property(TEST ${test} APPEND PROPERTY
         ENVIRONMENT "Q_TESTDATA_${var}=${_q_testdata_dir}")
   endforeach()
   set_tests_properties(${test} PROPERTIES SKIP_RETURN_CODE 77)
endfunction()
