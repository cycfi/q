/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <q/literals.hpp>
#include <q/sfx.hpp>
#include <q/pitch_detector.hpp>
#include <q_io/audio_file.hpp>

#include <vector>
#include <iostream>
#include <fstream>

#include "notes.hpp"

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
   constexpr auto n_channels = 4;
   std::vector<float> out(src.length() * n_channels);
   std::fill(out.begin(), out.end(), 0);

   ////////////////////////////////////////////////////////////////////////////
   // Process
   q::pitch_detector<>  pd{ lowest_freq, highest_freq, sps };
   q::edges const&      edges = pd.edges();
   q::bacf<> const&     bacf = pd.bacf();
   q::dynamic_smoother  lp{ lowest_freq / 2, 0.5, sps };

   for (auto i = 0; i != in.size(); ++i)
   {
      auto pos = i * n_channels;
      auto s = in[i];

      s = s * global_norm;
      out[pos] = s;

      s = lp(s);
      out[pos + 1] = s;

      bool proc = pd(s);
      out[pos + 2] = edges()? 0.8 : 0;

      out[pos + 3] = -0.8;   // Default placeholder

      if (proc)
      {
         auto out_i = (&out[pos + 3] - (bacf.size() * n_channels));
         auto const& info = bacf.result();
         for (auto n : info.correlation)
         {
            *out_i = n / float(info.max_count);
            out_i += n_channels;
         }
      }

      // // Pitch Detection
      // bool proc = pd(s);

      // // Default placeholder
      // out[pos + 1] = -0.8;

      // // Bits
      // out[pos + 2] = bacf[count++] ? 0.8 : 0;

      // if (proc)
      // {
      //    count = 0;
      //    csv << pd.frequency() << ", " << pd.periodicity() << std::endl;

      //    auto out_i = (&out[pos + 1] - (bacf.size() * n_channels));
      //    auto const& info = bacf.result();
      //    for (auto n : info.correlation)
      //    {
      //       *out_i = n / float(info.max_count);
      //       out_i += n_channels;
      //    }

      //    // out_i = (&out[pos + 2] - (bacf.size() * n_channels));
      //    // for (auto i = 0; i != bacf.size(); ++i)
      //    // {
      //    //    *out_i = bacf[i] ? 0.8 : 0;
      //    //    out_i += n_channels;
      //    // }
      // }

      // auto f = pd.frequency();
      // if (f != -1.0f)
      //    f /= double(highest_freq);
      // auto fi = int(i - bacf.size());
      // if (fi >= 0)
      //    out[(fi * n_channels) + 3] = f;
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

void process(std::string name, q::frequency lowest_freq)
{
   process(name, lowest_freq, lowest_freq * 4);
}

int main()
{
   using namespace notes;

   process("sin_440", d);
   process("1-Low E", low_e);
   process("2-Low E 2th", low_e);
   process("5-D", d);
   process("6-D 12th", d);
   process("Tapping D", d);
   process("harmonics_1318", high_e);

   return 0;
}

