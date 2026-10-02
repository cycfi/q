/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include "dexter_presenter.hpp"
#include "algorithm_boxes.hpp"
#include "envelope_control.hpp"
#include "palette.hpp"
#include "scaling_control.hpp"
#include <elements.hpp>
#include <q/synth/fm/dx7_routing.hpp>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

using namespace cycfi::elements;

namespace
{
   using f = dexter_controller;
   using element_ptr = q_plug::presenter::element_ptr;

   // A choice's button: lit while the parameter holds its value, and a
   // click sets it. A latching button stays lit, and the others go out
   // when the parameter moves, so the row behaves as radio buttons.
   struct choice_of
   {
      double position(double v) const
      {
         return std::lround(v) == k? 1.0 : 0.0;
      }

      double value(double) const
      {
         return k;
      }

      int k;
   };

   // A scaling curve's shape: lit while the curve has it, and a click
   // sets it, keeping the curve's direction, which the drag sets.
   struct curve_shape
   {
      double position(double v) const
      {
         return scaling_curve::is_exp(int(std::lround(v))) == exp;
      }

      double value(double) const
      {
         auto const c = int(std::lround(ctl->get_parameter(index)));
         return scaling_curve::of(scaling_curve::boosts(c), exp);
      }

      dexter_controller const* ctl;
      int index;
      bool exp;
   };

   auto text_button(std::string text)
   {
      return share(latching_button(
         button_styler{std::move(text)}
            .size(0.8)
            .active_body_color(palette::button_on)
      ));
   }

   auto switch_button(std::string text)
   {
      return share(toggle_button(
         button_styler{std::move(text)}
            .size(0.8)
            .active_body_color(palette::button_on)
      ));
   }

   // Anna's slider, with five labels at its quarters, bottom to top
   template <typename... Labels>
   auto labelled_slider(Labels&&... labels)
   {
      return share(slider(
         basic_rect_thumb<24, 14>()
       , slider_labels<20>(
            slider_marks_lin<28, 4, 5>(basic_track<6, true>())
          , 0.7, std::forward<Labels>(labels)...)
      ));
   }

   // The same without labels, where a readout says the value
   auto plain_slider()
   {
      return share(slider(
         basic_rect_thumb<20, 12>()
       , slider_marks_lin<20, 4, 5>(basic_track<5, true>())
      ));
   }

   template <typename Subject>
   auto captioned(Subject&& subject, char const* text)
   {
      return align_center(
         caption(vmargin({6, 6}, std::forward<Subject>(subject))
          , text, 0.8));
   }

   // A framed panel with its title inside, as Anna's
   template <typename Heading, typename Content>
   auto framed(Heading&& heading, Content&& content)
   {
      return margin({4, 4, 4, 4},
         make_group(
            margin_left_top({10, 10}, std::forward<Heading>(heading))
          , margin({14, 40, 14, 12}, std::forward<Content>(content))
          , false));
   }

   template <typename Content>
   auto framed(char const* title, Content&& content)
   {
      return framed(heading(title), std::forward<Content>(content));
   }

}

dexter_presenter::dexter_presenter(dexter_controller& ctl)
 : q_plug::presenter(ctl)
 , _ctl(ctl)
{}

///////////////////////////////////////////////////////////////////////////////
// Binding helpers
///////////////////////////////////////////////////////////////////////////////
template <typename Control>
void dexter_presenter::link(int index, std::shared_ptr<Control> control)
{
   bind(index, control, _ctl.parameters()[index]);
}

// The parameter's value as the host shows it
element_ptr dexter_presenter::readout(int index)
{
   auto r = share(label("").relative_font_size(0.9));
   auto const& p = _ctl.parameters()[index];
   view()->bindings().attach(_ctl.model(index), r
    , [&p](auto& l, double v)
      {
         char text[32];
         p.to_text(v, text, sizeof text);
         l.set_text(text);
      });
   return r;
}

// A row of buttons, one per value of an enumerated parameter
element_ptr dexter_presenter::choices(
   int index, char const* const names[], int count)
{
   auto row = share(htile_composite{});
   for (int k = 0; k != count; ++k)
   {
      auto b = text_button(names[k]);
      bind(index, b, choice_of{k});
      row->push_back(share(hmargin({1, 1}, hold(b))));
   }
   return row;
}

