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
   //
   // Envelope follower combines fast response, low ripple
   // Based on http://tinyurl.com/yat2tuf8
   //
   struct evf
   {
      evf(duration hold, std::uint32_t sps)
       : _tick(0)
       , _i(0)
       , reset(float(hold) * sps)
      {}

      float operator()(float s)
      {
         if (s > _y1)
            _y1 = s;
         if (s > _y2)
            _y2 = s;

         if (_tick++ == reset)
         {
            _tick = 0;
            if (_i++ & 1)
               _y1 = 0;
            else
               _y2 = 0;
         }
         return std::max(_y1, _y2);
      }

      float _y1, _y2;
      std::uint16_t _tick, _i, reset;
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
   auto env = q::evf{ 10_ms, sps };

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