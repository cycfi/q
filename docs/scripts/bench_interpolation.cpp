/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]

   Micro-benchmark for the sample_interpolation types. Feeds the measured
   ns-per-read numbers in gen_interpolation_figures.py (the
   interpolation_cpu.svg figure).

   Build and run (from the repo root):

      clang++ -O3 -std=c++20 -Iq_lib/include -Iinfra/include \
         docs/scripts/bench_interpolation.cpp -o /tmp/bench_interpolation
      /tmp/bench_interpolation

   Method: a 1024-sample noise buffer is read through each interpolation
   type at 4096 precomputed random fractional indices, repeated for ~16M
   reads per type, 3 repetitions, minimum taken. Eight independent
   accumulators keep the measurement from serializing on the FP-add
   dependency chain (which would hide the cost of the cheap types).
=============================================================================*/
#include <q/utility/fractional_ring_buffer.hpp>

#include <chrono>
#include <cstdio>
#include <random>
#include <vector>

namespace q = cycfi::q;
using namespace q::sample_interpolation;

constexpr std::size_t buff_size = 1024;
constexpr std::size_t n_indices = 4096;
constexpr int passes = 4000;        // 4096 * 4000 = ~16.4M reads
constexpr int reps = 3;

template <typename Interpolation>
double bench(std::vector<float> const& indices)
{
   q::fractional_ring_buffer<
      float, std::vector<float>, float, Interpolation> buf(buff_size);

   std::minstd_rand gen(12345);
   std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
   for (std::size_t i = 0; i != buff_size; ++i)
      buf.push(dist(gen));

   auto best = 1e30;
   for (auto r = 0; r != reps; ++r)
   {
      float s0 = 0, s1 = 0, s2 = 0, s3 = 0;
      float s4 = 0, s5 = 0, s6 = 0, s7 = 0;

      auto start = std::chrono::steady_clock::now();
      for (auto p = 0; p != passes; ++p)
      {
         for (std::size_t k = 0; k != n_indices; k += 8)
         {
            s0 += buf[indices[k]];
            s1 += buf[indices[k+1]];
            s2 += buf[indices[k+2]];
            s3 += buf[indices[k+3]];
            s4 += buf[indices[k+4]];
            s5 += buf[indices[k+5]];
            s6 += buf[indices[k+6]];
            s7 += buf[indices[k+7]];
         }
      }
      auto stop = std::chrono::steady_clock::now();

      volatile float sink = s0+s1+s2+s3+s4+s5+s6+s7;
      (void) sink;

      auto ns = std::chrono::duration<double, std::nano>(stop-start).count();
      best = std::min(best, ns / (double(passes) * n_indices));
   }
   return best;
}

int main()
{
   // Random fractional indices within the strictest (4-point) valid range
   std::vector<float> indices(n_indices);
   std::minstd_rand gen(54321);
   std::uniform_real_distribution<float> dist(2.0f, float(buff_size-4));
   for (auto& i : indices)
      i = dist(gen);

   std::printf("none     %.3f\n", bench<none>(indices));
   std::printf("linear   %.3f\n", bench<linear>(indices));
   std::printf("cosine   %.3f\n", bench<cosine>(indices));
   std::printf("cubic    %.3f\n", bench<cubic>(indices));
   std::printf("hermite  %.3f\n", bench<hermite>(indices));
   std::printf("bspline  %.3f\n", bench<bspline>(indices));
   return 0;
}
