/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/support/midi_processor.hpp>
#include <q/support/frequency.hpp>
#include <string>

namespace q = cycfi::q;
namespace midi = q::midi_1_0;

namespace
{
   enum class event_kind
   {
      none,
      base,
      note_off,
      note_on,
      poly_aftertouch,
      control_change,
      program_change,
      channel_aftertouch,
      pitch_bend,
      song_position,
      song_select,
      tune_request,
      timing_tick,
      start,
      continue_,
      stop,
      active_sensing,
      reset
   };

   struct recording_processor
   {
      event_kind kind = event_kind::none;
      std::size_t time = 0;
      std::uint8_t channel = 0;
      std::uint8_t key = 0;
      std::uint8_t velocity = 0;
      std::uint8_t pressure = 0;
      std::uint8_t controller = 0;
      std::uint8_t value = 0;
      std::uint8_t preset = 0;
      std::uint16_t bend = 0;
      std::uint16_t position = 0;
      std::uint8_t song_number = 0;

      void operator()(midi::message_base const&, std::size_t t)
      {
         kind = event_kind::base;
         time = t;
      }

      void operator()(midi::note_off const& msg, std::size_t t)
      {
         kind = event_kind::note_off;
         time = t;
         channel = msg.channel();
         key = msg.key();
         velocity = msg.velocity();
      }

      void operator()(midi::note_on const& msg, std::size_t t)
      {
         kind = event_kind::note_on;
         time = t;
         channel = msg.channel();
         key = msg.key();
         velocity = msg.velocity();
      }

      void operator()(midi::poly_aftertouch const& msg, std::size_t t)
      {
         kind = event_kind::poly_aftertouch;
         time = t;
         channel = msg.channel();
         key = msg.key();
         pressure = msg.pressure();
      }

      void operator()(midi::control_change const& msg, std::size_t t)
      {
         kind = event_kind::control_change;
         time = t;
         channel = msg.channel();
         controller = msg.controller();
         value = msg.value();
      }

      void operator()(midi::program_change const& msg, std::size_t t)
      {
         kind = event_kind::program_change;
         time = t;
         channel = msg.channel();
         preset = msg.preset();
      }

      void operator()(midi::channel_aftertouch const& msg, std::size_t t)
      {
         kind = event_kind::channel_aftertouch;
         time = t;
         channel = msg.channel();
         pressure = msg.pressure();
      }

      void operator()(midi::pitch_bend const& msg, std::size_t t)
      {
         kind = event_kind::pitch_bend;
         time = t;
         channel = msg.channel();
         bend = msg.value();
      }

      void operator()(midi::song_position const& msg, std::size_t t)
      {
         kind = event_kind::song_position;
         time = t;
         position = msg.position();
      }

      void operator()(midi::song_select const& msg, std::size_t t)
      {
         kind = event_kind::song_select;
         time = t;
         song_number = msg.song_number();
      }

      void operator()(midi::tune_request const&, std::size_t t)    { kind = event_kind::tune_request; time = t; }
      void operator()(midi::timing_tick const&, std::size_t t)     { kind = event_kind::timing_tick; time = t; }
      void operator()(midi::start const&, std::size_t t)           { kind = event_kind::start; time = t; }
      void operator()(midi::continue_ const&, std::size_t t)       { kind = event_kind::continue_; time = t; }
      void operator()(midi::stop const&, std::size_t t)            { kind = event_kind::stop; time = t; }
      void operator()(midi::active_sensing const&, std::size_t t)  { kind = event_kind::active_sensing; time = t; }
      void operator()(midi::reset const&, std::size_t t)           { kind = event_kind::reset; time = t; }
   };

   template <typename Message>
   midi::raw_message to_raw(Message const& msg)
   {
      std::uint32_t data = 0;
      for (int i = 0; i != Message::size; ++i)
         data |= std::uint32_t(msg.data[i]) << (i * 8);
      return {data};
   }
}

