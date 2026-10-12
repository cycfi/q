/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include "looper_presenter.hpp"
#include <elements.hpp>

looper_presenter::looper_presenter(looper_controller& ctl)
 : q_plug::presenter(ctl)
 , _ctl(ctl)
{}

using namespace cycfi::elements;

namespace
{
   auto constexpr bkd_color = rgba(35, 35, 37, 255);
   auto constexpr no_sample = "Drop a WAV file here";
   auto constexpr knob_scale = 0.4f;

   // A colour for each button, and two for Play: green to play, red to
   // pause.
   auto constexpr rewind_color = rgba(110, 120, 140, 255);
   auto constexpr play_color = rgba(46, 160, 67, 255);
   auto constexpr pause_color = rgba(200, 50, 50, 255);
   auto constexpr loop_color = rgba(30, 144, 255, 255);
}

void looper_presenter::on_attach(elements::view& view_)
{
   using ctl = looper_controller;

   // Play and Pause share one button, bound to the play parameter. Rewind
   // goes straight to the controller: it holds no value for the host.
   auto play = share(toggle_icon_button(
      icons::play, icons::pause, 1.5, play_color, pause_color));
   bind(ctl::play_id, play, _ctl.parameters()[ctl::play_id]);

   auto rewind = share(icon_button(icons::to_start, 1.5, rewind_color));
   rewind->on_click = [this](bool) { _ctl.rewind(); };

   // Loop is a switch: a straight arrow plays once, a circle loops.
   auto loop = share(toggle_icon_button(
      icons::right, icons::cycle, 1.5, loop_color));
   bind(ctl::loop_id, loop, _ctl.parameters()[ctl::loop_id]);

   // A dial for the volume, drawn from Elements' white knob sprites, with
   // Gain's console taper.
   auto const& param = _ctl.volume_param();
   auto knob = sprite{"knob_sprites_white_128x128.png", 128 * knob_scale
    , knob_scale};
   auto volume = share(dial(radial_marks<15>(knob)));
   bind(ctl::volume_id, volume, db_scale{param.min(), param.max()});

   // The drop box holds the controls, with the sample's name under them.
   // A WAV file dropped on it is loaded, on the main thread, and its name
   // shown.
   auto const& name = _ctl.sample_name();
   auto sample = share(label(name.empty()? no_sample : name));
   auto controls = htile(
      align_middle(margin_right(10, hold(rewind))),
      align_middle(margin_right(10, hold(play))),
      align_middle(margin_right(30, hold(loop))),
      align_middle(hold(volume))
   );
   auto drop = share(drop_box(
      layer(
         margin({20, 20, 20, 20},
            vtile(
               align_center(controls),
               margin_top(15, align_center(hold(sample)))
            )
         ),
         frame{}
      )
    , {"text/uri-list"}));

   drop->on_drop =
      [this, &view_, sample = get(sample)](drop_info const& info)
      {
         if (!contains_filepaths(info.data))
            return false;
         auto paths = get_filepaths(info.data);
         if (paths.size() != 1 || !_ctl.load(paths[0]))
            return false;
         if (auto l = sample.lock())
         {
            l->set_text(_ctl.sample_name());
            view_.refresh(*l);
         }
         return true;
      };

   view_.content(
      fixed_size({380, 190},
         margin({20, 20, 20, 20}, hold(drop))
      ),
      box(bkd_color)
   );
}
