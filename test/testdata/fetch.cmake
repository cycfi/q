###############################################################################
#  Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################
#
#  cmake -DNAME=<set> -DVERSION=<n> -DACCESS=<public|private>
#        -DSHA256=<hash> -DCACHE=<dir> -P fetch.cmake
#
#  Fetches one test data set into <CACHE>/<NAME>_v<VERSION>/ unless it is
#  already there. The zip is checked against SHA256 before it is unpacked,
#  and the set is only made visible, by a rename and a .complete marker,
#  once it is whole.
#
#  A set that cannot be fetched (offline, no credentials, not published
#  yet) prints a line starting "testdata unavailable", which the fixture
#  test reports as skipped. The tests that need the set skip in turn.
#
#  CYCFI_TESTDATA_URL replaces https://media.cycfi.com for public sets (a
#  mirror, or a file:// directory with the same layout).
#
###############################################################################
cmake_minimum_required(VERSION 3.22)

set(dir "${CACHE}/${NAME}_v${VERSION}")
set(marker "${dir}/.complete")

if (EXISTS "${marker}")
   file(READ "${marker}" have)
   string(STRIP "${have}" have)
   if (have STREQUAL SHA256)
      message(STATUS "testdata ${NAME}_v${VERSION}: cached in ${dir}")
      return()
   endif()
endif()

function(unavailable reason)
   message("testdata unavailable: ${NAME}_v${VERSION}: ${reason}")
   file(REMOVE "${zip}")
   file(REMOVE_RECURSE "${partial}")
endfunction()

file(MAKE_DIRECTORY "${CACHE}")
string(RANDOM LENGTH 8 tag)
set(zip "${CACHE}/${NAME}_v${VERSION}.${tag}.zip")
set(partial "${dir}.${tag}.partial")
set(object "${NAME}/${NAME}_v${VERSION}.zip")

# Download
if (ACCESS STREQUAL "public")
   set(base "https://media.cycfi.com")
   if (DEFINED ENV{CYCFI_TESTDATA_URL})
      set(base "$ENV{CYCFI_TESTDATA_URL}")
   endif()
   set(url "${base}/testdata/${object}")
   file(DOWNLOAD "${url}" "${zip}" STATUS status TIMEOUT 600)
   list(GET status 0 code)
   if (NOT code EQUAL 0)
      list(GET status 1 text)
      unavailable("${url}: ${text}")
      return()
   endif()
elseif (ACCESS STREQUAL "private")
   find_program(aws aws)
   if (NOT aws)
      unavailable("private set, and no aws command to fetch it")
      return()
   endif()
   execute_process(
      COMMAND "${aws}" s3 cp "s3://cycfi-testdata/${object}" "${zip}"
         --endpoint-url
         "https://24c350f8d000d722e6bcc12e3b03469a.r2.cloudflarestorage.com"
         --only-show-errors
      RESULT_VARIABLE code
      ERROR_VARIABLE text)
   if (NOT code EQUAL 0)
      string(STRIP "${text}" text)
      unavailable("s3://cycfi-testdata/${object}: ${text}")
      return()
   endif()
else()
   message(FATAL_ERROR "testdata ${NAME}: unknown access '${ACCESS}'")
endif()

# Check, unpack, publish
file(SHA256 "${zip}" got)
if (NOT got STREQUAL SHA256)
   unavailable("SHA-256 mismatch, got ${got}")
   return()
endif()

file(ARCHIVE_EXTRACT INPUT "${zip}" DESTINATION "${partial}")
file(WRITE "${partial}/.complete" "${SHA256}\n")
file(REMOVE_RECURSE "${dir}")
file(RENAME "${partial}" "${dir}")
file(REMOVE "${zip}")
message(STATUS "testdata ${NAME}_v${VERSION}: fetched into ${dir}")
