/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/detail/db_table.hpp>
#include <q/support/decibel.hpp>
#include <q/support/literals.hpp>

#include <cmath>
#include <iostream>
#include <fstream>
#include <chrono>
#include <cstring>
#include <cstdint>
#include <iomanip>

namespace q = cycfi::q;

TEST_CASE("Test_inverse_decibel_conversion")
{
   {
      auto db = 119.94;
      INFO("dB: " << db);
      auto result = q::detail::db2a(db);

      REQUIRE_THAT(result,
         Catch::Matchers::WithinRel(std::pow(10, db/20), 0.001)
      );
   }

   {
      auto db = std::numeric_limits<double>::infinity();
      INFO("dB: " << db);
      auto result = q::detail::db2a(db);
      CHECK(result == 1000000.0f); // this is our max limit
   }

   for (int i = 0; i < 1200; ++i)
   {
      {
         auto db = float(i/10.0);
         INFO("dB: " << db);
         auto result = q::detail::db2a(db);

         REQUIRE_THAT(result,
            Catch::Matchers::WithinRel(std::pow(10, db/20), 0.0001)
         );
      }

      for (int j = 0; j < 10; ++j)
      {
         auto db = float(i) + (j / 10.0f);
         INFO("dB: " << db);
         auto result = q::detail::db2a(db/10.0);

         REQUIRE_THAT(result,
            Catch::Matchers::WithinRel(std::pow(10, (db/10.0)/20), 0.0001)
         );
      }
   }
}

TEST_CASE("Test_negative_decibel")
{
   {
      auto db = -6;
      INFO("dB: " << db);
      auto result = q::detail::db2a(db);

      REQUIRE_THAT(result,
         Catch::Matchers::WithinRel(0.5, 0.01)
      );
   }

   {
      auto db = -24;
      INFO("dB: " << db);
      auto result = q::detail::db2a(db);

      REQUIRE_THAT(result,
         Catch::Matchers::WithinRel(0.063096, 0.0001)
      );
   }

   {
      auto db = -36;
      INFO("dB: " << db);
      auto result = q::detail::db2a(db);

      REQUIRE_THAT(result,
         Catch::Matchers::WithinRel(0.015849, 0.0001)
      );
   }
}

TEST_CASE("Test_decibel_operations")
{
   using namespace q::literals;
   {
      auto val = -6.0;
      q::decibel db = q::dB(val);

      CHECK(db == -6_dB);
      REQUIRE_THAT(lin_float(db),
         Catch::Matchers::WithinRel(0.5, 0.01)
      );
   }

   {
      constexpr q::decibel db = q::dB(6.0);

      CHECK(db == 6_dB);
   }

   {
      q::decibel db = 48_dB;
      {
         auto a = lin_float(db);
         CHECK(a == Approx(251.19).epsilon(0.01));
      }
      {
         // A square root is just divide by two in the log domain
         auto a = lin_float(db / 2.0f);

         REQUIRE_THAT(a,
            Catch::Matchers::WithinRel(15.85, 0.01)
         );
      }
   }
}

// Root Mean Square Error
double rmse(std::vector<float> const& ref, std::vector<float> const& test)
{
   double sum_squared_error = 0.0;
   int n = ref.size(); // assuming the signals have the same length
   for (int i = 0; i < n; i++) {
      double error = ref[i] - test[i];
      sum_squared_error += error * error;
   }
   double mean_squared_error = sum_squared_error / n;
   return sqrt(mean_squared_error);
}

TEST_CASE("Test_db_conversion_accuracy")
{
   {
      std::vector<float> ref, ref2;
      std::vector<float> test, test2;
      for (int i = 1; i < 1000000; ++i)
      {
         auto a = float(i) / 1000;
         auto db1 = 20 * cycfi::q::fast_log10(a);
         auto db2 = 20 * std::log10(a);
         test.push_back(db1);
         ref.push_back(db2);

         auto a1 = cycfi::q::fast_pow10(db2/20);
         auto a2 = std::pow(10, db2/20);
         test2.push_back(a1);
         ref2.push_back(a2);
      }
      std::cout << "Root Mean Square Error fast_log10: " << rmse(ref, test) << std::endl;
      std::cout << "Root Mean Square Error fast_pow10: " << rmse(ref2, test2) << std::endl;

      CHECK(rmse(ref, test) < 0.0006);
      // fast_pow10 now scales by the exact log2(10) (see base.hpp), cutting the
      // RMSE vs std::pow from ~0.086 to ~0.016. Threshold tightened accordingly.
      CHECK(rmse(ref2, test2) < 0.03);
   }

   {
      std::vector<float> ref, ref2;
      std::vector<float> test, test2;
      for (int i = 1; i < 1000000; ++i)
      {
         auto a = float(i) / 1000;
         auto db1 = 20 * cycfi::q::faster_log10(a);
         auto db2 = 20 * std::log10(a);
         test.push_back(db1);
         ref.push_back(db2);

         auto a1 = cycfi::q::faster_pow10(db2/20);
         auto a2 = std::pow(10, db2/20);
         test2.push_back(a1);
         ref2.push_back(a2);
      }
      std::cout << "Root Mean Square Error faster_log10: " << rmse(ref, test) << std::endl;
      std::cout << "Root Mean Square Error faster_pow10: " << rmse(ref2, test2) << std::endl;

      CHECK(rmse(ref, test) < 0.15);
      // faster_pow10 likewise now uses the exact log2(10), cutting the RMSE vs
      // std::pow from ~16.5 to ~8.8. Threshold tightened accordingly.
      CHECK(rmse(ref2, test2) < 10);
   }
}