TEST_CASE("midi dispatch: system common and real-time messages keep their full status byte")
{
   constexpr std::size_t time = 123;

   SECTION("song_position")
   {
      recording_processor proc;
      midi::dispatch(to_raw(midi::song_position{0x12, 0x34}), time, proc);
      CHECK(proc.kind == event_kind::song_position);
      CHECK(proc.time == time);
      CHECK(proc.position == 0x1A12);
   }

   SECTION("song_select")
   {
      recording_processor proc;
      midi::dispatch(to_raw(midi::song_select{0x45}), time, proc);
      CHECK(proc.kind == event_kind::song_select);
      CHECK(proc.time == time);
      CHECK(proc.song_number == 0x45);
   }

   SECTION("tune_request")
   {
      recording_processor proc;
      midi::dispatch(to_raw(midi::tune_request{}), time, proc);
      CHECK(proc.kind == event_kind::tune_request);
      CHECK(proc.time == time);
   }

   SECTION("timing_tick")
   {
      recording_processor proc;
      midi::dispatch(to_raw(midi::timing_tick{}), time, proc);
      CHECK(proc.kind == event_kind::timing_tick);
      CHECK(proc.time == time);
   }

   SECTION("start")
   {
      recording_processor proc;
      midi::dispatch(to_raw(midi::start{}), time, proc);
      CHECK(proc.kind == event_kind::start);
      CHECK(proc.time == time);
   }

   SECTION("continue")
   {
      recording_processor proc;
      midi::dispatch(to_raw(midi::continue_{}), time, proc);
      CHECK(proc.kind == event_kind::continue_);
      CHECK(proc.time == time);
   }

   SECTION("stop")
   {
      recording_processor proc;
      midi::dispatch(to_raw(midi::stop{}), time, proc);
      CHECK(proc.kind == event_kind::stop);
      CHECK(proc.time == time);
   }

   SECTION("active_sensing")
   {
      recording_processor proc;
      midi::dispatch(to_raw(midi::active_sensing{}), time, proc);
      CHECK(proc.kind == event_kind::active_sensing);
      CHECK(proc.time == time);
   }

   SECTION("reset")
   {
      recording_processor proc;
      midi::dispatch(to_raw(midi::reset{}), time, proc);
      CHECK(proc.kind == event_kind::reset);
      CHECK(proc.time == time);
   }
}

TEST_CASE("midi dispatch: channel voice messages still mask off the channel nibble")
{
   recording_processor proc;
   constexpr std::size_t time = 77;

   midi::dispatch(to_raw(midi::note_on{9, 64, 127}), time, proc);

   CHECK(proc.kind == event_kind::note_on);
   CHECK(proc.time == time);
   CHECK(proc.channel == 9);
   CHECK(proc.key == 64);
   CHECK(proc.velocity == 127);
}

TEST_CASE("midi dispatch: every channel voice message type reaches its handler")
{
   constexpr std::size_t time = 42;

   SECTION("note_off")
   {
      recording_processor proc;
      midi::dispatch(to_raw(midi::note_off{3, 60, 40}), time, proc);
      CHECK(proc.kind == event_kind::note_off);
      CHECK(proc.channel == 3);
      CHECK(proc.key == 60);
      CHECK(proc.velocity == 40);
   }

   SECTION("poly_aftertouch")
   {
      recording_processor proc;
      midi::dispatch(to_raw(midi::poly_aftertouch{3, 60, 90}), time, proc);
      CHECK(proc.kind == event_kind::poly_aftertouch);
      CHECK(proc.channel == 3);
      CHECK(proc.key == 60);
      CHECK(proc.pressure == 90);
   }

   SECTION("control_change")
   {
      recording_processor proc;
      midi::dispatch(
         to_raw(midi::control_change{5, midi::cc::sustain, 127}), time, proc);
      CHECK(proc.kind == event_kind::control_change);
      CHECK(proc.channel == 5);
      CHECK(proc.controller == midi::cc::sustain);
      CHECK(proc.value == 127);
   }

   SECTION("program_change")
   {
      recording_processor proc;
      midi::dispatch(to_raw(midi::program_change{7, 42}), time, proc);
      CHECK(proc.kind == event_kind::program_change);
      CHECK(proc.channel == 7);
      CHECK(proc.preset == 42);
   }

   SECTION("channel_aftertouch")
   {
      recording_processor proc;
      midi::dispatch(to_raw(midi::channel_aftertouch{9, 100}), time, proc);
      CHECK(proc.kind == event_kind::channel_aftertouch);
      CHECK(proc.channel == 9);
      CHECK(proc.pressure == 100);
   }

   SECTION("pitch_bend")
   {
      recording_processor proc;
      midi::dispatch(
         to_raw(midi::pitch_bend{2, std::uint16_t(0x1234)}), time, proc);
      CHECK(proc.kind == event_kind::pitch_bend);
      CHECK(proc.channel == 2);
      CHECK(proc.bend == 0x1234);       // 14-bit value survives the lsb/msb split
   }
}

