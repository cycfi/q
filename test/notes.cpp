/*=============================================================================
   Copyright (c) 2014-2023 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/support/note.hpp>
#include <q/support/note_names.hpp>

#include <cmath>
#include <chrono>

namespace q = cycfi::q;

TEST_CASE("Test_note_to_frequency_conversion")
{
   for (auto i = 0; i != 127; ++i)
   {
      auto n = q::note{i};
      auto expected = 440.0 * std::pow(2, (i-69)/12.0);
      auto result = as_double(as_frequency(n));
      INFO(
         "value: " << n.rep <<
         ", result:" << result <<
         ", expected:" << expected
      );
      CHECK(result == Approx(expected).epsilon(0.0001));
   }
}

TEST_CASE("Test_frequency_to_note_conversion")
{
   for (auto oct = 0; oct != 7; ++oct)
   {
      for (auto semi = 0; semi != 12; ++semi)
      {
         auto freq = q::note_frequencies[oct][semi];
         auto n = q::note{freq};
         auto result = as_double(as_frequency(n));
         CHECK(result == Approx(as_double(freq)).epsilon(0.0002));
      }
   }
}

TEST_CASE("Test_log2_speed")
{
   // This is here to prevent dead-code elimination
   float accu = 0;

   {
      auto start = std::chrono::high_resolution_clock::now();

      for (int j = 0; j < 1024; ++j)
      {
         for (int i = 1; i < 1024; ++i)
         {
            auto a = float(i);
            auto result = std::log2(a);
            accu += result;
         }
      }

      auto elapsed = std::chrono::high_resolution_clock::now() - start;
      auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed);

      std::cout << "std::log2(a) (ns): " << float(duration.count()) / (1024*1023) << std::endl;
      CHECK(duration.count() > 0);
   }

   {
      auto start = std::chrono::high_resolution_clock::now();
      using cycfi::q::fast_log2;

      for (int j = 0; j < 1024; ++j)
      {
         for (int i = 1; i < 1024; ++i)
         {
            auto a = float(i);
            auto result = fast_log2(a);
            accu += result;
         }
      }

      auto elapsed = std::chrono::high_resolution_clock::now() - start;
      auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed);

      std::cout << "fast_log2(a) elapsed (ns): " << float(duration.count()) / (1024*1023) << std::endl;
      CHECK(duration.count() > 0);
   }

   {
      auto start = std::chrono::high_resolution_clock::now();
      using cycfi::q::faster_log2;

      for (int j = 0; j < 1024; ++j)
      {
         for (int i = 1; i < 1024; ++i)
         {
            auto a = float(i);
            auto result = faster_log2(a);
            accu += result;
         }
      }

      auto elapsed = std::chrono::high_resolution_clock::now() - start;
      auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed);

      std::cout << "faster_log2(a) elapsed (ns): " << float(duration.count()) / (1024*1023) << std::endl;
      CHECK(duration.count() > 0);
   }

   // Prevent dead-code elimination
   CHECK(accu > 0);
}