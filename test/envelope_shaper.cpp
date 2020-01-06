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
 , std::uint32_t sps, q::duration hold
 , q::duration attack, q::duration decay, q::duration sustain)
{
   constexpr auto n_channels = 3;
   std::vector<float> out(in.size() * n_channels);

   ////////////////////////////////////////////////////////////////////////////

   auto i = out.begin();

   auto _env = q::fast_envelope_follower{ hold, sps };
   auto _onset = q::onset_detector{ -36_dB, sps };

   auto _eshape = q::envelope_shaper{
      attack      // Attack
    , decay       // Decay
    , -3_dB       // Sustain level
    , sustain     // Sustain Rate
    , -24_dB      // Release level
    , decay * 2   // Release
    , sps
   };

   for (auto s : in)
   {
      auto env = _env(s);
      auto onset = _onset(s) * env;
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

void process(
   std::string name, q::frequency f
 , q::duration attack = 10_ms
 , q::duration decay = 1_s
 , q::duration sustain = 15_s
)
{
   ////////////////////////////////////////////////////////////////////////////
   // Read audio file

   q::wav_reader src{"audio_files/" + name + ".wav"};
   std::uint32_t const sps = src.sps();

   std::vector<float> in(src.length());
   src.read(in);

   ////////////////////////////////////////////////////////////////////////////
   process(name, in, sps, f.period() * 1.1, attack, decay, sustain);
}

int main()
{
   process("1a-Low-E", low_e);
   process("1b-Low-E-12th", low_e, 100_ms);
   process("Tapping D", d);
   process("Hammer-Pull High E", high_e, 100_ms);
   process("Bend-Slide G", g, 100_ms);
   process("GStaccato", g, 10_ms, 10_ms, 50_ms);

   return 0;
}


