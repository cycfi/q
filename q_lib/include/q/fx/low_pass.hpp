/*=============================================================================
   Copyright (c) 2014-2019 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_EXP_LOW_PASS_DECEMBER_24_2015)
#define CYCFI_Q_EXP_LOW_PASS_DECEMBER_24_2015

#include <q/support/base.hpp>
#include <q/support/literals.hpp>

namespace cycfi { namespace q
{
	using namespace literals;

   ////////////////////////////////////////////////////////////////////////////
   // fixed_pt_leaky_integrator: If you want a fast filter for integers, use
   // a fixed point leaky-integrator. k will determine the effect of the
   // filter. Choose k to be a power of 2 for efficiency (the compiler will
   // optimize the computation using shifts). k = 16 is a good starting
   // point.
   //
   // This simulates the RC filter in digital form. The equation is:
   //
   //    y[i] = rho * y[i-1] + s
   //
   // where rho < 1. To avoid floating point, we use k instead, which
   // allows for integer operations. In terms of k, rho = 1 - (1 / k).
   // So the actual formula is:
   //
   //    y[i] += s - (y[i-1] / k);
   //
   // k will also be the filter gain, so the final result should be divided
   // by k. If you need to initialize the filter (y member) to a certain
   // state, you will also need to multiply the initial value by k.
   //
   ////////////////////////////////////////////////////////////////////////////
   template <int k, typename T = int>
   struct fixed_pt_leaky_integrator
   {
      typedef T result_type;

      T operator()(T s)
      {
         y += s - (y / k);
         return y;
      }

      T operator()() const
      {
         return y;
      }

      fixed_pt_leaky_integrator& operator=(float y_)
      {
         y = y_;
         return *this;
      }

      T y = 0;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Leaky Integrator
   ////////////////////////////////////////////////////////////////////////////
   struct leaky_integrator
   {
      leaky_integrator(float a = 0.995)
       : a(a)
      {}

      leaky_integrator(frequency f, std::uint32_t sps)
       : a(1.0f -(2_pi * double(f) / sps))
      {}

      float operator()(float s)
      {
         return y = s + a * (y - s);
      }

      float operator()() const
      {
         return y;
      }

      leaky_integrator& operator=(float y_)
      {
         y = y_;
         return *this;
      }

      void cutoff(frequency f, std::uint32_t sps)
      {
         a = 1.0f -(2_pi * double(f) / sps);
      }

      float y = 0.0f, a;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Basic one pole low-pass filter (6dB/Oct)
   ////////////////////////////////////////////////////////////////////////////
   struct one_pole_lowpass
   {
      one_pole_lowpass(float a)
       : a(a)
      {}

      one_pole_lowpass(frequency freq, std::uint32_t sps)
       : a(1.0 - fast_exp3(-2_pi * double(freq) / sps))
      {}

      float operator()(float s)
      {
         return y += a * (s - y);
      }

      float operator()() const
      {
         return y;
      }

      one_pole_lowpass& operator=(float y_)
      {
         y = y_;
         return *this;
      }

      void cutoff(frequency freq, std::uint32_t sps)
      {
         a = 1.0 - fast_exp3(-2_pi * double(freq) / sps);
      }

      float y = 0.0f, a;
   };

   ////////////////////////////////////////////////////////////////////////////
   // This filter consists of two first order low-pass filters in series,
   // with some of the difference between the two filter outputs fed back to
   // give a resonant peak.
   //
   // See: http://www.musicdsp.org/showone.php?id=29
   ////////////////////////////////////////////////////////////////////////////
   struct reso_filter
   {
      reso_filter(float freq, float reso)
       : _freq(freq)
       , _fb(reso + reso / (1.0f - _freq))
       , _reso(reso)
      {}

      float operator()(float s)
      {
         _y0 += _freq * (s - _y0 + _fb * (_y0 - _y1));
         _y1 += _freq * (_y0 - _y1);
         return _y1;
      }

      float operator()() const
      {
         return _y1;
      }

      void cutoff(float freq)
      {
         _freq = freq;
         _fb = _reso + _reso / (1.0f - _freq);
      }

      void resonance(float reso)
      {
         _reso = reso;
         _fb = reso + reso / (1.0f - _freq);
      }

      float _freq, _fb, _reso;
      float _y0 = 0, _y1 = 0;
   };
}}

#endif
