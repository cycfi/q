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
#include <stdexcept>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   class period_detector
   {
   public:

      static constexpr float pulse_threshold = 0.6;
      static constexpr float harmonic_periodicity_factor = 16;
      static constexpr float periodicity_diff_factor = 0.8 / 100; // % of the midpoint

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

      bool                    operator()(float s);
      bool                    operator()() const;

      bool                    is_ready() const        { return _zc.is_ready(); }
      bool                    is_reset() const        { return _zc.is_reset(); }
      std::size_t const       minimum_period() const  { return _min_period; }
      bitset<> const&         bits() const            { return _bits; }
      zero_crossing const&    edges() const           { return _zc; }
      float                   predict_period() const;

      info const&             fundamental() const     { return _fundamental; }
      float                   harmonic(std::size_t index) const;

   private:

      void                    set_bitstream();
      void                    autocorrelate();
      int                     autocorrelate(bitstream_acf<> const& ac, std::size_t& period, bool first) const;

      zero_crossing           _zc;
      info                    _fundamental;
      std::size_t const       _min_period;
      int                     _range;
      bitset<>                _bits;
      float const             _weight;
      std::size_t const       _mid_point;
      float const             _period_diff_threshold;
      mutable float           _predicted_period = -1.0f;
      std::size_t             _edge_mark = 0;
      mutable std::size_t     _predict_edge = 0;
      std::size_t             _num_pulses = 0;
      bool                    _half_empty = false;
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
    , _range(float(highest_freq) / float(lowest_freq))
    , _bits(_zc.window_size())
    , _weight(2.0 / _zc.window_size())
    , _mid_point(_zc.window_size() / 2)
    , _period_diff_threshold(_mid_point * periodicity_diff_factor)
   {
      if (highest_freq <= lowest_freq)
         throw std::runtime_error(
            "Error: highest_freq <= lowest_freq."
         );
   }

   inline void period_detector::set_bitstream()
   {
      auto threshold = _zc.peak_pulse() * pulse_threshold;
      std::size_t leading_edge = _zc.window_size();
      std::size_t trailing_edge = 0;

      _num_pulses = 0;
      _bits.clear();
      for (auto i = 0; i != _zc.num_edges(); ++i)
      {
         auto const& info = _zc[i];
         if (info._peak >= threshold)
         {
            ++_num_pulses;
            if (info._leading_edge < leading_edge)
               leading_edge = info._leading_edge;
            if (info._trailing_edge > trailing_edge)
               trailing_edge = info._trailing_edge;
            auto pos = std::max<int>(info._leading_edge, 0);
            auto n = info._trailing_edge - pos;
            _bits.set(pos, n, 1);
         }
      }
      _half_empty = leading_edge > _mid_point || trailing_edge < _mid_point;
   }

   namespace detail
   {
      struct sub_collector
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

         sub_collector(zero_crossing const& zc, float period_diff_threshold, int range_)
          : _zc(zc)
          , _harmonic_threshold(
               period_detector::harmonic_periodicity_factor*2 / zc.window_size())
          , _period_diff_threshold(period_diff_threshold)
          , _range(range_)
         {}

         bool empty() const
         {
            return _fundamental._period == -1;
         }

         float period_of(info const& x) const
         {
            auto const& first = _zc[x._i1];
            auto const& next = _zc[x._i2];
            return first.fractional_period(next);
         }

         void save(info const& incoming)
         {
            _fundamental = incoming;
            _fundamental._harmonic = 1;
            _first_period = period_of(_fundamental);
         };

         bool try_sub_harmonic(std::size_t harmonic, info const& incoming, float incoming_period)
         {
            if (std::abs(incoming_period - _first_period) < _period_diff_threshold)
            {
               // If incoming is a different harmonic and has better
               // periodicity ...
               if (incoming._periodicity > _fundamental._periodicity &&
                  harmonic != _fundamental._harmonic)
               {
                  auto period_diff = std::abs(
                     incoming._periodicity - _fundamental._periodicity);

                  // If incoming periodicity is within the harmonic
                  // periodicity threshold, then replace _fundamental with
                  // incoming. Take note of the harmonic for later.
                  if (period_diff <= _harmonic_threshold)
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
            if (incoming._period < _first_period)
               return false;

            float incoming_period = period_of(incoming);
            int multiple = std::max(1.0f, std::round(incoming_period / _first_period));
            return try_sub_harmonic(std::min(_range, multiple), incoming, incoming_period/multiple);
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
               result =
               {
                  period_of(info) / info._harmonic
                , info._periodicity
               };
            }
            else
            {
               result = period_detector::info{};
            }
         }

         float                   _first_period;
         info                    _fundamental;
         zero_crossing const&    _zc;
         float const             _harmonic_threshold;
         float const             _period_diff_threshold;
         int const               _range;
      };
   }

   inline int period_detector::autocorrelate(bitstream_acf<> const& ac, std::size_t& period, bool first) const
   {
      auto count = ac(period);
      auto mid = ac._mid_array * bitset<>::value_size;
      auto start = period;

      if (first && count == 0)   // make sure this is not a false correlation
      {
         if (ac(period/2) == 0)  // oops false correlation!
            return -1;           // flag the return as a false correlation
      }
      else if (period < 32) // Search minimum if the resolution is low
      {
         // Search upwards for the minimum autocorrelation count
         for (auto p = start + 1; p < mid; ++p)
         {
            auto c = ac(p);
            if (c > count)
               break;
            count = c;
            period = p;
         }
         // Search downwards for the minimum autocorrelation count
         for (auto p = start - 1; p > _min_period; --p)
         {
            auto c = ac(p);
            if (c > count)
               break;
            count = c;
            period = p;
         }
      }
      return count;
   }

   inline void period_detector::autocorrelate()
   {
      auto threshold = _zc.peak_pulse() * pulse_threshold;

      CYCFI_ASSERT(_zc.num_edges() > 1, "Not enough edges.");

      bitstream_acf<> ac{ _bits };
      detail::sub_collector collect{_zc, _period_diff_threshold, _range };

      if (_half_empty || _num_pulses < 2)
      {
         _fundamental._periodicity = -1; // force reset
         return;
      }
      else
      {
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
                           auto count = autocorrelate(ac, period, collect.empty());
                           if (count == -1)
                              return; // Return early if we have a false correlation
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
      }

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
                     if (edge1._peak >= threshold)
                     {
                        auto p = edge1.fractional_period(edge2);
                        if (p > _min_period)
                           return (_predicted_period = p);
                     }
                  }
                  return _predicted_period = -1.0f;
               }
            }
         }
      }
      return _predicted_period;
   }
}

#endif

