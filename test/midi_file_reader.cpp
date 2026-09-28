/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>

#include <q/midi/file_reader.hpp>
#include <cstdint>
#include <string>
#include <vector>

namespace q = cycfi::q;
namespace midi = q::midi_1_0;

namespace
{
   using bytes = std::vector<std::uint8_t>;

   void big(bytes& b, std::uint32_t v, int n)
   {
      for (int i = n - 1; i >= 0; --i)
         b.push_back(std::uint8_t((v >> (8 * i)) & 0xFF));
   }

   // A delta time, seven bits a byte.
   void variable(bytes& b, std::uint32_t v)
   {
      std::uint32_t buffer = v & 0x7F;
      while ((v >>= 7))
      {
         buffer <<= 8;
         buffer |= ((v & 0x7F) | 0x80);
      }
      for (;;)
      {
         b.push_back(std::uint8_t(buffer & 0xFF));
         if (buffer & 0x80)
            buffer >>= 8;
         else
            break;
      }
   }

   void track(bytes& file, bytes const& events)
   {
      file.insert(file.end(), {'M', 'T', 'r', 'k'});
      big(file, std::uint32_t(events.size()), 4);
      file.insert(file.end(), events.begin(), events.end());
   }

   bytes header(int format, int tracks, int division)
   {
      bytes b{'M', 'T', 'h', 'd'};
      big(b, 6, 4);
      big(b, std::uint32_t(format), 2);
      big(b, std::uint32_t(tracks), 2);
      big(b, std::uint32_t(division), 2);
      return b;
   }

   void end_of_track(bytes& b)
   {
      variable(b, 0);
      b.insert(b.end(), {0xFF, 0x2F, 0x00});
   }

   // What a processor saw.
   struct heard
   {
      std::string             what;
      int                     key = 0;
      int                     velocity = 0;
      std::size_t             time = 0;
   };

   struct listener
   {
      void operator()(midi::message_base const&, std::size_t) {}

      void operator()(midi::note_on msg, std::size_t time)
      {
         log.push_back({"on", msg.key(), msg.velocity(), time});
      }

      void operator()(midi::note_off msg, std::size_t time)
      {
         log.push_back({"off", msg.key(), msg.velocity(), time});
      }

      void operator()(midi::control_change msg, std::size_t time)
      {
         log.push_back({"cc", int(msg.controller()), msg.value(), time});
      }

      std::vector<heard> log;
   };

   // One quarter note at 96 ticks, a note on then off.
   bytes one_note_file(int division = 96)
   {
      bytes ev;
      variable(ev, 0);
      ev.insert(ev.end(), {0x90, 60, 100});           // note on, middle C
      variable(ev, std::uint32_t(division));          // a quarter later
      ev.insert(ev.end(), {0x80, 60, 0});             // note off
      end_of_track(ev);

      auto file = header(0, 1, division);
      track(file, ev);
      return file;
   }
}

TEST_CASE("TEST_midi_file_reads_a_note")
{
   auto file = one_note_file();
   midi::file_reader reader{file};

   REQUIRE(bool(reader));
   CHECK(reader.format() == 0);
   CHECK(reader.tracks() == 1);
   CHECK(reader.tempo() == Approx(120.0f));     // the default, 500000 us

   listener heard;
   reader(48000.0f, heard);

   REQUIRE(heard.log.size() == 2);
   CHECK(heard.log[0].what == "on");
   CHECK(heard.log[0].key == 60);
   CHECK(heard.log[0].velocity == 100);
   CHECK(heard.log[0].time == 0);

   // A quarter note at 120 bpm is half a second.
   CHECK(heard.log[1].what == "off");
   CHECK(heard.log[1].time == 24000);
   CHECK(reader.duration() == Approx(0.5));
}

TEST_CASE("TEST_midi_file_applies_tempo")
{
   // The same note under a 60 bpm tempo, so the quarter takes a second.
   bytes ev;
   variable(ev, 0);
   ev.insert(ev.end(), {0xFF, 0x51, 0x03});
   big(ev, 1000000, 3);                            // a second a quarter
   variable(ev, 0);
   ev.insert(ev.end(), {0x90, 60, 100});
   variable(ev, 96);
   ev.insert(ev.end(), {0x80, 60, 0});
   end_of_track(ev);

   auto file = header(0, 1, 96);
   track(file, ev);

   midi::file_reader reader{file};
   REQUIRE(bool(reader));
   CHECK(reader.tempo() == Approx(60.0f));

   listener heard;
   reader(48000.0f, heard);
   REQUIRE(heard.log.size() == 2);
   CHECK(heard.log[1].time == 48000);
   CHECK(reader.duration() == Approx(1.0));
}

TEST_CASE("TEST_midi_file_tempo_change_mid_piece")
{
   // Two quarter notes, the tempo doubling between them, so the second
   // takes half as long as the first.
   bytes ev;
   variable(ev, 0);
   ev.insert(ev.end(), {0x90, 60, 100});
   variable(ev, 96);
   ev.insert(ev.end(), {0xFF, 0x51, 0x03});
   big(ev, 250000, 3);                             // 240 bpm
   variable(ev, 0);
   ev.insert(ev.end(), {0x90, 62, 100});
   variable(ev, 96);
   ev.insert(ev.end(), {0x80, 62, 0});
   end_of_track(ev);

   auto file = header(0, 1, 96);
   track(file, ev);

   midi::file_reader reader{file};
   REQUIRE(bool(reader));

   listener heard;
   reader(48000.0f, heard);
   REQUIRE(heard.log.size() == 3);
   CHECK(heard.log[1].time == 24000);              // half a second in
   CHECK(heard.log[2].time == 36000);              // a quarter second later
}

