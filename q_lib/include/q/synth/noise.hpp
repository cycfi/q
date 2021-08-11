/*=================================================================================
   Copyright (c) 2014-2021 Joel de Guzman, Nikos Parastatidis. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
==================================================================================*/
#if !defined(CYCFI_Q_NOISE_HPP_AUGUST_3_2021)
#define CYCFI_Q_NOISE_HPP_AUGUST_3_2021

#include <q/detail/fast_math.hpp>
#include <q/fx/lowpass.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // white_noise_synth:
   // Synthesizes white noise using a fast random number generator.
   // Outputs values between -1 and 1.
   // Source: https://www.musicdsp.org/en/latest/Synthesis/216-fast-whitenoise-generator.html
   ////////////////////////////////////////////////////////////////////////////
   struct white_noise_synth
   {
      constexpr white_noise_synth() {}

      float operator()()
      {
         x1 ^= x2;
         s = x2 * scale;
         x2 += x1;

         return s;
      }

      int x1 = 0x67452301;
      int x2 = 0xefcdab89;
      constexpr static float scale = 2.0f / 0xffffffff;
      float s = 0.0f;
   };

   auto white_noise = white_noise_synth{};

   ////////////////////////////////////////////////////////////////////////////
   // pink_noise_synth:
   // Creates pink noise from white noise through a 3db/octave highpass
   // Works best for sampling rates between 44.1kHz and 96kHz
   // Source: https://www.musicdsp.org/en/latest/Filters/76-pink-noise-filter.html
   ////////////////////////////////////////////////////////////////////////////
   struct pink_noise_synth : white_noise_synth
   {
      pink_noise_synth(std::uint32_t sps) : white_noise_synth() {
         for (unsigned int i = 0; i < 7; ++i) {
            k[i] = fastexp(-2.0f * M_PI * f[i] / sps);
         }
      }

      float operator()()
      {
         auto white = white_noise_synth::operator()();

         for (unsigned int i = 0; i < 7; ++i) {
            b[i] = k[i] * (white + b[i]);
         }

         pink = 0.05f * (b[0] + b[1] + b[2] + b[3] + b[4] + b[5] + white - b[6]);

         return pink;
      }

      constexpr static float f[7] = {
         8227.219f,
         8227.219f,
         6388.570f,
         3302.754f,
         479.412f,
         151.070f,
         54.264f
      };
      float k[7] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
      float b[7] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
      float pink = 0.0f;
   };

   ////////////////////////////////////////////////////////////////////////////
   // brown_noise_synth:
   // Creates brown noise from white noise through a 6db/octave lowpass
   ////////////////////////////////////////////////////////////////////////////
   struct brown_noise_synth : white_noise_synth
   {
      brown_noise_synth(std::uint32_t sps)
         : white_noise_synth()
         , _lp(1_Hz, sps)
         {}

      float operator()()
      {
         auto white = white_noise_synth::operator()();
         auto brown = 20.0f * _lp(white);

         return brown;
      }

      one_pole_lowpass _lp;
   };
}

#endif
