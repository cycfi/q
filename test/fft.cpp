/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/fft/fft.hpp>
#include <q_io/audio_file.hpp>

#include <iostream>
#include <iomanip>
#include <array>

namespace q = cycfi::q;
using namespace q::literals;

constexpr auto sps = 48000;

int main()
{
   constexpr std::size_t p = 4;

   // sample data
   constexpr std::size_t n = 1<<p;
   std::array<double, n*2> data;
   for (int i=0; i<n; ++i)
   {
      data[2*i] = 2*i;
      data[2*i+1] = 2*i+1;
   }

   // compute FFT
   q::fft<n>(data.data());

   // print the results
   std::cout<<"--------------------------------" << std::endl;
   for (int i = 0; i < n; ++i)
   {
      std::cout << std::setw(10) << std::setprecision(5) << data[2*i] << "\t"
         << data[2*i+1] << "I" << std::endl;
   }

   return 0;
}
