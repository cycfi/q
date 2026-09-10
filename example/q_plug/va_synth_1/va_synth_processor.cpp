/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include "va_synth_processor.hpp"
#include <algorithm>
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
voice::voice(q::adsr_envelope_gen::config const& cfg, float sps)
 : _env{cfg, sps}
 , _sps{sps}
{}

void voice::on(q::frequency freq, float velocity)
{
   _velocity = velocity;
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

float voice::operator()()
{
   auto env = _env() * _velocity;
   return q::saw(_phase++) * env;
}

///////////////////////////////////////////////////////////////////////////////
// The synth
///////////////////////////////////////////////////////////////////////////////
va_synth_processor::va_synth_processor(va_synth_controller& ctl)
 : _ctl(ctl)
{}

q::adsr_envelope_gen::config va_synth_processor::envelope_config() const
{
   return
   {
      _ctl.attack()
    , _ctl.decay()
    , _ctl.sustain_level()
    , _ctl.sustain_rate()
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

   _pushed =
   {
      cfg.attack_rate.rep, cfg.decay_rate.rep, cfg.sustain_level.rep
    , cfg.sustain_rate.rep, cfg.release_rate.rep
   };
}

// A transport jump: silence everything rather than let notes ring across it.
void va_synth_processor::reset()
{
   auto const cfg = envelope_config();
   for (auto& v : _voices)
      v._env = q::adsr_envelope_gen{cfg, float(sps())};
   _order = 0;
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
    , _ctl.sustain_level().rep
    , _ctl.sustain_rate().rep
    , _ctl.release().rep
   };

   bool const attack = now.attack != _pushed.attack;
   bool const decay = now.decay != _pushed.decay;
   bool const sustain_level = now.sustain_level != _pushed.sustain_level;
   bool const sustain_rate = now.sustain_rate != _pushed.sustain_rate;
   bool const release = now.release != _pushed.release;

   if (!(attack || decay || sustain_level || sustain_rate || release))
      return;

   for (auto& v : _voices)
   {
      if (attack)
         v._env.attack_rate(q::duration{now.attack}, rate);
      if (decay)
         v._env.decay_rate(q::duration{now.decay}, rate);
      if (sustain_level)
         v._env.sustain_level(q::dB(now.sustain_level));
      if (sustain_rate)
         v._env.sustain_rate(q::duration{now.sustain_rate}, rate);
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
      auto mix = 0.0f;
      for (auto& v : _voices)
         if (v.active())
            mix += v();
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

void va_synth_processor::note_on(std::uint8_t key, float velocity)
{
   allocate(key).on(midi::note_frequency(key), velocity);
}

void va_synth_processor::note_off(std::uint8_t key)
{
   // Every voice playing that note, since a key struck twice before its
   // release finished holds two.
   for (auto& v : _voices)
      if (v.active() && v._key == key)
         v.off();
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
   return v;
}
