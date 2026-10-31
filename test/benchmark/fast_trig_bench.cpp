/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]

   Micro-benchmark for q::fast_sincos vs std::sin and std::cos, and
   q::fast_atan2 vs std::atan2. Build-only; not a CI test. Times are the
   best of 30 runs over 4096 values, in ns per call.

   Build and run (from the repo root):

      clang++ -O3 -std=c++20 -Iq_lib/include -Iinfra/include \
         test/benchmark/fast_trig_bench.cpp -o /tmp/fast_trig_bench
      /tmp/fast_trig_bench
=============================================================================*/
#include <q/support/base.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <vector>

namespace q = cycfi::q;

namespace
{
   // Summed and printed at the end to prevent dead-code elimination.
   float accu = 0;

   template <typename F>
   double ns_per_call(F f)
   {
      std::vector<float> x(4096), y(4096);
      for (int i = 0; i != 4096; ++i)
      {
         x[i] = -50.0f + 100.0f * i / 4096;
         y[i] = 0.3f - 0.6f * ((i * 37) % 101) / 101.0f;
      }

      double best = 1e9;
      for (int r = 0; r != 30; ++r)
      {
         auto start = std::chrono::steady_clock::now();
         for (int k = 0; k != 50; ++k)
         {
            for (int i = 0; i != 4096; ++i)
               accu += f(x[i], y[i]);
         }
         auto elapsed = std::chrono::steady_clock::now() - start;
         std::chrono::duration<double, std::nano> d = elapsed;
         best = std::min(best, d.count() / (50 * 4096));
      }
      return best;
   }
}

int main()
{
   std::cout << "std::sin + std::cos (ns): " << ns_per_call(
      [](float x, float) { return std::sin(x) + std::cos(x); }) << std::endl;

   std::cout << "q::fast_sincos (ns): " << ns_per_call(
      [](float x, float)
      {
         auto [s, c] = q::fast_sincos(x);
         return s + c;
      }) << std::endl;

   std::cout << "std::atan2 (ns): " << ns_per_call(
      [](float x, float y) { return std::atan2(y, x); }) << std::endl;

   std::cout << "q::fast_atan2 (ns): " << ns_per_call(
      [](float x, float y) { return q::fast_atan2(y, x); }) << std::endl;

   std::cout << "(accu " << accu << ")" << std::endl;
}
