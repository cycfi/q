/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_TABLE_LOOKUP_HPP_JANUARY_27_2015)
#define CYCFI_Q_TABLE_LOOKUP_HPP_JANUARY_27_2015

#include <bit>
#include <cstdint>
#include <q/support/base.hpp>
#include <q/support/phase.hpp>

namespace cycfi::q::detail
{
   template <std::size_t N>
   constexpr float table_lookup(phase ph, float const (&table)[N])
   {
      // q::phase generates from 0 to maximum value (e.g. 0xFFFFFFFF) for the
      // phase::value_type, corresponding to (0 to 2π). We use the highest 10
      // bits for our lookup table and the rest of the lowest bits (e.g. 22
      // bits) to interpolate between two values from the table.

      constexpr auto size = sizeof(phase::value_type) * 8;  // e.g. 32
      constexpr auto low_bits = size - 10;                  // e.g. 22
      constexpr auto denom = 1 << low_bits;                 // e.g. 0x400000 (4194304)
      constexpr auto mask = denom - 1;                      // e.g. 0x3FFFFF (4194303)
      constexpr auto factor = 1.0f / denom;                 // e.g. 0.000000238418579

      auto const index = ph.rep >> low_bits;
      auto v1 = table[index];
      auto v2 = table[index + 1];
      return linear_interpolate(v1, v2, (ph.rep & mask) * factor);

      // Note: for speed, we favor multiplication over division, so we
      // multiply by factor, a constexpr evaluating to 1.0f / denom, instead
      // of directly dividing by denom.
   }

   ////////////////////////////////////////////////////////////////////////////
   // Quarter-wave symmetric table lookup, for odd, quarter-wave symmetric
   // periodic functions (e.g. sine). `table` holds only the first quarter
   // cycle: N-1 segments from f(0) to f(τ/4), both inclusive, so the table
   // is 4x smaller than a full-cycle table at identical resolution.
   //
   // The full cycle is recovered by folding the phase, branchless: the top
   // two bits of q::phase are the quadrant. The second-highest bit mirrors
   // the position within the quadrant (XOR with an all-ones mask), and the
   // highest bit flips the sign of the result (XOR on the float sign bit).
   //
   // The XOR mirror reflects to the position one phase-LSB shy of the
   // exact mirror image; the resulting error (2π·2^-32 in phase) is orders
   // of magnitude below the table's interpolation error.
   ////////////////////////////////////////////////////////////////////////////
   template <std::size_t N>
   constexpr float quarter_wave_lookup(phase ph, float const (&table)[N])
   {
      static_assert(N > 1 && ((N-1) & (N-2)) == 0,
         "Error: table must hold 2^n segments (2^n + 1 entries).");

      using value_type = phase::value_type;

      constexpr auto size = sizeof(value_type) * 8;            // e.g. 32
      constexpr auto quad_bits = size - 2;                     // e.g. 30
      constexpr auto index_bits = std::bit_width(N-1) - 1;     // e.g. 8
      constexpr auto low_bits = quad_bits - index_bits;        // e.g. 22
      constexpr auto quad_mask = (value_type(1) << quad_bits) - 1;
      constexpr auto mask = (value_type(1) << low_bits) - 1;   // e.g. 0x3FFFFF
      constexpr auto factor = 1.0f / (value_type(1) << low_bits);

      auto const rep = ph.rep;
      auto const mirror = value_type(0) - ((rep >> quad_bits) & 1);
      auto const pos = (rep ^ mirror) & quad_mask;
      auto const index = pos >> low_bits;
      auto const r = linear_interpolate(
         table[index], table[index + 1], (pos & mask) * factor);

      auto const sign = std::uint32_t(rep >> (size-1)) << 31;
      return std::bit_cast<float>(std::bit_cast<std::uint32_t>(r) ^ sign);
   }
}

#endif
