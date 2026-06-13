/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/detail/sin_table.hpp>
#include <q/support/literals.hpp>

#include <cmath>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <limits>

namespace q = cycfi::q;
using namespace q::literals;
using cycfi::pi;

std::string full_precision(double d)
{
   auto s = std::ostringstream{};
   s << std::setprecision( std::numeric_limits<double>::max_digits10 ) << d;
   return s.str();
}

TEST_CASE("Test_sin_table")
{
   for (int i = 0; i < 100000; ++i)
   {
      {
         auto a = 2_pi * float(i) / 100000;
         INFO("i: " << full_precision(i));
         INFO("a: " << full_precision(a));
         auto result = q::sin_lu(a);

         INFO("result: " << full_precision(result));
         INFO("std::sin(a): " << full_precision(std::sin(a)));

         REQUIRE_THAT(result,
            Catch::Matchers::WithinAbs(std::sin(a), 0.00001)
         );
      }
   }
}

TEST_CASE("Test_accuracy")
{
   {
      float max_diff = 0;
      float total_diff = 0;
      for (int i = 0; i < 100000; ++i)
      {
         {
            float a = 2_pi * float(i) / 100000;
            float r1 = q::sin_lu(a);
            float r2 = std::sin(a);
            float diff = std::abs(r1-r2);
            max_diff = std::max(max_diff, diff);
            total_diff += diff;
         }
      }
      auto ave_diff = total_diff/100000;
      std::cout << "q::sin_lu(a) max diff: " << max_diff << std::endl;
      std::cout << "q::sin_lu(a) ave diff: " << ave_diff << std::endl;

      CHECK(max_diff < 5e-06);
      CHECK(ave_diff < 2e-06);
   }

   {
      float max_diff = 0;
      float total_diff = 0;
      for (int i = 0; i < 100000; ++i)
      {
         {
            float a = 2_pi * float(i) / 100000;
            float r1 = q::fast_sin(a-pi);
            float r2 = std::sin(a-pi);
            float diff = std::abs(r1-r2);
            max_diff = std::max(max_diff, diff);
            total_diff += diff;
         }
      }
      auto ave_diff = total_diff/100000;
      std::cout << "q::fast_sin(a) max diff: " << max_diff << std::endl;
      std::cout << "q::fast_sin(a) ave diff: " << ave_diff << std::endl;

      CHECK(max_diff < 4e-05);
      CHECK(ave_diff < 1.3e-05);
   }

   {
      float max_diff = 0;
      float total_diff = 0;
      for (int i = 0; i < 100000; ++i)
      {
         {
            float a = 2_pi * float(i) / 100000;
            float r1 = q::faster_sin(a-pi);
            float r2 = std::sin(a-pi);
            float diff = std::abs(r1-r2);
            max_diff = std::max(max_diff, diff);
            total_diff += diff;
         }
      }
      auto ave_diff = total_diff/100000;
      std::cout << "q::faster_sin(a) max diff: " << max_diff << std::endl;
      std::cout << "q::faster_sin(a) ave diff: " << ave_diff << std::endl;

      CHECK(max_diff < 9e-04);
      CHECK(ave_diff < 5e-04);
   }
}

TEST_CASE("Test_sin_table_quadrant_boundaries")
{
   // Exact quadrant corners and their immediate neighbors: a quarter-wave
   // folded implementation must stay accurate exactly where it folds
   // (mirror) and negates.
   constexpr std::uint32_t reps[] = {
      0x00000000u, 0x00000001u,
      0x3FFFFFFFu, 0x40000000u, 0x40000001u,
      0x7FFFFFFFu, 0x80000000u, 0x80000001u,
      0xBFFFFFFFu, 0xC0000000u, 0xC0000001u,
      0xFFFFFFFFu
   };
   for (auto rep : reps)
   {
      auto ph = q::phase{rep};
      auto expected = std::sin(q::frac_double(ph) * 2 * pi);
      INFO("rep: " << rep);
      REQUIRE_THAT(q::sin_lu(ph),
         Catch::Matchers::WithinAbs(expected, 0.00001));
   }
}
