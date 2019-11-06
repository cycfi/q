/*=============================================================================
   Copyright (c) 2014-2019 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <infra/doctest.hpp>

#include <q/support/literals.hpp>
#include <q/fx/moving_maximum.hpp>

namespace q = cycfi::q;
using namespace q::literals;

TEST_CASE("MovingMaximum")
{
   float input[] = {
      0.41, 0.55, 0.16, 0.35, 0.58
    , 0.50, 0.02, 0.97, 0.81, 0.73
    , 0.94, 0.81, 0.26, 0.01, 0.31
    , 0.29, 0.99, 0.07, 0.22, 0.63
   };

   float expected[] = {
      0.41, 0.55, 0.55, 0.55, 0.58
    , 0.58, 0.58, 0.97, 0.97, 0.97
    , 0.97, 0.97, 0.97, 0.97, 0.94
    , 0.94, 0.99, 0.99, 0.99, 0.99
   };

   auto mmax = q::moving_maximum<float>{ 7 };
   auto exp_i = std::begin(expected);
   for (auto s : input)
   {
      auto index = exp_i-std::begin(expected);
      INFO("index = " << index);

      auto r = mmax(s);
      CHECK(*exp_i++ == r);
   }
}








