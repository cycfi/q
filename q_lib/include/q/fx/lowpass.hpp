/*=============================================================================
   Copyright (c) 2014-2019 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_EXP_LOW_PASS_DECEMBER_24_2015)
#define CYCFI_Q_EXP_LOW_PASS_DECEMBER_24_2015

#include <q/support/base.hpp>
#include <q/support/literals.hpp>

namespace cycfi::q
{
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
   // See: https://www.w3.org/2011/audio/audio-eq-cookbook.html
   //
   // This is probably faster than the RBJ biquds (see biquad.hpp),
   // especially when computing the coefficients (e.g. when sweeping the
   // frequency) but this filter is rather limited, quirky and inacurate. Use
   // this as a quick and dirty 'musical' filter. E.g. when you don't care
   // about the actual frequency and Q, this one can be swept quickly using a
   // normalized values from 0.0 to less than 1.0, possibly using some custom
   // response curves (e.g. synth filters). Note that _f can't be == 1.0,
   // otherwise, the _fb computation will have a divide by zero. It is also
   // possible to supply the actual frequency, given the sps (samples per
   // second), but take note that the limit is around 7kHz given a sampling
   // rate of 44100, otherwise, the divide by zero.
   //
   // Unless you are in a pinch, I'd rather just use the RBJ biquds.
   ////////////////////////////////////////////////////////////////////////////
   struct reso_filter
   {
      reso_filter(frequency f, float reso, std::uint32_t sps)
       : _f(2.0f * fastsin(pi * float(f) / sps))
       , _fb(reso + reso / (1.0f - _f))
       , _reso(reso)
      {}

      reso_filter(float f, float reso)
       : _f(f)
       , _fb(reso + reso / (1.0f - _f))
       , _reso(reso)
      {}

      float operator()(float s)
      {
         _y0 += _f * (s - _y0 + _fb * (_y0 - _y1));
         _y1 += _f * (_y0 - _y1);
         return _y1;
      }

      float operator()() const
      {
         return _y1;
      }

      void cutoff(frequency f, std::uint32_t sps)
      {
         _f = 2.0f * fastsin(pi * float(f) / sps);
         _fb = _reso + _reso / (1.0f - _f);
      }

      void cutoff(float f)
      {
         _f = f;
         _fb = _reso + _reso / (1.0f - _f);
      }

      void resonance(float reso)
      {
         _reso = reso;
         _fb = reso + reso / (1.0f - _f);
      }

      float _f, _fb, _reso;
      float _y0 = 0, _y1 = 0;
   };
}

#endif
