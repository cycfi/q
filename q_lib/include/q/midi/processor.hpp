/*=============================================================================
   Copyright (C) 2012-2024 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_MIDI_PROCESSOR_HPP_OCTOBER_8_2012)
#define CYCFI_Q_MIDI_PROCESSOR_HPP_OCTOBER_8_2012

#include <q/midi/messages.hpp>

namespace cycfi::q::concepts
{
   namespace midi
   {
      template <typename T>
      concept Processor =
         requires(T&& proc, q::midi::message_base const& msg, std::size_t time)
      {
         proc(msg, time);
      };
   }

   // The concept requires a call with a message_base, which the messages
   // of both protocols derive from, so it serves both. It was first named
   // for MIDI 1.0, and that name still reaches it.
   namespace midi_1_0 = midi;
}

namespace cycfi::q::midi
{
   ////////////////////////////////////////////////////////////////////////////
   // processor: the default no-op processor, which takes every message
   // and ignores it. Derive from it, pull in its catch-all with a using
   // declaration, and overload the messages you care about.
   ////////////////////////////////////////////////////////////////////////////
   struct processor
   {
      void  operator()(message_base const& msg, std::size_t time) {}
   };
}

namespace cycfi::q::concepts::midi
{
   ////////////////////////////////////////////////////////////////////////////
   // Where MIDI comes from and where it goes. A Source runs a Processor
   // over what it has received. A Sink takes one whole message per call,
   // in the Unit its transport carries: a midi_2_0::packet, or a
   // byte_span for MIDI 1.0, sysex included. q_io's streams conform, and
   // so can a plugin host's event list or a device driver, which is why
   // the library states the requirement rather than a base class.
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   concept Source =
      requires(T& source, q::midi::processor& proc)
   {
      source.process(proc);
   };

   template <typename T, typename Unit>
   concept Sink =
      requires(T& sink, Unit const& unit)
   {
      sink.send(unit);
   };
}

namespace cycfi::q::midi::detail
{
   // The responders and builders take their sink either way: a Sink, or
   // a callable such as a lambda. This is the one place that tells them
   // apart.
   template <typename S, typename Unit>
   inline void emit(S& sink, Unit const& unit)
   {
      if constexpr (concepts::midi::Sink<S, Unit>)
         sink.send(unit);
      else
         sink(unit);
   }
}

namespace cycfi::q::midi_1_0
{
   using midi::processor;

   template <typename P>
   requires concepts::midi::Processor<P>
   inline void dispatch(raw_message msg, std::size_t time, P&& proc)
   {
      // Channel voice messages (0x80-0xEF) encode the channel number in
      // the low nibble, so only the high nibble is significant. System
      // common and system real-time messages (0xF0-0xFF), on the other
      // hand, occupy the entire status byte -- there is no channel to
      // mask off. Masking those with 0xF0 would collapse all of them to
      // 0xF0, none of which match any case below (see status::sysex,
      // 0xF0, which is handled elsewhere), silently dropping every
      // system common / real-time message.
      auto const status_byte = msg.data & 0xFF;
      auto const status_ = (status_byte < status::sysex)?
         (status_byte & 0xF0) : status_byte;

      switch (status_) // status
      {
         case status::note_off:
            proc(note_off{msg}, time);
            break;

         case status::note_on:
            proc(note_on{msg}, time);
            break;

         case status::poly_pressure:
            proc(poly_pressure{msg}, time);
            break;

         case status::control_change:
            proc(control_change{msg}, time);
            break;

         case status::program_change:
            proc(program_change{msg}, time);
            break;

         case status::channel_pressure:
            proc(channel_pressure{msg}, time);
            break;

         case status::pitch_bend:
            proc(pitch_bend{msg}, time);
            break;

         case status::song_position:
            proc(song_position{msg}, time);
            break;

         case status::song_select:
            proc(song_select{msg}, time);
            break;

         case status::tune_request:
            proc(tune_request{msg}, time);
            break;

         case status::timing_tick:
            proc(timing_tick{msg}, time);
            break;

         case status::start:
            proc(start{msg}, time);
            break;

         case status::continue_:
            proc(continue_{msg}, time);
            break;

         case status::stop:
            proc(stop{msg}, time);
            break;

         case status::active_sensing:
            proc(active_sensing{msg}, time);
            break;

         case status::reset:
            proc(reset{msg}, time);
            break;
      }
   }
}

#endif
