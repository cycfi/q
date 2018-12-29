/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_PERIOD_DETECTOR_HPP_MARCH_12_2018)
#define CYCFI_Q_PERIOD_DETECTOR_HPP_MARCH_12_2018

#include <q/utility/bitstream.hpp>
#include <q/utility/zero_crossing.hpp>
#include <q/detail/count_bits.hpp>
#include <unordered_map>
#include <cmath>

namespace cycfi { namespace q
{
   ////////////////////////////////////////////////////////////////////////////
   class period_detector
   {
   public:

      struct info
      {
         float             _period;
         float             _periodicity;
      };
                           period_detector(
                              frequency lowest_freq
                            , frequency highest_freq
                            , std::uint32_t sps
                            , decibel threshold
                           );

                           period_detector(period_detector const& rhs) = default;
                           period_detector(period_detector&& rhs) = default;

      period_detector&     operator=(period_detector const& rhs) = default;
      period_detector&     operator=(period_detector&& rhs) = default;

      bool                 operator()(float s);
      bool                 operator()() const;

      std::size_t          num_periods() const;
      info const&          operator[](std::size_t index) const;

   private:

      using info_storage = std::array<info, 4>;

      zero_crossing        _zc;
      info_storage         _info;
      std::size_t          _num_periods;
      std::size_t const    _min_period;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   inline period_detector::period_detector(
      frequency lowest_freq
    , frequency highest_freq
    , std::uint32_t sps
    , decibel threshold
   )
    : _zc(threshold, float(lowest_freq.period() * 2) * sps)
    , _min_period(float(highest_freq.period()) * sps)
   {}

   struct auto_correlator
   {
      static constexpr auto value_size = bitstream<>::value_size;

      auto_correlator(std::size_t window, zero_crossing const& zc)
       : _bits(window)
       , _size(_bits.size())
       , _mid_array(((_size / value_size) / 2) - 1)
       , _zc(zc)
      {
         for (auto i = 0; i != _zc.num_edges(); ++i)
         {
            auto const& info = _zc[i];
            auto pos = std::max<int>(info._leading_edge, 0);
            auto n = info._trailing_edge - pos;
            _bits.set(pos, n, 1);
         }
      }

      std::size_t mid_point() const { return _size / 2; }

      inline std::size_t operator()(std::size_t pos)
      {
         auto const index = pos / value_size;
         auto const shift = pos % value_size;

         auto const* p1 = _bits.data();
         auto const* p2 = _bits.data() + index;
         auto count = 0;

         if (shift == 0)
         {
            for (auto i = 0; i != _mid_array; ++i)
               count += detail::count_bits(*p1++ ^ *p2++);
         }
         else
         {
            auto shift2 = value_size - shift;
            for (auto i = 0; i != _mid_array; ++i)
            {
               auto v = *p2++ >> shift;
               v |= *p2 << shift2;
               count += detail::count_bits(*p1++ ^ v);
            }
         }
         return count;
      };

      bitstream<>             _bits;
      std::size_t const       _size;
      std::size_t const       _mid_array;
      zero_crossing const&    _zc;
   };

   inline bool period_detector::operator()(float s)
   {
      auto r = _zc(s);
      if (_zc.is_ready())
      {
         CYCFI_ASSERT(_zc.num_edges() > 1, "Not enough edges.");

         auto_correlator ac{ _zc.window_size(), _zc };
         auto const mid_point = ac.mid_point();
         std::size_t min_count = int_traits<uint16_t>::max;;
         std::size_t found_pos = 0;

         for (auto i = 0; i != _zc.num_edges()-1; ++i)
         {
            auto const& first = _zc[i];
            for (auto j = i+1; j != _zc.num_edges(); ++j)
            {
               auto const& next = _zc[j];
               auto period = first.period(next);
               if (period > mid_point)
                  break;
               if (period >= _min_period)
               {
                  auto count = ac(period);
                  if (count < min_count)
                  {
                     min_count = count;
                     found_pos = period;
                  }
               }
            }
         }
      }
      return r;
   }

   inline bool period_detector::operator()() const
   {
      return _zc();
   }

   inline std::size_t period_detector::num_periods() const
   {
      return _num_periods;
   }

   inline period_detector::info const&
   period_detector::operator[](std::size_t index) const
   {
      return _info[index];
   }
}}

#endif

