cmake_minimum_required(VERSION 3.21)

# macOS deployment target must be set before project()
if(NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")
endif()

project(QPlugGain VERSION 0.1.0 LANGUAGES C CXX)

if(APPLE)
    enable_language(OBJC)
    enable_language(OBJCXX)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED QPLUG_ROOT)
    set(QPLUG_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")
endif()

# ---------------------------------------------------------------------------
# clap-wrapper: download all required SDKs automatically
# ---------------------------------------------------------------------------
set(CLAP_WRAPPER_DOWNLOAD_DEPENDENCIES TRUE CACHE BOOL "" FORCE)
set(CLAP_WRAPPER_BUILD_TESTS           FALSE CACHE BOOL "" FORCE)

# Point the wrapper at our already-checked-out CLAP headers
set(CLAP_SDK_ROOT "${QPLUG_ROOT}/lib/clap" CACHE PATH "" FORCE)

add_subdirectory(
    "${QPLUG_ROOT}/lib/clap-wrapper"
    "${CMAKE_BINARY_DIR}/clap-wrapper"
    EXCLUDE_FROM_ALL
)

# ---------------------------------------------------------------------------
# Impl static library, all DSP and CLAP logic lives here
# ---------------------------------------------------------------------------
add_library(qplug_gain_impl STATIC gain.cpp)
target_link_libraries(qplug_gain_impl PUBLIC clap)

# ---------------------------------------------------------------------------
# Assemble CLAP + VST3 + AUv2 via clap-wrapper's make_clapfirst_plugins
# ---------------------------------------------------------------------------
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
