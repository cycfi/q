/*=============================================================================
   Copyright (C) 2012-2024 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_MIDI_PROCESSOR_HPP_OCTOBER_8_2012)
#define CYCFI_Q_MIDI_PROCESSOR_HPP_OCTOBER_8_2012

#include <q/support/midi_messages.hpp>

namespace cycfi::q::concepts
{
   namespace midi_1_0
   {
      template <typename T>
      concept Processor =
         requires(T&& proc, q::midi_1_0::message_base const& msg, std::size_t time)
      {
         proc(msg, time);
      };
   }
}

namespace cycfi::q::midi_1_0
{
   ////////////////////////////////////////////////////////////////////////////
   // processor
   ////////////////////////////////////////////////////////////////////////////
   struct processor
   {
      void  operator()(message_base const& msg, std::size_t time) {}
   };

   template <typename P>
   requires concepts::midi_1_0::Processor<P>
   inline void dispatch(raw_message msg, std::size_t time, P&& proc)
   {
      switch (msg.data & 0xF0) // status
      {
         case status::note_off:
            proc(note_off{msg}, time);
            break;

         case status::note_on:
            proc(note_on{msg}, time);
            break;

         case status::poly_aftertouch:
            proc(poly_aftertouch{msg}, time);
            break;

         case status::control_change:
            proc(control_change{msg}, time);
            break;

         case status::program_change:
            proc(program_change{msg}, time);
            break;

         case status::channel_aftertouch:
            proc(channel_aftertouch{msg}, time);
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
