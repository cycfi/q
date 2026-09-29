/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
// Flex data, message type 0xD: what a score knows and a note does not.
// M2-104-UM version 1.1.2, section 7.5 and Figures 69 to 82. The setup
// bank is read here word for word; the two text banks are gathered by
// packet_reader and read back as whole strings.
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/midi/packet_reader.hpp>
#include <q/midi/ump_flex.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace q = cycfi::q;
namespace midi2 = q::midi_2_0;

namespace
{
   struct recorder : midi2::processor
   {
      using midi2::processor::operator();

      void operator()(midi2::set_tempo m, std::size_t)
      {
         _seen.push_back("set_tempo");
         _values = {m.ten_nanoseconds()};
      }

      void operator()(midi2::set_time_signature m, std::size_t)
      {
         _seen.push_back("set_time_signature");
         _values = {m.numerator(), m.denominator(), m.thirty_seconds()};
      }

      void operator()(midi2::set_metronome m, std::size_t)
      {
         _seen.push_back("set_metronome");
         _values = {
            m.clocks_per_click(), m.accent_1(), m.accent_2(), m.accent_3()
          , m.subdivision_1(), m.subdivision_2()};
      }

      void operator()(midi2::set_key_signature m, std::size_t)
      {
         _seen.push_back("set_key_signature");
         _sharps = m.sharps_or_flats();
         _values = {m.note()};
      }

      void operator()(midi2::set_chord_name m, std::size_t)
      {
         _seen.push_back("set_chord_name");
         _values = {m.tonic_note(), m.type()};
      }

      void operator()(midi2::flex_text_view m, std::size_t)
      {
         _seen.push_back("flex_text");
         _text = std::string{m.text()};
         _values = {m.group(), m.address(), m.channel()
                  , m.status_bank(), m.status()};
      }

      std::vector<std::string>   _seen;
      std::vector<std::uint32_t> _values;
      std::string                _text;
      std::int8_t                _sharps = 0;
   };

   template <typename Message>
   midi2::packet as_packet(Message const& m)
   {
      return midi2::packet{m.word(0), m.word(1), m.word(2), m.word(3)};
   }

   // A text packet built by hand, the way the figures lay one out.
   midi2::packet text_packet(
      std::string_view text, std::uint8_t form, std::uint8_t bank
    , std::uint8_t status, std::uint8_t group = 0)
   {
      std::uint32_t w[4] = {
         (std::uint32_t(midi2::message_type::flex_data) << 28)
            | (std::uint32_t(group) << 24)
            | (std::uint32_t(form) << 22)
            | (std::uint32_t(midi2::flex_address::group) << 20)
            | (std::uint32_t(bank) << 8) | status
       , 0, 0, 0};

      for (std::size_t i = 0; i != 12 && i != text.size(); ++i)
         w[1 + i/4] |= std::uint32_t(std::uint8_t(text[i])) << (24 - (i%4)*8);

      return midi2::packet{w[0], w[1], w[2], w[3]};
   }
}

TEST_CASE("Figure 70 The tempo is how long a quarter note lasts")
{
   // 7.5.3: "The time per quarter note is 32 bits in units of 10
   // Nanoseconds." Half a second is 50,000,000 of them, which is 120 bpm.
   midi2::set_tempo const msg{50000000};

   CHECK(msg.word(0) == 0xD0100000u);
   CHECK(msg.word(1) == 50000000u);
   CHECK(msg.ten_nanoseconds() == 50000000u);

   // 7.5.3: the message is sent to the group, complete in one packet.
   CHECK(msg.form() == midi2::flex_format::complete);
   CHECK(msg.address() == midi2::flex_address::group);
   CHECK(msg.status_bank() == midi2::flex_bank::setup);
}

TEST_CASE("Figure 71 The time signature names a power of two below")
{
   // 7.5.4: the denominator is "a value in negative power of 2 (2
   // represents a quarter note, 3 represents an eighth note)".
   midi2::set_time_signature const msg{4, 2, 8};

   CHECK(msg.word(0) == 0xD0100001u);
   CHECK(msg.word(1) == 0x04020800u);
   CHECK(msg.numerator() == 4);
   CHECK(msg.denominator() == 2);
   CHECK(msg.thirty_seconds() == 8);
}

