/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_SPECTRAL_LEARNER_NOVEMBER_2_2026)
#define CYCFI_Q_SPECTRAL_LEARNER_NOVEMBER_2_2026

#include <q/fft/fft.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <span>
#include <vector>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // spectral_learner: learns a sound's long-term spectrum, and the
   // spectral envelope through it, from its playing. It runs off the audio
   // thread (a worker, or an MCU's low-priority loop) and allocates only
   // at construction.
   //
   // Frames of 50 ms, every 25 ms, above -50 dBFS, each at unit power so
   // loud notes do not dominate, are averaged: what belongs to one note
   // washes out and the shared response stays. The average is cumulative,
   // each frame weighted 1/N, so the longer one plays the steadier it
   // gets; past `max_frames` (10 minutes) it follows slowly. learn()
   // starts over; freeze() stops learning. coverage() is the part of the
   // band, 80 Hz to 8 kHz, where partials have been seen: a learning phase
   // can end once it nears 1. drift() is how far the last few seconds sit
   // from what was learned, in dB: a cue to learn again.
   //
   // save() and restore() carry what has been learned, state, across a
   // session: the average, its frame count and the cells seen. A state
   // saved at another sample rate is mapped onto this one's bins by
   // frequency. Learning goes on from it as if it had not stopped.
   //
   // spectral_envelope() draws the spectral envelope through the average's
   // peaks (an instrument's average is a line spectrum, seen only at its
   // harmonics), held flat beyond them and smoothed over a third of an
   // octave, in dB on the spectrum's bins.
   ////////////////////////////////////////////////////////////////////////////
   class spectral_learner
   {
   public:

      struct state
      {
         float                sps = 0;
         double               frames = 0;
         std::vector<float>   spectrum;      // the average, a value a bin
         std::vector<bool>    seen;          // the cells, 80 Hz up
      };

      explicit             spectral_learner(float sps);

      void                 operator()(std::span<float const> block);

      void                 learn();
      void                 save(state& s) const;
      bool                 restore(state const& s);
      void                 freeze(bool f);
      bool                 frozen() const;
      double               frames() const;
      float                coverage() const;
      float                drift() const;

      std::size_t          bins() const;
      float                sps() const;
      std::span<float const> spectrum() const;
      void                 spectral_envelope(std::span<float> db) const;

      static constexpr float        gate_db = -50.0f;
      static constexpr double       max_frames = 24000;     // 10 minutes
      static constexpr std::size_t  cells = 40;             // 80 Hz-8 kHz

   private:

      using spectrum_type = std::vector<float>;

      void                 frame();
      void                 mark();
      void                 draw(std::span<float const> p
                            , std::span<float> env) const;

      float                _sps;
      std::size_t          _len;
      std::size_t          _hop;
      std::size_t          _n;
      std::vector<float>   _window;
      std::vector<float>   _buf;
      std::size_t          _fill = 0;
      double               _count = 0;
      double               _recent_count = 0;
      bool                 _frozen = false;
      std::vector<bool>    _seen;
      spectrum_type        _mean;
      spectrum_type        _recent;

      // Scratch, for frame(), draw() and drift().
      basic_rfft<float>    _fft;
      std::vector<float>   _work;
      spectrum_type        _power;
      mutable spectrum_type _db;
      mutable spectrum_type _env1;
      mutable spectrum_type _env2;
      mutable std::vector<double> _prefix;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Inlines
   ////////////////////////////////////////////////////////////////////////////
   namespace detail
   {
      inline std::size_t pow2_at_least(std::size_t n)
      {
         std::size_t p = 2;
         while (p < n)
            p *= 2;
         return p;
      }
   }

   inline spectral_learner::spectral_learner(float sps)
    : _sps{sps}
    , _len{std::size_t(0.05 * sps)}
    , _hop{_len / 2}
    , _n{detail::pow2_at_least(_len)}
    , _window(_len)
    , _buf(_len, 0.0f)
    , _seen(cells, false)
    , _mean(_n / 2 + 1, 0.0f)
    , _recent(_n / 2 + 1, 0.0f)
    , _fft{_n}
    , _work(_n + 2, 0.0f)
    , _power(_n / 2 + 1)
    , _db(_n / 2 + 1)
    , _env1(_n / 2 + 1)
    , _env2(_n / 2 + 1)
    , _prefix(_n / 2 + 2)
   {
      auto const pi = std::numbers::pi;
      for (std::size_t i = 0; i != _len; ++i)
         _window[i] = float(0.5 - 0.5 * std::cos(2 * pi * (i + 0.5) / _len));
   }

   inline void spectral_learner::operator()(std::span<float const> block)
   {
      for (auto s : block)
      {
         _buf[_fill++] = s;
         if (_fill == _len)
         {
            frame();
            std::copy(_buf.begin() + _hop, _buf.end(), _buf.begin());
            _fill = _len - _hop;
         }
      }
   }

   // A frame above the gate, at unit power, into the cumulative average
   // and the recent one (about 3 seconds).
   inline void spectral_learner::frame()
   {
      double energy = 0;
      for (auto s : _buf)
         energy += double(s) * s;
      auto const floor = std::pow(10.0, gate_db / 10.0) * double(_len);
      if (_frozen || energy < floor)
         return;

      std::fill(_work.begin(), _work.end(), 0.0f);
      for (std::size_t i = 0; i != _len; ++i)
         _work[i] = _buf[i] * _window[i];
      _fft.forward(_work.data());

      double total = 0;
      for (std::size_t b = 0; b != _power.size(); ++b)
      {
         auto const re = _work[2 * b];
         auto const im = _work[2 * b + 1];
         _power[b] = re * re + im * im;
         total += _power[b];
      }
      if (!(total > 0))
         return;

      _count = std::min(_count + 1, max_frames);
      _recent_count += 1;
      auto const w = 1.0 / _count;
      auto const wr = std::max(
         1.0 / _recent_count, double(_hop) / (3.0 * _sps));
      for (std::size_t b = 0; b != _power.size(); ++b)
      {
         _power[b] = float(_power[b] / total);
         _mean[b] += float(w * (_power[b] - _mean[b]));
         _recent[b] += float(wr * (_power[b] - _recent[b]));
      }
      mark();
   }

   // Partials seen: local maxima 3 dB over their neighbourhood and at most
   // 40 dB under the frame's strongest, in sixth-octave cells from 80 Hz.
   inline void spectral_learner::mark()
   {
      auto const& p = _power;
      auto const top = *std::max_element(p.begin(), p.end());
      auto const bin_hz = _sps / float(_n);
      for (std::size_t b = 4; b + 4 < p.size(); ++b)
      {
         if (p[b] < top * 1e-4f || p[b] < p[b - 1] || p[b] < p[b + 1])
            continue;
         auto const at = p.begin() + b;
         auto const left = *std::min_element(at - 4, at);
         auto const right = *std::min_element(at + 1, at + 5);
         if (p[b] < 2.0f * std::max(left, right))
            continue;
         auto const cell = 6.0f * std::log2(float(b) * bin_hz / 80.0f);
         if (cell >= 0.0f && cell < float(cells))
            _seen[std::size_t(cell)] = true;
      }
   }

   inline void spectral_learner::learn()
   {
      std::fill(_mean.begin(), _mean.end(), 0.0f);
      std::fill(_recent.begin(), _recent.end(), 0.0f);
      std::fill(_seen.begin(), _seen.end(), false);
      _count = 0;
      _recent_count = 0;
      _fill = 0;
   }

   inline void spectral_learner::save(state& s) const
   {
      s.sps = _sps;
      s.frames = _count;
      s.spectrum.assign(_mean.begin(), _mean.end());
      s.seen.assign(_seen.begin(), _seen.end());
   }

   // At the same sample rate the average is taken as it is. At another,
   // each bin reads the saved average at its own frequency, interpolated,
   // and the result is brought back to the saved total power. The recent
   // average starts over, so drift() reads 0 until it fills again.
   inline bool spectral_learner::restore(state const& s)
   {
      auto const from = s.spectrum.size();
      if (!(s.sps > 0) || from < 2 || s.seen.size() != cells || !(s.frames > 0))
         return false;

      if (s.sps == _sps && from == _mean.size())
      {
         std::copy(s.spectrum.begin(), s.spectrum.end(), _mean.begin());
      }
      else
      {
         auto const from_hz = s.sps / (2.0 * double(from - 1));
         auto const bin_hz = double(_sps) / double(_n);
         double saved = 0, mapped = 0;
         for (auto p : s.spectrum)
            saved += p;
         for (std::size_t b = 0; b != _mean.size(); ++b)
         {
            auto const i = double(b) * bin_hz / from_hz;
            auto const j = std::size_t(i);
            float v = 0;
            if (j + 1 < from)
               v = s.spectrum[j] + float(i - double(j))
                  * (s.spectrum[j + 1] - s.spectrum[j]);
            else if (j + 1 == from)
               v = s.spectrum[j];
            _mean[b] = v;
            mapped += v;
         }
         if (mapped > 0)
            for (auto& v : _mean)
               v = float(v * saved / mapped);
      }

      std::copy(s.seen.begin(), s.seen.end(), _seen.begin());
      _count = std::min(s.frames, max_frames);
      std::fill(_recent.begin(), _recent.end(), 0.0f);
      _recent_count = 0;
      return true;
   }

   inline void spectral_learner::freeze(bool f)
   {
      _frozen = f;
   }

   inline bool spectral_learner::frozen() const
   {
      return _frozen;
   }

   inline double spectral_learner::frames() const
   {
      return _count;
   }

   inline float spectral_learner::coverage() const
   {
      auto const n = std::count(_seen.begin(), _seen.end(), true);
      return float(n) / float(cells);
   }

   // The rms difference in dB, 100 Hz to 8 kHz, between the learned and
   // the recent spectral envelopes, with their mean offset removed.
   inline float spectral_learner::drift() const
   {
      if (_count == 0 || _recent_count == 0)
         return 0.0f;
      draw(_mean, _env1);
      draw(_recent, _env2);

      auto const bin_hz = _sps / float(_n);
      auto const lo = std::size_t(100.0f / bin_hz);
      auto const hi = std::min(std::size_t(8000.0f / bin_hz), _env1.size());
      double sum = 0, sum2 = 0;
      for (std::size_t b = lo; b != hi; ++b)
      {
         auto const d = double(_env2[b] - _env1[b]);
         sum += d;
         sum2 += d * d;
      }
      auto const n = double(hi - lo);
      auto const mean = sum / n;
      return float(std::sqrt(std::max(sum2 / n - mean * mean, 0.0)));
   }

   inline std::size_t spectral_learner::bins() const
   {
      return _mean.size();
   }

   inline float spectral_learner::sps() const
   {
      return _sps;
   }

   inline std::span<float const> spectral_learner::spectrum() const
   {
      return _mean;
   }

   inline void spectral_learner::spectral_envelope(std::span<float> db) const
   {
      draw(_mean, db);
   }

   // The spectral envelope, in dB, through the peaks of a power spectrum:
   // local maxima 3 dB over their neighbourhood, within 70 dB of the
   // strongest, above 40 Hz; linear in dB between them, flat beyond, then
   // averaged over a third of an octave.
   inline void spectral_learner::draw(
      std::span<float const> p, std::span<float> env) const
   {
      auto const bins = p.size();
      auto& db = _db;
      for (std::size_t b = 0; b != bins; ++b)
         db[b] = 10.0f * std::log10(p[b] + 1e-20f);
      auto const top = *std::max_element(db.begin(), db.end());
      auto const first = std::size_t(40.0f * float(_n) / _sps) + 1;

      std::size_t prev = bins;      // the last peak, none yet
      auto const start = std::max<std::size_t>(first, 8);
      for (std::size_t b = start; b + 8 < bins; ++b)
      {
         if (db[b] < top - 70.0f || db[b] < db[b - 1] || db[b] < db[b + 1])
            continue;
         auto const at = db.begin() + b;
         auto const left = *std::min_element(at - 8, at);
         auto const right = *std::min_element(at + 1, at + 9);
         if (db[b] - std::max(left, right) < 3.0f)
            continue;

         if (prev == bins)
         {
            for (std::size_t i = 0; i <= b; ++i)
               env[i] = db[b];
         }
         else
         {
            for (std::size_t i = prev + 1; i <= b; ++i)
            {
               auto const t = float(i - prev) / float(b - prev);
               env[i] = db[prev] + t * (db[b] - db[prev]);
            }
         }
         prev = b;
      }
      if (prev == bins)
      {
         std::fill(env.begin(), env.end(), top);
         return;
      }
      for (std::size_t i = prev + 1; i != bins; ++i)
         env[i] = db[prev];

      // A third of an octave, as a running mean on prefix sums.
      auto& prefix = _prefix;
      prefix[0] = 0;
      for (std::size_t b = 0; b != bins; ++b)
         prefix[b + 1] = prefix[b] + env[b];
      auto const lo = std::pow(2.0, -1.0 / 6.0);
      auto const hi = std::pow(2.0, 1.0 / 6.0);
      for (std::size_t b = 1; b != bins; ++b)
      {
         auto const a = std::size_t(std::ceil(double(b) * lo));
         auto const z = std::min(bins - 1, std::size_t(double(b) * hi));
         env[b] = float((prefix[z + 1] - prefix[a]) / double(z + 1 - a));
      }
   }
}

#endif
