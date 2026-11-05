/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/fft/formant_learner.hpp>
#include <q/fx/bacf_pitch_shifter.hpp>
#include <q/fx/formant_corrector.hpp>
#include <q/fx/lowpass.hpp>
#include <q/midi/bend_range.hpp>
#include <q/midi/processor.hpp>
#include <q/support/literals.hpp>
#include <q/utility/sleep.hpp>
#include <q_io/audio_file.hpp>
#include <q_io/audio_stream.hpp>
#include <q_io/midi_file.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <span>
#include <string>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
// A whammy bar on q::bacf_pitch_shifter. The bar is MIDI pitch bend, as an
// e-Whammy sends it, read from a standard MIDI file timed against the
// audio: centre (8192) is unison, the ends are -12 and +12 semitones. The
// bend is held between messages and smoothed over about a millisecond,
// then drives the shifter's ratio sample by sample.
//
// Formant preservation keeps the guitar's own tone through the shift: a
// formant_corrector after the shifter, with a table learned from every
// sample in audio/ before anything plays. A live program learns as it
// plays, on a worker thread (see the Formant Preservation page).
//
// The output plays on the default audio device. With no arguments it
// offers the samples in audio/, each with the bend track of the same name
// in midi/, to pick from, and f to turn formant preservation on and off;
// given an audio file and a bend track, it learns from that file and
// plays it:
//
//    example_whammy [audio.wav bend.mid]
///////////////////////////////////////////////////////////////////////////////

namespace q = cycfi::q;
using namespace q::literals;
namespace fs = std::filesystem;

namespace
{
   // The bar: 12 semitones either way.
   constexpr q::midi::bend_range bar{12.0f};

   // The pitch bends of a MIDI file, each at its sample time, in
   // semitones.
   struct bends : q::midi::processor
   {
      using q::midi::processor::operator();

      void operator()(q::midi_1_0::pitch_bend msg, std::size_t time)
      {
         events.push_back({time, bar.semitones(msg)});
      }

      struct event
      {
         std::size_t    time;
         float          semitones;
      };

      std::vector<event> events;
   };

   // The shifter under the bar: the latest bend, smoothed, as the ratio,
   // then the formants put back, given a table.
   struct whammy
   {
      whammy(
         std::vector<bends::event> events, float sps
       , q::formant_table const* table)
       : _events{std::move(events)}
       , _shift{sps}
       , _smooth{100_Hz, sps}
      {
         _correct.table(table);
      }

      float operator()(float s)
      {
         while (_next != _events.size() && _events[_next].time <= _t)
            _semis = _events[_next++].semitones;
         ++_t;
         auto const ratio = std::exp2(_smooth(_semis) / 12.0f);
         return _correct(_shift(s, ratio), ratio);
      }

      std::vector<bends::event>  _events;
      q::bacf_pitch_shifter      _shift;
      q::formant_corrector       _correct;
      q::one_pole_lowpass        _smooth;
      std::size_t                _next = 0;
      std::size_t                _t = 0;
      float                      _semis = 0.0f;
   };

   struct take
   {
      std::vector<float>         audio;
      std::vector<bends::event>  bends;
      float                      sps = 44100;
   };

   bool load_audio(fs::path const& audio, take& t)
   {
      q::wav_reader wav{audio.string()};
      if (!wav)
      {
         std::cerr << "cannot open " << audio << '\n';
         return false;
      }
      t.sps = wav.sps();
      std::vector<float> x(wav.length() * wav.num_channels());
      wav.read(x);
      auto const ch = wav.num_channels();
      t.audio.resize(wav.length());
      for (std::size_t i = 0; i != t.audio.size(); ++i)
      {
         float sum = 0;
         for (std::size_t c = 0; c != ch; ++c)
            sum += x[i * ch + c];
         t.audio[i] = sum / ch;
      }
      return true;
   }