TEST_CASE("Figure 72 The metronome counts in MIDI clocks")
{
   // 7.5.5: 24 clocks to the quarter note, so 36 is a dotted quarter.
   // The accents divide a bar of five as three and two.
   midi2::set_metronome const msg{24, 3, 2, 0, 2};

   CHECK(msg.word(0) == 0xD0100002u);
   CHECK(msg.clocks_per_click() == 24);
   CHECK(msg.accent_1() == 3);
   CHECK(msg.accent_2() == 2);
   CHECK(msg.accent_3() == 0);
   CHECK(msg.subdivision_1() == 2);
   CHECK(msg.subdivision_2() == 0);
}

TEST_CASE("Figure 74 Sharps count up and flats count down")
{
   // 7.5.7: "a 4-bit field with a two's complement signed value".
   midi2::set_key_signature const sharps{2, midi2::tonic::d};
   CHECK(sharps.word(0) == 0xD0100005u);
   CHECK(sharps.word(1) == 0x24000000u);
   CHECK(sharps.sharps_or_flats() == 2);
   CHECK(sharps.note() == midi2::tonic::d);

   midi2::set_key_signature const flats{-3, midi2::tonic::e};
   CHECK(flats.sharps_or_flats() == -3);
   CHECK(flats.note() == midi2::tonic::e);

   midi2::set_key_signature const unknown{
      midi2::set_key_signature::unknown_key, midi2::tonic::unknown};
   CHECK(unknown.sharps_or_flats() == -8);
   CHECK(unknown.note() == midi2::tonic::unknown);
}

TEST_CASE("Figure 76 A chord reads back as its tonic, type and alterations")
{
   // The specification's own worked example: C major 7th with a raised
   // eleventh. Tonic natural C, type 0x03, alteration one raises degree 11.
   midi2::set_chord_name const msg{
      midi2::packet{0xD0100006u, 0x03033B00u, 0u, 0u}};

   CHECK(msg.status() == midi2::flex_status::set_chord_name);
   CHECK(msg.tonic_sharps_or_flats() == 0);
   CHECK(msg.tonic_note() == midi2::tonic::c);
   CHECK(msg.type() == midi2::chord_type::major_7th);

   auto const a1 = msg.alteration(0);
   CHECK(a1.type == midi2::alteration_type::raise);
   CHECK(a1.degree == 11);

   CHECK(msg.alteration(1).type == midi2::alteration_type::none);
   CHECK(msg.alteration(2).type == midi2::alteration_type::none);
   CHECK(msg.alteration(3).type == midi2::alteration_type::none);
}

TEST_CASE("7.5.8 A bass of its own reads back from the last word")
{
   // Tonic C major over an E bass: bass note E, no bass chord.
   midi2::set_chord_name const msg{
      midi2::packet{0xD0100006u, 0x03010000u, 0u, 0x05000000u}};

   CHECK(msg.tonic_note() == midi2::tonic::c);
   CHECK(msg.bass_sharps_or_flats() == 0);
   CHECK(msg.bass_note() == midi2::tonic::e);
   CHECK(msg.bass_type() == midi2::chord_type::clear);
   CHECK(msg.bass_alteration(0).type == midi2::alteration_type::none);
}

TEST_CASE("7.5 The setup bank reaches a processor")
{
   midi2::packet_reader<> reader;
   recorder rec;

   reader(as_packet(midi2::set_tempo{50000000}), 0, rec);
   reader(as_packet(midi2::set_time_signature{3, 2, 8}), 1, rec);
   reader(as_packet(midi2::set_metronome{36, 3}), 2, rec);
   reader(as_packet(midi2::set_key_signature{-2, midi2::tonic::b}), 3, rec);

   CHECK(rec._seen
      == std::vector<std::string>{
            "set_tempo", "set_time_signature", "set_metronome"
          , "set_key_signature"});
   CHECK(rec._sharps == -2);
}

TEST_CASE("7.5 An unknown setup status is passed over, not guessed at")
{
   midi2::packet_reader<> reader;
   recorder rec;

   // Status 0x03 is not defined in this version.
   reader(midi2::packet{0xD0100003u, 0u, 0u, 0u}, 0, rec);
   CHECK(rec._seen.empty());
}

TEST_CASE("7.5.9 Text that fits one packet arrives whole")
{
   midi2::packet_reader<> reader;
   recorder rec;

   reader(
      text_packet(
         "Hello", midi2::flex_format::complete, midi2::flex_bank::metadata
       , midi2::metadata_status::composition)
    , 0, rec);

   REQUIRE(rec._seen == std::vector<std::string>{"flex_text"});
   CHECK(rec._text == "Hello");
   CHECK(rec._values[3] == midi2::flex_bank::metadata);
   CHECK(rec._values[4] == midi2::metadata_status::composition);
}

