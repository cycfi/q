/*=============================================================================
   Copyright (c) 2014-2020 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_PITCH_DETECTOR_HPP_MARCH_12_2018)
#define CYCFI_Q_PITCH_DETECTOR_HPP_MARCH_12_2018

#include <q/support/literals.hpp>
#include <q/pitch/period_detector.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   struct pitch_info
   {
      float frequency = 0.0;
      float periodicity = 0.0;
   };

   class pitch_detector
   {
   public:

      static constexpr float     onset_periodicity = 0.95f;
      static constexpr float     min_periodicity = 0.90f;
      static constexpr decibel   default_hysteresis = -40_dB;

                              pitch_detector(
                                 frequency lowest_freq
                               , frequency highest_freq
                               , std::uint32_t sps
                               , decibel hysteresis = default_hysteresis
                              );

      bool                    operator()(float s);
      pitch_info              get_current() const           { return _current; }
      float                   get_frequency() const         { return _current.frequency; }
      float                   get_periodicity() const       { return _current.periodicity; }
      bool                    is_note_shift() const         { return _frames_after_shift == 0; }
      std::size_t             frames_after_shift() const    { return _frames_after_shift; }

      bitset<> const&         bits() const                  { return _pd.bits(); }
      zero_crossing const&    edges() const                 { return _pd.edges(); }
      period_detector const&  get_period_detector() const   { return _pd; }
      float                   predict_frequency() const;
      bool                    indeterminate() const         { return _current.frequency == 0.0f; }

   private:

      float                   calculate_frequency() const;
      pitch_info              bias(pitch_info const& incoming, bool& shift) const;
      void                    bias(pitch_info const& incoming);

      period_detector         _pd;
      pitch_info              _current;
      std::uint32_t           _sps;
      std::size_t             _frames_after_shift = 0;
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
     : _pd{ lowest_freq, highest_freq, sps, hysteresis }
     , _sps{ sps }
   {}

   inline pitch_info pitch_detector::bias(pitch_info const& incoming, bool& shift) const
   {
      auto error = _current.frequency / 32; // approx 1/2 semitone
      auto diff = std::abs(_current.frequency - incoming.frequency);

      // Try fundamental
      if (diff < error)
         return incoming;

      // Try harmonics and sub-harmonics
      if (_frames_after_shift > 1)
      {
         if (_current.frequency > incoming.frequency)
         {
            if (int multiple = std::round(_current.frequency / incoming.frequency); multiple > 1)
            {
               auto f = incoming.frequency * multiple;
               if (std::abs(_current.frequency - f) < error)
                  return { f, incoming.periodicity };
            }
         }
         else
         {
            if (int multiple = std::round(incoming.frequency / _current.frequency); multiple > 1)
            {
               auto f = incoming.frequency / multiple;
               if (std::abs(_current.frequency - f) < error)
                  return { f, incoming.periodicity };
            }
         }
      }

      // Don't do anything if the latest autocorrelation is not periodic
      // enough. Note that we only do this check on frequency shifts (i.e. at
      // this point, we are looking at a potential frequency shift, after
      // passing through the code above, checking for fundamental and
      // harmonics and sub-harmonics matches).
      if (_pd.fundamental()._periodicity > min_periodicity)
      {
         // Now we have a frequency shift
         shift = true;
         return incoming;
      }
      return _current;
   }

   inline void pitch_detector::bias(pitch_info const& incoming)
   {
      ++_frames_after_shift;
      bool shift = false;
      auto result = bias(incoming, shift);

      // Don't do anything if incoming is not periodic enough
      // Note that we only do this check on frequency shifts
      if (shift)
      {
         auto p = _pd.fundamental()._periodicity;
         if (p >= onset_periodicity)
         {
            _frames_after_shift = 0;
            _current = result;
         }
         else if (p < min_periodicity)
         {
            // Reset current
            _current = pitch_info{};
         }
      }
      else
      {
         _current = result;
      }
   }

   inline bool pitch_detector::operator()(float s)
   {
      _pd(s);

      // reset
      if (_pd.is_reset())
         _current = pitch_info{};   // Reset current

      if (_pd.is_ready())
      {
         if (_pd.fundamental()._periodicity == -1)
         {
            _current = pitch_info{};   // Force reset
            return false;
         }

         auto p = _pd.fundamental()._periodicity;
         if (_current.frequency == 0.0f)
         {
            // Disregard if we are not periodic enough
            if (p >= onset_periodicity)
            {
               auto f = calculate_frequency();
               if (f > 0.0f)
               {
                  _current = { f, p };
                  _frames_after_shift = 0;
               }
            }
         }
         else
         {
            if (p < min_periodicity)
               _frames_after_shift = 0;
            auto f = calculate_frequency();
            if (f > 0.0f)
               bias({ f, p });
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

   inline float pitch_detector::predict_frequency() const
   {
      if (auto p = _pd.predict_period(); p != -1.0f)
         return _sps / p;
      return 0.0f;
   }
}

#endif

