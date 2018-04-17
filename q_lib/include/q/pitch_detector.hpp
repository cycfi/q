/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(CYCFI_Q_PITCH_DETECTOR_HPP_MARCH_12_2018)
#define CYCFI_Q_PITCH_DETECTOR_HPP_MARCH_12_2018

#include <q/bitstream.hpp>
#include <q/detail/count_bits.hpp>
#include <cmath>

namespace cycfi { namespace q
{
   ////////////////////////////////////////////////////////////////////////////
   template <typename T = std::uint32_t>
   class bacf
   {
   public:

      using correlation_vector = std::vector<std::uint16_t>;

      struct info
      {
         correlation_vector   correlation;
         std::uint16_t        max_count;
         std::uint16_t        min_count;
         std::size_t          index;
      };

                              bacf(
                                 frequency lowest_freq
                               , frequency highest_freq
                               , std::uint32_t sps
                              );

                              bacf(bacf const& rhs) = default;
                              bacf(bacf&& rhs) = default;

      bacf&                   operator=(bacf const& rhs) = default;
      bacf&                   operator=(bacf&& rhs) = default;

      bool                    operator()(bool s);
      bool                    operator[](std::size_t i) const;

      std::size_t             size() const;
      info const&             result() const;
      bool                    is_start() const;
      std::size_t             position() const;
      std::size_t             minimum_period() const;

   private:

      static std::size_t      buff_size(frequency freq, std::uint32_t sps);

      bitstream<T>            _bits;
      std::size_t             _size;
      std::size_t             _count = 0;
      std::size_t             _min_period;
      info                    _info;
   };

   ////////////////////////////////////////////////////////////////////////////
   template <typename T = std::uint32_t>
   class pitch_detector
   {
   public:
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

   private:

      std::size_t             index() const;
      float                   calculate_frequency(std::size_t edge) const;

      using signal_iterator = typename std::vector<float>::iterator;

      q::bacf<T>              _bacf;
      std::vector<float>      _signal;
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
   inline std::size_t bacf<T>::buff_size(
      frequency freq, std::uint32_t sps)
   {
      auto period = sps / double(freq);
      return smallest_pow2<std::size_t>(std::ceil(period)) * 2;
   }

   template <typename T>
   inline bacf<T>::bacf(
      frequency lowest_freq
    , frequency highest_freq
    , std::uint32_t sps
   )
    : _bits(buff_size(lowest_freq, sps))
    , _min_period(std::floor(sps / double(highest_freq)))
   {
      _size = _bits.size();
      _info.correlation.resize(_size / 2, 0);
   }

   template <typename T, typename F>
   inline void auto_correlate(
      bitstream<T> const& bits, std::size_t start_pos, F f)
   {
      constexpr auto value_size = bitstream<T>::value_size;

      auto const size = bits.size();
      auto const array_size = size / value_size;
      auto const mid_pos = size / 2;
      auto const mid_array = (array_size / 2) - 1;

      auto index = start_pos / value_size;
      auto shift = start_pos % value_size;

      for (auto pos = start_pos; pos != mid_pos; ++pos)
      {
         auto* p1 = bits.data();
         auto* p2 = bits.data() + index;
         auto count = 0;

         if (shift == 0)
         {
            for (auto i = 0; i != mid_array; ++i)
               count += detail::count_bits(*p1++ ^ *p2++);
         }
         else
         {
            auto shift2 = value_size - shift;
            for (auto i = 0; i != mid_array; ++i)
            {
               auto v = *p2++ >> shift;
               v |= *p2 << shift2;
               count += detail::count_bits(*p1++ ^ v);
            }
         }
         ++shift;
         if (shift == value_size)
         {
            shift = 0;
            ++index;
         }

         f(pos, count);
      }
   }

   template <typename T>
   inline bool bacf<T>::operator()(bool s)
   {
      _bits.set(_count++, s);
      if (_count == _size)
      {
         _info.max_count = 0;
         _info.min_count = int_traits<uint16_t>::max;
         _info.index = 0;

         auto_correlate(_bits, _min_period,
            [&_info = this->_info](std::size_t pos, std::uint16_t count)
            {
               _info.correlation[pos] = count;
               _info.max_count = std::max(_info.max_count, count);
               if (count < _info.min_count)
               {
                  _info.min_count = count;
                  _info.index = pos;
               }
            }
         );

         _count = 0;
         return true; // We're ready!
      }
      return false; // We're not ready yet.
   }

