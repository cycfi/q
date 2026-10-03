/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/fx/moving_sum.hpp>
#include <cmath>
#include <cstdint>
#include <type_traits>

namespace q = cycfi::q;

TEST_CASE("Test_moving_sum_basic")
{
   auto ms = q::basic_moving_sum<int>{10};

   auto r = ms(1);
   CHECK(r == 1);

   r = ms(1);
   CHECK(r == 2);

   r = ms(1);
   CHECK(r == 3);

   r = ms(1);
   CHECK(r == 4);

   r = ms(1);
   CHECK(r == 5);

   r = ms(1);
   CHECK(r == 6);

   r = ms(1);
   CHECK(r == 7);

   r = ms(1);
   CHECK(r == 8);

   r = ms(1);
   CHECK(r == 9);

   r = ms(1);
   CHECK(r == 10);

   r = ms(1); // overflow; the oldest item is subtracted from the sum
   CHECK(r == 10);
}

TEST_CASE("Test_moving_sum_resize")
{
   auto ms = q::basic_moving_sum<int>{10};

   ms(3);
   ms(2);
   ms(1);
   ms(1);
   ms(1);
   ms(1);
   ms(1);
   ms(1);
   ms(4);
   ms(5);
   CHECK(ms() == 20);

   ms.resize(8, true);
   CHECK(ms() == 15);

   ms.resize(10, true);
   CHECK(ms() == 20);

   ms.resize(1, true);
   CHECK(ms() == 5);

   ms.resize(10, true);
   CHECK(ms() == 20);
}

TEST_CASE("Test_moving_sum_float_accumulator")
{
   // The default keeps float sums in double.
   static_assert(std::is_same_v<q::moving_sum::accumulator_type, double>);

   // A float accumulator re-sums every window, so its error stays bounded
   // against the double default. 2M samples in [0.5, 1.5), window 1000
   // (sum about 1000): measured max error 0.0029; the bound is 0.01.
   constexpr std::size_t window = 1000;
   auto fsum = q::basic_moving_sum<float, float>{window};
   auto dsum = q::basic_moving_sum<float>{window};

   std::uint32_t seed = 12345;
   double max_error = 0;
   for (std::size_t i = 0; i != 2000000; ++i)
   {
      seed = seed * 1664525u + 1013904223u;
      float s = 0.5f + float(seed >> 8) / float(1u << 24);
      double e = std::abs(double(fsum(s)) - dsum(s));
      max_error = std::max(max_error, e);
   }
   CHECK(max_error < 0.01);

   // clear() restarts the re-sum cleanly.
   fsum.clear();
   CHECK(fsum(1.0f) == 1.0f);
}
