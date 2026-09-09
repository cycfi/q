/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/midi/byte_reader.hpp>

#include <cstdint>
#include <initializer_list>
#include <vector>

namespace q = cycfi::q;
namespace midi = q::midi_1_0;

namespace
{
   struct recorder : midi::processor
   {
      using midi::processor::operator();

      struct note
      {
         bool           on;
         std::uint8_t   channel;
         std::uint8_t   key;
         std::uint8_t   velocity;
         std::size_t    time;
      };

      void operator()(midi::note_on msg, std::size_t time)
      {
         _notes.push_back(
            {true, msg.channel(), msg.key(), msg.velocity(), time});
      }

      void operator()(midi::note_off msg, std::size_t time)
      {
         _notes.push_back(
            {false, msg.channel(), msg.key(), msg.velocity(), time});
      }

      void operator()(midi::program_change msg, std::size_t)
      {
         _programs.push_back(msg.channel());
      }

      void operator()(midi::song_position msg, std::size_t)
      {
         _positions.push_back(msg.position());
      }

      void operator()(midi::timing_tick, std::size_t time)
      {
         _ticks.push_back(time);
      }

      void operator()(midi::sysex_view msg, std::size_t)
      {
         // The payload is only valid for the duration of this call, so it
         // is copied rather than kept.
         _sysex.push_back({msg.data().begin(), msg.data().end()});
         _manufacturers.push_back(msg.manufacturer());
      }

      std::vector<note>                         _notes;
      std::vector<std::uint8_t>                 _programs;
      std::vector<std::uint16_t>                _positions;
      std::vector<std::size_t>                  _ticks;
      std::vector<std::vector<std::uint8_t>>    _sysex;
      std::vector<std::uint32_t>                _manufacturers;
   };

   struct fixture
   {
      void feed(std::initializer_list<int> bytes, std::size_t time = 0)
      {
         std::vector<std::uint8_t> buf;
         for (auto b : bytes)
            buf.push_back(std::uint8_t(b));
         _reader(
            std::span<std::uint8_t const>{buf.data(), buf.size()}, time, _rec);
      }

      // The same bytes, one call per byte, to prove the reader does not care
      // where a buffer boundary falls.
      void feed_singly(std::initializer_list<int> bytes, std::size_t time = 0)
      {
         for (auto b : bytes)
         {
            std::uint8_t const one = std::uint8_t(b);
            _reader(std::span<std::uint8_t const>{&one, 1}, time, _rec);
         }
      }

      midi::byte_reader<>  _reader;
      recorder             _rec;
   };
}

TEST_CASE("A whole message is assembled from its bytes")
{
   fixture f;
   f.feed({0x90, 0x3C, 0x40}, 99);

   REQUIRE(f._rec._notes.size() == 1);
   auto const& n = f._rec._notes.front();
   CHECK(n.on);
   CHECK(n.channel == 0);
   CHECK(n.key == 60);
   CHECK(n.velocity == 64);
   CHECK(n.time == 99);
}

TEST_CASE("A message split across buffers is still one message")
{
   // A serial port hands over whatever has arrived, which is not a message.
   fixture f;
   f.feed({0x90, 0x3C});
   CHECK(f._rec._notes.empty());

   f.feed({0x40});
   CHECK(f._rec._notes.size() == 1);
}

TEST_CASE("Byte at a time gives the same result")
{
   fixture f;
   f.feed_singly({0x90, 0x3C, 0x40, 0x80, 0x3C, 0x00});

   REQUIRE(f._rec._notes.size() == 2);
   CHECK(f._rec._notes[0].on);
   CHECK_FALSE(f._rec._notes[1].on);
}

TEST_CASE("Running status repeats the last status byte")
{
   // A keyboard playing a chord sends the status once, then pairs of data
   // bytes. This is the common case on a busy wire, not an oddity.
   fixture f;
   f.feed({0x90, 0x3C, 0x40, 0x3E, 0x40, 0x40, 0x40});

   REQUIRE(f._rec._notes.size() == 3);
   for (auto const& n : f._rec._notes)
   {
      CHECK(n.on);
      CHECK(n.velocity == 64);
   }
   CHECK(f._rec._notes[0].key == 60);
   CHECK(f._rec._notes[1].key == 62);
   CHECK(f._rec._notes[2].key == 64);
}

TEST_CASE("Running status works for two byte messages too")
{
   fixture f;
   f.feed({0xC3, 0x07, 0x08});

   CHECK(f._rec._programs.size() == 2);
}

TEST_CASE("A real time byte interrupts without disturbing anything")
{
   // Clock bytes are inserted wherever they fall due, including between the
   // data bytes of another message. They carry no data of their own, so the
   // message in progress must survive one.
   fixture f;
   f.feed({0x90, 0x3C, 0xF8, 0x40});

   CHECK(f._rec._ticks.size() == 1);
   REQUIRE(f._rec._notes.size() == 1);
   CHECK(f._rec._notes.front().key == 60);
   CHECK(f._rec._notes.front().velocity == 64);
}

TEST_CASE("A real time byte does not break running status")
{
   fixture f;
   f.feed({0x90, 0x3C, 0x40, 0xF8, 0x3E, 0x40});

   CHECK(f._rec._ticks.size() == 1);
   CHECK(f._rec._notes.size() == 2);
}

TEST_CASE("System common clears running status")
{
   // Unlike real time, a system common message ends the run: the data bytes
   // after it belong to nothing until a new status arrives.
   fixture f;
   f.feed({0x90, 0x3C, 0x40, 0xF2, 0x00, 0x10, 0x3E, 0x40});

   CHECK(f._rec._notes.size() == 1);
   CHECK(f._rec._positions.size() == 1);
}

