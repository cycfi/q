/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include "dexter_processor.hpp"
#include <q/support/literals.hpp>
#include <algorithm>
#include <cmath>

using namespace cycfi::q::literals;

namespace
{
   using voice = dexter_processor::voice;

   // A voice's carriers add up: one note of the DX7's BRASS 1 at full
   // velocity peaks at 4.2, and a chord of six at 18.6. The mix is
   // brought down so that chord stays under full scale; the volume is
   // there for more.
   constexpr float headroom = 0.05f;

   // The glide's time, 0 to 99: none at 0, then 10 ms to 5 s, the same
   // ratio for every step.
   constexpr double glide_min = 0.01;
   constexpr double glide_span = 500.0;
}

///////////////////////////////////////////////////////////////////////////////
// The pitch of a voice: a phase step, gliding to where the key puts it.
// The operators follow the master's step, not its phase, so the step is
// all a voice needs.
///////////////////////////////////////////////////////////////////////////////
void voice::glide_to(double step_, double samples)
{
   step = step_;
   if (samples < 1.0 || now == 0.0)
   {
      now = step;
      glide = 0;
   }
   else
   {
      ratio = std::pow(step / now, 1.0 / samples);
      glide = std::uint32_t(samples);
   }
}

q::phase_iterator voice::next(float bend)
{
   if (glide)
   {
      now *= ratio;
      if (--glide == 0)
         now = step;
   }
   auto m = master;
   m._step = q::phase{std::uint32_t(now * bend), q::direct_unit};
   return m;
}

///////////////////////////////////////////////////////////////////////////////
// The processor
///////////////////////////////////////////////////////////////////////////////
dexter_processor::dexter_processor(dexter_controller& ctl)
 : _ctl(ctl)
{
   _keys.reserve(128);
}

// The sample rate is known here: compile the patch and build the pool.
void dexter_processor::activate()
{
   _patcher.emplace(_ctl.patch(), float(sps()));
   _voices.assign(num_voices, voice{});
   for (auto& v : _voices)
      v.fm = q::fm_voice{*_patcher};
   _dc.emplace(20_Hz, float(sps()));
   _volume.cutoff(4_Hz, float(sps()));
   _volume = q::lin_float(_ctl.volume());
}

// A transport jump: silence everything rather than let notes ring across.
void dexter_processor::reset()
{
   for (auto& v : _voices)
      v = voice{q::fm_voice{*_patcher}};
   _keys.clear();
   _order = 0;
   _last_step = 0.0;
   _sustain = false;
   _bend = 1.0f;
   _wheel = 0.0f;
   _volume = q::lin_float(_ctl.volume());
}

// A voice parameter moved: compile the patch again and give it to the
// notes sounding, each for its own key and velocity, where they are. The
// plugin's own parameters, the operator switches among them, are live:
// read as they are used, they do not bring this on.
void dexter_processor::parameters_changed()
{
   _patcher.emplace(_ctl.patch(), float(sps()));
   for (auto& v : _voices)
   {
      if (v.fm.active())
         v.fm.update(*_patcher, v.key, v.velocity);
   }
}

void dexter_processor::process(in_channels const& /*in*/
 , out_channels const& out)
{
   // The wheel and the operator switches reach the notes already
   // sounding, not only the next ones.
   q::fm_routing::mask enabled = 0;
   for (int op = 0; op != dexter_controller::num_operators; ++op)
   {
      if (_ctl.enabled(op))
         enabled |= 1u << op;
   }
   for (auto& v : _voices)
   {
      v.fm.enable(enabled);
      v.fm.mod_wheel(_wheel);
   }

   auto const volume = q::lin_float(_ctl.volume());
   auto left = out[0];
   auto right = out[1];
   for (auto frame : out.frames)
   {
      auto mix = 0.0f;
      for (auto& v : _voices)
      {
         if (v.fm.active())
            mix += v.fm(v.next(_bend));
      }
      auto const y = (*_dc)(mix * headroom * _volume(volume));
      left[frame] = right[frame] = y;
   }
}

