/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#include <q/fx/clip.hpp>
#include <q/fx/ladder.hpp>
#include <q/support/literals.hpp>
#include <q/synth/gen/envelope_gen.hpp>
#include <q/synth/gen/lfo_gen.hpp>
#include <q/synth/va/analog_osc.hpp>
#include <q_io/audio_file.hpp>
#include <q_io/audio_stream.hpp>
#include <q_io/midi_file.hpp>
#include <q/utility/sleep.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
// A virtual analog synthesizer, rendered offline: two analog oscillators a
// few cents apart, a ladder filter with an envelope on its cutoff, and the
// envelopes and LFO around them. This is the voice behind the clips the
// Virtual Analog page plays, and running it writes those clips.
//
// The notes come from the MIDI files beside this source, read with
// q::midi_file, which plays them into a processor in sample time. The
// patches are here in full, one per motif, so the numbers that make each
// sound are in plain sight.
///////////////////////////////////////////////////////////////////////////////

namespace q = cycfi::q;
namespace midi = q::midi_1_0;
using namespace q::literals;

// The rate everything is rendered at. Playing sets it from the
// device, since that is the rate the clips have to be made at.
float sps = 48000.0f;

///////////////////////////////////////////////////////////////////////////////
// The patch: what a front panel would set.
///////////////////////////////////////////////////////////////////////////////
struct patch
{
   // the oscillators
   float          osc_b_detune = 0.02f;   // semitones, the pair's beating
   float          osc_b_ratio = 1.0f;     // 0.5 an octave down, 2 an octave up
   float          a_saw = 0.6f, a_pulse = 0.0f;
   float          b_saw = 0.0f, b_tri = 0.0f, b_pulse = 0.0f;
   float          width = 0.5f;
   float          pwm = 0.0f;             // pulse width swing under the LFO

   // the filter
   float          cutoff = 1200.0f;       // hertz at rest
   float          env_amount = 4000.0f;   // hertz the envelope adds
   float          resonance = 0.2f;
   float          drive = 1.0f;
   float          key_track = 0.4f;       // how far the cutoff follows the note
   int            poles = 4;              // two or four
   bool           moog = false;           // the other cell, saturating
   float          mixer_drive = 1.0f;     // into the filter, as a mixer does

   // the envelopes, the LFO and the level
   q::duration    amp_a = 20_ms, amp_d = 300_ms, amp_r = 300_ms;
   q::decibel     amp_s = -6_dB;
   q::duration    flt_a = 20_ms, flt_d = 400_ms, flt_r = 300_ms;
   q::decibel     flt_s = -18_dB;
   float          lfo_rate = 0.5f;
   float          gain = 0.8f;
};

///////////////////////////////////////////////////////////////////////////////
// One voice: the oscillators, the filter and the two envelopes.
///////////////////////////////////////////////////////////////////////////////
struct voice
{
   voice(patch const& p)
    : _p{p}
    , _filt{q::frequency{p.cutoff}, sps, p.resonance}
    , _filt2{q::frequency{p.cutoff}, sps, p.resonance}
    , _moog{q::frequency{p.cutoff}, sps, p.resonance, true}
    , _amp{q::adsr_envelope_gen::config{
         p.amp_a, p.amp_d, p.amp_s, 30_s, p.amp_r}, sps}
    , _flt_env{q::adsr_envelope_gen::config{
         p.flt_a, p.flt_d, p.flt_s, 30_s, p.flt_r}, sps}
   {
      _a.width(p.width);
      _b.width(p.width);
      _filt.drive(p.drive);
      _filt2.cell().drive(p.drive);       // the knobs live on the cell
   }

   void attack(int note, float velocity)
   {
      if (!active())
      {
         // The voice has been silent, so the filter still holds the state
         // it had when the last note faded. Left there it rings through
         // the attack and ticks. A retrigger while the voice is still
         // sounding keeps its state, which is what makes a line smooth.
         _filt = 0.0f;
         _filt2 = 0.0f;
         _moog = 0.0f;
      }

      _note = note;
      _velocity = velocity;
      _hz = 440.0 * std::pow(2.0, (note - 69) / 12.0);
      _pa.set(q::frequency{_hz}, sps);
      _pb.set(q::frequency{
         _hz * _p.osc_b_ratio * std::pow(2.0, _p.osc_b_detune / 12)}, sps);
      _amp.attack();
      _flt_env.attack();
   }

   void release()
   {
      _amp.release();
      _flt_env.release();
   }

