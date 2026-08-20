/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
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
   // Two input samples in, one output sample out. fast_downsample_3 has this
   // time-domain expression:
   //
   //    y(k) = (x(2k-1) + 2x(2k) + x(2k+1)) / 4
   //
   // The kernel is multiplierless: the weights are shifts.
   //
   // This class is templated on the native integer or floating point
   // sample type (e.g. uint16_t).
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   struct fast_downsample_3
   {
      constexpr T operator()(T s1, T s2)
      {
         auto out = x + (s1/2);
         x = s2/4;
         return out + x;
      }

      T x = 0.0f;
   };

   // fast_downsample was this class's original name, from before there was
   // more than one of them. Retained so existing code keeps compiling.
   template <typename T>
   using fast_downsample = fast_downsample_3<T>;

   ////////////////////////////////////////////////////////////////////////////
   // fast_downsample_5 is the wider sibling of fast_downsample_3: the same
   // repeated two-sample averaging carried one round further, giving the
   // binomial kernel { 1, 4, 6, 4, 1 } / 16. Two input samples in, one
   // output sample out, with this time-domain expression:
   //
   //    y(k) = (x(2k-3) + 4x(2k-2) + 6x(2k-1) + 4x(2k) + x(2k+1)) / 16
   //
   // Like fast_downsample_3, it is multiplierless: 4x is a shift and 6x is a
   // shift plus a shift. It costs one more sample of group delay (two input
   // samples instead of one) and buys a much steeper rolloff. Both kernels
   // are repeated two-sample averages, so the magnitude response is exactly
   //
   //    |H(w)| = cos^2(w/2)   for fast_downsample_3
   //    |H(w)| = cos^4(w/2)   for fast_downsample_5
   //
   // giving, at the input sampling rate fs:
   //
   //                         fast_downsample_3   fast_downsample_5
   //    at fs/4 (Nyquist/2)       -6.0 dB             -12.0 dB
   //    at 0.40 fs                -20.4 dB            -40.8 dB
   //    DC gain                     1.0                 1.0
   //
   // Binomial kernels are monotonic, so there is no passband ripple to
   // compound when several of these are cascaded for a larger decimation
   // factor. That is the main reason to prefer this over an equal-length
   // equiripple halfband, which is sharper on its own but fares worse in a
   // cascade.
   //
   // The three stored samples are the theoretical minimum: a 5-tap window at
   // stride 2 overlaps itself by 5-2=3.
   //
   // This class is templated on the native integer or floating point sample
   // type (e.g. uint16_t). Note that the weighted sum reaches 16x the input
   // magnitude before the divide, so an integral T that is not promoted to a
   // wider type by the usual arithmetic conversions (i.e. T at least as wide
   // as int) needs four bits of headroom.
   //
   // The divide is written as /16 rather than as a shift on purpose. For
   // unsigned and floating point T it costs nothing either way, and for
   // signed T it rounds toward zero, which is symmetric. An arithmetic shift
   // would floor instead, biasing negative samples downward by up to one LSB
   // and leaving a DC offset that compounds when these are cascaded.
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   struct fast_downsample_5
   {
      constexpr T operator()(T s1, T s2)
      {
         auto out = (x0 + 4*x1 + 6*x2 + 4*s1 + s2) / 16;
         x0 = x2; x1 = s1; x2 = s2;
         return out;
      }

      T x0 = T(0), x1 = T(0), x2 = T(0);
   };
}

#endif
