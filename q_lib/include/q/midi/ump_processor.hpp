/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_MIDI_UMP_PROCESSOR_HPP_SEPTEMBER_9_2026)
#define CYCFI_Q_MIDI_UMP_PROCESSOR_HPP_SEPTEMBER_9_2026

#include <q/midi/ump_messages.hpp>
#include <q/midi/processor.hpp>
#include <cstdint>

namespace cycfi::q::midi_2_0
{
   ////////////////////////////////////////////////////////////////////////////
   // processor: the same shape as MIDI 1.0's. Derive from it, pull in its
   // catch-all with a using declaration, and overload the messages you
   // care about. Both protocols' messages share message_base, so a single
   // processor hears a MIDI 2.0 stream whole, the MIDI 1.0 voice messages
   // it carries included.
   ////////////////////////////////////////////////////////////////////////////
   struct processor : midi_1_0::processor
   {
      using midi_1_0::processor::operator();
   };

   ////////////////////////////////////////////////////////////////////////////
   // dispatch: one packet to one processor.
   //
   // Type 0x4 is the MIDI 2.0 voice messages, switched on the opcode. Type
   // 0x2 is a MIDI 1.0 voice message and type 0x1 a system message, each
   // packed the way the wire packed it, so both go to MIDI 1.0's dispatch
   // and reach the overloads a MIDI 1.0 processor already has. Type 0x0,
   // the utility messages, is a transport concern and dispatches nothing.
   // Data and reserved types dispatch nothing yet.
   ////////////////////////////////////////////////////////////////////////////
   template <typename P>
   requires concepts::midi_1_0::Processor<P>
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

         default:
            break;
      }
   }
}

#endif
