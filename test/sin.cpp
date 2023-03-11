/*=============================================================================
   Copyright (c) 2014-2023 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/detail/sin_table.hpp>
#include <q/support/literals.hpp>

#include <cmath>
#include <iostream>
#include <fstream>
#include <chrono>

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

TEST_CASE("Test_sin_speed")
{
   // This is here to prevent dead-code elimination
   float accu = 0;
   {
      auto start = std::chrono::high_resolution_clock::now();

      for (int j = 0; j < 1024; ++j)
      {
         for (std::uint32_t i = 0; i < 1024; ++i)
         {
            auto result = q::sin_lu(q::phase(i*4194304));
            accu += result;
         }
      }

      auto elapsed = std::chrono::high_resolution_clock::now() - start;
      auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed);

      std::cout << "q::sin_lu(a) elapsed (ns): " << float(duration.count()) / (1024*1024) << std::endl;
      CHECK(duration.count() > 0);
   }

   {
      auto start = std::chrono::high_resolution_clock::now();

      for (int j = 0; j < 1024; ++j)
      {
         for (std::uint32_t i = 0; i < 1024; ++i)
         {
            auto a = 2_pi * float(i) / 1024;
            auto result = std::sin(a);
            accu += result;
         }
      }

      auto elapsed = std::chrono::high_resolution_clock::now() - start;
      auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed);

      std::cout << "std::sin(a) elapsed (ns): " << float(duration.count()) / (1024*1024) << std::endl;
      CHECK(duration.count() > 0);
   }

   {
      auto start = std::chrono::high_resolution_clock::now();

      for (int j = 0; j < 1024; ++j)
      {
         for (std::uint32_t i = 0; i < 1024; ++i)
         {
            auto a = 2_pi * float(i) / 1024;
            auto result = q::fast_sin(a);
            accu += result;
         }
      }

      auto elapsed = std::chrono::high_resolution_clock::now() - start;
      auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed);

      std::cout << "q::fast_sin elapsed (ns): " << float(duration.count()) / (1024*1024) << std::endl;
      CHECK(duration.count() > 0);
   }

   {
      auto start = std::chrono::high_resolution_clock::now();

      for (int j = 0; j < 1024; ++j)
      {
         for (std::uint32_t i = 0; i < 1024; ++i)
         {
            auto a = 2_pi * float(i) / 1024;
            auto result = q::faster_sin(a);
            accu += result;
         }
      }

      auto elapsed = std::chrono::high_resolution_clock::now() - start;
      auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed);

      std::cout << "q::faster_sin elapsed (ns): " << float(duration.count()) / (1024*1024) << std::endl;
      CHECK(duration.count() > 0);
   }

   // Prevent dead-code elimination
   CHECK(accu != 0);
}

