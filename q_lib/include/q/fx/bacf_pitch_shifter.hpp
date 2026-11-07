/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_BACF_PITCH_SHIFTER_NOVEMBER_1_2026)
#define CYCFI_Q_BACF_PITCH_SHIFTER_NOVEMBER_1_2026

#include <q/fx/envelope.hpp>
#include <q/fx/transient_detector.hpp>
#include <q/fx/varispeed_tap.hpp>
#include <q/fx/median.hpp>
#include <q/fx/moving_average.hpp>
#include <q/pitch/bacf_follower.hpp>
#include <q/support/literals.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // bacf_pitch_shifter: a time-domain pitch shifter on a tracked period,
   // with no need to resolve the note: a period or a multiple of one
   // splices alike. A varispeed_tap reads at the ratio; going up its delay
   // runs out and it splices back by a period, keeping only the headroom
   // the next fade needs; going down it falls behind and splices forward
   // by as many periods as fit. The whole machinery (the tap, its splices
   // and the bacf_follower that gives the period) runs on the input
   // through a 1 kHz one-pole low-pass, and the output is that low-pass
   // undone, its exact inverse at 1 kHz times the ratio (the shift moves
   // the low-pass's shape up with the pitch). The lag is smoothed as the
   // period detectors do; each splice refines it within 4 samples.
   //
   // At each transient the read restarts at the pick's own start: going
   // down at once, going up on the pick's first estimate of its period
   // (the follower's provisional()), its lock, or after `max_latency` at
   // the latest, which is what latency() reports. Until then the last lag
   // stands in. A splice in flight always completes. Crossfades are half
   // a period (2 to 8 ms); no lag is under 1.25 crossfades or over 45 ms.
   //
   // The ratio may change every sample. Going down, picks come out a
   // millisecond behind the input. At rest the output is the input
   // itself, bit for bit, and a pull-up splices from there.
   ////////////////////////////////////////////////////////////////////////////
   class bacf_pitch_shifter
   {
   public:

                           bacf_pitch_shifter(
                              float sps
                            , duration max_latency = duration{0.010}
                           );

      float                operator()(float s, float ratio);
      std::size_t          latency() const;
      void                 reset();

   private:

      void                 transient();
      void                 up(float delay, float ratio);
      void                 down(float delay);
      void                 splice(float to);
      float                refine(float from, float to);
      std::size_t          fade() const;
      std::size_t          lag_lo() const;

      static constexpr float        lag_floor      = 1.25f; // crossfades
      static constexpr float        silence_floor  = 0.001f;
      static constexpr float        lp_corner      = 1000.0f; // Hz
      // Below a semitone up the read is near the input: with no period
      // yet it holds at the headroom (the input's own pitch, the smaller
      // error), and a splice in flight at a pick turns around. Above,
      // the last lag stands in and the splice completes.
      static constexpr float        hold_ratio     = 1.0594631f;

      using envelope_follower = peak_envelope_follower;

      float                _sps;
      std::size_t          _latency;
      std::size_t          _preroll;
      std::size_t          _window;
      std::size_t          _lag_min;
      std::size_t          _lag_max;
      std::size_t          _fade_min;
      std::size_t          _fade_max;

      varispeed_tap        _tap;
      envelope_follower    _env;
      transient_detector   _transient;
      float                _lp_g;              // the one-pole's gain
      float                _lp = 0.0f;         // its state
      float                _inv_ratio = 0.0f;  // the inverse's, at its ratio
      float                _inv_g = 1.0f;
      float                _inv = 0.0f;
      bacf_follower     _follower;

      std::size_t          _since;
      bool                 _pending = false;
      bool                 _acquired = false;
      float                _lag = 0.0f;
      float                _lag_raw = 0.0f;
      float                _prov = 0.0f;       // the pick's first estimate
      std::vector<float>   _scratch;           // the refine's windows
      median3              _lag_median;
      exp_moving_average<8> _lag_average;
      float                _last_lag = 0.0f;
      bool                 _picked = false;    // a pick since the start
   };

   ////////////////////////////////////////////////////////////////////////////
   // Inlines
   ////////////////////////////////////////////////////////////////////////////
   inline bacf_pitch_shifter::bacf_pitch_shifter(
      float sps, duration max_latency)
    : _sps{sps}
    , _latency{std::size_t(as_double(max_latency) * sps)}
    , _preroll{std::size_t(0.001 * sps)}
    , _window{std::size_t(
         (as_double(max_latency) > 0 ? 0.008 : 0.004) * sps)}
    , _lag_min{std::size_t(0.002 * sps)}
    , _lag_max{std::size_t(0.045 * sps)}
    , _fade_min{std::size_t(
         0.002 * sps)}
    , _fade_max{std::size_t(0.008 * sps)}
    , _tap{_latency + 4 * _lag_max + _fade_max + 4 * _window}
    , _env{duration{0.1}, sps}
    , _transient{sps}
    , _lp_g{float(1.0 - std::exp(-2_pi * lp_corner / sps))}
    , _follower{
         duration{double(lag_lo()) / sps}
       , duration{double(_lag_max) / sps}
       , sps
      }
    , _since{_tap.max_delay()}
    , _scratch(2 * _fade_max + 8)
   {}

   inline std::size_t bacf_pitch_shifter::latency() const
   {
      return _latency + _preroll;
   }

   inline void bacf_pitch_shifter::reset()
   {
      _tap.reset();
      _env = envelope_follower{duration{0.1}, _sps};
      _transient = transient_detector{_sps};
      _lp = 0.0f;
      _inv_ratio = 0.0f;
      _inv = 0.0f;
      _follower.reset();
      _since = _tap.max_delay();
      _pending = false;
      _acquired = false;
      _lag = 0.0f;
      _prov = 0.0f;
      _last_lag = 0.0f;
      _picked = false;
   }

   // The shortest lag: lag_floor crossfades, so a short period is spliced
   // two or more at a time.
   inline std::size_t bacf_pitch_shifter::lag_lo() const
   {
      return std::max(_lag_min, std::size_t(lag_floor * float(_fade_min)));
   }

   inline float bacf_pitch_shifter::operator()(float s, float ratio)
   {
      auto const env = _env(s);
      ++_since;
      if (_transient(s))
      {
         transient();
         // A splice in flight is never cut, but near unity its outgoing
         // read may be at the pick's onset already: the splice turns
         // around, so the onset plays once, where the read is.
         if (ratio != 1.0f && ratio < hold_ratio && _tap.splicing()
            && _tap.delay() <= float(_preroll + fade()))
            _tap.reverse();
         // A read within a fade of the pick's start plays the pick's onset
         // before a jump could cross away from it, so a restart would play
         // it twice: the pick plays where the read is (or lands, with a
         // splice in flight).
         if (ratio != 1.0f && _tap.target() <= float(_preroll + fade()))
            _pending = false;
      }

      // A lock since the last transient gives the lag; without one, the
      // pick's first estimate or the last lag stands in.
      _lp += _lp_g * (s - _lp);
      if (auto const l = _follower(_lp))
      {
         // As the period detectors do: a new lock, or a jump (another
         // multiple, a new note), is set outright; otherwise each new
         // reading goes through a median of 3 (a one-edge outlier never
         // reaches a splice) and an average.
         if (!_acquired || std::abs(l - _lag) > 0.1f * _lag)
         {
            _lag_median = median3{l};
            _lag_average = exp_moving_average<8>{l};
            _lag = l;
         }
         else if (l != _lag_raw)
         {
            _lag = _lag_average(_lag_median(l));
         }
         _lag_raw = l;
         _acquired = true;
      }
      if (!_acquired && _prov == 0.0f)
         _prov = _follower.provisional();

      // The new pick plays from its own start, a pre-roll before the
      // detection: going down at once (the read never runs out), going
      // up once it has an estimate or a lock of its period, or once
      // `max_latency` of it exists. Unshifted, the pick returns the read to
      // the input itself, crossfading over the pre-roll, as does silence.
      // A splice in flight is never cut: these wait for it.
      if (ratio == 1.0f)
      {
         if (_pending && !_tap.splicing())
         {
            _pending = false;
            if (_tap.target() > 0.0f)
               _tap.splice(0.0f, _preroll);
         }
         else if (env < silence_floor && _tap.target() > 0.0f
            && !_tap.splicing())
         {
            _tap.splice(0.0f, _fade_min);
         }
      }
      if (_pending && !_tap.splicing()
         && (ratio < 1.0f || _acquired || _prov > 0.0f || _since >= _latency))
      {
         _pending = false;
         auto const to = float(_since + _preroll);
         if (ratio > 1.0f)
         {
            // Both reads move at the ratio, so the old one closes on the
            // pick's start by `ratio` a sample: the fade ends before it.
            auto const most = (_tap.delay() - to - 4.0f) / ratio;
            _tap.splice(to, std::size_t(
               std::clamp(most, 0.0005f * _sps, float(fade()))));
         }
         else
         {
            splice(to);
         }
      }

      // A restart that is due goes first, at the next sample: no new splice
      // may hold it off.
      bool const due = _pending
         && (ratio < 1.0f || _acquired || _prov > 0.0f || _since >= _latency);
      auto const y = _tap(_lp, ratio);
      if (!_tap.splicing() && !due)
      {
         auto const delay = _tap.delay();
         if (ratio > 1.0f)
            up(delay, ratio);
         else if (ratio < 1.0f)
            down(delay);
      }

      // The low-pass undone, at its corner times the ratio. At rest the
      // read is the input's own sample, so the input itself goes out.
      if (ratio != _inv_ratio)
      {
         _inv_ratio = ratio;
         _inv_g = 1.0f - fast_exp(-2_pi * lp_corner * ratio / _sps);
      }
      auto const out = _inv + (y - _inv) / _inv_g;
      _inv = y;
      if (ratio == 1.0f && !_tap.splicing() && _tap.delay() == 0.0f)
         return s;
      return out;
   }

   inline void bacf_pitch_shifter::transient()
   {
      _since = 0;
      _pending = true;
      // A lock before the first pick (hum, a scrape) is no note's lag.
      if (_acquired && _picked)
         _last_lag = _lag;
      _picked = true;
      _acquired = false;
      _prov = 0.0f;
      _follower.drop(_last_lag);
   }

   // Half a period, from the lag in use
   inline std::size_t bacf_pitch_shifter::fade() const
   {
      auto const lag = _acquired ? _lag : _prov > 0.0f ? _prov : _last_lag;
      return lag > 0
         ? std::clamp(std::size_t(0.5f * lag), _fade_min, _fade_max)
         : _fade_min;
   }

   inline void bacf_pitch_shifter::splice(float to)
   {
      _tap.splice(to, fade());
   }

   // Going up, the read runs toward delay 0: back a period before it gets
   // there. While a pick waits for its turn, the old note keeps playing
   // from before it, back by as many of its periods as that takes.
   inline void bacf_pitch_shifter::up(float delay, float ratio)
   {
      // The headroom the next fade-out needs: it runs `fade` outputs at
      // the ratio, gaining fade * (ratio - 1) on the input.
      auto const lag_now = _acquired ? _lag : _prov > 0.0f ? _prov : _last_lag;
      auto const span = lag_now > 0 ? float(fade()) : float(_fade_max);
      auto const reach = span * (ratio - 1.0f) + 4.0f;
      if (_pending)
      {
         // Behind the pick's start by the restart's whole fade
         auto const keep_off = float(_since + _preroll)
            + reach + float(fade()) * ratio;
         if (delay < keep_off)
         {
            auto const lag = _last_lag > 0 ? _last_lag : float(_lag_max);
            auto const k = std::max(1.0f, std::ceil(
               (keep_off + float(_fade_min) * (ratio + 1.0f) - delay)
               / lag));
            splice(refine(delay, delay + k * lag));
         }
         return;
      }

      if (delay < reach)
      {
         if (_acquired)
            splice(refine(delay, delay + _lag));
         else if (_prov > 0.0f)
            splice(refine(delay, delay + _prov));
         else if (_last_lag > 0.0f && ratio >= hold_ratio)
            splice(refine(delay, delay + _last_lag));
         else
            _tap.delay(reach);      // hold, at the input's pitch
      }
   }

   // The splice target within 4 samples of `to` whose history best matches
   // the read's: the full band, over a fade's length behind each.
   inline float bacf_pitch_shifter::refine(float from, float to)
   {
      auto const n = _fade_max;
      auto const top = float(_tap.max_delay()) - float(n) - 8.0f;
      if (to + 4.0f > top || from + float(n) > top || to < 4.0f)
         return to;

      // The read's window and the 9 candidates' span, gathered once
      auto const f0 = std::size_t(from);
      auto const lo = std::size_t(to) - 4;
      auto* a = _scratch.data();
      auto* b = a + n;
      float aa = 0, bb = 0;
      for (std::size_t i = 0; i != n; ++i)
      {
         a[i] = _tap[f0 + i];
         aa += a[i] * a[i];
      }
      for (std::size_t i = 0; i != n + 8; ++i)
         b[i] = _tap[lo + i];
      for (std::size_t i = 0; i != n; ++i)
         bb += b[i] * b[i];

      // Each candidate's energy slides along from the one before
      float score[9];
      for (std::size_t k = 0; k != 9; ++k)
      {
         float ab = 0;
         for (std::size_t i = 0; i != n; ++i)
            ab += a[i] * b[k + i];
         score[k] = (aa > 0 && bb > 0) ? ab / std::sqrt(aa * bb) : 0.0f;
         if (k != 8)
            bb += b[k + n] * b[k + n] - b[k] * b[k];
      }

      std::size_t best = 4;
      for (std::size_t k = 0; k != 9; ++k)
         if (score[k] > score[best])
            best = k;
      auto frac = 0.0f;
      if (best > 0 && best < 8)
      {
         auto const l = score[best - 1], c = score[best], r = score[best + 1];
         auto const d = l - 2 * c + r;
         if (d < 0)
            frac = 0.5f * (l - r) / d;
      }
      return float(lo + best) + frac + (from - float(f0));
   }

   // Going down, the read falls behind: forward by as many periods as fit
   // once a whole one does, toward the input. The fade-in reads from near
   // the input and falls behind it at the ratio, so it never runs out.
   inline void bacf_pitch_shifter::down(float delay)
   {
      auto lag = _acquired ? _lag : _prov > 0.0f ? _prov : _last_lag;
      if (lag <= 0)
         lag = float(_lag_max);
      auto const margin = 4.0f;
      if (delay < lag + margin)
         return;
      auto const to = delay - std::floor((delay - margin) / lag) * lag;
      splice(refine(delay, to));
   }
}

#endif
