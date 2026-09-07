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
# It has to be a target of its own: a POST_BUILD command on the AUv2 target
# runs only when that target relinks, which a reconfigure does not cause.
# One target serves every plugin, so it costs the IDE a single entry.
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

   # A command can only be added to a target declared in the same
   # directory, so collect the work here and build the one target at the
   # top level, in qplug_finalize_auv2_plists.
   set_property(GLOBAL APPEND PROPERTY QPLUG_AUV2_PLISTS "${auv2}")
   set_property(GLOBAL APPEND PROPERTY QPLUG_AUV2_PLIST_FILES "${plist}")
endfunction()

# Call once from the top-level CMakeLists, after the examples.
function(qplug_finalize_auv2_plists)
   get_property(targets GLOBAL PROPERTY QPLUG_AUV2_PLISTS)
   get_property(plists GLOBAL PROPERTY QPLUG_AUV2_PLIST_FILES)
   if(NOT targets)
      return()
   endif()

   add_custom_target(qplug_auv2_plists ALL
      COMMENT "Restoring AudioComponents in the AUv2 Info.plists"
   )

   list(LENGTH targets count)
   math(EXPR last "${count} - 1")
   foreach(i RANGE ${last})
      list(GET targets ${i} target)
      list(GET plists ${i} plist)
      add_dependencies(qplug_auv2_plists ${target})
      add_custom_command(TARGET qplug_auv2_plists POST_BUILD
         COMMAND ${CMAKE_COMMAND} -E copy
            "${plist}" "$<TARGET_FILE_DIR:${target}>/../Info.plist"
         VERBATIM
      )
   endforeach()
endfunction()
