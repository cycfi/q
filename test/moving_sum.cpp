/*=============================================================================
   Copyright (c) 2014-2023 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/fx/moving_sum.hpp>

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

   ms.resize(8);
   CHECK(ms() == 15);

   ms.resize(10);
   CHECK(ms() == 20);

   ms.resize(1);
   CHECK(ms() == 5);

   ms.resize(10);
   CHECK(ms() == 20);
}

