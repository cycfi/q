/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/support/literals.hpp>
#include <q/pitch/pitch_follower.hpp>
#include <q_io/audio_file.hpp>
#include <q/synth/square.hpp>
#include <q/fx/special.hpp>

#include <vector>
#include <iostream>
#include <fstream>

#include "notes.hpp"

namespace q = cycfi::q;
using namespace q::literals;

#define debug_signals

void process(
   std::string name
 , q::frequency lowest_freq
 , q::frequency highest_freq
 , q::duration attack
 , q::duration decay)
{
   ////////////////////////////////////////////////////////////////////////////
   // Read audio file

   q::wav_reader src{"audio_files/" + name + ".wav"};
   std::uint32_t const sps = src.sps();

   std::vector<float> in(src.length());
   src.read(in);

   ////////////////////////////////////////////////////////////////////////////
   // Output
#ifdef debug_signals
   constexpr auto n_channels = 3;
#else
   constexpr auto n_channels = 2;
#endif
   std::vector<float> out(src.length() * n_channels);
   std::fill(out.begin(), out.end(), 0);

   ////////////////////////////////////////////////////////////////////////////
   // Synthesizer

   auto f = q::phase(440_Hz, sps);     // Initial synth frequency
   auto ph = q::phase();               // Our phase accumulator
   auto square = q::square;            // Our synth

   ////////////////////////////////////////////////////////////////////////////
   // Process

   // Envelope_tracker
   q::pitch_follower::config env_config;
   env_config.attack = attack;
   env_config.decay = decay;

   // The pitch follower
   auto pf = q::pitch_follower{ env_config, lowest_freq, highest_freq, sps };

   auto filt = q::reso_filter(0.5, 0.9);           // Resonant filter(s)
   auto map = q::map(0.1, 0.99);                   // Limits
   auto clip = q::clip();                          // Clipper

   for (auto i = 0; i != in.size(); ++i)
   {
      auto pos = i * n_channels;
      auto ch1 = pos;      // input
      auto ch2 = pos+1;    // synth
      auto ch3 = pos+2;    // synth envelope state
      auto ch4 = pos+3;    // synth envelope

      auto s = in[i];

      // Track envelope
      s = pf(s);
      out[ch1] = s;

      f = q::phase(pf.frequency(), sps);

      auto synth_val = 0.0f;
      auto synth_env = pf.envelope();

      if (synth_env > 0.0f)
      {
         // Set the filter frequency
         auto cutoff = map(synth_env);
         filt.cutoff(cutoff);

         // Synthesize
         synth_val = clip(filt(square(ph, f))) * synth_env;
         ph += f;
      }

#ifdef debug_signals
      out[ch3] = pf.signal_envelope();
#endif

      out[ch2] = synth_val;
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   q::wav_writer wav{
      "results/pitch_follower_" + name + ".wav", n_channels, sps
   };
   wav.write(out);
}

void process(
   std::string name
 , q::frequency lowest_freq
 , q::duration attack = 10_ms
 , q::duration decay = 3_s)
{
   process(name, lowest_freq * 0.8, lowest_freq * 5, attack, decay);
}

int main()
{
   using namespace notes;

   // process("-2a-F#", low_fs);
   // process("-2b-F#-12th", low_fs);
   // process("-2c-F#-24th", low_fs);

   // process("-1a-Low-B", low_b);
   // process("-1b-Low-B-12th", low_b);
   // process("-1c-Low-B-24th", low_b);

   // process("sin_440", d, 5_ms, 5_ms);

   // process("1a-Low-E", low_e);
   // process("1b-Low-E-12th", low_e);
   // process("1c-Low-E-24th", low_e);

   // process("2a-A", a);
   // process("2b-A-12th", a);
   // process("2c-A-24th", a);

   // process("3a-D", d);
   // process("3b-D-12th", d);
   // process("3c-D-24th", d);

   // process("4a-G", g);
   // process("4b-G-12th", g);
   // process("4c-G-24th", g);

   // process("5a-B", b);
   // process("5b-B-12th", b);
   // process("5c-B-24th", b);

   // process("6a-High-E", high_e);
   // process("6b-High-E-12th", high_e);
   // process("6c-High-E-24th", high_e);

   // process("Tapping D", d);
   // process("Hammer-Pull High E", high_e);
   process("Slide G", g);
   // process("Bend-Slide G", g);

   // process("GLines-Debug", g);
   // process("GLines1", g);
   // process("GLines2", g);
   // process("GLines3", g);
   // process("SingleStaccato", g);
   // process("GStaccato", g, 10_ms, 50_ms);

   return 0;
}

