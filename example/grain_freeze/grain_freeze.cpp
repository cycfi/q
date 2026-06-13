/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#include <q/support/literals.hpp>
#include <q/synth/grain.hpp>
#include <q/synth/sin_cos_gen.hpp>
#include <q/utility/fractional_ring_buffer.hpp>
#include <q_io/audio_stream.hpp>
#include <q_io/audio_file.hpp>
#include <q/utility/sleep.hpp>

#include <random>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
// Granular freeze using q::grain.
//
// The audio file plays through dry while being recorded into a ring
// buffer. From the freeze point on, two overlapping Hann grains are
// continually re-spawned, anchored at the frozen input moment (plus a
// few milliseconds of jitter), sustaining that moment as a smooth
// texture -- long after the source note has decayed.
//
// The grains overlap at half their width, so their Hann windows sum to
// one (constant overlap-add); the jitter keeps the texture alive instead
// of metallic. A slow sine LFO wanders the anchor around the frozen
// moment so the tail breathes and evolves, and a half-Hann ramp fades
// the texture to silence over the closing seconds.
///////////////////////////////////////////////////////////////////////////////

namespace q = cycfi::q;
using namespace q::literals;

struct grain_freeze : q::audio_stream
{
   static constexpr std::size_t width = 4096;     // Grain length
   static constexpr std::size_t hop = width / 2;  // 50% overlap (COLA)

   using buffer_type = q::fractional_ring_buffer<
      float, std::vector<float>, double, q::sample_interpolation::hermite>;

   grain_freeze(
      float* wav
    , std::size_t len
    , float sps
    , q::duration freeze_at
    , q::duration tail
    , q::duration fade
   )
    : audio_stream(0, 2, sps)
    , _wav(wav)
    , _len(len)
    , _buff(len + std::size_t(as_double(tail) * sps) + width)
    , _grains{q::grain<>{sps}, q::grain<>{sps}}
    , _freeze_pos(std::size_t(as_double(freeze_at) * sps))
    , _jitter(-int(sps * 0.005f), int(sps * 0.005f))
    , _lfo(0.3_Hz, sps)
    , _lfo_depth(sps * 0.025f)        // wander the anchor +/-25 ms
    , _fade(fade, sps)
    , _fade_start(len + std::size_t((as_double(tail) - as_double(fade)) * sps))
    , _fade_len(std::size_t(as_double(fade) * sps))
   {}

   void process(out_channels const& out)
   {
      auto left = out[0];
      auto right = out[1];
      for (auto frame : out.frames)
      {
         // Play the file through dry, recording it into the ring buffer;
         // silence after it ends, but the buffer keeps moving.
         auto s = _t < _len ? _wav[_t] : 0.0f;
         _buff.push(s);

         // A slow LFO (sin_cos_gen: the Chamberlin SVF oscillator, made
         // for LFO duty) wanders the anchor around the frozen moment so
         // the sustained texture breathes instead of standing still.
         auto wander = _lfo_depth * _lfo().first;

         // From the freeze point on, re-spawn a grain every half width,
         // anchored at the frozen moment (+/- the LFO wander and 5ms of
         // jitter). A fixed anchor in input time means a delay that grows
         // with the output clock.
         if (_t >= _freeze_pos + width && (_t % hop == 0))
         {
            auto anchor = double(_freeze_pos) + wander + _jitter(_rand);
            auto delay = double(_t) - anchor;
            _grains[(_t / hop) & 1].spawn(float(delay), width);
         }

         // Fade the texture to silence over the closing seconds
         auto gain =
            _t < _fade_start ? 1.0f
          : _t < _fade_start + _fade_len ? _fade()
          : 0.0f
          ;

         auto texture = _grains[0](_buff) + _grains[1](_buff);
         auto mix = s + 0.8f * gain * texture;
         left[frame] = mix;
         right[frame] = mix;
         ++_t;
      }
   }

   float*                              _wav;
   std::size_t                         _len;
   buffer_type                         _buff;
   q::grain<>                          _grains[2];
   std::size_t                         _freeze_pos;
   std::size_t                         _t = 0;
   std::minstd_rand                    _rand{12345};
   std::uniform_int_distribution<int>  _jitter;
   q::sin_cos_gen                      _lfo;
   float                               _lfo_depth;
   q::hann_downward_ramp_gen           _fade;
   std::size_t                         _fade_start;
   std::size_t                         _fade_len;
};

int main()
{
   q::wav_reader wav{AUDIO_DIR "/Low E.wav"};
   if (wav)
   {
      std::vector<float> in(wav.length());
      wav.read(in);

      auto tail = q::duration{6.0};
      grain_freeze proc{
         in.data(), in.size(), wav.sps()
       , 1.2_s   // freeze the note 1.2 seconds in
       , tail
       , 4_s     // fade the texture over the last 4 seconds
      };

      if (proc.is_valid())
      {
         proc.start();
         q::sleep(q::duration(wav.length() / wav.sps()) + tail);
         proc.stop();
      }
   }

   return 0;
}
