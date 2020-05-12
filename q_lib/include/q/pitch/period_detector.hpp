/*=============================================================================
   Copyright (c) 2014-2020 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_PERIOD_DETECTOR_HPP_MARCH_12_2018)
#define CYCFI_Q_PERIOD_DETECTOR_HPP_MARCH_12_2018

#include <q/utility/bitset.hpp>
#include <q/utility/zero_crossing.hpp>
#include <q/utility/bitstream_acf.hpp>
#include <q/fx/feature_detection.hpp>
#include <q/fx/envelope.hpp>
#include <cmath>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   class period_detector
   {
   public:

      static constexpr float pulse_threshold = 0.6;
      static constexpr float harmonic_periodicity_factor = 15;
      static constexpr float periodicity_diff_factor = 0.008;

      struct info
      {
         float                _period = -1;
         float                _periodicity = 0.0f;
      };

                              period_detector(
                                 frequency lowest_freq
                               , frequency highest_freq
                               , std::uint32_t sps
                               , decibel hysteresis
                              );

                              period_detector(period_detector const& rhs) = default;
                              period_detector(period_detector&& rhs) = default;

      period_detector&        operator=(period_detector const& rhs) = default;
      period_detector&        operator=(period_detector&& rhs) = default;

      bool                    operator()(float s);
      bool                    operator()() const;

      bool                    is_ready() const        { return _zc.is_ready(); }
      std::size_t const       minimum_period() const  { return _min_period; }
      bitset<> const&         bits() const            { return _bits; }
      zero_crossing const&    edges() const           { return _zc; }
      float                   predict_period() const;

      info const&             fundamental() const     { return _fundamental; }
      float                   harmonic(std::size_t index) const;

   private:

      void                    set_bitstream();
      void                    autocorrelate();

      zero_crossing           _zc;
      info                    _fundamental;
      std::size_t const       _min_period;
      bitset<>                _bits;
      float const             _weight;
      std::size_t const       _mid_point;
      float const             _periodicity_diff_threshold;
      mutable float           _predicted_period = -1.0f;
      std::size_t             _edge_mark = 0;
      mutable std::size_t     _predict_edge = 0;
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
    , _periodicity_diff_threshold(_mid_point * periodicity_diff_factor)
   {}

   inline void period_detector::set_bitstream()
   {
      auto threshold = _zc.peak_pulse() * pulse_threshold;

      _bits.clear();
      for (auto i = 0; i != _zc.num_edges(); ++i)
      {
         auto const& info = _zc[i];
         if (info._peak >= threshold)
         {
            auto pos = std::max<int>(info._leading_edge, 0);
            auto n = info._trailing_edge - pos;
            _bits.set(pos, n, 1);
         }
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
            std::size_t       _harmonic;
         };

         collector(zero_crossing const& zc, float periodicity_diff_threshold)
          : _zc(zc)
          , _harmonic_threshold(
               period_detector::harmonic_periodicity_factor*2 / zc.window_size())
          , _periodicity_diff_threshold(periodicity_diff_threshold)
         {}

         void save(info const& incoming)
         {
            _fundamental = incoming;
            _fundamental._harmonic = 1;
         };

         template <std::size_t harmonic>
         bool try_harmonic(info const& incoming)
         {
            int incoming_period = incoming._period / harmonic;
            int current_period = _fundamental._period;
            if (std::abs(incoming_period - current_period) < _periodicity_diff_threshold)
            {
               // If incoming is a different harmonic and has better
               // periodicity ...
               if (incoming._periodicity > _fundamental._periodicity &&
                  harmonic != _fundamental._harmonic)
               {
                  auto periodicity_diff = std::abs(
                     incoming._periodicity - _fundamental._periodicity);

                  // If incoming periodicity is within the harmonic
                  // periodicity threshold, then replace _fundamental with
                  // incoming. Take note of the harmonic for later.
                  if (periodicity_diff < _harmonic_threshold)
                  {
                     _fundamental._i1 = incoming._i1;
                     _fundamental._i2 = incoming._i2;
                     _fundamental._periodicity = incoming._periodicity;
                     _fundamental._harmonic = harmonic;
                  }

                  // If not, then we save incoming (replacing the current
                  // _fundamental).
                  else
                  {
                     save(incoming);
                  }
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
               save(incoming);

            else if (process_harmonics(incoming))
               return;

            else if (incoming._periodicity > _fundamental._periodicity)
               save(incoming);
         };

         void get(info const& info, period_detector::info& result)
         {
            if (info._period != -1.0f)
            {
               auto const& first = _zc[info._i1];
               auto const& next = _zc[info._i2];
               result =
               {
                  first.fractional_period(next) / info._harmonic
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
         float const             _periodicity_diff_threshold;
      };
   }

   inline void period_detector::autocorrelate()
   {
      auto threshold = _zc.peak_pulse() * pulse_threshold;

      CYCFI_ASSERT(_zc.num_edges() > 1, "Not enough edges.");

      bitstream_acf<>   ac{ _bits };
      detail::collector collect{ _zc, _periodicity_diff_threshold };

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
                           return; // Return early if we have perfect correlation
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
      // Zero crossing
      bool prev = _zc();
      bool zc = _zc(s);

      if (!zc && prev != zc)
      {
         ++_edge_mark;
         _predicted_period = -1.0f;
      }

      if (_zc.is_reset())
         _fundamental = info{};

      if (_zc.is_ready())
      {
         set_bitstream();
         autocorrelate();
         return true;
      }
      return false;
   }

   inline float period_detector::harmonic(std::size_t index) const
   {
      if (index > 0)
      {
         if (index == 1)
            return _fundamental._periodicity;

         auto target_period = _fundamental._period / index;
         if (target_period >= _min_period && target_period < _mid_point)
         {
            bitstream_acf<> ac{ _bits };
            auto count = ac(std::round(target_period));
            float periodicity = 1.0f - (count * _weight);
            return periodicity;
         }
      }
      return 0.0f;
   }

   inline bool period_detector::operator()() const
   {
      return _zc();
   }

   inline float period_detector::predict_period() const
   {
      if (_predicted_period == -1.0f && _edge_mark != _predict_edge)
      {
         _predict_edge = _edge_mark;
         if (_zc.num_edges() > 1)
         {
            auto threshold = _zc.peak_pulse() * pulse_threshold;
            for (int i = _zc.num_edges()-1; i > 0; --i)
            {
               auto const& edge2 = _zc[i];
               if (edge2._peak >= threshold)
               {
                  for (int j = i-1; j >= 0; --j)
                  {
                     auto const& edge1 = _zc[j];
                     if (edge1.similar(edge2))
                     {
                        _predicted_period = edge1.fractional_period(edge2);
                        return _predicted_period;
                     }
                  }
               }
            }
         }
      }
      return _predicted_period;
   }
}

#endif

