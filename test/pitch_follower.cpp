/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/literals.hpp>
#include <q/sfx.hpp>
#include <q_io/audio_file.hpp>
#include <q/synth.hpp>

#include <vector>
#include <iostream>
#include <fstream>

#include "notes.hpp"
#include <q/pitch_detector.hpp>

namespace q = cycfi::q;
using namespace q::literals;
namespace audio_file = q::audio_file;

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

   auto src = audio_file::reader{"audio_files/" + name + ".wav"};
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

   // Our envelope_tracker
   q::envelope_processor::config env_config;
   env_config.attack = attack;
   env_config.decay = decay;

   q::envelope_processor      env_proc{ env_config, sps };

   // Pitch detection
   q::one_pole_lowpass        lp1{ highest_freq, sps };
   q::one_pole_lowpass        lp2{ lowest_freq, sps };
   q::pitch_detector<>        pd{ lowest_freq, highest_freq, sps, 0.001 };

   auto filt = q::reso_filter(0.5, 0.9);           // Our resonant filter(s)
   auto interp = q::interpolate(0.1, 0.99);        // Limits
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
      s = env_proc(s);
      out[ch1] = s;

      // Pitch detection
      s = lp1(s);
      s -= lp2(s);
      s = pd(s);

      if (env_proc.is_note_on())
      {
         // Set frequency
         auto f_ = pd.frequency();
         if (f_ == 0.0f)
            f_ = pd.predict_frequency();
         if (f_ != 0.0f)
            f = q::phase(f_, sps);
      }

      auto synth_val = 0.0f;
      auto synth_env = env_proc.envelope();

      if (synth_env > 0.0f)
      {
         // Set the filter frequency
         auto cutoff = interp(synth_env);
         filt.cutoff(cutoff);

         // Synthesize
         synth_val = clip(filt(square(ph, f))) * synth_env;
         ph += f;
      }

#ifdef debug_signals
      out[ch3] = synth_env;
#endif

      out[ch2] = synth_val;
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   auto wav = audio_file::writer{
      "results/pitch_follower_" + name + ".wav", n_channels, sps
   };
   wav.write(out);
}

void process(
   std::string name
 , q::frequency lowest_freq
 , q::duration attack = 100_ms
 , q::duration decay = 3_s)
{
   process(name, lowest_freq * 0.8, lowest_freq * 5, attack, decay);
}

int main()
{
   using namespace notes;

   // process("sin_440", d, 5_ms, 5_ms);
   process("1-Low E", low_e);
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

   process("Tapping D", d);
   process("Hammer-Pull High E", high_e);
   process("Slide G", g);
   process("Bend-Slide G", g);

   process("GLines1", g);
   process("GLines2", g);
   process("GLines2a", g);
   process("GLines3", g);
   // process("SingleStaccato", g);
   // process("Staccato2", g, 10_ms, 50_ms);
   // process("Staccato3", g, 10_ms, 50_ms);
   process("GStaccato", g, 10_ms, 50_ms);

   return 0;
}

