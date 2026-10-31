/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]

   Micro-benchmark for q::fft/ifft and q::rfft/irfft, float: the native
   implementation vs PFFFT (Q_USE_PFFFT). A frame is what a phase vocoder
   does per hop: copy a frame in, forward, inverse. Times are the median of
   15 runs, in ns per frame. Build-only; not a CI test.

   Build and run (from the repo root, with PFFFT at $PFFFT):

      clang++ -O3 -std=c++20 -DQ_USE_PFFFT -Iq_lib/include -Iinfra/include \
         -I$PFFFT/include/pffft -I$PFFFT/src test/benchmark/fft_bench.cpp \
         test/benchmark/fft_bench_native.cpp -o /tmp/fft_bench
      /tmp/fft_bench
=============================================================================*/
#include "fft_bench_common.hpp"

#include <cstdio>

namespace fft_bench
{
   namespace native
   {
      frame_times times(std::size_t n);
   }

   frame_times times(std::size_t n)
   {
      return measure(n);
   }
}

int main()
{
#if defined(Q_USE_PFFFT)
   char const* other = "pffft";
#else
   char const* other = "native";
#endif
   std::printf("%6s %14s %14s %14s %14s\n", "N",
      "fft native", "fft", "rfft native", "rfft");
   std::printf("%6s %14s %14s %14s %14s\n", "",
      "", other, "", other);

   for (std::size_t n = 256; n <= 4096; n *= 2)
   {
      auto a = fft_bench::native::times(n);
      auto b = fft_bench::times(n);
      std::printf("%6zu %14.0f %14.0f %14.0f %14.0f\n",
         n, a.fft, b.fft, a.rfft, b.rfft);
   }
   std::printf("\n(sink %g)\n", fft_bench::sink);
}
