/*=============================================================================
   Copyright (c) 2014-2019 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/support/literals.hpp>
#include <q_io/audio_file.hpp>
#include <q/fx/onset_detector.hpp>
#include <q/fx/envelope_shaper.hpp>
#include <vector>
#include "notes.hpp"

namespace q = cycfi::q;
using namespace q::literals;
using namespace notes;

void process(
   std::string name, std::vector<float> const& in
 , std::uint32_t sps, q::duration hold)
{
   constexpr auto n_channels = 3;
   std::vector<float> out(in.size() * n_channels);

   ////////////////////////////////////////////////////////////////////////////
   // Detect waveform peaks

   auto i = out.begin();

   auto _env = q::fast_envelope_follower{ hold, sps };
   auto _onset = q::onset_detector{ -36_dB, sps };
   auto _edge = q::rising_edge{};
   auto _pulse = q::monostable{ 15_ms, sps };

   auto _eshape = q::envelope_shaper{
      10_ms    // Attack
    , 1_s      // Decay
    , 10_s     // Release
    , -3_dB    // Release threshold
    , sps
   };

   for (auto s : in)
   {
      auto env = _env(s);
      auto onset = _pulse(_edge(_onset(s))) * env;
      auto eshape = _eshape(onset);

      *i++ = s;
      *i++ = onset;
      *i++ = eshape;
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   q::wav_writer wav(
      "results/env_shaper_" + name + ".wav", n_channels, sps
   );
   wav.write(out);
}

void process(std::string name, q::frequency f)
{
   ////////////////////////////////////////////////////////////////////////////
   // Read audio file

   q::wav_reader src{"audio_files/" + name + ".wav"};
   std::uint32_t const sps = src.sps();

   std::vector<float> in(src.length());
   src.read(in);

   ////////////////////////////////////////////////////////////////////////////
   process(name, in, sps, f.period() * 1.1);
}

int main()
{
   process("1a-Low-E", low_e);
   process("1b-Low-E-12th", low_e);
   process("Tapping D", d);
   process("Hammer-Pull High E", high_e);
   process("Bend-Slide G", g);
   process("GStaccato", g);

   return 0;
}


