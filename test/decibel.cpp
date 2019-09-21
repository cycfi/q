/*=============================================================================
   Copyright (c) 2014-2019 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/detail/db_table.hpp>
#include <cmath>

namespace q = cycfi::q;

TEST_CASE("Test_decibel_conversion")
{
   for (int i = 1; i < 1024; ++i)
   {
      {
         auto a = float(i);
         INFO(" value: " << a);
         auto result = q::detail::a2db(a);
         CHECK(result == Approx(20 * std::log10(a)).epsilon(0.0001));
      }

      for (int j = 0; j < 10; ++j)
      {
         auto a = float(i) + (j / 10.0f);
         auto result = q::detail::a2db(a);
         double eps = 0.01;
         switch (i)
         {
            case 1: eps = 0.3; break;
            case 2: eps = 0.1; break;
         }
         INFO(" value: " << a << "eps: " << eps);
         CHECK(result == Approx(20 * std::log10(a)).epsilon(eps));
      }
   }

   for (int i = 1024; i < 1048576; ++i)
   {
      auto a = float(i);
      INFO(" value: " << a);
      auto result = q::detail::a2db(a);
      CHECK(result == Approx(20 * std::log10(a)).epsilon(0.01));
   }
}