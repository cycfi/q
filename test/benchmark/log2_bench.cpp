/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]

   Micro-benchmark for log2: std::log2 vs q::fast_log2 vs q::faster_log2.
   Build-only; not a CI test (there is nothing to assert, just measure and
   print). `accu` is summed and printed only to keep the compiler from
   eliding the loops.

   Build and run (from the repo root):

      clang++ -O3 -std=c++20 -Iq_lib/include -Iinfra/include \
         test/benchmark/log2_bench.cpp -o /tmp/log2_bench
      /tmp/log2_bench
=============================================================================*/
#include <q/support/base.hpp>

#include <cmath>
#include <chrono>
#include <iostream>

namespace q = cycfi::q;

int main()
{
   // Summed and printed at the end to prevent dead-code elimination.
   float accu = 0;

   {
      auto start = std::chrono::high_resolution_clock::now();

      for (int j = 0; j < 1024; ++j)
      {
         for (int i = 1; i < 1024; ++i)
         {
            auto a = float(i);
            auto result = std::log2(a);
            accu += result;
         }
      }

      auto elapsed = std::chrono::high_resolution_clock::now() - start;
      auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed);

      std::cout << "std::log2(a) (ns): " << float(duration.count()) / (1024*1023) << std::endl;
   }

   {
      auto start = std::chrono::high_resolution_clock::now();
      using cycfi::q::fast_log2;

      for (int j = 0; j < 1024; ++j)
      {
         for (int i = 1; i < 1024; ++i)
         {
            auto a = float(i);
            auto result = fast_log2(a);
            accu += result;
         }
      }

      auto elapsed = std::chrono::high_resolution_clock::now() - start;
      auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed);

      std::cout << "fast_log2(a) elapsed (ns): " << float(duration.count()) / (1024*1023) << std::endl;
   }

   {
      auto start = std::chrono::high_resolution_clock::now();
      using cycfi::q::faster_log2;

      for (int j = 0; j < 1024; ++j)
      {
         for (int i = 1; i < 1024; ++i)
         {
            auto a = float(i);
            auto result = faster_log2(a);
            accu += result;
         }
      }

      auto elapsed = std::chrono::high_resolution_clock::now() - start;
      auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed);

      std::cout << "faster_log2(a) elapsed (ns): " << float(duration.count()) / (1024*1023) << std::endl;
   }

   std::cout << "accu: " << accu << std::endl;
   return 0;
}
