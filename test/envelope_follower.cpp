/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/literals.hpp>
#include <q/sfx.hpp>
#include <q_io/audio_file.hpp>
#include <vector>
#include <string>
#include "notes.hpp"

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
       : _window(float(0.5_ms) * sps)
       , _reset((float(hold) * sps) / _window)
      {}

      float operator()(float s)
      {
         // Do this every 0.5ms (window), collecting the peak in the meantime
         if (_i2++ != _window)
         {
            if (s > _peak)
               _peak = s;
            return _latest;
         }

         // This part of the code gets called every 0.5ms (window)
         // Get the peak and hold it in _y1 and _y2
         _i2 = 0;
         if (_peak > _y1)
            _y1 = _peak;
         if (_peak > _y2)
            _y2 = _peak;

         // Reset _y1 and _y2 alternately every so often (the hold parameter)
         if (_tick++ == _reset)
         {
            _tick = 0;
            if (_i++ & 1)
               _y1 = 0;
            else
               _y2 = 0;
         }

         // The peak is the maximum of _y1 and _y2
         _latest = std::max(_y1, _y2);
         _peak = 0;
         return _latest;
      }

      float _y1 = 0, _y2 = 0, _peak = 0, _latest = 0;
      std::uint16_t _tick = 0, _i = 0, _i2 = 0;
      std::uint16_t const _window, _reset;
   };
}}

void process(std::string name, q::duration hold)
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
   auto env = q::evf{ hold, sps };

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
   using namespace notes;

   process("1-Low E", low_e.period());
   process("Tapping D", d.period());
   process("Hammer-Pull High E", high_e.period());
   process("Bend-Slide G", g.period());
   process("GStaccato", g.period());

   return 0;
}