// A value stepped down or up by one: the algorithm, the transpose
element_ptr dexter_presenter::stepper(int index)
{
   auto step = [this, index](int by)
   {
      return [this, index, by](bool down)
      {
         if (!down)
            return;
         auto const& p = _ctl.parameters()[index];
         auto const v = std::clamp(
            _ctl.get_parameter(index) + by, p.min(), p.max());
         _ctl.begin_edit(index);
         _ctl.edit_parameter(index, v);
         _ctl.end_edit(index);
      };
   };

   auto less = button("‹", 0.8);
   auto more = button("›", 0.8);
   less.on_click = step(-1);
   more.on_click = step(1);
   return share(htile(
      hsize(24, less)
    , hsize(52, align_center_middle(hold(readout(index))))
    , hsize(24, more)
   ));
}

// A rate is its corner's distance from the point before it, a level its
// height. The start follows L4 and the hold's end L3.
void dexter_presenter::link_envelope(
   std::shared_ptr<basic_curve_editor> e, int rate, int level)
{
   auto const& params = _ctl.parameters();
   for (int i = 0; i != 4; ++i)
   {
      auto const c = envelope::corner[i];
      bind(rate + i, x_of(e, c), envelope::rate_width{&params[rate + i]});
      link(level + i, y_of(e, c));
   }
   link(level + 3, y_of(e, envelope::start_point));
   link(level + 2, y_of(e, envelope::hold_point));
}

// One operator in miniature, a button that selects it: its name and
// level, its envelope, its level as a bar, a carrier's in the line color
// and a modulator's muted, and shaded while it is switched off.
element_ptr dexter_presenter::make_thumb(int op)
{
   auto at = [op](int field) { return f::op_index(op, field); };
   auto const& params = _ctl.parameters();

   auto env = envelope::make_thumb();
   link_envelope(env, at(f::rate), at(f::env_level));
   auto tint = [this, op, env](double algorithm)
   {
      auto const n = std::clamp(int(std::lround(algorithm)), 1, 32);
      auto const carrier =
         (cycfi::q::dx7_routing[n - 1].carriers() & (1u << op)) != 0;
      auto& lines = env->actual_subject();
      lines.line_color = carrier? palette::line : palette::muted;
      lines.fill_color = carrier? palette::fill : palette::muted_fill;
      if (auto v = view())
         v->refresh(*env);
   };
   tint(_ctl.get_parameter(f::algorithm_id));
   view()->bindings().observe(_ctl.model(f::algorithm_id), tint);

   // A vertical bar fills from the bottom with its background: the
   // foreground covers what is above the value.
   auto bar = share(progress_bar(
      box(palette::line), box(palette::background)));
   auto const& level = params[at(f::level)];
   view()->bindings().attach(_ctl.model(at(f::level)), bar
    , [&level](auto& b, double v) { b.value(level.position(v)); });

   auto off = share(hidable(rbox(palette::background.opacity(0.6), 4)));
   auto shade = [this, off](double on)
   {
      off->is_hidden = on != 0;
      if (auto v = view())
         v->refresh(*off);
   };
   shade(_ctl.get_parameter(at(f::on)));
   view()->bindings().observe(_ctl.model(at(f::on)), shade);

   // Its look, plain or selected: a dark box, edged and named in the
   // selected color while it is the one being edited
   auto look = [op](bool selected)
   {
      auto const c =
         selected? palette::selected : get_theme().label_font_color;
      return layer(
         margin({6, 3, 6, 3}, align_left_top(
            label("OP" + std::to_string(op + 1))
               .relative_font_size(0.8)
               .font_color(c)))
       , frame{selected? palette::selected : palette::grid
          , selected? 2.0f : 1.0f, 4.0f}
       , rbox(rgba(31, 31, 33, 255), 4.0f)
      );
   };

   env->fit_width = true;
   auto b = share(latching_button(layer(
      hold(off),
      margin({6, 4, 6, 6}, vtile(
         align_right(hold(readout(at(f::level)))),
         vspace(2),
         htile(
            hold(env),
            hspace(4),
            hsize(5, hold(bar))
         )
      )),
      button_face(look(false), look(true))
   )));
   b->on_click = [this, op](bool) { select(op); };
   _thumbs.push_back(b);
   return b;
}

