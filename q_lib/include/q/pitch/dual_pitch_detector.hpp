/*=============================================================================
   Copyright (c) 2014-2021 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_DUAL_PITCH_DETECTOR_HPP_AUGUST_9_2020)
#define CYCFI_Q_DUAL_PITCH_DETECTOR_HPP_AUGUST_9_2020

#include <q/pitch/pitch_detector.hpp>
#include <q/fx/moving_average.hpp>

namespace cycfi::q
{
   class dual_pitch_detector
   {
   public:
                              dual_pitch_detector(
                                 frequency lowest_freq
                               , frequency highest_freq
                               , std::uint32_t sps
                               , decibel hysteresis = pitch_detector::default_hysteresis
                              );

      bool                    operator()(float s);
      pitch_info              get_current() const           { return _current; }
      float                   get_frequency() const         { return _current.frequency; }
      float                   get_periodicity() const       { return _current.periodicity; }
      float                   predict_frequency() const;

   private:

      bool                    within_octave(float f) const;
      void                    compute_predicted_frequency() const;

      using mean_filter = exp_moving_average<8>;

      pitch_detector          _pd1;
      pitch_detector          _pd2;
      pitch_info              _current;
      mean_filter             _mean;
      mutable float           _predicted_frequency = 0.0f;
      bool                    _first = true;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   inline dual_pitch_detector::dual_pitch_detector(
       q::frequency lowest_freq
     , q::frequency highest_freq
     , std::uint32_t sps
     , decibel hysteresis
   )
     : _pd1{ lowest_freq, highest_freq, sps, hysteresis }
     , _pd2{ lowest_freq, highest_freq, sps, hysteresis }
     , _mean{ float(lowest_freq+((highest_freq-lowest_freq)/2)) }
   {
   }

   inline bool dual_pitch_detector::within_octave(float f) const
   {
      auto mean = _mean();
      return (f > mean)? f < (mean * 2) : f > (mean / 2);
   }

   inline bool dual_pitch_detector::operator()(float s)
   {
      bool pd1_ready = _pd1(s);
      bool pd2_ready = _pd2(-s);

      if (pd1_ready || pd2_ready)
      {
         if (!_pd1.indeterminate() && !_pd2.indeterminate())
         {
            auto _i1 = _pd1.get_current();
            auto _i2 = _pd2.get_current();
            auto mean = _mean();

            auto pd1_diff = std::abs(_i1.frequency-mean);
            auto pd2_diff = std::abs(_i2.frequency-mean);
            auto const& i = (pd1_diff < pd2_diff)? _i1 : _i2;

            if (_first)
            {
               _current = i;
               _mean = _current.frequency;
               _first = false;
               _predicted_frequency = 0.0f;
            }
            else if (within_octave(i.frequency))
            {
               _current = i;
               _mean(_current.frequency);
               _predicted_frequency = 0.0f;
            }
         }

         if (_pd1.indeterminate() && _pd2.indeterminate())
         {
            compute_predicted_frequency();
            _current = pitch_info{};   // Reset
         }
      }

      return pd1_ready || pd2_ready;
   }

   inline float dual_pitch_detector::predict_frequency() const
   {
      if (_predicted_frequency == 0.0f)
         compute_predicted_frequency();
      return _predicted_frequency;
   }

   inline void dual_pitch_detector::compute_predicted_frequency() const
   {
      auto f1 = _pd1.predict_frequency();
      if (f1 > 0.0)
      {
         auto f2 = _pd2.predict_frequency();
         if (f2 > 0.0f)
         {
            auto error = f1 / 10; // $$$ approx 1/2 semitone
            if (std::abs(f1-f2) < error)
            {
               if (_first || within_octave(f1))
               {
                  _predicted_frequency = f1;
                  return;
               }
            }
         }
      }
      _predicted_frequency = 0.0f;
   }
}

#endif

