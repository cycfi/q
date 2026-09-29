/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_MIDI_UMP_PROCESSOR_HPP_SEPTEMBER_9_2026)
#define CYCFI_Q_MIDI_UMP_PROCESSOR_HPP_SEPTEMBER_9_2026

#include <q/midi/ump_messages.hpp>
#include <q/midi/ump_utility.hpp>
#include <q/midi/ump_flex.hpp>
#include <q/midi/ump_stream.hpp>
#include <q/midi/processor.hpp>
#include <cstdint>

namespace cycfi::q::midi_2_0
{
   ////////////////////////////////////////////////////////////////////////////
   // processor: MIDI 1.0's default no-op processor under the MIDI 2.0
   // name. Derive from it, pull in its catch-all with a using declaration,
   // and overload the messages you care about. Both protocols' messages
   // share message_base, so a single processor receives a MIDI 2.0 stream
   // whole, the MIDI 1.0 voice messages it carries included.
   ////////////////////////////////////////////////////////////////////////////
   struct processor : midi::processor
   {
      using midi::processor::operator();
   };

   ////////////////////////////////////////////////////////////////////////////
   // dispatch: one packet to one processor.
   //
   // Type 0x4 is the MIDI 2.0 voice messages, switched on the opcode. Type
   // 0x2 is a MIDI 1.0 voice message and type 0x1 a system message, each
   // packed the way the wire packed it, so both go to MIDI 1.0's dispatch
   // and reach the overloads a MIDI 1.0 processor already has. Type 0xF is
   // the stream messages, switched on the status; the three that carry
   // text may span packets and are left to packet_reader. Type 0x0 is the
   // utility messages, switched on the status. Type 0xD is flex data,
   // whose setup bank is switched on the status here and whose two text
   // banks are left to packet_reader. Data and reserved types dispatch
   // nothing here.
   ////////////////////////////////////////////////////////////////////////////
   template <typename P>
   requires concepts::midi::Processor<P>
   inline void dispatch(packet const& p, std::size_t time, P&& proc)
   {
      switch (p.message_type())
      {
         case message_type::midi2_voice:
            switch (p.status())
            {
               case opcode::registered_per_note:
                  proc(registered_per_note_controller{p}, time);
                  break;
               case opcode::assignable_per_note:
                  proc(assignable_per_note_controller{p}, time);
                  break;
               case opcode::registered:
                  proc(registered_controller{p}, time);
                  break;
               case opcode::assignable:
                  proc(assignable_controller{p}, time);
                  break;
               case opcode::relative_registered:
                  proc(relative_registered_controller{p}, time);
                  break;
               case opcode::relative_assignable:
                  proc(relative_assignable_controller{p}, time);
                  break;
               case opcode::per_note_pitch_bend:
                  proc(per_note_pitch_bend{p}, time);
                  break;
               case opcode::note_off:
                  proc(note_off{p}, time);
                  break;
               case opcode::note_on:
                  proc(note_on{p}, time);
                  break;
               case opcode::poly_pressure:
                  proc(poly_pressure{p}, time);
                  break;
               case opcode::control_change:
                  proc(control_change{p}, time);
                  break;
               case opcode::program_change:
                  proc(program_change{p}, time);
                  break;
               case opcode::channel_pressure:
                  proc(channel_pressure{p}, time);
                  break;
               case opcode::pitch_bend:
                  proc(pitch_bend{p}, time);
                  break;
               case opcode::per_note_management:
                  proc(per_note_management{p}, time);
                  break;
               default:
                  break;
            }
            break;

         case message_type::utility:
            // 4.8: about the stream rather than about music. More may be
            // defined, so an unknown status is passed over.
            switch (p.status())
            {
               case utility_status::noop:
                  proc(noop{p}, time);
                  break;
               case utility_status::jr_clock:
                  proc(jr_clock{p}, time);
                  break;
               case utility_status::jr_timestamp:
                  proc(jr_timestamp{p}, time);
                  break;
               case utility_status::ticks_per_quarter_note:
                  proc(midi_2_0::ticks_per_quarter_note{p}, time);
                  break;
               case utility_status::delta_clockstamp:
                  proc(delta_clockstamp{p}, time);
                  break;
               default:
                  break;
            }
            break;

         case message_type::flex_data:
            // 7.5: only the setup bank is a message in one packet. The
            // text banks are gathered, and an unknown status is passed
            // over rather than guessed at.
            if (((p.word(0) >> 8) & 0xFF) == flex_bank::setup)
            {
               switch (p.word(0) & 0xFF)
               {
                  case flex_status::set_tempo:
                     proc(set_tempo{p}, time);
                     break;
                  case flex_status::set_time_signature:
                     proc(set_time_signature{p}, time);
                     break;
                  case flex_status::set_metronome:
                     proc(set_metronome{p}, time);
                     break;
                  case flex_status::set_key_signature:
                     proc(set_key_signature{p}, time);
                     break;
                  case flex_status::set_chord_name:
                     proc(set_chord_name{p}, time);
                     break;
                  default:
                     break;
               }
            }
            break;

         case message_type::midi1_voice:
         case message_type::system:
         {
            // Table 16 and 17: the status byte, then two data bytes, in the
            // low three bytes of the word. That is exactly the layout of a
            // MIDI 1.0 raw message, read from the other end.
            auto const w = p.word(0);
            midi_1_0::raw_message const msg{
               ((w >> 16) & 0xFF)
             | (((w >> 8) & 0x7F) << 8)
             | ((w & 0x7F) << 16)};
            midi_1_0::dispatch(msg, time, proc);
            break;
         }

         case message_type::stream:
            switch ((p.word(0) >> 16) & 0x3FF)
            {
               case stream_status::endpoint_discovery:
                  proc(endpoint_discovery{p}, time);
                  break;
               case stream_status::endpoint_info:
                  proc(endpoint_info{p}, time);
                  break;
               case stream_status::device_identity:
                  proc(device_identity{p}, time);
                  break;
               case stream_status::stream_configuration_request:
                  proc(stream_configuration_request{p}, time);
                  break;
               case stream_status::stream_configuration:
                  proc(stream_configuration{p}, time);
                  break;
               case stream_status::function_block_discovery:
                  proc(function_block_discovery{p}, time);
                  break;
               case stream_status::function_block_info:
                  proc(function_block_info{p}, time);
                  break;
               case stream_status::start_of_clip:
                  proc(start_of_clip{p}, time);
                  break;
               case stream_status::end_of_clip:
                  proc(end_of_clip{p}, time);
                  break;
               default:
                  break;
            }
            break;

         default:
            break;
      }
   }
}

#endif
