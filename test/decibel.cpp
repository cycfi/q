/*=============================================================================
   Copyright (c) 2014-2023 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/detail/db_table.hpp>
#include <q/support/decibel.hpp>
#include <q/support/literals.hpp>

#include <cmath>
#include <iostream>
#include <fstream>
#include <chrono>

namespace q = cycfi::q;

TEST_CASE("Test_inverse_decibel_conversion")
{
   {
      auto db = 119.94;
      INFO("dB: " << db);
      auto result = q::detail::db2a(db);

      REQUIRE_THAT(result,
         Catch::Matchers::WithinRel(std::pow(10, db/20), 0.001)
      );
   }

   {
      auto db = std::numeric_limits<double>::infinity();
      INFO("dB: " << db);
      auto result = q::detail::db2a(db);
      CHECK(result == 1000000.0f); // this is our max limit
   }

   for (int i = 0; i < 1200; ++i)
   {
      {
         auto db = float(i/10.0);
         INFO("dB: " << db);
         auto result = q::detail::db2a(db);

         REQUIRE_THAT(result,
            Catch::Matchers::WithinRel(std::pow(10, db/20), 0.0001)
         );
      }

      for (int j = 0; j < 10; ++j)
      {
         auto db = float(i) + (j / 10.0f);
         INFO("dB: " << db);
         auto result = q::detail::db2a(db/10.0);

         REQUIRE_THAT(result,
            Catch::Matchers::WithinRel(std::pow(10, (db/10.0)/20), 0.0001)
         );
      }
   }
}

TEST_CASE("Test_negative_decibel")
{
   {
      auto db = -6;
      INFO("dB: " << db);
      auto result = q::detail::db2a(db);

      REQUIRE_THAT(result,
         Catch::Matchers::WithinRel(0.5, 0.01)
      );
   }

   {
      auto db = -24;
      INFO("dB: " << db);
      auto result = q::detail::db2a(db);

      REQUIRE_THAT(result,
         Catch::Matchers::WithinRel(0.063096, 0.0001)
      );
   }

   {
      auto db = -36;
      INFO("dB: " << db);
      auto result = q::detail::db2a(db);

      REQUIRE_THAT(result,
         Catch::Matchers::WithinRel(0.015849, 0.0001)
      );
   }
}

TEST_CASE("Test_decibel_operations")
{
   using namespace q::literals;
   {
      q::decibel db = 48_dB;
      {
         auto a = as_float(db);
         CHECK(a == Approx(251.19).epsilon(0.01));
      }
      {
         // A square root is just divide by two in the log domain
         auto a = as_float(db / 2.0f);

         REQUIRE_THAT(a,
            Catch::Matchers::WithinRel(15.85, 0.01)
         );
      }
   }
}

// Root Mean Square Error
double rmse(std::vector<float> const& ref, std::vector<float> const& test)
{
   double sum_squared_error = 0.0;
   int n = ref.size(); // assuming the signals have the same length
   for (int i = 0; i < n; i++) {
      double error = ref[i] - test[i];
      sum_squared_error += error * error;
   }
   double mean_squared_error = sum_squared_error / n;
   return sqrt(mean_squared_error);
}

TEST_CASE("Test_db_conversion_accuracy")
{
   {
      std::vector<float> ref, ref2;
      std::vector<float> test, test2;
      for (int i = 1; i < 1000000; ++i)
      {
         auto a = float(i) / 1000;
         auto db1 = 20 * cycfi::q::fast_log10(a);
         auto db2 = 20 * std::log10(a);
         test.push_back(db1);
         ref.push_back(db2);

         auto a1 = cycfi::q::fast_pow10(db2/20);
         auto a2 = std::pow(10, db2/20);
         test2.push_back(a1);
         ref2.push_back(a2);
      }
      std::cout << "Root Mean Square Error fast_log10: " << rmse(ref, test) << std::endl;
      std::cout << "Root Mean Square Error fast_pow10: " << rmse(ref2, test2) << std::endl;

      CHECK(rmse(ref, test) < 0.0006);
      CHECK(rmse(ref2, test2) < 0.09);
   }

   {
      std::vector<float> ref, ref2;
      std::vector<float> test, test2;
      for (int i = 1; i < 1000000; ++i)
      {
         auto a = float(i) / 1000;
         auto db1 = 20 * cycfi::q::faster_log10(a);
         auto db2 = 20 * std::log10(a);
         test.push_back(db1);
         ref.push_back(db2);

         auto a1 = cycfi::q::faster_pow10(db2/20);
         auto a2 = std::pow(10, db2/20);
         test2.push_back(a1);
         ref2.push_back(a2);
      }
      std::cout << "Root Mean Square Error faster_log10: " << rmse(ref, test) << std::endl;
      std::cout << "Root Mean Square Error faster_pow10: " << rmse(ref2, test2) << std::endl;

      CHECK(rmse(ref, test) < 0.15);
      CHECK(rmse(ref2, test2) < 17);
   }
}

TEST_CASE("Test_decibel_speed")
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
            auto result = 20 * std::log10(a);
            accu += result;
         }
      }

      auto elapsed = std::chrono::high_resolution_clock::now() - start;
      auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed);

      std::cout << "20 * std::log10 elapsed(a) (ns): " << float(duration.count()) / (1024*1023) << std::endl;
      CHECK(duration.count() > 0);
   }

   {
      auto start = std::chrono::high_resolution_clock::now();
      using cycfi::q::fast_log10;

      for (int j = 0; j < 1024; ++j)
      {
         for (int i = 1; i < 1024; ++i)
         {
            auto a = float(i);
            auto result = 20 * fast_log10(a);
            accu += result;
         }
      }

      auto elapsed = std::chrono::high_resolution_clock::now() - start;
      auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed);

      std::cout << "20 * fast_log10(a) elapsed (ns): " << float(duration.count()) / (1024*1023) << std::endl;
      CHECK(duration.count() > 0);
   }

   {
      auto start = std::chrono::high_resolution_clock::now();
      using cycfi::q::faster_log10;

      for (int j = 0; j < 1024; ++j)
      {
         for (int i = 1; i < 1024; ++i)
         {
            auto a = float(i);
            auto result = 20 * faster_log10(a);
            accu += result;
         }
      }

      auto elapsed = std::chrono::high_resolution_clock::now() - start;
      auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed);

      std::cout << "20 * faster_log10(a) elapsed (ns): " << float(duration.count()) / (1024*1023) << std::endl;
      CHECK(duration.count() > 0);
   }

   {
      auto start = std::chrono::high_resolution_clock::now();

      for (int j = 0; j < 1024; ++j)
      {
         for (int i = 0; i < 1200; ++i)
         {
            auto db = float(i) / 10;
            auto result = q::detail::db2a(db);
            accu += result;
         }
      }

      auto elapsed = std::chrono::high_resolution_clock::now() - start;
      auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed);

      std::cout << "db2a(db) elapsed (ns): " << float(duration.count()) / (1024*1200) << std::endl;
      CHECK(duration.count() > 0);
   }

   {
      auto start = std::chrono::high_resolution_clock::now();

      for (int j = 0; j < 1024; ++j)
      {
         for (int i = 0; i < 1200; ++i)
         {
            auto db = float(i) / 10;
            auto result = std::pow(10, db/20);
            accu += result;
         }
      }

      auto elapsed = std::chrono::high_resolution_clock::now() - start;
      auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed);

      std::cout << "pow(10, db/20) elapsed (ns): " << float(duration.count()) / (1024*1200) << std::endl;
      CHECK(duration.count() > 0);
   }

   {
      auto start = std::chrono::high_resolution_clock::now();
      using cycfi::q::fast_pow10;

      for (int j = 0; j < 1024; ++j)
      {
         for (int i = 0; i < 1200; ++i)
         {
            auto db = float(i) / 10;
            auto result = fast_pow10(db/20);
            accu += result;
         }
      }

      auto elapsed = std::chrono::high_resolution_clock::now() - start;
      auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed);

      std::cout << "fast_pow10(db/20) elapsed (ns): " << float(duration.count()) / (1024*1200) << std::endl;
      CHECK(duration.count() > 0);
   }

   {
      auto start = std::chrono::high_resolution_clock::now();
      using cycfi::q::faster_pow10;

      for (int j = 0; j < 1024; ++j)
      {
         for (int i = 0; i < 1200; ++i)
         {
            auto db = float(i) / 10;
            auto result = faster_pow10(db/20);
            accu += result;
         }
      }

      auto elapsed = std::chrono::high_resolution_clock::now() - start;
      auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed);

      std::cout << "faster_pow10(db/20) elapsed (ns): " << float(duration.count()) / (1024*1200) << std::endl;
      CHECK(duration.count() > 0);
   }

   // Prevent dead-code elimination
   CHECK(accu > 0);
}

