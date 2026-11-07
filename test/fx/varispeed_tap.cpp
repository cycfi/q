/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>

#include <q/fx/varispeed_tap.hpp>

#include <cmath>
#include <numbers>
#include <vector>

namespace q = cycfi::q;
using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;

namespace
{
   constexpr double sps = 48000;

   float sine(double freq, std::size_t t)
   {
      return float(std::sin(2 * std::numbers::pi * freq * t / sps));
   }

   // The frequency of y over [from, to), from its rising zero crossings.
   double frequency(
      std::vector<float> const& y, std::size_t from, std::size_t to)
   {
      double first = -1, last = -1;
      int crossings = 0;
      for (auto t = from + 1; t != to; ++t)
      {
         if (y[t-1] < 0 && y[t] >= 0)
         {
            double at = t - 1 + y[t-1] / (y[t-1] - y[t]);
            if (first < 0)
               first = at;
            last = at;
            ++crossings;
         }
      }
      return (crossings - 1) * sps / (last - first);
   }
}

TEST_CASE("varispeed_tap: at delay 0 and ratio 1 it is the input",
   "[varispeed_tap]")
{
   q::varispeed_tap tap{1000};
   for (std::size_t t = 0; t != 2000; ++t)
   {
      auto s = sine(440, t);
      REQUIRE(tap(s, 1.0f) == s);
   }
   REQUIRE(tap.delay() == 0.0f);
}

TEST_CASE("varispeed_tap: ratio 1 keeps a set delay", "[varispeed_tap]")
{
   q::varispeed_tap tap{1000};
   tap.delay(100);
   std::vector<float> x(3000), y(3000);
   for (std::size_t t = 0; t != x.size(); ++t)
   {
      x[t] = sine(440, t);
      y[t] = tap(x[t], 1.0f);
   }
   for (std::size_t t = 200; t != x.size(); ++t)
      REQUIRE_THAT(y[t], WithinAbs(x[t - 100], 1e-6));
}

TEST_CASE("varispeed_tap: the pitch follows the ratio", "[varispeed_tap]")
{
   SECTION("down an octave, the delay growing from 0")
   {
      q::varispeed_tap tap{48000};
      std::vector<float> y(20000);
      for (std::size_t t = 0; t != y.size(); ++t)
         y[t] = tap(sine(440, t), 0.5f);
      REQUIRE_THAT(frequency(y, 2000, 20000), WithinRel(220.0, 1e-3));
      REQUIRE_THAT(tap.delay(), WithinAbs(10000.0, 1.0));
   }

   SECTION("up a fifth, the delay shrinking from a head start")
   {
      float ratio = float(std::pow(2.0, 7.0 / 12));
      q::varispeed_tap tap{48000};
      tap.delay(20000);
      std::vector<float> y(20000);
      for (std::size_t t = 0; t != y.size(); ++t)
         y[t] = tap(sine(440, t), ratio);
      auto f = frequency(y, 2000, 20000);
      REQUIRE_THAT(f, WithinRel(440.0 * ratio, 1e-3));
   }
}

TEST_CASE("varispeed_tap: held at an end, the pitch is the input's",
   "[varispeed_tap]")
{
   // Up an octave from delay 0: the read cannot get ahead of the input.
   q::varispeed_tap tap{1000};
   std::vector<float> y(10000);
   for (std::size_t t = 0; t != y.size(); ++t)
      y[t] = tap(sine(440, t), 2.0f);
   REQUIRE(tap.delay() == 0.0f);
   REQUIRE_THAT(frequency(y, 0, 10000), WithinRel(440.0, 1e-3));

   // Down, past max_delay: the delay stops at it.
   q::varispeed_tap down{1000};
   for (std::size_t t = 0; t != 5000; ++t)
      down(sine(440, t), 0.5f);
   REQUIRE(down.delay() == 1000.0f);
}

TEST_CASE("varispeed_tap: a dive and its return come back to the input",
   "[varispeed_tap]")
{
   // Down a whole tone for a while, then up by the inverse for as long:
   // the delay returns to 0, and the output is the input again.
   float down = float(std::pow(2.0, -2.0 / 12));
   q::varispeed_tap tap{48000};
   std::size_t const leg = 12000;
   for (std::size_t t = 0; t != leg; ++t)
      tap(sine(440, t), down);
   REQUIRE(tap.delay() > 1000);

   std::size_t t = leg;
   while (tap.delay() > 0)
      tap(sine(440, t), 2.0f - down), ++t;
   for (std::size_t i = 0; i != 1000; ++i, ++t)
   {
      auto s = sine(440, t);
      REQUIRE(tap(s, 1.0f) == s);
   }
}

TEST_CASE("varispeed_tap: operator[] reads at any delay", "[varispeed_tap]")
{
   q::varispeed_tap tap{1000};
   std::vector<float> x(500);
   for (std::size_t t = 0; t != x.size(); ++t)
   {
      x[t] = sine(440, t);
      tap(x[t], 1.0f);
   }
   REQUIRE(tap[0.0f] == x.back());
   REQUIRE_THAT(tap[50.0f], WithinAbs(x[x.size() - 1 - 50], 1e-6));
   REQUIRE(tap[std::size_t(50)] == x[x.size() - 1 - 50]);
   REQUIRE_THAT(tap[50.5f], WithinAbs(
      0.5f * (x[x.size() - 51] + x[x.size() - 52]), 1e-3));
}