///////////////////////////////////////////////////////////////////////////////
// The panels
///////////////////////////////////////////////////////////////////////////////
namespace
{
   char const* const mode_names[] = {"Ratio", "Fixed"};
   char const* const key_names[] = {"Poly", "Mono"};
   char const* const glide_names[] = {"Retain", "Follow"};

   // A row of the envelope's numbers: its rates, then its levels
   template <typename Readout>
   auto envelope_numbers(Readout readout, int rate, int level)
   {
      auto row = share(htile_composite{});
      auto add = [&](element_ptr e)
      {
         row->push_back(share(hsize(30, align_center(hold(e)))));
      };
      row->push_back(share(label("R").relative_font_size(0.8)));
      for (int i = 0; i != 4; ++i)
         add(readout(rate + i));
      row->push_back(share(hspace(16)));
      row->push_back(share(label("L").relative_font_size(0.8)));
      for (int i = 0; i != 4; ++i)
         add(readout(level + i));
      return align_center(hold(row));
   }
}

// The algorithms, all 32 as the DX7's panel shows them, to pick one.
// The picture is drawn at a quarter of its pixels: four per point.
element_ptr dexter_presenter::make_algorithm_menu()
{
   image_grid_highlight highlight{
      palette::selected.opacity(0.25), palette::selected};
   highlight.label = [](int i) { return std::to_string(i + 1); };
   auto grid = share(image_grid_menu(_algorithms, 8, 4, 0.25f, highlight));
   grid->value(_ctl.get_parameter<int>(f::algorithm_id) - 1);
   grid->on_change = [this](int i)
   {
      _ctl.begin_edit(f::algorithm_id);
      _ctl.edit_parameter(f::algorithm_id, double(i + 1));
      _ctl.end_edit(f::algorithm_id);
   };
   return share(layer(
      margin({6, 6, 6, 6}, hold(grid))
    , box(palette::background)
    , panel{}));
}

// The algorithm in use, its picture from the menu's image. Over its
// boxes, the operator being edited is framed and the switched-off ones
// are shaded, from the table that came with the image.
element_ptr dexter_presenter::make_algorithm()
{
   image_regions::regions_type boxes(32);
   for (int a = 0; a != 32; ++a)
   {
      for (auto const& b : algorithm_boxes[a])
         boxes[a].push_back({b[0], b[1], b[2], b[3]});
   }
   image_regions regions{std::move(boxes), [this](int, int op)
   {
      if (op == _selected)
         return image_regions::lit;
      if (!_ctl.enabled(op))
         return image_regions::dimmed;
      return image_regions::normal;
   }};
   regions.lit_color = palette::selected;
   regions.dim_color = palette::background.opacity(0.7);

   auto chart =
      share(image_grid_cell(_algorithms, 8, 4, std::move(regions)));
   view()->bindings().attach(_ctl.model(f::algorithm_id), chart
    , [](auto& c, double v) { c.value(int(std::lround(v)) - 1); });
   _chart = chart;

   // The algorithm is picked from a menu of all 32, drawn as charts. The
   // menu's button reads "Algorithm" and the parameter.
   auto name = share(label(""));
   view()->bindings().attach(_ctl.model(f::algorithm_id), name
    , [](auto& l, double v)
      {
         l.set_text("Algorithm " + std::to_string(std::lround(v)));
      });
   auto pick = make_selection_menu_button(name);
   pick.position(menu_position::bottom_right);
   pick.on_open_menu = [this](basic_button_menu& b)
   {
      b.menu(hold(make_algorithm_menu()));
   };
   _algorithm = share(std::move(pick));

   auto feedback = plain_slider();
   link(f::feedback_id, feedback);
   auto sync = switch_button("Osc Sync");
   link(f::osc_sync_id, sync);

   // The operator switches, to audition each one alone or without it
   auto switches = share(htile_composite{});
   for (int op = 0; op != f::num_operators; ++op)
   {
      auto b = switch_button(std::to_string(op + 1));
      link(f::op_index(op, f::on), b);
      switches->push_back(share(hmargin({2, 2}, hsize(36, hold(b)))));
   }

   // The panel's title is the menu's button.
   return share(framed(hsize(170, hold(_algorithm)),
      vtile(
         htile(
            vsize(230, hold(_chart)),
            hsize(56, align_middle(vtile(
               align_center(hold(readout(f::feedback_id))),
               captioned(vsize(110, hold(feedback)), "Feedback")
            )))
         ),
         align_center(hsize(90, hold(sync))),
         captioned(hold(switches), "Operators On")
      )
   ));
}

