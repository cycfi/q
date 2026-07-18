/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/support/midi_processor.hpp>

namespace q = cycfi::q;
namespace midi = q::midi_1_0;

namespace
{
   enum class event_kind
   {
      none,
      base,
      note_on,
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
      std::uint16_t position = 0;
      std::uint8_t song_number = 0;

      void operator()(midi::message_base const&, std::size_t t)
      {
         kind = event_kind::base;
         time = t;
      }

      void operator()(midi::note_on const& msg, std::size_t t)
      {
         kind = event_kind::note_on;
         time = t;
         channel = msg.channel();
         key = msg.key();
         velocity = msg.velocity();
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
