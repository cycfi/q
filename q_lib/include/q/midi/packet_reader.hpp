/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_MIDI_PACKET_READER_HPP_SEPTEMBER_9_2026)
#define CYCFI_Q_MIDI_PACKET_READER_HPP_SEPTEMBER_9_2026

#include <q/midi/ump_processor.hpp>
#include <q/midi/byte_reader.hpp>
#include <q/midi/ump_stream.hpp>
#include <q/midi/ump_flex.hpp>
#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include <string_view>

namespace cycfi::q::midi_2_0
{
   ////////////////////////////////////////////////////////////////////////////
   // sysex8_view: a system exclusive message in its 8 bit form, section
   // 4.5, seen in place. A payload byte keeps all eight of its bits, where
   // the 7 bit form allows only seven, so it cannot be sent to a MIDI 1.0
   // device. The stream id tells interleaved messages apart.
   //
   // Like sysex_view, data() refers to the reader's buffer and is valid
   // only for the duration of the call.
   ////////////////////////////////////////////////////////////////////////////
   struct sysex8_view : message_base
   {

      constexpr sysex8_view(
         std::uint8_t stream, byte_span data)
       : _stream(stream), _data(data)
      {}

      constexpr std::uint8_t     stream() const    { return _stream; }
      constexpr byte_span        data() const { return _data; }

   private:

      std::uint8_t                  _stream;
      byte_span _data;
   };

   ////////////////////////////////////////////////////////////////////////////
   // The mixed data set, section 4.6: message type 0x5 beside system
   // exclusive in its 8 bit form, carrying arbitrary data, a firmware
   // image or an XML document, with none of the 7 bit restriction.
   //
   // A chunk is one header packet and as many payload packets as its size
   // needs, and a data set is as many chunks. Nothing is gathered here:
   // a data set can run far past the Capacity that bounds a system
   // exclusive message, so each packet is handed over as it arrives and
   // the application assembles it.
   ////////////////////////////////////////////////////////////////////////////
   namespace mds_status
   {
      enum
      {
         header  = 0x8,
         payload = 0x9
      };
   }

   struct mds_message : packet_message<4>
   {
      using packet_message<4>::packet_message;

      // Up to sixteen data sets may be in flight in one group at once.
      constexpr std::uint8_t     mds_id() const
                                 { return (data[0] >> 16) & 0xF; }
   };

   struct mds_header : mds_message
   {
      using mds_message::mds_message;

      // The whole chunk, header included. A chunk that is not a multiple
      // of sixteen is padded with zeros in its last packet.
      constexpr std::uint16_t    valid_bytes() const
                                 { return data[0] & 0xFFFF; }

      // Zero when the sender does not know yet, as for a stream; the last
      // chunk then declares the count it turned out to be.
      constexpr std::uint16_t    chunks() const     { return data[1] >> 16; }
      constexpr std::uint16_t    chunk() const
                                 { return data[1] & 0xFFFF; }

      constexpr std::uint16_t    manufacturer() const
                                 { return data[2] >> 16; }

      // 0xFFFF is the all call, MIDI 1.0's 0x7F.
      constexpr std::uint16_t    device() const     { return data[2] & 0xFFFF; }
      constexpr std::uint16_t    sub_id_1() const   { return data[3] >> 16; }
      constexpr std::uint16_t    sub_id_2() const   { return data[3] & 0xFFFF; }
   };

   struct mds_payload : mds_message
   {
      using mds_message::mds_message;

      // Byte 3 to byte 16 of the packet are all data.
      static constexpr std::size_t size = 14;

      constexpr std::uint8_t     operator[](std::size_t i) const
      {
         auto const shift = i < 2? (8 - i*8) : (24 - ((i-2) % 4) * 8);
         auto const word = i < 2? 0 : 1 + (i-2) / 4;
         return std::uint8_t(data[word] >> shift);
      }

   private:

