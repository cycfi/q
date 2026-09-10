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
   parameter_taper.cpp
   bindable_proxy.cpp
)

# Tests that drive a whole plugin through its CLAP entry, named for the
# example they exercise and linked against its implementation.
# Each entry is a test that drives one example plugin, named for it.
set(plugin_tests
   va_synth_audio:qplug_va_synth_1_impl
   va_synth_2_audio:qplug_va_synth_2_impl
   va_synth_3_audio:qplug_va_synth_3_impl
   va_synth_4_audio:qplug_va_synth_4_impl
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

foreach(entry ${plugin_tests})
   string(REPLACE ":" ";" parts ${entry})
   list(GET parts 0 name)
   list(GET parts 1 impl)
   add_executable(test_${name} ${name}.cpp)
   target_link_libraries(test_${name} PRIVATE ${impl})
   target_include_directories(test_${name} PRIVATE
      ${QPLUG_ROOT}/lib/include
      ${QPLUG_ROOT}/lib/q/q_lib/include
      ${QPLUG_ROOT}/lib/infra/include
      $<TARGET_PROPERTY:elements,INTERFACE_INCLUDE_DIRECTORIES>
   )
   add_test(NAME test_${name} COMMAND test_${name})
endforeach()

# The proxy test makes a real element, so it is the one unit test that
# links Elements. It takes Elements' host sources the way a plugin does,
# and stubs the plugin factories qplug expects an implementation for.
target_link_libraries(test_bindable_proxy PRIVATE qplug)
qplug_add_host(test_bindable_proxy QPlugTestBindableProxy)

