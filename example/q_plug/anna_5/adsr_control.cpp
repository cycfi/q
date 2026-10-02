/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include "adsr_control.hpp"
#include <algorithm>

using namespace cycfi::elements;

namespace adsr
{
   namespace
   {
      auto constexpr line_color = rgba(100, 180, 230, 255);
      auto constexpr fill_color = rgba(100, 180, 230, 40);
      auto constexpr grid_color = rgba(80, 80, 84, 255);
      auto constexpr panel_color = rgba(35, 35, 37, 255);

      // Each point after the start holds its distance from the point
      // before it, so a drag on one moves those after it along.
      std::vector<curve_point> points()
      {
         return {
            {{0.0f, 0.0f}}
          , {{0.2f * stage_width, 1.0f}, true}
          , {{0.3f * stage_width, 0.6f}, true}
          , {{plateau_width, 0.6f}, true}
          , {{0.3f * stage_width, 0.0f}, true}
         };
      }

      // A corner keeps to its stage: no earlier than the point before
      // it, no later than a full stage after. The peak stays at the top,
      // the finish on the floor; only the corner of the fall is free.
      point constrain(
         basic_curve_editor::points_type const& pts, std::size_t i, point to)
      {
         to = in_unit_square(to);
         if (i > 0)
            to.x = std::clamp(to.x, pts[i - 1].x, pts[i - 1].x + stage_width);
         if (i == peak)
            to.y = 1.0f;
         else if (i == finish)
            to.y = 0.0f;
         return to;
      }

      bool movable(std::size_t i)
      {
         return i == peak || i == corner || i == finish;
      }
   }

   double stage::position(double v) const
   {
      return param->position(v) * stage_width;
   }

   double stage::value(double pos) const
   {
      return param->value(pos / stage_width);
   }

   std::shared_ptr<basic_curve_editor> make()
   {
      curve_lines lines{line_color, fill_color};
      lines.floor_color = grid_color;
      auto e = share(curve_editor(points(), std::move(lines)
       , curve_handle{line_color, panel_color}));
      e->constrain = constrain;
      e->movable = movable;
      return e;
   }
}
