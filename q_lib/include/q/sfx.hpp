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

namespace cycfi { namespace q
{
	using namespace literals;

   ////////////////////////////////////////////////////////////////////////////
   // Fast Downsampling with antialiasing. A quick and simple method of
   // downsampling a signal by a factor of two with a useful amount of
   // antialiasing. Each source sample is convolved with { 0.25, 0.5, 0.25 }
   // before downsampling. (from http://www.musicdsp.org/)
   //
   // This class is templated on the native integer sample type
   // (e.g. uint16_t).
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   struct fast_downsample
   {
      T operator()(T s1, T s2)
      {
         auto out = x + (s1 >> 1);
         x = s2 >> 2;
         return out + x;
      }

      T x = 0.0f;
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
      zero_cross(float hysteresis, frequency max_freq, std::uint32_t sps)
       : zero_cross(hysteresis, max_freq.period(), sps)
      {}

      zero_cross(float hysteresis, period min_period, std::uint32_t sps)
       : _hysteresis(hysteresis), _min_samples(double(min_period) * sps)
      {}

      float operator()(float s)
      {
         if (_count++ < _min_samples)
            return _state;

         if (s > _hysteresis && !_state)
         {
            _state = 1;
            _count = 0;
         }
         else if (s < -_hysteresis && _state)
         {
            _state = 0;
            _count = 0;
         }
         return _state;
      }

      bool edge() const { return _count == 0; }

      float const       _hysteresis = 0.0f;
      std::size_t const _min_samples;
      bool              _state = 0;
      std::size_t       _count = 0;
   };

   ////////////////////////////////////////////////////////////////////////////
   struct onset
   {
      onset(
         float sensitivity
       , float threshold
       , period min_period
       , duration decay
       , std::uint32_t sps
      )
       : _sensitivity(sensitivity)
       , _threshold(threshold)
       , _min_samples(double(min_period) * sps)
       , _lp(frequency(decay), sps)
       , _env(decay, sps)
      {}

      bool operator()(float s)
      {
         ++_count;

         auto env = _env(std::abs(s));
         auto lp = _lp(env);

         if (lp < _threshold)
            return 0;

         if (env * _sensitivity > lp)
         {
            if (_count < _min_samples)
               return 0;
            _count = 0;
            return 1;
         }
         return 0;
      }

      float                   _sensitivity;
      float                   _threshold;
      std::size_t const       _min_samples;
      one_pole_lowpass        _lp;
      peak_envelope_follower  _env;
      std::size_t             _count = 0;
   };

/*
   ////////////////////////////////////////////////////////////////////////////
   struct onset
   {
      onset(
         float sensitivity
       , duration capture_time
       , period min_period
       , duration decay
       , std::uint32_t sps
      )
       : _sensitivity(sensitivity)
       , _capture_time(double(capture_time) * sps)
       , _min_samples(double(min_period) * sps)
       , _decay(std::exp(-2.0f / (sps * double(decay))))
      {}

      std::pair<float, std::size_t> operator()(float s)
      {
         ++_count;

         if (s > _y)
         {
            _ascent += s - _y;
            _y = s;
         }
         else
         {
            auto peak = _y;
            auto descent = _y - s;
            _y = s + _decay * (_y - s);
            if (_ascent > _sensitivity && _count > _min_samples)
            {
               if (_highest_peak == 0.0f)
               {
                  // Start of capture. Get the current peak into _highest_peak,
                  // and mark the allowed end of capture.
                  _capture_end = _count + _capture_time;
                  _highest_peak = peak;
                  _onset_time = _count;
               }
               else if (_count > _capture_end)
               {
                  // End of capture. Return the highest peak value (_highest_peak).
                  auto result = _highest_peak;

                  // Reset _highest_peak, _ascent, and _count.
                  _highest_peak = _ascent = 0.0f;
                  _count = 0;
                  return { result, _count - _onset_time };
               }
               else
               {
                  // Get the larger of _highest_peak and the current peak.
                  if (_highest_peak < peak)
                  {
                     _highest_peak = peak;
                     _onset_time = _count;
                  }
               }
            }
            else if (_ascent > 0.0f)
            {
               _ascent -= descent;
            }
         }
         return { 0.0f, 0 };
      }

      float envelope() const { return _y; }

      float _y = 0.0f,  _sensitivity;
      float             _decay;
      float             _ascent = 0.0f;
      float             _highest_peak = 0.0f;
      std::size_t const _capture_time;
      std::size_t const _min_samples;
      std::size_t       _count = 0;
      std::size_t       _capture_end = 0;
      std::size_t       _onset_time = 0;
   };
*/

   // ////////////////////////////////////////////////////////////////////////////
   // struct peak
   // {
   //    peak(float sens)
   //     : _sens(sens)
   //    {}

   //    bool operator()(float s)
   //    {
   //       auto r = ((_y1 - _y2) > _sens) && ((_y1 - s) > _sens);
   //       _y2 = _y1;
   //       _y1 = s;
   //       return r;
   //    }

   //    float _y1 = 0.0f;
   //    float _y2 = 0.0f;
   //    float const _sens;
   // };

   // ////////////////////////////////////////////////////////////////////////////
   // // peak generates pulses that coincide with the peaks of a waveform. This
   // // is accomplished by comparing the signal with the (slightly attenuated)
   // // envelope of the signal (env) using a schmitt_trigger.
   // //
   // //    droop: Envelope droop amount (attenuation)
   // //    hysteresis: schmitt_trigger hysteresis amount
   // //
   // // The result is a bool corresponding to the peaks.
   // ////////////////////////////////////////////////////////////////////////////
   // struct peak
   // {
   //    peak(float droop, float hysteresis)
   //     : _droop(droop), _cmp(hysteresis)
   //    {}

   //    bool operator()(float s, float env)
   //    {
   //       return _cmp(s, env * _droop);
   //    }

   //    float const       _droop;
   //    schmitt_trigger   _cmp;
   // };

   // ////////////////////////////////////////////////////////////////////////////
   // struct onset
   // {
   //    static constexpr auto droop = 0.9f;
   //    static constexpr auto hysteresis = 0.002f;

   //    onset(period min_period, std::uint32_t sps)
   //     : _min_samples(double(min_period) * sps)
   //    {}

   //    bool operator()(float s, float env)
   //    {
   //       if (_count++ < _min_samples)
   //          return _state;

   //       auto pk = _pk(s, env);
   //       if (!_state && pk)
   //       {
   //          // if (_current_peak < std::abs(s))
   //          // {
   //             _current_peak = s;
   //             _state = 1;
   //             _count = 0;
   //          // }
   //       }
   //       else if (_state && !pk)
   //       {
   //          _state = 0;
   //          _count = 0;
   //       }
   //       return _state;
   //    }

   //    float             peak_val() const { return _current_peak; }
   //    void              reset() { _current_peak = 0.0f; }

   //    peak              _pk { droop, hysteresis };
   //    std::size_t const _min_samples;
   //    bool              _state = 0;
   //    std::size_t       _count = 0;
   //    float             _current_peak = 0.0f;
   // };
}}

#endif