namespace
{
   ////////////////////////////////////////////////////////////////////////////
   // Microbenchmark helpers.
   //
   // The earlier speed test reduced results with `accu += result` -- a serial
   // float-add dependency chain (~3-cycle latency). That floored every figure at
   // the add's latency (~0.9 ns on an Apple M2), hiding the real per-call cost of
   // the faster routines (they all reported ~0.9 ns regardless). We instead
   // measure two honest quantities:
   //
   //   throughput - independent calls, reduced by XOR-ing the result bits (a
   //                ~1-cycle integer op that does not serialize on the FPU). It
   //                is a lower bound on per-call cost; the compiler is free to
   //                auto-vectorize it, so it reflects best-case batch behavior.
   //
   //   latency    - each call feeds the next through `wrap`, forming a true
   //                serial dependency. This is the figure of merit for the
   //                per-sample DSP path (e.g. the onset detector, which is a
   //                stateful per-sample loop and cannot be vectorized), and the
   //                dependency also defeats auto-vectorization. The figure
   //                includes a small fixed harness overhead (the `wrap`), so the
   //                columns are best compared relatively.
   ////////////////////////////////////////////////////////////////////////////
   inline std::uint32_t to_bits(float x)
   {
      std::uint32_t b;
      std::memcpy(&b, &x, sizeof(b));
      return b;
   }

   // Map an arbitrary result back into [base, base + window) with a genuine data
   // dependency, keeping the latency chain inside the function's input domain
   // regardless of the magnitude of its output.
   inline float wrap(float base, float window, float v)
   {
      return base + window * (float(to_bits(v) & 0xFFFF) * (1.0f / 65536.0f));
   }

   template <typename F>
   double throughput_ns(F f, float lo, float hi, int n, int reps)
   {
      std::uint32_t sink = 0;
      auto const step = (hi - lo) / float(n);
      auto start = std::chrono::high_resolution_clock::now();
      for (int r = 0; r != reps; ++r)
         for (int i = 0; i != n; ++i)
            sink ^= to_bits(f(lo + float(i) * step));
      auto elapsed = std::chrono::high_resolution_clock::now() - start;
      volatile std::uint32_t keep = sink; (void) keep;   // defeat DCE
      return std::chrono::duration<double, std::nano>(elapsed).count() / (double(n) * reps);
   }

   template <typename F>
   double latency_ns(F f, float base, float window, float seed, long iters)
   {
      float x = seed;
      auto start = std::chrono::high_resolution_clock::now();
      for (long i = 0; i != iters; ++i)
         x = wrap(base, window, f(x));
      auto elapsed = std::chrono::high_resolution_clock::now() - start;
      volatile float keep = x; (void) keep;              // defeat DCE
      return std::chrono::duration<double, std::nano>(elapsed).count() / double(iters);
   }
}

TEST_CASE("Test_decibel_speed")
{
   using cycfi::q::fast_log10;
   using cycfi::q::faster_log10;
   using cycfi::q::fast_pow10;
   using cycfi::q::faster_pow10;

   int const  tn = 1023, treps = 4096;   // throughput: ~4.2M independent calls each
   long const lit = 5000000;             // latency:    5M dependent calls each

   double total = 0;
   std::cout << std::fixed << std::setprecision(3);

   // linear -> dB: input is a linear amplitude, exercised over [1, 1e6).
   auto bench_log = [&](char const* name, auto f)
   {
      double thr = throughput_ns(f, 1.0f, 1e6f, tn, treps);
      double lat = latency_ns(f, 1.0f, 1e6f, 1000.0f, lit);
      std::cout << "   " << name << "   throughput " << thr
                << " ns   latency " << lat << " ns\n";
      total += thr + lat;
   };

   // dB -> linear: input is dB, exercised over [0, 120) (the table's range).
   auto bench_pow = [&](char const* name, auto f)
   {
      double thr = throughput_ns(f, 0.0f, 120.0f, tn, treps);
      double lat = latency_ns(f, 0.0f, 120.0f, 60.0f, lit);
      std::cout << "   " << name << "   throughput " << thr
                << " ns   latency " << lat << " ns\n";
      total += thr + lat;
   };

   // NB: "latency" is the figure of merit for the per-sample path; "throughput"
   // may be auto-vectorized. Run a few times -- these are wall-clock and noisy.
   std::cout << "\n-- linear -> dB  (input: amplitude in [1, 1e6)) --\n";
   bench_log("std::log10  ", [](float a) { return 20.0f * std::log10(a); });
   bench_log("fast_log10  ", [](float a) { return 20.0f * fast_log10(a); });
   bench_log("faster_log10", [](float a) { return 20.0f * faster_log10(a); });

   // db2a (== lin_float) is the fast table path; lin_double is its std-only,
   // exact counterpart (std::pow). Both compute 10^(db/20).
   std::cout << "-- dB -> linear  (input: dB in [0, 120)) --\n";
   bench_pow("db2a        ", [](float db) { return q::detail::db2a(db); });
   bench_pow("lin_double  ", [](float db) { return float(q::lin_double(q::dB(db))); });
   bench_pow("fast_pow10  ", [](float db) { return fast_pow10(db / 20.0f); });
   bench_pow("faster_pow10", [](float db) { return faster_pow10(db / 20.0f); });
   std::cout << std::endl;

   // Sanity: the benchmarked work actually ran (and was not elided).
   CHECK(total > 0.0);
}
