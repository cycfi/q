/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/fx/delay.hpp>

namespace q = cycfi::q;

TEST_CASE("delay1: one sample delay")
{
   q::delay1 d;

   CHECK(d(1.0f) == 0.0f);   // initial state
   CHECK(d(2.0f) == 1.0f);
   CHECK(d(3.0f) == 2.0f);

   // operator()() peeks the stored sample (the next output)
   CHECK(d() == 3.0f);
   CHECK(d(4.0f) == 3.0f);

   d.reset();
   CHECK(d(5.0f) == 0.0f);
}

TEST_CASE("delay2: two sample delay")
{
   q::delay2 d;

   CHECK(d(1.0f) == 0.0f);
   CHECK(d(2.0f) == 0.0f);
   CHECK(d(3.0f) == 1.0f);
   CHECK(d(4.0f) == 2.0f);
   CHECK(d() == 3.0f);       // peek the next output

   d.reset();
   CHECK(d(5.0f) == 0.0f);
   CHECK(d(6.0f) == 0.0f);
}

TEST_CASE("basic_delay: duration constructor sizes as ceil(seconds * sps)")
{
   // 1 ms at 48 kHz = 48 samples, rounded up to a power of two
   q::nf_delay d{q::duration(0.001), 48000.0f};
   CHECK(d.size() == 64);

   // Explicit sample-count constructor
   q::nf_delay d2{48};
   CHECK(d2.size() == 64);

   q::delay d3{1024};
   CHECK(d3.size() == 1024);
}

TEST_CASE("nf_delay: push-and-read delays by index + 1")
{
   q::nf_delay d{8};

   // operator()(val, i) reads the delayed sample at index i, then pushes:
   // the result is the input delayed by i+1 samples. Feed an impulse with
   // i = 3: it must emerge at the 5th call (a delay of 4 samples).
   CHECK(d(1.0f, 3) == 0.0f);
   CHECK(d(0.0f, 3) == 0.0f);
   CHECK(d(0.0f, 3) == 0.0f);
   CHECK(d(0.0f, 3) == 0.0f);
   CHECK(d(0.0f, 3) == 1.0f);
   CHECK(d(0.0f, 3) == 0.0f);
}

TEST_CASE("nf_delay: operator()() reads the maximum delay")
{
   q::nf_delay d{4};

   for (float v : {10.0f, 20.0f, 30.0f, 40.0f})
      d.push(v);

   // The maximum-delay read is the oldest element
   CHECK(d() == 10.0f);
}

TEST_CASE("delay: fractional delay on a ramp")
{
   q::delay d{16};

   // Reading at fractional index 2.5 before each push yields the input
   // delayed by 3.5 samples: with a unit ramp input, out = n - 3.5 once
   // the line is filled.
   for (auto n = 0; n != 32; ++n)
   {
      auto out = d(float(n), 2.5f);
      if (n >= 4)
         CHECK(out == Approx(n - 3.5f));
   }
}

TEST_CASE("delay: multi-tap reads before push")
{
   q::delay d{16};

   // The documented multi-tap pattern: read the taps with the indexing
   // operator, then push the newest sample. At iteration n, before
   // pushing x[n] = n, tap i holds (n-1) - i.
   for (auto n = 0; n != 32; ++n)
   {
      auto tap1 = d[1.5f];
      auto tap2 = d[6.25f];
      if (n >= 8)
      {
         CHECK(tap1 == Approx((n-1) - 1.5f));
         CHECK(tap2 == Approx((n-1) - 6.25f));
      }
      d.push(float(n));
   }
}
