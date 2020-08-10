/*=============================================================================
   Copyright (c) 2014-2020 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>

#include <q/support/literals.hpp>
#include <q/pitch/dual_pitch_detector.hpp>
#include <q/pitch/pd_preprocessor.hpp>
#include <q_io/audio_file.hpp>

#include <vector>
#include <iostream>
#include <chrono>

#include "notes.hpp"

namespace q = cycfi::q;
using namespace q::literals;

int get_num(std::string const& s, int pos, float& num)
{
   auto new_pos = s.find_first_of(", ", pos);
   num = std::stod(s.substr(pos, new_pos));
   return new_pos + 2;
}

constexpr bool skip_tests = true;
constexpr auto break_time = 100.0;

void break_debug() // seconds
{
   // Set break_time to a specific time if you want to do some
   // timed debugging. Then set a break-point here.
}

void process(
   std::string name
 , q::frequency lowest_freq
 , q::frequency highest_freq)
{
   ////////////////////////////////////////////////////////////////////////////
   // Read audio file

   q::wav_reader src{"audio_files/" + name + ".wav"};
   std::uint32_t const sps = src.sps();

   std::vector<float> in(src.length());
   src.read(in);

   ////////////////////////////////////////////////////////////////////////////
   // Output
   constexpr auto n_channels = 2;
   std::vector<float> out(src.length() * n_channels);
   std::fill(out.begin(), out.end(), 0);

   ////////////////////////////////////////////////////////////////////////////
   // Process
   q::dual_pitch_detector     pd{ lowest_freq, highest_freq, sps };

   q::pd_preprocessor::config cfg;
   q::pd_preprocessor         pp{ cfg, lowest_freq, highest_freq, sps };

   std::uint64_t              nanoseconds = 0;

   for (auto i = 0; i != in.size(); ++i)
   {
      auto pos = i * n_channels;
      auto ch1 = pos;      // input
      auto ch2 = pos+1;    // frequency

      float time = i / float(sps);

      auto s = in[i];

      // Preprocessor
      s = pp(s);
      out[ch1] = s;

      if (time >= break_time)
         break_debug();

      // Pitch Detect
      auto start = std::chrono::high_resolution_clock::now();
      bool ready = pd(s);
      auto elapsed = std::chrono::high_resolution_clock::now() - start;
      auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed);
      nanoseconds += duration.count();

      // Print the frequency
      {
         auto f = pd.get_frequency() / double(highest_freq);
         out[ch2] = f;
      }
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
      "results/dual_pitch_detect_" + name + ".wav", n_channels, sps
   );
   wav.write(out);
}

void process(std::string name, q::frequency lowest_freq)
{
   process(name, lowest_freq * 0.8, lowest_freq * 4.8);
}

using namespace notes;

TEST_CASE("Test_low_frequencies")
{
   process("-2a-F#", low_fs);
   process("-2b-F#-12th", low_fs);
   process("-2c-F#-24th", low_fs);

   process("-1a-Low-B", low_b);
   process("-1b-Low-B-12th", low_b);
   process("-1c-Low-B-24th", low_b);
}

TEST_CASE("Test_basic")
{
   process("sin_440", d);

   process("1a-Low-E", low_e);
   process("1b-Low-E-12th", low_e);
   process("1c-Low-E-24th", low_e);

   process("2a-A", a);
   process("2b-A-12th", a);
   process("2c-A-24th", a);

   process("3a-D", d);
   process("3b-D-12th", d);
   process("3c-D-24th", d);
}

TEST_CASE("Test_basic2")
{
   process("4a-G", g);
   process("4b-G-12th", g);
   process("4c-G-24th", g);

   process("5a-B", b);
   process("5b-B-12th", b);
   process("5c-B-24th", b);

   process("6a-High-E", high_e);
   process("6b-High-E-12th", high_e);
   process("6c-High-E-24th", high_e);
}

TEST_CASE("Test_phrase")
{
   process("Tapping D", d);
   process("Hammer-Pull High E", high_e);
   process("Slide G", g);
   process("Bend-Slide G", g);
}

TEST_CASE("Test_glines")
{
   process("GLines1", g);
   process("GLines2", g);
   process("GLines3", g);
}

TEST_CASE("Test_staccato")
{
   process("SingleStaccato", g);
   process("GStaccato", g);
   process("ShortStaccato", g);
   process("Attack-Reset", g);
}

