/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/pitch/detail/bacf_note.hpp>

#include <cstdint>

namespace q = cycfi::q;
using q::detail::bacf_note;
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

TEST_CASE("bacf_note: one lag is at least the minimum, in whole words")
{
   bacf_note note{window, min_window};
   CHECK(note.one_lag(10) == words(min_window));
   CHECK(note.one_lag(min_window) == words(min_window));
   CHECK(note.one_lag(300) == words(300));
   CHECK(note.one_lag(3 * ws) == 3 * ws);
}

TEST_CASE("bacf_note: before any drop, the full window")
{
   // No note yet: scores run over the full window, every lag is ready,
   // and no pulse is after the drop.
   bacf_note note{window, min_window};
   for (std::uint32_t now : {0u, 1000u, 1u << 30})
   {
      CHECK(note.window(100, now) == window);
      CHECK(note.ready(100, now));
      CHECK(!note.after(now));
   }
   note.observe(bacf_pulse{1000, 0.5f, 20.0f});
   CHECK(note.peak() == 0.0f);
   CHECK(note.hint() == 0.0f);
}

TEST_CASE("bacf_note: after a drop, one lag until the window clears it")
{
   // A score at `lag` reaches a lag and a word back from its window's
   // end, so it runs over one lag of the note until the full window, the
   // lag and a word are all after the drop.
   std::uint32_t const drop = 10000, lag = 100;
   bacf_note note{window, min_window};
   note.drop(drop, 0.0f);

   CHECK(note.window(lag, drop) == note.one_lag(lag));
   CHECK(note.window(lag, drop + lag + ws) == note.one_lag(lag));
   auto const clear = drop + window + lag + ws;
   CHECK(note.window(lag, clear - 1) == note.one_lag(lag));
   CHECK(note.window(lag, clear) == window);
   CHECK(note.window(lag, clear + 12345) == window);
}

TEST_CASE("bacf_note: ready once one lag of the note can be scored")
{
   std::uint32_t const drop = 10000, lag = 100;
   bacf_note note{window, min_window};
   note.drop(drop, 0.0f);
   auto const at = drop + note.one_lag(lag) + lag + ws;
   CHECK(!note.ready(lag, at - 1));
   CHECK(note.ready(lag, at));
}

TEST_CASE("bacf_note: the peak is the strongest pulse since the drop")
{
   bacf_note note{window, min_window};
   note.drop(5000, 321.0f);
   CHECK(note.hint() == 321.0f);
   note.observe(bacf_pulse{4990, 0.9f, 20.0f});   // before the drop
   CHECK(note.peak() == 0.0f);
   note.observe(bacf_pulse{5000, 0.3f, 20.0f});
   note.observe(bacf_pulse{5100, 0.6f, 20.0f});
   note.observe(bacf_pulse{5200, 0.4f, 20.0f});
   CHECK(note.peak() == 0.6f);
   CHECK(!note.after(4999));
   CHECK(note.after(5000));

   // A new drop starts over.
   note.drop(6000, 0.0f);
   CHECK(note.peak() == 0.0f);
   CHECK(note.hint() == 0.0f);
}

TEST_CASE("bacf_note: across the clock's wrap")
{
   bacf_note note{window, min_window};
   std::uint32_t const drop = 0xFFFFFF00u;
   note.drop(drop, 0.0f);
   CHECK(note.after(0x10u));
   CHECK(!note.after(drop - 1));
   CHECK(note.window(100, drop + window + 100 + ws) == window);
}

TEST_CASE("bacf_note: reset forgets the drop")
{
   bacf_note note{window, min_window};
   note.drop(5000, 200.0f);
   note.observe(bacf_pulse{5100, 0.6f, 20.0f});
   note.reset();
   CHECK(note.peak() == 0.0f);
   CHECK(note.hint() == 0.0f);
   CHECK(!note.after(5100));
   CHECK(note.window(100, 5100) == window);
}
