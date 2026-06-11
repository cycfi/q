/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/synth/grain.hpp>
#include <q/utility/fractional_ring_buffer.hpp>

#include <vector>

namespace q = cycfi::q;

using grain_buffer = q::fractional_ring_buffer<
   float, std::vector<float>, float, q::sample_interpolation::hermite>;

constexpr auto sps = 48000.0f;

TEST_CASE("grain: inactive by default, active for exactly width ticks")
{
   grain_buffer buf(128);
   for (auto i = 0; i != 128; ++i)
      buf.push(1.0f);

   q::grain<> g{sps};
   CHECK_FALSE(g.active());
   CHECK(g(buf) == 0.0f);

   constexpr auto width = 32u;
   g.spawn(4.0f, width);
   CHECK(g.active());

   for (auto k = 0u; k != width; ++k)
   {
      INFO("tick: " << k);
      buf.push(1.0f);
      g(buf);
   }
   CHECK_FALSE(g.active());

   buf.push(1.0f);
   CHECK(g(buf) == 0.0f);
}

TEST_CASE("grain: applies the window (hann_gen reference) on DC")
{
   grain_buffer buf(128);
   for (auto i = 0; i != 128; ++i)
      buf.push(1.0f);

   constexpr auto width = 64u;
   q::grain<> g{sps};
   g.spawn(8.0f, width);

   // On a DC signal the grain's output is the window itself
   q::hann_gen ref{q::duration(width / sps), sps};
   for (auto k = 0u; k != width; ++k)
   {
      INFO("tick: " << k);
      buf.push(1.0f);
      CHECK(g(buf) == Approx(ref()).margin(1e-6));
   }
}

TEST_CASE("grain: rate-1 read at constant integer delay")
{
   // Unit ramp input: at every tick, the value at delay D is newest - D.
   // A constant delay means the tap advances at the write rate (rate 1).
   grain_buffer buf(128);
   for (auto i = 0; i != 64; ++i)
      buf.push(float(i));            // newest = 63

   constexpr auto width = 32u;
   constexpr auto delay = 10.0f;
   q::grain<> g{sps};
   g.spawn(delay, width);

   q::hann_gen ref{q::duration(width / sps), sps};
   for (auto k = 0u; k != width; ++k)
   {
      INFO("tick: " << k);
      auto newest = float(64 + k);
      buf.push(newest);
      auto expected = ref() * (newest - delay);
      CHECK(g(buf) == Approx(expected).margin(1e-3));
   }
}

TEST_CASE("grain: fractional delay reads through the interpolator")
{
   // Hermite interpolation is exact on ramps, so a fractional delay must
   // yield exactly newest - delay.
   grain_buffer buf(128);
   for (auto i = 0; i != 64; ++i)
      buf.push(float(i));

   constexpr auto width = 32u;
   constexpr auto delay = 10.25f;
   q::grain<> g{sps};
   g.spawn(delay, width);

   q::hann_gen ref{q::duration(width / sps), sps};
   for (auto k = 0u; k != width; ++k)
   {
      INFO("tick: " << k);
      auto newest = float(64 + k);
      buf.push(newest);
      auto expected = ref() * (newest - delay);
      CHECK(g(buf) == Approx(expected).margin(1e-3));
   }
}

TEST_CASE("grain: half-width hop reconstructs DC (overlap-add)")
{
   // Two grains, width N, alternately respawned every N/2 ticks: the Hann
   // windows at 50% overlap sum to one, so overlap-adding the grains on a
   // DC input reconstructs the input. This is the COLA property the PSOLA
   // scheduler relies on.
   grain_buffer buf(128);
   for (auto i = 0; i != 128; ++i)
      buf.push(1.0f);

   constexpr auto width = 64u;
   constexpr auto hop = width / 2;
   q::grain<> g[2] = {q::grain<>{sps}, q::grain<>{sps}};

   for (auto t = 0u; t != 256u; ++t)
   {
      if (t % hop == 0)
         g[(t / hop) % 2].spawn(5.0f, width);

      buf.push(1.0f);
      auto sum = g[0](buf) + g[1](buf);

      // Steady state once both grains overlap
      if (t >= width)
      {
         INFO("tick: " << t);
         CHECK(sum == Approx(1.0f).margin(0.01));
      }
   }
}
