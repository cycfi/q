/*=============================================================================
   Copyright (c) 2014-2020 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>

#include <q/support/literals.hpp>
#include <q/pitch/pitch_detector.hpp>

#include <vector>
#include <iostream>
#include "notes.hpp"

namespace q = cycfi::q;
using namespace q::literals;
using std::fixed;

constexpr auto pi = q::pi;
constexpr auto sps = 44100;

// Set this to 1 or 2 if you want verbose print outs
constexpr auto verbosity = 0;

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
 , q::frequency highest_freq
 , std::string name = "")
{
   if (verbosity > 1)
      std::cout << fixed << "Actual Frequency: "
      << double(actual_frequency) << std::endl;

   if (name.empty())
      name = std::to_string(int(double(actual_frequency)));

   ////////////////////////////////////////////////////////////////////////////
   // Process

   q::pitch_detector pd(lowest_freq, highest_freq, sps, -45_dB);
   auto result = test_result{};
   auto frames = 0;

   for (auto i = 0; i != in.size(); ++i)
   {
      auto s = in[i];

      // Period Detection
      bool is_ready = pd(s);

      if (is_ready)
      {
         auto frequency = pd.get_frequency();
         if (frequency != 0.0f)
         {
            auto error = 1200.0 * std::log2(frequency / double(actual_frequency));
            if (verbosity > 1)
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
   }

   result.ave_error /= frames;
   return result;
}

struct params
{
   float _offset = 0.0f;         // Waveform offset
   float _2nd_harmonic = 2.0f;   // Second harmonic multiple
   float _3rd_harmonic = 3.0f;   // Second harmonic multiple
   float _1st_level = 0.3f;      // Fundamental level
   float _2nd_level = 0.4f;      // Second harmonic level
   float _3rd_level = 0.3f;      // Third harmonic level
   float _1st_offset = 0.0f;     // Fundamental phase offset
   float _2nd_offset = 0.0f;     // Second harmonic phase offset
   float _3rd_offset = 0.0f;     // Third harmonic phase offset
};

std::vector<float>
gen_harmonics(q::frequency freq, params const& params_)
{
   auto period = double(sps / freq);
   float offset = params_._offset;
   std::size_t buff_size = sps; // 1 second

   std::vector<float> signal(buff_size);
   for (int i = 0; i < buff_size; i++)
   {
      auto angle = (i + offset) / period;
      signal[i] += params_._1st_level
         * std::sin(2 * pi * (angle + params_._1st_offset));
      signal[i] += params_._2nd_level
         * std::sin(params_._2nd_harmonic * 2 * pi * (angle + params_._2nd_offset));
      signal[i] += params_._3rd_level
         * std::sin(params_._3rd_harmonic * 2 * pi * (angle + params_._3rd_offset));
   }
   return signal;
}

float max_error = 0.01;   // 1% error or error

void check(float x, float expected, char const* what)
{
   if (x == 0 && expected == 0)
      return;

   auto error_percent = max_error * 100;
   auto error_threshold = expected * max_error;

   {
      INFO(
         what
         << " exceeded "
         << error_percent
         << "%. Got: "
         << x
         << ",  Expecting: ("
         << (expected - error_threshold)
         << "..."
         << (expected + error_threshold)
         << ')'
      );

      CHECK(x < (expected + error_threshold));
   }
}

void process(
   params const& params_
 , q::frequency actual_frequency
 , q::frequency lowest_freq
 , q::frequency highest_freq
 , double ave_error_expected
 , double min_error_expected
 , double max_error_expected
 , std::string name = ""
)
{
   auto result = process(
      gen_harmonics(actual_frequency, params_)
    , actual_frequency, lowest_freq, highest_freq, name
   );

   if (verbosity > 0)
   {
      std::cout << fixed << "Average Error: " << result.ave_error << " cent(s)." << std::endl;
      std::cout << fixed << "Min Error:     " << result.min_error << " cent(s)." << std::endl;
      std::cout << fixed << "Max Error:     " << result.max_error << " cent(s)." << std::endl;
   }

   check(result.ave_error, ave_error_expected, "Average error");
   check(result.min_error, min_error_expected, "Minimum error");
   check(result.max_error, max_error_expected, "Maximum error");
}

void process(
   params const& params_
 , q::frequency actual_frequency
 , q::frequency lowest_freq
 , double ave_error_expected = 0.01
 , double min_error_expected = 0.01
 , double max_error_expected = 0.02
 , std::string name = ""
)
{
   process(
      params_, actual_frequency, lowest_freq * 0.8, lowest_freq * 5
    , ave_error_expected, min_error_expected, max_error_expected, name
   );
}

void process(
  params const& params_
 , q::frequency actual_frequency
 , q::frequency lowest_freq
 , std::string name
)
{
   process(
           params_, actual_frequency, lowest_freq * 0.8, lowest_freq * 5
           , 0.01, 0.01, 0.02, name
   );
}


using namespace notes;

TEST_CASE("Test_middle_C")
{
   process(params{}, middle_c, 200_Hz);
}

TEST_CASE("Test_middle_A")
{
   process(params{}, 440_Hz, 200_Hz);
}

TEST_CASE("Test_low_E")
{
   process(params{}, low_e, low_e);
}

TEST_CASE("Test_E_12th")
{
   process(params{}, low_e_12th, low_e);
}

TEST_CASE("Test_E_24th")
{
   process(params{}, low_e_24th, low_e, "low_e_24th");
}

TEST_CASE("Test_A")
{
   process(params{}, a, a);
}

TEST_CASE("Test_A_12th")
{
   process(params{}, a_12th, a);
}

TEST_CASE("Test_A_24th")
{
   process(params{}, a_24th, a);
}

TEST_CASE("Test_D")
{
   process(params{}, d, d);
}

TEST_CASE("Test_D_12th")
{
   process(params{}, d_12th, d);
}

TEST_CASE("Test_D_24th")
{
   process(params{}, d_24th, d);
}

TEST_CASE("Test_G")
{
   process(params{}, g, g);
}

TEST_CASE("Test_G_12th")
{
   process(params{}, g_12th, g);
}

TEST_CASE("Test_G_24th")
{
   process(params{}, g_24th, g, 0.01517, 0.01, 0.0343084);
}

TEST_CASE("Test_B")
{
   process(params{}, b, b);
}

TEST_CASE("Test_B_12th")
{
   process(params{}, b_12th, b);
}

TEST_CASE("Test_B_24th")
{
   process(params{}, b_24th, b, 0.011, 0.01, 0.11);
}

TEST_CASE("Test_high_E")
{
   process(params{}, high_e, high_e);
}

TEST_CASE("Test_high_E_12th")
{
   process(params{}, high_e_12th, high_e);
}

TEST_CASE("Test_high_E_24th")
{
   process(params{}, high_e_24th, high_e, 0.035, 0.01, 0.07);
}

TEST_CASE("Test_non_integer_harmonics")
{
   params params_;
   params_._2nd_harmonic = 2.003;
   process(params_, low_e, low_e, 1.025, 0.951, 1.18, "non_integer");
}

TEST_CASE("Test_phase_offsets")
{
   params params_;
   params_._1st_offset = 0.1;
   params_._2nd_offset = 0.5;
   params_._3rd_offset = 0.4;
   process(params_, low_e, low_e, "phase_offset");
}

TEST_CASE("Test_missing_fundamental")
{
   params params_;
   params_._1st_level = 0.0;
   params_._2nd_level = 0.5;
   params_._3rd_level = 0.5;
   process(params_, low_e, low_e, "missing_fundamental");
}



