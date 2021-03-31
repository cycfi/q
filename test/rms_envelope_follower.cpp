/*=============================================================================
   Copyright (c) 2014-2021 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/support/literals.hpp>
#include <q_io/audio_file.hpp>
#include <q/fx/dynamic.hpp>
#include <q/fx/envelope.hpp>
#include <q/fx/moving_average.hpp>
#include <q/fx/lowpass.hpp>
#include <q/fx/special.hpp>
#include <q/fx/biquad.hpp>
#include <vector>
#include <string>
#include "notes.hpp"

namespace q = cycfi::q;
using namespace q::literals;
using namespace notes;

void process(std::string name, q::duration period)
{
   ////////////////////////////////////////////////////////////////////////////
   // Read audio file

   q::wav_reader src{"audio_files/" + name + ".wav"};
   std::uint32_t const sps = src.sps();

   std::vector<float> in(src.length());
   src.read(in);

   ////////////////////////////////////////////////////////////////////////////
   // Fast RMS envelope follower

   constexpr auto n_channels = 2;
   std::vector<float> out(src.length() * n_channels);

   auto max_val = *std::max_element(in.begin(), in.end(),
      [](auto a, auto b) { return std::abs(a) < std::abs(b); }
   );

   auto env = q::fast_rms_envelope_follower{ period/2, sps };

   for (auto i = 0; i != in.size(); ++i)
   {
      auto pos = i * n_channels;
      auto ch1 = pos;
      auto ch2 = pos+1;

      auto s = in[i];

      // Normalize
      s *= 1.0 / max_val;

      // Original signal
      out[ch1] = s;

      // Envelope
      out[ch2] = float(env(s));
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   q::wav_writer wav(
      "results/rms_envelope_follower_" + name + ".wav", n_channels, sps
   );
   wav.write(out);
}

int main()
{
   process("sin_envelope", a.period());
   process("1a-Low-E", low_e.period());
   process("1b-Low-E-12th", low_e.period());
   process("Tapping D", d.period());
   process("Hammer-Pull High E", high_e.period());
   process("Bend-Slide G", g.period());
   process("GStaccato", g.period());

   return 0;
}