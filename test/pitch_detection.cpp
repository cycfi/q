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
            << " cent(s)."
            << std::endl
         ;

         ave_error += error;
         ++frames;
         min_error = std::min<float>(min_error, std::abs(error));
         max_error = std::max<float>(max_error, std::abs(error));
      }
   }

   ave_error /= frames;
   std::cout << fixed << "Average Error: " << ave_error << " cent(s)." << std::endl;
   std::cout << fixed << "Min Error: " << min_error << " cent(s)." << std::endl;
   std::cout << fixed << "Max Error: " << max_error << " cent(s)." << std::endl;
   std::cout << "--------------------------------------------------" << std::endl;
}

struct params
{
   float _2nd_harmonic = 2;            // Second harmonic multiple
   float _3rd_harmonic = 3;            // Second harmonic multiple
   float _1st_level = 0.3;             // Fundamental level
   float _2nd_level = 0.4;             // Second harmonic level
   float _3rd_level = 0.3;             // Third harmonic level
   float _1st_harmonic_offset = 0.0;   // Fundamental phase offset
   float _2nd_harmonic_offset = 0.0;   // Second harmonic phase offset
   float _3rd_harmonic_offset = 0.0;   // Third harmonic phase offset
};

std::vector<float>
gen_harmonics(q::frequency freq, params const& params_)
{
   auto period = double(sps / freq);
   constexpr float offset = 100;
   std::size_t buff_size = 10000;

   std::vector<float> signal(buff_size);
   for (int i = 0; i < buff_size; i++)
   {
      auto angle = (i + offset) / period;
      signal[i] += params_._1st_level
         * std::sin(2 * pi * (angle + params_._1st_harmonic_offset));
      signal[i] += params_._2nd_level
         * std::sin(params_._2nd_harmonic * 2 * pi * (angle + params_._2nd_harmonic_offset));
      signal[i] += params_._3rd_level
         * std::sin(params_._3rd_harmonic * 2 * pi * (angle + params_._3rd_harmonic_offset));
   }
   return signal;
}

int main()
{
   params params_;
   std::cout << "=====================" << std::endl;
   std::cout << " Basic test middle C" << std::endl;
   std::cout << "=====================" << std::endl;
   process(gen_harmonics(261.626_Hz, params_), 261.626_Hz, 200_Hz, 1000_Hz);

   std::cout << "==================" << std::endl;
   std::cout << " Basic test Low E" << std::endl;
   std::cout << "==================" << std::endl;
   process(gen_harmonics(82.41_Hz, params_), 82.41_Hz, 70_Hz, 400_Hz);

   std::cout << "============================" << std::endl;
   std::cout << " Non-integer harmonics test" << std::endl;
   std::cout << "============================" << std::endl;
   params_._2nd_harmonic = 2.003;
   process(gen_harmonics(82.41_Hz, params_), 82.41_Hz, 70_Hz, 400_Hz);
   params_ = params{};

   std::cout << "====================" << std::endl;
   std::cout << " Phase offsets test" << std::endl;
   std::cout << "====================" << std::endl;
   params_._1st_harmonic_offset = 0.1;
   params_._2nd_harmonic_offset = 0.5;
   params_._3rd_harmonic_offset = 0.4;
   process(gen_harmonics(82.41_Hz, params_), 82.41_Hz, 70_Hz, 400_Hz);
   params_ = params{};

   return 0;
}

