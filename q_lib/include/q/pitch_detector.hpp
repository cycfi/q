/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(CYCFI_Q_PITCH_DETECTOR_HPP_MARCH_12_2018)
#define CYCFI_Q_PITCH_DETECTOR_HPP_MARCH_12_2018

#include <q/bacf.hpp>
#include <array>
#include <utility>

namespace cycfi { namespace q
{
   ////////////////////////////////////////////////////////////////////////////
   template <typename T = std::uint32_t>
   class pitch_detector
   {
   public:

      static constexpr float max_deviation = 0.94f;
      static constexpr std::size_t max_harmonics = 5;
      static constexpr float min_periodicity = 0.8f;
      static constexpr float min_onset_periodicity = 0.6f;

                           pitch_detector(
                              frequency lowest_freq
                            , frequency highest_freq
                            , std::uint32_t sps
                            , float threshold
                           );

                           pitch_detector(pitch_detector const& rhs) = default;
                           pitch_detector(pitch_detector&& rhs) = default;

      pitch_detector&      operator=(pitch_detector const& rhs) = default;
      pitch_detector&      operator=(pitch_detector&& rhs) = default;

      bool                 operator()(float s, std::size_t& extra);

      bacf<T> const&       bacf() const         { return _bacf; }
      float                frequency() const    { return _frequency(); }
      float                periodicity() const;

   private:

      std::size_t          harmonic() const;
      float                calculate_frequency() const;
      void                 bias(float incoming);
      edges::span          get_span(std::size_t& harmonic) const;

      using exp_moving_average = exp_moving_average<32>;

      q::bacf<T>           _bacf;
      exp_moving_average   _frequency;
      int                  _prev_index = -1;
      std::uint32_t        _sps;
      std::size_t          _ticks = 0;
      float                _max_val = 0.0f;
      std::size_t          _frames_after_onset = 0;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   inline pitch_detector<T>::pitch_detector(
       q::frequency lowest_freq
     , q::frequency highest_freq
     , std::uint32_t sps
     , float threshold
   )
     : _bacf(lowest_freq, highest_freq, sps, threshold)
     , _frequency(0.0f)
     , _sps(sps)
   {}

   template <typename T>
   inline void pitch_detector<T>::bias(float incoming)
   {
      auto current = _frequency();
      auto error = current/32;   // approx 1/2 semitone
      ++_frames_after_onset;

      // Try fundamental
      if (std::abs(current-incoming) < error)
      {
         _frequency(incoming);
         return;
      }

      if (_frames_after_onset > 1)
      {
         // Try fifth below
         auto f = incoming*3;
         if (std::abs(current-f) < error)
         {
            _frequency(f);
            return;
         }

         // Try octave below
         f = incoming*2;
         if (std::abs(current-f) < error)
         {
            _frequency(f);
            return;
         }

         // Try octave above
         f = incoming/2;
         if (std::abs(current-f) < error)
         {
            _frequency(f);
            return;
         }

         // Try fifth above
         f = incoming/3;
         if (std::abs(current-f) < error)
         {
            _frequency(f);
            return;
         }
      }

      // Don't do anything if incoming is not periodic enough
      // Note that we only do this check on frequency shifts
      if (_bacf.result().periodicity > min_periodicity)
      {
         // Now we have a frequency shift
         _frequency = incoming;
         _frames_after_onset = 0;
      }
   }

   template <typename T>
   inline bool pitch_detector<T>::operator()(float s, std::size_t& extra)
   {
      return _bacf(
         s
       , [this]()
         {
            if (_frequency() == 0.0f)
            {
               // Disregard if we are not periodic enough
               if (_bacf.result().periodicity > min_onset_periodicity)
               {
                  _frequency = calculate_frequency();
                  _frames_after_onset = 0;
               }
            }
            else
            {
               auto f = calculate_frequency();
               bias(f);
            }

            _prev_index = _bacf.result().index;
         }
       , extra
      );
   }

   namespace detail
   {
      template <std::size_t harmonic>
      struct find_harmonic
      {
         template <typename Correlation>
         static std::size_t
         call(
            Correlation const& corr, std::size_t index
          , std::size_t min_period, float threshold
         )
         {
            float delta = float(index) / harmonic;
            float until = index - delta;
            if (delta < min_period)
               return find_harmonic<harmonic-1>
                  ::call(corr, index, min_period, threshold);

            for (auto i = delta; i < until; i += delta)
            {
               auto corr_i = std::min(corr[i], corr[i+1]);
               if (corr_i > threshold)
                  return find_harmonic<harmonic-1>
                     ::call(corr, index, min_period, threshold);
            }
            return harmonic;
         }
      };

      template <>
      struct find_harmonic<2>
      {
         template <typename Correlation>
         static std::size_t
         call(
            Correlation const& corr, std::size_t index
          , std::size_t min_period, float threshold
         )
         {
            auto delta = index / 2;
            if (delta < min_period)
               return 1;
            auto corr_i = std::min(corr[delta], corr[delta+1]);
            return (corr_i > threshold)? 1 : 2;
         }
      };
   }

   template <typename T>
   inline std::size_t pitch_detector<T>::harmonic() const
   {
      auto const& info = _bacf.result();
      auto const& corr = info.correlation;
      auto index = info.index;
      auto diff = info.max_count - info.min_count;
      auto threshold = info.max_count - (max_deviation * diff);

      auto min_period = _bacf.minimum_period();
      auto found = detail::find_harmonic<max_harmonics>
         ::call(corr, index, min_period, threshold);
      return found;
   }

   template <typename T>
   inline edges::span pitch_detector<T>::get_span(std::size_t& harmonic) const
   {
      auto span = _bacf.get_span();
      auto index = _bacf.result().index;
      if (!span)
      {
         // If the first attempt (using the incoming index)
         // fails, try the previous index
         if ((_prev_index != -1) && (_prev_index != index))
            span = _bacf.get_span(_prev_index);

         // If this still fails, try to get the harmonic
         if (!span && harmonic > 1)
         {
            span = _bacf.get_span(index / harmonic);
            if (span)
               harmonic = 1;
         }
      }
      return span;
   }

   template <typename T>
   inline float pitch_detector<T>::calculate_frequency() const
   {
      auto h = harmonic();
      auto span = get_span(h);
      if (!span)
         return 0.0f;

      // Get the start edge
      auto prev1 = span.first->_crossing.first;
      auto curr1 = span.first->_crossing.second;
      auto dy1 = curr1 - prev1;
      auto dx1 = -prev1 / dy1;

      // Get the next edge
      auto prev2 = span.second->_crossing.first;
      auto curr2 = span.second->_crossing.second;
      auto dy2 = curr2 - prev2;
      auto dx2 = -prev2 / dy2;

      // Calculate the frequency
      auto n_span = span.second->_leading_edge - span.first->_leading_edge;
      float n_samples = n_span + (dx2 - dx1);
      return (_sps * h) / n_samples;
   }

   template <typename T>
   inline float pitch_detector<T>::periodicity() const
   {
      if (_frequency() == 0.0f)
         return 0.0f;
      return _bacf.result().periodicity;
   }
}}

#endif

