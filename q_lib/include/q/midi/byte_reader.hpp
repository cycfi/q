/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_MIDI_BYTE_READER_HPP_SEPTEMBER_9_2026)
#define CYCFI_Q_MIDI_BYTE_READER_HPP_SEPTEMBER_9_2026

#include <q/midi/processor.hpp>
#include <array>
#include <cstdint>
#include <span>

namespace cycfi::q::midi_1_0
{
   ////////////////////////////////////////////////////////////////////////////
   // sysex_view: a system exclusive message that arrived, seen in place.
   //
   // data() is everything between the markers, beginning with the
   // manufacturer identifier. It refers to the reader's own buffer and is
   // valid only for the duration of the call: a processor that needs to keep
   // a sysex must copy it. To build one to send, see sysex in
   // midi_messages.hpp.
   ////////////////////////////////////////////////////////////////////////////
   struct sysex_view : message_base
   {
      constexpr sysex_view(std::span<std::uint8_t const> data)
       : _data(data)
      {}

      constexpr std::span<std::uint8_t const> data() const { return _data; }

      // One byte, unless it is zero, which introduces a three byte
      // identifier. 0x7E and 0x7F are the universal identifiers, which is
      // how a device is asked what it is.
      constexpr std::uint32_t manufacturer() const
      {
         if (_data.empty())
            return 0;
         if (_data[0] != 0)
            return _data[0];
         if (_data.size() < 3)
            return 0;
         return (std::uint32_t(_data[1]) << 8) | _data[2];
      }

      constexpr bool universal() const
      {
         return !_data.empty() && (_data[0] == 0x7E || _data[0] == 0x7F);
      }

   private:

      std::span<std::uint8_t const> _data;
   };

   ////////////////////////////////////////////////////////////////////////////
   // byte_reader: bytes in, messages out.
   //
   //    byte_reader<> reader;
   //    reader(bytes, time, proc);
   //
   // A device that hands over whole messages needs none of this; a serial
   // port or a raw USB endpoint needs nothing else. Buffers may split
   // anywhere, including inside a message, and every byte handed over gets
   // the time of the buffer it arrived in.
   //
   // The three rules that make a MIDI stream awkward are all here. Running
   // status: a status byte holds until another replaces it, so a chord
   // arrives as one status and a run of data pairs. Real time bytes:
   // 0xF8 and above may appear anywhere, including between the data bytes of
   // another message and inside a sysex, and leave both untouched. And
   // system common: unlike real time, it ends a run of running status.
   //
   // Capacity bounds a sysex. One too long is dropped whole and counted,
   // rather than truncated: a truncated sysex is a different message from
   // the one sent, and acting on it could mean writing the wrong patch to
   // an instrument.
   ////////////////////////////////////////////////////////////////////////////
   template <std::size_t Capacity = 1024>
   class byte_reader
   {
   public:

      static constexpr std::size_t capacity = Capacity;

                              template <typename P>
                              requires concepts::midi_1_0::Processor<P>
      void                    operator()(
                                 std::span<std::uint8_t const> bytes
                               , std::size_t time, P&& proc);

      std::size_t             drops() const { return _drops; }

   private:

                              template <typename P>
      void                    read(std::uint8_t b, std::size_t time, P&& proc);

                              template <typename P>
      void                    end_sysex(std::size_t time, P&& proc);

      static std::size_t      data_length(std::uint8_t status);

      std::uint8_t            _status = 0;         // running status, 0 if none
      std::array<std::uint8_t, 2> _data = {};
      std::size_t             _have = 0;           // data bytes so far
      std::size_t             _want = 0;           // data bytes this message

      bool                    _in_sysex = false;
      bool                    _overflowed = false;
      std::size_t             _size = 0;
      std::array<std::uint8_t, Capacity> _buffer = {};

      std::size_t             _drops = 0;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Inline Implementation
   ////////////////////////////////////////////////////////////////////////////
   template <std::size_t Capacity>
   inline std::size_t byte_reader<Capacity>::data_length(std::uint8_t status)
   {
      if (status < status::sysex)
      {
         switch (status & 0xF0)
         {
            case status::program_change:
            case status::channel_aftertouch:
               return 1;
            default:
               return 2;
         }
      }

      switch (status)
      {
         case status::song_position:   return 2;
         case status::song_select:     return 1;
         default:                      return 0;
      }
   }

   template <std::size_t Capacity>
   template <typename P>
   inline void byte_reader<Capacity>::end_sysex(std::size_t time, P&& proc)
   {
      if (_overflowed)
         ++_drops;
      else
      {
         std::span<std::uint8_t const> const data{_buffer.data(), _size};
         proc(sysex_view{data}, time);
      }

      _in_sysex = false;
      _overflowed = false;
      _size = 0;
   }

   template <std::size_t Capacity>
   template <typename P>
   inline void byte_reader<Capacity>::read(
      std::uint8_t b, std::size_t time, P&& proc)
   {
      // Real time messages are single bytes that may appear anywhere at all.
      // They interrupt nothing: not a message being assembled, not a sysex,
      // not a run of running status.
      if (b >= status::timing_tick)
      {
         dispatch(raw_message{std::uint32_t(b)}, time, proc);
         return;
      }

      if (b & 0x80)
      {
         // Any status byte ends a sysex, whether or not the proper marker
         // came: some devices simply stop sending.
         if (_in_sysex)
            end_sysex(time, proc);

         if (b == status::sysex)
         {
            _in_sysex = true;
            _size = 0;
            _overflowed = false;
            _status = 0;               // a sysex ends a run
            return;
         }

         if (b == status::sysex_end)
            return;                    // the sysex it closed is already sent

         // A new status abandons whatever was half assembled: its missing
         // data byte is never coming.
         _status = b;
         _want = data_length(b);
         _have = 0;

         if (_want == 0)
         {
            dispatch(raw_message{std::uint32_t(b)}, time, proc);
            _status = 0;
         }
         return;
      }

      // A data byte.
      if (_in_sysex)
      {
         if (_size == Capacity)
            _overflowed = true;
         else
            _buffer[_size++] = b;
         return;
      }

      if (_status == 0)
         return;                       // data with no status before it

      _data[_have++] = b;
      if (_have == _want)
      {
         std::uint32_t data = _status;
         data |= std::uint32_t(_data[0]) << 8;
         if (_want > 1)
            data |= std::uint32_t(_data[1]) << 16;

         dispatch(raw_message{data}, time, proc);
         _have = 0;

         // A channel message's status holds for the messages that follow it.
         // A system common one does not: it ends the run.
         if (_status >= status::sysex)
            _status = 0;
      }
   }

   template <std::size_t Capacity>
   template <typename P>
   requires concepts::midi_1_0::Processor<P>
   inline void byte_reader<Capacity>::operator()(
      std::span<std::uint8_t const> bytes, std::size_t time, P&& proc)
   {
      for (auto b : bytes)
         read(b, time, proc);
   }
}

#endif