element_ptr dexter_presenter::make_envelope_page(int op)
{
   auto e = envelope::make();
   auto const rate = f::op_index(op, f::rate);
   auto const level = f::op_index(op, f::env_level);
   link_envelope(e, rate, level);
   return share(vtile(
      hold(e),
      envelope_numbers(
         [this](int i) { return readout(i); }, rate, level)
   ));
}

element_ptr dexter_presenter::make_scaling_page(int op)
{
   auto at = [op](int field) { return f::op_index(op, field); };
   auto const& params = _ctl.parameters();
   auto s = scaling::make([this, at](int side)
   {
      auto const c = _ctl.get_parameter<int>(
         at(side == 0? f::left_curve : f::right_curve));
      return scaling_curve::is_exp(c);
   });
   link(at(f::break_point), x_of(s, scaling::break_point));

   // Each end's height is its depth, and which way it goes is its
   // curve's direction: a drag across the middle flips the curve between
   // a cut and a boost, keeping its shape.
   struct side
   {
      std::size_t point;
      int depth, curve;
   };
   side const sides[] = {
      {scaling::left_point, f::left_depth, f::left_curve}
    , {scaling::right_point, f::right_depth, f::right_curve}
   };
   for (auto const& side : sides)
   {
      scaling::depth_at const depth{&params[at(side.depth)], &_ctl
       , at(side.curve)};
      bind(at(side.depth), y_of(s, side.point), depth);

      // The curve parameter: its direction to the end, here and whenever
      // it changes; the lines read its shape as they draw
      auto follow = [this, s, side, depth, at](double)
      {
         auto const d = _ctl.get_parameter(at(side.depth));
         s->y(side.point, depth.position(d));
         view()->refresh(*s);
      };
      follow(_ctl.get_parameter(at(side.curve)));
      view()->bindings().observe(_ctl.model(at(side.curve)), follow);
   }
   s->on_change = [this, sides, at](std::size_t i, point pos)
   {
      for (auto const& side : sides)
      {
         if (side.point != i)
            continue;
         auto const index = at(side.curve);
         auto const c = _ctl.get_parameter<int>(index);
         auto const now = scaling_curve::of(
            pos.y >= 0.5f, scaling_curve::is_exp(c));
         if (now != c)
         {
            _ctl.begin_edit(index);
            _ctl.edit_parameter(index, now);
            _ctl.end_edit(index);
         }
      }
   };

   auto number = [this](int index, char const* name)
   {
      return hsize(110, caption(hold(readout(index)), name, 0.8));
   };
   auto numbers = htile(
      align_left(number(at(f::left_depth), "Left Depth")),
      align_center(number(at(f::break_point), "Break Point")),
      align_right(number(at(f::right_depth), "Right Depth"))
   );
   // Each side's shape, Lin or Exp; its direction is the drag's
   auto shapes = [this](int index)
   {
      auto row = share(htile_composite{});
      for (auto exp : {false, true})
      {
         auto b = text_button(exp? "Exp" : "Lin");
         bind(index, b, curve_shape{&_ctl, index, exp});
         row->push_back(share(hmargin({1, 1}, hold(b))));
      }
      return row;
   };

   return share(vtile(
      hold(s),
      hmargin({scaling::inset, scaling::inset}
       , vsize(scaling::keys_height, image{"keys.png", image::fill})),
      numbers,
      vspace(6),
      htile(
         captioned(hold(shapes(at(f::left_curve))), "Left Curve"),
         hspace(12),
         captioned(hold(shapes(at(f::right_curve))), "Right Curve")
      )
   ));
}

