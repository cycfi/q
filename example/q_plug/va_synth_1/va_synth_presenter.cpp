/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include "va_synth_presenter.hpp"
#include <elements.hpp>

va_synth_presenter::va_synth_presenter(va_synth_controller& ctl)
 : qplug::presenter(ctl)
 , _ctl(ctl)
{}

using namespace cycfi::elements;

namespace
{
   auto constexpr bkd_color = rgba(35, 35, 37, 255);

   // A plain vertical slider, marked along its travel: the thumb, and a
   // track that knows it stands up.
   auto make_slider()
   {
      return share(
         slider(
            basic_thumb<22>()
          , slider_marks_lin<30>(basic_track<6, true>())
         )
      );
   }

   // A slider with its name under it. The caption carries the unit, so
   // nothing else has to.
   template <typename Subject>
   auto captioned(Subject&& subject, char const* text)
   {
      return align_center(
         vtile(
            align_center(
               vmargin({6, 6}, std::forward<Subject>(subject))),
            align_center(margin({0, 6, 0, 0}, label(text)))
         )
      );
   }
}

void va_synth_presenter::on_attach(elements::view& view_)
{
   using ctl = va_synth_controller;
   auto const& params = _ctl.parameters();

   auto attack = make_slider();
   auto decay = make_slider();
   auto sustain_level = make_slider();
   auto sustain_rate = make_slider();
   auto release = make_slider();

   // A control's travel is 0 to 1; the parameter maps it to its own range
   // and curve, so a slider needs nothing but the parameter it belongs to.
   bind(ctl::attack_id, attack, params[ctl::attack_id]);
   bind(ctl::decay_id, decay, params[ctl::decay_id]);
   bind(ctl::sustain_level_id, sustain_level, params[ctl::sustain_level_id]);
   bind(ctl::sustain_rate_id, sustain_rate, params[ctl::sustain_rate_id]);
   bind(ctl::release_id, release, params[ctl::release_id]);

   view_.content(
      margin({20, 16, 20, 16},
         vtile(
            align_left(margin({0, 0, 0, 10}, label("Envelope"))),
            vmin_size(200,
               htile(
                  captioned(hold(attack), "Attack"),
                  captioned(hold(decay), "Decay"),
                  captioned(hold(sustain_level), "Sustain"),
                  captioned(hold(sustain_rate), "S. Rate"),
                  captioned(hold(release), "Release")
               )
            )
         )
      ),
      box(bkd_color)
   );
}
