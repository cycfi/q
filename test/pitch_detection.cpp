/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <infra/doctest.hpp>

#include <q/literals.hpp>
#include <q/sfx.hpp>
#include <q/pitch_detector.hpp>
#include <q_io/audio_file.hpp>

#include <vector>
#include <iostream>
#include "notes.hpp"

namespace q = cycfi::q;
using namespace q::literals;
namespace audio_file = q::audio_file;
using std::fixed;

constexpr auto pi = q::pi;
constexpr auto sps = 44100;

// Set this to true if you want verbose print outs
constexpr auto verbose = false;

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
   if (verbose)
      std::cout << fixed << "Actual Frequency: "
      << double(actual_frequency) << std::endl;

   if (name.empty())
      name = std::to_string(int(double(actual_frequency)));

   auto max_val = *std::max_element(in.begin(), in.end(),
      [](auto a, auto b) { return std::abs(a) < std::abs(b); }
   );
   max_val = std::abs(max_val);

   ////////////////////////////////////////////////////////////////////////////
   // Process
   constexpr auto n_channels = 3;
   std::vector<float> out(in.size() * n_channels);

   q::pitch_detector<> pd{ lowest_freq, highest_freq, sps, 0.001 };
   auto const&       bacf = pd.bacf();
   q::edges const&   edges = bacf.edges();
   auto              size = bacf.size();
   auto              result = test_result{};
   auto              frames = 0;

   for (auto i = 0; i != in.size(); ++i)
   {
      auto s = in[i];

      // Generate for diagnostics
      auto pos = i * n_channels;
      out[pos] =  s * (1.0 / max_val);

      // Pitch Detection
      std::size_t extra;
      auto proc = pd(s, extra);

      // Default placeholders
      out[pos + 1] = -1;
      out[pos + 2] = -0.8;

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

         auto out_i = (&out[pos + 2] - ((size + extra) * n_channels));
         auto const& info = bacf.result();
         for (auto n : info.correlation)
         {
            *out_i = n / float(info.max_count);
            out_i += n_channels;
         }

         out_i = (&out[pos + 1] - ((size + extra) * n_channels));
         for (auto i = 0; i != size; ++i)
         {
            *out_i = bacf[i] * 0.8;
            out_i += n_channels;
         }
      }
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   auto wav = audio_file::writer{
      "results/pd_" + name + ".wav", n_channels, sps
   };
   wav.write(out);

   result.ave_error /= frames;
   return result;
}

struct params
{
   float _2nd_harmonic = 2;      // Second harmonic multiple
   float _3rd_harmonic = 3;      // Second harmonic multiple
   float _1st_level = 0.3;       // Fundamental level
   float _2nd_level = 0.4;       // Second harmonic level
   float _3rd_level = 0.3;       // Third harmonic level
   float _1st_offset = 0.0;      // Fundamental phase offset
   float _2nd_offset = 0.0;      // Second harmonic phase offset
   float _3rd_offset = 0.0;      // Third harmonic phase offset
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
         * std::sin(2 * pi * (angle + params_._1st_offset));
      signal[i] += params_._2nd_level
         * std::sin(params_._2nd_harmonic * 2 * pi * (angle + params_._2nd_offset));
      signal[i] += params_._3rd_level
         * std::sin(params_._3rd_harmonic * 2 * pi * (angle + params_._3rd_offset));
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
 , std::string name = ""
)
{
   auto result = process(
      gen_harmonics(actual_frequency, params_)
    , actual_frequency, lowest_freq, highest_freq, name
   );

   if (verbose)
   {
      std::cout << fixed << "Average Error: " << result.ave_error << " cent(s)." << std::endl;
      std::cout << fixed << "Min Error:     " << result.min_error << " cent(s)." << std::endl;
      std::cout << fixed << "Max Error:     " << result.max_error << " cent(s)." << std::endl;
   }

   CHECK_MESSAGE(result.ave_error < ave_error_expected,
      "Average error exceeded: " << result.ave_error << " expecting: " << ave_error_expected);
   CHECK_MESSAGE(result.min_error < min_error_expected,
      "Minimum error exceeded: " << result.min_error << " expecting: " << min_error_expected);
   CHECK_MESSAGE(result.max_error < max_error_expected,
      "Maximum error exceeded: " << result.max_error << " expecting: " << max_error_expected);
}

