/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
// The packets that are about the stream rather than about music.
// M2-104-UM version 1.1.2: section 7.2 and Figures 26, 27, 29, 31, 33 and
// 34 for the utility messages, message type 0x0; section 4.6, Figure 34 and
// Table 20 for the mixed data set, which shares message type 0x5 with
// system exclusive in its 8 bit form.
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/midi/packet_reader.hpp>
#include <q/midi/ump_utility.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace q = cycfi::q;
namespace midi = q::midi_1_0;
namespace midi2 = q::midi_2_0;

namespace
{
   using bytes = std::vector<std::uint8_t>;

   // A built message back as the packet a reader takes.
   template <typename Message>
   midi2::packet as_packet(Message const& m)
   {
      return midi2::packet{m.word(0)};
   }

   struct recorder : midi2::processor
   {
      using midi2::processor::operator();

      void operator()(midi2::noop, std::size_t)
      {
         _seen.push_back("noop");
      }

      void operator()(midi2::jr_clock m, std::size_t)
      {
         _seen.push_back("jr_clock");
         _ticks = m.ticks();
      }

      void operator()(midi2::jr_timestamp m, std::size_t)
      {
         _seen.push_back("jr_timestamp");
         _ticks = m.ticks();
      }

      void operator()(midi2::ticks_per_quarter_note m, std::size_t)
      {
         _seen.push_back("ticks_per_quarter_note");
         _ticks = m.ticks();
      }

      void operator()(midi2::delta_clockstamp m, std::size_t)
      {
         _seen.push_back("delta_clockstamp");
         _delta = m.ticks();
      }

      void operator()(midi2::mds_header m, std::size_t)
      {
         _seen.push_back("mds_header");
         _mds_id = m.mds_id();
         _values = {
            m.valid_bytes(), m.chunks(), m.chunk(), m.manufacturer()
          , m.device(), m.sub_id_1(), m.sub_id_2()};
      }

      void operator()(midi2::mds_payload m, std::size_t)
      {
         _seen.push_back("mds_payload");
         _mds_id = m.mds_id();
         for (std::size_t i = 0; i != midi2::mds_payload::size; ++i)
            _payload.push_back(m[i]);
      }

      void operator()(midi::sysex_view m, std::size_t)
      {
         _seen.push_back("sysex");
         _sysex.assign(m.data().begin(), m.data().end());
      }

      void operator()(midi2::sysex8_view m, std::size_t)
      {
         _seen.push_back("sysex8");
         _sysex.assign(m.data().begin(), m.data().end());
      }

      std::vector<std::string>   _seen;
      std::vector<std::uint32_t> _values;
      bytes                      _payload;
      bytes                      _sysex;
      std::uint8_t               _mds_id = 0xFF;
      std::uint16_t              _ticks = 0;
      std::uint32_t              _delta = 0;
   };
}

TEST_CASE("Figure 27 A no-op says nothing and is not an error")
{
   midi2::noop const msg;

   CHECK(msg.word(0) == 0x00000000u);
   CHECK(msg.message_type() == midi2::message_type::utility);
   CHECK(msg.status() == midi2::utility_status::noop);
}

TEST_CASE("Figure 26 A utility message has no group, only reserved bits")
{
   // Version 1.0 put a group here and 1.1 took it away, so nothing Q
   // builds may write anything into it.
   CHECK(midi2::noop{}.group() == 0);
   CHECK(midi2::jr_clock{0xFFFF}.group() == 0);
   CHECK(midi2::jr_timestamp{0xFFFF}.group() == 0);
   CHECK(midi2::ticks_per_quarter_note{0xFFFF}.group() == 0);
   CHECK(midi2::delta_clockstamp{0xFFFFF}.group() == 0);
}

TEST_CASE("Figure 29 A jitter reduction clock, word for word")
{
   midi2::jr_clock const msg{0x1234};

   CHECK(msg.word(0) == 0x00101234u);
   CHECK(msg.status() == midi2::utility_status::jr_clock);
   CHECK(msg.ticks() == 0x1234);
}

TEST_CASE("Figure 31 A jitter reduction timestamp, word for word")
{
   midi2::jr_timestamp const msg{0xABCD};

   CHECK(msg.word(0) == 0x0020ABCDu);
   CHECK(msg.status() == midi2::utility_status::jr_timestamp);
   CHECK(msg.ticks() == 0xABCD);
}

TEST_CASE("Figure 33 Ticks per quarter note sets the unit of a clip file")
{
   // 7.2.3.1: "may have a value of 1 to 65,535 (0 = Reserved)".
   midi2::ticks_per_quarter_note const msg{960};

   CHECK(msg.word(0) == 0x003003C0u);
   CHECK(msg.status() == midi2::utility_status::ticks_per_quarter_note);
   CHECK(msg.ticks() == 960);
}

