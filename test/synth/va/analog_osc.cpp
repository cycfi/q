/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/synth/va/analog_osc.hpp>
#include <q/synth/va/saw_osc.hpp>
#include <q/synth/va/pulse_osc.hpp>
#include <q/support/literals.hpp>

#include <cmath>
#include <vector>

namespace q = cycfi::q;
using namespace q::literals;

namespace
{
   constexpr auto sps = 48000.0f;

   template <typename F>
   std::vector<float> cycle(F&& f, q::frequency freq, int cycles = 8)
   {
      q::phase_iterator pi{freq, sps};
      auto n = int(cycles * sps / as_double(freq));
      std::vector<float> out;
      out.reserve(n);
      for (int i = 0; i != n; ++i, ++pi)
         out.push_back(f(pi));
      return out;
   }

   float mean(std::vector<float> const& y)
   {
      double sum = 0;
      for (auto v : y)
         sum += v;
      return float(sum / y.size());
   }
}

TEST_CASE("analog_osc: the sawtooth is q's sawtooth")
{
   // Same ramp, same correction: the core's value is worth checking against
   // the oscillator the library already has.
   q::analog_osc osc;
   q::saw_osc saw;
   q::phase_iterator a{440_Hz, sps};
   q::phase_iterator b{440_Hz, sps};

   for (int i = 0; i != 2000; ++i, ++a, ++b)
      CHECK(osc.saw(a) == Approx(saw(b)).margin(1e-6f));
}

TEST_CASE("analog_osc: the pulse width is what it says")
{
   // The mean of a pulse of width w is 2w - 1.
   for (float w : {0.1f, 0.25f, 0.5f, 0.75f, 0.9f})
   {
      q::analog_osc osc{w};
      auto y = cycle([&osc](auto i) { return osc.pulse(i); }, 200_Hz, 40);
      CHECK(mean(y) == Approx(2.0f * w - 1.0f).margin(0.02f));
   }
}

TEST_CASE("analog_osc: a square pulse is q's square")
{
   q::analog_osc osc{0.5f};
   q::pulse_osc pulse{0.5f};
   q::phase_iterator a{330_Hz, sps};
   q::phase_iterator b{330_Hz, sps};

   for (int i = 0; i != 2000; ++i, ++a, ++b)
      CHECK(osc.pulse(a) == Approx(pulse(b)).margin(1e-6f));
}

TEST_CASE("analog_osc: the triangle is q's triangle")
{
   q::analog_osc osc{0.5f, 0.3f};
   q::triangle_osc tri{0.3f};
   q::phase_iterator a{220_Hz, sps};
   q::phase_iterator b{220_Hz, sps};

   for (int i = 0; i != 2000; ++i, ++a, ++b)
      CHECK(osc.triangle(a) == Approx(tri(b)).margin(1e-6f));
}

TEST_CASE("analog_osc: every waveform stays in range")
{
   q::analog_osc osc{0.2f};
   osc.symmetry(0.3f);

   q::phase_iterator pi{2_kHz, sps};
   for (int i = 0; i != 5000; ++i, ++pi)
   {
      REQUIRE(std::abs(osc.saw(pi)) <= 1.2f);
      REQUIRE(std::abs(osc.pulse(pi)) <= 1.2f);
      REQUIRE(std::abs(osc.triangle(pi)) <= 1.0f);
   }
}

TEST_CASE("analog_osc: the controls are the oscillators' own")
{
   q::analog_osc osc;

   osc.width(0.3f);
   CHECK(osc.width() == Approx(0.3f));       // the pulse's
   osc.symmetry(0.7f);
   CHECK(osc.symmetry() == Approx(0.7f));    // the triangle's
}
