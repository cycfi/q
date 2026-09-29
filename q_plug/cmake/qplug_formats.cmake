###############################################################################
#  Copyright (c) 2019-2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################
# qplug_separate_import_libs(<name>)
#
# Gives every format built for <name> its own directory for the import
# library and export file MSVC writes beside a link. The formats share one
# OUTPUT_NAME, so on Windows the CLAP, the VST3 and the standalone all
# write "<OUTPUT_NAME>.lib" and ".exp" into the same directory, and two
# links running at once fail with LNK1104 on the file the other is
# holding. Nothing links against these; they exist because each module and
# the standalone export the CLAP entry.
#
# A no-op where the linker writes no such file.

function(qplug_separate_import_libs name)
   if(NOT MSVC)
      return()
   endif()
   foreach(format clap vst3 auv2 standalone)
      set(target ${name}_${format})
      if(TARGET ${target})
         set_target_properties(${target} PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${format}")
      endif()
   endforeach()
endfunction()