void process(
   params const& params_
 , q::frequency actual_frequency
 , q::frequency lowest_freq
 , float ave_error_expected
 , float min_error_expected
 , float max_error_expected
 , std::string name = ""
)
{
   process(
      params_, actual_frequency, lowest_freq * 0.8, lowest_freq * 5
    , ave_error_expected, min_error_expected, max_error_expected, name
   );
}


using namespace notes;
params params_;

TEST_CASE("Test_middle_C")
{
   process(params_, middle_c, 200_Hz, 0.0011, 0.000097, 0.0024);
}

TEST_CASE("Test_middle_A")
{
   process(params_, 440_Hz, 200_Hz, 0.002, 0.0000001, 0.008);
}

TEST_CASE("Test_low_E")
{
   process(params_, low_e, low_e, 0.000035, 000035, 0.000035);
}

TEST_CASE("Test_E_12th")
{
   process(params_, low_e_12th, low_e, 0.00022, 0.000035, 0.00052);
}

TEST_CASE("Test_E_24th")
{
   process(params_, low_e_24th, low_e, 0.00023, 0.000035, 0.00052, "low_e_24th");
}

TEST_CASE("Test_A")
{
   process(params_, a, a, 0.000001, 0.000001, 0.000001);
}

TEST_CASE("Test_A_12th")
{
   process(params_, a_12th, a, 0.00019, 0.000001, 0.00073);
}

TEST_CASE("Test_A_24th")
{
   process(params_, a_24th, a, 0.00048, 0.000001, 0.0011);
}

TEST_CASE("Test_D")
{
   process(params_, d, d, 0.00018, 0.000022, 0.00034);
}

TEST_CASE("Test_D_12th")
{
   process(params_, d_12th, d, 0.00058, 0.000022, 0.0017);
}

TEST_CASE("Test_D_24th")
{
   process(params_, d_24th, d, 0.0060, 0.0015, 0.0094);
}

TEST_CASE("Test_G")
{
   process(params_, g, g, 0.000061, 0.000061, 0.000061);
}

TEST_CASE("Test_G_12th")
{
   process(params_, g_12th, g, 0.00018, 0.000061, 0.00021);
}

TEST_CASE("Test_G_24th")
{
   process(params_, g_24th, g, 0.00015, 0.000061, 0.0047);
}

TEST_CASE("Test_B")
{
   process(params_, b, b, 0.00053, 0.000003,  0.0015);
}

TEST_CASE("Test_B_12th")
{
   process(params_, b_12th, b, 0.0068, 0.00011, 0.012);
}

TEST_CASE("Test_B_24th")
{
   process(params_, b_24th, b, 0.0035, 0.00065, 0.0076);
}

TEST_CASE("Test_high_E")
{
   process(params_, high_e, high_e, 0.00068, 0.000035, 0.0021);
}

TEST_CASE("Test_high_E_12th")
{
   process(params_, high_e_12th, high_e, 0.0034, 0.000035, 0.0095);
}

TEST_CASE("Test_high_E_24th")
{
   process(params_, high_e_24th, high_e, 0.027, 0.000035, 0.044);
}

TEST_CASE("Test_non_integer_harmonics")
{
   params_._2nd_harmonic = 2.003;
   process(params_, low_e, low_e, 1.1, 0.95, 1.1, "non_integer");
   params_ = params{};
}

TEST_CASE("Test_phase_offsets")
{
   params_._1st_offset = 0.1;
   params_._2nd_offset = 0.5;
   params_._3rd_offset = 0.4;
   process(params_, low_e, low_e, 0.00014, 0.000035, 0.00036, "phase_offset");
   params_ = params{};
}

TEST_CASE("Test_missing_fundamental")
{
   params_._1st_level = 0.0;
   params_._2nd_level = 0.5;
   params_._3rd_level = 0.5;
   process(params_, low_e, low_e, 0.000035, 0.000035, 0.000035, "missing_fundamental");
   params_ = params{};
}



