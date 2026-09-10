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
voice::voice(
   va_synth_envelope_config const& amp
 , va_synth_envelope_config const& filter
 , float sps)
 : _env{amp, sps}
 , _filter_env{filter, sps}
 , _filter{1_kHz, sps, 2.0}      // cutoff is set per sample; resonance Q
 , _filter2{1_kHz, sps, q::svf::default_q}
 , _sps{sps}
{}

void voice::filter(q::frequency cutoff, double depth, double resonance)
{
   _cutoff = as_float(cutoff);
   _depth = float(depth);
   _filter.resonance(resonance);
}

void voice::on(q::frequency freq, float velocity)
{
   _velocity = velocity;
   _phase.set(freq, _sps);

   // Retriggers cleanly even if this voice was still sounding (a stolen
   // voice): the attack starts from the level already there rather than
   // from silence, which would click. Both contours start together.
   _env.attack();
   _filter_env.attack();
}

void voice::off()
{
   _env.release();
   _filter_env.release();
}

bool voice::active() const
{
   return !_env.in_idle_phase();
}

float voice::operator()()
{
   auto env = _env() * _velocity;

   // The filter has a contour of its own, so how bright the note is does
   // not have to follow how loud it is. It sweeps the cutoff in octaves
   // above where the dial sits: the ear counts pitch in octaves, so a
   // sweep of two sounds the same whether it starts at 100 Hz or at
   // 1 kHz. Stopped short of Nyquist, where the filter has nothing left
   // to pass.
   auto const f = std::min(
      _cutoff * std::exp2(_filter_env() * _depth), _sps * 0.45f);
   _filter.cutoff(q::frequency{f}, _sps);
   _filter2.cutoff(q::frequency{f}, _sps);

   return _filter2(_filter(q::saw(_phase++))) * env;
}

///////////////////////////////////////////////////////////////////////////////
// The synth
///////////////////////////////////////////////////////////////////////////////
va_synth_processor::va_synth_processor(va_synth_controller& ctl)
 : _ctl(ctl)
{}

va_synth_envelope_config va_synth_processor::envelope_config() const
{
   return
   {
      _ctl.attack()
    , _ctl.decay()
    , _ctl.sustain_level()
    , _ctl.release()
   };
}

va_synth_envelope_config
va_synth_processor::filter_envelope_config() const
{
   // Q's envelope takes its sustain as a level in decibels, so the
   // fraction the panel deals in is converted here rather than there.
   return
   {
      _ctl.filter_attack()
    , _ctl.filter_decay()
    , q::lin_to_db(_ctl.filter_sustain_level())
    , _ctl.filter_release()
   };
}

// The sample rate is known here and stays put until the next activation,
// so this is where the pool is built. The Q example builds it in main.
void va_synth_processor::activate()
{
   auto const amp = envelope_config();
   auto const filter = filter_envelope_config();
   _voices.clear();
   _voices.reserve(num_voices);
   for (std::size_t i = 0; i != num_voices; ++i)
      _voices.emplace_back(amp, filter, float(sps()));
   update_filter();

   _pushed =
   {
      amp.attack_rate.rep, amp.decay_rate.rep, amp.sustain_level.rep
    , amp.release_rate.rep
   };
   _filter_pushed =
   {
      filter.attack_rate.rep, filter.decay_rate.rep
    , filter.sustain_level.rep, filter.release_rate.rep
   };
}

// A transport jump: silence everything rather than let notes ring across it.
void va_synth_processor::reset()
{
   auto const amp = envelope_config();
   auto const filter = filter_envelope_config();
   for (auto& v : _voices)
   {
      v._env = q::adsr_envelope_gen{amp, float(sps())};
      v._filter_env = q::adsr_envelope_gen{filter, float(sps())};
      v._held = false;
   }
   _order = 0;
   _sustain = false;
}

// The Q example hands its envelope a config once, in main, and never
// touches it again. A plugin has to move those settings while a note
// sounds, which is what the generator's setters are for. Pushing one into
// a segment that is running restarts that segment, so only a setting that
// actually moved since the last block is pushed.
// Push one contour's settings into one envelope. Pushing a rate into a
// segment that is running restarts that segment, so only a setting that
// actually moved since the last block is pushed.
// The sustain arrives as the fraction the envelope works in: the panel's
// amplifier level is converted from decibels once here, when it moved,
// and the filter contour's amount is a fraction to begin with.
void va_synth_processor::push(
   q::adsr_envelope_gen& env, settings const& now, settings const& then
 , float sustain, float sps)
{
   if (now.attack != then.attack)
      env.attack_rate(q::duration{now.attack}, sps);
   if (now.decay != then.decay)
      env.decay_rate(q::duration{now.decay}, sps);
   if (now.sustain_level != then.sustain_level)
      env.sustain_level(sustain);
   if (now.release != then.release)
      env.release_rate(q::duration{now.release}, sps);
}

// The Q example hands its envelope a config once, in main, and never
// touches it again. A plugin has to move those settings while a note
// sounds, which is what the generator's setters are for.
void va_synth_processor::update_envelopes()
{
   auto const rate = float(sps());

   settings const amp
   {
      _ctl.attack().rep
    , _ctl.decay().rep
    , _ctl.sustain_level().rep
    , _ctl.release().rep
   };

   settings const filter
   {
      _ctl.filter_attack().rep
    , _ctl.filter_decay().rep
    , _ctl.filter_sustain_level()
    , _ctl.filter_release().rep
   };

   if (amp == _pushed && filter == _filter_pushed)
      return;

   auto const amp_level = q::lin_float(q::dB(amp.sustain_level));
   for (auto& v : _voices)
   {
      push(v._env, amp, _pushed, amp_level, rate);
      push(v._filter_env, filter, _filter_pushed
         , float(filter.sustain_level), rate);
   }
   _pushed = amp;
   _filter_pushed = filter;
}

// The filter settings are plain values with no state behind them, so
// unlike the envelope's rates they can be pushed every block without
// disturbing anything.
void va_synth_processor::update_filter()
{
   auto const cutoff = _ctl.cutoff();
   auto const depth = _ctl.env_depth();
   auto const resonance = _ctl.resonance();
   for (auto& v : _voices)
      v.filter(cutoff, depth, resonance);
}

void va_synth_processor::process(in_channels const& /*in*/
 , out_channels const& out)
{
   update_envelopes();
   update_filter();

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

void va_synth_processor::operator()(midi::control_change msg, std::size_t)
{
   // 64 and above is down, below is up: the convention for the pedal
   // controllers, so that a half pedal reads as down rather than as an
   // eighth of something.
   if (msg.controller() == midi::cc::sustain)
      sustain(msg.value() >= 64);
}

void va_synth_processor::note_on(std::uint8_t key, float velocity)
{
   allocate(key).on(midi::note_frequency(key), velocity);
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
