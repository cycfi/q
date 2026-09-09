/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
// The packet streams, end to end through the operating system: a virtual
// port at one end, a port opened from the listing at the other, and Q on
// both sides. The counterpart of midi_loopback for MIDI 2.0.
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q_io/midi2_stream.hpp>
#include <q_io/midi_device.hpp>
#include <q/midi/packet_writer.hpp>

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace q = cycfi::q;
namespace midi = q::midi_1_0;
namespace midi2 = q::midi_2_0;

namespace
{
   char const* port_name = "Q packet loopback";

   // Every message the stream delivered, in the order it delivered them.
   struct recorder : midi2::processor
   {
      using midi2::processor::operator();

      struct entry
      {
         int                        kind;
         std::uint8_t               channel;
         std::uint8_t               key;
         std::uint32_t              value;
         std::size_t                time;
      };

      void operator()(midi2::note_on msg, std::size_t time)
      {
         _entries.push_back(
            {0x90, msg.channel(), msg.key(), msg.velocity(), time});
      }

      void operator()(midi2::note_off msg, std::size_t time)
      {
         _entries.push_back(
            {0x80, msg.channel(), msg.key(), msg.velocity(), time});
      }

      void operator()(midi2::control_change msg, std::size_t time)
      {
         _entries.push_back(
            {0xB0, msg.channel(), msg.controller(), msg.value(), time});
      }

      void operator()(midi::note_on msg, std::size_t time)
      {
         _entries.push_back(
            {0x190, msg.channel(), msg.key(), msg.velocity(), time});
      }

      void operator()(midi::sysex_view msg, std::size_t)
      {
         _sysex.assign(msg.data().begin(), msg.data().end());
      }

      std::vector<entry>         _entries;
      std::vector<std::uint8_t>  _sysex;
   };

   // The device a virtual port shows up as, in the packet listing.
   std::optional<q::midi_device> find_loopback(bool input)
   {
      for (auto const& device :
         q::midi_device::list(q::midi_device::midi_2_0))
      {
         auto const fits = input?
            device.num_inputs() != 0 : device.num_outputs() != 0;
         if (fits && device.name().find(port_name) != std::string::npos)
            return device;
      }
      return {};
   }

   // The port appears asynchronously, so the listing may need a moment.
   std::optional<q::midi_device> wait_for_loopback(bool input)
   {
      std::optional<q::midi_device> device;
      for (int i = 0; i != 40 && !device; ++i)
      {
         if (auto const found = find_loopback(input))
            device.emplace(*found);
         else
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
      }
      return device;
   }

