/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <qplug/plugin.hpp>
#include "delay_processor.hpp"
#include "delay_controller.hpp"
#include "delay_presenter.hpp"

namespace cycfi::qplug
{
   controller_ptr make_controller()
   {
      return std::make_unique<delay_controller>();
   }

   processor_ptr make_processor(controller& ctl)
   {
      return std::make_unique<delay_processor>(
         static_cast<delay_controller&>(ctl));
   }

   presenter_ptr make_presenter(controller& ctl)
   {
      return std::make_unique<delay_presenter>(
         static_cast<delay_controller&>(ctl));
   }

   plugin_info const& info()
   {
      static char const* const features[] =
      {
         "audio-effect",
         "delay",
         "mono",
         nullptr
      };

      static plugin_info const i =
      {
         "com.qplug.delay",              // id
         "QPlug Delay",                  // name
         "QPlug",                        // vendor
         "",                             // url
         "",                             // manual_url
         "",                             // support_url
         "0.1.1",                        // version
         "Simple mono delay plugin",     // description
         features,
         {400, 260},                      // view size
         1                              // state version
      };
      return i;
   }
}
