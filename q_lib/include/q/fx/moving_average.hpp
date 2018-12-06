/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_EXP_MOVING_AVERAGE_DECEMBER_24_2015)
#define CYCFI_Q_EXP_MOVING_AVERAGE_DECEMBER_24_2015

#include <q/support.hpp>
#include <q/ring_buffer.hpp>

namespace cycfi { namespace q
{
   ////////////////////////////////////////////////////////////////////////////
   // The moving average is the most common filter in DSP, mainly because it
   // is the easiest digital filter to understand and use. In spite of its
   // simplicity, the moving average filter is optimal for a common task:
   // reducing random noise while retaining a sharp step response. This makes
   // it the premier filter for time domain encoded signals. However, the
   // moving average is the worst filter for frequency domain encoded
   // signals, with little ability to separate one band of frequencies from
   // another. (Description from The Scientist and Engineer's Guide to
   // Digital Signal Processing.)
   //
   // This filter is implemented using a ring_buffer. The data type, T, is a
   // template parameter, allowing both floating point as well as integer
   // computations. Integers are typically faster than floating point and are
   // not prone to round-off errors.
   //
   // Take note that the final result is not divided by the the moving
   // average length, N. Only the sum is returned, which gives the filter a
   // gain of N. The fixed gain, N, can be compensated elsewhere. This makes
   // the filter very fast, requiring only one addition and one subtraction
   // per sample.∑
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   struct moving_average
   {
      moving_average(std::size_t n)
       : _buff(n)
      {
         _buff.clear();
      }

      T operator()(T s)
      {
         _sum += s;              // Add the latest sample to the sum
         _sum -= _buff.back();   // Subtract the oldest sample from the sum
         _buff.push(s);          // Push the latest sample, erasing the oldest
         return _sum;            // Return the sum (gain == n)
      }

      T operator()() const
      {
         return _sum;
      }

      using buffer = ring_buffer<T>;
      using accumulator = decltype(promote(T()));

      buffer      _buff = buffer{};
      accumulator _sum = 0;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Exponential moving average approximates an arithmetic moving average by
   // multiplying the last result by some factor, and adding it to the next
   // sample multiplied by some other factor.
   //
   // If b = 2/(n+1), where n is the number of samples you would have used in
   // an arithmetic average, the exponential moving average will approximate
   // the arithmetic average pretty well.
   //
   //    n: the number of samples.
   //    y: current value
   //
   // See: https://www.dsprelated.com/showthread/comp.dsp/47981-1.php
   ////////////////////////////////////////////////////////////////////////////
   template <int n>
   struct exp_moving_average
   {
      static constexpr float b = 2.0f / (n + 1);
      static constexpr float b_ = 1.0f - b;

      exp_moving_average(float y_ = 0.0f)
       : y(y_)
      {}

      float operator()(float s)
      {
         return y = b * s + b_ * y;
      }

      float operator()() const
      {
         return y;
      }

      exp_moving_average& operator=(float y_)
      {
         y = y_;
         return *this;
      }

      float y = 0.0f;
   };
}}

#endif
