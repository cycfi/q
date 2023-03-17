/*=============================================================================
   Copyright (c) 2014-2023 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/support/literals.hpp>
#include <q_io/audio_file.hpp>
#include <q/fx/signal_conditioner.hpp>
#include <q/fx/zero_crossing.hpp>

#include <vector>
#include <string>
#include <fstream>
#include "notes.hpp"

namespace q = cycfi::q;
using namespace q::literals;
using namespace notes;

void process(
   std::string name, std::vector<float> const& in
 , float sps, q::frequency f)
{
   // Prepare output file
   std::ofstream csv("results/pulses_" + name + ".csv");

   constexpr auto n_channels = 3;
   std::vector<float> out(in.size() * n_channels);

   auto sc_conf = q::signal_conditioner::config{};
   auto sig_cond = q::signal_conditioner{sc_conf, f, f*4, sps};
   auto zc = q::zero_crossing{-45_dB};
   auto zcx = q::zero_crossing_ex{-45_dB};

   float* edge_pos = 0;
   for (auto i = 0; i != in.size(); ++i)
   {
      auto pos = i * n_channels;
      auto ch1 = pos;
      auto ch2 = pos+1;
      auto ch3 = pos+2;

      auto s = in[i];

      // Signal conditioner
      s = sig_cond(s);

      // Original signal
      out[ch1] = s;

      // Zero Crossing
      out[ch2] = zc(s) * 0.8;

      // Extended Zero Crossing
      auto r = zcx(s);
      if (r == 1)
      {
         edge_pos = &out[ch3];
      }
      else if (r == -1)
      {
         auto info = zcx.get_info();
         auto w = info.width();
         auto h = info.height();

         for (auto i = 0; i != w; ++i)
         {
            *edge_pos = h;
            edge_pos += n_channels;
         }

         csv << w << ", " << h << std::endl;
      }
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   q::wav_writer wav(
      "results/zero_crossing_" + name + ".wav", n_channels, sps
   );
   wav.write(out);
   csv.close();
}

void process(std::string name, q::frequency f)
{
   ////////////////////////////////////////////////////////////////////////////
   // Read audio file

   q::wav_reader src{"audio_files/" + name + ".wav"};
   float const sps = src.sps();

   std::vector<float> in(src.length());
   src.read(in);

   ////////////////////////////////////////////////////////////////////////////
   process(name, in, sps, f);
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