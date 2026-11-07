/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/pitch/detail/bacf_provisional.hpp>

#include <cstdint>
#include <vector>

namespace q = cycfi::q;
using q::detail::bacf_slice;
using q::detail::bacf_provisional;
using q::detail::bacf_pulse;

namespace
{
   constexpr std::uint32_t ws = q::detail::bacf_bitstream::word_size;
   constexpr float long_hint = 624.0f;          // 13 ms at 48 kHz
   using pulses = std::vector<bacf_pulse>;      // newest first

   bacf_slice dropped(std::uint32_t at, float hint, pulses const& seen)
   {
      bacf_slice slice{8 * ws, 96};
      slice.drop(at, hint);
      for (auto i = seen.size(); i-- != 0;)
         slice.observe(seen[i]);
      return slice;
   }

   auto const repeats = [](std::uint32_t) { return 0.95f; };
   auto const does_not = [](std::uint32_t) { return 0.7f; };
}

TEST_CASE("bacf_provisional: the first strong pair after the drop")
{
   // The latest pulse, if strong (60% of the peak since the drop), to the
   // nearest like pulse after the drop.
   pulses const p{
      {1500, 1.0f, 40.0f}
    , {1300, 0.95f, 41.0f}     // 200, the nearest alike
    , {1100, 1.0f, 40.0f}
    , {900, 1.0f, 40.0f}       // before the drop
   };
   auto const slice = dropped(1000, 0.0f, p);
   bacf_provisional prov{96, 2000, long_hint};
   prov.estimate(p, slice);
   REQUIRE(prov.estimated());
   CHECK(prov.value() == 0.0f);      // until vetted, at the sample's end
   prov.vet(slice, 1501, repeats);
   CHECK(prov.value() == 200.0f);
   CHECK(!prov.pending());
}

TEST_CASE("bacf_provisional: only pulses after the drop pair")
{
   pulses const p{{1500, 1.0f, 40.0f}, {1300, 1.0f, 40.0f}};
   bacf_provisional prov{96, 2000, long_hint};
   prov.estimate(p, dropped(1400, 0.0f, p));
   CHECK(!prov.estimated());
   prov.estimate(p, dropped(1600, 0.0f, {}));
   CHECK(!prov.estimated());
}

TEST_CASE("bacf_provisional: a weak pulse gives none")
{
   // The latest pulse under 60% of the peak since the drop: none yet.
   pulses const p{
      {1500, 0.5f, 40.0f}, {1300, 0.5f, 40.0f}, {1100, 1.0f, 40.0f}};
   bacf_provisional prov{96, 2000, long_hint};
   prov.estimate(p, dropped(1000, 0.0f, p));
   CHECK(!prov.estimated());
}

TEST_CASE("bacf_provisional: lags out of range")
{
   // Under the shortest lag a pulse is passed over; past the longest the
   // search stops.
   pulses const p{
      {5000, 1.0f, 40.0f}, {4950, 1.0f, 40.0f}, {4800, 1.0f, 40.0f}};
   bacf_provisional prov{96, 2000, long_hint};
   prov.estimate(p, dropped(4000, 0.0f, p));
   prov.vet(dropped(4000, 0.0f, p), 5001, repeats);
   CHECK(prov.value() == 200.0f);

   bacf_provisional shorter{96, 150, long_hint};
   shorter.estimate(p, dropped(4000, 0.0f, p));
   CHECK(!shorter.estimated());
}

TEST_CASE("bacf_provisional: after a long hint, one that agrees stands")
{
   // After a chord (a hint over 13 ms) an estimate stands at once if the
   // hint is it, or 2 to 4 of it, within 2%.
   pulses const p{{1500, 1.0f, 40.0f}, {1250, 1.0f, 40.0f}};
   for (float k : {1.0f, 2.0f, 3.0f, 4.0f})
   {
      if (250.0f * k <= long_hint)
         continue;
      auto const slice = dropped(1000, 250.0f * k * 1.015f, p);
      bacf_provisional prov{96, 2000, long_hint};
      prov.estimate(p, slice);
      prov.vet(slice, 1501, does_not);
      CHECK(prov.value() == 250.0f);
   }
}

TEST_CASE("bacf_provisional: after a long hint, others must repeat")
{
   // Else it waits until one lag of the slice can be scored, and stands
   // only if it repeats over that lag (85%).
   pulses const p{{1500, 1.0f, 40.0f}, {1250, 1.0f, 40.0f}};
   auto const slice = dropped(1000, 816.0f, p);   // 3.26 times 250
   bacf_provisional prov{96, 2000, long_hint};
   prov.estimate(p, slice);
   auto const ready = 1000 + slice.one_lag(250) + 250 + ws;

   prov.vet(slice, ready - 1, repeats);
   CHECK(prov.pending());
   CHECK(prov.value() == 0.0f);

   prov.vet(slice, ready, does_not);
   CHECK(prov.pending());
   CHECK(prov.value() == 0.0f);

   prov.vet(slice, ready + 1, repeats);
   CHECK(!prov.pending());
   CHECK(prov.value() == 250.0f);
}

TEST_CASE("bacf_provisional: drop and reset")
{
   pulses const p{{1500, 1.0f, 40.0f}, {1300, 1.0f, 40.0f}};
   auto const slice = dropped(1000, 0.0f, p);
   bacf_provisional prov{96, 2000, long_hint};
   prov.estimate(p, slice);
   prov.vet(slice, 1501, repeats);
   REQUIRE(prov.value() == 200.0f);
   prov.drop();
   CHECK(!prov.estimated());
   CHECK(prov.value() == 0.0f);
   prov.estimate(p, slice);
   prov.reset();
   CHECK(!prov.estimated());
}
