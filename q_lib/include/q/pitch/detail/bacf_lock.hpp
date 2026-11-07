/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_DETAIL_BACF_LOCK_NOVEMBER_7_2026)
#define CYCFI_Q_DETAIL_BACF_LOCK_NOVEMBER_7_2026

#include <q/pitch/detail/bacf_bitstream.hpp>

#include <algorithm>
#include <cstdint>

namespace cycfi::q::detail
{
   ////////////////////////////////////////////////////////////////////////////
   // bacf_lock: the BACF follower's lock, the lag at which the signal
   // repeats, kept edge by edge. Unlocked, acquire() scores the latest
   // pulse against the like pulses before it and locks on the best under
   // the limit. Locked, track() nudges the lag to this edge's spacing from
   // the edge of its kind nearest a lag back, sweeps it two samples either
   // side and refines the best to a fraction of a sample; on a miss it
   // tries twice the lag, since a chord's common period is a multiple of
   // each note's. Two misses in a row drop the lock, a lag that cannot be
   // scored counting as one; lag() then holds the last good lag.
   //
   // The scorer gives `fits(lag)`, whether a lag can be scored, and
   // `operator()(lag)`, its mismatches on the full window's scale. Pulses
   // and edges are lists, newest first. Mismatches under `limit` hold.
   ////////////////////////////////////////////////////////////////////////////
   class bacf_lock
   {
   public:

      static constexpr int          max_misses  = 2;
      static constexpr int          sweep       = 2;
      static constexpr int          multiples   = 2;
      static constexpr int          partners    = 3;
      static constexpr std::uint32_t nudge      = 3;      // % of the lag

                           bacf_lock(
                              std::uint32_t min_lag
                            , std::uint32_t max_lag
                            , std::uint32_t window
                            , std::uint32_t limit
                           );

                           template <typename Pulses, typename Scorer>
      void                 acquire(Pulses const& pulses, Scorer const& score);

                           template <typename Edges, typename Scorer>
      void                 track(
                              Edges const& edges, std::uint32_t now
                            , Scorer const& score
                           );

      void                 drop();
      void                 reset();

      bool                 locked() const       { return _lag != 0; }
      float                lag() const;
      float                periodicity() const;

   private:

                           template <typename Scorer>
      void                 refine(std::uint32_t lag, Scorer const& score);
      void                 miss();
      void                 hold();

      std::uint32_t        _min_lag;
      std::uint32_t        _max_lag;
      std::uint32_t        _window;
      std::uint32_t        _limit;
      std::uint32_t        _lag = 0;
      float                _frac = 0.0f;
      float                _held = 0.0f;  // the last good lag()
      std::uint32_t        _mismatch = 0;
      int                  _misses = 0;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Inlines
   ////////////////////////////////////////////////////////////////////////////
   inline bacf_lock::bacf_lock(
      std::uint32_t min_lag
    , std::uint32_t max_lag
    , std::uint32_t window
    , std::uint32_t limit
   )
    : _min_lag{min_lag}
    , _max_lag{max_lag}
    , _window{window}
    , _limit{limit}
   {}

   // The lag in samples, with its fraction; unlocked, the last good one
   // (0 before any).
   inline float bacf_lock::lag() const
   {
      return _lag ? float(_lag) + _frac : _held;
   }

   // The last score's match: 1 when the bits repeat exactly, 0.5 for
   // noise.
   inline float bacf_lock::periodicity() const
   {
      return 1.0f - float(_mismatch) / float(_window);
   }

   inline void bacf_lock::drop()
   {
      _lag = 0;
      _misses = 0;
   }

   inline void bacf_lock::reset()
   {
      _lag = 0;
      _frac = 0.0f;
      _held = 0.0f;
      _mismatch = 0;
      _misses = 0;
   }

   template <typename Pulses, typename Scorer>
   inline void bacf_lock::acquire(Pulses const& pulses, Scorer const& score)
   {
      if (pulses.size() < 2)
         return;

      auto const& now = pulses[0];
      auto best = std::uint32_t(0);
      auto best_m = _limit;
      int tried = 0;
      for (std::size_t j = 1; j != pulses.size() && tried != partners; ++j)
      {
         auto const& p = pulses[j];
         auto const d = now.lead - p.lead;
         if (d > _max_lag)
            break;
         if (d < _min_lag || !similar(p, now))
            continue;
         if (!score.fits(d))
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
         hold();
      }
   }

   template <typename Edges, typename Scorer>
   inline void bacf_lock::track(
      Edges const& edges, std::uint32_t now, Scorer const& score)
   {
      // The nudge: the spacing from the edge nearest a lag back.
      auto lag = _lag;
      auto const tol = std::max<std::uint32_t>(2, _lag * nudge / 100);
      auto nearest = tol + 1;
      for (std::size_t i = 0; i != edges.size(); ++i)
      {
         auto const d = now - edges[i];
         if (d > _lag + tol)
            break;
         auto const off = d > _lag ? d - _lag : _lag - d;
         if (off < nearest)
         {
            nearest = off;
            lag = d;
         }
      }

      if (!score.fits(lag - sweep) || !score.fits(lag + sweep))
         miss();
      else
         refine(lag, score);
      hold();
   }

   // Sweep `lag`, refine the best to a fraction, try the multiples if it
   // misses, then keep or miss.
   template <typename Scorer>
   inline void bacf_lock::refine(std::uint32_t lag, Scorer const& score)
   {
      std::uint32_t m[2 * sweep + 1];
      int best = 0;
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
         if (!score.fits(at - reach) || !score.fits(at + reach))
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
      else
      {
         miss();
      }
   }

   inline void bacf_lock::miss()
   {
      if (++_misses >= max_misses)
      {
         _lag = 0;
         _misses = 0;
      }
   }

   inline void bacf_lock::hold()
   {
      if (_lag)
         _held = float(_lag) + _frac;
   }
}

#endif
