/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/pitch/detail/bacf_slice.hpp>

#include <cstdint>

namespace q = cycfi::q;
using q::detail::bacf_slice;
using q::detail::bacf_pulse;

namespace
{
   constexpr std::uint32_t ws = q::detail::bacf_bitstream::word_size;
   constexpr std::uint32_t window = 8 * ws;
   constexpr std::uint32_t min_window = 96;

   std::uint32_t words(std::uint32_t n)
   {
      return (n + ws - 1) / ws * ws;
   }
}

TEST_CASE("bacf_slice: one lag is at least the minimum, in whole words")
{
   bacf_slice slice{window, min_window};
   CHECK(slice.one_lag(10) == words(min_window));
   CHECK(slice.one_lag(min_window) == words(min_window));
   CHECK(slice.one_lag(300) == words(300));
   CHECK(slice.one_lag(3 * ws) == 3 * ws);
}

TEST_CASE("bacf_slice: before any drop, the full window")
{
   // No drop yet: scores run over the full window, every lag is ready,
   // and no pulse is after the drop.
   bacf_slice slice{window, min_window};
   for (std::uint32_t now : {0u, 1000u, 1u << 30})
   {
      CHECK(slice.window(100, now) == window);
      CHECK(slice.ready(100, now));
      CHECK(!slice.after(now));
   }
   slice.observe(bacf_pulse{1000, 0.5f, 20.0f});
   CHECK(slice.peak() == 0.0f);
   CHECK(slice.hint() == 0.0f);
}

TEST_CASE("bacf_slice: no drop yet, at any clock")
{
   // Before any drop, and after reset(), the slice has no start: the full
   // window, every lag ready, no pulse after it, whatever the clock reads,
   // past 2^31 samples and across the wrap included.
   bacf_slice slice{window, min_window};
   for (int pass = 0; pass != 2; ++pass)
   {
      for (std::uint32_t now : {0u, 0x7FFFFFFFu, 0x80000000u, 0x90000000u
       , 0xFFFFFFF0u})
      {
         CHECK(slice.window(100, now) == window);
         CHECK(slice.ready(100, now));
         CHECK(!slice.after(now));
         CHECK(!slice.after(now - 10));
      }
      slice.observe(bacf_pulse{0x90000000u, 0.5f, 20.0f});
      CHECK(slice.peak() == 0.0f);
      slice.drop(0x90000000u, 100.0f);
      slice.reset();
   }
}

TEST_CASE("bacf_slice: after a drop, one lag until the window clears it")
{
   // A score at `lag` reaches a lag and a word back from its window's
   // end, so it runs over one lag of the slice until the full window, the
   // lag and a word are all after the drop.
   std::uint32_t const drop = 10000, lag = 100;
   bacf_slice slice{window, min_window};
   slice.drop(drop, 0.0f);

   CHECK(slice.window(lag, drop) == slice.one_lag(lag));
   CHECK(slice.window(lag, drop + lag + ws) == slice.one_lag(lag));
   auto const clear = drop + window + lag + ws;
   CHECK(slice.window(lag, clear - 1) == slice.one_lag(lag));
   CHECK(slice.window(lag, clear) == window);
   CHECK(slice.window(lag, clear + 12345) == window);
}

TEST_CASE("bacf_slice: ready once one lag of the slice can be scored")
{
   std::uint32_t const drop = 10000, lag = 100;
   bacf_slice slice{window, min_window};
   slice.drop(drop, 0.0f);
   auto const at = drop + slice.one_lag(lag) + lag + ws;
   CHECK(!slice.ready(lag, at - 1));
   CHECK(slice.ready(lag, at));
}

TEST_CASE("bacf_slice: the peak is the strongest pulse since the drop")
{
   bacf_slice slice{window, min_window};
   slice.drop(5000, 321.0f);
   CHECK(slice.hint() == 321.0f);
   slice.observe(bacf_pulse{4990, 0.9f, 20.0f});   // before the drop
   CHECK(slice.peak() == 0.0f);
   slice.observe(bacf_pulse{5000, 0.3f, 20.0f});
   slice.observe(bacf_pulse{5100, 0.6f, 20.0f});
   slice.observe(bacf_pulse{5200, 0.4f, 20.0f});
   CHECK(slice.peak() == 0.6f);
   CHECK(!slice.after(4999));
   CHECK(slice.after(5000));

   // A new drop starts over.
   slice.drop(6000, 0.0f);
   CHECK(slice.peak() == 0.0f);
   CHECK(slice.hint() == 0.0f);
}

TEST_CASE("bacf_slice: across the clock's wrap")
{
   bacf_slice slice{window, min_window};
   std::uint32_t const drop = 0xFFFFFF00u;
   slice.drop(drop, 0.0f);
   CHECK(slice.after(0x10u));
   CHECK(!slice.after(drop - 1));
   CHECK(slice.window(100, drop + window + 100 + ws) == window);
}

TEST_CASE("bacf_slice: reset forgets the drop")
{
   bacf_slice slice{window, min_window};
   slice.drop(5000, 200.0f);
   slice.observe(bacf_pulse{5100, 0.6f, 20.0f});
   slice.reset();
   CHECK(slice.peak() == 0.0f);
   CHECK(slice.hint() == 0.0f);
   CHECK(!slice.after(5100));
   CHECK(slice.window(100, 5100) == window);
}
