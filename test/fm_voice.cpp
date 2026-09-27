/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/support/literals.hpp>
#include <q/synth/fm/fm_voice.hpp>
#include <q/synth/gen/envelope_gen.hpp>
#include <q/synth/va/saw_osc.hpp>
#include <q/synth/va/square_osc.hpp>
#include <q/synth/va/triangle_osc.hpp>
#include <cmath>
#include <vector>

namespace q = cycfi::q;
using namespace q::fm_ops;
using eg = q::dx_envelope_gen;
using namespace q::literals;

namespace
{
   constexpr float sps = 48000.0f;
   constexpr std::size_t n = 48000;
   constexpr auto silent = eg::floor;
   constexpr auto quick = q::duration{1e-6};    // within a sample

   constexpr eg::config instant = {
      .rate = {quick, quick, quick, quick}
    , .level = {q::dB(0), q::dB(0), q::dB(0), silent}};

   template <typename Voice>
   std::vector<float> render(Voice& v, q::frequency f, std::size_t count = n)
   {
      q::phase_iterator master{f, sps};
      std::vector<float> y(count);
      for (auto& s : y)
         s = v(master++);
      return y;
   }

   double mag(std::vector<float> const& x, double f)
   {
      double re = 0, im = 0;
      for (std::size_t i = 0; i != x.size(); ++i)
      {
         double a = 2.0 * q::pi * f * i / sps;
         re += x[i] * std::cos(a);
         im += x[i] * std::sin(a);
      }
      return 2.0 * std::hypot(re, im) / x.size();
   }

   double freq(std::vector<float> const& x)
   {
      int count = 0;
      for (std::size_t i = 1; i != x.size(); ++i)
         if (x[i-1] <= 0.0f && x[i] > 0.0f)
            ++count;
      return count * sps / x.size();
   }
}

TEST_CASE("A two operator note in ideal units")
{
   // Operator 2 at the note, 6 dB down, modulates operator 1 at 5 times
   // the note with an index of 1 radian per unit: J_k(0.5) sidebands at
   // 2200 + 440k
   q::basic_fm_voice<q::fm_operator, q::fm_operator> v{
      {.algorithm = {op<2> >> op<1>, 1.0f, 0.0f}}
    , sps};
   q::fm_note note;
   note.op[0] = {5.0, {}, instant};
   note.op[1] = {1.0, {}, {.rate = {quick, quick, quick, quick}
      , .level = {q::dB(-6.0206), q::dB(-6.0206), q::dB(-6.0206), silent}}};
   v.attack(note);
   CHECK(v.active());

   auto y = render(v, 440_Hz);
   CHECK(mag(y, 2200) == Approx(0.9385).margin(0.01));   // J0(0.5)
   CHECK(mag(y, 2640) == Approx(0.2423).margin(0.01));   // J1(0.5)
   CHECK(mag(y, 3080) == Approx(0.0306).margin(0.01));   // J2(0.5)

   v.release();
   render(v, 440_Hz, 100);
   CHECK(!v.active());
}

TEST_CASE("Vibrato bends the ratio operators and not a fixed one")
{
   q::basic_fm_voice<q::fm_operator, q::fm_operator> v{
      {.algorithm = {q::fm_routing{2}, 1.0f, 0.0f}
      , .lfo = {.rate = 6.0f}, .pitch_mod = 12.0f}, sps};
   q::fm_note note;
   note.op[0] = {1.0, {}, instant};
   note.op[1] = {0.0, q::phase{100_Hz, sps}, instant};
   v.attack(note);
   auto y = render(v, 440_Hz);

   // A full octave of vibrato leaves the 440 Hz bin; the fixed 100 Hz
   // operator is untouched
   CHECK(mag(y, 440) < 0.5);
   CHECK(mag(y, 100) == Approx(1.0).margin(0.01));
}

