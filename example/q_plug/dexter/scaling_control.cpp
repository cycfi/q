/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include "scaling_control.hpp"
#include "palette.hpp"
#include <algorithm>
#include <cmath>

using namespace cycfi::elements;

namespace scaling
{
   std::vector<curve_point> points()
   {
      return {{{0.0f, 0.5f}}, {{0.39f, 0.5f}}, {{1.0f, 0.5f}}};
   }

   // The break point along the middle, the ends up and down their edges
   point constrain(points_type const& pts, std::size_t i, point to)
   {
      to = in_unit_square(to);
      if (i == break_point)
         to.y = 0.5f;
      else
         to.x = pts[i].x;
      return to;
   }

   double depth_at::position(double v) const
   {
      auto const c = ctl->get_parameter<int>(curve_index);
      auto const d = param->position(v) / 2;
      return scaling_curve::boosts(c)? 0.5 + d : 0.5 - d;
   }

   double depth_at::value(double pos) const
   {
      return param->value(std::abs(pos - 0.5) * 2);
   }

   // A side's curve, exponential: (e^3d - 1) / (e^3 - 1) of the depth at
   // a distance d from the break point, 0 there and 1 at the keyboard's
   // end. The left side's segment runs from its end in to the break
   // point, so it is that curve run backward.
   std::shared_ptr<basic_curve_editor> make(exp_function exp)
   {
      auto curve = [](float d)
      {
         return (std::exp(3.0f * d) - 1.0f) / (std::exp(3.0f) - 1.0f);
      };

      curve_lines lines{palette::line, palette::fill.opacity(0)};
      lines.floor_color = palette::grid;
      lines.horizontal_guides = {0.5f};
      lines.vertical_guides = {break_point};
      lines.shape = [exp, curve](std::size_t segment, float t)
      {
         if (!exp(int(segment)))
            return t;
         return segment == 0? 1.0f - curve(1.0f - t) : curve(t);
      };

      auto e = share(curve_editor(points(), std::move(lines)
       , curve_handle{palette::line, palette::background}));
      e->constrain = constrain;
      e->inset = inset;
      return e;
   }
}
