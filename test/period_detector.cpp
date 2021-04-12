/*=============================================================================
   Copyright (c) 2014-2021 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>

#include <q/support/literals.hpp>
#include <q/pitch/period_detector.hpp>
#include <q_io/audio_file.hpp>

#include <vector>
#include <iostream>
#include <tuple>
#include <iostream>
#include "notes.hpp"

namespace q = cycfi::q;
using namespace q::literals;

constexpr auto pi = q::pi;
constexpr auto sps = 44100;

struct result_type
{
   float                      predicted_period;
   q::period_detector::info   info;
   float                      harmonic;
};

result_type process(
   std::vector<float>&& in
 , q::frequency actual_frequency
 , q::frequency lowest_freq
 , q::frequency highest_freq
 , std::string name
 , bool allow_harmonics = false
 , double epsilon = 0.0001)
{

   std::cout << "=========== Test " << name << " ===========" << std::endl;

   result_type result;
   constexpr auto n_channels = 3;
   std::vector<float> out(in.size() * n_channels);
   result.predicted_period = -1.0f;

   q::period_detector   pd(lowest_freq, highest_freq, sps, -30_dB);
   auto const&          bits = pd.bits();
   auto const&          edges = pd.edges();

   q::bitstream_acf<>   bacf{ bits };
   auto                 min_period = float(highest_freq.period()) * sps;

   float y = 0.15;
   bool first = true;

   for (auto i = 0; i != in.size(); ++i)
   {
      auto pos = i * n_channels;
      auto ch1 = pos;
      auto ch2 = pos+1;
      auto ch3 = pos+2;

      auto s = in[i];
      out[ch1] = s;

      // Detect period
      bool is_ready = pd(s);

      out[ch2] = -1;    // placeholder for bitset bits
      out[ch3] = 0.0f;  // placeholder for autocorrelation result

      if (is_ready)
      {
         if (first)
         {
            first = false;
            result.info = pd.fundamental();
            result.harmonic = pd.harmonic(2);
         }
         else
         {
            auto a = result.info._period;
            auto b = pd.fundamental()._period;

            if (allow_harmonics)
            {
               // Allow 2nd and 3rd harmonics
               CHECK((a == Approx(b)
                  || (a * 2) == Approx(b).epsilon(epsilon)
                  || (a * 3) == Approx(b).epsilon(epsilon)
                  || a == Approx(b * 2).epsilon(epsilon)
                  || a == Approx(b * 3).epsilon(epsilon)
               ));
            }
            else
            {
               if (!q::rel_within(a, b, epsilon))
               {
                  std::cout <<
                     "Warning: In test: " << name << ", "
                     << "error exceeded "
                     << epsilon * 100
                     << "%. Got: "
                     << b
                     << ",  Expecting: "
                     << a
                     << " (i = " << i << ')'
                     << std::endl;
               }
            }
         }

         auto frame = edges.frame() + (edges.window_size() / 2);
         auto extra = frame - edges.window_size();
         auto size = bits.size();

         // Print the bitset bits
         {
            auto out_i = (&out[ch2] - (((size-1) + extra) * n_channels));
            for (auto i = 0; i != size; ++i)
            {
               *out_i = bits.get(i) * 0.8;
               out_i += n_channels;
            }
         }

         // Print the autocorrelation results
         {
            auto weight = 2.0 / size;
            auto out_i = (&out[ch3] - (((size-1) + extra) * n_channels));
            for (auto i = 0; i != size/2; ++i)
            {
               if (i > min_period)
                  *out_i = 1.0f - (bacf(i) * weight);
               out_i += n_channels;
            }
         }
      }

      if ((i > (in.size()/2)) && (result.predicted_period == -1.0f))
         result.predicted_period = pd.predict_period();
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file
   q::wav_writer wav(
      "results/period_detect_" + name + ".wav", n_channels, sps
   );
   wav.write(out);

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
   std::size_t buff_size = sps / (1 / 100E-3); // 100ms

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

auto process(
   params const& params_
 , q::frequency actual_frequency
 , q::frequency lowest_freq
 , q::frequency highest_freq
 , char const* name
 , bool allow_harmonics)
{
   return process(
      gen_harmonics(actual_frequency, params_)
    , actual_frequency, lowest_freq, highest_freq, name
    , allow_harmonics
    , 0.0001
   );
}

auto process(
   params const& params_
 , q::frequency actual_frequency
 , q::frequency lowest_freq
 , q::frequency highest_freq
 , char const* name
 , double epsilon = 0.0001)
{
   return process(
      gen_harmonics(actual_frequency, params_)
    , actual_frequency, lowest_freq, highest_freq, name
    , false
    , epsilon
   );
}

auto process(
   params const& params_
 , q::frequency actual_frequency
 , q::frequency lowest_freq
 , q::frequency highest_freq
 , char const* name
 , bool allow_harmonics
 , double epsilon)
{
   return process(
      gen_harmonics(actual_frequency, params_)
    , actual_frequency, lowest_freq, highest_freq, name
    , allow_harmonics
    , epsilon
   );
}

using namespace notes;

float max_error = 0.001;   // 0.1% error

void check(float a, float b, char const* what)
{
   a = std::abs(a);
   b = std::abs(b);
   auto max = std::max(a, b);
   auto diff = std::abs(a - b);
   auto error_percent = max_error * 100;
   auto error_threshold = max * max_error;

   INFO(
      what
      << " exceeded "
      << error_percent
      << "%. Got: "
      << a
      << ",  Expecting: "
      << b
   );

   CHECK(diff < error_threshold);
}

void check(q::period_detector::info a, q::period_detector::info b)
{
   check(a._period, b._period, "Period");
   check(a._periodicity, b._periodicity, "Periodicity");
}

void check_null(q::period_detector::info a)
{
   CHECK(a._period == -1);
   CHECK(a._periodicity == 0.0f);
}

void check_null(float a)
{
   CHECK(a == 0.0f);
}

TEST_CASE("100_Hz_pure")
{
   params p;
   p._1st_level = 1.0;
   p._2nd_level = 0.0;
   p._3rd_level = 0.0;
   auto r = process(p, 100_Hz, 95_Hz, 410_Hz, "100_Hz_pure");

   check(r.predicted_period, sps/100.0, "Predicted Period");
   check(r.info, { sps/100.0, 1.0 });
   check(r.harmonic, 0.202, "Harmonic");
}

TEST_CASE("100_Hz")
{
   auto r = process(params{}, 100_Hz, 95_Hz, 410_Hz, "100_Hz");

   check(r.predicted_period, sps/100.0, "Predicted Period");
   check(r.info, { sps/100.0, 1.0 });
   check(r.harmonic, 0.5375, "Harmonic");
}

TEST_CASE("200_Hz")
{
   auto r = process(params{}, 200_Hz, 95_Hz, 410_Hz, "200_Hz");

   check(r.predicted_period, sps/200.0, "Predicted Period");
   check(r.info, { sps/200.0, 1.0 });
   check(r.harmonic, 0.544, "Harmonic");
}

TEST_CASE("300_Hz")
{
   auto r = process(params{}, 300_Hz, 95_Hz, 410_Hz, "300_Hz");

   check(r.predicted_period, sps/300.0, "Predicted Period");
   check(r.info, { sps/300.0, 1.0 });
   check_null(r.harmonic);
}

TEST_CASE("400_Hz")
{
   auto r = process(params{}, 400_Hz, 95_Hz, 410_Hz, "400_Hz");

   check(r.predicted_period, sps/400.0, "Predicted Period");
   check(r.info, { sps/400.0, 1.0 });
   check_null(r.harmonic);
}

TEST_CASE("100_Hz_strong_2nd")
{
   params p;
   p._1st_level = 0.2;
   p._2nd_level = 0.8;
   p._3rd_level = 0.0;
   auto r = process(p, 100_Hz, 95_Hz, 410_Hz, "100_Hz_strong_2nd");

   check(r.predicted_period, sps/100.0, "Predicted Period");
   check(r.info, { sps/100.0, 1.0 });
   check(r.harmonic, 0.925, "Harmonic");
}

TEST_CASE("100_Hz_stronger_2nd")
{
   params p;
   p._1st_level = 0.1;
   p._2nd_level = 0.9;
   p._3rd_level = 0.0;
   auto r = process(p, 100_Hz, 95_Hz, 410_Hz, "100_Hz_stronger_2nd", true); // allow octaves

   check(r.predicted_period, sps/200.0, "Predicted Period"); // expect wrong prediction
   check(r.info, { sps/100.0, 1.0 });
   check(r.harmonic, 0.966667, "Harmonic");
}

TEST_CASE("100_Hz_shifted_2nd")
{
   params p;
   p._1st_level = 0.4;
   p._2nd_level = 0.6;
   p._3rd_level = 0.0;
   p._2nd_offset = 0.15;
   auto r = process(p, 100_Hz, 95_Hz, 410_Hz, "100_Hz_shifted_2nd");

   CHECK(r.predicted_period != 0); // expect wrong prediction
   check(r.info, { sps/100.0, 1.0 });
   check(r.harmonic, 0.725, "Harmonic");
}

TEST_CASE("100_Hz_strong_3rd")
{
   params p;
   p._1st_level = 0.4;
   p._2nd_level = 0.0;
   p._3rd_level = 0.6;
   auto r = process(p, 100_Hz, 95_Hz, 410_Hz, "100_Hz_strong_3rd");

   CHECK(r.predicted_period != 0); // expect wrong prediction
   check(r.info, { sps/100.0, 1.0 });
   check(r.harmonic, 0.372917, "Harmonic");
}

TEST_CASE("100_Hz_stronger_3rd")
{
   params p;
   p._1st_level = 0.1;
   p._2nd_level = 0.0;
   p._3rd_level = 0.9;
   auto r = process(p, 100_Hz, 95_Hz, 410_Hz, "100_Hz_stronger_3rd", true); // allow harmonics

   CHECK(r.predicted_period != 0); // expect wrong prediction
   check(r.info, { sps/300.0, 1.0 });
   check_null(r.harmonic);
}

TEST_CASE("100_Hz_missing_fundamental")
{
   params p;
   p._1st_level = 0.0;
   p._2nd_level = 0.6;
   p._3rd_level = 0.4;
   auto r = process(p, 100_Hz, 95_Hz, 410_Hz, "100_Hz_missing_fundamental");

   check(r.predicted_period, sps/100.0, "Predicted Period");
   check(r.info, { sps/100.0, 1.0 });
   check(r.harmonic, 0.779167, "Harmonic");
}

TEST_CASE("Low_E_12th")
{
   auto r = process(params{}, low_e_12th, low_e * 0.8, low_e * 5, "Low_E_12th");

   check(r.predicted_period, 267.575, "Predicted Period");
   check(r.info, { 267.575f, 0.998512 });
   check(r.harmonic, 0.539, "Harmonic");
}

TEST_CASE("Low_E_24th")
{
   auto r = process(params{}, low_e_24th, low_e * 0.8, low_e * 5, "Low_E_24th");

   check(r.predicted_period, 133.787, "Predicted Period");
   check(r.info, { 133.787f, 0.998512 });
   check_null(r.harmonic);
}

TEST_CASE("B_24th")
{
   auto r = process(params{}, b_24th, b * 0.8, b * 5, "B_24th");

   check(r.predicted_period, 44.645, "Predicted Period");
   check(r.info, { 44.645f, 0.9955f });
   check_null(r.harmonic);
}

TEST_CASE("High_E_24th")
{
   auto r = process(params{}, high_e_24th, high_e * 0.8, high_e * 5, "High_E_24th", 0.01);

   check(r.predicted_period, 33.4477, "Predicted Period");
   check(r.info, { 33.4477f, 0.9948f });
   check_null(r.harmonic);
}

TEST_CASE("Non_integer_harmonics")
{
   params p;
   p._offset = 30239;
   p._2nd_harmonic = 2.003;
   auto r = process(p, low_e, low_e * 0.8, low_e * 5, "Non_integer_harmonics");

   CHECK(r.predicted_period != 0); // expect wrong prediction
   check(r.info, { 534.84f, 0.952f });
   check(r.harmonic, 0.537, "Harmonic");
}

TEST_CASE("Test_wide_range1")
{
   params p;
   p._1st_level = 1.0;
   p._2nd_level = 0.0;
   p._3rd_level = 0.0;
   {
      auto r = process(p, 100_Hz, 100_Hz, 1600_Hz, "wide1-100");
      check(r.info._period, 441.0, "Period");
   }
   {
      auto r = process(p, 200_Hz, 100_Hz, 1600_Hz, "wide1-200");
      check(r.info._period, 220.5, "Period");
   }
   {
      auto r = process(p, 400_Hz, 100_Hz, 1600_Hz, "wide1-400");
      check(r.info._period, 110.25, "Period");
   }
   {
      auto r = process(p, 800_Hz, 100_Hz, 1600_Hz, "wide1-800");
      check(r.info._period, 55.125, "Period");
   }
   {
      auto r = process(p, 1600_Hz, 100_Hz, 1600_Hz, "wide1-1600");
      check(r.info._period, 27.5625, "Period");
   }
}

TEST_CASE("Test_wide_range2")
{
   params p;
   p._1st_level = 1.0;
   p._2nd_level = 0.0;
   p._3rd_level = 0.0;
   {
      auto r = process(p, 150_Hz, 100_Hz, 1600_Hz, "wide2-150");
      check(r.info._period, 294.0, "Period");
   }
   {
      auto r = process(p, 300_Hz, 100_Hz, 1600_Hz, "wide2-300");
      check(r.info._period, 147.0, "Period");
   }
   {
      auto r = process(p, 600_Hz, 100_Hz, 1600_Hz, "wide2-600");
      check(r.info._period, 73.5, "Period");
   }
   {
      auto r = process(p, 1200_Hz, 100_Hz, 1600_Hz, "wide2-1200");
      check(r.info._period, 36.75, "Period");
   }
   {
      auto r = process(p, 1600_Hz, 100_Hz, 1600_Hz, "wide2-1600");
      check(r.info._period, 27.5625, "Period");
   }
}

TEST_CASE("Test_wide_range3")
{
   params p;
   {
      auto r = process(p, 220_Hz, 200_Hz, 3200_Hz, "wide3-220");
      check(r.info._period, 200.5, "Period");
   }
   {
      auto r = process(p, 440_Hz, 200_Hz, 3200_Hz, "wide3-440");
      check(r.info._period, 100.2, "Period");
   }
   {
      auto r = process(p, 880_Hz, 200_Hz, 3200_Hz, "wide3-880");
      check(r.info._period, 50.1, "Period");
   }
   {
      auto r = process(p, 1760_Hz, 200_Hz, 3200_Hz, "wide3-1760", 0.01);
      check(r.info._period, 25.059, "Period");
   }
   {
      auto r = process(p, 2000_Hz, 200_Hz, 3200_Hz, "wide3-2000", 0.01);
      check(r.info._period, 22.05, "Period");
   }
   {
      auto r = process(p, 2500_Hz, 200_Hz, 3200_Hz, "wide3-2000", 0.01);
      check(r.info._period, 17.64, "Period");
   }
   {
      auto r = process(p, 3000_Hz, 200_Hz, 3200_Hz, "wide3-3000", 0.01);
      check(r.info._period, 14.7, "Period");
   }
   {
      auto r = process(p, 4000_Hz, 200_Hz, 8000_Hz, "wide3-4000", 0.01);
      check(r.info._period, 11.025, "Period");
   }
}

TEST_CASE("Test_high_freq")
{
   params p;
   {
      auto r = process(p, 4000_Hz, 200_Hz, 8000_Hz, "wide4-4000", 0.01);
      check(r.info._period, 11.025, "Period");
   }
   {
      auto r = process(p, 4186_Hz, 200_Hz, 8000_Hz, "high_freq-4186", 0.02);
      check(r.info._period, 10.5328, "Period");
   }
   {
      auto r = process(p, 4500_Hz, 200_Hz, 8000_Hz, "high_freq-4500", 0.02);
      check(r.info._period, 9.7924, "Period");
   }
   {
      auto r = process(p, 4900_Hz, 200_Hz, 8000_Hz, "high_freq-4900", 0.02);
      check(r.info._period, 9, "Period");
   }
   {
      auto r = process(p, 5000_Hz, 200_Hz, 8000_Hz, "high_freq-5000", 0.02);
      check(r.info._period, 8.82108, "Period");
   }
   {
      auto r = process(p, 5000_Hz, 300_Hz, 8000_Hz, "high_freq-5000@300", 0.02);
      check(r.info._period, 8.82108, "Period");
   }
   {
      auto r = process(p, 5100_Hz, 300_Hz, 8000_Hz, "high_freq-5100", 0.02);
      check(r.info._period, 8.64706, "Period");
   }
   {
      auto r = process(p, 5500_Hz, 300_Hz, 8000_Hz, "high_freq-5500", 0.02);
      check(r.info._period, 8.02997, "Period");
   }
   {
      auto r = process(p, 6000_Hz, 300_Hz, 8000_Hz, "high_freq-6000", 0.02);
      check(r.info._period, 7.35, "Period");
   }
   {
      p._1st_offset = 4;
      auto r = process(p, 5000_Hz, 200_Hz, 8000_Hz, "high_freq-5000-shifted", 0.02);
      check(r.info._period, 8.82108, "Period");
   }
}

TEST_CASE("Test_violin_range")
{
   // G string (G3 – C5, G5)
   // D string (D4 – G5, D6)
   // A string (A4 – D6, A6)
   // E string (E5 – A7, D8)

   params p;
   p._1st_offset = 4;
   {
      // G3
      auto r = process(p, 196_Hz, 190_Hz, 5000_Hz, "violin-g3", 0.02);
      check(r.info._period, sps/196.0, "Period");
   }
   {
      // C5
      auto r = process(p, 523.25_Hz, 190_Hz, 5000_Hz, "violin-c5", 0.02);
      check(r.info._period, sps/523.25, "Period");
   }
   {
      // G5
      auto r = process(p, 783.99_Hz, 190_Hz, 5000_Hz, "violin-g5", 0.02);
      check(r.info._period, sps/783.99, "Period");
   }
   {
      // D4
      auto r = process(p, 293.66_Hz, 190_Hz, 5000_Hz, "violin-d4", 0.02);
      check(r.info._period, sps/293.66, "Period");
   }
   {
      // A6
      auto r = process(p, 1760.0_Hz, 190_Hz, 5000_Hz, "violin-a6", 0.02);
      check(r.info._period, sps/1760.0, "Period");
   }
   {
      // A4
      auto r = process(p, 440.0_Hz, 190_Hz, 5000_Hz, "violin-a4", 0.02);
      check(r.info._period, sps/440.0, "Period");
   }
   {
      // D6
      auto r = process(p, 1174.66_Hz, 190_Hz, 5000_Hz, "violin-d6", 0.02);
      check(r.info._period, sps/1174.66, "Period");
   }
   {
      // E5
      auto r = process(p, 659.26_Hz, 190_Hz, 5000_Hz, "violin-e5", 0.02);
      check(r.info._period, sps/659.26, "Period");
   }
   {
      // A7
      auto r = process(p, 3520.0_Hz, 190_Hz, 5000_Hz, "violin-a7", 0.02);
      check(r.info._period, sps/3520.0, "Period");
   }
   {
      // D8
      auto r = process(p, 4698.64_Hz, 190_Hz, 5000_Hz, "violin-d8", 0.02);
      check(r.info._period, sps/4698.64, "Period");
   }
}








