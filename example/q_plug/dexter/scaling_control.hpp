/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(Q_PLUG_DEXTER_SCALING_CONTROL_OCTOBER_1_2026)
#define Q_PLUG_DEXTER_SCALING_CONTROL_OCTOBER_1_2026

#include "dexter_controller.hpp"
#include <elements.hpp>
#include <functional>
#include <memory>
#include <vector>

namespace elements = cycfi::elements;
namespace q_plug = cycfi::q_plug;

///////////////////////////////////////////////////////////////////////////////
// A DX7 keyboard scaling curve, 0 to 3: -lin, -exp, +exp, +lin. Each is a
// direction, a cut or a boost, and a shape, linear or exponential, which
// the editor sets apart: the drag sets the direction, a button the shape.
///////////////////////////////////////////////////////////////////////////////
struct scaling_curve
{
   static bool       boosts(int c)  { return c >= 2; }
   static bool       is_exp(int c)  { return c == 1 || c == 2; }
   static int        of(bool boost, bool exp)
                     {
                        return boost? (exp? 2 : 3) : (exp? 1 : 0);
                     }
};

///////////////////////////////////////////////////////////////////////////////
// An operator's keyboard level scaling as a curve_editor over a strip of
// keys: a break point on the middle line, dragged along the keys, and at
// either end its curve's depth, dragged up for a boost or down for a cut.
// A curve's shape, linear or exponential, is set elsewhere; the editor's
// lines bend a side when it is exponential.
///////////////////////////////////////////////////////////////////////////////
namespace scaling
{
   using elements::point;
   using elements::curve_point;
   using points_type = elements::basic_curve_editor::points_type;

   constexpr std::size_t left_point = 0;
   constexpr std::size_t break_point = 1;
   constexpr std::size_t right_point = 2;

   // The keys the break point spans, A-1 to C8, are a picture,
   // resources/keys.png, under the editor, inset as the editor is
   constexpr float keys_height = 16.0f;
   constexpr float inset = 8.0f;

   std::vector<curve_point>
                     points();
   point             constrain(
                        points_type const& pts, std::size_t i, point to);

   // A depth parameter as its end's height, for the binder: the middle
   // and half of its travel, upward for a boost, downward for a cut, as
   // the curve parameter says.
   struct depth_at
   {
      double         position(double v) const;
      double         value(double pos) const;

      q_plug::parameter const*   param;
      dexter_controller const*   ctl;
      int                        curve_index;
   };

   // The editor. `exp(side)` says whether a side's curve is exponential,
   // 0 the left and 1 the right, read as the lines are drawn.
   using exp_function = std::function<bool(int side)>;
   std::shared_ptr<elements::basic_curve_editor> make(exp_function exp);
}

#endif
