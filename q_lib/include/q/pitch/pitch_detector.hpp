/*=============================================================================
   Copyright (c) 2014-2020 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_PITCH_DETECTOR_HPP_MARCH_12_2018)
#define CYCFI_Q_PITCH_DETECTOR_HPP_MARCH_12_2018

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
                               , std::uint32_t sps
                               , decibel hysteresis
                              );

                              pitch_detector(pitch_detector const& rhs) = default;
                              pitch_detector(pitch_detector&& rhs) = default;

      pitch_detector&         operator=(pitch_detector const& rhs) = default;
      pitch_detector&         operator=(pitch_detector&& rhs) = default;

      bool                    operator()(float s);
      float                   get_frequency() const         { return _frequency(); }
      float                   predict_frequency() const;
      bool                    is_note_onset() const;
      bool                    frames_after_onset() const    { return _frames_after_onset; }
      float                   periodicity() const;
      void                    reset()                       { _frequency = 0.0f; }

      bitset<> const&         bits() const                  { return _pd.bits(); }
      zero_crossing const&    edges() const                 { return _pd.edges(); }

   private:

      float                   calculate_frequency() const;
      float                   bias(float current, float incoming, bool& shift);
      void                    bias(float incoming);

      using exp_moving_average_type = exp_moving_average<4>;

      period_detector         _pd;
      exp_moving_average_type _frequency;
      median3                 _median;
      std::uint32_t           _sps;
      std::size_t             _frames_after_onset = 0;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   inline pitch_detector::pitch_detector(
       q::frequency lowest_freq
     , q::frequency highest_freq
     , std::uint32_t sps
     , decibel hysteresis
   )
     : _pd(lowest_freq, highest_freq, sps, hysteresis)
     , _frequency(0.0f)
     , _sps(sps)
   {}

   inline float pitch_detector::bias(float current, float incoming, bool& shift)
   {
      auto error = current / 32;   // approx 1/2 semitone
      auto diff = std::abs(current-incoming);

      // Try fundamental
      if (diff < error)
         return incoming;

      if (_frames_after_onset > 1)
      {
         if (current > incoming)
         {
            // is current the 5th harmonic of incoming?
            auto f = incoming * 5;
            if (std::abs(current-f) < error)
               return f;

            // is current the 4th harmonic of incoming?
            f = incoming * 4;
            if (std::abs(current-f) < error)
               return f;

            // is current the 3rd harmonic of incoming?
            f = incoming * 3;
            if (std::abs(current-f) < error)
               return f;

            // is current the 2nd harmonic of incoming?
            f = incoming * 2;
            if (std::abs(current-f) < error)
               return f;
         }
         else
         {
            // is incoming the 2nd harmonic of current?
            auto f = incoming * (1.0f / 2);  // Note: favor multiplication over division
            if (std::abs(current-f) < error)
               return f;

            // is incoming the 3rd harmonic of current?
            f = incoming * (1.0f / 3);       // Note: favor multiplication over division
            if (std::abs(current-f) < error)
               return f;
         }
      }

      // Don't do anything if the latest autocorrelation is not periodic
      // enough Note that we only do this check on frequency shifts (i.e. at
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
      auto current = _frequency();
      ++_frames_after_onset;
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
                  _frequency(_median(
                     (std::abs(current-f) < std::abs(current-f2))?
                     f : f2
                  ));
               }
            }
            else
            {
               _frequency(_median(f));
            }
         }
         else
         {
            // Now we have a frequency shift. Get the median of 3 (incoming
            // frequency and last two frequency shifts) to eliminate abrupt
            // changes. This will minimize potentially unwanted shifts.
            // See https://en.wikipedia.org/wiki/Median_filter
            _frequency = _median(incoming);
            if (_frequency() == incoming)
               _frames_after_onset = 0;
         }
      }
      else
      {
         _frequency(_median(f));
      }
   }

   inline bool pitch_detector::operator()(float s)
   {
      _pd(s);

      if (_pd.is_ready())
      {
         if (_frequency() == 0.0f)
         {
            // Disregard if we are not periodic enough
            if (_pd.fundamental()._periodicity >= max_deviation)
            {
               auto f = calculate_frequency();
               if (f > 0.0f)
               {
                  _median(f);       // Apply the median for the future
                  _frequency = f;   // But assign outright now
                  _frames_after_onset = 0;
               }
            }
         }
         else
         {
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

   inline bool pitch_detector::is_note_onset() const
   {
      return _frames_after_onset == 0;
   }

   inline float pitch_detector::predict_frequency() const
   {
      auto period = _pd.predict_period();
      if (period < _pd.minimum_period())
         return 0.0f;
      return _sps / period;
   }
}

#endif

