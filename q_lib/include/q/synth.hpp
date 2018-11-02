/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
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
   // basic square-wave synthesizer (not bandwidth limited)
   ////////////////////////////////////////////////////////////////////////////
   struct basic_square_synth
   {
      constexpr float operator()(phase p) const
      {
         constexpr auto middle = phase::max() / 2;
         return p < middle ? 1.0f : -1.0f;
      }
   };

   constexpr auto basic_square = basic_square_synth{};

   ////////////////////////////////////////////////////////////////////////////
   // square-wave synthesizer (bandwidth limited using poly_blep)
   ////////////////////////////////////////////////////////////////////////////
   struct square_synth
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

   constexpr auto square = square_synth{};

   ////////////////////////////////////////////////////////////////////////////
   // basic sawtooth-wave synthesizer (not bandwidth limited)
   ////////////////////////////////////////////////////////////////////////////
   struct basic_saw_synth
   {
      constexpr float operator()(phase p) const
      {
         constexpr float x = 2.0f / phase::one_cyc;
         return (p.val * x) - 1.0;
      }
   };

   constexpr auto basic_saw = basic_saw_synth{};

   ////////////////////////////////////////////////////////////////////////////
   // sawtooth-wave synthesizer (bandwidth limited)
   ////////////////////////////////////////////////////////////////////////////
   struct saw_synth
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

   constexpr auto saw = saw_synth{};

   ////////////////////////////////////////////////////////////////////////////
   // basic pulse synthesizer (not bandwidth limited).
   ////////////////////////////////////////////////////////////////////////////
   struct basic_pulse_synth
   {
      constexpr basic_pulse_synth(float width = 0.5)
       : _shift(phase(width))
      {}

      constexpr void width(float width)
      {
         _shift = phase(width);
      }

      constexpr float operator()(phase p) const
      {
         return p < _shift ? 1.0f : -1.0f;
      }

      phase _shift;
   };

   constexpr auto basic_pulse = basic_pulse_synth{};

   ////////////////////////////////////////////////////////////////////////////
   // pulse synthesizer (bandwidth limited).
   ////////////////////////////////////////////////////////////////////////////
   struct pulse_synth : basic_pulse_synth
   {
      constexpr pulse_synth(float width = 0.5)
       : basic_pulse_synth(width)
      {}

      constexpr float operator()(phase p, phase dt) const
      {
         constexpr auto end = phase::max();
         auto r = p < _shift ? 1.0f : -1.0f;

         // Correct rising discontinuity
         r += detail::poly_blep(p, dt);

         // Correct falling discontinuity
         r -= detail::poly_blep(p + (end - _shift), dt);

         return r;
      }
   };

   constexpr auto pulse = pulse_synth{};

   ////////////////////////////////////////////////////////////////////////////
   // basic triangle-wave synthesizer (not bandwidth limited)
   ////////////////////////////////////////////////////////////////////////////
   struct basic_triangle_synth
   {
      constexpr float operator()(phase p) const
      {
         constexpr float x = 4.0f / phase::one_cyc;
         return (abs(std::int32_t(p.val)) * x) - 1.0;
      }
   };

   constexpr auto basic_triangle = basic_triangle_synth{};

   ////////////////////////////////////////////////////////////////////////////
   // triangle-wave synthesizer (bandwidth limited)
   ////////////////////////////////////////////////////////////////////////////
   struct triangle_synth
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

   constexpr auto triangle = triangle_synth{};
}}

#endif