element_ptr dexter_presenter::make_slider_page(int op)
{
   auto at = [op](int field) { return f::op_index(op, field); };

   // The frequency as a ratio of the key's, or in hertz when fixed
   auto ratio = share(label("").font_color(palette::line));
   auto show = [this, ratio, at]()
   {
      auto const coarse = _ctl.get_parameter<int>(at(f::coarse));
      auto const fine = _ctl.get_parameter<int>(at(f::fine));
      char text[32];
      if (_ctl.get_parameter<int>(at(f::fixed)))
         std::snprintf(text, sizeof text, "%.0f Hz"
          , std::pow(10.0, (coarse % 4) + fine / 100.0));
      else
         std::snprintf(text, sizeof text, "%.2f"
          , (coarse == 0? 0.5 : coarse) * (1.0 + fine / 100.0));
      ratio->set_text(text);
      if (auto v = view())
         v->refresh(*ratio);
   };
   for (auto field : {f::fixed, f::coarse, f::fine})
      view()->bindings().observe(_ctl.model(at(field))
       , [show](auto) { show(); });
   show();

   struct { int field; char const* name; } const controls[] =
   {
      {f::level, "Level"}, {f::coarse, "Coarse"}, {f::fine, "Fine"}
    , {f::detune, "Detune"}, {f::velocity_sens, "Velocity"}
    , {f::amp_mod_sens, "AM"}, {f::rate_scaling, "Rate"}
   };
   auto row = share(htile_composite{});
   for (auto const& c : controls)
   {
      auto s = plain_slider();
      link(at(c.field), s);
      row->push_back(share(vtile(
         align_center(hold(readout(at(c.field)))),
         captioned(vsize(110, hold(s)), c.name)
      )));
   }

   return share(vtile(
      htile(
         hold(choices(at(f::fixed), mode_names, 2)),
         align_center_middle(hold(ratio))
      ),
      vspace(8),
      hold(row)
   ));
}

element_ptr dexter_presenter::make_operator()
{
   _envelopes = share(deck_composite{});
   _scalings = share(deck_composite{});
   _sliders = share(deck_composite{});
   for (int op = 0; op != f::num_operators; ++op)
   {
      _envelopes->push_back(make_envelope_page(op));
      _scalings->push_back(make_scaling_page(op));
      _sliders->push_back(make_slider_page(op));
   }

   // The thumbnails, three by two, select the operator edited
   auto rows = share(vtile_composite{});
   for (int r = 0; r != 2; ++r)
   {
      auto row = share(htile_composite{});
      for (int c = 0; c != 3; ++c)
         row->push_back(share(margin({3, 3, 3, 3}
          , hold(make_thumb(r * 3 + c)))));
      rows->push_back(share(vsize(76, hold(row))));
   }

   _title = share(label("Operator 1"));
   auto title = _title;
   auto small = [](char const* text)
   {
      return align_left(label(text).relative_font_size(0.8));
   };
   return share(framed(hold(title),
      htile(
         vtile(
            small("Envelope"),
            vsize(130, hold(_envelopes)),
            vspace(10),
            small("Keyboard Level Scaling"),
            hold(_scalings)
         ),
         hspace(14),
         hsize(300, vtile(hold(rows), vspace(8), hold(_sliders)))
      )
   ));
}

element_ptr dexter_presenter::make_pitch_envelope()
{
   auto e = envelope::make(true);
   link_envelope(e, f::pitch_rate_id, f::pitch_level_id);
   return share(framed("Pitch Envelope",
      vtile(
         hold(e),
         envelope_numbers(
            [this](int i) { return readout(i); }
          , f::pitch_rate_id, f::pitch_level_id)
      )
   ));
}

element_ptr dexter_presenter::make_lfo()
{
   // Each wave a button showing its row of the waves picture
   auto waves = share(vtile_composite{});
   for (int k = 0; k != 6; ++k)
   {
      auto wave = image_grid_cell(_waves, 1, 6, element{});
      wave.value(k);
      auto b = share(latching_button(layer(
         margin({10, 4, 10, 4}, std::move(wave))
       , button_body{palette::button_on}
      )));
      bind(f::lfo_wave_id, b, choice_of{k});
      waves->push_back(share(vmargin({1, 1}, vsize(20, hold(b)))));
   }
   auto sync = switch_button("Key Sync");
   link(f::lfo_sync_id, sync);

   struct { int index; char const* name; } const controls[] =
   {
      {f::lfo_speed_id, "Speed"}, {f::lfo_delay_id, "Delay"}
    , {f::lfo_pitch_depth_id, "Pitch"}, {f::lfo_amp_depth_id, "Amp"}
   };
   auto row = share(htile_composite{});
   for (auto const& c : controls)
   {
      auto s = labelled_slider("0", "25", "50", "75", "99");
      link(c.index, s);
      row->push_back(share(captioned(hold(s), c.name)));
   }
   auto sens = labelled_slider("0", "", "", "", "7");
   link(f::pitch_mod_sens_id, sens);
   row->push_back(share(captioned(hold(sens), "Sens")));

   return share(framed("LFO",
      htile(
         hsize(70, vtile(hold(waves), vspace(8), hold(sync))),
         hspace(8),
         hold(row)
      )
   ));
}

