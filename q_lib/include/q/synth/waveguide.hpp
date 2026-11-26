/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_WAVEGUIDE_HPP_NOVEMBER_8_2026)
#define CYCFI_Q_WAVEGUIDE_HPP_NOVEMBER_8_2026

#include <q/fx/delay.hpp>
#include <q/support/frequency.hpp>
#include <q/support/duration.hpp>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // basic_waveguide: two travelling waves on delay rails between a near
   // end and a far end, a string or a body alike. The far end reflects
   // inverted. The near end takes the loss, once a trip: a lowpass, so high
   // modes die faster (a two-point average, or with loss(p) a one-pole at
   // pole p, gentler), and a gain set from a decay time. It either reflects
   // inverted too, or joins a q::waveguide_junction: read incoming(), let
   // the junction answer, and pass its outgoing wave back.
   //
   // Places (the pluck point, the pickup) are fractions of the length at
   // the lowest frequency, from the near end, fixed in samples: frequency()
   // moves the far end only. The rails are cut at the places into Delay
   // segments (q::delay by default); only the far end's changes length,
   // its fraction by a first-order allpass, which tunes without dulling.
   // stiffness(B) sets the partials sharp as a stiff string's, k f0
   // sqrt((1 + B k^2) / (1 + B)), by a chain of allpasses on the outgoing
   // wave (designed when the period moves by more than 0.2%).
   //
   // At pluck 0 the excitation drives the near end, entering the outgoing
   // wave; past 0 it is a displacement at the pluck point, half into each
   // wave. The output is the wave leaving the near end, or at the pickup,
   // one wave (rail) or both (displacement). Moving a place while it rings
   // re-cuts the rails: set them while it is silent.
   ////////////////////////////////////////////////////////////////////////////
   template <typename Delay = delay>
   class basic_waveguide
   {
   public:

      enum readout { near_end, rail, displacement };

                     basic_waveguide(
                        q::frequency lowest, float sps
                      , duration t60 = duration{2.0});

      float          operator()(float excitation);
      float          operator()(float excitation, float outgoing);
      float          incoming() const;

      void           frequency(q::frequency f);
      void           decay(duration t60);
      void           pluck(float p);
      void           pickup(float p, readout r);
      void           loss(float pole);
      void           stiffness(float b, std::size_t stages = 32);
      void           reset();

   private:

      // A stretch of one rail: n samples long, n >= 1, or none (n = 0).
      struct segment
      {
         explicit    segment(std::size_t size) : line(size) {}
         float       out() const { return line(n - 1.0f); }

         Delay       line;
         float       n = 0;
      };

      // The waves arriving at the places this sample, going left.
      struct arrivals
      {
         float       fret, whole, lb, la, l0;
      };

      arrivals       arrive(float excitation) const;
      float          disperse(float x);
      void           design_dispersion();
      float          lowpass(float l0) const;
      float          advance(arrivals const& w, float excitation, float r0);
      void           cut();
      void           update_gain();

      float          _sps;
      float          _half;         // the length at the lowest, samples
      float          _max_period;
      float          _period;       // samples per period
      float          _t60;          // samples
      float          _gain;         // per round trip
      float          _pluck = 0;    // fractions of that length
      float          _pickup = 0;
      readout        _readout = near_end;
      std::size_t    _xp = 0, _xq = 0, _a = 0, _b = 0;
      segment        _r0, _r1, _r2; // near end to a, a to b, b to far end
      segment        _l2, _l1, _l0; // and back
      float          _prev = 0;     // the lowpass's last input
      float          _lp = 0;       // and output
      float          _pole = -1;    // < 0: the two-point average
      float          _eta = 0;      // the allpass's coefficient
      float          _ap_in = 0;    // and its last input and output
      float          _ap_out = 0;
      float          _stiff = 0;    // B; its chain of allpasses:
      float          _da = 0;       //    the coefficient,
      float          _designed = 0; //    the period it was designed for,
      std::vector<float> _dx, _dy;  //    each one's last input and output
   };

   using waveguide = basic_waveguide<>;

   ////////////////////////////////////////////////////////////////////////////
   // Inlines
   ////////////////////////////////////////////////////////////////////////////
   template <typename Delay>
   inline basic_waveguide<Delay>::basic_waveguide(
      q::frequency lowest, float sps, duration t60)
    : _sps{sps}
    , _half{0.5f * sps / as_float(lowest)}
    , _max_period{sps / as_float(lowest)}
    , _period{sps / as_float(lowest)}
    , _t60{float(as_double(t60)) * sps}
    , _r0{std::size_t(_half) + 2}, _r1{std::size_t(_half) + 2}
    , _r2{std::size_t(_max_period / 2) + 4}
    , _l2{std::size_t(_max_period / 2) + 4}
    , _l1{std::size_t(_half) + 2}, _l0{std::size_t(_half) + 2}
   {
      cut();
   }

   // The left-going wave from the far end to the near end; a place with no
   // segment before it passes on what leaves the place before. What
   // reaches the near end never depends on this sample's excitation.
   template <typename Delay>
   inline typename basic_waveguide<Delay>::arrivals
   basic_waveguide<Delay>::arrive(float excitation) const
   {
      auto const half = _xp == 0? 0.0f : 0.5f * excitation;
      auto const at_a = _xp == _a? half : 0.0f;
      auto const at_b = _xp == _b && _a != _b? half : 0.0f;
      arrivals w;
      w.whole = _r2.out();
      w.fret = _eta * w.whole + _ap_in - _eta * _ap_out;
      w.lb = _l2.out() + at_b;
      w.la = (_r1.n > 0? _l1.out() : w.lb) + at_a;
      w.l0 = _r0.n > 0? _l0.out() : w.la;
      return w;
   }

   template <typename Delay>
   inline float basic_waveguide<Delay>::lowpass(float l0) const
   {
      return _pole < 0? 0.5f * (l0 + _prev)
         : (1 - _pole) * l0 + _pole * _lp;
   }

   // The wave arriving at the near end, its loss taken: what a junction
   // joining this waveguide sees.
   template <typename Delay>
   inline float basic_waveguide<Delay>::incoming() const
   {
      return _gain * lowpass(arrive(0.0f).l0);
   }

   // The stiffness's allpasses, one after another, on the outgoing wave.
   template <typename Delay>
   inline float basic_waveguide<Delay>::disperse(float x)
   {
      for (std::size_t i = 0; i != _dx.size(); ++i)
      {
         auto const y = _da * x + _dx[i] - _da * _dy[i];
         _dx[i] = x;
         _dy[i] = y;
         x = y;
      }
      return x;
   }

   // The right-going wave from the near end to the far end, then the push.
   template <typename Delay>
   inline float basic_waveguide<Delay>::advance(
      arrivals const& w, float excitation, float r0)
   {
      auto const out = _stiff > 0? disperse(r0) : r0;
      auto const half = _xp == 0? 0.0f : 0.5f * excitation;
      auto const at_a = _xp == _a? half : 0.0f;
      auto const at_b = _xp == _b && _a != _b? half : 0.0f;
      auto const ra = (_r0.n > 0? _r0.out() : out) + at_a;
      auto const rb = (_r1.n > 0? _r1.out() : ra) + at_b;

      _lp = lowpass(w.l0);
      _prev = w.l0;
      _ap_in = w.whole;
      _ap_out = w.fret;
      _r0.line.push(out);
      _r1.line.push(ra);
      _r2.line.push(rb);
      _l2.line.push(-w.fret);
      _l1.line.push(w.lb);
      _l0.line.push(w.la);

      if (_readout == near_end)
         return r0;
      auto const r = _xq == _a? ra : rb;
      auto const l = _xq == _a? w.la : w.lb;
      return _readout == rail? r : r + l;
   }

   // The near end reflects inverted.
   template <typename Delay>
   inline float basic_waveguide<Delay>::operator()(float excitation)
   {
      auto const w = arrive(excitation);
      auto const drive = _xp == 0? excitation : 0.0f;
      return advance(w, excitation, -incoming() + drive);
   }

   // The near end joins a junction, which sends `outgoing` back in.
   template <typename Delay>
   inline float basic_waveguide<Delay>::operator()(
      float excitation, float outgoing)
   {
      auto const w = arrive(excitation);
      auto const drive = _xp == 0? excitation : 0.0f;
      return advance(w, excitation, outgoing + drive);
   }

   template <typename Delay>
   inline void basic_waveguide<Delay>::frequency(q::frequency f)
   {
      _period = std::min(_sps / as_float(f), _max_period);
      cut();
   }

   template <typename Delay>
   inline void basic_waveguide<Delay>::decay(duration t60)
   {
      _t60 = std::max(float(as_double(t60)) * _sps, 1.0f);
      update_gain();
   }

   template <typename Delay>
   inline void basic_waveguide<Delay>::pluck(float p)
   {
      _pluck = std::clamp(p, 0.0f, 1.0f);
      cut();
   }

   template <typename Delay>
   inline void basic_waveguide<Delay>::pickup(float p, readout r)
   {
      _pickup = std::clamp(p, 0.0f, 1.0f);
      _readout = r;
      cut();
   }

   template <typename Delay>
   inline void basic_waveguide<Delay>::loss(float pole)
   {
      _pole = pole < 0? -1.0f : std::min(pole, 0.99f);
      cut();
   }

   template <typename Delay>
   inline void basic_waveguide<Delay>::stiffness(float b, std::size_t stages)
   {
      // A new B with the same chain keeps its state, so it may follow a
      // note while it rings.
      _stiff = std::max(b, 0.0f);
      auto const n = _stiff > 0? stages : 0;
      if (n != _dx.size())
      {
         _dx.assign(n, 0.0f);
         _dy.assign(n, 0.0f);
      }
      _designed = 0;
      cut();
   }

   namespace detail
   {
      // A first-order allpass's phase delay at w (radians per sample).
      inline float allpass_delay(float a, float w)
      {
         auto const ph = -std::atan2(std::sin(w), a + std::cos(w))
            + std::atan2(a * std::sin(w), 1 + a * std::cos(w));
         return -ph / w;
      }
   }

   // The chain's coefficient: least squares, over the partials under 5
   // kHz, between the chain's phase delay and a stiff string's, relative
   // to the fundamental's (partial k at k f0 sqrt((1 + B k^2) / (1 + B)),
   // so the loop is shorter by P (1 - sqrt((1 + B) / (1 + B k^2))) there).
   // Golden-section search; the error is one-dipped in the coefficient.
   template <typename Delay>
   inline void basic_waveguide<Delay>::design_dispersion()
   {
      auto const p = _period;
      auto const m = float(_dx.size());
      std::vector<float> w, want;
      for (int k = 1; k < 60 && k < p / 2; ++k)
      {
         auto const s = std::sqrt((1 + _stiff * k * k) / (1 + _stiff));
         auto const hz = k * s * _sps / p;
         if (hz > 5000)
            break;
         w.push_back(2 * float(pi) * hz / _sps);
         want.push_back(p / s - p);
      }
      auto err = [&](float a)
      {
         float e = 0;
         auto const d0 = detail::allpass_delay(a, w[0]);
         for (std::size_t i = 1; i < w.size(); ++i)
         {
            auto const d = m * (detail::allpass_delay(a, w[i]) - d0) - want[i];
            e += d * d;
         }
         return e;
      };
      float lo = -0.99f, hi = 0.0f;
      constexpr float g = 0.618034f;
      auto x1 = hi - g * (hi - lo), x2 = lo + g * (hi - lo);
      auto e1 = err(x1), e2 = err(x2);
      for (int i = 0; i != 40; ++i)
      {
         if (e1 < e2)
         {
            hi = x2; x2 = x1; e2 = e1;
            x1 = hi - g * (hi - lo); e1 = err(x1);
         }
         else
         {
            lo = x1; x1 = x2; e1 = e2;
            x2 = lo + g * (hi - lo); e2 = err(x2);
         }
      }
      _da = 0.5f * (lo + hi);
      _designed = p;
   }

   template <typename Delay>
   inline void basic_waveguide<Delay>::reset()
   {
      for (auto* s : {&_r0, &_r1, &_r2, &_l2, &_l1, &_l0})
         s->line.clear();
      _prev = _lp = _ap_in = _ap_out = 0;
      std::fill(_dx.begin(), _dx.end(), 0.0f);
      std::fill(_dy.begin(), _dy.end(), 0.0f);
   }

   // The segments for the places and the period: the fixed ones from the
   // near end to the farther place, and the rest of the loop, less the
   // lowpass's delay at the fundamental, split between the far end's two
   // segments; on the way out, whole samples and then the allpass's
   // fraction, kept in [0.5, 1.5) where it is stable and nearly flat in
   // delay. A place the waveguide is too short for is held where it still
   // fits.
   template <typename Delay>
   inline void basic_waveguide<Delay>::cut()
   {
      auto const reach = std::max(0.0f, (_period - 0.5f) / 2 - 1.0f);
      auto at = [&](float p)
      {
         return std::size_t(std::min(std::round(p * _half), reach));
      };
      _xp = at(_pluck);
      _xq = _readout == near_end? 0 : at(_pickup);
      _a = std::min(_xp, _xq);
      _b = std::max(_xp, _xq);
      _r0.n = _l0.n = float(_a);
      _r1.n = _l1.n = float(_b - _a);
      auto const w0 = 2 * float(pi) / _period;
      auto lag = 0.5f;
      if (_pole >= 0)
         lag = std::atan2(_pole * std::sin(w0), 1 - _pole * std::cos(w0)) / w0;
      if (_stiff > 0)
      {
         if (std::abs(_period - _designed) > 0.002f * _period)
            design_dispersion();
         lag += _dx.size() * detail::allpass_delay(_da, w0);
      }
      auto const v = std::max(_period - lag - 2.0f * _b, 3.0f);
      _l2.n = std::floor(v / 2);
      auto const r = v - _l2.n;
      _r2.n = std::max(1.0f, std::floor(r - 0.5f));
      auto const d = r - _r2.n;
      _eta = (1 - d) / (1 + d);
      update_gain();
   }

   // A sample passes the gain once per trip round the loop, once a period;
   // 60 dB over t60 samples is a gain of 10^(-3 * period / t60) per pass.
   // The average adds its own loss, which grows with frequency.
   template <typename Delay>
   inline void basic_waveguide<Delay>::update_gain()
   {
      constexpr float ln_1000 = 6.9077553f;
      _gain = std::exp(-ln_1000 * _period / _t60);
   }
}

#endif
