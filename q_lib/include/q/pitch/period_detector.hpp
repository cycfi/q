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

      static constexpr float pulse_threshold = 0.6;

      struct info
      {
         float             _period;
         float             _periodicity;
      };

      using info_storage = std::array<info, 4>;

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

      void                 set_bitstream();
      void                 autocorrelate();

      zero_crossing        _zc;
      info_storage         _info;
      std::size_t          _num_periods = 0;
      std::size_t const    _min_period;
      bitstream<>          _bits;
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
    , _bits(_zc.window_size())
   {}

   inline void period_detector::set_bitstream()
   {
      for (auto i = 0; i != _zc.num_edges(); ++i)
      {
         auto const& info = _zc[i];
         auto pos = std::max<int>(info._leading_edge, 0);
         auto n = info._trailing_edge - pos;
         _bits.set(pos, n, 1);
      }
   }

   namespace detail
   {
      struct auto_correlator
      {
         static constexpr auto value_size = bitstream<>::value_size;

         auto_correlator(bitstream<> const& bits, zero_crossing const& zc)
          : _bits(bits)
          , _zc(zc)
          , _size(bits.size())
          , _mid_array(((_size / value_size) / 2) - 1)
         {}

         std::size_t mid_point() const { return _size / 2; }

         std::size_t operator()(std::size_t pos)
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

         bitstream<> const&         _bits;
         zero_crossing const&       _zc;
         std::size_t const          _size;
         std::size_t const          _mid_array;
      };

      struct collector
      {
         // Intermediate data structure for collecting autocorrelation results
         struct info
         {
            int               _i1 = -1;
            int               _i2 = -1;
            int               _period = -1;
            std::size_t       _count = int_traits<uint16_t>::max;
         };

         collector(zero_crossing const& zc, std::size_t& num_periods)
          : _zc(zc)
          , _num_periods(num_periods)
         {}

         void operator()(info const& next)
         {
            for (auto i = 0; i != 4; ++i)
            {
               // We first compare the quantized period (decimated by 2 LSBs)
               // to determine if next is more or less the same period. If so,
               // we get the best version.
               if (next._period/4 == _info_array[i]._period/4)
               {
                  if (next._count < _info_array[i]._count)
                     _info_array[i]= next;
                  break;
               }
               // Then, we set the incoming info in its proper place (sorted by
               // minimum _count)
               else if (next._count < _info_array[i]._count)
               {
                  if (i < 4)
                     std::memmove(&_info_array[i+1], &_info_array[i], sizeof(info));
                  _info_array[i]= next;
                  _num_periods = i+1;
                  break;
               }
            }
         };

         void operator()(period_detector::info_storage& _info)
         {
            for (auto i = 0; i != _num_periods; ++i)
            {
               auto const& first = _zc[_info_array[i]._i1];
               auto const& next = _zc[_info_array[i]._i2];
               _info[i] =
               {
                  first.fractional_period(next)
                , 1.0f - (float(_info_array[i]._count) / _zc.window_size())
               };
            }
         }

         std::array<info, 4>     _info_array;
         zero_crossing const&    _zc;
         std::size_t&            _num_periods;
      };
   }

   void period_detector::autocorrelate()
   {
      auto threshold = _zc.peak_pulse() * pulse_threshold;
      _num_periods = 0;

      CYCFI_ASSERT(_zc.num_edges() > 1, "Not enough edges.");

      detail::auto_correlator ac{ _bits, _zc };
      detail::collector collect{ _zc, _num_periods };
      auto const mid_point = ac.mid_point();

      for (auto i = 0; i != _zc.num_edges()-1; ++i)
      {
         auto const& first = _zc[i];
         if (first._peak >= threshold)
         {
            for (auto j = i+1; j != _zc.num_edges(); ++j)
            {
               auto const& next = _zc[j];
               if (next._peak >= threshold)
               {
                  auto period = first.period(next);
                  if (period > mid_point)
                     break;
                  if (period >= _min_period)
                  {
                     auto count = ac(period);
                     collect({ i, j, int(period), count });
                  }
               }
            }
         }
      }

      // Get the final resuts
      collect(_info);
   }

   inline bool period_detector::operator()(float s)
   {
      auto r = _zc(s);
      if (_zc.is_ready())
      {
         set_bitstream();
         autocorrelate();
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

