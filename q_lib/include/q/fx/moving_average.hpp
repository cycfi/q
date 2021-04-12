/*=============================================================================
   Copyright (c) 2014-2021 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_EXP_MOVING_AVERAGE_DECEMBER_7_2018)
#define CYCFI_Q_EXP_MOVING_AVERAGE_DECEMBER_7_2018

#include <q/support/base.hpp>
#include <q/utility/ring_buffer.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // moving_sum computes the moving sum of consecutive samples in a window
   // specified by max_size samples or duration d and std::size_t sps.
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   struct basic_moving_sum
   {
      basic_moving_sum(std::size_t max_size)
       : _buff(max_size)
       , _size(max_size)
       , _sum{ 0 }
      {
         _buff.clear();
      }

      basic_moving_sum(duration d, std::size_t sps)
       : basic_moving_sum(std::size_t(sps * float(d)))
      {}

      T operator()(T s)
      {
         _sum += s;              // Add the latest sample to the sum
         _sum -= _buff[_size-1]; // Subtract the oldest sample from the sum
         _buff.push(s);          // Push the latest sample, erasing the oldest
         return _sum;
      }

      T operator()() const
      {
         return _sum;            // Return the sum
      }

      T sum() const
      {
         return _sum;            // Return the sum
      }

      std::size_t size() const
      {
         return _size;
      }

      void clear()
      {
         _buff.clear();
         _sum = 0;
      }

      void fill(T val)
      {
         _buff.fill(val);
         _sum = val * _size;
      }

      void size(std::size_t size_)
      {
         return _size = std::min(size_, _buff.size());
      }

   private:

      using buffer = ring_buffer<T>;
      using accumulator = decltype(promote(T()));

      buffer      _buff = buffer{};
      std::size_t _size;
      accumulator _sum;
   };

   using moving_sum = basic_moving_sum<float>;

   ////////////////////////////////////////////////////////////////////////////
   // The moving_average is the simplest and most efficient FIR filter. It is
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
   // moving_average is based on the moving_sum. See above.
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   struct basic_moving_average : basic_moving_sum<T>
   {
      using basic_moving_sum<T>::basic_moving_sum;

      T operator()(T s)
      {
         basic_moving_sum<T>::operator()(s);
         return (*this)();
      }

      T operator()() const
      {
          // Return the average
         return this->sum() / this->size();
      }
   };

   using moving_average = basic_moving_average<float>;

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
   //    y: current value.
   //
   // See: https://www.dsprelated.com/showthread/comp.dsp/47981-1.php
   //
   // The exp_moving_average<n> template computes b at compile-time, where n
   // is supplied as a compile-time parameter.
   ////////////////////////////////////////////////////////////////////////////
   template <std::size_t n>
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

   ////////////////////////////////////////////////////////////////////////////
   // The rt_exp_moving_average class computes b at run time, where n is
   // supplied as a runtime parameter.
   ////////////////////////////////////////////////////////////////////////////
   struct rt_exp_moving_average
   {
      rt_exp_moving_average(float n, float y_ = 0.0f)
       : y(y_)
       , b(2.0f / (n + 1))
       , b_(1.0f - b)
      {}

      rt_exp_moving_average(duration d, std::size_t sps, float y_ = 0.0f)
       : rt_exp_moving_average(std::size_t(sps * float(d)), y_)
      {}

      void length(std::size_t n)
      {
         b = 2.0f / (n + 1);
      }

      float operator()(float s)
      {
         return y = b * s + b_ * y;
      }

      float operator()() const
      {
         return y;
      }

      rt_exp_moving_average& operator=(float y_)
      {
         y = y_;
         return *this;
      }

      void width(float n)
      {
         b = 2.0f / (n + 1);
      }

      float b, b_;
      float y = 0.0f;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Simple 2-point average filter
   ////////////////////////////////////////////////////////////////////////////
   struct moving_average2
   {
      moving_average2(float y_ = 0.0f)
       : y(y_)
      {}

      float operator()(float s)
      {
         return y = (s + y) / 2;
      }

      float operator()() const
      {
         return y;
      }

      moving_average2& operator=(float y_)
      {
         y = y_;
         return *this;
      }

      float y = 0.0f;
   };
}

#endif
