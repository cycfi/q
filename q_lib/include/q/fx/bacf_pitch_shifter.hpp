/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_BACF_PITCH_SHIFTER_NOVEMBER_1_2026)
#define CYCFI_Q_BACF_PITCH_SHIFTER_NOVEMBER_1_2026

#include <q/fx/biquad.hpp>
#include <q/fx/envelope.hpp>
#include <q/fx/transient_detector.hpp>
#include <q/fx/varispeed_tap.hpp>
#include <q/fx/median.hpp>
#include <q/fx/moving_average.hpp>
#include <q/pitch/bacf_lag_tracker.hpp>
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
   // splices alike. A varispeed_tap reads the input at the ratio; going up
   // its delay runs out and it splices back by a period, keeping only the
   // headroom the next fade needs; going down it falls behind and splices
   // forward by as many periods as fit, to just behind the input. The
   // period is the lag a bacf_lag_tracker holds on the input through a
   // 1 kHz low-pass, one bitstream correlation per zero crossing, smoothed
   // as the period detectors do (set outright on a new lock or a jump,
   // else a median of 3 and an average); each splice refines it to the
   // full band within 4 samples.
   //
   // At each transient (transient_detector) the read restarts at the
   // pick's own start: going down at once, going up as soon as the pick's
   // period is locked, or after `max_latency` at the latest, which is
   // what latency() reports. Until then, and through a lost lock, the last
   // lag stands in. Crossfades are half a period (2 to 8 ms), and no lag
   // is shorter than 1.25 crossfades or longer than 45 ms (an open G chord
   // repeats every 41 ms).
   //
   // The ratio may change every sample. Going down, picks come out a
   // millisecond behind the input. Unshifted, the read returns to the
   // input at the next pick, or in silence: at rest the output is the
   // input itself, with no latency, and a pull-up splices from there.
   ////////////////////////////////////////////////////////////////////////////
   class bacf_pitch_shifter
   {
   public:

                           bacf_pitch_shifter(
                              float sps
                            , duration max_latency = duration{0.020}
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
      lowpass              _lp;
      bacf_lag_tracker     _tracker;

      std::size_t          _since;
      bool                 _pending = false;
      bool                 _acquired = false;
      float                _lag = 0.0f;
      float                _lag_raw = 0.0f;
      std::vector<float>   _scratch;           // the refine's windows
      median3              _lag_median;
      exp_moving_average<8> _lag_average;
      float                _last_lag = 0.0f;
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
    , _lp{frequency{1000.0}, sps}
    , _tracker{
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
      _lp = lowpass{frequency{1000.0}, _sps};
      _tracker.reset();
      _since = _tap.max_delay();
      _pending = false;
      _acquired = false;
      _lag = 0.0f;
      _last_lag = 0.0f;
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
         transient();

      // A lock since the last transient gives the lag; without one, the
      // last stands in.
      if (_tracker(_lp(s)))
      {
         // As the period detectors do: a new lock, or a jump (another
         // multiple, a new note), is set outright; otherwise each new
         // reading goes through a median of 3 (a one-edge outlier never
         // reaches a splice) and an average.
         auto const l = _tracker.lag();
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

      // The new pick plays from its own start, a pre-roll before the
      // detection: going down at once (the read never runs out), going
      // up once its period is locked, or once `max_latency` of it exists.
      // Unshifted, the pick returns the read to the input itself,
      // crossfading over the pre-roll, as does silence.
      if (ratio == 1.0f)
      {
         if (_pending)
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
      if (_pending && (ratio < 1.0f || _acquired || _since >= _latency))
      {
         _pending = false;
         splice(float(_since + _preroll));
      }

      auto const y = _tap(s, ratio);
      if (_tap.splicing())
         return y;

      auto const delay = _tap.delay();
      if (ratio > 1.0f)
         up(delay, ratio);
      else if (ratio < 1.0f)
         down(delay);
      return y;
   }

   inline void bacf_pitch_shifter::transient()
   {
      _since = 0;
      _pending = true;
      if (_acquired)
         _last_lag = _lag;
      _acquired = false;
      _tracker.drop();
      if (_tap.splicing())          // land it, so the guard can act now
         _tap.delay(_tap.target());
   }

   // Half a period, from the lag in use
   inline std::size_t bacf_pitch_shifter::fade() const
   {
      auto const lag = _acquired ? _lag : _last_lag;
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
      auto const lag_now = _acquired ? _lag : _last_lag;
      auto const span = lag_now > 0 ? float(fade()) : float(_fade_max);
      auto const reach = span * (ratio - 1.0f) + 4.0f;
      if (_pending)
      {
         auto const keep_off = float(_since + _preroll)
            + reach + float(_fade_min) * ratio;
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
         else
            _tap.delay(reach);      // no period yet: hold, dry
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
      auto lag = _acquired ? _lag : _last_lag;
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
