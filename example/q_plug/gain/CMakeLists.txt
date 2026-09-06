###############################################################################
#  Copyright (c) 2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################

# The plugin's own code, plus the qplug adapter it links.
add_library(qplug_gain_impl STATIC
   gain.cpp
   gain_processor.cpp
   gain_controller.cpp
   gain_presenter.cpp
)
target_link_libraries(qplug_gain_impl PUBLIC qplug)

# Assemble CLAP, VST3 and AUv2 from the one implementation.
make_clapfirst_plugins(
   TARGET_NAME qplug-gain
   IMPL_TARGET qplug_gain_impl

   OUTPUT_NAME "QPlug Gain"

   ENTRY_SOURCE "${QPLUG_ROOT}/lib/src/qplug_entry.cpp"

   BUNDLE_IDENTIFER "com.qplug.gain"
   BUNDLE_VERSION   ${PROJECT_VERSION}

   COPY_AFTER_BUILD FALSE

   PLUGIN_FORMATS CLAP VST3 AUV2

   ASSET_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/products"

   AUV2_MANUFACTURER_NAME "QPlug"
   AUV2_MANUFACTURER_CODE "QPlg"
   AUV2_SUBTYPE_CODE      "QGan"
   AUV2_INSTRUMENT_TYPE   "aufx"
)

qplug_fix_auv2_plist(qplug-gain)
qplug_add_validation_tests(gain "QPlug Gain" aufx QGan QPlg)
