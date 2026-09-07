###############################################################################
#  Copyright (c) 2019-2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################
# qplug_add_resources(<name> [file...])
#
# Copies the fonts a plugin needs, and its own resource files, into the
# Resources directory of every bundle that make_clapfirst_plugins produced
# for <name>. Elements registers each .ttf it finds in its own bundle's
# Resources with CoreText when the first view is made, and looks images up
# there, so they show only if they are in there. clap-wrapper's
# RESOURCE_DIRECTORY does this for VST3 only, so it is done here for all
# three formats.
#
# The fonts are ELEMENTS_FONTS plus the icon font, the same set an Elements
# app gets, and for the same reason: each face registered costs a few
# milliseconds when the editor first opens, so a plugin ships what it draws
# with and no more. To add one, before including this file:
#
#    list(APPEND ELEMENTS_FONTS
#       ${QPLUG_ROOT}/lib/elements/resources/fonts/OpenSans-Bold.ttf)

function(qplug_add_resources name)
   if(NOT APPLE)
      return()
   endif()

   set(elements_fonts "${QPLUG_ROOT}/lib/elements/resources/fonts")
   if(NOT DEFINED ELEMENTS_ICON_FONT)
      set(ELEMENTS_ICON_FONT "${elements_fonts}/elements_basic.ttf")
   endif()
   if(NOT DEFINED ELEMENTS_FONTS)
      set(ELEMENTS_FONTS
         "${elements_fonts}/OpenSans-Regular.ttf"
         "${elements_fonts}/Roboto-Medium.ttf"
      )
   endif()

   set(fonts ${ELEMENTS_ICON_FONT} ${ELEMENTS_FONTS} ${ARGN})

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

      # POST_BUILD on the module itself, not a target of its own: the copy
      # belongs to the bundle, and a target per format per plugin fills an
      # IDE's target list with noise.
      add_custom_command(TARGET ${target} POST_BUILD
         ${commands}
         COMMENT "Copying resources into ${target}"
         VERBATIM
      )
   endforeach()
endfunction()