   bool load(fs::path const& audio, fs::path const& midi, take& t)
   {
      if (!load_audio(audio, t))
         return false;

      q::midi_file file{midi.string()};
      if (!file)
      {
         std::cerr << "cannot read " << midi << '\n';
         return false;
      }
      bends b;
      file(t.sps, b);
      t.bends = std::move(b.events);
      return true;
   }

   struct player : q::audio_stream
   {
      player(take const& t, q::formant_table const* table)
       : audio_stream(0, 2, t.sps)
       , _audio{t.audio}
       , _whammy{t.bends, t.sps, table}
      {}

      void process(out_channels const& out)
      {
         auto left = out[0];
         auto right = out[1];
         for (auto frame : out.frames)
         {
            auto s = _i < _audio.size() ? _audio[_i++] : 0.0f;
            auto y = _whammy(s);
            left[frame] = y;
            right[frame] = y;
         }
      }

      std::vector<float> const&  _audio;
      whammy                     _whammy;
      std::size_t                _i = 0;
   };

   void play(take const& t, q::formant_table const* table)
   {
      player p{t, table};
      if (!p.is_valid())
      {
         std::cerr << "no audio output\n";
         return;
      }
      p.start();
      q::sleep(q::duration{t.audio.size() / double(t.sps) + 0.5});
      p.stop();
   }

   // The guitar's formants, learned from the given samples, in blocks
   // as a worker would get them. False if nothing could be learned.
   bool learn(std::vector<fs::path> const& files, q::formant_table& table)
   {
      constexpr std::size_t block = 256;
      std::unique_ptr<q::formant_learner> learner;
      for (auto const& f : files)
      {
         take t;
         if (!load_audio(f, t))
            continue;
         if (!learner)
            learner = std::make_unique<q::formant_learner>(t.sps);
         for (std::size_t i = 0; i + block <= t.audio.size(); i += block)
            (*learner)(std::span<float const>{t.audio.data() + i, block});
      }
      return learner && learner->fit(table);
   }

   // The samples that have a bend track of the same name.
   std::vector<std::string> shipped()
   {
      std::vector<std::string> names;
      for (auto const& e : fs::directory_iterator{AUDIO_DIR})
      {
         auto const stem = e.path().stem().string();
         if (e.path().extension() == ".wav"
            && fs::exists(fs::path{MIDI_DIR} / (stem + ".mid")))
            names.push_back(stem);
      }
      std::sort(names.begin(), names.end());
      return names;
   }

   fs::path audio_of(std::string const& name)
   {
      return fs::path{AUDIO_DIR} / (name + ".wav");
   }

   fs::path midi_of(std::string const& name)
   {
      return fs::path{MIDI_DIR} / (name + ".mid");
   }
}

int main(int argc, char const* argv[])
{
   q::formant_table table;
   if (argc > 2)
   {
      take t;
      if (!load(argv[1], argv[2], t))
         return 1;
      auto const learned = learn({argv[1]}, table);
      play(t, learned ? &table : nullptr);
      return 0;
   }

   auto const names = shipped();
   std::vector<fs::path> files;
   for (auto const& name : names)
      files.push_back(audio_of(name));
   auto const learned = learn(files, table);
   auto formants = learned;
   if (!learned)
      std::cout << "nothing to learn the formants from\n";

   while (true)
   {
      std::cout << '\n';
      for (std::size_t i = 0; i != names.size(); ++i)
         std::cout << "  " << i + 1 << ": " << names[i] << '\n';
      std::cout
         << "  f: formant preservation (" << (formants ? "on" : "off")
         << ")\n  q: quit\nChoose: ";

      std::string in;
      if (!std::getline(std::cin, in) || in == "q")
         break;
      if (in == "f")
      {
         formants = learned && !formants;
         continue;
      }
      auto const k = std::atoi(in.c_str());
      if (k >= 1 && k <= int(names.size()))
      {
         take t;
         if (load(audio_of(names[k - 1]), midi_of(names[k - 1]), t))
            play(t, formants ? &table : nullptr);
      }
   }
   return 0;
}
