/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q_plug/plugin.hpp>
#include "sampler_processor.hpp"
#include "sampler_controller.hpp"
#include "sampler_presenter.hpp"

namespace cycfi::q_plug
{
   controller_ptr make_controller()
   {
      return std::make_unique<sampler_controller>();
   }

   processor_ptr make_processor(controller& ctl)
   {
      return std::make_unique<sampler_processor>(
         static_cast<sampler_controller&>(ctl));
   }

   presenter_ptr make_presenter(controller& ctl)
   {
      return std::make_unique<sampler_presenter>(
         static_cast<sampler_controller&>(ctl));
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
         "com.qplug.sampler",            // id
         "QPlug Sampler",                // name
         "QPlug",                        // vendor
         "",                             // url
         "",                             // manual_url
         "",                             // support_url
         "0.1.0",                        // version
         "Plays a WAV file",             // description
         features,
         {320, 190},                     // view size
         1                               // state version
      };
      return i;
   }
}
