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

   auto diff1 = q::differentiator{};
   auto diff2 = q::central_difference{};
   auto diff3 = q::differentiator{};
   auto diff4 = q::differentiator{};
   auto fast = q::fast_envelope_follower{ 4_ms, sps };
   auto env = q::envelope_follower{ 10_ms, 50_ms, sps };
   auto pk = q::peak{ 0.9f, 0.001f };
   auto cmp = q::timed_schmitt_trigger{ -32_dB, 15_ms, sps };

   for (auto s : in)
   {
      // Normalize
      s *= 1.0 / max_val;
      *i++ = s;

      // Second dirivative (acceleration)
      auto d1 = diff1(s);
      auto d2 = diff2(d1);
      auto d3 = diff3(d2);
      auto d4 = diff3(d3);

      // Fast Envelope Follower
      auto fe = fast(d1) * 10;

      // Peak detection
      auto e = env(fe);
      auto cm = cmp(fe, e);

      // *i++ = d2 * 10;
      // *i++ = d3 * 10;
      // *i++ = d4 * 10;

      *i++ = d1 * 10;
      *i++ = fe;
      *i++ = cm * 0.8;
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
   // process("Tapping D");
   process("Hammer-Pull High E");
   // process("Bend-Slide G");
   // process("GStaccato");
   return 0;
}

