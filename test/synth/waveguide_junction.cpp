/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>

#include <q/support/literals.hpp>
#include <q/synth/waveguide.hpp>
#include <q/synth/waveguide_junction.hpp>

#include <array>
#include <cmath>
#include <random>
#include <vector>

namespace q = cycfi::q;
using namespace q::literals;

namespace
{
   constexpr float sps = 48000;

   double energy(std::vector<float> const& x, std::size_t a, std::size_t n)
   {
      double e = 0;
      for (std::size_t i = a; i != a + n; ++i)
         e += double(x[i]) * x[i];
      return e / n;
   }
}

TEST_CASE("A junction only moves energy between its ports")
{
   std::mt19937 gen{1};
   std::uniform_real_distribution<float> u{-1, 1};
   q::waveguide_junction j{4};
   std::array<float, 4> in, out;
   // Velocity waves: the power a port carries in is r times its wave
   // squared, so sum r (in^2 - out^2) is zero.
   std::array<float, 4> rs{0.5f, 2.0f, 7.0f, 1.0f};
   for (std::size_t i = 0; i != 4; ++i)
      j.impedance(i, rs[i]);
   for (int trial = 0; trial != 100; ++trial)
   {
      for (auto& v : in)
         v = u(gen);
      j(in, out);
      double balance = 0, scale = 0;
      for (std::size_t i = 0; i != 4; ++i)
      {
         balance += rs[i] * (in[i] * in[i] - out[i] * out[i]);
         scale += rs[i] * in[i] * in[i];
      }
      REQUIRE(std::abs(balance) < 1e-5 * (1 + scale));
   }
}

TEST_CASE("Equal ports pass a wave straight through")
{
   q::waveguide_junction j{2};
   std::array<float, 2> in{0.7f, 0.0f}, out;
   j(in, out);
   CHECK(out[0] == Approx(0.0f).margin(1e-7));
   CHECK(out[1] == Approx(0.7f));
}

TEST_CASE("A stiff port makes the others reflect inverted")
{
   q::waveguide_junction j{2};
   j.impedance(1, 1e6f);
   std::array<float, 2> in{1.0f, 0.0f}, out;
   j(in, out);
   CHECK(out[0] == Approx(-1.0f).margin(1e-5));
}

TEST_CASE("Joined to a rigid port, a waveguide rings as if it ended there")
{
   q::waveguide alone{110_Hz, sps}, joined{110_Hz, sps};
   for (auto* w : {&alone, &joined})
      w->pluck(0.2f);
   q::waveguide_junction j{2};
   j.impedance(1, 1e7f);
   std::array<float, 2> in{}, out{};
   for (int i = 0; i != 20000; ++i)
   {
      auto const e = i < 400? std::sin(i * 0.05f) : 0.0f;
      in[0] = joined.incoming();
      j(in, out);
      auto const y = joined(e, out[0]);
      auto const z = alone(e);
      REQUIRE(y == Approx(z).margin(1e-4));
   }
}

TEST_CASE("A body drains a string, and moves a string tuned with it")
{
   // Two strings at 196 Hz and a lighter body, all at one junction. Only
   // string A is plucked. Joined, A decays faster than alone, and B,
   // never plucked, rings; detuned a semitone, B rings far less.
   auto run = [](float b_hz, std::vector<float>& a_out,
                 std::vector<float>& b_out)
   {
      q::waveguide a{82_Hz, sps, 4_s}, b{82_Hz, sps, 4_s};
      q::waveguide body{98_Hz, sps, 100_ms};
      a.frequency(196_Hz);
      b.frequency(q::frequency{b_hz});
      a.pluck(0.2f);
      q::waveguide_junction j{3};
      j.impedance(2, 30.0f);
      std::array<float, 3> in{}, out{};
      std::size_t const n = std::size_t(sps);
      a_out.resize(n);
      b_out.resize(n);
      for (std::size_t i = 0; i != n; ++i)
      {
         auto const e = i < 240? 1.0f - std::abs(1.0f - i / 120.0f) : 0.0f;
         in = {a.incoming(), b.incoming(), body.incoming()};
         j(in, out);
         a_out[i] = a(e, out[0]);
         b_out[i] = b(0.0f, out[1]);
         body(0.0f, out[2]);
      }
   };
   std::vector<float> a1, b1, a2, b2;
   run(196.0f, a1, b1);
   run(207.65f, a2, b2);

   q::waveguide solo{82_Hz, sps, 4_s};
   solo.frequency(196_Hz);
   solo.pluck(0.2f);
   std::vector<float> s(static_cast<std::size_t>(sps));
   for (std::size_t i = 0; i != s.size(); ++i)
      s[i] = solo(i < 240? 1.0f - std::abs(1.0f - i / 120.0f) : 0.0f);

   auto const tail = std::size_t(0.8 * sps), w = std::size_t(0.1 * sps);
   auto const head = std::size_t(0.05 * sps);
   auto const drop_joined = energy(a1, head, w) / energy(a1, tail, w);
   auto const drop_alone = energy(s, head, w) / energy(s, tail, w);
   INFO("A's drop, joined " << drop_joined << ", alone " << drop_alone);
   CHECK(drop_joined > 1.5 * drop_alone);

   auto const tuned = energy(b1, tail - w, w) / energy(a1, tail - w, w);
   auto const detuned = energy(b2, tail - w, w) / energy(a2, tail - w, w);
   INFO("B over A, tuned " << tuned << ", detuned " << detuned);
   CHECK(tuned > 0.01);
   CHECK(tuned > 10 * detuned);
}
