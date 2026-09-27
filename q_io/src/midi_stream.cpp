/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#include <q_io/midi_stream.hpp>
#include <q_io/detail/event_queue.hpp>
#include <q_io/detail/midi_convert.hpp>
#include <libremidi/libremidi.hpp>
#include <atomic>
#include <memory>

namespace cycfi::q
{
   namespace detail
   {
      int default_device_id = 0;

      bool input_port_of(std::uint32_t id, libremidi::input_port& port);
      bool output_port_of(
         midi_device::protocol_type p, std::uint32_t id
       , libremidi::output_port& port);
   }

   // libremidi calls back on its own thread as messages arrive, while the
   // program reads them from its own loop. The queues are what join the two
   // without either waiting for the other. 1024 events is a few seconds of
   // dense playing, far more than a loop running per audio block needs.
   //
   // A channel message travels in the entry. A system exclusive travels as
   // its byte count in the entry and its bytes in the byte queue, pushed
   // before the entry that announces them, so one writer and one reader
   // keep them in step. One that will not fit is dropped whole and counted;
   // truncating it would deliver a message the sender never sent.
   struct midi_input_stream::impl
   {
      static constexpr std::size_t queue_size = 1024;
      static constexpr std::size_t sysex_bytes = 16384;

      struct entry
      {
         midi_1_0::raw_message   msg;
         std::size_t             time;
         std::uint16_t           sysex_size;
      };

      using queue_type = detail::event_queue<entry, queue_size>;
      using byte_queue = detail::event_queue<std::uint8_t, sysex_bytes>;

      impl()
       : _midi_in{config(this)}
      {}

      // The queues are declared first, so their addresses are already
      // valid when the configuration below is handed to _midi_in.
      queue_type              _queue;
      byte_queue              _bytes;
      std::atomic<std::size_t> _sysex_drops{0};
      std::uint8_t            _sysex[sysex_bytes];
      libremidi::midi_in      _midi_in;

      // Runs on the device thread. It packs and enqueues, and does nothing
      // else: no allocation, no lock, no logging.
      void on_message(libremidi::message&& msg)
      {
         byte_span const bytes{msg.bytes.data(), msg.bytes.size()};
         auto const time = std::size_t(msg.timestamp);

         midi_1_0::raw_message raw;
         if (detail::to_raw_message(bytes, raw))
         {
            _queue.push({raw, time, 0});
            return;
         }
         if (bytes.size() < 2 || bytes.front() != 0xF0 || bytes.back() != 0xF7)
            return;

         auto const payload = bytes.subspan(1, bytes.size()-2);
         if (payload.size() > byte_queue::capacity - _bytes.size()
            || payload.size() > 0xFFFF)
         {
            _sysex_drops.fetch_add(1, std::memory_order_relaxed);
            return;
         }
         for (auto b : payload)
            _bytes.push(b);
         _queue.push({{}, time, std::uint16_t(payload.size())});
      }

   private:

      static libremidi::input_configuration config(impl* self)
      {
         libremidi::input_configuration cfg;
         cfg.on_message =
            [self](libremidi::message&& msg)
            {
               self->on_message(std::move(msg));
            };

         // libremidi drops system exclusive unless told otherwise. MIDI-CI
         // rides on it.
         cfg.ignore_sysex = false;

         // Absolute is the closest thing to when the note was played: the
         // host API's own stamp, in nanoseconds. PortMidi reported
         // milliseconds from a clock of its own.
         cfg.timestamps = libremidi::timestamp_mode::Absolute;
         return cfg;
      }
   };

   namespace detail
   {
      void input_stream_init(midi_input_stream::impl*& _impl, int id)
      {
         libremidi::input_port port;
         if (!input_port_of(id, port))
         {
            // The listing is what hands out ids, so a stream constructed
            // before anything was listed has nothing to open yet.
            midi_device::list();
            if (!input_port_of(id, port))
            {
               _impl = nullptr;
               return;
            }
         }

         auto self = std::make_unique<midi_input_stream::impl>();
         if (self->_midi_in.open_port(port) != stdx::error{})
         {
            _impl = nullptr;
            return;
         }
         _impl = self.release();
      }
   }

   midi_input_stream::midi_input_stream()
   {
      detail::input_stream_init(_impl, detail::default_device_id);
   }

   midi_input_stream::midi_input_stream(midi_device const& device)
   {
      detail::input_stream_init(_impl, device.id());
   }

   midi_input_stream::~midi_input_stream()
   {
      delete _impl;
   }

   bool midi_input_stream::next(event& ev)
   {
      impl::entry e;
      if (!_impl || !_impl->_queue.pop(e))
         return false;
      ev.msg = e.msg;
      ev.time = e.time;
      for (std::size_t i = 0; i != e.sysex_size; ++i)
         _impl->_bytes.pop(_impl->_sysex[i]);
      ev.sysex = byte_span{_impl->_sysex, e.sysex_size};
      return true;
   }

   void midi_input_stream::set_default_device(int id)
   {
      detail::default_device_id = id;
   }

   ////////////////////////////////////////////////////////////////////////////
   // Output
   ////////////////////////////////////////////////////////////////////////////
   struct midi_output_stream::impl
   {
      libremidi::midi_out     _midi_out;
   };

   midi_output_stream::midi_output_stream(midi_device const& device)
    : _impl{nullptr}
   {
      libremidi::output_port port;
      if (device.protocol() != midi_device::midi_1_0
         || !detail::output_port_of(midi_device::midi_1_0, device.id(), port))
         return;

      auto self = std::make_unique<impl>();
      if (self->_midi_out.open_port(port) != stdx::error{})
         return;
      _impl = self.release();
   }

   midi_output_stream::midi_output_stream(char const* virtual_port)
    : _impl{nullptr}
   {
      if (libremidi::midi1::default_api() == libremidi::API::DUMMY)
         return;
      auto self = std::make_unique<impl>();
      if (self->_midi_out.open_virtual_port(virtual_port) != stdx::error{})
         return;
      _impl = self.release();
   }

   midi_output_stream::~midi_output_stream()
   {
      delete _impl;
   }

   void midi_output_stream::send(byte_span bytes)
   {
      if (_impl)
         _impl->_midi_out.send_message(bytes.data(), bytes.size());
   }
}
