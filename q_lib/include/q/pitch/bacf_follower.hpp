/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_BACF_FOLLOWER_NOVEMBER_2_2026)
#define CYCFI_Q_BACF_FOLLOWER_NOVEMBER_2_2026

#include <q/pitch/detail/bacf_bitstream.hpp>
#include <q/pitch/detail/bacf_lock.hpp>
#include <q/pitch/detail/bacf_slice.hpp>
#include <q/pitch/detail/bacf_provisional.hpp>
#include <q/support/base.hpp>
#include <q/support/decibel.hpp>
#include <q/support/duration.hpp>
#include <q/support/literals.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // bacf_follower: the lag at which a signal repeats, tracked with
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
   // similar one. drop's `hint` is the last good lag: after a long one
   // (over 13 ms), an estimate that is not that lag or a clean fraction of
   // it must first repeat over one lag.
   //
   // The lag is a period or a multiple of one: octaves are not settled.
   // The work is per crossing, flat. Low-pass the input first (about 1 kHz
   // for a guitar): fewer crossings, cleaner lags. Allocates at
   // construction only. The parts are in q/pitch/detail.
   ////////////////////////////////////////////////////////////////////////////
   class bacf_follower
   {
      using bitstream = detail::bacf_bitstream;
      using slice = detail::bacf_slice;
      using lock = detail::bacf_lock;
      using provisional_lag = detail::bacf_provisional;

   public:

      static constexpr float        threshold      = 0.15f;  // of the window
      static constexpr int          max_misses     = lock::max_misses;
      static constexpr int          sweep          = lock::sweep;
      static constexpr int          multiples      = lock::multiples;
      static constexpr int          partners       = lock::partners;
      static constexpr std::size_t  edge_capacity  = 256;

                           bacf_follower(
                              duration min_lag
                            , duration max_lag
                            , float sps
                            , decibel hysteresis = -45_dB
                            , duration window = 11.6_ms
                           );

      float                operator()(float s);
      bool                 locked() const       { return _lock.locked(); }
      float                lag() const          { return _lock.lag(); }
      float                periodicity() const;
      float                provisional() const  { return _prov.value(); }
      std::size_t          window() const       { return _window; }
      void                 drop(float hint = 0.0f);
      void                 reset();

   private:

      static constexpr std::size_t  ws             = bitstream::word_size;
      static constexpr float        min_window     = 0.002f; // seconds
      static constexpr float        long_hint      = 0.013f; // seconds

      struct scorer;

      static std::size_t   history(
                              std::uint32_t window, std::uint32_t max_lag
                            , std::uint32_t min_window
                           );
      void                 edge(bitstream::edge_kind k);
      float                one_lag_periodicity(std::uint32_t lag) const;

      std::uint32_t        _min_lag;
      std::uint32_t        _max_lag;
      std::uint32_t        _window;       // bits, whole words
      bitstream            _bits;
      slice                _slice;
      lock                 _lock;
      provisional_lag      _prov;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Scores at a lag for the lock: over the slice's window at that lag,
   // scaled to the full window.
   ////////////////////////////////////////////////////////////////////////////
   struct bacf_follower::scorer
   {
      bool                 fits(std::uint32_t lag) const;
      std::uint32_t        operator()(std::uint32_t lag) const;

      bacf_follower const& _t;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Inlines
   ////////////////////////////////////////////////////////////////////////////
   inline bacf_follower::bacf_follower(
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
    , _bits{
         lin_float(hysteresis)
       , history(_window, _max_lag, std::uint32_t(min_window * sps))
       , edge_capacity
      }
    , _slice{_window, std::uint32_t(min_window * sps)}
    , _lock{_min_lag, _max_lag, _window, std::uint32_t(threshold * _window)}
    , _prov{_min_lag, _max_lag, long_hint * sps}
   {}

   // The bits a score can reach back: its window (the full one, or one lag
   // of the slice at the longest lag, whichever is longer), the lag, and a
   // word for the window's alignment.
   inline std::size_t bacf_follower::history(
      std::uint32_t window, std::uint32_t max_lag, std::uint32_t min_window)
   {
      auto const one_lag =
         (std::max(max_lag, min_window) + ws - 1) / ws * ws;
      return std::max<std::size_t>(window, one_lag) + max_lag + ws;
   }

   // One sample; the lag while locked, 0 while not.
   inline float bacf_follower::operator()(float s)
   {
      _bits(s, [this](bitstream::edge_kind k) { edge(k); });
      if (_prov.pending())
      {
         _prov.vet(_slice, _bits.now()
          , [this](std::uint32_t lag) { return one_lag_periodicity(lag); });
      }
      return _lock.locked() ? _lock.lag() : 0.0f;
   }

   inline float bacf_follower::periodicity() const
   {
      return _lock.periodicity();
   }

   // Drop the lock, at a new note: lag() holds until a new lock. `hint`
   // is the last good lag, in samples (0: none).
   inline void bacf_follower::drop(float hint)
   {
      _lock.drop();
      _slice.drop(_bits.now(), hint);
      _prov.drop();
   }

   inline void bacf_follower::reset()
   {
      _bits.reset();
      _slice.reset();
      _lock.reset();
      _prov.reset();
   }

   // At an edge: a pulse that ends counts toward the slice's peak; locked,
   // the lock tracks; unlocked, the provisional estimate and the lock's
   // acquisition both use the latest pulse.
   inline void bacf_follower::edge(bitstream::edge_kind k)
   {
      auto const pulses = _bits.pulses();
      if (k == bitstream::trailing)
         _slice.observe(pulses[0]);

      if (_lock.locked())
      {
         _lock.track(_bits.edges(k), _bits.now(), scorer{*this});
      }
      else if (pulses.size() >= 2)
      {
         if (!_prov.estimated())
            _prov.estimate(pulses, _slice);
         _lock.acquire(pulses, scorer{*this});
      }
   }

   // The match at `lag` over one lag of the slice, for the estimate's vet.
   inline float bacf_follower::one_lag_periodicity(
      std::uint32_t lag) const
   {
      auto const w = _slice.one_lag(lag);
      return 1.0f - float(_bits.mismatch(lag, w)) / float(w);
   }

   // In range, and history enough for the slice's window at `lag`.
   inline bool bacf_follower::scorer::fits(std::uint32_t lag) const
   {
      auto const now = _t._bits.now();
      return lag >= _t._min_lag && lag <= _t._max_lag
         && _t._bits.has(lag, _t._slice.window(lag, now))
         && _t._slice.ready(lag, now);
   }

   inline std::uint32_t bacf_follower::scorer::operator()(
      std::uint32_t lag) const
   {
      auto const w = _t._slice.window(lag, _t._bits.now());
      auto const m = _t._bits.mismatch(lag, w);
      return w == _t._window ? m
         : std::uint32_t(std::lround(double(m) * _t._window / w));
   }
}

#endif
