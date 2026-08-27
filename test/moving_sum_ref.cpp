/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>

// Both concepts headers in one translation unit, concepts.hpp FIRST: they
// used to share an include guard, so this order emptied basic_concepts.hpp
// and IndexableContainer went missing. The order matters -- pulling in
// anything that reaches basic_concepts.hpp first (envelope.hpp does, via
// decibel.hpp) hides the bug.
#include <q/support/concepts.hpp>
#include <q/support/basic_concepts.hpp>

#include <q/fx/moving_sum.hpp>
#include <q/fx/moving_average.hpp>
#include <q/fx/envelope.hpp>

#include <array>
#include <cmath>
#include <vector>

namespace q = cycfi::q;

namespace
{
   // A caller-owned history, written the way a host does it: the views read
   // the departing sample, THEN the host pushes. Same order basic_moving_sum
   // uses internally.
   constexpr float test_signal(int i)
   {
      return 1.0f + 0.5f * float((i * 7) % 13) - 0.25f * float((i * 3) % 5);
   }
}

///////////////////////////////////////////////////////////////////////////////
// The reference behavior: a _ref view fed from a caller-owned history must
// track the owning class sample for sample.
///////////////////////////////////////////////////////////////////////////////
TEST_CASE("Test_moving_sum_ref_matches_owning")
{
   constexpr std::size_t window = 10;
   auto owning = q::basic_moving_sum<float>{window};
   auto ref = q::basic_moving_sum_ref<float>{window};
   auto hist = q::ring_buffer<float>{window};
   hist.clear();

   for (int i = 0; i != 100; ++i)
   {
      auto s = test_signal(i);
      auto expected = owning(s);
      auto got = ref(s, hist);
      hist.push(s);
      CHECK(got == Approx(expected));
   }
}

TEST_CASE("Test_moving_average_ref_matches_owning")
{
   constexpr std::size_t window = 8;
   auto owning = q::basic_moving_average<float>{window};
   auto ref = q::basic_moving_average_ref<float>{window};
   auto hist = q::ring_buffer<float>{window};
   hist.clear();

   for (int i = 0; i != 100; ++i)
   {
      auto s = test_signal(i);
      auto expected = owning(s);
      auto got = ref(s, hist);
      hist.push(s);
      CHECK(got == Approx(expected));
   }
}

///////////////////////////////////////////////////////////////////////////////
// The point of the exercise: several windows over ONE history. Each view must
// read exactly what its own owning counterpart would.
///////////////////////////////////////////////////////////////////////////////
TEST_CASE("Test_moving_sum_ref_shared_history")
{
   constexpr std::size_t longest = 32;
   auto owning_4 = q::basic_moving_sum<float>{4};
   auto owning_9 = q::basic_moving_sum<float>{9};
   auto owning_32 = q::basic_moving_sum<float>{longest};

   auto ref_4 = q::basic_moving_sum_ref<float>{4};
   auto ref_9 = q::basic_moving_sum_ref<float>{9};
   auto ref_32 = q::basic_moving_sum_ref<float>{longest};

   // One history, sized to the longest window.
   auto hist = q::ring_buffer<float>{longest};
   hist.clear();

   for (int i = 0; i != 200; ++i)
   {
      auto s = test_signal(i);
      auto e4 = owning_4(s), e9 = owning_9(s), e32 = owning_32(s);
      auto g4 = ref_4(s, hist), g9 = ref_9(s, hist), g32 = ref_32(s, hist);
      hist.push(s);
      CHECK(g4 == Approx(e4));
      CHECK(g9 == Approx(e9));
      CHECK(g32 == Approx(e32));
   }
}

///////////////////////////////////////////////////////////////////////////////
// The two-value overload: the caller supplies both ends, so a history of the
// RAW signal can feed a sum of some function of it. This is what lets one
// history serve RMS and plain averages together.
///////////////////////////////////////////////////////////////////////////////
TEST_CASE("Test_moving_sum_ref_entering_departing")
{
   constexpr std::size_t window = 6;
   auto squares = q::basic_moving_sum<float>{window};   // history of squares
   auto ref = q::basic_moving_sum_ref<float>{window};   // history of raw s
   auto hist = q::ring_buffer<float>{window};
   hist.clear();

   for (int i = 0; i != 60; ++i)
   {
      auto s = test_signal(i);
      auto expected = squares(s * s);
      auto departing = hist[window-1];
      auto got = ref(s * s, departing * departing);
      hist.push(s);
      CHECK(got == Approx(expected));
   }
}

///////////////////////////////////////////////////////////////////////////////
// true_rms_envelope_follower_ref squares both ends itself, so the history it
// reads is the raw signal -- shareable with the plain views above.
///////////////////////////////////////////////////////////////////////////////
TEST_CASE("Test_true_rms_envelope_follower_ref_matches_owning")
{
   constexpr std::size_t window = 16;
   auto owning = q::true_rms_envelope_follower{window};
   auto ref = q::true_rms_envelope_follower_ref{window};
   auto hist = q::ring_buffer<float>{window};
   hist.clear();

   for (int i = 0; i != 200; ++i)
   {
      auto s = 0.1f * test_signal(i);
      auto expected = owning(s);
      auto got = ref(s, hist);
      hist.push(s);
      CHECK(got == Approx(expected));
      CHECK(ref.mean_square() == Approx(owning.mean_square()));
   }
}

