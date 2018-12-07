/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/support/literals.hpp>
#include <q/fx.hpp>
#include <q_io/audio_file.hpp>
#include <vector>

namespace q = cycfi::q;
using namespace q::literals;

void process(std::string name, q::frequency cutoff)
{
   ////////////////////////////////////////////////////////////////////////////
   // Read audio file

   auto src = q::wav_reader{"audio_files/" + name + ".wav"};
   std::uint32_t const sps = src.sps();

   std::vector<float> in(src.length());
   src.read(in);

   auto max_val = *std::max_element(in.begin(), in.end(),
      [](auto a, auto b) { return std::abs(a) < std::abs(b); }
   );

   ////////////////////////////////////////////////////////////////////////////
   // Detect zero crossings

   constexpr auto n_channels = 3;

   std::vector<float> out(src.length() * n_channels);
   auto i = out.begin();

   q::one_pole_lowpass lp{ cutoff, sps };
   q::zero_cross zc{ 0.001 };

   for (auto s : in)
   {
      // Normalize
      s *= 1.0 / max_val;

      // Original
      *i++ = s;

      // Low pass
      s = lp(s);

      *i++ = s;
      *i++ = zc(s) * 0.8;
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   auto wav = q::wav_writer{
      "results/zero_" + name + ".wav", n_channels, sps
   };
   wav.write(out);
}

int main()
{
   process("1-Low E", 329.64_Hz);
   process("2-Low E 2th", 329.64_Hz);
   return 0;
}

