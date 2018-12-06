/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_EXP_LOW_PASS_DECEMBER_24_2015)
#define CYCFI_Q_EXP_LOW_PASS_DECEMBER_24_2015

#include <q/support.hpp>

namespace cycfi { namespace q
{
	using namespace literals;

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
