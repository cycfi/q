/*=============================================================================
   Copyright (c) 2014-2019 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_DELAY_JULY_20_2014)
#define CYCFI_Q_DELAY_JULY_20_2014

#include <q/utility/fractional_ring_buffer.hpp>
#include <q/support/base.hpp>

namespace cycfi { namespace q
{
   ////////////////////////////////////////////////////////////////////////////
   // Basic one unit delay
   ////////////////////////////////////////////////////////////////////////////
   struct delay1
   {
      float operator()(float s)
      {
         auto r = y;
         y = s;
         return r;
      }

      float operator()() const
      {
         return y;
      }

      float y = 0.0f;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Basic two unit delay
   ////////////////////////////////////////////////////////////////////////////
   struct delay2
   {
      float operator()(float s)
      {
         return _d2(_d1(s));
      }

      delay1 _d1, _d2;
   };

   ////////////////////////////////////////////////////////////////////////////
   // delay: a basic class for fractional delays. The actual delay parameter
   // (in fractional samples) is decoupled from, and managed outside, the
   // class to allow both single and multi-tapped delays.
   ////////////////////////////////////////////////////////////////////////////
   class delay : public fractional_ring_buffer<float>
   {
   public:

      using base_type = fractional_ring_buffer<float>;

      delay(duration max_delay, std::uint32_t sps)
       : base_type(std::size_t(std::ceil(double(max_delay) * sps)))
      {}

      // Get the delayed signal (maximum delay).
      float operator()() const
      {
         return back();
      }

      // Get the delayed signal.
      float operator()(float samples_delay) const
      {
         return (*this)[samples_delay];
      }

      // Push a new signal and return the delayed signal. This is the
      // simplest (common) case for single delays. For multi-tapped delays,
      // you need to access the individual delays using the indexing operator
      // for various tap-points before pushing the latest sample.
      float operator()(float val, float samples_delay)
      {
         float delayed = (*this)[samples_delay];
         push(val);
         return delayed;
      }
   };
}}

#endif
