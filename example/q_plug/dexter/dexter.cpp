/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q_plug/plugin.hpp>
#include "dexter_processor.hpp"
#include "dexter_controller.hpp"
#include "dexter_presenter.hpp"

namespace cycfi::q_plug
{
   controller_ptr make_controller()
   {
      return std::make_unique<dexter_controller>();
   }

   processor_ptr make_processor(controller& ctl)
   {
      return std::make_unique<dexter_processor>(
         static_cast<dexter_controller&>(ctl));
   }

   presenter_ptr make_presenter(controller& ctl)
   {
      return std::make_unique<dexter_presenter>(
         static_cast<dexter_controller&>(ctl));
   }

   plugin_info const& info()
   {
      static char const* const features[] =
      {
         "instrument",
         "synthesizer",
         "stereo",
         nullptr
      };

      static plugin_info const i =
      {
         "com.qplug.dexter",            // id
         "Dexter",                      // name
         "QPlug",                       // vendor
         "",                            // url
         "",                            // manual_url
         "",                            // support_url
         "0.1.0",                       // version
         "Six operator FM synth that plays DX7 patches",
         features,
         {1100, 740},                   // view size
         1                              // state version
      };
      return i;
   }
}
