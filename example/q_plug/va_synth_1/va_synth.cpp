/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <qplug/plugin.hpp>
#include "va_synth_processor.hpp"
#include "va_synth_controller.hpp"
#include "va_synth_presenter.hpp"

namespace cycfi::qplug
{
   controller_ptr make_controller()
   {
      return std::make_unique<va_synth_controller>();
   }

   processor_ptr make_processor(controller& ctl)
   {
      return std::make_unique<va_synth_processor>(
         static_cast<va_synth_controller&>(ctl));
   }

   presenter_ptr make_presenter(controller& ctl)
   {
      return std::make_unique<va_synth_presenter>(
         static_cast<va_synth_controller&>(ctl));
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
         "com.qplug.va_synth_1",        // id
         "QPlug VA Synth 1",            // name
         "QPlug",                       // vendor
         "",                            // url
         "",                            // manual_url
         "",                            // support_url
         "0.1.1",                       // version
         "Polyphonic synth, stage 1: oscillator and envelope",
         features,
         {495, 330},                    // view size
         1                              // state version
      };
      return i;
   }
}
