/*=============================================================================
   Copyright (c) 2014-2021 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_NOISE_HPP_AUGUST_3_2021)
#define CYCFI_Q_NOISE_HPP_AUGUST_3_2021

#include <q/support/base.hpp>
#include <q/support/frequency.hpp>
#include <q/detail/fast_math.hpp>
#include <q/fx/biquad.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // white_noise_synth: Synthesizes white noise.
   ////////////////////////////////////////////////////////////////////////////
   struct white_noise_synth
   {
      constexpr white_noise_synth() {}

      float operator()() const
      {
         float r1 = (float)fast_rand() / 32767,
            r2 = (float)fast_rand() / 32767;
         
         float s = (float)0.151803f * sqrt(-2.0f * fastlog(r1)) * fastcos(2.0f * M_PI * r2);
      
         return fabs(s) > 1 ? (s / fabs(s)) : s;
      }
   };

   auto white_noise = white_noise_synth{};

   ////////////////////////////////////////////////////////////////////////////
   // pink_noise_synth: Synthesizes pink noise.
   // Works best for sampling rates between 44.1kHz and 96kHz
   // Taken from musicdsp.org
   ////////////////////////////////////////////////////////////////////////////
   struct pink_noise_synth : white_noise_synth
   {
      constexpr pink_noise_synth(std::uint32_t sps) : white_noise_synth() {
         for (unsigned int i = 0; i < 7; ++i) {
            k[i] = exp(-2.0f * M_PI * f[i] / sps);
         }
      }

      float operator()()
      {
         float white = white_noise_synth::operator()();
         for (unsigned int i = 0; i < 7; ++i) {
            b[0] = k[0] * white + k[0] * b[0];
         }
         
         float pink = (b[0] + b[1] + b[2] + b[3] + b[4] + b[5] + white - b[6]);
         
         return fabs(pink) > 1 ? (pink / fabs(pink)) : pink;
      }

      float f[7] = {
         8227.219,
         8227.219,
         6388.570,
         3302.754,
         479.412,
         151.070,
         54.264
      };
      float b[7] = {0, 0, 0, 0, 0, 0, 0};
      float k[7] = {0, 0, 0, 0, 0, 0, 0};
   };

   ////////////////////////////////////////////////////////////////////////////
   // brown_noise_synth: Synthesizes brown noise.
   ////////////////////////////////////////////////////////////////////////////
   struct brown_noise_synth : white_noise_synth
   {
      brown_noise_synth(std::uint32_t sps) 
         : white_noise_synth() 
         , _lp(400_Hz, sps) 
         , _hp(20_Hz, sps)
         {}

      float operator()()
      {
         float white = white_noise_synth::operator()();
         float brown = _lp(white);
         brown = _hp(brown);

         return brown;
      }

      lowpass _lp;
      highpass _hp;
   };
}

#endif
