/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(CYCFI_Q_BACF_HPP_MARCH_12_2018)
#define CYCFI_Q_BACF_HPP_MARCH_12_2018

#include <q/bitstream.hpp>
#include <q/detail/count_bits.hpp>
#include <q/buffer.hpp>
#include <cmath>

namespace cycfi { namespace q
{
   ////////////////////////////////////////////////////////////////////////////
   class edges
   {
   public:

      struct info
      {
         using crossing_data = std::pair<float, float>;

         void              update_peak(float s);

         crossing_data     _crossing;
         float             _peak;
         std::size_t       _index;
      };

                           edges(float threshold)
                            : _threshold(-threshold)
                           {}

      bool                 operator()(float s, std::size_t index);
      bool                 operator()() const;
      void                 reset();
      void                 shift(std::size_t n);
      std::size_t          size() const;

   private:

      using info_storage = buffer<info, std::array<info, 16>>;

      float                _prev = 0.0f;
      float const          _threshold;
      bool                 _state;
      info_storage         _info;
      std::size_t          _size = 0;
   };

   ////////////////////////////////////////////////////////////////////////////
   template <typename T = std::uint32_t>
   class bacf
   {
   public:

      using correlation_vector = std::vector<std::uint16_t>;
      static constexpr float noise_threshold = 0.001;

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
                               , float threshold
                              );

                              bacf(bacf const& rhs) = default;
                              bacf(bacf&& rhs) = default;

      bacf&                   operator=(bacf const& rhs) = default;
      bacf&                   operator=(bacf&& rhs) = default;

      bool                    operator()(float s);
      bool                    operator[](std::size_t i) const;

      std::size_t             size() const;
      info const&             result() const;
      bool                    is_start() const;
      std::size_t             position() const;
      std::size_t             minimum_period() const;
      void                    reset();

      edges const&            edges() const { return _edges; }

   private:

      static std::size_t      buff_size(frequency freq, std::uint32_t sps);

      bitstream<T>            _bits;
      std::size_t             _size;
      std::size_t             _count = 0;
      std::size_t             _min_period;
      info                    _info;
      q::edges                _edges;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   inline std::size_t bacf<T>::buff_size(
      frequency freq, std::uint32_t sps)
   {
      auto period = sps / double(freq);
      return std::ceil(period) * 2;
   }

   template <typename T>
   inline bacf<T>::bacf(
      frequency lowest_freq
    , frequency highest_freq
    , std::uint32_t sps
    , float threshold
   )
    : _bits(buff_size(lowest_freq, sps))
    , _min_period(std::floor(sps / double(highest_freq)))
    , _edges(threshold)
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
   inline bool bacf<T>::operator()(float s)
   {
      _bits.set(_count, _edges(s, _count));
      if (++_count == _size)
      {
         _info.max_count = 0;
         _info.min_count = int_traits<uint16_t>::max;
         _info.index = 0;

         // We need at least two rising edges. No need to autocorrelate
         // if we do not have enough edges!
         bool ready = _edges.size() > 1;

         // The new count will be half the size, so we can continue seamlessly
         _count = _size / 2;

         if (ready)
         {
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

            // Shift the edges by _count samples
            _edges.shift(_count);

            // Shift half of the contents:
            _bits.shift_half();
         }
         else
         {
            _edges.reset();
            _bits.clear();
         }

         return ready; // Return true if we're ready.
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
   void bacf<T>::reset()
   {
      _count = 0;
   }

   inline void edges::info::update_peak(float s)
   {
      _peak = std::max(s, _peak);
   }

   inline bool edges::operator()(float s, std::size_t index)
   {
      if (s > 0.0f)
      {
         if (!_state)
         {
            _info.push({ { _prev, s }, s, index });
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
      return _state;
   };

   inline bool edges::operator()() const
   {
      return _state;
   }

   inline void edges::reset()
   {
      _size = 0;
   }

   inline std::size_t edges::size() const
   {
      return _size;
   }

   inline void edges::shift(std::size_t n)
   {
      auto new_size = _size;
      for (auto i = 0; i != _size; ++i)
      {
         if (_info[i]._index < n)
            --new_size;
         else
            _info[i]._index -= n;
      }
      _size = new_size;
   }
}}

#endif

