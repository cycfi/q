/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include "bend_view.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <numbers>

using namespace cycfi::elements;

namespace bend
{
   namespace
   {
      auto constexpr line_color = rgba(42, 163, 224, 255);
      auto constexpr trace_color = rgba(42, 163, 224, 90);
      auto constexpr zone_color = rgba(67, 160, 71, 30);
      auto constexpr grid_color = rgba(58, 58, 62, 255);
      auto constexpr axis_color = rgba(85, 85, 90, 255);
      auto constexpr label_color = rgba(138, 138, 144, 255);
      auto constexpr dot_color = rgba(255, 255, 255, 255);
      auto constexpr panel_color = rgba(35, 35, 37, 255);

      // Where the middle ends, in the bar's travel from the center, 0 to
      // 1, and in semitones: bend_range's own (note 1 in its header).
      struct middle_end
      {
         float       travel;
         float       semitones;
      };

      middle_end middle_of(q::midi::bend_range r)
      {
         auto const k = (r.down + r.up) / 2;
         auto const m = std::clamp(r.linear, 0.0f, std::min(r.down, r.up));
         return {k > 0 ? m / k : 0.0f, m};
      }

      // The bar's travel, -1 to 1, that plays these semitones.
      float travel_of(q::midi::bend_range r, float st)
      {
         auto const [w, m] = middle_of(r);
         auto const depth = st < 0 ? r.down : r.up;
         auto const a = std::abs(st);
         float x;
         if (a <= m)
            x = m > 0 ? a / m * w : 0.0f;
         else
            x = depth > m ? w + (a - m) / (depth - m) * (1 - w) : 1.0f;
         return std::copysign(std::min(x, 1.0f), st);
      }

      float x_of_travel(float t) { return (t + 1) / 2; }

      // Ends move up and down only, each in its own half; the middle's
      // ends keep their place across and stay within the smaller depth.
      point constrain(
         basic_curve_editor::points_type const& pts, std::size_t i, point to)
      {
         to = in_unit_square(to);
         auto const dive = -semitones(pts[bottom].y);
         auto const up = semitones(pts[top].y);
         auto const lim = std::min(dive, up);
         switch (i)
         {
            case bottom:
               return {0.0f, std::clamp(to.y, 0.0f, 0.5f)};
            case top:
               return {1.0f, std::clamp(to.y, 0.5f, 1.0f)};
            case middle_lo:
               return {pts[i].x, std::clamp(to.y, height(-lim), 0.5f)};
            default:
               return {pts[i].x, std::clamp(to.y, 0.5f, height(lim))};
         }
      }
   }

   double depth::position(double v) const
   {
      return height(up ? float(v) : -float(v));
   }

   double depth::value(double pos) const
   {
      auto const st = semitones(float(pos));
      return std::clamp(double(up ? st : -st), 0.0, double(frame));
   }

   std::shared_ptr<basic_curve_editor>
   make_editor(bender_controller const& ctl)
   {
      curve_lines lines{line_color, rgba(0, 0, 0, 0)};
      lines.horizontal_guides.clear();
      lines.line_width = 2.5f;

      auto e = share(curve_editor(
         {{{0.0f, 0.5f}}, {{0.5f, 0.5f}}, {{0.5f, 0.5f}}, {{1.0f, 0.5f}}}
       , std::move(lines)
       , curve_handle{line_color, panel_color}));
      e->inset = inset;
      e->constrain = constrain;
      place(*e, ctl.range());
      return e;
   }

   void place(basic_curve_editor& e, q::midi::bend_range r)
   {
      auto const [w, m] = middle_of(r);
      e.y(bottom, height(-r.down));
      e.x(middle_lo, x_of_travel(-w));
      e.y(middle_lo, height(-m));
      e.x(middle_hi, x_of_travel(w));
      e.y(middle_hi, height(m));
      e.y(top, height(r.up));
   }

   ////////////////////////////////////////////////////////////////////////////
   scope::scope(bender_controller& ctl)
    : _ctl{ctl}
    , _trace(
         std::size_t(trace_time / bender_controller::scope_period), 0.0f)
   {}

   view_limits scope::limits(basic_context const&) const
   {
      return {{240, 160}, {full_extent, full_extent}};
   }

   void scope::poll()
   {
      _ctl.scope.consume(
         [this](void* p, std::size_t)
         {
            std::memcpy(&_now, p, sizeof(float));
            _trace[_head] = _now;
            _head = (_head + 1) % _trace.size();
         }
      );
   }

