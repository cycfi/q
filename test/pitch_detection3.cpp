/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <q/literals.hpp>
#include <q/sfx.hpp>
#include <q/pitch_detector.hpp>
#include <q_io/audio_file.hpp>

#include <vector>
#include <iostream>
#include <fstream>

#include "notes.hpp"

namespace q = cycfi::q;
using namespace q::literals;
namespace audio_file = q::audio_file;

void process(
   std::string name
 , q::frequency lowest_freq
 , q::frequency highest_freq)
{
   ////////////////////////////////////////////////////////////////////////////
   // Prepare output file

   std::ofstream csv("results/frequencies_" + name + ".csv");

   ////////////////////////////////////////////////////////////////////////////
   // Read audio file

   auto src = audio_file::reader{"audio_files/" + name + ".aif"};
   std::uint32_t const sps = src.sps();

   std::vector<float> in(src.length());
   src.read(in);

   ////////////////////////////////////////////////////////////////////////////
   // Output
   constexpr auto n_channels = 5;
   std::vector<float> out(src.length() * n_channels);
   std::fill(out.begin(), out.end(), 0);

   ////////////////////////////////////////////////////////////////////////////
   // Process
   q::pitch_detector<>        pd{ lowest_freq, highest_freq, sps, 0.001 };
   q::bacf<> const&           bacf = pd.bacf();
   auto                       size = bacf.size();
   q::edges const&            edges = bacf.edges();
   q::dynamic_smoother        lp_x{ lowest_freq / 2, 0.5, sps };
   q::peak_envelope_follower  env{ 1_s, sps };
   q::one_pole_lowpass        lp{ highest_freq, sps };
   q::one_pole_lowpass        lp2{ lowest_freq, sps };

   constexpr float            slope = 1.0f/20;
   q::compressor_expander     comp{ 0.5f, slope };
   q::clip                    clip;

   float                      onset_threshold = 0.005;
   float                      release_threshold = 0.001;
   float                      threshold = onset_threshold;

   for (auto i = 0; i != in.size(); ++i)
   {
      auto pos = i * n_channels;
      auto s = in[i];

      // Original signal
      out[pos] = s;

      // Bandpass filter
      s = lp(s);
      s -= lp2(s);

      // Envelope
      auto e = env(std::abs(s));

      if (e > threshold)
      {
         // Compressor + makeup-gain + hard clip
         s = clip(comp(s, e) * 1.0f/slope);
         threshold = release_threshold;
      }
      else
      {
         s = 0.0f;
         threshold = onset_threshold;
      }

      // Dynamic lowpass filter
      // s = lp(s);
      out[pos + 1] = s;

      // Pitch Detect
      std::size_t extra;
      bool proc = pd(s, extra);
      out[pos + 2] = -1;   // placeholder

      // BACF default placeholder
      out[pos + 3] = -0.8;

      if (proc)
      {
         auto out_i = (&out[pos + 3] - (((size-1) + extra) * n_channels));
         auto const& info = bacf.result();
         for (auto n : info.correlation)
         {
            *out_i = n / float(info.max_count);
            out_i += n_channels;
         }

         out_i = (&out[pos + 2] - (((size-1) + extra) * n_channels));
         for (auto i = 0; i != size; ++i)
         {
            *out_i = bacf[i] * 0.8;
            out_i += n_channels;
         }

         csv << pd.frequency() << ", " << pd.periodicity() << std::endl;
      }

      // Frequency
      auto f = pd.frequency();
      if (f != -1.0f)
         f /= double(highest_freq);
      auto fi = int(i - bacf.size());
      if (fi >= 0)
         out[(fi * n_channels) + 4] = f;
   }

   csv.close();

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   auto wav = audio_file::writer{
      "results/pitch_detect_" + name + ".wav", audio_file::wav, audio_file::_16_bits
    , n_channels, sps
   };
   wav.write(out);
}

void process(std::string name, q::frequency lowest_freq)
{
   process(name, lowest_freq * 0.8, lowest_freq * 5);
}

int main()
{
   using namespace notes;

   process("sin_440", d);
   process("1-Low E", low_e);
   process("2-Low E 2th", low_e);
   process("5-D", d);
   process("6-D 12th", d);
   process("Tapping D", d);
   process("Hammer-Pull High E", high_e);
   process("Bend-Slide G", g);

   process("watda1", high_e);

   return 0;
}

