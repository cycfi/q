/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/pitch/detail/bacf_lock.hpp>

#include <cmath>
#include <cstdint>
#include <functional>
#include <vector>

namespace q = cycfi::q;
using q::detail::bacf_lock;
using q::detail::bacf_pulse;
using Catch::Matchers::WithinAbs;

namespace
{
   constexpr std::uint32_t window = 512;
   constexpr std::uint32_t limit = 77;          // 15% of the window

   // Scripted scores: mismatches at each lag, and the lags that fit.
   struct scorer
   {
      std::function<std::uint32_t(std::uint32_t)> mismatches;
      std::uint32_t lo = 96, hi = 2000;

      bool fits(std::uint32_t lag) const { return lag >= lo && lag <= hi; }
      std::uint32_t operator()(std::uint32_t lag) const
      {
         return mismatches(lag);
      }
   };

   // A well of mismatches around `at`: 0 there, 10 per sample away.
   std::function<std::uint32_t(std::uint32_t)> well(double at)
   {
      return [=](std::uint32_t l)
      {
         return std::uint32_t(std::lround(10 * std::abs(l - at)));
      };
   }

   // Newest first, as the rings give them.
   using pulses = std::vector<bacf_pulse>;
   using edges = std::vector<std::uint32_t>;

   bacf_lock locked_at(std::uint32_t lag)
   {
      bacf_lock lock{96, 2000, window, limit};
      pulses const p{{1000 + lag, 1.0f, 50.0f}, {1000, 1.0f, 50.0f}};
      lock.acquire(p, scorer{well(lag)});
      REQUIRE(lock.locked());
      REQUIRE(lock.lag() == float(lag));
      return lock;
   }
}

TEST_CASE("bacf_lock: acquire on the best of the like pulses before")
{
   // The latest pulse against the like pulses before it, the latest
   // `partners` of them; the best score under the limit locks.
   bacf_lock lock{96, 2000, window, limit};
   pulses const p{
      {1000, 1.0f, 50.0f}
    , {900, 1.0f, 50.0f}       // 100: scores 80, over the limit
    , {800, 0.3f, 50.0f}       // not alike: passed over
    , {700, 1.0f, 50.0f}       // 300: 20
    , {600, 1.0f, 52.0f}       // 400: 10, the best of the three tried
    , {500, 1.0f, 50.0f}       // 500: 0, but past the partners
   };
   std::vector<std::uint32_t> tried;
   scorer s{[&](std::uint32_t l)
   {
      tried.push_back(l);
      return l == 100 ? 80u : l == 300 ? 20u : l == 400 ? 10u : 0u;
   }};
   lock.acquire(p, s);
   CHECK(tried == std::vector<std::uint32_t>{100, 300, 400});
   REQUIRE(lock.locked());
   CHECK(lock.lag() == 400.0f);
   CHECK_THAT(lock.periodicity(), WithinAbs(1.0 - 10.0 / window, 1e-6));
}

TEST_CASE("bacf_lock: acquire needs a score under the limit")
{
   bacf_lock lock{96, 2000, window, limit};
   pulses const p{{1000, 1.0f, 50.0f}, {800, 1.0f, 50.0f}};
   lock.acquire(p, scorer{[](std::uint32_t) { return limit; }});
   CHECK(!lock.locked());
   CHECK(lock.lag() == 0.0f);
}

TEST_CASE("bacf_lock: acquire passes over lags out of range")
{
   // Under the shortest lag a pulse is passed over; past the longest, or
   // the first that cannot be scored, the search stops.
   bacf_lock lock{96, 2000, window, limit};
   std::vector<std::uint32_t> tried;
   scorer s{[&](std::uint32_t l) { tried.push_back(l); return 0u; }};
   s.hi = 350;
   pulses const p{
      {5000, 1.0f, 50.0f}
    , {4950, 1.0f, 50.0f}      // 50: under 96
    , {4800, 1.0f, 50.0f}      // 200
    , {4600, 1.0f, 50.0f}      // 400: does not fit, stop
    , {4500, 1.0f, 50.0f}
   };
   lock.acquire(p, s);
   CHECK(tried == std::vector<std::uint32_t>{200});
   CHECK(lock.lag() == 200.0f);
}

TEST_CASE("bacf_lock: track nudges to an edge a lag back and refines")
{
   // Locked at 200, an edge 203 samples back (within 3%) moves the lag
   // there; the sweep finds the best and a parabola its fraction.
   auto lock = locked_at(200);
   edges const e{10000 - 150, 10000 - 203, 10000 - 400};
   scorer s{[](std::uint32_t l)
   {
      auto const d = l - 203.3;
      return std::uint32_t(std::lround(10 * d * d));
   }};
   lock.track(e, 10000, s);
   REQUIRE(lock.locked());
   CHECK_THAT(lock.lag(), WithinAbs(203.3, 0.01));
}

TEST_CASE("bacf_lock: track keeps the lag with no edge near it")
{
   auto lock = locked_at(200);
   edges const e{10000 - 150, 10000 - 260};
   lock.track(e, 10000, scorer{well(200)});
   CHECK(lock.lag() == 200.0f);
}

TEST_CASE("bacf_lock: on a miss, twice the lag is tried")
{
   // A chord's common period is a multiple of each note's.
   auto lock = locked_at(200);
   scorer s{[](std::uint32_t l) { return l == 401 ? 5u : 200u; }};
   lock.track(edges{}, 10000, s);
   REQUIRE(lock.locked());
   CHECK(lock.lag() == 401.0f);
}

TEST_CASE("bacf_lock: two misses drop the lock, the last lag held")
{
   auto lock = locked_at(200);
   scorer const bad{[](std::uint32_t) { return limit + 10; }};
   lock.track(edges{}, 10000, bad);
   CHECK(lock.locked());
   lock.track(edges{}, 10100, bad);
   CHECK(!lock.locked());
   CHECK(lock.lag() == 200.0f);
   auto const missed = 1.0 - (limit + 10.0) / window;
   CHECK_THAT(lock.periodicity(), WithinAbs(missed, 1e-6));
}

TEST_CASE("bacf_lock: a lag that cannot be scored is a miss")
{
   // A lag drifted out of range must not hold the lock for good.
   auto lock = locked_at(200);
   scorer s{well(200)};
   s.hi = 150;
   lock.track(edges{}, 10000, s);
   CHECK(lock.locked());
   lock.track(edges{}, 10100, s);
   CHECK(!lock.locked());
   CHECK(lock.lag() == 200.0f);
}

TEST_CASE("bacf_lock: a hit clears the misses")
{
   auto lock = locked_at(200);
   scorer const bad{[](std::uint32_t) { return limit + 10; }};
   lock.track(edges{}, 10000, bad);
   lock.track(edges{}, 10100, scorer{well(200)});
   lock.track(edges{}, 10200, bad);
   CHECK(lock.locked());
}

TEST_CASE("bacf_lock: drop and reset")
{
   auto lock = locked_at(200);
   lock.drop();
   CHECK(!lock.locked());
   CHECK(lock.lag() == 200.0f);
   lock.reset();
   CHECK(!lock.locked());
   CHECK(lock.lag() == 0.0f);
   CHECK(lock.periodicity() == 1.0f);
}
