/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_LADDER_HPP_JUNE_20_2026)
#define CYCFI_Q_LADDER_HPP_JUNE_20_2026

#include <q/support/base.hpp>
#include <q/support/frequency.hpp>
#include <cmath>
#include <cstddef>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // basic_ladder: an N-pole lowpass ladder filter with resonance, the
   // classic "fat" synthesizer voice at the usual four poles (24 dB/oct).
   //
   // N topology-preserving one-pole lowpasses in series, wrapped in a global
   // resonance feedback loop. When the cell is linear the instantaneous
   // (zero-delay) feedback is resolved analytically, so the filter is exact
   // and stays stable and zipper-free under per-sample cutoff modulation, the
   // same virtue as q::svf. See:
   //
   //    Vadim Zavalishin, "The Art of VA Filter Design".
   //    Will Pirkle, "Designing Software Synthesizer Plug-Ins in C++".
   //
   // Parametrization is exact:
   //
   //    cutoff      g = tan(pi * fc / fs)   bilinear prewarp; fc is the corner
   //                                        of each one-pole stage.
   //    resonance   r in [0, 1]             feedback gain, scaled so that
   //                                        r = 1 is self-oscillation for the
   //                                        pole count (k = 4 at four poles).
   //                                        As r rises the passband thins, the
   //                                        characteristic ladder behavior.
   //
   // operator() returns the lowpass, 6N dB an octave past the corner.
   //
   // What a real filter of this shape does to the signal is the cell's, and
   // it is the one thing that differs between them: a transistor ladder
   // saturates symmetrically, an OTA cell asymmetrically. The Cell supplies
   //
   //    bool  linear() const;                  take the exact path
   //    float input(float x, float k, float y) const;
   //
   // where `input` returns what the stages are fed, given this sample's
   // input, the feedback gain and the previous output. A nonlinear cell
   // therefore uses the previous output (a one-sample delay in the
   // nonlinearity only; the stages stay TPT), which is the standard
   // inexpensive nonlinear ladder. A linear cell has no such approximation.
   //
   // That delay has a cost worth knowing. It adds phase to the loop, so a
   // nonlinear cell reaches self-oscillation before r = 1 once the cutoff
   // is a large fraction of the sample rate. Measured, and the same at
   // 48 kHz and 96 kHz because it is a ratio: it sets in above a cutoff of
   // about fs/3.5 at r = 0.4, fs/8 at r = 0.6 and fs/30 at r = 0.8. Below
   // r = 0.3 it is out of reach. The output stays bounded either way; what
   // changes is where the filter starts to sing. The linear path is exact
   // and has no such limit.
   //
   // moog_ladder and ota_ladder below are the two the library ships.
   ////////////////////////////////////////////////////////////////////////////
   template <typename Cell, std::size_t N = 4>
   struct basic_ladder
   {
      using cell_type = Cell;
      static constexpr std::size_t poles = N;

                     basic_ladder(
                        frequency f, float sps, float reso = 0.0f
                     );

      // The feedback at which this many poles reach self-oscillation,
      // sec(pi/N)^N: 4 at four poles, 8 at three, 2.37 at six. See note 1.
      static float   k_limit();

      float          operator()(float x);

      void           cutoff(frequency f, float sps);
      void           resonance(float r);
      void           config(frequency f, float sps, float r);

      Cell&          cell()                  { return _cell; }
      Cell const&    cell() const            { return _cell; }

      basic_ladder&  operator=(float y);     // clear the state

   private:

      float          stages(float in);
      void           derive();

      Cell  _cell;
      float _G = 0.0f;                 // one-pole TPT gain g/(1+g)
      float _1mg = 1.0f;               // 1 - G  (= 1/(1+g))
      float _GN = 0.0f;                // G^N, for the feedback resolution
      float _k = 0.0f;                 // feedback gain (0..k_limit)
      float _g_res = 1.0f;             // 1/(1 + k*G^N)
      float _s[N] = {};                // one-pole states
      float _y = 0.0f;                 // last output (feedback tap)
   };

   // Note 1: the loop's phase reaches 180 degrees where N*atan(w/wc) = pi, so
   // at w/wc = tan(pi/N), and the chain's gain there is cos(pi/N)^N. The
   // feedback that cancels it is the reciprocal. Fewer than three poles never
   // reach 180 degrees and so never self-oscillate; those take the four-pole's
   // scale, where r is a feedback amount and nothing more.

   namespace detail
   {
      ////////////////////////////////////////////////////////////////////////
      // moog_cell: a transistor ladder's. The saturation sits in the
      // feedback path, and it is symmetric, so what it adds is odd
      // harmonics. Linear until it is switched on.
      ////////////////////////////////////////////////////////////////////////
      struct moog_cell
      {
         bool        linear() const          { return !_nonlinear; }
         float       input(float x, float k, float y) const;

         void        nonlinear(bool on)      { _nonlinear = on; }
         bool        nonlinear() const       { return _nonlinear; }

      private:

         bool _nonlinear = false;
      };

      ////////////////////////////////////////////////////////////////////////
      // ota_cell: a variable gain (OTA) cell's, as the Curtis CEM3320 and its
      // kin have. It runs off centre and distorts asymmetrically,
      // "predominantly second harmonic" at 0.1 percent in the passband
      // (CEM3320 data sheet), and it sees the signal and the feedback
      // together, the way the first cell in the chain does.
      //
      // The curve is a tanh taken off centre by `asymmetry`, shifted so zero
      // maps to zero and scaled so the small signal gain is 1, so the
      // asymmetry changes the harmonic mix without changing the level or
      // adding a DC step. `drive` is how hard the cell is pushed, and 0 is
      // linear.
      //
      // The default asymmetry of 0.9 comes from measurement. At no
      // resonance, drive 0.5 to 2 puts the second harmonic 17 to 24 times
      // above the third, which is what "predominantly" asks for, and 0.35
      // would give 1.2 to 3. Resonance raises what the cell sees, since it
      // takes the feedback along with the signal, so the same drive bites
      // harder: at resonance 0.3 the ratio is 17 down to 5 over that range,
      // and at 0.7 it is 12 down to 0.55, the odd harmonics having taken
      // over. Drive is worth backing off as resonance rises.
      ////////////////////////////////////////////////////////////////////////
      struct ota_cell
      {
                     ota_cell();

         bool        linear() const          { return _drive <= 0.0f; }
         float       input(float x, float k, float y) const;

         void        drive(float amount);
         float       drive() const           { return _drive; }
         void        asymmetry(float amount);
         float       asymmetry() const       { return _bias; }

      private:

         float       curve(float x) const;

         float _drive = 0.0f;             // cell curvature, 0 is linear
         float _bias = 0.9f;              // cell offset, the asymmetry
         float _tb = 0.0f;                // tanh(bias)
         float _norm = 1.0f;              // unity small signal gain
      };
   }

   ////////////////////////////////////////////////////////////////////////////
   // moog_ladder: the ladder with a transistor ladder's cell. The optional
   // nonlinear mode runs a tanh saturation in the feedback path for the
   // saturating Moog tone.
   ////////////////////////////////////////////////////////////////////////////
   struct moog_ladder : basic_ladder<detail::moog_cell, 4>
   {
      using base_type = basic_ladder<detail::moog_cell, 4>;
      using base_type::operator=;            // clearing the state

                     moog_ladder(
                        frequency f, float sps, float reso = 0.0f
                      , bool nonlinear = false
                     );

      void           nonlinear(bool on)      { cell().nonlinear(on); }
      bool           nonlinear() const       { return cell().nonlinear(); }
   };

   ////////////////////////////////////////////////////////////////////////////
   // ota_ladder: the ladder with a variable gain (OTA) cell, the four-pole as
   // a CEM3320 and its kin build it. `drive` pushes the cell and `asymmetry`
   // skews it; asymmetry 0 is a ladder's symmetric curve.
   ////////////////////////////////////////////////////////////////////////////
   struct ota_ladder : basic_ladder<detail::ota_cell, 4>
   {
      using base_type = basic_ladder<detail::ota_cell, 4>;
      using base_type::base_type;
      using base_type::operator=;            // clearing the state

      void           drive(float amount)     { cell().drive(amount); }
      float          drive() const           { return cell().drive(); }
      void           asymmetry(float amount) { cell().asymmetry(amount); }
      float          asymmetry() const       { return cell().asymmetry(); }
   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   template <typename Cell, std::size_t N>
   inline basic_ladder<Cell, N>::basic_ladder(
      frequency f, float sps, float reso)
   {
      config(f, sps, reso);
   }

   template <typename Cell, std::size_t N>
   inline float basic_ladder<Cell, N>::k_limit()
   {
      static float const k = []()
      {
         if constexpr (N < 3)
            return 4.0f;               // never self-oscillates; see note 1
         else
            return float(std::pow(1.0 / std::cos(pi / N), double(N)));
      }();
      return k;
   }

   template <typename Cell, std::size_t N>
   inline float basic_ladder<Cell, N>::operator()(float x)
   {
      if (!_cell.linear())
      {
         _y = stages(_cell.input(x, _k, _y));
         return _y;
      }

      // Resolve the zero-delay feedback. The summed state seen through the
      // chain, S = (1-G) * (G^(N-1)*s[0] + ... + s[N-1]), in Horner form.
      auto S = _s[0];
      for (std::size_t i = 1; i != N; ++i)
         S = S * _G + _s[i];
      S *= _1mg;

      _y = stages((x - _k * S) * _g_res);
      return _y;
   }

   // Retune the cutoff (cheap: one tan and a few mults).
   template <typename Cell, std::size_t N>
   inline void basic_ladder<Cell, N>::cutoff(frequency f, float sps)
   {
      auto g = fast_tan(float(pi * as_double(f) / sps));
      _G = g / (1.0 + g);
      derive();
   }

   // Resonance as a normalized amount in [0, 1]; r = 1 is self-oscillation
   // for three poles or more, and a full feedback amount below that.
   template <typename Cell, std::size_t N>
   inline void basic_ladder<Cell, N>::resonance(float r)
   {
      _k = k_limit() * (r < 0.0f ? 0.0f : (r > 1.0f ? 1.0f : r));
      _g_res = 1.0f / (1.0f + _k * _GN);
   }

   template <typename Cell, std::size_t N>
   inline void basic_ladder<Cell, N>::config(frequency f, float sps, float r)
   {
      auto g = fast_tan(float(pi * as_double(f) / sps));
      _G = g / (1.0 + g);
      _k = k_limit() * (r < 0.0f ? 0.0f : (r > 1.0f ? 1.0f : r));
      derive();
   }

   template <typename Cell, std::size_t N>
   inline basic_ladder<Cell, N>& basic_ladder<Cell, N>::operator=(float y)
   {
      for (auto& s : _s)
         s = y;
      _y = y;
      return *this;
   }

   template <typename Cell, std::size_t N>
   inline float basic_ladder<Cell, N>::stages(float in)
   {
      auto y = in;
      for (auto& s : _s)
      {
         auto v = (y - s) * _G;
         y = v + s;
         s = y + v;
      }
      return y;
   }

   template <typename Cell, std::size_t N>
   inline void basic_ladder<Cell, N>::derive()
   {
      _1mg = 1.0f - _G;
      _GN = 1.0f;
      for (std::size_t i = 0; i != N; ++i)
         _GN *= _G;
      _g_res = 1.0f / (1.0f + _k * _GN);
   }

   namespace detail
   {
      inline float moog_cell::input(float x, float k, float y) const
      {
         return x - k * fast_tanh(y);
      }

      inline ota_cell::ota_cell()
      {
         asymmetry(_bias);
      }

      inline float ota_cell::input(float x, float k, float y) const
      {
         return curve(x - k * y);
      }

      inline float ota_cell::curve(float x) const
      {
         return (fast_tanh(x * _drive + _bias) - _tb) * _norm;
      }

      inline void ota_cell::drive(float amount)
      {
         _drive = amount < 0.0f ? 0.0f : amount;
         asymmetry(_bias);             // the normalization follows the drive
      }

      inline void ota_cell::asymmetry(float amount)
      {
         _bias = amount;
         _tb = fast_tanh(_bias);
         auto slope = _drive * (1.0f - (_tb * _tb));
         _norm = slope > 1e-6f ? 1.0f / slope : 1.0f;
      }
   }

   inline moog_ladder::moog_ladder(
      frequency f, float sps, float reso, bool nonlinear_)
    : base_type{f, sps, reso}
   {
      nonlinear(nonlinear_);
   }
}

#endif