   template <typename T>
   inline bool bacf<T>::operator[](std::size_t i) const
   {
      return _bits.get(i);
   }

   template <typename T>
   inline std::size_t bacf<T>::size() const
   {
      return _bits.size();
   }

   template <typename T>
   inline typename bacf<T>::info const& bacf<T>::result() const
   {
      return _info;
   }

   template <typename T>
   inline bool bacf<T>::is_start() const
   {
      return _count == 0;
   }

   template <typename T>
   inline std::size_t bacf<T>::position() const
   {
      return _count;
   }

   template <typename T>
   inline std::size_t bacf<T>::minimum_period() const
   {
      return _min_period;
   }

   template <typename T>
   inline pitch_detector<T>::pitch_detector(
       q::frequency lowest_freq
     , q::frequency highest_freq
     , std::uint32_t sps
   )
     : _bacf(lowest_freq, highest_freq, sps)
     , _signal(_bacf.size(), 0.0f)
     , _frequency(0.0f)
     , _sps(sps)
   {}

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
         if (_max_val > 0.005) // noise gate
         {
            auto norm = 1.0 / _max_val;
            auto first_low = false;
            auto edge = -1;

            for (auto i = _signal.begin(); i != _signal.end(); ++i)
            {
               // Normalization
               auto& s = *i;
               s *= norm;
               if (s < -1.0f)
                  s = -1.0f;

               // Bitstream-ize
               auto b = _cmp(s);

               // Get the first edge index
               if (!b && !first_low)
                  first_low = true;
               else if (b && first_low && edge == -1)
                  edge = i - _signal.begin();

               // Correlation
               proc = _bacf(b);

               // Compute Frequency
               if (proc)
               {
                  auto f = calculate_frequency(edge);
                  if (f != 0)
                     _frequency = f;
               }
            }
         }

         // cycle the signal buffer
         _ticks = 0;
         _max_val = 0.0f; // clear normalization max
      }
      return proc;
   }

   namespace detail
   {
      template <std::size_t harmonic>
      struct find_harmonics
      {
         template <typename Correlation>
         static std::size_t
         call(
            Correlation const& corr, std::size_t index
          , std::size_t min_period, float threshold
         )
         {
            auto delta = index / harmonic;
            if (delta < min_period)
               return index;

            for (auto i = delta; i < index; i += delta)
               if (corr[i] > threshold)
                  return find_harmonics<harmonic+1>::call(
                     corr, index, min_period, threshold);
            return delta;
         }
      };

      template <>
      struct find_harmonics<5>
      {
         template <typename Correlation>
         static std::size_t
         call(
            Correlation const& corr, std::size_t index
          , std::size_t min_period, float threshold
         )
         {
            return index;
         }
      };
   }

   template <typename T>
   inline std::size_t pitch_detector<T>::index() const
   {
      auto const& info = _bacf.result();
      if (info.max_count == info.min_count)
         return 0;
      auto const& corr = info.correlation;
      auto index = info.index;
      auto threshold = 0.15 * info.max_count;
      auto min_period = _bacf.minimum_period();
      auto found = detail::find_harmonics<2>::call(corr, index, min_period, threshold);
      if (corr[found] > threshold)
         return 0;
      return found;
   }

   template <typename T>
   float pitch_detector<T>::calculate_frequency(std::size_t edge) const
   {
      auto pos = index();
      if (pos == 0)
         return 0.0f;

      // Get the start edge
      auto prev1 = _signal[edge - 1];
      auto curr1 = _signal[edge];
      auto dy1 = curr1 - prev1;
      auto dx1 = -prev1 / dy1;

      // Get the next edge
      auto next = edge + pos - 1;
      while (_signal[next] > 0.0f)
         --next;
      auto prev2 = _signal[next++];
      auto curr2 = _signal[next];
      auto dy2 = curr2 - prev2;
      auto dx2 = -prev2 / dy2;

      // Calculate the frequency
      float n_samples = (next - edge) + (dx2 - dx1);
      return _sps / n_samples;
   }
}}

#endif

