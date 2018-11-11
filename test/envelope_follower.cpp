/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/literals.hpp>
#include <q/sfx.hpp>
#include <q_io/audio_file.hpp>
#include <vector>
#include <string>

namespace q = cycfi::q;
namespace audio_file = q::audio_file;
using namespace q::literals;

namespace cycfi { namespace q
{
   struct evf
   {
      struct hold
      {
         void operator()(float s)
         {
            if (s > y)
               y = s;
         }
         float y = 0;
      };

      evf(frequency f, std::uint32_t sps)
       : _lp(f, sps)
       , tick(0)
       , i(0)
       , reset(float(f.period()) / 2 * sps)
      {}

      float operator()(float s)
      {
         y[0](s);
         y[1](s);
         y[2](s);
         y[3](s);

         if (tick++ == reset)
         {
            tick = 0;
            y[i++ & 3].y = 0;
         }
         return _lp(std::max_element(
            std::begin(y), std::end(y),
            [](auto a, auto b) { return a.y < b.y; }
         )->y);
      }

      hold y[4];
      std::uint16_t tick, i, reset;
      one_pole_lowpass _lp;
   };
}}

void process(std::string name)
{
   ////////////////////////////////////////////////////////////////////////////
   // Read audio file

   auto src = audio_file::reader{"audio_files/" + name + ".wav"};
   std::uint32_t const sps = src.sps();

   std::vector<float> in(src.length());
   src.read(in);

   ////////////////////////////////////////////////////////////////////////////
   // Attack detection

   constexpr auto n_channels = 2;

   std::vector<float> out(src.length() * n_channels);
   auto i = out.begin();

   // Envelope
   auto env = q::evf{ 50_Hz, sps };

   for (auto i = 0; i != in.size(); ++i)
   {
      auto pos = i * n_channels;
      auto ch1 = pos;
      auto ch2 = pos+1;

      auto s = in[i];

      // Original signal
      out[ch1] = s;

      // Envelope
      out[ch2] = env(std::abs(s));
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   auto wav = audio_file::writer{
           "results/env_follow_" + name + ".wav", n_channels, sps
   };
   wav.write(out);
}

int main()
{
   process("1-Low E");
   process("Tapping D");
   process("Hammer-Pull High E");
   process("Bend-Slide G");
   process("GStaccato");

   return 0;
}