TEST_CASE("midi dispatch: the sysex threshold is the exact boundary")
{
   constexpr std::size_t time = 5;

   SECTION("pitch_bend on channel 15 is the highest byte still masked")
   {
      // Status byte 0xEF: one below sysex (0xF0), so it must still be
      // masked down to 0xE0 and dispatch as a pitch_bend.
      recording_processor proc;
      midi::dispatch(
         to_raw(midi::pitch_bend{15, std::uint16_t(0x2000)}), time, proc);
      CHECK(proc.kind == event_kind::pitch_bend);
      CHECK(proc.channel == 15);
      CHECK(proc.bend == 0x2000);
   }

   SECTION("sysex (0xF0) dispatches to nothing")
   {
      // sysex has no case in dispatch(); it is handled elsewhere. Masking
      // it must not route it to a channel voice handler.
      recording_processor proc;
      midi::dispatch(midi::raw_message{0x0000'00F0}, time, proc);
      CHECK(proc.kind == event_kind::none);
   }

   SECTION("sysex_end (0xF7) dispatches to nothing")
   {
      recording_processor proc;
      midi::dispatch(midi::raw_message{0x0000'00F7}, time, proc);
      CHECK(proc.kind == event_kind::none);
   }
}

TEST_CASE("midi dispatch: channel nibble is preserved at both extremes")
{
   constexpr std::size_t time = 11;

   SECTION("channel 0")
   {
      recording_processor proc;
      midi::dispatch(
         to_raw(midi::control_change{0, midi::cc::modulation, 64}), time, proc);
      CHECK(proc.kind == event_kind::control_change);
      CHECK(proc.channel == 0);
      CHECK(proc.controller == midi::cc::modulation);
   }

   SECTION("channel 15")
   {
      recording_processor proc;
      midi::dispatch(
         to_raw(midi::control_change{15, midi::cc::modulation, 64}), time, proc);
      CHECK(proc.kind == event_kind::control_change);
      CHECK(proc.channel == 15);
      CHECK(proc.controller == midi::cc::modulation);
   }
}

TEST_CASE("midi dispatch: decodes raw little-endian wire bytes")
{
   constexpr std::size_t time = 0;

   SECTION("note_on, channel 2, key 0x3C, velocity 0x64")
   {
      // status 0x92, data-1 0x3C, data-2 0x64, packed little-endian.
      recording_processor proc;
      midi::dispatch(midi::raw_message{0x0064'3C92}, time, proc);
      CHECK(proc.kind == event_kind::note_on);
      CHECK(proc.channel == 2);
      CHECK(proc.key == 0x3C);
      CHECK(proc.velocity == 0x64);
   }

   SECTION("control_change, channel 0, controller 0x07, value 0x64")
   {
      recording_processor proc;
      midi::dispatch(midi::raw_message{0x0064'07B0}, time, proc);
      CHECK(proc.kind == event_kind::control_change);
      CHECK(proc.channel == 0);
      CHECK(proc.controller == 0x07);
      CHECK(proc.value == 0x64);
   }
}

TEST_CASE("midi note utilities: note_number and note_name round-trip")
{
   CHECK(midi::note_number("C4") == 60);
   CHECK(midi::note_number("A4") == 69);
   CHECK(midi::note_number("C#4") == 61);
   CHECK(midi::note_number("G9") == 127);

   CHECK(std::string(midi::note_name(60)) == "C4");
   CHECK(std::string(midi::note_name(69)) == "A4");
   CHECK(std::string(midi::note_name(0)) == "C-1");
   CHECK(std::string(midi::note_name(127)) == "G9");
   CHECK(std::string(midi::note_name(128)) == "--");

   // Every key whose name carries no octave sign round-trips (keys 12..127,
   // C0..G9; note_number does not parse the negative octaves 0..11).
   for (int key = 12; key <= 127; ++key)
      CHECK(midi::note_number(midi::note_name(key)) == key);

   CHECK(midi::note_number("") == -1);
   CHECK(midi::note_number("A") == -1);
   CHECK(midi::note_number("H4") == -1);      // letter outside A-G
}

TEST_CASE("midi note utilities: note_frequency")
{
   CHECK(q::as_double(midi::note_frequency(69)) == Approx(440.0));

   // Out of the supported key range [9, 119], frequency is zero.
   CHECK(q::as_double(midi::note_frequency(8)) == 0.0);
   CHECK(q::as_double(midi::note_frequency(120)) == 0.0);
}