      // A payload is bytes, not words. operator[] is the way in.
      using mds_message::word;
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
   // The three stream messages that carry text, an endpoint name, a product
   // instance id and a function block name, sections 7.1.4, 7.1.5 and
   // 7.1.9, span packets the same way and are gathered the same way, into
   // a buffer of their own since a name is at most 98 bytes.
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
                              requires concepts::midi::Processor<P>
      void                    operator()(
                                 packet const& p, std::size_t time, P&& proc);

      std::size_t             drops() const { return _drops; }

   private:

      enum class form : std::uint8_t { none, seven, eight };

      static constexpr std::size_t text_capacity = 128;

                              template <typename P>
      void                    read_stream(
                                 packet const& p, std::size_t time, P&& proc);
                              template <typename P>
      void                    finish_text(std::size_t time, P&& proc);
                              template <typename P>
      void                    read_flex(
                                 packet const& p, std::size_t time, P&& proc);
                              template <typename P>
      void                    finish_flex(std::size_t time, P&& proc);

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

      // The text message in progress: its status, the block it names if it
      // is a function block name, and the bytes so far.
      // The flex data text in progress, 7.5.9: twelve bytes to a packet
      // and at most 384 in all.
      static constexpr std::size_t flex_capacity = 384;

      std::uint8_t            _flex_group = 0;
      std::uint8_t            _flex_address = 0;
      std::uint8_t            _flex_channel = 0;
      std::uint8_t            _flex_bank = 0;
      std::uint8_t            _flex_status = 0;
      std::size_t             _flex_size = 0;
      std::array<char, flex_capacity> _flex = {};

      std::uint16_t           _text_status = 0;
      bool                    _in_text = false;
      std::uint8_t            _text_block = 0;
      std::size_t             _text_size = 0;
      std::array<char, text_capacity> _text = {};
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
         byte_span const data{_buffer.data(), _size};
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
   inline void packet_reader<Capacity>::finish_text(
      std::size_t time, P&& proc)
   {
      std::string_view const text{_text.data(), _text_size};
      switch (_text_status)
      {
         case stream_status::endpoint_name:
            proc(endpoint_name_view{text}, time);
            break;
         case stream_status::product_instance_id:
            proc(product_instance_id_view{text}, time);
            break;
         case stream_status::function_block_name:
            proc(function_block_name_view{_text_block, text}, time);
            break;
         default:
            break;
      }
      _in_text = false;
      _text_size = 0;
   }

   template <std::size_t Capacity>
   template <typename P>
   inline void packet_reader<Capacity>::read_stream(
      packet const& p, std::size_t time, P&& proc)
   {
      auto const status = std::uint16_t((p.word(0) >> 16) & 0x3FF);
      auto const is_text =
         status == stream_status::endpoint_name
         || status == stream_status::product_instance_id
         || status == stream_status::function_block_name;

      if (!is_text)
      {
         dispatch(p, time, proc);
         return;
      }

      // Table 33: the text fills the low two bytes of the first word and
      // the three words after, fourteen bytes, or thirteen when the first
      // is a block number. A zero byte ends it early.
      auto const form = std::uint8_t((p.word(0) >> 26) & 0x3);
      auto const named_block = status == stream_status::function_block_name;

      if (form == 0x0 || form == 0x1)
      {
         _in_text = true;
         _text_status = status;
         _text_size = 0;
         if (named_block)
            _text_block = std::uint8_t(p.word(0) >> 8);
      }
      else if (!_in_text || _text_status != status)
      {
         return;                 // a continuation of nothing
      }

      std::uint8_t const bytes[14] =
      {
         std::uint8_t(p.word(0) >> 8), std::uint8_t(p.word(0))
       , std::uint8_t(p.word(1) >> 24), std::uint8_t(p.word(1) >> 16)
       , std::uint8_t(p.word(1) >> 8), std::uint8_t(p.word(1))
       , std::uint8_t(p.word(2) >> 24), std::uint8_t(p.word(2) >> 16)
       , std::uint8_t(p.word(2) >> 8), std::uint8_t(p.word(2))
       , std::uint8_t(p.word(3) >> 24), std::uint8_t(p.word(3) >> 16)
       , std::uint8_t(p.word(3) >> 8), std::uint8_t(p.word(3))
      };

      for (std::size_t i = named_block? 1 : 0; i != 14; ++i)
      {
         if (bytes[i] == 0)
            break;
         if (_text_size < text_capacity)
            _text[_text_size++] = char(bytes[i]);
      }

      if (form == 0x0 || form == 0x3)
         finish_text(time, proc);
   }

