/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <qplug/plugin.hpp>
#include "gain_processor.hpp"
#include "gain_controller.hpp"
#include "gain_presenter.hpp"

namespace cycfi::qplug
{
   controller_ptr make_controller()
   {
      return std::make_unique<gain_controller>();
   }

   processor_ptr make_processor(controller& ctl)
   {
      return std::make_unique<gain_processor>(
         static_cast<gain_controller&>(ctl));
   }

   presenter_ptr make_presenter(controller& ctl)
   {
      return std::make_unique<gain_presenter>(
         static_cast<gain_controller&>(ctl));
   }

   plugin_info const& info()
   {
      static char const* const features[] =
      {
         "audio-effect",
         "mono",
         nullptr
      };

      static plugin_info const i =
      {
         "com.qplug.gain",              // id
         "QPlug Gain",                  // name
         "QPlug",                       // vendor
         "",                            // url
         "",                            // manual_url
         "",                            // support_url
         "0.1.1",                       // version
         "Simple mono gain plugin",     // description
         features,
         {400, 350},                     // view size
         1                              // state version
      };
      return i;
   }
}
