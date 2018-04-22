/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(CYCFI_Q_PITCH_DETECTOR_HPP_MARCH_12_2018)
#define CYCFI_Q_PITCH_DETECTOR_HPP_MARCH_12_2018

#include <q/bacf.hpp>
#include <array>

namespace cycfi { namespace q
{
   ////////////////////////////////////////////////////////////////////////////
   template <typename T = std::uint32_t>
   class pitch_detector
   {
   public:

      static constexpr float max_deviation = 0.8;
      static constexpr std::size_t max_harmonics = 7;
      static constexpr float noise_threshold = 0.005;

                              pitch_detector(
                                 frequency lowest_freq
                               , frequency highest_freq
                               , std::uint32_t sps
                              );

                              pitch_detector(pitch_detector const& rhs) = default;
                              pitch_detector(pitch_detector&& rhs) = default;

      pitch_detector&         operator=(pitch_detector const& rhs) = default;
      pitch_detector&         operator=(pitch_detector&& rhs) = default;

      bool                    operator()(float s);
      bacf<T> const&          bacf() const         { return _bacf; }
      float                   frequency() const    { return _frequency; }
      float                   periodicity() const;

   private:

      std::size_t             harmonic() const;
      float                   calculate_frequency(std::size_t num_edges) const;

      using signal_vector = std::vector<float>;
      using signal_iterator = typename signal_vector::iterator;
      using edges_array = std::array<std::uint32_t, 20>;

      q::bacf<T>              _bacf;
      signal_vector           _signal;
      edges_array             _edges;
      float                   _frequency;
      std::uint32_t           _sps;
      window_comparator       _cmp{ -0.2f, 0.0f };
      std::size_t             _ticks = 0;
      float                   _max_val = 0.0f;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   inline pitch_detector<T>::pitch_detector(
       q::frequency lowest_freq
     , q::frequency highest_freq
     , std::uint32_t sps
   )
     : _bacf(lowest_freq, highest_freq, sps)
     , _signal(_bacf.size(), 0.0f)
     , _frequency(-1.0f)
     , _sps(sps)
   {}

   namespace detail
   {
      struct normalize
      {
         normalize(float val)
          : _val(val)
         {}

         float operator()(float s) const
         {
            s *= _val;
            if (s < -1.0f)
               s = -1.0f;
            return s;
         }

         float _val;
      };
   }

   template <typename T>
   inline bool pitch_detector<T>::operator()(float s)
   {
      // Save signal
      _signal[_ticks++] = s;
      if (_max_val < s) // Get the maxima; Positive only!
         _max_val = s;

      bool proc = false;
      if (_ticks == _bacf.size())
      {
         if (_max_val > noise_threshold) // noise gate
         {
            auto norm = detail::normalize(1.0 / _max_val);
            auto first_low = false;
            auto num_edges = 0;
            auto half = _signal.size() / 2;

            // First half
            for (auto i = 0; i != half; ++i)
            {
               auto& s = _signal[i];
               bool prev_b = _cmp();

               s = norm(s);         // Normalization
               auto b = _cmp(s);    // Zero crossing
               proc = _bacf(b);     // Correlation

               if (!prev_b && b)    // Rising edge
                  if (i != 0 && num_edges != 20)
                     _edges[num_edges++] = i;
            }
            assert(!proc);

            // Second half (Note: we don't have to do the second
            // half if we do not have a rising edge in the first half).
            if (num_edges > 0 && _edges[0] < half)
            {
               for (auto i = half; i != _signal.size(); ++i)
               {
                  auto& s = _signal[i];
                  bool prev_b = _cmp();

                  s = norm(s);         // Normalization
                  auto b = _cmp(s);    // Zero crossing
                  proc = _bacf(b);     // Correlation

                  if (!prev_b && b)    // Rising edge
                     if (num_edges != 20)
                        _edges[num_edges++] = i;
               }
               assert(proc);

               // Compute Frequency
               auto f = calculate_frequency(num_edges);

               // If there's an abrupt frequency shift from the previous,
               // then we're most probably experiencing a harmonic shift!
               if (f != -1.0f && (_frequency == -1.0f || std::abs(f - _frequency) < std::abs(0.5 * _frequency)))
                  _frequency = f;
               else
                  proc = false; // No-go!
            }
         }

         _bacf.reset();    // Reset the BACF
         _ticks = 0;       // cycle the signal buffer
         _max_val = 0.0f;  // clear normalization max
      }
      return proc;
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
               return find_harmonic<harmonic-1>::call(
                     corr, index, min_period, threshold);

               for (auto i = delta; i < until; i += delta)
                  if (corr[i] > threshold)
                     return find_harmonic<harmonic-1>::call(
                           corr, index, min_period, threshold);
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
            return (delta < min_period || corr[delta] > threshold)? 1 : 2;
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
   inline float pitch_detector<T>::calculate_frequency(std::size_t num_edges) const
   {
      auto pos = _bacf.result().index;
      int first_edge = -1;
      int next_edge = -1;
      for (auto i = 0; i < num_edges; ++i)
         for (auto j = i + 1; j < num_edges; ++j)
            if (std::abs(int(pos) - int(_edges[j] - _edges[i])) < 2)
            {
               first_edge = _edges[i];
               next_edge = _edges[j];
            }

      if (first_edge == -1 || next_edge == -1)
         return -1.0f;

      // Get the start edge
      auto prev1 = _signal[first_edge - 1];
      auto curr1 = _signal[first_edge];
      auto dy1 = curr1 - prev1;
      auto dx1 = -prev1 / dy1;

      // Get the next edge
      auto prev2 = _signal[next_edge - 1];
      auto curr2 = _signal[next_edge];
      auto dy2 = curr2 - prev2;
      auto dx2 = -prev2 / dy2;

      // Calculate the frequency
      float n_samples = (next_edge - first_edge) + (dx2 - dx1);
      return (_sps * harmonic()) / n_samples;
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

