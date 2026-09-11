###############################################################################
#  Copyright (c) 2019-2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################
# qplug_add_validation_tests(
#    <name> <plugin_name> <au_type> <au_subtype> <au_mfr>)
#
# Registers one ctest test per plugin format, each running the matching script
# in scripts/ against the product in ${CMAKE_BINARY_DIR}/products. The
# validators come from qplug_validators.cmake (CLAP_VALIDATOR, PLUGINVAL); a
# script exits 77 when the validator it needs is missing, which ctest reports
# as SKIPPED rather than a failure. The AU test is macOS only and has a side
# effect: it installs the component into ~/Library/Audio/Plug-Ins/Components.
# Where there is no desktop to open an editor on, set QPLUG_SKIP_GUI_TESTS in
# the environment ctest runs in, and pluginval leaves out its editor tests.

# The scripts are bash. macOS and Linux run them directly; Windows cannot
# run a .sh, so there they go through the bash that comes with Git, which
# lives beside the git CMake finds.
set(QPLUG_SHELL "")
if(WIN32)
   find_package(Git QUIET)
   if(GIT_FOUND)
      get_filename_component(_git_dir "${GIT_EXECUTABLE}" DIRECTORY)
      find_program(QPLUG_BASH bash
         HINTS "${_git_dir}/../bin" "${_git_dir}/../usr/bin"
         DOC "The bash the validation scripts run under")
   endif()
   if(QPLUG_BASH)
      set(QPLUG_SHELL "${QPLUG_BASH}")
   else()
      message(WARNING "qplug: no bash found; the validation tests will fail."
         " Install Git for Windows, or set QPLUG_BASH.")
   endif()
endif()

function(qplug_add_validation_tests name plugin_name au_type au_subtype au_mfr)
   if(NOT QPLUG_BUILD_TEST)
      return()
   endif()

   set(scripts "${QPLUG_ROOT}/scripts")
   set(env "PLUGIN_NAME=${plugin_name}" "BUILD_DIR=${CMAKE_BINARY_DIR}")
   if(CLAP_VALIDATOR)
      list(APPEND env "CLAP_VALIDATOR=${CLAP_VALIDATOR}")
   endif()
   if(PLUGINVAL)
      list(APPEND env "PLUGINVAL=${PLUGINVAL}")
   endif()

   add_test(NAME ${name}.clap COMMAND ${QPLUG_SHELL} "${scripts}/validate.sh")
   add_test(NAME ${name}.vst3
      COMMAND ${QPLUG_SHELL} "${scripts}/validate-vst3.sh")
   set(tests ${name}.clap ${name}.vst3)

   if(APPLE)
      add_test(NAME ${name}.au COMMAND "${scripts}/validate-au.sh")
      list(APPEND tests ${name}.au)
   endif()

   foreach(t IN LISTS tests)
      set_property(TEST ${t} PROPERTY ENVIRONMENT ${env})
      set_property(TEST ${t} PROPERTY SKIP_RETURN_CODE 77)
   endforeach()

   if(APPLE)
      set_property(TEST ${name}.au APPEND PROPERTY ENVIRONMENT
         "AU_TYPE=${au_type}" "AU_SUBTYPE=${au_subtype}" "AU_MFR=${au_mfr}"
      )
   endif()
endfunction()
