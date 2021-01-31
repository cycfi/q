/*=============================================================================
   Copyright (c) 2014-2021 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_TABLE_LOOKUP_HPP_JANUARY_27_2015)
#define CYCFI_Q_TABLE_LOOKUP_HPP_JANUARY_27_2015

#include <cstdint>
#include <q/support/base.hpp>

namespace cycfi::q::detail
{
   template <std::size_t N>
   constexpr float table_lookup(phase ph, float const (&table)[N])
   {
      // q::phase generates from 0 to maximum value (e.g. 0xFFFFFFFF) for the
      // phase::value_type, corresponding to (0 to 2π). We use the highest 10
      // bits for our sin lookup table and the rest of the lowest bits (e.g. 22
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
}

#endif
