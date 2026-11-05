/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include "bender_presenter.hpp"
#include "bend_view.hpp"
#include <chrono>
#include <elements.hpp>

using namespace cycfi::elements;

namespace
{
   auto constexpr bkd_color = rgba(35, 35, 37, 255);
   auto constexpr button_on = rgba(43, 83, 132, 255);

   // A redraw about 30 times a second, while the editor is open.
   auto constexpr frame_time = std::chrono::milliseconds{33};

   // The middle's two ends, as their points' heights, for the binder.
   struct middle_end
   {
      double position(double v) const
      {
         return bend::height(hi ? float(v) : -float(v));
      }

      double value(double pos) const
      {
         auto const st = bend::semitones(float(pos));
         return std::max(0.0, double(hi ? st : -st));
      }

      bool hi;
   };

   auto make_dial()
   {
      return share(dial(radial_marks<20>(basic_knob<60>())));
   }

   template <typename Subject>
   auto captioned(Subject&& subject, char const* text)
   {
      return vtile(
         align_center(std::forward<Subject>(subject)),
         align_center(margin({0, 8, 0, 0}, label(text)))
      );
   }

   auto styled(char const* text)
   {
      return button_styler{text}.size(0.8).active_body_color(button_on);
   }

   auto switch_button(char const* text)
   {
      return share(toggle_button(styled(text)));
   }

   auto push_button(char const* text)
   {
      return share(momentary_button(styled(text)));
   }

   // A button's width, centered in its panel
   template <typename Subject>
   auto centered(Subject&& subject)
   {
      return align_center(hsize(110, std::forward<Subject>(subject)));
   }

   auto framed(char const* title, auto&& content)
   {
      return margin({6, 6, 6, 6},
         group(title
          , margin({14, 42, 14, 14}, std::move(content))
          , 1.0, false));
   }
}

bender_presenter::bender_presenter(bender_controller& ctl)
 : q_plug::presenter(ctl)
 , _ctl(ctl)
{}

void bender_presenter::on_attach(elements::view& view_)
{
   using ctl = bender_controller;
   auto const& params = _ctl.parameters();

   // The bend range: each depth is its end's height, and the middle is
   // its two ends' heights. The middle's ends also sit across where the
   // range puts them, so they are placed again whenever any of the
   // three moves.
   auto editor = bend::make_editor(_ctl);
   bind(ctl::dive_id, y_of(editor, bend::bottom), bend::depth{false});
   bind(ctl::pull_up_id, y_of(editor, bend::top), bend::depth{true});
   bind(ctl::middle_id, y_of(editor, bend::middle_lo), middle_end{false});
   bind(ctl::middle_id, y_of(editor, bend::middle_hi), middle_end{true});
   for (int id : {ctl::dive_id, ctl::pull_up_id, ctl::middle_id})
   {
      view_.bindings().observe(_ctl.model(id),
         [this, editor, &view_](double)
         {
            bend::place(*editor, _ctl.range());
            view_.refresh(*editor);
         });
   }
   auto scope = share(bend::scope{_ctl});

   // The wheel moves the Bend parameter, -1 to 1, and springs back.
   auto wheel = share(bend::wheel{});
   bind(ctl::bend_id, wheel, params[ctl::bend_id]);

   // The formants.
   auto on = switch_button("On");
   bind(ctl::formants_id, on, params[ctl::formants_id]);
   auto amount = make_dial();
   bind(ctl::amount_id, amount, params[ctl::amount_id]);
   auto freeze = switch_button("Freeze");
   bind(ctl::freeze_id, freeze, params[ctl::freeze_id]);
   auto learn = push_button("Learn");
   learn->on_click = [this](bool down)
   {
      if (down)
         _ctl.worker.learn();
   };
   auto coverage = share(bend::coverage{_ctl});

   view_.content(
      fixed_size({800, 420},
         margin({10, 10, 10, 10},
            vtile(
               hmargin({6, 6}, hold(make_header())),
               htile(
                  margin({6, 6, 6, 6},
                     layer(margin({8, 10, 8, 10}, hold(wheel)), frame{})),
                  framed("Bend",
                     layer(hold(editor), hold(scope))
                  ),
                  hsize(200, framed("Formants",
                     vtile(
                        align_right(hsize(60, hold(on))),
                        captioned(
                           radial_labels<15>(hold(amount), 0.7
                            , "0", "25", "50", "75", "100")
                         , "Amount (%)"),
                        margin_top({16}, centered(hold(learn))),
                        margin_top({8}, centered(hold(freeze))),
                        margin_top({12}, hold(coverage))
                     )
                  ))
               )
            )
         )
      ),
      box(bkd_color)
   );

   // The trace and the learner's coverage move without any parameter
   // moving, so a timer redraws them.
   _attached = true;
   _tick = [this, scope, coverage, &view_]
   {
      if (!_attached)
         return;
      scope->poll();
      view_.refresh(*scope);
      view_.refresh(*coverage);
      view_.post(frame_time, _tick);
   };
   view_.post(frame_time, _tick);
}

void bender_presenter::on_detach()
{
   _attached = false;
}
