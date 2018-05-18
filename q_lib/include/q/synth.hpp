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
   // square synthesizer (this is not bandwidth limited)
   ////////////////////////////////////////////////////////////////////////////
   class square_synth
   {
   public:

      constexpr float operator()(phase p) const
      {
         constexpr auto x = phase::one_cyc / 2;
         return p.val > x ? 1.0f : -1.0f;
      }
   };

   auto constexpr square = square_synth{};

   ////////////////////////////////////////////////////////////////////////////
   // sawtooth synthesizer (this is not bandwidth limited)
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
   // triangle_synth synthesizer (this is not bandwidth limited)
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

}}

#endif
