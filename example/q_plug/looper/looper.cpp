/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q_plug/plugin.hpp>
#include "looper_processor.hpp"
#include "looper_controller.hpp"
#include "looper_presenter.hpp"

namespace cycfi::q_plug
{
   controller_ptr make_controller()
   {
      return std::make_unique<looper_controller>();
   }

   processor_ptr make_processor(controller& ctl)
   {
      return std::make_unique<looper_processor>(
         static_cast<looper_controller&>(ctl));
   }

   presenter_ptr make_presenter(controller& ctl)
   {
      return std::make_unique<looper_presenter>(
         static_cast<looper_controller&>(ctl));
   }

   plugin_info const& info()
   {
      static char const* const features[] =
      {
         "audio-effect",
         "sampler",
         "stereo",
         nullptr
      };

      static plugin_info const i =
      {
         "com.qplug.looper",             // id
         "QPlug Looper",                 // name
         "QPlug",                        // vendor
         "",                             // url
         "",                             // manual_url
         "",                             // support_url
         "0.1.0",                        // version
         "Plays or loops a WAV file",    // description
         features,
         {380, 190},                     // view size
         1                               // state version
      };
      return i;
   }
}
