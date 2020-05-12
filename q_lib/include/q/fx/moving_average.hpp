/*=============================================================================
   Copyright (c) 2014-2020 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_EXP_MOVING_AVERAGE_DECEMBER_7_2018)
#define CYCFI_Q_EXP_MOVING_AVERAGE_DECEMBER_7_2018

#include <q/support/base.hpp>
#include <q/utility/ring_buffer.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // The moving average is the simplest and most efficient FIR filter. It is
   // also the most common filter in DSP primarily due to its simplicity. But
   // while it is technically a low pass FIR filter, it performs poorly in
   // the frequency domain with very slow roll-off and dreadful stopband
   // attenuation. On the other hand, it performs admirably in the time
   // domain. The moving average filter is optimal in reducing random noise
   // while retaining a sharp step response.
   //
   // Averaging N samples (the moving average length) increases the SNR by
   // the square root of N. For example, N=16 improves SNR by 4 (12dB). The
   // filter delay is exactly (N−1)/2.
   //
   // This filter is implemented using a ring_buffer. The data type, T, is a
   // template parameter, allowing both floating point as well as integer
   // computations. Integers are typically faster than floating point and are
   // not prone to round-off errors.
   //
   // Take note that the final result is optionally not divided by the moving
   // average length, N if template parameter `div` is set to false. Only the
   // sum is returned, which gives the filter a gain of N. The fixed gain, N,
   // can be compensated elsewhere. This makes the filter very fast,
   // requiring only one addition and one subtraction per sample. `div`
   // defaults to true.
   ////////////////////////////////////////////////////////////////////////////
   template <typename T, bool div = true>
   struct moving_average
   {
      moving_average(std::size_t size)
       : _buff(size)
       , _size(size)
      {
         _buff.clear();
      }

      moving_average(duration d, std::size_t sps)
       : moving_average(std::size_t(sps * float(d)))
      {
      }

      T operator()(T s)
      {
         _sum += s;              // Add the latest sample to the sum
         _sum -= _buff[_size-1]; // Subtract the oldest sample from the sum
         _buff.push(s);          // Push the latest sample, erasing the oldest

         if constexpr (div)
            return _sum / _size; // Return the average
         else
            return _sum;         // Return the sum (gain == size)
      }

      T operator()() const
      {
         return _sum;
      }

      std::size_t size() const
      {
         return _size;
      }

      using buffer = ring_buffer<T>;
      using accumulator = decltype(promote(T()));

      buffer      _buff = buffer{};
      std::size_t _size;
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
}

#endif
