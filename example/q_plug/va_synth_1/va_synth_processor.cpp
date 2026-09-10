/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include "va_synth_processor.hpp"
#include <algorithm>
#include <cmath>
#include <ranges>

using namespace cycfi::q::literals;
using voice = va_synth_processor::voice;

namespace
{
   // Sixteen voices at full tilt would clip a stereo bus, so the sum is
   // scaled before the clipper rather than leaning on it. How loud the
   // instrument is beyond that is the channel fader's business.
   constexpr float headroom = 0.3f;
}

///////////////////////////////////////////////////////////////////////////////
// The voice
///////////////////////////////////////////////////////////////////////////////
voice::voice(va_synth_envelope_config const& cfg, float sps)
 : _env{cfg, sps}
 , _sps{sps}
{}

void voice::on(q::frequency freq, float velocity)
{
   _velocity = velocity;
   _freq = freq;
   _phase.set(freq, _sps);

   // Retriggers cleanly even if this voice was still sounding (a stolen
   // voice): the attack starts from the level already there rather than
   // from silence, which would click.
   _env.attack();
}

void voice::off()
{
   _env.release();
}

bool voice::active() const
{
   return !_env.in_idle_phase();
}

float voice::operator()(float pitch_factor)
{
   auto env = _env() * _velocity;
   _phase.set(q::frequency{as_float(_freq) * pitch_factor}, _sps);
   return q::saw(_phase++) * env;
}

///////////////////////////////////////////////////////////////////////////////
// The synth
///////////////////////////////////////////////////////////////////////////////
va_synth_processor::va_synth_processor(va_synth_controller& ctl)
 : _ctl(ctl)
{}

va_synth_envelope_config va_synth_processor::envelope_config() const
{
   // Q's envelope takes its sustain as a level in decibels, so the
   // fraction the panel deals in is converted here rather than there.
   return
   {
      _ctl.attack()
    , _ctl.decay()
    , q::lin_to_db(_ctl.sustain_level())
    , _ctl.release()
   };
}

// The sample rate is known here and stays put until the next activation,
// so this is where the pool is built. The Q example builds it in main.
void va_synth_processor::activate()
{
   auto const cfg = envelope_config();
   _voices.clear();
   _voices.reserve(num_voices);
   for (std::size_t i = 0; i != num_voices; ++i)
      _voices.emplace_back(cfg, float(sps()));
   _lfo.config(vibrato_rate, float(sps()));

   _pushed =
   {
      cfg.attack_rate.rep, cfg.decay_rate.rep
    , double(q::lin_float(cfg.sustain_level)), cfg.release_rate.rep
   };
}

// A transport jump: silence everything rather than let notes ring across it.
void va_synth_processor::reset()
{
   auto const cfg = envelope_config();
   for (auto& v : _voices)
   {
      v._env = q::adsr_envelope_gen{cfg, float(sps())};
      v._held = false;
   }
   _order = 0;
   _sustain = false;
   _bend = 0.0f;
   _wheel = 0.0f;
   _lfo.config(vibrato_rate, float(sps()));
}

// The Q example hands its envelope a config once, in main, and never
// touches it again. A plugin has to move those settings while a note
// sounds, which is what the generator's setters are for. Pushing one into
// a segment that is running restarts that segment, so only a setting that
// actually moved since the last block is pushed.
void va_synth_processor::update_envelopes()
{
   auto const rate = float(sps());
   settings const now
   {
      _ctl.attack().rep
    , _ctl.decay().rep
    , _ctl.sustain_level()
    , _ctl.release().rep
   };

   bool const attack = now.attack != _pushed.attack;
   bool const decay = now.decay != _pushed.decay;
   bool const sustain_level = now.sustain_level != _pushed.sustain_level;
   bool const release = now.release != _pushed.release;

   if (!(attack || decay || sustain_level || release))
      return;

   for (auto& v : _voices)
   {
      if (attack)
         v._env.attack_rate(q::duration{now.attack}, rate);
      if (decay)
         v._env.decay_rate(q::duration{now.decay}, rate);
      if (sustain_level)
         v._env.sustain_level(float(now.sustain_level));
      if (release)
         v._env.release_rate(q::duration{now.release}, rate);
   }
   _pushed = now;
}

