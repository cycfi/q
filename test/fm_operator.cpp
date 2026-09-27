/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/support/literals.hpp>
#include <q/synth/fm/fm_operator.hpp>
#include <q/synth/va/saw_osc.hpp>
#include <q/synth/gen/envelope_gen.hpp>
#include <cmath>
#include <vector>

namespace q = cycfi::q;
using eg = q::dx_envelope_gen;
using namespace q::literals;
using op_t = q::fm_operator;

static_assert(q::concepts::Oscillator<op_t>);
static_assert(
   q::concepts::Oscillator<q::basic_fm_operator<q::basic_saw_osc>>);
static_assert(q::concepts::EnvelopeGenerator<q::adsr_envelope_gen>);
static_assert(q::concepts::Oscillator<
   q::basic_fm_operator<q::sin_osc, q::adsr_envelope_gen>>);

namespace
{
   constexpr float sps = 48000.0f;
   constexpr std::size_t n = 48000;

   constexpr auto at_once = q::duration{1e-6};  // within a sample

   // An envelope at once at full level, off at release
   eg::config instant()
   {
      return {.rate = {at_once, at_once, at_once, at_once}
         , .level = {q::dB(0), q::dB(0), q::dB(0), eg::floor}};
   }

   template <typename Op>
   std::vector<float> render(Op& op, q::frequency f, std::size_t count = n)
   {
      q::phase_iterator master{f, sps};
      std::vector<float> y(count);
      for (auto& v : y)
         v = op(master++);
      return y;
   }

   // Frequency in Hz by counting rising zero crossings
   double freq(std::vector<float> const& x)
   {
      int count = 0;
      for (std::size_t i = 1; i != x.size(); ++i)
         if (x[i-1] <= 0.0f && x[i] > 0.0f)
            ++count;
      return count * sps / x.size();
   }
}

TEST_CASE("Follows the master by its ratio")
{
   auto at = [](double ratio, q::frequency master)
   {
      op_t op;
      op.ratio(ratio);
      op.envelope(instant(), sps);
      op.attack();
      return freq(render(op, master));
   };
   CHECK(at(1.0, 440_Hz) == Approx(440).margin(1));
   CHECK(at(2.0, 440_Hz) == Approx(880).margin(1));
   CHECK(at(0.5, 440_Hz) == Approx(220).margin(1));
   CHECK(at(3.6, 100_Hz) == Approx(360).margin(1));

   // and a change of the master's pitch, at once
   op_t op;
   op.ratio(1.0);
   op.envelope(instant(), sps);
   op.attack();
   CHECK(freq(render(op, 440_Hz)) == Approx(440).margin(1));
   CHECK(freq(render(op, 220_Hz)) == Approx(220).margin(1));
}

TEST_CASE("A fixed frequency ignores the master")
{
   op_t op;
   op.fixed(q::phase{100_Hz, sps});
   op.envelope(instant(), sps);
   op.attack();
   CHECK(freq(render(op, 440_Hz)) == Approx(100).margin(1));
   CHECK(freq(render(op, 55_Hz)) == Approx(100).margin(1));
}

TEST_CASE("A sine from its own phase, restarted by sync")
{
   op_t op;
   op.ratio(1.0);
   op.envelope(instant(), sps);
   op.attack();
   op.sync();
   q::phase_iterator master{440_Hz, sps};
   for (std::size_t i = 0; i != n; ++i, ++master)
      REQUIRE(op(master) == Approx(std::sin(2.0 * q::pi * 440.0 * i / sps))
         .margin(1e-3));

   op.sync();
   CHECK(op(master) == Approx(0.0).margin(1e-6));
}

TEST_CASE("The envelope shapes the output, and active() follows it")
{
   op_t op;
   CHECK(!op.active());
   op.ratio(1.0);
   // A release that takes the whole range in half a second
   op.envelope({.rate = {eg::fastest, eg::fastest, eg::fastest, 0.5_s}
      , .level = {q::dB(0), q::dB(0), q::dB(0), eg::floor}}, sps);
   op.attack();
   CHECK(op.active());
   render(op, 440_Hz, 4800);

   op.release();
   CHECK(op.active());                         // still releasing
   auto y = render(op, 440_Hz, n);            // a second: past the release
   CHECK(std::abs(y[0]) < 1.0f);
   CHECK(!op.active());
   CHECK(y[n - 1] == 0.0f);
}

TEST_CASE("The modulation shifts the phase, and the gain scales the output")
{
   op_t op;
   op.ratio(1.0);
   op.envelope(instant(), sps);
   op.attack();
   auto quarter = q::phase{q::phase::one_cyc / 4, q::direct_unit};
   op.modulation(quarter);
   op.gain(0.5f);
   CHECK(op.modulation().rep == quarter.rep);
   CHECK(op.gain() == 0.5f);
   q::phase_iterator master{1000_Hz, sps};
   for (std::size_t i = 0; i != 4800; ++i, ++master)
   {
      auto y = op(master);
      auto r = 0.5 * std::sin(2.0 * q::pi * 1000.0 * i / sps + q::pi / 2);
      REQUIRE(y == Approx(r).margin(1e-3));
   }
}

TEST_CASE("Any envelope generator will do")
{
   using namespace q::literals;
   q::basic_fm_operator<q::sin_osc, q::adsr_envelope_gen> op;
   op.ratio(1.0);
   op.envelope({.attack_rate = 10_ms, .decay_rate = 10_ms
      , .sustain_level = -6_dB, .release_rate = 20_ms}, sps);
   op.attack();
   CHECK(op.active());
   auto y = render(op, 440_Hz, 4800);       // 100 ms: in the sustain
   CHECK(std::abs(y[10]) < 0.5f);           // still rising at 0.2 ms
   float peak = 0.0f;
   for (std::size_t i = 2400; i != 4800; ++i)
      peak = std::max(peak, std::abs(y[i]));
   CHECK(peak == Approx(0.5).margin(0.02));  // -6 dB
   op.release();
   render(op, 440_Hz, 48000);
   CHECK(!op.active());
}

TEST_CASE("Any oscillator will do")
{
   q::basic_fm_operator<q::basic_saw_osc> op;
   op.ratio(1.0);
   op.envelope(instant(), sps);
   op.attack();
   q::phase_iterator master{440_Hz, sps};
   q::phase_iterator ref{440_Hz, sps};
   for (std::size_t i = 0; i != n; ++i, ++master, ++ref)
      REQUIRE(op(master) == Approx(q::basic_saw(ref)).margin(1e-6));
}