TEST_CASE("TEST_midi_file_merges_tracks_in_time_order")
{
   // Two tracks, one playing on the beat and one off it. Their notes have
   // to arrive interleaved.
   bytes a;
   variable(a, 0);
   a.insert(a.end(), {0x90, 60, 100});
   variable(a, 192);
   a.insert(a.end(), {0x90, 62, 100});
   end_of_track(a);

   bytes b;
   variable(b, 96);
   b.insert(b.end(), {0x91, 48, 90});
   variable(b, 192);
   b.insert(b.end(), {0x91, 50, 90});
   end_of_track(b);

   auto file = header(1, 2, 96);
   track(file, a);
   track(file, b);

   midi::file_reader reader{file};
   REQUIRE(bool(reader));
   CHECK(reader.tracks() == 2);

   listener heard;
   reader(48000.0f, heard);

   REQUIRE(heard.log.size() == 4);
   CHECK(heard.log[0].key == 60);
   CHECK(heard.log[1].key == 48);
   CHECK(heard.log[2].key == 62);
   CHECK(heard.log[3].key == 50);
   for (std::size_t i = 1; i != heard.log.size(); ++i)
      CHECK(heard.log[i-1].time <= heard.log[i].time);
}

TEST_CASE("TEST_midi_file_running_status")
{
   // A run of note ons with the status byte written once, which is how a
   // sequencer writes a chord.
   bytes ev;
   variable(ev, 0);
   ev.insert(ev.end(), {0x90, 60, 100});
   variable(ev, 0);
   ev.insert(ev.end(), {64, 100});                 // no status byte
   variable(ev, 0);
   ev.insert(ev.end(), {67, 100});
   end_of_track(ev);

   auto file = header(0, 1, 96);
   track(file, ev);

   midi::file_reader reader{file};
   listener heard;
   reader(48000.0f, heard);

   REQUIRE(heard.log.size() == 3);
   CHECK(heard.log[0].key == 60);
   CHECK(heard.log[1].key == 64);
   CHECK(heard.log[2].key == 67);
   for (auto const& h : heard.log)
   {
      CHECK(h.what == "on");
      CHECK(h.time == 0);
   }
}

TEST_CASE("TEST_midi_file_smpte_division")
{
   // 25 frames a second, 40 ticks a frame, so 1000 ticks a second and the
   // tempo has no say in it.
   bytes ev;
   variable(ev, 0);
   ev.insert(ev.end(), {0x90, 60, 100});
   variable(ev, 500);
   ev.insert(ev.end(), {0x80, 60, 0});
   end_of_track(ev);

   auto file = header(0, 1, (((256 - 25) & 0xFF) << 8) | 40);
   track(file, ev);

   midi::file_reader reader{file};
   REQUIRE(bool(reader));

   listener heard;
   reader(48000.0f, heard);
   REQUIRE(heard.log.size() == 2);
   CHECK(heard.log[1].time == 24000);              // half a second
}

TEST_CASE("TEST_midi_file_control_change_and_sysex")
{
   bytes ev;
   variable(ev, 0);
   ev.insert(ev.end(), {0xB0, 7, 90});             // volume
   variable(ev, 0);
   ev.insert(ev.end(), {0xF0, 0x04, 0x43, 0x10, 0x01, 0xF7});
   variable(ev, 96);
   ev.insert(ev.end(), {0xB0, 7, 20});
   end_of_track(ev);

   auto file = header(0, 1, 96);
   track(file, ev);

   midi::file_reader reader{file};
   listener heard;
   reader(48000.0f, heard);

   REQUIRE(heard.log.size() == 2);
   CHECK(heard.log[0].what == "cc");
   CHECK(heard.log[0].key == 7);
   CHECK(heard.log[0].velocity == 90);
   CHECK(heard.log[1].time == 24000);
}

TEST_CASE("TEST_midi_file_rejects_rubbish")
{
   SECTION("empty")
   {
      bytes b;
      CHECK(!bool(midi::file_reader{b}));
   }

   SECTION("not a midi file")
   {
      bytes b{'R', 'I', 'F', 'F', 0, 0, 0, 4, 1, 2, 3, 4};
      CHECK(!bool(midi::file_reader{b}));
   }

   SECTION("header cut short")
   {
      auto b = header(0, 1, 96);
      b.resize(8);
      CHECK(!bool(midi::file_reader{b}));
   }

   SECTION("no division")
   {
      auto file = header(0, 1, 0);
      bytes ev;
      end_of_track(ev);
      track(file, ev);
      CHECK(!bool(midi::file_reader{file}));
   }

   SECTION("a track claiming more than the file holds")
   {
      auto file = header(0, 1, 96);
      file.insert(file.end(), {'M', 'T', 'r', 'k'});
      big(file, 9999, 4);
      file.insert(file.end(), {0x00, 0x90, 60});
      CHECK(!bool(midi::file_reader{file}));
   }

   SECTION("an event running past the end of its track")
   {
      // The track ends in the middle of a note on, with no end of track.
      auto file = header(0, 1, 96);
      bytes ev;
      variable(ev, 0);
      ev.insert(ev.end(), {0x90, 60});             // a byte missing
      track(file, ev);

      midi::file_reader reader{file};
      listener heard;
      reader(48000.0f, heard);                     // must not read past it
      CHECK(heard.log.empty());
   }

   SECTION("a delta time that never ends")
   {
      auto file = header(0, 1, 96);
      bytes ev{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x90, 60, 100};
      track(file, ev);

      midi::file_reader reader{file};
      listener heard;
      reader(48000.0f, heard);
      CHECK(heard.log.empty());
   }
}
