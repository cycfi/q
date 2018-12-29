/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/support/literals.hpp>
#include <q/utility/zero_crossing.hpp>
#include <q_io/audio_file.hpp>

#include <vector>
#include <iostream>
#include "notes.hpp"

namespace q = cycfi::q;
using namespace q::literals;

constexpr auto pi = q::pi;
constexpr auto sps = 44100;

void process(
   std::vector<float>&& in
 , q::frequency actual_frequency
 , q::frequency lowest_freq
 , q::frequency highest_freq
 , std::string name)
{
   constexpr auto n_channels = 4;
   std::vector<float> out(in.size() * n_channels);


   q::zero_crossing  zc(-60_dB, float(lowest_freq.period() * 2) * sps);

   for (auto i = 0; i != in.size(); ++i)
   {
      auto pos = i * n_channels;
      auto ch1 = pos;
      auto ch2 = pos+1;
      auto ch3 = pos+2;
      auto ch4 = pos+3;

      auto s = in[i];
      out[ch1] = s;

      // Default placeholders
      out[ch2] = -1;

      out[ch2] = zc(s) * 0.8;
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file
   q::wav_writer wav{
      "results/pd_exp_" + name + ".wav", n_channels, sps
   };
   wav.write(out);
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
   constexpr float offset = 0;
   std::size_t buff_size = sps / (1 / 30E-3); // 30ms

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
   q::frequency actual_frequency
 , q::frequency lowest_freq
 , q::frequency highest_freq
 , char const* name
)
{
   process(
      gen_harmonics(actual_frequency, params{})
    , actual_frequency, lowest_freq, highest_freq, name
   );
}

using namespace notes;

int main()
{
   process(100_Hz, 100_Hz, 400_Hz, "100_Hz");
   process(200_Hz, 100_Hz, 400_Hz, "200_Hz");
   process(400_Hz, 100_Hz, 400_Hz, "400_Hz");

   return 0;
}




