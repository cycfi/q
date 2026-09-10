###############################################################################
#  Copyright (c) 2019-2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################

# The plugin's own code, plus the qplug adapter it links.
add_library(qplug_va_synth_3_impl STATIC
   va_synth.cpp
   va_synth_processor.cpp
   va_synth_controller.cpp
   va_synth_presenter.cpp
)
# qplug is PRIVATE on purpose: the clap-wrapper format modules must link it
# and Elements but must not inherit their compile options, since Elements
# exports -fobjc-arc and clap-wrapper's Cocoa code is manual retain/release.
# clap is PUBLIC because the entry shim compiled into each module needs it.
target_link_libraries(qplug_va_synth_3_impl PRIVATE qplug PUBLIC clap)
qplug_add_host(qplug_va_synth_3_impl QPlugVASynth3)

# Assemble CLAP, VST3 and AUv2 from the one implementation.
make_clapfirst_plugins(
   TARGET_NAME qplug-va-synth-3
   IMPL_TARGET qplug_va_synth_3_impl

   OUTPUT_NAME "QPlug VA Synth 3"

   ENTRY_SOURCE "${QPLUG_ROOT}/lib/src/clap/qplug_entry.cpp"

   BUNDLE_IDENTIFIER "com.qplug.va_synth_3"
   BUNDLE_VERSION   "${qplug_VERSION}"

   COPY_AFTER_BUILD FALSE

   PLUGIN_FORMATS ${QPLUG_PLUGIN_FORMATS}

   ASSET_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/products"

   AUV2_MANUFACTURER_NAME "QPlug"
   AUV2_MANUFACTURER_CODE "QPlg"
   AUV2_SUBTYPE_CODE      "QVs3"
   AUV2_INSTRUMENT_TYPE   "aumu"
)

qplug_fix_auv2_plist(qplug-va-synth-3)
qplug_add_resources(qplug-va-synth-3)
qplug_add_validation_tests(
   va_synth_3 "QPlug VA Synth 3" aumu QVs3 QPlg)
