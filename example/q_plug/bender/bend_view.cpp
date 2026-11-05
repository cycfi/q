/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include "bend_view.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

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
