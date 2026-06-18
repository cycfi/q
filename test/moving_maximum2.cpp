/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/support/literals.hpp>
#include <q_io/audio_file.hpp>
#include <q/fx/moving_maximum.hpp>
#include <vector>
#include <string>
#include "pitch.hpp"
#include "golden_csv.hpp"
#include <filesystem>

namespace q = cycfi::q;
using namespace q::literals;
using namespace notes;

void process(
   std::string name, std::vector<float> const& in
 , float sps, std::size_t n)
{
   constexpr auto n_channels = 2;
   std::vector<float> out(in.size() * n_channels);

   auto mmax = q::moving_maximum<float>{ n };

   for (auto i = 0; i != in.size(); ++i)
   {
      auto pos = i * n_channels;
      auto ch1 = pos;
      auto ch2 = pos+1;
      auto ch3 = pos+2;
      auto ch4 = pos+3;

      auto s = in[i];

      // Original signal
      out[ch1] = s;

      // Peak
      out[ch2] = mmax(s);
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   if (!q_test::suppress_wav())
   {
      q::wav_writer wav(
         "results/moving_maximum_" + name + ".wav", n_channels, sps
      );
      wav.write(out);
   }

   std::filesystem::create_directories("results/golden");
   auto g_rows = q_test::windowed_level_csv(out, n_channels, sps);
   auto g_cols = q_test::level_columns(n_channels);
   q_test::write_golden_csv("results/golden/moving_maximum/" + name + ".csv", g_cols, g_rows);
   q_test::compare_golden_csv("moving_maximum/" + name, g_cols, g_rows);
}

void process(std::string name, q::frequency f)
{
   ////////////////////////////////////////////////////////////////////////////
   // Read audio file

   q::wav_reader src{"audio_files/" + name + ".wav"};
   float const sps = src.sps();

   std::vector<float> in(src.length());
   src.read(in);

   ////////////////////////////////////////////////////////////////////////////
   auto period = f.period();
   std::size_t n = as_float(period) * sps;
   process(name, in, sps, n * 1.1);
}

TEST_CASE("moving_maximum2: audio files")
{
   process("1a-Low-E", low_e);
   process("1b-Low-E-12th", low_e);
   process("Tapping D", d);
   process("Hammer-Pull High E", high_e);
   process("Bend-Slide G", g);
   process("GStaccato", g);
}