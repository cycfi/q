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

   // One slider for every parameter: four major divisions, so the ticks
   // fall at nothing, a quarter, a half, three quarters and all the way,
   // with five minor steps between each. The five labels name those same
   // five places, and every range is chosen so they land on round values.
   // Labels are given bottom to top, in the order the values run.
   template <typename... Labels>
   auto make_slider(Labels&&... labels)
   {
      static_assert(sizeof...(Labels) == 5, "one label per major tick");
      return share(
         slider(
            basic_rect_thumb<24, 14>()
          , slider_labels<20>(
               slider_marks_lin<28, 4, 5>(basic_track<6, true>())
             , 0.7, std::forward<Labels>(labels)...)
         )
      );
   }

   // A slider with its name under it: Elements' own caption. The labels
   // carry the units, so the name is only a name.
   template <typename Subject>
   auto captioned(Subject&& subject, char const* text)
   {
      return align_center(
         caption(
            vmargin({8, 8}, std::forward<Subject>(subject))
          , text, 0.9));
   }
}

void va_synth_presenter::on_attach(elements::view& view_)
{
   using ctl = va_synth_controller;
   auto const& params = _ctl.parameters();

   auto attack = make_slider("1ms", "10ms", "100ms", "1s", "10s");
   auto decay = make_slider("1ms", "10ms", "100ms", "1s", "10s");
   auto sustain_level = make_slider("0", "25", "50", "75", "100");
   auto release = make_slider("1ms", "10ms", "100ms", "1s", "10s");
   auto velocity = make_slider("0", "25", "50", "75", "100");

   // A control's travel is 0 to 1; the parameter maps it to its own range
   // and curve, so a slider needs nothing but the parameter it belongs to.
   bind(ctl::attack_id, attack, params[ctl::attack_id]);
   bind(ctl::decay_id, decay, params[ctl::decay_id]);
   bind(ctl::sustain_level_id, sustain_level, params[ctl::sustain_level_id]);
   bind(ctl::release_id, release, params[ctl::release_id]);
   bind(ctl::velocity_id, velocity, params[ctl::velocity_id]);

   // A framed group per section of the signal path. Stage 1 has one; the
   // filter and the oscillators get their own as they arrive. The top
   // margin is what leaves room for the heading the frame draws over it.
   // The panel is a fixed layout, and fixed_size is how it says so. Left
   // stretchable, Elements reports a maximum of 32768 in each direction,
   // the host believes the window can be pulled about, and on macOS it
   // will stretch the view and hand the new size back, which is seen as
   // the window springing when it opens.
   view_.content(
      fixed_size({600, 330},
         margin({16, 16, 16, 16},
            group("Envelope",
               margin({14, 42, 14, 14},
                  vmin_size(200,
                     htile(
                        captioned(hold(attack), "Attack"),
                        captioned(hold(decay), "Decay"),
                        captioned(hold(sustain_level), "Sustain"),
                        captioned(hold(release), "Release"),
                        captioned(hold(velocity), "Velocity")
                     )
                  )
               )
             , 1.0, false)
         )
      ),
      box(bkd_color)
   );
}