TEST_CASE("Figure 34 A delta clockstamp counts twenty bits of ticks")
{
   midi2::delta_clockstamp const msg{0xFFFFF};

   CHECK(msg.word(0) == 0x004FFFFFu);
   CHECK(msg.status() == midi2::utility_status::delta_clockstamp);
   CHECK(msg.ticks() == 0xFFFFF);
   CHECK(msg.ticks() == midi2::delta_clockstamp::max_ticks);

   // The count is twenty bits, not sixteen, so it reaches past a jitter
   // reduction tick count.
   midi2::delta_clockstamp const big{0x12345};
   CHECK(big.ticks() == 0x12345);
}

TEST_CASE("7.2.2.1 A tick is one thirty one thousand two hundred and fiftieth")
{
   // "clock ticks of 1/31250 of one second (32 microseconds)".
   CHECK(midi2::jr_message::seconds_per_tick == 1.0 / 31250.0);
   CHECK(midi2::jr_message::seconds_per_tick == Approx(0.000032));
}

TEST_CASE("7.2 The utility messages reach a processor")
{
   midi2::packet_reader<> reader;
   recorder rec;

   reader(as_packet(midi2::noop{}), 0, rec);
   reader(as_packet(midi2::jr_clock{100}), 1, rec);
   reader(as_packet(midi2::jr_timestamp{200}), 2, rec);
   reader(as_packet(midi2::ticks_per_quarter_note{480}), 3, rec);
   reader(as_packet(midi2::delta_clockstamp{12345}), 4, rec);

   REQUIRE(rec._seen
      == std::vector<std::string>{
            "noop", "jr_clock", "jr_timestamp"
          , "ticks_per_quarter_note", "delta_clockstamp"});
   CHECK(rec._ticks == 480);
   CHECK(rec._delta == 12345);
}

TEST_CASE("7.2 An unknown utility status is passed over, not guessed at")
{
   // More may be defined; a reader from today must not invent a meaning.
   midi2::packet_reader<> reader;
   recorder rec;

   reader({0x00700000u}, 0, rec);
   CHECK(rec._seen.empty());
}

TEST_CASE("7.2 A utility message between sysex packets changes nothing")
{
   midi2::packet_reader<> reader;
   recorder rec;

   // A sysex7 start, a clock in the middle, then the end.
   reader({0x30160102u, 0x03040506u}, 0, rec);      // start, six bytes
   reader(as_packet(midi2::jr_clock{42}), 1, rec);
   reader({0x30320708u, 0u}, 2, rec);               // end, two bytes

   REQUIRE(rec._seen == std::vector<std::string>{"jr_clock", "sysex"});
   CHECK(rec._sysex == bytes{1, 2, 3, 4, 5, 6, 7, 8});
}

TEST_CASE("4.6 A mixed data set header carries the shape of the data set")
{
   midi2::packet_reader<> reader;
   recorder rec;

   reader(
      {0x52830040u, 0x00020001u, 0x0021FFFFu, 0x00010002u}, 0, rec);

   REQUIRE(rec._seen == std::vector<std::string>{"mds_header"});
   CHECK(rec._mds_id == 3);
   CHECK(rec._values
      == std::vector<std::uint32_t>{0x40, 2, 1, 0x0021, 0xFFFF, 1, 2});
}

TEST_CASE("Table 20 A payload packet is fourteen bytes, from byte three")
{
   midi2::packet_reader<> reader;
   recorder rec;

   reader(
      {0x52930102u, 0x03040506u, 0x0708090Au, 0x0B0C0D0Eu}, 0, rec);

   REQUIRE(rec._seen == std::vector<std::string>{"mds_payload"});
   CHECK(rec._mds_id == 3);
   CHECK(rec._payload
      == bytes{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14});
}

TEST_CASE("4.6 A mixed data set is not gathered like a system exclusive")
{
   // A firmware image does not belong in a reader's buffer, so each
   // packet arrives on its own and the application assembles it.
   midi2::packet_reader<> reader;
   recorder rec;

   reader({0x52830020u, 0x00010001u, 0x0021FFFFu, 0u}, 0, rec);
   reader({0x52930102u, 0u, 0u, 0u}, 1, rec);
   reader({0x52930304u, 0u, 0u, 0u}, 2, rec);

   CHECK(rec._seen
      == std::vector<std::string>{
            "mds_header", "mds_payload", "mds_payload"});
   CHECK(rec._sysex.empty());
   CHECK(reader.drops() == 0);
}

TEST_CASE("4.5 The 8 bit system exclusive form still reaches its own reader")
{
   // Both share message type 0x5, and only the status tells them apart.
   midi2::packet_reader<> reader;
   recorder rec;

   reader({0x5005007Eu, 0x00FF8000u, 0u, 0u}, 0, rec);

   REQUIRE(rec._seen == std::vector<std::string>{"sysex8"});
   CHECK(rec._sysex == bytes{0x7E, 0x00, 0xFF, 0x80});
}
