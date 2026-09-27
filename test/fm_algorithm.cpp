/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/support/literals.hpp>
#include <q/synth/fm/fm_algorithm.hpp>
#include <q/synth/fm/fm_operator.hpp>
#include <cmath>
#include <tuple>
#include <vector>

namespace q = cycfi::q;
namespace ops = q::fm_ops;
using eg = q::dx_envelope_gen;
using namespace q::literals;
using routing = q::fm_routing;

namespace
{
   constexpr float sps = 48000.0f;
   constexpr std::size_t n = 48000;
   constexpr auto at_once = q::duration{1e-6};

   constexpr routing six_carriers =
      (ops::op<1> | ops::op<2> | ops::op<3>
       | ops::op<4> | ops::op<5> | ops::op<6>).feedback(ops::op<6>);
   constexpr routing two_into_one = routing{6}(ops::op<2> >> ops::op<1>);

   // Six operators at 100 Hz times 1..6, so each one has its own
   // frequency in the output; instant envelopes at full level.
   struct rig
   {
      rig(routing r, float index = 1.0f, float feedback = 0.0f)
       : alg{{r, index, feedback}}
      {
         std::apply([&](auto&... o)
         {
            double ratio = 1.0;
            ((o.ratio(ratio++), o.envelope(instant, sps), o.attack()), ...);
         }, op);
      }

      void sync()
      {
         std::apply([](auto&... o) { (o.sync(), ...); }, op);
         alg.sync();
      }

      std::vector<float> render()
      {
         q::phase_iterator master{100_Hz, sps};
         std::vector<float> y(n);
         for (auto& v : y)
            v = alg(op, master++);
         return y;
      }

      // At once at full level, within a sample
      static constexpr eg::config instant = {
         .rate = {at_once, at_once, at_once, at_once}
       , .level = {q::dB(0), q::dB(0), q::dB(0), eg::floor}};

      q::fm_algorithm alg;
      std::tuple<q::fm_operator, q::fm_operator, q::fm_operator
       , q::fm_operator, q::fm_operator, q::fm_operator> op;
   };

   // Magnitude at f Hz, normalized so a unit sine reads 1.
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
}

TEST_CASE("Carriers are the operators that modulate nobody")
{
   rig six{six_carriers};
   CHECK(six.alg.num_carriers() == 6);

   // Two of the six in a stack: the other four still sound
   rig r{two_into_one};
   CHECK(r.alg.num_carriers() == 5);
   CHECK(r.alg.is_carrier(0));
   CHECK(!r.alg.is_carrier(1));

   // The same stack as a voice of its own two
   q::fm_algorithm two{{ops::op<2> >> ops::op<1>, 1.0f, 0.0f}};
   CHECK(two.num_carriers() == 1);
   CHECK(two.is_carrier(0));
   CHECK(!two.is_carrier(1));
}

TEST_CASE("Six carriers sum")
{
   rig r{six_carriers};
   auto y = r.render();
   for (int k = 1; k <= 6; ++k)
      CHECK(mag(y, 100.0 * k) == Approx(1.0).margin(0.01));
}

TEST_CASE("A modulator offsets its carrier's phase by the index")
{
   // Operator 2 at 200 Hz modulates operator 1 at 1000 Hz by 2.235
   // radians: sidebands at 1000 + 200k with the Bessel amplitudes
   // J_k(2.235), none folding below 0 Hz.
   rig r{two_into_one, 2.235f};
   std::get<0>(r.op).ratio(10.0);
   std::apply([](auto&, auto&, auto&... rest) { (rest.gain(0.0f), ...); }
      , r.op);

   auto y = r.render();
   CHECK(mag(y, 1000) == Approx(0.0912).margin(0.01));   // J0
   CHECK(mag(y, 1200) == Approx(0.5508).margin(0.01));   // J1
   CHECK(mag(y, 800) == Approx(0.5508).margin(0.01));
   CHECK(mag(y, 1400) == Approx(0.4018).margin(0.01));   // J2
   CHECK(mag(y, 600) == Approx(0.4018).margin(0.01));
   CHECK(mag(y, 1600) == Approx(0.1684).margin(0.01));   // J3
   CHECK(mag(y, 400) == Approx(0.1684).margin(0.01));
}

