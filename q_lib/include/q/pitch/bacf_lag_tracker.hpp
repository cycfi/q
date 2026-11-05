/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_BACF_LAG_TRACKER_NOVEMBER_2_2026)
#define CYCFI_Q_BACF_LAG_TRACKER_NOVEMBER_2_2026

#include <q/support/base.hpp>
#include <q/support/decibel.hpp>
#include <q/support/duration.hpp>
#include <q/utility/bitset.hpp>
#include <q/utility/bitstream_acf.hpp>
#include <q/utility/ring_buffer.hpp>
#include <q/utility/zero_crossing_collector.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // bacf_lag_tracker: the lag at which a signal repeats, tracked with
   // bitstream autocorrelation, one correlation per zero crossing. A
   // comparator with hysteresis makes the bitstream; at each crossing,
   // leading and trailing, a locked lag is scored over the latest window
   // against the window one lag earlier, swept two samples either side,
   // and kept while under the mismatch threshold (a parabola through the
   // best and its neighbours gives the fraction of a sample). On a miss,
   // twice the lag is tried, since a chord's common period is a multiple
   // of each note's. Two misses in a row drop the lock; lag() then holds
   // the last good lag until a new lock. Unlocked, each completed pulse is
   // scored against the latest similar pulses before it in range.
   //
   // The lag is a period or a multiple of one: octaves are not settled.
   // The work is per crossing, flat: no search runs per sample or per
   // frame. Low-pass the input first (about 1 kHz for a guitar): fewer
   // crossings, cleaner lags. Allocates at construction only.
   ////////////////////////////////////////////////////////////////////////////
   class bacf_lag_tracker
   {
   public:

      static constexpr float        threshold      = 0.15f;  // of the window
      static constexpr int          max_misses     = 2;
      static constexpr int          sweep          = 2;
      static constexpr int          multiples      = 2;
      static constexpr int          partners       = 3;
      static constexpr std::size_t  edge_capacity  = 256;

                           bacf_lag_tracker(
                              duration min_lag
                            , duration max_lag
                            , float sps
                            , decibel hysteresis =
                                 decibel{-45.0, direct_unit}
                            , duration window = duration{0.0116}
                           );

      bool                 operator()(float s);
      bool                 locked() const;
      float                lag() const;
      float                periodicity() const;
      std::size_t          window() const;
      void                 drop();
      void                 reset();

   private:

      struct pulse
      {
         std::uint32_t     lead;
         float             peak;
         float             width;
      };

      using edge_ring = ring_buffer<std::uint32_t>;
      using pulse_ring = ring_buffer<pulse>;
      using word = bitset<>::value_type;
      static constexpr std::size_t ws = bitset<>::value_size;

      void                 edge(
                              edge_ring& kind, std::size_t& count
                            , bool trailing
                           );
      void                 track(edge_ring const& kind, std::size_t count);
      void                 acquire();
      std::uint32_t        score(std::uint32_t lag) const;
      bool                 fits(std::uint32_t lag) const;
      void                 compact();

      std::uint32_t        _min_lag;
      std::uint32_t        _max_lag;
      std::uint32_t        _window;       // bits, whole words
      std::uint32_t        _limit;        // mismatches
      float                _hysteresis;
      std::size_t          _keep;         // bits a correlation reaches back
      bitset<>             _bits;
      edge_ring            _lead;
      edge_ring            _trail;
      pulse_ring           _pulses;
      std::size_t          _num_lead = 0;
      std::size_t          _num_trail = 0;
      std::size_t          _num_pulses = 0;
      std::size_t          _pos = 0;      // the next bit
      std::uint32_t        _now = 0;      // samples, wrapping
      std::size_t          _since = 0;    // samples since reset, saturating
      bool                 _state = false;
      pulse                _pulse{};
      std::uint32_t        _lag = 0;
      float                _held = 0.0f;  // the last good lag()
      float                _frac = 0.0f;
      std::uint32_t        _mismatch = 0;
      int                  _misses = 0;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Inlines
   ////////////////////////////////////////////////////////////////////////////
   inline bacf_lag_tracker::bacf_lag_tracker(
      duration min_lag
    , duration max_lag
    , float sps
    , decibel hysteresis
    , duration window
   )
    : _min_lag{std::uint32_t(std::lround(as_double(min_lag) * sps))}
    , _max_lag{std::uint32_t(std::lround(as_double(max_lag) * sps))}
    , _window{std::max<std::uint32_t>(ws, std::uint32_t(
         std::lround(as_double(window) * sps / ws)) * ws)}
    , _limit{std::uint32_t(threshold * _window)}
    , _hysteresis{lin_float(hysteresis)}
    , _keep{
         ((_window + multiples * (_max_lag + sweep) + 2 * ws) / ws + 1) * ws}
    , _bits{2 * _keep + ws}
    , _lead(edge_capacity)
    , _trail(edge_capacity)
    , _pulses(edge_capacity)
   {}

   // One sample; true while locked.
   inline bool bacf_lag_tracker::operator()(float s)
   {
      if (_pos == 2 * _keep)
         compact();
      if (!_state && s > _hysteresis)
      {
         _state = true;
         _pulse = {_now, s, 0.0f};
         edge(_lead, _num_lead, false);
      }
      else if (_state && s < -_hysteresis)
      {
         _state = false;
         if (_pulse.width == 0.0f)
            _pulse.width = float(_now - _pulse.lead);
         _pulses.push(_pulse);
         _num_pulses = std::min(_num_pulses + 1, edge_capacity);
         edge(_trail, _num_trail, true);
      }
      if (_state)
      {
         _pulse.peak = std::max(_pulse.peak, s);
         if (_pulse.width == 0.0f && s < _pulse.peak * 0.3f)
            _pulse.width = float(_now - _pulse.lead);
      }
      _bits.set(_pos++, _state);
      ++_now;
      _since = std::min(_since + 1, _keep);
      if (_lag)
         _held = float(_lag) + _frac;
      return _lag != 0;
   }

   inline bool bacf_lag_tracker::locked() const
   {
      return _lag != 0;
   }

   // The tracked lag in samples, with its fraction; unlocked, the last
   // good one (0 before any).
   inline float bacf_lag_tracker::lag() const
   {
      return _lag ? float(_lag) + _frac : _held;
   }

   // The last score's match: 1 when the bits repeat exactly, 0.5 for
   // noise.
   inline float bacf_lag_tracker::periodicity() const
   {
      return 1.0f - float(_mismatch) / float(_window);
   }

   inline std::size_t bacf_lag_tracker::window() const
   {
      return _window;
   }

   // Drop the lock, at a new note: lag() holds until a new lock.
   inline void bacf_lag_tracker::drop()
   {
      _lag = 0;
      _misses = 0;
   }

   inline void bacf_lag_tracker::reset()
   {
      _bits.clear();
      _num_lead = _num_trail = _num_pulses = 0;
      _pos = 0;
      _since = 0;
      _state = false;
      _lag = 0;
      _held = 0.0f;
      _frac = 0.0f;
      _mismatch = 0;
      _misses = 0;
   }

   // Mismatches between the latest window and the window `lag` earlier,
   // the window's end aligned down to a word.
   inline std::uint32_t bacf_lag_tracker::score(std::uint32_t lag) const
   {
      auto const start = (_pos - _window - lag) / ws;
      bitstream_acf<> const acf{_bits};
      return std::uint32_t(acf(lag, start, _window / ws));
   }

   // Enough history for a score at `lag`, and in range.
   inline bool bacf_lag_tracker::fits(std::uint32_t lag) const
   {
      return lag >= _min_lag && lag <= _max_lag
         && _since >= _window + lag + ws;
   }

   inline void bacf_lag_tracker::edge(
      edge_ring& kind, std::size_t& count, bool trailing)
   {
      if (_lag)
         track(kind, count);
      else if (trailing)
         acquire();
      kind.push(_now);
      count = std::min(count + 1, edge_capacity);
   }

   // Locked: nudge the lag to this edge's spacing from the earlier edge
   // of its kind nearest a lag back (within 3%), then sweep it.
   inline void bacf_lag_tracker::track(
      edge_ring const& kind, std::size_t count)
   {
      auto lag = _lag;
      auto const tol = std::max<std::uint32_t>(2, _lag * 3 / 100);
      auto nearest = tol + 1;
      for (std::size_t i = 0; i != count; ++i)
      {
         auto const d = _now - kind[i];
         if (d > _lag + tol)
            break;
         auto const off = d > _lag ? d - _lag : _lag - d;
         if (off < nearest)
         {
            nearest = off;
            lag = d;
         }
      }
      if (!fits(lag - sweep) || !fits(lag + sweep))
         return;

      std::uint32_t m[2 * sweep + 1];
      std::uint32_t best = 0;
      for (int d = 0; d != 2 * sweep + 1; ++d)
      {
         m[d] = score(lag - sweep + d);
         if (m[d] < m[best])
            best = d;
      }
      auto best_lag = lag - sweep + best;
      auto best_m = m[best];

      _frac = 0.0f;
      if (best > 0 && best < 2 * sweep)
      {
         float const l = m[best - 1], c = m[best], r = m[best + 1];
         float const den = l - 2 * c + r;
         if (den > 0)
            _frac = std::clamp(0.5f * (l - r) / den, -0.5f, 0.5f);
      }

      for (int k = 2; k <= multiples && best_m >= _limit; ++k)
      {
         auto const at = lag * k;
         auto const reach = std::uint32_t(sweep * k);
         if (!fits(at - reach) || !fits(at + reach))
            break;
         for (auto l = at - reach; l <= at + reach; ++l)
         {
            auto const mm = score(l);
            if (mm < best_m)
            {
               best_m = mm;
               best_lag = l;
               _frac = 0.0f;
            }
         }
      }

      _mismatch = best_m;
      if (best_m < _limit)
      {
         _lag = best_lag;
         _misses = 0;
      }
      else if (++_misses >= max_misses)
      {
         _lag = 0;
         _misses = 0;
      }
   }

   // Unlocked, a pulse has ended: score it against the latest similar
   // pulses before it, at most `partners`, and lock on the best under the
   // threshold.
   inline void bacf_lag_tracker::acquire()
   {
      if (_num_pulses < 2)
         return;
      auto const& now = _pulses[0];
      auto best = std::uint32_t(0);
      auto best_m = _limit;
      int tried = 0;
      for (std::size_t j = 1; j != _num_pulses && tried != partners; ++j)
      {
         auto const& p = _pulses[j];
         auto const d = now.lead - p.lead;
         if (d > _max_lag)
            break;
         if (d < _min_lag)
            continue;
         bool const similar =
            rel_within(p.peak, now.peak
             , 1.0f - zero_crossing_collector::pulse_height_diff)
            && rel_within(p.width, now.width
             , 1.0f - zero_crossing_collector::pulse_width_diff);
         if (!similar)
            continue;
         if (!fits(d))
            break;
         ++tried;
         auto const m = score(d);
         if (m < best_m)
         {
            best_m = m;
            best = d;
         }
      }
      if (best)
      {
         _lag = best;
         _frac = 0.0f;
         _mismatch = best_m;
         _misses = 0;
      }
   }

   // Slide the bits: keep the latest _keep, moved to the start, a whole
   // number of words.
   inline void bacf_lag_tracker::compact()
   {
      auto* p = _bits.data();
      std::memmove(p, p + _keep / ws, (_keep / ws) * sizeof(word));
      std::fill(p + _keep / ws, p + (_bits.size() / ws), word{0});
      _pos = _keep;
   }
}

#endif
