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

   ////////////////////////////////////////////////////////////////////////////
   // Process
   q::pitch_detector<>  pd{ lowest_freq, highest_freq, sps };
   q::dc_block          dc{ 1_Hz, sps };
   q::one_pole_lowpass  lp{ highest_freq, sps };

   for (auto i = 0; i != in.size(); ++i)
   {
      auto s = in[i];

      // Low pass and DC block
      s = lp(dc(s));

      // Pitch Detection
      bool proc = pd(s);

      if (proc)
      {
         std::cout << pd.frequency() << ", " << pd.periodicity() << std::endl;
         csv << pd.frequency() << ", " << pd.periodicity() << std::endl;
      }
   }

   csv.close();
}

int main()
{
   process("1-Low E", 70_Hz, 400_Hz);

   return 0;
}

