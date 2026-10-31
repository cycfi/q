/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>

#include <q/fft/stft.hpp>
#include <q/fft/stft_window.hpp>

#include <cmath>
#include <numbers>
#include <vector>

namespace q = cycfi::q;
using Catch::Matchers::WithinAbs;

namespace
{
   // The overlap-add sum of analysis * synthesis at sample n of a frame:
   // what the output gets from every frame covering it.
   double overlap_sum(
      q::stft_window const& w, std::size_t hop, std::size_t n)
   {
      double sum = 0;
      for (auto j = n % hop; j < w.size(); j += hop)
         sum += double(w.analysis[j]) * w.synthesis[j];
      return sum;
   }

   // A deterministic test signal.
   std::vector<float> make_signal(std::size_t n)
   {
      std::vector<float> x(n);
      for (std::size_t t = 0; t != n; ++t)
         x[t] = float(0.5 * std::sin(0.031 * t) + 0.3 * std::sin(0.43 * t)
              + 0.1 * ((t * 7919) % 101) / 101.0);
      return x;
   }

   // Runs x through s with `f`, returning the output.
   template <typename F>
   std::vector<float> run(q::stft& s, std::vector<float> const& x, F&& f)
   {
      std::vector<float> y(x.size());
      for (std::size_t t = 0; t != x.size(); ++t)
         y[t] = s(x[t], f);
      return y;
   }

   auto const leave_alone = [](auto) {};

   // The largest |y(t) - x(t - latency)| over the whole run.
   double delay_error(
      std::vector<float> const& x, std::vector<float> const& y
    , std::size_t latency)
   {
      double err = 0;
      for (std::size_t t = 0; t != y.size(); ++t)
      {
         double expected = t >= latency? x[t - latency] : 0.0;
         err = std::max(err, std::abs(y[t] - expected));
      }
      return err;
   }
}

TEST_CASE("stft_window: overlap-add of analysis * synthesis is 1", "[stft]")
{
   constexpr std::size_t N = 1024;

   SECTION("hann")
   {
      for (std::size_t hop : {N/2, N/4, N/8, std::size_t(100)})
      {
         auto w = q::stft_window::hann(N, hop);
         for (std::size_t n = 0; n != N; ++n)
            REQUIRE_THAT(overlap_sum(w, hop, n), WithinAbs(1.0, 1e-6));
      }
   }

   SECTION("low_delay")
   {
      for (std::size_t delay : {std::size_t(256), std::size_t(512), N})
      {
         for (std::size_t hop : {delay/2, delay/4, std::size_t(48)})
         {
            auto w = q::stft_window::low_delay(N, hop, delay);
            for (std::size_t n = 0; n != N; ++n)
               REQUIRE_THAT(overlap_sum(w, hop, n), WithinAbs(1.0, 1e-6));
         }
      }
   }
}

TEST_CASE("stft_window: the latency", "[stft]")
{
   REQUIRE(q::stft_window::hann(512, 128).latency == 512);
   REQUIRE(q::stft_window::low_delay(2048, 128, 512).latency == 512);
}

TEST_CASE("stft_window: low_delay synthesis is at the newest end", "[stft]")
{
   constexpr std::size_t N = 2048;
   constexpr std::size_t delay = 512;
   auto w = q::stft_window::low_delay(N, 128, delay);

   for (std::size_t n = 0; n != N - delay; ++n)
      REQUIRE(w.synthesis[n] == 0.0f);
   for (std::size_t n = 1; n != N; ++n)
      REQUIRE(w.analysis[n] > 0.0f);
}

TEST_CASE("stft: left alone, the output is the input delayed", "[stft]")
{
   auto x = make_signal(20000);

   SECTION("hann, N = 512, hop = 128")
   {
      q::stft s{q::stft_window::hann(512, 128), 128};
      auto y = run(s, x, leave_alone);
      REQUIRE(s.latency() == 512);
      REQUIRE(delay_error(x, y, s.latency()) < 1e-5);
   }

   SECTION("hann, N = 1024, hop = 100")
   {
      q::stft s{q::stft_window::hann(1024, 100), 100};
      auto y = run(s, x, leave_alone);
      REQUIRE(delay_error(x, y, s.latency()) < 1e-5);
   }

   SECTION("low_delay, N = 2048, delay = 512, hop = 128")
   {
      q::stft s{q::stft_window::low_delay(2048, 128, 512), 128};
      auto y = run(s, x, leave_alone);
      REQUIRE(s.latency() == 512);
      REQUIRE(delay_error(x, y, s.latency()) < 1e-5);
   }

   SECTION("low_delay, N = 2048, delay = 480, hop = 60")
   {
      q::stft s{q::stft_window::low_delay(2048, 60, 480), 60};
      auto y = run(s, x, leave_alone);
      REQUIRE(s.latency() == 480);
      REQUIRE(delay_error(x, y, s.latency()) < 1e-5);
   }
}

TEST_CASE("stft: f is called once a hop", "[stft]")
{
   q::stft s{q::stft_window::hann(512, 128), 128};
   int calls = 0;
   auto count = [&](auto) { ++calls; };
   for (int t = 0; t != 1280; ++t)
      s(0.0f, count);
   REQUIRE(calls == 10);
}

TEST_CASE("stft: f sees rfft's bins", "[stft]")
{
   // A sine centred on bin 20 of a 512-point frame peaks there.
   constexpr std::size_t N = 512;
   constexpr std::size_t k0 = 20;
   q::stft s{q::stft_window::hann(N, 128), 128};

   std::size_t peak = 0;
   auto find_peak = [&](q::stft::bins_type bins)
   {
      float best = 0;
      for (std::size_t k = 0; k <= N/2; ++k)
      {
         auto m = std::hypot(bins[2*k], bins[2*k + 1]);
         if (m > best)
         {
            best = m;
            peak = k;
         }
      }
   };

   for (std::size_t t = 0; t != 4*N; ++t)
      s(float(std::sin(2 * std::numbers::pi * k0 * t / N)), find_peak);
   REQUIRE(peak == k0);
}

TEST_CASE("stft: zeroed bins give silence", "[stft]")
{
   auto x = make_signal(8192);
   q::stft s{q::stft_window::low_delay(1024, 128, 512), 128};
   auto zero = [](q::stft::bins_type bins)
   {
      for (auto& b : bins)
         b = 0.0f;
   };
   auto y = run(s, x, zero);
   for (auto v : y)
      REQUIRE(v == 0.0f);
}

TEST_CASE("stft: reset starts over", "[stft]")
{
   auto x = make_signal(6000);
   q::stft a{q::stft_window::hann(512, 128), 128};
   q::stft b{q::stft_window::hann(512, 128), 128};

   run(a, make_signal(1234), leave_alone);
   a.reset();
   REQUIRE(run(a, x, leave_alone) == run(b, x, leave_alone));
}
