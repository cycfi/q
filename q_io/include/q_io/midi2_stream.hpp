/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_MIDI2_STREAM_SEPTEMBER_11_2026)
#define CYCFI_Q_MIDI2_STREAM_SEPTEMBER_11_2026

#include <infra/support.hpp>
#include <q/midi/packet_reader.hpp>
#include <q_io/midi_device.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // midi2_input_stream: Universal MIDI Packets from a device, or from a
   // virtual port of the given name that other programs may open. The
   // counterpart of midi_input_stream for MIDI 2.0.
   //
   // process(proc) hands the next packet to a packet reader, so system
   // exclusive and the stream's text messages arrive gathered, and the rest
   // dispatch as the MIDI 2.0 messages do. Timestamps are the host's own,
   // in nanoseconds.
   ////////////////////////////////////////////////////////////////////////////
   class midi2_input_stream : non_copyable
   {
   public:

      struct impl;
                           midi2_input_stream(midi_device const& device);
                           midi2_input_stream(char const* virtual_port);
                           ~midi2_input_stream();

      bool                 is_valid() const { return _impl != nullptr; }

                           template <typename P>
                           requires concepts::midi_1_0::Processor<P>
      void                 process(P&& proc);

   private:

      struct event
      {
         std::uint32_t     words[4];
         std::size_t       time;
      };

      bool                 next(event& ev);

      impl*                _impl;
      midi_2_0::packet_reader<> _reader;
   };

   ////////////////////////////////////////////////////////////////////////////
   // midi2_output_stream: Universal MIDI Packets to a device, or out of a
   // virtual port of the given name.
   ////////////////////////////////////////////////////////////////////////////
   class midi2_output_stream : non_copyable
   {
   public:

      struct impl;
                           midi2_output_stream(midi_device const& device);
                           midi2_output_stream(char const* virtual_port);
                           ~midi2_output_stream();

      bool                 is_valid() const { return _impl != nullptr; }
      void                 send(midi_2_0::packet const& p);

   private:

      impl*                _impl;
   };

   ////////////////////////////////////////////////////////////////////////////
   template <typename P>
   requires concepts::midi_1_0::Processor<P>
   inline void midi2_input_stream::process(P&& proc)
   {
      event ev;
      if (next(ev))
         _reader({ev.words[0], ev.words[1], ev.words[2], ev.words[3]}
               , ev.time, proc);
   }
}

#endif
