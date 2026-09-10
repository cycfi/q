/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
// How a parameter maps its own units onto a control's travel.
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <qplug/parameter.hpp>

namespace qplug = cycfi::qplug;
using qplug::parameter;
using namespace cycfi::q::literals;

namespace
{
   parameter attack()
   {
      return parameter{1, "Attack", 20_ms}
         .range((1_ms).rep, (1_s).rep).log();
   }
}

TEST_CASE("A taper maps the ends of the range onto the ends of the travel")
{
   auto const p = attack();
   CHECK(p.value(0.0) == Approx(0.001));
   CHECK(p.value(1.0) == Approx(1.0));
   CHECK(p.position(0.001) == Approx(0.0));
   CHECK(p.position(1.0) == Approx(1.0));
}

TEST_CASE("A logarithmic taper puts the geometric mean at the middle")
{
   // Equal ratios per unit of travel: half way from 1 ms to 1 s is
   // sqrt(1 ms * 1 s), 31.6 ms, not half a second.
   auto const p = attack();
   CHECK(p.value(0.5) == Approx(0.0316228).epsilon(0.001));

   // And a quarter of the way is the same ratio again.
   CHECK(p.value(0.75) / p.value(0.5)
      == Approx(p.value(0.5) / p.value(0.25)).epsilon(0.001));
}

TEST_CASE("A value survives the round trip through the travel")
{
   auto const p = attack();
   for (double pos = 0.0; pos <= 1.0; pos += 0.05)
      CHECK(p.position(p.value(pos)) == Approx(pos).margin(1e-9));
}

TEST_CASE("A linear taper is the plain proportion")
{
   auto const p = parameter{2, "Sustain", -12_dB}.range(-60.0, 0.0);
   CHECK(p.value(0.0) == Approx(-60.0));
   CHECK(p.value(0.5) == Approx(-30.0));
   CHECK(p.value(1.0) == Approx(0.0));
}

TEST_CASE("Decibels are already a ratio, so they are not tapered again")
{
   // Asking for a log taper on a range that crosses zero cannot work: the
   // taper needs a ratio between the ends. It falls back rather than
   // producing a NaN.
   auto const p = parameter{3, "Level", -12_dB}.range(-60.0, 0.0).log();
   CHECK(!p.logarithmic());
   CHECK(p.value(0.5) == Approx(-30.0));
}

TEST_CASE("The curve still bunches values toward the low end")
{
   // The older taper, an exponent on the linear range.
   auto const p = parameter{4, "Time", 0.5}.range(0.0, 1.0).curve(2.0);
   CHECK(p.value(0.5) == Approx(0.25));
   CHECK(p.position(0.25) == Approx(0.5));
}

TEST_CASE("A value outside the range clamps to the travel")
{
   auto const p = attack();
   CHECK(p.position(0.0) == Approx(0.0));
   CHECK(p.position(100.0) == Approx(1.0));
   CHECK(p.value(-1.0) == Approx(0.001));
   CHECK(p.value(2.0) == Approx(1.0));
}
