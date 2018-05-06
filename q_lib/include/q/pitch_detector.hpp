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
   constexpr frequency max_frequency = 2000_Hz;
   constexpr std::size_t max_harmonics = 5;

   template <typename T = std::uint32_t>
   class pitch_detector
   {
   public:
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

      bool                 operator()(float s);
      float                frequency() const    { return _frequency; }
      float                periodicity() const;

      bacf<T> const&       bacf() const         { return _bacf; }

   private:

      std::size_t          harmonic() const;
      float                calculate_frequency() const;

      q::bacf<T>           _bacf;
      float                _frequency;
      std::uint32_t        _sps;
      std::size_t          _ticks = 0;
      float                _max_val = 0.0f;
      float                _min_samples;
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
     , _frequency(-1.0f)
     , _sps(sps)
     , _min_samples(float(sps / max_frequency))
   {}

   template <typename T>
   inline bool pitch_detector<T>::operator()(float s)
   {
      return _bacf(s,
         [this]()
         {
            // auto const& info = _bacf.result();
            // if (info.min_count > info.max_count * (1.0f - min_periodicity))
            //    _frequency = -1.0f;
            // else
               _frequency = calculate_frequency();
         }
      );
   }

   namespace detail
   {
      constexpr float max_deviation = 0.95;
      constexpr float min_deviation = 0.7;

      inline float compute_threshold(
         float delta, std::size_t max_count
       , std::size_t diff, float min_samples)
      {
         float deviation = linear_interpolate(
            min_deviation, max_deviation
          , 1.0f - (min_samples / delta)
         );
         return max_count - (deviation * diff);
      }

      template <std::size_t harmonic>
      struct find_harmonic
      {
         template <typename Correlation>
         static std::size_t
         call(
            Correlation const& corr, std::size_t index
          , std::size_t min_period, std::size_t max_count
          , std::size_t diff, float min_samples
         )
         {
            float delta = float(index) / harmonic;
            if (delta < min_period)
               return find_harmonic<harmonic-1>
                  ::call(corr, index, min_period, max_count, diff, min_samples);

            float until = index - delta;
            float threshold = compute_threshold(delta, max_count, diff, min_samples);
            for (auto i = delta; i < until; i += delta)
            {
               auto corr_i = std::min(corr[i], corr[i+1]);
               if (corr_i > threshold)
                  return find_harmonic<harmonic-1>
                     ::call(corr, index, min_period, max_count, diff, min_samples);
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
          , std::size_t min_period, std::size_t max_count
          , std::size_t diff, float min_samples
         )
         {
            auto delta = float(index) / 2;
            if (delta < min_period)
               return 1;

            float threshold = compute_threshold(delta, max_count, diff, min_samples);
            return std::min(corr[delta], corr[delta+1]) > threshold? 1 : 2;
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

      auto min_period = _bacf.minimum_period();
      auto found = detail::find_harmonic<max_harmonics>
         ::call(corr, index, min_period, info.max_count, diff, _min_samples);
      return found;
   }

   template <typename T>
   inline float pitch_detector<T>::calculate_frequency() const
   {
      auto h = harmonic();
      auto span = _bacf.get_span();
      if (!span.first|| !span.second)
         return -1.0f;

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
   float pitch_detector<T>::periodicity() const
   {
      if (_frequency == -1.0f)
         return 0.0f;
      auto const& info = _bacf.result();
      return 1.0 - (float(info.min_count) / info.max_count);
   }
}}

#endif