   ////////////////////////////////////////////////////////////////////////////
   // 7.5: flex data. The setup bank is one packet and goes straight to
   // dispatch; the two text banks may span packets and are gathered here,
   // twelve bytes at a time, the way the stream text messages are.
   //
   // 7.5.9: "If the text ends in the middle of a UMP, then the remaining
   // data bytes shall be set to 0x00", and that may happen only in a
   // complete or end packet, so the padding is trimmed there.
   ////////////////////////////////////////////////////////////////////////////
   template <std::size_t Capacity>
   template <typename P>
   inline void packet_reader<Capacity>::read_flex(
      packet const& p, std::size_t time, P&& proc)
   {
      auto const bank = std::uint8_t((p.word(0) >> 8) & 0xFF);
      if (bank == flex_bank::setup)
      {
         dispatch(p, time, proc);
         return;
      }

      auto const form = std::uint8_t((p.word(0) >> 22) & 0x3);
      if (form == flex_format::complete || form == flex_format::start)
      {
         _flex_size = 0;
         _flex_group = p.group();
         _flex_address = std::uint8_t((p.word(0) >> 20) & 0x3);
         _flex_channel = std::uint8_t((p.word(0) >> 16) & 0xF);
         _flex_bank = bank;
         _flex_status = std::uint8_t(p.word(0) & 0xFF);
      }
      else if (_flex_size == 0 && _flex_bank == 0 && _flex_status == 0)
      {
         // A continue or an end with no start before it. Nothing to add to.
         return;
      }

      for (std::size_t i = 1; i != 4; ++i)
      {
         for (int shift = 24; shift >= 0; shift -= 8)
         {
            if (_flex_size == flex_capacity)
            {
               ++_drops;
               return;
            }
            _flex[_flex_size++] = char(p.word(i) >> shift);
         }
      }

      if (form == flex_format::complete || form == flex_format::end)
         finish_flex(time, proc);
   }

   template <std::size_t Capacity>
   template <typename P>
   inline void packet_reader<Capacity>::finish_flex(
      std::size_t time, P&& proc)
   {
      auto size = _flex_size;
      while (size != 0 && _flex[size-1] == '\0')
         --size;

      proc(
         flex_text_view{
            _flex_group, _flex_address, _flex_channel, _flex_bank
          , _flex_status, std::string_view{_flex.data(), size}}
       , time);

      _flex_size = 0;
      _flex_bank = 0;
      _flex_status = 0;
   }

   template <std::size_t Capacity>
   template <typename P>
   requires concepts::midi::Processor<P>
   inline void packet_reader<Capacity>::operator()(
      packet const& p, std::size_t time, P&& proc)
   {
      switch (p.message_type())
      {
         case message_type::data64:
            read7(p, time, proc);
            return;

         case message_type::data128:
            // 4.6: a mixed data set shares this type with the 8 bit
            // system exclusive form, and is handed over packet by packet.
            if (p.status() == mds_status::header)
               proc(mds_header{p}, time);
            else if (p.status() == mds_status::payload)
               proc(mds_payload{p}, time);
            else
               read8(p, time, proc);
            return;

         case message_type::flex_data:
            read_flex(p, time, proc);
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

         case message_type::stream:
            // 4.4.1: not real time, so it ends a sysex in progress.
            discard();
            read_stream(p, time, proc);
            return;

         default:
            discard();
            dispatch(p, time, proc);
            return;
      }
   }
}

#endif
