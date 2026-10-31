/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_FFT_BENCH_COMMON_OCTOBER_31_2026)
#define CYCFI_Q_FFT_BENCH_COMMON_OCTOBER_31_2026

#include <q/fft/fft.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <vector>

// Shared by fft_bench.cpp and fft_bench_native.cpp. Each includes this with
// its own setting of Q_USE_PFFFT and puts `times` in its own namespace.
namespace fft_bench
{
   struct frame_times
   {
      double fft;       // copy in, fft, ifft: N complex points
      double rfft;      // copy in, rfft, irfft: N real samples
   };

   // Summed and printed by main to keep the loops from being elided.
   inline float sink = 0;

   // Internal linkage: each includer gets its own copy, bound to its own
   // q::fft, so the linker cannot fold the two builds into one.
   namespace
   {
   template <typename F>
   double median_ns(F&& f, int iters)
   {
      for (int i = 0; i != iters; ++i)
         f();

      std::vector<double> runs;
      for (int r = 0; r != 15; ++r)
      {
         auto t0 = std::chrono::steady_clock::now();
         for (int i = 0; i != iters; ++i)
            f();
         auto t1 = std::chrono::steady_clock::now();
         std::chrono::duration<double, std::nano> d = t1 - t0;
         runs.push_back(d.count() / iters);
      }
      std::sort(runs.begin(), runs.end());
      return runs[runs.size()/2];
   }

   template <std::size_t N>
   frame_times measure()
   {
      namespace q = cycfi::q;
      int iters = std::max(200, int(2'000'000 / N));

      std::vector<float> src(2*N), buf(2*N + 2);
      for (std::size_t i = 0; i != 2*N; ++i)
         src[i] = float(std::sin(0.37 * i) + 0.2 * std::cos(0.013 * i * i));

      auto cframe = [&]
      {
         std::memcpy(buf.data(), src.data(), 2*N*sizeof(float));
         q::fft<N>(buf.data());
         q::ifft<N>(buf.data());
         sink += buf[3];
      };

      auto rframe = [&]
      {
         std::memcpy(buf.data(), src.data(), N*sizeof(float));
         q::rfft<N>(buf.data());
         q::irfft<N>(buf.data());
         sink += buf[3];
      };

      return {median_ns(cframe, iters), median_ns(rframe, iters)};
   }

   frame_times measure(std::size_t n)
   {
      switch (n)
      {
         case 256:  return measure<256>();
         case 512:  return measure<512>();
         case 1024: return measure<1024>();
         case 2048: return measure<2048>();
         default:   return measure<4096>();
      }
   }
   }
}

#endif
