/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <q/literals.hpp>
#include <q/sfx.hpp>
#include <q/pitch_detector.hpp>
#include <q/notes.hpp>

#include <vector>
#include <iostream>
#include <boost/detail/lightweight_test.hpp>

namespace q = cycfi::q;
using namespace q::literals;
using namespace q::notes;
using std::fixed;

constexpr auto pi = q::pi;
constexpr auto sps = 44100;

// Set this to true if you want verbose print outs
constexpr auto verbose = true;

struct test_result
{
   float ave_error = 0.0f;
   float min_error = 100.0f;
   float max_error = 0.0f;
};

test_result process(
   std::vector<float>&& in
 , q::frequency actual_frequency
 , q::frequency lowest_freq
 , q::frequency highest_freq)
{
   std::cout << fixed << "Actual Frequency: " << double(actual_frequency) << std::endl;

   ////////////////////////////////////////////////////////////////////////////
   // Process
   q::pitch_detector<> pd{ lowest_freq, highest_freq, sps };
   auto const& bacf = pd.bacf();
   auto size = bacf.size();
   auto result = test_result{};
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

         result.ave_error += std::abs(error);
         ++frames;
         result.min_error = std::min<float>(result.min_error, std::abs(error));
         result.max_error = std::max<float>(result.max_error, std::abs(error));
      }
   }

   result.ave_error /= frames;
   return result;
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

void process(
   params const& params_
 , q::frequency actual_frequency
 , q::frequency lowest_freq
 , q::frequency highest_freq
 , float ave_error_expected
 , float min_error_expected
 , float max_error_expected
)
{
   auto result = process(
      gen_harmonics(actual_frequency, params_)
    , actual_frequency, lowest_freq, highest_freq
   );

   std::cout << fixed << "Average Error: " << result.ave_error << " cent(s)." << std::endl;
   std::cout << fixed << "Min Error: " << result.min_error << " cent(s)." << std::endl;
   std::cout << fixed << "Max Error: " << result.max_error << " cent(s)." << std::endl;

   BOOST_TEST(result.ave_error < ave_error_expected);
   BOOST_TEST(result.min_error < min_error_expected);
   BOOST_TEST(result.max_error < max_error_expected);
}

