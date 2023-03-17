/*=============================================================================
   Copyright (c) 2014-2023 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_FAST_DOWNSAMPLE_HPP_DECEMBER_24_2015)
#define CYCFI_Q_FAST_DOWNSAMPLE_HPP_DECEMBER_24_2015

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // Fast Downsampling with antialiasing. A quick and simple method of
   // downsampling a signal by a factor of two with a useful amount of
   // antialiasing. Each source sample is convolved with { 0.25, 0.5, 0.25 }
   // before downsampling. (from http://www.musicdsp.org/)
   //
   // This class is templated on the native integer or floating point
   // sample type (e.g. uint16_t).
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   struct fast_downsample
   {
      constexpr T operator()(T s1, T s2)
      {
         auto out = x + (s1/2);
         x = s2/4;
         return out + x;
      }

      T x = 0.0f;
   };
}

#endif
