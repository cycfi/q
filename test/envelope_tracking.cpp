/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/literals.hpp>
#include <q/sfx.hpp>
#include <q_io/audio_file.hpp>
#include <q/envelope.hpp>
#include <vector>
#include <string>

namespace q = cycfi::q;
namespace audio_file = q::audio_file;
using namespace q::literals;

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

   constexpr auto n_channels = 4;

   std::vector<float> out(src.length() * n_channels);
   auto i = out.begin();

   // Our envelope
   auto env_gen =
      q::envelope(
        15_ms     // attack rate
      , 70_ms     // decay rate
      , -6_dB     // sustain level
      , 50_s      // sustain rate
      , 15_ms     // release rate
      , sps
      );

   q::attack                  attack{ 0.6f, 100_ms, sps };
   q::peak_envelope_follower  env{ 1_s, sps };
   constexpr float            slope = 1.0f/10;
   q::compressor_expander     comp{ 0.5f, slope };
   q::clip                    clip;

   float                      onset_threshold = 0.005;
   float                      release_threshold = 0.001;
   float                      threshold = onset_threshold;

   for (auto i = 0; i != in.size(); ++i)
   {
      auto pos = i * n_channels;
      auto ch1 = pos;
      auto ch2 = pos+1;
      auto ch3 = pos+2;
      auto ch4 = pos+3;

      auto s = in[i];

      // Main envelope
      auto e = env(std::abs(s));

      // Compressor and noise gate
      if (e > threshold)
      {
         // Compressor + makeup-gain + hard clip
         s = clip(comp(s, e) * 1.0f/slope);
         threshold = release_threshold;
      }
      else
      {
         s = 0.0f;
         threshold = onset_threshold;
      }

      // Original signal
      out[ch1] = s;

      // Attack envelope
      out[ch3] = attack._env();

      // Attack
      auto prev = attack._lp();
      auto o = attack(s);
      out[ch2] = o;

      // Update generated envelope
      if (o != 0.0f)
      {
         env_gen.trigger(o, true); // trigger, no auto decay
      }
      else
      {
         if (env_gen.state() != q::envelope::note_off_state)
         {
            if (attack._lp() < env_gen.velocity() * 0.2) // <-- release threshold
            {
               // release
               env_gen.release();

               // Make the release envelope follow the input envelope
               env_gen.release_rate(attack._lp() / prev);
            }
            else
            {
               env_gen.decay();     // allow env_gen to proceed to decay
            }
         }
      }

      // Generated envelope
      auto ge = env_gen();
      out[ch4] = ge;
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   auto wav = audio_file::writer{
           "results/env_trk_" + name + ".wav", n_channels, sps
   };
   wav.write(out);
}

int main()
{
   process("Tapping D");
   process("Hammer-Pull High E");
   process("Bend-Slide G");
   process("GStaccato");

   return 0;
}