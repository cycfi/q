/*=============================================================================
   Copyright (c) 2014-2024 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_DIFFERENTIATOR_HPP_DECEMBER_24_2015)
#define CYCFI_Q_DIFFERENTIATOR_HPP_DECEMBER_24_2015

#include <q/fx/delay.hpp>
#include <q/utility/ring_buffer.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // The first-difference represents the signal change between two
   // consecutive samples of the input (s). The first-difference provides an
   // estimate of the signal slope at each point, which is useful for
   // detecting signal features such as edges and peaks.
   //
   // first_difference has this time-domain expression:
   //
   //    y(n) = x(n) - x(n-1)
   //
   ////////////////////////////////////////////////////////////////////////////
   struct first_difference
   {
      first_difference()
       : x(0.0f) {}

      float operator()(float s)
      {
         auto val = s - x;
         x = s;
         return val;
      }

      float x; // delayed input sample
   };

   ////////////////////////////////////////////////////////////////////////////
   // The central-difference is a more precise discrete approximation of the
   // signal's derivative than the first-difference. It is calculated by
   // subtracting two samples, one in the positive direction and one in the
   // negative direction, on either side of the evaluated point.
   //
   // central_difference has this time-domain expression:
   //
   //    y(n) = (x(n) - x(n-2)) / 2
   //
   // Unlike first-difference differentiator, central_difference has better
   // immunity to high-frequency noise.
   //
   // See https://www.dsprelated.com/showarticle/35.php
   ////////////////////////////////////////////////////////////////////////////
   struct central_difference
   {
      float operator()(float s)
      {
         return (s - _d(s)) / 2;
      }

      delay2 _d;
   };

   ////////////////////////////////////////////////////////////////////////////
   // `slope` tracks the steepness or incline of the signal over a period of
   // time given `dt` (delta time) and `sps` (samples per second). `slope` has
   // this time-domain expression:
   //
   //    y(n) = x(n) - x[n-m];
   //
   // where m is `sps * dt`.
   ////////////////////////////////////////////////////////////////////////////
   struct slope
   {
      slope(std::size_t max_size)
       : _buff{max_size}
       , _size(max_size)
      {}

      slope(duration dt, float sps)
       : slope(std::size_t(sps * as_float(dt)))
      {}

      float operator()(float s)
      {
         _buff.push(s);
         return s - _buff[_size-1];
      }

      float operator()() const
      {
         return _buff[0]-_buff[_size-1];
      }

   private:

      ring_buffer<float> _buff;
      std::size_t _size;
   };
}

#endif
