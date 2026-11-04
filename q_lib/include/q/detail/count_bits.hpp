/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_COUNT_BITS_HPP_MARCH_12_2018)
#define CYCFI_Q_COUNT_BITS_HPP_MARCH_12_2018

#include <bit>
#include <cstdint>

namespace cycfi::q::detail
{
   inline std::uint32_t count_bits(std::uint32_t i)
   {
      return std::popcount(i);
   }

   inline std::uint64_t count_bits(std::uint64_t i)
   {
      return std::popcount(i);
   }
}

#endif