   bool pump(q::midi2_input_stream& stream, recorder& rec, std::size_t n)
   {
      auto const deadline =
         std::chrono::steady_clock::now() + std::chrono::seconds(2);
      while (std::chrono::steady_clock::now() < deadline)
      {
         stream.process(rec);
         if (rec._entries.size() >= n)
            return true;
         std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
      return false;
   }

   bool pump_sysex(q::midi2_input_stream& stream, recorder& rec)
   {
      auto const deadline =
         std::chrono::steady_clock::now() + std::chrono::seconds(2);
      while (std::chrono::steady_clock::now() < deadline)
      {
         stream.process(rec);
         if (!rec._sysex.empty())
            return true;
         std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
      return false;
   }
}

TEST_CASE("Packets sent to a virtual output arrive at an input stream")
{
   q::midi2_output_stream out{port_name};
   if (!out.is_valid())
   {
      WARN("This platform will not open a virtual packet port; skipping.");
      return;
   }

   auto const device = wait_for_loopback(true);
   REQUIRE(device.has_value());
   CHECK(device->protocol() == q::midi_device::midi_2_0);

   q::midi2_input_stream in{*device};
   REQUIRE(in.is_valid());
   recorder rec;

   SECTION("A note survives the trip whole, 16 bit velocity included")
   {
      out.send(midi2::packet{0x40903C00u, 0xBEEF0000u});
      REQUIRE(pump(in, rec, 1));

      auto const& e = rec._entries.front();
      CHECK(e.kind == 0x90);
      CHECK(e.channel == 0);
      CHECK(e.key == 60);
      CHECK(e.value == 0xBEEF);
   }

   SECTION("Packets arrive in the order they were sent")
   {
      out.send(midi2::packet{0x41913C00u, 0x80000000u});
      out.send(midi2::packet{0x41B10700u, 0x40000000u});
      out.send(midi2::packet{0x41813C00u, 0u});
      REQUIRE(pump(in, rec, 3));

      REQUIRE(rec._entries.size() >= 3);
      CHECK(rec._entries[0].kind == 0x90);
      CHECK(rec._entries[1].kind == 0xB0);
      CHECK(rec._entries[1].key == 7);
      CHECK(rec._entries[1].value == 0x40000000u);
      CHECK(rec._entries[2].kind == 0x80);
      for (auto const& e : rec._entries)
         CHECK(e.channel == 1);
   }

   SECTION("A MIDI 1.0 packet dispatches as a MIDI 1.0 message")
   {
      out.send(midi2::packet{0x20903C64u});
      REQUIRE(pump(in, rec, 1));
      CHECK(rec._entries.front().kind == 0x190);
      CHECK(rec._entries.front().value == 100);
   }

   SECTION("System exclusive is gathered across packets")
   {
      std::uint8_t const payload[] =
         {0x7E, 0x7F, 0x0D, 0x70, 0x02, 0x11, 0x22, 0x33, 0x44};
      midi2::send_sysex7(payload
       , [&](midi2::packet const& p) { out.send(p); });
      REQUIRE(pump_sysex(in, rec));

      // data() is what lies between the brackets, as the byte reader's.
      REQUIRE(rec._sysex.size() == 9);
      CHECK(rec._sysex.front() == 0x7E);
      CHECK(rec._sysex[4] == 0x02);
      CHECK(rec._sysex.back() == 0x44);
   }

   SECTION("Timestamps advance and never run backwards")
   {
      for (int i = 0; i != 8; ++i)
      {
         out.send(midi2::packet{0x40903C00u + std::uint32_t(i), 0xFFFF0000u});
         std::this_thread::sleep_for(std::chrono::milliseconds(5));
      }
      REQUIRE(pump(in, rec, 8));

      REQUIRE(rec._entries.size() >= 8);
      CHECK(rec._entries.front().time > 0);
      for (std::size_t i = 1; i != rec._entries.size(); ++i)
         CHECK(rec._entries[i].time >= rec._entries[i-1].time);

      // Nanoseconds, 5ms apart: tens of milliseconds across eight.
      auto const span = rec._entries.back().time - rec._entries.front().time;
      CHECK(span > 20'000'000u);
   }
}

TEST_CASE("Packets sent to a virtual input arrive from an output stream")
{
   q::midi2_input_stream in{port_name};
   if (!in.is_valid())
   {
      WARN("This platform will not open a virtual packet port; skipping.");
      return;
   }

   auto const device = wait_for_loopback(false);
   REQUIRE(device.has_value());

   q::midi2_output_stream out{*device};
   REQUIRE(out.is_valid());

   recorder rec;
   out.send(midi2::packet{0x42923C00u, 0x12340000u});
   REQUIRE(pump(in, rec, 1));
   CHECK(rec._entries.front().channel == 2);
   CHECK(rec._entries.front().value == 0x1234);
}

TEST_CASE("A stream on a device of the other protocol is not valid")
{
   q::midi2_output_stream out{port_name};
   if (!out.is_valid())
      return;

   // The byte listing may also show the port; a packet stream must not
   // open through a byte device.
   for (auto const& device : q::midi_device::list())
   {
      if (device.name().find(port_name) != std::string::npos
         && device.num_inputs() != 0)
      {
         CHECK(device.protocol() == q::midi_device::midi_1_0);
         q::midi2_input_stream in{device};
         CHECK(!in.is_valid());
      }
   }
}
