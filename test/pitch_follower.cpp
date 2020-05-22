/*=============================================================================
   Copyright (c) 2014-2019 Joel de Guzman. All rights reserved.

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
#include <chrono>

#include "notes.hpp"

namespace q = cycfi::q;
using namespace q::literals;

#define debug_signals

constexpr auto break_time = 100.0;

void break_debug()
{
   // Set break_time to a specific time if you want to do some
   // timed debugging. Then set a break-point here.
}

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

   std::uint64_t nanoseconds = 0;

   for (auto i = 0; i != in.size(); ++i)
   {
      float time = i / float(sps);

      auto pos = i * n_channels;
      auto ch1 = pos;      // input
      auto ch2 = pos+1;    // synth
      auto ch3 = pos+2;    // debug

      auto s = in[i];

      // Track pitch
      auto start = std::chrono::high_resolution_clock::now();
      s = pf(s);
      auto elapsed = std::chrono::high_resolution_clock::now() - start;
      auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed);
      nanoseconds += duration.count();

      out[ch1] = s;

      auto pf_freq = pf.get_frequency();
      f = q::phase(pf_freq, sps);

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

      if (time >= break_time)
         break_debug();

#ifdef debug_signals
      auto f = pf_freq / double(highest_freq);
      out[ch3] = f;
#endif

      out[ch2] = synth_val;
   }

   ////////////////////////////////////////////////////////////////////////////
   // Print processing time
   std::cout
      << '"' << name << "\": "
      << (double(nanoseconds) / in.size())
      << " nanoseconds per sample."
      << std::endl
      ;

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   q::wav_writer wav(
      "results/pitch_follower_" + name + ".wav", n_channels, sps
   );
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

#define ALL_TESTS 1
#define LOW_FREQUENCY_TESTS 1
#define PHRASE_TESTS 1
#define STACCATO_TESTS 1

int main()
{
   using namespace notes;

#if LOW_FREQUENCY_TESTS==1 || ALL_TESTS==1

   process("-2a-F#", low_fs);
   process("-2b-F#-12th", low_fs);
   process("-2c-F#-24th", low_fs);

   process("-1a-Low-B", low_b);
   process("-1b-Low-B-12th", low_b);
   process("-1c-Low-B-24th", low_b);

#endif
#if ALL_TESTS==1

   process("sin_440", d, 5_ms, 5_ms);

   process("1a-Low-E", low_e);
   process("1b-Low-E-12th", low_e);
   process("1c-Low-E-24th", low_e);

   process("2a-A", a);
   process("2b-A-12th", a);
   process("2c-A-24th", a);

   process("3a-D", d);
   process("3b-D-12th", d);
   process("3c-D-24th", d);

   process("4a-G", g);
   process("4b-G-12th", g);
   process("4c-G-24th", g);

   process("5a-B", b);
   process("5b-B-12th", b);
   process("5c-B-24th", b);

   process("6a-High-E", high_e);
   process("6b-High-E-12th", high_e);
   process("6c-High-E-24th", high_e);

#endif
#if PHRASE_TESTS==1 || ALL_TESTS==1

   process("Tapping D", d);
   process("Hammer-Pull High E", high_e);
   process("Slide G", g);
   process("Bend-Slide G", g);

#endif
#if PHRASE_TESTS==1 || STACCATO_TESTS==1 || ALL_TESTS==1

   process("GLines1", g, 10_ms, 50_ms);
   process("GLines2", g, 10_ms, 50_ms);
   process("GLines3", g, 10_ms, 50_ms);
   process("SingleStaccato", g, 10_ms, 50_ms);
   process("GStaccato", g, 10_ms, 50_ms);
   process("ShortStaccato", g, 10_ms, 50_ms);
   process("Attack-Reset", g, 10_ms, 50_ms);

#endif

   return 0;
}

