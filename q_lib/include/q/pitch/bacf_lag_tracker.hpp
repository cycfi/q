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
   // of each note's. Two misses in a row drop the lock, as does a lag that
   // has drifted out of range; lag() then holds the last good lag until a
   // new lock. Unlocked, at each crossing the latest pulse is scored
   // against the latest similar pulses before it in range.
   //
   // drop() starts a new note: until the window holds only the note, the
   // score runs over one lag of it. provisional() is the note's first
   // estimate, before a lock: the latest strong pulse's span to the nearest
   // similar one. drop's `hint`, the last good lag, guards a chord: after a
   // long one (over 13 ms), an estimate that is not that lag or a clean
   // fraction of it must first repeat over one lag.
   //
   // The lag is a period or a multiple of one: octaves are not settled.
   // The work is per crossing, flat. Low-pass the input first (about 1 kHz
   // for a guitar): fewer crossings, cleaner lags. Allocates at
   // construction only.
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
      float                provisional() const;
      std::size_t          window() const;
      void                 drop(float hint = 0.0f);
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
      std::uint32_t        score(std::uint32_t lag, std::uint32_t w) const;
      std::uint32_t        one_lag(std::uint32_t lag) const;
      std::uint32_t        span(std::uint32_t lag) const;
      bool                 fits(std::uint32_t lag) const;
      void                 estimate();
      void                 vet();
      void                 compact();

      static constexpr float        height_diff    = 0.4f;   // similar pulses
      static constexpr float        strong         = 0.6f;   // of the peak pulse
      static constexpr float        long_hint      = 0.013f; // seconds
      static constexpr float        agree          = 0.02f;
      static constexpr float        vet_periodicity = 0.85f;

      std::uint32_t        _min_lag;
      std::uint32_t        _max_lag;
      std::uint32_t        _window;       // bits, whole words
      std::uint32_t        _min_window;   // samples, the least a score runs over
      float                _long_hint;    // samples
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
      // _now at the last drop; before any, long ago (half the wrap): the
      // full window from the start, and no estimate.
      static constexpr std::uint32_t never = 0x80000000u;
      std::uint32_t        _drop = never;
      float                _hint = 0.0f;
      float                _peak = 0.0f;  // the peak pulse since the drop
      std::uint32_t        _estimate = 0; // its first pair's span
      bool                 _vetted = false;
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
    , _min_window{std::uint32_t(0.002f * sps)}
    , _long_hint{long_hint * sps}
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
         if (std::int32_t(_pulse.lead - _drop) >= 0)
            _peak = std::max(_peak, _pulse.peak);
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
      if (_estimate && !_vetted)
         vet();
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

   // The note's first estimate since the drop, in samples; 0 until there
   // is one (or, after a long hint, until it agrees or has repeated).
   inline float bacf_lag_tracker::provisional() const
   {
      return _vetted ? float(_estimate) : 0.0f;
   }

   inline std::size_t bacf_lag_tracker::window() const
   {
      return _window;
   }

   // Drop the lock, at a new note: lag() holds until a new lock. `hint`
   // is the last good lag, in samples (0: none).
   inline void bacf_lag_tracker::drop(float hint)
   {
      _lag = 0;
      _misses = 0;
      _drop = _now;
      _hint = hint;
      _peak = 0.0f;
      _estimate = 0;
      _vetted = false;
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
      _drop = never;
      _hint = 0.0f;
      _peak = 0.0f;
      _estimate = 0;
      _vetted = false;
   }

   // Mismatches between the latest `w` bits and the `w` bits `lag`
   // earlier, the end aligned down to a word.
   inline std::uint32_t bacf_lag_tracker::score(
      std::uint32_t lag, std::uint32_t w) const
   {
      auto const start = (_pos - w - lag) / ws;
      bitstream_acf<> const acf{_bits};
      return std::uint32_t(acf(lag, start, w / ws));
   }

   // One lag (at least _min_window), whole words: the shortest score.
   inline std::uint32_t bacf_lag_tracker::one_lag(std::uint32_t lag) const
   {
      auto const m = std::max(lag, _min_window);
      return (m + ws - 1) / ws * ws;
   }

   // The window a score at `lag` runs over: one lag of the note until the
   // whole window is after the drop.
   inline std::uint32_t bacf_lag_tracker::span(std::uint32_t lag) const
   {
      auto const since = _now - _drop;
      if (since < lag + ws)
         return one_lag(lag);
      auto const after = (since - lag - ws) / ws * ws;
      return after >= _window ? _window : one_lag(lag);
   }

   // Mismatches at `lag`, scaled to the full window.
   inline std::uint32_t bacf_lag_tracker::score(std::uint32_t lag) const
   {
      auto const w = span(lag);
      auto const m = score(lag, w);
      return w == _window ? m
         : std::uint32_t(std::lround(double(m) * _window / w));
   }

   // Enough history for a score at `lag`, and in range.
   inline bool bacf_lag_tracker::fits(std::uint32_t lag) const
   {
      return lag >= _min_lag && lag <= _max_lag
         && _since >= span(lag) + lag + ws
         && _now - _drop >= one_lag(lag) + lag + ws;
   }

   inline void bacf_lag_tracker::edge(
      edge_ring& kind, std::size_t& count, bool trailing)
   {
      if (_lag)
         track(kind, count);
      else
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
      {
         // A lag that cannot be scored is a miss, or the lock would hold.
         if (++_misses >= max_misses)
         {
            _lag = 0;
            _misses = 0;
         }
         return;
      }

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
      if (!_estimate)
         estimate();
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
            rel_within(p.peak, now.peak, height_diff)
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

   // The note's first estimate: the latest pulse, if strong, to the
   // nearest similar pulse after the drop.
   inline void bacf_lag_tracker::estimate()
   {
      auto const& now = _pulses[0];
      if (std::int32_t(now.lead - _drop) < 0 || now.peak < strong * _peak)
         return;
      for (std::size_t j = 1; j != _num_pulses; ++j)
      {
         auto const& p = _pulses[j];
         if (std::int32_t(p.lead - _drop) < 0)
            break;
         auto const d = now.lead - p.lead;
         if (d > _max_lag)
            break;
         if (d < _min_lag)
            continue;
         if (rel_within(p.peak, now.peak, height_diff)
            && rel_within(p.width, now.width
             , 1.0f - zero_crossing_collector::pulse_width_diff))
         {
            _estimate = d;      // vetted at the end of the sample
            return;
         }
      }
   }

   // After a long hint (a chord), the estimate stands if it is the hint or
   // a clean fraction of it, or once it repeats over one lag.
   inline void bacf_lag_tracker::vet()
   {
      if (_hint <= _long_hint)
      {
         _vetted = true;
         return;
      }
      for (int k = 1; k <= 4; ++k)
      {
         if (std::abs(float(_estimate) * float(k) / _hint - 1.0f) <= agree)
         {
            _vetted = true;
            return;
         }
      }
      auto const w = one_lag(_estimate);
      if (_now - _drop >= w + _estimate + ws)
         _vetted = 1.0f - float(score(_estimate, w)) / float(w)
            >= vet_periodicity;
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
