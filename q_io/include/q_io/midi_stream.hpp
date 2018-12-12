/*=============================================================================
   Copyright (c) 2016-2018 Cycfi Research. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_MIDI_STREAM_DECEMBER_12_2018)
#define CYCFI_Q_MIDI_STREAM_DECEMBER_12_2018

#include <infra/support.hpp>
#include <q_io/midi.hpp>
#include <q_io/midi_device.hpp>

namespace cycfi { namespace q
{
   ////////////////////////////////////////////////////////////////////////////
   class midi_input_stream
   {
   public:
                     midi_input_stream(midi_device const& device);

                     template <typename Processor>
      void           process(Processor&& proc);

   private:

      struct event
      {
         midi::raw_message msg;
         std::size_t       time;
      };

      bool           next(event& ev);

      struct impl*   _impl;
   };

   ////////////////////////////////////////////////////////////////////////////
   template <typename Processor>
   void midi_input_stream::process(Processor&& proc)
   {
      event ev;
      if (next(ev))
      {
         using namespace midi::status;
         switch (ev.msg.data & 0xFF) // status
         {
            case note_off:
               proc(midi::note_off{ ev.msg }, ev.time);
               break;

            case note_on:
               proc(midi::note_on{ ev.msg }, ev.time);
               break;

            case poly_aftertouch:
               proc(midi::poly_aftertouch{ ev.msg }, ev.time);
               break;

            case control_change:
               proc(midi::control_change{ ev.msg }, ev.time);
               break;

            case program_change:
               proc(midi::program_change{ ev.msg }, ev.time);
               break;

            case channel_aftertouch:
               proc(midi::channel_aftertouch{ ev.msg }, ev.time);
               break;

            case pitch_bend:
               proc(midi::pitch_bend{ ev.msg }, ev.time);
               break;

            case song_position:
               proc(midi::song_position{ ev.msg }, ev.time);
               break;

            case song_select:
               proc(midi::song_select{ ev.msg }, ev.time);
               break;

            case tune_request:
               proc(midi::tune_request{ ev.msg }, ev.time);
               break;

            case timing_tick:
               proc(midi::timing_tick{ ev.msg }, ev.time);
               break;

            case start:
               proc(midi::start{ ev.msg }, ev.time);
               break;

            case continue_:
               proc(midi::continue_{ ev.msg }, ev.time);
               break;

            case stop:
               proc(midi::stop{ ev.msg }, ev.time);
               break;

            case active_sensing:
               proc(midi::active_sensing{ ev.msg }, ev.time);
               break;

            case reset:
               proc(midi::reset{ ev.msg }, ev.time);
               break;
         }
      }
   }
}}

#endif
