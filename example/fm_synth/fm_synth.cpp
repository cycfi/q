/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/support/literals.hpp>
#include <q/synth/fm/dx_patcher.hpp>
#include <q/synth/fm/fm_voice.hpp>
#include <q_io/audio_file.hpp>
#include <q_io/audio_stream.hpp>
#include <q_io/midi_file.hpp>
#include <q/utility/sleep.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
// The FM synthesizer playing four of the DX7's factory patches, rendered
// offline. This is what the FM Synthesis page plays, and running it writes
// those clips.
//
// The notes come from the MIDI files beside this source, read with
// q::midi_file. The patches are dx_patcher configurations in DX7 units,
// the numbers a DX7's panel shows, so what each sound is made of is in
// plain sight. A dx_patcher compiles one for a sample rate, and every
// voice in the pool plays from the same compiled patch.
///////////////////////////////////////////////////////////////////////////////

namespace q = cycfi::q;
namespace midi = q::midi_1_0;
using namespace q::literals;

// The rate everything is rendered at. Playing sets it from the
// device, since that is the rate the clips have to be made at.
float sps = 48000.0f;

///////////////////////////////////////////////////////////////////////////////
// E.PIANO 1, as the DX7 ships it.
///////////////////////////////////////////////////////////////////////////////
q::dx_patcher::config epiano_1()
{
   return
   {
      {  // OP1 to OP6: level, fixed, coarse, fine, detune, the
         // envelope, then the keyboard scaling and sensitivities
         {99, false,  1,  0, 10, {{96,25,25,67}, {99,75, 0, 0}},
             0,  0,  0, 0, 0, 3, 0, 2},
         {58, false, 14,  0,  7, {{95,50,35,78}, {99,75, 0, 0}},
             0,  0,  0, 0, 0, 3, 0, 7},
         {99, false,  1,  0,  7, {{95,20,20,50}, {99,95, 0, 0}},
             0,  0,  0, 0, 0, 3, 0, 2},
         {89, false,  1,  0,  7, {{95,29,20,50}, {99,95, 0, 0}},
             0,  0,  0, 0, 0, 3, 0, 6},
         {99, false,  1,  0,  0, {{95,20,20,50}, {99,95, 0, 0}},
             0,  0,  0, 0, 0, 3, 0, 0},
         {79, false,  1,  0, 14, {{95,29,20,50}, {99,95, 0, 0}},
            41,  0, 19, 0, 0, 3, 0, 6},
      },
      {{94,67,95,60}, {50,50,50,50}},   // the pitch envelope
      {5, 6},                              // algorithm, feedback
      {34, 33, false, 4},                    // the LFO
      0, 0, 3,                            // LFO depths and sensitivity
      false, 24,                          // oscillator key sync, transpose
      "E.PIANO 1"
   };
}

///////////////////////////////////////////////////////////////////////////////
// MARIMBA, as the DX7 ships it.
///////////////////////////////////////////////////////////////////////////////
q::dx_patcher::config marimba()
{
   return
   {
      {  // OP1 to OP6: level, fixed, coarse, fine, detune, the
         // envelope, then the keyboard scaling and sensitivities
         {95, false,  0,  0,  7, {{95,40,49,55}, {99,92, 0, 0}},
             0,  0,  0, 0, 0, 3, 0, 0},
         {96, false,  3,  0,  7, {{99,72, 0, 0}, {82,48, 0, 0}},
            54,  0, 46, 0, 0, 0, 0, 2},
         {99, false,  0,  0,  7, {{95,33,49,41}, {99,92, 0, 0}},
             0,  0,  0, 0, 0, 3, 0, 1},
         {85, false,  5,  0,  7, {{99,75, 0,82}, {82,48, 0, 0}},
            54,  0, 46, 0, 0, 0, 0, 2},
         {93, false,  0, 50,  7, {{99,75, 0, 8}, {82,48, 0, 0}},
            54,  0, 46, 0, 0, 0, 0, 2},
         {99, false,  4, 13,  7, {{ 0,63,55, 0}, {78,78, 0, 0}},
            41,  0,  0, 0, 0, 0, 0, 2},
      },
      {{94,67,95,60}, {50,50,50,50}},   // the pitch envelope
      {7, 0},                              // algorithm, feedback
      {35, 0, true, 0},                    // the LFO
      0, 0, 3,                            // LFO depths and sensitivity
      true, 24,                          // oscillator key sync, transpose
      "MARIMBA"
   };
}

