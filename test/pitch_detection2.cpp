/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <q/literals.hpp>
#include <q/sfx.hpp>
#include <q/pitch_detector.hpp>
#include <q_io/audio_file.hpp>
#include <q/notes.hpp>

#include <vector>
#include <iostream>
#include <fstream>

namespace q = cycfi::q;
using namespace q::literals;
namespace audio_file = q::audio_file;

void process(
   std::string name
 , q::frequency lowest_freq
 , q::frequency highest_freq)
{
   ////////////////////////////////////////////////////////////////////////////
   // Prepare output file

   std::ofstream csv("results/" + name + ".csv");

   ////////////////////////////////////////////////////////////////////////////
   // Read audio file

   auto src = audio_file::reader{"audio_files/" + name + ".aif"};
   std::uint32_t const sps = src.sps();

   std::vector<float> in(src.length());
   src.read(in);

   auto global_max_val = *std::max_element(in.begin(), in.end(),
      [](auto a, auto b) { return std::abs(a) < std::abs(b); }
   );
   global_max_val = std::abs(global_max_val);
   auto global_norm = 1.0 / global_max_val;

   ////////////////////////////////////////////////////////////////////////////
   // Output
   constexpr auto n_channels = 3;
   std::vector<float> out(src.length() * n_channels);
   std::fill(out.begin(), out.end(), 0);

   ////////////////////////////////////////////////////////////////////////////
   // Process
   q::pitch_detector<>  pd{ lowest_freq, highest_freq, sps };
   q::dc_block          dc{ 1_Hz, sps };
   q::one_pole_lowpass  lp{ highest_freq, sps };
   auto const&          bacf = pd.bacf();

   for (auto i = 0; i != in.size(); ++i)
   {
      auto pos = i * n_channels;
      auto s = in[i];
      out[pos] = s * global_norm;

      // Low pass and DC block
      s = lp(dc(s));

      // Pitch Detection
      bool proc = pd(s);

      // Default placeholder
      out[pos + 1] = -0.8;

      if (proc)
      {
         csv << pd.frequency() << ", " << pd.periodicity() << std::endl;

         auto out_i = (&out[pos + 1] - (bacf.size() * n_channels));
         auto const& info = bacf.result();
         for (auto n : info.correlation)
         {
            *out_i = n / float(info.max_count);
            out_i += n_channels;
         }
      }

      auto f = pd.frequency();
      if (f != -1.0f)
         f /= double(highest_freq);
      auto fi = int(i - bacf.size());
      if (fi >= 0)
         out[(fi * n_channels) + 2] = f;
   }

   csv.close();

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   auto wav = audio_file::writer{
      "results/pitch_detect_" + name + ".wav", audio_file::wav, audio_file::_16_bits
    , n_channels, sps
   };
   wav.write(out);
}

int main()
{
   process("1-Low E", 70_Hz, 400_Hz);
   process("5-D", 100_Hz, 600_Hz);
   process("Tapping D", 100_Hz, 600_Hz);

   return 0;
}

