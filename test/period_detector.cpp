/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <infra/doctest.hpp>

#include <q/support/literals.hpp>
#include <q/pitch/period_detector.hpp>
#include <q_io/audio_file.hpp>

#include <vector>
#include <iostream>
#include <tuple>
#include "notes.hpp"

namespace q = cycfi::q;
using namespace q::literals;

constexpr auto pi = q::pi;
constexpr auto sps = 44100;

using result_type
   = std::tuple<float, q::period_detector::info, q::period_detector::info>;

result_type process(
   std::vector<float>&& in
 , q::frequency actual_frequency
 , q::frequency lowest_freq
 , q::frequency highest_freq
 , std::string name
 , bool allow_harmonics = false)
{
   result_type result;
   constexpr auto n_channels = 3;
   std::vector<float> out(in.size() * n_channels);
   std::get<0>(result) = -1.0f;

   q::period_detector   pd(lowest_freq, highest_freq, sps, -30_dB);
   auto const&          bits = pd.bits();
   auto const&          edges = pd.edges();

   q::autocorrelator<>  bacf{ bits };
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
            std::get<1>(result) = pd.fundamental();
            std::get<2>(result) = pd.harmonic(2);
         }
         else
         {
           auto a = std::get<1>(result)._period;
           auto b = pd.fundamental()._period;

            if (allow_harmonics)
            {
               // Allow 2nd and 3rd harmonics
               CHECK((a == doctest::Approx(b)
                  || (a * 2) == doctest::Approx(b).epsilon(0.0001)
                  || (a * 3) == doctest::Approx(b).epsilon(0.0001)
                  || a == doctest::Approx(b * 2).epsilon(0.0001)
                  || a == doctest::Approx(b * 3).epsilon(0.0001)
               ));
            }
            else
            {
               CHECK(a == doctest::Approx(b).epsilon(0.0001));
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

      if ((i > (in.size()/2)) && (std::get<0>(result) == -1.0f))
         std::get<0>(result) = pd.predict_period();
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file
   q::wav_writer wav{
      "results/period_detect_" + name + ".wav", n_channels, sps
   };
   wav.write(out);

   return result;
}

struct params
{
   float _offset = 0.0;          // Waveform offset
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
 , bool allow_harmonics = false)
{
   return process(
      gen_harmonics(actual_frequency, params_)
    , actual_frequency, lowest_freq, highest_freq, name
    , allow_harmonics
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

   CHECK_MESSAGE((diff < error_threshold),
      what
      << " exceeded "
      << error_percent
      << "%. Got: "
      << a
      << ",  Expecting: "
      << b
   );
}

void check(q::period_detector::info a, q::period_detector::info b)
{
   check(a._period, b._period, "Period");
   check(a._periodicity, b._periodicity, "Periodicity");
}

void check_null(q::period_detector::info a)
{
   CHECK(a._period == -1);
   CHECK(a._periodicity == -1);
}

TEST_CASE("100_Hz_pure")
{
   params p;
   p._1st_level = 1.0;
   p._2nd_level = 0.0;
   p._3rd_level = 0.0;
   auto r = process(p, 100_Hz, 95_Hz, 410_Hz, "100_Hz_pure");

   check(std::get<0>(r), 441.0, "Predicted Period");
   check(std::get<1>(r), { 441.0, 1.0 });
   check(std::get<2>(r), { 220.5, 0.20625 });
}

TEST_CASE("100_Hz")
{
   auto r = process(params{}, 100_Hz, 95_Hz, 410_Hz, "100_Hz");

   check(std::get<0>(r), 441.0, "Predicted Period");
   check(std::get<1>(r), { 441.0, 1.0 });
   check(std::get<2>(r), { 220.5, 0.529 });
}

TEST_CASE("200_Hz")
{
   auto r = process(params{}, 200_Hz, 95_Hz, 410_Hz, "200_Hz");

   check(std::get<0>(r), 220.5, "Predicted Period");
   check(std::get<1>(r), { 220.5, 1.0 });
   check(std::get<2>(r), { 110.25, 0.537 });
}

TEST_CASE("300_Hz")
{
   auto r = process(params{}, 300_Hz, 95_Hz, 410_Hz, "300_Hz");

   check(std::get<0>(r), 147.0, "Predicted Period");
   check(std::get<1>(r), { 147.0, 1.0 });
   check_null(std::get<2>(r));
}

TEST_CASE("400_Hz")
{
   auto r = process(params{}, 400_Hz, 95_Hz, 410_Hz, "400_Hz");

   check(std::get<0>(r), 110.25, "Predicted Period");
   check(std::get<1>(r), { 110.25, 1.0 });
   check_null(std::get<2>(r));
}

TEST_CASE("100_Hz_strong_2nd")
{
   params p;
   p._1st_level = 0.2;
   p._2nd_level = 0.8;
   p._3rd_level = 0.0;
   auto r = process(p, 100_Hz, 95_Hz, 410_Hz, "100_Hz_strong_2nd");

   check(std::get<0>(r), 441.0, "Predicted Period");
   check(std::get<1>(r), { 441.0, 1.0 });
   check(std::get<2>(r), { 220.5, 0.925 });
}

TEST_CASE("100_Hz_stronger_2nd")
{
   params p;
   p._1st_level = 0.1;
   p._2nd_level = 0.9;
   p._3rd_level = 0.0;
   auto r = process(p, 100_Hz, 95_Hz, 410_Hz, "100_Hz_stronger_2nd", true); // allow octaves

   check(std::get<0>(r), 220.5, "Predicted Period"); // expect wrong prediction
   check(std::get<1>(r), { 441.0, 1.0 });
   check(std::get<2>(r), { 220.5, 0.966 });
}

TEST_CASE("100_Hz_shifted_2nd")
{
   params p;
   p._1st_level = 0.4;
   p._2nd_level = 0.6;
   p._3rd_level = 0.0;
   p._2nd_offset = 0.15;
   auto r = process(p, 100_Hz, 95_Hz, 410_Hz, "100_Hz_shifted_2nd");

   CHECK(std::get<0>(r) != 0); // expect wrong prediction
   check(std::get<1>(r), { 441.0, 1.0 });
   check(std::get<2>(r), { 220.5, 0.715 });
}

TEST_CASE("100_Hz_strong_3rd")
{
   params p;
   p._1st_level = 0.4;
   p._2nd_level = 0.0;
   p._3rd_level = 0.6;
   auto r = process(p, 100_Hz, 95_Hz, 410_Hz, "100_Hz_strong_3rd");

   CHECK(std::get<0>(r) != 0); // expect wrong prediction
   check(std::get<1>(r), { 441.0, 1.0 });
   check(std::get<2>(r), { 220.5, 0.3645 });
}

TEST_CASE("100_Hz_stronger_3rd")
{
   params p;
   p._1st_level = 0.1;
   p._2nd_level = 0.0;
   p._3rd_level = 0.9;
   auto r = process(p, 100_Hz, 95_Hz, 410_Hz, "100_Hz_stronger_3rd", true); // allow harmonics

   CHECK(std::get<0>(r) != 0); // expect wrong prediction
   check(std::get<1>(r), { 441.0, 1.0 });
   check(std::get<2>(r), { 220.5, 0.2104 });
}

TEST_CASE("100_Hz_missing_fundamental")
{
   params p;
   p._1st_level = 0.0;
   p._2nd_level = 0.6;
   p._3rd_level = 0.4;
   auto r = process(p, 100_Hz, 95_Hz, 410_Hz, "100_Hz_missing_fundamental");

   check(std::get<0>(r), 441.0, "Predicted Period");
   check(std::get<1>(r), { 441.0, 1.0 });
   check(std::get<2>(r), { 220.5, 0.85 });
}

TEST_CASE("Low_E_12th")
{
   auto r = process(params{}, low_e_12th, low_e * 0.8, low_e * 5, "Low_E_12th");

   check(std::get<0>(r), 267.575, "Predicted Period");
   check(std::get<1>(r), { 267.575, 1.0 });
   check(std::get<2>(r), { 133.78737, 0.533 });
}

TEST_CASE("Low_E_24th")
{
   auto r = process(params{}, low_e_24th, low_e * 0.8, low_e * 5, "Low_E_24th");

   check(std::get<0>(r), 133.787, "Predicted Period");
   check(std::get<1>(r), { 133.787, 0.998 });
   check_null(std::get<2>(r));
}

TEST_CASE("B_24th")
{
   auto r = process(params{}, b_24th, b * 0.8, b * 5, "B_24th");

   check(std::get<0>(r), 44.645, "Predicted Period");
   check(std::get<1>(r), { 44.645, 1.0 });
   check_null(std::get<2>(r));
}

TEST_CASE("High_E_24th")
{
   auto r = process(params{}, high_e_24th, high_e * 0.8, high_e * 5, "High_E_24th");

   check(std::get<0>(r), 33.4477, "Predicted Period");
   check(std::get<1>(r), { 33.4477, 0.989 });
   check_null(std::get<2>(r));
}

TEST_CASE("Non_integer_harmonics")
{
   params p;
   p._offset = 30239;
   p._2nd_harmonic = 2.003;
   auto r = process(p, low_e, low_e * 0.8, low_e * 5, "Non_integer_harmonics");

   CHECK(std::get<0>(r) != 0); // expect wrong prediction
   check(std::get<1>(r), { 534.84, 1.0 });
   check(std::get<2>(r), { 267.42, 0.58631 });
}








