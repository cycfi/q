/*=============================================================================
   Copyright (c) 2014-2020 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/support/literals.hpp>
#include <q/fx/dynamic.hpp>
#include <q/synth/saw.hpp>
#include <q_io/audio_file.hpp>
#include <array>

namespace q = cycfi::q;
using namespace q::literals;

constexpr auto sps = 44100;
constexpr auto size = sps;
constexpr auto buffer_size = size;

constexpr auto n_channels = 1;
using in_buffer = std::array<float, buffer_size>;
using out_buffer = std::array<float, buffer_size*n_channels>;

///////////////////////////////////////////////////////////////////////////////
void test_compressor(in_buffer const& ramp)
{
   q::wav_reader src{ "audio_files/curve_compressor.wav" };
   out_buffer out;

   auto comp = q::compressor{ -6_dB, 1.0/4 };

   for (auto i = 0; i != size; ++i)
   {
      auto pos = i * n_channels;
      auto ch1 = pos;
      auto s = ramp[i];
      out[ch1] = s * float(comp(q::decibel(s)));
   }

   q::wav_writer wav(
      "results/curve_compressor.wav", n_channels, sps
   );
   wav.write(out);
}

///////////////////////////////////////////////////////////////////////////////
void test_soft_knee_compressor(in_buffer const& ramp)
{
   q::wav_reader src{ "audio_files/curve_soft_knee_compressor.wav" };
   out_buffer out;

   auto comp = q::soft_knee_compressor{ -6_dB, 3_dB, 1.0/4 };

   for (auto i = 0; i != size; ++i)
   {
      auto pos = i * n_channels;
      auto ch1 = pos;
      auto s = ramp[i];
      out[ch1] = s * float(comp(q::decibel(s)));
   }

   q::wav_writer wav(
      "results/curve_soft_knee_compressor.wav", n_channels, sps
   );
   wav.write(out);
}

///////////////////////////////////////////////////////////////////////////////
void test_expander(in_buffer const& ramp)
{
   q::wav_reader src{ "audio_files/curve_expander.wav" };
   out_buffer out;

   auto exp = q::expander{ -6_dB, 4/1.0 };

   for (auto i = 0; i != size; ++i)
   {
      auto pos = i * n_channels;
      auto ch1 = pos;
      auto s = ramp[i];
      out[ch1] = s * float(exp(q::decibel(s)));
   }

   q::wav_writer wav(
      "results/curve_expander.wav", n_channels, sps
   );
   wav.write(out);
}

///////////////////////////////////////////////////////////////////////////////
void test_agc(in_buffer const& ramp)
{
   q::wav_reader src{ "audio_files/curve_agc.wav" };
   out_buffer out;

   auto agc = q::agc{ 12_dB };

   for (auto i = 0; i != size; ++i)
   {
      auto pos = i * n_channels;
      auto ch1 = pos;
      auto s = ramp[i];
      out[ch1] = s * float(agc(q::decibel(s), -6_dB));
   }

   q::wav_writer wav(
      "results/curve_agc.wav", n_channels, sps
   );
   wav.write(out);
}

///////////////////////////////////////////////////////////////////////////////
void gen_ramp(in_buffer& ramp)
{
   const auto f = q::phase(1_Hz, sps);       // The synth frequency
   auto ph = q::phase();                     // Our phase accumulator

   for (auto i = 0; i != size; ++i)
   {
      ramp[i] = (q::basic_saw(ph) + 1) / 2;
      ph += f;
   }
}

///////////////////////////////////////////////////////////////////////////////
int main()
{
   auto ramp = in_buffer{};
   gen_ramp(ramp);

   test_compressor(ramp);
   test_soft_knee_compressor(ramp);
   test_expander(ramp);
   test_agc(ramp);

   return 0;
}
