/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(CYCFI_Q_PITCH_DETECTOR_HPP_MARCH_12_2018)
#define CYCFI_Q_PITCH_DETECTOR_HPP_MARCH_12_2018

#include <q/bacf.hpp>
#include <q/buffer.hpp>
#include <array>
#include <utility>

namespace cycfi { namespace q
{
   ////////////////////////////////////////////////////////////////////////////
   class edges
   {
   public:

      struct info
      {
         void              update_peak(float s);

         using crossing_data = std::pair<float, float>;

         crossing_data     _crossing;
         float             _peak;
         std::size_t       _index;
      };

                           edges(float hysteresis)
                            : _threshold(-hysteresis)
                           {}

      bool                 operator()(float s);
      bool                 operator()() const;
      void                 reset();
      std::size_t          size() const;

   private:

      using info_storage = buffer<info, std::array<info, 16>>;

      float                _prev = 0.0f;
      float const          _threshold;
      bool                 _state;
      info_storage         _info;
      std::size_t          _index = 0;
      std::size_t          _size = 0;
   };

   ////////////////////////////////////////////////////////////////////////////
   template <typename T = std::uint32_t>
   class pitch_detector
   {
   public:

      static constexpr float max_deviation = 0.8;
      static constexpr std::size_t max_harmonics = 7;
      static constexpr float noise_threshold = 0.001;

                           pitch_detector(
                              frequency lowest_freq
                            , frequency highest_freq
                            , std::uint32_t sps
                           );

                           pitch_detector(pitch_detector const& rhs) = default;
                           pitch_detector(pitch_detector&& rhs) = default;

      pitch_detector&      operator=(pitch_detector const& rhs) = default;
      pitch_detector&      operator=(pitch_detector&& rhs) = default;

      bool                 operator()(float s);
      float                frequency() const    { return _frequency; }
      float                periodicity() const;

      bacf<T> const&       bacf() const         { return _bacf; }
      edges const&         edges() const        { return _edges; }

   private:

      std::size_t          harmonic() const;
      // float                   calculate_frequency(std::size_t num_edges) const;

      q::bacf<T>           _bacf;
      float                _frequency;
      std::uint32_t        _sps;
      q::edges             _edges{ noise_threshold };
      std::size_t          _ticks = 0;
      float                _max_val = 0.0f;
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
     , _frequency(-1.0f)
     , _sps(sps)
   {}

   template <typename T>
   inline bool pitch_detector<T>::operator()(float s)
   {
      auto b = _edges(s);
      bool proc = _bacf(b);
      if (proc)
      {
         _edges.reset();
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

/*
   template <typename T>
   inline std::pair<int, int> pitch_detector<T>::edges(std::size_t num_edges) const
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
      return { first_edge, next_edge };
   }

   template <typename T>
   inline float pitch_detector<T>::calculate_frequency(std::size_t num_edges) const
   {
      auto edge = edges(num_edges);
      if (edge.first == -1 || edge.second == -1)
         return -1.0f;

      // Get the start edge
      auto prev1 = _signal[edge.first - 1];
      auto curr1 = _signal[edge.first];
      auto dy1 = curr1 - prev1;
      auto dx1 = -prev1 / dy1;

      // Get the next edge
      auto prev2 = _signal[edge.second - 1];
      auto curr2 = _signal[edge.second];
      auto dy2 = curr2 - prev2;
      auto dx2 = -prev2 / dy2;

      // Calculate the frequency
      float n_samples = (edge.second - edge.first) + (dx2 - dx1);
      return (_sps * harmonic()) / n_samples;
   }
*/

   template <typename T>
   float pitch_detector<T>::periodicity() const
   {
      if (_frequency == -1.0f)
         return 0.0f;
      auto const& info = _bacf.result();
      return 1.0 - (float(info.min_count) / info.max_count);
   }

   inline void edges::info::update_peak(float s)
   {
      _peak = std::max(s, _peak);
   }

   inline bool edges::operator()(float s)
   {
      if (s > 0.0f)
      {
         if (!_state)
         {
            _info.push({ { _prev, s }, s, _index });
            ++_size;
            _state = 1;
         }
         else
         {
            _info[0].update_peak(s);
         }
      }
      else if (_state && s < _threshold)
      {
         _state = 0;
      }
      _prev = s;
      ++_index;
      return _state;
   };

   inline bool edges::operator()() const
   {
      return _state;
   }

   inline void edges::reset()
   {
      _index = 0;
      _size = 0;
   }

   inline std::size_t edges::size() const
   {
      return _size;
   }
}}

#endif

