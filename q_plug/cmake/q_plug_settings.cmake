###############################################################################
#  Copyright (c) 2019-2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################
# q_plug_settings()
#
# The build settings every directory that compiles a plugin needs. They are
# directory scoped, so q_plug's own do not reach a plugin built outside Q;
# a plugin's top-level CMakeLists calls this right after project(). It is a
# macro, not a function, so the settings land in the caller's scope.
#
# C++20 for QPlug's headers. Position independent code, since a plugin is a
# shared module built from static libraries, which on Linux must be PIC to
# link into one. The static MSVC runtime, for the whole build, because
# Elements, Artist and libunibreak pick the static runtime on their targets
# and clap-wrapper falls back to it, and mixing the two does not link; a
# plugin is loaded into a host that owes it no redistributable. On macOS,
# Objective-C and Objective-C++ for the host layer each plugin compiles,
# enabled here since a language must be enabled in the highest directory
# common to all the targets that use it. Hidden symbols, so a plugin exports
# its entry points and nothing else: two plugins in one host each carry
# their own Elements, quill and json, and on Linux exported copies merge
# into one, torn down at exit through the wrong plugin's code.

# The directory of this file, for the export map beside it: inside the
# macro, the list directory is the caller's. In the cache, so a plugin built
# outside Q, whose top-level directory calls the macro, sees it too.
set(Q_PLUG_SETTINGS_DIR "${CMAKE_CURRENT_LIST_DIR}" CACHE INTERNAL "")

macro(q_plug_settings)
   set(CMAKE_CXX_STANDARD 20)
   set(CMAKE_CXX_STANDARD_REQUIRED ON)
   set(CMAKE_POSITION_INDEPENDENT_CODE ON)
   set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
   set(CMAKE_C_VISIBILITY_PRESET hidden)
   set(CMAKE_CXX_VISIBILITY_PRESET hidden)
   set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
   if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
      add_link_options(
         "LINKER:--version-script=${Q_PLUG_SETTINGS_DIR}/q_plug_exports.map")
   endif()
   if(APPLE)
      enable_language(OBJC)
      enable_language(OBJCXX)
   endif()
endmacro()