   void scope::draw(context const& ctx)
   {
      auto& cnv = ctx.canvas;
      auto state = cnv.new_state();
      auto const plot = ctx.bounds.inset(inset);
      auto const range = _ctl.range();
      auto sx = [&](float x) { return plot.left + x * plot.width(); };
      auto sy = [&](float st)
      {
         return plot.bottom - height(st) * plot.height();
      };

      // The grid, a line every 3 semitones, labeled; the center brighter.
      cnv.font(get_theme().label_font.size(10));
      cnv.text_align(canvas::left | canvas::middle);
      for (int st = -12; st <= 12; st += 3)
      {
         cnv.begin_path();
         cnv.move_to(plot.left, sy(st));
         cnv.line_to(plot.right, sy(st));
         cnv.stroke_style(st == 0 ? axis_color : grid_color);
         cnv.line_width(1);
         cnv.stroke();

         char text[8];
         std::snprintf(text, sizeof(text), st > 0 ? "+%d" : "%d", st);
         cnv.fill_style(label_color);
         cnv.fill_text(text, plot.left + 4, sy(st) - 7);
      }
      cnv.begin_path();
      cnv.move_to(sx(0.5f), plot.top);
      cnv.line_to(sx(0.5f), plot.bottom);
      cnv.stroke_style(axis_color);
      cnv.stroke();

      // The middle, shaded.
      auto const w = middle_of(range).travel;
      cnv.begin_path();
      cnv.add_rect(sx(x_of_travel(-w)), plot.top
       , sx(x_of_travel(w)) - sx(x_of_travel(-w)), plot.height());
      cnv.fill_style(zone_color);
      cnv.fill();

      // The trace, oldest at the left.
      auto const n = _trace.size();
      cnv.begin_path();
      for (std::size_t i = 0; i != n; ++i)
      {
         auto const v = std::clamp(_trace[(_head + i) % n], -frame, frame);
         auto const x = plot.left + plot.width() * float(i) / float(n - 1);
         if (i == 0)
            cnv.move_to(x, sy(v));
         else
            cnv.line_to(x, sy(v));
      }
      cnv.stroke_style(trace_color);
      cnv.line_width(2);
      cnv.stroke();

      // Where the bar is now, and its semitones.
      auto const now = std::clamp(_now, -frame, frame);
      cnv.begin_path();
      cnv.add_circle(sx(x_of_travel(travel_of(range, now))), sy(now), 5);
      cnv.fill_style(dot_color);
      cnv.fill();

      char text[16];
      std::snprintf(text, sizeof(text), "%+.1f st", _now);
      cnv.font(get_theme().label_font.size(13));
      cnv.text_align(canvas::right | canvas::top);
      cnv.fill_style(line_color);
      cnv.fill_text(text, plot.right - 4, plot.top + 4);
   }

   ////////////////////////////////////////////////////////////////////////////
   wheel::wheel()
    : basic_slider_base{0.5}
   {}

   view_limits wheel::limits(basic_context const&) const
   {
      return {{width + 14, 160}, {width + 14, full_extent}};
   }

   void wheel::begin_tracking(context const&, tracker_info& track)
   {
      _grab_value = value();
      _grab_y = track.current.y;
   }

   // The whole travel, bottom to top, is the wheel's height.
   void wheel::keep_tracking(context const& ctx, tracker_info& track)
   {
      auto const h = ctx.bounds.height();
      auto const v = std::clamp(
         _grab_value - (track.current.y - _grab_y) / h, 0.0, 1.0);
      if (v != value())
      {
         edit_value(v);
         ctx.view.refresh(ctx);
      }
   }

   void wheel::end_tracking(context const& ctx, tracker_info&)
   {
      if (value() != 0.5)
      {
         edit_value(0.5);
         ctx.view.refresh(ctx);
      }
   }

   // The scroll wheel does not move it: a trackpad's momentum would keep
   // it rolling after the fingers lift, and no host says when they do.
   bool wheel::scroll(context const&, point, point)
   {
      return false;
   }

