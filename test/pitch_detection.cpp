/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <q/literals.hpp>
#include <q/sfx.hpp>
#include <q/pitch_detector.hpp>
#include <q/pitch_detector.hpp>
#include <vector>
#include <iostream>

namespace q = cycfi::q;
using namespace q::literals;

constexpr auto pi = q::pi;
constexpr auto sps = 44100;

void process(
   std::vector<float>&& in
 , q::frequency actual_frequency
 , q::frequency lowest_freq
 , q::frequency highest_freq)
{
   ////////////////////////////////////////////////////////////////////////////
   // Process
   q::pitch_detector<> pd{ lowest_freq, highest_freq, sps };
   auto const& bacf = pd.bacf();
   auto size = bacf.size();
   auto ave_error = 0.0f;
   auto min_error = 100.0f;
   auto max_error = 0.0f;
   auto frames = 0;
   using std::fixed;

   for (auto i = 0; i != in.size(); ++i)
   {
      auto s = in[i];

      // Pitch Detection
      bool proc = pd(s);

      if (proc)
      {
         auto frequency = pd.frequency();
         auto error = 1200.0 * std::log2(frequency / double(actual_frequency));
         std::cout
            << fixed
            << frequency
            << " Error: "
            << error
            << " cents."
            << std::endl
         ;

         ave_error += error;
         ++frames;
         min_error = std::min<float>(min_error, std::abs(error));
         max_error = std::max<float>(max_error, std::abs(error));
      }
   }

   ave_error /= frames;
   std::cout << fixed << "Average Error: " << ave_error << " cents." << std::endl;
   std::cout << fixed << "Min Error: " << min_error << " cents." << std::endl;
   std::cout << fixed << "Max Error: " << max_error << " cents." << std::endl;
}

std::vector<float> harmonics(q::frequency freq)
{
   constexpr float _1st_level = 0.3;   // Fundamental level
   constexpr float _2nd_level = 0.4;   // Second harmonic level
   constexpr float _3rd_level = 0.3;   // Third harmonic level

   auto period = double(sps / freq);
   constexpr float offset = 100;
   std::size_t buff_size = 10000;

   std::vector<float> signal(buff_size);
   for (int i = 0; i < buff_size; i++)
   {
      auto angle = (i + offset) / period;
      signal[i] += _1st_level * std::sin(2 * pi * angle);   // First harmonic
      signal[i] += _2nd_level * std::sin(4 * pi * angle);   // Second harmonic
      signal[i] += _3rd_level * std::sin(6 * pi * angle);   // Third harmonic
   }
   return signal;
}

int main()
{
   process(harmonics(261.626_Hz), 261.626_Hz, 200_Hz, 1000_Hz);
   return 0;
}