   bool active() const     { return !_amp.in_idle_phase(); }
   int note() const        { return _note; }
   float level() const     { return _amp.current(); }

   float operator()(float lfo)
   {
      if (_p.pwm > 0.0f)
      {
         _a.width(std::clamp(_p.width + _p.pwm * lfo, 0.05f, 0.95f));
         _b.width(std::clamp(_p.width - _p.pwm * lfo, 0.05f, 0.95f));
      }

      auto mix = _p.a_saw * _a.saw(_pa) + _p.a_pulse * _a.pulse(_pa)
         + _p.b_saw * _b.saw(_pb) + _p.b_tri * _b.triangle(_pb)
         + _p.b_pulse * _b.pulse(_pb);

      if (_p.mixer_drive > 1.0f)
         mix = _clip(mix * _p.mixer_drive);     // the mixer, overdriven

      // The filter envelope opens the cutoff, squared so the sweep is
      // heard evenly, and the note itself carries it a little further.
      auto e = _flt_env();
      auto track = 1.0 + _p.key_track * ((_hz / 261.6) - 1.0);
      auto fc = q::frequency{
         std::clamp((_p.cutoff + _p.env_amount * e * e * _velocity) * track,
            30.0, 16000.0)};

      ++_pa;
      ++_pb;

      if (_p.moog)
      {
         _moog.cutoff(fc, sps);
         return _moog(mix * 0.35f) * _amp() * _velocity;
      }
      if (_p.poles == 2)
      {
         _filt2.cutoff(fc, sps);
         return _filt2(mix * 0.35f) * _amp() * _velocity;
      }
      _filt.cutoff(fc, sps);
      return _filt(mix * 0.35f) * _amp() * _velocity;
   }

private:

   patch                _p;
   q::analog_osc        _a, _b;
   q::phase_iterator    _pa, _pb;
   q::ota_ladder        _filt;
   q::basic_ladder<q::detail::ota_cell, 2> _filt2;
   q::moog_ladder       _moog;
   q::soft_clip         _clip;
   q::adsr_envelope_gen _amp, _flt_env;
   int                  _note = -1;
   float                _velocity = 1.0f;
   double               _hz = 440.0;
};

///////////////////////////////////////////////////////////////////////////////
// What the MIDI file plays. A live synth would act on these as they
// arrive; rendering offline, they are collected first and consumed by the
// loop below.
///////////////////////////////////////////////////////////////////////////////
struct note_event
{
   std::size_t    time;                   // samples from the start
   int            note;
   float          velocity;               // 0 for a note off
};

struct recorder
{
   void operator()(midi::message_base const&, std::size_t) {}

   void operator()(midi::note_on msg, std::size_t time)
   {
      // A note on with no velocity is a note off, which is how a good
      // many sequencers write them.
      if (msg.velocity() == 0)
         events.push_back({time, msg.key(), 0.0f});
      else
         events.push_back({time, msg.key(), msg.velocity() / 127.0f});
   }

   void operator()(midi::note_off msg, std::size_t time)
   {
      events.push_back({time, msg.key(), 0.0f});
   }

   std::vector<note_event> events;
};

///////////////////////////////////////////////////////////////////////////////
// Play a file through a pool of voices.
///////////////////////////////////////////////////////////////////////////////
std::vector<float> render(
   q::midi_file const& file, patch const& p, std::size_t voices = 10)
{
   recorder rec;
   file(sps, rec);
   if (rec.events.empty())
      return {};

   // Note offs come before note ons at the same instant, so a voice freed
   // on a beat can take the note that starts on it.
   std::stable_sort(rec.events.begin(), rec.events.end(),
      [](auto const& a, auto const& b)
      {
         if (a.time != b.time)
            return a.time < b.time;
         return a.velocity == 0.0f && b.velocity != 0.0f;
      });

   // Every voice is built in place. Copying one prototype would give them
   // all the same envelope ramps.
   std::vector<voice> pool;
   pool.reserve(voices);
   for (std::size_t i = 0; i != voices; ++i)
      pool.emplace_back(p);

   q::lfo_gen lfo{{.rate = p.lfo_rate}, sps};

   // Room for the slowest release to finish after the last note.
   auto tail = std::size_t(
      (std::max(as_double(p.amp_r), as_double(p.flt_r)) + 0.5) * sps);
   std::vector<float> out(rec.events.back().time + tail, 0.0f);

   std::size_t next = 0;
   for (std::size_t i = 0; i != out.size(); ++i)
   {
      while (next != rec.events.size() && rec.events[next].time <= i)
      {
         auto const& e = rec.events[next++];
         if (e.velocity == 0.0f)
         {
            for (auto& v : pool)
               if (v.active() && v.note() == e.note)
                  v.release();
         }
         else
         {
            auto v = std::find_if(pool.begin(), pool.end(),
               [](auto const& v) { return !v.active(); });
            if (v == pool.end())          // steal the quietest
               v = std::min_element(pool.begin(), pool.end(),
                  [](auto const& a, auto const& b)
                  { return a.level() < b.level(); });
            v->attack(e.note, e.velocity);
         }
      }

      auto l = lfo();
      auto sum = 0.0f;
      for (auto& v : pool)
         if (v.active())
            sum += v(l);
      out[i] = sum;
   }

   // The patch's own level, taken at the peak so a patch keeps its
   // character without clipping.
   auto peak = 0.0f;
   for (auto s : out)
      peak = std::max(peak, std::abs(s));
   auto scale = p.gain / std::max(peak, 1e-6f);
   for (auto& s : out)
      s *= scale;

   return out;
}

