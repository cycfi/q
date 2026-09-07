/*=============================================================================
   Copyright (c) 2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include "gain_presenter.hpp"
#include <elements.hpp>
#include <q/support/decibel.hpp>

gain_presenter::gain_presenter(gain_controller& ctl)
 : _ctl(ctl)
{}

using namespace cycfi::elements;

namespace
{
   auto constexpr bkd_color = rgba(35, 35, 37, 255);
   auto constexpr volume_id = gain_controller::volume_id;
}

void gain_presenter::on_attach(elements::view& view_)
{
   // A fader: the decibel taper of a console, marked and labelled at the
   // usual points over the parameter's range.
   using cycfi::q::dB;
   auto const& param = _ctl.volume_param();
   db_scale scale{param._min, param._max};

   auto track = slider_labels_db<10>(
      slider_marks_db<40>(basic_track<5, true>(), scale),
      0.8,                                   // Label font size (relative)
      scale
   );

   auto slider_ptr = share(
      slider(
         align_center(image{"slider-white.png", 1.0/4}),
         track,
         scale.position(_ctl.volume().rep)
      )
   );

   // The user moves the fader: the controller gets the value in decibels
   // and sends it on to the host. Tracking brackets it as one gesture.
   slider_ptr->on_change =
      [this, scale](double pos)
      {
         _ctl.edit_parameter(volume_id, dB(scale.value(pos)));
      };

   view_.on_tracking =
      [this, slider_ = slider_ptr.get()](element& e, element::tracking state)
      {
         if (&e != slider_)
            return;
         if (state == element::begin_tracking)
            _ctl.begin_edit(volume_id);
         else if (state == element::end_tracking)
            _ctl.end_edit(volume_id);
      };

   // The model changes, from the host or the GUI: the fader follows.
   _ctl.volume_model().on_update(
      [&view_, scale, weak = std::weak_ptr<basic_slider_base>(slider_ptr)]
      (double volume)
      {
         if (auto slider_ = weak.lock())
         {
            slider_->value(scale.position(volume));
            view_.refresh(*slider_);
         }
      }
   );

   view_.content(
      align_center(vmargin({20, 20}, hold(slider_ptr))),
      box(bkd_color)
   );
}
