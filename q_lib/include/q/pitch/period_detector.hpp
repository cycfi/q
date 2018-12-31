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
         float             _period = -1;
         float             _periodicity = -1;
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

      bool                 is_ready() const { return _zc.is_ready(); }
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
      float const          _weight;
      std::size_t const    _mid_point;
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
    , _weight(2.0 / _zc.window_size())
    , _mid_point(_zc.window_size() / 2)
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
            float             _periodicity = 0.0f;
            std::size_t       _multiple;
         };

         collector(zero_crossing const& zc, std::size_t& num_periods)
          : _zc(zc)
          , _num_periods(num_periods)
         {}

         void operator()(info const& incoming)
         {
            auto&& save = [&](std::size_t i)
            {
               if (_num_periods && i < 4)
               {
                  auto bytes = sizeof(info) * (3-i);
                  std::memmove(&_info_array[i+1], &_info_array[i], bytes);
               }
               _info_array[i] = incoming;
               _info_array[i]._multiple = 1;
               if (_num_periods < 4)
                  ++_num_periods;
            };

            auto&& save_new = [&]()
            {
               // So now we got a new one coming, we set the incoming info in
               // its proper place (sorted by _periodicity)
               for (auto i = 0; i != 4; ++i)
               {
                  if (incoming._periodicity > _info_array[i]._periodicity)
                  {
                     save(i);
                     break;
                  }
               }
            };

            for (auto i = 0; i != 4; ++i)
            {
               if (i < _num_periods)
               {
                  // We compare the quantized period (decimated by 2 LSBs) to
                  // determine if incoming is more or less the same period.
                  // If so, we get the best version.
                  auto period = _info_array[i]._period/4;
                  auto&& try_harmonic = [&](auto harmonic)
                  {
                     if (incoming._period/(harmonic*4) == period)
                     {
                        auto diff = std::abs(
                           incoming._periodicity - _info_array[i]._periodicity
                        );

                        // If incoming peridicity is within 5%, then incoming
                        // is most probably a harmonic.
                        if (diff < 0.05)
                        {
                           if (incoming._periodicity > _info_array[i]._periodicity)
                           {
                              _info_array[i]._i1 = incoming._i1;
                              _info_array[i]._i2 = incoming._i2;
                              _info_array[i]._periodicity = incoming._periodicity;
                              _info_array[i]._multiple = harmonic;
                           }
                        }
                        else
                        {
                           save_new();
                        }
                        return true;
                     }
                     return false;
                  };

                  // First we try the 4th harmonic
                  if (try_harmonic(4))
                     break;

                  // Next we try the 3rd harmonic
                  if (try_harmonic(3))
                     break;

                  // Next we try the 2nd harmonic
                  if (try_harmonic(2))
                     break;

                  // Then we try the fundamental
                  if (try_harmonic(1))
                     break;
               }
               else
               {
                  save_new();
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
                  first.fractional_period(next) / _info_array[i]._multiple
                , _info_array[i]._periodicity
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

      for (auto i = 0; i != _zc.num_edges()-1; ++i)
      {
         auto const& first = _zc[i];
         // if (first._peak >= threshold)
         {
            for (auto j = i+1; j != _zc.num_edges(); ++j)
            {
               auto const& next = _zc[j];
               // if (next._peak >= threshold)
               {
                  auto period = first.period(next);
                  if (period > _mid_point)
                     break;
                  if (period >= _min_period)
                  {
                     auto count = ac(period);
                     float periodicity = 1.0f - (count * _weight);
                     collect({ i, j, int(period), periodicity });
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

