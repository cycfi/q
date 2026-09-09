/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_MIDI_PACKET_READER_HPP_SEPTEMBER_9_2026)
#define CYCFI_Q_MIDI_PACKET_READER_HPP_SEPTEMBER_9_2026

#include <q/midi/ump_processor.hpp>
#include <q/midi/byte_reader.hpp>
#include <algorithm>
#include <array>
#include <cstdint>
#include <span>

namespace cycfi::q::midi_2_0
{
   ////////////////////////////////////////////////////////////////////////////
   // sysex8_view: a system exclusive message in its 8 bit form, section
   // 4.5, seen in place. Every bit of every byte is data, which the 7 bit
   // form cannot say, and it cannot be sent to a MIDI 1.0 device. The
   // stream id tells interleaved messages apart.
   //
   // Like sysex_view, data() refers to the reader's buffer and is valid
   // only for the duration of the call.
   ////////////////////////////////////////////////////////////////////////////
   struct sysex8_view : message_base
   {
      constexpr sysex8_view(
         std::uint8_t stream, std::span<std::uint8_t const> data)
       : _stream(stream), _data(data)
      {}

      constexpr std::uint8_t     stream() const    { return _stream; }
      constexpr std::span<std::uint8_t const> data() const { return _data; }

   private:

      std::uint8_t                  _stream;
      std::span<std::uint8_t const> _data;
   };

   ////////////////////////////////////////////////////////////////////////////
   // packet_reader: packets in, messages out, system exclusive included.
   //
   //    packet_reader<> reader;
   //    reader(packet, time, proc);
   //
   // dispatch handles a packet that is a message by itself. The two things
   // that are not are the system exclusive forms, sections 4.4 and 4.5: a
   // message is one packet marked complete, or a start, any number of
   // continues, and an end, each carrying up to six bytes in the 7 bit form
   // and thirteen in the 8 bit one. This reader gathers them, and hands the
   // 7 bit form over as the same sysex_view the byte reader gives, since
   // 4.4 says the payload is the same.
   //
   // Section 4.4.1 sets the rules between start and end. A system real
   // time message or a utility message may come between and changes
   // nothing. Any other packet terminates the message in progress, which
   // is then discarded, and the packet itself is read as usual.
   //
   // Capacity bounds a message. One too long is dropped whole and counted,
   // never truncated, for the reason byte_reader gives.
   ////////////////////////////////////////////////////////////////////////////
   template <std::size_t Capacity = 1024>
   class packet_reader
   {
   public:

      static constexpr std::size_t capacity = Capacity;

                              template <typename P>
                              requires concepts::midi_1_0::Processor<P>
      void                    operator()(
                                 packet const& p, std::size_t time, P&& proc);

      std::size_t             drops() const { return _drops; }

   private:

      enum class form : std::uint8_t { none, seven, eight };

      void                    begin(form f, std::uint8_t stream);
      void                    append(std::uint8_t b);
      void                    discard();
                              template <typename P>
      void                    finish(std::size_t time, P&& proc);

                              template <typename P>
      void                    read7(
                                 packet const& p, std::size_t time, P&& proc);
                              template <typename P>
      void                    read8(
                                 packet const& p, std::size_t time, P&& proc);

      form                    _form = form::none;
      bool                    _overflowed = false;
      std::uint8_t            _stream = 0;
      std::size_t             _size = 0;
      std::array<std::uint8_t, Capacity> _buffer = {};
      std::size_t             _drops = 0;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Inline Implementation
   ////////////////////////////////////////////////////////////////////////////
   template <std::size_t Capacity>
   inline void packet_reader<Capacity>::begin(form f, std::uint8_t stream)
   {
      _form = f;
      _stream = stream;
      _size = 0;
      _overflowed = false;
   }

   template <std::size_t Capacity>
   inline void packet_reader<Capacity>::append(std::uint8_t b)
   {
      if (_size == Capacity)
         _overflowed = true;
      else
         _buffer[_size++] = b;
   }

   template <std::size_t Capacity>
   inline void packet_reader<Capacity>::discard()
   {
      if (_form != form::none)
         ++_drops;
      _form = form::none;
      _size = 0;
      _overflowed = false;
   }

   template <std::size_t Capacity>
   template <typename P>
   inline void packet_reader<Capacity>::finish(std::size_t time, P&& proc)
   {
      if (_overflowed)
      {
         ++_drops;
      }
      else
      {
         std::span<std::uint8_t const> const data{_buffer.data(), _size};
         if (_form == form::seven)
            proc(midi_1_0::sysex_view{data}, time);
         else
            proc(sysex8_view{_stream, data}, time);
      }
      _form = form::none;
      _size = 0;
      _overflowed = false;
   }