///////////////////////////////////////////////////////////////////////////////
// The notes
///////////////////////////////////////////////////////////////////////////////
void dexter_processor::operator()(midi::note_on msg, std::size_t)
{
   note_on(msg.key(), float(msg.velocity()) / 65535);
}

void dexter_processor::operator()(midi::note_off msg, std::size_t)
{
   note_off(msg.key());
}

void dexter_processor::operator()(midi::control_change msg, std::size_t)
{
   if (msg.controller() == cc::sustain)
      sustain(msg.value() >= 0x80000000u);
   else if (msg.controller() == cc::modulation)
      _wheel = float(msg.value()) / 4294967295.0f;
}

void dexter_processor::operator()(midi::pitch_bend msg, std::size_t)
{
   auto const semitones =
      (float(msg.value()) - float(midi::pitch_bend::center))
      / float(midi::pitch_bend::center) * bend_range;
   _bend = std::exp2(semitones / 12.0f);
}

double dexter_processor::step_of(std::uint8_t key) const
{
   auto const hz = 440.0 * std::exp2((key - 69) / 12.0);
   return double(q::phase{q::frequency{hz}, float(sps())}.rep);
}

double dexter_processor::glide_samples() const
{
   auto const g = _ctl.glide();
   if (g == 0)
      return 0.0;
   return glide_min * std::pow(glide_span, g / 99.0) * sps();
}

// A new note starts where the last one was and glides to its own pitch.
// With the portamento set to follow, the notes the pedal holds glide to
// it too; set to retain, they keep theirs.
void dexter_processor::note_on(std::uint8_t key, float velocity)
{
   auto const step = step_of(key);
   auto const glide = glide_samples();

   if (_ctl.mono())
   {
      std::erase(_keys, key);
      _keys.push_back(key);

      // Legato: a key down while another is held moves the note, and
      // the envelopes carry on.
      auto& v = _voices.front();
      if (_keys.size() > 1 && v.fm.active())
      {
         v.key = key;
         v.glide_to(step, glide);
         _last_step = step;
         return;
      }
   }

   if (_ctl.porta_follow())
   {
      for (auto& v : _voices)
      {
         if (v.held)
            v.glide_to(step, glide);
      }
   }

   auto& v = _ctl.mono()? _voices.front() : allocate();
   v.fm.set(_patcher->voice, float(sps()));
   v.fm.attack(*_patcher, key, velocity);
   v.key = key;
   v.velocity = velocity;
   v.order = ++_order;
   v.held = false;
   v.now = _last_step;
   v.glide_to(step, glide);
   _last_step = step;
}

// Mono: a key coming up while others are down goes back to the last of
// them, as a legato. Otherwise the pedal holds the note or it is let go.
void dexter_processor::note_off(std::uint8_t key)
{
   if (_ctl.mono())
   {
      std::erase(_keys, key);
      auto& v = _voices.front();
      if (v.key != key)
         return;
      if (!_keys.empty())
      {
         v.key = _keys.back();
         v.glide_to(step_of(v.key), glide_samples());
         _last_step = v.step;
         return;
      }
   }

   for (auto& v : _voices)
   {
      if (v.fm.active() && v.key == key && !v.held)
      {
         if (_sustain)
            v.held = true;
         else
            v.fm.release();
      }
   }
}

void dexter_processor::sustain(bool down)
{
   if (down == _sustain)
      return;
   _sustain = down;
   if (!_sustain)
   {
      for (auto& v : _voices)
      {
         if (v.held)
         {
            v.held = false;
            v.fm.release();
         }
      }
   }
}

// A free voice, or the oldest sounding one.
voice& dexter_processor::allocate()
{
   auto free = std::ranges::find_if(
      _voices, [](voice const& v) { return !v.fm.active(); });
   return (free != _voices.end())?
      *free : *std::ranges::min_element(_voices, {}, &voice::order);
}
