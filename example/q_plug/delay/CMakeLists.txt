###############################################################################
#  Copyright (c) 2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################

# The plugin's own code, plus the qplug adapter it links.
add_library(qplug_delay_impl STATIC
   delay.cpp
   delay_processor.cpp
   delay_controller.cpp
   delay_presenter.cpp
)
# qplug is PRIVATE on purpose: the clap-wrapper format modules must link it
# and Elements but must not inherit their compile options, since Elements
# exports -fobjc-arc and clap-wrapper's Cocoa code is manual retain/release.
# clap is PUBLIC because the entry shim compiled into each module needs it.
target_link_libraries(qplug_delay_impl PRIVATE qplug PUBLIC clap)

# Assemble CLAP, VST3 and AUv2 from the one implementation.
make_clapfirst_plugins(
   TARGET_NAME qplug-delay
   IMPL_TARGET qplug_delay_impl

   OUTPUT_NAME "QPlug Delay"

   ENTRY_SOURCE "${QPLUG_ROOT}/lib/src/qplug_entry.cpp"

   BUNDLE_IDENTIFIER "com.qplug.delay"
   BUNDLE_VERSION   "${qplug_VERSION}"

   COPY_AFTER_BUILD FALSE

   PLUGIN_FORMATS CLAP VST3 AUV2

   ASSET_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/products"

   AUV2_MANUFACTURER_NAME "QPlug"
   AUV2_MANUFACTURER_CODE "QPlg"
   AUV2_SUBTYPE_CODE      "QDly"
   AUV2_INSTRUMENT_TYPE   "aufx"
)

qplug_fix_auv2_plist(qplug-delay)
qplug_add_resources(qplug-delay
   "${CMAKE_CURRENT_SOURCE_DIR}/resources/slider-white.png")
qplug_add_validation_tests(delay "QPlug Delay" aufx QDly QPlg)