void va_synth_processor::process(in_channels const& /*in*/
 , out_channels const& out)
{
   update_envelopes();

   auto left = out[0];
   auto right = out[1];
   for (auto frame : out.frames)
   {
      // Bend and vibrato, in semitones, become one ratio every voice
      // multiplies its pitch by. Twelve semitones is a doubling.
      auto const vibrato = _lfo().first * _wheel * vibrato_depth;
      auto const pitch_factor = std::exp2((_bend + vibrato) / 12.0f);

      auto mix = 0.0f;
      for (auto& v : _voices)
         if (v.active())
            mix += v(pitch_factor);
      left[frame] = right[frame] = _clip(mix * headroom);
   }
}

///////////////////////////////////////////////////////////////////////////////
// The notes
///////////////////////////////////////////////////////////////////////////////
void va_synth_processor::operator()(midi::note_on msg, std::size_t)
{
   // A note on of zero velocity is a note off, by MIDI 1.0 convention.
   if (msg.velocity() == 0)
      note_off(msg.key());
   else
      note_on(msg.key(), float(msg.velocity()) / 127);
}

void va_synth_processor::operator()(midi::note_off msg, std::size_t)
{
   note_off(msg.key());
}

void va_synth_processor::operator()(midi::control_change msg, std::size_t)
{
   // 64 and above is down, below is up: the convention for the pedal
   // controllers, so that a half pedal reads as down rather than as an
   // eighth of something.
   if (msg.controller() == midi::cc::sustain)
      sustain(msg.value() >= 64);
   else if (msg.controller() == midi::cc::modulation)
      _wheel = float(msg.value()) / 127.0f;
}

void va_synth_processor::operator()(midi::pitch_bend msg, std::size_t)
{
   // Fourteen bits centred on 8192; full travel is the bend range.
   _bend = (float(msg.value()) - 8192.0f) / 8192.0f * bend_range;
}

void va_synth_processor::note_on(std::uint8_t key, float velocity)
{
   allocate(key).on(midi::note_frequency(key), sensed(velocity));
}

// The velocity a voice plays at, given the one the key was struck with.
// Sensitivity is a mix between that and full: at none the key's velocity
// is ignored and every note is the same.
float va_synth_processor::sensed(float velocity) const
{
   auto const s = float(_ctl.velocity());
   return (1.0f - s) + s * velocity;
}

// With the sustain pedal down, a key coming up does not end the note: the
// voice is marked held and keeps sounding until the pedal is lifted. This
// is what the damper does on a piano, and controller 64 is how a keyboard
// says it.
void va_synth_processor::note_off(std::uint8_t key)
{
   // Every voice playing that note, since a key struck twice before its
   // release finished holds two.
   for (auto& v : _voices)
   {
      if (v.active() && v._key == key)
      {
         if (_sustain)
            v._held = true;
         else
            v.off();
      }
   }
}

void va_synth_processor::sustain(bool down)
{
   if (down == _sustain)
      return;
   _sustain = down;

   // Lifting it releases everything it was holding.
   if (!_sustain)
   {
      for (auto& v : _voices)
      {
         if (v._held)
         {
            v._held = false;
            v.off();
         }
      }
   }
}

// A free voice, or the oldest sounding one. Stealing the oldest is what
// makes a held chord survive a run of fast notes over it.
voice& va_synth_processor::allocate(std::uint8_t key)
{
   auto free = std::ranges::find_if(
      _voices, [](voice const& v) { return !v.active(); });

   voice& v = (free != _voices.end())?
      *free
    : *std::ranges::min_element(_voices, {}, &voice::_order);

   v._key = key;
   v._order = ++_order;
   v._held = false;      // struck again: the pedal is no longer holding it
   return v;
}