///////////////////////////////////////////////////////////////////////////////
// BRASS   1, as the DX7 ships it.
///////////////////////////////////////////////////////////////////////////////
q::dx_patcher::config brass_1()
{
   return
   {
      {  // OP1 to OP6: level, fixed, coarse, fine, detune, the
         // envelope, then the keyboard scaling and sensitivities
         {98, false,  0,  0, 14, {{72,76,99,71}, {99,88,96, 0}},
            39,  0, 14, 3, 3, 0, 0, 0},
         {86, false,  0,  0, 14, {{62,51,29,71}, {82,95,96, 0}},
            27,  0,  7, 3, 1, 0, 0, 0},
         {99, false,  1,  0,  5, {{77,76,82,71}, {99,98,98, 0}},
            39,  0,  0, 3, 3, 0, 0, 2},
         {99, false,  1,  0,  7, {{77,36,41,71}, {99,98,98, 0}},
            39,  0,  0, 3, 3, 0, 0, 2},
         {98, false,  1,  0,  8, {{77,36,41,71}, {99,98,98, 0}},
            39,  0,  0, 3, 3, 0, 0, 2},
         {82, false,  1,  0,  7, {{49,99,28,68}, {98,98,91, 0}},
            39, 54, 50, 1, 1, 4, 0, 2},
      },
      {{84,95,95,60}, {50,50,50,50}},   // the pitch envelope
      {22, 7},                              // algorithm, feedback
      {37, 0, false, 4},                    // the LFO
      5, 0, 3,                            // LFO depths and sensitivity
      true, 24,                          // oscillator key sync, transpose
      "BRASS   1"
   };
}

///////////////////////////////////////////////////////////////////////////////
// BASS    1, as the DX7 ships it.
///////////////////////////////////////////////////////////////////////////////
q::dx_patcher::config bass_1()
{
   return
   {
      {  // OP1 to OP6: level, fixed, coarse, fine, detune, the
         // envelope, then the keyboard scaling and sensitivities
         {99, false,  0,  0,  7, {{95,62,17,58}, {99,95,32, 0}},
            36, 57, 14, 3, 0, 7, 0, 0},
         {80, false,  0,  0,  7, {{99,20, 0, 0}, {99, 0, 0, 0}},
            41,  0,  0, 0, 0, 7, 0, 0},
         {99, false,  0,  0,  7, {{88,96,32,30}, {79,65, 0, 0}},
             0,  0,  0, 0, 0, 6, 0, 3},
         {93, false,  5,  0,  7, {{90,42, 7,55}, {90,30, 0, 0}},
             0,  0,  0, 0, 0, 5, 0, 5},
         {62, false,  0,  0,  7, {{99, 0, 0, 0}, {99, 0, 0, 0}},
            52, 75,  0, 0, 0, 7, 0, 3},
         {85, false,  9,  0,  7, {{94,56,24,55}, {93,28, 0, 0}},
             0,  0,  0, 0, 0, 1, 0, 7},
      },
      {{94,67,95,60}, {50,50,50,50}},   // the pitch envelope
      {16, 7},                              // algorithm, feedback
      {35, 0, false, 0},                    // the LFO
      0, 0, 3,                            // LFO depths and sensitivity
      true, 12,                          // oscillator key sync, transpose
      "BASS    1"
   };
}

///////////////////////////////////////////////////////////////////////////////
// One voice: an fm_voice and the phase it is played at. The pitch is the
// caller's, as it is for every oscillator in Q, so the voice takes a phase
// iterator rather than holding a frequency of its own.
///////////////////////////////////////////////////////////////////////////////
struct slot
{
   slot(std::shared_ptr<q::dx_patcher const> p)
    : patch{p}, voice{*p}
   {}

   void attack(std::uint8_t key_, float velocity, std::uint64_t order_)
   {
      key = key_;
      order = order_;
      master.set(q::frequency{440.0 * std::pow(2.0, (key - 69) / 12.0)}, sps);
      voice.attack(*patch, key, velocity);
   }

   std::shared_ptr<q::dx_patcher const> patch;
   q::fm_voice           voice;
   q::phase_iterator     master;
   std::uint8_t          key = 0;
   std::uint64_t         order = 0;       // when it was last played
};

///////////////////////////////////////////////////////////////////////////////
// What the MIDI file plays. A live synth would act on these as they
// arrive; rendering offline, they are collected first.
///////////////////////////////////////////////////////////////////////////////
struct note_event
{
   std::size_t    time;                   // samples from the start
   std::uint8_t   key;
   float          velocity;               // 0 for a note off
};

