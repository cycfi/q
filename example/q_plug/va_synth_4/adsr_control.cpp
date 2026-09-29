/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include "adsr_control.hpp"
#include <algorithm>
#include <cmath>

using namespace cycfi::elements;

namespace
{
   // How close the cursor must come to a corner to take hold of it, and
   // how big the corners are drawn.
   constexpr float reach = 12.0f;
   constexpr float corner_radius = 5.0f;

   auto constexpr line_color = rgba(100, 180, 230, 255);
   auto constexpr fill_color = rgba(100, 180, 230, 40);
   auto constexpr grid_color = rgba(80, 80, 84, 255);

   float clamp01(float v)
   {
      return std::clamp(v, 0.0f, 1.0f);
   }
}

///////////////////////////////////////////////////////////////////////////////
// The shape
///////////////////////////////////////////////////////////////////////////////
adsr_control::unit_point adsr_control::start() const
{
   return {0.0f, 0.0f};
}

adsr_control::unit_point adsr_control::peak() const
{
   return {_attack * stage_width, 1.0f};
}

adsr_control::unit_point adsr_control::corner() const
{
   return {peak().x + (_decay * stage_width), _sustain};
}

adsr_control::unit_point adsr_control::plateau() const
{
   return {corner().x + plateau_width, _sustain};
}

adsr_control::unit_point adsr_control::finish() const
{
   return {plateau().x + (_release * stage_width), 0.0f};
}

adsr_control::handle
adsr_control::nearest(unit_point p, float reach_) const
{
   struct { handle which; unit_point at; } const corners[] =
   {
      {attack_handle, peak()}
    , {decay_handle, corner()}
    , {release_handle, finish()}
   };

   auto best = no_handle;
   auto best_distance = reach_;
   for (auto const& c : corners)
   {
      auto const d = std::hypot(p.x - c.at.x, p.y - c.at.y);
      if (d <= best_distance)
      {
         best = c.which;
         best_distance = d;
      }
   }
   return best;
}

// Each corner carries the values it is made of and no others, so a drag
// says one thing at a time, except the corner of the fall, which is
// where the decay and the sustain meet and so carries both.
void adsr_control::take(handle which, unit_point to)
{
   switch (which)
   {
      case attack_handle:
         _attack = clamp01(to.x / stage_width);
         break;

      case decay_handle:
         _decay = clamp01((to.x - peak().x) / stage_width);
         _sustain = clamp01(to.y);
         break;

      case release_handle:
         _release = clamp01((to.x - plateau().x) / stage_width);
         break;

      default:
         break;
   }
}

///////////////////////////////////////////////////////////////////////////////
// The control
///////////////////////////////////////////////////////////////////////////////
view_limits adsr_control::limits(basic_context const& /*ctx*/) const
{
   // Room enough to read, and free to grow with the panel.
   return {{180, 90}, {full_extent, full_extent}};
}

point adsr_control::to_screen(unit_point p, rect b) const
{
   return {b.left + (p.x * b.width()), b.bottom - (p.y * b.height())};
}

adsr_control::unit_point adsr_control::from_screen(point p, rect b) const
{
   return {(p.x - b.left) / b.width(), (b.bottom - p.y) / b.height()};
}

rect adsr_control::drawing(context const& ctx) const
{
   // Inset, so a corner sitting at the very edge is still drawn whole
   // rather than half outside.
   return ctx.bounds.inset(corner_radius + 2, corner_radius + 2);
}

adsr_control::handle adsr_control::pick(context const& ctx, point p) const
{
   auto const b = drawing(ctx);
   auto const r = reach / std::min(b.width(), b.height());
   return nearest(from_screen(p, b), r);
}

