/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_HANN_GEN_HPP_APRIL_26_2023)
#define CYCFI_Q_HANN_GEN_HPP_APRIL_26_2023

#include <q/synth/gen/sin_cos_gen.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // Hann window taper generator.
   //
   // The Hann window, also known as the Cosine Bell, is named after the
   // Austrian meteorologist Julius von Hann. The Hann window is a taper
   // formed with a cosine raised above zero. It is used as one of many
   // windowing functions for smoothing values. The The Hann window taper
   // belongs to both the cosine-sum and sine-power families. Unlike the
   // Hamming window, the Hann window's end points touch zero.
   //
   // The formula for the Hann window is:
   //    w(n) = 0.5*(1 - cos(2pin/(N-1)))
   //
   // T is the type of the basic_sin_cos_gen that makes the cosine.
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   struct basic_hann_gen
   {
      basic_hann_gen(duration width, float sps)
       : cos{frequency(1.0f / as_float(width)), sps}
      {
      }

      float operator()()
      {
         return 0.5f * (1.0f - cos().second);
      }

      void config(duration width, float sps, bool reset_ = false)
      {
         cos.config(frequency(1.0f / as_float(width)), sps);
         if (reset_)
            reset();
      }

      void reset()
      {
         cos.reset();
      }

      void midpoint()
      {
         cos.midpoint();
      }

   private:

      basic_sin_cos_gen<T> cos;
   };

   using hann_gen = basic_hann_gen<double>;

   ////////////////////////////////////////////////////////////////////////////
   // The Hann upward ramp generator generates a rising curve with the shape
   // of the first half of a Hann window taper.
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   struct basic_hann_upward_ramp_gen : basic_hann_gen<T>
   {
      using base_type = basic_hann_gen<T>;

      basic_hann_upward_ramp_gen(duration width, float sps)
       : base_type{width*2, sps}
      {
      }

      void config(duration width, float sps, bool reset_ = false)
      {
         base_type::config(width*2, sps);
         if (reset_)
            base_type::reset();
      }
   };

   using hann_upward_ramp_gen = basic_hann_upward_ramp_gen<double>;

   ////////////////////////////////////////////////////////////////////////////
   // The Hann downward ramp generator generates a falling curve with the
   // shape of the second half of a Hann window taper.
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   struct basic_hann_downward_ramp_gen : basic_hann_gen<T>
   {
      using base_type = basic_hann_gen<T>;

      basic_hann_downward_ramp_gen(duration width, float sps)
       : base_type{width*2, sps}
      {
         base_type::midpoint();
      }

      void reset()
      {
         base_type::midpoint();
      }

      void config(duration width, float sps, bool reset_ = false)
      {
         base_type::config(width*2, sps);
         if (reset_)
            reset();    // this reset: the window midpoint
      }
   };

   using hann_downward_ramp_gen = basic_hann_downward_ramp_gen<double>;
}

#endif
