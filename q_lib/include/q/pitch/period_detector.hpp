/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_PERIOD_DETECTOR_HPP_MARCH_12_2018)
#define CYCFI_Q_PERIOD_DETECTOR_HPP_MARCH_12_2018

#include <q/utility/bitstream.hpp>
#include <q/utility/zero_crossing.hpp>
#include <q/utility/auto_correlator.hpp>
#include <cmath>

namespace cycfi { namespace q
{
   ////////////////////////////////////////////////////////////////////////////
   class period_detector
   {
   public:

      static constexpr float minumum_pulse_threshold = 0.6;
      static constexpr float harmonic_periodicity_factor = 15;

      struct info
      {
         float             _period = -1;
         float             _periodicity = -1;
      };

                           period_detector(
                              frequency lowest_freq
                            , frequency highest_freq
                            , std::uint32_t sps
                            , decibel hysteresis
                           );

                           period_detector(period_detector const& rhs) = default;
                           period_detector(period_detector&& rhs) = default;

      period_detector&     operator=(period_detector const& rhs) = default;
      period_detector&     operator=(period_detector&& rhs) = default;

      bool                 operator()(float s);
      bool                 operator()() const;

      bool                 is_ready() const        { return _zc.is_ready(); }
      float                predict_period() const  { return _zc.predict_period(); }
      std::size_t const    minimum_period() const  { return _min_period; }
      bitstream<> const&   bits() const            { return _bits; }
      zero_crossing const& edges() const           { return _zc; }

      info const&          fundamental() const     { return _fundamental; }
      info                 harmonic(std::size_t index) const;

   private:

      void                 set_bitstream();
      void                 autocorrelate();

      zero_crossing        _zc;
      info                 _fundamental;
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
    , decibel hysteresis
   )
    : _zc(hysteresis, float(lowest_freq.period() * 2) * sps)
    , _min_period(float(highest_freq.period()) * sps)
    , _bits(_zc.window_size())
    , _weight(2.0 / _zc.window_size())
    , _mid_point(_zc.window_size() / 2)
   {}

   inline void period_detector::set_bitstream()
   {
      _bits.clear();
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

         collector(zero_crossing const& zc)
          : _zc(zc)
          , _harmonic_threshold(
               period_detector::harmonic_periodicity_factor*2 / zc.window_size())
         {}

         template <std::size_t harmonic>
         void save(info const& incoming, info& dest)
         {
            dest = incoming;
            dest._multiple = harmonic;
         };

         template <std::size_t harmonic>
         void save_fundamental(info const &incoming)
         {
            save<harmonic>(incoming, _fundamental);
         };

         template <std::size_t harmonic>
         void save_new(info const& incoming)
         {
            if (incoming._periodicity > _fundamental._periodicity)
               save_fundamental<harmonic>(incoming);
         }

         template <std::size_t harmonic>
         bool try_harmonic(info const& incoming)
         {
            int incoming_period = incoming._period/harmonic;
            int current_period = _fundamental._period;
            int diff = std::abs(incoming_period - current_period);
            if (diff < 2)
            {
               auto diff = std::abs(
                  incoming._periodicity - _fundamental._periodicity);

               if (diff < _harmonic_threshold && harmonic != _fundamental._multiple)
               {
                  // If incoming periodicity is within the harmonic
                  // periodicity threshold, then replace _first with
                  // incoming, if it has better periodicity, but taking note
                  // of the harmonic for later.
                  if (incoming._periodicity > _fundamental._periodicity)
                  {
                     _fundamental._i1 = incoming._i1;
                     _fundamental._i2 = incoming._i2;
                     _fundamental._periodicity = incoming._periodicity;
                     _fundamental._multiple = harmonic;
                  }
               }
               else
               {
                  // If not, then we save this a distinct harmonic.
                  if (harmonic != _fundamental._multiple)
                     save_new<1>(incoming);
               }
               return true;
            }
            return false;
         };

         bool process_harmonics(info const& incoming)
         {
            // First we try the 5th harmonic
            if (try_harmonic<5>(incoming))
               return true;

            // First we try the 4th harmonic
            if (try_harmonic<4>(incoming))
               return true;

            // Next we try the 3rd harmonic
            if (try_harmonic<3>(incoming))
               return true;

            // Next we try the 2nd harmonic
            if (try_harmonic<2>(incoming))
               return true;

            // Then we try the fundamental
            if (try_harmonic<1>(incoming))
               return true;

            return false;
         }

         void operator()(info const& incoming)
         {
            if (_fundamental._period == -1.0f)
               save<1>(incoming, _fundamental);

            else if (process_harmonics(incoming))
               return;

            else if (incoming._periodicity > _fundamental._periodicity)
               save_fundamental<1>(incoming);
         };

         void get(info const& info, period_detector::info& result)
         {
            if (info._period != -1.0f)
            {
               auto const& first = _zc[info._i1];
               auto const& next = _zc[info._i2];
               result =
               {
                  first.fractional_period(next) / info._multiple
                , info._periodicity
               };
            }
            else
            {
               result = period_detector::info{};
            }
         }

         info                    _fundamental;
         zero_crossing const&    _zc;
         float const             _harmonic_threshold;
      };
   }

   void period_detector::autocorrelate()
   {
      auto threshold = _zc.peak_pulse() * minumum_pulse_threshold;

      CYCFI_ASSERT(_zc.num_edges() > 1, "Not enough edges.");

      auto_correlator ac{ _bits };
      detail::collector collect{ _zc };

      [&]()
      {
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
                     if (period > _mid_point)
                        break;
                     if (period >= _min_period)
                     {
                        auto count = ac(period);
                        float periodicity = 1.0f - (count * _weight);
                        collect({ i, j, int(period), periodicity });
                        if (count == 0)
                           return;
                     }
                  }
               }
            }
         }
      }();

      // Get the final resuts
      collect.get(collect._fundamental, _fundamental);
   }

   inline bool period_detector::operator()(float s)
   {
      _zc(s);
      if (_zc.is_ready())
      {
         set_bitstream();
         autocorrelate();
         return true;
      }
      return false;
   }

   inline period_detector::info period_detector::harmonic(std::size_t index) const
   {
      if (index > 0)
      {
         if (index == 1)
            return _fundamental;

         auto target_period = _fundamental._period / index;
         if (target_period >= _min_period && target_period < _mid_point)
         {
            auto_correlator ac{ _bits };
            auto count = ac(std::round(target_period));
            float periodicity = 1.0f - (count * _weight);
            return info{ target_period, periodicity };
         }
      }
      return info{};
   }

   inline bool period_detector::operator()() const
   {
      return _zc();
   }
}}

#endif

