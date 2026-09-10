/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_CLAP_MIDI_EVENTS_HPP_SEPTEMBER_11_2026)
#define QPLUG_CLAP_MIDI_EVENTS_HPP_SEPTEMBER_11_2026

#include <q/midi/messages.hpp>
#include <q/midi/ump.hpp>
#include <clap/clap.h>
#include <algorithm>
#include <cstdint>

namespace cycfi::qplug
{
   ////////////////////////////////////////////////////////////////////////////
   // The host's note events as Q messages.
   //
   // CLAP offers three dialects (clap/ext/note-ports.h) and a plugin may
   // accept all of them. Two are MIDI already, one is CLAP's own typed note,
   // and each ends up in the same place: a Q message a processor answers.
   // The one that carries information the other two cannot, the note id, is
   // dropped here; a voice that wants it takes it from the note expression
   // events instead.
   ////////////////////////////////////////////////////////////////////////////

   // The MIDI dialect. A message is one, two or three bytes, and CLAP
   // leaves the ones it does not use as they were, so only the bytes the
   // status calls for are read.
   inline q::midi_1_0::raw_message to_raw_message(clap_event_midi_t const& ev)
   {
      auto const status = ev.data[0];
      auto const size =
         (status & 0xF0) == q::midi_1_0::status::program_change
         || (status & 0xF0) == q::midi_1_0::status::channel_aftertouch? 2 : 3;

      std::uint32_t data = status;
      for (int i = 1; i != size; ++i)
         data |= std::uint32_t(ev.data[i]) << (i * 8);
      return {data};
   }

   // The CLAP dialect. Its note is addressed by the tuple (port, channel,
   // key, note id), any part of which may be -1 for a wildcard, and its
   // velocity runs from zero to one. A wildcard addresses voices rather
   // than naming a note, and no three byte message can say it, so those
   // are left to the note expression path. Returns false for an event
   // that is not a note this translates.
   inline bool to_raw_message(
      clap_event_note_t const& ev, q::midi_1_0::raw_message& out)
   {
      if (ev.channel < 0 || ev.key < 0)
         return false;

      std::uint8_t status = 0;
      switch (ev.header.type)
      {
         case CLAP_EVENT_NOTE_ON:
            status = q::midi_1_0::status::note_on;
            break;
         case CLAP_EVENT_NOTE_OFF:
            status = q::midi_1_0::status::note_off;
            break;
         default:
            return false;      // choke and end address voices, not notes
      }

      // Zero to one becomes one to 127 for a note on, since MIDI 1.0 reads
      // a note on of zero velocity as a note off. A note off has no such
      // rule, and its release velocity may be zero.
      auto const floor = ev.header.type == CLAP_EVENT_NOTE_ON? 1 : 0;
      auto const velocity = std::uint8_t(
         std::clamp(int(ev.velocity * 127.0 + 0.5), floor, 127));

      out = {std::uint32_t(status | (ev.channel & 0x0F))
           | (std::uint32_t(ev.key & 0x7F) << 8)
           | (std::uint32_t(velocity) << 16)};
      return true;
   }

   // The MIDI 2.0 dialect. Four words, of which the packet's own type says
   // how many carry the message.
   inline q::midi_2_0::packet to_packet(clap_event_midi2_t const& ev)
   {
      return {ev.data[0], ev.data[1], ev.data[2], ev.data[3]};
   }
}

#endif
