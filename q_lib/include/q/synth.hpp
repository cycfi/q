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

   auto constexpr sin = sin_synth{};

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

   auto constexpr square = square_synth{};

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

   auto constexpr bl_square = bl_square_synth{};

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

   auto constexpr saw = saw_synth{};

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

   auto constexpr bl_saw = bl_saw_synth{};

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

   auto constexpr triangle = triangle_synth{};

   ////////////////////////////////////////////////////////////////////////////
   // triangle-wave synthesizer (bandwidth limited)
   ////////////////////////////////////////////////////////////////////////////
   class bl_triangle_synth
   {
   public:

      constexpr float operator()(phase p, phase dt) const
      {
         constexpr auto one_fourth = phase::end() / 4;
         constexpr auto one_half = phase::end() / 2;
         constexpr auto three_fourths = one_half + one_fourth;
         constexpr float x = 4.0f / phase::one_cyc;

         auto r = (abs(std::int32_t((p + one_fourth).val)) * x) - 1.0;
         auto scale = 4.0f * float(dt);

         // Correct falling discontinuity
         r += scale * detail::poly_blamp(p + one_fourth, dt);

         // Correct rising discontinuity
         r -= scale * detail::poly_blamp(p + three_fourths, dt);

         return r;
      }
   };

   auto constexpr bl_triangle = bl_triangle_synth{};

}}

#endif
