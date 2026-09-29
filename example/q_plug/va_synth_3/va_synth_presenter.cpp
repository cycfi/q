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
   auto volume = make_slider("-60", "-45", "-30", "-15", "0");

   // The cutoff spans the audible band rather than whole decades, so its
   // labels are the frequencies at the quarters rather than round powers
   // of ten. Resonance is four octaves; depth is octaves outright.
   auto cutoff = make_slider("20", "110", "630", "3.5k", "20k");
   auto resonance = make_slider("0.5", "1", "2", "4", "8");
   auto env_depth = make_slider("0", "2", "4", "6", "8");

   // The filter's own contour, on the same tapers as the amplifier's.
   auto f_attack = make_slider("1ms", "10ms", "100ms", "1s", "10s");
   auto f_decay = make_slider("1ms", "10ms", "100ms", "1s", "10s");
   auto f_sustain = make_slider("0", "25", "50", "75", "100");
   auto f_release = make_slider("1ms", "10ms", "100ms", "1s", "10s");
   auto f_velocity = make_slider("0", "25", "50", "75", "100");

   // The chorus: two decades of rate, so its quarters are not round;
   // depth in milliseconds; mix dry to wet.
   auto c_rate = make_slider("0.1", "0.3", "1", "3", "10");
   auto c_depth = make_slider("0", "2.5", "5", "7.5", "10");
   auto c_mix = make_slider("0", "25", "50", "75", "100");

   // A control's travel is 0 to 1; the parameter maps it to its own range
   // and curve, so a slider needs nothing but the parameter it belongs to.
   auto link = [&](int id, auto control)
   {
      bind(id, control, params[id]);
   };

   link(ctl::attack_id, attack);
   link(ctl::decay_id, decay);
   link(ctl::sustain_level_id, sustain_level);
   link(ctl::release_id, release);
   link(ctl::cutoff_id, cutoff);
   link(ctl::resonance_id, resonance);
   link(ctl::env_depth_id, env_depth);
   link(ctl::filter_attack_id, f_attack);
   link(ctl::filter_decay_id, f_decay);
   link(ctl::filter_sustain_level_id, f_sustain);
   link(ctl::filter_release_id, f_release);
   link(ctl::velocity_id, velocity);
   link(ctl::volume_id, volume);
   link(ctl::filter_velocity_id, f_velocity);
   link(ctl::chorus_rate_id, c_rate);
   link(ctl::chorus_depth_id, c_depth);
   link(ctl::chorus_mix_id, c_mix);

   // A framed group per section of the signal path. Stage 1 has one; the
   // filter and the oscillators get their own as they arrive. The top
   // margin is what leaves room for the heading the frame draws over it.
   // The panel is a fixed layout, and fixed_size is how it says so. Left
   // stretchable, Elements reports a maximum of 32768 in each direction,
   // the host believes the window can be pulled about, and on macOS it
   // will stretch the view and hand the new size back, which is seen as
   // the window springing when it opens.
   // A framed group per section of the signal path, laid out the way the
   // signal runs: the filter the oscillator goes through, then the
   // envelope that sweeps it and shapes the note.
   // Grouped the way the signal runs and the way a player thinks: what
   // the filter is set to, the contour that sweeps it, and the contour
   // that shapes the note. Two rows, so the window is not a letterbox.
   // A little room around each frame, so neighbours do not touch.
   auto panel = [](char const* title, auto&& content)
   {
      return margin({6, 6, 6, 6},
         group(title
          , margin({14, 42, 14, 14}, vmin_size(170, std::move(content)))
          , 1.0, false));
   };

   // Named as a virtual analog names them: the amplifier and the filter
   // are a VCA and a VCF, each with its own contour. The VCA is first, on
   // the left, and the VCF carries what it is set to and the contour that
   // sweeps it.
   view_.content(
      fixed_size({816, 504},
         margin({10, 10, 10, 10},
            vtile(
               htile(
                  panel("VCA",
                     htile(
                        captioned(hold(attack), "Attack"),
                        captioned(hold(decay), "Decay"),
                        captioned(hold(sustain_level), "Sustain"),
                        captioned(hold(release), "Release"),
                        captioned(hold(velocity), "Velocity"),
                        captioned(hold(volume), "Volume")
                     )),
                  panel("Chorus",
                     htile(
                        captioned(hold(c_rate), "Rate (Hz)"),
                        captioned(hold(c_depth), "Depth (ms)"),
                        captioned(hold(c_mix), "Mix")
                     ))
               ),
               panel("VCF",
                  htile(
                     captioned(hold(cutoff), "Cutoff"),
                     captioned(hold(resonance), "Reso"),
                     captioned(hold(env_depth), "Depth"),
                     captioned(hold(f_attack), "Attack"),
                     captioned(hold(f_decay), "Decay"),
                     captioned(hold(f_sustain), "Sustain"),
                     captioned(hold(f_release), "Release"),
                     captioned(hold(f_velocity), "Velocity")
                  ))
            )
         )
      ),
      box(bkd_color)
   );
}
