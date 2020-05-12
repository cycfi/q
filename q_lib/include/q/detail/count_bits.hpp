/*=============================================================================
   Copyright (c) 2014-2020 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_COUNT_BITS_HPP_MARCH_12_2018)
#define CYCFI_Q_COUNT_BITS_HPP_MARCH_12_2018

#ifdef _MSC_VER
# include <intrin.h>
# include <nmmintrin.h>
#endif

namespace cycfi::q::detail
{
   inline std::uint32_t count_bits(std::uint32_t i)
   {
#if defined(_MSC_VER)
      return __popcnt(i);
#elif defined(__GNUC__)
      return __builtin_popcount(i);
#else
# error Unsupported compiler
#endif
   }

   inline std::uint64_t count_bits(std::uint64_t i)
   {
#if defined(_MSC_VER)
      return _mm_popcnt_u64(i);
#elif defined(__GNUC__)
      return __builtin_popcountll(i);
#else
# error Unsupported compiler
#endif
   }
}

#endif

