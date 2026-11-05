/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/utility/bitstream_acf.hpp>
#include <cstdint>

namespace q = cycfi::q;

namespace
{
   // Brute force mismatch count over the bits bitstream_acf compares.
   template <typename T>
   std::size_t reference_acf(
      q::bitset<T> const& bs, std::size_t pos, std::size_t n)
   {
      std::size_t count = 0;
      for (std::size_t i = 0; i != n; ++i)
         count += bs.get(i) != bs.get(i + pos);
      return count;
   }

   template <typename T>
   void check_against_reference()
   {
      // 16 words of a square bitstream with a period of 20 bits.
      constexpr auto word = q::bitset<T>::value_size;
      q::bitset<T> bs{16 * word};
      for (std::size_t i = 0; i != bs.size(); ++i)
         bs.set(i, (i % 20) < 10);

      q::bitstream_acf<T> acf{bs};
      CHECK(acf.value_size == word);

      auto const n = acf._mid_array * word;
      for (std::size_t pos = 0; pos != bs.size() / 2; ++pos)
      {
         INFO("pos = " << pos);
         CHECK(acf(pos) == reference_acf(bs, pos, n));
      }
      CHECK(acf(20) == 0);
      CHECK(acf(40) == 0);
   }
}

TEST_CASE("bitstream_acf: 32-bit words match the reference")
{
   check_against_reference<std::uint32_t>();
}

TEST_CASE("bitstream_acf: 64-bit words match the reference")
{
   check_against_reference<std::uint64_t>();
}

namespace
{
   // Any window: `words` words from word `start`, against `pos` later.
   template <typename T>
   void check_any_window()
   {
      constexpr auto word = q::bitset<T>::value_size;
      q::bitset<T> bs{32 * word};
      std::uint32_t x = 12345;
      for (std::size_t i = 0; i != bs.size(); ++i)
      {
         x = x * 1664525u + 1013904223u;
         bs.set(i, (x >> 31) != 0);
      }

      q::bitstream_acf<T> acf{bs};
      for (std::size_t start : {0, 1, 5})
         for (std::size_t words : {1, 4, 9})
            for (std::size_t pos : {0, 1, 7, 31, 32, 33, 100})
            {
               INFO("start = " << start << ", words = " << words
                  << ", pos = " << pos);
               std::size_t ref = 0;
               auto const from = start * word;
               for (std::size_t i = 0; i != words * word; ++i)
                  ref += bs.get(from + i) != bs.get(from + i + pos);
               CHECK(acf(pos, start, words) == ref);
            }

      // The one-argument call is the window from the start.
      for (std::size_t pos = 0; pos != 64; ++pos)
         CHECK(acf(pos) == acf(pos, 0, acf._mid_array));
   }
}

TEST_CASE("bitstream_acf: any window matches the reference")
{
   check_any_window<std::uint32_t>();
   check_any_window<std::uint64_t>();
}

TEST_CASE("bitstream_acf: compares half the words less one, at least one")
{
   using bits = q::bitset<std::uint32_t>;

   bits one{32};
   bits two{64};
   bits four{128};
   bits eight{256};

   CHECK(q::bitstream_acf<std::uint32_t>{one}._mid_array == 1);
   CHECK(q::bitstream_acf<std::uint32_t>{two}._mid_array == 1);
   CHECK(q::bitstream_acf<std::uint32_t>{four}._mid_array == 1);
   CHECK(q::bitstream_acf<std::uint32_t>{eight}._mid_array == 3);

   // A one-word bitset correlates with itself at lag 0.
   one.set(3, true);
   CHECK(q::bitstream_acf<std::uint32_t>{one}(0) == 0);
}