   // A broad, matte black wheel in a recessed slot, after the MONTAGE M's.
   // The face darkens toward the ends, where it turns away; a concave
   // detent across its middle marks the center and rolls with the wheel.
   // The sides roll off into the dark, so the edges read round.
   void wheel::draw(context const& ctx)
   {
      constexpr auto pi = std::numbers::pi_v<float>;
      auto& cnv = ctx.canvas;
      auto state = cnv.new_state();
      auto const b = ctx.bounds;
      auto const x = b.left + (b.width() - width) / 2;
      auto const top = b.top + 6;
      auto const h = b.height() - 12;
      auto const cy = top + h / 2;
      auto const roll = float(0.5 - value()) * 0.8f * pi;
      constexpr float fillet = 9.0f;

      // The slot, a shade darker than the panel, its lower lip lit.
      cnv.begin_path();
      cnv.add_round_rect(x - 6, top - 6, width + 12, h + 12, 11);
      cnv.fill_style(rgba(14, 14, 15, 255));
      cnv.fill();
      cnv.begin_path();
      cnv.move_to(x - 2, top + h + 5.5f);
      cnv.line_to(x + width + 2, top + h + 5.5f);
      cnv.stroke_style(rgba(70, 70, 75, 255));
      cnv.line_width(1);
      cnv.stroke();

      // The face.
      auto face = canvas::linear_gradient{{0, top}, {0, top + h}};
      face.add_color_stop(0.0f, rgba(6, 6, 7, 255));
      face.add_color_stop(0.25f, rgba(24, 24, 26, 255));
      face.add_color_stop(0.5f, rgba(32, 32, 35, 255));
      face.add_color_stop(0.75f, rgba(24, 24, 26, 255));
      face.add_color_stop(1.0f, rgba(6, 6, 7, 255));
      cnv.begin_path();
      cnv.add_round_rect(x, top, width, h, fillet);
      cnv.fill_style(face);
      cnv.fill();

      // The fillet along the sides: the face rolls off into the dark over
      // its last few pixels, so its edges read round, not cut.
      auto sides = canvas::linear_gradient{{x, 0}, {x + width, 0}};
      auto const f = fillet / width;
      sides.add_color_stop(0.0f, color{0, 0, 0, 0.75f});
      sides.add_color_stop(f * 0.5f, color{0, 0, 0, 0.3f});
      sides.add_color_stop(f, color{0, 0, 0, 0.0f});
      sides.add_color_stop(1 - f, color{0, 0, 0, 0.0f});
      sides.add_color_stop(1 - f * 0.5f, color{0, 0, 0, 0.3f});
      sides.add_color_stop(1.0f, color{0, 0, 0, 0.75f});
      cnv.begin_path();
      cnv.add_round_rect(x, top, width, h, fillet);
      cnv.fill_style(sides);
      cnv.fill();

      // The concave detent at the center, where the wheel has rolled it: a
      // rounded scoop cut into the face, sharp where it meets it. Lit from
      // above, its upper wall faces down and starts dark at a crisp edge;
      // its lower wall faces up and brightens to a crisp edge.
      auto const half = 0.065f * pi;
      auto const a0 = roll - half, a1 = roll + half;
      if (a1 > -pi / 2 && a0 < pi / 2)
      {
         auto const y0 = cy + std::sin(std::max(a0, -pi / 2)) * h / 2;
         auto const y1 = cy + std::sin(std::min(a1, pi / 2)) * h / 2;
         auto const lit = 0.4f + 0.6f * std::cos(roll);
         auto dip = canvas::linear_gradient{{0, y0}, {0, y1}};
         dip.add_color_stop(0.0f, color{0, 0, 0, 0.5f});
         dip.add_color_stop(0.12f, color{0, 0, 0, 0.32f});
         dip.add_color_stop(0.5f, color{0, 0, 0, 0.1f});
         dip.add_color_stop(0.85f, color{1, 1, 1, 0.04f * lit});
         dip.add_color_stop(1.0f, color{1, 1, 1, 0.07f * lit});
         cnv.begin_path();
         cnv.add_rect(x, y0, width, y1 - y0);
         cnv.fill_style(dip);
         cnv.fill();
      }

      // The slot's notches at the center.
      cnv.fill_style(rgba(200, 200, 204, 255));
      cnv.begin_path();
      cnv.move_to(x - 6, cy - 5);
      cnv.line_to(x - 1, cy);
      cnv.line_to(x - 6, cy + 5);
      cnv.fill();
      cnv.begin_path();
      cnv.move_to(x + width + 6, cy - 5);
      cnv.line_to(x + width + 1, cy);
      cnv.line_to(x + width + 6, cy + 5);
      cnv.fill();
   }

   ////////////////////////////////////////////////////////////////////////////
   coverage::coverage(bender_controller const& ctl)
    : _ctl{ctl}
   {}

   view_limits coverage::limits(basic_context const&) const
   {
      return {{120, 24}, {full_extent, 24}};
   }

   void coverage::draw(context const& ctx)
   {
      auto& cnv = ctx.canvas;
      auto state = cnv.new_state();
      auto const b = ctx.bounds;
      auto const c = std::clamp(_ctl.worker.coverage(), 0.0f, 1.0f);

      cnv.begin_path();
      cnv.add_round_rect(b.left, b.top, b.width(), 6, 3);
      cnv.fill_style(rgba(58, 58, 62, 255));
      cnv.fill();
      cnv.begin_path();
      cnv.add_round_rect(b.left, b.top, b.width() * c, 6, 3);
      cnv.fill_style(line_color);
      cnv.fill();

      char text[24];
      std::snprintf(text, sizeof(text), "learned %d%%", int(c * 100 + 0.5f));
      cnv.font(get_theme().label_font.size(11));
      cnv.text_align(canvas::center | canvas::top);
      cnv.fill_style(label_color);
      cnv.fill_text(text, b.left + b.width() / 2, b.top + 9);
   }
}
