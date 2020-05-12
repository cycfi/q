/*=============================================================================
   Copyright (c) 2014-2020 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/support/literals.hpp>
#include <q/synth/sin.hpp>
#include <q/fx/allpass.hpp>
#include <q_io/audio_file.hpp>
#include <array>

namespace q = cycfi::q;
using namespace q::literals;

constexpr auto sps = 48000;
constexpr auto buffer_size = std::size_t(sps * 0.1);

int main()
{
   auto buff = std::array<float, buffer_size * 2>{};  // The output buffer
   auto i = buff.begin();

   constexpr auto f = q::phase(440_Hz, sps);          // The synth frequency
   auto ph = q::phase();                              // Our phase accumulator

   auto apf = q::one_pole_allpass{440_Hz, sps};

   for (auto i_ = 0; i_ != buffer_size; ++i_)
   {
      auto out = q::sin(ph);
      *i++ = out;
      *i++ = apf(out);
      ph += f;
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   q::wav_writer wav(
      "results/allpass.wav", 2, sps
   );
   wav.write(buff);

   return 0;
}
