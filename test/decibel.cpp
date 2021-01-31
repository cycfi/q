/*=============================================================================
   Copyright (c) 2014-2021 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/detail/db_table.hpp>
#include <q/support/decibel.hpp>
#include <q/support/literals.hpp>
#include <cmath>

namespace q = cycfi::q;

TEST_CASE("Test_decibel_conversion")
{
   for (int i = 1; i < 1024; ++i)
   {
      {
         auto a = float(i);
         INFO("value: " << a);
         auto result = q::detail::a2db(a);
         CHECK(result == Approx(20 * std::log10(a)).epsilon(0.0001));
      }

      for (int j = 0; j < 100; ++j)
      {
         auto a = float(i) + (j / 10.0f);
         auto result = q::detail::a2db(a);
         INFO("value: " << a);
         CHECK(result == Approx(20 * std::log10(a)).epsilon(0.01));
      }
   }

   for (int i = 1024; i < 1048576; ++i)
   {
      auto a = float(i);
      INFO("value: " << a);
      auto result = q::detail::a2db(a);
      CHECK(result == Approx(20 * std::log10(a)).epsilon(0.01));
   }
}

TEST_CASE("Test_inverse_decibel_conversion")
{
   {
      auto db = 119.94;
      INFO("dB: " << db);
      auto result = q::detail::db2a(db);
      CHECK(result == Approx(std::pow(10, db/20)).epsilon(0.001));
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
         CHECK(result == Approx(std::pow(10, db/20)).epsilon(0.0001));
      }

      for (int j = 0; j < 10; ++j)
      {
         auto db = float(i) + (j / 10.0f);
         INFO("dB: " << db);
         auto result = q::detail::db2a(db/10.0);
         CHECK(result == Approx(std::pow(10, (db/10.0)/20)).epsilon(0.0001));
      }
   }
}

TEST_CASE("Test_negative_decibel")
{
   {
      auto db = -6;
      INFO("dB: " << db);
      auto result = q::detail::db2a(db);
      CHECK(result == Approx(0.5).epsilon(0.01));
   }

   {
      auto db = -24;
      INFO("dB: " << db);
      auto result = q::detail::db2a(db);
      CHECK(result == Approx(0.063096).epsilon(0.0001));
   }

   {
      auto db = -36;
      INFO("dB: " << db);
      auto result = q::detail::db2a(db);
      CHECK(result == Approx(0.015849).epsilon(0.0001));
   }

   {
      auto a = 0.1;
      INFO("val: " << a);
      auto result = q::detail::a2db(a);
      CHECK(result == Approx(-20).epsilon(0.0001));
   }

   {
      auto a = 0.01;
      INFO("val: " << a);
      auto result = q::detail::a2db(a);
      CHECK(result == Approx(-40).epsilon(0.0001));
   }

   {
      auto a = 0.001;
      INFO("val: " << a);
      auto result = q::detail::a2db(a);
      CHECK(result == Approx(-60).epsilon(0.0001));
   }

   {
      auto a = 0.0001;
      INFO("val: " << a);
      auto result = q::detail::a2db(a);
      CHECK(result == Approx(-80).epsilon(0.001));
   }

   {
      auto a = 0;
      INFO("val: " << a);
      auto result = q::detail::a2db(a);
      CHECK(result < -120.0); // -120dB is the limit we can compute
   }
}

TEST_CASE("Test_decibel_operations")
{
   using namespace q::literals;
   {
      q::decibel db = 48_dB;
      {
         auto a = float(db);
         CHECK(a == Approx(251.19).epsilon(0.01));
      }
      {
         // A square root is just divide by two in the log domain
         auto a = float(db / 2.0f);
         CHECK(a == Approx(15.85).epsilon(0.01));
      }
   }
}