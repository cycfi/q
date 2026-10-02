/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(Q_PLUG_ANNA_5_ADSR_CONTROL_SEPTEMBER_11_2026)
#define Q_PLUG_ANNA_5_ADSR_CONTROL_SEPTEMBER_11_2026

#include <q_plug/parameter.hpp>
#include <elements.hpp>
#include <memory>

namespace elements = cycfi::elements;
namespace q_plug = cycfi::q_plug;

///////////////////////////////////////////////////////////////////////////////
// An envelope drawn as its own shape, its corners dragged to set it: an
// Elements curve_editor, given the points and the rule for moving them.
//
//          peak
//           /\__ corner ____ plateau
//          /                        \
//         /                          \
//     start                          finish
//
// Attack, decay and release each own a share of the width, so a long
// attack pushes the peak right without changing the fall after it, and
// everything at full still fits. The plateau is fixed: a sustain has no
// length, it lasts as long as the key is down. The start and the plateau
// are not dragged; the plateau follows the corner, bound to the same
// sustain parameter.
//
// The editor hands each axis of each point out as a control of its own,
// x_of and y_of, so the presenter binds an envelope exactly as it binds a
// slider, four times: a time to its corner's distance from the point
// before it, through stage, and the sustain to the corner's height.
///////////////////////////////////////////////////////////////////////////////
namespace adsr
{
   constexpr std::size_t peak = 1;
   constexpr std::size_t corner = 2;
   constexpr std::size_t plateau = 3;
   constexpr std::size_t finish = 4;

   constexpr float stage_width = 0.28f;
   constexpr float plateau_width = 0.16f;

   // A time parameter as its segment's width, for the binder. The
   // parameter's travel is 0 to 1, a share of the stage's width.
   struct stage
   {
      double         position(double v) const;
      double         value(double pos) const;

      q_plug::parameter const* param;
   };

   std::shared_ptr<elements::basic_curve_editor> make();
}

#endif
