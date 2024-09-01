/*=================================================================================
   Copyright (c) 2014-2024 Joel de Guzman, Nikos Parastatidis. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
==================================================================================*/
#if !defined(CYCFI_Q_NOISE_GEN_HPP_AUGUST_3_2021)
#define CYCFI_Q_NOISE_GEN_HPP_AUGUST_3_2021

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // white_noise_gen generates white noise using a fast random number
   // generator. Outputs values between -1 and 1.
   //
   // Source:
   // https://www.musicdsp.org/en/latest/Synthesis/216-fast-whitenoise-generator.html
   ////////////////////////////////////////////////////////////////////////////
   struct white_noise_gen
   {
      float operator()()
      {
         x1 ^= x2;
         s = x2 * scale;
         x2 += x1;

         return s;
      }

      std::uint32_t x1 = 0x67452301;
      std::uint32_t x2 = 0xefcdab89;
      constexpr static float scale = 2.0f / 0xffffffff;
      float s = 0.0f;
   };

   auto white_noise = white_noise_gen{};

   ////////////////////////////////////////////////////////////////////////////
   // pink_noise_gen generates pink noise from white noise through a
   // 3db/octave (-10dB/decade) filter using a weighted sum of first order
   // filters with accuracy of +/-0.5dB. The result will have an almost
   // gaussian level distribution.
   //
   // Source:
   // https://www.musicdsp.org/en/latest/Filters/76-pink-noise-filter.html
   ////////////////////////////////////////////////////////////////////////////
   struct pink_noise_gen : white_noise_gen
   {
      static constexpr float scale = 0.2f;
      static constexpr float c1 = 0.99765f;
      static constexpr float c2 = 0.0990460f * scale;
      static constexpr float c3 = 0.96300f;
      static constexpr float c4 = 0.2965164f * scale;
      static constexpr float c5 = 0.57000f;
      static constexpr float c6 = 1.0526913f * scale;
      static constexpr float c7 = 0.1848f * scale;

      float operator()()
      {
         auto white = white_noise_gen::operator()();
         b0 = c1 * b0 + white * c2;
         b1 = c3 * b1 + white * c4;
         b2 = c5 * b2 + white * c6;
         return b0 + b1 + b2 + white * c7;
      }

      float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f;
   };
}

#endif
