/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#include <q/support/literals.hpp>
#include <q/synth/saw_osc.hpp>
#include <q/synth/envelope_gen.hpp>
#include <q/fx/svf.hpp>
#include <q/fx/clip.hpp>
#include <q_io/audio_stream.hpp>
#include <q_io/midi_stream.hpp>
#include "example.hpp"

#include <vector>
#include <ranges>
#include <algorithm>
#include <cstdint>

///////////////////////////////////////////////////////////////////////////////
// A polyphonic, MIDI-controlled sawtooth synthesizer. Each note plays on its
// own voice: a bandwidth-limited sawtooth through an ADSR envelope that sweeps
// a resonant low-pass (q::svf) and the amplifier. A fixed pool of voices is
// allocated per note-on (a free voice, or the oldest one stolen), and the
// active voices are summed into the output.
//
// Polyphony needs no special library support: a voice is just an instance of
// the same fine-grained building blocks the monophonic square_synth uses, and
// the voice pool is a plain array plus a small allocator.
///////////////////////////////////////////////////////////////////////////////

namespace q = cycfi::q;
using namespace q::literals;
namespace midi = q::midi_1_0;

///////////////////////////////////////////////////////////////////////////////
// One synth voice.
struct voice
{
   voice(q::adsr_envelope_gen::config env_cfg, float sps)
    : _env{env_cfg, sps}
    , _filter{1_kHz, sps, 2.0}      // cutoff is set per sample; sps; resonance Q
    , _sps{sps}
   {}

   void on(q::frequency freq, float velocity)
   {
      _velocity = velocity;
      _phase.set(freq, _sps);
      _env.attack();                // retriggers cleanly even if this voice was
   }                                // still sounding (a stolen voice)

   void off() { _env.release(); }

   bool active() const { return !_env.in_idle_phase(); }

   float operator()()
   {
      auto env = _env() * _velocity;
      // Sweep the cutoff with the envelope, then amplify by it.
      _filter.cutoff(q::frequency{60.0 + (env * 6000.0)}, _sps);
      return _filter(q::saw(_phase++)) * env;
   }

   q::phase_iterator    _phase;
   q::adsr_envelope_gen _env;
   q::svf               _filter;
   float                _sps;
   float                _velocity = 0.0f;
   std::uint8_t         _key = 0;       // the MIDI key this voice is playing
   std::uint64_t        _order = 0;     // allocation order, for voice stealing
};

///////////////////////////////////////////////////////////////////////////////
// The polyphonic synth: a pool of voices and an allocator.
struct poly_synth : q::audio_stream
{
   static constexpr std::size_t num_voices = 16;
   static constexpr float master_gain = 0.3f;     // headroom for summed voices

   poly_synth(q::adsr_envelope_gen::config env_cfg, int device_id)
    : audio_stream(q::audio_device::get(device_id), 0, 2)
   {
      _voices.reserve(num_voices);
      for (std::size_t i = 0; i != num_voices; ++i)
         _voices.emplace_back(env_cfg, sampling_rate());
   }

   void note_on(std::uint8_t key, float velocity)
   {
      allocate(key).on(midi::note_frequency(key), velocity);
   }

   void note_off(std::uint8_t key)
   {
      for (auto& v : _voices)
         if (v.active() && v._key == key)
            v.off();
   }

   void process(out_channels const& out)
   {
      auto left = out[0];
      auto right = out[1];
      for (auto frame : out.frames)
      {
         auto mix = 0.0f;
         for (auto& v : _voices)
            if (v.active())
               mix += v();
         left[frame] = right[frame] = _clip(mix * master_gain);
      }
   }

private:

   // Pick a free (idle) voice, or steal the oldest one.
   voice& allocate(std::uint8_t key)
   {
      auto free = std::ranges::find_if(
         _voices, [](voice const& v) { return !v.active(); });
      voice& v = (free != _voices.end())
         ? *free
         : *std::ranges::min_element(_voices, {}, &voice::_order);
      v._key = key;
      v._order = ++_order;
      return v;
   }

   std::vector<voice> _voices;
   q::cubic_clip      _clip;
   std::uint64_t      _order = 0;
};

///////////////////////////////////////////////////////////////////////////////
// MIDI: route note-on / note-off to the synth.
struct my_midi_processor : midi::processor
{
   using midi::processor::operator();

   my_midi_processor(poly_synth& synth)
    : _synth(synth)
   {}

   void operator()(midi::note_on msg, std::size_t)
   {
      if (msg.velocity() == 0)                 // note-on with velocity 0 is a
         _synth.note_off(msg.key());           // note-off, by MIDI convention
      else
         _synth.note_on(msg.key(), float(msg.velocity()) / 127);
   }

   void operator()(midi::note_off msg, std::size_t)
   {
      _synth.note_off(msg.key());
   }

   poly_synth& _synth;
};

int main()
{
   q::midi_input_stream::set_default_device(get_midi_device());
   auto audio_device_id = get_audio_device();

   auto env_cfg = q::adsr_envelope_gen::config
   {
      100_ms      // attack rate
    , 1_s         // decay rate
    , -12_dB      // sustain level
    , 5_s         // sustain rate
    , 1_s         // release rate
   };

   poly_synth synth{env_cfg, audio_device_id};
   q::midi_input_stream stream;
   my_midi_processor proc{synth};

   if (!stream.is_valid())
      return -1;

   synth.start();
   while (running)
      stream.process(proc);
   synth.stop();

   return 0;
}
