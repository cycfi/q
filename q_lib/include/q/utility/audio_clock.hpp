/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_AUDIO_CLOCK_OCTOBER_12_2026)
#define CYCFI_Q_AUDIO_CLOCK_OCTOBER_12_2026

#include <q/support/duration.hpp>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // audio_clock: the audio thread's time, in frames processed.
   // The audio thread advances it by each block's frames; any thread can
   // read it. It stands still while no audio is processed, and in an
   // offline render it runs as fast as the render does, so a delay
   // measured on it is a delay in the audio.
   //
   // The sample rate converts a duration to frames. Set it before the
   // audio thread runs, as when a stream starts.
   ////////////////////////////////////////////////////////////////////////////
   class audio_clock
   {
   public:

      explicit          audio_clock(float sps);
                        audio_clock(audio_clock const&) = delete;
      audio_clock&      operator=(audio_clock const&) = delete;

      // Audio thread.
      void              advance(std::size_t frames);

      // Any thread.
      std::int64_t      now() const;
      std::int64_t      ticks(duration d) const;

      // Before the audio thread runs.
      void              sps(float sps_);

   private:

      using frame_count = std::atomic<std::int64_t>;
      using rate = std::atomic<float>;

      frame_count       _frames = 0;
      rate              _sps;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   inline audio_clock::audio_clock(float sps)
    : _sps(sps)
   {}

   // One writer, so a load and a store are enough.
   inline void audio_clock::advance(std::size_t frames)
   {
      _frames.store(
         _frames.load(std::memory_order_relaxed) + std::int64_t(frames)
       , std::memory_order_relaxed);
   }

   inline std::int64_t audio_clock::now() const
   {
      return _frames.load(std::memory_order_relaxed);
   }

   inline std::int64_t audio_clock::ticks(duration d) const
   {
      auto const sps = _sps.load(std::memory_order_relaxed);
      return std::llround(as_double(d) * sps);
   }

   inline void audio_clock::sps(float sps_)
   {
      _sps.store(sps_, std::memory_order_relaxed);
   }
}

#endif
