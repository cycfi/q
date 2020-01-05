/*=============================================================================
   Copyright (c) 2014-2019 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_AUTO_CORRELATOR_HPP_MARCH_12_2018)
#define CYCFI_Q_AUTO_CORRELATOR_HPP_MARCH_12_2018

#include <q/utility/bitset.hpp>
#include <q/detail/count_bits.hpp>
#include <q/support/base.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // The bitstream_acf correlates class a bit stream (stored in a bitset) by
   // itself shifted by position, pos.
   //
   // In standard ACF (autocorrelation function) the signal is multiplied by
   // a shifted version of itself, delayed by position, pos, with the result
   // accumulated for each point, half the window of interest. The higher the
   // sum, the higher the periodicity.
   //
   // With bitstream auto correlation, the more efficient XOR operation is
   // used instead. A single XOR operation works on N bits of an integer. We
   // get a speedup factor of N (e.g. 64 for a 64 bit machine) compared to
   // standard ACF, and not to mention that integer bit operations are a lot
   // faster than floating point multiplications).
   //
   // With XOR you get a one when there’s a mismatch:
   //
   //    0 ^ 0 = 0
   //    0 ^ 1 = 1
   //    1 ^ 0 = 1
   //    1 ^ 1 = 0
   //
   // After XOR, the number of bits (set to 1) is counted. The lower the
   // count, the higher the periodicity. A count of zero gives perfect
   // correlation: there is no mismatch.
   ////////////////////////////////////////////////////////////////////////////
   template <typename T = natural_uint>
   struct bitstream_acf
   {
      static constexpr auto value_size = bitset<>::value_size;

      bitstream_acf(bitset<T> const& bits)
         : _bits(bits)
         , _mid_array(((bits.size() / value_size) / 2) - 1)
      {}

      std::size_t operator()(std::size_t pos)
      {
         auto const index = pos / value_size;
         auto const shift = pos % value_size;

         auto const* p1 = _bits.data();
         auto const* p2 = _bits.data() + index;
         auto count = 0;

         if (shift == 0)
         {
            for (auto i = 0; i != _mid_array; ++i)
               count += detail::count_bits(*p1++ ^ *p2++);
         }
         else
         {
            auto shift2 = value_size - shift;
            for (auto i = 0; i != _mid_array; ++i)
            {
               auto v = *p2++ >> shift;
               v |= *p2 << shift2;
               count += detail::count_bits(*p1++ ^ v);
            }
         }
         return count;
      };

      bitset<T> const&     _bits;
      std::size_t const    _mid_array;
   };
}

#endif

