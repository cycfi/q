/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#include <q_io/midi_stream.hpp>
#include <q_io/detail/event_queue.hpp>
#include <q_io/detail/midi_convert.hpp>
#include <libremidi/libremidi.hpp>
#include <memory>

namespace cycfi::q
{
   namespace detail
   {
      int default_device_id = 0;

      bool input_port_of(std::uint32_t id, libremidi::input_port& port);
   }

   // libremidi calls back on its own thread as messages arrive, while the
   // program reads them from its own loop. The queue is what joins the two
   // without either waiting for the other. 1024 events is a few seconds of
   // dense playing, far more than a loop running per audio block needs.
   struct midi_input_stream::impl
   {
      static constexpr std::size_t queue_size = 1024;

      using queue_type = detail::event_queue<event, queue_size>;

      impl()
       : _midi_in{config(&_queue)}
      {}

      // The queue is declared first, so its address is already valid when
      // the configuration below is handed to _midi_in.
      queue_type              _queue;
      libremidi::midi_in      _midi_in;

   private:

      static libremidi::input_configuration config(queue_type* queue)
      {
         libremidi::input_configuration cfg;

         // Runs on the device thread. It packs and enqueues, and does
         // nothing else: no allocation, no lock, no logging.
         cfg.on_message =
            [queue](libremidi::message&& msg)
            {
               midi_1_0::raw_message raw;
               std::span<std::uint8_t const> const bytes{
                  msg.bytes.data(), msg.bytes.size()};
               if (detail::to_raw_message(bytes, raw))
                  queue->push({raw, std::size_t(msg.timestamp)});
            };

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
      return _impl? _impl->_queue.pop(ev) : false;
   }

   void midi_input_stream::set_default_device(int id)
   {
      detail::default_device_id = id;
   }
}
