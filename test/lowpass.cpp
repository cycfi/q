/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/fx/lowpass.hpp>
#include <q/support/literals.hpp>

#include <algorithm>

// leaky_integrator is deprecated; its tests stay until it is removed.
#if defined(_MSC_VER)
# pragma warning(disable : 4996)
#else
# pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

namespace q = cycfi::q;
using namespace q::literals;

namespace
{
   constexpr auto sps = 48000.0f;
}

TEST_CASE("leaky_integrator: the step response is 1 - a^n")
{
   q::leaky_integrator li{0.9f};

   float an = 1.0f;
   for (int n = 1; n != 50; ++n)
   {
      an *= 0.9f;
      CHECK(li(1.0f) == Approx(1.0f - an).margin(1e-6));
   }
}

TEST_CASE("leaky_integrator: unity gain at DC")
{
   q::leaky_integrator li;                   // a = 0.995
   CHECK(li.a == Approx(0.995f));

   for (int i = 0; i != 10000; ++i)
      li(0.25f);
   CHECK(li() == Approx(0.25f).margin(1e-6));
}

TEST_CASE("leaky_integrator: the pole from a cutoff is 1 - 2pi f / sps")
{
   q::leaky_integrator li{100_Hz, sps};
   CHECK(li.a == Approx(1.0f - (2 * q::pi * 100.0 / sps)));

   li.cutoff(1_kHz, sps);
   CHECK(li.a == Approx(1.0f - (2 * q::pi * 1000.0 / sps)));
}

TEST_CASE("leaky_integrator: assignment sets the state")
{
   q::leaky_integrator li{0.5f};
   li = 1.0f;
   CHECK(li() == 1.0f);
   CHECK(li(0.0f) == 0.5f);
}

TEST_CASE("fixed_pt_leaky_integrator: settles at k times the input")
{
   q::fixed_pt_leaky_integrator<16> li;
   CHECK(li.gain == 16);

   for (int i = 0; i != 1000; ++i)
      li(100);
   CHECK(li() / li.gain == 100);
}

TEST_CASE("fixed_pt_leaky_integrator: y += s - y / k")
{
   q::fixed_pt_leaky_integrator<4> li;
   CHECK(li(8) == 8);                        // 0 + 8 - 0
   CHECK(li(8) == 14);                       // 8 + 8 - 2
   CHECK(li(8) == 19);                       // 14 + 8 - 3
}

TEST_CASE("one_pole_lowpass: stable for every cutoff up to Nyquist")
{
   for (float f = 10.0f; f <= sps / 2; f *= 1.5f)
   {
      q::one_pole_lowpass lp{q::frequency(f), sps};
      CHECK(lp.a > 0.0f);
      CHECK(lp.a < 1.0f);

      // A step never overshoots and ends at the input.
      float peak = 0.0f;
      for (int i = 0; i != 100000; ++i)
         peak = std::max(peak, lp(1.0f));
      CHECK(peak <= 1.0f);
      CHECK(lp() == Approx(1.0f).margin(1e-4));
   }
}
