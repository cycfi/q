/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>

#include <q/midi/clip_reader.hpp>
#include <q/midi/ump_processor.hpp>

#include <cstdint>
#include <initializer_list>
#include <vector>

using namespace cycfi::q::midi_2_0;

namespace
{
   // Packets as words, as a clip file holds them.
   constexpr std::uint32_t dcs(std::uint32_t ticks)  { return 0x00400000 | ticks; }
   constexpr std::uint32_t dctpq(std::uint32_t tpq)  { return 0x00300000 | tpq; }

   // Start and End of Clip: stream messages, 128 bits.
   std::vector<std::uint32_t> const start_words = {0xF0200000, 0, 0, 0};
   std::vector<std::uint32_t> const end_words = {0xF0210000, 0, 0, 0};

   // Set Tempo, in 10 ns units a quarter: flex data, bank 0, status 0.
   std::vector<std::uint32_t> tempo(std::uint32_t ten_ns)
   {
      return {0xD0100000, ten_ns, 0, 0};
   }

   std::vector<std::uint32_t> on_words(int key)
   {
      return {0x40900000u | (std::uint32_t(key) << 8), 0x80000000u};
   }

   std::vector<std::uint32_t> off_words(int key)
   {
      return {0x40800000u | (std::uint32_t(key) << 8), 0};
   }

   std::vector<std::uint8_t> clip(
      std::initializer_list<std::vector<std::uint32_t>> parts
    , char const* magic = "SMF2CLIP")
   {
      std::vector<std::uint8_t> b(magic, magic + 8);
      for (auto const& part : parts)
      {
         for (auto w : part)
         {
            b.push_back(w >> 24);
            b.push_back(w >> 16);
            b.push_back(w >> 8);
            b.push_back(w);
         }
      }
      return b;
   }

   struct recorder : processor
   {
      using processor::operator();

      void operator()(note_on const& m, std::size_t time)
      {
         ons.push_back({m.key(), time});
      }

      void operator()(note_off const& m, std::size_t time)
      {
         offs.push_back({m.key(), time});
      }

      struct event { int key; std::size_t time; };
      std::vector<event> ons, offs;
   };

   // 480 ticks a quarter, 120 BPM; a note before Start of Clip (header,
   // not played), then a quarter at 120 BPM, a change to 60 BPM, a quarter
   // at 60 BPM.
   std::vector<std::uint8_t> two_tempos()
   {
      return clip({
         {dcs(0), dctpq(480)}, {dcs(0)}, tempo(50000000), on_words(30)
       , {dcs(0)}, start_words
       , on_words(60), {dcs(480)}, off_words(60)
       , tempo(100000000)
       , {dcs(480)}, on_words(62)
       , {dcs(0)}, end_words
      });
   }
}

TEST_CASE("A clip plays its sequence, timed in samples across a tempo change")
{
   auto const b = two_tempos();
   clip_reader reader{b};
   REQUIRE(bool(reader));
   CHECK(reader.ticks_per_quarter() == 480);
   CHECK(reader.duration() == Approx(1.5));

   recorder r;
   reader(48000.0f, r);
   REQUIRE(r.ons.size() == 2);                 // the header's note not played
   CHECK(r.ons[0].key == 60);
   CHECK(r.ons[0].time == 0);
   REQUIRE(r.offs.size() == 1);
   CHECK(r.offs[0].time == 24000);             // a quarter at 120 BPM
   CHECK(r.ons[1].key == 62);
   CHECK(r.ons[1].time == 72000);              // then a quarter at 60 BPM
}

TEST_CASE("A malformed clip reads as nothing")
{
   auto const whole = two_tempos();

   SECTION("no magic")
   {
      auto b = whole;
      b[0] = 'X';
      CHECK(!clip_reader{b});
   }

   SECTION("cut short")
   {
      auto b = whole;
      b.resize(b.size() - 4);                  // the End of Clip loses a word
      CHECK(!clip_reader{b});
   }

   SECTION("no ticks per quarter")
   {
      auto const b = clip({{dcs(0)}, start_words, on_words(60),
                           {dcs(0)}, end_words});
      clip_reader reader{b};
      CHECK(!reader);
      recorder r;
      reader(48000.0f, r);
      CHECK(r.ons.empty());
   }

   SECTION("no End of Clip")
   {
      auto const b = clip({{dcs(0), dctpq(480), dcs(0)}, start_words,
                           on_words(60)});
      CHECK(!clip_reader{b});
   }
}
