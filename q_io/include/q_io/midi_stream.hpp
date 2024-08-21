/*=============================================================================
   Copyright (c) 2016-2023 Cycfi Research. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_MIDI_STREAM_DECEMBER_12_2018)
#define CYCFI_Q_MIDI_STREAM_DECEMBER_12_2018

#include <infra/support.hpp>
#include <q/support/midi_processor.hpp>
#include <q_io/midi_device.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   class midi_input_stream : non_copyable
   {
   public:

      struct impl;
                           midi_input_stream();
                           midi_input_stream(midi_device const& device);
                           ~midi_input_stream();

      bool                 is_valid() const { return _impl != nullptr; }

                           template <typename P>
                           requires concepts::midi_1_0::Processor<P>
      void                 process(P&& proc);

                           template <typename Processor>
      void                 process_raw(Processor&& proc);

      static void          set_default_device(int id);

   private:

      struct event
      {
         midi_1_0::raw_message msg;
         std::size_t       time;
      };

      bool                 next(event& ev);

      impl*                _impl;
   };

   ////////////////////////////////////////////////////////////////////////////
   template <typename P>
   requires concepts::midi_1_0::Processor<P>
   inline void midi_input_stream::process(P&& proc)
   {
      event ev;
      if (next(ev))
         midi_1_0::dispatch(ev.msg, ev.time, proc);
   }

   template <typename Processor>
   inline void midi_input_stream::process_raw(Processor&& proc)
   {
      event ev;
      if (next(ev))
         proc.process_midi(ev.msg, ev.time);
   }
}

#endif
