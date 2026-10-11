/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/support/literals.hpp>
#include <q_io/audio_file.hpp>
#include <q/fx/lowpass.hpp>
#include <q/fx/envelope.hpp>
#include <q/fx/signal_conditioner.hpp>
#include <q/fx/peak.hpp>
#include <vector>
#include <type_traits>
#include "golden_csv.hpp"
#include <filesystem>

namespace q = cycfi::q;
using namespace q::literals;

void process(std::string name, q::frequency cutoff)
{
   ////////////////////////////////////////////////////////////////////////////
   // Read audio file

   q::wav_reader src{"audio_files/" + name + ".wav"};
   float const sps = src.sps();

   std::vector<float> in(src.length());
   src.read(in);

   ////////////////////////////////////////////////////////////////////////////
   // Detect waveform peaks

   constexpr auto n_channels = 3;
   std::vector<float> out(src.length() * n_channels);
   auto i = out.begin();

   auto sc_conf = q::signal_conditioner::config{};
   q::frequency f = cutoff/4;
   auto sig_cond = q::signal_conditioner{sc_conf, f, f*4, sps};

   q::peak pk{ 0.95f, -40_dB };
   q::peak_envelope_follower env{ cutoff.period()*16, sps };

   for (auto s : in)
   {
      // Signal conditioner
      s = sig_cond(s);
      *i++ = s;

      *i++ = pk(s, env(s)) * 0.8;
      *i++ = env();
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   if (!q_test::suppress_wav())
   {
      q::wav_writer wav(
         "results/peaks_" + name + ".wav", n_channels, sps
      );
      wav.write(out);
   }

   std::filesystem::create_directories("results/golden");
   auto g_rows = q_test::windowed_level_csv(out, n_channels, sps);
   auto g_cols = q_test::level_columns(n_channels);
   q_test::write_golden_csv("results/golden/peaks/" + name + ".csv", g_cols, g_rows);
   q_test::compare_golden_csv("peaks/" + name, g_cols, g_rows);
}

TEST_CASE("peaks: audio files")
{
   process("1a-Low-E", 329.64_Hz);
   process("1b-Low-E-12th", 329.64_Hz);
   process("2a-A", 440.00_Hz);
   process("2b-A-12th", 440.00_Hz);
   process("3a-D", 587.32_Hz);
   process("3b-D-12th", 587.32_Hz);
   process("4a-G", 784.00_Hz);
   process("4b-G-12th", 784.00_Hz);
   process("5a-B", 987.76_Hz);
   process("5b-B-12th", 987.76_Hz);
   process("6a-High-E", 1318.52_Hz);
   process("6b-High-E-12th", 1318.52_Hz);
   process("Tapping D", 587.32_Hz);
   process("Hammer-Pull High E", 1318.52_Hz);
}

TEST_CASE("peaks: schmitt_trigger is copy assignable")
{
   static_assert(std::is_copy_assignable_v<q::schmitt_trigger>);

   q::schmitt_trigger a{0.1f};
   q::schmitt_trigger b{0.5f};
   CHECK(b(1.0f, 0.0f));
   a = b;
   CHECK(a());                   // The state is copied
   CHECK(a(-0.2f, 0.0f));        // Inside b's band (+/-0.5): stays high
   CHECK_FALSE(a(-0.6f, 0.0f));
}

TEST_CASE("peaks: peak is copy assignable")
{
   static_assert(std::is_copy_assignable_v<q::peak>);

   q::peak a{0.5f, 0.0f};
   q::peak b{0.9f, 0.1f};
   CHECK(b(1.0f, 0.0f));
   a = b;
   CHECK(a());                   // The state is copied
   CHECK_FALSE(a(0.75f, 1.0f));  // Below 0.9 * 1.0 - 0.1: goes low
}
