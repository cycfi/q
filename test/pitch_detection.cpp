/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <q/literals.hpp>
#include <q/sfx.hpp>
#include <q/pitch_detector.hpp>
#include <q/pitch_detector.hpp>
#include <q/notes.hpp>

#include <vector>
#include <iostream>

namespace q = cycfi::q;
using namespace q::literals;

constexpr auto pi = q::pi;
constexpr auto sps = 44100;

// Set this to true if you want verbose print outs
constexpr auto verbose = false;

void process(
   std::vector<float>&& in
 , q::frequency actual_frequency
 , q::frequency lowest_freq
 , q::frequency highest_freq)
{
   using std::fixed;
   std::cout << fixed << "Actual Frequency: " << double(actual_frequency) << std::endl;

   ////////////////////////////////////////////////////////////////////////////
   // Process
   q::pitch_detector<> pd{ lowest_freq, highest_freq, sps };
   auto const& bacf = pd.bacf();
   auto size = bacf.size();
   auto ave_error = 0.0f;
   auto min_error = 100.0f;
   auto max_error = 0.0f;
   auto frames = 0;

   for (auto i = 0; i != in.size(); ++i)
   {
      auto s = in[i];

      // Pitch Detection
      bool proc = pd(s);

      if (proc)
      {
         auto frequency = pd.frequency();

         auto error = 1200.0 * std::log2(frequency / double(actual_frequency));
         if (verbose)
         {
            std::cout
               << fixed
               << frequency
               << " Error: "
               << error
               << " cent(s)."
               << std::endl
            ;
         }

         ave_error += std::abs(error);
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
   std::size_t buff_size = sps; // 1 second

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
   auto low_e = q::note[2].e;
   auto a = q::note[2].a;
   auto d = q::note[3].d;
   auto g = q::note[3].g;
   auto b = q::note[3].b;
   auto high_e = q::note[4].e;

   auto low_e_12th = q::note[3].e;
   auto a_12th = q::note[3].a;
   auto d_12th = q::note[4].d;
   auto g_12th = q::note[4].g;
   auto b_12th = q::note[4].b;
   auto high_e_12th = q::note[5].e;

   auto low_e_24th = q::note[4].e;
   auto a_24th = q::note[4].a;
   auto d_24th = q::note[5].d;
   auto g_24th = q::note[5].g;
   auto b_24th = q::note[5].b;
   auto high_e_24th = q::note[6].e;

   auto middle_c = q::note[4].c;

   params params_;
   std::cout << "==================================================" << std::endl;
   std::cout << " Test middle C" << std::endl;
   std::cout << "==================================================" << std::endl;
   process(gen_harmonics(middle_c, params_), middle_c, 200_Hz, 1000_Hz);

   std::cout << "==================================================" << std::endl;
   std::cout << " Test middle A" << std::endl;
   std::cout << "==================================================" << std::endl;
   process(gen_harmonics(440_Hz, params_), 440_Hz, 200_Hz, 1000_Hz);

   std::cout << "==================================================" << std::endl;
   std::cout << " Test Low E" << std::endl;
   std::cout << "==================================================" << std::endl;
   process(gen_harmonics(low_e, params_), low_e, 70_Hz, 400_Hz);

   std::cout << "==================================================" << std::endl;
   std::cout << " Test E 12th" << std::endl;
   std::cout << "==================================================" << std::endl;
   process(gen_harmonics(low_e_12th, params_), low_e_12th, 70_Hz, 400_Hz);

   std::cout << "==================================================" << std::endl;
   std::cout << " Test E 24th" << std::endl;
   std::cout << "==================================================" << std::endl;
   process(gen_harmonics(low_e_24th, params_), low_e_24th, 70_Hz, 400_Hz);

   std::cout << "==================================================" << std::endl;
   std::cout << " Test A" << std::endl;
   std::cout << "==================================================" << std::endl;
   process(gen_harmonics(a, params_), a, 100_Hz, 500_Hz);

   std::cout << "==================================================" << std::endl;
   std::cout << " Test A 12th" << std::endl;
   std::cout << "==================================================" << std::endl;
   process(gen_harmonics(a_12th, params_), a_12th, 100_Hz, 500_Hz);

   std::cout << "==================================================" << std::endl;
   std::cout << " Test A 24th" << std::endl;
   std::cout << "==================================================" << std::endl;
   process(gen_harmonics(a_24th, params_), a_24th, 100_Hz, 500_Hz);

   std::cout << "==================================================" << std::endl;
   std::cout << " Test D" << std::endl;
   std::cout << "==================================================" << std::endl;
   process(gen_harmonics(d, params_), d, 120_Hz, 600_Hz);

   std::cout << "==================================================" << std::endl;
   std::cout << " Test D 12th" << std::endl;
   std::cout << "==================================================" << std::endl;
   process(gen_harmonics(d_12th, params_), d_12th, 120_Hz, 600_Hz);

   std::cout << "==================================================" << std::endl;
   std::cout << " Test D 24th" << std::endl;
   std::cout << "==================================================" << std::endl;
   process(gen_harmonics(d_24th, params_), d_24th, 120_Hz, 600_Hz);

   std::cout << "==================================================" << std::endl;
   std::cout << " Test G" << std::endl;
   std::cout << "==================================================" << std::endl;
   process(gen_harmonics(g, params_), g, 180_Hz, 800_Hz);

   std::cout << "==================================================" << std::endl;
   std::cout << " Test G 12th" << std::endl;
   std::cout << "==================================================" << std::endl;
   process(gen_harmonics(g_12th, params_), g_12th, 180_Hz, 800_Hz);

   std::cout << "==================================================" << std::endl;
   std::cout << " Test D 24th" << std::endl;
   std::cout << "==================================================" << std::endl;
   process(gen_harmonics(g_24th, params_), g_24th, 180_Hz, 800_Hz);

   std::cout << "==================================================" << std::endl;
   std::cout << " Test B" << std::endl;
   std::cout << "==================================================" << std::endl;
   process(gen_harmonics(b, params_), b, 230_Hz, 1000_Hz);

   std::cout << "==================================================" << std::endl;
   std::cout << " Test B 12th" << std::endl;
   std::cout << "==================================================" << std::endl;
   process(gen_harmonics(b_12th, params_), b_12th, 230_Hz, 1000_Hz);

   std::cout << "==================================================" << std::endl;
   std::cout << " Test B 24th" << std::endl;
   std::cout << "==================================================" << std::endl;
   process(gen_harmonics(b_24th, params_), b_24th, 230_Hz, 1200_Hz);

   std::cout << "==================================================" << std::endl;
   std::cout << " Test High E" << std::endl;
   std::cout << "==================================================" << std::endl;
   process(gen_harmonics(high_e, params_), high_e, 300_Hz, 1400_Hz);

   std::cout << "==================================================" << std::endl;
   std::cout << " Test High E 12th" << std::endl;
   std::cout << "==================================================" << std::endl;
   process(gen_harmonics(high_e_12th, params_), high_e_12th, 300_Hz, 1400_Hz);

   std::cout << "==================================================" << std::endl;
   std::cout << " Test High E 24th" << std::endl;
   std::cout << "==================================================" << std::endl;
   process(gen_harmonics(high_e_24th, params_), high_e_24th, 300_Hz, 1400_Hz);

   std::cout << "==================================================" << std::endl;
   std::cout << " Non-integer harmonics test" << std::endl;
   std::cout << "==================================================" << std::endl;
   params_._2nd_harmonic = 2.003;
   process(gen_harmonics(low_e, params_), low_e, 70_Hz, 400_Hz);
   params_ = params{};

   std::cout << "==================================================" << std::endl;
   std::cout << " Phase offsets test" << std::endl;
   std::cout << "==================================================" << std::endl;
   params_._1st_harmonic_offset = 0.1;
   params_._2nd_harmonic_offset = 0.5;
   params_._3rd_harmonic_offset = 0.4;
   process(gen_harmonics(low_e, params_), low_e, 70_Hz, 400_Hz);
   params_ = params{};

   std::cout << "==================================================" << std::endl;
   std::cout << " Missing fundamental test" << std::endl;
   std::cout << "==================================================" << std::endl;
   params_._1st_level = 0.0;
   params_._2nd_level = 0.5;
   params_._3rd_level = 0.5;
   process(gen_harmonics(low_e, params_), low_e, 70_Hz, 400_Hz);
   params_ = params{};

   return 0;
}

