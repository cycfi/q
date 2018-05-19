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
   class square_synth
   {
   public:

      constexpr float operator()(phase p) const
      {
         constexpr auto middle = phase::end() / 2;
         return p < middle ? 1.0f : -1.0f;
      }
   };

   constexpr auto square = square_synth{};

   ////////////////////////////////////////////////////////////////////////////
   // square-wave synthesizer (bandwidth limited using poly_blep)
   ////////////////////////////////////////////////////////////////////////////
   class bl_square_synth
   {
   public:

      constexpr float operator()(phase p, phase dt) const
      {
         constexpr auto middle = phase::end() / 2;
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
   class saw_synth
   {
   public:

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
   class bl_saw_synth
   {
   public:

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
   // triangle-wave synthesizer (not bandwidth limited)
   ////////////////////////////////////////////////////////////////////////////
   class triangle_synth
   {
   public:

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
   class bl_triangle_synth
   {
   public:

      constexpr float operator()(phase p, phase dt) const
      {
         constexpr auto end = phase::end();
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
