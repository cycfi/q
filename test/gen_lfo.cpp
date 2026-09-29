/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/synth/concepts.hpp>
#include <q/synth/gen/lfo_gen.hpp>
#include <cmath>
#include <set>
#include <vector>

namespace q = cycfi::q;
using lfo = q::lfo_gen;

static_assert(q::concepts::Generator<lfo>);

namespace
{
   constexpr float sps = 48000.0f;

   std::vector<float> run(lfo& l, std::size_t n)
   {
      std::vector<float> y(n);
      for (auto& v : y)
         v = l();
      return y;
   }

   // Cycles per second by counting rising zero crossings
   double rate(std::vector<float> const& y)
   {
      int count = 0;
      for (std::size_t i = 1; i != y.size(); ++i)
         if (y[i-1] <= 0.0f && y[i] > 0.0f)
            ++count;
      return count * sps / y.size();
   }
}

TEST_CASE("Runs at its rate, from -1 to 1, and value() reads the last")
{
   lfo l{{.rate = 5.0f}, sps};
   l.key_on();
   auto y = run(l, 48000 * 4);
   CHECK(rate(y) == Approx(5.0).margin(0.3));
   CHECK(*std::min_element(y.begin(), y.end()) == Approx(-1.0).margin(0.01));
   CHECK(*std::max_element(y.begin(), y.end()) == Approx(1.0).margin(0.01));
   auto v = l();
   CHECK(l.value() == v);
}

TEST_CASE("Each waveform has its shape")
{
   auto probe = [](std::uint8_t wave)
   {
      lfo l{{.rate = 10.0f, .wave = wave}, sps};
      l.key_on();
      return run(l, 4800);                    // one cycle
   };
   auto tri = probe(lfo::triangle);
   CHECK(tri[0] == Approx(-1.0).margin(0.01));
   CHECK(tri[2400] == Approx(1.0).margin(0.01));
   auto down = probe(lfo::saw_down);
   CHECK(down[100] > down[2400]);
   auto up = probe(lfo::saw_up);
   CHECK(up[100] < up[2400]);
   auto sq = probe(lfo::square);
   CHECK(std::abs(sq[100]) == Approx(1.0));
   CHECK(sq[100] == -sq[2500]);
   auto sine = probe(lfo::sine);
   CHECK(sine[1200] == Approx(1.0).margin(0.01));

   lfo l{{.rate = 100.0f, .wave = lfo::sample_hold}, sps};
   l.key_on();
   std::set<float> values;
   for (auto v : run(l, 48000))
      values.insert(v);
   CHECK(values.size() > 50);              // a new value per cycle
}

TEST_CASE("Key-on waits, then fades in linearly")
{
   lfo l{{.rate = 10.0f, .delay = 0.5f, .fade = 1.0f, .wave = lfo::square}
      , sps};
   l.key_on();
   auto y = run(l, 48000 * 2);
   for (std::size_t i = 0; i != 24000; ++i)
      REQUIRE(y[i] == 0.0f);                 // the wait
   CHECK(std::abs(y[24000 + 24000]) == Approx(0.5).margin(0.02));   // half
   CHECK(std::abs(y[24000 + 48000 + 100]) == Approx(1.0).margin(0.01));

   // Without a delay the LFO is at full from the start
   lfo m{{.rate = 10.0f, .wave = lfo::square}, sps};
   m.key_on();
   CHECK(std::abs(m()) == Approx(1.0));
}

TEST_CASE("Key sync restarts the phase, or not")
{
   lfo a{{.rate = 7.0f, .key_sync = true}, sps};
   a.key_on();
   auto y1 = run(a, 1000);
   a.key_on();
   auto y2 = run(a, 1000);
   CHECK(y1 == y2);

   lfo b{{.rate = 7.0f, .key_sync = false}, sps};
   b.key_on();
   run(b, 1000);
   b.key_on();
   CHECK(b() != y1[0]);
}
