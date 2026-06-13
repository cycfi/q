/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]

   Micro-benchmark for sine: q::sin_lu vs std::sin vs q::fast_sin vs
   q::faster_sin. Build-only; not a CI test (there is nothing to assert,
   just measure and print). `accu` is summed and printed only to keep the
   compiler from eliding the loops.

   Build and run (from the repo root):

      clang++ -O3 -std=c++20 -Iq_lib/include -Iinfra/include \
         test/benchmark/sin_bench.cpp -o /tmp/sin_bench
      /tmp/sin_bench
=============================================================================*/
#include <q/detail/sin_table.hpp>
#include <q/support/literals.hpp>

#include <cmath>
#include <iostream>
#include <chrono>

namespace q = cycfi::q;
using namespace q::literals;

int main()
{
   // Summed and printed at the end to prevent dead-code elimination.
   float accu = 0;
   {
      auto start = std::chrono::high_resolution_clock::now();

      for (int j = 0; j < 1024; ++j)
      {
         for (std::uint32_t i = 0; i < 1024; ++i)
         {
            auto result = q::sin_lu(q::phase(i*4194304));
            accu += result;
         }
      }

      auto elapsed = std::chrono::high_resolution_clock::now() - start;
      auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed);

      std::cout << "q::sin_lu(a) elapsed (ns): " << float(duration.count()) / (1024*1024) << std::endl;
   }

   {
      auto start = std::chrono::high_resolution_clock::now();

      for (int j = 0; j < 1024; ++j)
      {
         for (std::uint32_t i = 0; i < 1024; ++i)
         {
            auto a = 2_pi * float(i) / 1024;
            auto result = std::sin(a);
            accu += result;
         }
      }

      auto elapsed = std::chrono::high_resolution_clock::now() - start;
      auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed);

      std::cout << "std::sin(a) elapsed (ns): " << float(duration.count()) / (1024*1024) << std::endl;
   }

   {
      auto start = std::chrono::high_resolution_clock::now();

      for (int j = 0; j < 1024; ++j)
      {
         for (std::uint32_t i = 0; i < 1024; ++i)
         {
            auto a = 2_pi * float(i) / 1024;
            auto result = q::fast_sin(a);
            accu += result;
         }
      }

      auto elapsed = std::chrono::high_resolution_clock::now() - start;
      auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed);

      std::cout << "q::fast_sin elapsed (ns): " << float(duration.count()) / (1024*1024) << std::endl;
   }

   {
      auto start = std::chrono::high_resolution_clock::now();

      for (int j = 0; j < 1024; ++j)
      {
         for (std::uint32_t i = 0; i < 1024; ++i)
         {
            auto a = 2_pi * float(i) / 1024;
            auto result = q::faster_sin(a);
            accu += result;
         }
      }

      auto elapsed = std::chrono::high_resolution_clock::now() - start;
      auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed);

      std::cout << "q::faster_sin elapsed (ns): " << float(duration.count()) / (1024*1024) << std::endl;
   }

   std::cout << "accu: " << accu << std::endl;
   return 0;
}
