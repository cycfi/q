###############################################################################
#  Copyright (c) 2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################
# Fetches the format validators the ctest suite runs, so a fresh machine
# needs nothing beyond CMake, a compiler and network. Modelled on Artist's
# SkiaPrebuilt.cmake: pinned version, sha256 verified, downloaded once into
# a per-user cache. A CLAP_VALIDATOR or PLUGINVAL given on the command line
# always wins. If a download fails the variable is left unset and the tests
# that need it skip. auval ships with macOS and needs nothing.
#
# Only macOS assets are pinned so far. Elsewhere, and with downloads off, the
# validators are located with find_program. To move to a newer release, set
# both the version and the sha256 of its archive.

option(QPLUG_DOWNLOAD_VALIDATORS
   "Download pinned clap-validator and pluginval for the tests" ON)

set(QPLUG_CLAP_VALIDATOR_VERSION "0.3.2" CACHE STRING
   "clap-validator release to download")
set(QPLUG_CLAP_VALIDATOR_SHA256
   "3750f3729adfd8489f2b29019f7f2ed65ba71bf9d5049735f6a2ca0fccb18ffd"
   CACHE STRING "sha256 of the clap-validator macOS archive")

set(QPLUG_PLUGINVAL_VERSION "v1.0.4" CACHE STRING
   "pluginval release to download")
set(QPLUG_PLUGINVAL_SHA256
   "3c4c533bda0c5059eea3ddaea752d757ee2025041f0f47e6bcb0e87f6082b29f"
   CACHE STRING "sha256 of the pluginval macOS archive")

if(DEFINED QPLUG_VALIDATOR_DIR)
   set(_cache "${QPLUG_VALIDATOR_DIR}")
elseif(DEFINED ENV{XDG_CACHE_HOME})
   set(_cache "$ENV{XDG_CACHE_HOME}/cycfi/qplug-validators")
elseif(DEFINED ENV{HOME})
   set(_cache "$ENV{HOME}/.cache/cycfi/qplug-validators")
elseif(DEFINED ENV{LOCALAPPDATA})
   set(_cache "$ENV{LOCALAPPDATA}/cycfi/qplug-validators")
else()
   set(_cache "${CMAKE_BINARY_DIR}/validators")
endif()

# _qplug_fetch(<var> <name> <url> <sha256> <binary>)
# Downloads and extracts <url> once into the cache, then sets <var> to the
# path of <binary> inside it. On any failure <var> is left alone.
function(_qplug_fetch var name url sha256 binary)
   set(dest "${_cache}/${name}")
   set(exe "${dest}/${binary}")

   if(NOT EXISTS "${exe}")
      get_filename_component(archive "${url}" NAME)
      set(file "${dest}/${archive}")
      file(MAKE_DIRECTORY "${dest}")
      message(STATUS "qplug: downloading ${name}")
      file(DOWNLOAD "${url}" "${file}"
         EXPECTED_HASH "SHA256=${sha256}" STATUS st)
      list(GET st 0 code)
      if(NOT code EQUAL 0)
         file(REMOVE "${file}")
         message(WARNING
            "qplug: could not fetch ${name} (${st}); its tests will skip. "
            "Point ${var} at a local copy, or QPLUG_DOWNLOAD_VALIDATORS=OFF.")
         return()
      endif()
      file(ARCHIVE_EXTRACT INPUT "${file}" DESTINATION "${dest}")
      file(REMOVE "${file}")
      # Archives normally carry the executable bit; make sure, once, and do
      # not fail configure over it.
      if(NOT WIN32 AND EXISTS "${exe}")
         execute_process(COMMAND chmod +x "${exe}" RESULT_VARIABLE rc)
         if(NOT rc EQUAL 0)
            message(WARNING "qplug: could not chmod +x ${exe}")
         endif()
      endif()
   endif()

   if(NOT EXISTS "${exe}")
      message(WARNING
         "qplug: ${name} archive did not contain ${binary}; tests will skip.")
      return()
   endif()

   set(${var} "${exe}" CACHE FILEPATH "Path to ${name}" FORCE)
   message(STATUS "qplug: using ${name} at ${exe}")
endfunction()

if(APPLE)
   set(_cv_ver "${QPLUG_CLAP_VALIDATOR_VERSION}")
   string(CONCAT _cv_url
      "https://github.com/free-audio/clap-validator/releases/download/"
      "${_cv_ver}/clap-validator-${_cv_ver}-macos-universal.tar.gz")
   set(_pv_ver "${QPLUG_PLUGINVAL_VERSION}")
   string(CONCAT _pv_url
      "https://github.com/Tracktion/pluginval/releases/download/"
      "${_pv_ver}/pluginval_macOS.zip")

   if(NOT CLAP_VALIDATOR AND QPLUG_DOWNLOAD_VALIDATORS)
      _qplug_fetch(CLAP_VALIDATOR "clap-validator-${_cv_ver}"
         "${_cv_url}" "${QPLUG_CLAP_VALIDATOR_SHA256}"
         "binaries/clap-validator")
   endif()
   if(NOT PLUGINVAL AND QPLUG_DOWNLOAD_VALIDATORS)
      _qplug_fetch(PLUGINVAL "pluginval-${_pv_ver}"
         "${_pv_url}" "${QPLUG_PLUGINVAL_SHA256}"
         "pluginval.app/Contents/MacOS/pluginval")
   endif()
endif()

if(NOT CLAP_VALIDATOR)
   find_program(CLAP_VALIDATOR clap-validator
      PATHS "$ENV{HOME}/.cargo/bin" /usr/local/bin
      DOC "Path to clap-validator")
endif()
if(NOT PLUGINVAL)
   find_program(PLUGINVAL pluginval
      PATHS /Applications/pluginval.app/Contents/MacOS
      DOC "Path to pluginval")
endif()
