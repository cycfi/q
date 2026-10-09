###############################################################################
#  Copyright (c) 2019-2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################
# Unit tests: the parts of q_plug that are plain code, tested without a host.
# The plugin formats themselves are checked by the validator scripts, see
# q_plug_tests.cmake.

set(test_sources
   midi_events.cpp
   event_slices.cpp
   parameter_taper.cpp
   bindable_proxy.cpp
   gui_scale.cpp
)

# Tests that drive a whole plugin through its CLAP entry, named for the
# example they exercise and linked against its implementation.
# Each entry is a test that drives one example plugin, named for it.
set(plugin_tests
   anna_audio:q_plug_anna_1_impl
   anna_2_audio:q_plug_anna_2_impl
   anna_3_audio:q_plug_anna_3_impl
   anna_4_audio:q_plug_anna_4_impl
   anna_5_audio:q_plug_anna_5_impl
   anna_5_presets:q_plug_anna_5_impl
   dexter_audio:q_plug_dexter_impl
   dexter_presets:q_plug_dexter_impl
   dexter_chart:q_plug_dexter_impl
)

foreach(source ${test_sources})
   get_filename_component(name ${source} NAME_WE)
   add_executable(test_${name} ${source})
   target_link_libraries(test_${name} PRIVATE libq clap)
   # The headers only: a unit test declares no view, so Elements and
   # Artist are on the include path without being linked.
   target_include_directories(test_${name} PRIVATE
      ${Q_PLUG_ROOT}/include
      $<TARGET_PROPERTY:elements,INTERFACE_INCLUDE_DIRECTORIES>
   )
   add_test(NAME test_${name} COMMAND test_${name})
endforeach()

foreach(entry ${plugin_tests})
   string(REPLACE ":" ";" parts ${entry})
   list(GET parts 0 name)
   list(GET parts 1 impl)
   add_executable(test_${name} ${name}.cpp)
   target_link_libraries(test_${name} PRIVATE ${impl} libq
      nlohmann_json::nlohmann_json)
   target_include_directories(test_${name} PRIVATE
      ${Q_PLUG_ROOT}/include
      $<TARGET_PROPERTY:elements,INTERFACE_INCLUDE_DIRECTORIES>
   )
   add_test(NAME test_${name} COMMAND test_${name})
endforeach()

# Two different plugins in one process, loaded from their built .clap files
# the way a host loads them, with both editors open in the host's windows.
# See two_plugins.cpp. It opens windows, so it needs a display: Linux CI
# runs it under xvfb-run.
if(TARGET q_plug-gain_clap AND TARGET q_plug-dexter_clap)
   if(WIN32)
      set(host_window host_window_win.cpp)
   elseif(APPLE)
      set(host_window host_window_mac.mm)
   else()
      set(host_window host_window_x11.cpp)
   endif()
   add_executable(test_two_plugins two_plugins.cpp ${host_window})
   target_link_libraries(test_two_plugins PRIVATE libq clap)
   if(APPLE)
      target_link_libraries(test_two_plugins PRIVATE "-framework Cocoa")
   elseif(NOT WIN32)
      find_package(X11 REQUIRED)
      target_link_libraries(test_two_plugins PRIVATE X11::X11 ${CMAKE_DL_LIBS})
   endif()
   target_compile_definitions(test_two_plugins PRIVATE
      Q_PLUG_PRODUCTS="${CMAKE_BINARY_DIR}/products")
   add_dependencies(test_two_plugins q_plug-gain_clap q_plug-dexter_clap)
   add_test(NAME test_two_plugins COMMAND test_two_plugins)
   # A collision can show as a message box that waits for a click, so a
   # hang fails the test rather than holding the run.
   set_property(TEST test_two_plugins PROPERTY TIMEOUT 120)
endif()

# The proxy test makes a real element, so it is the one unit test that
# links Elements. It takes Elements' host sources the way a plugin does,
# and stubs the plugin factories q_plug expects an implementation for.
target_link_libraries(test_bindable_proxy PRIVATE q_plug)
q_plug_add_host(test_bindable_proxy QPlugTestBindableProxy)
# Elements' Windows host brings _UNICODE, and Catch then opens with wmain,
# which needs an overload Catch only declares under UNICODE. The plain main
# is all a test needs.
target_compile_definitions(test_bindable_proxy PRIVATE DO_NOT_USE_WMAIN)

# The stage 5 preset test reads the file the plugin ships, from the source
# tree, so it checks what is committed rather than what was copied.
set(anna_5_presets_file
   "${q_SOURCE_DIR}/example/q_plug/anna_5/factory_presets.json")
target_compile_definitions(test_anna_5_presets PRIVATE
   Q_PLUG_ANNA_5_PRESETS="${anna_5_presets_file}")
# The preset name rides in the state through the controller, which the
# test drives directly.
target_include_directories(test_anna_5_presets PRIVATE
   ${q_SOURCE_DIR}/example/q_plug/anna_5)

# Dexter's preset test reads the file the plugin ships, from the source
# tree, and the chart test the example's own layout.
set(dexter_presets_file
   "${q_SOURCE_DIR}/example/q_plug/dexter/factory_presets.json")
target_compile_definitions(test_dexter_presets PRIVATE
   Q_PLUG_DEXTER_PRESETS="${dexter_presets_file}")
target_include_directories(test_dexter_presets PRIVATE
   ${q_SOURCE_DIR}/example/q_plug/dexter)
target_include_directories(test_dexter_chart PRIVATE
   ${q_SOURCE_DIR}/example/q_plug/dexter)
