/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include "adsr_control.hpp"

using namespace cycfi::elements;
using shape = qplug::adsr_shape;

namespace
{
   // How close the cursor must come to a corner to take hold of it, and
   // how big the corners are drawn.
   constexpr float reach = 12.0f;
   constexpr float corner_radius = 5.0f;

   auto constexpr line_color = rgba(100, 180, 230, 255);
   auto constexpr fill_color = rgba(100, 180, 230, 40);
   auto constexpr grid_color = rgba(80, 80, 84, 255);
}

view_limits adsr_control::limits(basic_context const& /*ctx*/) const
{
   // Room enough to read, and free to grow with the panel.
   return {{180, 90}, {full_extent, full_extent}};
}

point adsr_control::to_screen(shape::point p, rect b) const
{
   return {b.left + (p.x * b.width()), b.bottom - (p.y * b.height())};
}

shape::point adsr_control::from_screen(point p, rect b) const
{
   return {(p.x - b.left) / b.width(), (b.bottom - p.y) / b.height()};
}

rect adsr_control::drawing(context const& ctx) const
{
   // Inset, so a corner sitting at the very edge is still drawn whole
   // rather than half outside.
   return ctx.bounds.inset(corner_radius + 2, corner_radius + 2);
}

adsr_control::shape::handle
adsr_control::pick(context const& ctx, point p) const
{
   auto const b = drawing(ctx);
   auto const r = reach / std::min(b.width(), b.height());
   return values.nearest(from_screen(p, b), r);
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

   auto const start = to_screen(values.start(), b);
   auto const peak = to_screen(values.peak(), b);
   auto const corner = to_screen(values.corner(), b);
   auto const plateau = to_screen(values.plateau(), b);
   auto const finish = to_screen(values.finish(), b);

   // A line along the bottom, so the shape has a floor to stand on.
   cnv.line_width(1);
   cnv.stroke_style(grid_color);
   cnv.begin_path();
   cnv.move_to({ctx.bounds.left, b.bottom});
   cnv.line_to({ctx.bounds.right, b.bottom});
   cnv.stroke();

   // The envelope, filled under and drawn over.
   cnv.begin_path();
   cnv.move_to(start);
   cnv.line_to(peak);
   cnv.line_to(corner);
   cnv.line_to(plateau);
   cnv.line_to(finish);
   cnv.fill_style(fill_color);
   cnv.fill_preserve();
   cnv.line_width(2);
   cnv.stroke_style(line_color);
   cnv.stroke();

   // The corners that can be taken hold of. The one under the cursor, or
   // the one being dragged, is drawn larger.
   struct { shape::handle which; point at; } const corners[] =
   {
      {shape::attack_handle, peak}
    , {shape::decay_handle, corner}
    , {shape::release_handle, finish}
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
      if (_dragging != shape::no_handle && on_gesture)
         on_gesture(_dragging, true);
   }
   else if (_dragging != shape::no_handle)
   {
      if (on_gesture)
         on_gesture(_dragging, false);
      _dragging = shape::no_handle;
      ctx.view.refresh(ctx);
   }

   tracker<>::click(ctx, btn);
   return _dragging != shape::no_handle || !btn.down;
}

// Move one corner and report whatever it changed. The corner of the fall
// carries two values, and either may be the one that moved.
void adsr_control::move(shape::handle which, shape::point to)
{
   auto const before = values;
   values.drag(which, to);

   if (on_change)
   {
      if (values.attack != before.attack)
         on_change(shape::attack_handle, values.attack);
      if (values.decay != before.decay)
         on_change(shape::decay_handle, values.decay);
      if (values.release != before.release)
         on_change(shape::release_handle, values.release);
      if (values.sustain != before.sustain)
         on_change(shape::no_handle, values.sustain);   // the level
   }
}

void adsr_control::keep_tracking(context const& ctx, tracker_info& track)
{
   if (_dragging == shape::no_handle)
      return;

   move(_dragging, from_screen(track.current, drawing(ctx)));
   ctx.view.refresh(ctx);
}

bool adsr_control::cursor(context const& ctx, point p, cursor_tracking status)
{
   auto const was = _hot;
   _hot = (status == cursor_tracking::leaving)?
      shape::no_handle : pick(ctx, p);

   if (_hot != was)
      ctx.view.refresh(ctx);
   return _hot != shape::no_handle;
}
