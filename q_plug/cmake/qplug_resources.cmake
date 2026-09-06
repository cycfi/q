###############################################################################
#  Copyright (c) 2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################
# qplug_add_resources(<name>)
#
# Copies Elements' fonts into the Resources directory of every bundle that
# make_clapfirst_plugins produced for <name>. Elements registers each .ttf it
# finds in its own bundle's Resources with CoreText at load time, so text
# renders only if the fonts are in there. clap-wrapper's RESOURCE_DIRECTORY
# does this for VST3 only, so it is done here for all three formats.

function(qplug_add_resources name)
   if(NOT APPLE)
      return()
   endif()

   file(GLOB fonts "${QPLUG_ROOT}/lib/elements/resources/fonts/*.ttf")

   foreach(format clap vst3 auv2)
      set(target ${name}_${format})
      if(NOT TARGET ${target})
         continue()
      endif()

      set(dest "$<TARGET_FILE_DIR:${target}>/../Resources")
      set(commands COMMAND ${CMAKE_COMMAND} -E make_directory "${dest}")
      foreach(font IN LISTS fonts)
         list(APPEND commands
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${font}" "${dest}")
      endforeach()

      add_custom_target(${target}_resources ALL
         ${commands}
         DEPENDS ${target}
         COMMENT "Copying fonts into ${target}"
         VERBATIM
      )
   endforeach()
endfunction()
