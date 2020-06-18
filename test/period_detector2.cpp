/*=============================================================================
   Copyright (c) 2014-2020 Joel de Guzman. All rights reserved.

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
}




