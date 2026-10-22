/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/detail/db_table.hpp>
#include <q/support/decibel.hpp>
#include <q/support/literals.hpp>

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <vector>

namespace q = cycfi::q;

namespace
{
   // The exact conversions the tables are measured against.
   double exact_lin(double db) { return std::pow(10.0, db / 20); }
   double exact_db(double a)   { return 20 * std::log10(a); }

   // Relative error of db2a against std::pow over [lo, hi] dB in steps
   // of `step`, and whether the table stays monotonic across every seam.
   struct sweep_result { double max_err; double at; bool monotonic; };

   sweep_result sweep_db2a(double lo, double hi, double step)
   {
      sweep_result r{0, 0, true};
      float prev = 0;
      for (double db = lo; db <= hi; db += step)
      {
         auto got = q::detail::db2a(float(db));
         auto ref = exact_lin(db);
         auto err = std::abs(got - ref) / ref;
         if (err > r.max_err)
         {
            r.max_err = err;
            r.at = db;
         }
         if (db > lo && !(got > prev))
            r.monotonic = false;
         prev = got;
      }
      return r;
   }

   sweep_result sweep_a2db(double lo, double hi, double step)
   {
      sweep_result r{0, 0, true};
      float prev = 0;
      for (double db = lo; db <= hi; db += step)
      {
         auto a = float(exact_lin(db));
         auto got = q::detail::a2db(a);
         auto err = std::abs(double(got) - exact_db(a));
         if (err > r.max_err)
         {
            r.max_err = err;
            r.at = db;
         }
         if (db > lo && got < prev)
            r.monotonic = false;
         prev = got;
      }
      return r;
   }
}

TEST_CASE("Test_db2a_accuracy")
{
   // The working range, fine steps: the interpolation error of a 256-entry
   // table of 2^f is about 2e-6; the seams between entries and between
   // octaves must not show.
   auto r = sweep_db2a(-200, 200, 0.001);
   INFO("max relative error " << r.max_err << " at " << r.at << " dB");
   CHECK(r.max_err < 1e-5);
   CHECK(r.monotonic);

   // The whole float range, coarser.
   auto w = sweep_db2a(-750, 760, 0.01);
   INFO("max relative error " << w.max_err << " at " << w.at << " dB");
   CHECK(w.max_err < 1e-5);
   CHECK(w.monotonic);

   // Exact points
   CHECK(q::detail::db2a(0.0f) == 1.0f);
   CHECK(q::detail::db2a(-6.0206f) == Approx(0.5f).epsilon(1e-5));
   CHECK(q::detail::db2a(20.0f) == Approx(10.0f).epsilon(1e-5));
   CHECK(q::detail::db2a(-120.0f) == Approx(1e-6f).epsilon(1e-5));
}

TEST_CASE("Test_db2a_range")
{
   // Any float in: the extremes are held to the normal float range and
   // never go denormal, infinite or NaN.
   auto inf = std::numeric_limits<float>::infinity();
   auto nan = std::numeric_limits<float>::quiet_NaN();
   auto top = q::detail::db2a(inf);
   auto bottom = q::detail::db2a(-inf);
   CHECK(std::isfinite(top));
   CHECK(top > 1e37f);
   CHECK(bottom > 0.0f);
   CHECK(bottom >= std::numeric_limits<float>::min());
   CHECK(q::detail::db2a(1000.0f) == top);
   CHECK(q::detail::db2a(-1000.0f) == bottom);
   CHECK(q::detail::db2a(nan) == bottom);
}

TEST_CASE("Test_db2a_constexpr")
{
   // lin_float is usable in constant expressions (envelope followers set
   // their floors that way); the compile-time value matches the table.
   constexpr auto c = q::lin_float(q::dB(-120.0));
   static_assert(c > 0.9e-6f && c < 1.1e-6f);
   CHECK(c == Approx(q::detail::db2a(-120.0f)).epsilon(1e-5));

   constexpr auto unity = q::lin_float(q::dB(0.0));
   static_assert(unity == 1.0f);
   constexpr auto six = q::lin_float(q::dB(6.0));
   static_assert(six > 1.99f && six < 2.0f);
}

