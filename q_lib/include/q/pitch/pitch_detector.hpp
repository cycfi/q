/*=============================================================================
   Copyright (c) 2014-2023 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_PITCH_DETECTOR_HPP_MARCH_12_2018)
#define CYCFI_Q_PITCH_DETECTOR_HPP_MARCH_12_2018

#include <q/support/literals.hpp>
#include <q/pitch/pitch_detector.hpp>
#include <q/fx/moving_average.hpp>
#include <q/fx/median.hpp>
#include <q/pitch/period_detector.hpp>
#include <array>
#include <utility>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   class pitch_detector
   {
   public:

      static constexpr float  max_deviation = 0.90f;
      static constexpr float  min_periodicity = 0.8f;

                              pitch_detector(
                                 frequency lowest_freq
                               , frequency highest_freq
                               , float sps
                               , decibel hysteresis
                              );

                              pitch_detector(pitch_detector const& rhs) = default;
                              pitch_detector(pitch_detector&& rhs) = default;

      bool                    operator()(float s);
      float                   get_frequency() const         { return _frequency; }
      float                   predict_frequency(bool init = false);
      bool                    is_note_shift() const;
      std::size_t             frames_after_shift() const    { return _frames_after_shift; }
      float                   periodicity() const;
      void                    reset()                       { _frequency = 0.0f; }

      bitset<> const&         bits() const                  { return _pd.bits(); }
      zero_crossing_collector const&    edges() const                 { return _pd.edges(); }
      period_detector const&  get_period_detector() const   { return _pd; }

   private:

      float                   calculate_frequency() const;
      float                   bias(float current, float incoming, bool& shift);
      void                    bias(float incoming);

      using exp_moving_average_type = exp_moving_average<2>;

      period_detector         _pd;
      float                   _frequency;
      median3                 _median;
      median3                 _predict_median;
      float                   _sps;
      std::size_t             _frames_after_shift = 0;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   inline pitch_detector::pitch_detector(
       q::frequency lowest_freq
     , q::frequency highest_freq
     , float sps
     , decibel hysteresis
   )
     : _pd{ lowest_freq, highest_freq, sps, hysteresis }
     , _frequency{ 0.0f }
     , _sps{ sps }
   {}

   inline float pitch_detector::bias(float current, float incoming, bool& shift)
   {
      auto error = current / 32; // approx 1/2 semitone
      auto diff = std::abs(current-incoming);

      // Try fundamental
      if (diff < error)
         return incoming;

      // Try harmonics and sub-harmonics
      if (_frames_after_shift > 2)
      {
         if (current > incoming)
         {
            if (int multiple = std::round(current / incoming); multiple > 1)
            {
               auto f = incoming * multiple;
               if (std::abs(current-f) < error)
                  return f;
            }
         }
         else
         {
            if (int multiple = std::round(incoming / current); multiple > 1)
            {
               auto f = incoming / multiple;
               if (std::abs(current-f) < error)
                  return f;
            }
         }
      }

      // Don't do anything if the latest autocorrelation is not periodic
      // enough. Note that we only do this check on frequency shifts (i.e. at
      // this point, we are looking at a potential frequency shift, after
      // passing through the code above, checking for fundamental and
      // harmonic matches).
      if (_pd.fundamental()._periodicity > min_periodicity)
      {
         // Now we have a frequency shift
         shift = true;
         return incoming;
      }
      return current;
   }

   inline void pitch_detector::bias(float incoming)
   {
      auto current = _frequency;
      ++_frames_after_shift;
      bool shift = false;
      auto f = bias(current, incoming, shift);

      // Don't do anything if incoming is not periodic enough
      // Note that we only do this check on frequency shifts
      if (shift)
      {
         if (_pd.fundamental()._periodicity < max_deviation)
         {
            // If we don't have enough confidence in the autocorrelation
            // result, we'll try the zero-crossing edges to extract the
            // frequency and the one closest to the current frequency wins.
            bool shift2 = false;
            auto predicted = predict_frequency();
            if (predicted > 0.0f)
            {
               float f2 = bias(current, predicted, shift2);

               // If there's no shift, the edges wins
               if (!shift2)
               {
                  _frequency = _median(f2);
               }
               else // else, whichever is closest to the current frequency wins.
               {
                  bool predicted = std::abs(current-f) >= std::abs(current-f2);
                  _frequency = _median(!predicted? f : f2);
               }
            }
            else
            {
               _frequency = _median(f);
            }
         }
         else
         {
            // Now we have a frequency shift. Get the median of 3 (incoming
            // frequency and last two frequency shifts) to eliminate abrupt
            // changes. This will minimize potentially unwanted shifts.
            // See https://en.wikipedia.org/wiki/Median_filter
            auto f = _median(incoming);
            if (f == incoming)
               _frames_after_shift = 0;
            _frequency = f;
         }
      }
      else
      {
         _frequency = _median(f);
      }
   }

   inline bool pitch_detector::operator()(float s)
   {
      _pd(s);

      if (_pd.is_ready())
      {
         if (_frequency == 0.0f)
         {
            // Disregard if we are not periodic enough
            if (_pd.fundamental()._periodicity >= max_deviation)
            {
               auto f = calculate_frequency();
               if (f > 0.0f)
               {
                  _median(f);       // Apply the median for the future
                  _frequency = f;   // But assign outright now
                  _frames_after_shift = 0;
               }
            }
         }
         else
         {
            if (_pd.fundamental()._periodicity < min_periodicity)
               _frames_after_shift = 0;
            auto f = calculate_frequency();
            if (f > 0.0f)
               bias(f);
         }
      }
      return _pd.is_ready();
   }

   inline float pitch_detector::calculate_frequency() const
   {
      if (_pd.fundamental()._period != -1)
         return _sps / _pd.fundamental()._period;
      return 0.0f;
   }

   inline float pitch_detector::periodicity() const
   {
      return _pd.fundamental()._periodicity;
   }

   inline bool pitch_detector::is_note_shift() const
   {
      return _frames_after_shift == 0;
   }

   inline float pitch_detector::predict_frequency(bool init)
   {
      auto period = _pd.predict_period();
      if (period < _pd.minimum_period())
         return 0.0f;
      auto f = _sps / period;
      if (_frequency != f)
      {
         f = _predict_median(f);
         if (init)
            _frequency = _median(f);
      }
      return f;
   }
}

#endif