int main()
{
   auto low_e        = E[2];
   auto a            = A[2];
   auto d            = D[3];
   auto g            = G[3];
   auto b            = B[3];
   auto high_e       = E[4];

   auto low_e_12th   = E[3];
   auto a_12th       = A[3];
   auto d_12th       = D[4];
   auto g_12th       = G[4];
   auto b_12th       = B[4];
   auto high_e_12th  = E[5];

   auto low_e_24th   = E[4];
   auto a_24th       = A[4];
   auto d_24th       = D[5];
   auto g_24th       = G[5];
   auto b_24th       = B[5];
   auto high_e_24th  = E[6];

   auto middle_c     = C[4];

   params params_;
   // std::cout << "==================================================" << std::endl;
   // std::cout << " Test middle C" << std::endl;
   // std::cout << "==================================================" << std::endl;
   // process(params_, middle_c, 200_Hz, 1000_Hz, 0.004, 0.0001, 0.02);

   // std::cout << "==================================================" << std::endl;
   // std::cout << " Test middle A" << std::endl;
   // std::cout << "==================================================" << std::endl;
   // process(params_, 440_Hz, 200_Hz, 1000_Hz, 0.006, 0.0008, 0.03);

   std::cout << "==================================================" << std::endl;
   std::cout << " Test Low E" << std::endl;
   std::cout << "==================================================" << std::endl;
   process(params_, low_e, 70_Hz, 400_Hz, 0.00005, 00004, 0.0002);

   // std::cout << "==================================================" << std::endl;
   // std::cout << " Test E 12th" << std::endl;
   // std::cout << "==================================================" << std::endl;
   // process(params_, low_e_12th, 70_Hz, 400_Hz, 0.00009, 0.00004, 0.0003);

   // std::cout << "==================================================" << std::endl;
   // std::cout << " Test E 24th" << std::endl;
   // std::cout << "==================================================" << std::endl;
   // process(params_, low_e_24th, 70_Hz, 400_Hz, 0.0004, 0.00004, 0.001);

   // std::cout << "==================================================" << std::endl;
   // std::cout << " Test A" << std::endl;
   // std::cout << "==================================================" << std::endl;
   // process(params_, a, 100_Hz, 500_Hz, 0.00002, 0.000001, 0.0003);

   // std::cout << "==================================================" << std::endl;
   // std::cout << " Test A 12th" << std::endl;
   // std::cout << "==================================================" << std::endl;
   // process(params_, a_12th, 100_Hz, 500_Hz, 0.0003, 0.000001, 0.002);

   // std::cout << "==================================================" << std::endl;
   // std::cout << " Test A 24th" << std::endl;
   // std::cout << "==================================================" << std::endl;
   // process(params_, a_24th, 100_Hz, 500_Hz, 0.0007, 0.0002, 0.004);

   std::cout << "==================================================" << std::endl;
   std::cout << " Test D" << std::endl;
   std::cout << "==================================================" << std::endl;
   process(params_, d, 120_Hz, 600_Hz, 0.0007, 0.00003, 0.008);

   std::cout << "==================================================" << std::endl;
   std::cout << " Test D 12th" << std::endl;
   std::cout << "==================================================" << std::endl;
   process(params_, d_12th, 120_Hz, 600_Hz, 0.002, 0.00003, 0.006);

   // std::cout << "==================================================" << std::endl;
   // std::cout << " Test D 24th" << std::endl;
   // std::cout << "==================================================" << std::endl;
   // process(params_, d_24th, 120_Hz, 600_Hz, 0.006, 0.00003, 0.03);

   std::cout << "==================================================" << std::endl;
   std::cout << " Test G" << std::endl;
   std::cout << "==================================================" << std::endl;
   process(params_, g, 180_Hz, 800_Hz, 0.00007, 0.00007, 0.00007);

   std::cout << "==================================================" << std::endl;
   std::cout << " Test G 12th" << std::endl;
   std::cout << "==================================================" << std::endl;
   process(params_, g_12th, 180_Hz, 800_Hz, 0.00007, 0.00007, 0.00008);

   std::cout << "==================================================" << std::endl;
   std::cout << " Test D 24th" << std::endl;
   std::cout << "==================================================" << std::endl;
   process(params_, g_24th, 180_Hz, 800_Hz, 0.0002, 0.00007, 0.0004);

   // std::cout << "==================================================" << std::endl;
   // std::cout << " Test B" << std::endl;
   // std::cout << "==================================================" << std::endl;
   // process(params_, b, 230_Hz, 1000_Hz, 0.004, 0.0002,  0.03);

   std::cout << "==================================================" << std::endl;
   std::cout << " Test B 12th" << std::endl;
   std::cout << "==================================================" << std::endl;
   process(params_, b_12th, 230_Hz, 1000_Hz, 0.02, 0.0004, 0.06);

   // std::cout << "==================================================" << std::endl;
   // std::cout << " Test B 24th" << std::endl;
   // std::cout << "==================================================" << std::endl;
   // process(params_, b_24th, 230_Hz, 1200_Hz, 0.007, 0.0005, 0.02);

   // std::cout << "==================================================" << std::endl;
   // std::cout << " Test High E" << std::endl;
   // std::cout << "==================================================" << std::endl;
   // process(params_, high_e, 300_Hz, 1400_Hz, 0.004, 0.00004, 0.1);

   // std::cout << "==================================================" << std::endl;
   // std::cout << " Test High E 12th" << std::endl;
   // std::cout << "==================================================" << std::endl;
   // process(params_, high_e_12th, 300_Hz, 1400_Hz, 0.01, 0.0002, 0.03);

   std::cout << "==================================================" << std::endl;
   std::cout << " Test High E 24th" << std::endl;
   std::cout << "==================================================" << std::endl;
   process(params_, high_e_24th, 300_Hz, 1400_Hz, 0.04, 0.006, 0.09);

   std::cout << "==================================================" << std::endl;
   std::cout << " Non-integer harmonics test" << std::endl;
   std::cout << "==================================================" << std::endl;
   params_._2nd_harmonic = 2.003;
   process(params_, low_e, 70_Hz, 400_Hz, 1.0, 0.7, 1.2);
   params_ = params{};

   std::cout << "==================================================" << std::endl;
   std::cout << " Phase offsets test" << std::endl;
   std::cout << "==================================================" << std::endl;
   params_._1st_harmonic_offset = 0.1;
   params_._2nd_harmonic_offset = 0.5;
   params_._3rd_harmonic_offset = 0.4;
   process(params_, low_e, 70_Hz, 400_Hz, 0.0005, 0.00004, 0.001);
   params_ = params{};

   // std::cout << "==================================================" << std::endl;
   // std::cout << " Missing fundamental test" << std::endl;
   // std::cout << "==================================================" << std::endl;
   // params_._1st_level = 0.0;
   // params_._2nd_level = 0.5;
   // params_._3rd_level = 0.5;
   // process(params_, low_e, 70_Hz, 400_Hz, 0.002, 0.00004, 0.009);
   // params_ = params{};

   std::cout << "==================================================" << std::endl;
   return boost::report_errors();
}