///////////////////////////////////////////////////////////////////////////////
// The patches. Each one is voiced by ear, after the instruments of the
// era rather than any one of them.
///////////////////////////////////////////////////////////////////////////////
patch pad()                               // slow, wide and string-like
{
   patch p;
   p.a_saw = 0.5f; p.b_saw = 0.45f; p.osc_b_detune = 0.06f;
   p.width = 0.5f; p.pwm = 0.18f; p.lfo_rate = 0.35f;
   p.cutoff = 500.0f; p.env_amount = 2200.0f; p.resonance = 0.12f;
   p.drive = 0.8f; p.key_track = 0.5f;
   p.amp_a = 350_ms; p.amp_d = 2_s; p.amp_s = -3_dB; p.amp_r = 900_ms;
   p.flt_a = 900_ms; p.flt_d = 3_s; p.flt_s = -10_dB; p.flt_r = 900_ms;
   p.gain = 0.55f;
   return p;
}

patch square_lead()                       // a sustained square wave lead
{
   // Two squares a few cents apart, which is where the thickness comes
   // from; the filter stays out of the way and the note holds.
   patch p;
   p.a_saw = 0.0f; p.a_pulse = 0.85f;
   p.b_saw = 0.0f; p.b_pulse = 0.6f;
   p.width = 0.5f; p.osc_b_detune = 0.07f;
   p.cutoff = 2600.0f; p.env_amount = 2600.0f; p.resonance = 0.12f;
   p.drive = 1.1f; p.key_track = 0.6f;
   p.amp_a = 16_ms; p.amp_d = 900_ms; p.amp_s = -2_dB; p.amp_r = 260_ms;
   p.flt_a = 24_ms; p.flt_d = 700_ms; p.flt_s = -6_dB; p.flt_r = 260_ms;
   p.gain = 0.7f;
   return p;
}

patch bass()                              // eighth notes, filter envelope led
{
   patch p;
   p.a_saw = 0.75f; p.b_pulse = 0.35f; p.osc_b_ratio = 0.5f;
   p.width = 0.35f; p.osc_b_detune = 0.03f;
   p.cutoff = 220.0f; p.env_amount = 3000.0f; p.resonance = 0.35f;
   p.drive = 2.0f; p.key_track = 0.25f;
   p.amp_a = 4_ms; p.amp_d = 250_ms; p.amp_s = -9_dB; p.amp_r = 120_ms;
   p.flt_a = 3_ms; p.flt_d = 180_ms; p.flt_s = -26_dB; p.flt_r = 120_ms;
   p.gain = 0.8f;
   return p;
}

patch brass()                             // chord stabs
{
   patch p;
   p.a_saw = 0.55f; p.b_saw = 0.5f; p.osc_b_detune = 0.09f;
   p.cutoff = 700.0f; p.env_amount = 5000.0f; p.resonance = 0.18f;
   p.drive = 1.3f; p.key_track = 0.45f;
   p.amp_a = 14_ms; p.amp_d = 260_ms; p.amp_s = -14_dB; p.amp_r = 180_ms;
   p.flt_a = 10_ms; p.flt_d = 220_ms; p.flt_s = -22_dB; p.flt_r = 160_ms;
   p.gain = 0.65f;
   return p;
}

