/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q_io/midi_stream.hpp>
#include <q_io/midi_device.hpp>
#include <libremidi/libremidi.hpp>

#include <chrono>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace q = cycfi::q;
namespace midi = q::midi_1_0;

namespace
{
   char const* port_name = "Q loopback";

   // Every message the stream delivered, in the order it delivered them.
   struct recorder
   {
      struct entry
      {
         int            kind;       // the status byte, high nibble
         std::uint8_t   channel;
         std::uint8_t   key;
         std::uint8_t   velocity;
         std::size_t    time;
      };

      void operator()(midi::message_base const&, std::size_t) {}

      void operator()(midi::note_on msg, std::size_t time)
      {
         _entries.push_back(
            {0x90, msg.channel(), msg.key(), msg.velocity(), time});
      }

      void operator()(midi::note_off msg, std::size_t time)
      {
         _entries.push_back(
            {0x80, msg.channel(), msg.key(), msg.velocity(), time});
      }

      void operator()(midi::control_change msg, std::size_t time)
      {
         _entries.push_back(
            {0xB0, msg.channel(), std::uint8_t(msg.controller())
            , msg.value(), time});
      }

      std::vector<entry> _entries;
   };

   // Find the device the virtual port shows up as. Its name is the port
   // name on CoreMIDI, and may be decorated elsewhere, so match loosely.
   std::optional<q::midi_device> find_loopback()
   {
      for (auto const& device : q::midi_device::list())
      {
         if (device.num_inputs() != 0
            && device.name().find(port_name) != std::string::npos)
            return device;
      }
      return {};
   }

   // Pump the stream until it has delivered n messages, or time runs out.
   // A device is asynchronous: there is no point at which it is guaranteed
   // to have arrived, only a point at which we stop waiting.
   bool pump(q::midi_input_stream& stream, recorder& rec, std::size_t n)
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
}

TEST_CASE("Messages sent to a port arrive at the stream")
{
   libremidi::midi_out out;
   if (out.open_virtual_port(port_name) != stdx::error{})
   {
      WARN("This platform will not open a virtual MIDI port; skipping.");
      return;
   }

   // The port appears asynchronously, so the listing may need a moment.
   // midi_device refers to the listing rather than owning it, so it can be
   // constructed but not assigned. Hence emplace.
   std::optional<q::midi_device> device;
   for (int i = 0; i != 40 && !device; ++i)
   {
      if (auto const found = find_loopback())
         device.emplace(*found);
      else
         std::this_thread::sleep_for(std::chrono::milliseconds(50));
   }
   REQUIRE(device.has_value());

   q::midi_input_stream stream{*device};
   REQUIRE(stream.is_valid());

   recorder rec;

   SECTION("A note survives the trip whole")
   {
      out.send_message(0x90, 60, 100);
      REQUIRE(pump(stream, rec, 1));

      auto const& e = rec._entries.front();
      CHECK(e.kind == 0x90);
      CHECK(e.channel == 0);
      CHECK(e.key == 60);
      CHECK(e.velocity == 100);
   }

   SECTION("Messages arrive in the order they were sent")
   {
      out.send_message(0x91, 60, 100);
      out.send_message(0xB1, 7, 64);
      out.send_message(0x81, 60, 0);
      REQUIRE(pump(stream, rec, 3));

      REQUIRE(rec._entries.size() >= 3);
      CHECK(rec._entries[0].kind == 0x90);
      CHECK(rec._entries[0].key == 60);
      CHECK(rec._entries[1].kind == 0xB0);
      CHECK(rec._entries[1].key == 7);       // the controller number
      CHECK(rec._entries[1].velocity == 64); // its value
      CHECK(rec._entries[2].kind == 0x80);

      // Every message carried the same channel.
      for (auto const& e : rec._entries)
         CHECK(e.channel == 1);
   }

   SECTION("Timestamps advance and never run backwards")
   {
      for (int i = 0; i != 8; ++i)
      {
         out.send_message(0x90, 60+i, 100);
         std::this_thread::sleep_for(std::chrono::milliseconds(5));
      }
      REQUIRE(pump(stream, rec, 8));

      REQUIRE(rec._entries.size() >= 8);
      CHECK(rec._entries.front().time > 0);
      for (std::size_t i = 1; i != rec._entries.size(); ++i)
         CHECK(rec._entries[i].time >= rec._entries[i-1].time);

      // Stamps are in nanoseconds, and the notes were 5ms apart, so the
      // span across eight of them is tens of milliseconds, not a handful
      // of counts. This is what catches a unit mistake.
      auto const span = rec._entries.back().time - rec._entries.front().time;
      CHECK(span > 20'000'000u);
   }
}
