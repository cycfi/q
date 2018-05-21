/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(CYCFI_Q_SYNTH_HPP_DECEMBER_24_2015)
#define CYCFI_Q_SYNTH_HPP_DECEMBER_24_2015

#include <q/literals.hpp>
#include <q/phase.hpp>
#include <q/fx.hpp>
#include <q/detail/sin_table.hpp>
#include <q/detail/antialiasing.hpp>

namespace cycfi { namespace q
{
   ////////////////////////////////////////////////////////////////////////////
   // sin_synth: Synthesizes sine waves.
   ////////////////////////////////////////////////////////////////////////////
   struct sin_synth
   {
      constexpr float operator()(phase p) const
      {
         return detail::sin_gen(p);
      }
   };

   constexpr auto sin = sin_synth{};

   ////////////////////////////////////////////////////////////////////////////
   // square-wave synthesizer (not bandwidth limited)
   ////////////////////////////////////////////////////////////////////////////
   struct square_synth
   {
      constexpr float operator()(phase p) const
      {
         constexpr auto middle = phase::max() / 2;
         return p < middle ? 1.0f : -1.0f;
      }
   };

   constexpr auto square = square_synth{};

   ////////////////////////////////////////////////////////////////////////////
   // square-wave synthesizer (bandwidth limited using poly_blep)
   ////////////////////////////////////////////////////////////////////////////
   struct bl_square_synth
   {
      constexpr float operator()(phase p, phase dt) const
      {
         constexpr auto middle = phase::max() / 2;
         auto r = p < middle ? 1.0f : -1.0f;

         // Correct rising discontinuity
         r += detail::poly_blep(p, dt);

         // Correct falling discontinuity
         r -= detail::poly_blep(p + middle, dt);

         return r;
      }
   };

   constexpr auto bl_square = bl_square_synth{};

   ////////////////////////////////////////////////////////////////////////////
   // sawtooth-wave synthesizer (not bandwidth limited)
   ////////////////////////////////////////////////////////////////////////////
   struct saw_synth
   {
      constexpr float operator()(phase p) const
      {
         constexpr float x = 2.0f / phase::one_cyc;
         return (p.val * x) - 1.0;
      }
   };

   constexpr auto saw = saw_synth{};

   ////////////////////////////////////////////////////////////////////////////
   // sawtooth-wave synthesizer (bandwidth limited)
   ////////////////////////////////////////////////////////////////////////////
   struct bl_saw_synth
   {
      constexpr float operator()(phase p, phase dt) const
      {
         constexpr float x = 2.0f / phase::one_cyc;
         auto r = (p.val * x) - 1.0;

         // Correct discontinuity
         r -= detail::poly_blep(p, dt);
         return r;
      }
   };

   constexpr auto bl_saw = bl_saw_synth{};

   ////////////////////////////////////////////////////////////////////////////
   // pwm synthesizer (not bandwidth limited). The pwm synth is a sum of two
   // phase shifted saw synthesizers. The phase shift determines the pwm
   // width.
   ////////////////////////////////////////////////////////////////////////////
   struct pwm_synth
   {
      constexpr pwm_synth(float width = 0.5)
       : _shift(phase(width))
       , _offset(saw(phase::min()) - saw(_shift) + 1.0f)
      {}

      constexpr void width(float width)
      {
         _shift = phase(width);
         _offset = saw(phase::min()) - saw(_shift) + 1.0f;
      }

      constexpr float operator()(phase p) const
      {
         return saw(p) + saw(p + _shift);
      }

      phase _shift;
      float _offset;
   };

   constexpr auto pwm = pwm_synth{};

   ////////////////////////////////////////////////////////////////////////////
   // pwm synthesizer (bandwidth limited). The pwm synth is a sum of two
   // phase shifted bl_saw synthesizers. The phase shift determines the pwm
   // width.
   ////////////////////////////////////////////////////////////////////////////
   struct bl_pwm_synth : pwm_synth
   {
      constexpr bl_pwm_synth(float width = 0.5)
       : pwm_synth(width)
      {}

      constexpr float operator()(phase p, phase dt) const
      {
         return (bl_saw(p, dt) - bl_saw(p + _shift, dt)) - _offset;
      }
   };

   constexpr auto bl_pwm = bl_pwm_synth{};

   ////////////////////////////////////////////////////////////////////////////
   // triangle-wave synthesizer (not bandwidth limited)
   ////////////////////////////////////////////////////////////////////////////
   struct triangle_synth
   {
      constexpr float operator()(phase p) const
      {
         constexpr float x = 4.0f / phase::one_cyc;
         return (abs(std::int32_t(p.val)) * x) - 1.0;
      }
   };

   constexpr auto triangle = triangle_synth{};

   ////////////////////////////////////////////////////////////////////////////
   // triangle-wave synthesizer (bandwidth limited)
   ////////////////////////////////////////////////////////////////////////////
   struct bl_triangle_synth
   {
      constexpr float operator()(phase p, phase dt) const
      {
         constexpr auto end = phase::max();
         constexpr auto edge1 = end/4;
         constexpr auto edge2 = end-edge1;
         constexpr float x = 4.0f / phase::one_cyc;

         auto r = (abs(std::int32_t((p + edge1).val)) * x) - 1.0;

         // Correct falling discontinuity
         r += detail::poly_blamp(p + edge1, dt, 4);

         // Correct rising discontinuity
         r -= detail::poly_blamp(p + edge2, dt, 4);

         return r;
      }
   };

   constexpr auto bl_triangle = bl_triangle_synth{};
}}

#endif