patch pulse()                             // a narrow pulse, the width moving
{
   patch p;
   p.a_pulse = 0.8f; p.a_saw = 0.0f; p.b_pulse = 0.3f;
   p.width = 0.22f; p.pwm = 0.1f; p.lfo_rate = 0.8f;
   p.osc_b_ratio = 2.0f; p.osc_b_detune = 0.04f;
   p.cutoff = 900.0f; p.env_amount = 4200.0f; p.resonance = 0.25f;
   p.drive = 1.2f; p.key_track = 0.5f;
   p.amp_a = 5_ms; p.amp_d = 160_ms; p.amp_s = -12_dB; p.amp_r = 140_ms;
   p.flt_a = 4_ms; p.flt_d = 140_ms; p.flt_s = -20_dB; p.flt_r = 140_ms;
   p.gain = 0.7f;
   return p;
}

patch moog_growl()                        // the ladder bass
{
   // Saws in unison with a square an octave below, all of it pushed into
   // the filter hard enough to overdrive the mixer, the cutoff low and
   // resonant, and the filter closing over the note slowly.
   patch p;
   p.moog = true;
   p.a_saw = 0.85f;
   p.b_pulse = 0.55f; p.osc_b_ratio = 0.5f; p.width = 0.5f;
   p.osc_b_detune = 0.10f;
   p.mixer_drive = 2.6f;
   p.cutoff = 110.0f; p.env_amount = 2600.0f; p.resonance = 0.6f;
   p.key_track = 0.0f;
   p.amp_a = 8_ms; p.amp_d = 14_s; p.amp_s = -4_dB; p.amp_r = 2500_ms;
   p.flt_a = 40_ms; p.flt_d = 9_s; p.flt_s = -70_dB; p.flt_r = 2500_ms;
   p.gain = 0.85f;
   return p;
}

///////////////////////////////////////////////////////////////////////////////
// Playing a rendered buffer through the default output device. The stream
// says what rate it opened at, and everything is rendered at that rate, so
// a device that will not do 48 kHz plays in tune anyway.
///////////////////////////////////////////////////////////////////////////////
struct player : q::audio_stream
{
   player()
    : audio_stream(0, 2, sps)
   {}

   void play(std::vector<float> const& buffer)
   {
      _buffer = &buffer;
      _at = 0;
   }

   bool done() const
   {
      return _buffer == nullptr || _at >= _buffer->size();
   }

   void process(out_channels const& out)
   {
      auto left = out[0];
      auto right = out[1];
      for (auto frame : out.frames)
      {
         auto s = done()? 0.0f : (*_buffer)[_at++];
         left[frame] = right[frame] = s;
      }
   }

private:

   std::vector<float> const* _buffer = nullptr;
   std::size_t          _at = 0;
};

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
   struct item
   {
      char const* name;
      patch (*make)();
   };

   static item const items[] = {
      {"air_pad", pad},
      {"cars_hook", square_lead},
      {"new_wave_bass", bass},
      {"poly_stab", brass},
      {"pulse_arp", pulse},
      {"moog_growl", moog_growl},
   };

   // With no argument the clips play, one after another. Given a
   // directory, they are written there instead, which is how the ones the
   // documentation plays are made.
   std::string out_dir = argc > 1? argv[1] : "";

   player play;
   if (out_dir.empty())
   {
      if (!play.is_valid())
      {
         std::printf("no audio output: %s\n", play.error());
         return 1;
      }
      sps = float(play.sampling_rate());
      std::printf("playing through the default output at %.0f Hz\n", sps);
      std::fflush(stdout);
      play.start();
   }

   for (auto const& item : items)
   {
      auto path = std::string{MIDI_DIR} + "/" + item.name + ".mid";
      q::midi_file file{path};
      if (!file)
      {
         std::printf("could not read %s\n", path.c_str());
         return 1;
      }

      auto out = render(file, item.make());

      if (out_dir.empty())
      {
         std::printf("%-16s %5.1f s\n", item.name, out.size() / sps);
         std::fflush(stdout);
         play.play(out);
         while (!play.done())
            q::sleep(50_ms);
      }
      else
      {
         auto wav = out_dir + "/" + item.name + ".wav";
         q::wav_writer{wav, 1, sps}.write(out);
         std::printf("%-16s %5.1f s  %s\n",
            item.name, out.size() / sps, wav.c_str());
      }
   }

   if (out_dir.empty())
      play.stop();

   return 0;
}
