/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_GRAIN_HPP_JUNE_11_2026)
#define CYCFI_Q_GRAIN_HPP_JUNE_11_2026

#include <q/synth/hann_gen.hpp>
#include <q/support/duration.hpp>
#include <cstddef>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // grain: a windowed, rate-1 read tap over a ring buffer.
   //
   // A grain is spawned anchored at a fixed (possibly fractional) delay
   // and emits window * buffer[delay] for `width` samples, then goes
   // inactive. The delay stays constant for the grain's lifetime: with
   // the buffer's newest-first indexing, a constant delay advances
   // through the signal at the buffer's write rate (rate 1). Re-pitching,
   // time stretching, and the like come from grain *scheduling* -- when
   // grains are spawned and where they are anchored -- never from
   // resampling within the grain. This is the property that preserves
   // the spectral envelope in pitch-synchronous overlap-add (PSOLA) and
   // granular processing.
   //
   // Window is any ramp generator with config(duration, sps), reset()
   // and operator()() -- e.g. hann_gen (the default), blackman_gen,
   // hamming_gen.
   //
   // Call operator()(buf) exactly once per buffer push; it returns 0
   // when the grain is inactive. The buffer's interpolation type sets
   // the read quality (hermite recommended for sub-sample anchors), and
   // its valid index range bounds the usable delay.
   ////////////////////////////////////////////////////////////////////////////
   template <typename Window = hann_gen>
   class grain
   {
   public:

      explicit grain(float sps)
       : _window{duration{1.0}, sps}    // reconfigured by spawn
       , _sps{sps}
      {}

      // Arm the grain: read at `delay` (in samples, may be fractional)
      // under a window spanning `width` samples.
      void spawn(float delay, std::size_t width)
      {
         _window.config(duration{width / _sps}, _sps, true);
         _delay = delay;
         _remaining = width;
      }

      // Emit the next windowed sample. Call once per buffer push.
      template <typename Buffer>
      float operator()(Buffer const& buf)
      {
         if (_remaining == 0)
            return 0.0f;
         --_remaining;
         return _window() * buf[_delay];
      }

      bool           active() const    { return _remaining != 0; }
      float          delay() const     { return _delay; }
      std::size_t    remaining() const { return _remaining; }

   private:

      Window         _window;
      float          _sps;
      float          _delay = 0.0f;
      std::size_t    _remaining = 0;
   };
}

#endif
