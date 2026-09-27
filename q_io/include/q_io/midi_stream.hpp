/*=============================================================================
   Copyright (c) 2016-2023 Cycfi Research. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_MIDI_STREAM_DECEMBER_12_2018)
#define CYCFI_Q_MIDI_STREAM_DECEMBER_12_2018

#include <infra/support.hpp>
#include <q/midi/processor.hpp>
#include <q/midi/byte_reader.hpp>
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
                           requires concepts::midi::Processor<P>
      void                 process(P&& proc);

                           template <typename Processor>
      void                 process_raw(Processor&& proc);

      static void          set_default_device(int id);

   private:

      // A channel message in msg, or a system exclusive in sysex, its
      // markers stripped, held until the next call.
      struct event
      {
         midi_1_0::raw_message msg;
         std::size_t       time;
         byte_span         sysex;
      };

      bool                 next(event& ev);

      impl*                _impl;
   };

   ////////////////////////////////////////////////////////////////////////////
   // midi_output_stream: MIDI 1.0 bytes to a device, or out of a virtual
   // port of the given name that other programs may open. send takes one
   // whole message, system exclusive included, so the stream is a Sink
   // over byte_span; the overload takes a message struct directly.
   ////////////////////////////////////////////////////////////////////////////
   class midi_output_stream : non_copyable
   {
   public:

      struct impl;
                           midi_output_stream(midi_device const& device);
                           midi_output_stream(char const* virtual_port);
                           ~midi_output_stream();

      bool                 is_valid() const { return _impl != nullptr; }
      void                 send(byte_span bytes);

                           template <int N>
      void                 send(midi_1_0::message<N> const& msg)
                           { send(byte_span{msg.data, std::size_t(N)}); }

   private:

      impl*                _impl;
   };

   ////////////////////////////////////////////////////////////////////////////
   template <typename P>
   requires concepts::midi::Processor<P>
   inline void midi_input_stream::process(P&& proc)
   {
      event ev;
      if (next(ev))
      {
         if (!ev.sysex.empty())
            proc(midi_1_0::sysex_view{ev.sysex}, ev.time);
         else
            midi_1_0::dispatch(ev.msg, ev.time, proc);
      }
   }

   template <typename Processor>
   inline void midi_input_stream::process_raw(Processor&& proc)
   {
      event ev;
      if (next(ev) && ev.sysex.empty())
         proc.process_midi(ev.msg, ev.time);
   }

   static_assert(concepts::midi::Source<midi_input_stream>);
   static_assert(concepts::midi::Sink<midi_output_stream, byte_span>);
}

#endif