TEST_CASE("varispeed_tap: a splice by a period is seamless",
   "[varispeed_tap]")
{
   // At ratio 1, a jump back by one period of a sine meets the same
   // waveform: the crossfade changes nothing, and the delay ends a
   // period deeper.
   double const period = sps / 440;
   q::varispeed_tap tap{2000};
   tap.delay(100);
   std::vector<float> x(4000), y(4000);
   for (std::size_t t = 0; t != x.size(); ++t)
   {
      x[t] = sine(440, t);
      if (t == 2000)
         tap.splice(float(100 + period), 64);
      y[t] = tap(x[t], 1.0f);
   }
   for (std::size_t t = 200; t != 2000; ++t)
      REQUIRE_THAT(y[t], WithinAbs(x[t - 100], 1e-4));
   for (std::size_t t = 2000; t != x.size(); ++t)
      REQUIRE_THAT(y[t], WithinAbs(sine(440, t - 100), 1e-4));
   REQUIRE(!tap.splicing());
   REQUIRE_THAT(tap.delay(), WithinAbs(100 + period, 1e-3));
}

TEST_CASE("varispeed_tap: splice targets move at the ratio",
   "[varispeed_tap]")
{
   // Up an octave, both reads lose a sample of delay a call: the target
   // has moved by the fade's length when the splice lands.
   q::varispeed_tap tap{2000};
   tap.delay(500);
   for (std::size_t t = 0; t != 10; ++t)
      tap(0.0f, 2.0f);
   REQUIRE_THAT(tap.delay(), WithinAbs(490, 1e-4));
   tap.splice(800, 20);
   for (std::size_t t = 0; t != 20; ++t)
   {
      REQUIRE(tap.splicing());
      tap(0.0f, 2.0f);
   }
   REQUIRE(!tap.splicing());
   REQUIRE_THAT(tap.delay(), WithinAbs(780, 1e-4));

   // A splice in flight is finished by the next one.
   tap.splice(1000, 50);
   for (std::size_t t = 0; t != 10; ++t)
      tap(0.0f, 2.0f);
   tap.splice(500, 5);
   REQUIRE_THAT(tap.delay(), WithinAbs(990, 1e-4));
   for (std::size_t t = 0; t != 5; ++t)
      tap(0.0f, 2.0f);
   REQUIRE_THAT(tap.delay(), WithinAbs(495, 1e-4));

   // Setting the delay lands a splice in flight at once.
   tap.splice(300, 50);
   REQUIRE(tap.target() == 300.0f);
   tap.delay(tap.target());
   REQUIRE(!tap.splicing());
   REQUIRE(tap.delay() == 300.0f);
}

TEST_CASE("varispeed_tap: reverse turns a splice back without a jump",
   "[varispeed_tap]")
{
   // On a ramp at ratio 1 the output tells the share of each read: at
   // delay 100 and 400, y = t - 100 - 300 b, b the far read's share. A
   // splice from 100 to 400 turned back after 20 of its 64 reads runs that
   // share back down from where it was (the turn holds it one read), and
   // lands at 100 after 20 more.
   q::varispeed_tap tap{2000};
   tap.delay(100);
   auto share = [](std::size_t t, float y) { return (float(t) - 100 - y) / 300; };
   float last = 0;
   for (std::size_t t = 0; t != 1000; ++t)
      tap(float(t), 1.0f);
   tap.splice(400, 64);
   for (std::size_t t = 1000; t != 1020; ++t)
      last = share(t, tap(float(t), 1.0f));
   REQUIRE(last > 0.1f);
   tap.reverse();
   REQUIRE(tap.splicing());
   REQUIRE(tap.target() == 100.0f);
   for (std::size_t t = 1020; t != 1040; ++t)
   {
      REQUIRE(tap.splicing());
      auto const b = share(t, tap(float(t), 1.0f));
      REQUIRE(b < last + 0.005f);
      REQUIRE(last - b < 0.06f);
      last = b;
   }
   REQUIRE(!tap.splicing());
   REQUIRE(tap.delay() == 100.0f);
   REQUIRE_THAT(tap(1040.0f, 1.0f), WithinAbs(940.0f, 1e-3));

   // Turned back before any of it is heard, the splice is called off;
   // with none in flight, reverse does nothing.
   tap.splice(400, 64);
   tap.reverse();
   REQUIRE(!tap.splicing());
   REQUIRE(tap.delay() == 100.0f);
   tap.reverse();
   REQUIRE(!tap.splicing());
   REQUIRE(tap.delay() == 100.0f);
}

TEMPLATE_TEST_CASE("basic_varispeed_tap: any interpolation policy",
   "[varispeed_tap]"
 , q::sample_interpolation::linear, q::sample_interpolation::hermite)
{
   // Down an octave from delay 0, then a splice by a period at ratio 1:
   // the pitch and the seam hold whatever the reads interpolate with.
   q::basic_varispeed_tap<TestType> tap{48000};
   std::vector<float> y(20000);
   for (std::size_t t = 0; t != y.size(); ++t)
      y[t] = tap(sine(440, t), 0.5f);
   REQUIRE_THAT(frequency(y, 2000, 20000), WithinRel(220.0, 1e-3));
   REQUIRE_THAT(tap.delay(), WithinAbs(10000.0, 1.0));

   double const period = sps / 440;
   tap.delay(100);
   std::vector<float> x(4000);
   for (std::size_t t = 0; t != x.size(); ++t)
   {
      x[t] = sine(440, t);
      if (t == 2000)
         tap.splice(float(100 + period), 64);
      y[t] = tap(x[t], 1.0f);
   }
   for (std::size_t t = 2000; t != x.size(); ++t)
      REQUIRE_THAT(y[t], WithinAbs(sine(440, t - 100), 2e-3));
}