element_ptr dexter_presenter::make_voice()
{
   auto glide = labelled_slider("0", "25", "50", "75", "99");
   link(f::glide_id, glide);
   auto volume = labelled_slider("-60", "-45", "-30", "-15", "0");
   link(f::volume_id, volume);

   return share(framed("Voice",
      htile(
         vtile(
            captioned(hold(choices(f::mono_id, key_names, 2)), "Keys"),
            captioned(hold(stepper(f::transpose_id)), "Transpose"),
            captioned(hold(choices(f::porta_mode_id, glide_names, 2))
             , "Portamento")
         ),
         captioned(hold(glide), "Glide"),
         captioned(hold(volume), "Volume")
      )
   ));
}

///////////////////////////////////////////////////////////////////////////////
// The editor
///////////////////////////////////////////////////////////////////////////////
void dexter_presenter::select(int op)
{
   _selected = op;
   _envelopes->select(op);
   _scalings->select(op);
   _sliders->select(op);
   for (int i = 0; i != int(_thumbs.size()); ++i)
      _thumbs[i]->value(i == op);
   _title->set_text("Operator " + std::to_string(op + 1));
   if (auto v = view())
      v->refresh();
}

// The chart shades the operators switched off.
void dexter_presenter::watch()
{
   for (int op = 0; op != f::num_operators; ++op)
   {
      view()->bindings().observe(_ctl.model(f::op_index(op, f::on))
       , [this](auto)
         {
            if (auto v = view())
               v->refresh(*_chart);
         });
   }
}

// DX7 .syx files dropped on the editor: their voices become user
// presets, and the first is loaded. Elements delivers the files as paths.
bool dexter_presenter::import(drop_info const& info)
{
   auto v = view();
   if (!v || !contains_filepaths(info.data))
      return false;

   std::string report;
   std::string first;
   for (auto const& path : get_filepaths(info.data))
   {
      std::ifstream in(path, std::ios::binary);
      std::vector<std::uint8_t> syx{std::istreambuf_iterator<char>(in), {}};
      auto const file = path.filename().string();
      auto const names = _ctl.import_voices(syx, path.stem().string());
      if (names.empty())
      {
         report += file + " is not a DX7 cartridge or voice.\n";
         continue;
      }
      if (first.empty())
         first = names.front();
      report += "Imported " + std::to_string(names.size())
         + (names.size() == 1? " voice" : " voices") + " from " + file
         + ".\n";
   }

   if (!first.empty())
   {
      _ctl.load_preset(first);
      v->refresh();
   }
   open_popup(message_box1(*v, report, icons::attention, []() {}), *v);
   return !first.empty();
}

// The header's logo: a frequency modulated sine, from resources/logo.png
// (gen_images.py), drawn at a quarter of its pixels, four per point
element_ptr dexter_presenter::logo()
{
   return share(image{"logo.png", 0.25f});
}

void dexter_presenter::on_attach(elements::view& view_)
{
   _thumbs.clear();
   _algorithms = std::make_shared<cycfi::artist::image>("algorithms.png");
   _waves = std::make_shared<cycfi::artist::image>("waves.png");

   // The whole editor takes dropped files
   auto drop = share(drop_box(
      fixed_size({1100, 740},
         margin({10, 10, 10, 10},
            vtile(
               // Inset as the panels are, so its ends line up with theirs
               hmargin({4, 4}, hold(make_header())),
               htile(
                  hsize(360, hold(make_algorithm())),
                  hold(make_operator())
               ),
               vsize(246, htile(
                  hsize(360, hold(make_pitch_envelope())),
                  hsize(370, hold(make_lfo())),
                  hold(make_voice())
               ))
            )
         )
      ), {"text/uri-list"}));
   drop->on_drop = [this](drop_info const& info) { return import(info); };
   view_.content(hold(drop), box(palette::background));
   select(_selected);
   watch();
}