TEST_CASE("Data bytes with no status are dropped")
{
   fixture f;
   f.feed({0x3C, 0x40, 0x3E});

   CHECK(f._rec._notes.empty());
}

TEST_CASE("An unfinished message is abandoned when the next one starts")
{
   // The first note never got its velocity. Keeping the fragment would put
   // the following message's data into it.
   fixture f;
   f.feed({0x90, 0x3C, 0x80, 0x3E, 0x00});

   REQUIRE(f._rec._notes.size() == 1);
   CHECK_FALSE(f._rec._notes.front().on);
   CHECK(f._rec._notes.front().key == 62);
}

TEST_CASE("A sysex is assembled between its markers")
{
   // A universal non real time identity request.
   fixture f;
   f.feed({0xF0, 0x7E, 0x00, 0x06, 0x01, 0xF7});

   REQUIRE(f._rec._sysex.size() == 1);
   CHECK(f._rec._sysex.front() == std::vector<std::uint8_t>{0x7E, 0x00, 0x06, 0x01});
   CHECK(f._rec._manufacturers.front() == 0x7E);
}

TEST_CASE("A three byte manufacturer identifier is read whole")
{
   // A leading zero means the identifier is three bytes, not one.
   fixture f;
   f.feed({0xF0, 0x00, 0x21, 0x09, 0x01, 0x02, 0xF7});

   REQUIRE(f._rec._manufacturers.size() == 1);
   CHECK(f._rec._manufacturers.front() == 0x002109);
}

TEST_CASE("A sysex split across buffers is one message")
{
   fixture f;
   f.feed({0xF0, 0x43, 0x10});
   CHECK(f._rec._sysex.empty());

   f.feed({0x4C, 0xF7});
   REQUIRE(f._rec._sysex.size() == 1);
   CHECK(f._rec._sysex.front().size() == 3);
}

TEST_CASE("Real time bytes inside a sysex pass through it")
{
   fixture f;
   f.feed({0xF0, 0x43, 0xF8, 0x10, 0xF7});

   CHECK(f._rec._ticks.size() == 1);
   REQUIRE(f._rec._sysex.size() == 1);
   CHECK(f._rec._sysex.front() == std::vector<std::uint8_t>{0x43, 0x10});
}

TEST_CASE("A status byte ends an unterminated sysex")
{
   // Some devices simply stop sending. The specification allows any status
   // byte to end a sysex, and the message that follows must still parse.
   fixture f;
   f.feed({0xF0, 0x43, 0x10, 0x90, 0x3C, 0x40});

   REQUIRE(f._rec._sysex.size() == 1);
   CHECK(f._rec._sysex.front() == std::vector<std::uint8_t>{0x43, 0x10});
   REQUIRE(f._rec._notes.size() == 1);
   CHECK(f._rec._notes.front().key == 60);
}

TEST_CASE("A sysex too long for the buffer is dropped, not truncated")
{
   // A truncated sysex is a different message from the one that was sent,
   // and acting on it could mean writing the wrong patch to a synth.
   midi::byte_reader<8> reader;
   recorder rec;

   std::vector<std::uint8_t> bytes{0xF0};
   for (int i = 0; i != 32; ++i)
      bytes.push_back(0x01);
   bytes.push_back(0xF7);

   reader(std::span<std::uint8_t const>{bytes.data(), bytes.size()}, 0, rec);

   CHECK(rec._sysex.empty());
   CHECK(reader.drops() == 1);

   // And the reader recovers: the next message parses.
   std::vector<std::uint8_t> const note{0x90, 0x3C, 0x40};
   reader(std::span<std::uint8_t const>{note.data(), note.size()}, 0, rec);
   CHECK(rec._notes.size() == 1);
}

TEST_CASE("An empty sysex is still a message")
{
   fixture f;
   f.feed({0xF0, 0xF7});

   REQUIRE(f._rec._sysex.size() == 1);
   CHECK(f._rec._sysex.front().empty());
   CHECK(f._rec._manufacturers.front() == 0);
}

TEST_CASE("A sysex built by q is read back by q")
{
   // The builder came from Nexus, where it was written to send messages to
   // instruments. The reader was written here to receive them. This is the
   // assertion that the two agree on the wire format.
   fixture f;

   std::uint8_t const payload[] = {0x10, 0x4C, 0x7F, 0x00};
   midi::sysex<4> const msg{0x2109, payload};

   CHECK(msg.id() == 0x2109);
   CHECK(msg.size == 9);         // marker, three identifier bytes, four, end

   std::vector<std::uint8_t> bytes{msg.data, msg.data + msg.size};
   f._reader(
      std::span<std::uint8_t const>{bytes.data(), bytes.size()}, 0, f._rec);

   REQUIRE(f._rec._sysex.size() == 1);

   // What the reader saw: the identifier, then the payload.
   CHECK(f._rec._sysex.front() ==
      std::vector<std::uint8_t>{0x00, 0x21, 0x09, 0x10, 0x4C, 0x7F, 0x00});
   CHECK(f._rec._manufacturers.front() == 0x2109);
}

TEST_CASE("A payload byte with its top bit set is masked, not sent raw")
{
   // An eighth bit would read as a status byte and end the message early.
   fixture f;

   std::uint8_t const payload[] = {0xFF, 0x90};
   midi::sysex<2> const msg{0x7E, payload};

   std::vector<std::uint8_t> bytes{msg.data, msg.data + msg.size};
   f._reader(
      std::span<std::uint8_t const>{bytes.data(), bytes.size()}, 0, f._rec);

   REQUIRE(f._rec._sysex.size() == 1);
   CHECK(f._rec._sysex.front() ==
      std::vector<std::uint8_t>{0x00, 0x00, 0x7E, 0x7F, 0x10});
}
