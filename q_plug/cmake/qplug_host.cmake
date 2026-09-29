###############################################################################
#  Copyright (c) 2019-2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################
# qplug_add_host(<impl_target> <prefix>)
#
# Compiles Elements' host layer into this plugin, naming its view class
# after <prefix>. On macOS the Objective-C runtime keeps one flat namespace
# of class names for the whole process, so two plugins that each carry
# their own Elements would otherwise define the same class and the runtime
# would pick one of them, whichever version it happened to be.
#
# The sources come from ELEMENTS_HOST_SOURCES, which Elements publishes
# when ELEMENTS_HOST_IN_CONSUMER is on, and everything they need to compile
# arrives with linking Elements.
#
# <prefix> must be a valid identifier and unique to the plugin.

function(qplug_add_host target prefix)
   if(NOT ELEMENTS_HOST_SOURCES)
      message(FATAL_ERROR
         "qplug: ELEMENTS_HOST_SOURCES is empty. Elements must be "
         "configured with ELEMENTS_HOST_IN_CONSUMER=ON.")
   endif()

   target_sources(${target} PRIVATE ${ELEMENTS_HOST_SOURCES})
   target_compile_definitions(${target} PRIVATE
      ELEMENTS_CLASS_PREFIX=${prefix})
endfunction()
