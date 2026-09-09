/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#include <q_io/midi2_stream.hpp>
#include <q_io/detail/event_queue.hpp>
#include <libremidi/libremidi.hpp>
#include <memory>

namespace cycfi::q
{
   namespace detail
   {
      bool input_port_of(
         midi_device::protocol_type p, std::uint32_t id
       , libremidi::input_port& port);
      bool output_port_of(
         midi_device::protocol_type p, std::uint32_t id
       , libremidi::output_port& port);

      libremidi::API packet_api()
      {
         return libremidi::midi2::default_api();
      }
   }

   ////////////////////////////////////////////////////////////////////////////
   // Input
   ////////////////////////////////////////////////////////////////////////////

   // As the byte stream: libremidi calls back on its own thread, the program
   // reads from its loop, and the queue joins the two without either
   // waiting. A packet is four words at most, and 1024 of them is far more
   // than a loop running per audio block needs.
   struct midi2_input_stream::impl
   {
      static constexpr std::size_t queue_size = 1024;

      using queue_type = detail::event_queue<event, queue_size>;

      impl()
       : _midi_in{config(&_queue)
              , libremidi::midi_in_configuration_for(detail::packet_api())}
      {}

      // The queue is declared first, so its address is already valid when
      // the configuration below is handed to _midi_in.
      queue_type              _queue;
      libremidi::midi_in      _midi_in;

   private:

      static libremidi::ump_input_configuration config(queue_type* queue)
      {
         libremidi::ump_input_configuration cfg;

         // Runs on the device thread. It copies and enqueues, and does
         // nothing else: no allocation, no lock, no logging.
         cfg.on_message =
            [queue](libremidi::ump&& u)
            {
               queue->push({{u.data[0], u.data[1], u.data[2], u.data[3]}
                          , std::size_t(u.timestamp)});
            };

         // libremidi drops system exclusive unless told otherwise. MIDI-CI
         // rides on it, so a packet stream passes it through.
         cfg.ignore_sysex = false;

         // It also rewrites MIDI 1.0 voice packets as MIDI 2.0 ones unless
         // told otherwise. Whether to translate is the program's choice,
         // made with q::midi_2_0::to_midi2, so packets pass as they came.
         cfg.midi1_channel_events_to_midi2 = false;
         cfg.timestamps = libremidi::timestamp_mode::Absolute;
         return cfg;
      }
   };

   midi2_input_stream::midi2_input_stream(midi_device const& device)
    : _impl{nullptr}
   {
      libremidi::input_port port;
      if (device.protocol() != midi_device::midi_2_0
         || !detail::input_port_of(midi_device::midi_2_0, device.id(), port))
         return;

      auto self = std::make_unique<impl>();
      if (self->_midi_in.open_port(port) != stdx::error{})
         return;
      _impl = self.release();
   }

   midi2_input_stream::midi2_input_stream(char const* virtual_port)
    : _impl{nullptr}
   {
      if (detail::packet_api() == libremidi::API::DUMMY)
         return;
      auto self = std::make_unique<impl>();
      if (self->_midi_in.open_virtual_port(virtual_port) != stdx::error{})
         return;
      _impl = self.release();
   }

   midi2_input_stream::~midi2_input_stream()
   {
      delete _impl;
   }

   bool midi2_input_stream::next(event& ev)
   {
      return _impl? _impl->_queue.pop(ev) : false;
   }

   ////////////////////////////////////////////////////////////////////////////
   // Output
   ////////////////////////////////////////////////////////////////////////////
   struct midi2_output_stream::impl
   {
      impl()
       : _midi_out{libremidi::output_configuration{}
               , libremidi::midi_out_configuration_for(detail::packet_api())}
      {}

      libremidi::midi_out     _midi_out;
   };

   midi2_output_stream::midi2_output_stream(midi_device const& device)
    : _impl{nullptr}
   {
      libremidi::output_port port;
      if (device.protocol() != midi_device::midi_2_0
         || !detail::output_port_of(midi_device::midi_2_0, device.id(), port))
         return;

      auto self = std::make_unique<impl>();
      if (self->_midi_out.open_port(port) != stdx::error{})
         return;
      _impl = self.release();
   }

   midi2_output_stream::midi2_output_stream(char const* virtual_port)
    : _impl{nullptr}
   {
      if (detail::packet_api() == libremidi::API::DUMMY)
         return;
      auto self = std::make_unique<impl>();
      if (self->_midi_out.open_virtual_port(virtual_port) != stdx::error{})
         return;
      _impl = self.release();
   }

   midi2_output_stream::~midi2_output_stream()
   {
      delete _impl;
   }

   void midi2_output_stream::send(midi_2_0::packet const& p)
   {
      if (!_impl)
         return;
      std::uint32_t const words[4] =
         {p.word(0), p.word(1), p.word(2), p.word(3)};
      _impl->_midi_out.send_ump(words, p.words());
   }
}
