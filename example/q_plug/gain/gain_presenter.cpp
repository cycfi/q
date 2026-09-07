/*=============================================================================
   Copyright (c) 2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include "gain_presenter.hpp"
#include <elements.hpp>

gain_presenter::gain_presenter(gain_controller& ctl)
 : qplug::presenter(ctl)
 , _ctl(ctl)
{}

using namespace cycfi::elements;

namespace
{
   auto constexpr bkd_color = rgba(35, 35, 37, 255);
}

void gain_presenter::on_attach(elements::view& view_)
{
   // A fader: the decibel taper of a console, marked and labelled at the
   // usual points over the parameter's range.
   auto const& param = _ctl.volume_param();
   db_scale scale{param._min, param._max};

   auto track = slider_labels_db<10>(
      slider_marks_db<40>(basic_track<5, true>(), scale),
      0.8,                                   // Label font size (relative)
      scale
   );

   auto fader = share(
      slider(align_center(image{"slider-white.png", 1.0/4}), track)
   );

   bind(gain_controller::volume_id, fader, scale);

   view_.content(
      align_center(vmargin({20, 20}, hold(fader))),
      box(bkd_color)
   );
}
