/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_AUTO_CORRELATOR_HPP_MARCH_12_2018)
#define CYCFI_Q_AUTO_CORRELATOR_HPP_MARCH_12_2018

#include <q/utility/bitset.hpp>
#include <q/detail/count_bits.hpp>
#include <q/support/base.hpp>

namespace cycfi::q
{
   namespace detail
   {
#if defined(__ARM_NEON) || defined(__POPCNT__) || defined(_MSC_VER)
      // A hardware count: one a word.
      template <typename F>
      inline std::size_t count_xor(std::size_t words, F word)
      {
         std::size_t count = 0;
         for (std::size_t i = 0; i != words; ++i)
            count += count_bits(word());
         return count;
      }
#else
      // No hardware count (a Cortex-M, baseline x86-64): Harley-Seal.
      // Carry-save adders sum eight words bitwise into ones, twos, fours
      // and an eights word, and only the eights are counted, once every
      // eight words. On the M7, 14.5 cycles a word against 22.
      template <typename T>
      inline void csa(T& h, T& l, T a, T b, T c)
      {
         auto const u = a ^ b;
         h = (a & b) | (u & c);
         l = u ^ c;
      }

      template <typename F>
      inline std::size_t count_xor(std::size_t words, F word)
      {
         using T = decltype(word());
         T ones = 0, twos = 0, fours = 0;
         std::size_t eights = 0, i = 0;
         for (; i + 8 <= words; i += 8)
         {
            T twos_a, twos_b, fours_a, fours_b, e;
            T const a = word(), b = word(), c = word(), d = word();
            csa(twos_a, ones, ones, a, b);
            csa(twos_b, ones, ones, c, d);
            csa(fours_a, twos, twos, twos_a, twos_b);
            T const f = word(), g = word(), h = word(), k = word();
            csa(twos_a, ones, ones, f, g);
            csa(twos_b, ones, ones, h, k);
            csa(fours_b, twos, twos, twos_a, twos_b);
            csa(e, fours, fours, fours_a, fours_b);
            eights += count_bits(e);
         }
         std::size_t count = 8 * eights + 4 * count_bits(fours)
            + 2 * count_bits(twos) + count_bits(ones);
         for (; i != words; ++i)
            count += count_bits(word());
         return count;
      }
#endif
   }

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
   //
   // The window is half the bits, from the start. The three-argument call
   // takes any window: `words` words from word `start`, against the same
   // bits `pos` later, for a stream that keeps its bits in the bitset as
   // they slide.
   ////////////////////////////////////////////////////////////////////////////
   template <typename T = natural_uint>
   struct bitstream_acf
   {
      static constexpr auto value_size = bitset<T>::value_size;

      // Half the words, less one, but at least one. The max comes first so
      // the subtraction cannot wrap for a one-word bitset.
      bitstream_acf(bitset<T> const& bits)
         : _bits(bits)
         , _mid_array(
               std::max<std::size_t>((bits.size() / value_size) / 2, 2) - 1)
      {}

      std::size_t operator()(std::size_t pos) const
      {
         return (*this)(pos, 0, _mid_array);
      }

      std::size_t operator()(
         std::size_t pos, std::size_t start, std::size_t words) const
      {
         auto const shift = pos % value_size;
         auto const* p1 = _bits.data() + start;
         auto const* p2 = _bits.data() + start + pos / value_size;

         // Each word of the shifted bits is loaded once, carried to the
         // next word's high half.
         if (shift == 0)
            return detail::count_xor(words, [&]{ return *p1++ ^ *p2++; });
         auto const shift2 = value_size - shift;
         auto cur = *p2++;
         return detail::count_xor(words, [&]
         {
            auto const next = *p2++;
            auto const v = (cur >> shift) | (next << shift2);
            cur = next;
            return *p1++ ^ v;
         });
      }

      bitset<T> const&     _bits;
      std::size_t const    _mid_array;
   };
}

#endif

