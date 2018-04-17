/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(CYCFI_Q_BACF_HPP_MARCH_12_2018)
#define CYCFI_Q_BACF_HPP_MARCH_12_2018

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
}}

#endif

