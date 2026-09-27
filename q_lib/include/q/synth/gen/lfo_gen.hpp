/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_LFO_GEN_HPP_SEPTEMBER_15_2026)
#define CYCFI_Q_LFO_GEN_HPP_SEPTEMBER_15_2026

#include <q/support/phase.hpp>
#include <q/synth/sin_osc.hpp>
#include <q/synth/va/saw_osc.hpp>
#include <q/synth/va/square_osc.hpp>
#include <q/synth/va/triangle_osc.hpp>
#include <q/synth/sample_hold_osc.hpp>
#include <q/synth/gen/noise_gen.hpp>
#include <cstdint>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // lfo_gen: a low frequency oscillator with six waveforms and a delayed,
   // linear fade-in from key-on: it waits `delay` seconds, then fades in
   // over `fade` seconds. A Generator: operator() advances and returns the
   // waveform in -1..1, faded; value() reads it again in the same sample.
   ////////////////////////////////////////////////////////////////////////////
   struct lfo_gen
   {
      enum waveform : std::uint8_t
      {
         triangle, saw_down, saw_up, square, sine, sample_hold
      };

      struct config
      {
         float          rate        = 5.0f;     // Hz
         float          delay       = 0.0f;     // seconds, before the fade
         float          fade        = 0.0f;     // seconds, to full
         bool           key_sync    = true;     // restart on key-on
         std::uint8_t   wave        = triangle;
      };

                     lfo_gen();
                     lfo_gen(config const& cfg, float sps);

      void           set(config const& cfg, float sps);
      void           key_on();
      float          operator()();
      float          value() const     { return _y; }   // the last

   private:

      float          wave();

      phase_iterator _pi;
      std::uint32_t  _wait_samples; // before the fade starts
      std::uint32_t  _wait = 0;
      float          _fade_step;    // per sample, up to 1 at the fade's end
      bool           _key_sync;
      std::uint8_t   _wave;
      float          _fade = 1.0f;
      float          _y = 0.0f;
      sample_hold_osc _sample_hold;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   inline lfo_gen::lfo_gen()
    : lfo_gen{config{}, 44100.0f}
   {}

   inline lfo_gen::lfo_gen(config const& cfg, float sps)
   {
      set(cfg, sps);
   }

   inline void lfo_gen::set(config const& cfg, float sps)
   {
      _pi.set(frequency{cfg.rate}, sps);
      _wait_samples = std::uint32_t(cfg.delay * sps);
      _fade_step = cfg.fade > 0.0f ? 1.0f / (cfg.fade * sps) : 1.0f;
      _key_sync = cfg.key_sync;
      _wave = cfg.wave;
   }

   inline void lfo_gen::key_on()
   {
      _fade = _fade_step >= 1.0f ? 1.0f : 0.0f;
      _wait = _wait_samples;
      if (_key_sync)
         _pi._phase = phase{};
   }

   // The plain (not band-limited) Q oscillators: an LFO has no aliasing
   // to correct. Only sample and hold keeps state.
   inline float lfo_gen::wave()
   {
      switch (_wave)
      {
         case triangle:
            return basic_triangle(_pi);
         case saw_down:
            return -basic_saw(_pi);
         case saw_up:
            return basic_saw(_pi);
         case square:
            return basic_square(_pi);
         case sine:
            return q::sin(_pi);
         case sample_hold:
            return _sample_hold(_pi, white_noise());
      }
      return 0.0f;
   }

   inline float lfo_gen::operator()()
   {
      _y = wave() * _fade;
      if (_wait)
      {
         --_wait;
      }
      else if (_fade < 1.0f)
      {
         _fade += _fade_step;
         if (_fade > 1.0f)
            _fade = 1.0f;
      }
      ++_pi;
      return _y;
   }
}

#endif