struct recorder
{
   void operator()(midi::message_base const&, std::size_t) {}

   void operator()(midi::note_on msg, std::size_t time)
   {
      // A note on with no velocity is a note off, which is how a good
      // many sequencers write them.
      events.push_back({time, msg.key(),
         msg.velocity() == 0? 0.0f : msg.velocity() / 127.0f});
   }

   void operator()(midi::note_off msg, std::size_t time)
   {
      events.push_back({time, msg.key(), 0.0f});
   }

   std::vector<note_event> events;
};

///////////////////////////////////////////////////////////////////////////////
// Play a file through a pool of voices, and keep going until the last one
// has died away or the tail runs out. A DX7 envelope whose final level is
// above zero never dies away on its own, so the tail is what decides when
// such a patch stops.
///////////////////////////////////////////////////////////////////////////////
std::vector<float> render(
   q::midi_file const& file, q::dx_patcher::config const& cfg
 , std::size_t voices = 16, double tail = 3.0)
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

   auto patcher = std::make_shared<q::dx_patcher const>(cfg, sps);
   std::vector<slot> pool;
   pool.reserve(voices);
   for (std::size_t i = 0; i != voices; ++i)
      pool.emplace_back(patcher);

   std::vector<float> out;
   out.reserve(rec.events.back().time + std::size_t(4 * sps));

   std::uint64_t order = 0;
   std::size_t next = 0;
   for (std::size_t i = 0; ; ++i)
   {
      while (next != rec.events.size() && rec.events[next].time <= i)
      {
         auto const& e = rec.events[next++];
         if (e.velocity == 0.0f)
         {
            for (auto& s : pool)
               if (s.voice.active() && s.key == e.key)
                  s.voice.release();
         }
         else
         {
            auto free = std::find_if(pool.begin(), pool.end(),
               [](auto const& s) { return !s.voice.active(); });
            auto& s = free != pool.end()? *free
               : *std::min_element(pool.begin(), pool.end(),
                  [](auto const& a, auto const& b)
                  { return a.order < b.order; });   // steal the oldest
            s.attack(e.key, e.velocity, ++order);
         }
      }

      auto mix = 0.0f;
      auto sounding = false;
      for (auto& s : pool)
      {
         if (s.voice.active())
         {
            mix += s.voice(s.master++);
            sounding = true;
         }
      }
      out.push_back(mix);

      if (next == rec.events.size())
      {
         if (!sounding)
            break;
         if (i > rec.events.back().time + std::size_t(tail * sps))
            break;
      }
   }

   // A 20 Hz one-pole high-pass, as the instrument's output stage has.
   auto a = 1.0f / (1.0f + 2.0f * float(q::pi) * 20.0f / sps);
   auto px = 0.0f, py = 0.0f;
   for (auto& s : out)
   {
      auto x = s;
      py = a * (py + x - px);
      px = x;
      s = py;
   }

   return out;
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
      char const* motif;                  // the MIDI file
      char const* name;                   // what it is written as
      q::dx_patcher::config (*patch)();
   };

   static item const items[] = {
      {"epiano_ballad", "epiano_1", epiano_1},
      {"marimba_riff", "marimba", marimba},
      {"brass_stabs", "brass_1", brass_1},
      {"bass_synthpop", "bass_1", bass_1},
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
      auto path = std::string{MIDI_DIR} + "/" + item.motif + ".mid";
      q::midi_file file{path};
      if (!file)
      {
         std::printf("could not read %s\n", path.c_str());
         return 1;
      }

      auto out = render(file, item.patch());

      // Each clip is brought to the same peak. The patches' own levels
      // are more than 20 dB apart, BASS 1 being the quietest of these
      // and BRASS 1 the loudest, which is how the instrument ships them.
      auto peak = 0.0f;
      for (auto s : out)
         peak = std::max(peak, std::abs(s));
      auto gain = 0.891f / std::max(peak, 1e-6f);      // -1 dBFS
      for (auto& s : out)
         s *= gain;

      if (out_dir.empty())
      {
         std::printf("%-10s %5.1f s\n", item.name, out.size() / sps);
         std::fflush(stdout);
         play.play(out);
         while (!play.done())
            q::sleep(50_ms);
      }
      else
      {
         auto wav = out_dir + "/" + item.name + ".wav";
         q::wav_writer{wav, 1, sps}.write(out);
         std::printf("%-10s %5.1f s  %s\n",
            item.name, out.size() / sps, wav.c_str());
      }
   }

   if (out_dir.empty())
      play.stop();

   return 0;
}
