/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_CLAP_MIDI_EVENTS_HPP_SEPTEMBER_11_2026)
#define QPLUG_CLAP_MIDI_EVENTS_HPP_SEPTEMBER_11_2026

#include <q/midi/messages.hpp>
#include <q/midi/ump.hpp>
#include <q/midi/ump_messages.hpp>
#include <clap/clap.h>
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace cycfi::qplug
{
   ////////////////////////////////////////////////////////////////////////////
   // The host's note events as Q messages.
   //
   // CLAP offers three dialects (clap/ext/note-ports.h) and a plugin may
   // accept all of them. Two are MIDI already. CLAP's own typed note is
   // richer than MIDI 1.0, with a velocity in double and expressions per
   // note, so it becomes MIDI 2.0, which has room for those. What MIDI 2.0
   // cannot address is dropped here: the note id, which names one voice
   // where MIDI names a key, and a wildcard in the tuple.
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

   // The MIDI 2.0 dialect. Four words, of which the packet's own type says
   // how many carry the message.
   inline q::midi_2_0::packet to_packet(clap_event_midi2_t const& ev)
   {
      return {ev.data[0], ev.data[1], ev.data[2], ev.data[3]};
   }

   namespace detail
   {
      // The first word of a MIDI 2.0 voice message, group 0.
      constexpr std::uint32_t voice(
         std::uint8_t opcode, std::uint8_t channel
       , std::uint8_t byte3, std::uint8_t byte4)
      {
         return (std::uint32_t(q::midi_2_0::message_type::midi2_voice) << 28)
            | (std::uint32_t(opcode) << 20)
            | (std::uint32_t(channel & 0x0F) << 16)
            | (std::uint32_t(byte3 & 0x7F) << 8)
            | byte4;
      }

      // Zero to one as an unsigned value of the given width, the ends
      // exact.
      constexpr std::uint32_t unit(double x, unsigned bits)
      {
         auto const max = double((std::uint64_t(1) << bits) - 1);
         return std::uint32_t(std::clamp(x, 0.0, 1.0) * max + 0.5);
      }
   }

   // The CLAP dialect. Its note is addressed by the tuple (port, channel,
   // key, note id), any part of which may be -1 for a wildcard. A wildcard
   // addresses voices rather than naming a note, and no MIDI message can
   // say it, so those are dropped. Returns false for an event that is not
   // a note this translates.
   inline bool to_packet(
      clap_event_note_t const& ev, q::midi_2_0::packet& out)
   {
      if (ev.channel < 0 || ev.key < 0)
         return false;

      std::uint8_t opcode = 0;
      switch (ev.header.type)
      {
         case CLAP_EVENT_NOTE_ON:
            opcode = q::midi_2_0::opcode::note_on;
            break;
         case CLAP_EVENT_NOTE_OFF:
            opcode = q::midi_2_0::opcode::note_off;
            break;
         default:
            return false;      // choke and end address voices, not notes
      }

      // Zero to one becomes 16 bits. A note on of zero velocity is a quiet
      // note on in MIDI 2.0, section 4.2.2, so nothing is floored here.
      auto const channel = std::uint8_t(ev.channel);
      auto const key = std::uint8_t(ev.key);
      auto const velocity = detail::unit(ev.velocity, 16);
      out = {detail::voice(opcode, channel, key, 0), velocity << 16};
      return true;
   }

   // A note expression, addressed the same way, as the MIDI 2.0 message
   // for the same note: pressure as poly pressure, tuning as the registered
   // per-note controller Pitch 7.25, the absolute pitch of the note in
   // semitones with 25 bits of fraction, and the rest as the registered
   // per-note controllers of the same names. Volume above 1, a gain, has
   // no MIDI form and clamps to full.
   inline bool to_packet(
      clap_event_note_expression_t const& ev, q::midi_2_0::packet& out)
   {
      if (ev.channel < 0 || ev.key < 0)
         return false;

      namespace midi2 = q::midi_2_0;
      auto const channel = std::uint8_t(ev.channel);
      auto const key = std::uint8_t(ev.key);

      auto controller = [&](std::uint8_t index, std::uint32_t value)
      {
         auto const opcode = midi2::opcode::registered_per_note;
         out = {detail::voice(opcode, channel, key, index), value};
         return true;
      };

      switch (ev.expression_id)
      {
         case CLAP_NOTE_EXPRESSION_VOLUME:
            return controller(7, detail::unit(ev.value, 32));
         case CLAP_NOTE_EXPRESSION_PAN:
            return controller(10, detail::unit(ev.value, 32));
         case CLAP_NOTE_EXPRESSION_TUNING:
         {
            // Pitch 7.25: seven bits of semitone, 25 of fraction.
            auto const one = std::ldexp(1.0, 25);
            auto const top = 128.0 - 1.0 / one;
            auto const pitch = std::clamp(double(key) + ev.value, 0.0, top);
            auto const semitone = std::uint32_t(pitch);
            auto const fraction = std::uint32_t((pitch - semitone) * one);
            return controller(3, (semitone << 25) | fraction);
         }
         case CLAP_NOTE_EXPRESSION_VIBRATO:
            return controller(77, detail::unit(ev.value, 32));
         case CLAP_NOTE_EXPRESSION_EXPRESSION:
            return controller(11, detail::unit(ev.value, 32));
         case CLAP_NOTE_EXPRESSION_BRIGHTNESS:
            return controller(74, detail::unit(ev.value, 32));
         case CLAP_NOTE_EXPRESSION_PRESSURE:
            out = {
               detail::voice(midi2::opcode::poly_pressure, channel, key, 0)
             , detail::unit(ev.value, 32)
            };
            return true;
         default:
            return false;
      }
   }
}

#endif