TEST_CASE("Test_a2db_accuracy")
{
   // 256 entries of log2(1 + j/256) with linear interpolation: about
   // 3e-5 dB, the same at every level since the exponent is exact.
   auto r = sweep_a2db(-150, 150, 0.001);
   INFO("max error " << r.max_err << " dB at " << r.at << " dB");
   CHECK(r.max_err < 1e-4);
   CHECK(r.monotonic);

   auto w = sweep_a2db(-750, 760, 0.01);
   INFO("max error " << w.max_err << " dB at " << w.at << " dB");
   CHECK(w.max_err < 1e-4);

   // Exact points, the magnitude, and the floor at zero
   CHECK(q::detail::a2db(1.0f) == 0.0f);
   CHECK(q::detail::a2db(2.0f) == Approx(6.0206f).margin(1e-4));
   CHECK(q::detail::a2db(0.5f) == Approx(-6.0206f).margin(1e-4));
   CHECK(q::detail::a2db(-2.0f) == q::detail::a2db(2.0f));
   CHECK(std::isfinite(q::detail::a2db(0.0f)));
   CHECK(q::detail::a2db(0.0f) < -700.0f);
}

TEST_CASE("Test_db_round_trip")
{
   // dB -> linear -> dB lands within 1e-4 dB over the working range.
   double worst = 0;
   for (double db = -150; db <= 150; db += 0.01)
   {
      auto back = q::lin_to_db(q::lin_float(q::dB(db))).rep;
      worst = std::max(worst, std::abs(back - db));
   }
   INFO("worst round trip " << worst << " dB");
   CHECK(worst < 1e-4);
}

TEST_CASE("Test_negative_decibel")
{
   CHECK(q::detail::db2a(-6) == Approx(0.501187).epsilon(1e-5));
   CHECK(q::detail::db2a(-24) == Approx(0.063096).epsilon(1e-4));
   CHECK(q::detail::db2a(-36) == Approx(0.015849).epsilon(1e-4));
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

////////////////////////////////////////////////////////////////////////////
// Timing: the latency of each conversion on a serial chain (each result
// feeds the next input), the figure of merit for the per-sample path.
// Printed for the record; the only check is that nothing is pathological,
// since wall-clock numbers vary by machine and load. The full benchmark
// with throughput figures is test/benchmark/decibel_bench.cpp.
////////////////////////////////////////////////////////////////////////////
namespace
{
   template <typename F>
   double latency_ns(F f, float lo, float window)
   {
      using clock = std::chrono::steady_clock;
      constexpr int n = 2000000;
      float x = lo + window / 2;
      float acc = 0;
      double best = 1e9;
      for (int rep = 0; rep != 3; ++rep)
      {
         auto t0 = clock::now();
         for (int i = 0; i != n; ++i)
         {
            float y = f(x);
            x = lo + (y - std::floor(y)) * window;
            acc += x;
         }
         auto t1 = clock::now();
         auto ns = std::chrono::duration<double, std::nano>(t1 - t0)
            .count() / n;
         best = std::min(best, ns);
      }
      if (acc == 12345.0f)   // keep the chain alive
         std::cout << acc;
      return best;
   }
}

TEST_CASE("Test_decibel_timing")
{
   auto ident = [](float x) { return x * 1.0001f; };
   auto db2a = [](float db) { return q::detail::db2a(db); };
   auto pow = [](float db) { return float(std::pow(10.0f, db / 20)); };
   auto a2db = [](float a) { return q::detail::a2db(a); };
   auto log = [](float a) { return 20.0f * std::log10(a); };
   auto approx = [](float a) { return float(q::approx_db(a).rep); };

   auto overhead = latency_ns(ident, -60, 60);
   auto t_db2a = latency_ns(db2a, -60, 60);
   auto t_pow = latency_ns(pow, -60, 60);
   auto t_a2db = latency_ns(a2db, 0.001f, 0.9f);
   auto t_log = latency_ns(log, 0.001f, 0.9f);
   auto t_approx = latency_ns(approx, 0.001f, 0.9f);

#if defined(NDEBUG)
   char const* build = "optimized build";
#else
   char const* build = "unoptimized build, not representative";
#endif
   std::cout << std::fixed << std::setprecision(2)
      << "decibel latency, ns per call, serial chain (" << build
      << "), harness overhead " << overhead << " ns included:\n"
      << "   db2a (lin_float)   " << t_db2a << "\n"
      << "   std::pow           " << t_pow << "\n"
      << "   a2db (lin_to_db)   " << t_a2db << "\n"
      << "   std::log10         " << t_log << "\n"
      << "   approx_db          " << t_approx << "\n";

   for (auto t : {t_db2a, t_pow, t_a2db, t_log, t_approx})
   {
      CHECK(t > 0.0);
      CHECK(t < 1000.0);
   }
}
