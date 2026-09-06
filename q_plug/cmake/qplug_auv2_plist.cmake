###############################################################################
#  Copyright (c) 2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################
# qplug_fix_auv2_plist(<name>)
#
# Workaround for a clap-wrapper fragility. clap-wrapper generates the AUv2
# Info.plist (the one with the AudioComponents entry the system registers)
# at build time and copies it into the bundle with a PRE_BUILD command that
# only runs when the AUv2 target relinks. CMake, however, rewrites a bundle's
# Info.plist at configure time. So any reconfigure without a relink leaves
# the bundle with a plist that has no AudioComponents, and the component
# silently vanishes from the system. This target re-copies the generated
# plist on every build, after the AUv2 target, so the next build heals it.
#
# <name> is the TARGET_NAME given to make_clapfirst_plugins. Remove this once
# clap-wrapper's wrap_auv2.cmake does the copy from an always-run step.

function(qplug_fix_auv2_plist name)
   if(NOT APPLE)
      return()
   endif()

   set(auv2 ${name}_auv2)
   if(NOT TARGET ${auv2})
      return()
   endif()

   set(plist
      "${CMAKE_CURRENT_BINARY_DIR}/${auv2}-build-helper-output/auv2_Info.plist")

   add_custom_target(${auv2}_plist ALL
      COMMAND ${CMAKE_COMMAND} -E copy
         "${plist}" "$<TARGET_FILE_DIR:${auv2}>/../Info.plist"
      DEPENDS ${auv2}
      COMMENT "Restoring AudioComponents in ${auv2} Info.plist"
      VERBATIM
   )
endfunction()
