/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_PERIOD_DETECTOR_HPP_MARCH_12_2018)
#define CYCFI_Q_PERIOD_DETECTOR_HPP_MARCH_12_2018

#include <q/utility/bitstream.hpp>
#include <q/utility/zero_crossing.hpp>
#include <q/detail/count_bits.hpp>
#include <cmath>

namespace cycfi { namespace q
{
   ////////////////////////////////////////////////////////////////////////////
   template <typename T = natural_uint>
   class period_detector
   {
   public:

      using correlation_vector = std::vector<std::uint16_t>;
      static constexpr float pulse_threshold = 0.6;

                              period_detector(
                                 frequency lowest_freq
                               , frequency highest_freq
                               , std::uint32_t sps
                               , decibel threshold
                              );

                              period_detector(period_detector const& rhs) = default;
                              period_detector(period_detector&& rhs) = default;

      period_detector&        operator=(period_detector const& rhs) = default;
      period_detector&        operator=(period_detector&& rhs) = default;

                              template <typename F>
      bool                    operator()(float s, F f, std::size_t& extra);
      bool                    operator[](std::size_t i) const;

      std::size_t             size() const;
      info const&             result() const;
      bool                    is_start() const;
      bool                    is_half() const;
      std::size_t             position() const;
      std::size_t             minimum_period() const;
      void                    reset();

      edges const&            edges() const { return _edges; }
      edges::span             get_span(std::size_t index) const;
      edges::span             get_span() const;

   private:

      static std::size_t      buff_size(frequency freq, std::uint32_t sps);
      void                    set_bits(std::size_t from, std::size_t to);

      bitstream<T>            _bits;
      std::size_t const       _window_size;
      std::size_t             _frame = 0;
      std::size_t             _min_period;
      info                    _info;
      q::edges                _edges;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   inline std::size_t period_detector<T>::buff_size(
      frequency freq, std::uint32_t sps)
   {
      auto period = sps / double(freq);
      return std::ceil(period) * 2;
   }

   template <typename T>
   inline period_detector<T>::period_detector(
      frequency lowest_freq
    , frequency highest_freq
    , std::uint32_t sps
    , decibel threshold
   )
    : _bits(buff_size(lowest_freq, sps))
    , _window_size(_bits.size())
    , _min_period(std::floor(sps / double(highest_freq)))
    , _edges(threshold)
   {
      _info.correlation.resize(_window_size / 2, 0);
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
   void  period_detector<T>::set_bits(std::size_t from, std::size_t to)
   {
      // Get the highest peak
      float peak = 0;
      for (auto i = from; i != to; ++i)
      {
         if (_edges[i]._peak > peak)
            peak = _edges[i]._peak;
      }

      // Compute the threshold from the highest peak
      auto threshold = peak * pulse_threshold;

      // Set the bits
      for (auto i = from; i != to; ++i)
      {
         auto const& info = _edges[i];
         if (info._peak < threshold)
         {
            // inhibit weak pulses
            info._inhibited = true;
         }
         else
         {
            auto i = std::max<int>(info._leading_edge, 0);
            auto n = info._trailing_edge - i;
            _bits.set(i, n, 1);
         }
      }
   }

   template <typename T>
   template <typename F>
   inline bool period_detector<T>::operator()(float s, F f, std::size_t& extra)
   {
      if (_edges.is_full())
      {
         _edges.reset();
         _frame = 0;
         return false;
      }

      bool state = _edges(s, _frame);
      if (++_frame >= _window_size && !state)
      {
         _info.max_count = 0;
         _info.min_count = int_traits<uint16_t>::max;
         _info.index = 0;

         extra = _frame - _window_size;
         auto half = _window_size / 2;

         // Remove half the size from _frame, so we can continue seamlessly
         _frame -= half;

         // We need at least two rising edges. No need to autocorrelate
         // if we do not have enough edges!
         if (_edges.size() > 1)
         {
            // Set the bits
            _bits.clear();
            set_bits(0, _edges.size());

            // Autocorrelate
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

            if (_info.correlation[_min_period] != _info.min_count)
            {
               _info.periodicity = 1.0 - (float(_info.min_count) / _info.max_count);
               f(); // Call the user function before shifting
            }
            else
            {
               _info.periodicity = 0.0f;
            }

            // Shift the edges by half the number of samples
            _edges.shift(half);

            // We are ready
            return true;
         }
         else
         {
            _edges.reset();
            return false;
         }
      }
      return false; // We're not ready yet.
   }

   template <typename T>
   inline bool period_detector<T>::operator[](std::size_t i) const
   {
      return _bits.get(i);
   }

   template <typename T>
   inline std::size_t period_detector<T>::size() const
   {
      return _bits.size();
   }

   template <typename T>
   inline typename period_detector<T>::info const& period_detector<T>::result() const
   {
      return _info;
   }

   template <typename T>
   inline bool period_detector<T>::is_start() const
   {
      return _frame == 0;
   }

   template <typename T>
   inline bool period_detector<T>::is_half() const
   {
      return _frame == _window_size / 2;
   }

   template <typename T>
   inline std::size_t period_detector<T>::position() const
   {
      return _frame;
   }

   template <typename T>
   inline std::size_t period_detector<T>::minimum_period() const
   {
      return _min_period;
   }

   template <typename T>
   inline edges::span period_detector<T>::get_span(std::size_t index) const
   {
      return _edges.get_span(index);
   }

   template <typename T>
   inline edges::span period_detector<T>::get_span() const
   {
      return get_span(_info.index);
   }

   template <typename T>
   void period_detector<T>::reset()
   {
      _edges.reset();
      _frame = 0;
   }
}}

#endif