TEST_CASE("The pitch envelope ramps in semitones")
{
   q::basic_fm_voice<q::fm_operator> v{
      {.algorithm = {q::fm_routing{1}, 1.0f, 0.0f}
      , .pitch_env_level = {12, 12, 12, 0}
      , .pitch_env_rate = {1e4f, 1e4f, 1e4f, 1e4f}}, sps};
   q::fm_note note;
   // A level at L4, so the note holds after release
   note.op[0] = {1.0, {}, {.rate = {quick, quick, quick, quick}
      , .level = {q::dB(0), q::dB(0), q::dB(0), q::dB(0)}}};
   v.attack(note);
   CHECK(freq(render(v, 440_Hz)) == Approx(880).margin(2));
   v.release();
   CHECK(freq(render(v, 440_Hz)) == Approx(440).margin(2));
}

TEST_CASE("Tremolo cuts an operator by its depth at the LFO's trough")
{
   q::fm_voice_config cfg{
      .algorithm = {q::fm_routing{1}, 1.0f, 0.0f}, .lfo = {.rate = 6.0f}};
   cfg.amp_mod[0] = 40.0f;                 // dB
   q::basic_fm_voice<q::fm_operator> v{cfg, sps};
   q::fm_note note;
   note.op[0] = {1.0, {}, instant};
   v.attack(note);
   auto y = render(v, 440_Hz);
   float lo = 1.0f, hi = 0.0f;
   for (std::size_t i = 100; i < y.size(); i += 100)
   {
      float peak = 0.0f;
      for (std::size_t k = i - 100; k != i; ++k)
         peak = std::max(peak, std::abs(y[k]));
      lo = std::min(lo, peak);
      hi = std::max(hi, peak);
   }
   CHECK(hi > 0.95f);
   CHECK(lo < 0.02f);                      // -40 dB
}

TEST_CASE("A voice of other oscillators and envelopes")
{
   // Two band-limited saws an octave apart, ADSR envelopes
   using op = q::basic_fm_operator<q::saw_osc, q::adsr_envelope_gen>;
   q::basic_fm_voice<op, op> v{
      {.algorithm = {q::fm_routing{2}, 1.0f, 0.0f}}, sps};
   q::adsr_envelope_gen::config env{.attack_rate = 5_ms, .decay_rate = 5_ms
      , .sustain_level = 0_dB, .release_rate = 10_ms};
   decltype(v)::note note;
   note.op[0] = {1.0, {}, env};
   note.op[1] = {2.0, {}, env};
   v.attack(note);
   auto y = render(v, 440_Hz);

   // A saw's harmonics fall as 1/k: 880 has the octave's fundamental
   // on top of the note's second harmonic
   auto h1 = mag(y, 440);
   CHECK(h1 > 0.5);
   CHECK(mag(y, 1320) == Approx(h1 / 3).epsilon(0.1));
   CHECK(mag(y, 880) > h1);
   v.release();
   render(v, 440_Hz, 4800);
   CHECK(!v.active());
}

TEST_CASE("Any number of operators, of any oscillator, compiles and runs")
{
   using dx = q::dx_envelope_gen;
   q::basic_fm_voice<
      q::basic_fm_operator<q::sin_osc, dx>
    , q::basic_fm_operator<q::basic_saw_osc, dx>
    , q::basic_fm_operator<q::saw_osc, dx>
    , q::basic_fm_operator<q::basic_square_osc, dx>
    , q::basic_fm_operator<q::square_osc, dx>
    , q::basic_fm_operator<q::basic_triangle_osc, dx>
    , q::basic_fm_operator<q::triangle_osc, dx>
    , q::basic_fm_operator<q::sin_osc, dx>
   > v{{.algorithm = {q::fm_routing{8}, 1.0f, 0.0f}}, sps};  // all sound
   static_assert(decltype(v)::size == 8);
   q::fm_note note;
   for (std::size_t i = 0; i != 8; ++i)
      note.op[i] = {double(i + 1), {}, instant};
   v.attack(note);
   for (auto y : render(v, 110_Hz, 4800))
      REQUIRE(std::isfinite(y));
   v.release();
   render(v, 110_Hz, 100);
   CHECK(!v.active());
}

TEST_CASE("One operator is a sine")
{
   q::basic_fm_voice<q::fm_operator> v{
      {.algorithm = {q::fm_routing{1}, 1.0f, 0.0f}}, sps};
   q::fm_note note;
   note.op[0] = {1.0, {}, instant};
   v.attack(note);
   auto y = render(v, 440_Hz);
   CHECK(mag(y, 440) == Approx(1.0).margin(0.01));
}
