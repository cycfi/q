/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_FAST_DOWNSAMPLE_HPP_DECEMBER_24_2015)
#define CYCFI_Q_FAST_DOWNSAMPLE_HPP_DECEMBER_24_2015

#include <array>
#include <cstddef>
#include <utility>

namespace cycfi::q
{
   namespace detail
   {
      constexpr std::size_t binomial(std::size_t n, std::size_t k)
      {
         std::size_t r = 1;
         for (std::size_t i = 0; i != k; ++i)
            r = r * (n - i) / (i + 1);
         return r;
      }
   }

   ////////////////////////////////////////////////////////////////////////////
   // Fast Downsampling with antialiasing. A quick and simple method of
   // downsampling a signal by a factor of two with a useful amount of
   // antialiasing. Each source sample is convolved with a short binomial
   // kernel before downsampling. (from http://www.musicdsp.org/)
   //
   // Two input samples in, one output sample out. The kernel is row N-1 of
   // Pascal's triangle over 2^(N-1), which makes every member of the family
   // multiplierless (the weights are shifts and adds), unity gain at DC, and
   // free of passband ripple. N is the tap count, so N-1 is the number of
   // rounds of two-sample averaging applied, and everything else follows
   // from it:
   //
   //    kernel         row N-1 of Pascal's triangle, over 2^(N-1)
   //    |H(w)|         cos^(N-1)(w/2)
   //    group delay    (N-1)/2 samples
   //    state          N-2 samples, the theoretical minimum, since an N-tap
   //                   window at stride 2 overlaps itself by N-2
   //
   // The four widths with named aliases:
   //
   //                        _2           _3            _4              _5
   //    kernel          {1,1}/2      {1,2,1}/4    {1,3,3,1}/8   {1,4,6,4,1}/16
   //    at fs/4          -3.0 dB      -6.0 dB       -9.0 dB        -12.0 dB
   //    at 0.40 fs      -10.2 dB     -20.4 dB      -30.6 dB        -40.8 dB
   //    group delay      0.5 sa        1 sa         1.5 sa           2 sa
   //    state              0            1             2               3
   //
   // fast_downsample_2 needs no state at all: its window is exactly the two
   // samples handed to it.
   //
   // Cascade them to decimate by a larger power of two. Because every kernel
   // is a repeated two-sample average, n cascaded stages collapse exactly to
   // a CIC decimator of rate R = 2^n and order N-1:
   //
   //    prod(k = 0..n-1) (1 + z^-2^k) = 1 + z^-1 + ... + z^-(2^n - 1)
   //
   // So four stages of fast_downsample_5 are exactly a CIC N=4 R=16, but
   // factored as short FIRs at successively lower rates rather than in
   // Hogenauer's integrator-comb form. That factorization costs somewhat
   // more adds and in exchange every stage has bounded state, so it needs
   // none of the integer-only modular arithmetic a CIC's unbounded
   // integrators depend on.
   //
   // Binomial kernels are monotonic, which is what cascading rewards: an
   // equiripple halfband is sharper on its own, but its passband ripple
   // compounds stage over stage.
   //
   // This class is templated on the native integer or floating point sample
   // type (e.g. uint16_t). For integral T, note that the weighted sum
   // reaches 2^(N-1) times the input magnitude before the divide, so a T
   // that the usual arithmetic conversions do not promote to something wider
   // (a T at least as wide as int) needs N-1 bits of headroom.
   //
   // The divide is written as a division rather than as a shift on purpose.
   // For unsigned and floating point T it costs nothing either way, and for
   // signed T it rounds toward zero, which is symmetric. An arithmetic shift
   // would floor instead, biasing negative samples downward by up to one LSB
   // and leaving a DC offset that compounds in a cascade.
   ////////////////////////////////////////////////////////////////////////////
   template <std::size_t N, typename T>
   struct basic_fast_downsample
   {
      static_assert(N >= 2, "A downsampling kernel needs at least two taps.");

      static constexpr std::size_t order = N - 1;   // rounds of 2-sample averaging

      constexpr T operator()(T s1, T s2)
      {
         auto out =
            dot(std::make_index_sequence<(N > 2)? N-3 : 0>{}, s1, s2)
            / (1 << order);

         if constexpr (N > 4)                       // the window strides by 2
            shift(std::make_index_sequence<N-4>{});
         if constexpr (N >= 4)
            x[N-4] = s1;
         if constexpr (N >= 3)
            x[N-3] = s2;

         return out;
      }

      std::array<T, N-2> x = {};    // filter state, oldest first

   private:

      // Output k is the causal convolution at input index 2k+1, so the
      // weight on x[i] is binomial(order, order-i).
      //
      // Two details here are load bearing, and both cost an instruction if
      // undone. The fold is seeded with x[0] rather than with T(0), because
      // under strict floating point 0.0 + x is not removable (it differs for
      // x = -0.0); seeding with the outermost tap also skips a multiply by
      // its unit weight. And the weighting is written inline rather than
      // factored into a helper, because a compiler may only contract a
      // multiply and an add into an FMA within one expression, and a call
      // boundary breaks that.
      template <std::size_t... I>
      constexpr auto dot(std::index_sequence<I...>, T s1, T s2) const
      {
         if constexpr (N == 2)
            return s1 + s2;
         else
            return (x[0] + ... + (T(detail::binomial(order, order-I-1)) * x[I+1]))
               + T(detail::binomial(order, 1)) * s1 + s2;
      }

      template <std::size_t... I>
      constexpr void shift(std::index_sequence<I...>)
      {
         ((x[I] = x[I+2]), ...);
      }
   };

   template <typename T>
   using fast_downsample_2 = basic_fast_downsample<2, T>;

   template <typename T>
   using fast_downsample_3 = basic_fast_downsample<3, T>;

   template <typename T>
   using fast_downsample_4 = basic_fast_downsample<4, T>;

   template <typename T>
   using fast_downsample_5 = basic_fast_downsample<5, T>;

   // fast_downsample was the three-tap class's original name, from before
   // there was more than one of them. Retained so existing code keeps
   // compiling.
   template <typename T>
   using fast_downsample = fast_downsample_3<T>;
}

#endif
