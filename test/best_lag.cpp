/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/utility/best_lag.hpp>
#include <q/utility/ring_buffer.hpp>

#include <cmath>
#include <random>

namespace q = cycfi::q;

TEST_CASE("best_lag: recovers a sine's period with sub-sample precision")
{
   constexpr auto period = 100.25;
   q::ring_buffer<float> buf(4096);
   for (auto i = 0; i != 4096; ++i)
      buf.push(float(std::sin(2 * M_PI * i / period)));

   auto r = q::best_lag(buf, 512, 80, 130);
   CHECK(r.lag == Approx(period).margin(0.1));
   CHECK(r.similarity > 0.99f);
}

TEST_CASE("best_lag: polyphonic self-similarity, no fundamental tracking")
{
   // A dyad with periods 100 and 150: the common quasi-period is 300.
   // Nothing here requires a fundamental -- only self-similarity.
   q::ring_buffer<float> buf(4096);
   for (auto i = 0; i != 4096; ++i)
   {
      auto s = std::sin(2 * M_PI * i / 100.0)
         + 0.8 * std::sin(2 * M_PI * i / 150.0);
      buf.push(float(s));
   }

   auto r = q::best_lag(buf, 600, 250, 350);
   CHECK(r.lag == Approx(300.0).margin(0.1));
   CHECK(r.similarity > 0.99f);
}

TEST_CASE("best_lag: finds the best lag within the given bounds")
{
   // Period 100, but the search range only admits the second multiple
   constexpr auto period = 100.0;
   q::ring_buffer<float> buf(4096);
   for (auto i = 0; i != 4096; ++i)
      buf.push(float(std::sin(2 * M_PI * i / period)));

   auto r = q::best_lag(buf, 512, 150, 250);
   CHECK(r.lag >= 150.0f);
   CHECK(r.lag <= 250.0f);
   CHECK(r.lag == Approx(200.0).margin(0.1));
   CHECK(r.similarity > 0.99f);
}

TEST_CASE("best_lag: noise has low self-similarity")
{
   q::ring_buffer<float> buf(4096);
   std::minstd_rand gen(12345);
   std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
   for (auto i = 0; i != 4096; ++i)
      buf.push(dist(gen));

   auto r = q::best_lag(buf, 512, 80, 400);
   CHECK(r.similarity < 0.5f);
}