TEST_CASE("Test_true_rms_ref_shares_a_history_with_an_average")
{
   constexpr std::size_t window = 16;
   auto rms = q::true_rms_envelope_follower_ref{window};
   auto mean_abs = q::basic_moving_average_ref<float>{window};
   auto hist = q::ring_buffer<float>{window};
   hist.clear();

   // Reference followers, each owning its own copy of the same samples.
   auto rms_owning = q::true_rms_envelope_follower{window};
   auto mean_owning = q::basic_moving_average<float>{window};

   for (int i = 0; i != 200; ++i)
   {
      auto s = 0.1f * test_signal(i);
      auto departing = hist[window-1];
      auto got_rms = rms(s, hist);
      auto got_mean = mean_abs(std::abs(s), std::abs(departing));
      hist.push(s);
      CHECK(got_rms == Approx(rms_owning(s)));
      CHECK(got_mean == Approx(mean_owning(std::abs(s))));
   }
}

///////////////////////////////////////////////////////////////////////////////
// The history is a concept, not a fixed type: anything indexable will do.
///////////////////////////////////////////////////////////////////////////////
TEST_CASE("Test_moving_sum_ref_accepts_any_indexable_history")
{
   static_assert(q::concepts::IndexableContainer<q::ring_buffer<float>>);
   static_assert(q::concepts::IndexableContainer<std::vector<float>>);
   static_assert(q::concepts::IndexableContainer<std::array<float, 8>>);

   constexpr std::size_t window = 4;
   auto owning = q::basic_moving_sum<float>{window};
   auto ref = q::basic_moving_sum_ref<float>{window};

   // A plain vector as the history, newest at [0].
   std::vector<float> hist(window, 0.0f);

   for (int i = 0; i != 40; ++i)
   {
      auto s = test_signal(i);
      auto expected = owning(s);
      auto got = ref(s, hist);
      hist.insert(hist.begin(), s);
      hist.pop_back();
      CHECK(got == Approx(expected));
   }
}

///////////////////////////////////////////////////////////////////////////////
// Resizing. The plain resize clears; the history overload carries the sum
// over, matching the owning class's update=true.
///////////////////////////////////////////////////////////////////////////////
TEST_CASE("Test_moving_sum_ref_resize")
{
   constexpr std::size_t window = 10;
   auto owning = q::basic_moving_sum<int>{window};
   auto ref = q::basic_moving_sum_ref<int>{window};
   auto hist = q::ring_buffer<int>{window};
   hist.clear();

   int const in[] = {3, 2, 1, 1, 1, 1, 1, 1, 4, 5};
   for (auto s : in)
   {
      owning(s);
      ref(s, hist);
      hist.push(s);
   }
   CHECK(owning() == 20);
   CHECK(ref() == 20);

   owning.resize(8, true);
   ref.resize(8, hist);
   CHECK(ref() == owning());
   CHECK(ref() == 15);

   owning.resize(10, true);
   ref.resize(10, hist);
   CHECK(ref() == owning());
   CHECK(ref() == 20);

   owning.resize(1, true);
   ref.resize(1, hist);
   CHECK(ref() == owning());
   CHECK(ref() == 5);

   ref.resize(4);          // no history: clears
   CHECK(ref() == 0);
   CHECK(ref.size() == 4);
}

///////////////////////////////////////////////////////////////////////////////
// set(): the caller hands back a sum it computed itself. Needed when the
// window holds a value derived from the history and that derivation changes,
// since the running sum would go on subtracting terms it never added.
///////////////////////////////////////////////////////////////////////////////
TEST_CASE("Test_moving_sum_ref_set")
{
   constexpr std::size_t window = 8;
   std::vector<float> x(200);
   for (std::size_t i = 0; i != x.size(); ++i)
      x[i] = test_signal(int(i));

   // A view summing the square of the sample `lag` back: a derivation the
   // class cannot reproduce, so only the caller can rebuild it.
   auto squared_at = [&](std::size_t n, std::size_t lag)
   {
      return double(x[n - lag]) * x[n - lag];
   };
   auto direct = [&](std::size_t n, std::size_t lag)
   {
      double sum = 0;
      for (std::size_t k = 0; k != window; ++k)
         sum += squared_at(n - k, lag);
      return sum;
   };
   auto run = [&](q::moving_sum_ref& v, std::size_t from, std::size_t to,
                  std::size_t lag)
   {
      for (std::size_t n = from; n != to; ++n)
         v(float(squared_at(n, lag)), float(squared_at(n - window, lag)));
   };

   // Primed with the sum at the starting sample, it tracks from there.
   std::size_t lag = 3;
   auto view = q::moving_sum_ref{window};
   view.set(direct(20, lag));
   run(view, 21, 60, lag);
   CHECK(view() == Approx(direct(59, lag)).epsilon(1e-5));

   // Change the lag. Without a re-seed the sum keeps subtracting departing
   // terms from the OLD lag, which it never added, so it settles a constant
   // away from the truth and never recovers.
   auto stale = view;
   lag = 9;
   run(stale, 60, 120, lag);
   CHECK(stale() != Approx(direct(119, lag)).epsilon(1e-3));

   // Re-seeded at the change, it tracks the new derivation exactly.
   view.set(direct(59, lag));
   run(view, 60, 120, lag);
   CHECK(view() == Approx(direct(119, lag)).epsilon(1e-5));

   // set() replaces the sum outright, and leaves the window alone.
   view.set(0);
   CHECK(view() == 0);
   CHECK(view.size() == window);
}