TEST_CASE("A silent modulator leaves a pure carrier")
{
   rig r{two_into_one, 2.235f};
   std::get<1>(r.op).gain(0.0f);        // the modulator
   std::apply([](auto&, auto&, auto&... rest) { (rest.gain(0.0f), ...); }
      , r.op);                          // and the other four carriers
   auto y = r.render();
   CHECK(mag(y, 100) == Approx(1.0).margin(0.01));
   CHECK(mag(y, 300) < 0.01);
}

TEST_CASE("Feedback adds harmonics to its source, and sync clears it")
{
   // Operator 6 (600 Hz) into itself: a 2nd harmonic at 1200 Hz
   rig off{six_carriers, 1.0f, 0.0f};
   CHECK(mag(off.render(), 1200) < 0.01);

   rig on{six_carriers, 1.0f, float(q::pi / 2)};
   auto y = on.render();
   CHECK(mag(y, 1200) > 0.1);

   on.sync();
   auto again = on.render();
   for (std::size_t i = 0; i != n; ++i)
      REQUIRE(again[i] == y[i]);
}

TEST_CASE("The feedback harmonic grows with its radians")
{
   // The 2nd harmonic of a self-modulated sine grows with the index up
   // to about 1.8 radians
   auto level = [](float radians)
   {
      rig r{six_carriers, 1.0f, radians};
      return mag(r.render(), 1200);
   };
   CHECK(level(0.0f) < 1e-6);
   CHECK(level(0.2f) > 1e-3);
   CHECK(level(0.2f) < level(0.4f));
   CHECK(level(0.4f) < level(0.8f));
   CHECK(level(0.8f) < level(1.57f));
}

TEST_CASE("Feedback can loop across operators")
{
   // Operator 4 into operator 6, which modulates 5, which modulates 4
   constexpr routing loop =
      (ops::op<6> >> ops::op<5> >> ops::op<4>).feedback(ops::op<4>, ops::op<6>);
   rig off{loop, 1.0f, 0.0f};
   rig on{loop, 1.0f, 1.0f};
   auto y0 = off.render();
   auto y1 = on.render();
   double diff = 0;
   for (std::size_t i = 0; i != n; ++i)
      diff += std::abs(y0[i] - y1[i]);
   CHECK(diff / n > 0.01);
}

TEST_CASE("Fewer operators: the tuple's size is the count")
{
   std::tuple<q::fm_operator, q::fm_operator> op;
   std::get<0>(op).ratio(10.0);
   std::get<1>(op).ratio(2.0);
   std::apply([](auto&... o)
   {
      ((o.envelope(rig::instant, sps), o.attack()), ...);
   }, op);
   q::fm_algorithm alg{{ops::op<2> >> ops::op<1>, 2.235f, 0.0f}};
   q::phase_iterator master{100_Hz, sps};
   std::vector<float> y(n);
   for (auto& v : y)
      v = alg(op, master++);
   CHECK(mag(y, 1200) == Approx(0.5508).margin(0.01));   // J1(2.235)
}

TEST_CASE("A deviation in radians wraps past a cycle either way")
{
   using alg = q::fm_algorithm;
   CHECK(alg::deviation(0.0f).rep == 0);
   CHECK(alg::deviation(float(q::pi)).rep == 0x80000000u);   // half
   CHECK(alg::deviation(float(-q::pi)).rep == 0x80000000u);
   CHECK(alg::deviation(float(2 * q::pi)).rep < 100);          // wraps
   CHECK(alg::deviation(float(9 * q::pi)).rep
      == alg::deviation(float(q::pi)).rep);
}