   template <std::size_t Capacity>
   template <typename P>
   inline void packet_reader<Capacity>::read7(
      packet const& p, std::size_t time, P&& proc)
   {
      // Table 18: status and count share the third byte, then six data
      // bytes follow in order, big end of each word first.
      auto const status = p.status();
      auto const count = std::min<std::size_t>(p.word(0) >> 16 & 0xF, 6);
      std::uint8_t const data[6] =
      {
         std::uint8_t(p.word(0) >> 8), std::uint8_t(p.word(0))
       , std::uint8_t(p.word(1) >> 24), std::uint8_t(p.word(1) >> 16)
       , std::uint8_t(p.word(1) >> 8), std::uint8_t(p.word(1))
      };

      switch (status)
      {
         case 0x0:      // complete in one packet
            discard();
            begin(form::seven, 0);
            for (std::size_t i = 0; i != count; ++i)
               append(data[i]);
            finish(time, proc);
            return;

         case 0x1:      // start
            discard();
            begin(form::seven, 0);
            for (std::size_t i = 0; i != count; ++i)
               append(data[i]);
            return;

         case 0x2:      // continue
            if (_form != form::seven)
               return;
            for (std::size_t i = 0; i != count; ++i)
               append(data[i]);
            return;

         case 0x3:      // end
            if (_form != form::seven)
               return;
            for (std::size_t i = 0; i != count; ++i)
               append(data[i]);
            finish(time, proc);
            return;

         default:
            return;
      }
   }

   template <std::size_t Capacity>
   template <typename P>
   inline void packet_reader<Capacity>::read8(
      packet const& p, std::size_t time, P&& proc)
   {
      // Table 20: status and count in the third byte, the stream id in the
      // fourth, then thirteen data bytes. The count includes the stream id.
      auto const status = p.status();
      auto const stream = std::uint8_t(p.word(0) >> 8);
      auto const declared = std::size_t(p.word(0) >> 16 & 0xF);
      auto const count = declared == 0
         ? 0 : std::min<std::size_t>(declared-1, 13);
      std::uint8_t const data[13] =
      {
         std::uint8_t(p.word(0))
       , std::uint8_t(p.word(1) >> 24), std::uint8_t(p.word(1) >> 16)
       , std::uint8_t(p.word(1) >> 8), std::uint8_t(p.word(1))
       , std::uint8_t(p.word(2) >> 24), std::uint8_t(p.word(2) >> 16)
       , std::uint8_t(p.word(2) >> 8), std::uint8_t(p.word(2))
       , std::uint8_t(p.word(3) >> 24), std::uint8_t(p.word(3) >> 16)
       , std::uint8_t(p.word(3) >> 8), std::uint8_t(p.word(3))
      };

      switch (status)
      {
         case 0x0:
            discard();
            begin(form::eight, stream);
            for (std::size_t i = 0; i != count; ++i)
               append(data[i]);
            finish(time, proc);
            return;

         case 0x1:
            discard();
            begin(form::eight, stream);
            for (std::size_t i = 0; i != count; ++i)
               append(data[i]);
            return;

         case 0x2:
            if (_form != form::eight)
               return;
            for (std::size_t i = 0; i != count; ++i)
               append(data[i]);
            return;

         case 0x3:
            if (_form != form::eight)
               return;
            for (std::size_t i = 0; i != count; ++i)
               append(data[i]);
            finish(time, proc);
            return;

         default:
            return;
      }
   }

   template <std::size_t Capacity>
   template <typename P>
   requires concepts::midi_1_0::Processor<P>
   inline void packet_reader<Capacity>::operator()(
      packet const& p, std::size_t time, P&& proc)
   {
      switch (p.message_type())
      {
         case message_type::data64:
            read7(p, time, proc);
            return;

         case message_type::data128:
            read8(p, time, proc);
            return;

         case message_type::utility:
            // 4.4.1: timestamps and the like sit between packets freely.
            dispatch(p, time, proc);
            return;

         case message_type::system:
            // Real time, 0xF8 and above, is allowed between; system common
            // is not, and ends what was in progress.
            if (p.system_status() < midi_1_0::status::timing_tick)
               discard();
            dispatch(p, time, proc);
            return;

         default:
            discard();
            dispatch(p, time, proc);
            return;
      }
   }
}

#endif
