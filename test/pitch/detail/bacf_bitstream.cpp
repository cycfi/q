/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/pitch/detail/bacf_bitstream.hpp>

#include <cmath>
#include <cstdint>
#include <vector>

namespace q = cycfi::q;
using q::detail::bacf_bitstream;
using q::detail::bacf_pulse;

namespace
{
   constexpr auto ws = bacf_bitstream::word_size;
   double const pi = 3.14159265358979;

   // A test signal: a sine with a slowly changing period, so lags and
   // edges move about.
   float signal(std::size_t i)
   {
      double const f = 200.0 + 60.0 * std::sin(2 * pi * i / 9000.0);
      return float(0.5 * std::sin(2 * pi * f * i / 48000.0));
   }

   // The state after each sample, kept outside: the reference.
   struct recorder
   {
      std::vector<bool> bits;
      bool state = false;

      void push(float s, float hysteresis)
      {
         if (!state && s > hysteresis)
            state = true;
         else if (state && s < -hysteresis)
            state = false;
         bits.push_back(state);
      }

      // The latest `w` bits against the `w` bits `lag` earlier, the end
      // aligned down to a word, counted one bit at a time.
      std::uint32_t mismatch(std::uint32_t lag, std::uint32_t w) const
      {
         auto const start = (bits.size() - w - lag) / ws * ws;
         std::uint32_t n = 0;
         for (std::size_t i = 0; i != w; ++i)
            n += bits[start + i] != bits[start + i + lag];
         return n;
      }
   };
}

TEST_CASE("bacf_bitstream: edges at the hysteresis crossings")
{
   // Leading edges where the signal first rises over the hysteresis,
   // trailing where it first falls under its negative: reported at that
   // sample, before its bit enters the history.
   float const h = 0.1f;
   bacf_bitstream bits{h, 4096, 256};
   recorder ref;
   std::vector<std::uint32_t> leads, trails;
   for (std::size_t i = 0; i != 48000; ++i)
   {
      auto const s = signal(i);
      bool const was = ref.state;
      ref.push(s, h);
      bits(s, [&](bacf_bitstream::edge_kind k)
      {
         REQUIRE(bits.now() == i);
         REQUIRE(bits.history() == std::min<std::size_t>(i, 4096));
         if (k == bacf_bitstream::leading)
            leads.push_back(bits.now());
         else
            trails.push_back(bits.now());
      });
      if (!was && ref.state)
         REQUIRE((!leads.empty() && leads.back() == i));
      if (was && !ref.state)
         REQUIRE((!trails.empty() && trails.back() == i));
   }
   CHECK(leads.size() > 100);
   CHECK(trails.size() >= leads.size() - 1);
   CHECK(bits.now() == 48000);

   // The edges, newest first, as the rings hold them.
   auto const e = bits.edges(bacf_bitstream::leading);
   REQUIRE(e.size() == std::min<std::size_t>(leads.size(), 256));
   for (std::size_t i = 0; i != e.size(); ++i)
      CHECK(e[i] == leads[leads.size() - 1 - i]);
}

TEST_CASE("bacf_bitstream: pulses carry lead, peak and width")
{
   // A pulse runs from a leading edge to the trailing one. Its peak is the
   // highest sample, its width the samples from the lead to the first one
   // under 30% of the peak since (or to the trailing edge, if none is).
   bacf_bitstream bits{0.1f, 4096, 256};
   std::vector<float> x(400, -0.2f);
   // A pulse leading at 10, peaking 0.8 at 20, under 0.24 from 40,
   // trailing at 60.
   for (int i = 10; i != 60; ++i)
      x[i] = i <= 20 ? 0.15f + 0.65f * (i - 10) / 10.0f
         : i < 40 ? 0.8f - 0.5f * (i - 20) / 20.0f : 0.2f;
   x[20] = 0.8f;
   x[60] = -0.2f;
   // One with no fall under 30%: its width runs to the trailing edge (200).
   for (int i = 100; i != 200; ++i)
      x[i] = 0.5f;

   std::vector<bacf_pulse> seen;
   for (std::size_t i = 0; i != x.size(); ++i)
   {
      bits(x[i], [&](bacf_bitstream::edge_kind k)
      {
         if (k == bacf_bitstream::trailing)
            seen.push_back(bits.pulses()[0]);   // the pulse just ended
      });
   }
   REQUIRE(seen.size() == 2);
   CHECK(seen[0].lead == 10);
   CHECK(seen[0].peak == 0.8f);
   CHECK(seen[0].width == 30.0f);   // 0.2 at 40, the first under 0.24
   CHECK(seen[1].lead == 100);
   CHECK(seen[1].peak == 0.5f);
   CHECK(seen[1].width == 100.0f);

   auto const p = bits.pulses();
   REQUIRE(p.size() == 2);
   CHECK(p[0].lead == 100);
   CHECK(p[1].lead == 10);
}

TEST_CASE("bacf_bitstream: the mismatch is the bits that differ")
{
   // Against a count one bit at a time, at many lags and windows, through
   // several compactions of the history.
   float const h = 0.05f;
   std::size_t const history = 2048;
   bacf_bitstream bits{h, history, 256};
   recorder ref;
   std::size_t checked = 0;
   for (std::size_t i = 0; i != 20 * history; ++i)
   {
      auto const s = signal(i);
      ref.push(s, h);
      bits(s, [](bacf_bitstream::edge_kind) {});
      if (i % 97 != 0)
         continue;
      for (std::uint32_t lag : {96u, 160u, 241u, 500u, 977u})
      {
         std::uint32_t const ws1 = ws;
         for (std::uint32_t w : {ws1, 4 * ws1, 512u})
         {
            if (!bits.has(lag, w))
            {
               CHECK(bits.history() < w + lag + ws);
               continue;
            }
            REQUIRE(bits.mismatch(lag, w) == ref.mismatch(lag, w));
            ++checked;
         }
      }
   }
   CHECK(checked > 1000);
}

TEST_CASE("bacf_bitstream: history and reset")
{
   // history() counts the bits held, up to the size given; has() asks for
   // a window, a lag and a word more. reset() empties it all but the clock.
   bacf_bitstream bits{0.1f, 1024, 256};
   for (std::size_t i = 0; i != 3000; ++i)
      bits(signal(i), [](bacf_bitstream::edge_kind) {});
   CHECK(bits.history() == 1024);
   CHECK(bits.has(100, 1024 - 100 - ws));
   CHECK(!bits.has(100, 1024 - 100 - ws + 1));
   CHECK(bits.pulses().size() > 0);

   bits.reset();
   CHECK(bits.history() == 0);
   CHECK(bits.pulses().size() == 0);
   CHECK(bits.edges(bacf_bitstream::leading).size() == 0);
   CHECK(bits.edges(bacf_bitstream::trailing).size() == 0);
   CHECK(bits.now() == 3000);
   CHECK(!bits.has(1, ws));
}

TEST_CASE("bacf_pulse: similar pulses")
{
   // Within 40% in height and 15% in width.
   bacf_pulse const a{0, 1.0f, 100.0f};
   CHECK(similar(a, bacf_pulse{50, 0.61f, 100.0f}));
   CHECK(!similar(a, bacf_pulse{50, 0.59f, 100.0f}));
   CHECK(similar(a, bacf_pulse{50, 1.0f, 86.0f}));
   CHECK(!similar(a, bacf_pulse{50, 1.0f, 84.0f}));
}
