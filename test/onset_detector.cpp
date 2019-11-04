/*=============================================================================
   Copyright (c) 2014-2019 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/support/literals.hpp>
#include <q_io/audio_file.hpp>
#include <q/fx/lowpass.hpp>
#include <q/fx/envelope.hpp>
#include <q/fx/feature_detection.hpp>
#include <q/fx/special.hpp>
#include <vector>

namespace q = cycfi::q;
using namespace q::literals;

void process(std::string name)
{
   ////////////////////////////////////////////////////////////////////////////
   // Read audio file

   q::wav_reader src{"audio_files/" + name + ".wav"};
   std::uint32_t const sps = src.sps();

   std::vector<float> in(src.length());
   src.read(in);

   auto max_val = *std::max_element(in.begin(), in.end(),
      [](auto a, auto b) { return std::abs(a) < std::abs(b); }
   );

   ////////////////////////////////////////////////////////////////////////////
   // Detect waveform peaks

   constexpr auto n_channels = 4;
   std::vector<float> out(src.length() * n_channels);
   auto i = out.begin();

   auto diff1 = q::central_difference{};
   auto diff2 = q::central_difference{};
   auto fast = q::fast_envelope_follower{ 4_ms, sps };
   auto env1 = q::peak_envelope_follower{ 40_ms, sps };
   auto env2 = q::envelope_follower{ 10_ms, 50_ms, sps };
   auto pk = q::peak{ 0.9f, 0.001f };

   for (auto s : in)
   {
      // Normalize
      s *= 1.0 / max_val;
      *i++ = s;

      // Second dirivative (acceleration)
      s = diff1(diff2(s));

      // Fast Envelope Follower
      auto fe = fast(abs(s)) * 10;

      // Peak detection
      auto e1 = env1(fe);
      auto e2 = env2(fe);


      auto th = (fe - float(-36_dB)) > e2;
      // auto th = (e1 * 0.9) > e2;
      // auto gate = e2 > float(-36_dB);

      *i++ = e1;
      *i++ = fe;
      *i++ = th * 0.8;
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   q::wav_writer wav(
      "results/onset_" + name + ".wav", n_channels, sps
   );
   wav.write(out);
}

int main()
{
   // process("1a-Low-E");
   process("Tapping D");
   // process("Hammer-Pull High E");
   // process("Bend-Slide G");
   // process("GStaccato");
   return 0;
}

