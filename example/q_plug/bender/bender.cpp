/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q_plug/plugin.hpp>
#include "bender_processor.hpp"
#include "bender_controller.hpp"
#include "bender_presenter.hpp"

namespace cycfi::q_plug
{
   controller_ptr make_controller()
   {
      return std::make_unique<bender_controller>();
   }

   processor_ptr make_processor(controller& ctl)
   {
      return std::make_unique<bender_processor>(
         static_cast<bender_controller&>(ctl));
   }

   presenter_ptr make_presenter(controller& ctl)
   {
      return std::make_unique<bender_presenter>(
         static_cast<bender_controller&>(ctl));
   }

   plugin_info const& info()
   {
      static char const* const features[] =
      {
         "audio-effect",
         "pitch-shifter",
         "mono",
         nullptr
      };

      static plugin_info const i =
      {
         "com.qplug.bender",             // id
         "Q Bender",                     // name
         "QPlug",                        // vendor
         "",                             // url
         "",                             // manual_url
         "",                             // support_url
         "0.1.0",                        // version
         "Pitch bend for a whole sound, chords included, with formants",
         features,
         {800, 420},                     // view size
         1                               // state version
      };
      return i;
   }
}
