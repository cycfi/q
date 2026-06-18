/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/support/literals.hpp>
#include <q_io/audio_file.hpp>
#include <q/fx/differentiator.hpp>
#include <q/fx/signal_conditioner.hpp>
#include <q/fx/integrator.hpp>

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
 , float sps, q::frequency f)
{
   constexpr auto n_channels = 3;
   std::vector<float> out(in.size() * n_channels);

   auto sc_conf = q::signal_conditioner::config{};
   auto sig_cond = q::signal_conditioner{sc_conf, f, f*4, sps};
   auto vel = q::slope{16};
   auto acc = q::slope{16};

   for (auto i = 0; i != in.size(); ++i)
   {
      auto pos = i * n_channels;
      auto ch1 = pos;
      auto ch2 = pos+1;
      auto ch3 = pos+2;

      auto s = in[i];

      // Signal conditioner
      s = sig_cond(s);

      // Original signal
      out[ch1] = s;

      // Velocity
      out[ch2] = vel(s);

      // Acceleration
      out[ch3] = acc(out[ch2]);
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   if (!q_test::suppress_wav())
   {
      q::wav_writer wav(
         "results/signal_slope_" + name + ".wav", n_channels, sps
      );
      wav.write(out);
   }

   std::filesystem::create_directories("results/golden");
   auto g_rows = q_test::windowed_level_csv(out, n_channels, sps);
   auto g_cols = q_test::level_columns(n_channels);
   q_test::write_golden_csv("results/golden/signal_slope/" + name + ".csv", g_cols, g_rows);
   q_test::compare_golden_csv("signal_slope/" + name, g_cols, g_rows);
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
   process(name, in, sps, f);
}

TEST_CASE("signal_slope: audio files")
{
   process("1a-Low-E", low_e);
   process("1b-Low-E-12th", low_e);
   process("Tapping D", d);
   process("Hammer-Pull High E", high_e);
   process("Bend-Slide G", g);
   process("GStaccato", g);
}