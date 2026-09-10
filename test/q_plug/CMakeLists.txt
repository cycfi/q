###############################################################################
#  Copyright (c) 2019-2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################
# Unit tests: the parts of qplug that are plain code, tested without a host.
# The plugin formats themselves are checked by the validator scripts, see
# qplug_tests.cmake.

set(test_sources
   midi_events.cpp
   event_slices.cpp
)

foreach(source ${test_sources})
   get_filename_component(name ${source} NAME_WE)
   add_executable(test_${name} ${source})
   target_link_libraries(test_${name} PRIVATE clap)
   # The headers only: a unit test declares no view, so Elements and
   # Artist are on the include path without being linked.
   target_include_directories(test_${name} PRIVATE
      ${QPLUG_ROOT}/lib/include
      ${QPLUG_ROOT}/lib/q/q_lib/include
      ${QPLUG_ROOT}/lib/infra/include
      $<TARGET_PROPERTY:elements,INTERFACE_INCLUDE_DIRECTORIES>
   )
   add_test(NAME test_${name} COMMAND test_${name})
endforeach()
