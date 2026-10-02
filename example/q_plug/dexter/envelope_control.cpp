/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include "envelope_control.hpp"
#include "palette.hpp"
#include <algorithm>

using namespace cycfi::elements;

namespace envelope
{
   float width(float rate)
   {
      return min_width + (1.0f - rate) * stage_width;
   }

   // Each point after the first holds its distance from the one before
   std::vector<curve_point> points()
   {
      return {
         {{0.0f, 0.0f}}
       , {{width(0.9f), 1.0f}, true}
       , {{width(0.5f), 0.75f}, true}
       , {{width(0.5f), 0.0f}, true}
       , {{hold_width, 0.0f}, true}
       , {{width(0.6f), 0.0f}, true}
      };
   }

   // A corner stays in the unit square, its segment within its widths
   point constrain(points_type const& pts, std::size_t i, point to)
   {
      to = in_unit_square(to);
      if (i > 0)
      {
         auto const lo = pts[i - 1].x + min_width;
         to.x = std::clamp(to.x, lo, lo + stage_width);
      }
      return to;
   }

   bool movable(std::size_t i)
   {
      return i != start_point && i != hold_point;
   }

   double rate_width::position(double v) const
   {
      return width(param->position(v));
   }

   double rate_width::value(double pos) const
   {
      return param->value(1.0 - (pos - min_width) / stage_width);
   }

   std::shared_ptr<basic_curve_editor> make(bool centered)
   {
      curve_lines lines{palette::line, palette::fill};
      lines.floor_color = palette::grid;
      if (centered)
      {
         // A pitch envelope: the middle marked, nothing filled
         lines.fill_color = palette::fill.opacity(0);
         lines.horizontal_guides = {0.0f, 0.5f};
      }
      auto e = share(curve_editor(points(), std::move(lines)
       , curve_handle{palette::line, palette::background}));
      e->constrain = constrain;
      e->movable = movable;
      return e;
   }

   std::shared_ptr<thumb_type> make_thumb()
   {
      curve_lines lines{palette::line, palette::fill};
      lines.line_width = 1.5f;
      lines.min_size = {20, 20};
      lines.horizontal_guides.clear();
      auto e = share(curve_editor(points(), std::move(lines), element{}));
      e->movable = [](std::size_t) { return false; };
      e->inset = 1.0f;
      return e;
   }
}