void adsr_control::draw(context const& ctx)
{
   auto& cnv = ctx.canvas;

   // Nothing is drawn outside these bounds. Without this a corner near
   // the edge, or a line on its way to one, would leave a mark on the
   // panel that nothing owns and so nothing erases.
   auto state = cnv.new_state();
   cnv.add_rect(ctx.bounds);
   cnv.clip();

   auto b = drawing(ctx);

   auto const start_ = to_screen(start(), b);
   auto const peak_ = to_screen(peak(), b);
   auto const corner_ = to_screen(corner(), b);
   auto const plateau_ = to_screen(plateau(), b);
   auto const finish_ = to_screen(finish(), b);

   // A line along the bottom, so the shape has a floor to stand on.
   cnv.line_width(1);
   cnv.stroke_style(grid_color);
   cnv.begin_path();
   cnv.move_to({ctx.bounds.left, b.bottom});
   cnv.line_to({ctx.bounds.right, b.bottom});
   cnv.stroke();

   // The envelope, filled under and drawn over.
   cnv.begin_path();
   cnv.move_to(start_);
   cnv.line_to(peak_);
   cnv.line_to(corner_);
   cnv.line_to(plateau_);
   cnv.line_to(finish_);
   cnv.fill_style(fill_color);
   cnv.fill_preserve();
   cnv.line_width(2);
   cnv.stroke_style(line_color);
   cnv.stroke();

   // The corners that can be taken hold of. The one under the cursor, or
   // the one being dragged, is drawn larger.
   struct { handle which; point at; } const corners[] =
   {
      {attack_handle, peak_}
    , {decay_handle, corner_}
    , {release_handle, finish_}
   };

   for (auto const& c : corners)
   {
      auto const live = (c.which == _dragging) || (c.which == _hot);
      auto const r = live? corner_radius + 2 : corner_radius;

      cnv.begin_path();
      cnv.add_circle(circle{c.at.x, c.at.y, r});
      cnv.fill_style(live? line_color : rgba(35, 35, 37, 255));
      cnv.fill_preserve();
      cnv.line_width(2);
      cnv.stroke_style(line_color);
      cnv.stroke();
   }
}

bool adsr_control::click(context const& ctx, mouse_button btn)
{
   if (btn.down)
   {
      // Take hold of the corner under the cursor, if there is one.
      _dragging = pick(ctx, btn.pos);
      if (_dragging != no_handle)
         gesture(_dragging, true);
   }
   else if (_dragging != no_handle)
   {
      gesture(_dragging, false);
      _dragging = no_handle;
      ctx.view.refresh(ctx);
   }

   tracker<>::click(ctx, btn);
   return _dragging != no_handle || !btn.down;
}

// A drag on a corner is a gesture on each value the corner carries.
void adsr_control::gesture(handle which, bool begin)
{
   auto call = [begin](gesture_function const& f)
   {
      if (f)
         f(begin);
   };

   switch (which)
   {
      case attack_handle:
         call(on_attack_gesture);
         break;

      case decay_handle:
         call(on_decay_gesture);
         call(on_sustain_gesture);
         break;

      case release_handle:
         call(on_release_gesture);
         break;

      default:
         break;
   }
}

// Move one corner and report whatever it changed. The corner of the fall
// carries two values, and either may be the one that moved.
void adsr_control::move(handle which, unit_point to)
{
   auto const attack = _attack;
   auto const decay = _decay;
   auto const sustain = _sustain;
   auto const release = _release;
   take(which, to);

   auto report = [](change_function const& f, float was, float now)
   {
      if (f && now != was)
         f(now);
   };

   report(on_attack_change, attack, _attack);
   report(on_decay_change, decay, _decay);
   report(on_sustain_change, sustain, _sustain);
   report(on_release_change, release, _release);
}

void adsr_control::keep_tracking(context const& ctx, tracker_info& track)
{
   if (_dragging == no_handle)
      return;

   move(_dragging, from_screen(track.current, drawing(ctx)));
   ctx.view.refresh(ctx);
}

bool adsr_control::cursor(context const& ctx, point p, cursor_tracking status)
{
   auto const was = _hot;
   _hot = (status == cursor_tracking::leaving)? no_handle : pick(ctx, p);

   if (_hot != was)
      ctx.view.refresh(ctx);
   return _hot != no_handle;
}
