/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q_plug/plugin.hpp>
#include "anna_processor.hpp"
#include "anna_controller.hpp"
#include "anna_presenter.hpp"

namespace cycfi::q_plug
{
   controller_ptr make_controller()
   {
      return std::make_unique<anna_controller>();
   }

   processor_ptr make_processor(controller& ctl)
   {
      return std::make_unique<anna_processor>(
         static_cast<anna_controller&>(ctl));
   }

   presenter_ptr make_presenter(controller& ctl)
   {
      return std::make_unique<anna_presenter>(
         static_cast<anna_controller&>(ctl));
   }

   plugin_info const& info()
   {
      // "instrument" is what puts it in the right menu; a host that reads
      // the features list will not offer an instrument as an insert.
      static char const* const features[] =
      {
         "instrument",
         "synthesizer",
         "stereo",
         nullptr
      };

      static plugin_info const i =
      {
         "com.qplug.anna_3",            // id
         "Anna III",                    // name
         "QPlug",                       // vendor
         "",                            // url
         "",                            // manual_url
         "",                            // support_url
         "0.1.1",                       // version
         "Polyphonic synth, stage 3: a chorus",
         features,
         {816, 504},                    // view size
         2                              // state version: sustain is now %
      };
      return i;
   }
}
