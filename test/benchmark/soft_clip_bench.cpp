/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]

   Micro-benchmark for the soft clippers / saturators:
   hard clip vs soft_clip (cubic) vs soft_clip2 (rational/Pade tanh) vs the raw
   fast_rational_tanh vs std::tanh vs the exp-based fasttanh. Build-only; not a
   CI test (nothing to assert, just measure and print). `accu` is summed and
   printed only to keep the compiler from eliding the loops.
=============================================================================*/
#include <q/fx/clip.hpp>
#include <q/support/base.hpp>

#include <cmath>
#include <chrono>
#include <iostream>

namespace q = cycfi::q;

template <typename F>
void bench(char const* name, F f, float& accu)
{
   constexpr int outer = 4096;
   constexpr int inner = 1024;
   auto start = std::chrono::high_resolution_clock::now();
   for (int j = 0; j < outer; ++j)
      for (int i = 0; i < inner; ++i)
         accu += f((i - 512) * (6.0f / 1024));      // sweep -3 .. +3

   auto elapsed = std::chrono::high_resolution_clock::now() - start;
   auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count();
   std::cout << name << ": " << float(ns) / (float(outer) * inner) << " ns/call\n";
}

int main()
{
   float accu = 0;
   q::clip       hard{};
   q::soft_clip  cubic{};
   q::soft_clip2 sc2{};

   bench("clip (hard)       ", [&](float x){ return hard(x); }, accu);
   bench("soft_clip (cubic) ", [&](float x){ return cubic(x); }, accu);
   bench("soft_clip2 (pade) ", [&](float x){ return sc2(x); }, accu);
   bench("fast_rational_tanh", [&](float x){ return q::fast_rational_tanh(x); }, accu);
   bench("std::tanh         ", [&](float x){ return std::tanh(x); }, accu);
   bench("fasttanh (exp)    ", [&](float x){ return fasttanh(x); }, accu);

   std::cout << "accu: " << accu << '\n';
   return 0;
}
