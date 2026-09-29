###############################################################################
#  Copyright (c) 2019-2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################

# The plugin's own code, plus the q_plug adapter it links.
add_library(q_plug_va_synth_2_impl STATIC
   va_synth.cpp
   va_synth_processor.cpp
   va_synth_controller.cpp
   va_synth_presenter.cpp
)
# q_plug is PRIVATE on purpose: the clap-wrapper format modules must link it
# and Elements but must not inherit their compile options, since Elements
# exports -fobjc-arc and clap-wrapper's Cocoa code is manual retain/release.
# clap is PUBLIC because the entry shim compiled into each module needs it.
target_link_libraries(q_plug_va_synth_2_impl PRIVATE q_plug PUBLIC clap)
q_plug_add_host(q_plug_va_synth_2_impl QPlugVASynth2)

# Assemble CLAP, VST3 and AUv2 from the one implementation.
make_clapfirst_plugins(
   TARGET_NAME q_plug-va-synth-2
   IMPL_TARGET q_plug_va_synth_2_impl

   OUTPUT_NAME "QPlug VA Synth 2"

   ENTRY_SOURCE "${Q_PLUG_ROOT}/lib/src/clap/q_plug_entry.cpp"

   BUNDLE_IDENTIFIER "com.qplug.va_synth_2"
   BUNDLE_VERSION   "${q_plug_VERSION}"

   COPY_AFTER_BUILD FALSE
   WINDOWS_FOLDER_VST3 TRUE      # a bundle, so its fonts go inside

   PLUGIN_FORMATS ${Q_PLUG_PLUGIN_FORMATS}

   # A standalone app around the same CLAP, to try it without a host.
   STANDALONE_CONFIGURATIONS
      standalone "QPlug VA Synth 2" "com.qplug.va_synth_2"

   ASSET_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/products"

   AUV2_MANUFACTURER_NAME "QPlug"
   AUV2_MANUFACTURER_CODE "QPlg"
   AUV2_SUBTYPE_CODE      "QVs2"
   AUV2_INSTRUMENT_TYPE   "aumu"
)

q_plug_separate_import_libs(q_plug-va-synth-2)
q_plug_fix_auv2_plist(q_plug-va-synth-2)
q_plug_add_resources(q_plug-va-synth-2)
q_plug_add_validation_tests(
   va_synth_2 "QPlug VA Synth 2" aumu QVs2 QPlg)
