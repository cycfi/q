/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include "delay_presenter.hpp"
#include <elements.hpp>

delay_presenter::delay_presenter(delay_controller& ctl)
 : qplug::presenter(ctl)
 , _ctl(ctl)
{}

using namespace cycfi::elements;

namespace
{
   auto constexpr bkd_color = rgba(35, 35, 37, 255);

   auto make_dial()
   {
      return share(dial(radial_marks<20>(basic_knob<60>())));
   }

   // A labelled dial with a caption under it. The caption carries the
   // unit, so the labels stay bare numbers.
   template <typename Subject>
   auto captioned(Subject&& subject, char const* text)
   {
      return align_center_middle(
         vtile(
            align_center(std::forward<Subject>(subject)),
            align_center(margin({0, 8, 0, 0}, label(text)))
         )
      );
   }
}

void delay_presenter::on_attach(elements::view& view_)
{
   auto const& delay_param = _ctl.parameters()[delay_controller::delay_id];
   auto const& feedback_param =
      _ctl.parameters()[delay_controller::feedback_id];

   auto delay_dial = make_dial();
   auto feedback_dial = make_dial();

   // Each control's travel is 0 to 1; the parameter maps it to its own
   // range, so a dial needs nothing but the parameter it belongs to.
   bind(delay_controller::delay_id, delay_dial, delay_param);
   bind(delay_controller::feedback_id, feedback_dial, feedback_param);

   view_.content(
      margin({20, 20, 20, 20},
         htile(
            captioned(
               radial_labels<15>(hold(delay_dial), 0.7
                , "0", "250", "500", "750", "1000")
             , "Delay (ms)"
            ),
            captioned(
               radial_labels<15>(hold(feedback_dial), 0.7
                , "0", "25", "50", "75", "100")
             , "Feedback (%)"
            )
         )
      ),
      box(bkd_color)
   );
}
