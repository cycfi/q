###############################################################################
#  Copyright (c) 2019-2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################

cmake_minimum_required(VERSION 3.21)

# Before project(): Artist needs std::filesystem, from macOS 10.15, and its
# prebuilt bundles target 11.0.
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

project(q_plug_anna_2 LANGUAGES C CXX)

# Built on its own, outside Q: bring in Q, with QPlug, from Q_ROOT.
if(NOT TARGET q_plug)
   if(NOT Q_ROOT)
      message(FATAL_ERROR "Q_ROOT is not set")
   endif()
   set(Q_BUILD_PLUG ON)
   set(Q_BUILD_EXAMPLES OFF)
   set(Q_BUILD_TEST OFF)
   add_subdirectory(${Q_ROOT} q)
endif()

q_plug_settings()

# The plugin's own code, plus the q_plug adapter it links.
add_library(q_plug_anna_2_impl STATIC
   anna.cpp
   anna_processor.cpp
   anna_controller.cpp
   anna_presenter.cpp
)
# q_plug is PRIVATE on purpose: the clap-wrapper format modules must link it
# and Elements but must not inherit their compile options, since Elements
# exports -fobjc-arc and clap-wrapper's Cocoa code is manual retain/release.
# clap is PUBLIC because the entry shim compiled into each module needs it.
target_link_libraries(q_plug_anna_2_impl PRIVATE q_plug PUBLIC clap)
q_plug_add_host(q_plug_anna_2_impl QPlugAnna2)

# Assemble CLAP, VST3 and AUv2 from the one implementation.
make_clapfirst_plugins(
   TARGET_NAME q_plug-anna-2
   IMPL_TARGET q_plug_anna_2_impl

   OUTPUT_NAME "Anna II"

   ENTRY_SOURCE "${Q_PLUG_ROOT}/src/clap/q_plug_entry.cpp"

   BUNDLE_IDENTIFIER "com.qplug.anna_2"
   BUNDLE_VERSION   "${q_plug_VERSION}"

   COPY_AFTER_BUILD FALSE
   WINDOWS_FOLDER_VST3 TRUE      # a bundle, so its fonts go inside

   PLUGIN_FORMATS ${Q_PLUG_PLUGIN_FORMATS}

   # A standalone app around the same CLAP, to try it without a host.
   STANDALONE_CONFIGURATIONS
      standalone "Anna II" "com.qplug.anna_2"

   ASSET_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/products"

   AUV2_MANUFACTURER_NAME "QPlug"
   AUV2_MANUFACTURER_CODE "QPlg"
   AUV2_SUBTYPE_CODE      "QAn2"
   AUV2_INSTRUMENT_TYPE   "aumu"
)

q_plug_separate_import_libs(q_plug-anna-2)
q_plug_fix_auv2_plist(q_plug-anna-2)
q_plug_add_resources(q_plug-anna-2)
q_plug_add_validation_tests(
   anna_2 "Anna II" aumu QAn2 QPlg)
