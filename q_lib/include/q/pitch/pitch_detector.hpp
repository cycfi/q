/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_PITCH_DETECTOR_HPP_MARCH_12_2018)
#define CYCFI_Q_PITCH_DETECTOR_HPP_MARCH_12_2018

#include <q/fx.hpp>
#include <q/pitch/bacf.hpp>
#include <array>
#include <utility>

namespace cycfi { namespace q
{
   ////////////////////////////////////////////////////////////////////////////
   template <typename T = std::uint32_t>
   class pitch_detector
   {
   public:

      static constexpr float        max_deviation = 0.94f;
      static constexpr std::size_t  max_harmonics = 5;
      static constexpr float        min_periodicity = 0.8f;
      static constexpr float        min_onset_periodicity = 0.6f;

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
      bool                 operator()(float s);

      bacf<T> const&       bacf() const            { return _bacf; }
      float                frequency() const       { return _frequency(); }
      float                predict_frequency() const;
      bool                 is_note_onset() const;
      float                periodicity() const;
      void                 reset()                 { _frequency = 0.0f; }

   private:

      std::size_t          harmonic() const;
      float                calculate_frequency() const;
      float                bias(float current, float incoming, bool& shift);
      void                 bias(float incoming);
      edges::span          get_span(std::size_t& harmonic) const;

      using exp_moving_average = exp_moving_average<4>;

      q::bacf<T>           _bacf;
      exp_moving_average   _frequency;
      median3              _median;
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
   inline float pitch_detector<T>::bias(float current, float incoming, bool& shift)
   {
      auto error = current / 32;   // approx 1/2 semitone
      auto diff = std::abs(current-incoming);

      // Try fundamental
      if (diff < error)
         return incoming;

      if (_frames_after_onset > 1)
      {
         // Try fifth below
         auto f = incoming * 3;
         if (std::abs(current-f) < error)
            return f;

         // Try octave below
         f = incoming * 2;
         if (std::abs(current-f) < error)
            return f;

         // Try octave above
         f = incoming * (1.0f / 2);       // Note: favor multiplication over division
         if (std::abs(current-f) < error)
            return f;

         // Try fifth above
         f = incoming * (1.0f / 3);       // Note: favor multiplication over division
         if (std::abs(current-f) < error)
            return f;
      }

      // Don't do anything if incoming is not periodic enough
      // Note that we only do this check on frequency shifts
      if (_bacf.result().periodicity > min_periodicity)
      {
         // Now we have a frequency shift
         shift = true;
         return incoming;
      }
      return current;
   }

   template <typename T>
   inline void pitch_detector<T>::bias(float incoming)
   {
      auto current = _frequency();
      ++_frames_after_onset;
      bool shift = false;
      auto f = bias(current, incoming, shift);

      // Don't do anything if incoming is not periodic enough
      // Note that we only do this check on frequency shifts
      if (shift)
      {
         if (_bacf.result().periodicity < max_deviation)
         {
            // If we don't have enough confidence in the bacf result,
            // we'll try the edges to extract the frequency and the
            // one closest to the current frequency wins.
            bool shift2 = false;
            float f2 = bias(current, predict_frequency(), shift2);

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
                  auto f = calculate_frequency();
                  if (f > 0.0f)
                  {
                     _frequency = _median(f);
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

            _prev_index = _bacf.result().index;
         }
       , extra
      );
   }

   template <typename T>
   inline bool pitch_detector<T>::operator()(float s)
   {
      std::size_t extra; // unused
      return (*this)(s, extra);
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
      if (span)
      {
         auto n_samples = get_span(h).period();
         return (_sps * h) / n_samples;
      }
      return -1.0f;
   }

   template <typename T>
   inline float pitch_detector<T>::periodicity() const
   {
      if (_frequency() == 0.0f)
         return 0.0f;
      return _bacf.result().periodicity;
   }

   template <typename T>
   inline bool pitch_detector<T>::is_note_onset() const
   {
      return _frames_after_onset == 0;
   }

   template <typename T>
   inline float pitch_detector<T>::predict_frequency() const
   {
      auto period = _bacf.edges().predict_period();
      if (period == 0.0f || period < _bacf.minimum_period())
         return 0.0f;
      return _sps / period;
   }
}}

#endif

