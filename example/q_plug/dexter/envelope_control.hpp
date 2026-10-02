/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(Q_PLUG_DEXTER_ENVELOPE_CONTROL_OCTOBER_1_2026)
#define Q_PLUG_DEXTER_ENVELOPE_CONTROL_OCTOBER_1_2026

#include <q_plug/parameter.hpp>
#include <elements.hpp>
#include <memory>
#include <vector>

namespace elements = cycfi::elements;
namespace q_plug = cycfi::q_plug;

///////////////////////////////////////////////////////////////////////////////
// A DX7 envelope as a curve_editor: six points, the start and the end at
// L4, corners at L1, L2 and L3, and the end of the hold at L3, where the
// envelope waits while the key is down. A corner's height is its level
// and its distance from the point before it is its rate, a faster rate a
// shorter segment: each segment is at least min_width wide and grows by
// stage_width as its rate slows. All four at their slowest still fit.
//
// The start and the hold's end are not dragged: they follow L4 and L3,
// bound to the same parameters as the corners. The thumbnails are the
// same editor in miniature, bound the same way.
///////////////////////////////////////////////////////////////////////////////
namespace envelope
{
   using elements::point;
   using elements::curve_point;
   using points_type = elements::basic_curve_editor::points_type;

   constexpr float min_width = 0.06f;
   constexpr float stage_width = 0.16f;
   constexpr float hold_width = 0.12f;

   // The point of each rate and level, and the two that follow a level
   constexpr std::size_t corner[4] = {1, 2, 3, 5};
   constexpr std::size_t start_point = 0;    // follows L4
   constexpr std::size_t hold_point = 4;     // follows L3

   // A segment's width for a rate, 0 to 1
   float             width(float rate);

   // The editor's points, where a corner may go and which points move
   std::vector<curve_point>
                     points();
   point             constrain(
                        points_type const& pts, std::size_t i, point to);
   bool              movable(std::size_t i);

   // A rate parameter as its segment's width, for the binder: the
   // parameter's own travel, the fastest rate the shortest segment.
   struct rate_width
   {
      double         position(double v) const;
      double         value(double pos) const;

      q_plug::parameter const* param;
   };

   // An envelope editor in Anna's blue. A pitch envelope is centered: a
   // line marks the middle, where level 50 leaves the pitch alone, and
   // nothing is filled.
   std::shared_ptr<elements::basic_curve_editor> make(bool centered = false);

   // The same envelope in miniature, for a thumbnail: no handles, nothing
   // taken hold of, its colors free to change
   using thumb_type =
      elements::proxy<elements::curve_lines, elements::basic_curve_editor>;
   std::shared_ptr<thumb_type>
                     make_thumb();
}

#endif