TEST_CASE("7.5.9 The padding a short packet ends with is not the text")
{
   // "If the text ends in the middle of a UMP, then the remaining data
   // bytes shall be set to 0x00 to indicate the end of the text."
   midi2::packet_reader<> reader;
   recorder rec;

   reader(
      text_packet(
         "abc", midi2::flex_format::complete, midi2::flex_bank::metadata
       , midi2::metadata_status::composer)
    , 0, rec);

   CHECK(rec._text == "abc");
   CHECK(rec._text.size() == 3);
}

TEST_CASE("7.5.9 Longer text is gathered across packets")
{
   midi2::packet_reader<> reader;
   recorder rec;

   reader(
      text_packet(
         "123456789012", midi2::flex_format::start
       , midi2::flex_bank::metadata, midi2::metadata_status::copyright)
    , 0, rec);
   CHECK(rec._seen.empty());

   reader(
      text_packet(
         "345678901234", midi2::flex_format::continue_
       , midi2::flex_bank::metadata, midi2::metadata_status::copyright)
    , 1, rec);
   CHECK(rec._seen.empty());

   reader(
      text_packet(
         "end", midi2::flex_format::end
       , midi2::flex_bank::metadata, midi2::metadata_status::copyright)
    , 2, rec);

   REQUIRE(rec._seen == std::vector<std::string>{"flex_text"});
   CHECK(rec._text == "123456789012345678901234end");
}

TEST_CASE("7.5.9 UTF-8 survives, since the text is bytes and not characters")
{
   midi2::packet_reader<> reader;
   recorder rec;

   // The copyright sign, 0xC2 0xA9, which the specification's own example
   // uses.
   reader(
      text_packet(
         "\xC2\xA9 2026", midi2::flex_format::complete
       , midi2::flex_bank::metadata, midi2::metadata_status::copyright)
    , 0, rec);

   CHECK(rec._text == "\xC2\xA9 2026");
}

TEST_CASE("7.5.10 A lyric carries its bank and status, not just its text")
{
   midi2::packet_reader<> reader;
   recorder rec;

   reader(
      text_packet(
         "la", midi2::flex_format::complete
       , midi2::flex_bank::performance, midi2::performance_status::lyric, 3)
    , 0, rec);

   CHECK(rec._text == "la");
   CHECK(rec._values[0] == 3);                     // group
   CHECK(rec._values[3] == midi2::flex_bank::performance);
   CHECK(rec._values[4] == midi2::performance_status::lyric);
}

TEST_CASE("7.5.1 Text longer than a message may be is dropped and counted")
{
   // "A Flex Data Message shall not be larger than 32 UMPs", which is 384
   // bytes. One byte more is not truncated, it is discarded.
   midi2::packet_reader<> reader;
   recorder rec;

   reader(
      text_packet(
         "start1234567", midi2::flex_format::start
       , midi2::flex_bank::metadata, midi2::metadata_status::project)
    , 0, rec);

   for (int i = 0; i != 40; ++i)
      reader(
         text_packet(
            "123456789012", midi2::flex_format::continue_
          , midi2::flex_bank::metadata, midi2::metadata_status::project)
       , 1, rec);

   CHECK(rec._seen.empty());
   CHECK(reader.drops() != 0);
}

TEST_CASE("7.5.9 A new message does not carry what an abandoned one left")
{
   midi2::packet_reader<> reader;
   recorder rec;

   // A start that never ends, then a message of its own.
   reader(
      text_packet(
         "abandoned123", midi2::flex_format::start
       , midi2::flex_bank::metadata, midi2::metadata_status::project)
    , 0, rec);
   reader(
      text_packet(
         "fresh", midi2::flex_format::complete
       , midi2::flex_bank::metadata, midi2::metadata_status::composer)
    , 1, rec);

   REQUIRE(rec._seen == std::vector<std::string>{"flex_text"});
   CHECK(rec._text == "fresh");
   CHECK(rec._values[4] == midi2::metadata_status::composer);
}

TEST_CASE("7.5.9 A continue with no start before it is ignored")
{
   midi2::packet_reader<> reader;
   recorder rec;

   reader(
      text_packet(
         "orphan", midi2::flex_format::continue_
       , midi2::flex_bank::metadata, midi2::metadata_status::project)
    , 0, rec);

   CHECK(rec._seen.empty());
}
