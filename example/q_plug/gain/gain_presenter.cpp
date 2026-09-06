/*=============================================================================
   Copyright (c) 2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include "gain_presenter.hpp"
#include <elements.hpp>

gain_presenter::gain_presenter(gain_controller& ctl)
 : _ctl(ctl)
{}

using namespace cycfi::elements;

namespace
{
   auto constexpr bkd_color = rgba(35, 35, 37, 255);
   auto constexpr volume_id = 0;

   // Volume runs 0 to 2; the dial runs 0 to 1.
   double to_dial(double volume) { return volume / 2.0; }
   double to_volume(double pos) { return pos * 2.0; }
}

void gain_presenter::on_attach(elements::view& view_)
{
   auto dial_ptr = share(
      dial(
         radial_marks<20>(basic_knob<80>()),
         to_dial(_ctl.volume())
      )
   );

   // The user turns the dial: the controller gets the new value and sends
   // it on to the host. Tracking brackets it as one gesture.
   dial_ptr->on_change =
      [this](double pos)
      {
         _ctl.edit_parameter(volume_id, to_volume(pos));
      };

   view_.on_tracking =
      [this, dial = dial_ptr.get()](element& e, element::tracking state)
      {
         if (&e != dial)
            return;
         if (state == element::begin_tracking)
            _ctl.begin_edit(volume_id);
         else if (state == element::end_tracking)
            _ctl.end_edit(volume_id);
      };

   // The model changes, from the host or the GUI: the dial follows.
   _ctl.volume_model_().on_update(
      [&view_, weak = std::weak_ptr<basic_dial>(dial_ptr)](double volume)
      {
         if (auto dial_ = weak.lock())
         {
            dial_->value(to_dial(volume));
            view_.refresh(*dial_);
         }
      }
   );

   auto control = radial_labels<15>(
      hold(dial_ptr),
      0.7,                                // Label font size (relative size)
      "0", "1", "2", "3", "4",            // Labels
      "5", "6", "7", "8", "9", "10"
   );

   view_.content(
      align_center_middle(control),
      box(bkd_color)
   );
}

