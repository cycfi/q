/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_SFX_HPP_DECEMBER_24_2015)
#define CYCFI_Q_SFX_HPP_DECEMBER_24_2015

#include <cmath>
#include <algorithm>
#include <q/literals.hpp>
#include <q/support.hpp>
#include <q/fx.hpp>
#include <q/envelope.hpp>

namespace cycfi { namespace q
{
	using namespace literals;

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

   ////////////////////////////////////////////////////////////////////////////
   // dynamic_smoother based on Dynamic Smoothing Using Self Modulating Filter
   // by Andrew Simper, Cytomic, 2014, andy@cytomic.com
   //
   //    https://cytomic.com/files/dsp/DynamicSmoothing.pdf
   //
   // A robust and inexpensive dynamic smoothing algorithm based on using the
   // bandpass output of a 2 pole multimode filter to modulate its own cutoff
   // frequency. The bandpass signal is a meaure of how much the signal is
   // "changing" so is useful to increase the cutoff frequency dynamically
   // and allow for faster tracking when the input signal is changing more.
   // The absolute value of the bandpass signal is used since either a change
   // upwards or downwards should increase the cutoff.
   //
   ////////////////////////////////////////////////////////////////////////////
   struct dynamic_smoother
   {
      dynamic_smoother(frequency base, std::uint32_t sps)
       : dynamic_smoother(base, 0.5, sps)
      {}

      dynamic_smoother(frequency base, float sensitivity, std::uint32_t sps)
       : sense(sensitivity * 4.0f)  // efficient linear cutoff mapping
       , wc(double(base) / sps)
      {
         auto gc = std::tan(pi * wc);
         g0 = 2.0f * gc / (1.0f + gc);
      }

      float operator()(float s)
      {
         auto lowlz = low1;
         auto low2z = low2;
         auto bandz = lowlz - low2z;
         auto g = std::min(g0 + sense * std::abs(bandz), 1.0f);
         low1 = lowlz + g * (s - lowlz);
         low2 = low2z + g * (low1 - low2z);
         return low2z;
      }

      void base_frequency(frequency base, std::uint32_t sps)
      {
         wc = double(base) / sps;
         auto gc = std::tan(pi * wc);
         g0 = 2.0f * gc / (1.0f + gc);
      }

      float sense, wc, g0;
      float low1 = 0.0f;
      float low2 = 0.0f;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Dynamic one pole low-pass filter (6dB/Oct). Essentially the same as
   // one_pole_lowpass but with the coefficient, a, supplied dynamically.
   //
   //    _y: current value
   ////////////////////////////////////////////////////////////////////////////
   struct dynamic_lowpass
   {
      float operator()(float s, float a)
      {
         return _y += a * (s - _y);
      }

      float operator()() const
      {
         return _y;
      }

      dynamic_lowpass& operator=(float y_)
      {
         _y = y_;
         return *this;
      }

      float _y = 0.0f;
   };

   ////////////////////////////////////////////////////////////////////////////
   // zero_cross generates pulses that coincide with the zero crossings
   // of the signal. To minimize noise, 1) we apply some amount of hysteresis
   // and 2) constrain the time between transitions to a minumum given
   // min_period (or max_freq).
   ////////////////////////////////////////////////////////////////////////////
   struct zero_cross
   {
      zero_cross(float hysteresis)
       : _cmp(hysteresis)
      {}

      float operator()(float s)
      {
         return _cmp(s, 0);
      }

      schmitt_trigger   _cmp;
      bool              _state = 0;
   };

   ////////////////////////////////////////////////////////////////////////////
   // onset_detector is a feature based onset detector. A peak envelope
   // follower follows the signal's envelope. The peak envelope is low-pass
   // filtered using the same rate as the follower's decay (e.g. 100_ms).
   // With this setup, the low-pass filter is able to follow the envelope
   // except the attacks (the peaks). A schmitt_trigger is then used to
   // compare the filtered output and the original peak envelope, attenuated
   // by a certain amount (the sensitivity). The schmitt_trigger triggers
   // when the attenuated peak envelope exceeds the filtered result. This
   // coincides with the attack transients. The sensitivity determines how
   // much deviation we want to detect that constitutes an attack.
   //
   // The result is non-zero when an attack is detected. We return the peak
   // value (maximum) on attack detection, otherwise zero. Take note that the
   // attack may span multiple consecutive samples.
   ////////////////////////////////////////////////////////////////////////////
   struct onset_detector
   {
      onset_detector(float sensitivity, duration decay, std::uint32_t sps)
       : _sensitivity(sensitivity)
       , _lp(frequency(decay), sps)
       , _comp(double(-36_dB))
       , _env(decay, sps)
      {}

      float operator()(float abs_s)
      {
         auto env = _env(abs_s);
         auto lp = _lp(env);

         if (_comp(env * _sensitivity, lp))
            return _val = std::max(_val, abs_s);

         _val = 0.0f;
         return 0.0f;
      }

      float operator()() const
      {
         return _val;
      }

      peak_envelope_follower  _env;
      float                   _sensitivity;
      one_pole_lowpass        _lp;
      schmitt_trigger         _comp;
      float                   _val = 0.0f;
   };

   ////////////////////////////////////////////////////////////////////////////
   // peak generates pulses that coincide with the peaks of a waveform. This
   // is accomplished by comparing the signal with the (slightly attenuated)
   // envelope of the signal (env) using a schmitt_trigger.
   //
   //    sensitivity: Envelope droop amount (attenuation)
   //    hysteresis: schmitt_trigger hysteresis amount
   //
   // The result is a bool corresponding to the peaks.
   ////////////////////////////////////////////////////////////////////////////
   struct peak
   {
      peak(float sensitivity, float hysteresis)
       : _sensitivity(sensitivity), _cmp(hysteresis)
      {}

      bool operator()(float s, float env)
      {
         return _cmp(s, env * _sensitivity);
      }

      float const       _sensitivity;
      schmitt_trigger   _cmp;
   };

   ////////////////////////////////////////////////////////////////////////////
   ////////////////////////////////////////////////////////////////////////////
   struct hilbert_quadrature
   {
      std::pair<float, float> operator()(float s)
      {
         return {
            _dly(_d(_c(_b(_a(s)))))
          , _z(_y(_x(_w(s))))
         };
      }

      polyphase_allpass _a{ 0.47940086558884 };
      polyphase_allpass _b{ 0.87621849353931 };
      polyphase_allpass _c{ 0.976597589508199 };
      polyphase_allpass _d{ 0.997499255935549 };

      polyphase_allpass _w{ 0.161758498367701 };
      polyphase_allpass _x{ 0.733028932341491 };
      polyphase_allpass _y{ 0.945349700329113 };
      polyphase_allpass _z{ 0.990599156684529 };

      delay1            _dly;
   };
}}

#endif
