/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]

   Micro-benchmark for the decibel <-> linear conversions: fast_log10,
   faster_log10 (linear -> dB) and db2a (== lin_float), lin_double,
   fast_pow10, faster_pow10 (dB -> linear). Build-only; not a CI test
   (there is nothing to assert, just measure and print).

   Build and run (from the repo root):

      clang++ -O3 -std=c++20 -Iq_lib/include -Iinfra/include \
         test/benchmark/decibel_bench.cpp -o /tmp/decibel_bench
      /tmp/decibel_bench
=============================================================================*/
#include <q/detail/db_table.hpp>
#include <q/support/decibel.hpp>
#include <q/support/literals.hpp>

#include <cmath>
#include <iostream>
#include <chrono>
#include <cstring>
#include <cstdint>
#include <iomanip>

namespace q = cycfi::q;

namespace
{
   ////////////////////////////////////////////////////////////////////////////
   // Microbenchmark helpers.
   //
   // A naive speed test reduces results with `accu += result` -- a serial
   // float-add dependency chain (~3-cycle latency). That floors every figure at
   // the add's latency (~0.9 ns on an Apple M2), hiding the real per-call cost of
   // the faster routines (they all report ~0.9 ns regardless). We instead
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

int main()
{
   using cycfi::q::fast_log10;
   using cycfi::q::faster_log10;
   using cycfi::q::fast_pow10;
   using cycfi::q::faster_pow10;

   int const  tn = 1023, treps = 4096;   // throughput: ~4.2M independent calls each
   long const lit = 5000000;             // latency:    5M dependent calls each

   std::cout << std::fixed << std::setprecision(3);

   // linear -> dB: input is a linear amplitude, exercised over [1, 1e6).
   auto bench_log = [&](char const* name, auto f)
   {
      double thr = throughput_ns(f, 1.0f, 1e6f, tn, treps);
      double lat = latency_ns(f, 1.0f, 1e6f, 1000.0f, lit);
      std::cout << "   " << name << "   throughput " << thr
                << " ns   latency " << lat << " ns\n";
   };

   // dB -> linear: input is dB, exercised over [0, 120) (the table's range).
   auto bench_pow = [&](char const* name, auto f)
   {
      double thr = throughput_ns(f, 0.0f, 120.0f, tn, treps);
      double lat = latency_ns(f, 0.0f, 120.0f, 60.0f, lit);
      std::cout << "   " << name << "   throughput " << thr
                << " ns   latency " << lat << " ns\n";
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

   return 0;
}
