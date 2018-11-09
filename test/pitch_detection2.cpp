/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
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

   auto src = audio_file::reader{"audio_files/" + name + ".wav"};
   std::uint32_t const sps = src.sps();

   std::vector<float> in(src.length());
   src.read(in);

   ////////////////////////////////////////////////////////////////////////////
   // Output
   constexpr auto n_channels = 6;
   std::vector<float> out(src.length() * n_channels);
   std::fill(out.begin(), out.end(), 0);

   ////////////////////////////////////////////////////////////////////////////
   // Process
   q::pitch_detector<>        pd{ lowest_freq, highest_freq, sps, 0.001 };
   q::bacf<> const&           bacf = pd.bacf();
   auto                       size = bacf.size();
   q::edges const&            edges = bacf.edges();
   q::peak_envelope_follower  env{ 1_s, sps };
   q::one_pole_lowpass        lp{ highest_freq, sps };
   q::one_pole_lowpass        lp2{ lowest_freq, sps };
   q::onset_detector          attack{ 0.6f, 100_ms, sps };

   constexpr float            slope = 1.0f/20;
   q::compressor              comp{ -3_dB, slope };
   q::clip                    clip;

   float                      onset_threshold = 0.005;
   float                      release_threshold = 0.001;
   float                      threshold = onset_threshold;
   float                      prev_env = 0;

   int ii = 0;

   for (auto i = 0; i != in.size(); ++i)
   {
      auto pos = i * n_channels;
      auto ch1 = pos;      // input
      auto ch2 = pos+1;    // bacf
      auto ch3 = pos+2;
      auto ch4 = pos+3;    // attack
      auto ch5 = pos+4;    // frequency
      auto ch6 = pos+5;    // attack envelope

      auto s = in[i];

      // Original signal
      // out[ch1] = s;

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

      // attack
      auto o = attack(s);
      out[ch4] = bool(o) * 0.85f;
      out[ch6] = std::min<float>(2 * attack._env(), 1.0);

      out[ch1] = s;

      // Pitch Detect
      std::size_t extra;
      bool proc = pd(s, extra);
      out[ch2] = -1;   // placeholder

      // BACF default placeholder
      out[ch3] = -0.8;

      if (proc)
      {
         auto out_i = (&out[ch2] - (((size-1) + extra) * n_channels));
         auto const& info = bacf.result();
         for (auto n : info.correlation)
         {
            *out_i = n / float(info.max_count);
            out_i += n_channels;
         }

         out_i = (&out[ch3] - (((size-1) + extra) * n_channels));
         for (auto i = 0; i != size; ++i)
         {
            *out_i = bacf[i] * 0.8;
            out_i += n_channels;
         }

         bool is_attack = prev_env < e;
         prev_env = e;

         out_i = (&out[ch4] - (((size-1) + extra) * n_channels));
         if (pd.is_note_onset())
         {
            for (auto i = 0; i != size; ++i)
            {
               *out_i = std::max(0.5f, *out_i);
               out_i += n_channels;
            }
         }

         csv << pd.frequency() << ", " << pd.periodicity() << std::endl;
      }

      // Frequency
      auto f = pd.frequency() / double(highest_freq);
      auto fi = int(i - bacf.size());
      if (fi >= 0)
         out[(fi * n_channels) + 4] = f;
   }

   csv.close();

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   auto wav = audio_file::writer{
      "results/pitch_detect_" + name + ".wav", n_channels, sps
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

   // process("sin_440", d);
   // process("1-Low E", low_e);
   // process("2-Low E 2th", low_e);
   // process("3-A", a);
   // process("4-A 12th", a);
   // process("5-D", d);
   // process("6-D 12th", d);
   // process("7-G", g);
   // process("8-G 12th", g);
   // process("9-B", b);
   // process("10-B 12th", b);
   // process("11-High E", high_e);
   // process("12-High E 12th", high_e);
   // process("Tapping D", d);
   // process("Hammer-Pull High E", high_e);
   // process("Bend-Slide G", g);

   // process("Staccato3", g);
   process("GLines2a", g);

   return 0;
}

